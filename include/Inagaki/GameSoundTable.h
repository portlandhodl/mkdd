#ifndef GAMESOUNDTABLE_H
#define GAMESOUNDTABLE_H

#include <JSystem/JAudio/Interface/JAISound.h>
#include <JSystem/JAudio/Interface/JAIStreamDataMgr.h>
#include "JSystem/JAudio/JAUAudience.h"
#include "JSystem/JAudio/JAUSoundTable.h"

namespace GameAudio
{
    class CustomSoundTable : public JAISoundInfo, public JAUSoundInfo, public JAIStreamDataMgr
    {
    public:
        CustomSoundTable() : JAISoundInfo(true), JAUSoundInfo(true) {}

        virtual u32 getSoundType(JAISoundID soundID) const;
        virtual int getCategory(JAISoundID soundID) const;

        virtual u32 getPriority(JAISoundID soundID) const;
        virtual void getSeInfo(JAISoundID soundID, JAISe *se) const;
        virtual void getSeqInfo(JAISoundID soundID, JAISeq *seq) const;
        virtual void getStreamInfo(JAISoundID soundID, JAIStream *stream) const;

        virtual ~CustomSoundTable(){};
        virtual u16 getAudibleSw(JAISoundID soundID) const;
        virtual u16 getBgmSeqResourceID(JAISoundID soundID) const;

        virtual u8 getPTrack(JAISoundID soundID) const;
        virtual u32 getSwBit(JAISoundID soundID) const;
        virtual u8 getLevelFlag(JAISoundID soundID) const;
        virtual s32 getStreamFileEntry(JAISoundID soundID);

        void initAudibleSw();

        // UNUSED
        void getStreamFilePath(JAISoundID soundID);
    
    private:
        void getSoundInfo_(JAISoundID soundID, JAISound *sound) const;

        bool isValid() const {
            return JASGlobalInstance<JAUSoundTable>::getInstance() != NULL
                && JASGlobalInstance<JAUSoundTable>::getInstance()->getResource() != NULL;
        }
    };
}

#endif
