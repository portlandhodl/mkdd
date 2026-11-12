#include "Kawano/StaffRoll2D.h"

#include "JSystem/J2D/J2DAnimation.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JParticle/JPAResource.h"
#include "Kameda/ReadPrintMessage.h"
#include "Osako/ResMgr.h"
#include "Osako/system.h"
#include "string.h"

Font1letterPrinter::Font1letterPrinter()
{
    mIsActive = 0;
}

Font1letterPrinter::~Font1letterPrinter() {}

void Font1letterPrinter::get(J2DTextBox *textBox, f32 x, f32 y, char *str, f32 curX)
{
    mFont = textBox->getFont();
    mCharColor = textBox->mCharColor;
    mGradColor = textBox->mGradientColor;
    mWidth = textBox->mFontSize.x;
    mHeight = textBox->mFontSize.y;
    mPosX = x;
    mPosY = y;
    mChars[0] = str[0];
    mChars[1] = str[1];
    mCurX = curX;
    mIsActive = 1;
}

void Font1letterPrinter::print()
{
    if (mIsActive == 0)
        return;

    if (mCurX < mPosX) {
        mIsActive = 0;
        return;
    }

    mIsActive = 0;

    JUTFont::TWidth width;
    mFont->getWidthEntry(mChars[0], &width);

    int target = width.w1;
    f32 scale = (target - (mCurX - mPosX)) / mWidth;

    mCharColor.r = (u8)(mCharColor.r * scale);
    mCharColor.g = (u8)(mCharColor.g * scale);
    mCharColor.b = (u8)(mCharColor.b * scale);
    mGradColor.r = (u8)(mGradColor.r * scale);
    mGradColor.g = (u8)(mGradColor.g * scale);
    mGradColor.b = (u8)(mGradColor.b * scale);

    mFont->setGX();
    mFont->setGradColor(mCharColor, mGradColor);
    mFont->drawString_size_scale(mPosX, 130.0f + mPosY, mWidth, mHeight, mChars, 2, false);
}

void ExitBoxInfo::setBox(J2DPicture *picture)
{
    mBox = picture->getGlobalBounds();
}

const char *StaffRoll2D::mBloName[25] = {
    "StaffRoll_01.blo", "StaffRoll_02.blo", "StaffRoll_03.blo", "StaffRoll_04.blo",
    "StaffRoll_05.blo", "StaffRoll_06.blo", "StaffRoll_07.blo", "StaffRoll_08.blo",
    "StaffRoll_09.blo", "StaffRoll_10.blo", "StaffRoll_11.blo", "StaffRoll_12.blo",
    "StaffRoll_13.blo", "StaffRoll_14A.blo", "StaffRoll_15A.blo", "StaffRoll_16A.blo",
    "StaffRoll_17A.blo", "StaffRoll_14B.blo", "StaffRoll_15B.blo", "StaffRoll_16B.blo",
    "StaffRoll_17B.blo", "StaffRoll_16C.blo", "StaffRoll_17C.blo", "StaffRoll_18.blo",
    nullptr,
};

const char *StaffRoll2D::mBckName[25] = {
    "StaffRoll_01.bck", "StaffRoll_02.bck", "StaffRoll_03.bck", "StaffRoll_04.bck",
    "StaffRoll_05.bck", "StaffRoll_06.bck", "StaffRoll_07.bck", "StaffRoll_08.bck",
    "StaffRoll_09.bck", "StaffRoll_10.bck", "StaffRoll_11.bck", "StaffRoll_12.bck",
    "StaffRoll_13.bck", "StaffRoll_14A.bck", "StaffRoll_15A.bck", "StaffRoll_16A.bck",
    "StaffRoll_17A.bck", "StaffRoll_14B.bck", "StaffRoll_15B.bck", "StaffRoll_16B.bck",
    "StaffRoll_17B.bck", "StaffRoll_16C.bck", "StaffRoll_17C.bck", "StaffRoll_18.bck",
    nullptr,
};

