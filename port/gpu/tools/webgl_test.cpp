/* Synthetic test for the WebGL2 GPU backend (not part of the game link).
 *
 *   port/gpu/tools/webgl_test.sh        builds build/port/webgl_test/
 *   node port/gpu/tools/webgl_test.mjs  renders it in headless Chrome and
 *                                       writes build/port/webgl_test/shot.png
 *
 * Fills GpuRegs by hand (the way the GX library would program the BP/XF
 * registers) and draws GpuVertex primitives through the backend into a 3x3 grid
 * of 213x160 tiles, copies the EFB to the XFB and presents it:
 *   0 vertex colors          1 RGBA8 | C8+TLUT texture   2 two-stage TEV + konst
 *   3 alpha test             4 alpha blending            5 depth test
 *   6 EFB copies sampled     7 indirect warp             8 fog | subtract blend
 *     (RGB565 of tile 0 | I8 half-size of tile 1) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLES3/gl3.h>
#include "gpu.h"
#include "webgl_backend.h"
#include "texdec.h"

static uint8_t sTmem[1 << 20];
extern "C" uint8_t *gpu_tmem(void) { return sTmem; }

static GpuRegs R;
static const GpuBackend *B;

static void bp(int reg, uint32_t v)
{
    R.bp[reg] = v & 0xFFFFFF;
    if (reg >= 0xE0 && reg < 0xE8) (v & 0x800000 ? R.tevKonst : R.tevColor)[reg - 0xE0] = v & 0xFFFFFF;
}
static void xf(int idx, float f) { memcpy(&R.xf[idx], &f, 4); }
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

static void *ram(size_t n)
{
    void *p = aligned_alloc(32, (n + 31) & ~(size_t)31);
    memset(p, 0, n);
    return p;
}

/* ---- tiles / viewport ---- */

static void tile(int i, float nearZ = 0.0f, float farZ = 1.0f)
{
    int left = (i % 3) * 213, top = (i / 3) * 160, w = 213, h = 160;
    xf(0x1A, w / 2.0f);
    xf(0x1B, -h / 2.0f);
    xf(0x1C, (farZ - nearZ) * 16777215.0f);
    xf(0x1D, left + w / 2.0f + 342.0f);
    xf(0x1E, top + h / 2.0f + 342.0f);
    xf(0x1F, farZ * 16777215.0f);
    bp(0x20, (uint32_t)(top + 342) | (uint32_t)(left + 342) << 12);
    bp(0x21, (uint32_t)(top + 342 + h - 1) | (uint32_t)(left + 342 + w - 1) << 12);
    bp(0x59, 171u | 171u << 10);
}

/* ---- TEV helpers ---- */

enum { CPREV = 0, APREV, C0, A0, C1, A1, C2, A2, TEXC, TEXA, RASC, RASA, ONE, HALF, KONST, CZERO };
enum { AA_PREV = 0, AA_A0, AA_A1, AA_A2, AA_TEXA, AA_RASA, AA_KONST, AA_ZERO };

static uint32_t cenv(int a, int b, int c, int d, int dest = 0, int clamp = 1)
{
    return (uint32_t)d | (uint32_t)c << 4 | (uint32_t)b << 8 | (uint32_t)a << 12 | (uint32_t)clamp << 19 | (uint32_t)dest << 22;
}
static uint32_t aenv(int a, int b, int c, int d, int dest = 0, int clamp = 1)
{
    return (uint32_t)d << 4 | (uint32_t)c << 7 | (uint32_t)b << 10 | (uint32_t)a << 13 | (uint32_t)clamp << 19 | (uint32_t)dest << 22;
}
static void genmode(int ntex, int ncol, int ntev, int nind = 0)
{
    bp(0x00, (uint32_t)ntex | (uint32_t)ncol << 4 | (uint32_t)(ntev - 1) << 10 | (uint32_t)nind << 16);
}
static void order(int stage, int map, int coord, int enable, int chan)
{
    uint32_t v = (uint32_t)map | (uint32_t)coord << 3 | (uint32_t)enable << 6 | (uint32_t)chan << 7;
    uint32_t &r = R.bp[0x28 + stage / 2];
    int sh = (stage & 1) * 12;
    r = (r & ~(0xFFFu << sh)) | v << sh;
}
static void stage(int s, uint32_t c, uint32_t a)
{
    bp(0xC0 + s * 2, c);
    bp(0xC1 + s * 2, a);
}
static void passclr(void)
{
    genmode(0, 1, 1);
    order(0, 0, 0, 0, 0);
    stage(0, cenv(CZERO, CZERO, CZERO, RASC), aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_RASA));
    bp(0x10, 0);
}
static void modulate(int map = 0)
{
    genmode(1, 1, 1);
    order(0, map, 0, 1, 0);
    stage(0, cenv(CZERO, TEXC, RASC, CZERO), aenv(AA_ZERO, AA_TEXA, AA_RASA, AA_ZERO));
    bp(0x10, 0);
}

