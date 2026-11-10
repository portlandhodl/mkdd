#ifndef _DOLPHIN_UPNP_UPNPHTTP_H_
#define _DOLPHIN_UPNP_UPNPHTTP_H_

#include <dolphin/types.h>

s32 HTTPCreateRequest(void* http, const char* method, const char* uri, const char* version);
s32 HTTPCreateResponse(void* http, s32 status, const char* reason, const char* version);
s32 HTTPAddHeader(void* http, const char* name, const char* value);
s32 HTTPGetHeader(void* http, const char* name, char* value, s32 length);
s32 HTTPGetParameter(void* http, const char* name, char* value, s32 length);
s32 HTTPGetRequestURI(void* http, char* uri, s32 length);
s32 HTTPGetMethod(void* http, char* method);
s32 HTTPGetProtocol(void* http, char* protocol);
s32 HTTPGetStatus(void* http);

#endif
