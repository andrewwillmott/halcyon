//
//  File:       Viewer.h
//
//  Function:   Viewer prototype for rendering and UI tests
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef VIEWER_H
#define VIEWER_H

#include <IHLApp.h>
#include <IHLModelManager.h>
#include <IHLEffectsManager.h>
#include <IHLRenderer.h>

#include <CLSTL.h>
#include <CLValue.h>

namespace nHL
{
    class cSimpleCamera;
}

namespace nApp
{
    class cViewerMode :
        public nHL::cIAppMode,
        public nHL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;
        
        cViewerMode();
        ~cViewerMode();

        // cIAppMode
        bool PreInit() override;             ///< Called early enough to register system entities
        bool Init() override;
        bool Shutdown() override;

        void UpdateFromConfig(const nCL::cObjectValue* config) override;     ///< Called when any config is updated.

        void Activate() override;
        void Deactivate() override;

        void Update(float dt, float gameDT, nHL::cUIState* uiState) override;
        void HandleKeys(nCL::cInputState* inputState) override;

        void DebugMenu(nHL::cUIState* uiState) override;

    protected:
        // Utils
        void LoadModels (const nCL::cValue& config);
        void LoadEffects(const nCL::cValue& config);

        // Data
        nCL::cLink<nHL::cSimpleCamera> mCamera;

        // Models
        nCL::vector<nHL::tMIRef> mModels;
        nCL::vector<float>       mRotationSpeeds;

        // Effects
        nCL::vector<nHL::tEIRef> mEffects;
        int mCurrentEffect = 0;
        int mCurrentVolume = 0;
    };
}

#endif
