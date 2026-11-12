#include <dolphin/types.h>
#include <dolphin/upnp/UPnPHttpd.h>
#include <dolphin/upnp/UPnPHttp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <extras.h>

#define SO_SHUT_WR 1

s32 SOShutdown(s32 s, s32 how);
s32 SOWrite(s32 s, const void* buf, s32 len);

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

typedef struct MediaTypeEntry {
    const char* ext;
    const char* type;
    const char* subtype;
} MediaTypeEntry;

static char* Status1xx[] = {
    "Continue",
    "Switching Protocols",
};

static char* Status2xx[] = {
    "OK",
    "Created",
    "Accepted",
    "Non-Authoratative Information",
    "No Content",
    "Reset Content",
    "Partial Content",
};

static char* Status3xx[] = {
    "Multiple Choices",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Use Proxy",
    "",
    "Temporary Redirect",
};

static char* Status4xx[] = {
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Timeout",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Request Entity Too Large",
    "Request-URI Too Long",
    "Unsupported Media Type",
    "Requested Range Not Satisfiable",
    "Expectation Failed",
};

static char* Status5xx[] = {
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
};

static const MediaTypeEntry MediaType[] = {
    { "aif", "audio", "aiff" },
    { "aifc", "audio", "aiff" },
    { "aiff", "audio", "aiff" },
    { "asf", "video", "x-ms-asf" },
    { "asx", "video", "x-ms-asf" },
    { "au", "audio", "basic" },
    { "avi", "video", "msvideo" },
    { "bmp", "image", "bmp" },
    { "dcr", "application", "x-director" },
    { "dib", "image", "bmp" },
    { "dir", "application", "x-director" },
    { "dxr", "application", "x-director" },
    { "gif", "image", "gif" },
    { "hta", "text", "hta" },
    { "htm", "text", "html" },
    { "html", "text", "html" },
    { "jar", "application", "java-archive" },
    { "jfif", "image", "pjpeg" },
    { "jpe", "image", "jpeg" },
    { "jpeg", "image", "jpeg" },
    { "jpg", "image", "jpeg" },
    { "js", "application", "x-javascript" },
    { "kar", "audio", "midi" },
    { "m3u", "audio", "mpegurl" },
    { "mid", "audio", "midi" },
    { "midi", "audio", "midi" },
    { "mov", "video", "quicktime" },
    { "mp2v", "video", "x-mpeg2" },
    { "mp3", "audio", "mpeg" },
    { "mpe", "video", "mpeg" },
    { "mpeg", "video", "mpeg" },
    { "mpg", "video", "mpeg" },
    { "mpv", "video", "mpeg" },
    { "mpv2", "video", "x-mpeg2" },
    { "pdf", "application", "pdf" },
    { "pjp", "image", "jpeg" },
    { "pjpeg", "image", "jpeg" },
    { "plg", "text", "html" },
    { "pls", "audio", "scpls" },
    { "png", "image", "png" },
    { "qt", "video", "quicktime" },
    { "ram", "audio", "x-pn-realaudio" },
    { "rmi", "audio", "mid" },
    { "rmm", "audio", "x-pn-realaudio" },
    { "rtf", "application", "rtf" },
    { "shtml", "text", "html" },
    { "smf", "audio", "midi" },
    { "snd", "audio", "basic" },
    { "spl", "application", "futuresplash" },
    { "ssm", "application", "streamingmedia" },
    { "swf", "application", "x-shockwave-flash" },
    { "tar", "application", "tar" },
    { "tcl", "application", "x-tcl" },
    { "text", "text", "plain" },
    { "tif", "image", "tiff" },
    { "tiff", "image", "tiff" },
    { "txt", "text", "plain" },
    { "ulw", "audio", "basic" },
    { "wav", "audio", "wav" },
    { "wax", "audio", "x-ms-wax" },
    { "wm", "video", "x-ms-wm" },
    { "wma", "audio", "x-ms-wma" },
    { "wmv", "video", "x-ms-wmv" },
    { "wvx", "video", "x-ms-wvx" },
    { "xbm", "image", "x-xbitmap" },
    { "xml", "text", "xml" },
    { "xsl", "text", "xml" },
    { "z", "application", "x-compress" },
    { "zip", "application", "zip" },
};

