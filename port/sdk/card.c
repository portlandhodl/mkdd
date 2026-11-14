/* Memory card: for now no card is inserted in either slot (saves come later,
 * backed by browser storage). The game offers to continue without saving. */
#include "port_sdk.h"

s32 CARDCheck(s32 chan)
{
    (void)chan;
    return CARD_RESULT_NOCARD;
}

s32 CARDClose(CARDFileInfo * fileInfo)
{
    (void)fileInfo;
    return CARD_RESULT_NOCARD;
}

s32 CARDCreate(s32 chan, const char * fileName, u32 size, CARDFileInfo * fileInfo)
{
    (void)chan;
    (void)fileName;
    (void)size;
    (void)fileInfo;
    return CARD_RESULT_NOCARD;
}

s32 CARDDelete(s32 chan, const char * fileName)
{
    (void)chan;
    (void)fileName;
    return CARD_RESULT_NOCARD;
}

s32 CARDFastDelete(s32 chan, s32 fileNo)
{
    (void)chan;
    (void)fileNo;
    return CARD_RESULT_NOCARD;
}

s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo * fileInfo)
{
    (void)chan;
    (void)fileNo;
    (void)fileInfo;
    return CARD_RESULT_NOCARD;
}

s32 CARDFormat(s32 chan)
{
    (void)chan;
    return CARD_RESULT_NOCARD;
}

s32 CARDFreeBlocks(s32 chan, s32 * byteNotUsed, s32 * filesNotUsed)
{
    (void)chan;
    (void)byteNotUsed;
    (void)filesNotUsed;
    return CARD_RESULT_NOCARD;
}

s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat * stat)
{
    (void)chan;
    (void)fileNo;
    (void)stat;
    return CARD_RESULT_NOCARD;
}

void CARDInit(void)
{
}

s32 CARDMount(s32 chan, void * workArea, CARDCallback detachCallback)
{
    (void)chan;
    (void)workArea;
    (void)detachCallback;
    return CARD_RESULT_NOCARD;
}

s32 CARDOpen(s32 chan, const char * fileName, CARDFileInfo * fileInfo)
{
    (void)chan;
    (void)fileName;
    (void)fileInfo;
    return CARD_RESULT_NOCARD;
}

s32 CARDProbeEx(s32 chan, s32 * memSize, s32 * sectorSize)
{
    (void)chan;
    (void)memSize;
    (void)sectorSize;
    return CARD_RESULT_NOCARD;
}

s32 CARDRead(CARDFileInfo * fileInfo, void * addr, s32 length, s32 offset)
{
    (void)fileInfo;
    (void)addr;
    (void)length;
    (void)offset;
    return CARD_RESULT_NOCARD;
}

s32 CARDRename(s32 chan, const char * oldName, const char * newName)
{
    (void)chan;
    (void)oldName;
    (void)newName;
    return CARD_RESULT_NOCARD;
}

s32 CARDSetStatus(s32 chan, s32 fileNo, CARDStat * stat)
{
    (void)chan;
    (void)fileNo;
    (void)stat;
    return CARD_RESULT_NOCARD;
}

s32 CARDUnmount(s32 chan)
{
    (void)chan;
    return CARD_RESULT_NOCARD;
}

s32 CARDWrite(CARDFileInfo * fileInfo, const void * addr, s32 length, s32 offset)
{
    (void)fileInfo;
    (void)addr;
    (void)length;
    (void)offset;
    return CARD_RESULT_NOCARD;
}
