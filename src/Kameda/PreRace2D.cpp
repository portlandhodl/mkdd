#include "Kameda/PreRace2D.h"
#include "Kameda/J2DManager.h"
#include "Kameda/WipeManager.h"
#include "Kameda/SequenceInfo.h"
#include "Kameda/SceneCourseSelect.h"

#include "Kaneshige/RaceMgr.h"
#include "Osako/system.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/ResMgr.h"
#include "kartLocale.h"
#include "mathHelper.h"

#include "JSystem/JKernel.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JUtility/JUTAssert.h"

PreRace2DParam::PreRace2DParam() : _48(0), mFlag(1) {}

const u64 PreRace2D::mPreRaceBckTag[2] = {
    'root',
    'M_Pict1'
};

PreRace2D::PreRace2D(JKRHeap *heap)
{
    mNode = new (heap, 0) PreRace2DHioNode();

    mOrtho = System::mspJ2DOrtho;

    mScreen = new (heap, 0) J2DScreen();

    mScreen->set("before_race.blo", 0x40000, J2DManager::mThis->mArchive);

    for (int i = 0; i < 2; i++) {
        mAnim[i].mPane = (J2DPicture *)mScreen->search(mPreRaceBckTag[i]);
        mAnim[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("before_race.bck", J2DManager::mThis->mArchive));
        mAnim[i].mPane->setAnimation(mAnim[i].mAnm);
    }

    for (int i = 0; i < 9; i++) {
        J2DPicture *pane = (J2DPicture *)mScreen->search(0x67757331ULL + i);
#line 86
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mGuS[i] = pane;
    }

    for (int i = 0; i < 3; i++) {
        J2DPicture *pane = (J2DPicture *)mScreen->search(0x6775735f6331ULL + i);
#line 92
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mSC[i] = pane;
    }

    for (int i = 0; i < 8; i++) {
        J2DPicture *pane = (J2DPicture *)mScreen->search(0x6c61705f6d31ULL + i);
#line 98
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mPm[i] = pane;
    }

    J2DPicture *pane = (J2DPicture *)mScreen->search(0x6c617073);
#line 103
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    mLaps = pane;

    pane = (J2DPicture *)mScreen->search(0x636f757273655f31ULL);
#line 108
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    mCourseName = pane;

    mCourseName->hide();

    for (int i = 0; i < 22; i++) {
        J2DPicture *pane = (J2DPicture *)mScreen->search(0x576f72645f61ULL + i);
#line 115
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mRdA[i] = pane;
    }

    switch (RCMGetManager()->getRaceMode()) {
    case GRAND_PRIX:
    case AWARD_DEMO:
    case STAFF_ROLL:
        setGP();
        break;
    case VERSUS_RACE:
        setVS();
        break;
    case TIME_ATTACK:
        setTA();
        break;
    case BALLOON_BATTLE:
        setBalloon();
        break;
    case BOMB_BATTLE:
        setBomb();
        break;
    case ESCAPE_BATTLE:
        setEscape();
        break;
    default:
#line 143
        JUT_ASSERT(0);
        break;
    }

    if (RCMGetManager()->isRaceModeMiniGame()) {
        J2DPicture *pane = (J2DPicture *)mScreen->search(0x506f775f4eULL);
        pane->hide();
        pane = (J2DPicture *)mScreen->search(0x6c61705f4eULL);
        pane->hide();
    }

    switch (RCMGetManager()->getRaceLevel()) {
    case 0:
        mGuS[2]->hide();
        mGuS[1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_5), 0);
        mGuS[0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_0), 0);
        setLap();
        break;
    case 1:
        mGuS[2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_1), 0);
        mGuS[1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_0), 0);
        mGuS[0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_0), 0);
        setLap();
        break;
    case 2:
        mGuS[2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_1), 0);
        mGuS[1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_5), 0);
        mGuS[0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture(Kart2DCommon::Number_0), 0);
        setLap();
        break;
    case 3:
        setLap();
        for (int i = 0; i < 3; i++)
            mSC[i]->hide();
        setMirror();
        break;
    default:
