const char *cpaPowerName[4] = {
    "Power_Mozi.1.bti",
    "Power_Mozi.2.bti",
    "Power_Mozi.3.bti",
    "Power_Mozi.4.bti",
};

#include "Osako/Award2D.h"

#include "Inagaki/GameAudioMain.h"
#include "Kameda/SequenceInfo.h"
#include "Osako/NameEntryHelper.h"
#include "Osako/SystemFile.h"
#include "Osako/SystemRecord.h"
#include "Osako/system.h"

#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceTime.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/ResMgr.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

Award2D::Award2D(JKRHeap *heap) {
    Kart2DCommon *common = Kart2DCommon::ptr();
    _24 = gSequenceInfo.mMapInfo.cup;
    _28 = gSequenceInfo.mLevel;
    int kart;
    if (gSequenceInfo._354 == 1 || gSequenceInfo._318[0]._14 < gSequenceInfo._318[1]._14) {
        kart = 0;
    } else {
        kart = 1;
    }
    int rank = gSequenceInfo.get_318_14(kart);
    _2c = (u8 *)gSequenceInfo.get_318_Pad(kart);
    _14.set(gSequenceInfo.get_318_charId0(kart), gSequenceInfo.get_318_charId1(kart), gSequenceInfo.get_318_kartId(kart), _28, rank,
            gSequenceInfo.getRankPoint(kart), gSequenceInfo.get_318_coop(kart), "OSK", gSequenceInfo.get_318_RaceTime(kart));
    bool noTime = !gSequenceInfo.get_318_RaceTime(kart).isAvailable();

    if (gSystemRecord.getGPRecord((ERaceGpCup)_24, (ERaceLevel)_28).less(_14) && rank < 3) {
        _8 = 1;
        strncpy(_9, gSystemRecord.mDefaultName, 4);
    } else {
        _8 = 0;
    }

    mScreen = new (heap, 0) J2DScreen();
    JKRArchive *arc = ResMgr::getArchive(ResMgr::mcArcAward2D);
    mScreen->setPriority("Award2d.blo", 0x20000, arc);

    void *brk;
    void *bck = JKRFileLoader::getGlbResource("Award2d.bck", arc);
    void *btk;
    btk = (btk, JKRFileLoader::getGlbResource("Award2d.btk", arc));
    brk = (brk, JKRFileLoader::getGlbResource("Award2d.brk", arc));

    _48 = (u8 *)J2DAnmLoaderDataBase::load(bck);
    _4c = (u8 *)J2DAnmLoaderDataBase::load(bck);
    _50 = (u8 *)J2DAnmLoaderDataBase::load(btk);
    _54 = (u8 *)J2DAnmLoaderDataBase::load(brk);

    mScreen->setAnimation((J2DAnmTransform *)_48);
    mScreen->setAnimation((J2DAnmTextureSRTKey *)_50);
    mScreen->setAnimation((J2DAnmTevRegKey *)_54);

    _34._8 = (u8 *)J2DAnmLoaderDataBase::load(bck);
    _34._c = (u8 *)J2DAnmLoaderDataBase::load(btk);
    ((J2DAnmBase *)_34._c)->searchUpdateMaterialID(mScreen);
    _34._10 = (u8 *)J2DAnmLoaderDataBase::load(brk);
    ((J2DAnmBase *)_34._10)->searchUpdateMaterialID(mScreen);

    J2DPicture *pPane = (J2DPicture *)mScreen->search(0x4f4b5f77623131ull);
    pPane->setAnimation((J2DAnmTransform *)_34._8);
    pPane->setAnimation((J2DAnmTextureSRTKey *)_34._c);
    pPane->setAnimation((J2DAnmTevRegKey *)_34._10);
#line 134
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcRace2D, "timg/white.bti"), 0);

    if (rank >= 3) {
        pPane = (J2DPicture *)mScreen->search(0x436f6e674dull);
#line 143
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pPane->changeTexture("GoTrophy.bti", 0);
    }

    for (int i = 0; i < 8; i++) {
        pPane = (J2DPicture *)mScreen->search(0x706f7331ull + i);
        if (i == rank) {
#line 156
            JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
            J2DPicture *pos = pPane;
            pPane->changeTexture(common->getPosImg(rank), 0);
            pPane->show();
            pPane = (J2DPicture *)mScreen->search(0x706f734230ull);
#line 162
            JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
            pPane->setWhite(pos->getWhite());
            pPane->setBlack(pos->getBlack());
        } else {
            pPane->hide();
        }
    }

    pPane = (J2DPicture *)mScreen->search(0x43686130ull);
