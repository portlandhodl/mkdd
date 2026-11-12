#ifndef STATEOBSERVER_H
#define STATEOBSERVER_H

#include "types.h"
#include <JSystem/JUtility.h>

// TODO: check if this matches in every instance

#define Observer_FindAndInit(cname, max)                                                                                                 \
    const StateFuncSet<cname> *func = FindStateFunc<cname>(getState(), cname::sTable, max); \
    (this->*func->mInitFunc)();

#define Observer_FindAndExec(cname, max)                                                                                                 \
    const StateFuncSet<cname> *func = FindStateFunc<cname>(getState(), cname::sTable, max); \
    (this->*func->mExecFunc)();

class StateObserver
{
public:
    StateObserver();     // 0x802714b0
    void ExecuteState(); // 0x80271518
    void ResetState();   // 0x80271570
    void changeState();
    void setState(u16);
    u16 getState() const { return mPrevState; }
    u32 getStateCount() const { return mStateCount; } 

    // Vtable
    virtual ~StateObserver();
    virtual void MoveExec() = 0;
    virtual void InitExec() = 0;

    template <typename T>
    struct StateFuncSet
    {
        typedef void (T::*StateFunc)();
        u16 mId;
        StateFunc mInitFunc;
        StateFunc mExecFunc;
    };

    template <typename T>
    static const StateFuncSet<T> *FindStateFunc(u16 id, const StateFuncSet<T> *funcTable, u16 max)
    {
#line 55
        JUT_MINMAX_ASSERT(0, max, 32);
        for (u16 i = 0; i < max; i++)
        {
            if (id == funcTable[i].mId)
                return &funcTable[i];
        }
        return nullptr;
    }

private:
    u16 mState;      // 4
    u16 mPrevState;  // 6
    u32 mStateCount; // 8
};

template <typename T>
class StateObserverT : public StateObserver
{
public:
    ~StateObserverT() {}
};

template <typename T>
class TStateObserver
{
public:
    typedef void (T::*StateFunc)();

    TStateObserver() : mState(0), mIsInit(1), mStateCount(0), mFuncNum(0) {}

    void createStateFunc(T *owner, const u8 &num)
    {
        mFuncNum = num;
        mOwner = owner;
        mInitFuncs = new StateFunc[mFuncNum];
        mDoFuncs = new StateFunc[mFuncNum];
        resetState();
    }

    void resetState()
    {
        mPrevState = 0;
        mState = 0;
        mIsInit = 1;
        mStateCount = 0;
    }

    void changeState(const u8 &state)
    {
        mPrevState = mState;
        mState = state;
        mIsInit = 1;
        mStateCount = 0;
    }

    void registStateFunc(const u8 &id, const StateFunc &initFunc, const StateFunc &doFunc)
    {
#line 109
        JUT_MAX_ASSERT(id, mFuncNum);
        mInitFuncs[id] = initFunc;
        mDoFuncs[id] = doFunc;
    }

    u8 mState;              // 0x0
    u8 mPrevState;          // 0x1
    u8 mIsInit;             // 0x2
    T *mOwner;              // 0x4
    StateFunc *mInitFuncs;  // 0x8
    StateFunc *mDoFuncs;    // 0xc
    u32 mStateCount;        // 0x10
    u8 mFuncNum;            // 0x14
};

#endif
