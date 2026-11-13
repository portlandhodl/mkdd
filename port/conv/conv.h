// Disc data converter for the host port: rewrites GameCube big-endian file
// formats into host byte order, so decomp code can read them through its structs.
//
// Rule of thumb: everything the CPU reads through a struct or typed pointer is
// swapped to host order (headers, tables, offsets, vertex arrays, animation keys).
// Data only the GPU consumes stays big-endian, exactly as on hardware: display
// lists, texture pixel data and palettes. The GX backend decodes those as BE.
//
// Self-contained: no decomp headers, no pointers stored in data, so it builds both
// into the wasm (port/sdk/dvd.c calls it) and as a native CLI (port/conv/cli.cpp).
#ifndef PORT_CONV_H
#define PORT_CONV_H

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace conv {

// The game replaces global operator new with its own heaps (JKRHeap), which
// must not be touched by host-side code: containers here allocate with malloc.
template <class T> struct Alloc {
    typedef T value_type;
    Alloc() {}
    template <class U> Alloc(const Alloc<U> &) {}
    T *allocate(size_t n) { return (T *)malloc(n * sizeof(T)); }
    void deallocate(T *p, size_t) { free(p); }
    template <class U> bool operator==(const Alloc<U> &) const { return true; }
    template <class U> bool operator!=(const Alloc<U> &) const { return false; }
};
typedef std::basic_string<char, std::char_traits<char>, Alloc<char> > Str;
template <class T> using Vec = std::vector<T, Alloc<T> >;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;

struct Report {
    Vec<Str> errors;   // conversion failed or is suspect
    Vec<Str> warnings; // e.g. unclaimed bytes
    u32 files = 0;                     // files converted (incl. archive members)
    void error(const Str &where, const char *fmt, ...);
    void warn(const Str &where, const char *fmt, ...);
};

// In-place byte swapper over one file, with a claim map so every byte is
// accounted for exactly once:
//   - swapping the same range twice with the same width is a no-op (shared data)
//   - claiming a range with a conflicting width is an error
//   - reads are claim-aware: they return the logical value whether or not the
//     bytes were already swapped
// "Source order" is big-endian when converting disc data to host order, and
// little-endian for the reverse direction (used by the round-trip test).
class Swapper {
public:
    enum Claim : u8 { NONE = 0, RAW = 1, W16 = 2, W32 = 4 };

    Swapper(u8 *data, u32 size, bool srcBig, const Str &name, Report &rep);

    u32 size() const { return mSize; }
    u8 *data() const { return mData; }
    const Str &name() const { return mName; }
    bool ok() const { return mOk; }
    bool srcBig() const { return mSrcBig; }

    // claim-aware reads of the logical value (0 and an error when out of range)
    u8 r8(u32 off);
    u16 r16(u32 off);
    u32 r32(u32 off);
    s16 rs16(u32 off) { return (s16)r16(off); }
    s32 rs32(u32 off) { return (s32)r32(off); }
    f32 rf32(u32 off);

    // swap `count` consecutive elements starting at off
    void w16(u32 off, u32 count = 1);
    void w32(u32 off, u32 count = 1);
    // claim bytes that stay as-is (u8 data, strings, BE-only GPU data)
    void raw(u32 off, u32 len);
    // read then swap (returns the logical value)
    u16 g16(u32 off) { u16 v = r16(off); w16(off); return v; }
    u32 g32(u32 off) { u32 v = r32(off); w32(off); return v; }

    bool inRange(u32 off, u32 len) const { return off <= mSize && len <= mSize - off; }
    bool check(u32 off, u32 len, const char *what);
    void fail(const char *fmt, ...);
    void warn(const char *fmt, ...);

    // bytes no format walker claimed; reported as a warning by the dispatcher
    u32 unclaimed() const;
    u32 firstUnclaimed() const;
    Claim claimAt(u32 off) const { return off < mSize ? (Claim)mClaim[off] : NONE; }

private:
    bool claim(u32 off, u32 len, Claim c);
    u8 *mData;
    u32 mSize;
    bool mSrcBig;
    bool mOk;
    Str mName;
    Report &mRep;
    Vec<u8> mClaim;
};

// A format converter walks one file and swaps/claims its fields.
typedef void (*Converter)(Swapper &s);

// Converter for a file name (by extension) and contents (by magic), or null.
Converter find(const Str &name, const u8 *data, u32 size);

// Should the port route this file through the converter? (false for streamed
// media that is read piecewise and holds only BE sample data)
bool wants(const Str &name);

// Convert one file. Yaz0 input is expanded first; RARC archives convert each
// member and are rebuilt if members were compressed. `data` is replaced by the
// converted bytes. Returns false if the file type is unknown (data untouched
// apart from Yaz0 expansion).
bool convertFile(const Str &name, Vec<u8> &data, bool srcBig, Report &rep);

// Yaz0
bool isYaz0(const u8 *data, u32 size);
Vec<u8> yaz0Decode(const u8 *data, u32 size);

// format walkers (one per family; see fmt_*.cpp). Each file is owned by one family.
void convRARC(Swapper &s); // used directly only for the directory of an archive
void convBFN(Swapper &s);  // fmt_font.cpp
// fmt_j3d_model.cpp: bmd, bdl
void convJ3DModel(Swapper &s);
// fmt_j3d_anim.cpp: bca bck btk brk btp bpk bla blk bls bva, and bas (anime sound)
void convJ3DAnim(Swapper &s);
void convBAS(Swapper &s);
// fmt_j2d.cpp: blo (SCRN screens), bti (texture header), bmg (messages)
void convBLO(Swapper &s);
void convBTI(Swapper &s);
void convBMG(Swapper &s);
// fmt_course.cpp: bol (course data), bco (collision)
void convBOL(Swapper &s);
void convBCO(Swapper &s);
// fmt_game.cpp: small game parameter files; dispatches on s.name() extension
// (set ikp lbd kel pel iop dat snp ght)
void convGameParam(Swapper &s);
// fmt_jpa.cpp: jpc / jpa particle resources
void convJPA(Swapper &s);
// fmt_audio.cpp: baa (audio archive), bsft, bstn, bms (sequences)
void convAudio(Swapper &s);

// Vertex attribute arrays (J3D VTX1 etc.) are converted to host order per
// component width: the CPU reads them (skinning, collision) and so does the GX
// backend, which therefore reads arrays in host order. Display lists and texture/
// palette data stay big-endian.

} // namespace conv

#endif
