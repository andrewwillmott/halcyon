//
//  File:       HLSystem.cpp
//
//  Function:   Central system for the Halcyon lib
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLSystem.h>

#include <IHLAudioManager.h>
#include <IHLAVManager.h>
#include <IHLConfigManager.h>
#include <IHLEffectsManager.h>
#include <IHLModelManager.h>
#include <IHLRenderer.h>

#include <HLDebugDraw.h>
#include <HLServices.h>

#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLImage.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLSystemInfo.h>

#ifdef HL_LIB_UV
    #include "../../../External/LibUV/uv.h"
#endif

using namespace nHL;
using namespace nCL;

namespace
{

}

cSystem::cSystem()
{
}

cSystem::~cSystem()
{
}


#ifndef CL_RELEASE
void cSystem::SetProjectDir(const char* projectDir)
{
    mProjectDir = projectDir;
}
#endif

bool cSystem::Init()
{
    InitLogSystem();    // Do this immediately, otherwise logger will be auto-created with options we don't want.

#ifndef CL_RELEASE
    cFileSpec devDataSpec;

    bool foundDevDir = false;

    if (mProjectDir)
    {
        devDataSpec.SetDirectory(mProjectDir);
        devDataSpec.AddDirectory("Data");

        if (devDataSpec.DirectoryIsReadable())
        {
            CL_LOG("HLSystem", "Switching to using dev data at %s\n", devDataSpec.Directory());
            RegisterDirectory(kDirectoryData, devDataSpec);
            foundDevDir = true;
        }

    }

#ifdef CL_IOS
    if (!foundDevDir)
    {
        SetDirectory(&devDataSpec, kDirectoryDocuments);
        devDataSpec.AddDirectory("Data");

        if (devDataSpec.DirectoryIsReadable())
        {
            devDataSpec.SetNameAndExtension("ignore.txt");

            if (!devDataSpec.Exists())
            {
                devDataSpec.SetNameAndExtension(0);
                
                CL_LOG("HLSystem", "Switching to using dev data at %s\n", devDataSpec.Directory());
                RegisterDirectory(kDirectoryData, devDataSpec);
                // foundDevDir = true;
            }
        }
    }
#else
    if (!foundDevDir)
    {
        // Look upwards in app path for 'override' Resources dir
        SetDirectory(&devDataSpec, kDirectoryApp);

        // Expecting to be in <appName>.app/Contents/MacOS
        bool ok =  devDataSpec.RemoveDirectory()
                && devDataSpec.RemoveDirectory()
                && devDataSpec.RemoveDirectory();
        
        if (ok)
        {
            do
            {
                devDataSpec.AddDirectory("Data");
                
                if (devDataSpec.DirectoryIsReadable())
                {
                    CL_LOG("HLSystem", "Switching to using dev data at %s\n", devDataSpec.Directory());
                    RegisterDirectory(kDirectoryData, devDataSpec);
                    foundDevDir = true;
                    break;
                }
                
                devDataSpec.RemoveDirectory();
            }
            while (devDataSpec.RemoveDirectory());
        }
    }
#endif
#endif
        
    cServices* services = HLServiceSetup();
    cIAllocator* alloc = AllocatorFromObject(this);

    InitTagSystem(alloc);

    mConfigManager = CreateConfigManager(alloc);
    mConfigManager->Init();

    services->mConfigManager = mConfigManager;

    //////////

#ifndef CL_RELEASE
    if (mConfigManager->Config()->Member("showDirectories").AsBool())
    {
        CL_LOG("Shell", "App directory is at %s\n",       Directory(kDirectoryApp));
        CL_LOG("Shell", "Data directory is at %s\n",      Directory(kDirectoryData));
        CL_LOG("Shell", "Resources directory is at %s\n", Directory(kDirectoryResources));
        CL_LOG("Shell", "Documents directory is at %s\n", Directory(kDirectoryDocuments));
    }

    if (mConfigManager->Config()->Member("showInfo").AsBool())
    {
        printf("SYSTEM INFO\n");
        printf("===========\n");
        GetSystemInfo();
        printf("\n");

        // printf("GL INFO\n");
        // printf("=======\n");
        // GetGLLimits();
        // printf("\n");
    }
#endif

    //////////

    InitImageSystem();

    mRenderer = CreateRenderer(alloc);
    mRenderer->Init();

    services->mRenderer = mRenderer;

    mDebugDraw = CreateDebugDraw(alloc);
    mDebugDraw->Init();
    mRenderer->RegisterLayer(kRenderLayerDebugDraw, mDebugDraw);

    mModelManager = CreateModelManager(alloc);
    mModelManager->Init();
    mRenderer->RegisterLayer(kRenderLayerModelManager, mModelManager->AsLayer());

    services->mAudioManager = CreateAudioManager(alloc);
    if (services->mAudioManager)
        services->mAudioManager->Init();

    // Effects manager systems are dependent on core services above.
    mEffectsManager = CreateEffectsManager(alloc);
    mEffectsManager->Init();

    services->mModelManager   = mModelManager;
    services->mEffectsManager = mEffectsManager;
    services->mDebugDraw      = mDebugDraw;

    services->mAVManager = CreateAVManager(alloc);
    if (services->mAVManager)
        services->mAVManager->Init();

    return true;
}

