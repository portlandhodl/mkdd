/* WebGL2 backend: texture cache (RAM textures decoded with texdec) and the
 * registry of EFB copies that later texture loads resolve to. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webgl_internal.h"
#include "texdec.h"

/* ---- hashing ---- */

static uint64_t hash_bytes(const uint8_t *p, uint32_t n, uint64_t seed)
{
    uint64_t h = seed ^ (n * 0x9E3779B97F4A7C15ull);
    uint32_t i = 0;
    for (; i + 8 <= n; i += 8) {
        uint64_t v;
        memcpy(&v, p + i, 8);
        h ^= v * 0xC2B2AE3D27D4EB4Full;
        h = (h << 31 | h >> 33) * 0x9E3779B97F4A7C15ull;
    }
    for (; i < n; i++) h = (h ^ p[i]) * 0x100000001B3ull;
    h ^= h >> 29;
    h *= 0xBF58476D1CE4E5B9ull;
    h ^= h >> 32;
    wgl_stats_cur.texHashedBytes += n;
    return h;
}

/* ---- RAM texture cache ---- */

struct TexEntry {
    uint32_t addr, tlutOff;
    uint16_t w, h;
    uint8_t fmt, levels, tlutFmt, used, arbitraryMips;
    uint64_t hash;
    uint32_t checkedFrame, lastUsed;
    GLuint tex;
};

#define TEX_SLOTS 8192
#define TEX_MAX_LIVE 6000

static TexEntry *sTex;
static int sTexCount;
static uint8_t *sDecode, *sPrevLevel; /* 1024 x 1024 RGBA8 */
static GLuint sWhite;
static GLuint sSampler[8];
static uint64_t sSamplerKey[8];
static GLuint sBound[8];

/* ---- EFB copies ---- */

struct CopyEntry {
    uint32_t addr, bytes;
    int fmt, w, h; /* native size */
    int scale;     /* the texture is w * scale x h * scale */
    uint64_t ramHash;
    uint32_t checkedFrame;
    GLuint tex;
};

#define MAX_COPIES 128
static CopyEntry sCopies[MAX_COPIES];

void wgl_tex_init(void)
{
    if (!sTex) sTex = (TexEntry *)calloc(TEX_SLOTS, sizeof(TexEntry));
    if (!sDecode) sDecode = (uint8_t *)malloc(1024 * 1024 * 4);
    if (!sPrevLevel) sPrevLevel = (uint8_t *)malloc(512 * 512 * 4);
    glGenTextures(1, &sWhite);
    glBindTexture(GL_TEXTURE_2D, sWhite);
    static const uint8_t white[4] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glGenSamplers(8, sSampler);
    for (int i = 0; i < 8; i++) {
        sSamplerKey[i] = ~0ull;
        glBindSampler((GLuint)i, sSampler[i]);
    }
}

static uint32_t slot_hash(uint32_t addr, int w, int h, int fmt)
{
    uint32_t x = addr * 0x9E3779B1u ^ (uint32_t)(w << 20 | h << 8 | fmt) * 0x85EBCA6Bu;
    x ^= x >> 15;
    return x & (TEX_SLOTS - 1);
}

static void evict_old(void)
{
    /* Drop textures not used for a while and rebuild the open-addressing table. */
    static TexEntry *keep;
    if (!keep) keep = (TexEntry *)malloc(TEX_SLOTS * sizeof(TexEntry));
    int n = 0;
    for (int i = 0; i < TEX_SLOTS; i++) {
        TexEntry *e = &sTex[i];
        if (!e->used) continue;
        if (wgl_frame - e->lastUsed > 30) glDeleteTextures(1, &e->tex);
        else keep[n++] = *e;
    }
    memset(sTex, 0, TEX_SLOTS * sizeof(TexEntry));
    for (int i = 0; i < n; i++) {
        uint32_t s = slot_hash(keep[i].addr, keep[i].w, keep[i].h, keep[i].fmt);
        while (sTex[s].used) s = (s + 1) & (TEX_SLOTS - 1);
        sTex[s] = keep[i];
    }
    sTexCount = n;
}

