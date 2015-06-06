//
//  File:       SoundApp.cpp
//
//  Function:   Audio system test app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "SoundApp.h"

#include <HLMain.h>

#include <IHLAudioManager.h>
#include <IHLConfigManager.h>
#include <IHLEffectsManager.h>

#include <HLDebugDraw.h>
#include <HLServices.h>
#include <HLSystem.h>
#include <HLUI.h>

#include <CLLog.h>
#include <CLValue.h>

#include <math.h>
#include <stdlib.h>

using namespace nApp;
using namespace nHL;
using namespace nCL;

namespace
{
    const tUIItemID kMainMenuID = ItemID(0x00e61efe);

    const tTag kFixedTracks[4] =
    {
        CL_TAG("drums"),
        CL_TAG("synth"),
        CL_TAG("pad"),
        CL_TAG("top")
    };
}



//------------------------------------------------------------------------------
// cSoundProtoMode
//------------------------------------------------------------------------------

bool cSoundProtoMode::Init()
{
    const cServices* hl = HL();
    const cObjectValue* config = hl->mConfigManager->Config();
    cIAllocator* alloc = AllocatorFromObject(this);

    // ...
    auto am = hl->mAudioManager;
    mMasterPlay = am->MasterEnabled();

    int numTracks = am->NumChannels();
    mTrackEnable.resize(numTracks, true);
    mTrackGain  .resize(numTracks, 1.0f);
    mTrackPan   .resize(numTracks, 0.5f);
    mTrackEnable[2] = false;
    mTrackEnable[3] = false;

    for (int i = 0; i < CL_SIZE(kFixedTracks); i++)
    {
        int track = am->SoundRefFromTag(kFixedTracks[i]);
        am->PlayChannel(i, track);
    }

    mTriggerSamples[0] = am->SoundRefFromTag(CL_TAG("bubble"));
    mTriggerSamples[1] = am->SoundRefFromTag(CL_TAG("airlock"));

    SetMixerOptions();

    cUIMenuInfo* mainMenuInfo = HL()->mApp->UIState()->MenuInfo(kMainMenuID);
    mainMenuInfo->mPosition = { 20, 40 };

    return true;
}

bool cSoundProtoMode::Shutdown()
{
    return true;
}


void cSoundProtoMode::Update(float dt, float gameDT, nHL::cUIState* uiState)
{
    HandleUI(uiState);

    cDebugDraw* dd = HL()->mDebugDraw;

    dd->SetAlpha(1);
    dd->SetColour(kColourGreen);
    Vec2f ts = dd->TextSize("Sound App");
    Vec2f ss = dd->ScreenSize();
    DrawText(dd, (ss[0] - ts[0]) * 0.5f, 10, "Sound App");
}

void cSoundProtoMode::HandleUI(nHL::cUIState* uiState)
{
    uiState->BeginMenu(kMainMenuID);

    tUIItemID info = ItemID(0x00d5006b, 0);
    string menuText;

    cIAudioManager* manager = HL()->mAudioManager;

    if (uiState->HandleToggle(info++, "Master", &mMasterPlay))
        manager->SetMasterEnabled(mMasterPlay);

    if (uiState->HandleSlider(info++, "Master Gain", &mMasterGain))
        manager->SetMasterGain(mMasterGain);

    uiState->AdvanceCol();

    for (int i = 0, n = manager->NumChannels(); i < n; i++)
    {
        menuText.format("Track %d", i);

        if (i < CL_SIZE(kFixedTracks))
            menuText.append_format(" (%s)", kFixedTracks[i]);

        if (uiState->HandleToggle(info++, menuText.c_str(), &mTrackEnable[i]))
            manager->SetChannelEnabled(i, mTrackEnable[i]);

        if (uiState->HandleSlider(info++, "Level", &mTrackGain[i]))
            manager->SetChannelGain(i, mTrackGain[i]);

        if (uiState->HandleSlider(info++, "Pan", &mTrackPan[i]))
            manager->SetChannelPan(i, 2.0f * mTrackPan[i] - 1.0f);

        int playing = manager->ChannelPlaying(i);

        if (playing > 1)
            uiState->HandleButton(info++, menuText.format("Playing %d", playing).c_str());
        else if (playing > 0)
            uiState->HandleButton(info++, "Playing");
        else
            uiState->HandleButton(info++, "Silent");

        uiState->DrawSeparator();
    }

    uiState->AdvanceCol();

    for (int i = 0; i < CL_SIZE(mTriggerSamples); i++)
        if (uiState->HandleButton(info++, menuText.format("Sample %d", i).c_str()))
            manager->PlaySound(mTriggerSamples[i], kDefaultGroupRef);

    if (uiState->HandleButton(info++, "effect soundTest"))
    {
        auto em = HL()->mEffectsManager;
        em->StartSources(em->CreateOneShotInstance(CL_TAG("soundTest")));
    }
    if (uiState->HandleButton(info++, "effect multiSoundTest"))
    {
        auto em = HL()->mEffectsManager;
        em->StartSources(em->CreateOneShotInstance(CL_TAG("multiSoundTest")));
    }

    uiState->EndMenu(kMainMenuID);
}

void cSoundProtoMode::SetMixerOptions()
{
    cIAudioManager* manager = HL()->mAudioManager;

    for (int i = 0, n = mTrackEnable.size(); i < n; i++)
        manager->SetChannelEnabled(i, mTrackEnable[i]);

    for (int i = 0, n = mTrackGain.size(); i < n; i++)
        manager->SetChannelGain(i, mTrackGain[i]);

    manager->SetMasterGain(mMasterGain);
    manager->SetMasterEnabled(mMasterPlay);
}

bool nHL::SetupApp(cIApp* app)
{
    app->RegisterAppMode(CL_TAG("sound"), new (Allocator(kDefaultAllocator)) cSoundProtoMode);
    return true;
}
