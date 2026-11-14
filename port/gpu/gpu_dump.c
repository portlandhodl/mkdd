/* Debug software rasteriser backend: a validation tool, not a renderer.
 *
 * Enabled by MKDD_DUMP_FRAMES=N (no other backend attached): every Nth presented
 * frame is written as a PNG to MKDD_DUMP_DIR (default below). To stay cheap it
 * only rasterises during the few retraces before a dump. Triangles are clipped
 * against the near plane, mapped through the XF viewport, depth tested
 * (BP 0x40) and shaded with vertex color 0 modulated by texture map 0 of TEV
 * stage 0 (decoded with texdec), with a rough alpha test (BP 0xF3), blending
 * (BP 0x41) and scissor (BP 0x20/0x21). TEV combiners, fog, indirect textures
 * and EFB-copy textures are not emulated. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <emscripten.h>
#include "gpu_internal.h"
#include "texdec.h"

#define EFB_W 640
#define EFB_H 528
#define DEFAULT_DIR "build/port/frames"

EM_JS(char *, gpu_js_env_str, (const char *name), {
    var n = UTF8ToString(name);
    var v = (typeof process !== 'undefined' && process.env) ? process.env[n] : undefined;
    if (v === undefined && typeof Module !== 'undefined' && Module.env) v = Module.env[n];
    return v ? stringToNewUTF8(String(v)) : 0;
});

int port_env(const char *name);

static uint32_t sColor[EFB_W * EFB_H]; /* RGBA8, byte order R,G,B,A in memory */
static float sDepth[EFB_W * EFB_H];

typedef struct Xfb {
    uint32_t addr, w, h, clean, serial;
    uint32_t *pix;
} Xfb;
static Xfb sXfb[4];
static uint32_t sSerial;

static int sEvery;
static char sDir[512];
static uint32_t sPresents;
static int sActive;     /* rasterising this retrace */
static int sEfbClean;   /* EFB cleared since rasterising started */
static uint32_t sDumped;
static int sMode;       /* MKDD_DUMP_MODE: 0 shaded, 1 depth view, 2 draw-id colors */
static uint32_t sDrawId;
static uint32_t *sSnap[16]; /* trace: EFB at each copy of the frame */
static int sSnapN, sSnapKeep;
static int sTrace;      /* MKDD_DUMP_TRACE: write a per-draw log next to each dump */
static char *sLog;
static size_t sLogLen, sLogCap;

