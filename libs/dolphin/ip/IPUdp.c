#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

typedef struct UDPInfo UDPInfo;
typedef struct IPInterface IPInterface;

typedef void (*UDPCallback)(UDPInfo* info, s32 result);

typedef struct UDPAddr {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
} UDPAddr;

typedef struct UDPHeader {
    u16 src;
    u16 dst;
    u16 len;
    u16 sum;
} UDPHeader;

typedef struct IPVec {
    void* data;
    s32 len;
} IPVec;

typedef struct IPPacket {
    u32 unk00;
    u32 unk04;
    u8 unk08[0x1D];
    u8 unk25;
    u8 unk26[2];
    UDPCallback callback;
    UDPInfo* info;
    s32 nvec;
    IPVec vec[2];
} IPPacket;

typedef struct UDPInfoQueue {
    UDPInfo* head;
    UDPInfo* tail;
} UDPInfoQueueT;

struct UDPInfo {
    u8 proto;
    u8 ttl;
    u8 tos;
    u8 mttl;
    s16 poll;
    u16 unk06;
    UDPAddr local;
    UDPAddr remote;
    UDPInfo* next;
    UDPInfo* prev;
    OSThreadQueue queue;
    u32 flag;
    UDPCallback sendCallback;
    s32* sendResult;
    IPPacket packet;
    u8 header[28];
    u8 unk94[0xBC - 0x94];
    UDPCallback recvCallback;
    s32* recvResult;
    u8* recvBuf;
    s32 recvLen;
    UDPAddr* local_;
    UDPAddr* remote_;
    u8* ringBase;
    s32 ringSize;
    u8* ringHead;
    s32 ringUsed;
    u8* sendBuf;
    s32 sendSize;
    s32 sendUsed;
};

struct IPInterface {
    u8 unk00[0x44];
    u8 addr[4];
    u8 unk48[0x0C];
    u8 alias[4];
    u8 unk58[0x10];
    void* (*alloc)(IPInterface* interface, s32 size);
    void (*free)(IPInterface* interface, void* packet, s32 size);
    u8 unk70[0x18];
    s32 unk88;
    u8 unk8C[0x0C];
    s32 unk98;
};

struct ICMPHeader {
    u8 type;
    u8 code;
    u16 sum;
    u16 unk04;
    u16 unk06;
};

extern u32 IPAddrAny;
extern u32 IPLoopbackAddr;
extern IPInterface __IFDefault;

extern s32 IPBind(UDPInfoQueueT* queue, void* info, UDPAddr* addr, s32 reuse);
extern s32 IPConnect(UDPInfoQueueT* queue, void* info, UDPAddr* addr, u16* port);
extern u16 IPGetAnonPort(UDPInfoQueueT* queue, u16* port);
extern BOOL __IPIsMember(UDPInfoQueueT* queue, void* info);
extern void* IPLookupInfo(UDPInfoQueueT* queue, u8* src, u8* dst, u16 srcPort, u16 dstPort, u32 flag);
extern s32 IPOut(IPPacket* packet);
extern void IPCancel(IPPacket* packet);
extern void IPClose(void* info);
extern s32 IPSetSockOpt(void* info, s32 level, s32 optname, void* optval, s32 optlen);
extern void __IPWakeupPollingThreads(void);
extern s32 IFRingGet(u8* base, s32 size, u8* head, s32 used, IPVec* vec, s32 len);
extern u8* IFRingIn(u8* base, s32 size, u8* head, s32 used, void* src, s32 len);
extern u8* IFRingPut(u8* base, s32 size, u8* head, s32 used, s32 len);

#define IP_HLEN(ip) (((ip)->verlen & 0x0F) << 2)

static u16 Port = 1024;
UDPInfoQueueT UDPInfoQueue;

