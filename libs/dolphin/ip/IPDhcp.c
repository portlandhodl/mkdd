#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

typedef struct SOUdpSocket SOUdpSocket;

typedef struct OSAddress {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
} OSAddress;

typedef struct DHCPInfo DHCPInfo;
typedef struct DHCPControl DHCPControl;
typedef struct ARPConf ARPConf;

struct DHCPInfo {
    u8 addr[4];
    u8 netmask[4];
    u8 router[4];
    u8 dns1[4];
    u8 dns2[4];
    u8 hostname[256];
    u16 mtu;
    u8 broadcast[4];
    u16 pad;
    u32 lease;
    u8 server[4];
    u32 t1;
    u32 t2;
};

struct DHCPControl {
    u8 sock[0xEC];
    u32 unkEC;
    s32 state;
    u32 xid;
    u32 flags;
    u8 peer[8];
    u8 packet[536];
    s32 length;
    s32 maxRetry;
    s32 retry;
    OSAlarm alarm;
    OSAlarm t1;
    OSAlarm t2;
    OSAlarm ex;
    OSTime t2Time;
    OSTime exTime;
    DHCPInfo info;
    DHCPInfo temp;
    void (*callback)(s32);
    u32 arg;
};

struct ARPConf {
    u8* interface;
    ARPConf* prev;
    ARPConf* next;
    u8 addr[4];
    OSAlarm alarm;
    s32 claim;
    void (*handler)(ARPConf*, s32);
};

typedef void (*DHCPHandler)(s32);

#define DHCP_FLAG_ACTIVE  0x1
#define DHCP_FLAG_AUTO    0x2
#define DHCP_FLAG_RELEASE 0x4

enum {
    DHCP_STATE_STOPPED = 0,
    DHCP_STATE_INIT = 1,
    DHCP_STATE_SELECTING = 1,
    DHCP_STATE_REQUESTING = 2,
    DHCP_STATE_CHECKING = 3,
    DHCP_STATE_RENEWING = 4,
    DHCP_STATE_REBINDING = 5,
    DHCP_STATE_RELEASING = 6,
    DHCP_STATE_REBOOTING = 7,
    DHCP_STATE_DECLINING = 8,
};

enum {
    DHCPDISCOVER = 1,
    DHCPOFFER = 2,
    DHCPREQUEST = 3,
    DHCPDECLINE = 4,
    DHCPACK = 5,
    DHCPNAK = 6,
    DHCPRELEASE = 7,
};

extern u32 IPAddrAny;
extern u32 IPLimited;
extern u8 __IFDefault[];

extern void IPInitRoute(u8*, u8*, u8*);
extern void IPSetBroadcastAddr(void*, u32*);
extern void IPSetConfigError(void*, s32);
extern void IPSetMtu(void*, s32);
extern s32 IPGetMtu(void*, s32*);
extern void ARPClaim(void*, ARPConf*);

extern s32 UDPOpen(SOUdpSocket*, u8*, u32);
extern s32 UDPBind(SOUdpSocket*, u8*);
extern s32 UDPClose(SOUdpSocket*);
extern void UDPCancel(SOUdpSocket*);
extern s32 UDPSendAsync(SOUdpSocket*, const void*, s32, u8*,
                        void (*)(SOUdpSocket*, s32), void*);
extern s32 UDPReceiveAsync(SOUdpSocket*, void*, s32, s32, u8*,
                           void (*)(SOUdpSocket*, s32), void*);

static void UpdateDHCPInfo(DHCPInfo*, DHCPInfo*);
s32 DHCPProcessOptions(u8*, s32, DHCPInfo*);
static void RecvCallback(SOUdpSocket*, s32);
static void T1Handler(OSAlarm*, OSContext*);
static void T2Handler(OSAlarm*, OSContext*);
static void ExHandler(OSAlarm*, OSContext*);
static void RxmitHandler(OSAlarm*, OSContext*);
static void ReleaseCallback(SOUdpSocket*, s32);
static void ReleaseHandler(OSAlarm*, OSContext*);
static s32 DHCPDiscover(DHCPControl*);
static s32 DHCPDecline(DHCPControl*);
static s32 DHCPRelease(DHCPControl*);
static s32 DHCPRequest(DHCPControl*);
static void Start(DHCPControl*);
static void Stop(DHCPControl*);
static void ClaimHandler(ARPConf*, s32);
static void Claim(DHCPControl*);
static s32 Restart(DHCPControl*, OSTime);
static void NullCallback(s32);
s32 DHCPStartupEx(DHCPHandler, s32);
BOOL DHCPCleanup(void);
BOOL DHCPReboot(void);
BOOL DHCPAuto(BOOL);
s32 DHCPGetStatus(DHCPInfo*);

