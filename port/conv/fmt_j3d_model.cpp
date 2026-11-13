// J3D2 models: .bmd (bmd2/bmd3) and .bdl (bdl3/bdl4). Spec: J3DModelLoader.cpp,
// J3DMaterialFactory(_v21).cpp, J3DShapeFactory.cpp, J3DJointFactory.cpp.
//
// Header (J3DFileHeader): u32 'J3D2', u32 'bmd3', u32 size, u32 blockCount, 16 bytes
// (SVR3 + pad). Blocks: u32 tag (switched on as a u32), s32 size, then per type;
// every offset in a block is relative to the block start.
//
// Tables are sized the way the loader sizes them: from the referencing indices where
// the loader indexes them, otherwise from the gap up to the next table (or block end),
// minus the trailing alignment padding ("This is padding data to align").
// SHP1 and MDL3 display lists and TEX1 image/palette data stay big-endian.
#include "conv.h"

#include <algorithm>
#include <cstring>

namespace conv {

namespace {

// Alignment padding is written as pieces of this string (each piece restarts at
// "T"), sometimes after zero bytes.
const char kPad[] = "This is padding data to alignment";
const u32 kPadLen = sizeof kPad - 1;

// length of the padding piece (a prefix of kPad) at off, limited to end
u32 padPiece(Swapper &s, u32 off, u32 end)
{
    u32 n = 0;
    while (n < kPadLen && off + n < end && s.data()[off + n] == (u8)kPad[n])
        n++;
    return n;
}

// GXAttr / GXCompType / GXCompCnt values
enum { VA_POS = 9, VA_NRM = 10, VA_CLR0 = 11, VA_TEX0 = 13, VA_NBT = 25, VA_NULL = 0xFF };
enum { CT_U8, CT_S8, CT_U16, CT_S16, CT_F32 };
enum { RGB565, RGB8, RGBX8, RGBA4, RGBA6, RGBA8 };

struct Block {
    Swapper &s;
    u32 b, end;      // absolute block start / end
    Vec<u32> marks;  // block-relative section offsets, sorted

    Block(Swapper &s_, u32 b_, u32 end_) : s(s_), b(b_), end(end_) {}

