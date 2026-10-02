#include "Kameda/PauseManager.h"
#include "Kameda/Result2D.h"
#include "Kameda/WipeManager.h"
#include "mathHelper.h"

PauseManager *PauseManager::mThis;    // 0x80416298
bool PauseManager::mIsPause;
int PauseManager::mPauseSelector;
int PauseManager::mPauseChoice;
int PauseManager::mPauseNextChoice;
KartGamePad *PauseManager::mPausePad;

PauseManager::PauseManager(JKRHeap *) {}

void PauseManager::reset() {}

void PauseManager::draw() {}

void PauseManager::exec() {
    //void Result2D::getDrawFlag() {}
    //void Result2D::setDrawFlag(bool) {}
}

bool PauseManager::tstPause() {
    return mIsPause;
}

void PauseManager::wipeOut(int wipeType) {
    mWipeState = 1;
    mWipeType = wipeType;
    GETWipeManager()->getWipeCurtain()->reset();
}

int PauseManager::getPauseChoice() {}

void PauseManager::setTA() {
    mResult2D->setTA();
}

#include "JSystem/JAudio/JASFakeMatch2.h"
