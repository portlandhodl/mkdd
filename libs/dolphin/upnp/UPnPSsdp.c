#include <dolphin/types.h>
#include <dolphin/upnp/UPnPSsdp.h>
#include <dolphin/upnp/UPnPHttp.h>
#include <dolphin/upnp/UPnP.h>
#include <dolphin/ip/IPSocket.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>
#include <string.h>
#include <stdio.h>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/ctype_api.h>

#define SSDP_STATE_STOPPED 0
#define SSDP_STATE_INITIALIZED 1
#define SSDP_STATE_RUNNING 2
#define SSDP_STATE_STOPPING 3

#define SSDP_MAX_ENTRIES 16
#define SSDP_HTTP_MAX 512
#define SSDP_MULTICAST_ADDR (-268435462)

static char ServerString[0x40];
static char LocationString[0x40];
static char OutputBuffer[0x201];
static char InputBuffer[0x201];
static SSDPStat Stat;

static OSThreadQueue Event;
static SSDPInfo* InfoTable;
static s32 NumInfo;
static volatile s32 State;
static struct {
    SSDPSearchInfo* mHead;
    SSDPSearchInfo* mTail;
} SearchList;
static u8 Ttl;
static void (*Callback)(const char*, const void*);
static u32 HostAddr;

static int SSDPLoop(s32 sock);
static OSTime ProcessSearchList(s32 sock, OSTime now);
static void ClearSearchList(void);

static BOOL IsPrefix(const char* prefix, const char* str)
{
    while (*prefix != '\0') {
        if (*prefix != *str) {
            return FALSE;
        }
        prefix++;
        str++;
    }
    return TRUE;
}

static u32 Rotate(u32 x, u32 n)
{
    return (x << n) | (x >> (32 - n));
}

static OSTime Backoff(s32 state, s32 count)
{
    static const u32 exp[3] = {2250, 4750, 7250};
    u32 tick;
    u32 r;

    tick = OSGetTick();
    r = Rotate(tick, tick % 32);
    r %= OSMillisecondsToTicks(2200);
    return count * (OSMillisecondsToTicks(exp[3 - state]) - r) / 10;

}

static void SSDPSetTimer(SSDPTimer* timer, s32 count)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    timer->mCount = count;
    timer->mState = 3;
    timer->mStart = OSGetTime();
    timer->mNext = timer->mStart + Backoff(timer->mState, timer->mCount);
    OSRestoreInterrupts(enabled);
}

static void SSDPCancelTimer(SSDPTimer* timer)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    timer->mState = 0;
    OSRestoreInterrupts(enabled);
}

static OSTime SSDPTickTimer(SSDPTimer* timer, OSTime now)
{
    BOOL enabled;
    OSTime next;

    enabled = OSDisableInterrupts();
    if (timer->mCount > 0 && timer->mState > 0) {
        next = timer->mNext;
        if (now >= next && --timer->mState > 0) {
            timer->mNext = timer->mStart + Backoff(timer->mState, timer->mCount);
        }
    } else {
        next = now + OSSecondsToTicks((OSTime)1800);
    }
    OSRestoreInterrupts(enabled);
    return next;
}

static int SendNotify(s32 sock, SSDPInfo* info)
{
    SOSockAddr addr;
    u32 local;
    s32 len;

    memset(&addr, 0, sizeof(addr));
    addr.family = AF_INET;
    addr.len = sizeof(addr);
    *(u32*)addr.addr = SSDP_MULTICAST_ADDR;
    addr.port = 1900;

    if (State != SSDP_STATE_RUNNING && State != SSDP_STATE_STOPPING) {
        return -1;
    }

    HTTPCreateRequest(OutputBuffer, "NOTIFY", "*", "HTTP/1.1");
    HTTPAddHeader(OutputBuffer, "HOST", "239.255.255.250:1900");
    if (State == SSDP_STATE_RUNNING) {
        HTTPAddHeader(OutputBuffer, "CACHE-CONTROL", "max-age=1800");
        HTTPAddHeader(OutputBuffer, "LOCATION", LocationString);
        HTTPAddHeader(OutputBuffer, "NT", info->mName);
        HTTPAddHeader(OutputBuffer, "NTS", "ssdp:alive");
        HTTPAddHeader(OutputBuffer, "SERVER", ServerString);
    } else {
        HTTPAddHeader(OutputBuffer, "NT", info->mName);
        HTTPAddHeader(OutputBuffer, "NTS", "ssdp:byebye");
    }
    len = HTTPAddHeader(OutputBuffer, "USN", info->mUuidName);
    if (len <= 0) {
        return -1;
    }

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mNotifySent++;
    return SOSendTo(sock, OutputBuffer, len, 0, (u8*)&addr);
}