    // a block-relative u32 offset field (already swapped or not)
    u32 off(u32 field) { return s.r32(b + field); }
    void mark(u32 rel)
    {
        if (rel)
            marks.push_back(rel);
    }
    void sortMarks() { std::sort(marks.begin(), marks.end()); }
    // absolute end of the section starting at block-relative `rel`
    u32 spanEnd(u32 rel)
    {
        for (u32 m : marks)
            if (m > rel)
                return b + m < end ? b + m : end;
        return end;
    }
};

// Start of the trailing alignment padding in [off, end), rounded up so whole
// elements of `elem` bytes are kept.
u32 dataEnd(Swapper &s, u32 off, u32 end, u32 elem)
{
    if (end <= off)
        return off;
    // ok[i]: [end - i, end) is made of padding pieces
    const u32 kMax = 64;
    bool ok[kMax + 1] = {true};
    u32 maxp = std::min<u32>(end - off, kMax), p = 0;
    for (u32 i = 1; i <= maxp; i++) {
        u32 at = end - i, piece = padPiece(s, at, end);
        ok[i] = false;
        for (u32 l = 1; l <= piece && !ok[i]; l++)
            ok[i] = ok[i - l];
        if (ok[i])
            p = i;
    }
    u32 d = end - p;
    if (elem > 1) {
        u32 n = (d - off + elem - 1) / elem;
        d = std::min(end, off + n * elem);
    }
    return d;
}

// element count of a table running from `off` to the next section, padding excluded
u32 countIn(Swapper &s, u32 off, u32 end, u32 elem) { return (dataEnd(s, off, end, elem) - off) / elem; }

// Claim the unclaimed rest of [from, end) as raw if it is alignment padding.
void finish(Swapper &s, u32 from, u32 end, const char *what)
{
    for (u32 i = from; i < end; i++) {
        if (s.claimAt(i) != Swapper::NONE)
            continue;
        u32 j = i;
        while (j < end && s.claimAt(j) == Swapper::NONE)
            j++;
        bool pad = true;
        for (u32 q = i; q < j && pad;) {
            if (s.data()[q] == 0) {
                q++;
                continue;
            }
            u32 l = padPiece(s, q, j);
            pad = l != 0;
            q += l;
        }
        if (!pad)
            s.warn("%s: 0x%x unclaimed non-padding bytes at 0x%x", what, j - i, i);
        s.raw(i, j - i);
        i = j;
    }
}

// ResNTAB: u16 count, u16 pad, {u16 hash, u16 offset}[count], strings
void nameTable(Swapper &s, u32 at, u32 end)
{
    if (!s.check(at, 4, "name table"))
        return;
    u16 n = s.r16(at);
    if (!s.check(at, 4 + n * 4u, "name table entries"))
        return;
    s.w16(at, 2 + n * 2u);
    for (u32 i = 0; i < n; i++) {
        u32 str = at + s.r16(at + 6 + i * 4);
        if (str >= end || str >= s.size()) {
            s.fail("name string %u out of range at 0x%x", i, str);
            return;
        }
        u32 len = (u32)strnlen((const char *)s.data() + str, end - str);
        s.raw(str, std::min(len + 1, end - str));
    }
}

// ---- INF1 ----
void inf1(Block &k)
{
    Swapper &s = k.s;
    s.w16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 3);
    u32 h = k.b + k.off(0x14);
    for (;;) {
        if (!s.check(h, 4, "INF1 hierarchy"))
            return;
        u16 type = s.r16(h);
        s.w16(h, 2);
        h += 4;
        if (type == 0)
            break;
    }
}

// ---- VTX1 ----
struct Fmt {
    u32 cnt, type;
    bool found;
};

void vtx1(Block &k)
{
    Swapper &s = k.s;
    s.w32(k.b + 8, 13);
    for (u32 f = 0; f < 13; f++)
        k.mark(k.off(8 + f * 4));
    k.sortMarks();

    Fmt fmt[26] = {};
    u32 f = k.b + k.off(8);
    if (k.off(8)) {
        for (;;) {
            if (!s.check(f, 16, "VTX1 format"))
                return;
            u32 attr = s.r32(f);
            s.w32(f, 3);
            s.raw(f + 12, 4);
            if (attr == VA_NULL)
                break;
            if (attr < 26)
                fmt[attr] = {s.r32(f + 4), s.r32(f + 8), true};
            f += 16;
        }
    }
    auto getFmt = [&](u32 attr) { return fmt[attr].found ? fmt[attr] : Fmt{1, CT_F32, false}; };

    // array field order: pos, nrm, nbt, clr0, clr1, tex0..7
    for (u32 a = 0; a < 12; a++) {
        u32 rel = k.off(0xC + a * 4);
        if (!rel)
            continue;
        u32 at = k.b + rel, end = k.spanEnd(rel);
        u32 width, comps;
        if (a < 3) {
            Fmt ft = a == 0 ? getFmt(VA_POS) : a == 1 ? getFmt(VA_NRM) : getFmt(fmt[VA_NBT].found ? VA_NBT : VA_NRM);
            width = ft.type == CT_F32 ? 4 : ft.type >= CT_U16 ? 2 : 1;
            comps = a == 0 ? (ft.cnt ? 3 : 2) : a == 1 ? 3 : 9;
        } else if (a < 5) {
            Fmt ft = getFmt(VA_CLR0 + (a - 3));
            static const u8 bytes[6] = {2, 3, 4, 2, 3, 4};
            u32 t = ft.found ? ft.type : RGBA8;
            width = (t == RGB565 || t == RGBA4) ? 2 : 1;
            comps = t < 6 ? bytes[t] / width : 4;
        } else {
            Fmt ft = getFmt(VA_TEX0 + (a - 5));
            width = ft.type == CT_F32 ? 4 : ft.type >= CT_U16 ? 2 : 1;
            comps = ft.cnt ? 2 : 1;
        }
        u32 d = dataEnd(s, at, end, width * comps);
        u32 n = (d - at) / width;
        if (width == 4)
            s.w32(at, n);
        else if (width == 2)
            s.w16(at, n);
        else
            s.raw(at, n);
    }
}

// ---- EVP1 ----
void evp1(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 4);
    for (u32 f = 0; f < 4; f++)
        k.mark(k.off(0xC + f * 4));
    k.sortMarks();
    if (!n)
        return;
    u32 mix = k.b + k.off(0xC), idx = k.b + k.off(0x10), wt = k.b + k.off(0x14), inv = k.b + k.off(0x18);
    if (!s.check(mix, n, "EVP1 mix counts"))
        return;
    u32 total = 0;
    for (u32 i = 0; i < n; i++)
        total += s.r8(mix + i);
    s.raw(mix, n);
    if (!s.check(idx, total * 2, "EVP1 indices") || !s.check(wt, total * 4, "EVP1 weights"))
        return;
    s.w16(idx, total);
    s.w32(wt, total);
    if (k.off(0x18))
        s.w32(inv, countIn(s, inv, k.spanEnd(k.off(0x18)), 0x30) * 12);
}

