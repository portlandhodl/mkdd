#include "Kawano/PrintPal.h"

#include "Inagaki/GameAudioMain.h"
#include "Osako/kartPad.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "Kameda/ReadPrintMessage.h"
#include "Osako/FontMgr.h"
#include "Osako/ResMgr.h"
#include "Osako/system.h"
#include "string.h"

static const char sNum1[8] __attribute__((section(".rodata"))) = "Num1";
static const char sNum2[8] __attribute__((section(".rodata"))) = "Num2";
static const char sNum3[8] __attribute__((section(".rodata"))) = "Num3";

PrintPal::PrintPal(JKRHeap *heap)
{
    JKRGetCurrentHeap()->getFreeSize();

    mOrthoGraph = System::getJ2DOrtho();

    mArchive = JKRMountArchive(ResMgr::getPtr(ResMgr::mcArcSystem, "Screen/pal60.arc"),
                               heap,
                               JKRArchive::MOUNT_DIRECTION_HEAD);

    mScreenLogo = new (heap, 0) J2DScreen();
    mScreenLogo->J2DScreen::set("NintendoLogo.blo", 0x40000, mArchive);

    mScreenPal = new (heap, 0) J2DScreen();
    mScreenPal->J2DScreen::set("Pal_Hz.blo", 0x40000, mArchive);

    mTransform = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
        JKRFileLoader::getGlbResource("Pal_Hz.bck", mArchive));
    mScreenPal->setAnimation(mTransform);

    mPane = (J2DPane *)mScreenPal->search('n_an');

    mTextBox = (J2DTextBox *)mScreenPal->search('T_tx1');
    mTextBox->setFont(FontMgr::mspResFont);
    mTextBox->getFontSize(mFontSize);

    mTextBoxYes = (J2DTextBox *)mScreenPal->search('T_yes');
    mTextBoxYes->setFont(FontMgr::mspResFont);
    mTextBoxYes->getFontSize(mFontSizeYes);

    mTextBoxNo = (J2DTextBox *)mScreenPal->search('T_no');
    mTextBoxNo->setFont(FontMgr::mspResFont);
    mTextBoxNo->getFontSize(mFontSizeNo);

    mTextBoxNum = (J2DTextBox *)mScreenPal->search('Num1');
    mTextBoxNum->setFont(FontMgr::mspResFont);

    mPictureYes = (J2DPicture *)mScreenPal->search('lyes');
    mPictureYes->setAnimation(mTransform);

    mPictureNo = (J2DPicture *)mScreenPal->search('lno');
    mPictureNo->setAnimation(mTransform);
    mPictureNo->show();

    mPictureBack = (J2DPicture *)mScreenPal->search('back');

    mPictureNintendo = (J2DPicture *)mScreenLogo->search('m_nin1');
    mPictureDolby = (J2DPicture *)mScreenLogo->search('Dlby0');

    void *resource = JKRGetNameResource("Pal_Hz.bpk", mArchive);
    for (int i = 0; i < 4; ++i)
    {
        mColors[i] = (J2DAnmColor *)J2DAnmLoaderDataBase::load(resource);
        mColors[i]->searchUpdateMaterialID(mScreenPal);
    }

    mTextBoxYes->setAnimation(mColors[0]);
    mTextBoxNo->setAnimation(mColors[1]);
    mPictureYes->setAnimation(mColors[2]);
    mPictureNo->setAnimation(mColors[3]);

    mSTRKey = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(
        JKRFileLoader::getGlbResource("Pal_Hz.btk", mArchive));
    mScreenPal->setAnimation(mSTRKey);

    mColorRes = ResMgr::getPtr(ResMgr::mcColor);
    mPalRes = ResMgr::getPtr(ResMgr::mcEuro60);

    JUTColor color =
        ReadPrintMessage::getColor(mColorRes,
                                   ReadPrintMessage::getDefaultColor(mPalRes));
    mTextBox->setFontColor(color, color);

    char buffer[512];
    for (int i = 0; i < 512; ++i)
    {
        buffer[i] = ' ';
    }
    JKRGetCurrentHeap()->getFreeSize();
    buffer[511] = '\0';
    mTextBox->setString(buffer);
    buffer[63] = '\0';
    mTextBoxYes->setString(buffer);
    mTextBoxNo->setString(buffer);
    buffer[1] = '\0';
    mTextBoxNum->setString(buffer);
}

