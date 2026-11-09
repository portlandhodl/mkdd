#include "JSystem/J3D/J3DAnmColor.h"
#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DAnmVtxColor.h"

J3DAnmTransform::J3DAnmTransform(s16 frameMax, f32* scale, s16* rot, f32* trans)
    : J3DAnmBase(frameMax)
    , mScaleVals(scale)
    , mRotationVals(rot)
    , mTranslationVals(trans)
    , _18(0)
    , _1A(0)
    , _1C(0)
    , _1E(0)
{
}

J3DAnmColorFull::J3DAnmColorFull()
    : _2C(nullptr)
    , _30(nullptr)
    , _34(nullptr)
    , _38(nullptr)
    , mTable(nullptr)
{
}

J3DAnmColorKey::J3DAnmColorKey()
    : _2C(nullptr)
    , _30(nullptr)
    , _34(nullptr)
    , _38(nullptr)
    , mTable(nullptr)
{
}

J3DAnmTexPattern::J3DAnmTexPattern()
    : _0C(nullptr)
    , mAnmTable(nullptr)
    , _14(0)
    , mUpdateMaterialNum(0)
    , mUpdateMaterialID(nullptr)
    , mNameTab()
{
}

J3DAnmTextureSRTKey::J3DAnmTextureSRTKey()
    : _30()
    , _64()
{
    _0C                = 0;
    _1A                = 0;
    _18                = 0;
    _16                = 0;
    mUpdateMaterialNum = 0;
    _10                = nullptr;
    _24                = nullptr;
    _1C                = nullptr;
    _20                = nullptr;
    _48                = 0;
    _46                = 0;
    _44                = 0;
    _4A                = 0;
    _58                = nullptr;
    _54                = nullptr;
    _4C                = nullptr;
    _50                = nullptr;
    mTexMtxCalcType    = 0;
}

J3DAnmTevRegKey::J3DAnmTevRegKey()
    : mCRegNameTable()
    , mKRegNameTable()
{
    mKRegUpdateMaterialNum = 0;
    mCRegUpdateMaterialNum = 0;
    _16                    = 0;
    _14                    = 0;
    _12                    = 0;
    _10                    = 0;
    _1E                    = 0;
    _1C                    = 0;
    _1A                    = 0;
    _18                    = 0;
    mKRegUpdateMaterialID  = nullptr;
    mCRegUpdateMaterialID  = nullptr;
    mCAlphaVals            = nullptr;
    mCBlueVals             = nullptr;
    mCGreenVals            = nullptr;
    mCRedVals              = nullptr;
    mKAlphaVals            = nullptr;
    mKBlueVals             = nullptr;
    mKGreenVals            = nullptr;
    mKRedVals              = nullptr;
}

J3DAnmVtxColorFull::J3DAnmVtxColorFull()
{
    mTable[0] = nullptr;
    mTable[1] = nullptr;
}

J3DAnmVtxColorKey::J3DAnmVtxColorKey()
{
    mTable[0] = nullptr;
    mTable[1] = nullptr;
}
