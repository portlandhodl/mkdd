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

void IPRefreshRoute(void);
static BOOL AddRoute(u8* dst, u8* mask, u8* gateway, u32 flags);

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

BOOL IPIsBroadcastAddr(u8* interface, const u8* addr)
{
    u32 net;
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
    net = *(u32*)(p + 0x44);
    mask = *(u32*)(p + 0x48);
    if ((net & mask) != 0) {
        if ((net & mask) == (a & mask) && (~mask & a) == ~mask) {
            return TRUE;
        }
    }
    if ((p[0x44] & 0x80) == 0) {
        classMask = 0xFF000000;
    } else if ((p[0x44] & 0xC0) == 0x80) {
        classMask = 0xFFFF0000;
    } else if ((p[0x44] & 0xE0) == 0xC0) {
        classMask = 0xFFFFFF00;
    }
    if ((net & classMask) == (a & classMask) && (~classMask & a) == ~classMask) {
        return TRUE;
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
    u32 aliasMask[1] = { 0xFFFF0000 };
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
        if ((*(u32*)addr & aliasMask[0]) == (*(u32*)((u8*)iface + 0x54) & aliasMask[0])) {
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
    u8* a = addr;

    next = Up.head;
    while (next != NULL) {
        route = next;
        next = route->next;
        if (route->dst[0] == (a[0] & route->mask[0]) &&
            route->dst[1] == (a[1] & route->mask[1]) &&
            route->dst[2] == (a[2] & route->mask[2]) &&
            route->dst[3] == (a[3] & route->mask[3])) {
            if (gateway != NULL) {
                if (route->flags & 1) {
                    memmove(gateway, route->gateway, 4);
                } else {
                    memmove(gateway, a, 4);
                }
            }
            route->time = __OSGetSystemTime();
            return route->interface;
        }
    }
    return NULL;
}

BOOL AddRoute(u8* dst, u8* mask, u8* gateway, u32 flags)
{
    RouteEntry* route;
    RouteEntry* stale;
    RouteEntry* p;
    u32 f;
    int bits;

    if (!(dst[0] == (dst[0] & mask[0]) &&
          dst[1] == (dst[1] & mask[1]) &&
          dst[2] == (dst[2] & mask[2]) &&
          dst[3] == (dst[3] & mask[3]))) {
        return FALSE;
    }
    if (*(u32*)gateway == IPAddrAny) {
        f = flags & ~1;
    } else {
        f = flags | 1;
    }
    route = Up.head;
    while (route != NULL) {
        if (MatchRoute(route, dst)) {
            if ((f & 1) && !(route->flags & 6)) {
                if (*(u32*)route->gateway == *(u32*)gateway) {
                    return TRUE;
                }
                route = route->next;
                continue;
            }
            memmove(route->gateway, gateway, 4);
            memmove(route->mask, mask, 4);
            if (!(route->flags & 2)) {
                f &= ~2;
            }
            route->flags = f;
            route->time = __OSGetSystemTime();
            route->interface = __IFDefault;
            return TRUE;
        }
        route = route->next;
    }

    if (Free.head == NULL) {
        stale = NULL;
        for (p = Up.head; p != NULL; p = p->next) {
            if (p->flags & 2) {
                if (stale == NULL || p->time > stale->time) {
                    stale = p;
                }
            }
        }
        if (stale == NULL) {
            return FALSE;
        }
        if (stale->next == NULL) {
            Up.tail = stale->prev;
        } else {
            stale->next->prev = stale->prev;
        }
        if (stale->prev == NULL) {
            Up.head = stale->next;
        } else {
            stale->prev->next = stale->next;
        }
        route = stale;
    } else {
        route = Free.head;
        Free.head = route->next;
        if (Free.head == NULL) {
            Free.tail = NULL;
        } else {
            Free.head->prev = NULL;
        }
    }

    memmove(route->dst, dst, 4);
    memmove(route->mask, mask, 4);
    memmove(route->gateway, gateway, 4);
    f &= ~2;
    route->flags = f;
    route->time = __OSGetSystemTime();
    route->interface = __IFDefault;

    bits = CountBits(mask);
    for (p = Up.head; p != NULL; p = p->next) {
        if (bits < CountBits(p->mask)) {
            route->prev = p->prev;
            route->next = p;
            p->prev = route;
            if (route->next == NULL) {
                Up.head = route;
            } else {
                route->next->prev = route;
            }
            return TRUE;
        }
    }
    route->next = NULL;
    route->prev = Up.tail;
    if (route->prev == NULL) {
        Up.head = route;
    } else {
        route->prev->next = route;
    }
    Up.tail = route;
    return TRUE;
}

BOOL IPRedirect(u8* addr, u8* gateway, u8* src)
{
    u8 gw[4];
    RouteEntry* route;
    void* interface;
    void* found;

    if (*(u32*)gateway == IPAddrAny) {
        return FALSE;
    }
    for (route = Up.head; route != NULL; route = route->next) {
        if (MatchRoute(route, addr)) {
            if (route->flags & 1) {
                memmove(gw, route->gateway, 4);
            } else {
                memmove(gw, addr, 4);
            }
            route->time = __OSGetSystemTime();
            interface = route->interface;
            goto found;
        }
    }
    return FALSE;

found:
    if (interface == NULL) {
        return FALSE;
    }
    if ((*(u32*)gateway & *(u32*)((u8*)interface + 0x48)) !=
        (*(u32*)((u8*)interface + 0x44) & *(u32*)((u8*)interface + 0x48))) {
        return FALSE;
    }
    if (IPIsBroadcastAddr(interface, gateway)) {
        return FALSE;
    }
    if (*(u32*)gw != *(u32*)src) {
        return FALSE;
    }
    found = NULL;
    for (route = Up.head; route != NULL; route = route->next) {
        if (MatchRoute(route, gateway)) {
            route->time = __OSGetSystemTime();
            found = route->interface;
            break;
        }
    }
    interface = NULL;
    for (route = Up.head; route != NULL; route = route->next) {
        if (MatchRoute(route, src)) {
            route->time = __OSGetSystemTime();
            interface = route->interface;
            break;
        }
    }
    if (found != interface) {
        return FALSE;
    }
    if (*(u32*)gateway == *(u32*)(__IFDefault + 0x44)) {
        return FALSE;
    }
    if (*(u32*)gateway == *(u32*)(__IFDefault + 0x54)) {
        return FALSE;
    }
    return AddRoute(addr, NULL, gateway, 7);
}
void IPInitRoute(u8* addr, u8* netmask, u8* gateway)
{
    RouteEntry* route;
    RouteEntry* end;
    u8 broadcast[4];
    int enabled;

    route = RoutingTable;
    end = route + 32;
    enabled = OSDisableInterrupts();
    if (!Initialized) {
        Initialized = TRUE;
        memset(RoutingTable, 0, sizeof(RoutingTable));
        Up.head = NULL;
        Up.tail = NULL;
        Free.head = NULL;
        Free.tail = NULL;
        for (; route < end; route++) {
            route->next = NULL;
            route->prev = Free.tail;
            if (Free.tail == NULL) {
                Free.head = route;
            } else {
                Free.tail->next = route;
            }
            Free.tail = route;
        }
    }
    OSRestoreInterrupts(enabled);

    enabled = OSDisableInterrupts();
    memcpy(__IFDefault + 0x44, addr ? addr : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);

    enabled = OSDisableInterrupts();
    memcpy(__IFDefault + 0x48, netmask ? netmask : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);

    enabled = OSDisableInterrupts();
    memcpy(__IFDefault + 0x50, gateway ? gateway : (u8*)&IPAddrAny, 4);
    OSRestoreInterrupts(enabled);

    if (addr != NULL && netmask != NULL) {
        broadcast[0] = (addr[0] & netmask[0]) | ~netmask[0];
        broadcast[1] = (addr[1] & netmask[1]) | ~netmask[1];
        broadcast[2] = (addr[2] & netmask[2]) | ~netmask[2];
        broadcast[3] = (addr[3] & netmask[3]) | ~netmask[3];
        enabled = OSDisableInterrupts();
        memcpy(__IFDefault + 0x4C, broadcast, 4);
        OSRestoreInterrupts(enabled);
    }
    IPRefreshRoute();
}

void IPRefreshRoute(void)
{
    u32 loopback = 0x7F000000;
    u32 loopbackMask = 0xFF000000;
    u32 aliasMask = 0xFFFF0000;
    u8 net[4];
    RouteEntry* route;
    RouteEntry* next;
    int enabled;
    int e;
    int i;

    enabled = OSDisableInterrupts();
    for (route = Up.head; route != NULL; route = next) {
        next = route->next;
        if (route->next == NULL) {
            Up.tail = route->prev;
        } else {
            route->next->prev = route->prev;
        }
        if (route->prev == NULL) {
            Up.head = route->next;
        } else {
            route->prev->next = route->next;
        }
        if (Free.head == NULL) {
            Free.tail = route;
        } else {
            Free.head->prev = route;
        }
        route->next = Free.head;
        route->prev = NULL;
        Free.head = route;
    }

    e = OSDisableInterrupts();
    AddRoute((u8*)&IPLimited, NULL, (u8*)&IPAddrAny, 0);
    OSRestoreInterrupts(e);

    if (*(u32*)(__IFDefault + 0x44) != IPAddrAny) {
        e = OSDisableInterrupts();
        AddRoute(__IFDefault + 0x44, NULL, (u8*)&IPLoopbackAddr, 0);
        OSRestoreInterrupts(e);

        net[0] = __IFDefault[0x44] & __IFDefault[0x48];
        net[1] = __IFDefault[0x45] & __IFDefault[0x49];
        net[2] = __IFDefault[0x46] & __IFDefault[0x4A];
        net[3] = __IFDefault[0x47] & __IFDefault[0x4B];
        e = OSDisableInterrupts();
        AddRoute(net, __IFDefault + 0x48, (u8*)&IPAddrAny, 0);
        OSRestoreInterrupts(e);
    }
    if (*(u32*)(__IFDefault + 0x54) != IPAddrAny) {
        e = OSDisableInterrupts();
        AddRoute(__IFDefault + 0x54, NULL, (u8*)&IPLoopbackAddr, 0);
        OSRestoreInterrupts(e);

        net[0] = (u8)aliasMask & __IFDefault[0x54];
        net[1] = (u8)(aliasMask >> 8) & __IFDefault[0x55];
        net[2] = (u8)(aliasMask >> 16) & __IFDefault[0x56];
        net[3] = (u8)(aliasMask >> 24) & __IFDefault[0x57];
        e = OSDisableInterrupts();
        AddRoute(net, (u8*)&aliasMask, (u8*)&IPAddrAny, 0);
        OSRestoreInterrupts(e);
    }
    e = OSDisableInterrupts();
    AddRoute((u8*)&loopback, (u8*)&loopbackMask, (u8*)&IPLoopbackAddr, 0);
    OSRestoreInterrupts(e);

    if (*(u32*)(__IFDefault + 0x50) != IPAddrAny) {
        e = OSDisableInterrupts();
        AddRoute((u8*)&IPAddrAny, (u8*)&IPAddrAny, __IFDefault + 0x50, 0);
        OSRestoreInterrupts(e);
    }
    ARPGratuitous(__IFDefault);
    OSRestoreInterrupts(enabled);
}

static void DecayRoute(RouteEntry* route)
{
    RouteEntry* p;
    RouteEntry* next;
    RouteEntry* prev;
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

    next = Up.tail;
    p = next != NULL ? next->prev : NULL;
    while (next != NULL) {
        ebits = CountBits(next->mask);
        if (bits <= ebits) {
            route->prev = next;
            route->next = next->next;
            next->next = route;
            if (route->next == NULL) {
                Up.tail = route;
            } else {
                route->next->prev = route;
            }
            return;
        }
        next = p;
        p = next != NULL ? next->prev : NULL;
    }
    route->next = Up.head;
    if (route->next == NULL) {
        Up.tail = route;
    } else {
        route->next->prev = route;
    }
    route->prev = NULL;
    Up.head = route;
}

BOOL IPRecoverGateway(u8* addr)
{
    RouteEntry* route;

    if (*(u32*)addr == IPAddrAny || addr[0] == 127) {
        return FALSE;
    }
    for (route = Up.tail; route != NULL; route = route->prev) {
        if ((route->flags & 1) && route->gateway[0] != 127 &&
            *(u32*)route->gateway == *(u32*)addr) {
            DecayRoute(route);
            return TRUE;
        }
    }
    for (route = Up.head; route != NULL; route = route->next) {
        if (MatchRoute(route, addr)) {
            if (!(route->flags & 1) || route->gateway[0] == 127) {
                return FALSE;
            }
            DecayRoute(route);
            return TRUE;
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
