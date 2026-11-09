#include <dolphin/types.h>

#include <stdio.h>
#include <string.h>

typedef struct IPSocket {
    u8 unk00[0x10];
    u8 remote[8];
} IPSocket;

typedef struct IPInterface {
    u8 unk00[0x64];
    void (*cancel)(struct IPInterface* interface, void* packet);
} IPInterface;

typedef struct IPCancelPacket {
    IPInterface* interface;
} IPCancelPacket;

static char ascii[16];

char* IPNtoA(const u8* addr)
{
    sprintf(ascii, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return ascii;
}

s32 IPGetRemoteSocket(IPSocket* socket, u8* addr)
{
    memcpy(addr, socket->remote, sizeof(socket->remote));
    return 0;
}

void IPCancel(IPCancelPacket* packet)
{
    IPInterface* interface = packet->interface;

    if (interface != NULL) {
        interface->cancel(interface, packet);
    }
}