static DHCPControl Control;
static ARPConf Conf;
static u32 MagicCookie = 0x63825363;

static u32 Rotate(u32 x, u32 n)
{
    return (x << n) | (x >> (32 - n));
}

static OSTime DHCPBackoff(DHCPControl* control)
{
    s32 timeout;
    u32 r;
    OSTime t;

    if (control->retry <= control->maxRetry) {
        timeout = 4 << control->retry;
    } else {
        timeout = 64;
    }
    t = OSSecondsToTicks((OSTime)timeout);
    r = OSGetTick();
    r = Rotate(r, r & 31);
    r %= OS_TIMER_CLOCK * 2;
    return t + r - OS_TIMER_CLOCK;
}

static void UpdateDHCPInfo(DHCPInfo* info, DHCPInfo* src)
{
    if (*(u32*)info->addr == IPAddrAny) {
        memmove(info->addr, src->addr, 4);
    }
    if (*(u32*)src->netmask != IPAddrAny) {
        memmove(info->netmask, src->netmask, 4);
    }
    if (*(u32*)src->router != IPAddrAny) {
        memmove(info->router, src->router, 4);
    }
    if (*(u32*)src->dns1 != IPAddrAny) {
        memmove(info->dns1, src->dns1, 4);
    }
    if (*(u32*)src->dns2 != IPAddrAny) {
        memmove(info->dns2, src->dns2, 4);
    }
    if (src->hostname[0] != 0) {
        memmove(info->hostname, src->hostname, 256);
    }
    if (src->mtu != 0) {
        info->mtu = src->mtu;
    }
    if (*(u32*)src->broadcast != IPLimited) {
        memmove(info->broadcast, src->broadcast, 4);
    }
    if (src->lease != 0) {
        info->lease = src->lease;
    }
    if (*(u32*)src->server != IPAddrAny) {
        memmove(info->server, src->server, 4);
    }
    if (src->t1 != 0) {
        info->t1 = src->t1;
    }
    if (src->t2 != 0) {
        info->t2 = src->t2;
    }
}

