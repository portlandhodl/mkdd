#ifndef _DOLPHIN_IP_H_
#define _DOLPHIN_IP_H_

#include <dolphin/types.h>

// Protocols
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

typedef struct IPHeader {
    u8 verlen;
    u8 tos;
    u16 len;
    u16 id;
    u16 frag;
    u8 ttl;
    u8 proto;
    u16 sum;
    u8 src[4];
    u8 dst[4];
} IPHeader;

typedef struct IFFifo {
    u8* buffer;
    s32 size;
    u8* head;
    s32 used;
} IFFifo;

typedef struct ICMPHeader ICMPHeader;
typedef struct IPIcmpInterface IPIcmpInterface;

void IFFifoInit(IFFifo* fifo, void* buffer, s32 size);
u8* IFFifoAlloc(IFFifo* fifo, s32 size);
BOOL IFFifoFree(IFFifo* fifo, u8* ptr, s32 size);
s32 ICMPSendError(ICMPHeader* icmp, IPIcmpInterface* interface, IPHeader* ip, u32 flags);

BOOL IPIsLocalAddr(u32, const u8 *addr);
BOOL IPGetNetmask(u32 type, u32* addr);
BOOL IPGetAddr(u32 type, u32* addr);
BOOL IPGetAlias(u32 type, u32* addr);

#endif
