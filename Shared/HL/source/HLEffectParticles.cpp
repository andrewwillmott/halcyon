//
//  File:       HLEffectParticles.cpp
//
//  Function:   Particle system management
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectParticles.h>

#include <IHLRenderer.h>

#include <HLDebugDraw.h>
#include <HLParticleUtils.h>

#include <ICLInterface.h>
#include <CLFrustum.h>
#include <CLParams.h>
#include <CLString.h>
#include <CLTimer.h>
#include <CLValue.h>
#include <CLVecUtil.h>

#include <GLConfig.h> // for vertex format constants

using namespace nHL;
using namespace nCL;

// --- cEffectTypeParticles ----------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    const tTag kParticlesLayerTag    = CL_TAG("particles");
    const tTag kParticlesMaterialTag = kParticlesLayerTag;

    cEltInfo kParticleQuadFormat[] =
    {
        { kVBPositions, 3, GL_FLOAT,        12, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };

    typedef cEffectType<cDescParticles, cEffectParticles> tEffectTypeParticles;

    class cEffectTypeParticles :
        public tEffectTypeParticles,
        public cIRenderLayer
    {
    public:
        CL_ALLOC_LINK_DECL;

        // cIEffectType
        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void PostInit() override;
        void Shutdown() override;

        const char* StatsString(const char* typeName) const override;
        void DebugMenu(cUIState* uiState) override;

        // cIRenderer
        void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;

        // cEffectTypeParticles
        void DispatchParticleSystem(const cEffectParticles* effect, cIRenderer* renderer, const cTransform& c2w);

        // Data
        tSeed32     mSeed = kDefaultSeed32;

    protected:
        // Data
        int         mParticleQuadMesh = -1;

        vector<int> mPass[2];
        int         mShaderRef[4] = { 0 };

        bool        mDispatchEnabled = true;
        float       mDispatchMSPF = 0.0f;
        int         mParticleDispatches = 0;
    };

    void cEffectTypeParticles::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeParticles::Init(manager, alloc);

        mParticleQuadMesh = HL()->mRenderer->CreateQuadMesh(4096, CL_SIZE(kParticleQuadFormat), kParticleQuadFormat);    ///< Create a mesh of the given vertex format to be used in quad rendering, and return slot, or 0 on failure.

        mRenderer->RegisterLayer(kParticlesLayerTag, this);
    }

    void cEffectTypeParticles::PostInit()
    {
        mShaderRef[0] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam1")); 
        mShaderRef[1] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam2"));
        mShaderRef[2] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam3"));
        mShaderRef[3] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam4"));
    }

    void cEffectTypeParticles::Shutdown()
    {
        mRenderer->RegisterLayer(kParticlesLayerTag, 0);

        if (mParticleQuadMesh >= 0)
        {
            HL()->mRenderer->DestroyQuadMesh(mParticleQuadMesh);
            mParticleQuadMesh = -1;
        }

        tEffectTypeParticles::Shutdown();
    }

    const char* cEffectTypeParticles::StatsString(const char* typeName) const
    {
        mStats.clear();

        int numInstances = mSlots.NumSlotsInUse();
        int numActiveInstances = 0;
        int totalParticles = 0;

        for (int i = 0, n = mEffects.size(); i < n; i++)
            if (mEffects[i])
            {
                totalParticles += mEffects[i]->NumParticles();

                if (mEffects[i]->IsActive())
                    numActiveInstances++;
            }

        if (totalParticles > 0)
            mStats.format("%d quads", totalParticles);

        if (numInstances > 0)
        {
            if (!mStats.empty())
                mStats.append(", ");

            mStats.append_format("%d/%d %s", numActiveInstances, numInstances, typeName);
        }

        bool showTime = mDispatchMSPF > 1.0f;

        if (mParticleDispatches || showTime)
        {
            if (!mStats.empty())
                mStats.append(", ");

            mStats.append_format("%d draws", mParticleDispatches);

            if (showTime)
                mStats.append_format(" @ %1.1f", mDispatchMSPF);
        }

        return mStats.empty() ? 0 : mStats.c_str();
    }

    void cEffectTypeParticles::DebugMenu(cUIState* uiState)
    {
        tEffectTypeParticles::DebugMenu(uiState);

        uiState->HandleToggle(ItemID(0x022c3477), "Dispatch", &mDispatchEnabled);
    }

    // cIRenderer
    struct cParticlesSortLess
    {
        cParticlesSortLess(const cEffectParticles* effects) : mEffects(effects) {}

        bool operator()(int a, int b) const
        {
            if (mEffects[a].mRenderOrder != mEffects[b].mRenderOrder)
                return mEffects[a].mRenderOrder > mEffects[b].mRenderOrder;

            return mEffects[a].mRenderHash < mEffects[b].mRenderHash;
        }

        const cEffectParticles* mEffects = 0;
    };

    void cEffectTypeParticles::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
    {
        mParticleDispatches = 0;

        if (!mDispatchEnabled || mSlots.NumSlotsInUse() == 0)
            return;

        cProgramTimer timer;
        timer.Start();

        cEffectsManagerParams* params = mManager->Params();
        const cTransform& c2w = params->mCameraToWorld;

        Vec4f planes[6];
        ExtractPlanes(params->mProjectionMatrix, planes);
    #ifndef CL_RELEASE
        OffsetPlanes(HL()->mConfigManager->Config()->Member(CL_TAG("clipPlanesOffset")).AsFloat(0.0f), planes);
    #endif

        mPass[0].clear();
        mPass[1].clear();

        uint32_t layerFlags = state.mLayerFlags;
        if (layerFlags == 0)
            layerFlags = 0x00FF;

        int layerBegin = ((layerFlags >> 8) & 0xFF) - 128;
        int layerEnd   = ((layerFlags >> 0) & 0xFF) - 128;

        for (int i = 0; i < mSlots.NumSlots(); i++)
        {
            if (!mSlots.InUse(i))
                continue;

            const cEffectParticles* effect = mEffects[i];

            if (!effect || !effect->IsActive())
                continue;

            if (effect->mParticles.Size() == 0)
                continue;

            if (effect->mDesc->mLayer < layerBegin || layerEnd <= effect->mDesc->mLayer)
                continue;

            if (FrustumTestAABB(effect->mBounds, planes) & kOutsideFrustum)
                continue;

            if (IsValid(effect->mMaterial1))
                mPass[0].push_back(i);

            if (effect->mMaterial2 > 0)    // just skip second pass if invalid material
                mPass[1].push_back(i);
        }

        for (int i = 0, n = mPass[0].size(); i < n; i++)
        {
            const cEffectParticles* effect = mEffects[mPass[0][i]];

            Mat4f modelToWorld;
            effect->mEffectToWorld.MakeMat4(&modelToWorld);
            renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, modelToWorld);

            if (renderer->SetMaterial(effect->mMaterial1))
                DispatchParticleSystem(effect, renderer, c2w);
        }

        for (int i = 0, n = mPass[1].size(); i < n; i++)
        {
            const cEffectParticles* effect = mEffects[mPass[1][i]];

            Mat4f modelToWorld;
            effect->mEffectToWorld.MakeMat4(&modelToWorld);
            renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, modelToWorld);

            if (renderer->SetMaterial(effect->mMaterial2))
                DispatchParticleSystem(effect, renderer, c2w);
        }

        UpdateMSPF(timer.GetTime(), &mDispatchMSPF);
    }

    void cEffectTypeParticles::DispatchParticleSystem(const cEffectParticles* effect, cIRenderer* renderer, const cTransform& c2w)
    {
        const tStandardParticles& particles = effect->mParticles;

        if (effect->mTexture1 >= 0)
            renderer->SetTexture(kTextureDiffuseMap, effect->mTexture1);

        if (effect->mTexture2 >= 0)
            renderer->SetTexture(kTextureNormalMap, effect->mTexture2);

        Mat4f modelToWorld;
        effect->mEffectToWorld.MakeMat4(&modelToWorld);
        renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, modelToWorld);

        if (effect->mDispatchParams)
        {
            renderer->PushState("effectParams");

            for (int i = 0; i < CL_SIZE(mShaderRef); i++)
                if (effect->mDispatchParams->HasParam(kEffectParamShader1))
                    renderer->SetShaderDataT(mShaderRef[i], effect->mDispatchParams->Param(kEffectParamShader1, Vec4f(vl_0)));
        }

        uint8_t* frames;
        size_t frameStride;
        uint8_t frameStart;

        if (particles.mFrames)
        {
            frames = particles.mFrames;
            frameStride = sizeof(particles.mFrames[0]);
        }
        else
        {
            frameStart = effect->mDesc->mCreate.mFrameStart;
            frames = &frameStart;
            frameStride = 0;
        }

        DispatchParticles
        (
            renderer,
            mParticleQuadMesh,

            effect->mDesc->mDispatch,
            effect->mSourceToEffect,
            effect->mEffectToWorld,
            c2w,
            &effect->mDispatchScale,

            particles.Size(),

            particles.mAge, particles.mAgeStep, sizeof(particles.mAge[0]),
            particles.mPosition, sizeof(particles.mPosition[0]),
            particles.mVelocity, sizeof(particles.mVelocity[0]),

            particles.mColour,   sizeof(particles.mColour  [0]) * particles.mAlloc.mColour,
            particles.mAlpha,    sizeof(particles.mAlpha   [0]) * particles.mAlloc.mAlpha,
            particles.mSize,     sizeof(particles.mSize    [0]) * particles.mAlloc.mSize,
            particles.mRotation, sizeof(particles.mRotation[0]) * particles.mAlloc.mRotation,
            particles.mAspect,   sizeof(particles.mAspect  [0]) * particles.mAlloc.mAspect,
            frames, frameStride
        );

        if (effect->mDispatchParams)
            renderer->PopState("effectParams");

        mParticleDispatches++;
    }
}

