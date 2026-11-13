/* MSL (Metrowerks C library) symbols that game code reaches through MSL's own
 * headers. Everything else in libc comes from Emscripten's musl. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* MSL's FILE is a different struct; only its address identifies the stream. */
typedef struct { unsigned char opaque[0x50]; } MSLFile;
MSLFile __files[4];

static FILE *host_stream(void *f)
{
    return f == &__files[2] ? stderr : stdout;
}

/* Game code's fprintf (renamed by port/include/mkdd_port.h) takes MSL FILE
 * pointers; route them to the host streams. */
int port_msl_fprintf(void *f, const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(host_stream(f), fmt, ap);
    va_end(ap);
    return r;
}

unsigned char __ctype_map[256];
unsigned char __lower_map[256];
unsigned char __upper_map[256];

__attribute__((constructor)) static void msl_ctype_init(void)
{
    int c;
    for (c = 0; c < 256; c++) {
        unsigned char m = 0;
        /* bits from MSL ctype_api.h */
        if (c < 128) {
            if (iscntrl(c) && !isspace(c)) m |= 0x01;
            if (isspace(c) && c != ' ') m |= 0x02;
            if (c == ' ') m |= 0x04;
            if (ispunct(c)) m |= 0x08;
            if (isdigit(c)) m |= 0x10;
            if (isxdigit(c)) m |= 0x20;
            if (islower(c)) m |= 0x40;
            if (isupper(c)) m |= 0x80;
        }
        __ctype_map[c] = m;
        __lower_map[c] = (unsigned char)(c < 128 ? tolower(c) : c);
        __upper_map[c] = (unsigned char)(c < 128 ? toupper(c) : c);
    }
}

int __float_huge[] = {0x7F800000};
int __float_max[] = {0x7F7FFFFF};
int __float_epsilon[] = {0x34000000};

int stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int strnicmp(const char *a, const char *b, int n) { return strncasecmp(a, b, (size_t)n); }
int __msl_strnicmp(const char *a, const char *b, int n) { return strncasecmp(a, b, (size_t)n); }
