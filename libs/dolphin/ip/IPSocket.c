#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SO_EPROTONOSUPPORT -69
#define SO_EOPNOTSUPP -68
#define SO_EBADR -63
#define SO_EMFILE -56
#define SO_EBADMSG -51
#define SO_EPROTOTYPE -49
#define SO_ENOMSG -42
#define SO_ELOOP -40
#define SO_ENOTINIT -39
#define SO_ENAMETOOLONG -38
#define SO_EOVERFLOW -76
#define SO_EFBIG -27
#define SO_ENOBUFS -33
#define SO_EAGAIN -35
#define SO_EEXIST -17
#define SO_EFAULT -15
#define SO_EACCES -13
#define SO_ENOMEM -12
#define SO_EINVAL -28
#define SO_EWOULDBLOCK -6
#define SO_EAFNOSUPPORT -5
#define SO_EINTR -4
#define SO_EBADF -8
#define SO_ENOENT -2
#define SO_EPERM -1
#define SO_ESRCH -3

#define SO_FNONBLOCK 0x04

#define SO_STREAM 1
#define SO_DGRAM 2

#define SO_F_GETFL 3
#define SO_F_SETFL 4

#define SO_SOL_SOCKET 0xFFFF
#define SO_SO_SNDBUF 4097
#define SO_SO_RCVBUF 4098
#define SO_SO_LINGER 128

#define AF_INET 2

typedef void* (*SOAllocFn)(s32 type, u32 size);
typedef void (*SOFreeFn)(s32 type, void* ptr, u32 size);

typedef struct SOSock SOsock;
typedef struct SOTcpSocket SOTcpSocket;
typedef struct SOUdpSocket SOUdpSocket;

struct SOSock {
    u8 proto;
    u8 flag;
    s16 ref;
    void* socket;

    OSMutex recvMutex;
    OSMutex sendMutex;
};

struct SOUdpSocket {
    u8 unk00[0xD4];
    void* unkD4;
    u32 unkD8;
    u8 unkDC[8];
    void* unkE4;
    u32 unkE8;
};

struct SOTcpSocket {
    TCPSocket tcp;

    OSAlarm alarm;
    TCPSocket* listener;
    void* head;
    void* tail;
    void* pendingTail;
    void* pendingHead;
    void* next;
    void* prev;
    s32 count;
    SOsock* handle;

    u8 unk32C[4];
};

typedef struct SOTcpLinks {
    u8 pad[0x18];
    struct SOTcpLinks* next;
    struct SOTcpLinks* prev;
} SOTcpLinks;

typedef struct SOResolver {
    u8 pad00[0x130];
    u32 flag;
    u8 pad134[0x560 - 0x134];
    void* unk560;
    void* unk564;
    u16 unk568;
    u16 unk56A;
    void* unk56C;
    u8 pad570[0x790 - 0x570];
    u8 dns1[4];
    u8 dns2[4];
} SOResolver;

typedef struct SOConfig {
    u16 family;
    u16 maxSockets;
    SOAllocFn alloc;
    SOFreeFn free;
    u32 flag;
    u8 addr[4];
    u8 netmask[4];
    u8 gateway[4];
    u8 dns1[4];
    u8 dns2[4];
    s32 timeWaitBufSize;
    s32 reassemblyBufSize;
    s32 mtu;
    s32 rwin;
    s32 unk34;
    s64 r2;
    void* lcp;
    void* ipcp;
    u8 unk48[8];
    void* dhcp;
} SOConfig;

typedef struct SOPollFd {
    s32 fd;
    s16 events;
    s16 revents;
} SOPollFd;

typedef struct SOLingerQueue {
    void* head;
    void* tail;
} SOLingerQueue;

static BOOL OnReset(BOOL final);
static OSResetFunctionInfo ResetFunctionInfo = {OnReset, OS_RESET_PRIO_SO};

extern u32 IPAddrAny;
extern u32 IPLimited;
extern u8 __IFDefault[];
extern u8 PPPLcpConf[];
extern u8 PPPIpcpConf[];
extern OSThreadQueue TCPInfoQueue;

extern char* IPNtoA(const u8* addr);
extern void DNSClose(void* dns);
extern void DNSOpen(void* dns, u8* addr);
extern s32 DHCPGetStatus(void* status);
extern s32 DHCPStartupEx(void (*handler)(s32), void* arg);
extern void DHCPAuto(s32);
extern void DHCPCleanup(void);
extern void IPSetMtu(s32 type, s32 mtu);
extern s32 IPGetMtu(s32 type, s32* mtu);
extern void IPSetReassemblyBuffer(u8* buffer, u32 size, u32 mtu);
extern void IPClearConfigError(s32);
extern void IPInitRoute(u8* addr, u8* netmask, u8* gateway);
extern void IPSetBroadcastAddr(void* interface, u32* addr);
extern void IPAutoConfig(s32);
extern void IPAutoStop(s32);
extern void ARPRefresh(s32);
extern BOOL IFInit(s32);
extern BOOL IFMute(BOOL);
extern s32 PPPInit(void* ifp, void* lcp, void* ipcp, void* lcpConf, void* ipcpConf);
extern void PPPClose(void);
extern s32 PPPGetState(void* lcp);
extern void TCPSetTimeWaitBuffer(u8* buffer, u32 size);
extern s32 TCPCancel(TCPSocket* so);

extern s32 TCPOpen(TCPSocket* so, u8* recv, u32 recvSize, u8* send, u32 sendSize);
extern void TCPSetTimeout(TCPSocket* so, s64 rxt);
extern s32 TCPListen(TCPSocket* so, u8* addr, u16 localPort, s32 backlog, u32 flag);
extern s32 TCPGetStatus(TCPSocket* so);
extern s32 TCPGetRemoteSocket(TCPSocket* so, u8* addr);
extern s32 TCPBind(TCPSocket* so, u8* addr);
extern s32 TCPShutdown(TCPSocket* so, s32 how);
extern s32 TCPReceiveEx(TCPSocket* so, void* buf, s32 len, s32 flags);
extern s32 TCPReceiveUrgEx(TCPSocket* so, void* buf, s32 len, s32 flags);
extern s32 TCPSend(TCPSocket* so, const void* buf, s32 len);
extern s32 TCPSendUrg(TCPSocket* so, const void* buf, s32 len);
extern s32 TCPSendNonblock(TCPSocket* so, const void* buf, s32 len);
extern s32 TCPSendUrgNonblock(TCPSocket* so, const void* buf, s32 len);
extern s32 TCPGetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, s32* len);
extern s32 TCPSetSockOpt(TCPSocket* so, s32 level, s32 opt, void* val, s32 len);
extern s32 TCPSetSendBuff(TCPSocket* so, u8* buffer, u32 size);
extern s32 TCPGetSendBuff(TCPSocket* so, u8** buffer, u32* size);
extern s32 TCPSetRecvBuff(TCPSocket* so, u8* buffer, u32 size);
extern s32 TCPGetRecvBuff(TCPSocket* so, u8** buffer, u32* size);
extern s32 TCPClose(TCPSocket* so);
extern s32 TCPCloseAsync(TCPSocket* so, void (*callback)(TCPSocket*), s32 arg);
extern void TCPAcceptAsync(TCPSocket* so, TCPSocket* listener,
                           void (*callback)(TCPSocket*, s32), s32 arg);

