#include <JSystem/J2D/J2DAnmLoader.h>
#include <JSystem/JKernel/JKRFileLoader.h>
#include "Osako/LANBackground.h"

void LANBackground::setup(JKRArchive* archive)
{
    setPriority("Back1.blo", 0x20000, archive);
    J2DAnmColor* bpk;
    J2DAnmTransform* bck = (J2DAnmTransform*)JKRFileLoader::getGlbResource("Back1.bck", archive);
    J2DAnmTextureSRTKey* btk = (J2DAnmTextureSRTKey*)JKRFileLoader::getGlbResource("Back1.btk", archive);
    bpk = (bpk, (J2DAnmColor*)JKRFileLoader::getGlbResource("Back1.bpk", archive));

    mTransform = (J2DAnmTransform*)J2DAnmLoaderDataBase::load(bck);
    mTexSRTKey = (J2DAnmTextureSRTKey*)J2DAnmLoaderDataBase::load(btk);
    mColor = (J2DAnmColor*)J2DAnmLoaderDataBase::load(bpk);

    setAnimation(mTransform);
    setAnimation(mTexSRTKey);
    setAnimation(mColor);
}

void LANBackground::calc()
{
    mTransform->setFrame(mTransformFrame);
    mTexSRTKey->setFrame(mColorFrame);

    if (++mTransformFrame >= 97)
        mTransformFrame = 0;
    if (++mColorFrame >= 360)
        mColorFrame = 0;

    animation();
}

void LANBackground::entry()
{
    mColor->setFrame(1.0f);
}

void LANBackground::unentry()
{
    mColor->setFrame(0.0f);
}
