#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

#pragma auto_inline off

typedef struct IFInterface IFInterface;
typedef struct IFDatagram IFDatagram;
typedef struct IFQueue IFQueue;
typedef struct ARPEntry ARPEntry;
typedef struct ARPEntryList ARPEntryList;
typedef struct ARPConf ARPConf;
typedef struct ARPHeader ARPHeader;

typedef void (*IFCallback)(void* param, s32 result);

struct IFQueue {
    IFDatagram* head;
    IFDatagram* tail;
};

typedef struct IFVec {
    void* data;
    s32 len;
} IFVec;

struct IFDatagram {
    IFInterface* interface;
    IFQueue* queue;
    IFDatagram* next;
    IFDatagram* prev;
    u16 type;
    u8 dst[6];
    u8 nextHop[4];
    u8 unk1C[9];
    u8 flag;
    u8 unk26[2];
    IFCallback callback;
    void* param;
    s32 nVec;
    IFVec vec[1];
};

struct IFInterface {
    u8 unk00[16];
    OSAlarm alarm;
    u8 mac[6];
    u8 unk3E[2];
    s32 mtu;
    u8 addr[4];
    u8 netmask[4];
    u8 broadcast[4];
    u8 gateway[4];
    u8 alias[4];
    u8 unk58[8];
    void (*out)(IFInterface* interface, IFDatagram* datagram);
    void (*cancel)(IFInterface* interface, IFDatagram* datagram);
    IFDatagram* (*alloc)(IFInterface* interface, s32 len);
    s32 (*hold)(IFInterface* interface, void* data, s32 len);
    void* filter[2];
    struct {
        ARPConf* head;
        ARPConf* tail;
    } claim;
};

struct ARPConf {
    IFInterface* interface;
    ARPConf* next;
    ARPConf* prev;
    u8 addr[4];
    OSAlarm alarm;
    s32 claim;
    void (*handler)(ARPConf* conf, s32 result);
};

struct ARPEntry {
    ARPEntry* next;
    ARPEntry* prev;
    OSAlarm alarm;
    s32 timeout;
    s32 state;
    u8 mac[6];
    u8 ip[4];
    IFInterface* interface;
    IFDatagram datagram;
    u8 arp[28];
    IFQueue pending;
};

struct ARPEntryList {
    ARPEntry* head;
    ARPEntry* tail;
};

struct ARPHeader {
    u16 hrd;
    u16 pro;
    u8 hln;
    u8 pln;
    u16 op;
};

#define ARP_SHA(arp) ((u8*)(arp) + 8)
#define ARP_SPA(arp) ((u8*)(arp) + 8 + (arp)->hln)
#define ARP_THA(arp) ((u8*)(arp) + 8 + (arp)->hln + (arp)->pln)
#define ARP_TPA(arp) ((u8*)(arp) + 8 + 2 * (arp)->hln + (arp)->pln)

#define ARP_STATE_FREE        0
#define ARP_STATE_INCOMPLETE  1
#define ARP_STATE_REACHABLE   2
#define ARP_STATE_REVALIDATE  3

#define ADDR(p) (*(u32*)(p))

#define DequeueItem(type, queue, item, next, prev)                                                 \
    do {                                                                                          \
        type __next = (item)->next;                                                               \
        type __prev = (item)->prev;                                                               \
        if (__next == NULL) {                                                                     \
            (queue)->tail = __prev;                                                               \
        } else {                                                                                  \
            __next->prev = __prev;                                                                \
        }                                                                                         \
        if (__prev == NULL) {                                                                     \
            (queue)->head = __next;                                                               \
        } else {                                                                                  \
            __prev->next = __next;                                                                \
        }                                                                                         \
    } while (0)

#define EnqueueHead(type, queue, item, next, prev)                                                 \
    do {                                                                                          \
        type __head = (queue)->head;                                                              \
        if (__head == NULL) {                                                                     \
            (queue)->tail = (item);                                                               \
        } else {                                                                                  \
            __head->prev = (item);                                                                \
        }                                                                                         \
        (item)->next = __head;                                                                    \
        (item)->prev = NULL;                                                                      \
        (queue)->head = (item);                                                                   \
    } while (0)