static void PeekInput(UDPInfo* info, s32 offset, void* buf, s32 len)
{
    IPVec vec[2];
    u8* head;
    s32 n;
    s32 i;
    u8* dst;

    head = info->ringHead + offset;
    if (info->ringHead + info->ringSize <= head) {
        head -= info->ringSize;
    }
    n = IFRingGet(info->ringBase, info->ringSize, head, info->ringUsed - offset, vec, len);
    dst = (u8*)buf;
    for (i = 0; i < n; i++) {
        memmove(dst, vec[i].data, vec[i].len);
        dst += vec[i].len;
    }
}

static void DiscardInput(UDPInfo* info, IPHeader* ip)
{
    info->ringHead = IFRingPut(info->ringBase, info->ringSize, info->ringHead, info->ringUsed, ip->len + 4);
    info->ringUsed -= ip->len + 4;
}

static IPHeader* SaveInput(UDPInfo* info, IPHeader* ip, u32 flag)
{
    if ((u32)(info->ringSize - info->ringUsed) < ip->len + 4) {
        return NULL;
    }
    info->ringHead = IFRingIn(info->ringBase, info->ringSize, info->ringHead, info->ringUsed, ip, ip->len);
    info->ringUsed += ip->len;
    info->ringHead = IFRingIn(info->ringBase, info->ringSize, info->ringHead, info->ringUsed, &flag, 4);
    info->ringUsed += 4;
    return ip;
}

static void CopySockets(UDPAddr* local, UDPAddr* remote, IPHeader* ip, UDPHeader* udp)
{
    if (local) {
        local->len = 8;
        local->family = 2;
        memmove(local->addr, ip->dst, 4);
        local->port = udp->dst;
    }
    if (remote) {
        remote->len = 8;
        remote->family = 2;
        memmove(remote->addr, ip->src, 4);
        remote->port = udp->src;
    }
}

static void NullCallback(UDPInfo* info, s32 result)
{
}

static void SyncCallback(UDPInfo* info, s32 result)
{
    OSWakeupThread(&info->queue);
}

u16 UDPCheckSum(IPVec* vec, s32 n)
{
    IPHeader* ip;
    u32 sum;
    u16* p;
    s32 len;

    ip = (IPHeader*)vec->data;
    sum = ((u16*)ip->src)[0];
    sum += ((u16*)ip->src)[1];
    sum += ((u16*)ip->dst)[0];
    sum += ((u16*)ip->dst)[1];
    sum += IP_PROTO_UDP;
    sum += ip->len - IP_HLEN(ip);
    p = (u16*)((u8*)ip + IP_HLEN(ip));
    len = vec->len - IP_HLEN(ip);
    for (;;) {
        while (len > 1) {
            sum += *p++;
            len -= 2;
        }
        if (len == 1) {
            sum += *(u8*)p << 8;
        }
        if (--n <= 0) {
            break;
        }
        p = (u16*)vec[1].data;
        len = vec[1].len;
        vec++;
    }
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);
    return sum ^ 0xFFFF;
}

