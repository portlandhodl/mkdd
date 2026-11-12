#include <dolphin/types.h>
#include <dolphin/md5.h>

typedef struct UPnPUuid {
    u32 timeLow;
    u16 timeMid;
    u16 timeHiAndVersion;
    u8 clockSeqHiAndReserved;
    u8 clockSeqLow;
    u8 node[6];
} UPnPUuid;

char* IPGetUuidString(const UPnPUuid* uuid, char* string);
s32 IPScanUuid(const char* string, UPnPUuid* uuid);

UPnPUuid UPnPUdnNameSpace = {
    0x53e6872a, 0x8dd7, 0x11c4, 0x9c, 0x55, { 0x00, 0x09, 0xbf, 0x00, 0x01, 0x3f }
};

char* UPnPGetUuidString(const UPnPUuid* uuid, char* string)
{
    return IPGetUuidString(uuid, string);
}

s32 UPnPScanUuid(const char* string, UPnPUuid* uuid)
{
    return IPScanUuid(string, uuid);
}

void UPnPCreateUuidFromName(UPnPUuid* uuid, const UPnPUuid* nameSpace, const void* name, s32 len)
{
    MD5Context context;

    MD5Init(&context);
    MD5Update(&context, (u8*)nameSpace, sizeof(UPnPUuid));
    MD5Update(&context, (u8*)name, len);
    MD5Final((u8*)uuid, &context);
    uuid->timeHiAndVersion &= ~0xF000;
    uuid->timeHiAndVersion |= (3 << 12);
    uuid->clockSeqHiAndReserved &= ~0xC0;
    uuid->clockSeqHiAndReserved |= 0x80;
}