#define EnqueueTail(type, queue, item, next, prev)                                                 \
    do {                                                                                          \
        type __tail = (queue)->tail;                                                              \
        if (__tail == NULL) {                                                                     \
            (queue)->head = (item);                                                               \
        } else {                                                                                  \
            __tail->next = (item);                                                                \
        }                                                                                         \
        (item)->prev = __tail;                                                                    \
        (item)->next = NULL;                                                                      \
        (queue)->tail = (item);                                                                   \
    } while (0)

#define DequeueHead(type, queue, item, next, prev) \
    do { \
        type __next; \
        (item) = (queue)->head; \
        __next = (item)->next; \
        if (__next == NULL) (queue)->tail = NULL; \
         \
        else __next->prev = NULL; \
        (queue)->head = __next; \
    } while (0)

#define DequeueTail(type, queue, item, next, prev) \
    do { \
        type __prev; \
        (item) = (queue)->tail; \
        __prev = (item)->prev; \
        if (__prev == NULL) (queue)->head = NULL; \
         \
        else __prev->next = NULL; \
        (queue)->tail = __prev; \
    } while (0)

extern u32 IPAddrAny;

extern void ETHOut(IFInterface* interface, IFDatagram* datagram);
extern BOOL IPIsBroadcastAddr(IFInterface* interface, const u8* addr);
extern BOOL IPIsLoopbackAddr(IFInterface* interface, const u8* addr);
extern void IPRecoverGateway(const u8* addr);
extern void IPSetConfigError(IFInterface* interface, s32 error);
extern BOOL IPAutoConfig(void);

void ARPOut(IFInterface* interface, u16 op, const u8* tpa, const u8* tha, const u8* spa, ARPEntry* entry);
static void ARPCancel(ARPEntry* entry);

static u8 HwBroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static ARPEntry Cache[64];
static OSAlarm GratuitousAlarm;
static ARPEntryList Up;
static ARPEntryList Free;

inline IFDatagram* DequeuePending(ARPEntry* entry)
{
    IFDatagram* datagram;

    DequeueHead(IFDatagram*, &entry->pending, datagram, next, prev);
    return datagram;
}

static void DiscardPendingPackets(ARPEntry* entry, s32 result)
{
    IFDatagram* datagram;

    while (entry->pending.head != NULL) {
        datagram = DequeuePending(entry);
        datagram->interface = NULL;
        datagram->queue = NULL;
        if (datagram->callback) {
            datagram->callback(datagram->param, result);
        }
    }
}

static void Revalidate(ARPEntry* entry)
{
    IFInterface* interface;

    switch (entry->state) {
    case ARP_STATE_FREE:
        break;
    case ARP_STATE_REACHABLE:
        entry->state = ARP_STATE_REVALIDATE;
        entry->timeout = 1;
    case ARP_STATE_INCOMPLETE:
    case ARP_STATE_REVALIDATE:
        interface = entry->interface;
        ARPCancel(entry);
        if (entry->timeout < 32) {
            entry->timeout *= 2;
            ARPOut(interface, 1, entry->ip, (entry->state == ARP_STATE_REVALIDATE) ? entry->mac : NULL,
                   (ADDR(interface->addr) == IPAddrAny) ? interface->alias : interface->addr, entry);
        } else {
            IPRecoverGateway(entry->ip);
            DequeueItem(ARPEntry*, &Up, entry, next, prev);
            OSCancelAlarm(&entry->alarm);
            entry->state = ARP_STATE_FREE;
            EnqueueHead(ARPEntry*, &Free, entry, next, prev);
            DiscardPendingPackets(entry, -2);
        }
        break;
    }
}

static void TimeoutCallback(OSAlarm* alarm, OSContext* context)
{
    Revalidate((ARPEntry*)((u8*)alarm - 8));
}

static void SendCallback(ARPEntry* entry, s32 result)
{
    switch (entry->state) {
    case ARP_STATE_INCOMPLETE:
    case ARP_STATE_REVALIDATE:
        OSCancelAlarm(&entry->alarm);
        OSSetAlarm(&entry->alarm, OSSecondsToTicks((OSTime)entry->timeout), TimeoutCallback);
        break;
    }
}

void ARPInit(void)
{
    ARPEntry* entry;

    memset(Cache, 0, sizeof(Cache));
    Up.head = Up.tail = NULL;
    Free.head = Free.tail = NULL;
    for (entry = Cache; entry < &Cache[64]; entry++) {
        OSCreateAlarm(&entry->alarm);
        EnqueueTail(ARPEntry*, &Free, entry, next, prev);
    }
}

