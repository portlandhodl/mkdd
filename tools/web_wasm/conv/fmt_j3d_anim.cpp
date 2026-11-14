// J3D1 animation files (J3DAnmLoader.cpp, J3DClusterLoader.cpp) and .bas anime sound
// (JAUSoundAnimation, cast directly onto the file by DriverModel).
//
// File header (J3DFileHeader): u32 'J3D1', u32 'bck1'..., u32 size, u32 blockCount,
// 16 bytes raw (SVR1 tag / 0xff). Blocks: u32 tag, u32 size, then per tag. All tags
// are compared as u32 by the loaders. Offsets in blocks are relative to the block.
//
// Key tables (J3DAnmKeyTableBase) are u16 {count, offset, type}; full tables are u16
// {count, offset}. Values: transform scale/translation f32, rotation s16; key color
// values s16; full color values u8; cluster weights f32; texture pattern values u16.
// Section sizes come from the counts in the block headers; the bytes between
// sections must be the J3D tool's "This is padding data to alignment" filler.
#include "conv.h"

#include <cstring>

namespace conv {

namespace {

const char kPad[] = "This is padding data to alignment.......";

// [off, off+len) is filler: a prefix of the padding string, or 0x00/0xff bytes
bool isPadding(Swapper &s, u32 off, u32 len)
{
    const u8 *p = s.data() + off;
    if (len <= sizeof kPad - 1 && !memcmp(p, kPad, len))
        return true;
    for (u32 i = 0; i < len; i++)
        if (p[i] && p[i] != 0xff)
            return false;
    return true;
}

// Claim every unclaimed byte in [off, end) as raw, warning about gaps that do not
// look like padding (they would hint at a miscounted section).
void fillGaps(Swapper &s, u32 off, u32 end)
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
        if (!isPadding(s, i, j - i))
            s.warn("non-padding gap 0x%x..0x%x", i, j);
        s.raw(i, j - i);
        i = j;
    }
}

// Typed sections relative to a block. Zero offset means absent.
struct Blk {
    Swapper &s;
    u32 b, end;

    u32 off32(u32 at) { return s.r32(b + at); }
    bool sec(u32 rel, u32 bytes, const char *what)
    {
        if (!bytes)
            return false;
        if (!rel || rel > end - b || bytes > end - b - rel) {
            s.fail("%s 0x%x+0x%x outside block 0x%x..0x%x", what, rel, bytes, b, end);
            return false;
        }
        return true;
    }
    void a16(u32 rel, u32 n, const char *what)
    {
        if (sec(rel, n * 2, what))
            s.w16(b + rel, n);
    }
    void a32(u32 rel, u32 n, const char *what)
    {
        if (sec(rel, n * 4, what))
            s.w32(b + rel, n);
    }
    void a8(u32 rel, u32 n, const char *what)
    {
        if (sec(rel, n, what))
            s.raw(b + rel, n);
    }
    // Table of `stride`-byte records, `words` u16 each then raw bytes, count unknown:
    // the records run from `rel` to `next` minus the trailing padding.
    void tableTo(u32 rel, u32 next, u32 stride, u32 words, const char *what)
    {
        if (!rel || next <= rel || !sec(rel, next - rel, what))
            return;
        u32 p = b + rel, e = b + next;
        while (p + stride <= e && !(e - p < sizeof kPad && isPadding(s, p, e - p)))
            p += stride;
        for (u32 q = b + rel; q < p; q += stride) {
            s.w16(q, words);
            if (stride > words * 2)
                s.raw(q + words * 2, stride - words * 2);
        }
    }
    // ResNTAB: u16 count, u16 pad, {u16 hash, u16 offset}[count], NUL-terminated strings
    void nameTab(u32 rel)
    {
        if (!rel || !sec(rel, 4, "name table"))
            return;
        u32 t = b + rel;
        u16 n = s.g16(t);
        s.w16(t + 2);
        if (!sec(rel, 4 + n * 4u, "name table entries"))
            return;
        s.w16(t + 4, n * 2u);
        for (u32 i = 0; i < n; i++) {
            u32 str = t + s.r16(t + 4 + i * 4 + 2);
            u32 e = str;
            while (e < end && s.data()[e])
                e++;
            if (e >= end) {
                s.fail("name table string at 0x%x unterminated", str);
                return;
            }
            s.raw(str, e + 1 - str);
        }
    }
};

