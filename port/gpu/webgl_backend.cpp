/* WebGL2 rendering backend for the emulated GPU: EFB render target, pixel
 * pipeline state from the BP registers, batching, EFB copies and presenting
 * the XFB to the page canvas. Shaders: webgl_tev.cpp, textures:
 * webgl_texture.cpp.
 *
 * Conventions: the EFB is a 640x528 RGBA8 + D24S8 framebuffer whose GL row r
 * holds EFB row r (top row first), so textures, EFB copies and the EFB share
 * one orientation; only present() flips. The vertex shader applies the XF
 * viewport (incl. the 342 offset and the scissor-box offset) and the depth
 * range itself, so glViewport always covers the whole EFB. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/heap.h>
#include "webgl_internal.h"

GLuint wgl_main_vs;
uint32_t wgl_frame = 1;
GpuWebglStats wgl_stats_cur;
static GpuWebglStats sStatsPrev;

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE sCtx;
static GpuBackend sBackend;
static int sReady;

/* EFB */
static GLuint sEfbFbo, sEfbColor, sEfbDepth;
/* streaming vertices */
#define RING_VERTS (1 << 16)
#define STAGE_VERTS (1 << 15)
static GLuint sVao, sVbo;
static uint32_t sRingPos;
static GpuVertex *sStage;
static uint32_t sStageCount;
static int sStagePrim;
static WglSnap sSnap, sTmpSnap;
static int sSnapValid;
/* copies / present */
static GLuint sCopyProg, sPresentProg, sEmptyVao, sCopyFbo;
static GLint uCopyRect, uCopyHalf, uCopyDepth, uCopyPixFmt, uCopyLayout, uCopyISel, uCopyASel, uCopyZRgb, uCopyXfb,
    uCopyGamma, uCopyVf, uCopyDst;
static GLint uPresentRect;

#define MAX_XFB 4
struct XfbEntry {
    uint32_t addr, stride, lastUsed;
    int w, h;
    GLuint tex;
};
static XfbEntry sXfb[MAX_XFB];
static uint32_t sXfbSeq;

/* MKDD_WGL_TRACE=N (?wgltrace=N): log every batch, EFB copy and present of frame N (and
 * texcoord-scale mismatches) to the console. MKDD_WGL_PICK=x*1000+y: log every batch
 * that changes EFB pixel (x, y), or covers it without changing it (handy with the FIFO
 * log player, port/gpu/tools/dffplay.*). MKDD_WGL_DEBUG: see wgl_debug. */
extern "C" int port_env(const char *name);
static int sTraceFrame, sDebug;
/* MKDD_DEFLICKER: apply the copy filter's vertical taps (console behaviour; Dolphin's
 * default and ours leave them out, which keeps the picture sharp) */
static int sDeflicker;

void wgl_log(const char *fmt, ...)
{
    static int count;
    if (count++ > 200) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

const uint8_t *wgl_ram(uint32_t addr, uint32_t size)
{
    size_t heap = emscripten_get_heap_size();
    if (addr < 0x1000 || (size_t)addr + size > heap) return NULL;
    return (const uint8_t *)(uintptr_t)addr;
}

GLuint wgl_shader(GLenum type, const char *src, const char *what)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof log, NULL, log);
        wgl_log("[webgl] %s shader failed: %s\n%s", what, log, src);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint wgl_compile(const char *vs, const char *fs, const char *what)
{
    GLuint v = wgl_shader(GL_VERTEX_SHADER, vs, what), f = wgl_shader(GL_FRAGMENT_SHADER, fs, what);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof log, NULL, log);
        wgl_log("[webgl] %s link failed: %s", what, log);
        return 0;
    }
    return p;
}

/* ---- shaders ---- */

static const char *kMainVs =
    "#version 300 es\n"
    "in vec4 a_pos; in vec4 a_c0; in vec4 a_c1;\n"
    "in vec3 a_t0; in vec3 a_t1; in vec3 a_t2; in vec3 a_t3; in vec3 a_t4; in vec3 a_t5; in vec3 a_t6; in vec3 a_t7;\n"
    "uniform vec4 u_vp;\n"     /* scale x, scale y, offset x, offset y in EFB pixels */
    "uniform vec2 u_depth;\n"  /* offset z, scale z, normalised to [0,1] */
    "uniform float u_psize;\n"
    "out vec4 v_c0; out vec4 v_c1;\n"
    "out vec3 v_t0; out vec3 v_t1; out vec3 v_t2; out vec3 v_t3; out vec3 v_t4; out vec3 v_t5; out vec3 v_t6; out vec3 v_t7;\n"
    "void main() {\n"
    "  vec4 p = a_pos;\n"
    /* the console samples pixels at (x + 7/12, y + 7/12), GL at (x + 1/2, y + 1/2) */
    "  float x = (u_vp.x * p.x + (u_vp.z - 1.0 / 12.0) * p.w) * (2.0 / 640.0) - p.w;\n"
    "  float y = (u_vp.y * p.y + (u_vp.w - 1.0 / 12.0) * p.w) * (2.0 / 528.0) - p.w;\n"
    "  float d = u_depth.x * p.w + u_depth.y * 0.5 * (p.z - p.w);\n"
    "  gl_Position = vec4(x, y, 2.0 * d - p.w, p.w);\n"
    "  gl_PointSize = u_psize;\n"
    "  v_c0 = a_c0; v_c1 = a_c1;\n"
    "  v_t0 = a_t0; v_t1 = a_t1; v_t2 = a_t2; v_t3 = a_t3; v_t4 = a_t4; v_t5 = a_t5; v_t6 = a_t6; v_t7 = a_t7;\n"
    "}\n";

static const char *kQuadVs =
    "#version 300 es\n"
    "void main() { vec2 p = vec2(float((gl_VertexID & 1) * 4 - 1), float((gl_VertexID & 2) * 2 - 1));\n"
    "  gl_Position = vec4(p, 0.0, 1.0); }\n";

/* EFB -> texture / XFB conversion. Writes the texture the way the texture unit
 * would sample the copy's format (layout + channel selection). */
