#ifndef _DOLPHIN_UPNP_UPNPHTTPD_H_
#define _DOLPHIN_UPNP_UPNPHTTPD_H_

#include <dolphin/types.h>
#include <dolphin/os.h>

typedef struct HTTPFile {
    const char* mPath; // 0x00
    void* mData;       // 0x04
    s32 mSize;         // 0x08
} HTTPFile;

typedef struct HTTPConfig {
    OSThread* mThread; // 0x00
    void* mStack;      // 0x04
    u32 mStackSize;    // 0x08
    u32 mPriority;     // 0x0C
    HTTPFile* mFiles;  // 0x10
    u32 mNumFiles;     // 0x14
} HTTPConfig;

void UPnPHttpInit(HTTPConfig* config);
void UPnPHttpStart(void);

BOOL SendResponseOK(s32 sock, s32 size, const char* path, const char* lang);
BOOL SendResponseStatus(s32 sock, s32 status, const char* flags);

int UPnPReplaceEscaped(char* str);
int UPnPRemoveSemicolon(char* str);
int UPnPRemoveDots(char* str);

#endif
