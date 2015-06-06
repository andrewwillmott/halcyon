//
//  File:       Demo.h
//
//  Function:   Demo app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef DEMO_H
#define DEMO_H

#include <IHLApp.h>

#include <CLLink.h>
#include <CLMemory.h>

namespace nHL
{
    class cSimpleCamera;
}

namespace nApp
{
    class cDemoMode :
        public nHL::cIAppMode,
        public nCL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        // cIAppMode
        bool PreInit() override { return true; }
        bool Init() override;
        bool Shutdown() override;

        void UpdateFromConfig(const nCL::cObjectValue* config) override;

        void Activate() override;
        void Deactivate() override;

        void Update(float dt, float gameDT, nHL::cUIState* uiState) override;
        void HandleKeys(nCL::cInputState* inputState) override;
        void DebugMenu(nHL::cUIState* uiState) override;

    protected:
        nHL::cLink<nHL::cSimpleCamera> mCamera;
    };
}

#endif
