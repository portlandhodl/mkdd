/* WebGL2 backend: TEV -> GLSL ES 3.0 fragment shader generation and the
 * program cache.
 *
 * The combiners run in integer arithmetic like the hardware: inputs a/b/c are
 * 8-bit, d and the registers are signed 11-bit, the lerp uses c + (c >> 7) so
 * that 255 means 1.0, and results are clamped to 0..255 or -1024..1023. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "webgl_internal.h"

/* ---- string builder ---- */

static char *sSrc;
static size_t sLen, sCap;

static void emit(const char *fmt, ...)
{
    va_list ap;
    for (;;) {
        va_start(ap, fmt);
        int n = vsnprintf(sSrc + sLen, sCap - sLen, fmt, ap);
        va_end(ap);
        if (n >= 0 && sLen + (size_t)n < sCap) {
            sLen += (size_t)n;
            return;
        }
        size_t cap = sCap * 2 + (size_t)(n > 0 ? n : 0) + 1024;
        char *p = (char *)realloc(sSrc, cap);
        if (!p) return;
        sSrc = p;
        sCap = cap;
    }
}

/* ---- key ---- */

void wgl_tev_key(WglKey *key, const WglSnap *s, int alphaOut, int srcMode)
{
    const uint32_t *bp = s->bp;
    int ntev = (int)wgl_bits(bp[0x00], 10, 4) + 1;
    int nind = (int)wgl_bits(bp[0x00], 16, 3);
    if (nind > 4) nind = 4;
    uint32_t *w = key->w;
    int n = 0;
    w[n++] = (uint32_t)ntev | (uint32_t)nind << 4 | (uint32_t)alphaOut << 8 | (uint32_t)srcMode << 10 |
             wgl_bits(bp[0xF1], 20, 4) << 16 | wgl_bits(bp[0xF5], 0, 4) << 20;
    w[n++] = wgl_bits(bp[0xF3], 16, 8);
    uint32_t swap = 0;
    for (int i = 0; i < 8; i++) swap |= wgl_bits(bp[0xF6 + i], 0, 4) << (i * 4);
    w[n++] = swap;
    w[n++] = nind ? wgl_bits(bp[0x27], 0, nind * 6) : 0;
    for (int st = 0; st < ntev; st++) {
        uint32_t order = wgl_bits(bp[0x28 + st / 2], (st & 1) * 12, 12);
        uint32_t ksel = bp[0xF6 + st / 2];
        uint32_t k = (st & 1) ? wgl_bits(ksel, 14, 10) : wgl_bits(ksel, 4, 10);
        w[n++] = order | k << 12;
        w[n++] = bp[0xC0 + st * 2] & 0xFFFFFF;
        w[n++] = bp[0xC1 + st * 2] & 0xFFFFFF;
        w[n++] = bp[0x10 + st] & 0x1FFFFF;
    }
    key->n = n;
}

/* ---- generation ---- */

static const char *kColorIn[16] = {
    "prev.rgb", "prev.aaa", "c0.rgb", "c0.aaa", "c1.rgb", "c1.aaa", "c2.rgb", "c2.aaa",
    "tex.rgb", "tex.aaa", "ras.rgb", "ras.aaa", "ivec3(255)", "ivec3(128)", "kc", "ivec3(0)",
};
static const char *kAlphaIn[8] = { "prev.a", "c0.a", "c1.a", "c2.a", "tex.a", "ras.a", "ka", "0" };
static const char *kRegName[4] = { "prev", "c0", "c1", "c2" };
static const char kChan[4] = { 'r', 'g', 'b', 'a' };
static const int kFrac[8] = { 255, 223, 191, 159, 127, 95, 63, 31 };

static void konst_color(char *out, size_t n, int sel)
{
    if (sel < 8) snprintf(out, n, "ivec3(%d)", kFrac[sel]);
    else if (sel >= 0x0C && sel <= 0x0F) snprintf(out, n, "u_kon[%d].rgb", sel - 0x0C);
    else if (sel >= 0x10) snprintf(out, n, "u_kon[%d].%c%c%c", sel & 3, kChan[(sel - 0x10) >> 2], kChan[(sel - 0x10) >> 2], kChan[(sel - 0x10) >> 2]);
    else snprintf(out, n, "ivec3(0)");
}