static const char* GetStatusString(s32 status)
{
    s32 index;

    if (status < 100 && 600 <= status) {
        return NULL;
    }

    index = status % 100;
    if (status / 100 == 1 && index < ARRAY_COUNT(Status1xx)) {
        return Status1xx[index];
    }
    if (status / 100 == 2 && index < ARRAY_COUNT(Status2xx)) {
        return Status2xx[index];
    }
    if (status / 100 == 3 && index < ARRAY_COUNT(Status3xx)) {
        return Status3xx[index];
    }
    if (status / 100 == 4 && index < ARRAY_COUNT(Status4xx)) {
        return Status4xx[index];
    }
    if (status / 100 == 5 && index < ARRAY_COUNT(Status5xx)) {
        return Status5xx[index];
    }
    return NULL;
}

static s32 CreateContent(char* body, s32 status)
{
    const char* reason;
    s32 len;

    reason = GetStatusString(status);
    len = sprintf(body, "<HTML><HEAD>");
    len += sprintf(body + len, "<TITLE>%d %s</TITLE>", status, reason);
    len += sprintf(body + len, "</HEAD><BODY>");
    len += sprintf(body + len, "<H1>%s</H1>", reason);
    sprintf(body + len, "</BODY></HTML>");
    return strlen(body);
}

static int compare(const void* key, const void* entry)
{
    return stricmp((const char*)key, ((const MediaTypeEntry*)entry)->ext);
}

static BOOL SearchExtension(const char* ext, const char** type, const char** subtype)
{
    const MediaTypeEntry* entry;

    entry = (const MediaTypeEntry*)bsearch(ext, MediaType, ARRAY_COUNT(MediaType), sizeof(MediaTypeEntry), compare);
    if (entry) {
        *type = entry->type;
        *subtype = entry->subtype;
        return TRUE;
    }
    return FALSE;
}

static void GetContentType(char* buf, const char* path)
{
    const char* ext;
    const char* type;
    const char* subtype;
    BOOL found;

    found = FALSE;
    ext = strrchr(path, '.');
    if (ext && SearchExtension(ext + 1, &type, &subtype)) {
        found = TRUE;
    }
    if (!found) {
        type = "application";
        subtype = "octet-stream";
    }
    snprintf(buf, 64, "%s/%s", type, subtype);
}

BOOL SendResponseOK(s32 sock, s32 size, const char* path, const char* lang)
{
    char response[256];
    char contentType[64];
    char contentLength[16];
    s32 len;

    HTTPCreateResponse(response, 200, "OK", "HTTP/1.1");
    HTTPAddHeader(response, "Connection", "close");
    if (strlen(lang) != 0) {
        HTTPAddHeader(response, "Content-Language", "en");
    }
    GetContentType(contentType, path);
    HTTPAddHeader(response, "Content-Type", contentType);
    sprintf(contentLength, "%d", size);
    len = HTTPAddHeader(response, "Content-Length", contentLength);
    if (SOWrite(sock, response, len) < 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL SendResponseStatus(s32 sock, s32 status, const char* flags)
{
    char response[256];
    char body[256];
    char contentLength[16];
    s32 len;
    s32 bodyLen;
    BOOL hasBody;
    char c;

    hasBody = FALSE;
    GetStatusString(status);
    len = HTTPCreateResponse(response, status, GetStatusString(status), "HTTP/1.1");
    while ((c = *flags++) != '\0') {
        if (c == 'C') {
            len = HTTPAddHeader(response, "Connection", "close");
        } else if (c == 'A') {
            len = HTTPAddHeader(response, "Allow", "GET, HEAD");
        } else if (c == 'B') {
            hasBody = TRUE;
            bodyLen = CreateContent(body, status);
            sprintf(contentLength, "%d", bodyLen);
            len = HTTPAddHeader(response, "Content-Length", contentLength);
        }
    }
    if (hasBody && 256 - len > bodyLen) {
        len += sprintf(response + len, body);
    }
    if (SOWrite(sock, response, len) < 0) {
        return FALSE;
    }
    SOShutdown(sock, SO_SHUT_WR);
    return TRUE;
}
