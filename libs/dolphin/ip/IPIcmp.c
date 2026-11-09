#include <dolphin/types.h>
#include <dolphin/ip/ip.h>

#include <string.h>

typedef struct ICMPHeader {
    u8 type;
    u8 code;
    u16 sum;
} ICMPHeader;

typedef struct ICMPErrorHeader {
    u8 type;
    u8 code;
    u16 sum;
    u16 id;
    u16 seq;
} ICMPErrorHeader;

typedef struct TCPInfoLink TCPInfoLink;
struct TCPInfoLink {
    u8 unk00[20];
    u32 addr;
    TCPInfoLink* next;
    u8 unk1c[216];
    s32 mss;
    u8 unkf8[392];
    s32 mss2;
};

extern TCPInfoLink* TCPInfoQueue;

typedef struct IPIcmpPacket {
    u32 unk00;
    u32 unk04;
    u8 unk08[29];
    u8 unk25;
    u8 unk26[2];
    u32 unk28;
    u32 unk2c;
    u32 unk30;
    u8* data;
    u32 len;
    u8 buf[];
} IPIcmpPacket;

typedef struct IPIcmpInterface {
    u8 unk00[64];
    s32 mtu;
    u32 addr;
    u8 unk48[12];
    u32 alias;
    u8 unk58[16];
    IPIcmpPacket* (*alloc)(struct IPIcmpInterface* interface, u32 len);
    void (*free)(struct IPIcmpInterface* interface, IPIcmpPacket* packet, u32 len);
} IPIcmpInterface;

extern BOOL IPIsBroadcastAddr(IPIcmpInterface* interface, const u8* addr);
extern void IPUpdateRecordRoute(IPHeader* ip, u8* src);
extern void IPReverseSourceRoute(IPHeader* ip);
extern s32 IPOut(IPIcmpPacket* packet);
extern IPIcmpInterface* IPGetRoute(u8* addr, u8* gateway);
extern void IPRedirect(u8* addr, u8* gateway, u8* src);
extern void UDPNotify(IPHeader* ip, u8* src, s32 error);
extern void TCPNotify(IPHeader* ip, u8* src, s32 error);
extern void TCPSourceQuench(IPHeader* ip, u8* src);

static s32 ICMPIsErrorMessage(ICMPHeader* icmp)
{
    switch (icmp->type) {
    case 3:
        return 8;
    case 4:
        return 8;
    case 5:
        return 8;
    case 11:
        return 8;
    case 12:
        return 8;
    default:
        return 0;
    }
}