s32 DHCPProcessOptions(u8* packet, s32 length, DHCPInfo* info)
{
    s32 type;
    u8* opt;
    s32 len;
    s32 left;
    u8* overloadSname;
    u8* overloadFile;
    u16 mtu;
    register u8 code;

    left = length - 236;
    opt = packet + 236;
    overloadSname = NULL;
    overloadFile = NULL;
    if (left < 5) {
        goto bad;
    }
    if (*(u32*)opt == MagicCookie) {
        goto good;
    }
bad:
    return -1;
good:
    type = -1;
    opt += 4;
    left -= 4;

    memmove(info->addr, &IPAddrAny, 4);
    memmove(info->netmask, &IPAddrAny, 4);
    memmove(info->router, &IPAddrAny, 4);
    memmove(info->dns1, &IPAddrAny, 4);
    memmove(info->dns2, &IPAddrAny, 4);
    memset(info->hostname, 0, 256);
    info->mtu = 0;
    memmove(info->broadcast, &IPLimited, 4);
    info->lease = 0;
    memmove(info->server, &IPAddrAny, 4);
    info->t1 = 0;
    info->t2 = 0;

restart:
    while (left > 0 && *opt != 255) {
        left--;
        code = *opt++;
        if (left < 1) {
            return -1;
        }
        len = *opt++;
        left--;
        if (left < len) {
            return -1;
        }
        switch (code) {
        case 1:
            if (len != 4) {
                return -1;
            }
            memmove(info->netmask, opt, 4);
            break;
        case 3:
            if (len <= 0 || len % 4 != 0) {
                return -1;
            }
            memmove(info->router, opt, 4);
            break;
        case 6:
            if (len <= 0 || len % 4 != 0) {
                return -1;
            }
            memmove(info->dns1, opt, 4);
            if (len >= 8) {
                memmove(info->dns2, opt + 4, 4);
            }
            break;
        case 15:
            memcpy(info->hostname, opt, (u32)len > 256 ? 256 : len);
            break;
        case 26:
            mtu = *(u16*)opt;
            if (mtu >= 68) {
                info->mtu = mtu < 65535 ? mtu : 65535;
            }
            break;
        case 28:
            if (len != 4) {
                return -1;
            }
            memmove(info->broadcast, opt, 4);
            break;
        case 51:
            if (len != 4) {
                return -1;
            }
            info->lease = *(u32*)opt;
            break;
        case 52:
            if (len != 1) {
                return -1;
            }
            switch (*opt) {
            case 1:
                overloadFile = packet + 108;
                break;
            case 2:
                overloadSname = packet + 44;
                break;
            case 3:
                overloadFile = packet + 108;
                overloadSname = packet + 44;
                break;
            }
            break;
        case 53:
            if (len != 1) {
                return -1;
            }
            type = *opt;
            break;
        case 54:
            if (len != 4) {
                return -1;
            }
            memmove(info->server, opt, 4);
            break;
        case 58:
            if (len != 4) {
                return -1;
            }
            info->t1 = *(u32*)opt;
            break;
        case 59:
            if (len != 4) {
                return -1;
            }
            info->t2 = *(u32*)opt;
            break;
        }
        opt += len;
        left -= len;
    }
    if (overloadSname != NULL) {
        opt = overloadSname;
        left = 64;
        overloadSname = NULL;
        goto restart;
    }
    if (overloadFile != NULL) {
        opt = overloadFile;
        left = 128;
        overloadFile = NULL;
        goto restart;
    }
    memmove(info->addr, packet + 16, 4);
    return type;
}

static u8* AddRequestList(u8* p)
{
    *p++ = 55;
    *p++ = 7;
    *p++ = 1;
    *p++ = 3;
    *p++ = 6;
    *p++ = 26;
    *p++ = 28;
    *p++ = 58;
    *p++ = 59;
    return p;
}

static void RecvCallback(SOUdpSocket* so, s32 len)
{
    DHCPControl* control = (DHCPControl*)so;
    u8* packet = control->packet;
    s32 type;

    if (len < 241) {
        goto receive;
    }
    if (*(u32*)(packet + 4) != control->xid) {
        goto receive;
    }
    if (packet[1] != 1) {
        goto receive;
    }
    if (packet[2] != 6) {
        goto receive;
    }
    if (memcmp(packet + 28, __IFDefault + 56, 6) != 0) {
        goto receive;
    }
    type = DHCPProcessOptions(packet, *(s32*)(control->sock + 200),
                              &control->temp);
    if (type < 0) {
        goto receive;
    }

    switch (control->state) {
    case DHCP_STATE_SELECTING:
        if (type != DHCPOFFER) {
            goto receive;
        }
        if (control->temp.lease == 0) {
            goto receive;
        }
        memmove(&control->info, &control->temp, 300);
        control->retry = 0;
        control->state = DHCP_STATE_REQUESTING;
        control->callback(control->state);
        control->xid++;
        DHCPRequest(control);
        return;

    case DHCP_STATE_REQUESTING:
        switch (type) {
        case DHCPACK:
            if (control->temp.lease == 0) {
                return;
            }
            UpdateDHCPInfo(&control->info, &control->temp);
            Claim(control);
            return;
        case DHCPNAK:
            Stop(control);
            IPSetConfigError(NULL, -102);
            return;
        default:
            goto receive;
        }

    case DHCP_STATE_CHECKING:
    case DHCP_STATE_RELEASING:
        goto receive;

    case DHCP_STATE_RENEWING:
    case DHCP_STATE_REBINDING:
        switch (type) {
        case DHCPACK:
            if (control->temp.lease == 0) {
                return;
            }
            UpdateDHCPInfo(&control->info, &control->temp);
            Start(control);
            return;
        case DHCPNAK:
            Stop(control);
            IPSetConfigError(NULL, -102);
            return;
        default:
            goto receive;
        }

    case DHCP_STATE_REBOOTING:
        switch (type) {
        case DHCPACK:
            if (control->temp.lease == 0) {
                return;
            }
            UpdateDHCPInfo(&control->info, &control->temp);
            Claim(control);
            return;
        case DHCPNAK:
            Stop(control);
            IPSetConfigError(NULL, -102);
            if (control->flags & DHCP_FLAG_AUTO) {
                return;
            }
            Restart(control, (OSTime)300 * OS_TIMER_CLOCK);
            return;
        default:
            goto receive;
        }

    default:
        goto receive;
    }

receive:
    UDPReceiveAsync(so, control->packet, 536, 0, control->peer, RecvCallback, NULL);
}

