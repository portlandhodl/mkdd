#include <dolphin/ip/ip.h>

void UPnPGetHostAddr(u32* addr)
{
    IPGetAddr(0, addr);
    if (*addr == 0) {
        IPGetAlias(0, addr);
    }
}

void UPnPGetNetmask(u32* mask)
{
    u32 addr;

    IPGetAddr(0, &addr);
    if (addr == 0) {
        IPGetAlias(0, &addr);
        if (addr != 0) {
            *mask = 0xffff0000;
            return;
        }
    }
    IPGetNetmask(0, mask);
}
