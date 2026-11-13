/* GameCube texture decoder: GX texture formats -> RGBA8.
 *
 * Source texels and palettes are big-endian, exactly as in hardware memory.
 * Output is tightly packed RGBA8 (4 bytes per pixel, row-major, top row
 * first), straight (non-premultiplied) alpha. No allocation; the caller owns
 * every buffer. Usable from C and C++. */
#ifndef PORT_GPU_TEXDEC_H
#define PORT_GPU_TEXDEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Format values are GXTexFmt / GXTlutFmt (include/dolphin/gx/GXEnum.h):
 *   0x0 I4      0x1 I8      0x2 IA4     0x3 IA8     0x4 RGB565  0x5 RGB5A3
 *   0x6 RGBA8   0x8 C4      0x9 C8      0xA C14X2   0xE CMPR
 * EFB-copy formats (GX_CTF_*, 0x20 | n) and Z formats (0x10 | n) are accepted
 * and decoded with the texture layout they alias (texdec_layout()):
 *   CTF_R4 -> I4, CTF_RA4 -> IA4, CTF_RA8/RG8/GB8 -> IA8,
 *   CTF_A8/R8/G8/B8 -> I8, CTF_YUVA8 -> RGBA8, Z8 -> I8, Z16 -> IA8,
 *   Z24X8 -> RGBA8.  (The texture unit only sees the aliased layout, so e.g.
 *   a CTF_RG8 copy samples as IA8 with I = R and A = G.)
 * TLUT formats: 0 IA8, 1 RGB565, 2 RGB5A3. */

enum {
    TEXDEC_I4 = 0x0, TEXDEC_I8 = 0x1, TEXDEC_IA4 = 0x2, TEXDEC_IA8 = 0x3,
    TEXDEC_RGB565 = 0x4, TEXDEC_RGB5A3 = 0x5, TEXDEC_RGBA8 = 0x6,
    TEXDEC_C4 = 0x8, TEXDEC_C8 = 0x9, TEXDEC_C14X2 = 0xA, TEXDEC_CMPR = 0xE
};

enum { TEXDEC_TL_IA8 = 0, TEXDEC_TL_RGB565 = 1, TEXDEC_TL_RGB5A3 = 2 };

/* Texture layout that `fmt` is decoded as (one of the TEXDEC_* values above),
 * or -1 if the format is unknown. */
int texdec_layout(int fmt);

/* Tile size in texels (e.g. 8x8 for I4/C4/CMPR, 8x4 for 8bpp, 4x4 for 16bpp
 * and RGBA8) and bytes per tile (32, or 64 for RGBA8). Returns 0 if unknown. */
int texdec_tile(int fmt, int *tileW, int *tileH, int *tileBytes);

/* Bytes of one encoded level of w x h texels (padded to whole tiles). */
uint32_t texdec_size(int fmt, int w, int h);

/* Byte offset of mip `level` in a chain whose level 0 is w x h (each level
 * halves both sizes, minimum 1, as GXGetTexBufferSize does). level 0 -> 0. */
uint32_t texdec_mip_offset(int fmt, int w, int h, int level);

/* Total bytes of a chain of `levels` levels (texdec_mip_offset(.., levels)). */
uint32_t texdec_chain_size(int fmt, int w, int h, int levels);

/* Dimensions of mip `level` (max(1, w >> level)). */
int texdec_mip_dim(int dim, int level);

/* Decode one level to outRGBA (w * h * 4 bytes). `src` must hold
 * texdec_size(fmt, w, h) bytes. For C4/C8/C14X2, `tlut` points at tlutCount
 * big-endian 16-bit entries in format tlutFmt; indices past tlutCount (or a
 * null tlut) decode as transparent black. Returns 0 on success, -1 for an
 * unknown format or bad size (outRGBA is then untouched). */
int texdec_decode(uint8_t *outRGBA, const uint8_t *src, int fmt, int w, int h,
                  const uint8_t *tlut, int tlutFmt, int tlutCount);

/* Decode one 16-bit big-endian palette entry (or IA8/RGB565/RGB5A3 texel
 * value) to RGBA8. */
void texdec_color16(uint8_t out[4], uint16_t v, int tlutFmt);

#ifdef __cplusplus
}
#endif

#endif
