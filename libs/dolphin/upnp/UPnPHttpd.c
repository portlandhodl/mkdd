#include <dolphin/types.h>
#include <dolphin/upnp/UPnPHttpd.h>
#include <dolphin/upnp/UPnPHttp.h>
#include <dolphin/upnp/UPnP.h>
#include <dolphin/ip/IPSocket.h>
#include <dolphin/os.h>
#include <string.h>

#define HTTP_STATE_STOPPED 0
#define HTTP_STATE_INITIALIZED 1
#define HTTP_STATE_RUNNING 2

#define HTTP_MAX_CONNECTIONS 5
#define HTTP_BUFFER_SIZE 512

#define HTTP_METHOD_GET 0
#define HTTP_METHOD_HEAD 3
#define HTTP_NUM_METHODS 8

#define SO_SOCK_STREAM 1
#define SO_SOL_SOCKET 0xFFFF
#define SO_SO_REUSEADDR 4
#define SO_SO_LINGER 0x80
#define SO_F_GETFL 3
#define SO_F_SETFL 4
#define SO_O_NONBLOCK 4
#define SO_SHUT_WR 1
#define SO_POLLWRNORM 0x0008
#define SO_POLLHUP 0x0020

typedef struct SOLinger {
    s32 onoff;
    s32 linger;
} SOLinger;

typedef struct HTTPConnection {
    char mBuffer[HTTP_BUFFER_SIZE]; // 0x000
    s32 mLength;                    // 0x200
    s32 mSize;                      // 0x204
    u8* mData;                      // 0x208
    s32 mSent;                      // 0x20C
    OSTime mTimeout;                // 0x210
    BOOL mKeepAlive;                // 0x218
} HTTPConnection;

s32 SOListen(s32 s, s32 backlog);
s32 SOAccept(s32 s, u8* addr);
s32 SOShutdown(s32 s, s32 how);
s32 SORead(s32 s, void* buf, s32 len);
s32 SOWrite(s32 s, const void* buf, s32 len);
s32 SOFcntl(s32 s, s32 cmd, ...);

static OSThreadQueue Event;
static u32 HostAddr;
static HTTPFile* HttpFile;
static u32 NumOfFile;
static s32 State;
static HTTPConnection Sdata[HTTP_MAX_CONNECTIONS];

static s32 GetRequestedFile(char* uri)
{
    char path[HTTP_BUFFER_SIZE];
    char http[] = "http://";
    u32 len;
    u32 i;
    char* p;

    len = strlen(uri);
    if (strncmp(uri, http, 7) == 0) {
        if (len <= 7) {
            return -1;
        }
        p = strchr(uri + 7, '/');
        uri = p;
        if (p == NULL) {
            return -1;
        }
    }
    UPnPReplaceEscaped(uri);
    UPnPRemoveSemicolon(uri);
    if (UPnPRemoveDots(uri) < 0) {
        return -1;
    }
    if (*uri != '/') {
        strcpy(path, "/");
        strcat(path, uri);
    } else {
        strcpy(path, uri);
    }
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    for (i = 0; i < NumOfFile; i++) {
        if (strcmp(path, HttpFile[i].mPath) == 0) {
            break;
        }
    }
    if (i == NumOfFile) {
        return -1;
    }
    return i;
}

static void AbortSocket(s32 s, SOPollFd* fds, s32 i, s32 sock)
{
    SOLinger linger;

    linger.linger = 0;
    linger.onoff = 1;
    SOSetSockOpt(s, SO_SOL_SOCKET, SO_SO_LINGER, &linger, sizeof(SOLinger));
    SOClose(s);
    fds[i].fd = -1;
    if (fds[0].fd == -1) {
        fds[0].fd = sock;
    }
}

static void CloseSocket(s32 s, SOPollFd* fds, s32 i, s32 sock)
{
    s32 flags;

    flags = SOFcntl(s, SO_F_GETFL, 0);
    SOFcntl(s, SO_F_SETFL, flags & ~SO_O_NONBLOCK);
    SOClose(s);
    fds[i].fd = -1;
    if (fds[0].fd == -1) {
        fds[0].fd = sock;
    }
}

static s32 CheckMethod(char* method, BOOL complete)
{
    const char* methods[HTTP_NUM_METHODS] = {
        "GET", "PUT", "POST", "HEAD", "TRACE", "DELETE", "CONNECT", "OPTIONS",
    };
    u32 minLen;
    u32 maxLen;
    u32 len;
    u32 i;
    u32 n;
    u32 unused;

    minLen = strlen(methods[0]);
    maxLen = strlen(methods[HTTP_NUM_METHODS - 1]);
    len = strlen(method);
    if (len > maxLen) {
        return -1;
    }
    if (complete) {
        if (len < minLen) {
            return -1;
        }
        for (i = 0; i < HTTP_NUM_METHODS; i++) {
            n = strlen(methods[i]);
            if (len == n && strncmp(method, methods[i], n) == 0) {
                return i;
            }
        }
        return -1;
    }
    for (i = 0; i < HTTP_NUM_METHODS; i++) {
        if (strncmp(method, methods[i], len) == 0) {
            return HTTP_NUM_METHODS;
        }
    }
    return -1;
}