static int SendOk(s32 sock, SSDPInfo* info, SSDPEntry* entry)
{
    u32 local;
    SOSockAddr addr;
    s32 len;

    if (State != SSDP_STATE_RUNNING) {
        return -1;
    }

    SSDPCancelTimer(&entry->mTimer);
    entry->mTimer.mCount = 0;

    HTTPCreateResponse(OutputBuffer, 200, "OK", "HTTP/1.1");
    HTTPAddHeader(OutputBuffer, "CACHE-CONTROL", "max-age=1800");
    HTTPAddHeader(OutputBuffer, "EXT", "");
    HTTPAddHeader(OutputBuffer, "LOCATION", LocationString);
    HTTPAddHeader(OutputBuffer, "SERVER", ServerString);
    HTTPAddHeader(OutputBuffer, "ST", info->mName);
    len = HTTPAddHeader(OutputBuffer, "USN", info->mUuidName);

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mOkSent++;
    return SOSendTo(sock, OutputBuffer, len, 0, entry->mAddr);
}

static int DoRequest(char* msg, SOSockAddr* addr, char* method)
{
    char uri[2];
    char host[24];
    char man[24];
    char mxbuf[12];
    char st[128];
    BOOL all;
    s32 i;
    SSDPInfo* info;
    char* p;
    s32 j;
    s32 mx;
    SSDPEntry* entry;

    HTTPGetRequestURI(msg, uri, 2);
    if (strcmp(uri, "*") != 0) {
        return -1;
    }
    HTTPGetHeader(msg, "HOST", host, 24);
    if (strcmp(host, "239.255.255.250:1900") != 0) {
        return -1;
    }

    if (stricmp(method, "M-SEARCH") == 0) {
        if (HTTPGetHeader(msg, "MAN", man, 24) <= 0 || stricmp(man, "\"ssdp:discover\"") != 0) {
            return -1;
        }
        if (HTTPGetHeader(msg, "MX", mxbuf, 12) <= 0) {
            return -1;
        }
        mx = 0;
        for (p = mxbuf; isdigit(*p); p++) {
            mx *= 10;
            mx += *p - '0';
        }
        if (*p != '\0' || mx == 0) {
            return -1;
        }
        if (mx > 10) {
            mx = 10;
        }
        if (HTTPGetHeader(msg, "ST", st, 128) <= 0) {
            return -1;
        }
        Stat.mSearch++;
        all = (stricmp(st, "ssdp:all") == 0) ? TRUE : FALSE;

        for (i = 0; i < NumInfo; i++) {
            info = &InfoTable[i];
            if (all || IsPrefix(st, info->mName)) {
                for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
                    entry = &info->mEntries[j];
                    if (entry->mTimer.mCount == 0) {
                        memmove(entry->mAddr, addr, addr->len);
                        SSDPSetTimer(&entry->mTimer, mx);
                        break;
                    }
                }
                if (j >= SSDP_MAX_ENTRIES) {
                    Stat.mExpire++;
                }
            }
        }
    } else if (stricmp(method, "NOTIFY") == 0) {
        Stat.mNotify++;
        if (Callback != NULL) {
            Callback(msg, addr->addr);
        }
        return 0;
    } else {
        return -1;
    }
    return 0;
}

static int DoResponse(char* msg, SOSockAddr* addr)
{
    SSDPSearchInfo* search;
    SSDPSearchInfo* next;
    s32 status;
    char st[128];
    BOOL match;

    status = HTTPGetStatus(InputBuffer);
    if (status < 0 || status != 200) {
        return -1;
    }
    if (HTTPGetHeader(msg, "ST", st, 128) <= 0) {
        return -1;
    }
    Stat.mResponse++;

    search = SearchList.mHead;
    next = (search == NULL) ? NULL : search->mNext;
    while (search != NULL) {
        if ((stricmp(search->mTarget, "ssdp:all") == 0 ? TRUE : FALSE) || IsPrefix(search->mTarget, st)) {
            search->mCallback(msg, addr->addr, search);
        }

        search = next;
        next = (search == NULL) ? NULL : search->mNext;
    }
    return 0;
}