static void defaults(void)
{
    memset(&R, 0, sizeof R);
    for (int i = 0; i < 4; i++) { /* identity swap tables */
        bp(0xF6 + i * 2, 0 | 1 << 2);
        bp(0xF7 + i * 2, 2 | 3 << 2);
    }
    bp(0x40, 1 | 3 << 1 | 1 << 4);                        /* z: on, LEQUAL, update */
    bp(0x41, 1 << 3 | 1 << 4 | 1 << 8 | 0 << 5);          /* no blend, color+alpha update, ONE/ZERO */
    bp(0x42, 0);
    bp(0x43, 0);                                          /* RGB8_Z24 */
    bp(0xF3, 7u << 16 | 7u << 19);                        /* alpha test always */
    bp(0x22, 6 << 8);
}

static void blend(int src, int dst, int sub = 0)
{
    bp(0x41, 1 | 1 << 3 | 1 << 4 | (uint32_t)dst << 5 | (uint32_t)src << 8 | (uint32_t)sub << 11);
}
static void noblend(void) { bp(0x41, 1 << 3 | 1 << 4 | 1 << 8); }

/* ---- textures ---- */

static void texmap(int map, const void *addr, int w, int h, int fmt, int wrap = 1, int linear = 1, uint32_t tlut = 0)
{
    int base = map < 4 ? 0x80 + map : 0xA0 + map - 4;
    bp(base, (uint32_t)wrap | (uint32_t)wrap << 2 | (uint32_t)linear << 4 | (uint32_t)(linear ? 4 : 0) << 5);
    bp(base + 4, 0);
    bp(base + 8, (uint32_t)(w - 1) | (uint32_t)(h - 1) << 10 | (uint32_t)fmt << 20);
    bp(base + 0x14, (uint32_t)((uintptr_t)addr >> 5));
    bp(base + 0x18, tlut);
}

/* GX RGBA8: 4x4 tiles, 32 bytes AR pairs then 32 bytes GB pairs */
static uint8_t *encode_rgba8(int w, int h, void (*px)(int, int, uint8_t *))
{
    uint8_t *out = (uint8_t *)ram(texdec_size(TEXDEC_RGBA8, w, h));
    uint8_t *p = out;
    for (int ty = 0; ty < h; ty += 4)
        for (int tx = 0; tx < w; tx += 4) {
            for (int i = 0; i < 16; i++) {
                uint8_t c[4];
                px(tx + i % 4, ty + i / 4, c);
                p[i * 2] = c[3];
                p[i * 2 + 1] = c[0];
                p[32 + i * 2] = c[1];
                p[32 + i * 2 + 1] = c[2];
            }
            p += 64;
        }
    return out;
}

/* GX C8: 8x4 tiles of 8-bit indices */
static uint8_t *encode_c8(int w, int h, int (*px)(int, int))
{
    uint8_t *out = (uint8_t *)ram(texdec_size(TEXDEC_C8, w, h));
    uint8_t *p = out;
    for (int ty = 0; ty < h; ty += 4)
        for (int tx = 0; tx < w; tx += 8) {
            for (int i = 0; i < 32; i++) p[i] = (uint8_t)px(tx + i % 8, ty + i / 8);
            p += 32;
        }
    return out;
}

