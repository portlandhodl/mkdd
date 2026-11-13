/* WebGL2 rendering backend for the emulated GPU (see gpu.h).
 *
 * Usage (browser build only; under node gpu_webgl_create() returns NULL):
 *
 *     const GpuBackend *be = gpu_webgl_create();   // after gpu_init()
 *     gpu_set_backend(be);                          // NULL keeps the null backend
 *
 * The backend renders into an offscreen EFB (640x528) and draws XFB copies to
 * the page canvas "#screen" in present(). It reads texture images from main
 * RAM (physical address == host pointer) and palettes from gpu_tmem(). */
#ifndef PORT_GPU_WEBGL_BACKEND_H
#define PORT_GPU_WEBGL_BACKEND_H

#include "gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create the WebGL2 context on the canvas with id "screen" and return the
 * backend, or NULL when there is no DOM/canvas or WebGL2 is unavailable.
 * Calling it again returns the same backend. */
const GpuBackend *gpu_webgl_create(void);

/* Per-frame statistics of the backend (previous completed frame). */
typedef struct GpuWebglStats {
    uint32_t flushes, drawCalls, programsCompiled, programsCached;
    uint32_t texUploads, texCached, texHashedBytes, efbCopies, xfbCopies;
} GpuWebglStats;
const GpuWebglStats *gpu_webgl_stats(void);

#ifdef __cplusplus
}
#endif

#endif