bool cSystem::PostInit()
{
    UpdateFromConfig();

    mDebugDraw->PostInit();
    mEffectsManager->PostInit();

    return true;
}

bool cSystem::Shutdown()
{
#ifdef HL_LIB_UV
    uv_stop(uv_default_loop());
#endif
    
    cServices* services = HLServiceSetup();

    if (services->mAVManager)
    {
        services->mAVManager->Shutdown();
        DestroyAVManager(services->mAVManager);
        services->mAVManager = 0;
    }

    services->mDebugDraw = 0;
    services->mModelManager = 0;

    mEffectsManager->Shutdown();
    mEffectsManager = 0;

    if (services->mAudioManager)
    {
        services->mAudioManager->Shutdown();
        // services->mAudioManager->Link(-1);
        services->mAudioManager = 0;
    }

    mModelManager->Shutdown();
    mModelManager = 0;

    mDebugDraw->Shutdown();
    mDebugDraw = 0;

    services->mRenderer = 0;
    mRenderer->Shutdown();
    DestroyRenderer(mRenderer);
    mRenderer = 0;

    ShutdownImageSystem();

    services->mConfigManager = 0;
    mConfigManager->Shutdown();
    mConfigManager = 0;

    ShutdownLogSystem();

    ShutdownTagSystem();

    return true;
}

void cSystem::Update(float dt, float gameDT)
{
#ifndef CL_RELEASE
    mDebugDraw->Clear();    // Prepare for a new tick.
#endif


#ifdef HL_LIB_UV
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
#endif
    
    mConfigManager->Update();

    if (mConfigManager->ConfigModified())
        UpdateFromConfig();

    if (HL()->mAudioManager)
        HL()->mAudioManager->Update(dt);

    mModelManager->Update(dt);

    Vec2f orientedSize = mRenderer->ShaderDataT<Vec2f>(kDataIDOrientedViewSize);
    cICamera* camera = mRenderer->Camera(kMainTag);

    if (camera)
    {
        auto params = mEffectsManager->Params();
        params->mCameraToWorld = camera->CameraToWorld();
        camera->FindViewProjection(orientedSize, &params->mProjectionMatrix);
    }
    mEffectsManager->Update(dt, gameDT);

    mRenderer->Update(dt);

#ifndef CL_RELEASE
    mDebugDraw->SetScreenSize(orientedSize);

    if (camera)
    {
        Mat4f worldToClip;
        camera->FindViewProjection(mDebugDraw->ScreenSize(), &worldToClip);
        mDebugDraw->SetWorldToClip(worldToClip);
    }
#endif
}

