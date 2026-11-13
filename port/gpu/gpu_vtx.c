/* GPU front end, part 2: vertex loader, transform/lighting/texgen (XF) and
 * primitive assembly.
 *
 * Vertex data in the stream (DIRECT attributes) is big-endian. INDEXED
 * attributes are fetched from arrays in RAM (CP 0xA0 + n base, 0xB0 + n
 * stride), which the port keeps in HOST byte order: each component is read at
 * its own width (u16/s16/f32 swapped, bytes as they are; 16-bit colors as one
 * host u16, 24/32-bit colors as bytes). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu_internal.h"

enum { VA_NONE = 0, VA_DIRECT = 1, VA_INDEX8 = 2, VA_INDEX16 = 3 };

typedef struct Attr {
    uint8_t mode;   /* VA_* */
    uint8_t comps;  /* components */
    uint8_t type;   /* 0 u8, 1 s8, 2 u16, 3 s16, 4 f32; colors: GX color format */
    uint8_t csize;  /* bytes per component (colors: bytes per element) */
    float scale;    /* 1 / (1 << frac) */
} Attr;

typedef struct VtxFmt {
    uint32_t size;
    uint8_t pnIdx, texIdx[8];   /* direct matrix index bytes present */
    uint8_t nbt, nrmIdx3;
    Attr pos, nrm, col[2], tex[8];
} VtxFmt;

static VtxFmt sFmt[8];
static uint8_t sFmtValid;

void gpu_vtx_invalidate(void) { sFmtValid = 0; }

static const uint8_t kCompSize[8] = {1, 1, 2, 2, 4, 4, 4, 4};
static const uint8_t kColorSize[8] = {2, 3, 4, 2, 3, 4, 4, 4};

static uint32_t attr_bytes(const Attr *a, int indices)
{
    switch (a->mode) {
    case VA_DIRECT: return a->comps * a->csize;
    case VA_INDEX8: return indices;
    case VA_INDEX16: return 2 * indices;
    default: return 0;
    }
}

static void build_fmt(int f)
{
    VtxFmt *v = &sFmt[f];
    uint32_t lo = gpu_r.cp[0x50], hi = gpu_r.cp[0x60];
    uint32_t a = gpu_r.cp[0x70 + f], b = gpu_r.cp[0x80 + f], c = gpu_r.cp[0x90 + f];
    uint32_t texCnt[8], texType[8], texShift[8];
    int i;

    memset(v, 0, sizeof *v);
    v->pnIdx = lo & 1;
    for (i = 0; i < 8; i++)
        v->texIdx[i] = (lo >> (1 + i)) & 1;

    v->pos.mode = (lo >> 9) & 3;
    v->pos.comps = (a & 1) ? 3 : 2;
    v->pos.type = (a >> 1) & 7;
    v->pos.csize = kCompSize[v->pos.type];
    v->pos.scale = v->pos.type < 4 ? 1.0f / (float)(1u << ((a >> 4) & 31)) : 1.0f;

    v->nrm.mode = (lo >> 11) & 3;
    v->nbt = (a >> 9) & 1;
    v->nrmIdx3 = v->nbt && ((a >> 31) & 1);
    v->nrm.comps = v->nbt ? 9 : 3;
    v->nrm.type = (a >> 10) & 7;
    v->nrm.csize = kCompSize[v->nrm.type];
    v->nrm.scale = v->nrm.type == 1 ? 1.0f / 64 : v->nrm.type == 3 ? 1.0f / 16384 :
                   v->nrm.type == 0 ? 1.0f / 128 : v->nrm.type == 2 ? 1.0f / 32768 : 1.0f;

    for (i = 0; i < 2; i++) {
        Attr *col = &v->col[i];
        col->mode = (lo >> (13 + 2 * i)) & 3;
        col->type = (a >> (14 + 4 * i)) & 7;
        col->comps = 1;
        col->csize = kColorSize[col->type];
    }

    texCnt[0] = (a >> 21) & 1; texType[0] = (a >> 22) & 7; texShift[0] = (a >> 25) & 31;
    texCnt[1] = b & 1;         texType[1] = (b >> 1) & 7;   texShift[1] = (b >> 4) & 31;
    texCnt[2] = (b >> 9) & 1;  texType[2] = (b >> 10) & 7;  texShift[2] = (b >> 13) & 31;
    texCnt[3] = (b >> 18) & 1; texType[3] = (b >> 19) & 7;  texShift[3] = (b >> 22) & 31;
    texCnt[4] = (b >> 27) & 1; texType[4] = (b >> 28) & 7;  texShift[4] = c & 31;
    texCnt[5] = (c >> 5) & 1;  texType[5] = (c >> 6) & 7;   texShift[5] = (c >> 9) & 31;
    texCnt[6] = (c >> 14) & 1; texType[6] = (c >> 15) & 7;  texShift[6] = (c >> 18) & 31;
    texCnt[7] = (c >> 23) & 1; texType[7] = (c >> 24) & 7;  texShift[7] = (c >> 27) & 31;
    for (i = 0; i < 8; i++) {
        Attr *t = &v->tex[i];
        t->mode = (hi >> (2 * i)) & 3;
        t->comps = texCnt[i] ? 2 : 1;
        t->type = texType[i];
        t->csize = kCompSize[t->type];
        t->scale = t->type < 4 ? 1.0f / (float)(1u << texShift[i]) : 1.0f;
    }

    v->size = v->pnIdx;
    for (i = 0; i < 8; i++)
        v->size += v->texIdx[i];
    v->size += attr_bytes(&v->pos, 1);
    v->size += attr_bytes(&v->nrm, v->nrmIdx3 ? 3 : 1);
    v->size += attr_bytes(&v->col[0], 1) + attr_bytes(&v->col[1], 1);
    for (i = 0; i < 8; i++)
        v->size += attr_bytes(&v->tex[i], 1);
}