namespace
{
    struct cCreateParticlesData :
        public cIEffectData,
        public nCL::cAllocLinkable
    {
        int    mCount      = 0;
        Vec3f* mPositions  = 0;
        Vec3f* mVelocities = 0;

        enum tLocalIID : uint32_t { kIID = 0x01ff93c3 };

        void* AsInterface(uint32_t iid) { if (iid == kIID) return this; return 0; }
        int Link(int count) const override { return nCL::cAllocLinkable::Link(count); }

        cCreateParticlesData() = default;
        
        ~cCreateParticlesData()
        {
            cIAllocator* alloc = AllocatorFromObject(this);
            alloc->Free((void*) mPositions);
            alloc->Free((void*) mVelocities);
        };
    };

}

cIEffectData* nHL::CreateParticlesEffectData(int numParticles, const Vec3f positions[], const Vec3f velocities[])
{
    cIAllocator* alloc = Allocator(kGraphicsAllocator);

// TODO - this uses global allocator, not cAllocatable    cCreateParticlesData* data = Create<cCreateParticlesData>(alloc);
    cCreateParticlesData* data = new(alloc) cCreateParticlesData;

    data->mCount = numParticles;
    data->mPositions = CreateArray<Vec3f>(alloc, numParticles);
    copy_n(positions, numParticles, data->mPositions);

    if (velocities)
    {
        data->mVelocities = CreateArray<Vec3f>(alloc, numParticles);
        copy_n(velocities, numParticles, data->mVelocities);
    }

    return data;
}