inline ARPEntry* LookupEntry(const u8* addr)
{
    ARPEntry* entry;
    ARPEntry* next;

    for (entry = Up.head, next = !entry ? NULL : entry->next; entry; entry = next, next = !entry ? NULL : entry->next) {
        if (ADDR(entry->ip) == ADDR(addr) &&
            (entry->state == ARP_STATE_REACHABLE || entry->state == ARP_STATE_REVALIDATE))
        {
            return entry;
        }
    }
    return NULL;
}

s32 ARPLookup(IFInterface* interface, const u8* addr, u8* mac)
{
    ARPEntry* entry;

    if ((addr[0] & 0xF0) == 0xE0) {
        mac[0] = 0x01;
        mac[1] = 0x00;
        mac[2] = 0x5E;
        mac[3] = addr[1] & 0x7F;
        mac[4] = addr[2];
        mac[5] = addr[3];
        return 3;
    }
    if (IPIsBroadcastAddr(interface, addr)) {
        memset(mac, 0xFF, 6);
        return 2;
    }
    if (IPIsLoopbackAddr(interface, addr)) {
        memmove(mac, interface->mac, 6);
        return 1;
    }
    entry = LookupEntry(addr);
    if (entry) {
        if (Up.head != entry) {
            DequeueItem(ARPEntry*, &Up, entry, next, prev);
            EnqueueHead(ARPEntry*, &Up, entry, next, prev);
        }
        memmove(mac, entry->mac, 6);
        return 0;
    }
    return -1;
}

void ARPRevalidate(const u8* addr)
{
    ARPEntry* entry;

    entry = LookupEntry(addr);
    if (entry) {
        Revalidate(entry);
    }
}

static ARPEntry* ARPAlloc(const u8* addr, BOOL create)
{
    ARPEntry* entry;
    ARPEntry* next;

    for (entry = Up.head, next = !entry ? NULL : entry->next; entry; entry = next, next = !entry ? NULL : entry->next) {
        if (ADDR(entry->ip) == ADDR(addr)) {
            return entry;
        }
    }
    if (!create) {
        return NULL;
    }
    if (Free.head) {
        DequeueHead(ARPEntry*, &Free, entry, next, prev);
    } else {
        DequeueTail(ARPEntry*, &Up, entry, next, prev);
        ARPCancel(entry);
        OSCancelAlarm(&entry->alarm);
        if (entry->state == ARP_STATE_INCOMPLETE) {
            DiscardPendingPackets(entry, -7);
        }
    }
    memset(entry, 0, sizeof(ARPEntry));
    entry->pending.head = entry->pending.tail = NULL;
    OSCreateAlarm(&entry->alarm);
    entry->timeout = 1;
    memmove(entry->ip, addr, 4);
    EnqueueHead(ARPEntry*, &Up, entry, next, prev);
    return entry;
}

void ARPHold(IFInterface* interface, IFDatagram* datagram)
{
    ARPEntry* entry;
    s32 state;
    IFCallback callback;
    void* param;
    s32 err;
    s32 n;
    IFVec vec[4];

    callback = datagram->callback;
    param = datagram->param;
    datagram->interface = NULL;
    datagram->queue = NULL;
    entry = ARPAlloc(datagram->nextHop, TRUE);
    state = entry->state;
    entry->state = ARP_STATE_INCOMPLETE;
    entry->interface = interface;
    n = datagram->nVec;
    memmove(vec, datagram->vec, n * sizeof(IFVec));
    err = 0;
    while (0 < n--) {
        err |= interface->hold(interface, vec[n].data, vec[n].len);
    }
    err |= interface->hold(interface, datagram,
                           sizeof(IFDatagram) + ((1 < datagram->nVec) ? (datagram->nVec - 1) * sizeof(IFVec) : 0));
    if (err == 0) {
        datagram->interface = interface;
        datagram->queue = &entry->pending;
        EnqueueTail(IFDatagram*, &entry->pending, datagram, next, prev);
    } else if (callback) {
        callback(param, -7);
    }
    if (state == ARP_STATE_FREE) {
        ARPOut(interface, 1, entry->ip, NULL,
               (ADDR(interface->addr) == IPAddrAny) ? interface->alias : interface->addr, entry);
    }
}

