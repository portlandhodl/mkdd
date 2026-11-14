/* Decode every BTI (standalone, in RARC archives, and TEX1 sections of
 * BMD/BDL models) on an extracted disc with tools/web_wasm/gpu/texdec, print a
 * per-format summary and write sample PNGs plus per-format contact sheets.
 *
 *   c++ -std=c++17 -O2 -I tools/web_wasm/gpu tools/web_wasm/gpu/texdec.cpp \
 *       tools/web_wasm/gpu/tools/texdec_test.cpp -o texdec_test
 *   ./texdec_test build/web_wasm/disc/files OUTDIR [samplesPerFormat]
 */
#include "texdec.h"

#include <dirent.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

typedef std::vector<uint8_t> Bytes;

static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }

/* ---- PNG (zlib stored blocks) ---- */

static uint32_t crcTab[256];
static uint32_t crc(uint32_t c, const uint8_t *p, size_t n)
{
    if (!crcTab[1])
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t v = i;
            for (int k = 0; k < 8; k++) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            crcTab[i] = v;
        }
    c = ~c;
    while (n--) c = crcTab[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return ~c;
}

static void put32(Bytes &b, uint32_t v)
{
    b.push_back(v >> 24); b.push_back(v >> 16); b.push_back(v >> 8); b.push_back(v);
}

static void chunk(Bytes &f, const char *type, const Bytes &d)
{
    put32(f, (uint32_t)d.size());
    size_t s = f.size();
    f.insert(f.end(), type, type + 4);
    f.insert(f.end(), d.begin(), d.end());
    put32(f, crc(0, f.data() + s, f.size() - s));
}

static bool writePng(const std::string &path, const uint8_t *rgba, int w, int h)
{
    Bytes raw;
    for (int y = 0; y < h; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba + (size_t)y * w * 4, rgba + (size_t)(y + 1) * w * 4);
    }
    Bytes z = { 0x78, 0x01 };
    for (size_t i = 0; i < raw.size() || i == 0; i += 65535) {
        size_t n = raw.size() - i < 65535 ? raw.size() - i : 65535;
        z.push_back(i + n >= raw.size());
        z.push_back(n); z.push_back(n >> 8); z.push_back(~n); z.push_back((~n) >> 8);
        z.insert(z.end(), raw.begin() + i, raw.begin() + i + n);
        if (n == 0) break;
    }
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    put32(z, b << 16 | a);
    Bytes f = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    Bytes ih;
    put32(ih, w); put32(ih, h);
    ih.push_back(8); ih.push_back(6); ih.push_back(0); ih.push_back(0); ih.push_back(0);
    chunk(f, "IHDR", ih);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", Bytes());
    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) return false;
    fwrite(f.data(), 1, f.size(), fp);
    fclose(fp);
    return true;
}

/* ---- Yaz0 / RARC ---- */

static bool yaz0(const Bytes &s, Bytes &d)
{
    uint32_t size = rd32(&s[4]);
    d.clear();
    d.reserve(size);
    size_t i = 16;
    while (d.size() < size && i < s.size()) {
        uint8_t fl = s[i++];
        for (int b = 0; b < 8 && d.size() < size; b++) {
            if (fl & (0x80 >> b)) {
                if (i >= s.size()) return false;
                d.push_back(s[i++]);
            } else {
                if (i + 1 >= s.size()) return false;
                int b1 = s[i], b2 = s[i + 1];
                i += 2;
                size_t dist = ((b1 & 0xF) << 8 | b2) + 1;
                int n = b1 >> 4;
                if (n == 0) n = s[i++] + 0x12; else n += 2;
                if (dist > d.size()) return false;
                for (int k = 0; k < n; k++) d.push_back(d[d.size() - dist]);
            }
        }
    }
    return d.size() == size;
}

struct Stats {
    int textures = 0, levels = 0, failed = 0, unique = 0;
    std::set<int> tlutFmts;
    std::map<std::string, int> sizes;
};