static void T1Handler(OSAlarm* alarm, OSContext* context)
{
    DHCPControl* control = (DHCPControl*)((u8*)alarm - 848);

    control->state = DHCP_STATE_RENEWING;
    control->callback(control->state);
    control->xid++;
    control->retry = 0;
    DHCPRequest(control);
}

static void T2Handler(OSAlarm* alarm, OSContext* context)
{
    DHCPControl* control = (DHCPControl*)((u8*)alarm - 888);

    control->state = DHCP_STATE_REBINDING;
    control->callback(control->state);
    control->xid++;
    control->retry = 0;
    DHCPRequest(control);
}

static void ExHandler(OSAlarm* alarm, OSContext* context)
{
    DHCPControl* control = (DHCPControl*)((u8*)alarm - 928);

    IPSetConfigError(NULL, -101);
    Stop(control);
}

static void RxmitHandler(OSAlarm* alarm, OSContext* context)
{
    DHCPControl* control = (DHCPControl*)((u8*)alarm - 808);

    control->retry++;
    if (control->retry == control->maxRetry) {
        switch (control->state) {
        case DHCP_STATE_INIT:
        case DHCP_STATE_REQUESTING:
            IPSetConfigError(NULL, -100);
            Stop(control);
            return;
            break;
        case DHCP_STATE_RENEWING:
        case DHCP_STATE_REBINDING:
            return;
        case DHCP_STATE_REBOOTING:
            control->state = DHCP_STATE_CHECKING;
            control->callback(control->state);
            return;
        case DHCP_STATE_CHECKING:
        case DHCP_STATE_RELEASING:
            break;
        }
    }
    if (control->maxRetry <= control->retry) {
        control->retry = control->maxRetry;
    }
    switch (control->state) {
    case DHCP_STATE_INIT:
        DHCPDiscover(control);
        break;
    case DHCP_STATE_REQUESTING:
    case DHCP_STATE_RENEWING:
    case DHCP_STATE_REBINDING:
    case DHCP_STATE_REBOOTING:
        DHCPRequest(control);
        break;
    case DHCP_STATE_CHECKING:
        break;
    }
}

static void ReleaseCallback(SOUdpSocket* so, s32 result)
{
    Stop((DHCPControl*)so);
}

static void ReleaseHandler(OSAlarm* alarm, OSContext* context)
{
    Stop((DHCPControl*)((u8*)alarm - 808));
}