static void checker_px(int x, int y, uint8_t *c)
{
    int k = ((x >> 3) ^ (y >> 3)) & 1;
    c[0] = k ? 255 : (uint8_t)(x * 4);
    c[1] = k ? 255 : (uint8_t)(y * 4);
    c[2] = k ? 64 : 200;
    c[3] = 255;
}
static int rings_px(int x, int y)
{
    int dx = x - 16, dy = y - 16;
    return (int)sqrtf((float)(dx * dx + dy * dy)) / 3 & 7;
}
static void warp_px(int x, int y, uint8_t *c)
{
    c[3] = (uint8_t)(128 + 100 * sinf(y * 6.2831853f / 16.0f)); /* s offset (alpha) */
    c[2] = (uint8_t)(128 + 100 * sinf(x * 6.2831853f / 16.0f)); /* t offset (blue) */
    c[1] = 128;
    c[0] = 128;
}

/* ---- geometry ---- */

static GpuVertex V[64];

static void vtx(GpuVertex *v, float x, float y, float z, const float *col, float s, float t, float alpha = -1)
{
    memset(v, 0, sizeof *v);
    v->pos[0] = x;
    v->pos[1] = y;
    v->pos[2] = z;
    v->pos[3] = 1.0f;
    for (int i = 0; i < 4; i++) v->color[0][i] = v->color[1][i] = col[i];
    if (alpha >= 0) v->color[0][3] = alpha;
    for (int i = 0; i < GPU_MAX_TEXCOORDS; i++) {
        v->tex[i][0] = s;
        v->tex[i][1] = t;
        v->tex[i][2] = 1.0f;
    }
}

/* quad in NDC (y up): corners TL TR BL BR; z per left/right edge; alpha per left/right */
static void quad(float x0, float y0, float x1, float y1, const float *col, float zl = 0, float zr = 0, float al = -1,
                 float ar = -1)
{
    GpuVertex tl, tr, bl, br;
    vtx(&tl, x0, y1, zl, col, 0, 0, al);
    vtx(&tr, x1, y1, zr, col, 1, 0, ar);
    vtx(&bl, x0, y0, zl, col, 0, 1, al);
    vtx(&br, x1, y0, zr, col, 1, 1, ar);
    V[0] = tl; V[1] = bl; V[2] = tr;
    V[3] = tr; V[4] = bl; V[5] = br;
    B->draw(B->ctx, &R, GPU_TRIANGLES, V, 6);
}

static const float WHITE[4] = { 1, 1, 1, 1 }, RED[4] = { 1, 0, 0, 1 }, GREEN[4] = { 0, 1, 0, 1 }, BLUE[4] = { 0, 0, 1, 1 };

static void copy_regs(int sx, int sy, int w, int h, const void *dst, uint32_t exec)
{
    bp(0x49, (uint32_t)sx | (uint32_t)sy << 10);
    bp(0x4A, (uint32_t)(w - 1) | (uint32_t)(h - 1) << 10);
    bp(0x4B, (uint32_t)((uintptr_t)dst >> 5));
    bp(0x4D, (uint32_t)(w * 2) >> 5);
    bp(0x52, exec);
}

static void scene1(uint8_t *xfb);
static void scene2(uint8_t *xfb);

int main(int argc, char **argv)
{
    B = gpu_webgl_create();
    if (!B) {
        printf("WEBGL_TEST_FAIL no backend\n");
        return 1;
    }
    static uint8_t *xfb = (uint8_t *)ram(640 * 2 * 576);
    if (argc > 1 && argv[1][0] == '2') scene2(xfb);
    else scene1(xfb);

    /* EFB -> XFB, present */
    copy_regs(0, 0, 640, 480, xfb, 0);
    B->efb_copy(B->ctx, &R, 1, 0);
    B->present(B->ctx, (uint32_t)(uintptr_t)xfb);

    GLenum err = glGetError();
    const GpuWebglStats *st = gpu_webgl_stats();
    printf("stats: flushes=%u draws=%u programs=%u cached=%u texUploads=%u efbCopies=%u xfbCopies=%u glError=0x%x\n",
           st->flushes, st->drawCalls, st->programsCompiled, st->programsCached, st->texUploads, st->efbCopies,
           st->xfbCopies, err);
    printf("WEBGL_TEST_DONE\n");
    return 0;
}