static std::map<int, Stats> gStats;
static std::map<int, std::vector<std::pair<std::string, Bytes>>> gSheet; /* thumbnails as encoded tiles */
static std::set<uint64_t> gSeen;
static std::string gOut;
static int gSamples = 6;
static int gBadHeaders = 0;
static std::map<int, int> gWritten;

struct Thumb { int w, h; Bytes rgba; std::string name; };
static std::map<int, std::vector<Thumb>> gThumbs;

static const char *fmtName(int f)
{
    switch (f) {
    case 0: return "I4"; case 1: return "I8"; case 2: return "IA4"; case 3: return "IA8";
    case 4: return "RGB565"; case 5: return "RGB5A3"; case 6: return "RGBA8";
    case 8: return "C4"; case 9: return "C8"; case 10: return "C14X2"; case 14: return "CMPR";
    }
    static char buf[16];
    snprintf(buf, sizeof buf, "fmt%02x", f);
    return buf;
}

static std::string sanitize(std::string s)
{
    for (char &c : s)
        if (c == '/' || c == ' ' || (unsigned char)c >= 0x80) c = '_';
    return s;
}

/* hdr: 0x20-byte ResTIMG; imageBase/palBase are what its offsets are relative to. */
static void decodeTimg(const std::string &name, const uint8_t *hdr, const uint8_t *base, size_t baseLen)
{
    int fmt = hdr[0];
    int w = rd16(hdr + 2), h = rd16(hdr + 4);
    int palEnabled = hdr[8], tlutFmt = hdr[9], tlutCount = rd16(hdr + 10);
    uint32_t palOff = rd32(hdr + 0xC);
    int mip = hdr[0x10];
    int count = hdr[0x18];
    uint32_t imgOff = rd32(hdr + 0x1C);
    size_t hdrPos = hdr - base;
    int levels = mip ? (count ? count : 1) : 1;

    if (texdec_layout(fmt) < 0 || w == 0 || h == 0 || w > 1024 || h > 1024) {
        fprintf(stderr, "bad header: %s fmt=%02x %dx%d\n", name.c_str(), fmt, w, h);
        gBadHeaders++;
        return;
    }
    Stats &st = gStats[fmt];
    st.textures++;
    char dim[32];
    snprintf(dim, sizeof dim, "%dx%d", w, h);
    st.sizes[dim]++;

    const uint8_t *tlut = nullptr;
    bool isCI = fmt == 8 || fmt == 9 || fmt == 10;
    if (isCI) {
        st.tlutFmts.insert(tlutFmt);
        if (!palEnabled || hdrPos + palOff + tlutCount * 2 > baseLen)
            fprintf(stderr, "palette missing: %s\n", name.c_str());
        else
            tlut = base + hdrPos + palOff;
    }

    uint32_t total = texdec_chain_size(fmt, w, h, levels);
    if (hdrPos + imgOff + total > baseLen) {
        /* Some files truncate the chain; decode only levels that fit. */
        while (levels > 0 && hdrPos + imgOff + texdec_chain_size(fmt, w, h, levels) > baseLen) levels--;
        if (levels == 0) {
            fprintf(stderr, "truncated: %s\n", name.c_str());
            st.failed++;
            return;
        }
    }
    const uint8_t *img = base + hdrPos + imgOff;

    uint64_t hash = 1469598103934665603ull;
    for (uint32_t i = 0; i < texdec_size(fmt, w, h); i++) hash = (hash ^ img[i]) * 1099511628211ull;
    hash ^= (uint64_t)fmt << 56 ^ (uint64_t)w << 40 ^ (uint64_t)h << 24;
    bool fresh = gSeen.insert(hash).second;
    if (fresh) st.unique++;

    for (int l = 0; l < levels; l++) {
        int lw = texdec_mip_dim(w, l), lh = texdec_mip_dim(h, l);
        Bytes out((size_t)lw * lh * 4);
        if (texdec_decode(out.data(), img + texdec_mip_offset(fmt, w, h, l), fmt, lw, lh, tlut, tlutFmt, tlutCount) != 0) {
            st.failed++;
            return;
        }
        st.levels++;
        if (!fresh) continue;
        if (l == 0) {
            /* Reservoir-sample 64 thumbnails per format across the whole disc. */
            std::vector<Thumb> &tv = gThumbs[fmt];
            static uint32_t rng = 12345;
            rng = rng * 1103515245u + 12345u;
            if (tv.size() < 64) tv.push_back({ lw, lh, out, name });
            else if ((rng >> 8) % st.unique < 64) tv[(rng >> 4) % 64] = { lw, lh, out, name };
        }
        if (gWritten[fmt] < gSamples || (l > 0 && gWritten[fmt] <= gSamples && levels > 1)) {
            char suffix[64];
            snprintf(suffix, sizeof suffix, "_%dx%d_L%d.png", lw, lh, l);
            writePng(gOut + "/" + fmtName(fmt) + "__" + sanitize(name) + suffix, out.data(), lw, lh);
        }
    }
    if (fresh) gWritten[fmt]++;
}

