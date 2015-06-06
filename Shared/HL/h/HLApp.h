//
//  File:       HLApp.h
//
//  Function:   Base implementation of cIApp.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_APP_H
#define HL_APP_H

#include <IHLApp.h>

#include <HLMain.h>
#include <HLUI.h>

#include <CLLink.h>
#include <CLMemory.h>
#include <CLTimer.h>

#include <VL234f.h>

namespace nCL
{
    class cInputState;
}

namespace nHL
{
    class cSystem;
    class cUIState;
    class cEventReceiver;

    const tUIItemID kDevMenuID = ItemID(0x00e6287b);

    class cApp :
        public cIApp,
        public nCL::cAllocLinkable
    {
    public:
        CL_ALLOC_LINK_DECL;

        cApp();
        ~cApp();
        
        bool Init() override;
        bool Shutdown() override;

        void Pause() override;
        void Unpause() override;
        bool IsPaused() const override;

        cUIState* UIState() const override;
        bool      DevMode() const override;

        // App modes
        void RegisterAppMode(nCL::tTag tag, cIAppMode* appMode) override;

        bool SetAppMode(int modeIndex) override;
        bool SetAppMode(nCL::tTag modeTag) override;

        cIAppMode*      AppMode() override;
        int             AppModeIndex() override;

        // Shell access
        void SetDevMode(bool enabled) override;
        void Update() override;
        void SetFrameBufferInfo(uint32_t id, int w, int h) override;
        void Render() override;

        void PointerDown (int index, float x, float y, int button) override;
        void PointerUp   (int index, float x, float y, int button) override;
        void PointerMove (int index, float x, float y) override;
        void PointersCancel() override;

        void KeyDown(int index, int key) override;
        void KeyUp  (int index, int key) override;
        void KeyModifiers(int index, uint32_t mods) override;

        void Acceleration(const float acc[3]) override;
        void Orientation (tDeviceOrientation orientation) override;
        void ScrollDelta (float x, float y) override;

        void TimeStamp(uint64_t timeMS) override;

        // cApp
        bool  HandleGlobalKeys();                       ///< Call to handle app-wide keys. Returns true if keys were claimed.

        bool  BeginDevUI();
        void  EndDevUI();

    protected:
        // Utils
        void  UpdateFBInfo();
        Vec2f AdjustForOrientation(Vec2f c) const;
        void  SetOrientedSize(Vec2f c);

        // Data
        nCL::cProgramTimer      mFrameTimer;
        float                   mMSPF           = 0.0f;
        int                     mPauseCount     = 0;
        float                   mTimeScale      = 1.0f;
        bool                    mDevMode        = true;
        bool                    mShowEffectsStats = true;

        uint32_t                mFBID           = 0;
        Vec2f                   mFBSize         = vl_0;
        tDeviceOrientation      mOrientation    = kOrientPortraitUp;

        nCL::cInputState*       mInputState     = 0;
        nHL::cUIState*          mUIState        = 0;
        
        cLink<cSystem>          mSystem;

        cEventReceiver*         mEventReceiver  = 0;

        bool                    mAppKeyPause = false;   ///< pause key toggle

        // App modes
        nCL::vector<cLink<cIAppMode>>   mAppModes;
        nCL::map<tTag, int>             mAppModeTagToIndex;

        cLink<cIAppMode>                mAppMode;
        int                             mAppModeIndex = -1;

    #ifndef CL_RELEASE
        bool mShowingDevMenu = false;
        bool mDebugInput = false;
    #endif
    };


    // --- Inlines -------------------------------------------------------------

    inline void cApp::Pause()
    {
        mPauseCount++;
    }

    inline void cApp::Unpause()
    {
        CL_ASSERT(mPauseCount > 0);
        mPauseCount--;
    }

    inline bool cApp::IsPaused() const
    {
        return mPauseCount > 0;
    }

    inline cUIState* cApp::UIState() const
    {
        return mUIState;
    }

    inline bool cApp::DevMode() const
    {
        return mDevMode;
    }

    inline cIAppMode* cApp::AppMode()
    {
        return mAppMode;
    }

    inline int cApp::AppModeIndex()
    {
        return mAppModeIndex;
    }

    inline void cApp::SetDevMode(bool enabled)
    {
        mDevMode = enabled;
    }
}

#endif
