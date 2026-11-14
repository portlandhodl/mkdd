/* GameCube texture decoder. See texdec.h. */
#include "texdec.h"

namespace {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

inline u16 be16(const u8 *p) { return (u16)((p[0] << 8) | p[1]); }

inline u8 x4(u32 v) { return (u8)(v * 0x11); }
inline u8 x5(u32 v) { return (u8)((v << 3) | (v >> 2)); }
inline u8 x6(u32 v) { return (u8)((v << 2) | (v >> 4)); }
inline u8 x3(u32 v) { return (u8)((v << 5) | (v << 2) | (v >> 1)); }

inline void set(u8 *o, u8 r, u8 g, u8 b, u8 a)
{
    o[0] = r;
    o[1] = g;
    o[2] = b;
    o[3] = a;
}

inline void color16(u8 *o, u16 v, int fmt)
{
    switch (fmt) {
    case TEXDEC_TL_IA8:
        set(o, (u8)v, (u8)v, (u8)v, (u8)(v >> 8));
        break;
    case TEXDEC_TL_RGB565:
        set(o, x5(v >> 11), x6((v >> 5) & 0x3F), x5(v & 0x1F), 0xFF);
        break;
    case TEXDEC_TL_RGB5A3:
        if (v & 0x8000)
            set(o, x5((v >> 10) & 0x1F), x5((v >> 5) & 0x1F), x5(v & 0x1F), 0xFF);
        else
            set(o, x4((v >> 8) & 0xF), x4((v >> 4) & 0xF), x4(v & 0xF), x3((v >> 12) & 7));
        break;
    default:
        set(o, 0, 0, 0, 0);
        break;
    }
}

struct Palette {
    const u8 *data;
    int fmt;
    int count;
    void lookup(u8 *o, u32 idx) const
    {
        if (data && (int)idx < count)
            color16(o, be16(data + idx * 2), fmt);
        else
            set(o, 0, 0, 0, 0);
    }
};

/* GameCube CMPR: like DXT1, but the 2/3 blend is 5/8:3/8 and the
 * transparent entry keeps the average colour (hardware-verified by Dolphin). */
void decode_cmpr_block(u8 *out, int w, int h, int bx, int by, const u8 *b)
{
    u16 c0 = be16(b), c1 = be16(b + 2);
    u8 pal[4][4];
    u32 r0 = x5(c0 >> 11), g0 = x6((c0 >> 5) & 0x3F), b0 = x5(c0 & 0x1F);
    u32 r1 = x5(c1 >> 11), g1 = x6((c1 >> 5) & 0x3F), b1 = x5(c1 & 0x1F);
    set(pal[0], (u8)r0, (u8)g0, (u8)b0, 0xFF);
    set(pal[1], (u8)r1, (u8)g1, (u8)b1, 0xFF);
    if (c0 > c1) {
        set(pal[2], (u8)((r0 * 5 + r1 * 3) >> 3), (u8)((g0 * 5 + g1 * 3) >> 3),
            (u8)((b0 * 5 + b1 * 3) >> 3), 0xFF);
        set(pal[3], (u8)((r1 * 5 + r0 * 3) >> 3), (u8)((g1 * 5 + g0 * 3) >> 3),
            (u8)((b1 * 5 + b0 * 3) >> 3), 0xFF);
    } else {
        set(pal[2], (u8)((r0 + r1) >> 1), (u8)((g0 + g1) >> 1), (u8)((b0 + b1) >> 1), 0xFF);
        set(pal[3], (u8)((r0 + r1) >> 1), (u8)((g0 + g1) >> 1), (u8)((b0 + b1) >> 1), 0x00);
    }
    for (int y = 0; y < 4; y++) {
        int py = by + y;
        u8 bits = b[4 + y];
        if (py >= h)
            break;
        for (int x = 0; x < 4; x++) {
            int px = bx + x;
            if (px >= w)
                break;
            const u8 *c = pal[(bits >> (6 - x * 2)) & 3];
            set(out + ((u32)py * w + px) * 4, c[0], c[1], c[2], c[3]);
        }
    }
}

} // namespace

