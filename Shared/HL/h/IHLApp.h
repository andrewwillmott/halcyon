//
//  File:       IHLApp.h
//
//  Function:   Interface by which platform-specific shell drives app
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_APP_H
#define HL_I_APP_H

#include <HLDefs.h>

namespace nCL
{
    class cObjectValue;
    class cInputState;
}

namespace nHL
{
    class cUIState;

    enum tDeviceOrientation : int
    {
        kOrientPortraitUp,
        kOrientPortraitDown,
        kOrientLandscapeLeft,
        kOrientLandscapeRight,
        kMaxDeviceOrientation
    };

    class cIAppMode
    {
    public:
        virtual int Link(int count) const = 0;    ///< Add/remove given reference links. Returns final link count. Call with 0 to get current count.

        virtual bool PreInit() = 0;             ///< Called early enough to register system entities
        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual void UpdateFromConfig(const nCL::cObjectValue* config) = 0;     ///< Called when any config is updated.

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;

        virtual void Update(float dt, float gameDT, cUIState* uiState) = 0;
        virtual void HandleKeys(nCL::cInputState* inputState) = 0;

        virtual void DebugMenu(cUIState* uiState) = 0;
    };

    class cIApp
    {
    public:
        virtual int Link(int count) const = 0;    ///< Add/remove given reference links. Returns final link count. Call with 0 to get current count.

        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual void Pause   () = 0;        ///< Pause app.
        virtual void Unpause () = 0;        ///< Unpause app. Pauses and unpauses are cumulative; pause, pause, unpause will leave the app still paused, a further unpause will set it running again.
        virtual bool IsPaused() const = 0;  ///< Return current pause state.

        virtual cUIState* UIState() const = 0;    ///< Main UI state object
        virtual bool      DevMode() const = 0;      ///< Returns true if app is in dev mode.

        // App modes
        virtual void RegisterAppMode(nCL::tTag tag, nHL::cIAppMode* appMode) = 0;

        virtual bool SetAppMode(int modeIndex) = 0;
        virtual bool SetAppMode(nCL::tTag modeTag) = 0;

        virtual nHL::cIAppMode* AppMode() = 0;
        virtual int             AppModeIndex() = 0;


        // For internal use by top-level shell code...
        virtual void SetDevMode(bool enabled) = 0;
        virtual void Update() = 0;                                          ///< Top-level main thread update
        virtual void SetFrameBufferInfo(uint32_t id, int w, int h) = 0;     ///< Communicate frame buffer binding
        virtual void Render() = 0;

        // Pointer manipulation
        virtual void PointerDown (int index, float x, float y, int button) = 0; ///< Given pointer button went down
        virtual void PointerUp   (int index, float x, float y, int button) = 0; ///< Given pointer button went up
        virtual void PointerMove (int index, float x, float y) = 0;             ///< Given pointer moved
        virtual void PointersCancel() = 0;  ///< Reset all pointers

        // Keyboard(s)
        virtual void KeyDown     (int index, int keyCode) = 0;          ///< Given key went down on given keyboard
        virtual void KeyUp       (int index, int keyCode) = 0;          ///< Given key went up on given keyboard
        virtual void KeyModifiers(int index, uint32_t modifiers) = 0;   ///< Modifiers changed on given keyboard to given values

        // Device movement
        virtual void Acceleration(const float[3]) = 0;  ///< Device acceleration
        virtual void Orientation (tDeviceOrientation orientation) = 0;  ///< Device orientation
        virtual void ScrollDelta (float x, float y) = 0; ///< Deltas from scroll devices

        // Time stamping
        virtual void TimeStamp(uint64_t elapsedTimeInMs) = 0;   ///< Stamp following events with this time.
    };

    cIApp* CreateApp();
}

#endif
