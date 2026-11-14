#include "JSystem/JAUdio/JAUAudience.h"
#include "JSystem/JAudio/System/JASHeapCtrl.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/JASFakeMatch.h"

typedef JAUDopplerAudible<4> TDopplerAudible4;
typedef JAUDopplerAudible<2> TDopplerAudible2;
typedef JAUDopplerAudible<1> TDopplerAudible1;
typedef JAUAudible<4> TAudible4;
typedef JAUAudible<2> TAudible2;
typedef JAUAudible<1> TAudible1;

JASMemPool<TDopplerAudible4> JASPoolAllocObject<TDopplerAudible4>::memPool_;
JASMemPool<TDopplerAudible2> JASPoolAllocObject<TDopplerAudible2>::memPool_;
JASMemPool<TDopplerAudible1> JASPoolAllocObject<TDopplerAudible1>::memPool_;
JASMemPool<TAudible4> JASPoolAllocObject<TAudible4>::memPool_;
JASMemPool<TAudible2> JASPoolAllocObject<TAudible2>::memPool_;
JASMemPool<TAudible1> JASPoolAllocObject<TAudible1>::memPool_;

// sdata2 ordering is incorrect, probably requires an exact match in testInstanciate...

void JAUAudience_withSetting::updateSetting() {
#line 30
    JUT_ASSERT(setting_.minVolume >= 0.0f);
    setting_._3c = (setting_._20 - setting_._1c) / setting_._10;
    setting_._40 = (setting_._18 - setting_._1c) / setting_._14;
    setting_._44 = (setting_.minVolume - 1.0f) / (setting_._04 - setting_._08);
    setting_._48 = (f32)setting_._38 / (setting_._04 - setting_._08);
    setting_._4c = (setting_._28 - setting_._24) / (setting_._04 - setting_._08);
}

namespace {

    void testInstanciate() {
        //JAUGenericAudible_<4, JAUDopplerAudibleChannel, JAUDopplerAudibleAbsPos> *audible = new JAUGenericAudible_<4, JAUDopplerAudibleChannel, JAUDopplerAudibleAbsPos>[4];
       
        ((JAUDopplerAudible<4>*)NULL)->getOuterParams(0);
        new JAUDopplerAudience<4>;
        JASPoolAllocObject< JAUDopplerAudible<4> >::newMemPool(0x4e);

    }
}