extern s32 UDPOpen(SOUdpSocket* so, u8* recv, u32 recvSize);
extern s32 UDPSetSendBuff(SOUdpSocket* so, u8* buffer, u32 size);
extern void UDPGetSendBuff(SOUdpSocket* so, u8** buffer, u32* size);
extern s32 UDPSetRecvBuff(SOUdpSocket* so, u8* buffer, u32 size);
extern void UDPGetRecvBuff(SOUdpSocket* so, u8** buffer, u32* size);
extern s32 UDPBind(SOUdpSocket* so, u8* addr);
extern s32 UDPSend(SOUdpSocket* so, const void* buf, s32 len, u8* addr);
extern s32 UDPReceiveEx(SOUdpSocket* so, void* buf, s32 len, s32 unused,
                        u8* addr, s32 flags);
extern s32 UDPClose(SOUdpSocket* so);
extern s32 UDPSetSockOpt(SOUdpSocket* so, s32 level, s32 opt, void* val, s32 len);
extern s32 __UDPPoll(SOUdpSocket* so);
extern s32 __TCPPoll(TCPSocket* so);

s32 SORecvFrom(s32 s, void* buf, s32 len, u32 flags, u8* addr);
s32 SOSendTo(s32 s, const void* buf, s32 len, s32 flags, u8* addr);
u32 SOGetHostID(void);
void __IPWakeupPollingThreads(void);
s32 SOGetResolver(u8* dns1, u8* dns2);
static SOAllocFn Alloc;
static SOFreeFn Free;
static s32 Allocated;
static SOLingerQueue LingerQueue;
static u8* TimeWaitBuf;
static s32 TimeWaitBufSize;
static u8* ReassemblyBuffer;
static s32 ReassemblyBufferSize;
static s32 State;
static u32 Flag;
static s32 Mtu;
static s32 Rwin;
static s64 R2;
static OSThreadQueue CleaningQueue;
static OSThreadQueue PollingQueue;
static s32 LowInitialized;
static s32 Initialized;

static SOsock SocketTable[256] __attribute__((aligned(16)));
static SOResolver Resolver __attribute__((aligned(16)));

static void PutNode(SOsock* node);
static s32 __SOClose(s32 s);
static s32 __SOSetSockOpt(s32 s, s32 level, s32 opt, void* val, u32 len);
static void LcpHandler(void* conf);
static void DhcpHandler(s32 event);
static void LingerCallback(OSAlarm* alarm, OSContext* context);
static void LingerTimeout(OSAlarm* alarm, OSContext* context);
static void AcceptCallback(TCPSocket* so, s32 result);
static SOTcpSocket* AddBackLog(TCPSocket* so);
static void PollTimeout(OSAlarm* alarm, OSContext* context);

static inline void* SOAlloc(s32 type, u32 size)
{
    void* ptr = Alloc(type, size);
    BOOL enabled;

    if (ptr != NULL) {
        enabled = OSDisableInterrupts();
        Allocated += size;
        OSRestoreInterrupts(enabled);
    }
    return ptr;
}

static inline void SOFree(s32 type, void* ptr, u32 size)
{
    BOOL enabled;

    Free(type, ptr, size);
    enabled = OSDisableInterrupts();
    Allocated -= size;
    if (Allocated == 0 && State == 2) {
        OSWakeupThread(&CleaningQueue);
    }
    OSRestoreInterrupts(enabled);
}

char* SOInetNtoA(const u8* addr)
{
    return (char*)IPNtoA(addr);
}

char* SOInetNtoP(s32 af, const u8* addr, char* buf, u32 len)
{
    if (af == AF_INET && buf != NULL && len >= 16) {
        sprintf(buf, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        return buf;
    }
    return NULL;
}

static SOsock* GetNode(s32 s, void** socket)
{
    SOsock* node;
    SOsock* next;
    SOsock* prev;
    SOsock* list;
    SOsock* tail;
    SOsock* cur;
    SOTcpSocket* so;
    SOsock* ret;
    void* p;
    BOOL enabled;

    tail = NULL;
    list = NULL;

    enabled = OSDisableInterrupts();
    node = (SOsock*)LingerQueue.head;
    next = node != NULL ? (SOsock*)node->recvMutex.link.next : NULL;
    while (node != NULL) {
        so = (SOTcpSocket*)node;
        if (so->handle != NULL && so->handle->ref != 0) {
            goto next;
        }
        prev = (SOsock*)node->recvMutex.link.prev;
        {
            SOsock* after = next;
            if (after != NULL) {
                after->recvMutex.link.prev = (OSMutex*)prev;
            } else {
                LingerQueue.tail = prev;
            }
        }
        if (prev == NULL) {
            LingerQueue.head = next;
        } else {
            prev->recvMutex.link.next = (OSMutex*)next;
        }
        if (tail == NULL) {
            list = node;
        } else {
            tail->recvMutex.link.next = (OSMutex*)node;
        }
        node->recvMutex.link.prev = (OSMutex*)tail;
        tail = node;
        node->recvMutex.link.next = NULL;
    next:
        node = next;
        next = next != NULL ? (SOsock*)next->recvMutex.link.next : NULL;
    }
    OSRestoreInterrupts(enabled);

    while (list != NULL) {
        cur = list;
        next = (SOsock*)cur->recvMutex.link.next;
        so = (SOTcpSocket*)cur;
        SOFree(2, (void*)so->tcp.unk1F8, so->tcp.unk1FC);
        SOFree(1, (void*)so->tcp.unk174, so->tcp.unk178);
        SOFree(0, so, 0x330);
        list = next;
    }

    ret = NULL;
    enabled = OSDisableInterrupts();
    if (0 <= s && s < 256) {
        node = &SocketTable[s];
        if (node->ref <= 0 || node->socket == NULL) {
            ret = NULL;
        } else {
            node->ref++;
            ret = node;
            if (socket != NULL) {
                p = node->socket;
                *socket = p;
            }
        }
    }
    OSRestoreInterrupts(enabled);
    return ret;
}


static void PutNode(SOsock* sock)
{
    SOsock* node;
    u8 proto;
    void* so;
    s16 ref;
    BOOL enabled;

    node = sock;
    proto = 0;
    so = NULL;
    enabled = OSDisableInterrupts();
    ref = node->ref - 1;
    node->ref = ref;
    if (ref == 0 && node->socket != NULL) {
        so = node->socket;
        node->socket = NULL;
        proto = node->proto;
        node->proto = 0;
    }
    OSRestoreInterrupts(enabled);

    if (so != NULL) {
        switch (proto) {
        case IP_PROTO_UDP: {
            SOUdpSocket* udp = (SOUdpSocket*)so;
            SOFree(5, udp->unkD4, udp->unkD8);
            SOFree(4, udp->unkE4, udp->unkE8);
            SOFree(3, udp, 0xF0);
            break;
        }
        case IP_PROTO_TCP: {
            SOTcpSocket* tcp = (SOTcpSocket*)so;
            SOFree(2, (void*)tcp->tcp.unk1F8, tcp->tcp.unk1FC);
            SOFree(1, (void*)tcp->tcp.unk174, tcp->tcp.unk178);
            SOFree(0, tcp, 0x330);
            break;
        }
        default:
            OSPanic("IPSocket.c", 540, "PutNode: unknown proto");
            break;
        }
    }
}

s32 SOSetResolver(const u8* dns1, const u8* dns2)
{
    if (State != 1) {
        return SO_ENOTINIT;
    }
    memmove(&Resolver.dns1, dns1 != NULL ? dns1 : (const u8*)&IPAddrAny, 4);
    memmove(&Resolver.dns2, dns2 != NULL ? dns2 : (const u8*)&IPAddrAny, 4);
    DNSClose(&Resolver);
    if (*(u32*)&Resolver.dns1 != IPAddrAny) {
        DNSOpen(&Resolver, Resolver.dns1);
    } else if (*(u32*)&Resolver.dns2 != IPAddrAny) {
        DNSOpen(&Resolver, Resolver.dns2);
    }
    Resolver.flag |= 4;
    return 0;
}

s32 SOGetResolver(u8* dns1, u8* dns2)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    if (State != 1) {
        result = SO_ENOTINIT;
    } else {
        if (dns1 != NULL) {
            memcpy(dns1, &Resolver.dns1, 4);
        }
        if (dns2 != NULL) {
            memcpy(dns2, &Resolver.dns2, 4);
        }
        result = 0;
    }
    OSRestoreInterrupts(enabled);
    return result;
}