static void konst_alpha(char *out, size_t n, int sel)
{
    if (sel < 8) snprintf(out, n, "%d", kFrac[sel]);
    else if (sel >= 0x10) snprintf(out, n, "u_kon[%d].%c", sel & 3, kChan[(sel - 0x10) >> 2]);
    else snprintf(out, n, "0");
}

static void swizzle(char *out, uint32_t swapRegs, int table)
{
    /* table t: R,G from bp[0xF6+2t] bits 0-3, B,A from bp[0xF7+2t] */
    uint32_t lo = (swapRegs >> (table * 8)) & 15, hi = (swapRegs >> (table * 8 + 4)) & 15;
    out[0] = kChan[lo & 3];
    out[1] = kChan[lo >> 2];
    out[2] = kChan[hi & 3];
    out[3] = kChan[hi >> 2];
    out[4] = 0;
}

static const char *cmp_expr(int comp)
{
    static const char *ops[8] = { "false", "(a < %s)", "(a == %s)", "(a <= %s)", "(a > %s)", "(a != %s)", "(a >= %s)", "true" };
    return ops[comp & 7];
}

static void gen_fragment(const WglKey *key, uint32_t *usedMaps, uint32_t *usedCoords)
{
    const uint32_t *w = key->w;
    int ntev = (int)(w[0] & 15), nind = (int)((w[0] >> 4) & 7);
    int alphaOut = (int)((w[0] >> 8) & 3), srcMode = (int)((w[0] >> 10) & 3);
    int fogSel = (int)((w[0] >> 16) & 15);
    int ztex = (int)((w[0] >> 20) & 15);
    uint32_t atest = w[1], swap = w[2], iref = w[3];
    const uint32_t *stw = w + 4;
    uint32_t maps = 0, coords = 0;

    for (int i = 0; i < nind; i++) {
        maps |= 1u << wgl_bits(iref, i * 6, 3);
        coords |= 1u << wgl_bits(iref, i * 6 + 3, 3);
    }
    for (int st = 0; st < ntev; st++) {
        uint32_t order = stw[st * 4] & 0xFFF;
        if (order & 0x40) {
            maps |= 1u << (order & 7);
            coords |= 1u << ((order >> 3) & 7);
        }
    }

    sLen = 0;
    emit("#version 300 es\nprecision highp float;\nprecision highp int;\n");
    emit("in vec4 v_c0;\nin vec4 v_c1;\n");
    for (int i = 0; i < 8; i++)
        if (coords & (1u << i)) emit("in vec3 v_t%d;\n", i);
    for (int i = 0; i < 8; i++)
        if (maps & (1u << i)) emit("uniform sampler2D s%d;\n", i);
    emit("uniform ivec4 u_reg[4];\nuniform ivec4 u_kon[4];\nuniform vec2 u_tsize[8];\nuniform vec2 u_su[8];\n"
         "uniform float u_lodbias[8];\nuniform ivec2 u_aref;\nuniform vec4 u_fog;\nuniform vec3 u_fogcolor;\n"
         "uniform int u_dstalpha;\nuniform vec3 u_indm[6];\nuniform vec2 u_indss[4];\nuniform vec3 u_iscale;\n"
         "uniform int u_zbias;\n");
    emit("out vec4 o_col;\n");
    emit("ivec3 lerp3(ivec3 a, ivec3 b, ivec3 c) { a &= 255; b &= 255; c &= 255; c += c >> 7; return a * (256 - c) + b * c; }\n"
         "int lerp1(int a, int b, int c) { a &= 255; b &= 255; c &= 255; c += c >> 7; return a * (256 - c) + b * c; }\n"
         "ivec3 tev3(ivec3 d, ivec3 l, int sub, int bias, int sh) {\n"
         "  ivec3 v = (d << 8) + (sub != 0 ? -l : l) + ivec3(bias);\n"
         "  if (sh == 3) return (v + 256) >> 9;\n  return ((v << sh) + 128) >> 8; }\n"
         "int tev1(int d, int l, int sub, int bias, int sh) {\n"
         "  int v = (d << 8) + (sub != 0 ? -l : l) + bias;\n"
         "  if (sh == 3) return (v + 256) >> 9;\n  return ((v << sh) + 128) >> 8; }\n"
         "int c16(ivec3 v) { v &= 255; return v.r | (v.g << 8); }\n"
         "int c24(ivec3 v) { v &= 255; return v.r | (v.g << 8) | (v.b << 16); }\n"
         "ivec4 smp(sampler2D s, vec2 uv, float bias) { return ivec4(round(texture(s, uv, bias) * 255.0)); }\n");
    emit("void main() {\n");
    emit("  ivec4 prev = u_reg[0], c0 = u_reg[1], c1 = u_reg[2], c2 = u_reg[3];\n");
    emit("  ivec4 rc0 = ivec4(round(clamp(v_c0, 0.0, 1.0) * 255.0));\n");
    emit("  ivec4 rc1 = ivec4(round(clamp(v_c1, 0.0, 1.0) * 255.0));\n");
    for (int i = 0; i < 8; i++)
        if (coords & (1u << i))
            emit("  vec2 tc%d = v_t%d.xy / (v_t%d.z != 0.0 ? v_t%d.z : 1.0) * u_su[%d];\n", i, i, i, i, i);
    for (int i = 0; i < nind; i++) {
        int map = (int)wgl_bits(iref, i * 6, 3), tc = (int)wgl_bits(iref, i * 6 + 3, 3);
        emit("  ivec3 ind%d = smp(s%d, tc%d * u_indss[%d] / u_tsize[%d], 0.0).abg;\n", i, map, tc, i, map);
    }
    emit("  ivec4 tex = ivec4(255), ras = ivec4(0);\n  ivec3 kc = ivec3(0);\n  int ka = 0;\n");
    emit("  vec2 ptc = vec2(0.0);\n  int bump = 0;\n");

    int lastC = 0, lastA = 0;
    for (int st = 0; st < ntev; st++) {
        uint32_t order = stw[st * 4] & 0xFFF, ksel = stw[st * 4] >> 12;
        uint32_t ce = stw[st * 4 + 1], ae = stw[st * 4 + 2], ind = stw[st * 4 + 3];
        int map = (int)(order & 7), tc = (int)((order >> 3) & 7), en = (int)((order >> 6) & 1);
        int chan = (int)((order >> 7) & 7);
        char sw[5], buf[64];
        emit("  // stage %d\n  {\n", st);

        /* indirect texture coordinate */
        int bt = (int)wgl_bits(ind, 0, 2), ifmt = (int)wgl_bits(ind, 2, 2), ibias = (int)wgl_bits(ind, 4, 3);
        int bs = (int)wgl_bits(ind, 7, 2), mtx = (int)wgl_bits(ind, 9, 4), wrs = (int)wgl_bits(ind, 13, 3);
        int wrt = (int)wgl_bits(ind, 16, 3), addprev = (int)wgl_bits(ind, 20, 1);
        int hasInd = ind != 0;
        if (hasInd) {
            static const int masks[4] = { 0xFF, 0x1F, 0x0F, 0x07 };
            static const int bumpMasks[4] = { 0xF8, 0xE0, 0xF0, 0xF8 };
            if (bt < nind) emit("    ivec3 ic = ind%d;\n", bt);
            else emit("    ivec3 ic = ivec3(0);\n");
            if (bs) emit("    bump = ic.%c & %d;\n", "xyz"[bs - 1], bumpMasks[ifmt]);
            emit("    ic &= %d;\n", masks[ifmt]);
            if (ibias) {
                int b = ifmt == 0 ? -128 : 1;
                emit("    ic += ivec3(%d, %d, %d);\n", (ibias & 1) ? b : 0, (ibias & 2) ? b : 0, (ibias & 4) ? b : 0);
            }
        }
        if (en) {
            if (hasInd) {
                emit("    vec2 t = tc%d;\n", tc);
                emit("    vec2 off = vec2(0.0);\n");
                if (mtx >= 1 && mtx <= 3)
                    emit("    off = vec2(dot(u_indm[%d], vec3(ic)), dot(u_indm[%d], vec3(ic)));\n", (mtx - 1) * 2, (mtx - 1) * 2 + 1);
                else if (mtx >= 5 && mtx <= 7)
                    emit("    off = t * float(ic.x) / 256.0 * u_iscale[%d];\n", mtx - 5);
                else if (mtx >= 9 && mtx <= 11)
                    emit("    off = t * float(ic.y) / 256.0 * u_iscale[%d];\n", mtx - 9);
                static const char *wraps[8] = { "", "256.0", "128.0", "64.0", "32.0", "16.0", "0", "" };
                if (wrs == 6) emit("    t.x = 0.0;\n");
                else if (wrs >= 1 && wrs <= 5) emit("    t.x = mod(t.x, %s);\n", wraps[wrs]);
                if (wrt == 6) emit("    t.y = 0.0;\n");
                else if (wrt >= 1 && wrt <= 5) emit("    t.y = mod(t.y, %s);\n", wraps[wrt]);
                emit("    t += off%s;\n", addprev ? " + ptc" : "");
                emit("    ptc = t;\n");
            } else {
                emit("    vec2 t = tc%d;\n    ptc = t;\n", tc);
            }
            swizzle(sw, swap, (int)wgl_bits(ae, 2, 2));
            emit("    tex = smp(s%d, t / u_tsize[%d], u_lodbias[%d]).%s;\n", map, map, map, sw);
        } else {
            emit("    tex = ivec4(255);\n");
        }

        /* rasterized color */
        swizzle(sw, swap, (int)wgl_bits(ae, 0, 2));
        switch (chan) {
        case 0: emit("    ras = rc0.%s;\n", sw); break;
        case 1: emit("    ras = rc1.%s;\n", sw); break;
        case 5: emit("    ras = ivec4(bump).%s;\n", sw); break;
        case 6: emit("    ras = ivec4(bump | (bump >> 5)).%s;\n", sw); break;
        default: emit("    ras = ivec4(0);\n"); break;
        }

        /* konst */
        konst_color(buf, sizeof buf, (int)(ksel & 31));
        emit("    kc = %s;\n", buf);
        konst_alpha(buf, sizeof buf, (int)((ksel >> 5) & 31));
        emit("    ka = %s;\n", buf);

        /* color combiner */
        int cd = (int)wgl_bits(ce, 0, 4), cc = (int)wgl_bits(ce, 4, 4), cb = (int)wgl_bits(ce, 8, 4), ca = (int)wgl_bits(ce, 12, 4);
        int cbias = (int)wgl_bits(ce, 16, 2), csub = (int)wgl_bits(ce, 18, 1), cclamp = (int)wgl_bits(ce, 19, 1);
        int cscale = (int)wgl_bits(ce, 20, 2), cdest = (int)wgl_bits(ce, 22, 2);
        emit("    ivec3 ca = %s, cb = %s, cc = %s, cd = %s;\n", kColorIn[ca], kColorIn[cb], kColorIn[cc], kColorIn[cd]);
        if (cbias == 3) {
            const char *op = csub ? "==" : ">";
            switch (cscale) {
            case 0: emit("    ivec3 cr = cd + (((ca.r & 255) %s (cb.r & 255)) ? (cc & 255) : ivec3(0));\n", op); break;
            case 1: emit("    ivec3 cr = cd + ((c16(ca) %s c16(cb)) ? (cc & 255) : ivec3(0));\n", op); break;
            case 2: emit("    ivec3 cr = cd + ((c24(ca) %s c24(cb)) ? (cc & 255) : ivec3(0));\n", op); break;
            default:
                emit("    ivec3 cr = cd + ivec3(%s(ca & 255, cb & 255)) * (cc & 255);\n", csub ? "equal" : "greaterThan");
                break;
            }
        } else {
            static const int biasv[3] = { 0, 128 << 8, -(128 << 8) };
            emit("    ivec3 cr = tev3(cd, lerp3(ca, cb, cc), %d, %d, %d);\n", csub, biasv[cbias], cscale);
        }
        emit("    %s.rgb = clamp(cr, %s);\n", kRegName[cdest], cclamp ? "0, 255" : "-1024, 1023");

        /* alpha combiner (reads the registers as they were before this stage's color write) */
        int ad = (int)wgl_bits(ae, 4, 3), ac = (int)wgl_bits(ae, 7, 3), ab = (int)wgl_bits(ae, 10, 3), aa = (int)wgl_bits(ae, 13, 3);
        int abias = (int)wgl_bits(ae, 16, 2), asub = (int)wgl_bits(ae, 18, 1), aclamp = (int)wgl_bits(ae, 19, 1);
        int ascale = (int)wgl_bits(ae, 20, 2), adest = (int)wgl_bits(ae, 22, 2);
        (void)cdest;
        emit("    int aa = %s, ab = %s, ac = %s, ad = %s;\n", kAlphaIn[aa], kAlphaIn[ab], kAlphaIn[ac], kAlphaIn[ad]);
        if (abias == 3) {
            const char *op = asub ? "==" : ">";
            switch (ascale) {
            case 0: emit("    int ar = ad + (((ca.r & 255) %s (cb.r & 255)) ? (ac & 255) : 0);\n", op); break;
            case 1: emit("    int ar = ad + ((c16(ca) %s c16(cb)) ? (ac & 255) : 0);\n", op); break;
            case 2: emit("    int ar = ad + ((c24(ca) %s c24(cb)) ? (ac & 255) : 0);\n", op); break;
            default: emit("    int ar = ad + (((aa & 255) %s (ab & 255)) ? (ac & 255) : 0);\n", op); break;
            }
        } else {
            static const int biasv[3] = { 0, 128 << 8, -(128 << 8) };
            emit("    int ar = tev1(ad, lerp1(aa, ab, ac), %d, %d, %d);\n", asub, biasv[abias], ascale);
        }
        emit("    %s.a = clamp(ar, %s);\n", kRegName[adest], aclamp ? "0, 255" : "-1024, 1023");
        emit("  }\n");
        lastC = cdest;
        lastA = adest;
    }
    /* The color combiner writes before the alpha combiner reads: on hardware
     * both read the registers from before the stage. Inputs are captured into
     * locals (ca..cd / aa..ad) before either write, so emit order is safe as
     * long as the alpha inputs are read before the color result is stored. */

    emit("  ivec4 o = ivec4(clamp(%s.rgb, 0, 255), clamp(%s.a, 0, 255));\n", kRegName[lastC], kRegName[lastA]);

    /* alpha test */
    int comp0 = (int)(atest & 7), comp1 = (int)((atest >> 3) & 7), logic = (int)((atest >> 6) & 3);
    int trivial = (comp0 == 7 && comp1 == 7 && logic <= 1) || (logic == 1 && (comp0 == 7 || comp1 == 7)) ||
                  (logic == 0 && comp0 == 7 && comp1 == 7);
    if (!trivial) {
        char e0[64], e1[64];
        snprintf(e0, sizeof e0, cmp_expr(comp0), "u_aref.x");
        snprintf(e1, sizeof e1, cmp_expr(comp1), "u_aref.y");
        static const char *lops[4] = { "&&", "||", "!=", "==" };
        emit("  { int a = o.a; if (!(%s %s %s)) discard; }\n", e0, lops[logic], e1);
    }

    /* z texture */
    if ((ztex >> 2) & 3) {
        int zfmt = ztex & 3, zop = (ztex >> 2) & 3;
        const char *zv = zfmt == 0 ? "(tex.r & 255)" : zfmt == 1 ? "(((tex.a & 255) << 8) | (tex.r & 255))"
                                                                 : "(((tex.r & 255) << 16) | ((tex.g & 255) << 8) | (tex.b & 255))";
        if (zop == 1)
            emit("  gl_FragDepth = clamp((gl_FragCoord.z * 16777215.0 + float(%s + u_zbias)) / 16777215.0, 0.0, 1.0);\n", zv);
        else
            emit("  gl_FragDepth = clamp(float((%s + u_zbias) & 16777215) / 16777215.0, 0.0, 1.0);\n", zv);
    }

    /* fog: u_fog = (A, B, C, ortho) */
    int fsel = fogSel & 7;
    if (fsel >= 2) {
        emit("  {\n    float z = gl_FragCoord.z;\n");
        emit("    float ze = u_fog.w != 0.0 ? u_fog.x * z : u_fog.x / (u_fog.y - z);\n");
        emit("    float f = clamp(ze - u_fog.z, 0.0, 1.0);\n");
        switch (fsel) {
        case 4: emit("    f = 1.0 - exp2(-8.0 * f);\n"); break;
        case 5: emit("    f = 1.0 - exp2(-8.0 * f * f);\n"); break;
        case 6: emit("    f = exp2(-8.0 * (1.0 - f));\n"); break;
        case 7: emit("    f = exp2(-8.0 * (1.0 - f) * (1.0 - f));\n"); break;
        default: break;
        }
        emit("    int fi = int(round(f * 256.0));\n");
        emit("    o.rgb = (o.rgb * (256 - fi) + ivec3(round(u_fogcolor * 255.0)) * fi + 128) >> 8;\n  }\n");
    }

    if (srcMode == WGL_SRC_INVERT) emit("  o.rgb = 255 - o.rgb; o.a = 255 - o.a;\n");
    else if (srcMode == WGL_SRC_ONE) emit("  o = ivec4(255);\n");
    if (alphaOut == WGL_ALPHAOUT_CONST) emit("  o.a = u_dstalpha;\n");
    emit("  o_col = vec4(o) / 255.0;\n}\n");

    *usedMaps = maps;
    *usedCoords = coords;
}