extern "C" {

int texdec_layout(int fmt)
{
    switch (fmt) {
    case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x6:
    case 0x8: case 0x9: case 0xA: case 0xE:
        return fmt;
    case 0x20: return TEXDEC_I4;    /* CTF_R4 */
    case 0x22: return TEXDEC_IA4;   /* CTF_RA4 */
    case 0x23: return TEXDEC_IA8;   /* CTF_RA8 */
    case 0x26: return TEXDEC_RGBA8; /* CTF_YUVA8 */
    case 0x27: return TEXDEC_I8;    /* CTF_A8 / TF_A8 */
    case 0x28: return TEXDEC_I8;    /* CTF_R8 */
    case 0x29: return TEXDEC_I8;    /* CTF_G8 */
    case 0x2A: return TEXDEC_I8;    /* CTF_B8 */
    case 0x2B: return TEXDEC_IA8;   /* CTF_RG8 */
    case 0x2C: return TEXDEC_IA8;   /* CTF_GB8 */
    case 0x11: return TEXDEC_I8;    /* TF_Z8 */
    case 0x13: return TEXDEC_IA8;   /* TF_Z16 */
    case 0x16: return TEXDEC_RGBA8; /* TF_Z24X8 */
    default: return -1;
    }
}

int texdec_tile(int fmt, int *tileW, int *tileH, int *tileBytes)
{
    int tw, th, tb = 32;
    switch (texdec_layout(fmt)) {
    case TEXDEC_I4: case TEXDEC_C4: case TEXDEC_CMPR:
        tw = 8; th = 8;
        break;
    case TEXDEC_I8: case TEXDEC_IA4: case TEXDEC_C8:
        tw = 8; th = 4;
        break;
    case TEXDEC_IA8: case TEXDEC_RGB565: case TEXDEC_RGB5A3: case TEXDEC_C14X2:
        tw = 4; th = 4;
        break;
    case TEXDEC_RGBA8:
        tw = 4; th = 4; tb = 64;
        break;
    default:
        return 0;
    }
    if (tileW) *tileW = tw;
    if (tileH) *tileH = th;
    if (tileBytes) *tileBytes = tb;
    return 1;
}

uint32_t texdec_size(int fmt, int w, int h)
{
    int tw, th, tb;
    if (!texdec_tile(fmt, &tw, &th, &tb) || w <= 0 || h <= 0)
        return 0;
    return (u32)((w + tw - 1) / tw) * (u32)((h + th - 1) / th) * (u32)tb;
}

int texdec_mip_dim(int dim, int level)
{
    dim >>= level;
    return dim < 1 ? 1 : dim;
}

uint32_t texdec_mip_offset(int fmt, int w, int h, int level)
{
    u32 off = 0;
    for (int i = 0; i < level; i++) {
        off += texdec_size(fmt, w, h);
        w = w > 1 ? w >> 1 : 1;
        h = h > 1 ? h >> 1 : 1;
    }
    return off;
}

uint32_t texdec_chain_size(int fmt, int w, int h, int levels)
{
    return texdec_mip_offset(fmt, w, h, levels);
}

void texdec_color16(uint8_t out[4], uint16_t v, int tlutFmt)
{
    color16(out, v, tlutFmt);
}

int texdec_decode(uint8_t *out, const uint8_t *src, int fmt, int w, int h,
                  const uint8_t *tlut, int tlutFmt, int tlutCount)
{
    int layout = texdec_layout(fmt);
    int tw, th, tb;
    if (!texdec_tile(fmt, &tw, &th, &tb) || w <= 0 || h <= 0)
        return -1;
    Palette pal = { tlut, tlutFmt, tlutCount };
    const u8 *tile = src;

    for (int ty = 0; ty < h; ty += th) {
        for (int tx = 0; tx < w; tx += tw, tile += tb) {
            if (layout == TEXDEC_CMPR) {
                for (int s = 0; s < 4; s++)
                    decode_cmpr_block(out, w, h, tx + (s & 1) * 4, ty + (s >> 1) * 4, tile + s * 8);
                continue;
            }
            for (int y = 0; y < th; y++) {
                int py = ty + y;
                if (py >= h)
                    break;
                for (int x = 0; x < tw; x++) {
                    int px = tx + x;
                    if (px >= w)
                        break;
                    int i = y * tw + x;
                    u8 *o = out + ((u32)py * w + px) * 4;
                    switch (layout) {
                    case TEXDEC_I4: {
                        u8 v = x4((tile[i >> 1] >> ((i & 1) ? 0 : 4)) & 0xF);
                        set(o, v, v, v, v);
                        break;
                    }
                    case TEXDEC_I8:
                        set(o, tile[i], tile[i], tile[i], tile[i]);
                        break;
                    case TEXDEC_IA4: {
                        u8 v = x4(tile[i] & 0xF);
                        set(o, v, v, v, x4(tile[i] >> 4));
                        break;
                    }
                    case TEXDEC_IA8:
                        color16(o, be16(tile + i * 2), TEXDEC_TL_IA8);
                        break;
                    case TEXDEC_RGB565:
                        color16(o, be16(tile + i * 2), TEXDEC_TL_RGB565);
                        break;
                    case TEXDEC_RGB5A3:
                        color16(o, be16(tile + i * 2), TEXDEC_TL_RGB5A3);
                        break;
                    case TEXDEC_RGBA8:
                        set(o, tile[i * 2 + 1], tile[32 + i * 2], tile[32 + i * 2 + 1], tile[i * 2]);
                        break;
                    case TEXDEC_C4:
                        pal.lookup(o, (tile[i >> 1] >> ((i & 1) ? 0 : 4)) & 0xF);
                        break;
                    case TEXDEC_C8:
                        pal.lookup(o, tile[i]);
                        break;
                    case TEXDEC_C14X2:
                        pal.lookup(o, be16(tile + i * 2) & 0x3FFF);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

} // extern "C"
