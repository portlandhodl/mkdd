#include "Kawano/driverTPAnim.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DMaterialAnm.h"
#include "Kaneshige/KartLoader.h"
#include "Kawano/driverMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"

static const f32 sZeroQuatset[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

const char *DriverTPAnim::mscpaBTPFileName[14] = {
    "wink.btp",       "a_smile.btp",   "speak.btp",     "a_kizetsu.btp",
    "a_damage.btp",   "b_damage_a.btp", "b_damage_b.btp", "b_fire.btp",
    "e_1st_b.btp",    "e_bad_a.btp",   "e_bad_b1.btp",  "e_bad_b2.btp",
    "e_good_a.btp",   "e_good_b.btp",
};

u32 DriverTPAnim::mProbabilityWink = 60;
u32 DriverTPAnim::mMinFrameWink = 5;
u32 DriverTPAnim::mFrameSmile = 60;

DriverTPAnim::DriverTPAnim(DriverModel *model, int dataID)
{
    mDriver = model;
    mDataID = dataID;
    if (mDataID == TPA_NONE)
        return;

    mCounter = 0;
    mAnimIdx = 0;
    LoadBTP();
}

DriverTPAnim::~DriverTPAnim()
{
    for (int i = 0; i < 14; i++)
        mAnmTrans[i] = nullptr;
}

void DriverTPAnim::LoadBTP()
{
    for (int i = 13; i >= 0; i--) {
        char buf[64];
        snprintf(buf, 64, "/Driver/%s/%s", DriverLoader::getDriverName(mDataID), mscpaBTPFileName[i]);

        void *file = ObjUtility::getPtrMainArc(buf);
        if (file != nullptr) {
            mAnmTrans[i] = (J3DAnmTexPattern *)J3DAnmLoaderDataBase::load(file, (J3DAnmLoaderDataBaseFlag)0);

            if (mDriver->getModelData()->getMaterialTable().getMaterialNodePointer(1)->getMaterialAnm() == nullptr) {
                J3DMaterialAnm *anm = new J3DMaterialAnm();
                mDriver->getModelData()->getMaterialTable().getMaterialNodePointer(1)->change();
                mDriver->getModelData()->getMaterialTable().getMaterialNodePointer(1)->setMaterialAnm(anm);
            }

            mDriver->getModelData()->getMaterialTable().entryTexNoAnimator(mAnmTrans[i]);
        } else {
            mAnmTrans[i] = nullptr;
        }
    }
}

void DriverTPAnim::setDriverTPAnim(TPAStateEnum state)
{
    if (mDataID == TPA_NONE)
        return;

    int old = mAnimIdx;
    mAnimIdx = state;
    mAnm[state].changeMatAnm(&mAnm[old]);
}

void DriverTPAnim::startDriverTPAnim(TPAStateEnum state)
{
    if (mDataID == TPA_NONE)
        return;

    J3DFrameCtrl *ctrl = mAnm[state].getFrameCtrl();
    ctrl->setFrame((f32)ctrl->mStartFrame);
    ctrl->setRate(0.0f);
    ctrl->mState = 0;
}

void DriverTPAnim::update()
{
    int b1 = 0;
    int b2 = 0;
    int dataID = mDataID;

    if (DriverManager::ptr()->getDriverFlag(mDataID) != 0 && mAnimIdx != TPA_SPEAK)
        b1 = 1;
    if (mDriver->getCharSoundMgr()->isSpeak())
        b2 = 1;

    if (mDataID == TPA_NONE)
        return;

    switch (mAnimIdx) {
    case TPA_1STB:
    case TPA_GOODA:
    case TPA_GOODB:
        if (b1 && b2) {
            setDriverTPAnim(TPA_SPEAK);
            startDriverTPAnim(TPA_SPEAK);
            break;
        }
        goto modelSwitch;
    case TPA_WINK:
    case TPA_SPEAK:
    case TPA_FIRE:
    case TPA_BADA:
    case TPA_BADB1:
    case TPA_BADB2:
    modelSwitch:
        switch (mDriver->mState) {
        case 128:
        case 129:
            if (b2 == 0) {
                setDriverTPAnim(TPA_1STB);
            } else if (b1) {
                setDriverTPAnim(TPA_SPEAK);
                startDriverTPAnim(TPA_SPEAK);
            }
            break;
        case 130:
        case 131:
            if (b2 == 0) {
                setDriverTPAnim(TPA_GOODA);
            } else if (b1) {
                setDriverTPAnim(TPA_SPEAK);
                startDriverTPAnim(TPA_SPEAK);
            }
            break;
        case 132:
            if (b2 == 0) {
                setDriverTPAnim(TPA_GOODB);
            } else if (b1) {
                setDriverTPAnim(TPA_SPEAK);
                startDriverTPAnim(TPA_SPEAK);
            }
            break;
        case 133:
            setDriverTPAnim(TPA_BADA);
            break;
        case 134:
            setDriverTPAnim(TPA_BADB1);
            break;
        case 135:
            setDriverTPAnim(TPA_BADB2);
            break;
        default:
            if (mDriver->IsFire()) {
                setDriverTPAnim(TPA_FIRE);
            } else if (mDriver->IsDriverDamage()) {
                setDriverTPAnim(TPA_ADAMAGE);
                startDriverTPAnim(TPA_ADAMAGE);
            } else if (mDriver->IsDamageS12()) {
                setDriverTPAnim(TPA_BDAMAGEA);
                startDriverTPAnim(TPA_BDAMAGEA);
            } else if (mDriver->IsDamageL()) {
                setDriverTPAnim(TPA_BDAMAGEB);
                startDriverTPAnim(TPA_BDAMAGEB);
            } else if (mDriver->IsPiyo()) {
                setDriverTPAnim(TPA_PIYO);
                startDriverTPAnim(TPA_PIYO);
            } else if (mAnimIdx != TPA_SPEAK) {
                if (DriverManager::ptr()->getDriverFlag(mDataID) != 0 &&
                    mDriver->getCharSoundMgr()->isSpeak()) {
                    setDriverTPAnim(TPA_SPEAK);
                    startDriverTPAnim(TPA_SPEAK);
                } else {
                    setDriverTPAnim(TPA_WINK);
                }
            } else if (!mDriver->getCharSoundMgr()->isSpeak()) {
                setDriverTPAnim(TPA_WINK);
            }
            break;
        }
        break;
    default:
        break;
    }

    switch (mAnimIdx) {
    case TPA_WINK:
        update_TPAWink();
        break;
    case TPA_SMILE:
        update_TPASmile();
        break;
    case TPA_SPEAK:
        update_TPASpeak();
        break;
    case TPA_PIYO:
        update_TPAPiyo();
        break;
    case TPA_ADAMAGE:
        update_TPAADamage();
        break;
    case TPA_BDAMAGEA:
        update_TPABDamageA();
        break;
    case TPA_BDAMAGEB:
        update_TPABDamageB();
        break;
    case TPA_FIRE:
        update_TPAFire();
        break;
    case TPA_1STB:
    case TPA_BADA:
    case TPA_BADB1:
    case TPA_BADB2:
    case TPA_GOODA:
    case TPA_GOODB:
        update_basic();
        break;
    default:
        break;
    }
}

void DriverTPAnim::update_TPAWink()
{
    mAnm[0].mFrameCtrl.update();
    mAnm[0].anmFrameProc();

    if (mAnm[0].mFrameCtrl.mRate == 0.0f) {
        mCounter++;
        if (mCounter > mMinFrameWink) {
            if (stGetRnd(5)->getRandomMax(mProbabilityWink) == 0) {
                mAnm[0].mFrameCtrl.mFrame = (f32)mAnm[0].mFrameCtrl.mStartFrame;
                mAnm[0].mFrameCtrl.mRate = 0.0f;
                mAnm[0].mFrameCtrl.mState = 0;
            }
        }
    } else {
        mCounter = 0;
    }
}

void DriverTPAnim::update_TPASmile()
{
    J3DFrameCtrl *ctrl = &mAnm[mAnimIdx].mFrameCtrl;

    if (ctrl->mFrame < 8.0f || ctrl->mFrame >= 9.0f) {
        ctrl->update();
        mAnm[mAnimIdx].anmFrameProc();

        if (ctrl->mRate == 0.0f)
            setDriverTPAnim(TPA_WINK);

        mCounter = 0;
    } else {
        mCounter++;
        if (mCounter < mFrameSmile)
            ctrl->setFrame(8.0f);
        else
            ctrl->setFrame(9.0f);
    }
}

void DriverTPAnim::update_TPASpeak()
{
    mAnm[mAnimIdx].mFrameCtrl.update();
    mAnm[mAnimIdx].anmFrameProc();
}

void DriverTPAnim::update_TPAFire()
{
}

void DriverTPAnim::update_TPAPiyo()
{
    J3DFrameCtrl *ctrl = &mAnm[mAnimIdx].mFrameCtrl;

    if (ctrl->mFrame < 1.0f || ctrl->mFrame >= 2.0f) {
        ctrl->update();
        mAnm[mAnimIdx].anmFrameProc();

        if (ctrl->mRate == 0.0f)
            setDriverTPAnim(TPA_WINK);
    } else {
        if (mDriver->IsPiyo())
            ctrl->setFrame(1.0f);
        else
            ctrl->setFrame(2.0f);
    }
}

void DriverTPAnim::update_TPAADamage()
{
    J3DFrameCtrl *ctrl = &mAnm[mAnimIdx].mFrameCtrl;

    if (mDriver->IsDrive()) {
        if (ctrl->mFrame < 1.0f || ctrl->mFrame >= 2.0f) {
            mAnm[mAnimIdx].mFrameCtrl.update();
            mAnm[mAnimIdx].anmFrameProc();

            if (ctrl->mRate == 0.0f)
                setDriverTPAnim(TPA_WINK);
        } else {
            if (mDriver->IsDriverDamage())
                ctrl->setFrame(1.0f);
            else
                ctrl->setFrame(2.0f);
        }
    } else {
        mAnm[mAnimIdx].mFrameCtrl.update();
        mAnm[mAnimIdx].anmFrameProc();

        if (ctrl->mRate == 0.0f)
            setDriverTPAnim(TPA_WINK);
    }
}

void DriverTPAnim::update_TPABDamageA()
{
    J3DFrameCtrl *ctrl = &mAnm[mAnimIdx].mFrameCtrl;

    if (ctrl->mFrame < 35.0f || ctrl->mFrame >= 36.0f) {
        ctrl->update();
        mAnm[mAnimIdx].anmFrameProc();

        if (ctrl->mRate == 0.0f)
            setDriverTPAnim(TPA_WINK);
    } else {
        if (mDriver->IsDamageS12())
            ctrl->setFrame(35.0f);
        else
            ctrl->setFrame(36.0f);
    }
}

void DriverTPAnim::update_TPABDamageB()
{
    J3DFrameCtrl *ctrl = &mAnm[mAnimIdx].mFrameCtrl;

    if (ctrl->mRate != 0.0f) {
        ctrl->update();
        mAnm[mAnimIdx].anmFrameProc();
    }

    if (ctrl->mRate == 0.0f) {
        if (!mDriver->IsDamageL12()) {
            setDriverTPAnim(TPA_ADAMAGE);
            startDriverTPAnim(TPA_ADAMAGE);
        }
    }

    int idx = mAnimIdx;
    *(f32 *)((char *)this + idx * 0x20 + 0x68) = 2.0f;
}

void DriverTPAnim::update_basic()
{
    mAnm[mAnimIdx].mFrameCtrl.update();
    mAnm[mAnimIdx].anmFrameProc();
}

void DriverTPAnim::setup(DriverModel *model)
{
    if (mDataID == TPA_NONE)
        return;

    for (int i = 0; i < 14; i++) {
        if (mAnmTrans[i] != nullptr) {
            mAnm[i].mModel = model;
            mAnm[i].mAnmBase = mAnmTrans[i];
            mAnm[i].initFrameCtrl(mAnm[i].mAnmBase);
            mAnm[i].getFrameCtrl()->setAttribute(1);
        }
    }

    mAnm[6].getFrameCtrl()->setAttribute(0);
    mAnm[2].getFrameCtrl()->setAttribute(2);
    mAnm[9].getFrameCtrl()->setAttribute(2);
    mAnm[11].getFrameCtrl()->setAttribute(2);
    mAnm[12].getFrameCtrl()->setAttribute(2);
    mAnm[13].getFrameCtrl()->setAttribute(2);
    mAnm[8].getFrameCtrl()->setAttribute(2);
}
