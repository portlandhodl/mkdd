#include <dolphin/types.h>
#include <dolphin/ip/ip.h>
#include <dolphin/os.h>

#include <stdio.h>
#include <string.h>

#define IP_CLASSD(addr) (((addr)[0] & 0xF0) == 0xE0)
#define IP_CLASSE(addr) (((addr)[0] & 0xF8) == 0xF0)

#define IP_HLEN(ip) (((ip)->verlen & 0x0F) << 2)

typedef struct IPSockAddr {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
} IPSockAddr;

typedef struct IPInfo IPInfo;
struct IPInfo {
    u8 proto;
    u8 ttl;
    u8 tos;
    u8 mttl;
    s16 poll;
    u16 flag;
    IPSockAddr local;
    IPSockAddr remote;
    IPInfo* next;
    IPInfo* prev;
};

typedef struct IPInfoQueue {
    IPInfo* head;
    IPInfo* tail;
} IPInfoQueue;

typedef struct IPVec {
    void* data;
    s32 len;
} IPVec;

typedef struct IPInterface IPInterface;

typedef struct IPPacket {
    IPInterface* interface;
    u32 unk04;
    u8 unk08[8];
    u16 type;
    u8 unk12[6];
    u8 dst[4];
    u8 unk1c[9];
    u8 unk25;
    u8 unk26[2];
    u32 unk28;
    u32 unk2c;
    s32 nvec;
    IPVec vec[1];
} IPPacket;

struct IPInterface {
    s32 type;
    u8 unk04[60];
    s32 mtu;
    u32 addr;
    u8 unk48[12];
    u32 alias;
    u8 unk58[8];
    void (*out)(IPInterface* interface, IPPacket* packet);
    void (*cancel)(IPInterface* interface, IPPacket* packet);
    void* (*alloc)(IPInterface* interface, u32 len);
    void (*free)(IPInterface* interface, void* packet, u32 len);
};

extern IPInterface __IFDefault;

extern s32 IPMulticastLookup(const u8* group, const u8* ifaddr);
extern s32 IPMulticastJoin(const u8* group, const u8* ifaddr);
extern s32 IPMulticastLeave(const u8* group, const u8* ifaddr);
extern IPInterface* IPGetRoute(const u8* addr, u8* gateway);
extern BOOL IPIsBroadcastAddr(IPInterface* interface, const u8* addr);
extern BOOL TCPLookupTimeWaitInfo(const u8* addr, u16 port, const u8* local);
extern s32 IPProcessSourceRoute(IPHeader* ip);
extern IPHeader* IPReassemble(IPInterface* interface, IPHeader* ip, s32 flag);
extern void ICMPIn(IPInterface* interface, IPHeader* ip, s32 flag);
extern void IGMPIn(IPInterface* interface, IPHeader* ip, s32 flag);
extern void UDPIn(IPInterface* interface, IPHeader* ip, s32 flag);
extern void TCPIn(IPInterface* interface, IPHeader* ip, s32 flag);
extern u16 IGMPCheckSum(void* igmp);
extern u16 UDPCheckSum(IPVec* vec, s32 nvec);
extern u16 TCPCheckSum(IPVec* vec, s32 nvec);

static u16 Id = 1;

const u8 IPAddrAny[4] = { 0, 0, 0, 0 };
const u8 IPLoopbackAddr[4] = { 127, 0, 0, 1 };
const u8 IPLimited[4] = { 255, 255, 255, 255 };