#line 180
        JUT_ASSERT(0);
        break;
    }

    ResTIMG *crsName = (ResTIMG *)ResMgr::getPtr(ResMgr::mcCourseNameBti);
    if (crsName != nullptr) {
        mCourseName->changeTexture(crsName, 0);
        mCourseName->show();
    }

    int laps = RCMGetManager()->getTotalLapNumberForDisplay();
    if (laps >= 1 && laps <= 9) {
        mLaps->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)laps), 0);
    } else {
        mLaps->hide();
    }

    mWipeType = 0;
    if (RCMGetManager()->getRacePhase() != PHASE_CRS_DEMO) {
        mScreen->hide();
        mWipeType = 1;
    }

    init();
}

void PreRace2D::init()
{
    for (int i = 0; i < 2; i++)
        mAnim[i].mFrame = 0.0f;

    mCounter = 0;
    mStatus = 0;
    mScreen->animation();
}

PreRace2D::~PreRace2D() {}

void PreRace2D::draw()
{
    if (!(mNode->mParam.mFlag & 1))
        return;

    switch (mWipeType) {
    case 0:
        if (mCounter < 250) {
            mOrtho->setPort();
            mScreen->draw(0.0f, 0.0f, mOrtho);
        }
        break;
    case 1:
        break;
    default:
#line 258
        JUT_ASSERT(0);
        break;
    }
}

void PreRace2D::calc()
{
    if (!(mNode->mParam.mFlag & 1))
        return;

    switch (mWipeType) {
    case 0: {
        if (mCounter < 180) {
            mStatus = 0;
            if (mCounter == 179)
                mCounter = 0;
        } else if (mCounter < 181) {
            mStatus = 1;
        } else if (mCounter == 181) {
            mStatus = 2;
        } else if (mCounter < 250) {
            mStatus = 3;
        } else {
            mStatus = 4;
            mWipeType = 1;
            mCounter = 0;
            return;
        }

        f32 frame1 = (mAnim[1].mFrame += 1.0f);
        if (frame1 >= 60.0f)
            mAnim[1].mFrame = 0.0f;

        mAnim[0].mFrame = (f32)mCounter;

        if (mStatus == 181 || mStatus == 3) {
            mAnim[1].mFrame = 200.0f;
            mAnim[0].mFrame = 200.0f;
        }

        for (int i = 0; i < 2; i++)
            mAnim[i].mAnm->mCurrentFrame = mAnim[i].mFrame;

        mScreen->animation();
        mCounter++;
        break;
    }
    case 1: {
        int counter = mCounter;
        if (counter == 0)
            break;

        if (counter < 30) {
            J2DManager::mThis->mDuration = 30 - counter;
            WipeManager::mThis->mMario->draw((f32)(30 - mCounter) / 30.0f, JUTColor(0, 0, 0, 255));
            mStatus = 3;
            mCounter++;
        } else {
            J2DManager::mThis->mDuration = 0;
            mStatus = 4;
            mCounter = 0;
        }
        break;
    }
    default:
#line 332
        JUT_ASSERT(0);
        break;
    }
}