s32 UDPBind(UDPInfo* info, UDPAddr* addr)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else {
        rc = IPBind(&UDPInfoQueue, info, addr, (info->flag & 0x10000) ? 1 : 0);
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPConnect(UDPInfo* info, UDPAddr* addr)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else {
        rc = IPConnect(&UDPInfoQueue, info, addr, &Port);
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPGetRecvBuff(UDPInfo* info, u8** buf, s32* size)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else {
        rc = 0;
        if (size) {
            *size = info->ringSize;
        }
        if (buf) {
            *buf = info->ringBase;
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPSetRecvBuff(UDPInfo* info, u8* buf, s32 size)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else {
        info->ringBase = buf;
        rc = 0;
        info->ringSize = size;
        info->ringHead = buf;
        info->ringUsed = 0;
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPGetSendBuff(UDPInfo* info, u8** buf, s32* size)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else {
        rc = 0;
        if (size) {
            *size = info->sendSize;
        }
        if (buf) {
            *buf = info->sendBuf;
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPSetSendBuff(UDPInfo* info, u8* buf, s32 size)
{
    BOOL enabled;
    s32 rc;

    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else if (info->sendUsed != 0) {
        rc = -1;
    } else {
        info->sendBuf = buf;
        rc = 0;
        info->sendSize = size;
        info->sendUsed = 0;
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPOpen(UDPInfo* info, u8* buf, s32 size)
{
    BOOL enabled;
    BOOL member;

    if (info == NULL) {
        return -12;
    }
    enabled = OSDisableInterrupts();
    member = __IPIsMember(&UDPInfoQueue, info);
    OSRestoreInterrupts(enabled);
    if (member) {
        return -5;
    }

    memset(info, 0, sizeof(UDPInfo));
    info->proto = IP_PROTO_UDP;
    info->tos = 0;
    info->ttl = 255;
    info->mttl = 1;
    info->poll = 0;
    info->unk06 = 0x8000;
    info->local.len = 8;
    info->local.family = 2;
    info->remote.len = 8;
    info->remote.family = 2;
    OSInitThreadQueue(&info->queue);
    info->ringBase = buf;
    info->ringSize = size;
    info->ringHead = buf;
    info->ringUsed = 0;

    enabled = OSDisableInterrupts();
    {
        UDPInfo* prev = UDPInfoQueue.tail;

        if (prev == NULL) {
            UDPInfoQueue.head = info;
        } else {
            prev->next = info;
        }
        info->prev = prev;
        info->next = NULL;
        UDPInfoQueue.tail = info;
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

static void SendCallback(UDPInfo* info, s32 result)
{
    result = (result < 0) ? result : info->packet.vec[1].len;
    if (info->sendResult) {
        *info->sendResult = result;
        info->sendResult = NULL;
    }
    if (info->sendBuf) {
        info->sendUsed = 0;
    } else {
        UDPCallback callback = info->sendCallback;

        info->sendCallback = NULL;
        callback(info, result);
    }
    if (info->poll > 0 && info->sendBuf == NULL) {
        __IPWakeupPollingThreads();
    }
}

s32 UDPSendAsync(UDPInfo* info, const void* data, s32 len, UDPAddr* addr, UDPCallback callback, s32* result)
{
    BOOL enabled;
    IPHeader* ip;
    UDPHeader* udp;
    IPPacket* packet;
    IPInterface* interface;
    u8* dst;
    s32 rc;

    interface = NULL;
    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
        goto Exit;
    }
    if (info->local.port == 0) {
        info->local.port = IPGetAnonPort(&UDPInfoQueue, &Port);
        if (info->local.port == 0) {
            rc = -7;
            goto Exit;
        }
    }
    if (len < 0 || 65535 - 8 < len) {
        rc = -17;
        goto Exit;
    }
    if (info->sendCallback && info->sendBuf == NULL) {
        rc = -1;
        goto Exit;
    }
    if (info->remote.port == 0) {
        if (addr == NULL) {
            rc = -6;
            goto Exit;
        }
        if (addr->len != 8 || addr->family != 2 || addr->port == 0 || (addr->addr[0] & 0xF8) == 0xF0) {
            rc = -12;
            goto Exit;
        }
        if (*(u32*)addr->addr == IPAddrAny) {
            rc = -13;
            goto Exit;
        }
    }
    if (info->sendBuf && info->sendSize < len) {
        rc = -17;
        goto Exit;
    }

    if (info->sendBuf == NULL) {
        ip = (IPHeader*)info->header;
        udp = (UDPHeader*)(info->header + 20);
        packet = &info->packet;
        info->sendCallback = callback ? callback : NullCallback;
        info->sendResult = result;
        if (result) {
            *result = -1;
        }
        packet->vec[1].data = (void*)data;
        packet->callback = SendCallback;
    } else if (info->sendUsed <= 0) {
        ip = (IPHeader*)info->header;
        udp = (UDPHeader*)(info->header + 20);
        packet = &info->packet;
        memmove(info->sendBuf, data, len);
        info->sendUsed = len;
        packet->vec[1].data = info->sendBuf;
        packet->callback = SendCallback;
    } else {
        interface = &__IFDefault;
        packet = interface->alloc(interface, len + 96);
        if (packet == NULL) {
            interface->unk98++;
            rc = -7;
            goto Exit;
        }
        ip = (IPHeader*)(packet + 1);
        udp = (UDPHeader*)(ip + 1);
        dst = (u8*)(udp + 1);
        memmove(dst, data, len);
        packet->vec[1].data = dst;
        packet->callback = NULL;
        packet->unk04 = 0;
        packet->unk00 = 0;
    }

    ip->verlen = 0x45;
    ip->tos = info->tos;
    ip->len = IP_HLEN(ip) + 8 + len;
    ip->proto = IP_PROTO_UDP;
    ip->frag = 0;
    udp->src = info->local.port;
    udp->len = len + 8;
    udp->sum = 0;
    if (info->remote.port) {
        udp->dst = info->remote.port;
        memmove(ip->dst, info->remote.addr, 4);
    } else {
        udp->dst = addr->port;
        memmove(ip->dst, addr->addr, 4);
    }
    ip->ttl = ((ip->dst[0] & 0xF0) == 0xE0) ? info->mttl : info->ttl;
    if ((info->local.addr[0] & 0xF0) != 0xE0 && *(u32*)info->local.addr != IPAddrAny) {
        memmove(ip->src, info->local.addr, 4);
    } else if (ip->dst[0] == 127) {
        memmove(ip->src, &IPLoopbackAddr, 4);
    } else {
        IPInterface* def = &__IFDefault;
        u8* alias = def->alias;
        u8* addr;

        if (memcmp(ip->dst, alias, 2) == 0 || (addr = def->addr, *(u32*)def->addr == IPAddrAny)) {
            memmove(ip->src, alias, 4);
        } else {
            memmove(ip->src, addr, 4);
        }
    }

    packet->nvec = 2;
    packet->vec[0].data = ip;
    packet->vec[0].len = IP_HLEN(ip) + 8;
    packet->vec[1].len = len;
    packet->info = info;
    packet->unk25 = 0;
    rc = IPOut(packet);
    if (rc < 0) {
        if (result) {
            *result = rc;
        }
        info->sendCallback = NULL;
        if (info->poll > 0 && info->sendBuf == NULL) {
            __IPWakeupPollingThreads();
        }
        if (interface) {
            interface->free(interface, packet, len + 96);
        }
    } else if (info->sendBuf) {
        if (result) {
            *result = len;
        }
        if (callback) {
            callback(info, len);
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;

Exit:
    if (result) {
        *result = rc;
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPSend(UDPInfo* info, const void* data, s32 len, UDPAddr* addr)
{
    s32 rc;
    s32 result;
    BOOL enabled;

    rc = UDPSendAsync(info, data, len, addr, SyncCallback, &result);
    if (rc < 0) {
        return rc;
    }
    enabled = OSDisableInterrupts();
    while (result == -1) {
        OSSleepThread(&info->queue);
    }
    OSRestoreInterrupts(enabled);
    return result;
}


static inline s32 Min(s32 a, s32 b)
{
    if (a < b) {
        return a;
    }
    return b;
}


s32 UDPReceiveExAsync(UDPInfo* info, void* buf, s32 len, UDPAddr* local, UDPAddr* remote, u32 flags,
                      UDPCallback callback, s32* result)
{
    IPHeader ip;
    BOOL enabled;
    s32 rc;
    UDPHeader udp;
    s32 n;
    UDPCallback cb;
    s32 hlen;

    rc = 0;
    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else if (len < 0) {
        rc = -17;
    } else if (info->local.port == 0) {
        rc = -4;
    } else if (info->recvCallback && !(flags & 4)) {
        rc = -1;
    }
    if (rc != 0) {
        if (result) {
            *result = rc;
        }
        OSRestoreInterrupts(enabled);
        return rc;
    }

    cb = callback ? callback : NullCallback;
    if (info->ringUsed > 0) {
        n = 20;
        if (n > info->ringUsed) {
            n = info->ringUsed;
        }
        if (n > 0) {
            PeekInput(info, 0, &ip, n);
        }
        n = 8;
        if (info->ringUsed < IP_HLEN(&ip) + 8) {
            n = info->ringUsed - IP_HLEN(&ip);
        }
        if (n > 0) {
            PeekInput(info, IP_HLEN(&ip), &udp, n);
        }
        rc = Min(udp.len - 8, len);
        if (info->ringUsed < IP_HLEN(&ip) + 8 + rc) {
            rc = info->ringUsed - (IP_HLEN(&ip) + 8);
        }
        if (rc <= 0) {
            rc = 0;
        } else {
            PeekInput(info, IP_HLEN(&ip) + 8, buf, rc);
        }
        CopySockets(local, remote, &ip, &udp);
        if (!(flags & 2)) {
            DiscardInput(info, &ip);
        }
        if (result) {
            *result = udp.len - 8;
        }
        cb(info, udp.len - 8);
    } else if (flags & 4) {
        rc = -9;
        if (result) {
            *result = -9;
        }
    } else {
        if (flags & 2) {
            info->flag |= 0x400;
        } else {
            info->flag &= ~0x400;
        }
        info->recvCallback = cb;
        info->recvResult = result;
        if (result) {
            *result = -1;
        }
        info->recvBuf = buf;
        info->recvLen = len;
        info->remote_ = remote;
        info->local_ = local;
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPReceiveAsync(UDPInfo* info, void* buf, s32 len, UDPAddr* local, UDPAddr* remote, UDPCallback callback,
                    s32* result)
{
    return UDPReceiveExAsync(info, buf, len, local, remote, 0, callback, result);
}

s32 UDPReceiveEx(UDPInfo* info, void* buf, s32 len, UDPAddr* local, UDPAddr* remote, u32 flags)
{
    s32 rc;
    s32 result;
    BOOL enabled;

    rc = UDPReceiveExAsync(info, buf, len, local, remote, flags, SyncCallback, &result);
    if (rc < 0) {
        return rc;
    }
    enabled = OSDisableInterrupts();
    while (result == -1) {
        OSSleepThread(&info->queue);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

static void Cancel(UDPInfo* info, s32 code)
{
    UDPCallback callback;

    IPCancel(&info->packet);
    callback = info->sendCallback;
    if (callback) {
        info->sendCallback = NullCallback;
        if (info->sendResult) {
            if (*info->sendResult == -1) {
                *info->sendResult = code;
            }
            code = *info->sendResult;
            info->sendResult = NULL;
        }
        callback(info, code);
    }
    callback = info->recvCallback;
    if (callback) {
        info->recvCallback = NullCallback;
        if (info->recvResult) {
            if (*info->recvResult == -1) {
                *info->recvResult = code;
            }
            code = *info->recvResult;
            info->recvResult = NULL;
        }
        callback(info, code);
    }
    info->sendCallback = NULL;
    info->recvCallback = NULL;
    if (info->poll > 0) {
        __IPWakeupPollingThreads();
    }
}

s32 UDPCancel(UDPInfo* info)
{
    BOOL enabled;
    s32 rc;

    rc = 0;
    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    }
    if (rc != 0) {
        OSRestoreInterrupts(enabled);
        return rc;
    }
    Cancel(info, -16);
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPClose(UDPInfo* info)
{
    BOOL enabled;
    s32 rc;

    rc = 0;
    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    }
    if (rc != 0) {
        OSRestoreInterrupts(enabled);
        return rc;
    }
    Cancel(info, -8);
    IPClose(info);
    {
        UDPInfo* next = info->next;
        UDPInfo* prev = info->prev;

        if (next == NULL) {
            UDPInfoQueue.tail = prev;
        } else {
            next->prev = prev;
        }
        if (prev == NULL) {
            UDPInfoQueue.head = next;
        } else {
            prev->next = next;
        }
    }
    info->proto = 0;
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 UDPSetSockOpt(UDPInfo* info, s32 level, s32 optname, void* optval, u32 optlen)
{
    BOOL enabled;
    s32 rc;

    rc = -14;
    enabled = OSDisableInterrupts();
    if (info->proto != IP_PROTO_UDP) {
        rc = -12;
    } else if (level == 0xFFFF) {
        switch (optname) {
        case 4:
            if (optlen >= 4) {
                if (*(s32*)optval) {
                    info->flag |= 0x10000;
                } else {
                    info->flag &= ~0x10000;
                }
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        }
    } else if (level == 0) {
        rc = IPSetSockOpt(info, level, optname, optval, optlen);
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 __UDPPoll(UDPInfo* info)
{
    s32 events;

    if (info->proto != IP_PROTO_UDP) {
        return 0x80;
    }
    events = 0;
    if (info->sendCallback == NULL || info->sendBuf) {
        events |= 8;
    }
    if (info->ringUsed > 0) {
        events |= 1;
    }
    return events;
}



void UDPIn(IPInterface* interface, IPHeader* ip, u32 flag)
{
    UDPInfo* info;
    UDPHeader* udp;
    UDPCallback callback;
    UDPAddr* local;
    UDPAddr* remote;
    s32 n;
    IPVec vec;
    ICMPHeader icmp;

    udp = (UDPHeader*)((u8*)ip + IP_HLEN(ip));
    if (ip->len < IP_HLEN(ip) + 8 || ip->len < IP_HLEN(ip) + udp->len || udp->len < 8) {
        return;
    }
    vec.data = ip;
    vec.len = ip->len;
    if (udp->sum != 0 && UDPCheckSum(&vec, 1) != 0) {
        return;
    }
    info = IPLookupInfo(&UDPInfoQueue, ip->src, ip->dst, udp->src, udp->dst, flag);
    if (info == NULL) {
        if (flag == 0) {
            icmp.type = 3;
            icmp.code = 3;
            icmp.unk04 = 0;
            icmp.unk06 = 0;
            ICMPSendError(&icmp, (IPIcmpInterface*)interface, ip, flag);
        }
        return;
    }
    callback = info->recvCallback;
    if (callback == NULL || (info->flag & 0x400)) {
        ip = SaveInput(info, ip, flag);
        if (ip == NULL) {
            interface->unk88++;
            return;
        }
        if (info->poll > 0) {
            __IPWakeupPollingThreads();
        }
    }
    if (callback == NULL) {
        return;
    }
    n = (info->recvLen < udp->len - 8) ? info->recvLen : udp->len - 8;
    memmove(info->recvBuf, udp + 1, n);
    local = info->local_;
    remote = info->remote_;
    CopySockets(local, remote, ip, udp);
    if (info->recvResult) {
        *info->recvResult = udp->len - 8;
        info->recvResult = NULL;
    }
    info->recvCallback = NULL;
    callback(info, udp->len - 8);
}

void UDPNotify(IPHeader* ip, u8* src, s32 code)
{
    UDPHeader* udp;
    UDPInfo* info;
    UDPInfo* next;

    udp = (UDPHeader*)((u8*)ip + IP_HLEN(ip));
    if (udp->dst == 0) {
        return;
    }
    for (info = UDPInfoQueue.head, next = !info ? NULL : info->next; info;
         info = next, next = !info ? NULL : info->next) {
        if (info->local.port != 0 && info->local.port == udp->src &&
            (*(u32*)info->local.addr == IPAddrAny || *(u32*)info->local.addr == *(u32*)ip->src) &&
            info->remote.port == udp->dst && *(u32*)info->remote.addr == *(u32*)ip->dst) {
            Cancel(info, code);
        }
    }
}

BOOL UDPOnReset(BOOL final)
{
    if (UDPInfoQueue.head == NULL) {
        return TRUE;
    }
    while (UDPInfoQueue.head) {
        UDPClose(UDPInfoQueue.head);
    }
    return FALSE;
}
