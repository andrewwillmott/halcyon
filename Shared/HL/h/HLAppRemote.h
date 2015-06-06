//
//  File:       HLAppRemote.h
//
//  Function:   Provides remote UI event support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_APP_REMOTE_H
#define HL_APP_REMOTE_H

#include <CLDefs.h>

namespace nHL
{
    class cIApp;
    
    class cEventBroadcaster
    /// Broadcasts event callbacks to listeners
    {
    public:
        bool Init();
        bool Shutdown();

        // Pointer manipulation
        void PointerDown (int index, float x, float y, int button); ///< Given pointer button went down
        void PointerUp   (int index, float x, float y, int button); ///< Given pointer button went up
        void PointerMove (int index, float x, float y);             ///< Given pointer moved
        void PointersCancel();  ///< Reset all pointers

        // Keyboard(s)
        void KeyDown     (int index, int keyCode);          ///< Given key went down on given keyboard
        void KeyUp       (int index, int keyCode);          ///< Given key went up on given keyboard
        void KeyModifiers(int index, uint32_t modifiers);   ///< Modifiers changed on given keyboard to given values

        // Device movement
        void Acceleration(const float[3]);  ///< Device acceleration

        // Time stamping
        void TimeStamp(uint64_t elapsedTimeInMs);   ///< Stamp following events with this time.

    protected:
        bool SendMessage(size_t dataSize, const void* data);

        uint8_t mChannel        = 0;
        int     mUDPSocket      = -1;
        uint8_t mTargetAddr[16] = { 0 };
    };

    class cEventReceiver
    /// Receives event broadcasts and passes them to the target app.
    {
    public:
        bool Init();
        bool Shutdown();

        void SetTarget(cIApp* app);

        void Update();  ///< Call if using syncronous version

    protected:
        void Listen();
        void ProcessMessage(const uint8_t* data, size_t dataSize);

        int     mUDPSocket  = -1;
        cIApp*  mApp        = 0;
    };
}

#endif