static const VtxFmt *get_fmt(int f)
{
    if (!(sFmtValid & (1u << f))) {
        build_fmt(f);
        sFmtValid |= (uint8_t)(1u << f);
    }
    return &sFmt[f];
}

uint32_t gpu_vtx_size(int fmt) { return get_fmt(fmt)->size; }

/* ---- component readers ---- */

static inline float rd_be(const uint8_t *p, int type)
{
    switch (type) {
    case 0: return (float)p[0];
    case 1: return (float)(int8_t)p[0];
    case 2: return (float)(uint16_t)gpu_be16(p);
    case 3: return (float)(int16_t)gpu_be16(p);
    default: {
        uint32_t u = gpu_be32(p);
        float f;
        memcpy(&f, &u, 4);
        return f;
    }
    }
}

static inline float rd_host(const uint8_t *p, int type)
{
    switch (type) {
    case 0: return (float)p[0];
    case 1: return (float)(int8_t)p[0];
    case 2: { uint16_t u; memcpy(&u, p, 2); return (float)u; }
    case 3: { int16_t s; memcpy(&s, p, 2); return (float)s; }
    default: { float f; memcpy(&f, p, 4); return f; }
    }
}

static void rd_color(float out[4], const uint8_t *p, int type, int host)
{
    uint32_t v;
    switch (type) {
    case 0: /* RGB565 */
        if (host) { uint16_t u; memcpy(&u, p, 2); v = u; } else v = gpu_be16(p);
        out[0] = ((v >> 11) & 31) / 31.0f; out[1] = ((v >> 5) & 63) / 63.0f; out[2] = (v & 31) / 31.0f; out[3] = 1;
        break;
    case 1: /* RGB8 */
    case 2: /* RGBX8 */
        out[0] = p[0] / 255.0f; out[1] = p[1] / 255.0f; out[2] = p[2] / 255.0f; out[3] = 1;
        break;
    case 3: /* RGBA4 */
        if (host) { uint16_t u; memcpy(&u, p, 2); v = u; } else v = gpu_be16(p);
        out[0] = ((v >> 12) & 15) / 15.0f; out[1] = ((v >> 8) & 15) / 15.0f;
        out[2] = ((v >> 4) & 15) / 15.0f; out[3] = (v & 15) / 15.0f;
        break;
    case 4: /* RGBA6 */
        v = (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2];
        out[0] = ((v >> 18) & 63) / 63.0f; out[1] = ((v >> 12) & 63) / 63.0f;
        out[2] = ((v >> 6) & 63) / 63.0f; out[3] = (v & 63) / 63.0f;
        break;
    default: /* RGBA8 */
        out[0] = p[0] / 255.0f; out[1] = p[1] / 255.0f; out[2] = p[2] / 255.0f; out[3] = p[3] / 255.0f;
        break;
    }
}