static int SSDPLoop(s32 sock)
{
    SOSockAddr addr;
    char protocol[16];
    char method[16];
    SOPollFd fd;
    OSTime now;
    OSTime cycle;
    OSTime next;
    OSTime timeout;
    s32 result;
    s32 i;
    s32 j;
    SSDPInfo* info;
    SSDPEntry* entry;
    BOOL found;

    for (i = 0; i < NumInfo; i++) {
        info = &InfoTable[i];
        SSDPSetTimer(&info->mTimer, 10);
    }

    fd.fd = sock;
    fd.events = SO_POLLIN;
    cycle = OSGetTime();

    while (State == SSDP_STATE_RUNNING || State == SSDP_STATE_STOPPING) {
        timeout = cycle - OSGetTime();
        if (0 < timeout) {
            if (OSMillisecondsToTicks(200) < timeout) {
                timeout = timeout % OSMillisecondsToTicks(100) + OSMillisecondsToTicks(100);
            }
            result = SOPoll(&fd, 1, timeout);
        } else {
            result = 0;
        }
        if (result < 0) {
            return result;
        }

        if (result == 0) {
            now = OSGetTime();
            cycle = now + OSSecondsToTicks((OSTime)1800);
            for (i = 0; i < NumInfo; i++) {
                info = &InfoTable[i];
                if (now >= info->mTime) {
                    info->mTime = now + OSSecondsToTicks((OSTime)1800) / 2;
                    SSDPSetTimer(&info->mTimer, 10);
                }
                next = SSDPTickTimer(&info->mTimer, now);
                if (now >= next) {
                    SendNotify(sock, info);
                }
                cycle = (cycle < next) ? cycle : next;
                if (State == SSDP_STATE_RUNNING) {
                    for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
                        entry = &info->mEntries[j];
                        if (entry->mTimer.mCount != 0) {
                            next = SSDPTickTimer(&entry->mTimer, now);
                            if (now >= next) {
                                SendOk(sock, info, entry);
                            }
                            cycle = (cycle < next) ? cycle : next;
                        }
                    }
                }
            }
            if (State == SSDP_STATE_RUNNING) {
                next = ProcessSearchList(sock, now);
                cycle = (cycle < next) ? cycle : next;
            }
        } else {
            memset(&addr, 0, sizeof(addr));
            addr.len = sizeof(addr);
            result = SORecvFrom(sock, InputBuffer, sizeof(InputBuffer), SO_MSG_DONTWAIT, (u8*)&addr);
            if (result == -39) {
                return result;
            }
            if (State != SSDP_STATE_RUNNING || result <= 0 || SSDP_HTTP_MAX < result) {
                continue;
            }
            InputBuffer[result] = '\0';
            if (strstr(InputBuffer, "\r\n\r\n") == NULL) {
                continue;
            }
            if (HTTPGetProtocol(InputBuffer, protocol) <= 0) {
                continue;
            }
            if (strcmp(protocol, "HTTP/1.1") != 0) {
                continue;
            }
            if (HTTPGetMethod(InputBuffer, method) > 0) {
                DoRequest(InputBuffer, &addr, method);
            } else {
                DoResponse(InputBuffer, &addr);
            }
        }

        if (State == SSDP_STATE_STOPPING) {
            found = FALSE;
            for (i = 0; i < NumInfo; i++) {
                if (InfoTable[i].mTimer.mState > 0) {
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }
    }
    ClearSearchList();
    return 0;
}

static void* SSDPMain(void* arg)
{
    u32 one;
    struct { u32 mAddr; u32 mInterface; } mreq;
    u8 zero;
    SOSockAddr addr;
    s32 sock;
    char buf[16];

    (void)arg;

    for (;;) {
        UPnPGetHostAddr(&HostAddr);
        if (HostAddr == 0 || State != SSDP_STATE_RUNNING) {
            OSSleepThread(&Event);
            continue;
        }

        sprintf(LocationString, "http://%s:%d/rootDesc.xml", SOInetNtoP(AF_INET, (u8*)&HostAddr, buf, 16), 1900);

        sock = SOSocket(AF_INET, SO_DGRAM, 0);
        if (sock < 0) {
            continue;
        }

        one = 1;
        if (SOSetSockOpt(sock, 0xFFFF, 4, &one, 4) < 0) {
            goto done;
        }
        mreq.mAddr = SSDP_MULTICAST_ADDR;
        mreq.mInterface = 0;
        if (SOSetSockOpt(sock, 0, 11, &mreq, 8) < 0) {
            goto done;
        }
        zero = 0;
        if (SOSetSockOpt(sock, 0, 9, &zero, 1) < 0) {
            goto done;
        }
        if (SOSetSockOpt(sock, 0, 10, &Ttl, 1) < 0) {
            goto done;
        }

        memset(&addr, 0, sizeof(addr));
        addr.family = AF_INET;
        addr.len = sizeof(addr);
        *(u32*)addr.addr = 0;
        addr.port = 1900;
        if (SOBind(sock, (u8*)&addr) < 0) {
            goto done;
        }
        if (SSDPLoop(sock) < 0) {
            goto done;
        }
        mreq.mAddr = SSDP_MULTICAST_ADDR;
        mreq.mInterface = 0;
        SOSetSockOpt(sock, 0, 12, &mreq, 8);
done:
        SOClose(sock);
        State = SSDP_STATE_INITIALIZED;
    }
}

void SSDPInit(SSDPConfig* config)
{
    if (State != SSDP_STATE_STOPPED) {
        return;
    }

    State = SSDP_STATE_INITIALIZED;
    Callback = config->mCallback;
    {
        u8 ttl = config->mTtl;
        if (ttl == 0) {
            ttl = 4;
        }
        Ttl = ttl;
    }
    SearchList.mTail = NULL;
    SearchList.mHead = NULL;

    {
        DVDDiskID* id = DVDGetCurrentDiskID();
        sprintf(ServerString, "Nintendo-GameCube/1.0 UPnP/1.0 %2.2s-%4.4s/1.0", id->company, id->gameName);
    }
    OSInitThreadQueue(&Event);
    OSCreateThread(config->mThread, SSDPMain, NULL, config->mStack, config->mStackSize, config->mPriority, 0);
    OSResumeThread(config->mThread);
}

void SSDPStart(SSDPInfo* info, s32 numInfo)
{
    s32 j;
    OSTime now;
    SSDPInfo* cur;
    SSDPEntry* entry;
    s32 i;
    BOOL enabled;
    OSTime t1;
    OSTime t2;

    enabled = OSDisableInterrupts();
    if (State == SSDP_STATE_INITIALIZED) {
        InfoTable = info;
        NumInfo = numInfo;
        SearchList.mTail = NULL;
        SearchList.mHead = NULL;
        now = OSGetTime();
        for (i = 0; i < NumInfo; i++) {
            cur = &InfoTable[i];
            cur->mTime = now + OSSecondsToTicks((OSTime)1800) / 2;
            SSDPCancelTimer(&cur->mTimer);
            for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
                entry = &cur->mEntries[j];
                SSDPCancelTimer(&entry->mTimer);
                entry->mTimer.mCount = 0;
            }
        }
        State = SSDP_STATE_RUNNING;
        OSWakeupThread(&Event);
    }
    OSRestoreInterrupts(enabled);
}

