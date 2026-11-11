#include <dolphin/types.h>
#include <dolphin/upnp/UPnPHttp.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static BOOL IsProtocol(const char* token, int len)
{
    if (len == 4 && strncmp(token, "HTTP", 4) == 0) {
        return TRUE;
    }
    if (len == 3 && strncmp(token, "SIP", 3) == 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL __HTTPIsSpace(char c)
{
    return c == ' ' || c == '\t';
}

static char* __HTTPSkipSpace(char* p)
{
    while (__HTTPIsSpace(*p)) {
        p++;
    }
    return p;
}

static char* __HTTPSkipSWS(char* p)
{
    for (;;) {
        p = __HTTPSkipSpace(p);
        if (p[0] == '\r' && p[1] == '\n' && __HTTPIsSpace(p[2])) {
            p += 3;
        } else {
            return p;
        }
    }
}

static char* __HTTPSkipChar(char* p, char c)
{
    p = __HTTPSkipSWS(p);
    if (p == NULL || *p != c) {
        return NULL;
    }
    return __HTTPSkipSWS(p + 1);
}

static char* __HTTPSkipColon(char* p)
{
    p = __HTTPSkipSpace(p);
    if (*p != ':') {
        return NULL;
    }
    return __HTTPSkipSWS(p + 1);
}

static char* __HTTPGetToken(char* p, char** token, int* len)
{
    *token = NULL;
    *len = 0;
    while (*p != '\0' && (isalpha(*p) || isdigit(*p) || strchr("-.!%*_+`'~", *p))) {
        if (*token == NULL) {
            *token = p;
        }
        ++*len;
        ++p;
    }
    return p;
}

static char* __HTTPGetDigit(char* p, char** token, int* len)
{
    *token = NULL;
    *len = 0;
    while (isdigit(*p)) {
        if (*token == NULL) {
            *token = p;
        }
        ++*len;
        ++p;
    }
    return p;
}

static char* __HTTPSkipToNextHeader(char* p)
{
    for (;;) {
        if (*p == '\0') {
            return NULL;
        }
        if (*p == '\r' && p[1] == '\n' && !__HTTPIsSpace(*(p += 2))) {
            return p;
        }
        p++;
    }
}

static inline int StrNICmp(const char* s1, const char* s2, u32 n)
{
    u32 i;
    char c1;
    char c2;

    for (i = 0; i < n; i++) {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return 0;
}

static inline int Min(int a, int b)
{
    return a < b ? a : b;
}

char* __HTTPGetHeader(char* msg, const char* name)
{
    char* sub;
    char* p;
    char* token;
    int len;

    sub = strchr(name, '/');
    if (sub != NULL) {
        sub++;
    }
    while (msg != NULL) {
        p = __HTTPGetToken(msg, &token, &len);
        msg = p;
        if (token == NULL) {
            return NULL;
        }
        if (sub != NULL) {
            if (StrNICmp(token, sub, len) == 0 && sub[len] == '\0') {
                msg = __HTTPSkipColon(msg);
                break;
            }
            if (StrNICmp(token, name, len) == 0 && name[len] == '/') {
                msg = __HTTPSkipColon(msg);
                break;
            }
        } else {
            if (StrNICmp(token, name, len) == 0 && name[len] == '\0') {
                msg = __HTTPSkipColon(msg);
                break;
            }
        }
        msg = __HTTPSkipToNextHeader(p);
    }
    return msg;
}

char* __HTTPGetVersion(char* msg, char** version, int* versionLen)
{
    char* p;
    char* token;
    int len;

    *version = NULL;
    *versionLen = 0;
    p = __HTTPGetToken(msg, &token, &len);
    if (token == NULL) {
        return p;
    }
    if (!IsProtocol(token, len)) {
        return p;
    }
    if (*p != '/') {
        return p;
    }
    p = __HTTPGetDigit(p + 1, &token, &len);
    if (token == NULL) {
        return p;
    }
    if (*p != '.') {
        return p;
    }
    p = __HTTPGetDigit(p + 1, &token, &len);
    if (token == NULL) {
        return p;
    }
    *version = msg;
    *versionLen = p - msg;
    return p;
}

static char* __HTTPGetURI(char* p, char** token, int* len)
{
    char* q;

    *token = NULL;
    *len = 0;
    for (q = p; *q != '\0' && strchr(" ", *q) == NULL; q++) {
        if (*token == NULL) {
            *token = p;
        }
    }
    *len = q - p;
    return q;
}

char* __HTTPGetParameter(char* p, const char* name)
{
    char* token;
    int len;
    BOOL quote;

    do {
        p = __HTTPGetToken(p, &token, &len);
        if (len > 0 && StrNICmp(token, name, len) == 0 && name[len] == '\0') {
            p = __HTTPSkipSWS(p);
            if (p == NULL) {
                return token + len;
            }
            return p;
        }
        quote = FALSE;
        while (quote || *p != ';') {
            if (*p == '\0') {
                p = NULL;
                break;
            }
            if (*p == '\r' && p[1] == '\n' && !__HTTPIsSpace(*(p += 2))) {
                p = NULL;
                break;
            }
            if (*p == '"') {
                quote ^= TRUE;
            }
            p++;
        }
        if (p == NULL) {
            return NULL;
        }
    } while ((p = __HTTPSkipChar(p, ';')) != NULL);
    return NULL;
}

s32 HTTPGetProtocol(char* msg, char* protocol)
{
    char* p;
    char* token;
    int len;

    p = __HTTPGetToken(msg, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL) {
        return -1;
    }
    if (*p == '/') {
        p = __HTTPGetVersion(token, &token, &len);
        if (*p == '\0') {
            return 0;
        }
        if (token == NULL || *p != ' ') {
            return -1;
        }
    } else if (*p == ' ') {
        p = __HTTPGetURI(p + 1, &token, &len);
        if (*p == '\0') {
            return 0;
        }
        if (token == NULL || *p != ' ') {
            return -1;
        }
        p = __HTTPGetVersion(p + 1, &token, &len);
        if (*p == '\0') {
            return 0;
        }
        if (token == NULL || *p != '\r') {
            return -1;
        }
    } else {
        return -1;
    }
    memcpy(protocol, token, len);
    protocol[len] = '\0';
    return len;
}

s32 HTTPGetMethod(char* msg, char* method)
{
    char* p;
    char* token;
    int len;

    p = __HTTPGetToken(msg, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || *p != ' ' || len >= 16) {
        return -1;
    }
    memcpy(method, token, len);
    method[len] = '\0';
    return len;
}

s32 HTTPGetRequestURI(char* msg, char* uri, s32 size)
{
    char* p;
    char* token;
    int len;

    if (size < 1) {
        return -1;
    }
    p = __HTTPGetToken(msg, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || *p != ' ' || len >= 16) {
        return -1;
    }
    p = __HTTPGetURI(p + 1, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || *p != ' ') {
        return -1;
    }
    size = (len < size - 1) ? len : size - 1;
    memmove(uri, token, size);
    uri[size] = '\0';
    return len;
}

s32 HTTPGetHeader(char* msg, const char* name, char* value, s32 size)
{
    char* p;
    char* header;
    char* end;
    int len;
    int n;

    if (size < 1) {
        return -1;
    }
    p = strchr(msg, '\n');
    if (p != NULL) {
        n = p - msg - 1;
        msg = p + 1;
    } else {
        n = 0;
    }
    if (n <= 0) {
        return -1;
    }
    p = header = __HTTPGetHeader(msg, name);
    if (header == NULL) {
        return -1;
    }
    end = __HTTPSkipToNextHeader(header);
    if (end == NULL) {
        return -1;
    }
    len = end - header - 2;
    while (len > 0 && __HTTPIsSpace(p[len - 1])) {
        len--;
    }
    n = Min(len, size - 1);
    memcpy(value, p, size = n);
    value[n] = 0;

    return len;
}

s32 HTTPGetParameter(char* msg, const char* name, char* value, s32 size)
{
    const char* delim;
    char* p;
    int len;

    if (size < 1) {
        return -1;
    }
    msg = __HTTPGetParameter(msg, name);
    if (msg == NULL) {
        return -1;
    }
    len = 0;
    if (*msg == '=') {
        msg = __HTTPSkipChar(msg, '=');
        if (msg != NULL) {
            if (*msg == '"') {
                delim = ";\"\r\n";
                msg++;
            } else {
                delim = " ;\t\r\n";
            }
            for (p = msg; strchr(delim, *p) == NULL && *p != '\0'; p++) {
                len++;
            }
        }
    }
    if (len < --size) {
        size = len;
    }
    memmove(value, msg, size);
    value[size] = '\0';
    return len;
}

s32 HTTPGetStatus(char* msg)
{
    char* p;
    char* token;
    int len;
    s32 status;

    p = __HTTPGetToken(msg, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || *p != '/') {
        return -1;
    }
    p = __HTTPGetVersion(token, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || *p != ' ') {
        return -1;
    }
    p = __HTTPGetDigit(p + 1, &token, &len);
    if (*p == '\0') {
        return 0;
    }
    if (token == NULL || len != 3 || *p != ' ') {
        return -1;
    }
    status = 0;
    while (len > 0) {
        status *= 10;
        status += *token - '0';
        len--;
        token++;
    }
    return status;
}

s32 HTTPAddHeader(char* msg, const char* name, const char* value)
{
    char* p;
    int len;

    p = strstr(msg, "\r\n\r\n");
    if (p == NULL) {
        return -1;
    }
    len = sprintf(p, "\r\n%s: %s\r\n\r\n", name, value);
    return len + (p - msg);
}

s32 HTTPCreateRequest(char* msg, const char* method, const char* uri, const char* version)
{
    char* start = msg;

    strcpy(msg, method);
    msg += strlen(method);
    *msg++ = ' ';
    strcpy(msg, uri);
    msg += strlen(uri);
    *msg++ = ' ';
    strcpy(msg, version);
    msg += strlen(version);
    strcpy(msg, "\r\n");
    strcpy(msg + 2, "\r\n");
    return msg + 4 - start;
}

s32 HTTPCreateResponse(char* msg, s32 status, const char* reason, const char* version)
{
    char* start = msg;

    msg += sprintf(msg, "%s %d %s\r\n", version, status, reason);
    strcpy(msg, "\r\n");
    return msg + 2 - start;
}
