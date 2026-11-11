#ifndef PRINTMEMORYCARD_H
#define PRINTMEMORYCARD_H

#include <JSystem/JKernel/JKRHeap.h>
#include <JSystem/JUtility/TColor.h>

class PrintMemoryCard
{
public:
    PrintMemoryCard(JKRHeap *heap); // 0x8016be58

    // TODO: are these actually correct?
    enum MessageID
    {
        mcNoCardContinue,
        mcNoUseContinue,
        mcWithouMKContinue,
        mcBrokenNoSaveContinue,
        mcBrokenGoFormat,
        mcNoFileNoSave,
        mcGameSavingNoTouch,
        mcGetSavingNoTouch,
        mcMissSaved,
        mcInCardPushButton,
        mcChangeCardPushButton,
        mcMissFormat,
        mcFormatLost,
        mcFormatNoTouch,
        mcNoSaveContinue,
        mcMakeFile,
        mcMakeFileNoTouch,
        mcMadeFile,
        mcMissMakeFile,
        mcNoSpaceGoIPL,
        mcTooManyFilesGoIPL,
        mcNoUse,
        mcWithoutMK,
        mcBrokenNoUse,
        mcBrokenNeedFormat,
        mcGstLoad,
        mcLoaded,
        mcNoSpaceNoSave,
        mcTooManyFiles,
        mcGstSave,
        mcSaved,
        mcGstOverwrite,
        mcNoGst,
        mcGstNoLoad,
        mcGstBrokenNoLoad,
        mcBrokenNoLoad,
        // these are lan related for whatever reason
        mcTooManyFilesContinue, // begin lan setup
        mcNoSpaceContinue,      // disconnect lan link
        mcMsg38,                // disconnecting LAN Link
        mcMsg39,                // Quit are you sure?
        mcMsg40,                // black screen, no text here, also panic if bmg gets requested
        mcMsgMax
    };
    
    // TODO
    void setStuff(u8 a, u8 b, u8 c) {
        _c = a ;
        _d = b;
        _e = c;
    }

    void reset();           // 0x8016bf24
    void init(MessageID);   // 0x8016bf90
    void changeMessage();   // 0x8016bfac
    void draw();            // 0x8016c3e0
    void calc();            // 0x8016c410
    void closeWindow();     // 0x8016c92c
    void closeWindowNoSe(); // 0x8016c9f8
    void setBmgPtr();       // 0x8016ca9c
    bool isMessage();       // 0x8016cb0c
    int getChoiceType();    // 0x8016cb98
    int getWindowSize();    // 0x8016cc34
    int getWindowColor();   // 0x8016cd00
    // Inline/Unused
    // void initWindow();
    // ~PrintMemoryCard();

    int get_1cThing() {
        if (_20 == 0 || _20 == 25)
            return 3;
        return _1c;
     }

     int get_14Thing() {
         if (_1c == 2 && (_20 == 0 || _20 == 25))
             return _14;
         return 2;
     }

private:
    void *mColorPtr;             // 0x0
    void *mBmgPtr;               // 0x4
    class PrintWindow *mpWindow; // 0x8
public:
    u8 _c;
    u8 _d;
    u8 _e;
private:
    u8 _f;                          // 0x0f
    MessageID mMessageID;           // 0x10
    int _14;                        // 0x14
    int _18;                        // 0x18
    int _1c;                        // 0x1c
    int _20;                        // 0x20
    int _24;                        // 0x24
    int _28;                        // 0x28
    JUtility::TColor mColor;        // 0x2c
}; // Size: 0x30

#endif // PRINTMEMORYCARD_H
