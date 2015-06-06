//
//  File:       UIProto.cpp
//
//  Function:   UI prototyping
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "UIProto.h"

#include <HLMain.h>

#include <IHLConfigManager.h>

#include <HLAppRemote.h>
#include <HLDebugDraw.h>
#include <HLServices.h>
#include <HLCamera.h>
#include <HLSystem.h>

#include <CLColour.h>
#include <CLLog.h>
#include <CLValue.h>
#include <CLVecUtil.h>

using namespace nApp;
using namespace nHL;
using namespace nCL;

namespace
{
    const char* const kAppName = "UI Proto";
}



//------------------------------------------------------------------------------
// cUIProtoMode
//------------------------------------------------------------------------------

bool cUIProtoMode::Init()
{
    cIAllocator* alloc = AllocatorFromObject(this);

    mCamera = new(alloc) cSimpleCamera;
    HL()->mRenderer->RegisterCamera(kMainTag, mCamera);

    mRects[0].MakeSquare(Vec2f(200.0f,  20.0f), 80.0f);
    mRects[1].MakeSquare(Vec2f(420.0f, 220.0f), 80.0f);
    mRects[2].MakeSquare(Vec2f(320.0f, 120.0f), 80.0f);

    return true;
}

bool cUIProtoMode::Shutdown()
{
    HL()->mRenderer->RegisterCamera(kMainTag, 0);
    mCamera = 0;

    return true;
}

void cUIProtoMode::Update(float dt, float gameDT, cUIState* uiState)
{
    const cServices* hl = HL();

    cDebugDraw* dd = hl->mDebugDraw;

    // Call in order of precedence
    HandleTestMenu(uiState);
    HandleSquares(uiState);
    HandleSwipes(uiState);

    HandleStandardInput(uiState, mCamera);

    DebugDrawInputState(*uiState->InputState(), dd);

    dd->SetAlpha(1);
    dd->SetColour(kColourGreen);
    Vec2f ts = dd->TextSize(kAppName);
    Vec2f ss = dd->ScreenSize();
    DrawText(dd, (ss[0] - ts[0]) * 0.5f, 10, kAppName);
}

void cUIProtoMode::HandleTestMenu(nHL::cUIState* uiState)
{
    const tUIItemID testMenuID = ItemID(0x00e61efe);

    cUIMenuInfo* mainMenuInfo = uiState->MenuInfo(testMenuID);
    mainMenuInfo->mPosition = { 20, 40 };

    uiState->BeginMenu(testMenuID);

    tUIItemID info = ItemID(0x00d5006b, 0);
    
    if (uiState->HandleButtonWithMark(info++, "Test Mark", "+"))
        CL_LOG("Test", "Got mark\n");

    uiState->DrawSeparator();

    if (uiState->HandleButton(info++, "Button 1"))
        CL_LOG("Test", "Got button 1\n");
    if (uiState->HandleButton(info++, "Button 2"))
        CL_LOG("Test", "Got button 2\n");
    if (uiState->HandleButton(info++, "Button 3"))
        CL_LOG("Test", "Got button 3\n");

    if (uiState->HandleToggle(info++, "Toggle", &mToggleOn))
        CL_LOG("Test", "Got toggle\n");

    if (uiState->HandleButtonWithSwatch(info++, "Swatch", mSwatchOn ? kColourOrange : kColourBlue))
    {
        mSwatchOn = !mSwatchOn;
        CL_LOG("Test", "Got button 3\n");
    }

    if (uiState->HandleButton(info++, "Disabled Button", kUIDisabled))
        CL_LOG("Test", "Got button 3\n");

//    const tUIItemID subMenu = ItemID(0x00e61eff);
    tUIItemID subMenuID = info++;

    if (uiState->BeginSubMenu(subMenuID, "Sub Menu"))
    {
        tUIItemID info = ItemID(0x00d500bc, 0);

        if (uiState->HandleButton(info++, "Button 1"))
            CL_LOG("Test", "Got button 1\n");
        if (uiState->HandleButton(info++, "Button 2"))
            CL_LOG("Test", "Got button 2\n");
        if (uiState->HandleButton(info++, "Button 3"))
            CL_LOG("Test", "Got button 3\n");

        uiState->DrawSeparator();

        static float sSliderValue = 0.5f;

        if (uiState->HandleSlider(info++, "Slider", &sSliderValue))
            CL_LOG("Test", "New value: %f\n", sSliderValue);

        uiState->EndSubMenu(subMenuID);
    }

    if (uiState->BeginSubMenu(info++, "Sub Menu"))
    {
        tUIItemID info = ItemID(0x00d500bc, 0);

        if (uiState->HandleButton(info++, "Button 1"))
            CL_LOG("Test", "Got button 1\n");
        if (uiState->HandleButton(info++, "Button 2"))
            CL_LOG("Test", "Got button 2\n");
        if (uiState->HandleButton(info++, "Button 3"))
            CL_LOG("Test", "Got button 3\n");

        uiState->DrawSeparator();

        static float sSliderValue = 0.5f;

        switch (uiState->HandleSlider(info++, "Slider2", &sSliderValue))
        {
        case kSliderMove:
            CL_LOG("Test", "Slider dragged to: %f\n", sSliderValue);
            break;
        case kSliderFinal:
            CL_LOG("Test", "Final slider value: %f\n", sSliderValue);
            break;
        default:
            ;
        }

        uiState->EndSubMenu(kNullUIItemID);
    }

    uiState->ButtonSelected(info++, "Deselect Sub");

    uiState->EndMenu(testMenuID);
}

