#include "Kawano/StaffRoll2D.h"

#include "JSystem/J2D/J2DAnimation.h"

ExitBoxInfo::ExitBoxInfo() {}

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

void StaffRoll2D::reset()
{
    f32 *base = (f32 *)((u8 *)this - 0xB60);
    for (int i = 0; i < 18; i++) {
        f32 *row = (f32 *)((u8 *)base + i * 60);
        for (int j = 0; j < 15; j++)
            row[j + 728] = -1.0f;
    }
    mField457C = 0;
    mField0F98 = -1;
    mField4584 = 0;
}

void StaffRoll2D::draw()
{
    mGrafContext->setPort();

    for (int i = 0; i < 18; i++)
        mScreen[i]->draw(0.0f, 0.0f, mGrafContext);

    mScreenTrue->draw(0.0f, 0.0f, mGrafContext);

    if (mField317C == 0)
        return;

    if (mField3178 < mField315C) {
        mField317C = 0;
        return;
    }

    mField317C = 0;

    JUTFont::TWidth width;
    mFont->getWidthEntry(mChars[0], &width);

    int target = width.w1;
    f32 scale = (target - (mField3178 - mField315C)) / mField3164;

    mColor1.r = (u8)(mColor1.r * scale);
    mColor1.g = (u8)(mColor1.g * scale);
    mColor1.b = (u8)(mColor1.b * scale);
    mColor2.r = (u8)(mColor2.r * scale);
    mColor2.g = (u8)(mColor2.g * scale);
    mColor2.b = (u8)(mColor2.b * scale);

    mFont->setGX();
    mFont->setGradColor(mColor1, mColor2);
    mFont->drawString_size_scale(mField315C, 130.0f + mField3160, mField3164,
                                 mField3168, mChars, 2, false);
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
        if (mAnmTexSRTTrue != nullptr)
            mAnmTexSRTTrue->setFrame(mField0F98);

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

                f32 frame = mValue[i][j];
                if (frame < 0.0f)
                    frame = 0.0f;

                mAnmColor[i]->setFrame(frame);

                if (mTextBox[i][j] != nullptr) {
                    mTextBox[i][j]->animationTransform();
                    if (mAnmTransform[i] != nullptr)
                        mAnmTransform[i]->setFrame(frame);
                }
            }

            mScreen[i]->animation();
        }
    }

    if (mField4580 < 4)
        mField4580++;

    if (mField4580 == 3) {
        for (int i = 0; i < 18; i++) {
            for (int j = 0; j < 8; j++) {
                if (mExitBoxPicture[i][j] != nullptr)
                    mExitBoxInfo[i][j].getBox() = *mExitBoxPicture[i][j]->getBounds();
            }
        }
    }
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
        const char **name = (const char **)((u8 *)mName_jpa
                                            + getSceneDataID(id, mVersion) * 60
                                            + i * 4);
        mgr->createEmt2D(*name, pos);
    }
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
