//
//  File:       HLApp.cpp
//
//  Function:   Base implementation of cIApp.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLApp.h>

#include <IHLAudioManager.h>
#include <IHLAVManager.h>
#include <IHLConfigManager.h>
#include <IHLRenderer.h>
#include <IHLEffectsManager.h>

#include <HLAppRemote.h>
#include <HLDebugDraw.h>
#include <HLServices.h>
#include <HLShell.h>
#include <HLSystem.h>
#include <HLUI.h>

#include <CLInputState.h>
#include <CLJSON.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLSystemInfo.h>
#include <CLUtilities.h>
#include <CLValue.h>

using namespace nHL;
using namespace nCL;

namespace
{
    const float kMaxDeltaTime = 0.5f;
}

// Declare this locally so the destructor isn't instantiated in other compilation units.
cApp::cApp()
{
}

cApp::~cApp()
{
}

// cIApp

bool cApp::Init()
{
    if (mSystem)
        return false;

    cIAllocator* alloc = AllocatorFromObject(this);

    mSystem = CreateSystem(alloc);
    if (!mSystem)
        return false;

#if !defined(CL_RELEASE)
    mSystem->SetProjectDir(ProjectDir());
#endif

    if (!mSystem->Init())
        return false;
    
    if (!SetupApp(this))
        return false;

    for (auto appMode : mAppModes)
        appMode->PreInit();

    CL_LOG_I("Memory", "Before app post init: %4.1f MB\n", UsedMemoryInMB());

    if (!mSystem)
        return false;

    UpdateFBInfo();

    if (!mSystem->PostInit())
        return false;

    mInputState = Create<cInputState>(alloc);
    mUIState    = Create<cUIState   >(alloc);
    mUIState->SetInputState(mInputState);

    cIUIDraw* debugUIDraw = CreateDebugUIDraw(alloc);
    debugUIDraw->Config(HL()->mConfigManager->Config()->Member("debugUI").AsObject());
    mUIState->SetUIDraw(debugUIDraw);

    mUIState->MenuInfo(kDevMenuID)->mPosition = { -10, +10 };

#if !defined(CL_IOS) || TARGET_IPHONE_SIMULATOR
    mEventReceiver = Create<cEventReceiver>(alloc);
    mEventReceiver->Init();
    mEventReceiver->SetTarget(this);
#endif

    CL_LOG_I("Memory", "After app post init: %4.1f MB\n", UsedMemoryInMB());

#ifndef CL_RELEASE
    auto appInfo = HL()->mConfigManager->AppInfo();
    cValue wrapper(appInfo);
    LogJSON("App", "App settings", wrapper);
#endif

    // Set up app modes
    for (auto appMode : mAppModes)
        appMode->Init();

    const cObjectValue* config = HL()->mConfigManager->Config();

    for (auto appMode : mAppModes)
        appMode->UpdateFromConfig(config);

    if (!mAppModes.empty() && mAppModeIndex < 0)
        SetAppMode(0);

    CL_LOG_I("Memory", "After app mode init: %4.1f MB\n", UsedMemoryInMB());

    return true;
}

bool cApp::Shutdown()
{
    if (!mSystem)
        return false;
    
    cIAllocator* alloc = AllocatorFromObject(this);

    if (mAppMode)
    {
        mAppMode->Deactivate();
        mAppMode = 0;
    }

    for (auto appMode : mAppModes)
        appMode->Shutdown();

    mAppModes.clear();
    mAppModeTagToIndex.clear();
    mAppModeIndex = -1;

    if (mEventReceiver)
    {
        mEventReceiver->Shutdown();
        Destroy(&mEventReceiver, alloc);
    }

    Destroy(&mUIState, alloc);
    Destroy(&mInputState, alloc);

    mSystem->Shutdown();
    mSystem = 0;

    return true;
}

void cApp::RegisterAppMode(nCL::tTag tag, cIAppMode* appMode)
{
    auto it = mAppModeTagToIndex.find(tag);

    if (it != mAppModeTagToIndex.end())
    {
        if (mAppModes[it->second])
            mAppModes[it->second]->Shutdown();

        mAppModes[it->second] = appMode;
    }
    else
    {
        mAppModeTagToIndex[tag] = mAppModes.size();
        mAppModes.push_back(appMode);
    }
}

bool cApp::SetAppMode(int modeIndex)
{
    if (mAppModes[modeIndex] == mAppMode)
        return false;

    if (mAppMode)
        mAppMode->Deactivate();

    mAppMode = mAppModes[modeIndex];
    mAppModeIndex = modeIndex;

    if (mAppMode)
        mAppMode->Activate();

    return true;
}