// ANK1 (bck) / ANF1 (bca): u8 attr, u8 rotScale, s16 frames, u16 joints, u16 scale/rot/trans
// counts, offsets: table, scale f32, rot s16, trans f32.
void blockTransform(Blk &k, bool key)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 5);
    s.w32(b + 0x14, 4);
    u32 joints = s.r16(b + 0xC);
    // per joint, per axis: {scale, rotation, translation} key tables (3 u16) or full
    // tables (2 u16)
    k.a16(k.off32(0x14), joints * 3 * 3 * (key ? 3 : 2), "transform table");
    k.a32(k.off32(0x18), s.r16(b + 0xE), "scale values");
    k.a16(k.off32(0x1C), s.r16(b + 0x10), "rotation values");
    k.a32(k.off32(0x20), s.r16(b + 0x12), "translation values");
}

// TTK1 (btk), J3DAnmTextureSRTKeyData
void blockTexSRT(Blk &k)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 5);
    s.w32(b + 0x14, 8);
    s.w16(b + 0x34, 4);
    s.w32(b + 0x3C, 9);
    // two track sets: main (counts at 0x0C, offsets at 0x14) and post (0x34, 0x3C)
    static const u32 kCount[2] = {0xC, 0x34}, kOff[2] = {0x14, 0x3C};
    for (int set = 0; set < 2; set++) {
        u32 c = kCount[set], o = kOff[set];
        u32 n = s.r16(b + c); // material count * 3
        u32 mats = n / 3;
        k.a16(k.off32(o), n * 9, "SRT table");
        k.a16(k.off32(o + 4), mats, "SRT material ids");
        k.nameTab(k.off32(o + 8));
        k.a8(k.off32(o + 12), mats, "SRT tex mtx ids");
        k.a32(k.off32(o + 16), mats * 3, "SRT centers");
        k.a32(k.off32(o + 20), s.r16(b + c + 2), "SRT scale values");
        k.a16(k.off32(o + 24), s.r16(b + c + 4), "SRT rotation values");
        k.a32(k.off32(o + 28), s.r16(b + c + 6), "SRT translation values");
    }
}

// PAK1 (bpk) / PAF1 (bpa): u8 attr + 3 pad, s16 frames, u16 materials, u16 R/G/B/A
// counts, offsets: table, material ids, name table, R, G, B, A
void blockColor(Blk &k, bool key)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 4);
    s.w16(b + 0xC, 6);
    s.w32(b + 0x18, 7);
    u32 mats = s.r16(b + 0xE);
    k.a16(k.off32(0x18), mats * (key ? 12 : 8), "color table");
    k.a16(k.off32(0x1C), mats, "color material ids");
    k.nameTab(k.off32(0x20));
    for (u32 i = 0; i < 4; i++) {
        u32 n = s.r16(b + 0x10 + i * 2);
        if (key)
            k.a16(k.off32(0x24 + i * 4), n, "color values");
        else
            k.a8(k.off32(0x24 + i * 4), n, "color values");
    }
}

// TRK1 (brk): u8 attr, u8 pad, s16 frames, u16 creg/kreg materials, u16 counts
// CR CG CB CA KR KG KB KA, offsets: ctable ktable cids kids cnames knames, CR..CA, KR..KA
void blockTevReg(Blk &k)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 11);
    s.w32(b + 0x20, 14);
    for (u32 r = 0; r < 2; r++) {
        u32 mats = s.r16(b + 0xC + r * 2);
        u32 tab = k.off32(0x20 + r * 4);
        // J3DAnmCRegKeyTable: 4 key tables, u8 color id, 3 pad
        if (k.sec(tab, mats * 0x1C, "tev reg table"))
            for (u32 i = 0; i < mats; i++) {
                s.w16(b + tab + i * 0x1C, 12);
                s.raw(b + tab + i * 0x1C + 0x18, 4);
            }
        k.a16(k.off32(0x28 + r * 4), mats, "tev reg material ids");
        k.nameTab(k.off32(0x30 + r * 4));
        for (u32 c = 0; c < 4; c++)
            k.a16(k.off32(0x38 + r * 0x10 + c * 4), s.r16(b + 0x10 + r * 8 + c * 2), "tev reg values");
    }
}

