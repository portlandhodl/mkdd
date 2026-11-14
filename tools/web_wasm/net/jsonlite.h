// Tiny JSON reader for the bridge's JSON strings (session, lobby state, bus events). Values are
// pointers into the source text; nothing is allocated. Only what the online code needs: objects,
// arrays, integers, booleans and printable ASCII strings.
#ifndef MKDD_NET_JSONLITE_H
#define MKDD_NET_JSONLITE_H

#include <string.h>

namespace jsonlite {

inline const char *jws(const char *p)
{
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
        p++;
    return p;
}
inline const char *jstr(const char *p) // p at '"': past the string
{
    for (p++; *p && *p != '"'; p++)
        if (*p == '\\' && p[1])
            p++;
    return *p ? p + 1 : p;
}
inline const char *jskip(const char *p)
{
    p = jws(p);
    if (*p == '"')
        return jstr(p);
    if (*p == '{' || *p == '[') {
        int depth = 0;
        while (*p) {
            if (*p == '"') {
                p = jstr(p);
                continue;
            }
            if (*p == '{' || *p == '[')
                depth++;
            else if ((*p == '}' || *p == ']') && --depth == 0)
                return p + 1;
            p++;
        }
        return p;
    }
    while (*p && *p != ',' && *p != '}' && *p != ']')
        p++;
    return p;
}
// value of `key` in the object at p, or nullptr
inline const char *jfield(const char *p, const char *key)
{
    if (!p)
        return nullptr;
    p = jws(p);
    if (*p != '{')
        return nullptr;
    p++;
    size_t n = strlen(key);
    for (;;) {
        p = jws(p);
        if (*p != '"')
            return nullptr;
        const char *k = p + 1, *ke = jstr(p);
        p = jws(ke);
        if (*p != ':')
            return nullptr;
        p = jws(p + 1);
        if ((size_t)(ke - 1 - k) == n && !memcmp(k, key, n))
            return p;
        p = jws(jskip(p));
        if (*p != ',')
            return nullptr;
        p++;
    }
}
// element i of the array at p, or nullptr
inline const char *jelem(const char *p, int i)
{
    if (!p || *(p = jws(p)) != '[')
        return nullptr;
    p = jws(p + 1);
    for (int k = 0; *p && *p != ']'; k++) {
        if (k == i)
            return p;
        p = jws(jskip(p));
        if (*p == ',')
            p = jws(p + 1);
    }
    return nullptr;
}
inline int jint(const char *p, int def)
{
    if (!p)
        return def;
    if (!strncmp(p, "true", 4))
        return 1;
    if (!strncmp(p, "false", 5) || !strncmp(p, "null", 4))
        return 0;
    int sign = 1, v = 0;
    if (*p == '-') {
        sign = -1;
        p++;
    }
    if (*p < '0' || *p > '9')
        return def;
    while (*p >= '0' && *p <= '9')
        v = v * 10 + (*p++ - '0');
    return sign * v;
}
inline void jcopy(const char *p, char *out, int cap) // printable ASCII of a string value
{
    int n = 0;
    if (p && *p == '"')
        for (p++; *p && *p != '"' && n < cap - 1; p++) {
            if (*p == '\\' && p[1])
                p++;
            if (*p >= 0x20 && *p < 0x7f && *p != '"' && *p != '\\')
                out[n++] = *p;
        }
    out[n] = 0;
}

} // namespace jsonlite

#endif