/* Mean absolute difference (0-255 per channel) between level `prev` (pw x ph) box-filtered
 * down to lw x lh and the next level `cur`. */
static float mip_difference(const uint8_t *prev, int pw, int ph, const uint8_t *cur, int lw, int lh)
{
    uint64_t sum = 0;
    for (int y = 0; y < lh; y++)
        for (int x = 0; x < lw; x++) {
            int x0 = x * 2 < pw ? x * 2 : pw - 1, x1 = x * 2 + 1 < pw ? x * 2 + 1 : pw - 1;
            int y0 = y * 2 < ph ? y * 2 : ph - 1, y1 = y * 2 + 1 < ph ? y * 2 + 1 : ph - 1;
            for (int c = 0; c < 4; c++) {
                int v = (prev[(y0 * pw + x0) * 4 + c] + prev[(y0 * pw + x1) * 4 + c] + prev[(y1 * pw + x0) * 4 + c] +
                         prev[(y1 * pw + x1) * 4 + c] + 2) >> 2;
                int d = v - cur[(y * lw + x) * 4 + c];
                sum += (uint64_t)(d < 0 ? -d : d);
            }
        }
    return (float)sum / (float)(lw * lh * 4);
}

static void upload(int unit, TexEntry *e, const uint8_t *src, const uint8_t *tlut, int tlutCount)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, e->tex);
    sBound[unit] = e->tex;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    int pw = 0, ph = 0, diffN = 0;
    float diffSum = 0.0f;
    for (int l = 0; l < e->levels; l++) {
        int lw = texdec_mip_dim(e->w, l), lh = texdec_mip_dim(e->h, l);
        const uint8_t *ls = src + texdec_mip_offset(e->fmt, e->w, e->h, l);
        if (texdec_decode(sDecode, ls, e->fmt, lw, lh, tlut, e->tlutFmt, tlutCount) != 0)
            memset(sDecode, 0xFF, (size_t)lw * lh * 4);
        glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA8, lw, lh, 0, GL_RGBA, GL_UNSIGNED_BYTE, sDecode);
        /* Arbitrary mipmaps (levels that aren't downscaled copies of the level above, e.g. a
         * distance fade drawn into the mips): detected as Dolphin does, so the LOD bias can
         * keep their level choice native at a higher internal resolution. */
        if (l == 0) {
            e->arbitraryMips = 0;
            diffSum = 0.0f;
            diffN = 0;
        } else if (pw * ph * 4 <= 512 * 512 * 4 && lw >= 4 && lh >= 4) {
            diffSum += mip_difference(sPrevLevel, pw, ph, sDecode, lw, lh);
            diffN++;
        }
        if (e->levels > 1 && l + 1 < e->levels && lw * lh <= 512 * 512) {
            memcpy(sPrevLevel, sDecode, (size_t)lw * lh * 4);
            pw = lw;
            ph = lh;
        } else {
            pw = ph = 0;
        }
    }
    /* threshold set above the block-compression noise of MKDD's CMPR mip chains (up to ~51) */
    e->arbitraryMips = diffN && diffSum / (float)diffN > 64.0f;
    if (e->arbitraryMips) {
        static int nlog;
        if (nlog++ < 8) fprintf(stderr, "[webgl] arbitrary mipmaps: texture %08x %dx%d format %d (mean level difference %.1f)\n", e->addr, e->w, e->h, e->fmt, diffSum / (float)diffN);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, e->levels - 1);
    glActiveTexture(GL_TEXTURE0);
    wgl_stats_cur.texUploads++;
}

static int is_ci(int fmt) { return fmt == TEXDEC_C4 || fmt == TEXDEC_C8 || fmt == TEXDEC_C14X2; }