static void ARPUpdate(IFInterface* interface, ARPHeader* arp)
{
    ARPEntry* entry;
    s32 state;
    u8* spa;
    BOOL create;
    IFDatagram* datagram;

    spa = ARP_SPA(arp);
    if ((spa[0] & 0xF0) == 0xE0 || (spa[0] & 0xF8) == 0xF0 || ADDR(spa) == IPAddrAny ||
        IPIsBroadcastAddr(interface, spa))
    {
        return;
    }
    create = TRUE;
    if (ADDR(ARP_TPA(arp)) != ADDR(interface->addr) && ADDR(ARP_TPA(arp)) != ADDR(interface->alias)) {
        create = FALSE;
    }
    entry = ARPAlloc(spa, create);
    if (entry == NULL) {
        return;
    }
    ARPCancel(entry);
    OSCancelAlarm(&entry->alarm);
    entry->timeout = 1200;
    OSSetAlarm(&entry->alarm, OSSecondsToTicks((OSTime)entry->timeout), TimeoutCallback);
    state = entry->state;
    entry->state = ARP_STATE_REACHABLE;
    memmove(entry->mac, ARP_SHA(arp), 6);
    if (entry->interface != interface) {
        DiscardPendingPackets(entry, -2);
        entry->interface = interface;
    }
    if (state == ARP_STATE_INCOMPLETE) {
        while ((datagram = entry->pending.head) != NULL) {
            DequeueHead(IFDatagram*, &entry->pending, datagram, next, prev);
            datagram->queue = NULL;
            interface->out(interface, datagram);
        }
    }
}

void ARPOut(IFInterface* interface, u16 op, const u8* tpa, const u8* tha, const u8* spa, ARPEntry* entry)
{
    BOOL enabled;
    IFDatagram* datagram;
    ARPHeader* arp;

    enabled = OSDisableInterrupts();
    if (interface->out != ETHOut) {
        OSRestoreInterrupts(enabled);
        return;
    }
    if (entry) {
        datagram = &entry->datagram;
        arp = (ARPHeader*)entry->arp;
    } else {
        datagram = interface->alloc(interface, sizeof(IFDatagram) + 28);
        arp = (ARPHeader*)(datagram + 1);
    }
    if (datagram) {
        arp->hrd = 1;
        arp->pro = 0x0800;
        arp->hln = 6;
        arp->pln = 4;
        arp->op = op;
        memmove(ARP_TPA(arp), tpa, 4);
        if (tha) {
            memmove(ARP_THA(arp), tha, 6);
        } else {
            memset(ARP_THA(arp), 0, 6);
        }
        memmove(ARP_SPA(arp), spa, 4);
        memmove(ARP_SHA(arp), interface->mac, 6);
        datagram->nVec = 1;
        datagram->vec[0].data = arp;
        datagram->vec[0].len = 28;
        datagram->callback = entry ? (IFCallback)SendCallback : NULL;
        datagram->param = entry;
        datagram->type = 0x0806;
        datagram->flag = 0;
        if (tha && !(spa[0] == 169 && spa[1] == 254)) {
            memmove(datagram->dst, tha, 6);
        } else {
            memmove(datagram->dst, HwBroadcastAddr, 6);
        }
        datagram->queue = NULL;
        interface->out(interface, datagram);
    }
    OSRestoreInterrupts(enabled);
}

inline void ARPCancel(ARPEntry* entry)
{
    IFDatagram* datagram = &entry->datagram;
    IFInterface* interface = datagram->interface;

    if (interface) {
        interface->cancel(interface, datagram);
    }
}

static void GratuitousCallback(OSAlarm* alarm, OSContext* context)
{
    IFInterface* interface = (IFInterface*)((u8*)alarm - 16);

    if (ADDR(interface->addr) != IPAddrAny) {
        ARPOut(interface, 1, interface->addr, NULL, interface->addr, NULL);
    }
    if (ADDR(interface->alias) != IPAddrAny) {
        ARPOut(interface, 1, interface->alias, NULL, interface->alias, NULL);
    }
}

void ARPGratuitous(IFInterface* interface)
{
    OSCancelAlarm(&interface->alarm);
    OSSetAlarm(&interface->alarm, OSSecondsToTicks(3), GratuitousCallback);
    if (ADDR(interface->addr) != IPAddrAny) {
        ARPOut(interface, 1, interface->addr, NULL, interface->addr, NULL);
    }
    if (ADDR(interface->alias) != IPAddrAny) {
        ARPOut(interface, 1, interface->alias, NULL, interface->alias, NULL);
    }
}

