#ifndef _DOLPHIN_UPNP_UPNPHTTP_H_
#define _DOLPHIN_UPNP_UPNPHTTP_H_

#include <dolphin/types.h>

s32 HTTPCreateRequest(char* msg, const char* method, const char* uri, const char* version);
s32 HTTPCreateResponse(char* msg, s32 status, const char* reason, const char* version);
s32 HTTPAddHeader(char* msg, const char* name, const char* value);
s32 HTTPGetHeader(char* msg, const char* name, char* value, s32 size);
s32 HTTPGetParameter(char* msg, const char* name, char* value, s32 size);
s32 HTTPGetRequestURI(char* msg, char* uri, s32 size);
s32 HTTPGetMethod(char* msg, char* method);
s32 HTTPGetProtocol(char* msg, char* protocol);
s32 HTTPGetStatus(char* msg);

#endif