/* ---- decoded input vertex ---- */

typedef struct InVtx {
    uint8_t pn, tm[8];
    float pos[3], nrm[3], bin[3], tan[3];
    float col[2][4];
    float tex[8][2];
} InVtx;

static const uint8_t *array_elem(int array, uint32_t index, uint32_t bytes)
{
    uint32_t addr = gpu_r.cp[0xA0 + array] + index * gpu_r.cp[0xB0 + array];
    static const uint8_t zero[64];
    if (!gpu_ram_ok(addr, bytes))
        return zero;
    return (const uint8_t *)(uintptr_t)addr;
}

/* Reads an attribute's components into out[]; returns bytes consumed from p. */
static uint32_t rd_attr(const Attr *a, int array, const uint8_t *p, float *out, int comps)
{
    int k;
    if (a->mode == VA_DIRECT) {
        for (k = 0; k < comps; k++)
            out[k] = rd_be(p + k * a->csize, a->type) * a->scale;
        return a->comps * a->csize;
    } else {
        uint32_t idx = a->mode == VA_INDEX8 ? p[0] : gpu_be16(p);
        const uint8_t *e = array_elem(array, idx, (uint32_t)comps * a->csize);
        for (k = 0; k < comps; k++)
            out[k] = rd_host(e + k * a->csize, a->type) * a->scale;
        return a->mode == VA_INDEX8 ? 1 : 2;
    }
}