static s32 DHCPDiscover(DHCPControl* control)
{
    u8* p;
    OSAddress peer;
    s32 result;

    if (control->state != DHCP_STATE_INIT) {
        return -12;
    }
    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);

    p = control->packet + 236;
    memmove(p, &MagicCookie, 4);
    p += 4;
    *p++ = 53;
    *p++ = 1;
    *p++ = DHCPDISCOVER;
    *p++ = 61;
    *p++ = 7;
    *p++ = 1;
    memmove(p, __IFDefault + 56, 6);
    p = AddRequestList(p + 6);
    *p++ = 255;
    control->length = p - control->packet;
    if (control->length < 300) {
        memset(p, 0, 300 - control->length);
        control->length = 300;
    }

    p = control->packet;
    p[0] = 1;
    p[1] = 1;
    p[2] = 6;
    p[3] = 0;
    *(u32*)(p + 4) = control->xid;
    *(u16*)(p + 8) = 0;
    *(u16*)(p + 10) = 0;
    memset(p + 12, 0, 4);
    memset(p + 16, 0, 4);
    memset(p + 20, 0, 4);
    memset(p + 24, 0, 4);
    memmove(p + 28, __IFDefault + 56, 6);
    memset(p + 44, 0, 64);
    memset(p + 108, 0, 128);

    peer.len = 8;
    peer.family = 2;
    memmove(peer.addr, &IPLimited, 4);
    peer.port = 67;
    result = UDPSendAsync((SOUdpSocket*)control, control->packet, control->length, (u8*)&peer, NULL, NULL);
    OSSetAlarm(&control->alarm, DHCPBackoff(control), RxmitHandler);
    UDPReceiveAsync((SOUdpSocket*)control, control->packet, 536, 0, control->peer, RecvCallback, NULL);
    return result;
}

static s32 DHCPDecline(DHCPControl* control)
{
    u8* p;
    OSAddress peer;

    if (control->state != DHCP_STATE_REQUESTING &&
        control->state != DHCP_STATE_REBOOTING &&
        control->state != DHCP_STATE_RENEWING &&
        control->state != DHCP_STATE_REBINDING) {
        return -12;
    }
    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);

    p = control->packet + 236;
    memmove(p, &MagicCookie, 4);
    p += 4;
    *p++ = 53;
    *p++ = 1;
    *p++ = DHCPDECLINE;
    *p++ = 54;
    *p++ = 4;
    memmove(p, control->info.server, 4);
    p += 4;
    *p++ = 50;
    *p++ = 4;
    memmove(p, control->info.addr, 4);
    p += 4;
    *p++ = 61;
    *p++ = 7;
    *p++ = 1;
    memmove(p, __IFDefault + 56, 6);
    p += 6;
    *p++ = 255;
    control->length = p - control->packet;
    if (control->length < 300) {
        memset(p, 0, 300 - control->length);
        control->length = 300;
    }

    p = control->packet;
    p[0] = 1;
    p[1] = 1;
    p[2] = 6;
    p[3] = 0;
    *(u32*)(p + 4) = control->xid;
    *(u16*)(p + 8) = 0;
    *(u16*)(p + 10) = 0;
    memset(p + 12, 0, 4);
    memset(p + 16, 0, 4);
    memset(p + 20, 0, 4);
    memset(p + 24, 0, 4);
    memmove(p + 28, __IFDefault + 56, 6);
    memset(p + 44, 0, 64);
    memset(p + 108, 0, 128);

    OSSetAlarm(&control->alarm, DHCPBackoff(control), ReleaseHandler);

    peer.len = 8;
    peer.family = 2;
    memmove(peer.addr, &IPLimited, 4);
    peer.port = 67;
    return UDPSendAsync((SOUdpSocket*)control, control->packet, control->length, (u8*)&peer, ReleaseCallback, NULL);
}

static s32 DHCPRelease(DHCPControl* control)
{
    u8* p;
    OSAddress peer;

    if (control->state != DHCP_STATE_CHECKING &&
        control->state != DHCP_STATE_RENEWING &&
        control->state != DHCP_STATE_REBINDING) {
        return -12;
    }
    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);

    p = control->packet + 236;
    memmove(p, &MagicCookie, 4);
    p += 4;
    *p++ = 53;
    *p++ = 1;
    *p++ = DHCPRELEASE;
    *p++ = 54;
    *p++ = 4;
    memmove(p, control->info.server, 4);
    p += 4;
    *p++ = 61;
    *p++ = 7;
    *p++ = 1;
    memmove(p, __IFDefault + 56, 6);
    p += 6;
    *p++ = 255;
    control->length = p - control->packet;
    if (control->length < 300) {
        memset(p, 0, 300 - control->length);
        control->length = 300;
    }

    p = control->packet;
    p[0] = 1;
    p[1] = 1;
    p[2] = 6;
    p[3] = 0;
    *(u32*)(p + 4) = control->xid;
    *(u16*)(p + 8) = 0;
    *(u16*)(p + 10) = 0;
    memmove(p + 12, control->info.addr, 4);
    memset(p + 16, 0, 4);
    memset(p + 20, 0, 4);
    memset(p + 24, 0, 4);
    memmove(p + 28, __IFDefault + 56, 6);
    memset(p + 44, 0, 64);
    memset(p + 108, 0, 128);

    OSSetAlarm(&control->alarm, DHCPBackoff(control), ReleaseHandler);

    peer.len = 8;
    peer.family = 2;
    memmove(peer.addr, control->info.server, 4);
    peer.port = 67;
    return UDPSendAsync((SOUdpSocket*)control, control->packet, control->length, (u8*)&peer, ReleaseCallback, NULL);
}

