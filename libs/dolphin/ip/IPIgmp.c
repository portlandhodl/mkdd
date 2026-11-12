#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

#define IGMP_TABLE_SIZE 4

#define IGMP_QUERY  1
#define IGMP_REPORT 2

#define IP_CLASSD(addr) (((addr)[0] & 0xF0) == 0xE0)

#define IP_HLEN(ip) (((ip)->verlen & 0x0F) << 2)

typedef struct IGMPHeader {
    u8 verType;
    u8 unused;
    u16 sum;
    u8 group[4];
} IGMPHeader;

typedef struct IGMPEntry {
    u8 group[4];
    u8 ifaddr[4];
    OSAlarm alarm;
    s16 refs;
} IGMPEntry;

typedef struct IGMPPacket {
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
} IGMPPacket;

typedef struct IGMPInterface IGMPInterface;
struct IGMPInterface {
    s32 type;
    u8 unk04[64];
    u32 addr;
    u8 unk48[12];
    u32 alias;
    u8 unk58[16];
    IGMPPacket* (*alloc)(IGMPInterface* interface, u32 len);
    void (*free)(IGMPInterface* interface, IGMPPacket* packet, u32 len);
};

typedef struct IGMPInfo {
    u8 unk00[6];
    u16 mcast;
} IGMPInfo;

extern u32 IPAddrAny;
extern IGMPInterface __IFDefault;

extern s32 IPOut(IGMPPacket* packet);
extern void ETHAddMulticastAddress(u8* addr);

static IGMPEntry IgmpTable[IGMP_TABLE_SIZE];

const u8 IPAllHosts[4] = { 224, 0, 0, 1 };

u16 IGMPCheckSum(IGMPHeader* igmp)
{
    u32 sum;
    u16* p;

    sum = 0;
    for (p = (u16*)igmp; p < (u16*)((u8*)igmp + sizeof(IGMPHeader)); p++) {
        sum += *p;
    }
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return sum ^ 0xffff;
}

static inline IGMPEntry* LookupEntry(const u8* group)
{
    IGMPEntry* e;

    if (*(u32*)group == IPAddrAny) {
        return NULL;
    }
    for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
        if (*(u32*)group == *(u32*)e->group) {
            return e;
        }
    }
    return NULL;
}

static inline OSTime GetRandomValue2(IGMPEntry* e)
{
    OSTime t;

    t = OSGetTime();
    t += *(u32*)e->group;
    t = OSMicrosecondsToTicks(t);
    t %= OSSecondsToTicks((OSTime)10);
    return (t >= 0) ? t : -t;
}

static inline OSTime GetRandomValue(IGMPEntry* e)
{
    OSTime t;
    OSTime d;

    t = OSGetTime();
    t += *(u32*)e->group;
    t = OSMicrosecondsToTicks(t);
    t %= OSSecondsToTicks((OSTime)10);
    d = (t >= 0) ? t : -t;
    return d;
}

static inline void SetMulticastAddress(u8* mac, const u8* group)
{
    mac[0] = 0x01;
    mac[1] = 0x00;
    mac[2] = 0x5E;
    mac[3] = group[1] & 0x7F;
    mac[4] = group[2];
    mac[5] = group[3];
}

static void IGMPOut(u8 type, u8* group)
{
    IGMPInterface* interface;
    IGMPEntry* e;
    IPHeader* ip;
    IGMPPacket* packet;
    IGMPHeader* igmp;

    interface = &__IFDefault;
    e = LookupEntry(group);
    if (e == NULL) {
        return;
    }

    packet = interface->alloc(interface, sizeof(IGMPPacket) + sizeof(IPHeader) + sizeof(IGMPHeader));
    if (packet == NULL) {
        return;
    }

    packet->unk28 = 0;
    packet->unk04 = 0;
    packet->unk00 = 0;

    ip = (IPHeader*)packet->buf;
    ip->verlen = 0x45;
    ip->tos = 0;
    ip->len = IP_HLEN(ip) + sizeof(IGMPHeader);
    ip->ttl = 1;
    ip->proto = 2;
    ip->frag = 0;

    igmp = (IGMPHeader*)((u8*)ip + sizeof(IPHeader));
    igmp->verType = 0x10 | (type & 0x0F);
    igmp->unused = 0;

    switch (type) {
    case IGMP_QUERY:
        memmove(igmp->group, &IPAddrAny, 4);
        memmove(ip->dst, IPAllHosts, 4);
        break;
    case IGMP_REPORT:
        memmove(igmp->group, group, 4);
        memmove(ip->dst, group, 4);
        break;
    default:
        OSPanic("IPIgmp.c", 169, "IGMPOut() fatal error.");
        break;
    }

    if (*(u32*)e->ifaddr != IPAddrAny) {
        memmove(ip->src, e->ifaddr, 4);
    } else if (interface->addr == IPAddrAny) {
        memmove(ip->src, &interface->alias, 4);
    } else {
        memmove(ip->src, &interface->addr, 4);
    }

    packet->unk30 = 1;
    packet->data = (u8*)ip;
    packet->len = IP_HLEN(ip) + sizeof(IGMPHeader);
    packet->unk2c = 0;
    packet->unk25 = 0;

    if (IPOut(packet) < 0) {
        interface->free(interface, packet, sizeof(IGMPPacket) + sizeof(IPHeader) + sizeof(IGMPHeader));
    }
}