char* IPNtoA(const u8* addr)
{
    static char ascii[16];

    sprintf(ascii, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return ascii;
}

IPInfo* IPLookupInfo(IPInfoQueue* queue, const u8* src, const u8* dst, u16 srcPort, u16 dstPort, u32 flag)
{
    IPInfo* info;
    IPInfo* next;
    s32 bestScore;
    IPInfo* best;
    s32 score;
    s32 index;

    bestScore = 3;
    best = NULL;
    if (IP_CLASSD(dst)) {
        index = IPMulticastLookup(dst, IPAddrAny);
        if (index < 0) {
            return NULL;
        }
    }

    for (info = queue->head, next = !info ? NULL : info->next; info;
         info = next, next = !info ? NULL : info->next) {
        if (info->local.port == 0 || info->local.port != dstPort) {
            continue;
        }
        if (IP_CLASSD(dst)) {
            if (!(info->flag & (1 << index))) {
                continue;
            }
            if ((flag & 4) && !(info->flag & 0x8000)) {
                continue;
            }
        }

        score = 0;
        if (*(u32*)dst != *(u32*)IPAddrAny) {
            if (*(u32*)info->local.addr == *(u32*)IPAddrAny) {
                score = 1;
            } else if (*(u32*)info->local.addr != *(u32*)dst) {
                continue;
            }
        } else if (*(u32*)info->local.addr != *(u32*)IPAddrAny) {
            score = 1;
        }

        if (*(u32*)src != *(u32*)IPAddrAny) {
            if (*(u32*)info->remote.addr == *(u32*)IPAddrAny) {
                score++;
            } else if (info->remote.port != srcPort || *(u32*)info->remote.addr != *(u32*)src) {
                continue;
            }
        } else if (*(u32*)info->remote.addr != *(u32*)IPAddrAny) {
            score++;
        }

        if (score < bestScore) {
            best = info;
            bestScore = score;
            if (score == 0) {
                break;
            }
        }
    }
    return best;
}

BOOL __IPIsMember(IPInfoQueue* queue, IPInfo* info)
{
    IPInfo* p;
    IPInfo* next;

    for (p = queue->head, next = !p ? NULL : p->next; p; p = next, next = !p ? NULL : p->next) {
        if (p == info) {
            return TRUE;
        }
    }
    return FALSE;
}

s32 IPBind(IPInfoQueue* queue, IPInfo* info, IPSockAddr* addr, BOOL reuse)
{
    IPInfo* p;
    IPInfo* next;
    u32 a;

    if (addr->len != sizeof(IPSockAddr) || addr->family != 2 || addr->port == 0 || IP_CLASSE(addr->addr)) {
        return -12;
    }
    if (addr->port != 0 && (a = *(u32*)addr->addr) != *(u32*)IPAddrAny && a != *(u32*)IPLoopbackAddr && a != __IFDefault.addr && a != __IFDefault.alias &&
            !IP_CLASSD(addr->addr)) {
        return -13;
    }

    for (p = queue->head, next = !p ? NULL : p->next; p; p = next, next = !p ? NULL : p->next) {
        if (p != info && p->local.port == addr->port && *(u32*)p->local.addr == a) {
            if (!reuse || (p->remote.port == info->remote.port &&
                           *(u32*)p->remote.addr == *(u32*)info->remote.addr)) {
                return -5;
            }
        }
    }
    memmove(&info->local, addr, sizeof(IPSockAddr));
    return 0;
}

u16 IPGetAnonPort(IPInfoQueue* queue, u16* port)
{
    u16 anon;
    IPInfo* info;
    IPInfo* next;
    s32 i;

    i = 0;
    if (*port < 1024 || 5000 < *port) {
        *port = 1024;
    }
again:
    anon = (*port)++;
    if (5000 < *port) {
        *port = 1024;
    }
    for (info = queue->head, next = !info ? NULL : info->next; info;
         info = next, next = !info ? NULL : info->next) {
        if (info->local.port == anon) {
            if (++i <= 5000 - 1024) {
                goto again;
            }
            return 0;
        }
    }
    return anon;
}

s32 IPConnect(IPInfoQueue* queue, IPInfo* info, IPSockAddr* addr, u16* port)
{
    IPInterface* interface;
    u8* local;
    IPInfo* next;
    IPInfo* p;
    u16 lport;

    if (addr == NULL || addr->len != sizeof(IPSockAddr) || addr->family != 2 || addr->port == 0 ||
        IP_CLASSE(addr->addr)) {
        return -12;
    }
    if (*(u32*)addr->addr == *(u32*)IPAddrAny) {
        return -13;
    }
    interface = IPGetRoute(addr->addr, NULL);
    if (interface == NULL) {
        return -2;
    }
    if (info->proto == 6 && (IP_CLASSD(addr->addr) || IPIsBroadcastAddr(interface, addr->addr))) {
        return -13;
    }

    if (*(u32*)info->local.addr == *(u32*)IPAddrAny) {
        if (addr->addr[0] == 127) {
            local = (u8*)IPLoopbackAddr;
        } else if (memcmp(addr->addr, &interface->alias, 2) == 0 || interface->addr == *(u32*)IPAddrAny) {
            local = (u8*)&interface->alias;
        } else {
            local = (u8*)&interface->addr;
        }
    } else {
        local = info->local.addr;
    }

    lport = info->local.port;
    if (lport == 0) {
    again:
        info->local.port = IPGetAnonPort(queue, port);
        lport = info->local.port;
        if (lport == 0) {
            return -7;
        }
        if (info->proto == 6 && TCPLookupTimeWaitInfo(addr->addr, addr->port, local)) {
            goto again;
        }
    } else {
        for (p = queue->head, next = !p ? NULL : p->next; p; p = next, next = !p ? NULL : p->next) {
            if (p != info && p->local.port == lport && p->remote.port == info->remote.port &&
                *(u32*)p->local.addr == *(u32*)local && *(u32*)p->remote.addr == *(u32*)info->remote.addr) {
                return -5;
            }
        }
        if (info->proto == 6 && TCPLookupTimeWaitInfo(addr->addr, addr->port, local)) {
            return -5;
        }
    }

    memmove(&info->remote, addr, sizeof(IPSockAddr));
    if (*(u32*)info->local.addr == *(u32*)IPAddrAny) {
        memmove(info->local.addr, local, 4);
    }
    return 0;
}

s32 IPGetRemoteSocket(IPInfo* info, u8* addr)
{
    memcpy(addr, &info->remote, sizeof(IPSockAddr));
    return 0;
}

s32 IPGetSockOpt(IPInfo* info, s32 level, s32 optname, void* optval, u32* optlen)
{
    s32 rc;
    BOOL enabled;

    rc = -14;
    enabled = OSDisableInterrupts();
    if (level == 0) {
        switch (optname) {
        case 7:
            if (*optlen >= 4) {
                *(s32*)optval = info->tos;
                *optlen = 4;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 8:
            if (*optlen >= 4) {
                *(s32*)optval = info->ttl;
                *optlen = 4;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 9:
            if (*optlen >= 1) {
                *(u8*)optval = (info->flag & 0x8000) ? 1 : 0;
                *optlen = 1;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 10:
            if (*optlen >= 1) {
                *(u8*)optval = info->mttl;
                *optlen = 1;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

s32 IPSetSockOpt(IPInfo* info, s32 level, s32 optname, void* optval, u32 optlen)
{
    s32 rc;
    BOOL enabled;

    rc = -14;
    enabled = OSDisableInterrupts();
    if (level == 0) {
        switch (optname) {
        case 7:
            if (optlen >= 4) {
                info->tos = (*(s32*)optval < 255) ? *(s32*)optval : 255;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 8:
            if (optlen >= 4) {
                info->ttl = (*(s32*)optval < 255) ? *(s32*)optval : 255;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 9:
            if (optlen >= 1) {
                if (*(u8*)optval) {
                    info->flag |= 0x8000;
                } else {
                    info->flag &= ~0x8000;
                }
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 10:
            if (optlen >= 1) {
                info->mttl = *(u8*)optval;
                rc = 0;
            } else {
                rc = -12;
            }
            break;
        case 11:
            if (optlen >= 8) {
                u8* mreq = (u8*)optval;
                s32 index;

                index = IPMulticastLookup(mreq, mreq + 4);
                if (index >= 0) {
                    if (!(info->flag & (1 << index))) {
                        index = IPMulticastJoin(mreq, mreq + 4);
                        info->flag |= 1 << index;
                    }
                    rc = 0;
                } else {
                    rc = IPMulticastJoin(mreq, mreq + 4);
                    if (rc >= 0) {
                        info->flag |= 1 << rc;
                        rc = 0;
                    }
                }
            } else {
                rc = -12;
            }
            break;
        case 12:
            if (optlen >= 8) {
                u8* mreq = (u8*)optval;
                s32 index;

                rc = IPMulticastLookup(mreq, mreq + 4);
                if (rc >= 0) {
                    if (info->flag & (1 << rc)) {
                        index = IPMulticastLeave(mreq, mreq + 4);
                        rc = 0;
                        info->flag &= ~(1 << index);
                    } else {
                        rc = -4;
                    }
                }
            } else {
                rc = -12;
            }
            break;
        }
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

u16 IPCheckSum(IPHeader* ip)
{
    u32 sum;
    u16* p;
    s32 len;

    sum = 0;
    p = (u16*)ip;
    len = IP_HLEN(ip);
    while (len > 0) {
        sum += *p++;
        len -= 2;
    }
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return sum ^ 0xffff;
}

void IPIn(IPInterface* interface, IPHeader* ip, s32 len, s32 flag)
{
    BOOL bcast;

    bcast = IPIsBroadcastAddr(interface, ip->dst);
    if (len < 20 || len < ip->len) {
        return;
    }
    if ((flag & 1) && !bcast) {
        return;
    }
    if ((flag & 2) && !IP_CLASSD(ip->dst)) {
        return;
    }
    if ((ip->verlen >> 4) != 4 || ip->len < IP_HLEN(ip)) {
        return;
    }
    if (IPCheckSum(ip) != 0) {
        return;
    }
    if (IP_CLASSD(ip->src) || (*(u32*)ip->src != *(u32*)IPAddrAny && IPIsBroadcastAddr(interface, ip->src))) {
        return;
    }
    if (IP_CLASSE(ip->dst) || IP_CLASSE(ip->src)) {
        return;
    }
    if (IPProcessSourceRoute(ip) < 0) {
        return;
    }
    if (interface->addr != *(u32*)IPAddrAny && !IP_CLASSD(ip->dst) && !bcast && ip->dst[0] != 127 &&
        *(u32*)ip->dst != interface->addr && *(u32*)ip->dst != interface->alias) {
        return;
    }
    if ((ip->frag & 0x2000) || ((ip->frag & 0x1FFF) << 3)) {
        ip = IPReassemble(interface, ip, flag);
        if (ip == NULL) {
            return;
        }
    }

    switch (ip->proto) {
    case IP_PROTO_ICMP:
        ICMPIn(interface, ip, flag);
        break;
    case 2:
        IGMPIn(interface, ip, flag);
        break;
    case IP_PROTO_UDP:
        UDPIn(interface, ip, flag);
        break;
    case IP_PROTO_TCP:
        if (!(flag & 3)) {
            TCPIn(interface, ip, flag);
        }
        break;
    }
    ip->verlen = 0;
}

s32 IPOut(IPPacket* packet)
{
    IPHeader* ip;
    IPInterface* interface;
    u8* hdr;

    ip = (IPHeader*)packet->vec[0].data;
    ip->id = Id++;
    if (IP_CLASSD(ip->dst)) {
        interface = &__IFDefault;
        memmove(packet->dst, ip->dst, 4);
    } else {
        interface = IPGetRoute(ip->dst, packet->dst);
        if (interface == NULL) {
            return -2;
        }
    }
    if (interface->mtu < ip->len) {
        return -17;
    }
    if (ip->src[0] != 127 && interface->addr != *(u32*)ip->src && interface->alias != *(u32*)ip->src) {
        return -19;
    }
    ip->sum = 0;
    ip->sum = IPCheckSum(ip);

    switch (ip->proto) {
    case 2:
        hdr = (u8*)ip + IP_HLEN(ip);
        *(u16*)(hdr + 2) = 0;
        *(u16*)(hdr + 2) = IGMPCheckSum(hdr);
        break;
    case IP_PROTO_UDP:
        hdr = (u8*)ip + IP_HLEN(ip);
        *(u16*)(hdr + 6) = 0;
        *(u16*)(hdr + 6) = UDPCheckSum(packet->vec, packet->nvec);
        if (*(u16*)(hdr + 6) == 0) {
            *(u16*)(hdr + 6) = 0xFFFF;
        }
        break;
    case IP_PROTO_TCP:
        hdr = (u8*)ip + IP_HLEN(ip);
        *(u16*)(hdr + 16) = 0;
        *(u16*)(hdr + 16) = TCPCheckSum(packet->vec, packet->nvec);
        break;
    }

    packet->type = 0x0800;
    interface->out(interface, packet);
    return 0;
}

void IPCancel(IPPacket* packet)
{
    IPInterface* interface = packet->interface;

    if (interface != NULL) {
        interface->cancel(interface, packet);
    }
}