static s32 DHCPRequest(DHCPControl* control)
{
    u8* p;
    OSAddress peer;
    s32 result;
    OSTime t;

    if (control->state != DHCP_STATE_REQUESTING &&
        control->state != DHCP_STATE_REBOOTING &&
        control->state != DHCP_STATE_RENEWING &&
        control->state != DHCP_STATE_REBINDING) {
        return -12;
    }
    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);

    p = control->packet + 236;
    memmove(p, &MagicCookie, 4);
    p += 4;
    *p++ = 53;
    *p++ = 1;
    *p++ = DHCPREQUEST;
    if (control->state == DHCP_STATE_REQUESTING) {
        *p++ = 54;
        *p++ = 4;
        memmove(p, control->info.server, 4);
        p += 4;
    }
    if (control->state == DHCP_STATE_REQUESTING ||
        control->state == DHCP_STATE_REBOOTING) {
        *p++ = 50;
        *p++ = 4;
        memmove(p, control->info.addr, 4);
        p += 4;
    }
    *p++ = 61;
    *p++ = 7;
    *p++ = 1;
    memmove(p, __IFDefault + 56, 6);
    p = AddRequestList(p + 6);
    *p++ = 255;
    control->length = p - control->packet;
    if (control->length < 300) {
        memset(p, 0, 300 - control->length);
        control->length = 300;
    }

    p = control->packet;
    if (control->state == DHCP_STATE_RENEWING ||
        control->state == DHCP_STATE_REBINDING) {
        memmove(p + 12, control->info.addr, 4);
    } else {
        memset(p + 12, 0, 4);
    }
    p[0] = 1;
    p[1] = 1;
    p[2] = 6;
    p[3] = 0;
    *(u32*)(p + 4) = control->xid;
    *(u16*)(p + 8) = 0;
    *(u16*)(p + 10) = 0;
    memset(p + 16, 0, 4);
    memset(p + 20, 0, 4);
    memset(p + 24, 0, 4);
    memmove(p + 28, __IFDefault + 56, 6);
    memset(p + 44, 0, 64);
    memset(p + 108, 0, 128);

    peer.len = 8;
    peer.family = 2;
    if (control->state == DHCP_STATE_RENEWING) {
        memmove(peer.addr, control->info.server, 4);
    } else {
        memmove(peer.addr, &IPLimited, 4);
    }
    peer.port = 67;

    switch (control->state) {
    case DHCP_STATE_REQUESTING:
    case DHCP_STATE_REBOOTING:
        t = DHCPBackoff(control);
        break;
    case DHCP_STATE_RENEWING:
        control->retry = 0;
        t = (control->t2Time - OSGetTime()) / 2;
        if (t <= OSSecondsToTicks((OSTime)60)) {
            control->retry = control->maxRetry - 1;
        }
        break;
    case DHCP_STATE_REBINDING:
        control->retry = 0;
        t = (control->exTime - OSGetTime()) / 2;
        if (t <= OSSecondsToTicks((OSTime)60)) {
            control->retry = control->maxRetry - 1;
        }
        break;
    case DHCP_STATE_CHECKING:
        break;
    }
    result = UDPSendAsync((SOUdpSocket*)control, control->packet, control->length, (u8*)&peer, NULL, NULL);
    OSSetAlarm(&control->alarm, t, RxmitHandler);
    UDPReceiveAsync((SOUdpSocket*)control, control->packet, 536, 0, control->peer, RecvCallback, NULL);
    return result;
}

