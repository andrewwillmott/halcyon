//
//  File:       TouchInfo.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef TOUCH_INFO_H
#define TOUCH_INFO_H

#include <CLDefs.h>
#include <VL234f.h>

namespace nHL
{
    class cIApp;
}

enum { kMaxTouches = 16 };

struct cTouchInfo
{
    int     mID;
    Vec2f   mPosition;
};
bool operator==(const cTouchInfo& a, const cTouchInfo& b);

struct cTouchesInfo
{
    int         mDevice;
    int         mNumTouches;
    cTouchInfo  mTouches[kMaxTouches];
    double      mTimeStamp;
    int         mFrame;
};

void SetTouchpadEnabled(bool enabled);
bool TouchpadEnabled();

void SendTouchUpdatesToApp(nHL::cIApp* app, const cTouchesInfo* touches, Vec2f windowSize);


#endif
