//
//  File:       Demo.cpp
//
//  Function:   Demo app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "Demo.h"

#include <HLMain.h>

#include <IHLConfigManager.h>
#include <IHLModelManager.h>

#include <HLDebugDraw.h>
#include <HLGLUtilities.h>
#include <HLServices.h>
#include <HLCamera.h>
#include <HLSystem.h>
#include <HLUI.h>

#include <HLNet.h>  // TEMP: for testing

#include <CLBits.h>
#include <CLColour.h>
#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLMatUtil.h>
#include <CLValue.h>

using namespace nApp;
using namespace nHL;
using namespace nCL;


//------------------------------------------------------------------------------
// cDemoMode
//------------------------------------------------------------------------------

bool cDemoMode::Init()
{
    const cServices* hl = HL();
    cIAllocator* alloc = AllocatorFromObject(this);

    mCamera = Create<cSimpleCamera>(alloc);
    hl->mRenderer->RegisterCamera(kMainTag, mCamera);

    hl->mModelManager->CreateInstance(CL_TAG("teapot"));

    return true;
}

bool cDemoMode::Shutdown()
{
    HL()->mRenderer->RegisterCamera(kMainTag, 0);
    mCamera = 0;

    return true;
}

void cDemoMode::UpdateFromConfig(const nCL::cObjectValue* config)
{
    mCamera->Config(config->Member(CL_TAG("camera")).AsObject());
}

void cDemoMode::Activate()
{
}

void cDemoMode::Deactivate()
{
}

void cDemoMode::Update(float dt, float gameDT, cUIState* uiState)
{
    HandleStandardInput(uiState, mCamera);

    cDebugDraw* dd = HL()->mDebugDraw;

    dd->SetAlpha(1.0f);
    DrawAxes(dd, vl_zero, 1.0f);

    dd->SetColour(kColourOrange);
    DrawCircle(dd, Vec3f(vl_zero), 0.5f);

    dd->SetColour(kColourGrey50);
    FillBox(dd, Vec3f(-0.05f), Vec3f(+0.05f));

    dd->SetAlpha(1);
    dd->SetColour(kColourGreen);

    const char* const kCaption = "Demo App";

    Vec2f ts = dd->TextSize(kCaption);
    Vec2f ss = dd->ScreenSize();
    DrawText(dd, (ss[0] - ts[0]) * 0.5f, 10, kCaption);

    // 2D test
    dd->SetColour(kColourBlack);
    ts = DrawText(dd, 15, 10, "Hello out there");

    dd->SetColourAlpha(cColourAlpha(kColourGreen));
    FillRect(dd, 15 - 5, 10 - 4, 15 + ts[0] + 5, 10 + ts[1]);
}

void cDemoMode::HandleKeys(nCL::cInputState* inputState)
{
    if (inputState->KeyIsDown('A'))
    {
        cDebugDraw* dd = HL()->mDebugDraw;
        dd->SetColour(kColourCyan);
        DrawText(dd, 15, -10, "The 'A' key is down");
    }
}

void cDemoMode::DebugMenu(nHL::cUIState* uiState)
{
}

bool nHL::SetupApp(cIApp* app)
{
    app->RegisterAppMode(CL_TAG("demo"), new (Allocator(kDefaultAllocator)) cDemoMode);
    return true;
}