static const uint8_t *tlut_ptr(uint32_t tlutReg, int fmt, int *count)
{
    uint8_t *tmem = gpu_tmem();
    uint32_t off = 0x80000u + ((tlutReg & 0x3FF) << 9);
    int n = fmt == TEXDEC_C4 ? 16 : fmt == TEXDEC_C8 ? 256 : 16384;
    uint32_t maxN = (0x100000u - off) / 2;
    if ((uint32_t)n > maxN) n = (int)maxN;
    *count = n;
    return tmem ? tmem + off : NULL;
}

static CopyEntry *find_copy(uint32_t addr, int w, int h)
{
    for (int i = 0; i < MAX_COPIES; i++) {
        CopyEntry *c = &sCopies[i];
        if (!c->tex || c->addr != addr) continue;
        if (c->checkedFrame != wgl_frame) {
            /* The copy isn't written to RAM; if the CPU rewrote the area since,
             * the game reuses the memory for something else. */
            const uint8_t *p = wgl_ram(addr, c->bytes);
            uint64_t hh = p ? hash_bytes(p, c->bytes, 7) : 0;
            if (hh != c->ramHash) {
                glDeleteTextures(1, &c->tex);
                memset(c, 0, sizeof *c);
                return NULL;
            }
            c->checkedFrame = wgl_frame;
        }
        return (c->w == w && c->h == h) ? c : NULL;
    }
    return NULL;
}

