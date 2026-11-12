#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <string.h>

typedef struct ARPConf ARPConf;

struct ARPConf {
    u8* interface;
    ARPConf* prev;
    ARPConf* next;
    u8 addr[4];
    OSAlarm alarm;
    s32 claim;
    void (*handler)(ARPConf*, s32);
};

extern u32 IPAddrAny;
extern u8 __IFDefault[];

extern void ARPClaim(void*, ARPConf*);
extern void IPSetAlias(void*, void*);
extern void IPRefreshRoute(void);

static ARPConf Conf;
static u64 Seed;
static s32 Collision;

static void GetAutoAddr(u8* addr)
{
    u16 r;

    do {
        Seed = (Seed * 0x5DEECE66DULL + 0xB) % (1ULL << 48);
        r = (u16)(Seed >> 32);
    } while (r < 0x100 || 0xFF00 <= r);
    if (addr) {
        addr[0] = 169;
        addr[1] = 254;
        addr[2] = (u8)(r >> 8);
        addr[3] = (u8)r;
    }
}

static void ClaimHander(ARPConf* conf, s32 result)
{
    if (result == 0) {
        IPSetAlias(__IFDefault, conf->addr);
        IPRefreshRoute();
        memset(conf->addr, 0, 4);
        ARPClaim(conf->interface, conf);
    } else if (Collision++, Collision < 10) {
        GetAutoAddr(conf->addr);
        ARPClaim(conf->interface, conf);
    } else {
        GetAutoAddr(NULL);
        memset(conf->addr, 0, 4);
        ARPClaim(conf->interface, conf);
        if (*(u32*)(__IFDefault + 0x54) != IPAddrAny) {
            IPSetAlias(__IFDefault, &IPAddrAny);
            IPRefreshRoute();
        }
    }
}

BOOL IPAutoConfig(void)
{
    BOOL enabled;
    u16 hi;

    enabled = OSDisableInterrupts();
    if (Seed == 0) {
        memcpy((u8*)&Seed + 2, __IFDefault + 56, 6);
        OSCreateAlarm(&Conf.alarm);
        Conf.handler = ClaimHander;
        GetAutoAddr(Conf.addr);
    } else {
        hi = (u16)(Seed >> 32);
        Conf.addr[0] = 169;
        Conf.addr[1] = 254;
        Conf.addr[2] = (u8)(hi >> 8);
        Conf.addr[3] = (u8)hi;
    }
    Collision = 0;
    ARPClaim(__IFDefault, &Conf);
    OSRestoreInterrupts(enabled);
    return TRUE;
}

void IPAutoStop(void)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    memset(Conf.addr, 0, 4);
    ARPClaim(__IFDefault, &Conf);
    IPSetAlias(__IFDefault, &IPAddrAny);
    IPRefreshRoute();
    OSRestoreInterrupts(enabled);
}
