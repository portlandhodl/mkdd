#include <dolphin/ip/ip.h>
#include <string.h>

#define IP_HLEN(ip) (((ip)->verlen & 0x0F) << 2)

#define IPOPT_EOL  0
#define IPOPT_NOP  1
#define IPOPT_RR   7
#define IPOPT_LSRR 131
#define IPOPT_SSRR 137

s32 IPProcessSourceRoute(IPHeader* ip)
{
    int len;
    u8* opt;
    int optlen;
    int i;
    u8 type;

    opt = (u8*)(ip + 1);
    optlen = IP_HLEN(ip) - 20;
    for (i = 0; i < optlen && opt[i] != IPOPT_EOL; i += len) {
        type = opt[i];
        switch (type) {
        case IPOPT_EOL:
        case IPOPT_NOP:
            len = 1;
            break;
        default:
            if (optlen <= i + 1) {
                return -12;
            }
            len = opt[i + 1];
            if (optlen < i + len) {
                return -12;
            }
            break;
        }
        if (type == IPOPT_LSRR || type == IPOPT_SSRR) {
            if (len < 7) {
                return -12;
            }
            if (opt[i + 2] + 3 <= len) {
                return -12;
            }
            return 0;
        }
    }
    return 0;
}

s32 IPReverseSourceRoute(IPHeader* ip)
{
    int optlen;
    int i;
    u8 type;
    u8* opt;
    u8* end;
    u8* q;
    int len;
    u8* r;
    u8 addr[4];
    u8 tmp[4];

    if (ip == NULL) {
        return -12;
    }
    optlen = IP_HLEN(ip) - 20;
    if (optlen <= 0) {
        return 0;
    }
    opt = (u8*)(ip + 1);
    for (i = 0; i < optlen && opt[i] != IPOPT_EOL; i += len) {
        type = opt[i];
        switch (type) {
        case IPOPT_EOL:
        case IPOPT_NOP:
            len = 1;
            break;
        default:
            if (optlen <= i + 1) {
                return -12;
            }
            len = opt[i + 1];
            if (optlen < i + len) {
                return -12;
            }
            break;
        }
        if (type == IPOPT_LSRR || type == IPOPT_SSRR) {
            if (len < 7) {
                return -12;
            }
            if (opt[i + 2] + 3 <= len) {
                return -12;
            }
            opt += i;
            opt[2] = 4;
            len -= 3;
            len &= ~3;
            memmove(addr, ip->dst, 4);
            end = opt + len;
            q = end - 1;
            memmove(ip->dst, q, 4);
            memmove(q, addr, 4);
            len = (len - 4) / 2;
            end -= 5;
            for (; len >= 4; len -= 4) {
                q = opt + len + 3;
                memmove(tmp, q, 4);
                r = end - len;
                memmove(q, r, 4);
                memmove(r, tmp, 4);
            }
            return 0;
        }
    }
    return 0;
}

s32 IPUpdateRecordRoute(IPHeader* ip, u8* addr)
{
    int len;
    u8* opt;
    int optlen;
    int i;
    u8 type;
    u8 ptr;

    if (ip == NULL || addr == NULL) {
        return -12;
    }
    optlen = IP_HLEN(ip) - 20;
    if (optlen <= 0) {
        return 0;
    }
    opt = (u8*)(ip + 1);
    for (i = 0; i < optlen && opt[i] != IPOPT_EOL; i += len) {
        type = opt[i];
        switch (type) {
        case IPOPT_EOL:
        case IPOPT_NOP:
            len = 1;
            break;
        default:
            if (optlen <= i + 1) {
                return -12;
            }
            len = opt[i + 1];
            if (optlen < i + len) {
                return -12;
            }
            break;
        }
        if (type == IPOPT_RR) {
            if (len < 7) {
                return -12;
            }
            ptr = opt[i + 2];
            if (ptr + 3 <= len) {
                memmove(opt + i + ptr - 1, addr, 4);
                opt[i + 2] = ptr + 4;
            }
            return 0;
        }
    }
    return 0;
}