static const char *kCopyFs =
    "#version 300 es\n"
    "precision highp float; precision highp int;\n"
    "uniform highp sampler2D u_col; uniform highp sampler2D u_dep;\n"
    "uniform ivec4 u_rect; uniform ivec2 u_dst;\n"
    "uniform int u_half, u_depthsrc, u_pixfmt, u_layout, u_isel, u_asel, u_zrgb, u_xfb;\n"
    "uniform float u_gamma; uniform uvec3 u_vf;\n"
    "out vec4 o;\n"
    "vec4 fc(ivec2 p) { p = clamp(p, u_rect.xy, u_rect.xy + u_rect.zw - 1); return texelFetch(u_col, p, 0); }\n"
    "float fz(ivec2 p) { p = clamp(p, u_rect.xy, u_rect.xy + u_rect.zw - 1); return texelFetch(u_dep, p, 0).r; }\n"
    "float q(float v, float bits) { float m = exp2(bits) - 1.0; return floor(v * m + 0.5) / m; }\n"
    /* One source row of the copy as 0..255 integers (depth: the Z bytes high, mid, low), the
     * way Dolphin samples it: a half-size copy averages the 2x2 block (bilinear at its centre) */
    "uvec4 row(ivec2 p, int r) {\n"
    "  p.y += r;\n"
    "  if (u_depthsrc != 0) {\n"
    "    float z = u_half != 0 ? (fz(p) + fz(p + ivec2(1, 0)) + fz(p + ivec2(0, 1)) + fz(p + ivec2(1, 1))) * 0.25 : fz(p);\n"
    "    uint zi = min(uint(clamp(z, 0.0, 1.0) * 16777216.0), 16777215u);\n"
    "    return uvec4((zi >> 16) & 255u, (zi >> 8) & 255u, zi & 255u, 255u);\n"
    "  }\n"
    "  vec4 c = u_half != 0 ? (fc(p) + fc(p + ivec2(1, 0)) + fc(p + ivec2(0, 1)) + fc(p + ivec2(1, 1))) * 0.25 : fc(p);\n"
    "  return uvec4(clamp(c, 0.0, 1.0) * 255.0 + 0.001);\n"
    "}\n"
    "void main() {\n"
    "  ivec2 d = ivec2(gl_FragCoord.xy) - u_dst;\n"
    "  ivec2 p = u_rect.xy + (u_half != 0 ? d * 2 : d);\n"
    /* vertical (deflicker) filter: integer weights for the rows above, at and below,
     * normally summing to 64 */
    "  uvec4 cur = row(p, 0);\n"
    "  uvec3 sum = cur.rgb * u_vf.y;\n"
    "  if (u_vf.x != 0u) sum += row(p, -1).rgb * u_vf.x;\n"
    "  if (u_vf.z != 0u) sum += row(p, 1).rgb * u_vf.z;\n"
    "  uvec4 ci = uvec4(min(sum >> 6, uvec3(255u)), u_pixfmt == 1 ? cur.a : 255u);\n"
    "  if (u_depthsrc != 0) {\n"
    "    float zh = float(ci.r) / 255.0, zm = float(ci.g) / 255.0, zl = float(ci.b) / 255.0;\n"
    "    float ch[9] = float[9](zh, zm, zl, 1.0, zh, zh, zm, zl, 1.0);\n"
    "    float I = ch[u_isel], A = ch[u_asel];\n"
    "    if (u_layout == 0) o = vec4(q(I, 4.0));\n"
    "    else if (u_layout == 1) o = vec4(I);\n"
    "    else if (u_layout == 3) o = vec4(vec3(I), A);\n"
    "    else o = vec4(zh, zm, zl, 1.0);\n"
    "    return;\n"
    "  }\n"
    "  vec4 c = vec4(ci) / 255.0;\n"
    "  if (u_pixfmt == 2) c = vec4(q(c.r, 5.0), q(c.g, 6.0), q(c.b, 5.0), 1.0);\n"
    "  if (u_xfb != 0) { o = vec4(pow(c.rgb, vec3(1.0 / u_gamma)), 1.0); return; }\n"
    "  float y = clamp(0.257 * c.r + 0.504 * c.g + 0.098 * c.b + 16.0 / 255.0, 0.0, 1.0);\n"
    "  float ch[9] = float[9](c.r, c.g, c.b, c.a, y, 0.0, 0.0, 0.0, 1.0);\n"
    "  float I = ch[u_isel], A = ch[u_asel];\n"
    "  vec4 rgba = c;\n"
    "  if (u_layout == 0) { I = q(I, 4.0); o = vec4(I); }\n"
    "  else if (u_layout == 1) o = vec4(I);\n"
    "  else if (u_layout == 2) o = vec4(vec3(q(I, 4.0)), q(A, 4.0));\n"
    "  else if (u_layout == 3) o = vec4(vec3(I), A);\n"
    "  else if (u_layout == 4) o = vec4(q(rgba.r, 5.0), q(rgba.g, 6.0), q(rgba.b, 5.0), 1.0);\n"
    "  else if (u_layout == 5) {\n"
    "    if (rgba.a >= 224.0 / 255.0) o = vec4(q(rgba.r, 5.0), q(rgba.g, 5.0), q(rgba.b, 5.0), 1.0);\n"
    "    else o = vec4(q(rgba.r, 4.0), q(rgba.g, 4.0), q(rgba.b, 4.0), q(rgba.a, 3.0));\n"
    "  } else o = rgba;\n"
    "}\n";

static const char *kPresentFs =
    "#version 300 es\n"
    "precision highp float;\n"
    "uniform sampler2D u_tex; uniform vec4 u_rect;\n" /* texture-space x, y, w, h (normalised) */
    "uniform vec2 u_canvas;\n"
    "out vec4 o;\n"
    "void main() {\n"
    "  vec2 uv = gl_FragCoord.xy / u_canvas; uv.y = 1.0 - uv.y;\n"
    "  o = vec4(texture(u_tex, u_rect.xy + uv * u_rect.zw).rgb, 1.0);\n"
    "}\n";

/* ---- register snapshot ---- */

static void sign_extend_11(int *v) { *v = (*v & 0x400) ? (*v | ~0x7FF) : (*v & 0x7FF); }

static void capture(WglSnap *s, const GpuRegs *regs)
{
    memcpy(s->bp, regs->bp, sizeof s->bp);
    /* registers that don't affect rasterisation */
    static const uint8_t ignore[] = {
        0x01, 0x02, 0x03, 0x04, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52,
        0x53, 0x54, 0x55, 0x56, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92,
        0x93, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xFE, 0xFF,
    };
    for (size_t i = 0; i < sizeof ignore; i++) s->bp[ignore[i]] = 0;
    /* The TEV color registers and the konst colors share BP 0xE0-0xE7; the
     * front end keeps them apart. */
    for (int i = 0xE0; i < 0xE8; i++) s->bp[i] = 0;
    memcpy(s->creg, regs->tevColor, sizeof s->creg);
    memcpy(s->konst, regs->tevKonst, sizeof s->konst);
    for (int i = 0; i < 6; i++) memcpy(&s->vp[i], &regs->xf[0x1A + i], 4);
}

/* ---- GL state helpers ---- */

static const GLenum kCompare[8] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };

static void bind_efb(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, sEfbFbo);
    glViewport(0, 0, WGL_EFB_W, WGL_EFB_H);
}

static void scissor_offset(const uint32_t *bp, int *ox, int *oy)
{
    uint32_t r = bp[0x59];
    if (!r) { *ox = 342; *oy = 342; return; }
    *ox = (int)(r & 0x3FF) * 2;
    *oy = (int)((r >> 10) & 0x3FF) * 2;
}

struct BlendSetup {
    int enable;
    GLenum eq, src, dst;
    float constant;
    int srcMode;
};

static GLenum gl_factor(int f, int isDst, int hasAlpha)
{
    switch (f) {
    case 0: return GL_ZERO;
    case 1: return GL_ONE;
    case 2: return isDst ? GL_SRC_COLOR : GL_DST_COLOR;
    case 3: return isDst ? GL_ONE_MINUS_SRC_COLOR : GL_ONE_MINUS_DST_COLOR;
    case 4: return GL_SRC_ALPHA;
    case 5: return GL_ONE_MINUS_SRC_ALPHA;
    case 6: return hasAlpha ? GL_DST_ALPHA : GL_ONE;
    default: return hasAlpha ? GL_ONE_MINUS_DST_ALPHA : GL_ZERO;
    }
}

