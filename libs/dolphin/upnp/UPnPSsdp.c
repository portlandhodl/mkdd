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

static const u32 exp[3] = {0x8CA, 0x128E, 0x1C52};

static char ServerString[0x40];
static char LocationString[0x40];
static char OutputBuffer[0x201];
static char InputBuffer[0x201];
static struct {
    u32 mNotify;
    u32 mResponse;
    u32 mSearch;
    u32 mExpire;
    u32 mNotifySent;
    u32 mOkSent;
    u32 mSearchSent;
} Stat;

static OSThreadQueue Event;
static SSDPInfo* InfoTable;
static s32 NumInfo;
static s32 State;
static struct {
    SSDPSearchInfo* mHead;
    SSDPSearchInfo* mTail;
} SearchList;
static u8 Ttl;
static void (*Callback)(const char*, const void*);
static u32 HostAddr;

static int DoRequest(s32 sock, SOSockAddr* addr, const char* method);
static int DoResponse(s32 sock, SOSockAddr* addr);
static int SendNotify(s32 sock, SSDPInfo* info);
static int DoRequest(s32 sock, SOSockAddr* addr, const char* method);
static int DoResponse(s32 sock, SOSockAddr* addr);
static int SendNotify(s32 sock, SSDPInfo* info);
static int SendOk(s32 sock, SSDPInfo* info, SSDPEntry* entry);
static int SendSearch(s32 sock, SSDPSearchInfo* info);
static int SSDPLoop(s32 sock);
static void* SSDPMain(void* arg);
static void ClearSearchList(void);
static OSTime ProcessSearchList(s32 sock, s32 unused, OSTime time);

static void IsPrefix(void);
static void Rotate(void);
static void Backoff(void);
static void SSDPSetTimer(void);
static void SSDPCancelTimer(void);
static void SSDPTickTimer(void);
static void SSDPGetStat(void);
static void SSDPClearStat(void);

#define SSDP_NEXT_TIME(dst, base, count, state) \
    do { \
        OSTick tick = OSGetTick(); \
        u32 shift = tick & 31; \
        u32 rotate = (tick << shift) | (tick >> (32 - shift)); \
        u32 phase = rotate % (ticksPerMs * 2200); \
        (dst) = (base) + (OSTime)((u32)((count) * (exp[3 - (state)] * ticksPerMs - phase)) / 10); \
    } while (0)

static void IsPrefix(void) { }
static void Rotate(void) { }
static void Backoff(void) { }
static void SSDPSetTimer(void) { }
static void SSDPCancelTimer(void) { }
static void SSDPTickTimer(void) { }

static int SendNotify(s32 sock, SSDPInfo* info)
{
    SOSockAddr addr;
    u32 local;
    s32 len;

    memset(&addr, 0, sizeof(addr));
    addr.len = sizeof(addr);
    addr.family = AF_INET;
    *(u32*)addr.addr = SSDP_MULTICAST_ADDR;
    addr.port = 1900;

    if (State != SSDP_STATE_RUNNING && State != SSDP_STATE_STOPPING) {
        return -1;
    }

    HTTPCreateRequest((void*)(ServerString + 128), "NOTIFY", "*", "HTTP/1.1");
    HTTPAddHeader((void*)(ServerString + 128), "HOST", "239.255.255.250:1900");
    if (State == SSDP_STATE_RUNNING) {
        HTTPAddHeader((void*)(ServerString + 128), "CACHE-CONTROL", "max-age=1800");
        HTTPAddHeader((void*)(ServerString + 128), "LOCATION", ServerString + 64);
        HTTPAddHeader((void*)(ServerString + 128), "NT", info->mName);
        HTTPAddHeader((void*)(ServerString + 128), "NTS", "ssdp:alive");
        HTTPAddHeader((void*)(ServerString + 128), "SERVER", ServerString);
    } else {
        HTTPAddHeader((void*)(ServerString + 128), "NT", info->mName);
        HTTPAddHeader((void*)(ServerString + 128), "NTS", "ssdp:byebye");
    }
    len = HTTPAddHeader((void*)(ServerString + 128), "USN", info->mUuidName);
    if (len <= 0) {
        return -1;
    }

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mNotifySent++;
    return SOSendTo(sock, ServerString + 128, len, 0, (u8*)&addr);
}