bool cApp::SetAppMode(nCL::tTag tag)
{
    auto it = mAppModeTagToIndex.find(tag);

    if (it != mAppModeTagToIndex.end())
        return SetAppMode(it->second);

    return false;
}

// Shell access

void cApp::Update()
{
    const cServices* hl = HL();

    const cObjectValue* config = hl->mConfigManager->Config();

    HL()->mEffectsManager->Params()->mFlags.mDebugBoundingBoxes = hl->mConfigManager->Preferences()->Member(CL_TAG("showBounds")).AsBool();

    float deltaT = mFrameTimer.DeltaTime();

    if (deltaT > kMaxDeltaTime)
        deltaT = kMaxDeltaTime;

    float gameDT = deltaT;

    mMSPF = lerp(mMSPF, deltaT * 1000.0f, 0.1f);

    if (IsPaused())
        gameDT = 0.0f;
    else
    {
        float timeScale = config->Member("timeScale").AsFloat(1.0f);
        gameDT *= timeScale;
    }

    mSystem->Update(deltaT, gameDT);

    SetInputShaderData(mInputState, hl->mRenderer);

    if (mAppMode && hl->mConfigManager->ConfigModified())
        mAppMode->UpdateFromConfig(hl->mConfigManager->Config());

    Vec2f screenSize = hl->mRenderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    mUIState->Begin(screenSize);

#ifndef CL_RELEASE
    auto dd = hl->mDebugDraw;

    if (mDevMode)
    {
        dd->Reset();

        if (BeginDevUI())
            EndDevUI();

        if (mDebugInput)
            nHL::DebugDrawInputState(*mInputState, dd);
    }
#endif

    if (mAppMode)
        mAppMode->Update(deltaT, gameDT, mUIState);

    HandleGlobalKeys();

    if (mAppMode)
        mAppMode->HandleKeys(mInputState);

#ifndef CL_RELEASE
    dd->Reset();

    const char* version = hl->mConfigManager->Preferences()->Member("app").Member("version").AsString();
    if (version)
    {
        dd->SetColour(kColourYellow);
        DrawTextF(dd, 10, 10, "%s", version);
    }

    if (mDevMode)
    {
        const cUIMenuInfo* devMenuInfo = mUIState->MenuInfo(kDevMenuID);

        float lineX = -20.0f - devMenuInfo->mTopMaxSize[0];
        float lineY = 10.0f;

        if (mMSPF > 40.0f)
            dd->SetColour(cColour(1.0f, 0.2f, 0.0f));
        else if (mMSPF > 34.0f)
            dd->SetColour(cColour(1.0f, 0.6f, 0.0f));
        else
            dd->SetColour(kColourYellow);

        lineY += DrawTextF(dd, lineX, lineY, "MSPF: %5.1f", mMSPF)[1];
        lineY += DrawTextF(dd, lineX, lineY, "MB: %5.1f", UsedMemory() / 1024.0f / 1024.0f)[1];

        dd->SetColour(kColourYellow);

        if (mShowEffectsStats)
        {
            const char* effectsStats = HL()->mEffectsManager->StatsString();
            if (effectsStats)
                lineY += DrawTextF(dd, lineX, lineY, "%s", effectsStats)[1];
        }

        if (IsPaused())
        {
            Vec2f ss = dd->ScreenSize();

            dd->SetColourAlpha(cColourAlpha(kColourRed));
            DrawRect(dd, Vec2f(vl_zero) + Vec2f(0.5f, 0.5f), ss - Vec2f(0.5f, 0.5f));
        }
    }
#endif

    mUIState->End();
}

void cApp::Render()
{
    const cServices* hl = HL();
    cIRenderer* renderer = hl->mRenderer;

    renderer->Render();
}

void cApp::PointerDown(int index, float x, float y, int button)
{
    Vec2f c = AdjustForOrientation(Vec2f(x, y));
    mInputState->OnPointerDown(index, c[0], c[1], button);
}

void cApp::PointerUp(int index, float x, float y, int button)
{
    Vec2f c = AdjustForOrientation(Vec2f(x, y));
    mInputState->OnPointerUp(index, c[0], c[1], button);
}

void cApp::PointerMove(int index, float x, float y)
{
    Vec2f c = AdjustForOrientation(Vec2f(x, y));
    mInputState->OnPointerMove(index, c[0], c[1]);
}