static void decode(const VtxFmt *f, const uint8_t *p, InVtx *v)
{
    uint32_t mA = gpu_r.cp[0x30], mB = gpu_r.cp[0x40];
    int i;

    v->pn = f->pnIdx ? (*p++ & 63) : (mA & 63);
    for (i = 0; i < 8; i++) {
        uint8_t def = (uint8_t)(i < 4 ? (mA >> (6 + 6 * i)) & 63 : (mB >> (6 * (i - 4))) & 63);
        v->tm[i] = f->texIdx[i] ? (*p++ & 63) : def;
    }

    if (f->pos.mode) {
        v->pos[2] = 0;
        p += rd_attr(&f->pos, 0, p, v->pos, f->pos.comps);
    } else {
        v->pos[0] = v->pos[1] = v->pos[2] = 0;
    }

    if (f->nrm.mode) {
        if (f->nrm.mode == VA_DIRECT || !f->nrmIdx3) {
            float t[9];
            p += rd_attr(&f->nrm, 1, p, t, f->nrm.comps);
            memcpy(v->nrm, t, 12);
            if (f->nbt) {
                memcpy(v->bin, t + 3, 12);
                memcpy(v->tan, t + 6, 12);
            }
        } else {
            /* three indices: normal, binormal, tangent, each into its slot of the element */
            int k, j;
            float *dst[3] = {v->nrm, v->bin, v->tan};
            for (k = 0; k < 3; k++) {
                uint32_t idx = f->nrm.mode == VA_INDEX8 ? p[0] : gpu_be16(p);
                const uint8_t *e = array_elem(1, idx, 9u * f->nrm.csize) + 3 * k * f->nrm.csize;
                for (j = 0; j < 3; j++)
                    dst[k][j] = rd_host(e + j * f->nrm.csize, f->nrm.type) * f->nrm.scale;
                p += f->nrm.mode == VA_INDEX8 ? 1 : 2;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        const Attr *a = &f->col[i];
        if (a->mode == VA_DIRECT) {
            rd_color(v->col[i], p, a->type, 0);
            p += a->csize;
        } else if (a->mode) {
            uint32_t idx = a->mode == VA_INDEX8 ? p[0] : gpu_be16(p);
            rd_color(v->col[i], array_elem(2 + i, idx, a->csize), a->type, 1);
            p += a->mode == VA_INDEX8 ? 1 : 2;
        } else {
            v->col[i][0] = v->col[i][1] = v->col[i][2] = v->col[i][3] = 1;
        }
    }

    for (i = 0; i < 8; i++) {
        const Attr *a = &f->tex[i];
        if (a->mode) {
            v->tex[i][1] = 0;
            p += rd_attr(a, 4 + i, p, v->tex[i], a->comps);
        }
    }
}

/* ---- transform, lighting, texgen ---- */

static inline const float *xm(uint32_t addr) { return &gpu_r.xfmem[addr & 0x7FF]; }

static inline void mul34(const float *m, const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[1] * in[1] + m[2] * in[2] + m[3];
    out[1] = m[4] * in[0] + m[5] * in[1] + m[6] * in[2] + m[7];
    out[2] = m[8] * in[0] + m[9] * in[1] + m[10] * in[2] + m[11];
}

static inline void mul33(const float *m, const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[1] * in[1] + m[2] * in[2];
    out[1] = m[3] * in[0] + m[4] * in[1] + m[5] * in[2];
    out[2] = m[6] * in[0] + m[7] * in[1] + m[8] * in[2];
}

static inline float dot3(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

static inline void unpack_rgba(uint32_t c, float out[4])
{
    out[0] = (c >> 24) / 255.0f;
    out[1] = ((c >> 16) & 255) / 255.0f;
    out[2] = ((c >> 8) & 255) / 255.0f;
    out[3] = (c & 255) / 255.0f;
}

/* Per-draw constant state. */
typedef struct XfState {
    float proj[6];
    int ortho;
    int numColors, numTexGens;
    int needNormal;
    uint32_t chanCtrl[4]; /* color0, color1, alpha0, alpha1 */
    float matReg[2][4], ambReg[2][4];
    uint32_t texInfo[8], postInfo[8];
    int dualTex;
    float lightColor[8][4];
} XfState;

static void xf_state(XfState *s)
{
    int i;
    for (i = 0; i < 6; i++)
        s->proj[i] = gpu_xf_float(0x20 + i);
    s->ortho = gpu_r.xf[0x26] != 0;
    s->numColors = gpu_r.xf[0x09] & 3;
    s->numTexGens = gpu_r.xf[0x3F] & 15;
    if (s->numTexGens > 8)
        s->numTexGens = 8;
    for (i = 0; i < 4; i++)
        s->chanCtrl[i] = gpu_r.xf[0x0E + i];
    for (i = 0; i < 2; i++) {
        unpack_rgba(gpu_r.xf[0x0C + i], s->matReg[i]);
        unpack_rgba(gpu_r.xf[0x0A + i], s->ambReg[i]);
    }
    s->dualTex = gpu_r.xf[0x12] & 1;
    s->needNormal = 0;
    for (i = 0; i < 4; i++)
        if (s->chanCtrl[i] & 2)
            s->needNormal = 1;
    for (i = 0; i < 8; i++) {
        uint32_t c;
        memcpy(&c, xm(0x600 + i * 16 + 3), 4);
        unpack_rgba(c, s->lightColor[i]);
    }
    for (i = 0; i < s->numTexGens; i++) {
        s->texInfo[i] = gpu_r.xf[0x40 + i];
        s->postInfo[i] = gpu_r.xf[0x50 + i];
        if (((s->texInfo[i] >> 4) & 7) == 1)
            s->needNormal = 1;
    }
}

/* One lighting channel (rgb: comp 0-2, or alpha: comp 3). */
static void light_chan(const XfState *s, uint32_t ctrl, int chan, int alpha, const float vcol[4],
                       const float eye[3], const float n[3], float out[4])
{
    const float *mat = (ctrl & 1) ? vcol : s->matReg[chan];
    int c0 = alpha ? 3 : 0, c1 = alpha ? 4 : 3, c, l;
    float acc[4];
    uint32_t mask;
    int dfn, attn, spot;

    if (!(ctrl & 2)) {
        for (c = c0; c < c1; c++)
            out[c] = mat[c];
        return;
    }
    {
        const float *amb = (ctrl & 0x40) ? vcol : s->ambReg[chan];
        for (c = c0; c < c1; c++)
            acc[c] = amb[c];
    }
    mask = ((ctrl >> 2) & 0xF) | (((ctrl >> 11) & 0xF) << 4);
    dfn = (ctrl >> 7) & 3;
    attn = (ctrl >> 9) & 1;
    spot = (ctrl >> 10) & 1;
    for (l = 0; l < 8; l++) {
        const float *lp, *ld, *la, *lk;
        float L[3], dist2, dist, diff, att = 1.0f;
        if (!(mask & (1u << l)))
            continue;
        la = xm(0x600 + l * 16 + 4);
        lk = xm(0x600 + l * 16 + 7);
        lp = xm(0x600 + l * 16 + 10);
        ld = xm(0x600 + l * 16 + 13);
        L[0] = lp[0] - eye[0];
        L[1] = lp[1] - eye[1];
        L[2] = lp[2] - eye[2];
        dist2 = dot3(L, L);
        dist = sqrtf(dist2);
        if (dist > 0) {
            L[0] /= dist; L[1] /= dist; L[2] /= dist;
        }
        diff = dfn == 0 ? 1.0f : dot3(n, L);
        if (dfn == 2 && diff < 0)
            diff = 0;
        if (attn) {
            float x, aa, dd;
            if (spot) {
                x = dot3(L, ld);
                aa = la[0] + la[1] * x + la[2] * x * x;
                dd = lk[0] + lk[1] * dist + lk[2] * dist2;
            } else {
                float nn = sqrtf(dot3(n, n));
                x = nn > 0 ? dot3(n, ld) / nn : 0;
                aa = x > 0 ? la[0] + la[1] * x + la[2] * x * x : 0;
                dd = lk[0] + lk[1] * x + lk[2] * x * x;
            }
            if (aa < 0)
                aa = 0;
            att = dd != 0 ? aa / dd : 0;
        }
        for (c = c0; c < c1; c++)
            acc[c] += diff * att * s->lightColor[l][c];
    }
    for (c = c0; c < c1; c++) {
        float a = acc[c] < 0 ? 0 : acc[c] > 1 ? 1 : acc[c];
        out[c] = mat[c] * a;
    }
}

static void xform(const XfState *s, const InVtx *in, GpuVertex *o)
{
    float eye[3], n[3] = {0, 0, 0}, b[3] = {0, 0, 0}, t[3] = {0, 0, 0};
    const float *p = s->proj;
    int i;

    mul34(xm(in->pn * 4), in->pos, eye);
    if (s->needNormal) {
        const float *nm = xm(0x400 + (in->pn & 31) * 3);
        mul33(nm, in->nrm, n);
        mul33(nm, in->bin, b);
        mul33(nm, in->tan, t);
    }

    if (s->ortho) {
        o->pos[0] = p[0] * eye[0] + p[1];
        o->pos[1] = p[2] * eye[1] + p[3];
        o->pos[3] = 1.0f;
    } else {
        o->pos[0] = p[0] * eye[0] + p[1] * eye[2];
        o->pos[1] = p[2] * eye[1] + p[3] * eye[2];
        o->pos[3] = -eye[2];
    }
    o->pos[2] = 2.0f * (p[4] * eye[2] + p[5]) + o->pos[3];

    for (i = 0; i < 2; i++) {
        if (i < s->numColors) {
            light_chan(s, s->chanCtrl[i], i, 0, in->col[i], eye, n, o->color[i]);
            light_chan(s, s->chanCtrl[2 + i], i, 1, in->col[i], eye, n, o->color[i]);
        } else {
            o->color[i][0] = o->color[i][1] = o->color[i][2] = o->color[i][3] = 1.0f;
        }
    }

    for (i = 0; i < s->numTexGens; i++) {
        uint32_t info = s->texInfo[i];
        int type = (info >> 4) & 7, row = (info >> 7) & 31;
        float src[4] = {0, 0, 1, 1}, *out = o->tex[i];
        if (type == 0) {
            const float *m = xm(in->tm[i] * 4);
            switch (row) {
            case 0: memcpy(src, in->pos, 12); break;
            case 1: memcpy(src, in->nrm, 12); break;
            case 2: src[0] = o->color[0][0]; src[1] = o->color[0][1]; break;
            case 3: memcpy(src, in->bin, 12); break;
            case 4: memcpy(src, in->tan, 12); break;
            default:
                if (row >= 5 && row <= 12) {
                    src[0] = in->tex[row - 5][0];
                    src[1] = in->tex[row - 5][1];
                }
                break;
            }
            if (!(info & 4)) /* AB11 */
                src[2] = 1.0f;
            out[0] = m[0] * src[0] + m[1] * src[1] + m[2] * src[2] + m[3];
            out[1] = m[4] * src[0] + m[5] * src[1] + m[6] * src[2] + m[7];
            out[2] = (info & 2) ? m[8] * src[0] + m[9] * src[1] + m[10] * src[2] + m[11] : 1.0f;
            if (s->dualTex) {
                uint32_t post = s->postInfo[i];
                const float *pm = xm(0x500 + (post & 63) * 4);
                float v[3] = {out[0], out[1], out[2]};
                if (post & 0x100) {
                    float len = sqrtf(dot3(v, v));
                    if (len > 0) {
                        v[0] /= len; v[1] /= len; v[2] /= len;
                    }
                }
                mul34(pm, v, out);
            }
        } else if (type == 1) { /* emboss: shift a previous coordinate by the light direction */
            int srcGen = (info >> 12) & 7, light = (info >> 15) & 7;
            const float *lp = xm(0x600 + light * 16 + 10);
            float L[3] = {lp[0] - eye[0], lp[1] - eye[1], lp[2] - eye[2]};
            float len = sqrtf(dot3(L, L));
            if (len > 0) {
                L[0] /= len; L[1] /= len; L[2] /= len;
            }
            out[0] = o->tex[srcGen][0] + dot3(L, t);
            out[1] = o->tex[srcGen][1] + dot3(L, b);
            out[2] = 1.0f;
        } else { /* color texgen: (s, t) = (r, g) of color channel 0 or 1 */
            const float *c = o->color[type == 3 ? 1 : 0];
            out[0] = c[0];
            out[1] = c[1];
            out[2] = 1.0f;
        }
    }
    for (; i < GPU_MAX_TEXCOORDS; i++)
        o->tex[i][0] = o->tex[i][1] = 0, o->tex[i][2] = 1.0f;
}

/* ---- primitive assembly ---- */

static GpuVertex *sXv, *sOut;
static uint32_t sXvCap, sOutCap;

static int ensure(GpuVertex **buf, uint32_t *cap, uint32_t n)
{
    if (n > *cap) {
        uint32_t c = *cap ? *cap : 1024;
        GpuVertex *nb;
        while (c < n)
            c *= 2;
        nb = (GpuVertex *)realloc(*buf, (size_t)c * sizeof(GpuVertex));
        if (!nb)
            return 0;
        *buf = nb;
        *cap = c;
    }
    return 1;
}

/* GX: front faces wind clockwise on screen. BP GEN_MODE bits 14-15 as written
 * by GXSetCullMode: 1 culls back faces, 2 culls front faces, 3 culls all. */
static int culled(int mode, const GpuVertex *a, const GpuVertex *b, const GpuVertex *c, float winSign)
{
    float det;
    if (mode == 0)
        return 0;
    if (mode == 3)
        return 1;
    det = a->pos[0] * (b->pos[1] * c->pos[3] - c->pos[1] * b->pos[3])
        - b->pos[0] * (a->pos[1] * c->pos[3] - c->pos[1] * a->pos[3])
        + c->pos[0] * (a->pos[1] * b->pos[3] - b->pos[1] * a->pos[3]);
    det *= winSign; /* > 0: clockwise in window space (y down) = front */
    return mode == 1 ? det < 0 : det > 0;
}

static uint32_t sOutN;
static int sCull;
static float sWinSign;

static inline void emit_tri(const GpuVertex *v, uint32_t a, uint32_t b, uint32_t c)
{
    if (culled(sCull, &v[a], &v[b], &v[c], sWinSign))
        return;
    sOut[sOutN++] = v[a];
    sOut[sOutN++] = v[b];
    sOut[sOutN++] = v[c];
}

void gpu_vtx_draw(int prim, int fmt, uint32_t count, const uint8_t *data)
{
    const VtxFmt *f = get_fmt(fmt);
    XfState s;
    InVtx in;
    uint32_t i, maxOut;
    const uint8_t *p = data;

    if (!ensure(&sXv, &sXvCap, count))
        return;
    memset(&in, 0, sizeof in);
    xf_state(&s);
    for (i = 0; i < count; i++) {
        decode(f, p, &in);
        p += f->size;
        xform(&s, &in, &sXv[i]);
    }

    maxOut = prim <= 1 ? count / 4 * 6 : count * 3;
    if (!ensure(&sOut, &sOutCap, maxOut + 3))
        return;
    sOutN = 0;
    sCull = (gpu_r.bp[0x00] >> 14) & 3;
    {
        float sx = gpu_xf_float(0x1A), sy = gpu_xf_float(0x1B);
        sWinSign = (sx * sy) < 0 ? -1.0f : 1.0f;
    }

    switch (prim) {
    case 0: /* quads */
    case 1: /* quads (alternate encoding) */
        for (i = 0; i + 3 < count; i += 4) {
            emit_tri(sXv, i, i + 1, i + 2);
            emit_tri(sXv, i, i + 2, i + 3);
        }
        break;
    case 2: /* triangles */
        for (i = 0; i + 2 < count; i += 3)
            emit_tri(sXv, i, i + 1, i + 2);
        break;
    case 3: /* triangle strip */
        for (i = 0; i + 2 < count; i++) {
            if (i & 1)
                emit_tri(sXv, i + 1, i, i + 2);
            else
                emit_tri(sXv, i, i + 1, i + 2);
        }
        break;
    case 4: /* triangle fan */
        for (i = 1; i + 1 < count; i++)
            emit_tri(sXv, 0, i, i + 1);
        break;
    case 5: /* lines */
        for (i = 0; i + 1 < count; i += 2) {
            sOut[sOutN++] = sXv[i];
            sOut[sOutN++] = sXv[i + 1];
        }
        if (sOutN)
            gpu_backend->draw(gpu_backend->ctx, &gpu_r, GPU_LINES, sOut, sOutN);
        return;
    case 6: /* line strip */
        for (i = 0; i + 1 < count; i++) {
            sOut[sOutN++] = sXv[i];
            sOut[sOutN++] = sXv[i + 1];
        }
        if (sOutN)
            gpu_backend->draw(gpu_backend->ctx, &gpu_r, GPU_LINES, sOut, sOutN);
        return;
    default: /* points */
        gpu_backend->draw(gpu_backend->ctx, &gpu_r, GPU_POINTS, sXv, count);
        return;
    }
    if (sOutN)
        gpu_backend->draw(gpu_backend->ctx, &gpu_r, GPU_TRIANGLES, sOut, sOutN);
}