void PreRace2D::setGP()
{
    const char *texts[5] = { "K_gu", "K_ra", "K_n", "K_pu", "K_ri" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        if (J2DManager::mNetFlag)
            common->changeStringTexture("VERSUS", mRdA, 22);
        break;
    case FRENCH:
        if (J2DManager::mNetFlag)
            common->changeStringTexture("VS", mRdA, 22);
        break;
    case GERMAN:
        if (J2DManager::mNetFlag)
            common->changeStringTexture("VERSUS", mRdA, 22);
        break;
    case ITALIAN:
        if (J2DManager::mNetFlag) {
            common->changeStringTexture("SFIDA", mRdA, 22);
        } else {
            common->changeStringTexture("GRAN PREMIO", mRdA, 22);
        }
        break;
    case JAPANESE:
        if (J2DManager::mNetFlag) {
            common->changeStringTexture("VS", mRdA, 22);
        } else {
            for (int i = 0; i < 5; i++)
                changeTexture(texts[i], mRdA[i]);
            for (int i = 5; i < 22; i++)
                mRdA[i]->hide();
        }
        break;
    case SPANISH:
        if (J2DManager::mNetFlag)
            common->changeStringTexture("VERSUS", mRdA, 22);
        break;
    default:
#line 396
        JUT_ASSERT(0);
        break;
    }

    if (!J2DManager::mNetFlag) {
        int idx = 21;
        int num;
        if (SceneCourseSelect::mRaceGpCup[gSequenceInfo.getCup()] == ALL_CUP_TOUR)
            num = 16;
        else
            num = 4;

        do {
            mRdA[idx]->show();
            mRdA[idx]->changeTexture(common->getNumberTexture((Kart2DCommon::NumberTextureID)(num % 10)), 0);
            num /= 10;
            idx--;
        } while (num != 0);

        mRdA[idx]->show();
        mRdA[idx]->changeTexture("MarioFontSlash.bti", 0);
        idx--;

        int course = gSequenceInfo.getCourse() + 1;
        do {
            mRdA[idx]->show();
            mRdA[idx]->changeTexture(common->getNumberTexture((Kart2DCommon::NumberTextureID)(course % 10)), 0);
            course /= 10;
            idx--;
        } while (course != 0);
    }

    J2DPicture *pane = mAnim[1].mPane;
#line 429
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);

    if (J2DManager::mNetFlag) {
        pane->changeTexture("Cup_Pict_LAN.bti", 0);
    } else {
        pane->changeTexture(Kart2DCommon::ptr()->getCupIconTexture((ERaceGpCup)SceneCourseSelect::mRaceGpCup[gSequenceInfo.getCup()]), 0);
    }
}

void PreRace2D::setVS() {}

void PreRace2D::setTA() {}

void PreRace2D::setBalloon()
{
    const char *texts[7] = { "H_fu", "H_u", "H_se", "H_n", "K_ba", "K_to", "K_ru" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        common->changeStringTexture("BALLOON BATTLE", mRdA, 22);
        break;
    case FRENCH:
        common->changeStringTexture("BATAILLE DE BALLONS", mRdA, 22);
        break;
    case GERMAN:
        common->changeStringTexture("BALLON-WETTKAMPF", mRdA, 22);
        break;
    case ITALIAN:
        common->changeStringTexture("BATTAGLIA PALLONCINI", mRdA, 22);
        break;
    case JAPANESE:
        for (int i = 0; i < 7; i++)
            changeTexture(texts[i], mRdA[i]);
        for (int i = 7; i < 22; i++)
            mRdA[i]->hide();
        break;
    case SPANISH:
        common->changeStringTexture("BATALLA DE GLOBOS", mRdA, 22);
        break;
    default:
#line 507
        JUT_ASSERT(0);
        break;
    }

    J2DPicture *pane = mAnim[1].mPane;
#line 513
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);

    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(0), 0);
}

void PreRace2D::setBomb()
{
    const char *texts[9] = { "K_do", "K_xtu", "K_ka", "K_n", "Exclamation", "K_bo", "K_mu", "H_he", "H_i" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        common->changeStringTexture("BOB-OMB BLAST", mRdA, 22);
        break;
    case FRENCH:
        common->changeStringTexture("EXPLOSION BOB-OMB", mRdA, 22);
        break;
    case GERMAN:
        common->changeStringTexture("BOB-OMB-WURF", mRdA, 22);
        break;
    case ITALIAN:
        common->changeStringTexture("SCOPPIO DI BOB-OMBE", mRdA, 22);
        break;
    case JAPANESE:
        for (int i = 0; i < 9; i++)
            changeTexture(texts[i], mRdA[i]);
        for (int i = 9; i < 22; i++)
            mRdA[i]->hide();
        break;
    case SPANISH:
        common->changeStringTexture("BOB-OMBARDEO", mRdA, 22);
        break;
    default:
#line 565
        JUT_ASSERT(0);
        break;
    }

    J2DPicture *pane = mAnim[1].mPane;
#line 571
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);

    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(1), 0);
}

