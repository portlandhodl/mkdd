#include <dolphin/ip/ip.h>
#include <dolphin/os.h>
#include <dolphin/types.h>

#include <string.h>

typedef struct DNSAddr {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
} DNSAddr;

typedef struct DNSInfo {
    u8 udp[0xF0];
    DNSAddr remote;
    u8 unkF8[0x108 - 0xF8];
    OSAlarm alarm;
    u32 flag;
    u8 unk134[0x540 - 0x134];
    u16 id;
    u8 unk542[0x550 - 0x542];
    s32 count;
    OSThreadQueue queue;
} DNSInfo;

extern s32 UDPOpen(void* udp, s32 port, s32 flag);
extern s32 UDPClose(void* udp);
extern s32 UDPConnect(void* udp, void* addr);
extern void* IPGetRoute(const u8* dst, u8* gateway);
extern BOOL IPIsBroadcastAddr(void* interface, const u8* addr);

s32 DNSOpen(DNSInfo* info, u8* addr)
{
    BOOL enabled;
    s32 result;

    enabled = OSDisableInterrupts();
    result = UDPOpen(info, 0, 0);
    if (result >= 0) {
        memset(&info->remote, 0, 0x470);
        info->id = (u16)(OSGetTime() / (OS_TIMER_CLOCK / 250000));
        OSCreateAlarm(&info->alarm);
        info->remote.len = sizeof(DNSAddr);
        info->remote.family = 2;
        memmove(info->remote.addr, addr, 4);
        info->remote.port = 53;
        OSInitThreadQueue(&info->queue);
        result = UDPConnect(info, &info->remote);
        if (result < 0) {
            UDPClose(info);
        } else if (IPIsBroadcastAddr(IPGetRoute(addr, NULL), addr)) {
            info->flag |= 1;
        } else if ((addr[0] & 0xF0) == 0xE0) {
            info->flag |= 2;
        }
    }
    OSRestoreInterrupts(enabled);
    return result;
}

s32 DNSClose(DNSInfo* info)
{
    s32 result;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    if (info->count > 0) {
        result = -1;
    } else {
        result = UDPClose(info);
    }
    if (result >= 0) {
        OSCancelAlarm(&info->alarm);
        OSWakeupThread(&info->queue);
    }
    OSRestoreInterrupts(enabled);
    return result;
}