static void decodeBti(const std::string &name, const Bytes &d)
{
    if (d.size() < 0x20) { gBadHeaders++; return; }
    decodeTimg(name, d.data(), d.data(), d.size());
}

static void decodeJ3d(const std::string &name, const Bytes &d)
{
    if (d.size() < 0x20) return;
    uint32_t nsec = rd32(&d[0xC]);
    size_t p = 0x20;
    for (uint32_t s = 0; s < nsec && p + 8 <= d.size(); s++) {
        uint32_t size = rd32(&d[p + 4]);
        if (!memcmp(&d[p], "TEX1", 4)) {
            int n = rd16(&d[p + 8]);
            uint32_t hdrOff = rd32(&d[p + 0xC]);
            for (int i = 0; i < n; i++) {
                size_t hp = p + hdrOff + i * 0x20;
                if (hp + 0x20 > d.size()) break;
                char sub[32];
                snprintf(sub, sizeof sub, "#tex%d", i);
                decodeTimg(name + sub, &d[hp], d.data(), d.size());
            }
        }
        if (size == 0) break;
        p += size;
    }
}

static void walk(const std::string &path, Bytes data);

static void walkRarc(const std::string &path, const Bytes &d)
{
    uint32_t hl = rd32(&d[8]), fdo = rd32(&d[12]);
    const uint8_t *info = &d[hl];
    uint32_t nf = rd32(info + 8), fo = rd32(info + 12), so = rd32(info + 20);
    for (uint32_t i = 0; i < nf; i++) {
        const uint8_t *e = info + fo + i * 20;
        uint32_t flag = rd32(e + 4), doff = rd32(e + 8), size = rd32(e + 12);
        std::string n = (const char *)(info + so + (flag & 0xFFFFFF));
        if ((flag >> 24) & 2 || n == "." || n == "..") continue;
        size_t start = (size_t)hl + fdo + doff;
        if (start + size > d.size()) continue;
        walk(path + "/" + n, Bytes(d.begin() + start, d.begin() + start + size));
    }
}

static int gBtiFiles, gModels;

static void walk(const std::string &path, Bytes data)
{
    if (data.size() >= 16 && !memcmp(data.data(), "Yaz0", 4)) {
        Bytes dec;
        if (!yaz0(data, dec)) { fprintf(stderr, "yaz0 fail: %s\n", path.c_str()); return; }
        data.swap(dec);
    }
    if (data.size() >= 0x20 && !memcmp(data.data(), "RARC", 4)) {
        walkRarc(path, data);
        return;
    }
    std::string ext = path.size() > 4 ? path.substr(path.size() - 4) : "";
    for (char &c : ext) c = (char)tolower(c);
    if (ext == ".bti") {
        gBtiFiles++;
        decodeBti(path, data);
    } else if (data.size() >= 8 && (!memcmp(data.data(), "J3D2bmd", 7) || !memcmp(data.data(), "J3D2bdl", 7))) {
        gModels++;
        decodeJ3d(path, data);
    }
}

