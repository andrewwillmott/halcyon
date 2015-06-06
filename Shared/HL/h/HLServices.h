//
//  File:       HLServices.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_SERVICES_H
#define HL_SERVICES_H

#include <CLLink.h>

namespace nHL
{
    class cIApp;
    class cIConfigManager;
    class cIRenderer;
    class cIModelManager;
    class cIEffectsManager;
    class cIAudioManager;
    class cIAVManager;

    class cDebugDraw;

    struct cServices
    {
        cIApp*              mApp;
        cIConfigManager*    mConfigManager;

        cIRenderer*         mRenderer;
        cIModelManager*     mModelManager;
        cIEffectsManager*   mEffectsManager;
        cIAudioManager*     mAudioManager;

        cDebugDraw*         mDebugDraw;
        cIAVManager*        mAVManager;

        cServices();
        ~cServices();
    };
}

const nHL::cServices* HL();
///< Accessor for global services.

nHL::cServices* HLServiceSetup();
///< Accessor for services for setup and teardown.



#endif