GLuint wgl_tex_copy_target(uint32_t addr, int fmt, int w, int h, int scale)
{
    CopyEntry *slot = NULL;
    for (int i = 0; i < MAX_COPIES && !slot; i++)
        if (sCopies[i].tex && sCopies[i].addr == addr) slot = &sCopies[i];
    for (int i = 0; i < MAX_COPIES && !slot; i++)
        if (!sCopies[i].tex) slot = &sCopies[i];
    if (!slot) { /* recycle the least recently checked */
        slot = &sCopies[0];
        for (int i = 1; i < MAX_COPIES; i++)
            if (sCopies[i].checkedFrame < slot->checkedFrame) slot = &sCopies[i];
        slot->addr = 0;
    }
    if (!slot->tex) glGenTextures(1, &slot->tex);
    if (slot->addr != addr || slot->w != w || slot->h != h || slot->scale != scale) {
        glBindTexture(GL_TEXTURE_2D, slot->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w * scale, h * scale, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        wgl_tex_forget_bindings();
    }
    slot->addr = addr;
    slot->fmt = fmt;
    slot->w = w;
    slot->h = h;
    slot->scale = scale;
    return slot->tex;
}

void wgl_tex_register_copy(uint32_t addr, int fmt, int w, int h, GLuint tex)
{
    for (int i = 0; i < MAX_COPIES; i++) {
        CopyEntry *c = &sCopies[i];
        if (c->tex != tex) continue;
        c->bytes = texdec_size(fmt, w, h);
        if (!c->bytes) c->bytes = (uint32_t)w * h * 4;
        const uint8_t *p = wgl_ram(addr, c->bytes);
        c->ramHash = p ? hash_bytes(p, c->bytes, 7) : 0;
        c->checkedFrame = wgl_frame;
        return;
    }
}

void wgl_tex_new_frame(void)
{
    /* also every ~10 s: textures of a course left behind would otherwise stay
     * (GPU memory) until TEX_MAX_LIVE of them pile up over several races */
    if (sTexCount > TEX_MAX_LIVE || (wgl_frame % 600 == 0 && sTexCount > 256)) evict_old();
}

extern "C" int gpu_webgl_live_textures(void) { return sTexCount; }

void wgl_tex_settings_changed(void)
{
    for (int i = 0; i < 8; i++) sSamplerKey[i] = ~0ull;
}

/* flags: 1 an EFB copy, 2 arbitrary mipmaps */
static void set_sampler(int unit, uint32_t mode0, uint32_t mode1, int levels, int flags)
{
    uint64_t key = (uint64_t)(mode0 & 0x1FFFFF) | (uint64_t)(mode1 & 0xFFFF) << 21 | (uint64_t)(levels & 31) << 37 |
                   (uint64_t)flags << 42;
    if (sSamplerKey[unit] == key) return;
    sSamplerKey[unit] = key;
    GLuint s = sSampler[unit];
    static const GLint wraps[4] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT, GL_REPEAT };
    glSamplerParameteri(s, GL_TEXTURE_WRAP_S, wraps[mode0 & 3]);
    glSamplerParameteri(s, GL_TEXTURE_WRAP_T, wraps[(mode0 >> 2) & 3]);
    glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, (mode0 >> 4) & 1 ? GL_LINEAR : GL_NEAREST);
    GLint minf;
    switch ((mode0 >> 5) & 7) {
    case 1: minf = GL_NEAREST_MIPMAP_NEAREST; break;
    case 2: minf = GL_NEAREST_MIPMAP_LINEAR; break;
    case 4: minf = GL_LINEAR; break;
    case 5: minf = GL_LINEAR_MIPMAP_NEAREST; break;
    case 6: minf = GL_LINEAR_MIPMAP_LINEAR; break;
    default: minf = GL_NEAREST; break;
    }
    if (levels <= 1) minf = (minf == GL_NEAREST || minf == GL_NEAREST_MIPMAP_NEAREST || minf == GL_NEAREST_MIPMAP_LINEAR) ? GL_NEAREST : GL_LINEAR;
    /* enhancements (never for EFB copies, whose texels map to screen pixels) */
    int tf = (flags & 1) ? 0 : wgl_gfx.texFilter;
    if (tf >= 1 && levels > 1) { /* trilinear: blend between mip levels */
        if (minf == GL_LINEAR_MIPMAP_NEAREST) minf = GL_LINEAR_MIPMAP_LINEAR;
        else if (minf == GL_NEAREST_MIPMAP_NEAREST) minf = GL_NEAREST_MIPMAP_LINEAR;
    }
    if (tf >= 2) { /* force linear (Dolphin's "force texture filtering") */
        minf = levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (wgl_max_aniso > 0) {
        /* as Dolphin: every texture that isn't point-sampled (min and mag nearest) is
         * filtered linearly, between mips too, with anisotropy */
        float an = 1.0f;
        int point = !((mode0 >> 4) & 1) && !((mode0 >> 7) & 1);
        if (wgl_gfx.aniso > 1 && !point && !(flags & 2)) { /* arbitrary mips: not with anisotropy (Dolphin) */
            an = (float)wgl_gfx.aniso;
            if (an > wgl_max_aniso) an = wgl_max_aniso;
            minf = levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
            glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glSamplerParameterf(s, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY_EXT */, an);
    }
    glSamplerParameteri(s, GL_TEXTURE_MIN_FILTER, minf);
    glSamplerParameterf(s, GL_TEXTURE_MIN_LOD, (float)(mode1 & 0xFF) / 16.0f);
    glSamplerParameterf(s, GL_TEXTURE_MAX_LOD, (float)((mode1 >> 8) & 0xFF) / 16.0f);
}

static void bind_unit(int unit, GLuint tex)
{
    if (sBound[unit] == tex) return;
    sBound[unit] = tex;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
    glActiveTexture(GL_TEXTURE0);
}

void wgl_tex_bind(const WglSnap *s, int map, WglTexInfo *out)
{
    int base = map < 4 ? 0x80 + map : 0xA0 + (map - 4);
    uint32_t mode0 = s->bp[base], mode1 = s->bp[base + 4], image0 = s->bp[base + 8];
    uint32_t image3 = s->bp[base + 0x14], tlutReg = s->bp[base + 0x18];
    int w = (int)(image0 & 0x3FF) + 1, h = (int)((image0 >> 10) & 0x3FF) + 1, fmt = (int)((image0 >> 20) & 15);
    uint32_t addr = (image3 & 0xFFFFFF) << 5;
    int minf = (int)((mode0 >> 5) & 7);
    int levels = 1;
    if (minf == 1 || minf == 2 || minf == 5 || minf == 6) {
        int maxLevels = 1;
        while ((w >> maxLevels) || (h >> maxLevels)) maxLevels++;
        levels = (int)((mode1 >> 8) & 0xFF) / 16 + 1;
        if (levels > maxLevels) levels = maxLevels;
        if (levels > 11) levels = 11;
    }
    out->w = w;
    out->h = h;
    out->levels = levels;

    CopyEntry *c = find_copy(addr, w, h);
    if (c) {
        out->tex = c->tex;
        out->levels = 1;
        out->isCopy = 1;
        bind_unit(map, c->tex);
        set_sampler(map, mode0, mode1, 1, 1);
        return;
    }

    int tlutFmt = 0, tlutCount = 0;
    uint32_t tlutOff = 0;
    const uint8_t *tlut = NULL;
    if (is_ci(fmt)) {
        tlutFmt = (int)((tlutReg >> 10) & 3);
        tlutOff = tlutReg & 0x3FF;
        tlut = tlut_ptr(tlutReg, fmt, &tlutCount);
    }
    uint32_t bytes = texdec_chain_size(fmt, w, h, levels);
    const uint8_t *src = bytes ? wgl_ram(addr, bytes) : NULL;
    if (!src) {
        out->tex = sWhite;
        out->w = out->h = 1;
        out->levels = 1;
        bind_unit(map, sWhite);
        set_sampler(map, mode0, mode1, 1, 0);
        return;
    }

    uint32_t slot = slot_hash(addr, w, h, fmt);
    TexEntry *e = NULL;
    for (;;) {
        TexEntry *t = &sTex[slot];
        if (!t->used) break;
        if (t->addr == addr && t->w == w && t->h == h && t->fmt == fmt && t->levels == levels &&
            (!is_ci(fmt) || (t->tlutOff == tlutOff && t->tlutFmt == tlutFmt))) {
            e = t;
            break;
        }
        slot = (slot + 1) & (TEX_SLOTS - 1);
    }
    int fresh = 0;
    if (!e) {
        if (sTexCount >= TEX_SLOTS - 64) {
            evict_old();
            wgl_tex_bind(s, map, out);
            return;
        }
        e = &sTex[slot];
        memset(e, 0, sizeof *e);
        e->used = 1;
        e->addr = addr;
        e->w = (uint16_t)w;
        e->h = (uint16_t)h;
        e->fmt = (uint8_t)fmt;
        e->levels = (uint8_t)levels;
        e->tlutFmt = (uint8_t)tlutFmt;
        e->tlutOff = tlutOff;
        glGenTextures(1, &e->tex);
        sTexCount++;
        fresh = 1;
    }
    if (fresh || e->checkedFrame != wgl_frame) {
        uint64_t hh = hash_bytes(src, bytes, 1);
        if (tlut) hh = hash_bytes(tlut, (uint32_t)tlutCount * 2, hh);
        if (fresh || hh != e->hash) {
            e->hash = hh;
            upload(map, e, src, tlut, tlutCount);
        } else {
            wgl_stats_cur.texCached++;
        }
        e->checkedFrame = wgl_frame;
    }
    e->lastUsed = wgl_frame;
    out->tex = e->tex;
    out->arbitraryMips = e->arbitraryMips;
    bind_unit(map, e->tex);
    set_sampler(map, mode0, mode1, levels, e->arbitraryMips ? 2 : 0);
}

/* Called when a texture object is deleted elsewhere / the backend binds other
 * textures to the units (EFB copy, present). */
void wgl_tex_forget_bindings(void)
{
    for (int i = 0; i < 8; i++) sBound[i] = 0;
}

void wgl_tex_restore_samplers(void)
{
    for (int i = 0; i < 8; i++) glBindSampler((GLuint)i, sSampler[i]);
}