static void LcpHandler(void* conf)
{
    if (*(s32*)((u8*)conf + 4) == 0 && State == 2) {
        OSWakeupThread(&CleaningQueue);
    }
}

static void DhcpHandler(s32 event)
{
    u8 status[296];
    u8 dns1[4];
    u8 dns2[4];
    volatile BOOL enabled;
    s32 result;

    switch (event) {
    case 3:
        DHCPGetStatus(status);
        enabled = OSDisableInterrupts();
        if (State != 1) {
            result = SO_ENOTINIT;
        } else {
            memcpy(dns1, &Resolver.dns1, 4);
            memcpy(dns2, &Resolver.dns2, 4);
            result = 0;
        }
        OSRestoreInterrupts(enabled);
        break;
    case 0:
        IPSetMtu(0, Mtu);
        if (State == 2) {
            OSWakeupThread(&CleaningQueue);
        }
        return;
    default:
        return;
    }

    if (result != 0) {
        return;
    }
    if (*(u32*)dns1 == IPAddrAny && *(u32*)dns2 == IPAddrAny) {
        if (State != 1) {
            return;
        }
        memmove(&Resolver.dns1, status + 12 != NULL ? status + 12 : (u8*)&IPAddrAny, 4);
        memmove(&Resolver.dns2, status + 16 != NULL ? status + 16 : (u8*)&IPAddrAny, 4);
    } else {
        if (State != 1) {
            return;
        }
        memmove(&Resolver.dns1, dns1 != NULL ? dns1 : (u8*)&IPAddrAny, 4);
        memmove(&Resolver.dns2, dns2 != NULL ? dns2 : (u8*)&IPAddrAny, 4);
    }
    DNSClose(&Resolver);
    if (*(u32*)&Resolver.dns1 != IPAddrAny) {
        DNSOpen(&Resolver, Resolver.dns1);
    } else if (*(u32*)&Resolver.dns2 != IPAddrAny) {
        DNSOpen(&Resolver, Resolver.dns2);
    }
    Resolver.flag |= 4;
}

void SOInit(void)
{
    if (!Initialized) {
        Initialized = 1;
        OSRegisterResetFunction(&ResetFunctionInfo);
    }
    IFInit(4);
    if (State == 0) {
        if (SOGetHostID() != 0 || DHCPGetStatus(NULL) != 0) {
            LowInitialized = 1;
        } else {
            IFMute(TRUE);
        }
    }
}

s32 SOStartup(SOConfig* config)
{
    s32 mtu;
    s32 rwin;
    s64 r2;
    u8* buf;
    void* handler;
    s32 result;

    if (config->family != 0 || config->maxSockets != 256) {
        return SO_EINVAL;
    }
    if (!IFInit(4)) {
        return SO_EINVAL;
    }
    if (State != 0) {
        return SO_EINVAL;
    }

    mtu = config->mtu;
    if (mtu > 0) {
        if (mtu > 1500) {
            mtu = 1500;
        }
        if (mtu < 68) {
            mtu = 68;
        }
    } else {
        mtu = 1500;
    }
    Mtu = mtu;
    IPSetMtu(0, mtu);

    rwin = config->rwin;
    if (rwin > 0) {
        if (rwin < 28) {
            rwin = 28;
        }
        Rwin = rwin;
    } else {
        Rwin = 0;
    }

    r2 = config->r2;
    if (r2 <= 0) {
        R2 = (s64)(OS_BUS_CLOCK / 4) * 100;
    } else {
        R2 = r2;
    }

    OSInitThreadQueue(&CleaningQueue);
    OSInitThreadQueue(&PollingQueue);
    Alloc = config->alloc;
    Free = config->free;
    Flag = config->flag;

    if (!LowInitialized) {
        if (config->timeWaitBufSize != 0) {
            TimeWaitBufSize = config->timeWaitBufSize;
            TimeWaitBuf = SOAlloc(6, config->timeWaitBufSize);
            TCPSetTimeWaitBuffer(TimeWaitBuf, TimeWaitBufSize);
        }
        if (config->reassemblyBufSize != 0) {
            ReassemblyBufferSize = config->reassemblyBufSize;
            ReassemblyBuffer = SOAlloc(7, config->reassemblyBufSize);
            IPSetReassemblyBuffer(ReassemblyBuffer, ReassemblyBufferSize, mtu);
        }
        IPClearConfigError(0);
    }

    if (!LowInitialized) {
        if (Flag & 2) {
            Flag &= ~2;
            if (PPPInit(__IFDefault, PPPLcpConf, PPPIpcpConf, config->lcp,
                        config->ipcp)) {
                *(void**)(PPPLcpConf + 0x84) = LcpHandler;
            } else {
                goto fail;
            }
        } else if (Flag & 1) {
            if (DHCPStartupEx(DhcpHandler, config->dhcp) == 0) {
                LowInitialized = 1;
            }
            DHCPAuto(0);
        } else if (config->addr != NULL) {
            if (SOGetHostID() == 0) {
                IPInitRoute(config->addr, config->netmask, config->gateway);
            } else {
                LowInitialized = 1;
            }
        }
    }

    if (!LowInitialized) {
        ARPRefresh(0);
    }
    if (Flag & 0x10000) {
        IPAutoConfig(0);
    }

    LingerQueue.head = NULL;
    LingerQueue.tail = NULL;
    memset(&Resolver, 0, sizeof(Resolver));
    *(u32*)((u8*)&Resolver + 0x670) = 0;
    *(void**)((u8*)&Resolver + 0x560) = (u8*)&Resolver + 0x570;
    *(void**)((u8*)&Resolver + 0x564) = (u8*)&Resolver + 0x670;
    *(u16*)((u8*)&Resolver + 0x568) = 2;
    *(u16*)((u8*)&Resolver + 0x56A) = 4;
    *(void**)((u8*)&Resolver + 0x56C) = (u8*)&Resolver + 0x700;

    if (State == 1) {
        memmove(&Resolver.dns1, config->dns1 != NULL ? config->dns1
                                                     : (const u8*)&IPAddrAny,
                4);
        memmove(&Resolver.dns2, config->dns2 != NULL ? config->dns2
                                                     : (const u8*)&IPAddrAny,
                4);
        DNSClose(&Resolver);
        if (*(u32*)&Resolver.dns1 != IPAddrAny) {
            DNSOpen(&Resolver, Resolver.dns1);
        } else if (*(u32*)&Resolver.dns2 != IPAddrAny) {
            DNSOpen(&Resolver, Resolver.dns2);
        }
        Resolver.flag |= 4;
    }
    State = 1;
    return 0;

fail:
    if (TimeWaitBuf != NULL && TimeWaitBufSize != 0) {
        SOFree(6, TimeWaitBuf, TimeWaitBufSize);
    }
    if (ReassemblyBuffer != NULL && ReassemblyBufferSize != 0) {
        SOFree(7, ReassemblyBuffer, ReassemblyBufferSize);
    }
    return SO_EINVAL;
}