static void ClaimHandler(OSAlarm* alarm, OSContext* context)
{
    ARPConf* conf = (ARPConf*)((u8*)alarm - 16);
    IFInterface* interface = conf->interface;

    conf->claim++;
    if (conf->claim <= 4) {
        ARPOut(interface, 1, conf->addr, NULL, (u8*)&IPAddrAny, NULL);
        OSSetAlarm(&conf->alarm, OSSecondsToTicks(2), ClaimHandler);
    } else {
        conf->handler(conf, 0);
    }
}

void ARPClaim(IFInterface* interface, ARPConf* conf)
{
    BOOL enabled;
    OSTime timeout;

    enabled = OSDisableInterrupts();
    if (ADDR(conf->addr) != IPAddrAny) {
        if (conf->interface == NULL) {
            conf->interface = interface;
            EnqueueTail(ARPConf*, &interface->claim, conf, next, prev);
        }
        OSCancelAlarm(&conf->alarm);
        conf->claim = 0;
        timeout = OSMillisecondsToTicks(1200);
        OSSetAlarm(&conf->alarm, timeout + OSGetTick() % OSMillisecondsToTicks(1600) - OSMillisecondsToTicks(800),
                   ClaimHandler);
    } else if (conf->interface == interface) {
        OSCancelAlarm(&conf->alarm);
        DequeueItem(ARPConf*, &interface->claim, conf, next, prev);
        conf->interface = NULL;
    }
    OSRestoreInterrupts(enabled);
}

static BOOL DetectCollision(IFInterface* interface, ARPHeader* arp)
{
    ARPConf* conf;
    ARPConf* next;

    for (conf = interface->claim.head, next = !conf ? NULL : conf->next; conf;
         conf = next, next = !conf ? NULL : conf->next)
    {
        if (IPAddrAny != ADDR(conf->addr) &&
            ((ADDR(ARP_SPA(arp)) == ADDR(conf->addr) && memcmp(ARP_SHA(arp), interface->mac, 6) != 0) ||
             (ADDR(ARP_TPA(arp)) == ADDR(conf->addr) && ADDR(ARP_SPA(arp)) == IPAddrAny)))
        {
            OSCancelAlarm(&conf->alarm);
            if (conf->handler) {
                conf->handler(conf, -111);
            }
        }
    }
    if (IPAddrAny != ADDR(interface->addr) && ADDR(ARP_SPA(arp)) == ADDR(interface->addr) &&
        memcmp(ARP_SHA(arp), interface->mac, 6) != 0)
    {
        IPSetConfigError(interface, -111);
        return TRUE;
    }
    if (IPAddrAny != ADDR(interface->alias) && ADDR(ARP_SPA(arp)) == ADDR(interface->alias) &&
        memcmp(ARP_SHA(arp), interface->mac, 6) != 0)
    {
        IPAutoConfig();
        return TRUE;
    }
    return FALSE;
}

void ARPIn(IFInterface* interface, u8* packet, s32 len)
{
    ARPHeader* arp = (ARPHeader*)(packet + 14);

    if (interface->out != ETHOut || len < 14 + 8 ||
        len < 14 + 8 + 2 * (arp->hln + arp->pln))
    {
        return;
    }
    if (arp->hrd != 1 || arp->pro != 0x0800 || arp->hln != 6 || arp->pln != 4) {
        return;
    }
    DetectCollision(interface, arp);
    ARPUpdate(interface, arp);
    if (ADDR(ARP_TPA(arp)) == IPAddrAny ||
        (ADDR(ARP_TPA(arp)) != ADDR(interface->addr) && ADDR(ARP_TPA(arp)) != ADDR(interface->alias))) {
        return;
    }
    if (arp->op == 1) {
        ARPOut(interface, 2, ARP_SPA(arp), ARP_SHA(arp), ARP_TPA(arp), NULL);
    }
}

void ARPRefresh(void)
{
    BOOL enabled;
    ARPEntry* entry;

    enabled = OSDisableInterrupts();
    while ((entry = Up.head) != NULL) {
        DequeueHead(ARPEntry*, &Up, entry, next, prev);
        if (entry->datagram.interface) {
            entry->datagram.interface->cancel(entry->datagram.interface, &entry->datagram);
        }
        OSCancelAlarm(&entry->alarm);
        entry->state = ARP_STATE_FREE;
        EnqueueHead(ARPEntry*, &Free, entry, next, prev);
        DiscardPendingPackets(entry, 0);
    }
    OSRestoreInterrupts(enabled);
}
