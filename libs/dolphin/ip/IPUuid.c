#include <dolphin/types.h>

#include <ctype.h>
#include <stdio.h>

typedef struct IPUuid {
    u32 timeLow;
    u16 timeMid;
    u16 timeHiAndVersion;
    u8 clockSeqHiAndReserved;
    u8 clockSeqLow;
    u8 node[6];
} IPUuid;

char* IPGetUuidString(const IPUuid* uuid, char* string)
{
    int i;
    char* p;

    p = string;
    p += sprintf(p, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-", uuid->timeLow, uuid->timeMid, uuid->timeHiAndVersion,
                 uuid->clockSeqHiAndReserved, uuid->clockSeqLow);
    for (i = 0; i < 6; i++) {
        p += sprintf(p, "%2.2x", uuid->node[i]);
    }
    return string;
}

s32 IPScanUuid(const char* string, IPUuid* uuid)
{
    int c;
    int x;
    u8* p;
    int i;
    int n;

    p = (u8*)uuid;

    for (i = 0, n = 0; i < 36; i++, string++) {
        switch (i) {
        case 8:
        case 13:
        case 18:
        case 23:
            if (*string != '-') {
                return -1;
            }
            break;
        default:
            c = *string;
            if (!isxdigit(c)) {
                return -1;
            }
            if (_isdigit(c)) {
                c -= '0';
            } else {
                c = tolower(c) - 'a' + 10;
            }
            if (n & 1) {
                x = (x << 4) | c;
                *p++ = x;
            } else {
                x = c;
            }
            n++;
            break;
        }
    }
    return 0;
}