#line 176
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getKartIconTexture(gSequenceInfo.get_318_kartId(kart)), 0);

    pPane = (J2DPicture *)mScreen->search(0x43686131ull);
#line 181
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getCharIconTexture(gSequenceInfo.get_318_charId1(kart)), 0);

    pPane = (J2DPicture *)mScreen->search(0x43686132ull);
#line 186
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getCharIconTexture(gSequenceInfo.get_318_charId0(kart)), 0);

    RaceTime raceTime = gSequenceInfo.get_318_RaceTime(kart);
    int minutes, seconds, milliseconds;
    raceTime.get(&minutes, &seconds, &milliseconds);

    J2DPicture *pics[3];
    for (int i = 0; i < 2; i++) {
        pPane = (J2DPicture *)mScreen->search(0x74696d654e35ull + i);
#line 199
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pics[i] = pPane;
    }
    common->changeNumberTexture(minutes, pics, 2, false, noTime);

    for (int i = 0; i < 2; i++) {
        pPane = (J2DPicture *)mScreen->search(0x74696d654e33ull + i);
#line 208
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pics[i] = pPane;
    }
    common->changeNumberTexture(seconds, pics, 2, true, noTime);

    for (int i = 0; i < 3; i++) {
        pPane = (J2DPicture *)mScreen->search(0x74696d654e30ull + i);
#line 217
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pics[i] = pPane;
    }
    common->changeNumberTexture(milliseconds, pics, 3, true, noTime);

    u64 colonTag = 0x74696d657431ull;
    pPane = (J2DPicture *)mScreen->search(colonTag++);
#line 224
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, "timg/MarioFontColon2.bti"), 0);

    pPane = (J2DPicture *)mScreen->search(colonTag);
#line 229
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, "timg/MarioFontColon1.bti"), 0);

    pPane = (J2DPicture *)mScreen->search(0x4375704e616d65ull);
#line 235
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getCupNameTexture((ERaceGpCup)_24), 0);

    pPane = (J2DPicture *)mScreen->search(0x43757050696374ull);
#line 241
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getCupIconTexture((ERaceGpCup)_24), 0);

    for (int i = 0; i < 3; i++) {
        pPane = (J2DPicture *)mScreen->search(0x506f696e7431ull + i);
#line 250
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pics[i] = pPane;
    }
    common->changeNumberTexture(gSequenceInfo.getRankPoint(kart), pics, 3, false, false);

    pPane = (J2DPicture *)mScreen->search(0x506f696e744dull);
#line 256
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture(common->getPartsLocName(0), 0);

    pPane = (J2DPicture *)mScreen->search(0x506f77ull);
#line 262
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
    pPane->changeTexture((ResTIMG *)JKRFileLoader::getGlbResource(cpaPowerName[_28], arc), 0);

    for (int i = 0; i < 9; i++) {
        pPane = (J2DPicture *)mScreen->search(0x456630ull + i);
        pPane->setAnimation((J2DAnmTransform *)_4c);
#line 272
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pPane->changeTexture(common->getPartName(2), 0);
    }

    for (int i = 0; i < 2; i++) {
        pPane = (J2DPicture *)mScreen->search(0x4e45556e64657231ull + i);
#line 282
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pPane->changeTexture(common->getPartName(3), 0);
    }

    for (int i = 0; i < 3; i++) {
        pPane = (J2DPicture *)mScreen->search(0x4368615730ull + i);
#line 292
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture)
        pPane->changeTexture(common->getPartName(1), 0);
    }

    if (_8) {
        J2DPane *slots[3];
        for (int i = 0; i < 3; i++) {
            slots[i] = mScreen->search(0x4e536c6f7431ull + i);
        }
        _30 = new (heap, 0) NameEntryHelper((J2DPane **)slots, _9, heap, (JUTGamePad *)_2c, false);
    }

    mState = 0;
}