void cApp::PointersCancel()
{
    mInputState->OnPointersCancel();
}

void cApp::KeyDown(int index, int key)
{
    mInputState->OnKeyDown(index, key);
}

void cApp::KeyUp(int index, int key)
{
    mInputState->OnKeyUp(index, key);
}

void cApp::KeyModifiers(int index, uint32_t mods)
{
    mInputState->OnKeyModifiers(index, tModifiers(mods));
}

void cApp::Acceleration(const float a[3])
{
    mInputState->OnAcceleration(Vec3f(a));
}

void cApp::Orientation(tDeviceOrientation orientation)
{
    if (mOrientation != orientation)
    {
        mOrientation = orientation;

        UpdateFBInfo();
    }
}

void cApp::ScrollDelta(float x, float y)
{
    mInputState->OnScroll(Vec2f(x, y));
}

void cApp::TimeStamp(uint64_t timeStampMS)
{
    mInputState->OnTimeStamp(timeStampMS);
}

void cApp::SetFrameBufferInfo(uint32_t id, int w, int h)
{
    mFBID = id;
    mFBSize = { float(w), float(h) };

    UpdateFBInfo();
}


// cApp internal

void cApp::UpdateFBInfo()
{
    cIRenderer* renderer = HL()->mRenderer;

    if (renderer)
        renderer->SetFrameBufferInfo(mFBID, mFBSize[0], mFBSize[1], mOrientation);
}

Vec2f cApp::AdjustForOrientation(Vec2f c) const
{
    switch (mOrientation)
    {
    case kOrientPortraitUp:
        return c;
    case kOrientPortraitDown:
        return Vec2f(mFBSize[0] - c[0], mFBSize[1] - c[1]);
    case kOrientLandscapeLeft:
        return Vec2f(c[1], mFBSize[0] - c[0]);
    case kOrientLandscapeRight:
        return Vec2f(mFBSize[1] - c[1], c[0]);
    default:
        break;
    }

    return c;
}

bool cApp::HandleGlobalKeys()
{
    bool handled = false;

    cObjectValue* prefs = HL()->mConfigManager->Preferences();

    if (mInputState->KeyWentDown('B', tModifiers(kModControl | kModShift)))
    {
        cValue& showBounds = prefs->InsertMember("showBounds");
        showBounds = !showBounds.AsBool();
    }

    if (mInputState->KeyWentDown('C', tModifiers(kModControl | kModShift)))
    {
        OpenLogConsole();
        handled = true;
    }

    if (mInputState->KeyWentDown('D', kModControl | kModShift))
        HL()->mRenderer->SetRenderFlag(kRenderFlagDebug, !HL()->mRenderer->RenderFlag(kRenderFlagDebug));

    if (mInputState->KeyWentDown('V'))
    {
        bool startRecording = !HL()->mAVManager->IsRecording();

        if (startRecording)
            HL()->mAVManager->StartRecording();
        else
            HL()->mAVManager->StopRecording();
    }

    if (mInputState->KeyWentDown('E', kModControl | kModShift))
    {
        mShowEffectsStats = !mShowEffectsStats;
    }
    if (mInputState->KeyWentDown('E'))
    {
        HL()->mConfigManager->OpenLastErrorFile();
        handled = true;
    }
    if (mInputState->KeyWentDown('T'))
    {
        HL()->mConfigManager->OpenConfig();
        handled = true;
    }
    if (mInputState->KeyWentDown('T', tModifiers(kModControl | kModShift)))
    {
        cValue& useTrackpad = prefs->InsertMember("useTrackpad");
        useTrackpad = !useTrackpad.AsBool();
    }
    if (mInputState->KeyWentDown('W', tModifiers(kModControl | kModShift)))
    {
        cValue& wireframe = prefs->InsertMember("wireframe");
        wireframe = !wireframe.AsBool();
    }
    if (mInputState->KeyWentDown('`'))
    {
        mAppKeyPause = !mAppKeyPause;

        if (mAppKeyPause)
            Pause();
        else
            Unpause();

        handled = true;
    }
    if (mInputState->KeyWentDown('=', kModControl | kModShift))
    {
        mDevMode = !mDevMode;
        handled = true;
    }

    return handled;
}


// Dev-only