/* ---- cache ---- */

#define CACHE_SIZE 4096

struct CacheEntry {
    uint32_t hash;
    int used;
    WglKey key;
    WglProgram prog;
};

static CacheEntry *sCache;

static uint32_t key_hash(const WglKey *k)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < k->n; i++) {
        h ^= k->w[i];
        h *= 16777619u;
        h ^= h >> 15;
    }
    return h ? h : 1;
}

void wgl_tev_init(void)
{
    if (!sCache) sCache = (CacheEntry *)calloc(CACHE_SIZE, sizeof(CacheEntry));
    if (!sSrc) {
        sCap = 1 << 16;
        sSrc = (char *)malloc(sCap);
    }
}

static int link_program(WglProgram *p, const WglKey *key)
{
    uint32_t maps, coords;
    gen_fragment(key, &maps, &coords);
    GLuint fs = wgl_shader(GL_FRAGMENT_SHADER, sSrc, "tev fragment");
    if (!fs) return 0;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, wgl_main_vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "a_pos");
    glBindAttribLocation(prog, 1, "a_c0");
    glBindAttribLocation(prog, 2, "a_c1");
    static const char *tn[8] = { "a_t0", "a_t1", "a_t2", "a_t3", "a_t4", "a_t5", "a_t6", "a_t7" };
    for (int i = 0; i < 8; i++) glBindAttribLocation(prog, 3 + i, tn[i]);
    glLinkProgram(prog);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(prog, sizeof log, NULL, log);
        wgl_log("[webgl] TEV program link failed: %s", log);
        glDeleteProgram(prog);
        return 0;
    }
    memset(p, 0, sizeof *p);
    p->prog = prog;
    p->usedMaps = maps;
    p->usedCoords = coords;
    glUseProgram(prog);
    static const char *sn[8] = { "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7" };
    for (int i = 0; i < 8; i++) {
        GLint loc = glGetUniformLocation(prog, sn[i]);
        if (loc >= 0) glUniform1i(loc, i);
    }
    p->uReg = glGetUniformLocation(prog, "u_reg");
    p->uKon = glGetUniformLocation(prog, "u_kon");
    p->uTSize = glGetUniformLocation(prog, "u_tsize");
    p->uSu = glGetUniformLocation(prog, "u_su");
    p->uLodBias = glGetUniformLocation(prog, "u_lodbias");
    p->uAlphaRef = glGetUniformLocation(prog, "u_aref");
    p->uFog = glGetUniformLocation(prog, "u_fog");
    p->uFogColor = glGetUniformLocation(prog, "u_fogcolor");
    p->uDstAlpha = glGetUniformLocation(prog, "u_dstalpha");
    p->uIndM = glGetUniformLocation(prog, "u_indm");
    p->uIndSS = glGetUniformLocation(prog, "u_indss");
    p->uZBias = glGetUniformLocation(prog, "u_zbias");
    p->uVp = glGetUniformLocation(prog, "u_vp");
    p->uDepth = glGetUniformLocation(prog, "u_depth");
    p->uPointSize = glGetUniformLocation(prog, "u_psize");
    p->uIScale = glGetUniformLocation(prog, "u_iscale");
    return 1;
}

WglProgram *wgl_tev_program(const WglKey *key)
{
    uint32_t h = key_hash(key);
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        CacheEntry *e = &sCache[(h + i) & (CACHE_SIZE - 1)];
        if (!e->used) {
            if (!link_program(&e->prog, key)) return NULL;
            e->used = 1;
            e->hash = h;
            e->key = *key;
            wgl_stats_cur.programsCompiled++;
            return &e->prog;
        }
        if (e->hash == h && e->key.n == key->n && !memcmp(e->key.w, key->w, (size_t)key->n * 4)) {
            wgl_stats_cur.programsCached++;
            return &e->prog;
        }
    }
    wgl_log("[webgl] program cache full");
    return NULL;
}

/* Exposed for the test harness: the GLSL of the last generated shader. */
extern "C" const char *gpu_webgl_last_shader(void) { return sSrc ? sSrc : ""; }
