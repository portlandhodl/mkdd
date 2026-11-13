#include "JSystem/JAudio/JAUAudioMgr.h"
#include "JSystem/JAudio/System/JASGadget.h"

#include "macros.h"

#ifdef __MWERKS__
template <>
DECL_WEAK JAUSoundInfo *JASGlobalInstance<JAUSoundInfo>::sInstance;
#endif

JAUAudioMgr::JAUAudioMgr(JAUSectionHeap *) : JAUSoundInfo(true) {

}

#include "JSystem/JAudio/JASFakeMatch12.h"