static void clear_efb(uint8_t *xfb)
{
    copy_regs(0, 0, 640, 528, xfb, 0);
    bp(0x4F, 40 | 255 << 8);
    bp(0x50, 40 << 8 | 48);
    bp(0x51, 0xFFFFFF);
    B->efb_copy(B->ctx, &R, 1, 1);
}

static void scene1(uint8_t *xfb)
{
    defaults();

    clear_efb(xfb);

    /* 0: vertex colors */
    tile(0);
    passclr();
    {
        GpuVertex t[3];
        vtx(&t[0], 0.0f, 0.8f, 0, RED, 0, 0);
        vtx(&t[1], -0.8f, -0.8f, 0, GREEN, 0, 0);
        vtx(&t[2], 0.8f, -0.8f, 0, BLUE, 0, 0);
        B->draw(B->ctx, &R, GPU_TRIANGLES, t, 3);
    }

    /* 1: textured: RGBA8 checker (left), C8 + RGB565 TLUT rings (right) */
    tile(1);
    uint8_t *checker = encode_rgba8(64, 64, checker_px);
    texmap(0, checker, 64, 64, TEXDEC_RGBA8);
    modulate();
    quad(-0.95f, -0.9f, -0.05f, 0.9f, WHITE);
    uint8_t *rings = encode_c8(32, 32, rings_px);
    static const uint16_t pal[8] = { 0xF800, 0xFBE0, 0xFFE0, 0x07E0, 0x07FF, 0x001F, 0x781F, 0xFFFF };
    for (int i = 0; i < 256; i++) { /* TLUT at TMEM 0x80000 + (2 << 9), big-endian RGB565 */
        uint16_t c = pal[i & 7];
        sTmem[0x80000 + (2 << 9) + i * 2] = (uint8_t)(c >> 8);
        sTmem[0x80000 + (2 << 9) + i * 2 + 1] = (uint8_t)c;
    }
    texmap(1, rings, 32, 32, TEXDEC_C8, 1, 0, 2 | TEXDEC_TL_RGB565 << 10);
    modulate(1);
    quad(0.05f, -0.9f, 0.95f, 0.9f, WHITE);

    /* 2: two stages: tex * ras -> prev; mix(prev, konst0, 1/2) + c0 * texA */
    tile(2);
    texmap(0, checker, 64, 64, TEXDEC_RGBA8);
    genmode(1, 1, 2);
    order(0, 0, 0, 1, 0);
    order(1, 0, 0, 1, 0);
    stage(0, cenv(CZERO, TEXC, RASC, CZERO), aenv(AA_ZERO, AA_TEXA, AA_RASA, AA_ZERO));
    stage(1, cenv(CPREV, KONST, HALF, CZERO), aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_PREV));
    bp(0x10, 0);
    bp(0x11, 0);
    bp(0xF6, (0 | 1 << 2) | 0x0C << 14);                 /* stage 1 kcsel = K0 */
    bp(0xE0, 255 | 0 << 12 | 0x800000);                  /* K0 R=255 A=0 */
    bp(0xE1, 0 | 0 << 12 | 0x800000);                    /* K0 B=0 G=0 */
    {
        const float cyan[4] = { 0.2f, 1.0f, 1.0f, 1.0f };
        quad(-0.9f, -0.9f, 0.9f, 0.9f, cyan);
    }
    bp(0xF6, 0 | 1 << 2);

    /* 3: alpha test: blue backdrop, then white quad with alpha 0..1 left->right, pass if a > 128 */
    tile(3);
    passclr();
    quad(-0.9f, -0.9f, 0.9f, 0.9f, BLUE);
    bp(0xF3, 128 | 0 << 8 | 4u << 16 | 7u << 19 | 0u << 22); /* GREATER 128 AND ALWAYS */
    quad(-0.8f, -0.5f, 0.8f, 0.5f, WHITE, 0, 0, 0.0f, 1.0f);
    bp(0xF3, 7u << 16 | 7u << 19);

    /* 4: blending: red quad, then green alpha 0.5 overlapping (SRCALPHA, INVSRCALPHA) */
    tile(4);
    passclr();
    quad(-0.9f, -0.6f, 0.3f, 0.9f, RED);
    blend(4, 5);
    quad(-0.3f, -0.9f, 0.9f, 0.6f, GREEN, 0, 0, 0.5f, 0.5f);
    noblend();

    /* 5: depth: red quad z from near (left) to far (right), green quad at mid depth drawn after */
    tile(5);
    passclr();
    quad(-0.9f, -0.5f, 0.9f, 0.9f, RED, -1.0f, 1.0f);
    quad(-0.9f, -0.9f, 0.9f, 0.5f, GREEN, 0.0f, 0.0f);

    /* 6: EFB copies: tile 0 as RGB565 (left), tile 1 as I8 half size (right) */
    void *copyA = ram(texdec_size(TEXDEC_RGB565, 213, 160));
    copy_regs(0, 0, 213, 160, copyA, 4u << 4);
    B->efb_copy(B->ctx, &R, 0, 0);
    void *copyB = ram(texdec_size(TEXDEC_I8, 107, 80));
    copy_regs(213, 0, 213, 160, copyB, 1u << 4 | 1u << 9 | 1u << 15);
    B->efb_copy(B->ctx, &R, 0, 0);
    tile(6);
    texmap(0, copyA, 213, 160, TEXDEC_RGB565, 0);
    modulate();
    quad(-0.95f, -0.9f, -0.05f, 0.9f, WHITE);
    texmap(0, copyB, 107, 80, TEXDEC_I8, 0);
    quad(0.05f, -0.9f, 0.95f, 0.9f, WHITE);

    /* 7: indirect warp of the checker: ind stage 0 = map 1 / coord 1, matrix 0 = 0.5 * 2^-3 */
    tile(7);
    uint8_t *warp = encode_rgba8(16, 16, warp_px);
    texmap(0, checker, 64, 64, TEXDEC_RGBA8);
    texmap(1, warp, 16, 16, TEXDEC_RGBA8, 1, 1);
    genmode(2, 1, 1, 1);
    order(0, 0, 0, 1, 0);
    stage(0, cenv(CZERO, TEXC, RASC, CZERO), aenv(AA_ZERO, AA_TEXA, AA_RASA, AA_ZERO));
    bp(0x27, 1 | 1 << 3);
    bp(0x25, 0);
    {
        int e = -3 + 17;
        bp(0x06, 512u | 0u << 11 | (uint32_t)(e & 3) << 22);
        bp(0x07, 0u | 512u << 11 | (uint32_t)((e >> 2) & 3) << 22);
        bp(0x08, (uint32_t)((e >> 4) & 3) << 22);
    }
    bp(0x10, 0 | 0 << 2 | 3 << 4 | 1 << 9); /* bt 0, 8-bit, bias s+t, matrix 0 */
    quad(-0.9f, -0.9f, 0.9f, 0.9f, WHITE);
    bp(0x10, 0);

    /* 8: fog (ortho, linear: fog = depth) on the left; subtract blend on the right */
    tile(8);
    passclr();
    bp(0xEE, fbits(1.0f) >> 12);
    bp(0xEF, 0);
    bp(0xF0, 0);
    bp(0xF1, (fbits(0.0f) >> 12) | 1u << 20 | 2u << 21);
    bp(0xF2, 255u << 16 | 128u << 8 | 0u);
    quad(-0.95f, -0.9f, -0.05f, 0.9f, BLUE, -1.0f, 1.0f);
    bp(0xF1, 0);
    quad(0.05f, -0.9f, 0.95f, 0.9f, WHITE);
    blend(1, 1, 1);
    {
        const float c[4] = { 0.5f, 0.25f, 0.0f, 1.0f };
        quad(0.15f, -0.6f, 0.85f, 0.6f, c);
    }
    noblend();
}

