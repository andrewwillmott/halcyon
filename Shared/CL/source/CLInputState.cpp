//
//  File:       CLInputState.cpp
//
//  Function:   Centralises state for UI interaction, allowing a poll-style
//              model rather than event-handling.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLInputState.h>

#include <CLLog.h>

#include <string.h>

#if 0
    #define INPUT_INFO printf
#else
    #define INPUT_INFO(M_ARGS...)
#endif

using namespace nCL;

namespace
{
    cPointerInfo kDefaultPointerInfo;
}

cInputState::cInputState() :
    mNumPointers(0),
//    mPointers[kMaxPointers];
    mCounter(0)
{
}

void cInputState::Begin()
{
    mScrollDelta = mNextScrollDelta;
    mNextScrollDelta = vl_0;
}

void cInputState::End()
{
    mCounter++;

    // If pointer was up previous frame, and is still up, we no longer care about it.
    while (mNumPointers > 0
        && ((mPointers[mNumPointers - 1].mLastModifiers & kModPointerMask) == 0)
        && ((mPointers[mNumPointers - 1].mModifiers     & kModPointerMask) == 0)
    )
    {
        mNumPointers--;

        INPUT_INFO("deactivate pointer %d at %d\n", mNumPointers, mCounter);
    }

    for (int i = 0; i < kMaxPointers; i++)
    {
        cPointerInfo& info = mPointers[i];

        info.mLastModifiers = info.mModifiers;
        info.mLastPos       = info.mCurrentPos;

        info.mModifiersDown = kModNone;
        info.mModifiersUp   = kModNone;
    }

    memcpy(mLastKeyStates, mKeyStates, sizeof(mKeyStates));
    mLastKeyModifiers = mKeyModifiers;
}

void cInputState::OnPointerDown(int index, float x, float y, int button)
{
    if (uint(index) < kMaxPointers)
    {
        cPointerInfo& info = mPointers[index];

        Vec2f p(x, y);

        info.mModifiers     |= kModButton1 << (button - 1);
        info.mModifiersDown |= kModButton1 << (button - 1);

        info.mCurrentPos = p;
        info.mLastPos    = p;   // In case there's no position info when the pointer is up
        info.mStartPos   = p;
        info.mStartModifiers = info.mModifiers | mKeyModifiers;
        info.mStartCounter = mCounter;
        info.mStartTimeStamp = mTimeStamp;

        if (index >= mNumPointers)
            mNumPointers = index + 1;

        INPUT_INFO("PointerDown %d (%d) @ %g, %g\n", index, button, x, y);
    }
}

void cInputState::OnPointerUp(int index, float x, float y, int button)
{
    if (uint(index) < kMaxPointers)
    {
        cPointerInfo& info = mPointers[index];

        info.mModifiers &= ~(kModButton1 << (button - 1));
        info.mModifiersUp |= kModButton1 << (button - 1);

        INPUT_INFO("PointerUp %d (%d)\n", index, button);
    }
}

void cInputState::OnPointerMove(int index, float x, float y)
{
    if (uint(index) < kMaxPointers)
    {
        cPointerInfo& info = mPointers[index];

        info.mCurrentPos = Vec2f(x, y);
        INPUT_INFO("PointerMove %d @ %g, %g\n", index, x, y);
    }
}

void cInputState::OnPointersCancel()
{
    mNumPointers = 0;

    for (int i = 0; i < kMaxPointers; i++)
        mPointers[i].mModifiers = kModNone;

    INPUT_INFO("PointersCancel\n");
}

void cInputState::OnKeyDown(int index, int keyCode)
{
    mKeyStates[keyCode >> kKeyCodeWordShift] |= 1 << (keyCode & kKeyCodeWordMask);
}

void cInputState::OnKeyUp(int index, int keyCode)
{
    mKeyStates[keyCode >> kKeyCodeWordShift] &= ~(1 << (keyCode & kKeyCodeWordMask));
}

void cInputState::OnKeyModifiers(int index, tModifierSet modifiers)
{
    mKeyModifiers = modifiers;
}

void cInputState::OnAcceleration(Vec3f acc)
{
    mAcceleration = acc;
}

void cInputState::OnScroll(Vec2f delta)
{
    mNextScrollDelta += delta;
}

void cInputState::Reset()
{
    mNumPointers = 0;

    for (int i = 0; i < kMaxPointers; i++)
        mPointers[i] = kDefaultPointerInfo;

    memset(mKeyStates,     0, sizeof(mKeyStates));
    memset(mLastKeyStates, 0, sizeof(mLastKeyStates));

    mKeyModifiers          = kModNone;
    mLastKeyModifiers      = kModNone;

    mScrollDelta = vl_0;
    mNextScrollDelta = vl_0;
}

