#ifndef PRINTPAL_H
#define PRINTPAL_H

#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DPane.h"
#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "types.h"

class PrintPal {
public:
    enum MessageID // Based of how it's structured in the file itself, might be wrong
    {
        mcShow_Dialog,
        mcSet_Option1,
        mcSet_Option2,
        mcShow_Logo,
        mcShow_Set
    };

    PrintPal(JKRHeap *heap);
    void init(MessageID);
    void draw();
    void calc(int);
    bool isMessage();
    bool isChoice();
    int getCounter();

    int getState() { return mState; }
    int getOption() { return mOption; }
    void setOption(int val) { mOption = val; }

private:
    void *mColorRes;                     // 0x0
    void *mPalRes;                       // 0x4
    J2DOrthoGraph *mOrthoGraph;          // 0x8
    JKRArchive *mArchive;                // 0xc
    J2DScreen *mScreenPal;               // 0x10
    J2DScreen *mScreenLogo;              // 0x14
    J2DPane *mPane;                      // 0x18
    J2DTextBox *mTextBox;                // 0x1c
    J2DTextBox *mTextBoxYes;             // 0x20
    J2DTextBox *mTextBoxNo;              // 0x24
    J2DTextBox *mTextBoxNum;             // 0x28
    J2DPicture *mPictureYes;             // 0x2c
    J2DPicture *mPictureNo;              // 0x30
    J2DPicture *mPictureNintendo;        // 0x34
    J2DPicture *mPictureDolby;           // 0x38
    J2DPicture *mPictureBack;            // 0x3c
    u8 _40[0xc];                         // 0x40
    J2DAnmTransform *mTransform;         // 0x4c
    J2DAnmColor *mColors[4];             // 0x50
    J2DAnmTextureSRTKey *mSTRKey;        // 0x60
    s32 mTransformFrame;                 // 0x64
    s32 mSTRKeyFrame;                    // 0x68
    s32 mFrame;                          // 0x6c
    MessageID mMsgId;                    // 0x70
    int mState;                          // 0x74
    int mOption;                         // 0x78
    s32 mColorsFrame;                    // 0x7c
    J2DTextBox::TFontSize mFontSize;     // 0x80
    J2DTextBox::TFontSize mFontSizeYes;  // 0x88
    J2DTextBox::TFontSize mFontSizeNo;   // 0x90
    u8 _98[8];                           // 0x98
};

#endif