// ---- DRW1 ----
void drw1(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 2);
    u32 flags = k.b + k.off(0xC), data = k.b + k.off(0x10);
    if (!s.check(flags, n, "DRW1 flags") || !s.check(data, n * 2u, "DRW1 data"))
        return;
    s.raw(flags, n);
    s.w16(data, n);
}

// remap table of u16[count]; returns the number of entries it references
u32 remap(Swapper &s, u32 at, u32 count)
{
    if (!s.check(at, count * 2, "remap table"))
        return 0;
    s.w16(at, count);
    u32 m = 0;
    for (u32 i = 0; i < count; i++)
        m = std::max<u32>(m, s.r16(at + i * 2) + 1u);
    return m;
}

// ---- JNT1 ----
void jnt1(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 3);
    k.mark(k.off(0xC));
    k.mark(k.off(0x10));
    k.mark(k.off(0x14));
    k.sortMarks();
    u32 inits = remap(s, k.b + k.off(0x10), n);
    u32 init = k.b + k.off(0xC);
    if (!s.check(init, inits * 0x40, "JNT1 init data"))
        return;
    for (u32 i = 0; i < inits; i++) {
        u32 e = init + i * 0x40;
        s.w16(e);          // kind
        s.raw(e + 2, 2);   // ignoreParentScale, pad
        s.w32(e + 4, 3);   // scale
        s.w16(e + 0x10, 3); // rotation
        s.raw(e + 0x16, 2); // pad
        s.w32(e + 0x18, 10); // translation, radius, min, max
    }
    if (k.off(0x14))
        nameTable(s, k.b + k.off(0x14), k.spanEnd(k.off(0x14)));
}

