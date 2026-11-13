/* DVD: the disc filesystem served from an extracted directory on the host.
 *
 * Entry numbers index a table built by walking the directory once, in sorted
 * order like the disc FST. Lookups are case-insensitive as on hardware.
 * Reads complete immediately; async callbacks are delivered from port_poll(). */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <emscripten.h>
#include "port_sdk.h"

typedef struct Entry {
    char *name;     /* component name */
    char *hostPath; /* full host path */
    int parent;
    int isDir;
    int next;       /* index after this entry's subtree (dirs) */
    u32 size;       /* as served: the converted size for converted files */
    u8 *conv;       /* host-order copy (port/conv), loaded on first open */
    int convOpen;   /* open handles using conv */
    int convState;  /* 0 unknown, 1 converted, 2 served raw */
} Entry;

int port_conv_wants(const char *name);
unsigned char *port_conv_file(const char *name, const unsigned char *in, unsigned inSize, unsigned *outSize);

static Entry *sEntries;
static int sNumEntries, sCapEntries;
static char sRoot[512];
static DVDDiskID sDiskID;
static int sCurrentDir;

EM_JS(const char *, port_js_disc_root, (void), {
    var p = (typeof process !== 'undefined' && process.env && process.env.MKDD_DISC) || '/disc';
    return stringToNewUTF8(p);
});

EM_JS(int, port_js_env, (const char *name), {
    var v = (typeof process !== 'undefined' && process.env) ? process.env[UTF8ToString(name)] : undefined;
    return v ? (parseInt(v, 10) || 1) : 0;
});

int port_env(const char *name) { return port_js_env(name); }

const char *port_disc_root(void)
{
    if (!sRoot[0]) {
        const char *r = port_js_disc_root();
        snprintf(sRoot, sizeof sRoot, "%s", r);
        free((void *)r);
    }
    return sRoot;
}

static int add_entry(const char *name, const char *hostPath, int parent, int isDir, u32 size)
{
    Entry *e;
    if (sNumEntries == sCapEntries) {
        sCapEntries = sCapEntries ? sCapEntries * 2 : 256;
        sEntries = (Entry *)realloc(sEntries, sCapEntries * sizeof(Entry));
    }
    e = &sEntries[sNumEntries];
    e->name = strdup(name);
    e->hostPath = strdup(hostPath);
    e->parent = parent;
    e->isDir = isDir;
    e->next = 0;
    e->size = size;
    e->conv = NULL;
    e->convOpen = 0;
    e->convState = 0;
    return sNumEntries++;
}

static int cmp_names(const void *a, const void *b) { return strcasecmp(*(char *const *)a, *(char *const *)b); }

static void walk(const char *hostDir, int self)
{
    DIR *d = opendir(hostDir);
    struct dirent *de;
    char **names = NULL;
    int n = 0, cap = 0, i;
    if (!d) {
        fprintf(stderr, "[dvd] cannot open %s\n", hostDir);
        sEntries[self].next = sNumEntries;
        return;
    }
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (n == cap)
            names = (char **)realloc(names, (cap = cap ? cap * 2 : 32) * sizeof(char *));
        names[n++] = strdup(de->d_name);
    }
    closedir(d);
    qsort(names, n, sizeof(char *), cmp_names);
    for (i = 0; i < n; i++) {
        char path[1024];
        struct stat st;
        int idx;
        snprintf(path, sizeof path, "%s/%s", hostDir, names[i]);
        if (stat(path, &st) != 0)
            continue;
        idx = add_entry(names[i], path, self, S_ISDIR(st.st_mode), (u32)st.st_size);
        if (S_ISDIR(st.st_mode))
            walk(path, idx);
        free(names[i]);
    }
    free(names);
    sEntries[self].next = sNumEntries;
}

void DVDInit(void)
{
    char boot[1024];
    FILE *f;
    if (sNumEntries)
        return;
    snprintf(boot, sizeof boot, "%s/files", port_disc_root());
    add_entry("", boot, 0, 1, 0);
    walk(boot, 0);
    snprintf(boot, sizeof boot, "%s/sys/boot.bin", port_disc_root());
    if ((f = fopen(boot, "rb"))) {
        fread(&sDiskID, 1, sizeof sDiskID, f);
        fclose(f);
    } else {
        memcpy(sDiskID.gameName, "GM4E", 4);
        memcpy(sDiskID.company, "01", 2);
    }
    fprintf(stderr, "[dvd] %d entries under %s\n", sNumEntries, port_disc_root());
}

static int child_named(int dir, const char *name, int len)
{
    int i = dir + 1;
    while (i < sEntries[dir].next) {
        if ((int)strlen(sEntries[i].name) == len && !strncasecmp(sEntries[i].name, name, len))
            return i;
        i = sEntries[i].isDir ? sEntries[i].next : i + 1;
    }
    return -1;
}

s32 DVDConvertPathToEntrynum(const char *path)
{
    int dir;
    DVDInit();
    dir = (*path == '/') ? 0 : sCurrentDir;
    while (*path) {
        const char *end;
        int len;
        while (*path == '/')
            path++;
        if (!*path)
            break;
        end = strchr(path, '/');
        len = end ? (int)(end - path) : (int)strlen(path);
        if (len == 1 && path[0] == '.') {
        } else if (len == 2 && path[0] == '.' && path[1] == '.') {
            dir = sEntries[dir].parent;
        } else {
            dir = child_named(dir, path, len);
            if (dir < 0)
                return -1;
        }
        path += len;
    }
    return dir;
}