const char *StaffRoll2D::mBrkName[25] = {
    "StaffRoll_01.brk", "StaffRoll_02.brk", "StaffRoll_03.brk", "StaffRoll_04.brk",
    "StaffRoll_05.brk", "StaffRoll_06.brk", "StaffRoll_07.brk", "StaffRoll_08.brk",
    "StaffRoll_09.brk", "StaffRoll_10.brk", "StaffRoll_11.brk", "StaffRoll_12.brk",
    "StaffRoll_13.brk", "StaffRoll_14A.brk", "StaffRoll_15A.brk", "StaffRoll_16A.brk",
    "StaffRoll_17A.brk", "StaffRoll_14B.brk", "StaffRoll_15B.brk", "StaffRoll_16B.brk",
    "StaffRoll_17B.brk", "StaffRoll_16C.brk", "StaffRoll_17C.brk", "StaffRoll_18.brk",
    nullptr,
};

const char *StaffRoll2D::mBloNameTrue[2] = {"StaffRoll_19N.blo", "StaffRoll_19.blo"};

const char *StaffRoll2D::mBckNameTrue[2] = {"StaffRoll_19N.bck", "StaffRoll_19.bck"};

const char *StaffRoll2D::mBpkNameTrue[2] = {"StaffRoll_19N.bpk", "StaffRoll_19.bpk"};

const u64 StaffRoll2D::mTag_textbox[25][15] = {
    0x545f303130ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303230ull, 0x545f303231ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303330ull, 0x545f303331ull, 0x545f303332ull,
    0x545f303333ull, 0x545f303334ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303430ull, 0x545f303431ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303530ull, 0x545f303531ull, 0x545f303532ull,
    0x545f303533ull, 0x545f303534ull, 0x545f303535ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303630ull, 0x545f303631ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303730ull, 0x545f303731ull, 0x545f303732ull,
    0x545f303733ull, 0x545f303734ull, 0x545f303735ull,
    0x545f303736ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303830ull, 0x545f303831ull, 0x545f303832ull,
    0x545f303833ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f303930ull, 0x545f303931ull, 0x545f303932ull,
    0x545f303933ull, 0x545f303934ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f313030ull, 0x545f313031ull, 0x545f313032ull,
    0x545f313033ull, 0x545f313034ull, 0x545f313035ull,
    0x545f313036ull, 0x545f313037ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f313130ull, 0x545f313131ull, 0x545f313132ull,
    0x545f313133ull, 0x545f313134ull, 0x545f313135ull,
    0x545f313136ull, 0x545f313137ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f313230ull, 0x545f313231ull, 0x545f313232ull,
    0x545f313233ull, 0x545f313234ull, 0x545f313235ull,
    0x545f313236ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f313330ull, 0x545f313331ull, 0x545f313332ull,
    0x545f313333ull, 0x545f313334ull, 0x545f313335ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31344130ull, 0x545f31344131ull, 0x545f31344132ull,
    0x545f31344133ull, 0x545f31344134ull, 0x545f31344135ull,
    0x545f31344136ull, 0x545f31344137ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31354130ull, 0x545f31354131ull, 0x545f31354132ull,
    0x545f31354133ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31364130ull, 0x545f31364131ull, 0x545f31364132ull,
    0x545f31364133ull, 0x545f31364134ull, 0x545f31364135ull,
    0x545f31364136ull, 0x545f31364137ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31374130ull, 0x545f31374131ull, 0x545f31374132ull,
    0x545f31374133ull, 0x545f31374134ull, 0x545f31374135ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f3134423030ull, 0x545f3134423031ull, 0x545f3134423032ull,
    0x545f3134423033ull, 0x545f3134423034ull, 0x545f3134423035ull,
    0x545f3134423036ull, 0x545f3134423037ull, 0x545f3134423038ull,
    0x545f3134423039ull, 0x545f3134423130ull, 0x545f3134423131ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f3135423030ull, 0x545f3135423031ull, 0x545f3135423032ull,
    0x545f3135423033ull, 0x545f3135423034ull, 0x545f3135423035ull,
    0x545f3135423036ull, 0x545f3135423037ull, 0x545f3135423038ull,
    0x545f3135423039ull, 0x545f3135423130ull, 0x545f3135423131ull,
    0x545f3135423132ull, 0x545f3135423133ull, 0x7878787878787878ull,
    0x545f31364230ull, 0x545f31364231ull, 0x545f31364232ull,
    0x545f31364233ull, 0x545f31364234ull, 0x545f31364235ull,
    0x545f31364236ull, 0x545f31364237ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31374230ull, 0x545f31374231ull, 0x545f31374232ull,
    0x545f31374233ull, 0x545f31374234ull, 0x545f31374235ull,
    0x545f31374236ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f31364330ull, 0x545f31364331ull, 0x545f31364332ull,
    0x545f31364333ull, 0x545f31364334ull, 0x545f31364335ull,
    0x545f31364336ull, 0x545f31364337ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x545f3137433030ull, 0x545f3137433031ull, 0x545f3137433032ull,
    0x545f3137433033ull, 0x545f3137433034ull, 0x545f3137433035ull,
    0x545f3137433036ull, 0x545f3137433037ull, 0x545f3137433038ull,
    0x545f3137433039ull, 0x545f3137433130ull, 0x545f3137433131ull,
    0x545f3137433132ull, 0x545f3137433133ull, 0x545f3137433134ull,
    0x545f313830ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
};

