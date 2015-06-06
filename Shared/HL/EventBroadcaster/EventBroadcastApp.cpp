//
//  File:       EventBroadcastApp.cpp
//
//  Function:   Broadcast events from touch device
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "EventBroadcastApp.h"

#include <IHLConfigManager.h>

#include <HLAppRemote.h>
#include <HLCamera.h>
#include <HLDebugDraw.h>
#include <HLServices.h>
#include <HLUI.h>

#include <CLVecUtil.h>
#include <CLMatUtil.h>

using namespace nCL;
using namespace nHL;

namespace
{
}



//------------------------------------------------------------------------------
// cEBApp
//------------------------------------------------------------------------------

bool cEBApp::Init()
{
    cIAllocator* alloc = AllocatorFromObject(this);

    if (!cApp::Init())
        return false;

    mEventBroadcaster = Create<cEventBroadcaster>(alloc);
    mEventBroadcaster->Init();

    if (mEventReceiver)
        mEventReceiver->SetTarget(0);       // Ensure we don't get into a receive loop in simulator!

    mEventReceiver = 0;

    return true;
}

bool cEBApp::Shutdown()
{
    cIAllocator* alloc = AllocatorFromObject(this);

    if (mEventBroadcaster)
    {
        mEventBroadcaster->Shutdown();
        Destroy(&mEventBroadcaster, alloc);
    }

    return cApp::Shutdown();
}

void cEBApp::Update()
{
    cApp::Update();
    EBUpdate();
}

void cEBApp::EBUpdate()
{
    const cServices* hl = HL();
    const cObjectValue* config = hl->mConfigManager->Config();

    mUIState->Begin(hl->mDebugDraw->ScreenSize());

    cDebugDraw* dd = HL()->mDebugDraw;

    dd->SetAlpha(1);
    dd->SetColour(kColourGreen);
    Vec2f ts = dd->TextSize("Event Broadcaster");
    Vec2f ss = dd->ScreenSize();
    DrawText(dd, (ss[0] - ts[0]) * 0.5f, 10, "Event Broadcaster");

    DebugDrawInputState(*mInputState, dd);

   mUIState->End();
}

void cEBApp::PointerDown(int index, float x, float y, int button)
{
    mInputState->OnPointerDown(index, x, y, button);
    if (mEventBroadcaster)
        mEventBroadcaster->PointerDown(index, x, y, button);
}

void cEBApp::PointerUp(int index, float x, float y, int button)
{
    mInputState->OnPointerUp(index, x, y, button);
    if (mEventBroadcaster)
        mEventBroadcaster->PointerUp(index, x, y, button);
}

void cEBApp::PointerMove(int index, float x, float y)
{
    mInputState->OnPointerMove(index, x, y);
    if (mEventBroadcaster)
        mEventBroadcaster->PointerMove(index, x, y);
}

void cEBApp::PointersCancel()
{
    mInputState->OnPointersCancel();
    if (mEventBroadcaster)
        mEventBroadcaster->PointersCancel();
}

void cEBApp::KeyDown(int index, int key)
{
    mInputState->OnKeyDown(index, key);
    if (mEventBroadcaster)
        mEventBroadcaster->KeyDown(index, key);
}

void cEBApp::KeyUp(int index, int key)
{
    mInputState->OnKeyUp(index, key);
    if (mEventBroadcaster)
        mEventBroadcaster->KeyUp(index, key);
}

void cEBApp::KeyModifiers(int index, uint32_t mods)
{
    mInputState->OnKeyModifiers(index, tModifiers(mods));
    if (mEventBroadcaster)
        mEventBroadcaster->KeyModifiers(index, mods);
}

void cEBApp::Acceleration(const float a[3])
{
    mInputState->OnAcceleration(Vec3f(a));
    if (mEventBroadcaster)
        mEventBroadcaster->Acceleration(a);
}

void cEBApp::TimeStamp(uint64_t timeStampMS)
{
    mInputState->OnTimeStamp(timeStampMS);
    if (mEventBroadcaster)
        mEventBroadcaster->TimeStamp(timeStampMS);
}


nHL::cIApp* nHL::CreateApp()
{
    return new(Allocator(kDefaultAllocator)) cEBApp;
}

namespace nHL
{
    const char* ProjectName()
    {
        return HL_PROJECT_NAME;
    }

    const char* ProjectDir()
    {
        return HL_PROJECT_DIR;
    }

    bool SetupApp(cIApp* app)
    {
        return true;
    }
}