void PreRace2D::setEscape()
{
    const char *texts[8] = { "H_i", "H_ta", "H_da", "H_ki", "K_si", "K_xya", "K_i", "K_n" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        common->changeStringTexture("SHINE THIEF", mRdA, 22);
        break;
    case FRENCH:
        common->changeStringTexture("VOLEUR DE SOLEIL", mRdA, 22);
        break;
    case GERMAN:
        common->changeStringTexture("INSIGNIEN-DIEBSTAHL", mRdA, 22);
        break;
    case ITALIAN:
        common->changeStringTexture("RUBA IL SOLE CUSTODE", mRdA, 22);
        break;
    case JAPANESE:
        for (int i = 0; i < 8; i++)
            changeTexture(texts[i], mRdA[i]);
        for (int i = 8; i < 22; i++)
            mRdA[i]->hide();
        break;
    case SPANISH:
        common->changeStringTexture("ASALTO AL SOL", mRdA, 22);
        break;
    default:
#line 622
        JUT_ASSERT(0);
        break;
    }

    J2DPicture *pane = mAnim[1].mPane;
#line 628
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);

    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(2), 0);
}

void PreRace2D::setLap()
{
    const char *texts[3] = { "K_pu", "K_xtu", "K_ra" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    mPm[0]->changeTexture("MarioFontColon.bti", 0);

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        common->changeStringTexture("SPAL", &mPm[1], 7);
        break;
    case FRENCH:
        common->changeStringTexture("SRUOT", &mPm[1], 7);
        break;
    case GERMAN:
        common->changeStringTexture("NEDNUR", &mPm[1], 7);
        mSC[2]->show();
        break;
    case ITALIAN:
        common->changeStringTexture("IRIG", &mPm[1], 7);
        break;
    case JAPANESE:
        for (int i = 0; i < 3; i++)
            changeTexture(texts[i], mPm[i]);
        for (int i = 3; i < 8; i++)
            mPm[i]->hide();
        break;
    case SPANISH:
        common->changeStringTexture("SATLEUV", &mPm[1], 7);
        break;
    default:
#line 682
        JUT_ASSERT(0);
        break;
    }
}

void PreRace2D::setMirror()
{
    const char *texts[2] = { "K_ra", "K_mi" };
    Kart2DCommon *common = Kart2DCommon::ptr();

    switch (KartLocale::getLanguage()) {
    case ENGLISH:
        common->changeStringTexture("RORRIM", mGuS, 9);
        break;
    case FRENCH:
        common->changeStringTexture("RIORIM", mGuS, 9);
        break;
    case GERMAN:
        common->changeStringTexture("LEGEIPS", mGuS, 9);
        break;
    case ITALIAN:
        common->changeStringTexture("ERALUCEPS", mGuS, 9);
        break;
    case JAPANESE:
        common->changeStringTexture("-", mGuS, 1);
        for (int i = 0; i < 2; i++)
            changeTexture(texts[i], mGuS[i + 1]);
        for (int i = 3; i < 9; i++)
            mGuS[i]->hide();
        break;
    case SPANISH:
        common->changeStringTexture("OJEPSE", mGuS, 9);
        break;
    default:
#line 734
        JUT_ASSERT(0);
        break;
    }
}

void PreRace2D::changeTexture(const char *name, J2DPicture *picture)
{
    char buf[32];

    snprintf(buf, 32, "MarioFont%s.bti", name);

    picture->changeTexture(buf, 0);
}

void PreRace2D::start()
{
    switch (mWipeType) {
    case 0:
        mCounter = 180;
        break;
    case 1:
        mCounter = 1;
        break;
    default:
#line 770
        JUT_ASSERT(0);
        break;
    }

    mStatus = 0;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