void PrintPal::init(PrintPal::MessageID messageID)
{
    mMsgId = messageID;

    mState = 2;
    mColorsFrame = 0;

    if (isChoice())
    {
        mOption = 0;
    }
    else
    {
        mOption = 2;
    }

    if (isChoice())
    {
        mPane->show();
        mTextBoxNum->show();
    }
    else
    {
        mPane->hide();
        mTextBoxNum->hide();
    }

    if (isMessage())
    {
        mTextBox->show();
    }
    else
    {
        mTextBox->hide();
    }

    if (isMessage())
    {
        int bmgMessageID = ReadPrintMessage::getBmgMessageID(mMsgId);
        char *message = ReadPrintMessage::getMessage(mPalRes, bmgMessageID);

        mTextBox->setFontSize(
            mFontSize.x * ReadPrintMessage::getRate(mPalRes, bmgMessageID) / 100.0f,
            mFontSize.y * ReadPrintMessage::getRate(mPalRes, bmgMessageID) / 100.0f);

        mTextBox->setCharSpace(ReadPrintMessage::getCharSpace(mPalRes, bmgMessageID));

        J2DTextBox::TFontSize fontSize;
        mTextBox->getFontSize(fontSize);
        char buf[512];
        ReadPrintMessage::tagCnv(message, mColorRes, fontSize, sizeof(buf), buf);
        strncpy(mTextBox->getStringPtr(), buf, sizeof(buf));
    }

    {
        char *message = ReadPrintMessage::getMessage(mPalRes, 3);

        mTextBoxYes->setFontSize(mFontSizeYes.x * ReadPrintMessage::getRate(mPalRes, 3) / 100.0f,
                                 mFontSizeYes.y * ReadPrintMessage::getRate(mPalRes, 3) / 100.0f);

        mTextBoxYes->setCharSpace(ReadPrintMessage::getCharSpace(mPalRes, 3));

        J2DTextBox::TFontSize fontSize;
        mTextBoxYes->getFontSize(fontSize);
        char buf[64];
        ReadPrintMessage::tagCnv(message, mColorRes, fontSize, sizeof(buf), buf);
        strncpy(mTextBoxYes->getStringPtr(), buf, sizeof(buf));
    }

    {
        char *message = ReadPrintMessage::getMessage(mPalRes, 4);

        mTextBoxNo->setFontSize(mFontSizeNo.x * ReadPrintMessage::getRate(mPalRes, 4) / 100.0f,
                                mFontSizeNo.y * ReadPrintMessage::getRate(mPalRes, 4) / 100.0f);

        mTextBoxNo->setCharSpace(ReadPrintMessage::getCharSpace(mPalRes, 4));

        J2DTextBox::TFontSize fontSize;
        mTextBoxNo->getFontSize(fontSize);
        char buf[64];
        ReadPrintMessage::tagCnv(message, mColorRes, fontSize, sizeof(buf), buf);
        strncpy(mTextBoxNo->getStringPtr(), buf, sizeof(buf));
    }

    switch (mMsgId)
    {
    case mcShow_Logo:
        mPictureNintendo->show();
        mPictureDolby->hide();
        break;
    case mcShow_Set:
        mPictureNintendo->hide();
        mPictureDolby->show();
        break;
    default:
        mPictureNintendo->hide();
        mPictureDolby->hide();
        break;
    }

    mTransformFrame = 0;
    mSTRKeyFrame = 0;
    mFrame = 419;
}

void PrintPal::draw()
{
    mOrthoGraph->setPort();

    switch (mMsgId)
    {
    case mcShow_Logo:
    case mcShow_Set:
        mScreenLogo->draw(0.0f, 0.0f, mOrthoGraph);
        break;
    default:
        mScreenPal->draw(0.0f, 0.0f, mOrthoGraph);
    }
}