static void blend_setup(const uint32_t *bp, BlendSetup *b)
{
    uint32_t cm0 = bp[0x41];
    int hasAlpha = (bp[0x43] & 7) == 1;
    b->enable = 1;
    b->eq = GL_FUNC_ADD;
    b->src = GL_ONE;
    b->dst = GL_ZERO;
    b->constant = 1.0f;
    b->srcMode = WGL_SRC_NORMAL;
    if (cm0 & 1) {
        if (cm0 & 0x800) {
            b->eq = GL_FUNC_REVERSE_SUBTRACT;
            b->src = GL_ONE;
            b->dst = GL_ONE;
        } else {
            b->src = gl_factor((int)((cm0 >> 8) & 7), 0, hasAlpha);
            b->dst = gl_factor((int)((cm0 >> 5) & 7), 1, hasAlpha);
        }
    } else if (cm0 & 2) {
        /* Logic ops approximated with blending (exact for 0/1 channel values). */
        switch ((cm0 >> 12) & 15) {
        case 0: b->src = GL_ZERO; b->dst = GL_ZERO; break;                                   /* clear */
        case 1: b->src = GL_DST_COLOR; b->dst = GL_ZERO; break;                              /* and */
        case 2: b->src = GL_ONE_MINUS_DST_COLOR; b->dst = GL_ZERO; break;                    /* s & ~d */
        case 3: b->enable = 0; break;                                                        /* copy */
        case 4: b->src = GL_ZERO; b->dst = GL_ONE_MINUS_SRC_COLOR; break;                    /* ~s & d */
        case 5: b->src = GL_ZERO; b->dst = GL_ONE; break;                                    /* noop */
        case 6: b->src = GL_ONE_MINUS_DST_COLOR; b->dst = GL_ONE_MINUS_SRC_COLOR; break;     /* xor */
        case 7: b->src = GL_ONE; b->dst = GL_ONE_MINUS_SRC_COLOR; break;                     /* or */
        case 8: b->srcMode = WGL_SRC_INVERT; b->src = GL_ONE_MINUS_DST_COLOR; b->dst = GL_ZERO; break; /* nor */
        case 9: b->srcMode = WGL_SRC_INVERT; b->src = GL_ONE_MINUS_DST_COLOR; b->dst = GL_ONE_MINUS_SRC_COLOR; break; /* equiv */
        case 10: b->srcMode = WGL_SRC_ONE; b->src = GL_ONE_MINUS_DST_COLOR; b->dst = GL_ZERO; break; /* ~d */
        case 11: b->enable = 0; break;                                                       /* s | ~d (approx: copy) */
        case 12: b->srcMode = WGL_SRC_INVERT; b->enable = 0; break;                          /* ~s */
        case 13: b->srcMode = WGL_SRC_INVERT; b->src = GL_ONE; b->dst = GL_ONE_MINUS_SRC_COLOR; break; /* ~s | d */
        case 14: b->srcMode = WGL_SRC_INVERT; b->enable = 0; break;                          /* nand (approx: ~s) */
        default: b->src = GL_CONSTANT_COLOR; b->dst = GL_ZERO; break;                        /* set */
        }
    } else {
        b->enable = 0;
    }
}