s32 SOCleanup(void)
{
    SOsock* node;
    SOTcpSocket* so;
    s32 s;
    s32 ret;
    u8* ptr;
    u32 size;
    u32 linger[2];

    if (State != 1) {
        return SO_ENOTINIT;
    }
    State = 2;
    __IPWakeupPollingThreads();

    for (s = 0; s < 256; s++) {
        node = &SocketTable[s];
        if (node->ref == 0) {
            continue;
        }
        switch (node->proto) {
        case IP_PROTO_UDP:
            __SOClose(s);
            break;
        case IP_PROTO_TCP:
            linger[0] = 1;
            linger[1] = 0;
            __SOSetSockOpt(s, -1, SO_SO_LINGER, linger, 8);
            __SOClose(s);
            break;
        }
    }

    so = TCPInfoQueue.head ? (SOTcpSocket*)TCPInfoQueue.head->link.next : NULL;
    while (so != NULL) {
        SOTcpSocket* next = so->next ? (SOTcpSocket*)so->next : NULL;
        if (so->tcp.outputCallback == (void*)LingerCallback) {
            TCPCancel(&so->tcp);
        }
        so = next;
    }

    GetNode(-1, NULL);

    if (Flag & 0x10000) {
        IPAutoStop(0);
    }
    DNSClose(&Resolver);

    if (!LowInitialized) {
        if (Flag & 2) {
            BOOL enabled;
            PPPClose();
            enabled = OSDisableInterrupts();
            while (PPPGetState(PPPLcpConf) != 0) {
                OSSleepThread(&CleaningQueue);
            }
            OSRestoreInterrupts(enabled);
        } else if (Flag & 1) {
            BOOL enabled;
            enabled = OSDisableInterrupts();
            DHCPCleanup();
            while (DHCPGetStatus(NULL) != 0) {
                OSSleepThread(&CleaningQueue);
            }
            OSRestoreInterrupts(enabled);
        } else {
            IPInitRoute(NULL, NULL, NULL);
            IPSetBroadcastAddr(__IFDefault, &IPLimited);
        }
    }

    if (TimeWaitBuf != NULL && TimeWaitBufSize != 0) {
        SOFree(6, TimeWaitBuf, TimeWaitBufSize);
    }
    if (ReassemblyBuffer != NULL) {
        IPSetReassemblyBuffer(NULL, 0, 1500);
        SOFree(7, ReassemblyBuffer, ReassemblyBufferSize);
    }

    {
        BOOL enabled = OSDisableInterrupts();
        while (Allocated != 0) {
            OSSleepThread(&CleaningQueue);
        }
        OSRestoreInterrupts(enabled);
    }

    if (!LowInitialized) {
        IFMute(TRUE);
        ARPRefresh(0);
    }
    State = 0;
    return 0;
}

s32 SOSocket(s32 domain, s32 type, s32 flags)
{
    SOsock* node;
    SOTcpSocket* tcp;
    SOUdpSocket* udp;
    u8* buf1;
    u8* buf2;
    s32 rwin;
    s32 mtu;
    s32 result;
    s32 index;
    BOOL enabled;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (domain != AF_INET) {
        return SO_EAFNOSUPPORT;
    }
    if (flags != 0) {
        return SO_EOPNOTSUPP;
    }

    GetNode(-1, NULL);

    enabled = OSDisableInterrupts();
    node = NULL;
    for (index = 0; index < 256; index++) {
        if (SocketTable[index].ref == 0) {
            SocketTable[index].ref = 2;
            node = &SocketTable[index];
            break;
        }
    }
    OSRestoreInterrupts(enabled);
    if (node == NULL) {
        return SO_ENOBUFS;
    }

    rwin = Rwin;
    if (rwin == 0) {
        IPGetMtu(0, &mtu);
        rwin = (mtu - 40) << 1;
    }

    tcp = NULL;
    udp = NULL;
    buf1 = NULL;
    buf2 = NULL;

    switch (type) {
    case SO_STREAM:
        tcp = SOAlloc(0, sizeof(SOTcpSocket));
        buf1 = SOAlloc(1, rwin);
        buf2 = SOAlloc(2, rwin);
        result = TCPOpen(&tcp->tcp, buf1, rwin, buf2, rwin);
        if (result >= 0) {
            TCPSetTimeout(&tcp->tcp, R2);
        }
        break;
    case SO_DGRAM:
        udp = SOAlloc(3, sizeof(SOUdpSocket));
        buf1 = SOAlloc(4, 1458);
        buf2 = SOAlloc(5, 4096);
        result = UDPOpen(udp, buf2, 4096);
        if (result >= 0) {
            result = UDPSetSendBuff(udp, buf1, 1458);
        }
        break;
    default:
        PutNode(node);
        PutNode(node);
        return SO_EPROTONOSUPPORT;
    }

    if (result < 0) {
        switch (type) {
        case SO_STREAM:
            if (tcp != NULL) {
                SOFree(0, tcp, sizeof(SOTcpSocket));
            }
            if (buf1 != NULL) {
                SOFree(1, buf1, rwin);
            }
            if (buf2 != NULL) {
                SOFree(2, buf2, rwin);
            }
            break;
        case SO_DGRAM:
            if (udp != NULL) {
                SOFree(3, udp, sizeof(SOUdpSocket));
            }
            if (buf1 != NULL) {
                SOFree(4, buf1, 1458);
            }
            if (buf2 != NULL) {
                SOFree(5, buf2, 4096);
            }
            break;
        }
        PutNode(node);
        PutNode(node);
        return SO_EPROTOTYPE;
    }

    node->flag = 0;
    OSInitMutex(&node->recvMutex);
    OSInitMutex(&node->sendMutex);
    switch (type) {
    case SO_STREAM:
        tcp->handle = node;
        node->proto = IP_PROTO_TCP;
        node->socket = tcp;
        break;
    case SO_DGRAM:
        node->proto = IP_PROTO_UDP;
        node->socket = udp;
        break;
    }
    PutNode(node);
    return index;
}

