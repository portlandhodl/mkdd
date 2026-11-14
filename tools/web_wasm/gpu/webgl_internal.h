/* Internals shared by the WebGL2 backend sources (webgl_*.cpp). */
#ifndef PORT_GPU_WEBGL_INTERNAL_H
#define PORT_GPU_WEBGL_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <GLES3/gl3.h>
#include "gpu.h"
#include "webgl_backend.h"

#define WGL_EFB_W 640
#define WGL_EFB_H 528

/* Graphics settings (webgl_backend.cpp). The defaults are console-accurate; the page
 * (tools/web_wasm/web/index.html) sets MKDD_GFX_* in Module.env and bumps Module.gfxVersion to
 * apply changes at the next present. */
struct WglGfx {
    int scale;      /* internal resolution in use: the EFB, EFB copies and XFB are scale x native */
    int scaleSet;   /* MKDD_GFX_SCALE: 1-4, 0 = from the canvas size (auto) */
    int msaa;       /* MKDD_GFX_MSAA: samples (0/1 off, 2, 4, 8) */
    int aniso;      /* MKDD_GFX_ANISO: max anisotropy (1 = off) */
    int texFilter;  /* MKDD_GFX_TEXFILTER: 0 as the game sets it, 1 trilinear mips, 2 force linear */
    int fxaa;       /* MKDD_GFX_FXAA: FXAA at present */
    int present;    /* MKDD_GFX_PRESENT: 0 smooth, 1 integer (pixel-exact) */
    int aspect;     /* MKDD_GFX_ASPECT: 0 4:3, 1 stretch to the canvas */
    int widescreen; /* MKDD_GFX_WIDESCREEN: the game renders anamorphic 16:9 (tools/web_wasm/sdk/vi_pad.c), shown at 16:9 */
    int deflicker;  /* MKDD_DEFLICKER: the copy filter's vertical taps */
    int shaderWarm; /* MKDD_GFX_SHADER_WARM: compile remembered TEV programs at start (default 1) */
};
extern WglGfx wgl_gfx;
extern float wgl_max_aniso; /* 0 = EXT_texture_filter_anisotropic unavailable */

/* Register state a batch is drawn with. Copied from GpuRegs when the state
 * changes, so pending draws keep the state they were issued with. */
struct WglSnap {
    uint32_t bp[0x100];   /* BP registers; registers that don't affect drawing are zeroed */
    uint32_t creg[8];     /* TEV color registers: [2i] = R/A (0xE0+2i), [2i+1] = B/G */
    uint32_t konst[8];    /* TEV konst colors, same layout */
    float vp[6];          /* XF viewport: scale x/y/z, offset x/y/z (0x101A-0x101F) */
};

/* ---- bit helpers ---- */
static inline uint32_t wgl_bits(uint32_t v, int lo, int n) { return (v >> lo) & ((1u << n) - 1u); }

/* ---- program (TEV shader) cache: webgl_tev.cpp ---- */

#define WGL_KEY_WORDS 96

struct WglKey {
    uint32_t w[WGL_KEY_WORDS];
    int n;
};

enum { WGL_ALPHAOUT_NORMAL = 0, WGL_ALPHAOUT_CONST = 1 };
enum { WGL_SRC_NORMAL = 0, WGL_SRC_INVERT = 1, WGL_SRC_ONE = 2 };

struct WglProgram {
    GLuint prog;
    uint32_t usedMaps;     /* bitmask of texture maps sampled */
    uint32_t usedCoords;   /* bitmask of texcoords read */
    GLint uReg, uKon, uTSize, uSu, uLodBias, uAlphaRef, uFog, uFogI, uFogK, uFogColor, uDstAlpha;
    GLint uIndM, uIndSS, uIScale, uZBias;
    GLint uVp, uDepth, uPointSize;
};

/* Builds the key for `s` (alphaOut / srcMode come from the blend setup; quant: bit 0
 * truncate the color output to 6 bits, bit 1 the alpha output, for an RGBA6 EFB). */
void wgl_tev_key(WglKey *key, const WglSnap *s, int alphaOut, int srcMode, int quant);
/* Looks up or compiles the program for `key`; NULL on failure. */
WglProgram *wgl_tev_program(const WglKey *key);
void wgl_tev_init(void);
/* Program warm-up: compiles the TEV programs remembered from earlier sessions
 * (browser localStorage) without waiting for them; wgl_tev_poll finishes the ones the
 * driver has completed (KHR_parallel_shader_compile) and saves newly seen keys. */
void wgl_tev_warm(int parallel);
void wgl_tev_poll(void);
extern int wgl_debug;

/* ---- textures: webgl_texture.cpp ---- */

struct WglTexInfo {
    GLuint tex;
    int w, h;       /* level 0 size */
    int levels;
    int isCopy;     /* an EFB copy */
    int arbitraryMips; /* mip levels with their own content (gets +log2(scale) LOD bias) */
};

void wgl_tex_init(void);
/* Texture for map `map` (0-7) in state `s`; binds it to unit `map` and sets
 * its sampler state. Fills `out` (size used for coordinate scaling). */
void wgl_tex_bind(const WglSnap *s, int map, WglTexInfo *out);
/* Register an EFB copy (GL texture owned by the texture module afterwards). */
void wgl_tex_register_copy(uint32_t addr, int fmt, int w, int h, GLuint tex);
/* Find (or make) the GL texture for an EFB copy of w x h to `addr` (allocated at
 * w * scale x h * scale texels; sampling uses normalised coordinates). */
GLuint wgl_tex_copy_target(uint32_t addr, int fmt, int w, int h, int scale);
void wgl_tex_settings_changed(void); /* sampler settings (filtering, anisotropy) changed */
void wgl_tex_new_frame(void);
void wgl_tex_forget_bindings(void); /* after binding textures outside wgl_tex_bind */
void wgl_tex_restore_samplers(void); /* rebind the per-unit sampler objects */

/* ---- shared helpers: webgl_backend.cpp ---- */

GLuint wgl_compile(const char *vs, const char *fs, const char *what);
GLuint wgl_shader(GLenum type, const char *src, const char *what);
extern GLuint wgl_main_vs;      /* the vertex shader shared by all TEV programs */
extern uint32_t wgl_frame;      /* frame counter (incremented by present) */
extern GpuWebglStats wgl_stats_cur;
const uint8_t *wgl_ram(uint32_t addr, uint32_t size); /* NULL if out of range */
void wgl_log(const char *fmt, ...);

#endif