void SSDPStop(void)
{
    s32 j;
    BOOL enabled;
    u32 host;
    OSTime t1;
    SSDPEntry* entry;
    SSDPInfo* info;
    s32 i;

    if (State != SSDP_STATE_RUNNING) {
        return;
    }

    enabled = OSDisableInterrupts();
    State = SSDP_STATE_STOPPING;
    UPnPGetHostAddr(&host);
    for (i = 0; i < NumInfo; i++) {
        info = &InfoTable[i];
        for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
            entry = &info->mEntries[j];
            SSDPCancelTimer(&entry->mTimer);
            entry->mTimer.mCount = 0;
        }
        SSDPCancelTimer(&info->mTimer);
        if (host == HostAddr) {
            SSDPSetTimer(&info->mTimer, 10);
        }
    }
    OSRestoreInterrupts(enabled);
}

s32 SSDPGetState(void)
{
    return State;
}

void SSDPSearch(SSDPSearchInfo* info)
{
    BOOL enabled;
    SSDPSearchInfo* tail;

    enabled = OSDisableInterrupts();
    if (State == SSDP_STATE_RUNNING) {
        SSDPSetTimer(&info->mTimer, 10);
        tail = SearchList.mTail;
        if (tail == NULL) {
            SearchList.mHead = info;
        } else {
            tail->mNext = info;
        }
        info->mPrev = tail;
        info->mNext = NULL;
        SearchList.mTail = info;
    }
    OSRestoreInterrupts(enabled);
}