static void logf_(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void logf_(const char *fmt, ...)
{
    va_list ap;
    int n;
    if (!sTrace)
        return;
    if (sLogCap - sLogLen < 512) {
        size_t cap = sLogCap ? sLogCap * 2 : 1 << 20;
        char *nb = (char *)realloc(sLog, cap);
        if (!nb)
            return;
        sLog = nb;
        sLogCap = cap;
    }
    va_start(ap, fmt);
    n = vsnprintf(sLog + sLogLen, sLogCap - sLogLen, fmt, ap);
    va_end(ap);
    if (n > 0)
        sLogLen += (size_t)n < sLogCap - sLogLen ? (size_t)n : sLogCap - sLogLen - 1;
}

/* ---- texture cache (cleared whenever a rasterising window starts) ---- */

typedef struct Tex {
    uint32_t key[6];
    int w, h;
    uint8_t *rgba;
} Tex;
/* EFB copies to textures, kept as RGBA and sampled in place of RAM contents */
typedef struct Copy {
    uint32_t addr, serial;
    Tex tex;
} Copy;
static Copy sCopies[16];
static uint32_t sCopySerial;

#define TEX_CACHE 512
static Tex sTex[TEX_CACHE];
static int sTexN;

static void tex_flush(void)
{
    int i;
    for (i = 0; i < sTexN; i++)
        free(sTex[i].rgba);
    sTexN = 0;
}

static const Tex *tex_get(int map)
{
    static const uint8_t base[8] = {0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3};
    uint32_t r0 = base[map];
    uint32_t img0 = gpu_r.bp[r0 + 0x08], img3 = gpu_r.bp[r0 + 0x14], tl = gpu_r.bp[r0 + 0x18];
    uint32_t key[6];
    int w = (img0 & 0x3FF) + 1, h = ((img0 >> 10) & 0x3FF) + 1, fmt = (img0 >> 20) & 15;
    uint32_t addr = (img3 & 0x1FFFFF) << 5, size;
    int i;
    Tex *t;

    for (i = 0; i < 16; i++)
        if (sCopies[i].tex.rgba && sCopies[i].addr == addr)
            return &sCopies[i].tex;
    key[0] = addr; key[1] = img0; key[2] = tl; key[3] = 0; key[4] = 0; key[5] = 0;
    for (i = 0; i < sTexN; i++)
        if (!memcmp(sTex[i].key, key, sizeof key))
            return sTex[i].rgba ? &sTex[i] : NULL;
    if (sTexN == TEX_CACHE)
        tex_flush();
    t = &sTex[sTexN++];
    memcpy(t->key, key, sizeof key);
    t->w = w;
    t->h = h;
    t->rgba = NULL;
    size = texdec_size(fmt, w, h);
    if (!size || !gpu_ram_ok(addr, size))
        return NULL;
    t->rgba = (uint8_t *)malloc((size_t)w * h * 4);
    if (!t->rgba)
        return NULL;
    {
        uint32_t tlutOff = 0x80000 + ((tl & 0x3FF) << 9);
        int count = tlutOff < (1u << 20) ? (int)(((1u << 20) - tlutOff) / 2) : 0;
        if (count > 16384)
            count = 16384;
        if (texdec_decode(t->rgba, (const uint8_t *)(uintptr_t)addr, fmt, w, h,
                          gpu_tmem() + (tlutOff & 0xFFFFF), (tl >> 10) & 3, count) != 0) {
            free(t->rgba);
            t->rgba = NULL;
            return NULL;
        }
    }
    return t;
}

static inline int wrap(int v, int n, int mode)
{
    if (mode == 1) { /* repeat */
        v %= n;
        return v < 0 ? v + n : v;
    }
    if (mode == 2) { /* mirror */
        int p = n * 2;
        v %= p;
        if (v < 0)
            v += p;
        return v < n ? v : p - 1 - v;
    }
    return v < 0 ? 0 : v >= n ? n - 1 : v;
}

/* ---- rasteriser ---- */

typedef struct RVtx {
    float x, y, z, iw;   /* window x/y, depth 0..1, 1/w */
    float c[4], s, t, q; /* attributes divided by w */
} RVtx;

typedef struct DrawState {
    int depthTest, depthUpdate, depthFunc;
    int colorUpdate, alphaUpdate, blend, sub, srcF, dstF;
    int alphaComp[2], alphaRef[2], alphaOp, alphaTest;
    int sx0, sy0, sx1, sy1;
    const Tex *tex;
    int wrapS, wrapT;
    int texCoord;
    int useColor;
    int zOp, zType;
    uint32_t zBias;
    float vpx, vpy, vsx, vsy, vsz, voz;
} DrawState;

static int cmp(int func, float a, float b)
{
    switch (func) {
    case 0: return 0;
    case 1: return a < b;
    case 2: return a == b;
    case 3: return a <= b;
    case 4: return a > b;
    case 5: return a != b;
    case 6: return a >= b;
    default: return 1;
    }
}

static float blend_factor(int f, int src, const float s[4], const float d[4], int ch)
{
    switch (f) {
    case 0: return 0;
    case 1: return 1;
    case 2: return src ? d[ch] : s[ch];             /* dst color (src side) / src color (dst side) */
    case 3: return 1 - (src ? d[ch] : s[ch]);
    case 4: return s[3];
    case 5: return 1 - s[3];
    case 6: return d[3];
    default: return 1 - d[3];
    }
}

static void setup(DrawState *d)
{
    uint32_t z = gpu_r.bp[0x40], cm = gpu_r.bp[0x41], at = gpu_r.bp[0xF3];
    uint32_t tref = gpu_r.bp[0x28];
    uint32_t sc0 = gpu_r.bp[0x20], sc1 = gpu_r.bp[0x21];
    d->depthTest = z & 1;
    d->depthFunc = (z >> 1) & 7;
    d->depthUpdate = (z >> 4) & 1;
    d->blend = cm & 1;
    d->colorUpdate = (cm >> 3) & 1;
    d->alphaUpdate = (cm >> 4) & 1;
    d->dstF = (cm >> 5) & 7;
    d->srcF = (cm >> 8) & 7;
    d->sub = (cm >> 11) & 1;
    d->alphaRef[0] = at & 255;
    d->alphaRef[1] = (at >> 8) & 255;
    d->alphaComp[0] = (at >> 16) & 7;
    d->alphaComp[1] = (at >> 19) & 7;
    d->alphaOp = (at >> 22) & 3;
    d->alphaTest = !(d->alphaComp[0] == 7 && d->alphaComp[1] == 7);
    d->sy0 = (int)(sc0 & 0x7FF) - 342;
    d->sx0 = (int)((sc0 >> 12) & 0x7FF) - 342;
    d->sy1 = (int)(sc1 & 0x7FF) - 342;
    d->sx1 = (int)((sc1 >> 12) & 0x7FF) - 342;
    if (d->sx0 < 0) d->sx0 = 0;
    if (d->sy0 < 0) d->sy0 = 0;
    if (d->sx1 > EFB_W - 1) d->sx1 = EFB_W - 1;
    if (d->sy1 > EFB_H - 1) d->sy1 = EFB_H - 1;
    d->tex = NULL;
    if (tref & 0x40) { /* stage 0 texture enabled */
        int map = tref & 7;
        uint32_t mode0 = gpu_r.bp[map < 4 ? 0x80 + map : 0xA0 + (map - 4)];
        d->tex = tex_get(map);
        d->wrapS = mode0 & 3;
        d->wrapT = (mode0 >> 2) & 3;
        d->texCoord = (tref >> 3) & 7;
    }
    d->useColor = ((gpu_r.bp[0x00] >> 4) & 7) != 0 || !d->tex; /* GEN_MODE: color channels */
    d->zOp = d->tex ? (gpu_r.bp[0xF5] >> 2) & 3 : 0;
    d->zType = gpu_r.bp[0xF5] & 3;
    d->zBias = gpu_r.bp[0xF4] & 0xFFFFFF;
    d->vsx = gpu_xf_float(0x1A);
    d->vsy = gpu_xf_float(0x1B);
    d->vsz = gpu_xf_float(0x1C);
    d->vpx = gpu_xf_float(0x1D) - 342;
    d->vpy = gpu_xf_float(0x1E) - 342;
    d->voz = gpu_xf_float(0x1F);
}

static void to_window(const DrawState *d, const GpuVertex *v, RVtx *r)
{
    float iw = 1.0f / v->pos[3];
    float zgx = (v->pos[2] * iw - 1.0f) * 0.5f; /* back to GX NDC z in [-1, 0] */
    int i;
    r->x = d->vpx + d->vsx * v->pos[0] * iw;
    r->y = d->vpy + d->vsy * v->pos[1] * iw;
    r->z = (d->voz + d->vsz * zgx) / 16777215.0f;
    r->iw = iw;
    for (i = 0; i < 4; i++)
        r->c[i] = v->color[0][i] * iw;
    if (d->tex) {
        const float *t = v->tex[d->texCoord];
        r->s = t[0] * iw;
        r->t = t[1] * iw;
        r->q = t[2] * iw;
    }
}

static void shade_pixel(const DrawState *d, int x, int y, float z, const float col[4], float s, float t, float q)
{
    uint32_t idx = (uint32_t)y * EFB_W + x;
    float src[4], dst[4], outc[4];
    uint8_t *px;
    int i;

    if (z < 0 || z > 1)
        return;
    if (d->zOp == 1 || d->zOp == 2) { /* z texture (e.g. clearing the EFB with a quad) */
        float iq = q != 0 ? 1.0f / q : 0;
        int tx = wrap((int)floorf(s * iq * d->tex->w), d->tex->w, d->wrapS);
        int ty = wrap((int)floorf(t * iq * d->tex->h), d->tex->h, d->wrapT);
        const uint8_t *tp = d->tex->rgba + ((size_t)ty * d->tex->w + tx) * 4;
        uint32_t zt = d->zType == 0 ? tp[0] : d->zType == 1 ? (uint32_t)tp[3] << 8 | tp[0] : (uint32_t)tp[0] << 16 | (uint32_t)tp[1] << 8 | tp[2];
        float zz = (float)((zt + d->zBias) & 0xFFFFFF) / 16777215.0f;
        z = d->zOp == 2 ? zz : fminf(z + zz, 1.0f);
    }
    if (d->depthTest && !cmp(d->depthFunc, z, sDepth[idx]))
        return;
    if (sMode) {
        if (sMode == 1) {
            float g = 1.0f - z;
            g = g < 0 ? 0 : g * 8.0f > 1 ? 1 : g * 8.0f;
            src[0] = src[1] = src[2] = g;
        } else {
            uint32_t h = sDrawId * 2654435761u;
            src[0] = (h >> 24) / 255.0f; src[1] = ((h >> 16) & 255) / 255.0f; src[2] = ((h >> 8) & 255) / 255.0f;
        }
        src[3] = 1;
        if (!d->colorUpdate)
            return;
        if (!d->depthTest || cmp(3, z, sDepth[idx])) {
            uint8_t *q8 = (uint8_t *)&sColor[idx];
            sDepth[idx] = z;
            for (i = 0; i < 4; i++)
                q8[i] = (uint8_t)(src[i] * 255.0f);
        }
        return;
    }
    if (d->useColor)
        memcpy(src, col, sizeof src);
    else
        src[0] = src[1] = src[2] = src[3] = 1;
    if (d->tex) {
        float iq = q != 0 ? 1.0f / q : 0;
        int tx = wrap((int)floorf(s * iq * d->tex->w), d->tex->w, d->wrapS);
        int ty = wrap((int)floorf(t * iq * d->tex->h), d->tex->h, d->wrapT);
        const uint8_t *tp = d->tex->rgba + ((size_t)ty * d->tex->w + tx) * 4;
        for (i = 0; i < 4; i++)
            src[i] *= tp[i] / 255.0f;
    }
    if (d->alphaTest) {
        int a = (int)(src[3] * 255.0f + 0.5f);
        int r0 = cmp(d->alphaComp[0], (float)a, (float)d->alphaRef[0]);
        int r1 = cmp(d->alphaComp[1], (float)a, (float)d->alphaRef[1]);
        int pass = d->alphaOp == 0 ? (r0 && r1) : d->alphaOp == 1 ? (r0 || r1) : d->alphaOp == 2 ? (r0 != r1) : (r0 == r1);
        if (!pass)
            return;
    }
    if (d->depthTest && d->depthUpdate)
        sDepth[idx] = z;
    if (!d->colorUpdate && !d->alphaUpdate)
        return;
    px = (uint8_t *)&sColor[idx];
    for (i = 0; i < 4; i++)
        dst[i] = px[i] / 255.0f;
    if (d->blend) {
        for (i = 0; i < 4; i++) {
            if (d->sub)
                outc[i] = dst[i] - src[i];
            else
                outc[i] = src[i] * blend_factor(d->srcF, 1, src, dst, i) + dst[i] * blend_factor(d->dstF, 0, src, dst, i);
        }
    } else {
        memcpy(outc, src, sizeof outc);
    }
    for (i = 0; i < 4; i++) {
        float v = outc[i] < 0 ? 0 : outc[i] > 1 ? 1 : outc[i];
        if ((i < 3 && d->colorUpdate) || (i == 3 && d->alphaUpdate))
            px[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
}

static void raster_tri(const DrawState *d, const RVtx *a, const RVtx *b, const RVtx *c)
{
    float area = (b->x - a->x) * (c->y - a->y) - (c->x - a->x) * (b->y - a->y);
    int x0, x1, y0, y1, x, y;
    float inv;
    if (area == 0 || !isfinite(area))
        return;
    inv = 1.0f / area;
    x0 = (int)floorf(fminf(a->x, fminf(b->x, c->x)));
    x1 = (int)ceilf(fmaxf(a->x, fmaxf(b->x, c->x)));
    y0 = (int)floorf(fminf(a->y, fminf(b->y, c->y)));
    y1 = (int)ceilf(fmaxf(a->y, fmaxf(b->y, c->y)));
    if (x0 < d->sx0) x0 = d->sx0;
    if (y0 < d->sy0) y0 = d->sy0;
    if (x1 > d->sx1) x1 = d->sx1;
    if (y1 > d->sy1) y1 = d->sy1;
    for (y = y0; y <= y1; y++) {
        float py = y + 0.5f;
        for (x = x0; x <= x1; x++) {
            float px = x + 0.5f;
            float w0 = ((b->x - px) * (c->y - py) - (c->x - px) * (b->y - py)) * inv;
            float w1 = ((c->x - px) * (a->y - py) - (a->x - px) * (c->y - py)) * inv;
            float w2 = 1.0f - w0 - w1;
            float iw, z, col[4], s = 0, t = 0, q = 1;
            int i;
            if (w0 < 0 || w1 < 0 || w2 < 0)
                continue;
            iw = w0 * a->iw + w1 * b->iw + w2 * c->iw;
            if (iw <= 0)
                continue;
            z = w0 * a->z + w1 * b->z + w2 * c->z;
            for (i = 0; i < 4; i++)
                col[i] = (w0 * a->c[i] + w1 * b->c[i] + w2 * c->c[i]) / iw;
            if (d->tex) {
                s = (w0 * a->s + w1 * b->s + w2 * c->s) / iw;
                t = (w0 * a->t + w1 * b->t + w2 * c->t) / iw;
                q = (w0 * a->q + w1 * b->q + w2 * c->q) / iw;
            }
            shade_pixel(d, x, y, z, col, s, t, q);
        }
    }
}

static void lerp_vtx(const GpuVertex *a, const GpuVertex *b, float t, GpuVertex *o)
{
    const float *pa = (const float *)a, *pb = (const float *)b;
    float *po = (float *)o;
    size_t i, n = sizeof(GpuVertex) / sizeof(float);
    for (i = 0; i < n; i++)
        po[i] = pa[i] + (pb[i] - pa[i]) * t;
}

/* Clip a triangle against the near plane (z_gl >= -w) and w > epsilon. */
static int clip_near(const GpuVertex *in, GpuVertex *out)
{
    int n = 0, i;
    for (i = 0; i < 3; i++) {
        const GpuVertex *a = &in[i], *b = &in[(i + 1) % 3];
        float da = a->pos[2] + a->pos[3], db = b->pos[2] + b->pos[3];
        if (a->pos[3] < 1e-6f && da >= 0)
            da = -1;
        if (da >= 0)
            out[n++] = *a;
        if ((da >= 0) != (db >= 0) && da != db)
            lerp_vtx(a, b, da / (da - db), &out[n++]);
    }
    return n;
}

static void dump_draw(void *ctx, const GpuRegs *regs, int prim, const GpuVertex *v, uint32_t count)
{
    DrawState d;
    uint32_t i;
    (void)ctx; (void)regs;
    if (!sActive)
        return;
    sDrawId++;
    setup(&d);
    if (sTrace) {
        uint32_t img0 = d.tex ? gpu_r.bp[(gpu_r.bp[0x28] & 7) < 4 ? 0x88 + (gpu_r.bp[0x28] & 7) : 0xA8 + (gpu_r.bp[0x28] & 7) - 4] : 0;
        logf_("draw %u prim %d n %u z %06x cm %06x at %06x gen %06x tref %06x img0 %06x tev0 %06x/%06x v0 (%.3g %.3g %.3g %.3g) c0 (%.2f %.2f %.2f %.2f) t0 (%.3g %.3g %.3g)\n",
              sDrawId, prim, count, gpu_r.bp[0x40], gpu_r.bp[0x41], gpu_r.bp[0xF3], gpu_r.bp[0x00], gpu_r.bp[0x28], img0,
              gpu_r.bp[0xC0], gpu_r.bp[0xC1], v[0].pos[0], v[0].pos[1], v[0].pos[2], v[0].pos[3],
              v[0].color[0][0], v[0].color[0][1], v[0].color[0][2], v[0].color[0][3], v[0].tex[0][0], v[0].tex[0][1], v[0].tex[0][2]);
        logf_("    proj %g %g %g %g %g %g type %u vp %g %g %g %g %g %g\n", gpu_xf_float(0x20), gpu_xf_float(0x21),
              gpu_xf_float(0x22), gpu_xf_float(0x23), gpu_xf_float(0x24), gpu_xf_float(0x25), gpu_r.xf[0x26],
              d.vsx, d.vsy, d.vsz, d.vpx, d.vpy, d.voz);
    }
    if (prim == GPU_TRIANGLES) {
        for (i = 0; i + 2 < count; i += 3) {
            GpuVertex poly[4];
            RVtx r[4];
            int n = clip_near(&v[i], poly), k;
            if (n < 3)
                continue;
            for (k = 0; k < n; k++)
                to_window(&d, &poly[k], &r[k]);
            raster_tri(&d, &r[0], &r[1], &r[2]);
            if (n == 4)
                raster_tri(&d, &r[0], &r[2], &r[3]);
        }
    } else {
        /* lines and points: plot vertices / step along the segment */
        uint32_t step = prim == GPU_LINES ? 2 : 1;
        for (i = 0; i + step - 1 < count; i += step) {
            RVtx a, b;
            int k, n;
            if (v[i].pos[3] <= 0 || (step == 2 && v[i + 1].pos[3] <= 0))
                continue;
            to_window(&d, &v[i], &a);
            b = a;
            if (step == 2)
                to_window(&d, &v[i + 1], &b);
            n = (int)fmaxf(fabsf(b.x - a.x), fabsf(b.y - a.y)) + 1;
            if (n > 2048)
                n = 2048;
            for (k = 0; k < n; k++) {
                float t = n > 1 ? (float)k / (n - 1) : 0;
                int x = (int)(a.x + (b.x - a.x) * t), y = (int)(a.y + (b.y - a.y) * t);
                float col[4];
                int c;
                if (x < d.sx0 || x > d.sx1 || y < d.sy0 || y > d.sy1)
                    continue;
                for (c = 0; c < 4; c++)
                    col[c] = (a.c[c] / a.iw) * (1 - t) + (b.c[c] / b.iw) * t;
                shade_pixel(&d, x, y, a.z + (b.z - a.z) * t, col, a.s / a.iw, a.t / a.iw, a.q / a.iw);
            }
        }
    }
}

static void dump_copy(void *ctx, const GpuRegs *regs, int toXfb, int clear)
{
    uint32_t src = regs->bp[0x49], size = regs->bp[0x4A];
    int x0 = src & 0x3FF, y0 = (src >> 10) & 0x3FF;
    int w = (size & 0x3FF) + 1, h = ((size >> 10) & 0x3FF) + 1;
    int x, y;
    (void)ctx;
    if (!sActive)
        return;
    if (x0 + w > EFB_W) w = EFB_W - x0;
    if (y0 + h > EFB_H) h = EFB_H - y0;
    if (w <= 0 || h <= 0)
        return;
    if (toXfb) {
        uint32_t addr = (regs->bp[0x4B] & 0x1FFFFF) << 5;
        Xfb *slot = &sXfb[0];
        int i;
        for (i = 0; i < 4; i++)
            if (sXfb[i].addr == addr) {
                slot = &sXfb[i];
                break;
            }
        if (i == 4)
            for (i = 1; i < 4; i++)
                if (sXfb[i].serial < slot->serial)
                    slot = &sXfb[i];
        if (!slot->pix)
            slot->pix = (uint32_t *)malloc(EFB_W * EFB_H * 4);
        if (slot->pix) {
            slot->addr = addr;
            slot->w = w;
            slot->h = h;
            slot->clean = sEfbClean;
            slot->serial = ++sSerial;
            for (y = 0; y < h; y++)
                memcpy(slot->pix + y * w, sColor + (y0 + y) * EFB_W + x0, (size_t)w * 4);
        }
    }
    if (!toXfb) {
        uint32_t addr = (regs->bp[0x4B] & 0x1FFFFF) << 5, i;
        int half = (regs->bp[0x52] >> 9) & 1, ow = half ? w / 2 : w, oh = half ? h / 2 : h;
        Copy *c = &sCopies[0];
        for (i = 0; i < 16; i++)
            if (sCopies[i].addr == addr) {
                c = &sCopies[i];
                break;
            }
        if (i == 16)
            for (i = 1; i < 16; i++)
                if (sCopies[i].serial < c->serial)
                    c = &sCopies[i];
        if (ow > 0 && oh > 0) {
            if (c->tex.rgba && (c->tex.w != ow || c->tex.h != oh)) {
                free(c->tex.rgba);
                c->tex.rgba = NULL;
            }
            if (!c->tex.rgba)
                c->tex.rgba = (uint8_t *)malloc((size_t)ow * oh * 4);
            if (c->tex.rgba) {
                c->addr = addr;
                c->serial = ++sCopySerial;
                c->tex.w = ow;
                c->tex.h = oh;
                for (y = 0; y < oh; y++)
                    for (x = 0; x < ow; x++) {
                        uint8_t *o = c->tex.rgba + ((size_t)y * ow + x) * 4;
                        if (half) {
                            int k, sum[4] = {0, 0, 0, 0}, dx, dy;
                            for (dy = 0; dy < 2; dy++)
                                for (dx = 0; dx < 2; dx++) {
                                    const uint8_t *p = (const uint8_t *)&sColor[(y0 + y * 2 + dy) * EFB_W + x0 + x * 2 + dx];
                                    for (k = 0; k < 4; k++)
                                        sum[k] += p[k];
                                }
                            for (k = 0; k < 4; k++)
                                o[k] = (uint8_t)(sum[k] / 4);
                        } else {
                            memcpy(o, &sColor[(y0 + y) * EFB_W + x0 + x], 4);
                        }
                    }
            }
        }
    }
    if (clear) {
        uint32_t ar = regs->bp[0x4F], gb = regs->bp[0x50];
        uint8_t c[4] = {(uint8_t)(ar & 255), (uint8_t)(gb >> 8), (uint8_t)(gb & 255), (uint8_t)(ar >> 8)};
        uint32_t cv;
        float z = (regs->bp[0x51] & 0xFFFFFF) / 16777215.0f;
        memcpy(&cv, c, 4);
        for (y = y0; y < y0 + h; y++)
            for (x = x0; x < x0 + w; x++) {
                sColor[y * EFB_W + x] = cv;
                sDepth[y * EFB_W + x] = z;
            }
        sEfbClean = 1;
    }
    logf_("copy %d toXfb %d clear %d src %06x size %06x dst %08x cp %06x\n", sSnapN, toXfb, clear, src, size,
          (regs->bp[0x4B] & 0x1FFFFF) << 5, regs->bp[0x52]);
    if (sTrace && sSnapN < 16) {
        if (!sSnap[sSnapN])
            sSnap[sSnapN] = (uint32_t *)malloc(sizeof sColor);
        if (sSnap[sSnapN])
            memcpy(sSnap[sSnapN++], sColor, sizeof sColor);
    }
    if (clear && toXfb) {
        sSnapKeep = sSnapN;
        sSnapN = 0;
        sLogLen = 0;
    }
}

/* ---- PNG writer (stored deflate blocks) ---- */

static uint32_t sCrcTab[256];

static uint32_t crc(uint32_t c, const uint8_t *p, size_t n)
{
    size_t i;
    if (!sCrcTab[1]) {
        uint32_t k, j;
        for (k = 0; k < 256; k++) {
            uint32_t v = k;
            for (j = 0; j < 8; j++)
                v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            sCrcTab[k] = v;
        }
    }
    c = ~c;
    for (i = 0; i < n; i++)
        c = sCrcTab[(c ^ p[i]) & 255] ^ (c >> 8);
    return ~c;
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t hdr[8];
    uint32_t c;
    put32(hdr, len);
    memcpy(hdr + 4, type, 4);
    fwrite(hdr, 1, 8, f);
    if (len)
        fwrite(data, 1, len, f);
    c = crc(0, hdr + 4, 4);
    c = crc(c, data, len);
    put32(hdr, c);
    fwrite(hdr, 1, 4, f);
}

static int write_png(const char *path, const uint32_t *pix, int w, int h)
{
    size_t raw = (size_t)h * (1 + (size_t)w * 3), nblk = (raw + 65534) / 65535;
    size_t zlen = 2 + raw + nblk * 5 + 4, off = 0, i;
    uint8_t *rows = (uint8_t *)malloc(raw), *z = (uint8_t *)malloc(zlen), ihdr[13];
    uint32_t s1 = 1, s2 = 0;
    FILE *f;
    int x, y;
    if (!rows || !z) {
        free(rows);
        free(z);
        return -1;
    }
    for (y = 0; y < h; y++) {
        uint8_t *r = rows + (size_t)y * (1 + w * 3);
        r[0] = 0;
        for (x = 0; x < w; x++) {
            const uint8_t *p = (const uint8_t *)&pix[y * w + x];
            r[1 + x * 3] = p[0];
            r[2 + x * 3] = p[1];
            r[3 + x * 3] = p[2];
        }
    }
    z[off++] = 0x78;
    z[off++] = 0x01;
    for (i = 0; i < raw; i += 65535) {
        size_t n = raw - i < 65535 ? raw - i : 65535;
        z[off++] = i + n == raw;
        z[off++] = (uint8_t)n;
        z[off++] = (uint8_t)(n >> 8);
        z[off++] = (uint8_t)~n;
        z[off++] = (uint8_t)(~n >> 8);
        memcpy(z + off, rows + i, n);
        off += n;
    }
    for (i = 0; i < raw; i++) {
        s1 = (s1 + rows[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    put32(z + off, s2 << 16 | s1);
    off += 4;
    f = fopen(path, "wb");
    if (!f) {
        free(rows);
        free(z);
        return -1;
    }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    put32(ihdr, (uint32_t)w);
    put32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, (uint32_t)off);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(rows);
    free(z);
    return 0;
}

static void mkdirs(const char *dir)
{
    char tmp[512];
    size_t i, n = strlen(dir);
    if (n >= sizeof tmp)
        return;
    memcpy(tmp, dir, n + 1);
    for (i = 1; i <= n; i++)
        if (tmp[i] == '/' || tmp[i] == 0) {
            char c = tmp[i];
            tmp[i] = 0;
            mkdir(tmp, 0755);
            tmp[i] = c;
        }
}

/* Rasterise during the retraces leading up to a dump (enough to cover a whole
 * frame of a 30 fps game plus the one still being drawn when the window opens). */
#define WINDOW 6

static void dump_present(void *ctx, uint32_t xfbAddr)
{
    int wasActive = sActive;
    (void)ctx;
    sPresents++;
    if (sPresents % (uint32_t)sEvery == 0) {
        const Xfb *best = NULL;
        int i;
        for (i = 0; i < 4; i++) {
            const Xfb *x = &sXfb[i];
            if (!x->pix || !x->clean)
                continue;
            if (x->addr == xfbAddr && (!best || best->addr != xfbAddr || x->serial > best->serial))
                best = x;
            else if (!best || (best->addr != xfbAddr && x->serial > best->serial))
                best = x;
        }
        if (best) {
            char path[640];
            snprintf(path, sizeof path, "%s/frame_%06u.png", sDir, sPresents);
            if (sTrace && sLog) {
                char lp[660];
                FILE *lf;
                int k;
                for (k = 0; k < sSnapN; k++) {
                    snprintf(lp, sizeof lp, "%s/frame_%06u_copy%d.png", sDir, sPresents, k);
                    write_png(lp, sSnap[k], EFB_W, 480);
                }
                snprintf(lp, sizeof lp, "%s/frame_%06u.txt", sDir, sPresents);
                if ((lf = fopen(lp, "w")) != NULL) {
                    fwrite(sLog, 1, sLogLen, lf);
                    fclose(lf);
                }
            }
            if (write_png(path, best->pix, (int)best->w, (int)best->h) == 0) {
                if (sDumped++ < 3 || sDumped % 50 == 0)
                    fprintf(stderr, "[gpu] dumped %s (%ux%u)\n", path, best->w, best->h);
            }
        }
    }
    {
        uint32_t toNext = (uint32_t)sEvery - sPresents % (uint32_t)sEvery; /* presents until the next dump */
        sActive = sEvery <= WINDOW || toNext <= WINDOW;
    }
    gpu_xform_enabled = sActive;
    if (sActive && !wasActive) {
        uint32_t i;
        sEfbClean = 0;
        for (i = 0; i < 4; i++)
            sXfb[i].clean = 0;
        tex_flush();
    }
    if (sActive && sEvery <= WINDOW && sPresents % 64 == 0)
        tex_flush(); /* always rasterising: textures may change underneath */
}

static const GpuBackend sDumpBackend = {dump_draw, dump_copy, dump_present, NULL};

const GpuBackend *gpu_dump_backend(void)
{
    char *dir;
    sEvery = port_env("MKDD_DUMP_FRAMES");
    sMode = port_env("MKDD_DUMP_MODE");
    sTrace = port_env("MKDD_DUMP_TRACE");
    if (sEvery <= 0)
        return NULL;
    dir = gpu_js_env_str("MKDD_DUMP_DIR");
    snprintf(sDir, sizeof sDir, "%s", dir ? dir : DEFAULT_DIR);
    free(dir);
    mkdirs(sDir);
    sActive = sEvery <= WINDOW;
    fprintf(stderr, "[gpu] debug rasteriser: dumping every %d frames to %s\n", sEvery, sDir);
    return &sDumpBackend;
}