static void set_uniforms(const WglProgram *p, const WglSnap *s, const WglTexInfo *tex)
{
    const uint32_t *bp = s->bp;
    int ox, oy;
    scissor_offset(bp, &ox, &oy);
    glUniform4f(p->uVp, s->vp[0], s->vp[1], s->vp[3] - (float)ox, s->vp[4] - (float)oy);
    glUniform2f(p->uDepth, s->vp[5] / 16777215.0f, s->vp[2] / 16777215.0f);
    if (p->uPointSize >= 0) {
        float ps = (float)((bp[0x22] >> 8) & 0xFF) / 6.0f;
        glUniform1f(p->uPointSize, ps < 1.0f ? 1.0f : ps);
    }
    int regs[16], kon[16];
    for (int i = 0; i < 4; i++) {
        uint32_t ra = s->creg[i * 2], bg = s->creg[i * 2 + 1];
        int r = (int)(ra & 0x7FF), a = (int)((ra >> 12) & 0x7FF), b = (int)(bg & 0x7FF), g = (int)((bg >> 12) & 0x7FF);
        sign_extend_11(&r);
        sign_extend_11(&g);
        sign_extend_11(&b);
        sign_extend_11(&a);
        regs[i * 4 + 0] = r;
        regs[i * 4 + 1] = g;
        regs[i * 4 + 2] = b;
        regs[i * 4 + 3] = a;
        uint32_t kra = s->konst[i * 2], kbg = s->konst[i * 2 + 1];
        kon[i * 4 + 0] = (int)(kra & 0xFF);
        kon[i * 4 + 1] = (int)((kbg >> 12) & 0xFF);
        kon[i * 4 + 2] = (int)(kbg & 0xFF);
        kon[i * 4 + 3] = (int)((kra >> 12) & 0xFF);
    }
    glUniform4iv(p->uReg, 4, regs);
    glUniform4iv(p->uKon, 4, kon);

    float tsize[16], su[16], lod[8];
    for (int m = 0; m < 8; m++) {
        tsize[m * 2] = (float)(tex[m].w > 0 ? tex[m].w : 1);
        tsize[m * 2 + 1] = (float)(tex[m].h > 0 ? tex[m].h : 1);
        int base = m < 4 ? 0x80 + m : 0xA0 + (m - 4);
        int b = (int)((bp[base] >> 9) & 0xFF);
        if (b & 0x80) b -= 256;
        lod[m] = (float)b / 32.0f;
    }
    /* texcoord scale: SU size registers when set, else the size of the first
     * map sampled with that coordinate (texgen output is normalised) */
    int ntev = (int)((bp[0x00] >> 10) & 15) + 1;
    for (int c = 0; c < 8; c++) {
        su[c * 2] = 1.0f;
        su[c * 2 + 1] = 1.0f;
        int mapFor = -1;
        for (int st = 0; st < ntev && mapFor < 0; st++) {
            uint32_t o = (bp[0x28 + st / 2] >> ((st & 1) * 12)) & 0xFFF;
            if ((o & 0x40) && (int)((o >> 3) & 7) == c) mapFor = (int)(o & 7);
        }
        int nind = (int)((bp[0x00] >> 16) & 7);
        for (int i = 0; i < nind && i < 4 && mapFor < 0; i++)
            if ((int)((bp[0x27] >> (i * 6 + 3)) & 7) == c) mapFor = (int)((bp[0x27] >> (i * 6)) & 7);
        uint32_t ss = bp[0x30 + c * 2], ts = bp[0x31 + c * 2];
        if (sTraceFrame && mapFor >= 0 && ss && ts &&
            ((float)((ss & 0xFFFF) + 1) != tsize[mapFor * 2] || (float)((ts & 0xFFFF) + 1) != tsize[mapFor * 2 + 1])) {
            static int nmis;
            if (nmis < 60) {
                nmis++;
                int base = mapFor < 4 ? 0x80 + mapFor : 0xA0 + (mapFor - 4);
                fprintf(stderr, "[wgl] su mismatch frame %u coord %d map %d: su %ux%u tex %gx%g img0=%06x img3=%06x tref=%06x\n", wgl_frame, c,
                        mapFor, (ss & 0xFFFF) + 1, (ts & 0xFFFF) + 1, tsize[mapFor * 2], tsize[mapFor * 2 + 1], bp[base + 8],
                        bp[base + 0x14], bp[0x28]);
            }
        }
        if (ss && ts && !(sDebug & 1)) {
            su[c * 2] = (float)((ss & 0xFFFF) + 1);
            su[c * 2 + 1] = (float)((ts & 0xFFFF) + 1);
        } else if (mapFor >= 0) {
            su[c * 2] = tsize[mapFor * 2];
            su[c * 2 + 1] = tsize[mapFor * 2 + 1];
        }
    }
    glUniform2fv(p->uTSize, 8, tsize);
    glUniform2fv(p->uSu, 8, su);
    glUniform1fv(p->uLodBias, 8, lod);
    if (p->uAlphaRef >= 0) glUniform2i(p->uAlphaRef, (int)(bp[0xF3] & 0xFF), (int)((bp[0xF3] >> 8) & 0xFF));
    if (p->uDstAlpha >= 0) glUniform1i(p->uDstAlpha, (int)(bp[0x42] & 0xFF));
    if (p->uZBias >= 0) glUniform1i(p->uZBias, (int)(bp[0xF4] & 0xFFFFFF));
    if (p->uFog >= 0) {
        uint32_t ab = (bp[0xEE] & 0xFFFFF) << 12, cb = (bp[0xF1] & 0xFFFFF) << 12;
        float a, c;
        memcpy(&a, &ab, 4);
        memcpy(&c, &cb, 4);
        int range = (int)((bp[0xE8] >> 10) & 1);
        float center = 0.0f, width = 1.0f;
        if (range) {
            /* range adjustment (BP 0xE8-0xED), as Dolphin's PixelShaderManager sets it up */
            float wd = s->vp[0] < 0 ? -s->vp[0] : s->vp[0];
            center = ((float)((int)(bp[0xE8] & 0x3FF) - 342) / (2.0f * wd)) * 2.0f - 1.0f;
            width = 2.0f * wd;
            float k[10];
            for (int i = 0; i < 5; i++) {
                uint32_t r = bp[0xE9 + i];
                k[i * 2] = (float)((r >> 12) & 0xFFF) / 256.0f * 4.0f;
                k[i * 2 + 1] = (float)(r & 0xFFF) / 256.0f * 4.0f;
            }
            if (p->uFogK >= 0) glUniform1fv(p->uFogK, 10, k);
        }
        glUniform4f(p->uFog, a, c, center, width);
        glUniform4i(p->uFogI, (int)(bp[0xEF] & 0xFFFFFF), (int)(bp[0xF0] & 0x1F), (int)((bp[0xF1] >> 20) & 1), range);
        uint32_t fc = bp[0xF2];
        glUniform3f(p->uFogColor, (float)((fc >> 16) & 0xFF) / 255.0f, (float)((fc >> 8) & 0xFF) / 255.0f, (float)(fc & 0xFF) / 255.0f);
    }
    if (p->uIndM >= 0) {
        float m[18], isc[3];
        for (int k = 0; k < 3; k++) {
            uint32_t r0 = bp[0x06 + k * 3], r1 = bp[0x07 + k * 3], r2 = bp[0x08 + k * 3];
            int e = (int)(((r0 >> 22) & 3) | ((r1 >> 22) & 3) << 2 | ((r2 >> 22) & 3) << 4) - 17;
            float sc = 1.0f;
            for (int i = 0; i < (e < 0 ? -e : e); i++) sc = e < 0 ? sc * 0.5f : sc * 2.0f;
            uint32_t col[3] = { r0, r1, r2 };
            for (int j = 0; j < 3; j++) {
                int a = (int)(col[j] & 0x7FF), b = (int)((col[j] >> 11) & 0x7FF);
                sign_extend_11(&a);
                sign_extend_11(&b);
                m[k * 6 + j] = (float)a / 1024.0f * sc;
                m[k * 6 + 3 + j] = (float)b / 1024.0f * sc;
            }
            isc[k] = sc;
        }
        glUniform3fv(p->uIndM, 6, m);
        if (p->uIScale >= 0) glUniform3fv(p->uIScale, 1, isc);
        float ss[8];
        for (int i = 0; i < 4; i++) {
            uint32_t r = bp[i < 2 ? 0x25 : 0x26] >> ((i & 1) * 8);
            ss[i * 2] = 1.0f / (float)(1u << (r & 15));
            ss[i * 2 + 1] = 1.0f / (float)(1u << ((r >> 4) & 15));
        }
        glUniform2fv(p->uIndSS, 4, ss);
    } else if (p->uIndSS >= 0) {
        float ss[8];
        for (int i = 0; i < 4; i++) {
            uint32_t r = bp[i < 2 ? 0x25 : 0x26] >> ((i & 1) * 8);
            ss[i * 2] = 1.0f / (float)(1u << (r & 15));
            ss[i * 2 + 1] = 1.0f / (float)(1u << ((r >> 4) & 15));
        }
        glUniform2fv(p->uIndSS, 4, ss);
    }
}

/* ---- batching ---- */

static int sPick = -1;
static uint8_t sPickLast[4];

static void trace_batch(const WglSnap *s, const WglProgram *p, const WglTexInfo *tex, uint32_t count)
{
    const uint32_t *bp = s->bp;
    uint32_t zm = bp[0x40], cm0 = bp[0x41];
    const GpuVertex *v0 = sStage;
    fprintf(stderr, "[wgl] n=%u prim=%d gen=%06x tref=%06x %06x c0=%06x a0=%06x maps=%x z=%x cm=%x ss0=%06x ts0=%06x"
            " t0=(%.3f %.3f %.3f) c=(%.2f %.2f %.2f %.2f) sc=%x %x pe=%06x at=%06x da=%06x\n",
            count, sStagePrim, bp[0], bp[0x28], bp[0x29], bp[0xC0], bp[0xC1], p->usedMaps, zm, cm0, bp[0x30], bp[0x31],
            v0->tex[0][0], v0->tex[0][1], v0->tex[0][2], v0->color[0][0], v0->color[0][1], v0->color[0][2],
            v0->color[0][3], bp[0x20], bp[0x21], bp[0x43], bp[0xF3], bp[0x42]);
    int nst = (int)((bp[0] >> 10) & 15) + 1;
    if ((bp[0xF1] >> 21) & 7)
        fprintf(stderr, "[wgl]    fog a=%06x b=%06x shift=%06x c=%06x color=%06x range=%06x %06x %06x\n", bp[0xEE], bp[0xEF], bp[0xF0], bp[0xF1],
                bp[0xF2], bp[0xE8], bp[0xE9], bp[0xEA]);
    fprintf(stderr, "[wgl]    tev");
    for (int st = 0; st < nst; st++) fprintf(stderr, " [%06x %06x ind%06x]", bp[0xC0 + st * 2], bp[0xC1 + st * 2], bp[0x10 + st]);
    fprintf(stderr, "\n[wgl]    ksel %06x %06x %06x %06x  creg %06x %06x %06x %06x %06x %06x  kon %06x %06x %06x %06x\n", bp[0xF6],
            bp[0xF7], bp[0xF8], bp[0xF9], s->creg[0], s->creg[1], s->creg[2], s->creg[3], s->creg[4], s->creg[5], s->konst[0],
            s->konst[1], s->konst[2], s->konst[3]);
    for (int m = 0; m < 8; m++)
        if (p->usedMaps & (1u << m)) {
            int base = m < 4 ? 0x80 + m : 0xA0 + (m - 4);
            fprintf(stderr, "[wgl]    map%d tex=%u %dx%d lv=%d mode0=%06x mode1=%06x img0=%06x img3=%06x\n", m, tex[m].tex,
                    tex[m].w, tex[m].h, tex[m].levels, bp[base], bp[base + 4], bp[base + 8], bp[base + 0x14]);
        }
    const GpuRegs *r = gpu_regs(); /* XF state as of now (may be past this batch) */
    fprintf(stderr, "[wgl]    xf ncol=%u chan %06x %06x %06x %06x amb %08x %08x mat %08x %08x ntex=%u texgen %06x %06x\n", r->xf[0x09],
            r->xf[0x0E], r->xf[0x0F], r->xf[0x10], r->xf[0x11], r->xf[0x0A], r->xf[0x0B], r->xf[0x0C], r->xf[0x0D], r->xf[0x3F],
            r->xf[0x40], r->xf[0x41]);
    for (int l = 0; l < 8; l++) {
        uint32_t mask = ((r->xf[0x0E] >> 2) & 0xF) | (((r->xf[0x0E] >> 11) & 0xF) << 4);
        if (!(mask & (1u << l))) continue;
        const float *L = &r->xfmem[0x600 + l * 16];
        uint32_t col;
        memcpy(&col, &L[3], 4);
        fprintf(stderr, "[wgl]    light%d col %08x cos %g %g %g dist %g %g %g pos %g %g %g dir %g %g %g\n", l, col, L[4], L[5], L[6], L[7],
                L[8], L[9], L[10], L[11], L[12], L[13], L[14], L[15]);
    }
}