static int SendOk(s32 sock, SSDPInfo* info, SSDPEntry* entry)
{
    u32 local;
    s32 len;
    BOOL enabled;

    if (State != SSDP_STATE_RUNNING) {
        return -1;
    }

    enabled = OSDisableInterrupts();
    entry->mState = 0;
    OSRestoreInterrupts(enabled);
    entry->mSendCount = 0;

    HTTPCreateResponse((void*)(ServerString + 128), 200, "OK", "HTTP/1.1");
    HTTPAddHeader((void*)(ServerString + 128), "CACHE-CONTROL", "max-age=1800");
    HTTPAddHeader((void*)(ServerString + 128), "EXT", "");
    HTTPAddHeader((void*)(ServerString + 128), "LOCATION", ServerString + 64);
    HTTPAddHeader((void*)(ServerString + 128), "SERVER", ServerString);
    HTTPAddHeader((void*)(ServerString + 128), "ST", info->mName);
    len = HTTPAddHeader((void*)(ServerString + 128), "USN", info->mUuidName);

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mOkSent++;
    return SOSendTo(sock, ServerString + 128, len, 0, entry->mAddr);
}

static int DoRequest(s32 sock, SOSockAddr* addr, const char* method)
{
    char uri[2];
    char buf[24];
    char man[24];
    char mxbuf[12];
    char st[128];
    char* p;
    u32 mx;
    s32 i;
    s32 j;
    int all;
    u32 bus;
    u32 ticksPerMs;
    u32 slot;
    u32 shift;
    u32 tick;
    u32 rotate;
    u32 phase;
    SSDPInfo* info;
    SSDPEntry* entry;

    HTTPGetRequestURI((void*)sock, uri, 2);
    if (strcmp(uri, "*") != 0) {
        return -1;
    }
    HTTPGetHeader((void*)sock, "HOST", buf, 24);
    if (strcmp(buf, "239.255.255.250:1900") != 0) {
        return -1;
    }

    if (stricmp(method, "M-SEARCH") != 0) {
        if (stricmp(method, "NOTIFY") == 0) {
            Stat.mNotify++;
            if (Callback != NULL) {
                Callback((const char*)sock, addr->addr);
            }
            return 0;
        }
        return -1;
    }

    if (HTTPGetHeader((void*)sock, "MAN", man, 24) <= 0 || stricmp(man, "\"ssdp:discover\"") != 0) {
        return -1;
    }
    if (HTTPGetHeader((void*)sock, "MX", mxbuf, 12) <= 0) {
        return -1;
    }
    mx = 0;
    p = mxbuf;
    while (isdigit(*p)) {
        mx = mx * 10 + (*p - '0');
        p++;
    }
    if (*p != 0 || mx == 0) {
        return -1;
    }
    if (mx > 10) {
        mx = 10;
    }
    if (HTTPGetHeader((void*)sock, "ST", st, 128) <= 0) {
        return -1;
    }
    Stat.mSearch++;
    all = (stricmp(st, "ssdp:all") == 0);

    bus = OS_BUS_CLOCK / 4;
    ticksPerMs = bus / 1000;
    slot = ticksPerMs * 2200;

    for (i = 0; i < NumInfo; i++) {
        info = &InfoTable[i];
        if (!all) {
            const char* a = st;
            const char* b = info->mName;
            int match = 1;
            while (*a != 0) {
                if (*a != *b) {
                    match = 0;
                    break;
                }
                a++;
                b++;
            }
            if (!match) {
                continue;
            }
        }
        for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
            entry = &info->mEntries[j];
            if (entry->mSendCount != 0) {
                continue;
            }
            memmove(entry->mAddr, addr, addr->len);
            entry->mSendCount = mx;
            entry->mState = 3;
            entry->mTime = OSGetTime();
            shift = OSGetTick() & 31;
            tick = OSGetTick();
            rotate = (tick << shift) | (tick >> (32 - shift));
            phase = rotate % slot;
            entry->mNextTime = entry->mTime + (OSTime)((u32)(entry->mSendCount * (exp[3 - entry->mState] * ticksPerMs - phase)) / 10);
            break;
        }
        if (j == SSDP_MAX_ENTRIES) {
            Stat.mExpire++;
        }
    }
    return 0;
}