static void LingerCallback(OSAlarm* alarm, OSContext* context)
{
    TCPSocket* so = (TCPSocket*)alarm;
    SOTcpSocket* sock = (SOTcpSocket*)so;
    SOTcpLinks* links = (SOTcpLinks*)so;
    SOTcpLinks* tail;

    if (sock->handle != NULL) {
        sock->handle->ref--;
    }
    tail = LingerQueue.tail;
    if (tail == NULL) {
        LingerQueue.head = so;
    } else {
        tail->next = links;
    }
    links->prev = tail;
    links->next = NULL;
    LingerQueue.tail = links;
}

static void LingerTimeout(OSAlarm* alarm, OSContext* context)
{
    TCPCancel((TCPSocket*)((u8*)alarm - 0x2E0));
}

static s32 __SOClose(s32 s)
{
    SOsock* node;
    TCPSocket* so;
    SOTcpSocket* tcp;
    SOTcpSocket* next;
    SOTcpSocket* list;
    SOTcpSocket* tail;
    u8* ptr;
    u32 size;
    s32 result;
    s32 linger;
    BOOL enabled;
    u32 optval[2];
    s32 optlen;

    node = GetNode(s, (void**)&so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (so->unk00[0]) {
    case IP_PROTO_UDP:
        result = UDPClose((SOUdpSocket*)so);
        node->ref--;
        goto done;
    case IP_PROTO_TCP:
        break;
    default:
        result = SO_EBADF;
        goto done;
    }

    tcp = (SOTcpSocket*)so;
    optlen = 8;
    TCPGetSockOpt(so, SO_SOL_SOCKET, SO_SO_LINGER, optval, &optlen);

    list = NULL;
    tail = NULL;
    enabled = OSDisableInterrupts();

    while (tcp->head != NULL) {
        next = tcp->head;
        {
            SOTcpSocket* after = next->next;
            if (after == NULL) {
                tcp->tail = NULL;
            } else {
                after->prev = NULL;
            }
            tcp->head = after;
        }
        next->listener = NULL;
        TCPCancel(&next->tcp);
        if (list == NULL) {
            tail = next;
        } else {
            list->next = next;
        }
        next->prev = list;
        list = next;
        next->next = NULL;
    }

    while (tcp->pendingTail != NULL) {
        next = tcp->pendingTail;
        {
            SOTcpSocket* after = next->next;
            if (after == NULL) {
                tcp->pendingHead = NULL;
            } else {
                after->prev = NULL;
            }
            tcp->pendingTail = after;
        }
        next->listener = NULL;
        TCPCancel(&next->tcp);
        if (list == NULL) {
            tail = next;
        } else {
            list->next = next;
        }
        next->prev = list;
        list = next;
        next->next = NULL;
    }

    if (TCPGetStatus(so) == 1) {
        result = TCPCancel(so);
        if (tcp->count > 0) {
            OSWakeupThread((OSThreadQueue*)&so->unk20[0]);
        }
        node->ref--;
    } else if (node->flag & SO_FNONBLOCK) {
        result = TCPCancel(so);
        node->ref--;
    } else if (optval[0] != 0) {
        linger = optval[1];
        if (linger <= 0) {
            result = TCPCancel(so);
            node->ref--;
        } else {
            OSSetAlarm(&tcp->alarm, OSSecondsToTicks((s64)linger),
                       LingerTimeout);
            result = TCPClose(so);
            node->ref--;
        }
    } else {
        OSSetAlarm(&tcp->alarm, OSSecondsToTicks((s64)15), LingerCallback);
        result = TCPCloseAsync(so, (void (*)(TCPSocket*))LingerCallback, 0);
        if (node->ref == 2) {
            tcp->handle = NULL;
            node->ref--;
        }
        node->socket = NULL;
    }

    OSRestoreInterrupts(enabled);

    while (list != NULL) {
        next = list->next;
        so = (TCPSocket*)list;
        if (next != NULL) {
            next->prev = NULL;
        }
        list = next;
        SOFree(2, (void*)((SOTcpSocket*)so)->tcp.unk1F8,
               ((SOTcpSocket*)so)->tcp.unk1FC);
        SOFree(1, (void*)((SOTcpSocket*)so)->tcp.unk174,
               ((SOTcpSocket*)so)->tcp.unk178);
        SOFree(0, so, 0x330);
    }

done:
    PutNode(node);
    if (result < 0) {
        return SO_EBADF;
    }
    return 0;
}

s32 SOClose(s32 s)
{
    if (State != 1) {
        return SO_ENOTINIT;
    }
    return __SOClose(s);
}

static void AcceptCallback(TCPSocket* so, s32 result)
{
    SOTcpSocket* listener;
    SOTcpSocket* sock;
    SOTcpSocket* next;
    SOTcpSocket* prev;
    SOTcpSocket* ptr;

    sock = (SOTcpSocket*)so;
    listener = (SOTcpSocket*)sock->listener;

    if (listener == NULL) {
        return;
    }

    if (result >= 0) {
        next = sock->next;
        prev = sock->prev;
        if (next == NULL) {
            listener->tail = prev;
        } else {
            next->prev = prev;
        }
        if (prev == NULL) {
            listener->head = next;
        } else {
            prev->next = next;
        }
        ptr = listener->pendingHead;
        if (ptr == NULL) {
            listener->pendingTail = sock;
        } else {
            ptr->next = sock;
        }
        sock->prev = ptr;
        sock->next = NULL;
        listener->pendingHead = sock;
        OSWakeupThread((OSThreadQueue*)&listener->tcp.unk20[0]);
        if (listener->tcp.unk04 > 0) {
            __IPWakeupPollingThreads();
        }
        return;
    }

    next = sock->next;
    prev = sock->prev;
    if (next == NULL) {
        listener->tail = prev;
    } else {
        next->prev = prev;
    }
    if (prev == NULL) {
        listener->head = next;
    } else {
        prev->next = next;
    }

    TCPCancel(&sock->tcp);
    TCPOpen(&sock->tcp, (u8*)sock->tcp.unk174, sock->tcp.unk178,
            (u8*)sock->tcp.unk1F8, sock->tcp.unk1FC);
    TCPSetTimeout(&sock->tcp, R2);
    sock->listener = &listener->tcp;
    prev = listener->tail;
    if (prev == NULL) {
        listener->head = sock;
    } else {
        prev->next = sock;
    }
    sock->prev = prev;
    sock->next = NULL;
    listener->tail = sock;
    TCPAcceptAsync(&sock->tcp, &listener->tcp, AcceptCallback, 0);
}

static SOTcpSocket* AddBackLog(TCPSocket* so)
{
    SOTcpSocket* tcp;
    SOTcpSocket* listener;
    u8* buf1;
    u8* buf2;
    s32 size1;
    s32 size2;
    BOOL enabled;
    s32 result;

    listener = (SOTcpSocket*)so;
    tcp = SOAlloc(0, 0x330);
    size1 = listener->tcp.unk178;
    size2 = listener->tcp.unk1FC;
    buf1 = SOAlloc(1, size1);
    buf2 = SOAlloc(2, size2);
    result = TCPOpen(&tcp->tcp, buf1, size1, buf2, size2);
    if (result < 0) {
        goto fail;
    }
    TCPSetTimeout(&tcp->tcp, R2);

    enabled = OSDisableInterrupts();
    if (TCPGetStatus(so) == 1) {
        tcp->listener = so;
        {
            SOTcpSocket* prev = (SOTcpSocket*)listener->tail;
            if (prev == NULL) {
                listener->head = tcp;
            } else {
                prev->next = tcp;
            }
            tcp->prev = prev;
            tcp->next = NULL;
            listener->tail = tcp;
        }
        OSRestoreInterrupts(enabled);

        TCPAcceptAsync(&tcp->tcp, so, AcceptCallback, 0);
        OSRestoreInterrupts(enabled);
        return tcp;
    }
    OSRestoreInterrupts(enabled);

fail:
    if (buf2 != 0) {
        SOFree(2, buf2, size2);
    }
    if (buf1 != 0) {
        SOFree(1, buf1, size1);
    }
    if (tcp != 0) {
        SOFree(0, tcp, 0x330);
    }
    return NULL;
}

s32 SOListen(s32 s, s32 arg)
{
    SOsock* node;
    TCPSocket* so;
    s32 result;
    s32 backlog;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    backlog = arg;
    if (backlog < 1) {
        backlog = 1;
    }

    node = GetNode(s, (void**)&so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (so->unk00[0]) {
    case IP_PROTO_UDP:
        result = SO_EBADR;
        break;
    case IP_PROTO_TCP:
        result = TCPListen(so, NULL, 0, 0, 0);
        switch (result) {
        case -7:
            result = -42;
            break;
        case -5:
            result = SO_EINVAL;
            break;
        case 0:
            while (backlog-- > 0) {
                if (AddBackLog(so) == NULL) {
                    break;
                }
            }
            break;
        default:
            result = SO_EINVAL;
            break;
        }
        break;
    default:
        PutNode(node);
        return SO_EBADF;
    }

    PutNode(node);
    return result;
}

s32 SOAccept(s32 s, u8* addr)
{
    BOOL enabled;
    SOsock* node;
    void* socket;
    SOTcpSocket* so;
    SOTcpSocket* so2;
    s32 index;
    s32 result;
    s32 status;
    SOsock* entry;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (addr != NULL && addr[0] < 8) {
        return SO_EINVAL;
    }

    node = GetNode(s, &socket);
    if (node == NULL || socket == NULL) {
        return SO_EBADF;
    }

    enabled = OSDisableInterrupts();
    switch (((u8*)socket)[0]) {
    case IP_PROTO_UDP:
        result = SO_EBADR;
        goto done;
    case IP_PROTO_TCP:
        break;
    default:
        result = SO_EBADF;
        goto done;
    }

    so = (SOTcpSocket*)socket;
    for (index = 0, entry = SocketTable; index < 256; index++, entry++) {
        if (entry->ref != 0) {
            continue;
        }
        for (;;) {
            if (TCPGetStatus((TCPSocket*)so) != 1) {
                result = SO_EINVAL;
                goto done;
            }
            so->count++;
            while (TCPGetStatus((TCPSocket*)so) == 1 && so->pendingTail != NULL) {
                if (node->flag & SO_FNONBLOCK) {
                    so->count--;
                    result = SO_EWOULDBLOCK;
                    goto done;
                }
                OSSleepThread((OSThreadQueue*)&so->tcp.unk20[0]);
            }
            so->count--;
            if (TCPGetStatus((TCPSocket*)so) != 1) {
                result = SO_EACCES;
                goto done;
            }

            so2 = so->pendingTail;
            if (so2->next == NULL) {
                so->pendingHead = NULL;
            } else {
                ((SOTcpSocket*)so2->next)->prev = NULL;
            }
            so->pendingTail = so2->next;

            result = 0;
            if (addr != NULL) {
                result = TCPGetRemoteSocket(&so2->tcp, addr);
            }

            status = TCPGetStatus(&so2->tcp);
            if ((status == 4 || status == 7) && result >= 0) {
                entry->flag = node->flag;
                entry->ref = 1;
                OSInitMutex(&entry->recvMutex);
                OSInitMutex(&entry->sendMutex);
                entry->proto = IP_PROTO_TCP;
                result = index;
                entry->socket = so2;
                OSRestoreInterrupts(enabled);
                AddBackLog((TCPSocket*)so);
                goto out;
            }

            TCPCancel(&so2->tcp);
            TCPOpen(&so2->tcp, (u8*)so2->tcp.unk174, so2->tcp.unk178,
                    (u8*)so2->tcp.unk1F8, so2->tcp.unk1FC);
            TCPSetTimeout(&so2->tcp, R2);
            so2->listener = (TCPSocket*)so;
            if (so->tail == NULL) {
                so->head = so2;
            } else {
                ((SOTcpSocket*)so->tail)->next = so2;
            }
            so2->prev = so->tail;
            so2->next = NULL;
            so->tail = so2;
            OSRestoreInterrupts(enabled);
            TCPAcceptAsync(&so2->tcp, (TCPSocket*)so, AcceptCallback, 0);
            OSRestoreInterrupts(enabled);
        }
    }
out:
    if (index >= 256) {
        result = SO_ENOBUFS;
    }

done:
    OSRestoreInterrupts(enabled);
    PutNode(node);
    return result;
}

s32 SOBind(s32 s, u8* addr)
{
    SOsock* node;
    void* so;
    s32 result;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (addr == NULL || addr[0] < 8) {
        return SO_EINVAL;
    }

    node = GetNode(s, &so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (((u8*)so)[0]) {
    case IP_PROTO_UDP:
        result = UDPBind(so, addr);
        break;
    case IP_PROTO_TCP:
        result = TCPBind(so, addr);
        break;
    default:
        PutNode(node);
        return SO_EBADF;
    }

    PutNode(node);
    switch (result) {
    case 0:
        return 0;
    case -13:
        return SO_EAFNOSUPPORT;
    case -5:
        return SO_ESRCH;
    case -12:
    default:
        return SO_EINVAL;
    }
}

s32 SOShutdown(s32 s, s32 how)
{
    SOsock* node;
    TCPSocket* so;
    s32 result;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (how >= 3 || how < 0) {
        return SO_EINVAL;
    }

    node = GetNode(s, (void**)&so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (((u8*)so)[0]) {
    case IP_PROTO_UDP:
        result = 0;
        break;
    case IP_PROTO_TCP:
        result = TCPShutdown(so, how);
        break;
    default:
        PutNode(node);
        return SO_EBADF;
    }

    PutNode(node);
    switch (result + 12) {
    case 4:
    case 12:
        return 0;
    case 8:
        return SO_EMFILE;
    case 0:
    default:
        return SO_EINVAL;
    }
}

s32 SORead(s32 s, void* buf, s32 len)
{
    return SORecvFrom(s, buf, len, 0, NULL);
}

s32 SORecvFrom(s32 s, void* buf, s32 len, u32 flags, u8* addr)
{
    SOsock* node;
    TCPSocket* so;
    s32 result;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (addr != NULL && addr[0] < 8) {
        return SO_EINVAL;
    }

    node = GetNode(s, (void**)&so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (so->unk00[0]) {
    case IP_PROTO_UDP:
        if (flags & 0xF) {
            PutNode(node);
            return SO_EBADR;
        }
        OSLockMutex(&node->recvMutex);
        if (node->socket == NULL) {
            result = SO_EBADF;
        } else {
            if (node->flag & SO_FNONBLOCK) {
                flags |= SO_FNONBLOCK;
            }
            result = UDPReceiveEx((SOUdpSocket*)so, buf, len, 0, addr, flags);
        }
        OSUnlockMutex(&node->recvMutex);
        break;
    case IP_PROTO_TCP:
        if (flags & ~0x7) {
            PutNode(node);
            return SO_EBADR;
        }
        if (addr != NULL) {
            result = TCPGetRemoteSocket(so, addr);
            if (result < 0) {
                PutNode(node);
                return SO_EBADF;
            }
        }
        OSLockMutex(&node->recvMutex);
        if (node->socket == NULL) {
            result = SO_EBADF;
        } else {
            if (node->flag & SO_FNONBLOCK) {
                flags |= SO_FNONBLOCK;
            }
            {
                TCPSocket* sock = so;
                if (!(flags & 1)) {
                    result = TCPReceiveEx(sock, buf, len, flags);
                } else {
                    result = TCPReceiveUrgEx(sock, buf, len, flags);
                }
            }
        }
        OSUnlockMutex(&node->recvMutex);
        break;
    default:
        PutNode(node);
        return SO_EBADF;
    }

    PutNode(node);
    if (result < 0) {
        switch (result) {
        case -9:
        case -1:
            result = SO_EWOULDBLOCK;
            break;
        case -8:
        case -4:
            result = SO_EMFILE;
            break;
        case -16:
            result = SO_EFBIG;
            break;
        case -19:
        case -10:
            result = SO_EOVERFLOW;
            break;
        case -18:
        case -11:
        case -3:
            result = SO_EFAULT;
            break;
        default:
            result = SO_EINVAL;
            break;
        }
    }
    return result;
}

s32 SOWrite(s32 s, const void* buf, s32 len)
{
    return SOSendTo(s, buf, len, 0, NULL);
}

s32 SOSendTo(s32 s, const void* buf, s32 len, s32 flags, u8* addr)
{
    SOsock* node;
    TCPSocket* so;
    s32 result;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (addr != NULL && addr[0] < 8) {
        return SO_EINVAL;
    }

    node = GetNode(s, (void**)&so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (so->unk00[0]) {
    case IP_PROTO_UDP:
        if (flags) {
            PutNode(node);
            return SO_EBADR;
        }
        OSLockMutex(&node->sendMutex);
        if (node->socket == NULL) {
            result = SO_EBADF;
        } else if (flags == 0) {
            result = UDPSend((SOUdpSocket*)so, buf, len, addr);
        }
        OSUnlockMutex(&node->sendMutex);
        break;
    case IP_PROTO_TCP:
        if (flags & -6) {
            PutNode(node);
            return SO_EBADR;
        }
        OSLockMutex(&node->sendMutex);
        if (node->socket == NULL) {
            result = SO_EBADF;
        } else {
            if (node->flag & SO_FNONBLOCK) {
                flags |= SO_FNONBLOCK;
            }
            {
                TCPSocket* sock = so;
                switch (flags) {
                case 0:
                    result = TCPSend(sock, buf, len);
                    break;
                case 1:
                    result = TCPSendUrg(sock, buf, len);
                    break;
                case 4:
                    result = TCPSendNonblock(sock, buf, len);
                    break;
                case 6:
                    result = TCPSendUrgNonblock(sock, buf, len);
                    break;
                default:
                    result = SO_EINVAL;
                    break;
                }
            }
        }
        OSUnlockMutex(&node->sendMutex);
        break;
    default:
        PutNode(node);
        return SO_EBADF;
    }

    PutNode(node);
    if (result < 0) {
        switch (result + 19) {
        case 6:
            result = SO_EAFNOSUPPORT;
            break;
        case 13:
            result = SO_EEXIST;
            break;
        case 2:
            result = SO_EAGAIN;
            break;
        case 17:
            result = SO_ELOOP;
            break;
        case 12:
            result = SO_ENOMSG;
            break;
        case 10:
        case 18:
            result = SO_EWOULDBLOCK;
            break;
        case 11:
        case 15:
            result = SO_EMFILE;
            break;
        case 3:
            result = SO_EFBIG;
            break;
        case 9:
            result = SO_EOVERFLOW;
            break;
        case 1:
        case 8:
        case 16:
            result = SO_EFAULT;
            break;
        case 7:
            result = SO_EINVAL;
            break;
        case 0:
            result = SO_ENAMETOOLONG;
            break;
        case 5:
        case 4:
        case 14:
            result = SO_EBADF;
            break;
        default:
            result = SO_EBADF;
            break;
        }
    }
    return result;
}

static s32 __SOSetSockOpt(s32 s, s32 level, s32 opt, void* val, u32 len)
{
    SOsock* node;
    void* so;
    u8* buf;
    u8* old;
    u32 oldSize;
    s32 size;
    s32 result;

    node = GetNode(s, &so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (((u8*)so)[0]) {
    case IP_PROTO_UDP:
        if (((u32)level >> 16) == 0xFFFF) {
        SOUdpSocket* udp = (SOUdpSocket*)so;
        switch (opt) {
        case SO_SO_SNDBUF:
            result = SO_ENOMEM;
            if (len < 4 || val == NULL) {
                goto done;
            }
            size = *(u32*)val;
            if (size < 536) {
                size = 536;
            }
            buf = SOAlloc(4, size + 88);
            if (buf == NULL) {
                result = -7;
                goto done;
            }
            UDPGetSendBuff(so, &old, &oldSize);
            result = UDPSetSendBuff(so, buf, size + 88);
            if (result == 0) {
                if (old != NULL) {
                    SOFree(4, old, oldSize);
                }
            } else {
                SOFree(4, buf, size + 88);
            }
            goto done;
        case SO_SO_RCVBUF:
            result = SO_ENOMEM;
            if (len < 4 || val == NULL) {
                goto done;
            }
            size = *(u32*)val;
            if (size < 536) {
                size = 536;
            }
            buf = SOAlloc(5, size);
            if (buf == NULL) {
                result = -7;
                goto done;
            }
            UDPGetRecvBuff(so, &old, &oldSize);
            result = UDPSetRecvBuff(so, buf, size);
            if (result == 0) {
                if (old != NULL) {
                    SOFree(5, old, oldSize);
                }
            } else {
                SOFree(5, buf, size);
            }
            goto done;
        }
        }
        result = UDPSetSockOpt(so, level, opt, val, len);
        goto done;
    case IP_PROTO_TCP:
        if (((u32)level >> 16) == 0xFFFF) {
        SOTcpSocket* tcp = (SOTcpSocket*)so;
        switch (opt) {
        case SO_SO_SNDBUF:
            result = SO_ENOMEM;
            if (len < 4 || val == NULL) {
                goto done;
            }
            size = *(u32*)val;
            if (size < 536) {
                size = 536;
            }
            buf = SOAlloc(1, size);
            if (buf == NULL) {
                result = -7;
                goto done;
            }
            TCPGetSendBuff(&tcp->tcp, &old, &oldSize);
            result = TCPSetSendBuff(&tcp->tcp, buf, size);
            if (result == 0) {
                if (old != NULL) {
                    SOFree(1, old, oldSize);
                }
            } else {
                SOFree(1, buf, size);
            }
            goto done;
        case SO_SO_RCVBUF:
            result = SO_ENOMEM;
            if (len < 4 || val == NULL) {
                goto done;
            }
            size = *(u32*)val;
            if (size < 536) {
                size = 536;
            }
            buf = SOAlloc(2, size);
            if (buf == NULL) {
                result = -7;
                goto done;
            }
            TCPGetRecvBuff(&tcp->tcp, &old, &oldSize);
            result = TCPSetRecvBuff(&tcp->tcp, buf, size);
            if (result == 0) {
                if (old != NULL) {
                    SOFree(2, old, oldSize);
                }
            } else {
                SOFree(2, buf, size);
            }
            goto done;
        }
        }
        result = TCPSetSockOpt(so, level, opt, val, len);
        goto done;
    default:
        PutNode(node);
        return SO_EBADF;
    }

done:
    PutNode(node);
    switch (result) {
    case -7:
        result = SO_EPROTOTYPE;
        break;
    case -14:
        result = SO_EBADMSG;
        break;
    case 0:
        result = 0;
        break;
    default:
        result = SO_EINVAL;
        break;
    }
    return result;
}

s32 SOSetSockOpt(s32 s, s32 level, s32 opt, void* val, s32 len)
{
    if (State != 1) {
        return SO_ENOTINIT;
    }
    return __SOSetSockOpt(s, level, opt, val, len);
}

s32 SOFcntl(s32 s, s32 cmd, ...)
{
    void* so;
    SOsock* node;
    s32 result;
    va_list ap;

    if (State != 1) {
        return SO_ENOTINIT;
    }

    node = GetNode(s, &so);
    if (node == NULL || so == NULL) {
        return SO_EBADF;
    }

    switch (((u8*)so)[0]) {
    case IP_PROTO_UDP:
    case IP_PROTO_TCP:
        switch (cmd) {
        case SO_F_GETFL:
            result = node->flag;
            break;
        case SO_F_SETFL:
            va_start(ap, cmd);
            node->flag = *(u32*)__va_arg(ap, 1);
            result = 0;
            break;
        default:
            result = SO_ENOMEM;
            break;
        }
        break;
    default:
        result = SO_ENOMEM;
        break;
    }

    PutNode(node);
    if (result >= 0) {
        return result;
    }
    return SO_EINVAL;
}
u32 SOGetHostID(void)
{
    u32 addr;

    IPGetAddr(0, &addr);
    return addr;
}

static void PollTimeout(OSAlarm* alarm, OSContext* context)
{
    OSWakeupThread(&PollingQueue);
}

void __IPWakeupPollingThreads(void)
{
    OSWakeupThread(&PollingQueue);
}

s32 SOPoll(SOPollFd* fds, s32 nfds, s64 timeout)
{
    OSAlarm alarm;
    SOsock* node;
    TCPSocket* so;
    s32 result;
    s32 count;
    s32 fd;
    s32 i;
    s16 events;
    BOOL enabled;

    if (State != 1) {
        return SO_ENOTINIT;
    }
    if (nfds > 256) {
        return SO_EINVAL;
    }

    enabled = OSDisableInterrupts();
    for (i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        fd = fds[i].fd;
        node = NULL;
        if (0 <= fd && fd < 256) {
            node = &SocketTable[fd];
            if (node->ref > 0 && node->socket != NULL) {
                node->ref++;
            } else {
                node = NULL;
            }
        }
        if (node != NULL) {
            so = node->socket;
            ((s16*)((u8*)so + 4))[0]++;
        }
    }
    OSRestoreInterrupts(enabled);

    if (timeout != 0) {
        OSCreateAlarm(&alarm);
        OSSetAlarm(&alarm, timeout, PollTimeout);
    }

    enabled = OSDisableInterrupts();
    result = 0;
    for (;;) {
        for (i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            fd = fds[i].fd;
            node = NULL;
            if (0 <= fd && fd < 256) {
                node = &SocketTable[fd];
                if (node->ref > 0 && node->socket != NULL) {
                } else {
                    node = NULL;
                }
            }
            if (node == NULL) {
                continue;
            }
            so = node->socket;
            events = fds[i].events | 0xE0;
            switch (so->unk00[0]) {
            case IP_PROTO_UDP:
                events &= __UDPPoll((SOUdpSocket*)so);
                break;
            case IP_PROTO_TCP:
                events &= __TCPPoll(so);
                break;
            default:
                events = 0;
                break;
            }
            if (events != 0) {
                fds[i].revents = events;
                result++;
            }
        }

        if (result > 0) {
            break;
        }
        if (timeout != 0) {
            if (timeout > 0) {
                OSSleepThread(&PollingQueue);
            } else if (alarm.fire != 0) {
                OSSleepThread(&PollingQueue);
            } else {
                break;
            }
        } else {
            break;
        }
        if (State != 1) {
            break;
        }
    }
    OSRestoreInterrupts(enabled);

    if (timeout != 0) {
        OSCancelAlarm(&alarm);
    }

    count = 0;
    for (i = 0; i < nfds; i++) {
        fd = fds[i].fd;
        node = NULL;
        if (0 <= fd && fd < 256) {
            node = &SocketTable[fd];
            if (node->ref > 0 && node->socket != NULL) {
            } else {
                node = NULL;
            }
        }
        if (node == NULL) {
            continue;
        }
        enabled = OSDisableInterrupts();
        so = node->socket;
        ((s16*)((u8*)so + 4))[0]--;
        OSRestoreInterrupts(enabled);
        PutNode(node);
        count++;
    }
    return result;
}

static BOOL OnReset(BOOL final)
{
    State = 3;
    return TRUE;
}
