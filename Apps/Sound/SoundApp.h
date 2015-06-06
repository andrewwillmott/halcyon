//
//  File:       SoundApp.h
//
//  Function:   Audio system test app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef SOUND_APP_H
#define SOUND_APP_H

#include <IHLApp.h>
#include <CLMemory.h>

namespace nApp
{
    class cSoundProtoMode :
        public nHL::cIAppMode,
        public nHL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        // cIAppMode
        bool PreInit() override { return true; }
        bool Init() override;
        bool Shutdown() override;

        void UpdateFromConfig(const nCL::cObjectValue* config) override {}

        void Activate  () override {}
        void Deactivate() override {}

        void Update(float dt, float gameDT, nHL::cUIState* uiState) override;
        void HandleKeys(nCL::cInputState* inputState) override {}

        void DebugMenu(nHL::cUIState* uiState) override {}

    protected:
        void HandleUI(nHL::cUIState* uiState);
        void SetMixerOptions();
        
        bool  mMasterPlay = false;
        float mMasterGain = 0.5f;

        nCL::vector<bool>  mTrackEnable;
        nCL::vector<float> mTrackGain;
        nCL::vector<float> mTrackPan;

        int mTriggerSamples[2] = { 0 };
    };
}

#endif