static int DoResponse(s32 sock, SOSockAddr* addr)
{
    SSDPSearchInfo* search;
    SSDPSearchInfo* next;
    SSDPSearchInfo* tmp;
    const char* a;
    const char* b;
    BOOL match;
    s32 status;
    char st[128];
    u32 dummy;

    status = HTTPGetStatus((void*)InputBuffer);
    if (status < 0 || status != 200) {
        return -1;
    }
    if (HTTPGetHeader((void*)sock, "ST", st, 128) <= 0) {
        return -1;
    }
    Stat.mResponse++;

    dummy = 0;
    (void)dummy;
    search = SearchList.mHead;
    if (search == NULL) {
        tmp = NULL;
    } else {
        tmp = search->mNext;
    }
    next = tmp;
    while (search != NULL) {
        if (stricmp(search->mTarget, "ssdp:all") == 0) {
            match = 1;
        } else {
            match = 0;
        }
        if (!match) {
            a = search->mTarget;
            b = st;
            while (*a != 0) {
                if (*a != *b) {
                    break;
                }
                a++;
                b++;
            }
            if (*a == 0) {
                match = 1;
            }
        }
        if (match) {
            search->mCallback((const char*)sock, addr->addr, search);
        }
        search = next;
        if (next == NULL) {
            tmp = NULL;
        } else {
            tmp = next->mNext;
        }
        next = tmp;
    }
    return 0;
}