// ---- SHP1 ----
void shp1(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 8);
    for (u32 f = 0; f < 8; f++)
        k.mark(k.off(0xC + f * 4));
    k.sortMarks();
    u32 init = k.b + k.off(0xC), desc = k.b + k.off(0x18), mtxTab = k.b + k.off(0x1C);
    u32 dl = k.b + k.off(0x20), mtxInit = k.b + k.off(0x24), drawInit = k.b + k.off(0x28);
    u32 inits = remap(s, k.b + k.off(0x10), n);
    if (!s.check(init, inits * 0x28, "SHP1 init data"))
        return;
    u32 nMtxInit = 0, nDrawInit = 0;
    for (u32 i = 0; i < inits; i++) {
        u32 e = init + i * 0x28;
        s.raw(e, 2);        // mtx type, pad
        s.w16(e + 2, 4);    // group num, vtx desc offset, mtx init index, draw init index
        s.raw(e + 0xA, 2);  // pad
        s.w32(e + 0xC, 7);  // radius, min, max
        u32 groups = s.r16(e + 2);
        nMtxInit = std::max(nMtxInit, s.r16(e + 6) + groups);
        nDrawInit = std::max(nDrawInit, s.r16(e + 8) + groups);
        // vertex descriptor list, terminated by GX_VA_NULL
        for (u32 d = desc + s.r16(e + 4);; d += 8) {
            if (!s.check(d, 8, "SHP1 vtx desc"))
                return;
            u32 attr = s.r32(d);
            s.w32(d, 2);
            if (attr == VA_NULL)
                break;
        }
    }
    if (!s.check(mtxInit, nMtxInit * 8, "SHP1 mtx init") || !s.check(drawInit, nDrawInit * 8, "SHP1 draw init"))
        return;
    u32 nMtx = 0;
    for (u32 i = 0; i < nMtxInit; i++) {
        u32 e = mtxInit + i * 8;
        s.w16(e, 2);
        s.w32(e + 4);
        nMtx = std::max(nMtx, s.r32(e + 4) + s.r16(e + 2));
    }
    if (nMtx && s.check(mtxTab, nMtx * 2, "SHP1 mtx table"))
        s.w16(mtxTab, nMtx);
    for (u32 i = 0; i < nDrawInit; i++) {
        u32 e = drawInit + i * 8;
        s.w32(e, 2);
        u32 size = s.r32(e), at = dl + s.r32(e + 4);
        if (s.check(at, size, "SHP1 display list"))
            s.raw(at, size); // GPU display list: stays big-endian
    }
    if (k.off(0x14))
        nameTable(s, k.b + k.off(0x14), k.spanEnd(k.off(0x14)));
}

// ---- MAT3 / MAT2 ----
enum Elem {
    E_NONE,
    E_RAW1,  // u8
    E_RAW4,  // 4 x u8 (GXColor, small infos)
    E_RAW8,
    E_RAW20, // J3DTevStageInfo
    E_U16,
    E_U32,   // GXCullMode
    E_S10,   // GXColorS10
    E_TEXMTX,
    E_FOG,
    E_NBT,
    E_IND,
    E_LIGHT,
    E_NAMES,
};

void texMtx(Swapper &s, u32 e)
{
    s.raw(e, 4);         // projection, info, pad
    s.w32(e + 4, 5);     // center, scale x/y
    s.w16(e + 0x18);     // rotation
    s.raw(e + 0x1A, 2);  // pad
    s.w32(e + 0x1C, 18); // translation x/y, effect mtx
}

void fog(Swapper &s, u32 e)
{
    s.raw(e, 2);
    s.w16(e + 2);
    s.w32(e + 4, 4);
    s.raw(e + 0x14, 4);
    s.w16(e + 0x18, 10);
}

void ind(Swapper &s, u32 e)
{
    s.raw(e, 0x14); // enabled, stage num, pad, tex orders
    for (u32 m = 0; m < 3; m++) {
        s.w32(e + 0x14 + m * 0x1C, 6);
        s.raw(e + 0x2C + m * 0x1C, 4);
    }
    s.raw(e + 0x68, 0x138 - 0x68); // coord scales, tev stages
}

void table(Swapper &s, Elem t, u32 at, u32 end)
{
    static const u32 size[] = {0, 1, 4, 8, 20, 2, 4, 8, 0x64, 0x2C, 0x10, 0x138, 0x30, 0};
    if (t == E_NAMES) {
        nameTable(s, at, end);
        return;
    }
    u32 n = countIn(s, at, end, size[t]);
    for (u32 i = 0; i < n; i++) {
        u32 e = at + i * size[t];
        switch (t) {
        case E_RAW1:
        case E_RAW4:
        case E_RAW8:
        case E_RAW20:
            s.raw(e, size[t]);
            break;
        case E_U16:
            s.w16(e);
            break;
        case E_U32:
            s.w32(e);
            break;
        case E_S10:
            s.w16(e, 4);
            break;
        case E_TEXMTX:
            texMtx(s, e);
            break;
        case E_FOG:
            fog(s, e);
            break;
        case E_NBT:
            s.raw(e, 4);
            s.w32(e + 4, 3);
            break;
        case E_IND:
            ind(s, e);
            break;
        case E_LIGHT:
            // J3DLightInfo: never read by the loader; position/direction/color/
            // attenuation, all words
            s.w32(e, 12);
            break;
        default:
            break;
        }
    }
}

