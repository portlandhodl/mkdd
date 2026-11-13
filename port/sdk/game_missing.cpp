// Game and JSystem symbols the decomp declares but does not define in a form the
// host build can link. Each is either data the original defined elsewhere, or a
// function whose source has not been written yet (logged on first call).
#include "port_sdk.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRThread.h"
#include "JSystem/JSupport/JSUStream.h"
#include "JSystem/JAudio/Interface/JAISoundHandles.h"
#include "JSystem/JAudio/dsp.h"
#include "Inagaki/GameSoundMgr.h"
#include "Kameda/Result2D.h"
#include "Sato/ItemBanana.h"
#include "Sato/NeckCtrl.h"
#include "Kawano/driver.h"
#include "JSystem/J3D/J3DMaterialAnm.h"
#include <string.h>

// J3DMaterialAttach stores the vtable address straight into a J3DTexNoAnm; give it
// a copy of the Itanium vtable slots, so its address works as a vptr.
extern "C" void *port_vt_J3DTexNoAnm[8];
void *port_vt_J3DTexNoAnm[8];
// driver.cpp reads DriverModel's callback table through its mangled name.
J3DJointCallBack port_DriverModel_mscaCallBackTbl[27];

__attribute__((constructor)) static void port_fakematch_tables()
{
    J3DTexNoAnm proto;
    memcpy(port_vt_J3DTexNoAnm, *(void ***)&proto, 3 * sizeof(void *)); // calc, ~D1, ~D0
    memcpy(port_DriverModel_mscaCallBackTbl, DriverModel::mscaCallBackTbl, sizeof port_DriverModel_mscaCallBackTbl);
}

// static data members defined in units the decomp has not split out yet
CharIKParamMgr *CharIKParamMgr::mMgr;
J3DAnmTexPattern *ItemFakeItemBox::sTexPattern;
J3DAnmTextureSRTKey *ItemFakeItemBox::mAnmTex;
GameAudio::CustomAudience<4> *GameAudio::ShoreSoundMgr::smAudience;
template <> f32 GameAudio::CustomAudience<4>::smCameraVolume[4] = {};

// MWCC runtime: is a pointer-to-member-function non-null? (Itanium layout: {ptr, adj})
extern "C" int __ptmf_test(void *pmf) { return ((long *)pmf)[0] != 0; }

JKRArchive::JKRArchive() {}

void JKRThreadSwitch::draw(JKRThreadName_ *list) { draw(list, nullptr); }
JKRThreadSwitch::~JKRThreadSwitch() {}

void JSUOutputStream::_0C() { MKDD_STUB("JSUOutputStream::_0C"); }

bool JAISoundHandles::isActive() const
{
    for (int i = 0; i < numHandles_; i++)
        if (handles_[i].isSoundAttached())
            return true;
    return false;
}

Result2D::~Result2D() { MKDD_STUB("Result2D::~Result2D"); }
void Result2D::getCharPos(int) { MKDD_STUB("Result2D::getCharPos"); }

// DSP microcode interface: no DSP on the host; the audio mixer replaces it later.
void DspBoot(DSPCallback) { MKDD_STUB("DspBoot"); }
void DspFinishWork(u16) {}
void DsyncFrame2(u32, u32, u32) {}
void DSPReleaseHalt2(u32) {}
void DsetupTable(u32, u32, u32, u32, u32) {}
void DsetMixerLevel(f32) {}