const u64 StaffRoll2D::mTag_exitbox[25][8] = {
    0x6d5f303130ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303330ull, 0x6d5f303331ull, 0x6d5f303332ull, 0x6d5f303333ull,
    0x6d5f303334ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303430ull, 0x6d5f303431ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303530ull, 0x6d5f303531ull, 0x6d5f303532ull, 0x6d5f303533ull,
    0x6d5f303534ull, 0x6d5f303535ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303630ull, 0x6d5f303631ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303830ull, 0x6d5f303831ull, 0x6d5f303832ull, 0x6d5f303833ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f303930ull, 0x6d5f303931ull, 0x6d5f303932ull, 0x6d5f303933ull,
    0x6d5f303934ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x6d5f313130ull, 0x6d5f313131ull, 0x6d5f313132ull, 0x6d5f313133ull,
    0x6d5f313134ull, 0x6d5f313135ull, 0x6d5f313136ull, 0x6d5f313137ull,
    0x6d5f313230ull, 0x6d5f313231ull, 0x6d5f313232ull, 0x6d5f313233ull,
    0x6d5f313234ull, 0x6d5f313235ull, 0x6d5f313236ull, 0x7878787878787878ull,
    0x6d5f313330ull, 0x6d5f313331ull, 0x6d5f313332ull, 0x6d5f313333ull,
    0x6d5f313334ull, 0x6d5f313335ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull, 0x7878787878787878ull,
    0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
};

const int StaffRoll2D::maConvertChartSceneNo2DataID[12] = {
    13, 14, 15, 16,
    17, 18, 19, 20,
    17, 18, 21, 22,
};

const u64 StaffRoll2D::mTag_textboxTrue[2] = {0x545f31394e30ull, 0x545f3139};

const char *StaffRoll2D::mName_jpa[25][15] = {
    "T_010", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_020", "T_021", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_030", "T_031", "T_032", "T_033", "T_034",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_040", "T_041", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_050", "T_051", "T_052", "T_053", "T_054",
    "T_055", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_060", "T_061", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_070", "T_071", "T_072", "T_073", "T_074",
    "T_075", "T_076", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_080", "T_081", "T_082", "T_083", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_090", "T_091", "T_092", "T_093", "T_094",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_100", "T_101", "T_102", "T_103", "T_104",
    "T_105", "T_106", "T_107", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_110", "T_111", "T_112", "T_113", "T_114",
    "T_115", "T_116", "T_117", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_120", "T_121", "T_122", "T_123", "T_124",
    "T_125", "T_126", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_130", "T_131", "T_132", "T_133", "T_134",
    "T_135", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_14A0", "T_14A1", "T_14A2", "T_14A3", "T_14A4",
    "T_14A5", "T_14A6", "T_14A7", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_15A0", "T_15A1", "T_15A2", "T_15A3", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_16A0", "T_16A1", "T_16A2", "T_16A3", "T_16A4",
    "T_16A5", "T_16A6", "T_16A7", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_17A0", "T_17A1", "T_17A2", "T_17A3", "T_17A4",
    "T_17A5", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_14B00", "T_14B01", "T_14B02", "T_14B03", "T_14B04",
    "T_14B05", "T_14B06", "T_14B07", "T_14B08", "T_14B09",
    "T_14B10", "T_14B11", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_15B00", "T_15B01", "T_15B02", "T_15B03", "T_15B04",
    "T_15B05", "T_15B06", "T_15B07", "T_15B08", "T_15B09",
    "T_15B10", "T_15B11", "T_15B12", "T_15B13", "XXXXXXX",
    "T_16B0", "T_16B1", "T_16B2", "T_16B3", "T_16B4",
    "T_16B5", "T_16B6", "T_16B7", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_17B0", "T_17B1", "T_17B2", "T_17B3", "T_17B4",
    "T_17B5", "T_17B6", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_16C0", "T_16C1", "T_16C2", "T_16C3", "T_16C4",
    "T_16C5", "T_16C6", "T_16C7", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "T_17C00", "T_17C01", "T_17C02", "T_17C03", "T_17C04",
    "T_17C05", "T_17C06", "T_17C07", "T_17C08", "T_17C09",
    "T_17C10", "T_17C11", "T_17C12", "T_17C13", "T_17C14",
    "T_180", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX",
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
};