void PrintPal::calc(int param)
{
    if (param >= 0)
    {
        mFrame = param * 60;
    }

    switch (mMsgId)
    {
    case mcShow_Logo:
    case mcShow_Set:
        return;
    default:
        break;
    }

    if (isChoice())
    {
        if (gpaGamePad[0]->testTrigger(JUTGamePad::A))
        {
            if (mState == 2)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20003);

                switch (mOption)
                {
                case 0:
                    mState = 0;
                    break;
                case 1:
                    mState = 1;
                    break;
                }
            }
        }
        else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT))
        {
            if (mState == 2)
            {
                switch (mOption)
                {
                case 1:
                    GameAudio::Main::getAudio()->startSystemSe(0x20000);
                    mOption = 0;
                    mColorsFrame = 0;
                    mTransformFrame = 0;
                    break;
                }
            }
        }
        else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT))
        {
            if (mState == 2)
            {
                switch (mOption)
                {
                case 0:
                    GameAudio::Main::getAudio()->startSystemSe(0x20000);
                    mOption = 1;
                    mColorsFrame = 0;
                    mTransformFrame = 0;
                    break;
                }
            }
        }
    }
    mSTRKeyFrame += 1;
    if (mSTRKeyFrame < 0)
    {
        mSTRKeyFrame = 0;
    }
    if (mSTRKeyFrame > 359)
    {
        mSTRKeyFrame = 0;
    }
    mSTRKey->setFrame(mSTRKeyFrame);

    if (isChoice())
    {
        switch (mOption)
        {
        case 0:
            mColors[0]->setFrame(1.0f);
            mColors[1]->setFrame(0.0f);
            if (mColorsFrame++ + 1 >= 359)
            {
                mColorsFrame = 0;
            }
            mColors[2]->setFrame(mColorsFrame);
            mColors[3]->setFrame(0.0f);
            break;
        case 1:
            mColors[0]->setFrame(0.0f);
            mColors[1]->setFrame(1.0f);
            if (mColorsFrame++ + 1 >= 359)
            {
                mColorsFrame = 0;
            }
            mColors[2]->setFrame(0.0f);
            mColors[3]->setFrame(mColorsFrame);
            break;
        }
    }

    mScreenPal->animation();

    if (isChoice())
    {
        mTransformFrame += 1;
        if (mTransformFrame < 0)
        {
            mTransformFrame = 0;
        }
        if (mTransformFrame > 8)
        {
            mTransformFrame = 8;
        }

        switch (mOption)
        {
        case 0:
            mTransform->setFrame(mTransformFrame);
            mPictureYes->animationTransform();
            mTransform->setFrame(0.0f);
            mPictureNo->animationTransform();
            break;
        case 1:
            mTransform->setFrame(0.0f);
            mPictureYes->animationTransform();
            mTransform->setFrame(mTransformFrame);
            mPictureNo->animationTransform();
            break;
        }
    }

    if (isChoice())
    {
        if (mFrame > 0)
        {
            mFrame -= 1;
        }
        else
        {
            mFrame = 0;
        }

        char buff[2];
        buff[0] = getCounter() + '0';
        buff[1] = '\0';
        strcpy(mTextBoxNum->getStringPtr(), buff);
    }

    if (mFrame <= 0 && mState == 2)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20003);

        switch (mOption)
        {
        case 0:
            mState = 0;
            break;
        case 1:
            mState = 1;
            break;
        }
    }
}

bool PrintPal::isMessage()
{
    switch (mMsgId)
    {
    case mcShow_Dialog:
    case mcSet_Option1:
    case mcSet_Option2:
        return true;
    case mcShow_Logo:
    case mcShow_Set:
        return false;
    }
}

bool PrintPal::isChoice()
{
    switch (mMsgId)
    {
    case mcShow_Dialog:
        return true;
    case mcSet_Option1:
    case mcSet_Option2:
    case mcShow_Logo:
    case mcShow_Set:
        return false;
    }
}

int PrintPal::getCounter()
{
    int counter = -1;

    switch (mMsgId)
    {
    case mcShow_Dialog:
        if (mFrame != 0)
        {
            counter = mFrame / 60 + 1;
        }
        else
        {
            counter = 0;
        }
        break;
    default:
        break;
    }

    return counter;
}