namespace nHL
{
    cIEffectType* CreateEffectTypeParticles(cIAllocator* alloc)
    {
        return new(alloc) cEffectTypeParticles;
    }
}

// --- cDescParticles ----------------------------------------------------------

void cDescParticles::Config(const nCL::cValue& v, cIEffectType* type, cIEffectsManager* manager)
{
    mCreate  .Config(v);
    mDispatch.Config(v);
    mPhysics .Config(v);

    const cValue&      layerV = v["layer"];
    const cValue& depthLayerV = v["depthLayer"];

    if (layerV.IsInt())
    {
        mLayer = layerV.AsInt();
        mDepth = 0;
    }
    if (depthLayerV.IsInt())
    {
        mLayer = depthLayerV.AsInt();
        mDepth = 1;
    }

    mControllerTag = v["controller"].AsTag();

    // Derived data
    mDispatch.mAlignDir = mCreate.mEmitDir.Centre();
}


// --- cEffectParticles --------------------------------------------------------

bool cEffectParticles::Init(cIEffectType* effectType)
{
    CL_ASSERT(!mTypeManager);

    mTypeManager = static_cast<cEffectTypeParticles*>(effectType);

    return true;
}

bool cEffectParticles::Shutdown()
{
    // mTypeManager = 0;

    SetDescription(0);

    return true;
}