#ifndef CL_RELEASE
bool cApp::BeginDevUI()
{
    mUIState->SetCellSize(Vec2f(20.0f));

    mUIState->BeginMenu(kDevMenuID);

    tUIItemID itemID = ItemID(0x00d500c4, 0);
    bool wasShowingDevMenu = mShowingDevMenu;

    if (mUIState->HandleButton(itemID++, "+", mShowingDevMenu ? 0 : kUIGhost))
        mShowingDevMenu = !mShowingDevMenu;

    if (!mShowingDevMenu)
    {
        mUIState->EndMenu(kDevMenuID);

        if (!wasShowingDevMenu)
            mUIState->MenuInfo(kDevMenuID)->mTopMaxSize = { 20.0f, 20.0f };

        return false;
    }

    auto hl = HL();

    mUIState->DrawSeparator();

#ifdef CL_IOS
    if (mUIState->InputState()->NumActivePointers() >= 2)
    {
        if (mUIState->HandleButton(itemID++, "Quit"))
            exit(1);

        if (mUIState->HandleButton(itemID++, "Crash"))
        {
            int* p = 0;
            *p = 0;
        }
    }
#endif

    if (mUIState->HandleToggle(itemID++, "Pause", &mAppKeyPause))
    {
        if (mAppKeyPause)
            Pause();
        else
            Unpause();
    }

    cObjectValue* prefs = hl->mConfigManager->Preferences();

    if (mUIState->BeginSubMenu(itemID++, "Render"))
    {
        bool wireframe = prefs->Member("wireframe").AsBool();

        if (mUIState->HandleToggle(itemID++, "Wireframe", &wireframe))
            prefs->InsertMember("wireframe") = wireframe;

        bool showBounds = prefs->InsertMember("showBounds").AsBool();

        if (mUIState->HandleToggle(itemID++, "Show Bounds", &showBounds))
            prefs->InsertMember("showBounds") = showBounds;

        if (hl->mRenderer)
            hl->mRenderer->DebugMenu(mUIState);

        mUIState->EndSubMenu();
    }

    if (hl->mEffectsManager && mUIState->BeginSubMenu(itemID++, "Effects"))
    {
        hl->mEffectsManager->DebugMenu(mUIState);

        mUIState->EndSubMenu();
    }

    if (mUIState->BeginSubMenu(itemID++, "UI"))
    {
    #ifndef HL_NO_FEEDBACK
        if (mUIState->HandleButton(itemID++, "Feedback"))
            ShellGatherUserFeedback("Generic");
    #endif

        if (mUIState->HandleButton(itemID++, "Web"))
            ShellOpenWebView("http://www.google.com");

        mUIState->HandleToggle(ItemID(0x01ae88bf), "Debug Input", &mDebugInput);

    #ifdef CL_OSX
        bool useTrackpad = prefs->Member("useTrackpad").AsBool();

        if (mUIState->HandleToggle(itemID++, "Use Mac Trackpad", &useTrackpad))
            prefs->InsertMember("useTrackpad") = useTrackpad;
    #endif

        mUIState->EndSubMenu();
    }

    if (hl->mAudioManager && mUIState->BeginSubMenu(itemID++, "Audio"))
    {
        hl->mAudioManager->DebugMenu(mUIState);

        mUIState->EndSubMenu();
    }

    auto config = hl->mConfigManager->Config();

    if (mUIState->InputState()->KeyModifiers() & kModShift)
        ShowObjectMenu(mUIState, itemID++, "Config", config, kObjectMenuLocalAndParents);
    else
        ShowObjectMenu(mUIState, itemID++, "Config", config, kObjectMenuNormal);

    if (!mAppModes.empty() && mUIState->BeginSubMenu(itemID++, "Modes"))
    {
        tUIItemID itemID = ItemID(0x00d500c5, 0);

        for (int i = 0, n = mAppModeTagToIndex.size(); i < n; i++)
        {
            const char* modeName  = StringFromTag(mAppModeTagToIndex.at(i).first);
            int         modeIndex = mAppModeTagToIndex.at(i).second;

            if (mUIState->HandleButton(itemID++, modeName, (modeIndex == mAppModeIndex) ? kUISelected : 0))
                SetAppMode(modeIndex);
        }

        mUIState->EndSubMenu();
    }

    return true;
}

void cApp::EndDevUI()
{
    if (mAppMode)
    {
        mUIState->DrawSeparator();

        mAppMode->DebugMenu(mUIState);
    }

    mUIState->EndMenu(kDevMenuID);
}

#endif

nHL::cIApp* nHL::CreateAppDefault()
{
    return new(Allocator(kDefaultAllocator)) cApp;
}