static s32 ReadHttpHeader(char* buf, s32 len, s32* method, char* uri)
{
    char methodName[16];
    char protocol[16];
    s32 result;
    s32 i;

    if (buf[0] == '\r' && buf[1] == '\n') {
        buf += 2;
        len -= 2;
        if (len <= 0) {
            return 1;
        }
    }
    memset(methodName, 0, sizeof(methodName));
    result = HTTPGetMethod(buf, methodName);
    if (result < 0) {
        return -1;
    }
    if (result == 0) {
        if (CheckMethod(methodName, FALSE) == -1) {
            return -1;
        }
        return 1;
    }
    *method = CheckMethod(methodName, TRUE);
    if (*method == -1) {
        return -1;
    }
    memset(uri, 0, HTTP_BUFFER_SIZE);
    if (HTTPGetRequestURI(buf, uri, HTTP_BUFFER_SIZE) == 0) {
        return 1;
    }
    result = HTTPGetProtocol(buf, protocol);
    if (result < 0) {
        return -1;
    }
    if (result == 0) {
        return 1;
    }
    for (i = len - 4; i >= 0; i--) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return 0;
        }
    }
    return 1;
}

static s32 SendResponse(s32 sock, char* buf, s32 method, char* uri, s32* file)
{
    char lang[64] = "";

    if (buf[0] == '\r' && buf[1] == '\n') {
        buf += 2;
    }
    if (method != HTTP_METHOD_GET && method != HTTP_METHOD_HEAD) {
        if (SendResponseStatus(sock, 405, "CAB")) {
            return 0;
        }
        return -1;
    }
    *file = GetRequestedFile(uri);
    if (*file < 0) {
        if (SendResponseStatus(sock, 404, "CB")) {
            return 0;
        }
        return -1;
    }
    HTTPGetHeader(buf, "Accept-Language", lang, sizeof(lang));
    if (!SendResponseOK(sock, HttpFile[*file].mSize, HttpFile[*file].mPath, lang)) {
        return -1;
    }
    if (method == HTTP_METHOD_GET) {
        return 1;
    }
    SOShutdown(sock, SO_SHUT_WR);
    return 0;
}

static s32 SendContent(s32 sock, HTTPConnection* conn)
{
    s32 len;
    s32 result;

    len = conn->mSize - conn->mSent;
    result = SOWrite(sock, conn->mData + conn->mSent, len);
    if (result < 0) {
        return -1;
    }
    if (result < len) {
        conn->mSent += result;
        return 1;
    }
    conn->mSent += result;
    SOShutdown(sock, SO_SHUT_WR);
    return 0;
}