static int SSDPLoop(s32 sock)
{
    SOPollFd fd;
    SOSockAddr addr;
    SSDPInfo* info;
    SSDPEntry* entry;
    OSTime now;
    OSTime cycle;
    OSTime next;
    OSTime rem;
    OSTime timeout;
    u32 ticksPerSec;
    u32 ticksPerMs;
    u32 slot;
    u32 count;
    u32 state;
    s32 i;
    s32 j;
    s32 result;
    BOOL enabled;
    char protocol[16];
    char method[16];

    ticksPerSec = OS_BUS_CLOCK / 4;
    ticksPerMs = ticksPerSec / 1000;
    slot = ticksPerMs * 2200;

    for (i = 0; i < NumInfo; i++) {
        info = &InfoTable[i];
        enabled = OSDisableInterrupts();
        info->mSendCount = 10;
        info->mState = 3;
        info->mEndTime = OSGetTime();
        count = info->mSendCount;
        state = info->mState;
        SSDP_NEXT_TIME(info->mNextTime, info->mEndTime, count, state);
        OSRestoreInterrupts(enabled);
    }

    fd.fd = sock;
    fd.events = 0x0001;
    cycle = OSGetTime();

    for (;;) {
        now = OSGetTime();
        if (now < cycle) {
            rem = cycle - now;
            if (rem <= (OSTime)200 * ticksPerMs) {
                timeout = rem;
            } else {
                timeout = rem % ((OSTime)100 * ticksPerMs) + (OSTime)100 * ticksPerMs;
            }
            result = SOPoll(&fd, 1, timeout);
            if (result < 0) {
                return 0;
            }
            if (result != 0) {
                memset(&addr, 0, sizeof(addr));
                addr.len = sizeof(addr);
                result = SORecvFrom(sock, InputBuffer, 513, 4, (u8*)&addr);
                if (result == -39) {
                    return 0;
                }
                if (State == SSDP_STATE_RUNNING && result > 0 && result <= SSDP_HTTP_MAX) {
                    InputBuffer[result] = 0;
                    if (strstr(InputBuffer, "\r\n\r\n") != NULL) {
                        if (HTTPGetProtocol((void*)InputBuffer, protocol) > 0) {
                            if (strcmp(protocol, "HTTP/1.1") == 0) {
                                if (HTTPGetMethod((void*)InputBuffer, method) > 0) {
                                    DoRequest(sock, &addr, method);
                                } else {
                                    DoResponse(sock, &addr);
                                }
                            }
                        }
                    }
                }
                if (State != SSDP_STATE_RUNNING) {
                    if (State == SSDP_STATE_STOPPING) {
                        continue;
                    }
                    ClearSearchList();
                    return 0;
                }
                continue;
            }
        }

        now = OSGetTime();
        cycle = now + (OSTime)ticksPerSec * 1800;

        for (i = 0; i < NumInfo; i++) {
            info = &InfoTable[i];
            if (now >= info->mTime) {
                info->mTime = now + (OSTime)ticksPerSec * 1800 / 2;
                info->mSendCount = 10;
                info->mState = 3;
                info->mEndTime = OSGetTime();
                count = info->mSendCount;
                state = info->mState;
                SSDP_NEXT_TIME(info->mNextTime, info->mEndTime, count, state);
            }

            enabled = OSDisableInterrupts();
            if (info->mSendCount > 0 && info->mState > 0) {
                info->mSendCount--;
                if (info->mSendCount > 0) {
                    count = info->mSendCount;
                    state = info->mState;
                    SSDP_NEXT_TIME(info->mNextTime, info->mEndTime, count, state);
                }
            } else {
                next = now + (OSTime)ticksPerSec * 1800;
                info->mNextTime = next;
            }
            OSRestoreInterrupts(enabled);

            if (now >= info->mNextTime) {
                if (State == SSDP_STATE_RUNNING) {
                    SendNotify(sock, info);
                }
            }

            next = info->mNextTime;
            if (next < cycle) {
            } else {
                cycle = next;
            }

            if (State == SSDP_STATE_RUNNING) {
                for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
                    entry = &info->mEntries[j];
                    if (entry->mSendCount == 0) {
                        continue;
                    }

                    enabled = OSDisableInterrupts();
                    if (entry->mSendCount > 0 && entry->mState > 0) {
                        entry->mState--;
                        if (entry->mState > 0) {
                            count = entry->mSendCount;
                            state = entry->mState;
                            SSDP_NEXT_TIME(entry->mNextTime, entry->mTime, count, state);
                        }
                    } else {
                        next = now + (OSTime)ticksPerSec * 1800;
                        entry->mNextTime = next;
                    }
                    OSRestoreInterrupts(enabled);

                    if (now >= entry->mNextTime) {
                        SendOk(sock, info, entry);
                    }

                    next = entry->mNextTime;
                    if (next < cycle) {
                    } else {
                        cycle = next;
                    }
                }
            }
        }

        if (State == SSDP_STATE_RUNNING) {
            next = ProcessSearchList(sock, 0, now);
            if (next < cycle) {
            } else {
                cycle = next;
            }
        }

        if (State == SSDP_STATE_STOPPING) {
            s32 found = 0;
            for (i = 0; i < NumInfo; i++) {
                if (InfoTable[i].mState > 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                ClearSearchList();
                return 0;
            }
        }

        if (State != SSDP_STATE_RUNNING && State != SSDP_STATE_STOPPING) {
            ClearSearchList();
            return 0;
        }
    }
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
    BOOL enabled;
    OSTime now;
    SSDPInfo* cur;
    SSDPEntry* entry;
    s32 i;
    s32 j;
    u32 ticksPerSec;

    enabled = OSDisableInterrupts();
    if (State == SSDP_STATE_INITIALIZED) {
        InfoTable = info;
        NumInfo = numInfo;
        SearchList.mTail = NULL;
        SearchList.mHead = NULL;

        now = OSGetTime();
        ticksPerSec = OS_BUS_CLOCK / 4;

        for (i = 0; i < NumInfo; i++) {
            InfoTable[i].mTime = now + (OSTime)ticksPerSec * 1800 / 2;
            cur = &InfoTable[i];
            OSDisableInterrupts();
            cur->mState = 0;
            OSRestoreInterrupts(enabled);
            for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
                entry = &cur->mEntries[j];
                OSDisableInterrupts();
                entry->mState = 0;
                OSRestoreInterrupts(enabled);
                entry->mSendCount = 0;
            }
        }
        State = SSDP_STATE_RUNNING;
        OSWakeupThread(&Event);
    }
    OSRestoreInterrupts(enabled);
}