// block-relative header offsets 0x0C.. (init data and remap handled separately)
const Elem kMat3[] = {
    E_NONE, E_NONE, E_NAMES, E_IND, E_U32, E_RAW4, E_RAW1, E_RAW8, E_RAW4, E_LIGHT,
    E_RAW1, E_RAW4, E_RAW4, E_TEXMTX, E_TEXMTX, E_U16, E_RAW4, E_S10, E_RAW4, E_RAW1,
    E_RAW20, E_RAW4, E_RAW4, E_FOG, E_RAW8, E_RAW4, E_RAW4, E_RAW1, E_RAW1, E_NBT,
};
const Elem kMat2[] = {
    E_NONE, E_NONE, E_NAMES, E_U32, E_RAW4, E_RAW1, E_RAW8, E_RAW1, E_RAW4, E_RAW4,
    E_TEXMTX, E_TEXMTX, E_U16, E_RAW4, E_S10, E_RAW4, E_RAW1, E_RAW20, E_RAW4, E_RAW4,
    E_FOG, E_RAW8, E_RAW4, E_RAW4, E_RAW1, E_RAW1, E_NBT,
};

void mat(Block &k, bool v21)
{
    Swapper &s = k.s;
    const Elem *tabs = v21 ? kMat2 : kMat3;
    u32 nTabs = v21 ? sizeof kMat2 / sizeof kMat2[0] : sizeof kMat3 / sizeof kMat3[0];
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, nTabs);
    for (u32 f = 0; f < nTabs; f++)
        k.mark(k.off(0xC + f * 4));
    k.sortMarks();
    u32 inits = remap(s, k.b + k.off(0x10), n);
    u32 init = k.b + k.off(0xC), isz = v21 ? 0x138 : 0x14C;
    if (!s.check(init, inits * isz, "material init data"))
        return;
    for (u32 i = 0; i < inits; i++) {
        u32 e = init + i * isz;
        s.raw(e, 8); // mode, cull/chan/texgen/stage/zcomp/zmode/dither indices
        if (v21) {
            s.w16(e + 8, (0x88 - 8) / 2);
            s.raw(e + 0x88, 0x20); // kcolor/kalpha sel
            s.w16(e + 0xA8, (0x138 - 0xA8) / 2);
        } else {
            s.w16(e + 8, (0x9C - 8) / 2);
            s.raw(e + 0x9C, 0x20);
            s.w16(e + 0xBC, (0x14C - 0xBC) / 2);
        }
    }
    for (u32 f = 2; f < nTabs; f++) {
        u32 rel = k.off(0xC + f * 4);
        if (rel)
            table(s, tabs[f], k.b + rel, k.spanEnd(rel));
    }
}

// ---- MDL3 (bdl) ----
void mdl3(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 6);
    for (u32 f = 0; f < 6; f++)
        k.mark(k.off(0xC + f * 4));
    k.sortMarks();
    u32 dli = k.b + k.off(0xC), patch = k.b + k.off(0x10), cur = k.b + k.off(0x14);
    u32 mode = k.b + k.off(0x18), unk = k.b + k.off(0x1C);
    if (!s.check(dli, n * 8u, "MDL3 DL init") || !s.check(patch, n * 0x10u, "MDL3 patching") ||
        !s.check(cur, n * 8u, "MDL3 current mtx") || !s.check(mode, n, "MDL3 draw mode"))
        return;
    for (u32 i = 0; i < n; i++) {
        u32 e = dli + i * 8;
        s.w32(e, 2);
        u32 at = e + s.r32(e), size = s.r32(e + 4);
        if (s.check(at, size, "MDL3 display list"))
            s.raw(at, size); // GPU display list: stays big-endian
        s.w16(patch + i * 0x10, 6);
        s.raw(patch + i * 0x10 + 0xC, 4);
    }
    s.w32(cur, n * 2);
    s.raw(mode, n);
    // 0x1C: u16 per material, not read by the loader (material index remap)
    if (k.off(0x1C) && s.check(unk, n * 2u, "MDL3 0x1C table"))
        s.w16(unk, n);
    if (k.off(0x20))
        nameTable(s, k.b + k.off(0x20), k.spanEnd(k.off(0x20)));
}

