// Debug aid: MKDD_HEAPCHECK=1 validates every JKRExpHeap once per retrace
// (from port_poll, skipping heaps whose mutex is held) and aborts with a stack trace on the first frame a
// heap is found corrupted, so the corrupting code is in the frame just run.
#include "types.h"
#define private public
#define protected public
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#undef private
#undef protected
#include <dolphin/os.h>

extern "C" int port_env(const char *name);
extern "C" void abort(void);
extern "C" u32 port_vi_retrace_count(void);
#define VIGetRetraceCountNoPoll port_vi_retrace_count

static int sChecks;
static bool sVerbose;

static int sHeaps;

static bool checkTree(JKRHeap *heap, int depth)
{
    bool ok = true;
    sHeaps++;
    u32 t = heap->getHeapType();
    if (sVerbose)
        OSReport("[heapcheck] %*s%p %c%c%c%c start %p size %08x free %08x%s\n", depth * 2, "", heap, (char)(t >> 24), (char)(t >> 16), (char)(t >> 8), (char)t,
                 heap->getStartAddr(), (unsigned)heap->getHeapSize(), (unsigned)heap->getFreeSize(), heap == JKRHeap::getCurrentHeap() ? " (current)" : "");
    if (heap->getHeapType() == 'EXPH' && heap->mMutex.thread == nullptr)
    {
        if (!((JKRExpHeap *)heap)->check())
        {
            OSReport("[heapcheck] heap %p (start %p size %08x depth %d) corrupted\n", heap, heap->getStartAddr(),
                    (unsigned)heap->getHeapSize(), depth);
            ok = false;
        }
    }
    const JSUTree<JKRHeap> &tree = heap->getHeapTree();
    for (JSUTreeIterator<JKRHeap> it(tree.getFirstChild()); it != tree.getEndChild(); ++it)
        ok &= checkTree(it.getObject(), depth + 1);
    return ok;
}

extern "C" void port_heap_check(const char *where)
{
    static int enabled = -1, from;
    if (enabled < 0)
    {
        enabled = port_env("MKDD_HEAPCHECK");
        from = port_env("MKDD_HEAPCHECK_FROM");
    }
    if (!enabled || !JKRHeap::getRootHeap())
        return;
    // where == NULL: every port_poll, once the retrace count reaches MKDD_HEAPCHECK_FROM
    if (!where && (!from || VIGetRetraceCountNoPoll() < (u32)from))
        return;
    sChecks++;
    sVerbose = where && (sChecks == 1 || VIGetRetraceCountNoPoll() % 600 == 0);
    if (!checkTree(JKRHeap::getRootHeap(), 0))
    {
        OSReport("[heapcheck] corruption detected at %s (check #%d)\n", where ? where : "poll", sChecks);
        abort();
    }
}

// Called from JS after a trap (MKDD_HEAPCHECK installs the hook) to describe the heaps.
extern "C" __attribute__((used, export_name("port_heap_dump"))) void port_heap_dump(void)
{
    JKRHeap *cur = JKRHeap::getCurrentHeap();
    OSReport("[heapdump] root %p system %p current %p\n", JKRHeap::getRootHeap(), JKRHeap::getSystemHeap(), cur);
    if (cur && cur->getHeapType() == 'EXPH')
    {
        JKRExpHeap *h = (JKRExpHeap *)cur;
        OSReport("[heapdump] current: start %p end %p free list:\n", h->mStartAddress, h->mEndAddress);
        int n = 0;
        u32 lo = (u32)h->mStartAddress, hi = (u32)h->mEndAddress;
        for (JKRExpHeap::CMemBlock *b = h->mHead; b && (u32)b >= lo && (u32)b < hi && n < 64; b = b->mNext, n++)
            OSReport("  free %p size %08x prev %p next %p\n", b, b->mAllocatedSpace, b->mPrev, b->mNext);
        if (h->mHead && (u32)h->mHead >= lo && (u32)h->mHead < hi)
        {
            u32 *p = (u32 *)(((u32)h->mTailUsedList & ~15) - 0x200);
            for (int i = 0; i < 0x300 / 4; i += 4, p += 4)
                OSReport("  %p: %08x %08x %08x %08x\n", p, p[0], p[1], p[2], p[3]);
        }
        n = 0;
        for (JKRExpHeap::CMemBlock *b = h->mTailUsedList; b && (u32)b >= lo && (u32)b < hi && n < 24; b = b->mPrev, n++)
            OSReport("  used(tail) %p hdr %04x size %08x prev %p next %p\n", b, b->mUsageHeader, b->mAllocatedSpace, b->mPrev,
                     b->mNext);
    }
}