static void level_px(int, int, uint8_t *) {}

/* Scene 2:
 *   0 TEV compare (tex.r > 128)   1 swap table (b,g,r)         2 scale x2, bias -1/2, + c0 reg
 *   3 mipmaps (1x, 4x, 16x repeat; level colors red/green/blue/yellow/...)
 *   4 RGBA6 + dst alpha (constant alpha 0 under the green DSTALPHA-blended quad)
 *   5 logic XOR (white ^ red = cyan)   6 Z copy (Z8) sampled as I8   7 points + lines
 *   8 copy with clear of a sub-rectangle (magenta) */
static void scene2(uint8_t *xfb)
{
    defaults();
    clear_efb(xfb);
    uint8_t *checker = encode_rgba8(64, 64, checker_px);

    /* 0: compare: out = (tex.r > K0.r) ? ONE : 0 */
    tile(0);
    texmap(0, checker, 64, 64, TEXDEC_RGBA8);
    genmode(1, 1, 1);
    order(0, 0, 0, 1, 0);
    bp(0xE0, 128 | 255 << 12 | 0x800000);
    bp(0xE1, 0x800000);
    bp(0xF6, (0 | 1 << 2) | 0x0C << 4);
    /* compare: bias 3, op 0 (GT), scale 0 (R8); a = TEXC, b = KONST, c = ONE, d = ZERO */
    stage(0, cenv(TEXC, KONST, ONE, CZERO) | 3u << 16, aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_RASA));
    bp(0x10, 0);
    quad(-0.9f, -0.9f, 0.9f, 0.9f, WHITE);
    bp(0xF6, 0 | 1 << 2);

    /* 1: swap table 1 = (b, g, r, a) applied to the texture */
    tile(1);
    bp(0xF8, 2 | 1 << 2);
    bp(0xF9, 0 | 3 << 2);
    modulate();
    R.bp[0xC1] |= 1u << 2; /* tswap = table 1 */
    quad(-0.9f, -0.9f, 0.9f, 0.9f, WHITE);
    bp(0xF8, 0 | 1 << 2);
    bp(0xF9, 2 | 3 << 2);

    /* 2: (tex * 2 - 0.5) + c0, c0 = (0, 0, 100) */
    tile(2);
    bp(0xE2, 0 | 255 << 12);
    bp(0xE3, 100 | 0 << 12);
    genmode(1, 1, 2);
    order(0, 0, 0, 1, 0);
    order(1, 0, 0, 0, 0);
    stage(0, cenv(CZERO, CZERO, CZERO, TEXC) | 2u << 16 | 1u << 20, aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_TEXA));
    stage(1, cenv(CZERO, CZERO, CZERO, C0) | 0, aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_PREV));
    stage(1, cenv(CPREV, CZERO, CZERO, C0) | 0, aenv(AA_ZERO, AA_ZERO, AA_ZERO, AA_PREV));
    /* d + lerp(a=prev, b=0, c=0) = c0 + prev */
    quad(-0.9f, -0.9f, 0.9f, 0.9f, WHITE);

    /* 3: mipmapped 64x64 RGBA8, 7 levels of solid colors */
    tile(3);
    {
        static const uint8_t lc[7][4] = { { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 }, { 255, 255, 0, 255 },
                                          { 0, 255, 255, 255 }, { 255, 0, 255, 255 }, { 255, 255, 255, 255 } };
        uint32_t total = texdec_chain_size(TEXDEC_RGBA8, 64, 64, 7);
        uint8_t *mip = (uint8_t *)ram(total);
        for (int l = 0; l < 7; l++) {
            int d = texdec_mip_dim(64, l);
            uint8_t *p = mip + texdec_mip_offset(TEXDEC_RGBA8, 64, 64, l);
            int tiles = ((d + 3) / 4) * ((d + 3) / 4);
            for (int t = 0; t < tiles; t++, p += 64)
                for (int i = 0; i < 16; i++) {
                    p[i * 2] = lc[l][3];
                    p[i * 2 + 1] = lc[l][0];
                    p[32 + i * 2] = lc[l][1];
                    p[32 + i * 2 + 1] = lc[l][2];
                }
        }
        (void)level_px;
        texmap(0, mip, 64, 64, TEXDEC_RGBA8);
        bp(0x80, 1 | 1 << 2 | 1 << 4 | 6u << 5);   /* repeat, linear, LIN_MIP_LIN */
        bp(0x84, 0 | (6u * 16) << 8);              /* min lod 0, max lod 6 */
        modulate();
        for (int k = 0; k < 3; k++) {
            float rep = k == 0 ? 1.0f : k == 1 ? 4.0f : 16.0f;
            float x0 = -0.95f + k * 0.65f, x1 = x0 + 0.6f;
            GpuVertex q[6];
            vtx(&q[0], x0, 0.9f, 0, WHITE, 0, 0);
            vtx(&q[1], x0, -0.9f, 0, WHITE, 0, rep);
            vtx(&q[2], x1, 0.9f, 0, WHITE, rep, 0);
            q[3] = q[2];
            q[4] = q[1];
            vtx(&q[5], x1, -0.9f, 0, WHITE, rep, rep);
            B->draw(B->ctx, &R, GPU_TRIANGLES, q, 6);
        }
    }

    /* 4: RGBA6 with destination alpha */
    tile(4);
    bp(0x43, 1);
    passclr();
    bp(0x42, 0 | 1 << 8);                      /* dst alpha on, value 0 */
    blend(4, 5);                               /* blended -> two-pass path */
    quad(-0.9f, -0.5f, 0.2f, 0.9f, RED, 0, 0, 1.0f, 1.0f);
    bp(0x42, 0);
    blend(7, 6);                               /* src * (1 - dstA) + dst * dstA */
    quad(-0.4f, -0.9f, 0.9f, 0.4f, GREEN);
    noblend();
    bp(0x43, 0);

    /* 5: logic op XOR */
    tile(5);
    passclr();
    quad(-0.9f, -0.5f, 0.3f, 0.9f, WHITE);
    bp(0x41, 2 | 1 << 3 | 1 << 4 | 6u << 12);
    quad(-0.3f, -0.9f, 0.9f, 0.5f, RED);
    noblend();

    /* 6: depth gradient (left half), copied as Z8 and sampled as I8 (right half) */
    tile(6);
    passclr();
    bp(0x41, 1 << 4 | 1 << 8);                 /* color update off: only z */
    bp(0x40, 1 | 7 << 1 | 1 << 4);             /* always, update */
    quad(-1.0f, -1.0f, 0.0f, 1.0f, RED, -1.0f, 1.0f);
    noblend();
    bp(0x40, 1 | 3 << 1 | 1 << 4);
    void *zc = ram(texdec_size(TEXDEC_I8, 106, 160));
    bp(0x43, 3);                               /* pixel format Z24 during the copy */
    copy_regs(0, 320, 106, 160, zc, 1u << 4);  /* Z8 */
    B->efb_copy(B->ctx, &R, 0, 0);
    bp(0x43, 0);
    texmap(0, zc, 106, 160, TEXDEC_I8, 0);
    modulate();
    quad(0.05f, -1.0f, 1.0f, 1.0f, WHITE);

    /* 7: points and lines */
    tile(7);
    passclr();
    bp(0x22, 6 | (6 * 8) << 8);                /* 8 px points */
    {
        GpuVertex p[8];
        for (int i = 0; i < 8; i++) {
            const float *c = i % 3 == 0 ? RED : i % 3 == 1 ? GREEN : BLUE;
            vtx(&p[i], -0.8f + i * 0.22f, 0.5f, 0, c, 0, 0);
        }
        B->draw(B->ctx, &R, GPU_POINTS, p, 8);
        GpuVertex l[8];
        for (int i = 0; i < 4; i++) {
            vtx(&l[i * 2], -0.9f, -0.9f + i * 0.3f, 0, WHITE, 0, 0);
            vtx(&l[i * 2 + 1], 0.9f, -0.2f + i * 0.1f, 0, WHITE, 0, 0);
        }
        B->draw(B->ctx, &R, GPU_LINES, l, 8);
    }

    /* 8: copy with clear of a sub-rectangle */
    tile(8);
    passclr();
    quad(-1.0f, -1.0f, 1.0f, 1.0f, GREEN);
    {
        void *dummy = ram(texdec_size(TEXDEC_RGBA8, 100, 60));
        copy_regs(426 + 50, 320 + 50, 100, 60, dummy, 6u << 4);
        bp(0x4F, 255 | 255 << 8);
        bp(0x50, 0 << 8 | 255);
        B->efb_copy(B->ctx, &R, 0, 1);
    }
}
