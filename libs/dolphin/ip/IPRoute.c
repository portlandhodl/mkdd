#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/ip/ip.h>
#include <string.h>

typedef struct RouteEntry RouteEntry;
struct RouteEntry {
    RouteEntry* next;
    RouteEntry* prev;
    u8 dst[4];
    u8 mask[4];
    u8 gateway[4];
    u32 flags;
    OSTime time;
    void* interface;
};

typedef struct RouteList {
    RouteEntry* head;
    RouteEntry* tail;
} RouteList;

extern u8 __IFDefault[0xA8];
extern u32 IPAddrAny;
extern u32 IPLimited;
extern u32 IPLoopbackAddr;
extern void ARPGratuitous(void*);

static RouteEntry RoutingTable[32];
static BOOL Initialized;
static RouteList Up;
static RouteList Free;

BOOL IPRefreshRoute(void);
static BOOL AddRoute(u8* dst, u8* mask, u8* gateway, u32 flags);

static inline BOOL IsValidNet(const u8* dst, const u8* mask)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (dst[i] != (dst[i] & mask[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static inline int CountBits(const u8* p)
{
    int i;
    int n = 0;
    u8 mask;

    for (i = 0; i < 4; i++) {
        for (mask = 0x80; mask != 0; mask >>= 1) {
            if (p[i] & mask) {
                n++;
            }
        }
    }
    return n;
}

void IPGetMacAddr(u8* interface, u8* mac)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(mac, p + 0x38, 6);
    OSRestoreInterrupts(enabled);
}

BOOL IPGetNetmask(u32 interface, u32* netmask)
{
    u8* p;
    int enabled;

    p = (interface != 0) ? (u8*)interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(netmask, p + 0x48, 4);
    OSRestoreInterrupts(enabled);
}

BOOL IPGetAddr(u32 interface, u32* addr)
{
    u8* p;
    int enabled;

    p = (interface != 0) ? (u8*)interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(addr, p + 0x44, 4);
    OSRestoreInterrupts(enabled);
}

BOOL IPGetAlias(u32 interface, u32* alias)
{
    u8* p;
    int enabled;

    p = (interface != 0) ? (u8*)interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(alias, p + 0x54, 4);
    OSRestoreInterrupts(enabled);
}

void IPGetMtu(u8* interface, u32* mtu)
{
    int enabled;

    interface = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    *mtu = *(u32*)(interface + 0x40);
    OSRestoreInterrupts(enabled);
}

void IPGetLinkState(u8* interface, u32* state)
{
    int enabled;

    interface = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    *state = *(u32*)(interface + 0x04);
    OSRestoreInterrupts(enabled);
}

void IPSetBroadcastAddr(u8* interface, u8* addr)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(p + 0x4C, addr ? addr : (u8*)&IPLimited, 4);
    OSRestoreInterrupts(enabled);
}

void IPSetNetmask(u8* interface, u8* addr)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(p + 0x48, addr ? addr : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);
}

void IPSetAddr(u8* interface, u8* addr)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(p + 0x44, addr ? addr : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);
}

void IPSetGateway(u8* interface, u8* addr)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(p + 0x50, addr ? addr : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);
}

void IPSetAlias(u8* interface, u8* alias)
{
    u8* p;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    memcpy(p + 0x54, alias ? alias : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);
}

void IPSetMtu(u8* interface, s32 mtu)
{
    int enabled;

    interface = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    *(u32*)(interface + 0x40) = mtu < 68 ? 68 : mtu;
    OSRestoreInterrupts(enabled);
}