// TPT1 (btp): u8 attr, u8 pad, s16 frames, u16 materials, u16 values, offsets: table,
// values u16, material ids, name table. Table entry: u16 count, u16 offset, u8 texmap,
// u8 pad, u16 pad (the texmap byte is read as ((u8 *)entry)[4]).
void blockTexPattern(Blk &k)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 3);
    s.w32(b + 0x10, 4);
    u32 mats = s.r16(b + 0xC);
    u32 tab = k.off32(0x10);
    if (k.sec(tab, mats * 8, "tex pattern table"))
        for (u32 i = 0; i < mats; i++) {
            s.w16(b + tab + i * 8, 2);
            s.raw(b + tab + i * 8 + 4, 4);
        }
    k.a16(k.off32(0x14), s.r16(b + 0xE), "tex pattern values");
    k.a16(k.off32(0x18), mats, "tex pattern material ids");
    k.nameTab(k.off32(0x1C));
}

// VAF1 (bva): u8 attr, u8 pad, s16 frames, u16 shapes, u16 values, offsets: table
// (u16 count, u16 offset), values u8
void blockVisibility(Blk &k)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 3);
    s.w32(b + 0x10, 2);
    k.a16(k.off32(0x10), s.r16(b + 0xC) * 2u, "visibility table");
    k.a8(k.off32(0x14), s.r16(b + 0xE), "visibility values");
}

// CLK1 (blk) / CLF1 (bla): u8 attr, u8, s16 frames, u16, u16 weight count, offsets:
// table, weights f32. The table count is not stored in the block (the deform data
// supplies it); the table runs up to the weights.
void blockCluster(Blk &k, bool key)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.raw(b + 8, 2);
    s.w16(b + 0xA, 3);
    s.w32(b + 0x10, 2);
    u32 tab = k.off32(0x10), vals = k.off32(0x14);
    k.tableTo(tab, vals > tab ? vals : k.end - b, key ? 6 : 4, key ? 3 : 2, "cluster table");
    k.a32(vals, s.r16(b + 0xE), "cluster weights");
}

// CLS1 (bls), J3DClusterBlock: u16 clusters, keys, vertices, positions, normals, pad;
// offsets: clusters, keys, vertices, positions f32, normals f32, cluster names, key names.
void blockClusterDeform(Blk &k)
{
    Swapper &s = k.s;
    u32 b = k.b;
    s.w16(b + 8, 6);
    s.w32(b + 0x14, 7);
    u32 nClusters = s.r16(b + 8), nKeys = s.r16(b + 0xA), nVerts = s.r16(b + 0xC);
    k.a32(k.off32(0x20), s.r16(b + 0xE) * 3u, "deform positions");
    k.a32(k.off32(0x24), s.r16(b + 0x10) * 3u, "deform normals");
    k.nameTab(k.off32(0x28));
    k.nameTab(k.off32(0x2C));
    // J3DCluster (0x24): f32 max, f32 min, u32 keys, u8 flags, 3 pad, u16 keyNum,
    // posNum, nrmNum, vtxNum, u32 pos index offset (read as u32), u32 vertices,
    // u32 deformer (runtime)
    u32 c = k.off32(0x14);
    if (k.sec(c, nClusters * 0x24, "clusters"))
        for (u32 i = 0; i < nClusters; i++) {
            u32 e = b + c + i * 0x24;
            s.w32(e, 3);
            s.raw(e + 0xC, 4);
            s.w16(e + 0x10, 4);
            s.w32(e + 0x18, 3);
            k.a16(s.r32(e + 0x18), s.r16(e + 0x12), "cluster pos indices");
        }
    // J3DClusterKey / J3DClusterVertex (0xC): u16 n0, u16 n1, u32 off0, u32 off1;
    // key: off0 -> n0 u16 (positions), off1 -> n1 u16 (normals);
    // vertex: both -> n0 u16
    for (int v = 0; v < 2; v++) {
        u32 t = k.off32(v ? 0x1C : 0x18), n = v ? nVerts : nKeys;
        if (!k.sec(t, n * 0xC, v ? "cluster vertices" : "cluster keys"))
            continue;
        for (u32 i = 0; i < n; i++) {
            u32 e = b + t + i * 0xC;
            s.w16(e, 2);
            s.w32(e + 4, 2);
            u32 n0 = s.r16(e), n1 = v ? n0 : s.r16(e + 2);
            k.a16(s.r32(e + 4), n0, "cluster index array");
            k.a16(s.r32(e + 8), n1, "cluster index array");
        }
    }
}

} // namespace

