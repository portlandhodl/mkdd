#include "JSystem/JAudio/JAUAudioMgr.h"
#include "JSystem/JAudio/System/JASGadget.h"

#include "macros.h"

template <>
DECL_WEAK JAUSoundInfo *JASGlobalInstance<JAUSoundInfo>::sInstance;

JAUAudioMgr::JAUAudioMgr(JAUSectionHeap *) : JAUSoundInfo(true) {

}

#include "JSystem/JAudio/JASFakeMatch12.h"