void cEffectParticles::SetDescription(const cDescParticles* desc)
{
    if (mDesc)
    {
        mController = 0;
        mMaterial1 = -1;
        mMaterial2 = -1;
        mTexture1 = -1;
        mTexture2 = -1;
    }

    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mEffectActive = false;
        return;
    }

    mState.Setup(mDesc->mCreate);

    cIRenderer* renderer = mTypeManager->Renderer();

    if (mDesc->mDispatch.mMaterialTag != 0)
        mMaterial1 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterialTag);
    else
        mMaterial1 = renderer->MaterialRefFromTag(kParticlesMaterialTag);

    if (mDesc->mDispatch.mMaterial2Tag != 0)
        mMaterial2 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterial2Tag);

    if (mDesc->mDispatch.mTextureTag != 0)
        mTexture1 = renderer->TextureRefFromTag(mDesc->mDispatch.mTextureTag);
    if (mDesc->mDispatch.mTexture2Tag != 0)
        mTexture2 = renderer->TextureRefFromTag(mDesc->mDispatch.mTexture2Tag);

    if (mDesc->mControllerTag != 0)
        mController = mTypeManager->Manager()->PhysicsController(mDesc->mControllerTag);

#if 0
    // XXX TODO
    mParticles.mAlloc.mColour   = !mDesc->mDispatch.mColourFrames.empty();
    mParticles.mAlloc.mAlpha    = !mDesc->mDispatch.mAlphaFrames .empty();

    mParticles.mAlloc.mSize     = !mDesc->mDispatch.mSizeFrames.empty();
    mParticles.mAlloc.mRotation = !mDesc->mDispatch.mRotateFrames.empty();
    mParticles.mAlloc.mAspect   = !mDesc->mDispatch.mAspectFrames.empty();
#else
    mParticles.mAlloc.mColour   = true;
    mParticles.mAlloc.mAlpha    = true;

    mParticles.mAlloc.mSize     = true;
    mParticles.mAlloc.mRotation = true;
    mParticles.mAlloc.mAspect   = true;
#endif

    mParticles.mAlloc.mFrames   = mDesc->mCreate.mFrameRandom != 0;

    if (mParticles.Size() != 0)
        AllocNewArrays(&mParticles);

    // TODO: these are all small integers, may be better to just xor across 4 bytes?
    mRenderHash = CRC32((uint8_t*) &mMaterial1, sizeof(mMaterial1));
    mRenderHash = CRC32((uint8_t*) &mMaterial2, sizeof(mMaterial1), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture1 , sizeof(mTexture1 ), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture2 , sizeof(mTexture2 ), mRenderHash);

    mRenderOrder &= 0x00FFFFFF;
    mRenderOrder |= (127 - mDesc->mLayer) << 24;
}