static void Start(DHCPControl* control)
{
    DHCPInfo* info = &control->info;
    u32 t1;
    u32 t2;
    u32 lease;
    OSTime now;
    s32 mtu;

    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);
    control->retry = 0;
    OSCancelAlarm(&control->t1);
    OSCancelAlarm(&control->t2);
    OSCancelAlarm(&control->ex);

    lease = info->lease;
    t1 = info->t1;
    if (t1 == 0 || lease <= t1) {
        t1 = lease / 2;
    }
    t2 = info->t2;
    if (t2 == 0 || lease <= t2) {
        t2 = (u64)lease * 7 / 8;
    }
    if (t2 <= t1) {
        t1 = (u64)t2 * 8 / 14;
    }
    if (t1 > 10) {
        t1 -= 10;
        t2 -= 10;
        lease -= 10;
    }

    now = OSGetTime();
    control->t2Time = now + OSSecondsToTicks((OSTime)t2);
    control->exTime = now + OSSecondsToTicks((OSTime)lease);
    OSSetAlarm(&control->t1, OSSecondsToTicks((OSTime)t1), T1Handler);
    OSSetAlarm(&control->t2, OSSecondsToTicks((OSTime)t2), T2Handler);
    OSSetAlarm(&control->ex, OSSecondsToTicks((OSTime)lease), ExHandler);

    IPInitRoute(info->addr, info->netmask, info->router);
    IPSetBroadcastAddr(__IFDefault, (u32*)info->broadcast);
    if (info->mtu >= 68) {
        IPGetMtu(__IFDefault, &mtu);
        mtu = info->mtu < mtu ? info->mtu : mtu;
        IPSetMtu(__IFDefault, mtu);
    }
    control->state = DHCP_STATE_CHECKING;
    control->callback(control->state);
}