// ---- TEX1 ----
void tex1(Block &k)
{
    Swapper &s = k.s;
    u16 n = s.g16(k.b + 8);
    s.raw(k.b + 0xA, 2);
    s.w32(k.b + 0xC, 2);
    k.mark(k.off(0xC));
    k.mark(k.off(0x10));
    k.sortMarks();
    u32 h = k.b + k.off(0xC);
    if (!s.check(h, n * 0x20u, "TEX1 headers"))
        return;
    for (u32 i = 0; i < n; i++) {
        u32 e = h + i * 0x20;
        s.raw(e, 2);        // format, transparency
        s.w16(e + 2, 2);    // width, height
        s.raw(e + 6, 4);    // wrap s/t, palette flag/format
        s.w16(e + 0xA);     // palette entries
        s.w32(e + 0xC);     // palette offset
        s.raw(e + 0x10, 0xA); // mip, edge lod, bias clamp, aniso, filters, lods, image count, pad
        s.w16(e + 0x1A);    // lod bias
        s.w32(e + 0x1C);    // image offset
    }
    if (k.off(0x10))
        nameTable(s, k.b + k.off(0x10), k.spanEnd(k.off(0x10)));
    // image and palette data: GPU, stays big-endian
    for (u32 i = k.b; i < k.end; i++)
        if (s.claimAt(i) == Swapper::NONE)
            s.raw(i, 1);
}

} // namespace

void convJ3DModel(Swapper &s)
{
    if (!s.check(0, 0x20, "J3D header"))
        return;
    u32 magic = s.r32(0), type = s.r32(4);
    if (magic != 0x4A334432) { // J3D2
        s.fail("not a J3D2 model (magic 0x%08x)", magic);
        return;
    }
    if (type != 0x626D6432 && type != 0x626D6433 && type != 0x62646C33 && type != 0x62646C34)
        s.warn("unexpected J3D model type 0x%08x", type);
    s.w32(0, 4); // magic, type, file size, block count
    s.raw(0x10, 0x10);
    u32 n = s.r32(0xC), off = 0x20;
    for (u32 i = 0; i < n && s.ok(); i++) {
        if (!s.check(off, 8, "J3D block"))
            return;
        u32 tag = s.g32(off), size = s.g32(off + 4);
        if (size < 8 || !s.check(off, size, "J3D block body"))
            return;
        Block k(s, off, off + size);
        const char *what = "block";
        switch (tag) {
        case 0x494E4631: inf1(k); what = "INF1"; break;
        case 0x56545831: vtx1(k); what = "VTX1"; break;
        case 0x45565031: evp1(k); what = "EVP1"; break;
        case 0x44525731: drw1(k); what = "DRW1"; break;
        case 0x4A4E5431: jnt1(k); what = "JNT1"; break;
        case 0x53485031: shp1(k); what = "SHP1"; break;
        case 0x4D415433: mat(k, false); what = "MAT3"; break;
        case 0x4D415432: mat(k, true); what = "MAT2"; break;
        case 0x4D444C33: mdl3(k); what = "MDL3"; break;
        case 0x54455831: tex1(k); what = "TEX1"; break;
        default:
            s.fail("unknown J3D block 0x%08x at 0x%x", tag, off);
            return;
        }
        if (!s.ok())
            return;
        finish(s, off, off + size, what);
        off += size;
    }
    if (off < s.size())
        finish(s, off, s.size(), "trailer");
}

} // namespace conv
