//
//  File:       EventBroadcastApp.h
//
//  Function:   Broadcast events from touch device
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  
//

#ifndef EVENT_BROADCAST_APP_H
#define EVENT_BROADCAST_APP_H

#include <HLApp.h>

namespace nHL
{
    class cEventBroadcaster;

    // For this app we actually override the default cApp implementation, so we
    // can hook the incoming UI events.

    class cEBApp : public nHL::cApp
    {
    public:
        // cIApp
        bool Init() override;
        bool Shutdown() override;

        void Update() override;

        void PointerDown (int index, float x, float y, int button) override;
        void PointerUp   (int index, float x, float y, int button) override;
        void PointerMove (int index, float x, float y) override;
        void PointersCancel() override;

        void KeyDown(int index, int key) override;
        void KeyUp  (int index, int key) override;
        void KeyModifiers(int index, uint32_t mods) override;

        void Acceleration(const float acc[3]) override;

        void TimeStamp(uint64_t timeMS) override;

        // cEBApp
        void EBUpdate();

    protected:
        cEventBroadcaster* mEventBroadcaster = 0;
    };
}

#endif