void convJ3DAnim(Swapper &s)
{
    if (!s.check(0, 0x20, "J3D header"))
        return;
    s.w32(0, 4);
    s.raw(0x10, 0x10);
    if (s.r32(0) != 0x4A334431) { // 'J3D1'
        s.fail("not a J3D1 file (0x%08x)", s.r32(0));
        return;
    }
    u32 n = s.r32(0xC), off = 0x20;
    for (u32 i = 0; i < n && s.ok(); i++) {
        if (!s.check(off, 8, "J3D block"))
            return;
        u32 tag = s.g32(off), size = s.g32(off + 4);
        // the loaders never step past the last block; CLS1 leaves its size unset
        if (i == n - 1 && (size < 8 || size > s.size() - off))
            size = s.size() - off;
        if (size < 8 || !s.check(off, size, "J3D block body"))
            return;
        Blk k{s, off, off + size};
        switch (tag) {
        case 0x414E4B31: blockTransform(k, true); break;   // ANK1
        case 0x414E4631: blockTransform(k, false); break;  // ANF1
        case 0x54544B31: blockTexSRT(k); break;            // TTK1
        case 0x50414B31: blockColor(k, true); break;       // PAK1
        case 0x50414631: blockColor(k, false); break;      // PAF1
        case 0x54524B31: blockTevReg(k); break;            // TRK1
        case 0x54505431: blockTexPattern(k); break;        // TPT1
        case 0x56414631: blockVisibility(k); break;        // VAF1
        case 0x434C4B31: blockCluster(k, true); break;     // CLK1
        case 0x434C4631: blockCluster(k, false); break;    // CLF1
        case 0x434C5331: blockClusterDeform(k); break;     // CLS1
        default:
            s.fail("unknown J3D anim block 0x%08x at 0x%x", tag, off);
            return;
        }
        if (!s.ok())
            return;
        fillGaps(s, off, off + size);
        off += size;
    }
    // after the last block: alignment fill holding leftover tool memory, never read
    if (s.ok() && off < s.size())
        s.raw(off, s.size() - off);
}

// JAUSoundAnimation: u16 numSounds, u16 pad, u32 control (runtime pointer), then
// JAUSoundAnimationSound[numSounds] (0x20): u32 sound id, f32 start, f32 end,
// f32 pitch, u32 flags, 12 bytes of u8/s8 parameters.
void convBAS(Swapper &s)
{
    if (!s.check(0, 8, "BAS header"))
        return;
    u32 n = s.g16(0);
    s.w16(2);
    s.w32(4);
    if (!s.check(8, n * 0x20, "BAS sounds"))
        return;
    for (u32 i = 0; i < n; i++) {
        u32 e = 8 + i * 0x20;
        s.w32(e, 5);
        s.raw(e + 0x14, 0xC);
    }
    u32 end = 8 + n * 0x20;
    if (end < s.size()) {
        if (!isPadding(s, end, s.size() - end))
            s.warn("BAS trailing data 0x%x..0x%x", end, s.size());
        s.raw(end, s.size() - end);
    }
}

} // namespace conv
