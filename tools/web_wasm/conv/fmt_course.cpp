// Course data: BOL (CrsData::SOblHeader and its section tables) and BCO
// (CrsData::SColHeader, grids, index/ground/vertex/material tables).
// Layouts follow include/Kaneshige/Course/CrsData.h.
#include "conv.h"

namespace conv {

// Walk one record with a layout string: 'w' = 32-bit field, 'h' = 16-bit field,
// 'b' = byte left as-is; spaces are ignored. Returns the record size.
static u32 rec(Swapper &s, u32 off, const char *layout)
{
    u32 p = off;
    for (const char *c = layout; *c; c++) {
        switch (*c) {
        case 'w': s.w32(p); p += 4; break;
        case 'h': s.w16(p); p += 2; break;
        case 'b': s.raw(p, 1); p += 1; break;
        default: break;
        }
    }
    return p - off;
}

static void table(Swapper &s, u32 off, u32 count, u32 size, const char *layout, const char *what)
{
    if (!count || !s.check(off, count * size, what))
        return;
    for (u32 i = 0; i < count && s.ok(); i++)
        rec(s, off + i * size, layout);
}

// ---- BOL ----

static const char kOblHeader[] =
    "w bbbb bbbb www bb hhhhhhh bbbb ww bbbb bbbb bbbb wwwwwwwwwwww bbbbbbbbbbbb";
static const char kCLPoint[] = "www hh w bbbbbbbbbbbb";           // SCLPoint, 0x20
static const char kCheckPath[] = "hh hhhh hhhh";                  // CheckPath, 0x14
static const char kCheckPoint[] = "www www bbbb";                 // SCheckPoint, 0x1c
static const char kPath[] = "hhhh bbbbbbbb";                      // PathData, 0x10
static const char kPoint[] = "www w bbbbbbbbbbbbbbbb";            // PointData, 0x20
static const char kObject[] = "www www hhh hhh hhhh bbbb hhhhhhhh"; // SObject, 0x40
static const char kStart[] = "www www hhh bbbbbb bb h";           // StartPoint, 0x28
static const char kArea[] = "www www hhh hhh bb h w w hhhh";      // SArea, 0x38
static const char kCamera[] =
    "www hhh bbbbbb www www hhhhhhhhhh w";                        // Camera, 0x48
static const char kJugem[] = "www hhh hhh hhhh";                  // SJugemPoint, 0x20
static const char kLight[] = "bbbb www bbbb";                     // LightParam, 0x14
static const char kMG[] = "hhhh";                                 // MGParam, 0x8

void convBOL(Swapper &s)
{
    if (!s.check(0, 0x7C, "BOL header"))
        return;
    if (s.r32(0) != 0x30303135) { // '0015', compared as a u32 by CrsData
        s.fail("BOL bad version 0x%08x", s.r32(0));
        return;
    }
    rec(s, 0, kOblHeader);

    u32 clNum = s.r16(0x1A), cpathNum = s.r16(0x1C), objNum = s.r16(0x1E);
    u32 areaNum = s.r16(0x20), camNum = s.r16(0x22), pathNum = s.r16(0x24);
    u32 jugemNum = s.r16(0x26);
    u32 startNum = s.r8(0x3B), lightNum = s.r8(0x3D), mgNum = s.r8(0x3E);
    u32 clOff = s.r32(0x44), cpathOff = s.r32(0x48), pathOff = s.r32(0x4C);
    u32 pointOff = s.r32(0x50), objOff = s.r32(0x54), startOff = s.r32(0x58);
    u32 areaOff = s.r32(0x5C), camOff = s.r32(0x60), jugemOff = s.r32(0x64);
    u32 lightOff = s.r32(0x68), mgOff = s.r32(0x6C);

    table(s, clOff, clNum, 0x20, kCLPoint, "BOL CL points");

    // check points follow the check path table
    table(s, cpathOff, cpathNum, 0x14, kCheckPath, "BOL check paths");
    u32 cpNum = 0;
    for (u32 i = 0; i < cpathNum && s.ok(); i++)
        cpNum += s.r16(cpathOff + i * 0x14);
    table(s, cpathOff + cpathNum * 0x14, cpNum, 0x1C, kCheckPoint, "BOL check points");

    table(s, pathOff, pathNum, 0x10, kPath, "BOL paths");
    u32 pointNum = 0;
    for (u32 i = 0; i < pathNum && s.ok(); i++) {
        u32 end = (u32)s.r16(pathOff + i * 0x10 + 2) + s.r16(pathOff + i * 0x10);
        if (end > pointNum)
            pointNum = end;
    }
    table(s, pointOff, pointNum, 0x20, kPoint, "BOL points");

    table(s, objOff, objNum, 0x40, kObject, "BOL objects");
    // getStartPoint reads one entry even when the count is 0
    table(s, startOff, startNum ? startNum : (s.inRange(startOff, 0x28) ? 1 : 0), 0x28, kStart,
          "BOL start points");
    table(s, areaOff, areaNum, 0x38, kArea, "BOL areas");
    table(s, camOff, camNum, 0x48, kCamera, "BOL cameras");
    table(s, jugemOff, jugemNum, 0x20, kJugem, "BOL jugem points");
    table(s, lightOff, lightNum, 0x14, kLight, "BOL light params");
    if (mgOff)
        table(s, mgOff, mgNum, 0x8, kMG, "BOL MG params");
}

// ---- BCO ----

// Bitfields: MWCC allocates them from the most significant bit, the host
// compiler (little-endian clang) from the least significant bit. These are
// involutions, so the same transform serves both directions.
static u8 revCrumbs(u8 v) // four 2-bit fields
{
    return (u8)((v >> 6 & 3) | (v >> 2 & 0xC) | (v << 2 & 0x30) | (v << 6 & 0xC0));
}
static u8 revNibbles(u8 v) // two 4-bit fields
{
    return (u8)(v >> 4 | v << 4);
}

static const char kColHeader[] = "w hh ww ww h bb wwww";       // SColHeader, 0x2c
static const char kGrid[] = "bb h w";                          // Grid, 0x8
static const char kGround[] = "www w hhh bb b b hhh b b b b";  // SGround, 0x24
static const char kMaterial[] = "bb h w w";                    // SMaterial, 0xc

// tail of a section that does not fill whole records: must be zero padding
static void pad(Swapper &s, u32 off, u32 end, const char *what)
{
    for (u32 i = off; i < end; i++)
        if (s.r8(i)) {
            s.warn("%s: nonzero trailing bytes at 0x%x", what, i);
            break;
        }
    if (end > off)
        s.raw(off, end - off);
}

void convBCO(Swapper &s)
{
    if (!s.check(0, 0x2C, "BCO header"))
        return;
    if (s.r32(0) != 0x30303033) { // '0003', compared as a u32 by CrsData
        s.fail("BCO bad version 0x%08x", s.r32(0));
        return;
    }
    rec(s, 0, kColHeader);
    u32 matNum = s.r16(0x18);
    u32 idxOff = s.r32(0x1C), gndOff = s.r32(0x20), vtxOff = s.r32(0x24), matOff = s.r32(0x28);
    if (!(0x2C <= idxOff && idxOff <= gndOff && gndOff <= vtxOff && vtxOff <= matOff &&
          matOff <= s.size())) {
        s.fail("BCO unexpected section order");
        return;
    }
    // grid table: root cells plus child cells (Grid::_2 indexes children)
    u32 gridNum = (idxOff - 0x2C) / 8;
    if (gridNum < (u32)s.r16(4) * s.r16(6)) {
        s.fail("BCO grid table too small");
        return;
    }
    table(s, 0x2C, gridNum, 8, kGrid, "BCO grids");
    pad(s, 0x2C + gridNum * 8, idxOff, "BCO grids");

    u32 idxNum = (gndOff - idxOff) / 2;
    s.w16(idxOff, idxNum);
    pad(s, idxOff + idxNum * 2, gndOff, "BCO index table");

    u32 gndNum = (vtxOff - gndOff) / 0x24;
    u8 *d = s.data();
    for (u32 i = 0; i < gndNum && s.ok(); i++) {
        u32 g = gndOff + i * 0x24;
        rec(s, g, kGround);
        d[g + 0x18] = revCrumbs(d[g + 0x18]);
        d[g + 0x20] = revNibbles(d[g + 0x20]);
        d[g + 0x22] = revNibbles(d[g + 0x22]);
    }
    pad(s, gndOff + gndNum * 0x24, vtxOff, "BCO ground table");

    u32 vtxNum = (matOff - vtxOff) / 4;
    s.w32(vtxOff, vtxNum);
    pad(s, vtxOff + vtxNum * 4, matOff, "BCO vertex table");

    table(s, matOff, matNum, 0xC, kMaterial, "BCO materials");
    if (s.ok())
        pad(s, matOff + matNum * 0xC, s.size(), "BCO tail");
}

} // namespace conv
