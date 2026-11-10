#ifndef STAFFROLL2D_H
#define STAFFROLL2D_H

#include <JSystem/J2D/J2DPicture.h>
#include <JSystem/J2D/J2DScreen.h>
#include <JSystem/J2D/J2DGrafContext.h>
#include <JSystem/JKernel/JKRHeap.h>
#include <JSystem/JUtility/JUTFont.h>

#include "JSystem/JGeometry/Box.h"
#include "Sato/JPEffectMgr.h"
#include "kartLocale.h"

#include "types.h"

class Font1letterPrinter {
public:
    Font1letterPrinter();
    ~Font1letterPrinter();
    void get(J2DTextBox *, f32, f32, char *, f32);
    void print();
};

class ExitBoxInfo {
public:
    ExitBoxInfo();
    void setBox(J2DPicture *);
    JGeometry::TBox2f &getBox() { return mBox; }
    int getDir() { return mDir; }
private:
    JGeometry::TBox2f mBox;
    int mDir;
}; // Size: 0x14 

class StaffRoll2D {
public:
    StaffRoll2D(JKRHeap *, Language, VideoMode, int); // 0x801f5b40
    void init(); // 0x801f6640
    void reset(); // 0x801f6694
    void draw(); // 0x801f66ec
    void calc(); // 0x801f6948
    void start(int); // 0x801f6c28
    ExitBoxInfo *getExitBoxInfoPtr(int, int); // 0x801f6d2c
    int getExitBoxNum(int); // 0x801f6d64
    int getVersion(Language, VideoMode); // 0x801f6d94
    int getSceneDataID(int, int); // 0x801f6db0
    
    static u64 mTag_textbox[375]; // 0x8037b9d8
    static u64 mTag_exitbox[200]; // 0x8037c590
    static int maConvertChartSceneNo2DataID[14]; // 0x8037cbd0, unsure
    static u64 mTag_textboxTrue[2]; // 0x8037cc00
    static const char *mBloName[25]; // 0x8039bcc8
    static const char *mBckName[25]; // 0x8039bd2c
    static const char *mBrkName[25]; // 0x8039bd90
    static const char *mName_jpa[375]; // 0x8039bdf4
    static const char *mBloNameTrue[2]; // 0x80414840
    static const char *mBckNameTrue[2]; // 0x80414848
    static const char *mBpkNameTrue[2]; // 0x80414850
    // Inline/Unused    
    //void Constructer_JPA(JKRHeap *);
    ~StaffRoll2D();
    //void initExitBoxInfo();
    //void getWidth(char *);

private:
    J2DGrafContext *mGrafContext;            // 0x0000
    J2DPane *mPane;                          // 0x0004
    J2DTextBox *mTextBox[18][15];            // 0x0008
    J2DTextBox *mTextBoxVersion;             // 0x0440
    J2DPicture *mExitBoxPicture[18][8];      // 0x0444
    u8 _0684[4];                             // 0x0684
    J2DScreen *mScreen[18];                  // 0x0688
    J2DAnmColor *mAnmColor[18];              // 0x06D0
    J2DAnmTransform *mAnmTransform[18];      // 0x0718
    u8 _0760[0x3F0];                         // 0x0760
    J2DScreen *mScreenTrue;                  // 0x0B50
    J2DAnmTransform *mAnmTransformTrue;      // 0x0B54
    J2DAnmColor *mAnmColorTrue;              // 0x0B58
    J2DAnmTextureSRTKey *mAnmTexSRTTrue;     // 0x0B5C
    f32 mValue[18][15];                      // 0x0B60
    int mField0F98;                          // 0x0F98
    u8 _0F9C[0x21C0];                        // 0x0F9C
    f32 mField315C;                          // 0x315C
    f32 mField3160;                          // 0x3160
    f32 mField3164;                          // 0x3164
    f32 mField3168;                          // 0x3168
    JUtility::TColor mColor1;                // 0x316C
    JUtility::TColor mColor2;                // 0x3170
    JUTFont *mFont;                          // 0x3174
    f32 mField3178;                          // 0x3178
    int mField317C;                          // 0x317C
    char mChars[4];                          // 0x3180
    ExitBoxInfo mExitBoxInfo[18][8];         // 0x3184
    f32 mValue2[18][15];                     // 0x3CC4
    f32 mValue3[18][15];                     // 0x40FC
    int mExitBoxNum[18];                     // 0x4534
    int mField457C;                          // 0x457C
    int mField4580;                          // 0x4580
    int mField4584;                          // 0x4584
    int mVersion;                            // 0x4588
    u8 _458C[0xC];                           // 0x458C
};

#endif
