#ifndef _DOLPHIN_IP_IPSOCKET_H_
#define _DOLPHIN_IP_IPSOCKET_H_

#include <dolphin/types.h>

#define AF_INET 2

#define SO_DGRAM 2

#define SO_MSG_PEEK 0x02
#define SO_MSG_DONTWAIT 0x04

#define SO_POLLRDNORM 0x0001
#define SO_POLLIN SO_POLLRDNORM

typedef struct SOSockAddr {
    u8 len;
    u8 family;
    u16 port;
    u8 addr[4];
} SOSockAddr;

typedef struct SOPollFd {
    s32 fd;
    s16 events;
    s16 revents;
} SOPollFd;

s32 SOSocket(s32 domain, s32 type, s32 flags);
s32 SOClose(s32 s);
s32 SOBind(s32 s, u8* addr);
s32 SOSetSockOpt(s32 s, s32 level, s32 opt, void* val, s32 len);
s32 SORecvFrom(s32 s, void* buf, s32 len, u32 flags, u8* addr);
s32 SOSendTo(s32 s, const void* buf, s32 len, s32 flags, u8* addr);
s32 SOPoll(SOPollFd* fds, u32 nfds, s64 timeout);
char* SOInetNtoP(s32 af, const u8* addr, char* buf, u32 len);

#endif
