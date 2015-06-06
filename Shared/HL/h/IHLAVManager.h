//
//  File:       IHLAVManager.h
//
//  Function:   Audio/visual capture/record support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_AV_MANAGER_H
#define HL_I_AV_MANAGER_H

#include <HLDefs.h>

namespace nCL
{
    class cIAllocator;
}

namespace nHL
{
    class cIAVManager
    {
    public:
        virtual bool Init() = 0;
        virtual bool Shutdown() = 0;

        virtual void StartRecording() = 0;
        virtual void StopRecording() = 0;
        virtual bool IsRecording() const = 0;

        virtual void CapturePicture() = 0;

        virtual void SetCaptureName(const char* name) = 0;

        // OSX only, for OS-level window capture
        virtual void SetWindowBounds(const float rect[4]) = 0;
    };

    cIAVManager* CreateAVManager (nCL::cIAllocator* alloc);
    void         DestroyAVManager(cIAVManager* manager);
}

#endif
