// JParticle resource containers, version "2-10" (JPAResourceLoader::load_jpc).
// Header: u32 magic "JPAC", u32 version '2-10' (compared as int), u16 resNum,
// u16 texNum, u32 texOffset. Resources start at 0x10: u16 usrIdx, u16 blockNum,
// u8 fldNum, keyNum, texNum, pad; then blocks {u32 tag (compared as u32), u32 size}.
// Textures at texOffset: TEX1 blocks {u32 tag, u32 size, u32, char name[0x14],
// ResTIMG at 0x20}; pixel/palette data stays big-endian.
#include "conv.h"

namespace conv {

namespace {

// claim any bytes of [off, end) no field claimed as raw (padding, u8 tables, colors)
void rawGaps(Swapper &s, u32 off, u32 end)
{
    u32 i = off;
    while (i < end) {
        if (s.claimAt(i) != Swapper::NONE) {
            i++;
            continue;
        }
        u32 j = i;
        while (j < end && s.claimAt(j) == Swapper::NONE)
            j++;
        s.raw(i, j - i);
        i = j;
    }
}

bool need(Swapper &s, u32 b, u32 size, u32 len, const char *what)
{
    if (size < len) {
        s.fail("JPA %s block at 0x%x too small (0x%x < 0x%x)", what, b, size, len);
        return false;
    }
    return true;
}

void convBEM1(Swapper &s, u32 b, u32 size)
{
    // JPADynamicsBlockData
    if (!need(s, b, size, 0x7C, "BEM1"))
        return;
    s.w32(b + 0x08, 2);    // flags, resUserWork
    s.w32(b + 0x10, 0x16); // emitter scl/trs/dir, 13 f32 parameters (0x10..0x68)
    s.w16(b + 0x68, 7);    // rot[3], maxFrame, startFrame, lifeTime, volumeSize
    s.w16(b + 0x76);       // divNumber
    // 0x78 u8 rateStep, then padding
}

void convFLD1(Swapper &s, u32 b, u32 size)
{
    // JPAFieldBlock::Data
    if (!need(s, b, size, 0x44, "FLD1"))
        return;
    s.w32(b + 0x08);       // flags (type/addType/sttFlag)
    s.w32(b + 0x0C, 0x0D); // offset, velocity, amplitude, magRndm, val1, fade/en/dis times
    // 0x40 u8 cycle
}

void convKFA1(Swapper &s, u32 b, u32 size)
{
    // JPAKeyBlockData then keyFrameCount * {frame, value, tanIn, tanOut} f32
    if (!need(s, b, size, 0x0C, "KFA1"))
        return;
    u32 n = s.r8(b + 0x09);
    if (!need(s, b, size, 0x0C + n * 16, "KFA1 keys"))
        return;
    s.w32(b + 0x0C, n * 4);
}

void convBSP1(Swapper &s, u32 b, u32 size)
{
    // JPABaseShapeData (0x34), then optional tex-coord anm table (10 f32),
    // tex index anm table (u8), color key tables {s16 index, GXColor}
    if (!need(s, b, size, 0x34, "BSP1"))
        return;
    s.w32(b + 0x08);    // flags
    s.w16(b + 0x0C, 2); // clrPrmAnmOffset, clrEnvAnmOffset
    s.w32(b + 0x10, 2); // baseSizeX/Y
    s.w16(b + 0x18);    // blendModeCfg
    s.w16(b + 0x24);    // clrAnmFrmMax
    u32 flags = s.r32(b + 0x08);
    u8 clrFlg = s.r8(b + 0x21); // tex index anm table (texFlg & 1) is u8
    if (flags & 0x01000000) {
        if (!need(s, b, size, 0x34 + 0x28, "BSP1 texcrd"))
            return;
        s.w32(b + 0x34, 10);
    }
    if (clrFlg & 0x02) {
        u32 off = (u16)s.r16(b + 0x0C), n = s.r8(b + 0x22);
        if (!need(s, b, size, off + n * 6, "BSP1 prm colors"))
            return;
        for (u32 i = 0; i < n; i++)
            s.w16(b + off + i * 6);
    }
    if (clrFlg & 0x08) {
        u32 off = (u16)s.r16(b + 0x0E), n = s.r8(b + 0x23);
        if (!need(s, b, size, off + n * 6, "BSP1 env colors"))
            return;
        for (u32 i = 0; i < n; i++)
            s.w16(b + off + i * 6);
    }
}

void convESP1(Swapper &s, u32 b, u32 size)
{
    // JPAExtraShapeData
    if (!need(s, b, size, 0x60, "ESP1"))
        return;
    s.w32(b + 0x08, 8);    // flags, scale timing/values/random
    s.w16(b + 0x28, 2);    // scaleAnmCycleX/Y
    s.w32(b + 0x2C, 0x0D); // alpha and rotate parameters
}

void convSSP1(Swapper &s, u32 b, u32 size)
{
    // JPAChildShapeData
    if (!need(s, b, size, 0x48, "SSP1"))
        return;
    s.w32(b + 0x08, 11); // flags, posRndm .. inheritRGB
    // 0x34 prm, 0x38 env GXColor
    s.w32(b + 0x3C);     // timing
    s.w16(b + 0x40, 2);  // life, rate
    // 0x44 u8 step, 0x45 u8 texIdx
    s.w16(b + 0x46);     // rotSpeed
}

void convETX1(Swapper &s, u32 b, u32 size)
{
    // JPAExTexShapeData
    if (!need(s, b, size, 0x27, "ETX1"))
        return;
    s.w32(b + 0x08, 7); // flags, Mtx23 indTexMtx
    // 0x24 s8 expScale, indTexIdx, secTexIdx
}

void convTDB1(Swapper &s, u32 b, u32 size, u32 texNum)
{
    // u16 texIdxTbl[texNum] (JPAResource::getTexIdx)
    if (!need(s, b, size, 8 + texNum * 2, "TDB1"))
        return;
    s.w16(b + 8, texNum);
}

void convTEX1(Swapper &s, u32 b, u32 size)
{
    if (!need(s, b, size, 0x40, "TEX1"))
        return;
    // ResTIMG header at 0x20 (offsets relative to it); image/palette data raw
    u32 t = b + 0x20;
    s.w16(t + 0x02, 2);
    s.w16(t + 0x0A);
    s.w32(t + 0x0C);
    s.w16(t + 0x1A);
    s.w32(t + 0x1C);
}

} // namespace

void convJPA(Swapper &s)
{
    if (!s.check(0, 0x10, "JPA header"))
        return;
    if (s.r32(0) != 0x4A504143 /* JPAC */ || s.r32(4) != 0x322D3130 /* 2-10 */) {
        s.fail("JPA: unsupported magic/version 0x%08x 0x%08x", s.r32(0), s.r32(4));
        return;
    }
    s.w32(0, 2);
    u32 resNum = s.g16(8), texNum = s.g16(0xA), texOff = s.g32(0xC);

    u32 off = 0x10;
    for (u32 i = 0; i < resNum && s.ok(); i++) {
        if (!s.check(off, 8, "JPA resource"))
            return;
        s.w16(off, 2); // usrIdx, blockNum
        s.raw(off + 4, 4); // u8 fldNum, keyNum, texNum, pad
        u32 blocks = s.r16(off + 2), resTex = s.r8(off + 6);
        off += 8;
        for (u32 j = 0; j < blocks && s.ok(); j++) {
            if (!s.check(off, 8, "JPA block"))
                return;
            u32 tag = s.g32(off), size = s.g32(off + 4);
            if (size < 8 || !s.check(off, size, "JPA block body"))
                return;
            switch (tag) {
            case 0x42454D31: convBEM1(s, off, size); break; // BEM1
            case 0x464C4431: convFLD1(s, off, size); break; // FLD1
            case 0x4B464131: convKFA1(s, off, size); break; // KFA1
            case 0x42535031: convBSP1(s, off, size); break; // BSP1
            case 0x45535031: convESP1(s, off, size); break; // ESP1
            case 0x53535031: convSSP1(s, off, size); break; // SSP1
            case 0x45545831: convETX1(s, off, size); break; // ETX1
            case 0x54444231: convTDB1(s, off, size, resTex); break; // TDB1
            default:
                s.fail("unknown JPA block 0x%08x at 0x%x", tag, off);
                return;
            }
            rawGaps(s, off + 8, off + size);
            off += size;
        }
    }
    if (!s.ok())
        return;
    if (texOff < off || texOff > s.size()) {
        s.fail("JPA texture offset 0x%x out of order (resources end at 0x%x)", texOff, off);
        return;
    }
    rawGaps(s, off, texOff);
    off = texOff;
    for (u32 i = 0; i < texNum && s.ok(); i++) {
        if (!s.check(off, 8, "JPA texture"))
            return;
        u32 tag = s.g32(off), size = s.g32(off + 4);
        if (tag != 0x54455831 /* TEX1 */ || size < 8 || !s.check(off, size, "JPA texture body")) {
            s.fail("bad JPA texture block 0x%08x size 0x%x at 0x%x", tag, size, off);
            return;
        }
        s.w32(off + 8);
        convTEX1(s, off, size);
        rawGaps(s, off + 8, off + size);
        off += size;
    }
    if (s.ok() && off < s.size())
        rawGaps(s, off, s.size());
}

} // namespace conv