static int SendSearch(s32 sock, SSDPSearchInfo* info)
{
    SOSockAddr addr;
    u32 local;
    s32 len;

    memset(&addr, 0, sizeof(addr));
    addr.family = AF_INET;
    addr.len = sizeof(addr);
    *(u32*)addr.addr = SSDP_MULTICAST_ADDR;
    addr.port = 1900;

    if (State != SSDP_STATE_RUNNING && State != SSDP_STATE_STOPPING) {
        return -1;
    }

    HTTPCreateRequest(OutputBuffer, "M-SEARCH", "*", "HTTP/1.1");
    HTTPAddHeader(OutputBuffer, "HOST", "239.255.255.250:1900");
    HTTPAddHeader(OutputBuffer, "MAN", "\"ssdp:discover\"");
    HTTPAddHeader(OutputBuffer, "MX", "10");
    len = HTTPAddHeader(OutputBuffer, "ST", info->mTarget);
    if (len <= 0) {
        return -1;
    }

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mSearchSent++;
    return SOSendTo(sock, OutputBuffer, len, 0, (u8*)&addr);
}

static inline void RemoveSearch(SSDPSearchInfo* info)
{
    BOOL enabled;
    SSDPSearchInfo* prev;
    SSDPSearchInfo* next;

    enabled = OSDisableInterrupts();
    next = info->mNext;
    prev = info->mPrev;
    if (next == NULL) {
        SearchList.mTail = prev;
    } else {
        next->mPrev = prev;
    }
    if (prev == NULL) {
        SearchList.mHead = next;
    } else {
        prev->mNext = next;
    }
    OSRestoreInterrupts(enabled);
}

static OSTime ProcessSearchList(s32 sock, OSTime now)
{
    SSDPSearchInfo* search;
    SSDPSearchInfo* next;
    OSTime cycle;
    OSTime t;
    BOOL enabled;
    OSTime unused;

    enabled = OSDisableInterrupts();
    cycle = now + OSSecondsToTicks((OSTime)1800);
    search = SearchList.mHead;
    next = (search == NULL) ? NULL : search->mNext;
    while (search != NULL) {
        OSRestoreInterrupts(enabled);
        t = SSDPTickTimer(&search->mTimer, now);
        if (now >= t) {
            SendSearch(sock, search);
        }
        cycle = (cycle < t) ? cycle : t;
        if (now >= search->mTimer.mStart + OSSecondsToTicks(search->mTimer.mCount)) {
            RemoveSearch(search);
            SSDPCancelTimer(&search->mTimer);
            search->mCallback(NULL, NULL, search);
        }
        enabled = OSDisableInterrupts();
        search = next;
        next = (search == NULL) ? NULL : search->mNext;
    }
    OSRestoreInterrupts(enabled);
    return cycle;
}

static inline SSDPSearchInfo* DequeueSearch(void)
{
    SSDPSearchInfo* info;
    SSDPSearchInfo* next;

    info = SearchList.mHead;
    next = info->mNext;
    if (next == NULL) {
        SearchList.mTail = NULL;
    } else {
        next->mPrev = NULL;
    }
    SearchList.mHead = next;
    return info;
}

static void ClearSearchList(void)
{
    SSDPSearchInfo* search;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    while ((search = SearchList.mHead) != NULL) {
        DequeueSearch();
        OSRestoreInterrupts(enabled);
        SSDPCancelTimer(&search->mTimer);
        search->mCallback(NULL, NULL, search);
        enabled = OSDisableInterrupts();
    }
    OSRestoreInterrupts(enabled);
}

const char* SSDPGetServerName(void)
{
    return ServerString;
}

s32 SSDPGetCacheControlMaxAge(char* http)
{
    char value[24];
    char maxAge[24];
    u32 max;
    char* p;

    if (HTTPGetHeader(http, "CACHE-CONTROL", value, 24) <= 0) {
        return -1;
    }
    if (HTTPGetParameter(value, "max-age", maxAge, 24) <= 0) {
        return -1;
    }
    max = 0;
    p = maxAge;
    while (isdigit(*p)) {
        max = max * 10;
        max = *p + max;
        max = max - '0';
        p++;
    }
    if (*p != 0) {
        return -1;
    }
    return max;
}

void SSDPGetStat(SSDPStat* stat)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    memcpy(stat, &Stat, sizeof(SSDPStat));
    OSRestoreInterrupts(enabled);
}

void SSDPClearStat(void)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    memset(&Stat, 0, sizeof(SSDPStat));
    OSRestoreInterrupts(enabled);
}