void cEffectParticles::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
    CL_ASSERT(!HasNAN(sourceXform.mTranslation));
    CL_ASSERT(!HasNAN(effectXform.mTranslation));

    mSourceToEffect = sourceXform;
    mEffectToWorld = effectXform;

    if (mDesc->mDepth)
    {
        Vec3f representativePosition = mSourceToEffect.Trans(); // TODO: track active bounding box instead
        Vec3f worldPos = mEffectToWorld.TransformPoint(representativePosition);
        float depth = sqrlen(worldPos - mTypeManager->Manager()->Params()->mCameraToWorld.Trans());

        mRenderOrder &= 0xFF000000;
        mRenderOrder |= OrderedU32(depth) >> 8;
    }
}

void cEffectParticles::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mFlags.mSourceActive = true;
    mFlags.mEffectActive = true;
    mFlags.mLoopsActive  = true;

    mState.Setup(mDesc->mCreate);
    mFlags.mPreroll = (transition == kTransitionImmediate);
}

void cEffectParticles::Stop(tTransitionType transition)
{
    mFlags.mSourceActive = false;
    mFlags.mLoopsActive  = false;

    if (transition == kTransitionImmediate)
    {
        mParticles.Clear();
        mFlags.mEffectActive = false;
    }
}

void cEffectParticles::Update(float dt, const cEffectParams* params)
{
    if (!mFlags.mEffectActive)
        return;

    if (mFlags.mPreroll)
    {
        dt += MaxElt(mDesc->mCreate.mLife);
        mFlags.mPreroll = false;
    }

    float animScale = params->Param(kEffectParamAnimSpeed, 1.0f);

    if (mDesc->mCreate.mFlags.mLoopParticles && mFlags.mLoopsActive)
        UpdateAgesWrap(dt * animScale, mParticles.Size(), mParticles.mAge, mParticles.mAgeStep, sizeof(tPtAge), mParticles.mAge);
    else
        UpdateAges(dt * animScale, mParticles.Size(), mParticles.mAge, mParticles.mAgeStep, sizeof(tPtAge), mParticles.mAge);

    CompactParticles(&mParticles);

    UpdatePhysicsSimple
    (
        mDesc->mPhysics,
        mParticles.Size(),
        &dt, 0,
        mParticles.mPosition, sizeof(mParticles.mPosition[0]),
        mParticles.mVelocity, sizeof(mParticles.mVelocity[0])
    );

    if (mController)
        mController->Update
        (
            mEffectToWorld,
            mParticles.Size(),
            &dt, 0,
            mParticles.mPosition, sizeof(mParticles.mPosition[0]),
            mParticles.mVelocity, sizeof(mParticles.mVelocity[0])
        );

    mBounds.MakeEmpty();

    if (mFlags.mSourceActive)
        CreateParticles(dt, animScale, params);
    else if (mParticles.Size() == 0)    // done?
    {
        mFlags.mEffectActive = false;
        return;
    }

    for (int i = 0, n = mParticles.Size(); i < n; i++)
        mBounds.Add(mParticles.mPosition[i]);
    mBounds = mEffectToWorld.TransformBounds(mBounds);

    if (mParamsModCount != params->ParamsModCount())
    {
        mDispatchScale.mColour = params->Param(kEffectParamColour, Vec3f(vl_1));
        mDispatchScale.mAlpha  = params->Param(kEffectParamAlpha,  1.0f);
        mDispatchScale.mSize   = params->Param(kEffectParamSize,   1.0f);

        if (params->HasParam(kEffectParamShader1)
         || params->HasParam(kEffectParamShader2)
         || params->HasParam(kEffectParamShader3)
         || params->HasParam(kEffectParamShader4)
        )
        {
            mDispatchParams = params;
        }

        mParamsModCount = params->ParamsModCount();
    }

#ifndef CL_RELEASE
    if (mTypeManager->Manager()->Params()->mFlags.mDebugBoundingBoxes)
    {
        auto dd = HL()->mDebugDraw;

        dd->Reset();
        dd->SetColour(kColourOrange);

//        dd->ClearTransform3D();
        DrawBox(dd, mBounds.mMin, mBounds.mMax);
    }
#endif
}

// Internal