StaffRoll2D::StaffRoll2D(JKRHeap *heap, Language language, VideoMode videoMode, int isTrue)
{
    mVersion = getVersion(language, videoMode);
    JKRArchive *archive = JKRMountArchive(ResMgr::getPtr(ResMgr::mcArcCourse, "staffroll2d.arc"), heap,
                                          JKRArchive::MOUNT_DIRECTION_HEAD);

    for (int i = 0; i < 18; i++) {
        int j;
        int id = getSceneDataID(i, mVersion);
        mScreen[i] = new (heap, 0) J2DScreen();
        mScreen[i]->set(mBloName[id], 0x40000, archive);
        mScreen[i]->mIsScissor = true;
        mAnmTransform[i] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource(mBckName[id], archive));
        mScreen[i]->setAnimation(mAnmTransform[i]);
        void *brk = JKRFileLoader::getGlbResource(mBrkName[id], archive);

        for (j = 0; j < 15; j++) {
            for (int k = 0; k < 32; k++)
                mStrings[i][j][k] = '\0';

            mTextBox[i][j] = (J2DTextBox *)mScreen[i]->search(mTag_textbox[id][j]);
            if (mTextBox[i][j] != nullptr) {
                strncpy(mStrings[i][j], mTextBox[i][j]->getStringPtr(), 32);
                for (int k = 0; k < 32; k++) {
                }

                mAnmTevReg[i][j] = nullptr;
                if (brk != nullptr) {
                    mAnmTevReg[i][j] = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(brk);
                    mAnmTevReg[i][j]->searchUpdateMaterialID(mScreen[i]);
                    mTextBox[i][j]->setAnimation(mAnmTevReg[i][j]);
                    mValue2[i][j] = mTextBox[i][j]->getTranslateX();
                    mValue3[i][j] = mTextBox[i][j]->getTranslateY();
                    if (mTextBox[i][j]->getParentPane() != nullptr)
                        mValue2[i][j] += mTextBox[i][j]->getParentPane()->getTranslateX();
                    if (mTextBox[i][j]->getParentPane() != nullptr)
                        mValue3[i][j] += mTextBox[i][j]->getParentPane()->getTranslateY();
                }
            }
        }

        mExitBoxNum[i] = 0;
        for (j = 0; j < 8; j++) {
            mExitBoxPicture[i][j] = (J2DPicture *)mScreen[i]->search(mTag_exitbox[id][j]);
            if (mExitBoxPicture[i][j] != nullptr) {
                mExitBoxPicture[i][j]->hide();
                switch (mExitBoxPicture[i][j]->mBasePosition) {
                case J2DPOS_TopLeft:
                case J2DPOS_CenterLeft:
                case J2DPOS_BottomLeft:
                    mExitBoxInfo[i][j].setDir(0);
                    break;
                default:
                    mExitBoxInfo[i][j].setDir(1);
                    break;
                }
                mExitBoxNum[i]++;
            }
        }
    }

    void *colorRes = ResMgr::getPtr(ResMgr::mcColor);
    void *msgRes = ResMgr::getPtr(ResMgr::mcStaffRoll);
    JUtility::TColor color = ReadPrintMessage::getColor(colorRes, ReadPrintMessage::getDefaultColor(msgRes));
    char space[32];
    for (int k = 0; k < 32; k++)
        space[k] = ' ';

    if (language == ITALIAN) {
        space[31] = '\0';
        mTextBox[0][0]->setString(space);
        J2DTextBox::TFontSize size;
        mTextBox[0][0]->getFontSize(size);
        char *msg = ReadPrintMessage::getMessage(msgRes, 0);
        mTextBox[0][0]->setFontSize(size.x * ReadPrintMessage::getRate(msgRes, 0) / 100.0f,
                                    size.y * ReadPrintMessage::getRate(msgRes, 0) / 100.0f);
        mTextBox[0][0]->setCharSpace(ReadPrintMessage::getCharSpace(msgRes, 0));
        J2DTextBox::TFontSize fontSize;
        mTextBox[0][0]->getFontSize(fontSize);
        char buf[32];
        ReadPrintMessage::tagCnv(msg, colorRes, fontSize, 32, buf);
        strncpy(mTextBox[0][0]->getStringPtr(), buf, 32);
    }

    if (mVersion == 2) {
        mTextBox[16][3]->setString(space);
        mTextBox[16][5]->setString(space);
        {
            J2DTextBox::TFontSize size;
            mTextBox[16][3]->getFontSize(size);
            char *msg = ReadPrintMessage::getMessage(msgRes, 1);
            mTextBox[16][3]->setFontSize(size.x * ReadPrintMessage::getRate(msgRes, 1) / 100.0f,
                                         size.y * ReadPrintMessage::getRate(msgRes, 1) / 100.0f);
            mTextBox[16][3]->setCharSpace(1.0f);
            J2DTextBox::TFontSize fontSize;
            mTextBox[16][3]->getFontSize(fontSize);
            char buf[32];
            ReadPrintMessage::tagCnv(msg, colorRes, fontSize, 32, buf);
            strncpy(mTextBox[16][3]->getStringPtr(), buf, 32);
        }
        {
            J2DTextBox::TFontSize size;
            mTextBox[16][5]->getFontSize(size);
            char *msg = ReadPrintMessage::getMessage(msgRes, 2);
            mTextBox[16][5]->setFontSize(size.x * ReadPrintMessage::getRate(msgRes, 2) / 100.0f,
                                         size.y * ReadPrintMessage::getRate(msgRes, 2) / 100.0f);
            mTextBox[16][5]->setCharSpace(0.0f);
            J2DTextBox::TFontSize fontSize;
            mTextBox[16][5]->getFontSize(fontSize);
            char buf[32];
            ReadPrintMessage::tagCnv(msg, colorRes, fontSize, 32, buf);
            strncpy(mTextBox[16][5]->getStringPtr(), buf, 32);
        }
    }

    if (isTrue != 0)
        isTrue = 1;

    mScreenTrue = new (heap, 0) J2DScreen();
    mScreenTrue->set(mBloNameTrue[isTrue], 0x40000, archive);
    mAnmTransformTrue = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
        JKRFileLoader::getGlbResource(mBckNameTrue[isTrue], archive));
    mScreenTrue->setAnimation(mAnmTransformTrue);
    mAnmColorTrue = (J2DAnmColor *)J2DAnmLoaderDataBase::load(
        JKRFileLoader::getGlbResource(mBpkNameTrue[isTrue], archive));
    mScreenTrue->setAnimation(mAnmColorTrue);
    mAnmTevRegTrue = nullptr;
    if (isTrue) {
        mAnmTevRegTrue = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("StaffRoll_19.brk", archive));
        mScreenTrue->setAnimation(mAnmTevRegTrue);
    }

    mTextBoxVersion = (J2DTextBox *)mScreenTrue->search(mTag_textboxTrue[isTrue]);
    mTextBoxVersion->setString(space);
    {
        J2DTextBox::TFontSize size;
        mTextBoxVersion->getFontSize(size);
        char *msg = ReadPrintMessage::getMessage(msgRes, 3);
        mTextBoxVersion->setFontSize(size.x * ReadPrintMessage::getRate(msgRes, 3) / 100.0f,
                                     size.y * ReadPrintMessage::getRate(msgRes, 3) / 100.0f);
        mTextBoxVersion->setCharSpace(0.0f);
        J2DTextBox::TFontSize fontSize;
        mTextBoxVersion->getFontSize(fontSize);
        char buf[32];
        ReadPrintMessage::tagCnv(msg, colorRes, fontSize, 32, buf);
        strncpy(mTextBoxVersion->getStringPtr(), buf, 32);
    }

    mGrafContext = System::getJ2DOrtho();
    mTextFont = mTextBox[0][0]->getFont();
    mField4580 = 0;
    init();
}