void Award2D::calc() {
    if (mState == 0) {
        return;
    }

    switch (mState) {
    case 1:
        GameAudio::Main::getAudio()->startSystemSe(0x2005c);
        _6 = 0;
        _4 = 0;
        mState = 2;
    case 2:
        if (gSequenceInfo._35c != 0) {
            gSystemFile._6024 = 1;
        }
        if (_8 == 0)
            goto noedit;
        if (_4 >= 163) {
            mState = 4;
            _30->startEdit();
        }
        goto edited;
    noedit:
        if (_4 >= 120) {
            mState = 3;
        }
    edited:
        *(f32 *)(_48 + 8) = (f32)_4;
        *(f32 *)(_50 + 8) = (f32)_4;
        *(f32 *)(_54 + 8) = (f32)_4;
        _4 = _4 + 1;
        break;
    case 3:
        if (*(u32 *)(_2c + 0x1c) & 0x100) {
            mState = 6;
        }
        break;
    case 4:
        switch (*(int *)((u8 *)_30 + 0x14)) {
        case 3:
            mState = 5;
            _34.zoomIn();
            break;
        }
        break;
    case 5:
        if (*(u32 *)(_2c + 0x1c) & 0x100) {
            GameAudio::Main::getAudio()->startSystemSe(0x20002);
            mState = 6;
            _14.setName(_9);
            gSystemRecord.setGPRecord((ERaceGpCup)_24, (ERaceLevel)_28, _14);
            gSystemRecord.setDefaultName(_9);
            bool special = gSequenceInfo._35e != 0 || gSequenceInfo._35f != 0;
            if (special) {
                gSequenceInfo._360 = 1;
                gSystemFile._6024 = 0;
            } else {
                gSystemFile._6024 = 1;
            }
            _34.zoomOut();
        } else if (*(u32 *)(_2c + 0x1c) & 0x200) {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mState = 4;
            _30->startEdit();
            _34.zoomOut();
        }
        break;
    }
afterSwitch:

    *(f32 *)(_4c + 8) = (f32)_6;
    if (++_6 >= 300) {
        _6 = 110;
    }

    if (_8 != 0) {
        _30->calc();
    }
    _34.calcAnm();
    mScreen->animation();
}

void Award2D::draw() {
    if (mState != 0) {
        J2DOrthoGraph *ortho = System::mspJ2DOrtho;
        ortho->setPort();
        mScreen->draw(0.0f, 0.0f, ortho);
        if (_8 != 0) {
            _30->draw(ortho);
        }
    }
}

void Award2D::OKButton::calcAnm() {
    if (_0 == 0) {
        goto after;
    }

    *(f32 *)(_8 + 8) = (f32)_4;
    *(f32 *)(_c + 8) = (f32)_6;
    *(f32 *)(_10 + 8) = (f32)_6;
    if (++_6 >= 120) {
        _6 = 0;
    }

after:
    switch (_0) {
    case 1:
        if (++_4 >= 195) {
            _0 = 2;
        }
        break;
    case 3:
        if (--_4 < 180) {
            _0 = 0;
        }
        break;
    }
}

void Award2D::OKButton::zoomIn() {
    _0 = 1;
    _6 = 0;
    _4 = 180;
}

void Award2D::OKButton::zoomOut() {
    _0 = 3;
    _4 = 194;
}