int cEffectParticles::CreateParticles(float dt, float animScale, const cEffectParams* params)
{
    float timeAlive[256];
    int totalCount = 0;

    cParticlesCreateScale createScale;

    createScale.Setup(mDesc->mCreate, mSourceToEffect);
    if (params->HasParams())
        SetupCreateParams(params, &createScale);

    tSeed32* seed = &mTypeManager->mSeed;

    do
    {
        int createCount = 0;

        switch (mDesc->mCreate.mFlags.mMode)
        {
        case kEmitModeRate:
            createCount = CreateRateParticles(dt, mDesc->mCreate, createScale, &mState, CL_SIZE(timeAlive), timeAlive);
            break;
        case kEmitModeInject:
            createCount = CreateInjectParticles(dt, mDesc->mCreate, createScale, &mState, CL_SIZE(timeAlive), timeAlive);
            break;
        case kEmitModeMaintain:
            createCount = CreateMaintainParticles(dt, mDesc->mCreate, createScale, &mState, mParticles.Size(), CL_SIZE(timeAlive), timeAlive);
            break;
        }

        dt = 0.0f;

        if (createCount <= 0)
        {
            if (createCount < 0)
                mFlags.mSourceActive = false;   // but don't disable loops.

            break;
        }

        // for now ignore preroll
        int oldCount = mParticles.Size();

        mParticles.Resize(oldCount + createCount, mTypeManager->Allocator());

        CreateParticleBasics
        (
            mDesc->mCreate,
            createScale,
            seed,
            oldCount,
            createCount,
            mParticles.mAgeStep,
            mParticles.mPosition,
            mParticles.mVelocity,
            params->ParamData<cBounds3>(kEffectParamEmitVolume)
        );

        CreateParticleAttributes
        (
            mDesc->mCreate,
            createScale,
            seed,
            oldCount,
            createCount,
            mParticles.mSize,
            mParticles.mAlpha,
            mParticles.mColour,
            mParticles.mRotation,
            mParticles.mAspect
        );

        if (mParticles.mFrames)
            CreateParticleFrames
            (
                mDesc->mCreate,
                seed,
                oldCount,
                createCount,
                mParticles.mFrames
            );

        mSourceToEffect.TransformPoints    (createCount, mParticles.mPosition + oldCount);
        mSourceToEffect.TransformDirections(createCount, mParticles.mVelocity + oldCount);   // purposefully exclude scale  -- we're post-scaling the emission volume, not the particles per se

        totalCount += createCount;

        // Would do emit offset and local surface interaction here.

        // Sim particles for the rest of this timestep
        if (mDesc->mCreate.mFlags.mLoopParticles && mFlags.mLoopsActive)
            UpdateAgesWrap
            (
                createCount,
                animScale,
                timeAlive, sizeof(timeAlive[0]),
                mParticles.mAge + oldCount, mParticles.mAgeStep + oldCount, sizeof(tPtAge),
                mParticles.mAge + oldCount
            );
        else
            UpdateAges
            (
                createCount,
                animScale,
                timeAlive, sizeof(timeAlive[0]),
                mParticles.mAge + oldCount, mParticles.mAgeStep + oldCount, sizeof(tPtAge),
                mParticles.mAge + oldCount
            );

        UpdatePhysicsSimple
        (
            mDesc->mPhysics,
            createCount,
            timeAlive, sizeof(float),
            mParticles.mPosition + oldCount, sizeof(mParticles.mPosition[0]),
            mParticles.mVelocity + oldCount, sizeof(mParticles.mVelocity[0])
        );

        if (mController)
            mController->Update
            (
                mEffectToWorld,
                createCount,
                timeAlive, sizeof(float),
                mParticles.mPosition + oldCount, sizeof(mParticles.mPosition[0]),
                mParticles.mVelocity + oldCount, sizeof(mParticles.mVelocity[0])
            );
    }
    while (mState.mParticlesToCreate >= 1.0f);

    for (int i = 0, n = params->NumData(); i < n; i++)
    {
        const cCreateParticlesData* createData = AsInterface<cCreateParticlesData>(params->Data(i));

        if (!createData)
            continue;

        int oldCount    = mParticles.Size();
        int createCount = createData->mCount;
        int newCount    = oldCount + createCount;

        mParticles.Resize(newCount, mTypeManager->Allocator());

        copy_n(createData->mPositions, createCount, mParticles.mPosition + oldCount);

        if (createData->mVelocities)
            copy_n(createData->mVelocities, createCount, mParticles.mVelocity + oldCount);
        else
            fill_n(mParticles.mVelocity + oldCount, createCount, vl_0);

        if (mParticles.mAgeStep)
            for (int i = oldCount; i < newCount; i++)
                mParticles.mAgeStep[i] = LifeToAgeStep(RandomRange(mDesc->mCreate.mLife, seed));

        CreateParticleAttributes
        (
            mDesc->mCreate,
            createScale,
            seed,
            oldCount, createData->mCount,
            mParticles.mSize,
            mParticles.mAlpha,
            mParticles.mColour,
            mParticles.mRotation,
            mParticles.mAspect
        );

        if (mParticles.mFrames)
            CreateParticleFrames
            (
                mDesc->mCreate,
                seed,
                oldCount, createCount,
                mParticles.mFrames
            );

//        cTransform worldToEffect = mEffectToWorld.Inverse();
//        worldToEffect.TransformPoints    (createCount, mParticles.mPosition + oldCount);
//        worldToEffect.TransformDirections(createCount, mParticles.mVelocity + oldCount);   // purposefully exclude scale  -- we're post-scaling the emission volume, not the particles per se

        totalCount += createCount;
    }

    return totalCount;
}