void StaffRoll2D::Constructer_JPA(JKRHeap *heap)
{
    JPAResourceManager *resMgr =
        new (heap, 0) JPAResourceManager(ResMgr::getPtr(ResMgr::mcArcCourse, "sr2djpa.jpc"), heap);
    JPEffectMgr::getEffectMgr()->create2DEmtMgr(resMgr, 0x100, 0x400);
}

StaffRoll2D::~StaffRoll2D() {}

void StaffRoll2D::init()
{
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 15; j++)
            mValue[i][j] = -1.0f;
    }
    mField457C = 0;
    mField0F98 = -1;
    mField4584 = 0;
}

void StaffRoll2D::initExitBoxInfo()
{
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 8; j++) {
            if (mExitBoxPicture[i][j] != nullptr)
                mExitBoxInfo[i][j].setBox(mExitBoxPicture[i][j]);
        }
    }
}

void StaffRoll2D::reset()
{
    init();
}


void StaffRoll2D::draw()
{
    mGrafContext->setPort();

    for (int i = 0; i < 18; i++)
        mScreen[i]->draw(0.0f, 0.0f, mGrafContext);

    mScreenTrue->draw(0.0f, 0.0f, mGrafContext);
    mPrinter.print();
}

void StaffRoll2D::calc()
{
    if (mField4584 != 0) {
        for (int i = 0; i < 18; i++)
            mScreen[i]->hide();

        mScreenTrue->show();

        if (mField0F98 < 100)
            mField0F98++;

        mAnmTransformTrue->setFrame(mField0F98);
        mAnmColorTrue->setFrame(mField0F98);
        if (mAnmTevRegTrue != nullptr)
            mAnmTevRegTrue->setFrame(mField0F98);

        mScreenTrue->animation();
    } else {
        mScreenTrue->hide();
    }

    if (mField4584 == 0) {
        int i;
        for (i = 0; i < 18; i++) {
            if (mValue[i][0] == -1.0f) {
                mScreen[i]->hide();
                continue;
            }

            mScreen[i]->show();

            for (int j = 0; j < 15; j++) {
                if (mValue[i][j] >= 0.0f)
                    mValue[i][j] += 1.0f;
                if (mValue[i][j] >= 700.0f)
                    mValue[i][j] = -1.0f;

                f32 v = mValue[i][j];
                f32 frame = (v < 0.0f) ? 0.0f : v;

                mAnmTransform[i]->setFrame(frame);

                if (mTextBox[i][j] != nullptr) {
                    mTextBox[i][j]->animationTransform();
                    if (mAnmTevReg[i][j] != nullptr)
                        mAnmTevReg[i][j]->setFrame(frame);
                }
            }

            mScreen[i]->animation();
        }
    }

    if (mField4580 < 4)
        mField4580++;

    if (mField4580 == 3)
        initExitBoxInfo();
}