static s32 HTTPLoop(s32 sock)
{
    SOPollFd fds[HTTP_MAX_CONNECTIONS + 1];
    SOSockAddr addr;
    s32 file;
    s32 method;
    char uri[HTTP_BUFFER_SIZE];
    HTTPConnection* conn;
    s32 result;
    s32 s;
    s32 i;

    fds[0].fd = sock;
    fds[0].events = SO_POLLRDNORM;
    for (i = 1; i < HTTP_MAX_CONNECTIONS + 1; i++) {
        fds[i].fd = -1;
        memset(&Sdata[i - 1], 0, sizeof(HTTPConnection));
    }

    for (;;) {
        result = SOPoll(fds, HTTP_MAX_CONNECTIONS + 1, OSSecondsToTicks(1));
        if (result < 0) {
            return result;
        }

        if (fds[0].revents & SO_POLLRDNORM) {
            memset(&addr, 0, sizeof(addr));
            addr.len = sizeof(addr);
            result = SOAccept(sock, (u8*)&addr);
            if (result < 0) {
                return result;
            }
            for (i = 1; i < HTTP_MAX_CONNECTIONS + 1; i++) {
                if (fds[i].fd < 0) {
                    conn = &Sdata[i - 1];
                    fds[i].fd = result;
                    fds[i].events = SO_POLLRDNORM;
                    memset(conn, 0, sizeof(HTTPConnection));
                    conn->mTimeout = OSGetTime() + OSSecondsToTicks(30);
                    conn->mKeepAlive = TRUE;
                    break;
                }
            }
            for (i = 1; i < HTTP_MAX_CONNECTIONS + 1; i++) {
                if (fds[i].fd < 0) {
                    break;
                }
            }
            if (i == HTTP_MAX_CONNECTIONS + 1) {
                fds[0].fd = -1;
            }
        } else if (fds[0].revents & SO_POLLHUP) {
            continue;
        }

        for (i = 1; i < HTTP_MAX_CONNECTIONS + 1; i++) {
            s = fds[i].fd;
            conn = &Sdata[i - 1];
            if (s < 0) {
                continue;
            }
            if (conn->mTimeout < OSGetTime()) {
                AbortSocket(s, fds, i, sock);
                continue;
            }
            if (fds[i].revents & SO_POLLRDNORM) {
                result = SORead(s, conn->mBuffer + conn->mLength, HTTP_BUFFER_SIZE - conn->mLength);
                if (result < 0) {
                    AbortSocket(s, fds, i, sock);
                    continue;
                }
                if (result == 0) {
                    if (!conn->mKeepAlive) {
                        CloseSocket(s, fds, i, sock);
                    } else {
                        AbortSocket(s, fds, i, sock);
                    }
                    continue;
                }
                if ((conn->mLength += result) >= HTTP_BUFFER_SIZE) {
                    AbortSocket(s, fds, i, sock);
                    continue;
                }
                switch (ReadHttpHeader(conn->mBuffer, conn->mLength, &method, uri)) {
                case 1:
                    break;
                case -1:
                    AbortSocket(s, fds, i, sock);
                    break;
                case 0:
                    switch (SendResponse(s, conn->mBuffer, method, uri, &file)) {
                    case -1:
                        AbortSocket(s, fds, i, sock);
                        break;
                    case 1:
                        conn->mData = HttpFile[file].mData;
                        conn->mSize = HttpFile[file].mSize;
                        switch (SendContent(s, conn)) {
                        case -1:
                            AbortSocket(s, fds, i, sock);
                            break;
                        case 1:
                            fds[i].events = SO_POLLWRNORM;
                            break;
                        case 0:
                            conn->mKeepAlive = FALSE;
                            break;
                        }
                        break;
                    case 0:
                        conn->mKeepAlive = FALSE;
                        break;
                    }
                    break;
                }
            } else if (fds[i].revents & SO_POLLWRNORM) {
                switch (SendContent(s, conn)) {
                case 1:
                    break;
                case -1:
                    AbortSocket(s, fds, i, sock);
                    break;
                case 0:
                    conn->mKeepAlive = FALSE;
                    fds[i].events = SO_POLLRDNORM;
                    break;
                }
            } else if (fds[i].revents & SO_POLLHUP) {
                SORead(s, conn->mBuffer + conn->mLength, HTTP_BUFFER_SIZE - conn->mLength);
                AbortSocket(s, fds, i, sock);
            }
        }
    }
}

static void* HTTPMain(void* arg)
{
    u32 one;
    SOSockAddr addr;
    s32 sock;
    s32 flags;

    (void)arg;

    for (;;) {
        UPnPGetHostAddr(&HostAddr);
        if (HostAddr == 0 || State != HTTP_STATE_RUNNING) {
            OSSleepThread(&Event);
            continue;
        }

        sock = SOSocket(AF_INET, SO_SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }

        one = 1;
        SOSetSockOpt(sock, SO_SOL_SOCKET, SO_SO_REUSEADDR, &one, 4);

        memset(&addr, 0, sizeof(addr));
        addr.family = AF_INET;
        addr.len = sizeof(addr);
        *(u32*)addr.addr = 0;
        addr.port = 1900;
        if (SOBind(sock, (u8*)&addr) < 0) {
            goto done;
        }
        if (SOListen(sock, 5) < 0) {
            goto done;
        }
        flags = SOFcntl(sock, SO_F_GETFL, 0);
        SOFcntl(sock, SO_F_SETFL, flags | SO_O_NONBLOCK);
        HTTPLoop(sock);
done:
        SOClose(sock);
        State = HTTP_STATE_INITIALIZED;
    }
}

void UPnPHttpInit(HTTPConfig* config)
{
    if (State != HTTP_STATE_STOPPED) {
        return;
    }

    State = HTTP_STATE_INITIALIZED;
    OSInitThreadQueue(&Event);
    HttpFile = config->mFiles;
    NumOfFile = config->mNumFiles;
    OSCreateThread(config->mThread, HTTPMain, NULL, config->mStack, config->mStackSize, config->mPriority, 0);
    OSResumeThread(config->mThread);
}

void UPnPHttpStart(void)
{
    if (State == HTTP_STATE_INITIALIZED) {
        State = HTTP_STATE_RUNNING;
        OSWakeupThread(&Event);
    }
}