void cSystem::UpdateFromConfig()
{
#ifdef CL_IOS
    const char* platformTag = CL_HIDDEN_VALUE_TAG("iOS");
#elif defined(CL_OSX)
    const char* platformTag = CL_HIDDEN_VALUE_TAG("OSX");
#else
    const char* platformTag = CL_HIDDEN_VALUE_TAG("unknown");
#endif

    const cObjectValue* config = mConfigManager->Config();

    const cObjectValue* shaderDataConfig  = config->Member("shaderData").AsObject();
    const cObjectValue* texturesConfig    = config->Member("textures").AsObject();
    const cObjectValue* materialsConfig   = config->Member("materials").AsObject();
    const cObjectValue* renderConfig      = config->Member("render").AsObject();
    const cObjectValue* modelsConfig      = config->Member("models").AsObject();
    const cObjectValue* effectsConfig     = config->Member("effects").AsObject();
    const cObjectValue* audioConfig       = config->Member("sounds").AsObject();

    CL_LOG_I("Memory", "Before config load: %4.1f MB\n", UsedMemoryInMB());
    if (shaderDataConfig && (mShaderDataConfig != shaderDataConfig || mShaderDataModCount != shaderDataConfig->ModCount()))
    {
        mShaderDataConfig = shaderDataConfig;
        mShaderDataModCount = mShaderDataConfig->ModCount();
        mRenderer->LoadShaderData(shaderDataConfig);
    }

    if (texturesConfig && (mTexturesConfig != texturesConfig || mTexturesModCount != texturesConfig->ModCount()))
    {
        mTexturesConfig = texturesConfig;
        mTexturesModCount = mTexturesConfig->ModCount();
        mRenderer->LoadTextures(texturesConfig);
        CL_LOG_I("Memory", "After textures load: %4.1f MB\n", UsedMemoryInMB());
    }

    if (materialsConfig && (mMaterialsConfig != materialsConfig || mMaterialsModCount != materialsConfig->ModCount()))
    {
        mMaterialsConfig = materialsConfig;
        mMaterialsModCount = mMaterialsConfig->ModCount();
        mRenderer->LoadMaterials(materialsConfig);
        mRenderer->LoadMaterials(materialsConfig->Member(platformTag).AsObject());
        CL_LOG_I("Memory", "After materials load: %4.1f MB\n", UsedMemoryInMB());
    }

    if (renderConfig && (mRenderConfig != renderConfig || mRenderModCount != renderConfig->ModCount()))
    {
        mRenderConfig = renderConfig;
        mRenderModCount = mRenderConfig->ModCount();
        mRenderer->LoadLayersAndBuffers(renderConfig);
        CL_LOG_I("Memory", "After layers/buffers load: %4.1f MB\n", UsedMemoryInMB());
    }

    if (modelsConfig && (mModelsConfig != modelsConfig || mModelsModCount != modelsConfig->ModCount()))
    {
        mModelsConfig = modelsConfig;
        mModelsModCount = mModelsConfig->ModCount();
        mModelManager->LoadModels(modelsConfig);
        CL_LOG_I("Memory", "After models load: %4.1f MB\n", UsedMemoryInMB());
    }

    if (effectsConfig && (mEffectsConfig != effectsConfig || mEffectsModCount != effectsConfig->ModCount()))
    {
        mEffectsConfig = effectsConfig;
        mEffectsModCount = mEffectsConfig->ModCount();
        mEffectsManager->LoadEffects(effectsConfig);
        CL_LOG_I("Memory", "After effects load: %4.1f MB\n", UsedMemoryInMB());
    }

    if (HL()->mAudioManager && audioConfig && (mAudioModCount != audioConfig->ModCount()))
    {
        mAudioModCount = audioConfig->ModCount();
        HL()->mAudioManager->LoadSounds(audioConfig);
        CL_LOG_I("Memory", "After audio load: %4.1f MB\n", UsedMemoryInMB());
    }
}

cISystem* nHL::CreateSystem(nCL::cIAllocator* alloc)
{
    return new(alloc) cSystem;
}