void StaffRoll2D::start(int id)
{
    switch (id) {
    case 19:
        mField4584 = 1;
        return;
    case 20:
        mField4584 = 2;
        return;
    }

    if (id < 0)
        return;
    if (id >= 18)
        return;

    for (int i = 0; i < 15; i++) {
        mValue[id][i] = 0.0f;

        if (id == 17)
            continue;
        if (mTextBox[id][i] == nullptr)
            continue;

        JGeometry::TVec3f pos(mValue2[id][i], mValue3[id][i], 0.0f);
        JPEffectMgr *mgr = JPEffectMgr::getEffectMgr();
        mgr->createEmt2D(mName_jpa[getSceneDataID(id, mVersion)][i], pos);
    }
}

int StaffRoll2D::getWidth(char *str)
{
    int width = 0;
    while (*str != '\0') {
        JUTFont::TWidth w;
        mTextFont->getWidthEntry(*str, &w);
        width += w.w1;
        str++;
    }
    return width;
}

ExitBoxInfo *StaffRoll2D::getExitBoxInfoPtr(int scene, int index)
{
    if (index >= mExitBoxNum[scene])
        return nullptr;
    return &mExitBoxInfo[scene][index];
}

int StaffRoll2D::getExitBoxNum(int scene)
{
    if (scene < 0)
        return 0;
    if (scene >= 18)
        return 0;
    return mExitBoxNum[scene];
}

int StaffRoll2D::getVersion(Language language, VideoMode mode)
{
    if (mode == VideoMode_PAL)
        return 2;
    return (u32)__cntlzw(language) >> 5;
}

int StaffRoll2D::getSceneDataID(int scene, int version)
{
    if (version >= 3 || version < 0)
        version = 0;

    int data = scene;
    if (scene == 17) {
        data = 23;
    } else {
        if (scene < 13)
            data = scene;
        else
            data = maConvertChartSceneNo2DataID[scene - 13 + version * 4];
    }

    return data;
}