static BOOL Hit(RouteEntry* route, const u8* addr)
{
    int i;
    int a;

    for (i = 0; i < 4; i++) {
        a = addr[i];
        if (route->dst[i] != (a & route->mask[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL IPIsBroadcastAddr(u8* interface, const u8* addr)
{
    u32 mask;
    u32 a;
    u32 classMask;
    u8* p;

    p = (interface != NULL) ? interface : __IFDefault;
    if ((addr[0] & 0xF0) == 0xE0 || (addr[0] & 0xF8) == 0xF0) {
        return FALSE;
    }
    a = *(u32*)addr;
    if (a == IPLimited || a == IPAddrAny || a == *(u32*)(p + 0x4C)) {
        return TRUE;
    }
    mask = *(u32*)(p + 0x48);
    if ((classMask = *(u32*)(p + 0x44) & mask) != 0) {
        if (classMask == (a & mask) && (~mask & a) == ~mask) {
            return TRUE;
        }
        if ((p[0x44] & 0x80) == 0) {
            mask = 0xFF000000;
        } else if ((p[0x44] & 0xC0) == 0x80) {
            mask = 0xFFFF0000;
        } else if ((p[0x44] & 0xE0) == 0xC0) {
            mask = 0xFFFFFF00;
        }
        if ((*(u32*)(p + 0x44) & mask) == (a & mask) && (~mask & a) == ~mask) {
            return TRUE;
        }
    }
    if ((*(u32*)(p + 0x54) & 0xFFFF0000) != 0 &&
        (*(u32*)(p + 0x54) & 0xFFFF0000) == (a & 0xFFFF0000) &&
        (a & 0xFFFF) == 0xFFFF) {
        return TRUE;
    }
    return FALSE;
}

BOOL IPIsLoopbackAddr(u32 interface, const u8* addr)
{
    if (addr[0] == 127) {
        return TRUE;
    }
    return FALSE;
}

BOOL IPIsLocalAddr(u32 interface, const u8* addr)
{
    u32 iface;
    BOOL local = FALSE;
    u8 aliasMask[4] = { 255, 255, 0, 0 };
    int enabled;

    iface = (interface != 0) ? interface : (u32)__IFDefault;
    enabled = OSDisableInterrupts();
    if (*(u32*)((u8*)iface + 0x44) != IPAddrAny) {
        if ((*(u32*)addr & *(u32*)((u8*)iface + 0x48)) ==
            (*(u32*)((u8*)iface + 0x44) & *(u32*)((u8*)iface + 0x48))) {
            local = TRUE;
        }
    }
    if (*(u32*)((u8*)iface + 0x54) != IPAddrAny) {
        if ((*(u32*)addr & *(u32*)aliasMask) == (*(u32*)((u8*)iface + 0x54) & *(u32*)aliasMask)) {
            local = TRUE;
        }
    }
    OSRestoreInterrupts(enabled);
    return local;
}

void* IPGetRoute(u8* addr, u8* gateway)
{
    RouteEntry* route;
    RouteEntry* next;

    for (route = Up.head, next = !route ? NULL : route->next; route;
         route = next, next = !route ? NULL : route->next) {
        if (Hit(route, addr)) {
            if (gateway != NULL) {
                if (route->flags & 1) {
                    memmove(gateway, route->gateway, 4);
                } else {
                    memmove(gateway, addr, 4);
                }
            }
            route->time = __OSGetSystemTime();
            return route->interface;
        }
    }
    return NULL;
}

static BOOL AddRoute(u8* dst, u8* mask, u8* gateway, u32 flags)
{
    RouteEntry* route;
    RouteEntry* next;
    RouteEntry* p;
    RouteEntry* q;
    int bits;
    int i;

    if (!IsValidNet(dst, mask)) {
        return FALSE;
    }
    if (*(u32*)gateway == IPAddrAny) {
        flags &= ~1;
    } else {
        flags |= 1;
    }

    for (p = Up.head, next = !p ? NULL : p->next; p;
         p = next, next = !p ? NULL : p->next) {
        for (i = 0; i < 4; i++) {
            if ((p->dst[i] & p->mask[i]) != (dst[i] & mask[i])) {
                break;
            }
        }
        if (i < 4) {
            continue;
        }
        if ((flags & 1) && !(p->flags & 6)) {
            if (*(u32*)gateway == *(u32*)p->gateway) {
                return TRUE;
            }
            continue;
        }
        memmove(p->gateway, gateway, 4);
        memmove(p->mask, mask, 4);
        if (!(p->flags & 2)) {
            flags &= ~2;
        }
        p->flags = flags;
        p->time = __OSGetSystemTime();
        p->interface = __IFDefault;
        return TRUE;
    }

    if (Free.head == NULL) {
        route = NULL;
        for (p = Up.head, next = !p ? NULL : p->next; p; p = next, next = !p ? NULL : p->next) {
            if (p->flags & 2) {
                if (route == NULL) {
                    route = p;
                } else if (p->time < route->time) {
                    route = p;
                }
            }
        }
        if (route == NULL) {
            return FALSE;
        }
        p = route->next;
        q = route->prev;
        if (p == NULL) {
            Up.tail = q;
        } else {
            p->prev = q;
        }
        if (q == NULL) {
            Up.head = p;
        } else {
            q->next = p;
        }
    } else {
        route = Free.head;
        p = route->next;
        if (p == NULL) {
            Free.tail = NULL;
        } else {
            p->prev = NULL;
        }
        Free.head = p;
    }

    memmove(route->dst, dst, 4);
    memmove(route->mask, mask, 4);
    memmove(route->gateway, gateway, 4);
    flags &= ~4;
    route->flags = flags;
    route->time = __OSGetSystemTime();
    route->interface = __IFDefault;

    bits = CountBits(mask);
    for (p = Up.head, next = !p ? NULL : p->next; p; p = next, next = !p ? NULL : p->next) {
        if (CountBits(p->mask) < bits) {
            route->prev = p->prev;
            route->next = p;
            p->prev = route;
            if (route->prev == NULL) {
                Up.head = route;
            } else {
                route->prev->next = route;
            }
            return TRUE;
        }
    }
    q = Up.tail;
    if (q == NULL) {
        Up.head = route;
    } else {
        q->next = route;
    }
    route->prev = q;
    route->next = NULL;
    Up.tail = route;
    return TRUE;
}

BOOL IPRedirect(u8* addr, u8* gateway, u8* src)
{
    u8 gw[4];
    u8* interface;

    if (*(u32*)gateway == IPAddrAny) {
        return FALSE;
    }
    interface = IPGetRoute(addr, gw);
    if (interface == NULL) {
        return FALSE;
    }
    if ((*(u32*)gateway & *(u32*)(interface + 0x48)) != (*(u32*)(interface + 0x44) & *(u32*)(interface + 0x48))) {
        return FALSE;
    }
    if (IPIsBroadcastAddr(interface, gateway)) {
        return FALSE;
    }
    if (*(u32*)gw != *(u32*)src) {
        return FALSE;
    }
    if (IPGetRoute(gateway, NULL) != IPGetRoute(src, NULL)) {
        return FALSE;
    }
    if (*(u32*)gateway == *(u32*)(__IFDefault + 0x44) || *(u32*)gateway == *(u32*)(__IFDefault + 0x54)) {
        return FALSE;
    }
    return AddRoute(addr, (u8*)&IPLimited, gateway, 7);
}

BOOL IPAddRoute(u8* dst, u8* mask, u8* gateway)
{
    BOOL rc;
    int enabled;

    enabled = OSDisableInterrupts();
    rc = AddRoute(dst, mask, gateway, 0);
    OSRestoreInterrupts(enabled);
    return rc;
}

void IPInitRoute(u8* addr, u8* netmask, u8* gateway)
{
    RouteEntry* route;
    RouteEntry* end;
    RouteEntry* prev;
    u8 broadcast[4];
    int enabled;
    int i;

    enabled = OSDisableInterrupts();
    if (!Initialized) {
        Initialized = TRUE;
        memset(RoutingTable, 0, sizeof(RoutingTable));
        Up.head = Up.tail = NULL;
        Free.head = Free.tail = NULL;
        for (route = RoutingTable, end = RoutingTable + 32; route < end; route++) {
            prev = Free.tail;
            if (prev == NULL) {
                Free.head = route;
            } else {
                prev->next = route;
            }
            route->prev = prev;
            route->next = NULL;
            Free.tail = route;
        }
    }
    OSRestoreInterrupts(enabled);

    IPSetAddr(__IFDefault, addr);
    IPSetNetmask(__IFDefault, netmask);
    IPSetGateway(__IFDefault, gateway);
    if (addr != NULL && netmask != NULL) {
        for (i = 0; i < 4; i++) {
            broadcast[i] = addr[i] & netmask[i];
            broadcast[i] |= ~netmask[i];
        }
        IPSetBroadcastAddr(__IFDefault, broadcast);
    }
    IPRefreshRoute();
}

BOOL IPRefreshRoute(void)
{
    int enabled;
    u8 loopback[4] = { 127, 0, 0, 0 };
    u8 loopbackMask[4] = { 255, 0, 0, 0 };
    u8 aliasMask[4] = { 255, 255, 0, 0 };
    u8 net[4];
    RouteEntry* route;
    RouteEntry* next;
    RouteEntry* p;
    RouteEntry* q;
    u8* addr;
    u8* netmask;
    u8* gateway;
    u8* alias;
    int i;

    enabled = OSDisableInterrupts();
    addr = __IFDefault + 0x44;
    netmask = __IFDefault + 0x48;
    gateway = __IFDefault + 0x50;
    alias = __IFDefault + 0x54;
    for (route = Up.head, next = !route ? NULL : route->next; route;
         route = next, next = !route ? NULL : route->next) {
        p = route->next;
        q = route->prev;
        if (p == NULL) {
            Up.tail = q;
        } else {
            p->prev = q;
        }
        if (q == NULL) {
            Up.head = p;
        } else {
            q->next = p;
        }
        p = Free.head;
        if (p == NULL) {
            Free.tail = route;
        } else {
            p->prev = route;
        }
        route->next = p;
        route->prev = NULL;
        Free.head = route;
    }

    IPAddRoute((u8*)&IPLimited, (u8*)&IPLimited, (u8*)&IPAddrAny);
    if (*(u32*)addr != IPAddrAny) {
        IPAddRoute(addr, (u8*)&IPLimited, (u8*)&IPLoopbackAddr);
        for (i = 0; i < 4; i++) {
            net[i] = addr[i] & netmask[i];
        }
        IPAddRoute(net, netmask, (u8*)&IPAddrAny);
    }
    if (*(u32*)alias != IPAddrAny) {
        IPAddRoute(alias, (u8*)&IPLimited, (u8*)&IPLoopbackAddr);
        for (i = 0; i < 4; i++) {
            net[i] = aliasMask[i] & alias[i];
        }
        IPAddRoute(net, aliasMask, (u8*)&IPAddrAny);
    }
    IPAddRoute(loopback, loopbackMask, (u8*)&IPLoopbackAddr);
    if (*(u32*)gateway != IPAddrAny) {
        IPAddRoute((u8*)&IPAddrAny, (u8*)&IPAddrAny, gateway);
    }
    ARPGratuitous(__IFDefault);
    OSRestoreInterrupts(enabled);
    return TRUE;
}

static void DecayRoute(RouteEntry* route)
{
    RouteEntry* next;
    RouteEntry* prev;
    RouteEntry* e;
    RouteEntry* p;
    int bits;
    int ebits;

    bits = CountBits(route->mask);
    next = route->next;
    prev = route->prev;
    if (next == NULL) {
        Up.tail = prev;
    } else {
        next->prev = prev;
    }
    if (prev == NULL) {
        Up.head = next;
    } else {
        prev->next = next;
    }

    for (e = Up.tail, p = !e ? NULL : e->prev; e; e = p, p = !e ? NULL : e->prev) {
        ebits = CountBits(e->mask);
        if (bits <= ebits) {
            route->prev = e;
            route->next = e->next;
            e->next = route;
            if (route->next == NULL) {
                Up.tail = route;
            } else {
                route->next->prev = route;
            }
            return;
        }
    }
    next = Up.head;
    if (next == NULL) {
        Up.tail = route;
    } else {
        next->prev = route;
    }
    route->next = next;
    route->prev = NULL;
    Up.head = route;
}

BOOL IPRecoverGateway(u8* addr)
{
    RouteEntry* route;
    RouteEntry* next;

    if (*(u32*)addr == IPAddrAny || addr[0] == 127) {
        return FALSE;
    }
    for (route = Up.tail, next = !route ? NULL : route->prev; route;
         route = next, next = !route ? NULL : route->prev) {
        if ((route->flags & 1) && route->gateway[0] != 127 && *(u32*)route->gateway == *(u32*)addr) {
            DecayRoute(route);
            return TRUE;
        }
    }
    for (route = Up.head, next = !route ? NULL : route->next; route;
         route = next, next = !route ? NULL : route->next) {
        if (Hit(route, addr)) {
            if ((route->flags & 1) && route->gateway[0] != 127) {
                DecayRoute(route);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

s32 IPGetConfigError(u8* interface)
{
    u8* p;

    p = (interface != NULL) ? interface : __IFDefault;
    return *(s32*)(p + 0x08);
}

s32 IPSetConfigError(u8* interface, s32 error)
{
    int enabled;

    interface = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    if (*(s32*)(interface + 0x08) == 0) {
        *(s32*)(interface + 0x08) = error;
    }
    OSRestoreInterrupts(enabled);
    return *(s32*)(interface + 0x08);
}

s32 IPClearConfigError(u8* interface)
{
    u8* p;
    s32 error;
    int enabled;

    p = (interface != NULL) ? interface : __IFDefault;
    enabled = OSDisableInterrupts();
    error = *(s32*)(p + 0x08);
    *(s32*)(p + 0x08) = 0;
    OSRestoreInterrupts(enabled);
    return error;
}
