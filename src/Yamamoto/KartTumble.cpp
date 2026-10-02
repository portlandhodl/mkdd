#include "Yamamoto/KartTumble.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartTumble::Init(int) {}

void KartTumble::MakeWanWanTumble(ItemObj *) {
    // void ItemObj::getPos(JGeometry::TVec3<float> *) {}
}

void KartTumble::MakeKameTumble(ItemObj *) {}

void KartTumble::MakeStarTumble() {}

void KartTumble::MakeDashTumble() {}

void KartTumble::DoTumble() {}

void KartTumble::DoPakunTumble() {}

void KartTumble::DoHanaTumble() {}

void KartTumble::MakePoiHanaTumble() {}

void KartTumble::DoShootCrashCrl() {}

void KartTumble::DoTumbleCrl() {
    if (_14 != 0) {
        _14--;
    } else {
        _4 &= 0xfe;
    }
}

void KartTumble::DoAfterTumbleCrl() {}