// --- Utilities ---------------------------------------------------------------

void nHL::SetupCreateParams(const nCL::cParams* params, cParticlesCreateScale* createScale)
{
    CL_ASSERT(params->HasParams());

    createScale->mRate   = params->Param(kEffectParamEmitRate,   1.0f);
    createScale->mSpeed  = params->Param(kEffectParamEmitSpeed,  1.0f);
    createScale->mSize   = params->Param(kEffectParamEmitSize,   1.0f);
    createScale->mColour = params->Param(kEffectParamEmitColour, Vec3f(vl_1));
    createScale->mAlpha  = params->Param(kEffectParamEmitAlpha,  1.0f);
}


// --- cStandardParticleArrays ---------------------------------------------------------------

void nHL::CopyArray(int count, const cStandardParticles& a, cStandardParticles* b, int aStart, int bStart)
{
    using nHL::CopyArray;

    CopyArray(count, a.mPosition , b->mPosition, aStart, bStart);
    CopyArray(count, a.mVelocity , b->mVelocity, aStart, bStart);

    if (a.mColour)
        CopyArray(count, a.mColour, b->mColour, aStart, bStart);
    if (a.mAlpha)
        CopyArray(count, a.mAlpha , b->mAlpha , aStart, bStart);

    if (a.mSize)
        CopyArray(count, a.mSize,     b->mSize,     aStart, bStart);
    if (a.mRotation)
        CopyArray(count, a.mRotation, b->mRotation, aStart, bStart);
    if (a.mAspect)
        CopyArray(count, a.mAspect,   b->mAspect,   aStart, bStart);
    if (a.mFrames)
        CopyArray(count, a.mFrames,   b->mFrames,   aStart, bStart);

    CopyArray(count, a.mAge     , b->mAge    , aStart, bStart);
    CopyArray(count, a.mAgeStep , b->mAgeStep, aStart, bStart);
}

void nHL::InitArray(int count, cStandardParticles* p, int start)
{
    using nHL::InitArray;

    InitArray(count, p->mPosition , start);
    InitArray(count, p->mVelocity , start);

    if (p->mColour)
        InitArray(count, p->mColour   , start, Vec3f(vl_1));
    if (p->mAlpha)
        InitArray(count, p->mAlpha    , start, 1.0f);

    if (p->mSize)
        InitArray(count, p->mSize    , start, 1.0f);
    if (p->mRotation)
        InitArray(count, p->mRotation, start, 1.0f);
    if (p->mAspect)
        InitArray(count, p->mAspect  , start, 1.0f);

    if (p->mFrames)
        InitArray(count, p->mFrames  , start, uint8_t(0));

    InitArray(count, p->mAge,     start);
    InitArray(count, p->mAgeStep, start);
}

