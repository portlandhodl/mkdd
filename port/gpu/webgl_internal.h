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
    GLint uReg, uKon, uTSize, uSu, uLodBias, uAlphaRef, uFog, uFogColor, uDstAlpha;
    GLint uIndM, uIndSS, uIScale, uZBias;
    GLint uVp, uDepth, uPointSize;
};

/* Builds the key for `s` (alphaOut / srcMode come from the blend setup). */
void wgl_tev_key(WglKey *key, const WglSnap *s, int alphaOut, int srcMode);
/* Looks up or compiles the program for `key`; NULL on failure. */
WglProgram *wgl_tev_program(const WglKey *key);
void wgl_tev_init(void);
extern int wgl_debug;

/* ---- textures: webgl_texture.cpp ---- */

struct WglTexInfo {
    GLuint tex;
    int w, h;       /* level 0 size */
    int levels;
    int isCopy;     /* an EFB copy */
};

void wgl_tex_init(void);
/* Texture for map `map` (0-7) in state `s`; binds it to unit `map` and sets
 * its sampler state. Fills `out` (size used for coordinate scaling). */
void wgl_tex_bind(const WglSnap *s, int map, WglTexInfo *out);
/* Register an EFB copy (GL texture owned by the texture module afterwards). */
void wgl_tex_register_copy(uint32_t addr, int fmt, int w, int h, GLuint tex);
/* Find (or make) the GL texture for an EFB copy of w x h to `addr`. */
GLuint wgl_tex_copy_target(uint32_t addr, int fmt, int w, int h);
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