static void scanDir(const std::string &root, const std::string &rel)
{
    DIR *dir = opendir((root + "/" + rel).c_str());
    if (!dir) return;
    std::vector<std::string> names;
    while (dirent *e = readdir(dir))
        if (e->d_name[0] != '.') names.push_back(e->d_name);
    closedir(dir);
    for (auto &n : names) {
        std::string r = rel.empty() ? n : rel + "/" + n;
        std::string full = root + "/" + r;
        struct stat st;
        if (stat(full.c_str(), &st)) continue;
        if (S_ISDIR(st.st_mode)) { scanDir(root, r); continue; }
        FILE *fp = fopen(full.c_str(), "rb");
        if (!fp) continue;
        Bytes d(st.st_size);
        size_t got = fread(d.data(), 1, d.size(), fp);
        fclose(fp);
        d.resize(got);
        walk(r, d);
    }
}

/* Contact sheet: thumbnails (nearest-scaled to 128x128 cells) over a checkerboard. */
static void writeSheet(int fmt, const std::vector<Thumb> &th)
{
    const int cell = 132, cols = 8;
    int rows = ((int)th.size() + cols - 1) / cols;
    int W = cols * cell, H = rows * cell;
    Bytes img((size_t)W * H * 4);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            uint8_t c = ((x >> 3) ^ (y >> 3)) & 1 ? 0xC0 : 0x80;
            uint8_t *o = &img[((size_t)y * W + x) * 4];
            o[0] = c; o[1] = c; o[2] = c == 0xC0 ? 0xC0 : 0xA0; o[3] = 255;
        }
    for (size_t i = 0; i < th.size(); i++) {
        const Thumb &t = th[i];
        int ox = (int)(i % cols) * cell + 2, oy = (int)(i / cols) * cell + 2;
        int m = t.w > t.h ? t.w : t.h;
        double sc = 128.0 / m;
        int dw = (int)(t.w * sc), dh = (int)(t.h * sc);
        for (int y = 0; y < dh; y++)
            for (int x = 0; x < dw; x++) {
                const uint8_t *s = &t.rgba[((size_t)(y * t.h / dh) * t.w + (x * t.w / dw)) * 4];
                uint8_t *o = &img[((size_t)(oy + y) * W + ox + x) * 4];
                for (int k = 0; k < 3; k++) o[k] = (uint8_t)((s[k] * s[3] + o[k] * (255 - s[3])) / 255);
            }
    }
    writePng(gOut + "/sheet_" + fmtName(fmt) + ".png", img.data(), W, H);
    FILE *fp = fopen((gOut + "/sheet_" + fmtName(fmt) + ".txt").c_str(), "w");
    for (size_t i = 0; i < th.size(); i++) fprintf(fp, "%zu %dx%d %s\n", i, th[i].w, th[i].h, th[i].name.c_str());
    fclose(fp);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s DISC_FILES_DIR OUTDIR [samplesPerFormat]\n", argv[0]);
        return 2;
    }
    gOut = argv[2];
    if (argc > 3) gSamples = atoi(argv[3]);
    mkdir(gOut.c_str(), 0755);
    scanDir(argv[1], "");

    printf("BTI files: %d, J3D models: %d, bad headers: %d\n", gBtiFiles, gModels, gBadHeaders);
    printf("%-8s %8s %8s %8s %6s  tlut fmts\n", "format", "textures", "unique", "levels", "failed");
    int tot = 0, totU = 0, totL = 0, totF = 0;
    for (auto &kv : gStats) {
        const Stats &s = kv.second;
        std::string tl;
        for (int t : s.tlutFmts) tl += t == 0 ? "IA8 " : t == 1 ? "RGB565 " : t == 2 ? "RGB5A3 " : "? ";
        printf("%-8s %8d %8d %8d %6d  %s\n", fmtName(kv.first), s.textures, s.unique, s.levels, s.failed, tl.c_str());
        tot += s.textures; totU += s.unique; totL += s.levels; totF += s.failed;
        writeSheet(kv.first, gThumbs[kv.first]);
    }
    printf("%-8s %8d %8d %8d %6d\n", "total", tot, totU, totL, totF);
    return totF || gBadHeaders ? 1 : 0;
}