void nHL::AllocArray(cIAllocator* alloc, int count, cStandardParticles* p)
{
    AllocArray(alloc, count, &p->mPosition );
    AllocArray(alloc, count, &p->mVelocity );

    if (p->mAlloc.mColour)
        AllocArray(alloc, count, &p->mColour);
    if (p->mAlloc.mAlpha)
        AllocArray(alloc, count, &p->mAlpha);

    if (p->mAlloc.mSize)
        AllocArray(alloc, count, &p->mSize);
    if (p->mAlloc.mRotation)
        AllocArray(alloc, count, &p->mRotation);
    if (p->mAlloc.mAspect)
        AllocArray(alloc, count, &p->mAspect);

    if (p->mAlloc.mFrames)
        AllocArray(alloc, count, &p->mFrames);

    AllocArray(alloc, count, &p->mAge);
    AllocArray(alloc, count, &p->mAgeStep);
}

void nHL::FreeArray(cIAllocator* alloc, cStandardParticles* p)
{
    using nHL::FreeArray;

    FreeArray(alloc, &p->mPosition );
    FreeArray(alloc, &p->mVelocity );
    FreeArray(alloc, &p->mColour   );
    FreeArray(alloc, &p->mAlpha    );
    FreeArray(alloc, &p->mSize     );
    FreeArray(alloc, &p->mRotation );
    FreeArray(alloc, &p->mAspect   );
    FreeArray(alloc, &p->mFrames   );
    FreeArray(alloc, &p->mAge      );
    FreeArray(alloc, &p->mAgeStep  );
}


void nHL::CompactParticles(tStandardParticles* p)
{
    // TODO: look for runs and call CopyArray with count > 1
    while (p->mCount > 0 && (p->mAge[p->mCount - 1] >= kPtAgeExpired))
        p->mCount--;

    for (int i = 0; i < p->mCount - 1; i++)
    {
        if (IsExpired(p->mAge[i]))
        {
            CopyArray(1, *p, p, p->mCount - 1, i);

            do
            {
                p->mCount--;
            }
            while (p->mCount > 0 && (p->mAge[p->mCount - 1] >= kPtAgeExpired));
        }
    }
}

#ifdef TODO
void nHL::CompactParticles(cStandardParticleArrays* p, int start, int end)
{
    while (p->mCount > 0 && (p->mFlags[p->mCount - 1] & kFluidFlagUnused))
        p->mCount--;

    for (int i = start; i < end && i < p->mCount - 1; i++)
    {
        if (p->mFlags[i] & kFluidFlagUnused)
        {
            CopyArray(1, *p, p, p->mCount - 1, i);
            p->mCount--;
        }
    }
}
#endif

void nHL::AllocNewArrays(tStandardParticles* p)
{
    if (p->mAlloc.mColour && !p->mColour)
        AllocArray(p->mAllocator, p->mAllocated, &p->mColour);
    if (p->mAlloc.mAlpha && !p->mColour)
        AllocArray(p->mAllocator, p->mAllocated, &p->mAlpha);

    if (p->mAlloc.mSize && !p->mSize)
        AllocArray(p->mAllocator, p->mAllocated, &p->mSize);
    if (p->mAlloc.mRotation && !p->mRotation)
        AllocArray(p->mAllocator, p->mAllocated, &p->mRotation);
    if (p->mAlloc.mAspect && !p->mAspect)
        AllocArray(p->mAllocator, p->mAllocated, &p->mAspect);

    if (p->mAlloc.mFrames && !p->mFrames)
        AllocArray(p->mAllocator, p->mAllocated, &p->mFrames);
}