void SSDPStop(void)
{
    SSDPInfo* info;
    SSDPEntry* entry;
    BOOL enabled;
    u32 host;
    u32 ticksPerMs;
    u32 slot;
    u32 count;
    u32 state;
    s32 i;
    s32 j;

    if (State != SSDP_STATE_RUNNING) {
        return;
    }

    enabled = OSDisableInterrupts();
    State = SSDP_STATE_STOPPING;
    UPnPGetHostAddr(&host);

    ticksPerMs = (OS_BUS_CLOCK / 4) / 1000;
    slot = ticksPerMs * 2200;

    for (i = 0; i < NumInfo; i++) {
        info = &InfoTable[i];
        for (j = 0; j < SSDP_MAX_ENTRIES; j++) {
            entry = &info->mEntries[j];
            OSDisableInterrupts();
            entry->mState = 0;
            OSRestoreInterrupts(enabled);
            entry->mSendCount = 0;
        }
        OSDisableInterrupts();
        info->mState = 0;
        OSRestoreInterrupts(enabled);

        if (host == HostAddr) {
            OSDisableInterrupts();
            info->mSendCount = 10;
            info->mState = 3;
            info->mEndTime = OSGetTime();
            count = info->mSendCount;
            state = info->mState;
            SSDP_NEXT_TIME(info->mNextTime, info->mEndTime, count, state);
            OSRestoreInterrupts(enabled);
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
    BOOL level;
    SSDPSearchInfo* tail;
    u32 ticksPerMs;
    u32 slot;
    u32 count;
    u32 state;
    u32 shift;
    u32 tick;
    u32 rotate;
    u32 phase;

    enabled = OSDisableInterrupts();
    if (State == SSDP_STATE_RUNNING) {
        level = OSDisableInterrupts();
        info->mSendCount = 10;
        info->mState = 3;
        info->mTime = OSGetTime();
        count = info->mSendCount;
        state = info->mState;
        tick = OSGetTick();
        shift = tick & 31;
        ticksPerMs = (OS_BUS_CLOCK / 4) / 1000;
        slot = ticksPerMs * 2200;
        rotate = (tick << shift) | (tick >> (32 - shift));
        phase = rotate % slot;
        info->mNextTime = info->mTime + (OSTime)((u32)(count * (exp[3 - state] * ticksPerMs - phase)) / 10);
        OSRestoreInterrupts(level);

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

    HTTPCreateRequest((void*)(ServerString + 128), "M-SEARCH", "*", "HTTP/1.1");
    HTTPAddHeader((void*)(ServerString + 128), "HOST", "239.255.255.250:1900");
    HTTPAddHeader((void*)(ServerString + 128), "MAN", "\"ssdp:discover\"");
    HTTPAddHeader((void*)(ServerString + 128), "MX", "10");
    len = HTTPAddHeader((void*)(ServerString + 128), "ST", info->mTarget);
    if (len <= 0) {
        return -1;
    }

    UPnPGetHostAddr(&local);
    if (local != HostAddr) {
        return -1;
    }

    Stat.mSearchSent++;
    return SOSendTo(sock, ServerString + 128, len, 0, (u8*)&addr);
}

static OSTime ProcessSearchList(s32 sock, s32 unused, OSTime time)
{
    SSDPSearchInfo* search;
    SSDPSearchInfo* next;
    SSDPSearchInfo* tmp;
    OSTime end;
    OSTime now;
    OSTime target;
    BOOL enabled;
    u32 ticksPerSec;
    u32 ticksPerMs;
    u32 slot;
    u32 count;
    u32 state;
    u32 shift;
    u32 tick;
    u32 rotate;
    u32 phase;

    (void)unused;

    enabled = OSDisableInterrupts();
    ticksPerSec = OS_BUS_CLOCK / 4;
    now = time;
    target = now + (OSTime)ticksPerSec * 1800;
    search = SearchList.mHead;
    next = search != NULL ? search->mNext : NULL;
    ticksPerMs = ticksPerSec / 1000;
    slot = ticksPerMs * 2200;

    while (search != NULL) {
        OSRestoreInterrupts(enabled);
        enabled = OSDisableInterrupts();

        if (search->mSendCount > 0 && search->mState > 0) {
            if (now >= search->mNextTime) {
                search->mState--;
                if (search->mState > 0) {
                    count = search->mSendCount;
                    state = search->mState;
                    shift = OSGetTick() & 31;
                    tick = OSGetTick();
                    rotate = (tick << shift) | (tick >> (32 - shift));
                    phase = rotate % slot;
                    search->mNextTime = search->mTime + (OSTime)((u32)((count * (exp[3 - state] * ticksPerMs - phase)) / 10));
                }
            }
        } else {
            search->mNextTime = now + (OSTime)ticksPerSec * 1800;
        }
        OSRestoreInterrupts(enabled);

        if (now >= search->mNextTime) {
            SendSearch(sock, search);
        }

        if (search->mNextTime < target) {
            target = search->mNextTime;
        }

        end = search->mTime + (OSTime)search->mSendCount * ticksPerSec;
        if (now >= end) {
            OSDisableInterrupts();
            if (search->mPrev == NULL) {
                SearchList.mHead = search->mNext;
            } else {
                search->mPrev->mNext = search->mNext;
            }
            if (search->mNext == NULL) {
                SearchList.mTail = search->mPrev;
            } else {
                search->mNext->mPrev = search->mPrev;
            }
            OSRestoreInterrupts(enabled);
            OSDisableInterrupts();
            search->mState = 0;
            OSRestoreInterrupts(enabled);
            search->mCallback(NULL, NULL, search);
        }
        OSDisableInterrupts();
        search = next;
        if (next == NULL) {
            tmp = NULL;
        } else {
            tmp = next->mNext;
        }
        next = tmp;
    }
    OSRestoreInterrupts(enabled);

    return target;
}

static void ClearSearchList(void)
{
    SSDPSearchInfo* search;
    SSDPSearchInfo* next;
    SSDPSearchInfo* cur;
    BOOL enabled;
    s32 dummy;

    dummy = 0;
    (void)dummy;
    enabled = OSDisableInterrupts();
    for (search = SearchList.mHead; search != NULL; search = SearchList.mHead) {
        next = search->mNext;
        cur = search;
        if (next == NULL) {
            SearchList.mTail = NULL;
        } else {
            next->mPrev = NULL;
        }
        SearchList.mHead = next;
        OSRestoreInterrupts(enabled);
        enabled = OSDisableInterrupts();
        cur->mState = 0;
        OSRestoreInterrupts(enabled);
        cur->mCallback(NULL, NULL, cur);
        enabled = OSDisableInterrupts();
    }
    OSRestoreInterrupts(enabled);
}

const char* SSDPGetServerName(void)
{
    return ServerString;
}

s32 SSDPGetCacheControlMaxAge(void* http)
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

static void SSDPGetStat(void) { }
static void SSDPClearStat(void) { }