static void flush(void)
{
    if (!sStageCount) return;
    const WglSnap *s = &sSnap;
    const uint32_t *bp = s->bp;
    uint32_t count = sStageCount;
    sStageCount = 0;
    wgl_stats_cur.flushes++;

    BlendSetup bl;
    blend_setup(bp, &bl);
    uint32_t cm0 = bp[0x41];
    int hasAlpha = (bp[0x43] & 7) == 1;
    int colorUpd = (int)((cm0 >> 3) & 1), alphaUpd = (int)((cm0 >> 4) & 1) && hasAlpha;
    int dstAlpha = (int)((bp[0x42] >> 8) & 1) && alphaUpd;
    int twoPass = dstAlpha && bl.enable;

    /* RGBA6 EFB: the pixel is stored with 6 bits per channel (Dolphin truncates the TEV
     * output); alpha keeps 8 bits when it is a blend factor (Dolphin blends with the
     * 8-bit alpha through dual-source blending) */
    int quant = 0;
    if ((bp[0x43] & 7) == 1) {
        int srcAlphaFactor = bl.enable && (bl.src == GL_SRC_ALPHA || bl.src == GL_ONE_MINUS_SRC_ALPHA ||
                                           bl.dst == GL_SRC_ALPHA || bl.dst == GL_ONE_MINUS_SRC_ALPHA);
        quant = srcAlphaFactor ? 1 : 3;
    }

    WglKey key;
    wgl_tev_key(&key, s, (dstAlpha && !twoPass) ? WGL_ALPHAOUT_CONST : WGL_ALPHAOUT_NORMAL, bl.srcMode, quant);
    WglProgram *p = wgl_tev_program(&key);
    if (!p) return;

    bind_efb();
    glBindVertexArray(sVao);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    if (sRingPos + count > RING_VERTS) {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)RING_VERTS * sizeof(GpuVertex), NULL, GL_STREAM_DRAW);
        sRingPos = 0;
    }
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)sRingPos * sizeof(GpuVertex), (GLsizeiptr)count * sizeof(GpuVertex), sStage);
    GLint first = (GLint)sRingPos;
    sRingPos += count;

    WglTexInfo tex[8];
    memset(tex, 0, sizeof tex);
    for (int m = 0; m < 8; m++)
        if (p->usedMaps & (1u << m)) wgl_tex_bind(s, m, &tex[m]);

    if (sDebug & 16)
        for (int m = 0; m < 8; m++)
            if (tex[m].isCopy) return;
    glUseProgram(p->prog);
    set_uniforms(p, s, tex);

    /* scissor */
    int ox, oy;
    scissor_offset(bp, &ox, &oy);
    int top = (int)(bp[0x20] & 0x7FF) - oy, left = (int)((bp[0x20] >> 12) & 0x7FF) - ox;
    int bottom = (int)(bp[0x21] & 0x7FF) - oy + 1, right = (int)((bp[0x21] >> 12) & 0x7FF) - ox + 1;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > WGL_EFB_W) right = WGL_EFB_W;
    if (bottom > WGL_EFB_H) bottom = WGL_EFB_H;
    if (right <= left || bottom <= top) return;
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, top, right - left, bottom - top);

    /* depth */
    uint32_t zm = bp[0x40];
    int zEnable = (int)(zm & 1), zUpdate = (int)((zm >> 4) & 1);
    GLenum zFunc = zEnable ? kCompare[(zm >> 1) & 7] : GL_ALWAYS;
    if (zEnable || zUpdate) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(zFunc);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(zUpdate ? GL_TRUE : GL_FALSE);

    /* blending */
    if (bl.enable) {
        glEnable(GL_BLEND);
        glBlendEquation(bl.eq);
        glBlendFunc(bl.src, bl.dst);
        glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_BLEND);
    }
    glColorMask(colorUpd, colorUpd, colorUpd, (alphaUpd && !twoPass) ? GL_TRUE : GL_FALSE);

    GLenum mode = sStagePrim == GPU_LINES ? GL_LINES : sStagePrim == GPU_POINTS ? GL_POINTS : GL_TRIANGLES;
    glDrawArrays(mode, first, (GLsizei)count);
    wgl_stats_cur.drawCalls++;

    if (sTraceFrame && (int)wgl_frame == sTraceFrame) trace_batch(s, p, tex, count);
    if (sPick >= 0) {
        /* MKDD_WGL_PICK=x*1000+y: log the batches that change that EFB pixel */
        uint8_t px[4];
        glReadPixels(sPick / 1000, sPick % 1000, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        if (memcmp(px, sPickLast, 4)) {
            fprintf(stderr, "[pick] (%d,%d) %02x%02x%02x%02x -> %02x%02x%02x%02x\n", sPick / 1000, sPick % 1000, sPickLast[0],
                    sPickLast[1], sPickLast[2], sPickLast[3], px[0], px[1], px[2], px[3]);
            trace_batch(s, p, tex, count);
            memcpy(sPickLast, px, 4);
        } else if (sStagePrim == GPU_TRIANGLES) {
            /* also report triangles over the pixel that left it unchanged (depth/alpha test) */
            int ox2, oy2;
            scissor_offset(bp, &ox2, &oy2);
            float px0 = (float)(sPick / 1000) + 0.5f, py0 = (float)(sPick % 1000) + 0.5f;
            for (uint32_t t = 0; t + 2 < count; t += 3) {
                float X[3], Y[3];
                int ok = 1;
                for (int k = 0; k < 3; k++) {
                    const float *P = sStage[t + k].pos;
                    if (P[3] <= 0) { ok = 0; break; }
                    X[k] = s->vp[3] - ox2 + s->vp[0] * P[0] / P[3];
                    Y[k] = s->vp[4] - oy2 + s->vp[1] * P[1] / P[3];
                }
                if (!ok) continue;
                float d0 = (X[1] - X[0]) * (py0 - Y[0]) - (Y[1] - Y[0]) * (px0 - X[0]);
                float d1 = (X[2] - X[1]) * (py0 - Y[1]) - (Y[2] - Y[1]) * (px0 - X[1]);
                float d2 = (X[0] - X[2]) * (py0 - Y[2]) - (Y[0] - Y[2]) * (px0 - X[2]);
                if ((d0 >= 0 && d1 >= 0 && d2 >= 0) || (d0 <= 0 && d1 <= 0 && d2 <= 0)) {
                    fprintf(stderr, "[pick] covered but unchanged:\n");
                    trace_batch(s, p, tex, count);
                    break;
                }
            }
        }
    }

    if (twoPass) {
        /* Destination alpha: write the constant alpha in a second, alpha-only pass. */
        wgl_tev_key(&key, s, WGL_ALPHAOUT_CONST, bl.srcMode, quant ? (quant | 2) : 0);
        WglProgram *p2 = wgl_tev_program(&key);
        if (p2) {
            glUseProgram(p2->prog);
            set_uniforms(p2, s, tex);
            glDisable(GL_BLEND);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
            glDepthMask(GL_FALSE);
            if (zUpdate && zEnable) glDepthFunc(GL_EQUAL);
            glDrawArrays(mode, first, (GLsizei)count);
            wgl_stats_cur.drawCalls++;
        }
    }
}

