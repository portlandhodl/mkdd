// Small game parameter files, dispatched on the extension of s.name():
//   .set  KartBody::mSettingPtrs (Yamamoto/kartBody.cpp): f32 array
//   .ikp  NeckIKParam (Sato/NeckCtrl.h), via CharIKParamMgr
//   .lbd  log bridge params (Sato/logbridge.cpp): f32[9], u8[4], f32[5]
//   .kel  stEffectMgr::loadCourseParam: u8 course ids/counts + stGndAttrParam (all u8/char)
//   .pel  JPEffectPerformer::loadCourseParam: u8 course ids/counts + 75-byte u8/char records
//   .iop  ItemObj::ItemHandOffsetData[] (ItemObjMgr)
//   .dat  EnemyKart.dat (RivalSpeedCtrl, s16 array), GeoHitTable.dat (SiUtil::setItemReaction,
//         read through JSUMemoryInputStream), ItemSlotList*.dat (ItemShuffleMgr, u8 only)
//   .snp  stEmitter template (Sato/stEmitter.h)
//   .ght  KartPadRecord (Osako/KartPadRecord.h), staff ghosts
#include "conv.h"

#include <cctype>
#include <cstring>

namespace conv {

static Str lower(const Str &in)
{
    Str o = in;
    for (auto &c : o)
        c = (char)tolower((unsigned char)c);
    return o;
}

// .set: read as f32 *unknownSettings[i]
static void convSet(Swapper &s)
{
    if (s.size() % 4) {
        s.fail("set: size not a multiple of 4");
        return;
    }
    s.w32(0, s.size() / 4);
}

// .ikp: NeckIKParam, 0x64
static void convIkp(Swapper &s)
{
    if (!s.check(0, 0x64, "NeckIKParam"))
        return;
    s.w32(0x00, 11);  // m0..m28
    s.raw(0x2C, 4);   // m2c, m2d, _2e[2]
    s.w32(0x30, 10);  // m30[5], m44..m54
    s.raw(0x58, 4);   // m58, _59[3]
    s.w32(0x5C, 2);   // m5c, m60
    s.raw(0x64, s.size() - 0x64);
}

// .lbd: LogBridge reads ((f32 *)p)[0..8], ((u8 *)p)[0x24..0x27], ((f32 *)p)[0xa..0xe]
static void convLbd(Swapper &s)
{
    if (!s.check(0, 0x3C, "bridge param"))
        return;
    s.w32(0x00, 9);
    s.raw(0x24, 4);
    s.w32(0x28, 5);
    s.raw(0x3C, s.size() - 0x3C);
}

// .iop: ItemHandOffsetData { TVec3f; s16[3]; } padded to 0x14
static void convIop(Swapper &s)
{
    u32 n = s.size() / 0x14;
    for (u32 i = 0; i < n; i++) {
        u32 o = i * 0x14;
        s.w32(o, 3);
        s.w16(o + 0xC, 3);
        s.raw(o + 0x12, 2);
    }
    s.raw(n * 0x14, s.size() - n * 0x14);
}

// .snp: stEmitter, 0xd8. Some files are shorter (tool01.snp 0xbc, kinokodash*
// 0xc0): older layouts that end early; convert the fields that are present.
static void convSnp(Swapper &s)
{
    static const struct { u16 off, len; char w; } kFields[] = {
        {0x00, 0x18, 'w'}, // _00, _0c
        {0x18, 4, 'b'},    // mCreateRate + pad
        {0x1C, 4, 'w'},    // _1c
        {0x20, 2, 'b'},    // mRateStep + pad
        {0x22, 2, 'h'},    // mLife
        {0x24, 0x84, 'w'}, // mLifeRndRate .. _9c
        {0xA8, 14, 'b'},   // 4x stColorRGB, _b4, pad
        {0xB6, 2, 'h'},    // mFlags
        {0xB8, 4, 'b'},    // mTevMode, mKind, mDrawType, _bb
        {0xBC, 4, 'w'},    // _bc
        {0xC0, 4, 'b'},    // mOrderColor
        {0xC4, 0x14, 'w'}, // _c4 .. mSizeYLastRate
    };
    u32 end = 0;
    for (const auto &f : kFields) {
        if (f.off + f.len > s.size())
            break;
        if (f.w == 'w')
            s.w32(f.off, f.len / 4);
        else if (f.w == 'h')
            s.w16(f.off, f.len / 2);
        else
            s.raw(f.off, f.len);
        end = f.off + f.len;
    }
    if (end < 0xBC)
        s.fail("stEmitter too short (0x%x)", s.size());
    else if (end < s.size())
        s.warn("stEmitter: 0x%x bytes past the last whole field", s.size() - end);
    s.raw(end, s.size() - end);
}

// .ght: KartPadRecord. KartPadData's first byte is the bitfield pair
// { s8 mStickX : 5; s8 mStickY : 3; }: MWCC allocates bitfields from the most
// significant bit, little-endian clang from the least significant bit.
static void convGht(Swapper &s)
{
    if (!s.check(0, 0x28, "KartPadRecord header"))
        return;
    s.raw(0x00, 8);   // mCharIDs, mKartID, mCourseID, mName
    s.w32(0x08);      // mTime
    s.w32(0x0C);      // mFrame
    s.w32(0x10, 6);   // mLapSplits
    u32 n = (s.size() - 0x28) / 2;
    if (n != 17864)
        s.warn("ght: %u pad frames (expected 17864)", n);
    u8 *d = s.data();
    for (u32 i = 0; i < n; i++) {
        u8 &v = d[0x28 + i * 2];
        if (s.srcBig()) // stickX bits 7..3, stickY 2..0 -> stickX 4..0, stickY 7..5
            v = (u8)(v >> 3 | (v & 7) << 5);
        else
            v = (u8)((v & 0x1F) << 3 | v >> 5);
    }
    s.raw(0x28, s.size() - 0x28);
}

// GeoHitTable.dat, read by SiUtil::setItemReaction through JSUMemoryInputStream:
// s32 size, u16 nibbleCount, u16 recordCount, then records { u16 kind; u8 nibbles[] }
// (unaligned). The stride used when skipping a record is (u32)(0.5f * n + 2) - 2.
static void convGeoHitTable(Swapper &s)
{
    if (!s.check(0, 8, "GeoHitTable header"))
        return;
    u32 size = s.g32(0);
    if (size != s.size())
        s.warn("GeoHitTable: size field 0x%x != file size 0x%x", size, s.size());
    u32 nib = s.g16(4), num = s.g16(6);
    u32 stride = nib / 2;
    u32 off = 8;
    for (u32 i = 0; i < num && s.ok(); i++) {
        if (!s.check(off, 2 + stride, "GeoHitTable record"))
            return;
        s.w16(off);
        s.raw(off + 2, stride);
        off += 2 + stride;
    }
    s.raw(off, s.size() - off);
}

static void convDat(Swapper &s)
{
    const Str &n = s.name();
    size_t slash = n.rfind('/');
    Str base = lower(slash == Str::npos ? n : n.substr(slash + 1));
    if (base == "enemykart.dat") {
        // RivalSpeedCtrl::LoadCrsLevelParam: read as s16 throughout
        if (s.size() % 2) {
            s.fail("EnemyKart.dat: odd size");
            return;
        }
        s.w16(0, s.size() / 2);
    } else if (base == "geohittable.dat") {
        convGeoHitTable(s);
    } else if (base.compare(0, 12, "itemslotlist") == 0) {
        // ItemShuffleMgr: u8 counts and u8 item kinds only
        s.raw(0, s.size());
    } else {
        s.fail("unknown .dat file");
    }
}

void convGameParam(Swapper &s)
{
    const Str &n = s.name();
    size_t dot = n.rfind('.');
    Str ext = lower(dot == Str::npos ? Str() : n.substr(dot + 1));
    if (ext == "set")
        convSet(s);
    else if (ext == "ikp")
        convIkp(s);
    else if (ext == "lbd")
        convLbd(s);
    else if (ext == "kel" || ext == "pel")
        s.raw(0, s.size()); // byte-only tables (ids, counts, names)
    else if (ext == "iop")
        convIop(s);
    else if (ext == "dat")
        convDat(s);
    else if (ext == "snp")
        convSnp(s);
    else if (ext == "ght")
        convGht(s);
    else
        s.fail("convGameParam: unhandled extension");
}

} // namespace conv
