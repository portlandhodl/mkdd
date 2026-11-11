#ifndef JAUDIO_JAUSTREAMARAMMGR_H
#define JAUDIO_JAUSTREAMARAMMGR_H

#include "JSystem/JAudio/Interface/JAIStreamDataMgr.h"
#include "JSystem/JAudio/System/JASAramStream.h"
#include "JSystem/JAudio/System/JASHeap.h"
#include "JSystem/JAudio/System/JASKernel.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "std/bitset.h"

template <size_t A0>
class JAUStreamAramMgrBase_ : public JAIStreamAramMgr {
public:
    JAUStreamAramMgrBase_() {
        for (int i = 0; i < A0; i++) {
            field_0x4.reset(i);
        }
    }
    virtual ~JAUStreamAramMgrBase_() { releaseAram_JAUStreamAramMgrBase_(); }
    bool isStreamUsingAram() { return field_0x4.any(); }
    void releaseAram_JAUStreamAramMgrBase_() {
#line 38
        JUT_ASSERT(! isStreamUsingAram());
        for (int i = 0; i < A0; i++) {
            if (mHeaps[i].isAllocated()) {
                if (!mHeaps[i].free()) {
#line 47
                    JUT_ASSERT(0);
                }
            }
        }
    }

protected:
    std::bitset<A0> field_0x4;
    JASHeap mHeaps[A0];
};

template <size_t MAX_CHUNKS_>
class JAUStreamStaticAramMgr_ : public JAUStreamAramMgrBase_<MAX_CHUNKS_> {
public:
    JAUStreamStaticAramMgr_() { field_0x4c = 0; }
    virtual u32 newStreamAram(u32 *size) {
        for (u32 i = 0; i < field_0x4c; i++) {
            if (this->field_0x4.test(i)) {
                continue;
            }
            this->field_0x4.set(i, true);
            *size = this->mHeaps[i].mSize;
            return (u32)this->mHeaps[i].getBase();
        }
        return 0;
    }
    virtual int deleteStreamAram(u32 addr) {
        for (u32 i = 0; i < field_0x4c; i++) {
            if (!this->field_0x4.test(i)) {
                continue;
            }
            if ((u32)this->mHeaps[i].getBase() != addr) {
                continue;
            }
            this->field_0x4.reset(i);
            return true;
        }
        return false;
    }
    bool isAramReserved() const { return field_0x4c > 0; }
    void reserveAram(JASHeap *heap, int numReserve, u32 blocks) {
#line 72
        JUT_ASSERT(! isAramReserved());
        JUT_ASSERT(! isStreamUsingAram());
        if (!heap) {
            heap = JASKernel::getAramHeap();
        }
        if (numReserve <= 0) {
            numReserve = 1;
        }
#line 83
        JUT_ASSERT(numReserve <= MAX_CHUNKS);
        int size = blocks * JASAramStream::getBlockSize();
        for (int i = 0; i < numReserve; i++) {
            if (!this->mHeaps[i].alloc(heap, size)) {
                break;
            }
            field_0x4c = i + 1;
        }
    }

private:
    static const size_t MAX_CHUNKS = MAX_CHUNKS_;

    int field_0x4c;
};

#endif
