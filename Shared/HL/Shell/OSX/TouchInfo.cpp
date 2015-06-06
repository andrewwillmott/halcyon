//
//  File:       TouchInfo.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "TouchInfo.h"

#include <IHLApp.h>

#include <CoreGraphics/CoreGraphics.h>

#include <stdio.h>

#define LOCAL_LOG(M_FORMAT...) 

namespace
{
    struct cPointerTouch
    {
        int   mID;
        bool  mWasActive;
        bool  mIsActive;
        Vec2f mPos;
    };

    cPointerTouch sPointerInfo[16] = { 0 };
    int sPointerCount = 0;

    bool sTouchpadEnabled = false;
}

bool operator==(const cTouchInfo& a, const cTouchInfo& b)
{
    return        a.mID                     ==        b.mID
        && floorf(a.mPosition[0] * 100.0f) == floorf(b.mPosition[0] * 100.0f)
        && floorf(a.mPosition[1] * 100.0f) == floorf(b.mPosition[1] * 100.0f)
        ;
}


void SetTouchpadEnabled(bool enabled)
{
    if (enabled == sTouchpadEnabled)
        return;

    if (enabled)
    {
        // Decouple cursor and hide it
        CGDisplayHideCursor(kCGNullDirectDisplay);
        CGAssociateMouseAndMouseCursorPosition(false);
    }
    else
    {
        // Hand control back to the mouse
        CGDisplayShowCursor(kCGNullDirectDisplay);
        CGAssociateMouseAndMouseCursorPosition(true);
    }

    sTouchpadEnabled = enabled;
}

bool TouchpadEnabled()
{
    return sTouchpadEnabled;
}


void SendTouchUpdatesToApp(nHL::cIApp* app, const cTouchesInfo* touches, Vec2f windowSize)
{
    if (!sTouchpadEnabled)
        return;

    for (int i = 0; i < sPointerCount; i++)
    {
        sPointerInfo[i].mWasActive = sPointerInfo[i].mIsActive;
        sPointerInfo[i].mIsActive = false;
    }

    for (int i = 0; i < touches->mNumTouches; i++)
    {
        // LOCAL_LOG("  %d: at %g, %g\n", touches->mTouches[i].mID, touches->mTouches[i].mPosition[0], touches->mTouches[i].mPosition[0]);

        int tid = touches->mTouches[i].mID;
        int j = 0;

        for ( ; j < sPointerCount; j++)
            if (tid == sPointerInfo[j].mID)
                break;

        if (j == sPointerCount)
        {
            sPointerCount++;
            sPointerInfo[j].mID = tid;
            sPointerInfo[j].mWasActive = false;
        }

        sPointerInfo[j].mIsActive = true;
        sPointerInfo[j].mPos = touches->mTouches[i].mPosition * windowSize;
        sPointerInfo[j].mPos[1] = windowSize[1] - sPointerInfo[j].mPos[1];
    }

    for (int i = 0; i < sPointerCount; i++)
    {
        LOCAL_LOG("pointer %d: %g, %g, active: %d, %d\n", i, sPointerInfo[i].mPos[0], sPointerInfo[i].mPos[1],
            sPointerInfo[i].mWasActive, sPointerInfo[i].mIsActive);

        if (!sPointerInfo[i].mIsActive && sPointerInfo[i].mWasActive)
        {
            LOCAL_LOG("UP %d\n", i);
            app->PointerUp(i, sPointerInfo[i].mPos[0], sPointerInfo[i].mPos[1], 1);
        }
        else if (sPointerInfo[i].mIsActive && !sPointerInfo[i].mWasActive)
        {
            LOCAL_LOG("DOWN %d\n", i);
            app->PointerDown(i, sPointerInfo[i].mPos[0], sPointerInfo[i].mPos[1], 1);
        }
        else
            app->PointerMove(i, sPointerInfo[i].mPos[0], sPointerInfo[i].mPos[1]);
    }

    while (sPointerCount > 0 && !sPointerInfo[sPointerCount - 1].mIsActive)
        sPointerCount--;
}