void cUIProtoMode::HandleSquares(nHL::cUIState* uiState)
{
    cDebugDraw* dd = HL()->mDebugDraw;

    tUIItemID itemID = ItemID(0x00da4ff4);

    ClrTrans selectXform = RGBHueRotate(60.0f); // bugged =P
//    ClrTrans selectXform = RGBComplement();
//    ClrTrans selectXform = RGBPixelMix(0.5f, kColourYellow);
    ClrTrans activeXform = RGBSaturate(0.5f);

    dd->SetColour(kColourWhite);
    float rotateRect[CL_SIZE(mRects)] = { 0 };

    for (int i = 0; i < CL_SIZE(mRects); i++)
    {
        cBounds2& r = mRects[i];

        uiState->AddItemRect(itemID + i, r);

        int interactionCount = uiState->InteractionCount();

        if (interactionCount == 1)
        {
            Vec2f dp = uiState->InteractionPointDeltaLast();
            r.Offset(dp);
        }
        else if (interactionCount >= 2)
        {
            float s = uiState->InteractionSpanDeltaLast();
            r.Inflate(s);

            float t = uiState->InteractionTwist();
            Vec2f p = r.Centre();
            float w = MinElt(r.Width()) * 0.5f;

            rotateRect[i] = t;
            DrawLine(dd, p[0], p[1], p[0] + w * cos(t), p[1] + w * sin(t));
        }
    }

    for (int i = CL_SIZE(mRects) - 1; i >= 0; i--)  // reverse so draw order matches interaction order
    {
        cBounds2& r = mRects[i];

        if (uiState->IsSelected(itemID + i))
            dd->SetColour(xform(selectXform, mRectColours[i]));
        else if (uiState->IsActive(itemID + i))
            dd->SetColour(xform(activeXform, mRectColours[i]));
        else
            dd->SetColour(mRectColours[i]);

        dd->SetTransform(HTrans3f(-r.Centre()) * HRot3f(rotateRect[i]) * HTrans3f(r.Centre()));
        FillRect(dd, r.mMin[0], r.mMin[1], r.mMax[0], r.mMax[1]);

        if (uiState->IsSelected(itemID + i))
        {
            int selectCount = uiState->SelectedCount(itemID + i);
            Vec2f tp = r.Centre();
            dd->SetColour(kColourBlack);
            DrawTextF(dd, tp[0], tp[1], "%d", selectCount);
        }
    }

    dd->SetTransform(Mat3f(vl_I));
}

void cUIProtoMode::HandleSwipes(nHL::cUIState* uiState)
{
    cDebugDraw* dd = HL()->mDebugDraw;

    uiState->AddItemCanvas(ItemID(0x00dd3e9c));

    int sd;
    if ((sd = uiState->InteractionHorizontalSwipe()))
    {
        if (sd == -1)
            mSwipeText = "Swipe left";
        else
            mSwipeText = "Swipe right";
    }

    if ((sd = uiState->InteractionVerticalSwipe()))
    {
        if (sd == -1)
            mSwipeText = "Swipe up";
        else
            mSwipeText = "Swipe down";
    }

    Vec2f swipeDir;
    if (uiState->InteractionSwipe(&swipeDir))
        mSwipeText.format("Swipe: %f, %f\n", swipeDir[0], swipeDir[1]);

    dd->Reset();
    dd->SetColour(kColourYellow);

    if (!mSwipeText.empty())
        DrawTextF(dd, 10, -15, "%s, active: %d, selected: %d", mSwipeText.c_str(), uiState->ActiveCount(), uiState->SelectedCount());
    else
        DrawTextF(dd, 10, -15, "active: %d, selected: %d", uiState->ActiveCount(), uiState->SelectedCount());
}



bool nHL::SetupApp(cIApp* app)
{
    app->RegisterAppMode(CL_TAG("uiProto"), new (Allocator(kDefaultAllocator)) cUIProtoMode);
    return true;
}