static void Stop(DHCPControl* control)
{
    if (control->state == DHCP_STATE_STOPPED) {
        return;
    }
    control->state = DHCP_STATE_STOPPED;
    UDPClose((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);
    control->retry = 0;
    OSCancelAlarm(&control->t1);
    OSCancelAlarm(&control->t2);
    OSCancelAlarm(&control->ex);
    control->flags &= ~DHCP_FLAG_RELEASE;
    IPInitRoute(NULL, NULL, NULL);
    IPSetBroadcastAddr(__IFDefault, &IPLimited);
    memcpy(Conf.addr, &IPAddrAny, 4);
    ARPClaim(__IFDefault, &Conf);
    control->callback(control->state);
    if (control->flags & DHCP_FLAG_AUTO) {
        Restart(control, (OSTime)300 * OS_TIMER_CLOCK);
    }
}

static void ClaimHandler(ARPConf* conf, s32 delay)
{
    memset(conf->addr, 0, 4);
    ARPClaim(conf->interface, conf);
    if (delay == 0) {
        Start(&Control);
        return;
    }
    OSCancelAlarm(&Control.alarm);
    Control.retry = 0;
    OSCancelAlarm(&Control.t1);
    OSCancelAlarm(&Control.t2);
    OSCancelAlarm(&Control.ex);
    Control.flags |= DHCP_FLAG_RELEASE;
    Control.xid++;
    if (DHCPDecline(&Control) < 0) {
        Stop(&Control);
    }
    IPSetConfigError(conf->interface, -111);
}

static void Claim(DHCPControl* control)
{
    u8* addr;

    addr = control->info.addr;
    UDPCancel((SOUdpSocket*)control);
    OSCancelAlarm(&control->alarm);
    control->retry = 0;
    OSCancelAlarm(&control->t1);
    OSCancelAlarm(&control->t2);
    OSCancelAlarm(&control->ex);
    memcpy(Conf.addr, addr, 4);
    ARPClaim(__IFDefault, &Conf);
}

static s32 Restart(DHCPControl* control, OSTime wait)
{
    OSAddress addr;

    if (control->state != DHCP_STATE_STOPPED) {
        return 0;
    }
    control->state = DHCP_STATE_INIT;
    control->callback(control->state);
    control->xid++;
    UDPOpen((SOUdpSocket*)control, NULL, 0);
    addr.len = 8;
    addr.family = 2;
    memmove(addr.addr, &IPAddrAny, 4);
    addr.port = 68;
    if (UDPBind((SOUdpSocket*)control, (u8*)&addr) < 0) {
        return 0;
    }
    switch (control->state) {
    case DHCP_STATE_INIT:
        control->retry = -1;
        OSCancelAlarm(&control->alarm);
        OSSetAlarm(&control->alarm, wait, RxmitHandler);
        break;
    case DHCP_STATE_REQUESTING:
        DHCPRequest(control);
        break;
    }
    return 1;
}

static void NullCallback(s32 state)
{
}

s32 DHCPStartupEx(DHCPHandler handler, s32 retry)
{
    DHCPControl* control;
    BOOL enabled;
    s32 result;

    control = &Control;
    enabled = OSDisableInterrupts();
    if (!(control->flags & DHCP_FLAG_ACTIVE)) {
        memset(control, 0, sizeof(DHCPControl));
        control->flags |= DHCP_FLAG_ACTIVE | DHCP_FLAG_AUTO;
        control->maxRetry = retry <= 0 ? 4 : retry;
        control->xid = OSGetTick();
        OSCreateAlarm(&Conf.alarm);
        Conf.handler = ClaimHandler;
    }
    control->callback = handler != NULL ? handler : NullCallback;
    result = Restart(control,
                     (OSTime)(OS_TIMER_CLOCK + OSGetTick() % OS_TIMER_CLOCK));
    OSRestoreInterrupts(enabled);
    return result;
}

BOOL DHCPCleanup(void)
{
    DHCPControl* control;
    BOOL enabled;
    BOOL done;

    control = &Control;
    enabled = OSDisableInterrupts();
    if (control->flags & DHCP_FLAG_ACTIVE) {
        DHCPAuto(FALSE);
        if (control->state != DHCP_STATE_STOPPED && !(control->flags & DHCP_FLAG_RELEASE)) {
            OSCancelAlarm(&control->alarm);
            control->retry = 0;
            OSCancelAlarm(&control->t1);
            OSCancelAlarm(&control->t2);
            OSCancelAlarm(&control->ex);
            control->flags |= DHCP_FLAG_RELEASE;
            control->xid++;
            if (DHCPRelease(control) < 0) {
                Stop(control);
            }
        }
        done = TRUE;
    } else {
        done = FALSE;
    }
    OSRestoreInterrupts(enabled);
    return done;
}

BOOL DHCPReboot(void)
{
    DHCPControl* control = &Control;

    switch (control->state) {
    case DHCP_STATE_CHECKING:
    case DHCP_STATE_RENEWING:
    case DHCP_STATE_REBINDING:
        control->state = DHCP_STATE_REBOOTING;
        control->callback(control->state);
        control->retry = 0;
        control->xid++;
        DHCPRequest(control);
        return TRUE;
    }
    return FALSE;
}

BOOL DHCPAuto(BOOL on)
{
    DHCPControl* control;
    BOOL enabled;
    BOOL wasAuto;

    control = &Control;
    enabled = OSDisableInterrupts();
    wasAuto = (control->flags & DHCP_FLAG_AUTO) ? TRUE : FALSE;
    if (on) {
        control->flags |= DHCP_FLAG_AUTO;
        Restart(control, (OSTime)300 * OS_TIMER_CLOCK);
    } else {
        control->flags &= ~DHCP_FLAG_AUTO;
    }
    OSRestoreInterrupts(enabled);
    return wasAuto;
}

s32 DHCPGetStatus(DHCPInfo* info)
{
    DHCPControl* control;
    BOOL enabled;
    s32 state;

    control = &Control;
    enabled = OSDisableInterrupts();
    state = control->state;
    if (info != NULL) {
        memmove(info, &control->info, 300);
    }
    OSRestoreInterrupts(enabled);
    return state;
}
