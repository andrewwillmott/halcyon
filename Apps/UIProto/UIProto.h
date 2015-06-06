//
//  File:       UIProto.h
//
//  Function:   UI prototyping
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef UI_PROTO_APP_H
#define UI_PROTO_APP_H

#include <IHLApp.h>

#include <HLUI.h>

#include <CLBounds.h>
#include <CLLink.h>
#include <CLInputState.h>
#include <CLString.h>
#include <CLTimer.h>

namespace nHL
{
    class cSimpleCamera;
}

namespace nApp
{
    class cUIProtoMode :
        public nHL::cIAppMode,
        public nCL::cAllocLinkable
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
        // Utils
        void HandleTestMenu(nHL::cUIState* uiState);
        void HandleSquares (nHL::cUIState* uiState);
        void HandleSwipes  (nHL::cUIState* uiState);

        // Data
        nCL::cLink<nHL::cSimpleCamera> mCamera;
        
        // Test stuff
        bool mToggleOn = true;
        bool mSwatchOn = false;

        nCL::cBounds2 mRects[3] =
        {
            { { 200.0f,  20.0f }, { 280.0f, 100.0f } },
            { { 420.0f, 220.0f }, { 500.0f, 300.0f } },
            { { 320.0f, 120.0f }, { 400.0f, 200.0f } }
        };

        nCL::cColour mRectColours[3] =
        {
            nCL::kColourBlue,
            nCL::kColourGreen,
            nCL::kColourRed
        };

        nCL::string mSwipeText;
    };
}

#endif