static void be_draw(void *ctx, const GpuRegs *regs, int prim, const GpuVertex *v, uint32_t count)
{
    (void)ctx;
    if (!sReady || !count) return;
    capture(&sTmpSnap, regs);
    if (!sSnapValid || sStagePrim != prim || memcmp(&sTmpSnap, &sSnap, sizeof sSnap) != 0) {
        flush();
        sSnap = sTmpSnap;
        sSnapValid = 1;
        sStagePrim = prim;
    }
    uint32_t unit = prim == GPU_TRIANGLES ? 3 : prim == GPU_LINES ? 2 : 1;
    count -= count % unit;
    while (count) {
        uint32_t room = STAGE_VERTS - sStageCount;
        room -= room % unit;
        if (!room) {
            flush();
            continue;
        }
        uint32_t n = count < room ? count : room;
        memcpy(sStage + sStageCount, v, n * sizeof(GpuVertex));
        sStageCount += n;
        v += n;
        count -= n;
    }
}

/* ---- frame capture for comparisons with Dolphin (MKDD_DFF, gpu_fifolog.c) ---- */

extern "C" int gpu_fifolog_capture_index(void);
extern "C" const char *gpu_fifolog_path(void);

/* Writes the XFB image as <MKDD_DFF>.ours-<frame>.ppm. */
static void capture_xfb(GLuint tex, int w, int h, int frame)
{
    static uint8_t *buf;
    if (!buf) buf = (uint8_t *)malloc(640 * 576 * 4);
    if (!buf || w <= 0 || h <= 0) return;
    glBindFramebuffer(GL_FRAMEBUFFER, sCopyFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    char path[600];
    snprintf(path, sizeof path, "%s.ours-%05d.ppm", gpu_fifolog_path(), frame);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) fwrite(buf + i * 4, 1, 3, f);
    fclose(f);
}

/* ---- EFB copies ---- */

/* How a copy format is laid out for sampling: texdec layout + channel sources.
 * Channel ids: 0 R, 1 G, 2 B, 3 A, 4 Y (luma), 5 Z hi, 6 Z mid, 7 Z lo, 8 one. */
struct CopyFormat {
    int gxFmt, layout, isel, asel, zrgb;
};

static CopyFormat copy_format(uint32_t exec, int depth)
{
    int f = (int)((exec >> 4) & 7) | (int)((exec >> 3) & 1) << 3;
    int intensity = (int)((exec >> 15) & 1);
    CopyFormat c = { 0x26, 6, 0, 3, 0 };
    if (depth) {
        switch (f) {
        case 0: c = (CopyFormat){ 0x30, 0, 5, 5, 0 }; break; /* Z4 */
        case 1: case 8: c = (CopyFormat){ 0x11, 1, 5, 5, 0 }; break; /* Z8 */
        case 9: c = (CopyFormat){ 0x39, 1, 6, 6, 0 }; break; /* Z8M */
        case 10: c = (CopyFormat){ 0x3A, 1, 7, 7, 0 }; break; /* Z8L */
        /* Z16 samples as IA8 with I = Z bits 23-16 and A = bits 15-8 (the 16-bit value
         * A | I << 8 the TEV compare modes see, as MKDD's depth of field relies on) */
        case 11: c = (CopyFormat){ 0x13, 3, 5, 6, 0 }; break; /* Z16 */
        case 12: c = (CopyFormat){ 0x3C, 3, 6, 7, 0 }; break; /* Z16L: I = bits 15-8, A = 7-0 */
        default: c = (CopyFormat){ 0x16, 6, 5, 8, 1 }; break; /* Z24X8 */
        }
        return c;
    }
    if (intensity) {
        switch (f) {
        case 0: c = (CopyFormat){ 0x0, 0, 4, 8, 0 }; break;
        case 1: c = (CopyFormat){ 0x1, 1, 4, 8, 0 }; break;
        case 2: c = (CopyFormat){ 0x2, 2, 4, 3, 0 }; break;
        case 3: c = (CopyFormat){ 0x3, 3, 4, 3, 0 }; break;
        default: c = (CopyFormat){ 0x26, 6, 0, 3, 0 }; break; /* YUVA8 */
        }
        return c;
    }
    switch (f) {
    case 0: c = (CopyFormat){ 0x20, 0, 0, 0, 0 }; break;  /* R4 */
    case 1: c = (CopyFormat){ 0x28, 1, 0, 0, 0 }; break;  /* (R8) */
    case 2: c = (CopyFormat){ 0x22, 2, 0, 3, 0 }; break;  /* RA4 */
    case 3: c = (CopyFormat){ 0x23, 3, 0, 3, 0 }; break;  /* RA8 */
    case 4: c = (CopyFormat){ 0x4, 4, 0, 3, 0 }; break;   /* RGB565 */
    case 5: c = (CopyFormat){ 0x5, 5, 0, 3, 0 }; break;   /* RGB5A3 */
    case 6: c = (CopyFormat){ 0x6, 6, 0, 3, 0 }; break;   /* RGBA8 */
    case 7: c = (CopyFormat){ 0x27, 1, 3, 3, 0 }; break;  /* A8 */
    case 8: c = (CopyFormat){ 0x28, 1, 0, 0, 0 }; break;  /* R8 */
    case 9: c = (CopyFormat){ 0x29, 1, 1, 1, 0 }; break;  /* G8 */
    case 10: c = (CopyFormat){ 0x2A, 1, 2, 2, 0 }; break; /* B8 */
    case 11: c = (CopyFormat){ 0x2B, 3, 0, 1, 0 }; break; /* RG8: I = R, A = G */
    default: c = (CopyFormat){ 0x2C, 3, 1, 2, 0 }; break; /* GB8: I = G, A = B */
    }
    return c;
}

