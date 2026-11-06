#include "Kawano/driver.h"

void OneShotAnimator2::update()
{
    if (mIsFinished)
        return;
    mFrame += mStepFrame;
    if (mFrame < mMaxFrame) {
        mCurrentValue = mMinValue + mTime * (f32)mFrame * 1.0f / 16.0f / 60.0f;
    } else {
        mCurrentValue = mMaxValue;
        mIsFinished = 1;
    }
}