/* Disc formats are big-endian; serve converted (host-order) copies of the
 * formats port/conv knows. Media streams are read piecewise and stay raw. */
static void load_converted(Entry *e)
{
    FILE *f;
    u8 *raw;
    unsigned outSize;
    long n;
    if (e->convState == 2 || e->conv)
        return;
    if (!port_conv_wants(e->name)) {
        e->convState = 2;
        return;
    }
    f = fopen(e->hostPath, "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    raw = (u8 *)malloc((size_t)n);
    fread(raw, 1, (size_t)n, f);
    fclose(f);
    e->conv = port_conv_file(e->name, raw, (unsigned)n, &outSize);
    free(raw);
    e->size = outSize;
    e->convState = 1;
}

BOOL DVDFastOpen(s32 entrynum, DVDFileInfo *fi)
{
    DVDInit();
    if (entrynum < 0 || entrynum >= sNumEntries || sEntries[entrynum].isDir)
        return FALSE;
    load_converted(&sEntries[entrynum]);
    if (sEntries[entrynum].conv)
        sEntries[entrynum].convOpen++;
    memset(fi, 0, sizeof *fi);
    fi->startAddr = (u32)entrynum; /* host: the entry number stands in for the disc offset */
    fi->length = sEntries[entrynum].size;
    fi->cb.state = DVD_STATE_END;
    return TRUE;
}

BOOL DVDOpen(const char *fileName, DVDFileInfo *fi)
{
    s32 n = DVDConvertPathToEntrynum(fileName);
    if (n < 0) {
        fprintf(stderr, "[dvd] not found: %s\n", fileName);
        return FALSE;
    }
    return DVDFastOpen(n, fi);
}

BOOL DVDClose(DVDFileInfo *fi)
{
    Entry *e;
    if (fi->startAddr >= (u32)sNumEntries)
        return TRUE;
    e = &sEntries[fi->startAddr];
    if (e->conv && --e->convOpen <= 0) {
        free(e->conv);
        e->conv = NULL;
        e->convOpen = 0;
    }
    return TRUE;
}

static s32 read_entry(DVDFileInfo *fi, void *addr, s32 length, s32 offset)
{
    Entry *e = &sEntries[fi->startAddr];
    FILE *f;
    size_t got;
    if (e->convState == 1 && !e->conv)
        load_converted(e); /* read after close: reload */
    if (e->conv) {
        got = offset < 0 || (u32)offset >= e->size ? 0 : e->size - (u32)offset;
        if (got > (size_t)length)
            got = (size_t)length;
        memcpy(addr, e->conv + offset, got);
        if ((s32)got < length)
            memset((u8 *)addr + got, 0, (size_t)length - got);
        return length;
    }
    f = fopen(e->hostPath, "rb");
    if (!f)
        return DVD_RESULT_FATAL_ERROR;
    fseek(f, offset, SEEK_SET);
    got = fread(addr, 1, (size_t)length, f);
    fclose(f);
    /* the drive transfers whole 32-byte units; zero what lies past EOF */
    if ((s32)got < length)
        memset((u8 *)addr + got, 0, (size_t)length - got);
    return length;
}

s32 DVDReadPrio(DVDFileInfo *fi, void *addr, s32 length, s32 offset, s32 prio)
{
    (void)prio;
    return read_entry(fi, addr, length, offset);
}

static void read_done(void *a, void *b)
{
    DVDFileInfo *fi = (DVDFileInfo *)a;
    s32 result = (s32)(uintptr_t)b;
    fi->cb.state = result < 0 ? DVD_STATE_FATAL_ERROR : DVD_STATE_END;
    fi->cb.transferredSize = result < 0 ? 0 : (u32)result;
    if (fi->callback)
        fi->callback(result, fi);
}

BOOL DVDReadAsyncPrio(DVDFileInfo *fi, void *addr, s32 length, s32 offset, DVDCallback callback, s32 prio)
{
    s32 r;
    (void)prio;
    fi->callback = callback;
    fi->cb.state = DVD_STATE_BUSY;
    fi->cb.addr = addr;
    fi->cb.length = (u32)length;
    fi->cb.offset = (u32)offset;
    r = read_entry(fi, addr, length, offset);
    port_defer(read_done, fi, (void *)(uintptr_t)r);
    return TRUE;
}

s32 DVDGetCommandBlockStatus(const DVDCommandBlock *block)
{
    port_poll();
    return block->state;
}

s32 DVDGetDriveStatus(void)
{
    port_poll();
    return DVD_STATE_END;
}

int DVDCheckDisk(void) { return 1; }
DVDDiskID *DVDGetCurrentDiskID(void) { return &sDiskID; }

BOOL DVDOpenDir(char *dirName, DVDDir *dir)
{
    s32 n = DVDConvertPathToEntrynum(dirName);
    if (n < 0 || !sEntries[n].isDir)
        return FALSE;
    dir->entryNum = (u32)n;
    dir->location = (u32)n + 1;
    dir->next = (u32)sEntries[n].next;
    return TRUE;
}

BOOL DVDReadDir(DVDDir *dir, DVDDirEntry *dirent)
{
    u32 i = dir->location;
    Entry *e;
    if (i <= dir->entryNum || i >= dir->next)
        return FALSE;
    e = &sEntries[i];
    dirent->entryNum = i;
    dirent->isDir = e->isDir;
    dirent->name = e->name;
    dir->location = e->isDir ? (u32)e->next : i + 1;
    return TRUE;
}

BOOL DVDCloseDir(DVDDir *dir)
{
    (void)dir;
    return TRUE;
}
