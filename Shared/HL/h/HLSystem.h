//
//  File:       HLSystem.h
//
//  Function:   System for the Halcyon lib -- responsible for setting up and
//              updating various core engine services.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_SYSTEM_H
#define HL_SYSTEM_H

#include <HLDefs.h>
#include <CLLink.h>
#include <CLMemory.h>
#include <CLValue.h>

namespace nCL
{
    class cValue;
}

namespace nHL
{
    class cIConfigManager;
    class cIRenderer;
    class cIModelManager;
    class cIEffectsManager;
    class cDebugDraw;
    
    class cSystem :
        public nCL::cAllocLinkable
    {
    public:
        cSystem();
        ~cSystem();

        void SetProjectDir(const char* projectDir);
        ///< Sets project/app name

        bool Init();            ///< Create and Init all HL services
        bool PostInit();        ///< Configure all HL services
        bool Shutdown();        ///< Tear down HL services

        void Update(float dt, float gameDT);  ///< Tick all services

    protected:
        // Utils
        void UpdateFromConfig();

        // Data
    #ifndef CL_RELEASE
        const char*             mProjectDir = 0;
    #endif

        cLink<cIConfigManager>  mConfigManager = 0;

        cIRenderer*             mRenderer = 0;
        cLink<cIModelManager>   mModelManager;
        cLink<cIEffectsManager> mEffectsManager;
        cLink<cDebugDraw>       mDebugDraw;

        // Config support
        nCL::tConstObjectLink   mShaderDataConfig;
        uint32_t                mShaderDataModCount = 0;
        nCL::tConstObjectLink   mTexturesConfig;
        uint32_t                mTexturesModCount = 0;
        nCL::tConstObjectLink   mMaterialsConfig;
        uint32_t                mMaterialsModCount = 0;
        nCL::tConstObjectLink   mRenderConfig;
        uint32_t                mRenderModCount = 0;
        nCL::tConstObjectLink   mModelsConfig;
        uint32_t                mModelsModCount = 0;
        nCL::tConstObjectLink   mEffectsConfig;
        uint32_t                mEffectsModCount = 0;
        uint32_t                mAudioModCount = 0;
    };

    typedef cSystem cISystem;
    cISystem* CreateSystem(nCL::cIAllocator*);
}

#endif