static void run_copy(GLuint dstTex, int dx, int dy, int w, int h, const uint32_t *bp, int sx, int sy, int sw, int sh,
                     int half, int toXfb, const CopyFormat *cf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, sCopyFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex, 0);
    glViewport(0, 0, dx + w, dy + h);
    glEnable(GL_SCISSOR_TEST);
    glScissor(dx, dy, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(sCopyProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sEfbColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sEfbDepth);
    glBindSampler(0, 0);
    glBindSampler(1, 0);
    glActiveTexture(GL_TEXTURE0);
    wgl_tex_forget_bindings();

    uint32_t exec = bp[0x52];
    glUniform4i(uCopyRect, sx, sy, sw, sh);
    glUniform2i(uCopyDst, dx, dy);
    glUniform1i(uCopyHalf, half);
    glUniform1i(uCopyDepth, (bp[0x43] & 7) == 3);
    glUniform1i(uCopyPixFmt, (int)(bp[0x43] & 7));
    glUniform1i(uCopyLayout, cf ? cf->layout : 6);
    glUniform1i(uCopyISel, cf ? cf->isel : 0);
    glUniform1i(uCopyASel, cf ? cf->asel : 3);
    glUniform1i(uCopyZRgb, cf ? cf->zrgb : 0);
    glUniform1i(uCopyXfb, toXfb);
    static const float gammas[4] = { 1.0f, 1.7f, 2.2f, 2.2f };
    glUniform1f(uCopyGamma, gammas[(exec >> 7) & 3]);
    /* vertical (deflicker) filter: taps 0-1 previous line, 2-4 this line, 5-6 next line */
    uint32_t f0 = bp[0x53], f1 = bp[0x54];
    GLuint vf[3] = { (f0 & 63) + ((f0 >> 6) & 63), ((f0 >> 12) & 63) + ((f0 >> 18) & 63) + (f1 & 63),
                     ((f1 >> 6) & 63) + ((f1 >> 12) & 63) };
    if (!sDeflicker) { /* as Dolphin by default: keep the brightness, drop the vertical blur */
        vf[1] += vf[0] + vf[2];
        vf[0] = vf[2] = 0;
    }
    if (!vf[1] && !vf[0] && !vf[2]) vf[1] = 64; /* never programmed: pass through */
    glUniform3uiv(uCopyVf, 1, vf);
    glBindVertexArray(sEmptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    wgl_tex_restore_samplers();
}

/* The XFB image a copy lands in. A copy whose source starts at the top of the
 * EFB begins a new image at its destination address (games often keep their
 * XFBs back to back in RAM, so the next XFB lies inside the previous one's
 * 576-row range); a copy of lower EFB rows continues the image it falls in
 * (e.g. a frame copied in two halves). */
static XfbEntry *xfb_for_copy(uint32_t addr, uint32_t stride, int srcY, int *row, int *col)
{
    for (int i = 0; i < MAX_XFB; i++) {
        XfbEntry *e = &sXfb[i];
        if (e->tex && e->stride == stride && e->addr == addr) {
            *row = *col = 0;
            return e;
        }
    }
    if (srcY > 0) {
        for (int i = 0; i < MAX_XFB; i++) {
            XfbEntry *e = &sXfb[i];
            if (!e->tex || e->stride != stride || !stride) continue;
            if (addr > e->addr && addr < e->addr + stride * 576) {
                *row = (int)((addr - e->addr) / stride);
                *col = (int)(((addr - e->addr) % stride) / 2);
                return e;
            }
        }
    }
    XfbEntry *e = &sXfb[0];
    for (int i = 0; i < MAX_XFB; i++) {
        if (!sXfb[i].tex) { e = &sXfb[i]; break; }
        if (sXfb[i].lastUsed < e->lastUsed) e = &sXfb[i];
    }
    if (!e->tex) {
        glGenTextures(1, &e->tex);
        glBindTexture(GL_TEXTURE_2D, e->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 640, 576, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        wgl_tex_forget_bindings();
    }
    e->addr = addr;
    e->stride = stride;
    e->w = e->h = 0;
    *row = *col = 0;
    return e;
}

static void be_efb_copy(void *ctx, const GpuRegs *regs, int toXfb, int clear)
{
    (void)ctx;
    if (!sReady) return;
    flush();
    const uint32_t *bp = regs->bp;
    uint32_t exec = bp[0x52];
    int sx = (int)(bp[0x49] & 0x3FF), sy = (int)((bp[0x49] >> 10) & 0x3FF);
    int sw = (int)(bp[0x4A] & 0x3FF) + 1, sh = (int)((bp[0x4A] >> 10) & 0x3FF) + 1;
    uint32_t addr = (bp[0x4B] & 0xFFFFFF) << 5;
    uint32_t stride = (bp[0x4D] & 0x3FF) << 5;
    if (sx + sw > WGL_EFB_W) sw = WGL_EFB_W - sx;
    if (sy + sh > WGL_EFB_H) sh = WGL_EFB_H - sy;

    if (sw > 0 && sh > 0) {
        if (toXfb) {
            int row, col;
            XfbEntry *e = xfb_for_copy(addr, stride, sy, &row, &col);
            int w = sw, h = sh;
            if (col + w > 640) w = 640 - col;
            if (row + h > 576) h = 576 - row;
            if (w > 0 && h > 0) {
                run_copy(e->tex, col, row, w, h, bp, sx, sy, sw, sh, 0, 1, NULL);
                if (!row && !col) e->w = e->h = 0; /* a copy to the base address starts a new image */
                if (col + w > e->w) e->w = col + w;
                if (row + h > e->h) e->h = row + h;
            }
            e->lastUsed = ++sXfbSeq;
            int cap = gpu_fifolog_capture_index();
            if (cap >= 0) capture_xfb(e->tex, e->w, e->h, cap);
            if (sTraceFrame && (int)wgl_frame == sTraceFrame)
                fprintf(stderr, "[wgl] xfb copy addr=%08x stride=%u row=%d src=%d,%d %dx%d entry=%08x\n", addr, stride, row, sx, sy, sw, sh, e->addr);
            wgl_stats_cur.xfbCopies++;
        } else {
            int half = (int)((exec >> 9) & 1);
            int w = half ? (sw + 1) / 2 : sw, h = half ? (sh + 1) / 2 : sh;
            CopyFormat cf = copy_format(exec, (bp[0x43] & 7) == 3);
            GLuint t = wgl_tex_copy_target(addr, cf.gxFmt, w, h);
            run_copy(t, 0, 0, w, h, bp, sx, sy, sw, sh, half, 0, &cf);
            if (sTraceFrame && (int)wgl_frame == sTraceFrame)
                fprintf(stderr, "[wgl] copy tex=%u addr=%08x fmt=%x %dx%d src=%d,%d %dx%d half=%d pix=%x clear=%d\n", t, addr, cf.gxFmt, w,
                        h, sx, sy, sw, sh, half, bp[0x43], clear);
            wgl_tex_register_copy(addr, cf.gxFmt, w, h, t);
            wgl_stats_cur.efbCopies++;
        }
    }

    if (clear) {
        /* honours the color/alpha/z update enables, as the hardware does */
        bind_efb();
        uint32_t cm0 = bp[0x41];
        int cu = (int)((cm0 >> 3) & 1), au = (int)((cm0 >> 4) & 1), zu = (int)((bp[0x40] >> 4) & 1);
        glEnable(GL_SCISSOR_TEST);
        glScissor(sx, sy, sw, sh);
        glColorMask(cu, cu, cu, au);
        glDepthMask(zu ? GL_TRUE : GL_FALSE);
        float col[4] = { (float)(bp[0x4F] & 0xFF) / 255.0f, (float)((bp[0x50] >> 8) & 0xFF) / 255.0f,
                         (float)(bp[0x50] & 0xFF) / 255.0f, (float)((bp[0x4F] >> 8) & 0xFF) / 255.0f };
        if (cu || au) glClearBufferfv(GL_COLOR, 0, col);
        if (zu) glClearBufferfi(GL_DEPTH_STENCIL, 0, (float)(bp[0x51] & 0xFFFFFF) / 16777215.0f, 0);
    }
}

static void be_present(void *ctx, uint32_t xfbAddr)
{
    (void)ctx;
    if (!sReady) return;
    flush();
    XfbEntry *e = NULL;
    for (int i = 0; i < MAX_XFB && !e && xfbAddr; i++)
        if (sXfb[i].tex && sXfb[i].addr == xfbAddr) e = &sXfb[i];
    for (int i = 0; i < MAX_XFB && !e; i++)
        if (sXfb[i].tex && xfbAddr > sXfb[i].addr && xfbAddr < sXfb[i].addr + sXfb[i].stride * 576) e = &sXfb[i];
    if (!e && xfbAddr) { /* fall back to the most recent copy; 0 = display blanked */
        for (int i = 0; i < MAX_XFB; i++)
            if (sXfb[i].tex && (!e || sXfb[i].lastUsed > e->lastUsed)) e = &sXfb[i];
    }
    int cw = 640, ch = 480;
    emscripten_get_canvas_element_size("#screen", &cw, &ch);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, cw, ch);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (!e || !e->w || !e->h) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glUseProgram(sPresentProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->tex);
        glBindSampler(0, 0);
        wgl_tex_forget_bindings();
        glUniform4f(uPresentRect, 0.0f, 0.0f, (float)e->w / 640.0f, (float)e->h / 576.0f);
        glUniform2f(glGetUniformLocation(sPresentProg, "u_canvas"), (float)cw, (float)ch);
        glBindVertexArray(sEmptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        wgl_tex_restore_samplers();
    }
    if (sTraceFrame && (int)wgl_frame == sTraceFrame) fprintf(stderr, "[wgl] present %08x\n", xfbAddr);
    sStatsPrev = wgl_stats_cur;
    memset(&wgl_stats_cur, 0, sizeof wgl_stats_cur);
    wgl_frame++;
    if (sTraceFrame && wgl_frame % 250 == 0) fprintf(stderr, "[wgl] frame %u\n", wgl_frame);
    wgl_tex_new_frame();
}

/* ---- creation ---- */

static int init_gl(void)
{
    glGenTextures(1, &sEfbColor);
    glBindTexture(GL_TEXTURE_2D, sEfbColor);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WGL_EFB_W, WGL_EFB_H);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenTextures(1, &sEfbDepth);
    glBindTexture(GL_TEXTURE_2D, sEfbDepth);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, WGL_EFB_W, WGL_EFB_H);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &sEfbFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sEfbFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sEfbColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, sEfbDepth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        wgl_log("[webgl] EFB framebuffer incomplete");
        return 0;
    }
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glGenFramebuffers(1, &sCopyFbo);

    wgl_main_vs = wgl_shader(GL_VERTEX_SHADER, kMainVs, "main vertex");
    if (!wgl_main_vs) return 0;

    glGenVertexArrays(1, &sVao);
    glBindVertexArray(sVao);
    glGenBuffers(1, &sVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sVbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)RING_VERTS * sizeof(GpuVertex), NULL, GL_STREAM_DRAW);
    GLsizei stride = sizeof(GpuVertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(GpuVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(GpuVertex, color[0]));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(GpuVertex, color[1]));
    for (int i = 0; i < 8; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 3, GL_FLOAT, GL_FALSE, stride, (void *)(offsetof(GpuVertex, tex) + i * 12));
    }
    glGenVertexArrays(1, &sEmptyVao);
    sStage = (GpuVertex *)malloc(STAGE_VERTS * sizeof(GpuVertex));
    if (!sStage) return 0;

    sCopyProg = wgl_compile(kQuadVs, kCopyFs, "efb copy");
    sPresentProg = wgl_compile(kQuadVs, kPresentFs, "present");
    if (!sCopyProg || !sPresentProg) return 0;
    glUseProgram(sCopyProg);
    glUniform1i(glGetUniformLocation(sCopyProg, "u_col"), 0);
    glUniform1i(glGetUniformLocation(sCopyProg, "u_dep"), 1);
    uCopyRect = glGetUniformLocation(sCopyProg, "u_rect");
    uCopyDst = glGetUniformLocation(sCopyProg, "u_dst");
    uCopyHalf = glGetUniformLocation(sCopyProg, "u_half");
    uCopyDepth = glGetUniformLocation(sCopyProg, "u_depthsrc");
    uCopyPixFmt = glGetUniformLocation(sCopyProg, "u_pixfmt");
    uCopyLayout = glGetUniformLocation(sCopyProg, "u_layout");
    uCopyISel = glGetUniformLocation(sCopyProg, "u_isel");
    uCopyASel = glGetUniformLocation(sCopyProg, "u_asel");
    uCopyZRgb = glGetUniformLocation(sCopyProg, "u_zrgb");
    uCopyXfb = glGetUniformLocation(sCopyProg, "u_xfb");
    uCopyGamma = glGetUniformLocation(sCopyProg, "u_gamma");
    uCopyVf = glGetUniformLocation(sCopyProg, "u_vf");
    glUseProgram(sPresentProg);
    glUniform1i(glGetUniformLocation(sPresentProg, "u_tex"), 0);
    uPresentRect = glGetUniformLocation(sPresentProg, "u_rect");

    wgl_tev_init();
    wgl_tex_init();
    return 1;
}

