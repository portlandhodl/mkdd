#ifndef LANBACKGROUND_H
#define LANBACKGROUND_H

#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/JKernel/JKRArchive.h"

class J2DAnmTransform;
class J2DAnmTextureSRTKey;
class J2DAnmColor;

class LANBackground : public J2DScreen
{
public:
    void setup(JKRArchive*);
    void calc();
    void entry();
    void unentry();

    J2DAnmTransform* mTransform;      // 0x118
    J2DAnmTextureSRTKey* mTexSRTKey;  // 0x11c
    J2DAnmColor* mColor;              // 0x120
    s16 mTransformFrame;              // 0x124
    s16 mColorFrame;                  // 0x126
};

#endif // LANBACKGROUND_H
