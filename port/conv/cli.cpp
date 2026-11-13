// mkdd-conv: run the converter over an extracted disc on the host.
//
//   port/conv/build_cli.sh && build/port/mkdd-conv build/port/disc/files [options]
//     --only EXT     only files (and archive members) with this extension
//     --roundtrip    convert to host order and back; the result must equal the input
//     --verbose      print every error and warning
//     --dump DIR     write converted top-level files to DIR
#include "conv.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <map>
#include <sys/stat.h>

using namespace conv;

static Str gOnly;
static bool gRoundtrip, gVerbose;
static Str gDump;

struct Stat {
    u32 files = 0, errors = 0, warnings = 0, rtFail = 0;
};
static std::map<Str, Stat> gStats;
static u32 gTotalErrors, gTotalWarnings, gTotalRt;

static Str extOf(const Str &n)
{
    size_t d = n.rfind('.');
    Str e = d == Str::npos ? "(none)" : n.substr(d + 1);
    for (auto &c : e)
        c = (char)tolower((unsigned char)c);
    return e;
}

static Str keyOf(const Str &msg)
{
    // "path/to/file.ext: message" -> ext of the innermost file
    size_t colon = msg.find(": ");
    return extOf(msg.substr(0, colon));
}

static bool readFile(const Str &p, Vec<u8> &out)
{
    FILE *f = fopen(p.c_str(), "rb");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize((size_t)n);
    size_t got = fread(out.data(), 1, out.size(), f);
    fclose(f);
    return got == out.size();
}

static void process(const Str &path, const Str &rel)
{
    if (!wants(rel))
        return;
    Vec<u8> data;
    if (!readFile(path, data))
        return;
    if (isYaz0(data.data(), (u32)data.size()))
        data = yaz0Decode(data.data(), (u32)data.size());
    Vec<u8> orig = data;
    Report rep;
    convertFile(rel, data, true, rep);
    bool rebuilt = data.size() != orig.size();
    if (gRoundtrip && !rebuilt) {
        Report back;
        Vec<u8> again = data;
        convertFile(rel, again, false, back);
        if (again != orig) {
            u32 i = 0;
            while (i < again.size() && again[i] == orig[i])
                i++;
            rep.error(rel, "round trip differs at 0x%x", i);
            gTotalRt++;
        }
    }
    for (auto &e : rep.errors) {
        Str k = keyOf(e);
        if (!gOnly.empty() && k != gOnly)
            continue;
        gStats[k].errors++;
        gTotalErrors++;
        if (gVerbose || gStats[k].errors <= 3)
            printf("ERROR %s\n", e.c_str());
    }
    for (auto &w : rep.warnings) {
        Str k = keyOf(w);
        if (!gOnly.empty() && k != gOnly)
            continue;
        gStats[k].warnings++;
        gTotalWarnings++;
        if (gVerbose || gStats[k].warnings <= 3)
            printf("WARN  %s\n", w.c_str());
    }
    if (!gDump.empty()) {
        Str out = gDump + "/" + rel;
        for (size_t i = gDump.size() + 1; i < out.size(); i++)
            if (out[i] == '/') {
                out[i] = 0;
                mkdir(out.c_str(), 0755);
                out[i] = '/';
            }
        FILE *f = fopen(out.c_str(), "wb");
        if (f) {
            fwrite(data.data(), 1, data.size(), f);
            fclose(f);
        }
    }
}

static void walk(const Str &dir, const Str &rel)
{
    DIR *d = opendir(dir.c_str());
    if (!d)
        return;
    while (struct dirent *e = readdir(d)) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        Str p = dir + "/" + e->d_name, r = rel.empty() ? e->d_name : rel + "/" + e->d_name;
        struct stat st;
        if (stat(p.c_str(), &st))
            continue;
        if (S_ISDIR(st.st_mode))
            walk(p, r);
        else
            process(p, r);
    }
    closedir(d);
}

int main(int argc, char **argv)
{
    Str root;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--only") && i + 1 < argc)
            gOnly = argv[++i];
        else if (!strcmp(argv[i], "--roundtrip"))
            gRoundtrip = true;
        else if (!strcmp(argv[i], "--verbose"))
            gVerbose = true;
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc)
            gDump = argv[++i];
        else
            root = argv[i];
    }
    if (root.empty()) {
        fprintf(stderr, "usage: mkdd-conv DISC_FILES_DIR [--only EXT] [--roundtrip] [--verbose] [--dump DIR]\n");
        return 2;
    }
    walk(root, "");
    printf("\n%-8s %8s %8s\n", "ext", "errors", "warns");
    for (auto &kv : gStats)
        printf("%-8s %8u %8u\n", kv.first.c_str(), kv.second.errors, kv.second.warnings);
    printf("total: %u errors, %u warnings, %u round-trip failures\n", gTotalErrors, gTotalWarnings, gTotalRt);
    return gTotalErrors ? 1 : 0;
}