static void Timeout(OSAlarm* alarm, OSContext* context)
{
    IGMPEntry* e = (IGMPEntry*)((u8*)alarm - 8);

    IGMPOut(IGMP_REPORT, e->group);
}

void IGMPIn(IGMPInterface* interface, IPHeader* ip)
{
    IGMPHeader* igmp;
    IGMPEntry* e;

    igmp = (IGMPHeader*)((u8*)ip + IP_HLEN(ip));
    if (ip->len < IP_HLEN(ip) + 8) {
        return;
    }
    if (ip->ttl != 1) {
        return;
    }
    if (*(u32*)ip->src == interface->addr || *(u32*)ip->src == interface->alias) {
        return;
    }
    if ((igmp->verType >> 4) != 1) {
        return;
    }
    if (IGMPCheckSum(igmp) != 0) {
        return;
    }
    if (*(u32*)igmp->group == *(u32*)IPAllHosts) {
        return;
    }

    switch (igmp->verType & 0x0F) {
    case IGMP_QUERY:
        if (*(u32*)igmp->group != IPAddrAny) {
            break;
        }
        for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
            if (*(u32*)e->group != IPAddrAny && e->alarm.handler == NULL) {
                OSSetAlarm(&e->alarm, GetRandomValue(e), Timeout);
            }
        }
        break;
    case IGMP_REPORT:
        e = LookupEntry(igmp->group);
        if (e != NULL && *(u32*)ip->dst == *(u32*)e->group) {
            OSCancelAlarm(&e->alarm);
        }
        break;
    }
}

void IGMPInit(IGMPInterface* interface)
{
    u8 mac[6];

    switch (interface->type) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        SetMulticastAddress(mac, IPAllHosts);
        ETHAddMulticastAddress(mac);
        break;
    }
}

BOOL IGMPOnReset(BOOL final)
{
    IGMPEntry* e;

    if (!final) {
        for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
            if (*(u32*)e->group != IPAddrAny) {
                OSCancelAlarm(&e->alarm);
                memmove(e->group, &IPAddrAny, 4);
            }
        }
    }
    return TRUE;
}

s32 IPMulticastLookup(const u8* group, const u8* ifaddr)
{
    IGMPEntry* e;

    if (!IP_CLASSD(group)) {
        return -12;
    }
    if (*(u32*)group == *(u32*)IPAllHosts) {
        return 4;
    }
    for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
        if (*(u32*)e->group == *(u32*)group && *(u32*)e->ifaddr == *(u32*)ifaddr) {
            return e - IgmpTable;
        }
    }
    return -4;
}

s32 IPMulticastJoin(const u8* group, const u8* ifaddr)
{
    IGMPEntry* e;
    IGMPEntry* empty;
    IGMPInterface* interface;
    u8 mac[6];

    empty = NULL;
    interface = &__IFDefault;
    if (!IP_CLASSD(group)) {
        return -12;
    }
    if (!(*(u32*)ifaddr == IPAddrAny || *(u32*)ifaddr == interface->addr ||
          *(u32*)ifaddr == interface->alias)) {
        return -12;
    }
    if (*(u32*)group == *(u32*)IPAllHosts) {
        return 4;
    }

    for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
        if (*(u32*)e->group == *(u32*)group && *(u32*)e->ifaddr == *(u32*)ifaddr) {
            e->refs++;
            return e - IgmpTable;
        }
        if (empty == NULL && *(u32*)e->group == IPAddrAny) {
            empty = e;
        }
    }
    if (empty == NULL) {
        return -7;
    }

    memmove(empty->group, group, 4);
    memmove(empty->ifaddr, ifaddr, 4);
    OSCreateAlarm(&empty->alarm);
    empty->refs = 1;

    switch (interface->type) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        SetMulticastAddress(mac, group);
        ETHAddMulticastAddress(mac);
        break;
    }

    IGMPOut(IGMP_REPORT, empty->group);
    OSSetAlarm(&empty->alarm, GetRandomValue2(empty), Timeout);
    return empty - IgmpTable;
}

s32 IPMulticastLeave(const u8* group, const u8* ifaddr)
{
    IGMPEntry* e;

    if (!IP_CLASSD(group)) {
        return -12;
    }
    if (*(u32*)group == *(u32*)IPAllHosts) {
        return 4;
    }
    for (e = IgmpTable; e < &IgmpTable[IGMP_TABLE_SIZE]; e++) {
        if (*(u32*)e->group == *(u32*)group && *(u32*)e->ifaddr == *(u32*)ifaddr) {
            e->refs--;
            if (e->refs <= 0) {
                OSCancelAlarm(&e->alarm);
                memmove(e->group, &IPAddrAny, 4);
            }
            return e - IgmpTable;
        }
    }
    return -4;
}

s32 IPClose(IGMPInfo* info)
{
    IGMPEntry* e;
    s32 i;

    for (i = 0; i < IGMP_TABLE_SIZE; i++) {
        if (info->mcast & (1 << i)) {
            e = &IgmpTable[i];
            IPMulticastLeave(e->group, e->ifaddr);
            info->mcast &= ~(1 << i);
        }
    }
    return 0;
}