extern "C" const GpuBackend *gpu_webgl_create(void)
{
    if (sReady) return &sBackend;
    int hasCanvas = EM_ASM_INT({
        return (typeof document !== 'undefined' && document.querySelector('#screen')) ? 1 : 0;
    });
    if (!hasCanvas) return NULL;
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.majorVersion = 2;
    attr.minorVersion = 0;
    attr.alpha = 0;
    attr.depth = 0;
    attr.stencil = 0;
    attr.antialias = 0;
    attr.premultipliedAlpha = 0;
    attr.preserveDrawingBuffer = 0;
    sCtx = emscripten_webgl_create_context("#screen", &attr);
    if (sCtx <= 0) {
        wgl_log("[webgl] could not create a WebGL2 context (%d)", (int)sCtx);
        return NULL;
    }
    emscripten_webgl_make_context_current(sCtx);
    if (!init_gl()) return NULL;
    sTraceFrame = port_env("MKDD_WGL_TRACE");
    sDebug = wgl_debug = port_env("MKDD_WGL_DEBUG");
    if (port_env("MKDD_WGL_PICK")) sPick = port_env("MKDD_WGL_PICK");
    sDeflicker = port_env("MKDD_DEFLICKER");
    sBackend.draw = be_draw;
    sBackend.efb_copy = be_efb_copy;
    sBackend.present = be_present;
    sBackend.ctx = NULL;
    sReady = 1;
    return &sBackend;
}

extern "C" const GpuWebglStats *gpu_webgl_stats(void) { return &sStatsPrev; }