static u16 ICMPCheckSum(u8* data, s32 len)
{
    u32 sum;
    u16* p;

    sum = 0;
    p = (u16*)data;

    while (len > 0) {
        sum += *p++;
        len -= 2;
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return sum ^ 0xffff;
}

static void DoEchoRequest(IPIcmpInterface* interface, IPHeader* ip, u32 flags)
{
    IPIcmpPacket* packet;
    IPHeader* out;
    ICMPHeader* icmp;

    if (ip->dst[0] != 127 && *(u32*)ip->dst != interface->addr &&
        *(u32*)ip->dst != interface->alias) {
        return;
    }

    if (flags & 3 || IPIsBroadcastAddr(interface, ip->src) ||
        (ip->src[0] & 0xf0) == 0xe0 || (ip->src[0] & 0xf8) == 0xf0) {
        return;
    }

    packet = interface->alloc(interface, ip->len + 60);
    if (packet == NULL) {
        return;
    }

    packet->data = packet->buf;
    packet->len = ip->len;
    out = (IPHeader*)packet->data;
    memmove(out, ip, (ip->verlen & 0xf) << 2);
    memmove(out->src, ip->dst, sizeof(out->src));
    memmove(out->dst, ip->src, sizeof(out->dst));
    out->tos = 0;
    out->ttl = 0xff;

    if (((out->verlen & 0xf) << 2) > 20) {
        IPUpdateRecordRoute(out, out->src);
        IPReverseSourceRoute(out);
    }

    icmp = (ICMPHeader*)((u8*)out + ((out->verlen & 0xf) << 2));
    memmove(icmp, (u8*)ip + ((ip->verlen & 0xf) << 2),
            ip->len - ((ip->verlen & 0xf) << 2));
    icmp->type = 0;
    icmp->sum = 0;
    icmp->sum = ICMPCheckSum((u8*)icmp, out->len - ((out->verlen & 0xf) << 2));

    packet->unk30 = 1;
    packet->unk28 = 0;
    packet->unk2c = 0;
    packet->unk04 = 0;
    packet->unk25 = 0;

    if (IPOut(packet) < 0) {
        interface->free(interface, packet, ip->len + 60);
    }
}

static s32 ReduceMtu(s32 mtu)
{
    if (mtu > 1492) {
        return 1006;
    }
    if (mtu > 1006) {
        return 508;
    }
    return 68;
}

static void DoNotify(IPHeader* ip, u8* src, s32 error);

static void DoUnreachable(IPIcmpInterface* interface, IPHeader* ip, u32 flags)
{
    ICMPErrorHeader* icmp;
    IPHeader* original;
    TCPInfoLink* info;
    TCPInfoLink* next;
    u16 mtu;
    s32 mss;

    icmp = (ICMPErrorHeader*)((u8*)ip + ((ip->verlen & 0xf) << 2));
    original = (IPHeader*)(icmp + 1);

    switch (icmp->code) {
    case 4:
        if (icmp->seq != 0) {
            mtu = (u16)(icmp->seq < 68 ? 68 : icmp->seq);
        }
        else {
            mss = interface->mtu < original->len ? interface->mtu : original->len;
            if (mss > 1492) {
                mtu = 1006;
            }
            else if (mss > 1006) {
                mtu = 508;
            }
            else {
                mtu = 68;
            }
        }

    calc_mss:
        {
            s32 tcpMss;

            tcpMss = (u16)(interface->mtu < (u16)mtu ? interface->mtu : (u16)mtu);
            tcpMss -= 40;
            info = TCPInfoQueue;
            next = info == NULL ? NULL : info->next;
            while (info != NULL) {
                if (info->addr == *(u32*)original->dst) {
                    if (info->mss < tcpMss) {
                        mss = info->mss + 40;
                        if (mss > 1492) {
                            mtu = 1006;
                        }
                        else if (mss > 1006) {
                            mtu = 508;
                        }
                        else {
                            mtu = 68;
                        }
                        goto calc_mss;
                    }
                    info->mss = tcpMss;
                    info->mss2 = info->mss;
                }
                info = next;
                next = info == NULL ? NULL : info->next;
            }
        }

        if (original->proto == IP_PROTO_TCP) {
            break;
        }
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        DoNotify(original, ip->src, -2);
        break;
    }
}

static void DoRedirect(IPHeader* ip)
{
    ICMPErrorHeader* icmp;
    IPHeader* original;

    icmp = (ICMPErrorHeader*)((u8*)ip + ((ip->verlen & 0xf) << 2));
    original = (IPHeader*)(icmp + 1);
    if (icmp->code <= 3) {
        IPRedirect(original->dst, (u8*)icmp + sizeof(ICMPHeader), ip->src);
    }
}

static void DoSourceQuench(IPHeader* ip, u8* src)
{
    switch (ip->proto) {
    case IP_PROTO_UDP:
        UDPNotify(ip, src, -18);
        break;
    case IP_PROTO_TCP:
        TCPSourceQuench(ip, src);
        break;
    }
}

static void DoNotify(IPHeader* ip, u8* src, s32 error)
{
    switch (ip->proto) {
    case IP_PROTO_UDP:
        UDPNotify(ip, src, error);
        break;
    case IP_PROTO_TCP:
        TCPNotify(ip, src, error);
        break;
    }
}

void ICMPIn(IPIcmpInterface* interface, IPHeader* ip, u32 flags)
{
    ICMPHeader* icmp;
    IPHeader* original;
    s32 hlen = (ip->verlen & 0xf) << 2;
    s32 len;
    volatile s32 unused;
    s32 originalHLen;
    s32 errorLen;
    s32 offset;

    icmp = (ICMPHeader*)((u8*)ip + hlen);
    if (ip->len < hlen + 4) {
        return;
    }

    len = ip->len;
    if (ICMPCheckSum((u8*)icmp, len - hlen) == 0) {
        goto valid;
    }
    goto done;

valid:
    errorLen = ICMPIsErrorMessage(icmp);
    if (errorLen > 0) {
        original = (IPHeader*)((u8*)icmp + errorLen);
        offset = hlen + errorLen;
        if (len < offset + 28) {
            return;
        }
        originalHLen = (original->verlen & 0xf) << 2;
        if (originalHLen < 20) {
            return;
        }
        if (len < originalHLen + (offset + 8)) {
            return;
        }
        if (original->proto == IP_PROTO_ICMP) {
            return;
        }
        if (IPIsBroadcastAddr(interface, original->dst) ||
            (original->dst[0] & 0xf0) == 0xe0 || (original->dst[0] & 0xf8) == 0xf0) {
            return;
        }
        if (flags & 3) {
            return;
        }
        if ((original->frag << 3) & 0xfff8) {
            return;
        }
        if (original->src[0] == 127 || IPIsBroadcastAddr(interface, original->src) ||
            (original->src[0] & 0xf0) == 0xe0 || (original->src[0] & 0xf8) == 0xf0) {
            return;
        }
    }
    else {
        original = NULL;
    }

    switch (icmp->type) {
    case 0:
    case 1:
    case 2:
        break;
    case 3:
        DoUnreachable(interface, ip, flags);
        break;
    case 4:
        DoSourceQuench(original, ip->src);
        break;
    case 8:
        DoEchoRequest(interface, ip, flags);
        break;
    case 5:
        DoRedirect(ip);
        break;
    case 11:
        DoNotify(original, ip->src, -10);
        break;
    case 12:
        DoNotify(original, ip->src, -12);
        break;
    }

done:
    return;
}

s32 ICMPSendError(ICMPHeader* icmp, IPIcmpInterface* interface, IPHeader* ip, u32 flags)
{
    IPIcmpPacket* packet;
    ICMPErrorHeader* error;
    IPHeader* out;
    s32 hlen;
    s32 len;
    s32 result;

    if (icmp == NULL) {
        goto invalid;
    }
    if (ICMPIsErrorMessage(icmp) == 0) {
        goto invalid;
    }
    if (ip == NULL) {
        goto invalid;
    }

    hlen = (ip->verlen & 0xf) << 2;
    if (hlen < 20) {
        goto invalid;
    }
    if (ip->len < hlen + 8) {
        goto invalid;
    }
    if (*(u32*)ip->dst != interface->addr && *(u32*)ip->dst != interface->alias) {
        goto invalid;
    }
    goto valid;

invalid:
    return -12;

valid:
    if (ip->proto == IP_PROTO_ICMP &&
        ICMPIsErrorMessage((ICMPHeader*)((u8*)ip + hlen)) != 0) {
        return -11;
    }
    if (IPIsBroadcastAddr(interface, ip->dst) ||
        (ip->dst[0] & 0xf0) == 0xe0 || (ip->dst[0] & 0xf8) == 0xf0) {
        return -11;
    }
    if (flags & 3) {
        return -11;
    }
    if ((ip->frag << 3) & 0xfff8) {
        return -11;
    }
    if (ip->src[0] == 127 || IPIsBroadcastAddr(interface, ip->src) ||
        (ip->src[0] & 0xf0) == 0xe0 || (ip->src[0] & 0xf8) == 0xf0) {
        return -11;
    }

    interface = IPGetRoute(ip->src, NULL);
    if (interface == NULL) {
        return -2;
    }

    len = ((ip->verlen & 0xf) << 2) + 36;
    packet = interface->alloc(interface, len + 60);
    if (packet == NULL) {
        return -7;
    }

    packet->data = packet->buf;
    packet->len = len;
    out = (IPHeader*)packet->data;
    out->verlen = 0x45;
    out->tos = 0;
    out->len = len;
    out->frag = 0;
    out->ttl = 0xff;
    out->proto = IP_PROTO_ICMP;
    memmove(out->src, ip->dst, sizeof(out->src));
    memmove(out->dst, ip->src, sizeof(out->dst));

    error = (ICMPErrorHeader*)(out + 1);
    memmove(error, icmp, sizeof(*error));
    memmove((u8*)out + 28, ip, (u8)(((ip->verlen & 0xf) << 2) + 8));
    icmp = (ICMPHeader*)error;
    icmp->sum = 0;
    icmp->sum = ICMPCheckSum((u8*)icmp, out->len - ((out->verlen & 0xf) << 2));

    packet->unk30 = 1;
    packet->unk28 = 0;
    packet->unk2c = 0;
    packet->unk04 = 0;
    packet->unk25 = 0;

    result = IPOut(packet);
    if (result < 0) {
        interface->free(interface, packet, len + 60);
    }
    return result;
}
