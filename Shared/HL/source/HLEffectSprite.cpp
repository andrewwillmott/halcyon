//
//  File:       HLEffectSprite.cpp
//
//  Function:   Sprite support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectSprite.h>

#include <IHLRenderer.h>

#include <HLParticleUtils.h>

#include <CLHash.h>
#include <CLParams.h>
#include <CLString.h>
#include <CLTimer.h>
#include <CLValue.h>
#include <CLVecUtil.h>

#include <GLConfig.h> // for vertex format constants

using namespace nHL;
using namespace nCL;


namespace
{
    const tTag kSpritesLayerTag    = CL_TAG("sprites");
    const tTag kSpritesMaterialTag = CL_TAG("particles");    // just re-use particle material for now

    cEltInfo kSpriteQuadFormat[] =
    {
        { kVBPositions, 3, GL_FLOAT,        12, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };
}

// --- cEffectTypeSprites ----------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    typedef cEffectTypeValue<cDescSprite, cEffectSprite> tEffectTypeSprites;

    class cEffectTypeSprites :
        public tEffectTypeSprites,
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

        // cEffectTypeSprites
        void DispatchSprites(const cEffectSprite* effect, cIRenderer* renderer, const cTransform& c2w);

        // Data
        tSeed32     mSeed = kDefaultSeed32;

    protected:
        // Data
        int         mParticleQuadMesh = -1;

        vector<int> mPass[2];       ///< For sorting sprites

        int         mShaderRef[4] = { 0 };

        float       mDispatchMSPF = 0.0f;
        bool        mDispatchEnabled = true;
    };

    void cEffectTypeSprites::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeSprites::Init(manager, alloc);

        mParticleQuadMesh = HL()->mRenderer->CreateQuadMesh(4096, CL_SIZE(kSpriteQuadFormat), kSpriteQuadFormat);    ///< Create a mesh of the given vertex format to be used in quad rendering, and return slot, or 0 on failure.

        mRenderer->RegisterLayer(kSpritesLayerTag, this);
    }

    void cEffectTypeSprites::PostInit()
    {
        mShaderRef[0] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam1")); 
        mShaderRef[1] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam2"));
        mShaderRef[2] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam3"));
        mShaderRef[3] = mRenderer->ShaderDataRefFromTag(CL_TAG("effectParam4"));
    }

    void cEffectTypeSprites::Shutdown()
    {
        mRenderer->RegisterLayer(kSpritesLayerTag, 0);

        if (mParticleQuadMesh >= 0)
        {
            HL()->mRenderer->DestroyQuadMesh(mParticleQuadMesh);
            mParticleQuadMesh = -1;
        }

        tEffectTypeSprites::Shutdown();
    }

    const char* cEffectTypeSprites::StatsString(const char* typeName) const
    {
        mStats.clear();

        int numInstances = mSlots.NumSlotsInUse();
        int numActiveInstances = 0;

        for (int i = 0, n = mEffects.size(); i < n; i++)
            if (mEffects[i].IsActive())
                numActiveInstances++;

        if (numInstances > 0)
        {
            if (!mStats.empty())
                mStats.append(", ");

            mStats.append_format("%d/%d %s", numActiveInstances, numInstances, typeName);
        }

        if (mDispatchMSPF > 1.0f)
        {
            if (!mStats.empty())
                mStats.append(", ");

            mStats.append_format("%1.1f draw", mDispatchMSPF);
        }

        return mStats.empty() ? 0 : mStats.c_str();
    }

    void cEffectTypeSprites::DebugMenu(cUIState* uiState)
    {
        tEffectTypeSprites::DebugMenu(uiState);

        uiState->HandleToggle(ItemID(0x022c3477), "Dispatch", &mDispatchEnabled);
    }


    // cIRenderLayer

    struct cSpriteSortLess
    {
        cSpriteSortLess(const cEffectSprite* effects) : mEffects(effects) {}

        bool operator()(int a, int b) const
        {
            if (mEffects[a].mRenderOrder != mEffects[b].mRenderOrder)
                return mEffects[a].mRenderOrder > mEffects[b].mRenderOrder;

            return mEffects[a].mRenderHash < mEffects[b].mRenderHash;
        }

        const cEffectSprite* mEffects = 0;
    };

    void cEffectTypeSprites::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
    {
        if (!mDispatchEnabled)
            return;

        cProgramTimer timer;
        timer.Start();

        const cTransform& c2w = mManager->Params()->mCameraToWorld;

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

            const cEffectSprite& effect = mEffects[i];

            if (!effect.IsActive())
                continue;

            if (effect.mDesc->mLayer < layerBegin || layerEnd <= effect.mDesc->mLayer)
                continue;

            if (IsValid(effect.mMaterial1))
                mPass[0].push_back(i);

            if (effect.mMaterial2 > 0)    // just skip second pass if invalid material
                mPass[1].push_back(i);
        }

        sort(mPass[0].begin(), mPass[0].end(), cSpriteSortLess(mEffects.data()));
        sort(mPass[1].begin(), mPass[1].end(), cSpriteSortLess(mEffects.data()));

        for (int i = 0, n = mPass[0].size(); i < n; i++)
        {
            const cEffectSprite& effect = mEffects[mPass[0][i]];

            Mat4f modelToWorld;
            effect.mEffectToWorld.MakeMat4(&modelToWorld);
            renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, modelToWorld);

            if (renderer->SetMaterial(effect.mMaterial1))
                DispatchSprites(&effect, renderer, c2w);
        }

        for (int i = 0, n = mPass[1].size(); i < n; i++)
        {
            const cEffectSprite& effect = mEffects[mPass[1][i]];

            Mat4f modelToWorld;
            effect.mEffectToWorld.MakeMat4(&modelToWorld);
            renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, modelToWorld);

            if (renderer->SetMaterial(effect.mMaterial2))
                DispatchSprites(&effect, renderer, c2w);
        }

        UpdateMSPF(timer.GetTime(), &mDispatchMSPF);
    }

    void cEffectTypeSprites::DispatchSprites(const cEffectSprite* effect, cIRenderer* renderer, const cTransform& c2w)
    {
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

        DispatchParticles
        (
            renderer,
            mParticleQuadMesh,

            effect->mDesc->mDispatch,
            effect->mSourceToEffect,
            effect->mEffectToWorld,
            c2w,
            &effect->mDispatchScale,

            1,

            &effect->mAge, &effect->mAgeStep, 0,
            &effect->mPosition, 0,
            &effect->mVelocity, 0,

            &effect->mColour,   0,
            &effect->mAlpha,    0,
            &effect->mSize,     0,
            &effect->mRotation, 0,
            &effect->mAspect,   0,
            &effect->mFrame,    0
        );

        if (effect->mDispatchParams)
            renderer->PopState("effectParams");
    }
}

namespace nHL
{
    cIEffectType* CreateEffectTypeSprite(cIAllocator* alloc)
    {
        return new(alloc) cEffectTypeSprites;
    }
}

// --- cDescSprite ----------------------------------------------------------

void cDescSprite::Config(const nCL::cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mDispatch.Config(config);
    mPhysics .Config(config);

    mLife = AsRange(config.Member("life"), mLife);

    const cValue& emitDirV = config.Member("emitDir");

    if (!emitDirV.IsNull())
    {
        mEmitDir.MakePoint(norm_safe(AsVec3(emitDirV)));

        Vec3f emitSpread = AsVec3(config.Member("emitSpread"));
        mEmitDir.Inflate(emitSpread);
    }

    mEmitSpeed = AsRange(config.Member("emitSpeed"), mEmitSpeed);

    const cValue* v;
    Vec3f* baseColour = 0;

    if (!(v = &config["colourVary"])->IsNull())
    {
        mColourVary = AsRangedVec3(*v, 0.0f, 1.0f, mColourVary);
    }
    else if (baseColour && !(v = &config["colourVaryTo"])->IsNull())
    {
        Vec3f cs = *baseColour;
        Vec3f cd = AsRangedVec3(*v, 0.0f, 1.0f, mColourVary);

        Vec3f sum = (cd + cs);

        mColourVary = (cd - cs) / sum;
        *baseColour = 0.5f * sum;
    }

    mAlphaVary    = AsUnitFloat(config["alphaVary"],  mAlphaVary);

    mSizeVary     = AsUnitFloat(config["sizeVary"],   mSizeVary);
    mRotateVary   = AsUnitFloat(config["rotateVary"], mRotateVary);
    mAspectVary   = AsUnitFloat(config["aspectVary"], mAspectVary);

    const cValue& animFramesV = config.Member("animFrames");

    if (animFramesV.IsObject())
    {
        mFrameStart  = animFramesV.Member("start") .AsInt(mFrameStart);
        mFrameCount  = animFramesV.Member("count") .AsInt(mFrameCount);
        mFrameRandom = animFramesV.Member("random").AsInt(mFrameRandom);
    }

    const cValue&      layerV = config["layer"];
    const cValue& depthLayerV = config["depthLayer"];

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

    mControllerTag = config["controller"].AsTag();

    // Derived data
    mDispatch.mAlignDir = mEmitDir.Centre();
}


// --- cEffectSprite --------------------------------------------------------

bool cEffectSprite::Init(cIEffectType* effectType)
{
    CL_ASSERT(!mTypeManager);

    mTypeManager = static_cast<cEffectTypeSprites*>(effectType);

    return true;
}

bool cEffectSprite::Shutdown()
{
    // mTypeManager = 0;

    SetDescription(0);

    return true;
}

void cEffectSprite::SetDescription(const cDescSprite* desc)
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

    cIRenderer* renderer = mTypeManager->Renderer();

    if (mDesc->mDispatch.mMaterialTag != 0)
        mMaterial1 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterialTag);
    else
        mMaterial1 = renderer->MaterialRefFromTag(kSpritesMaterialTag);

    if (mDesc->mDispatch.mMaterial2Tag != 0)
        mMaterial2 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterial2Tag);

    if (mDesc->mDispatch.mTextureTag != 0)
        mTexture1 = renderer->TextureRefFromTag(mDesc->mDispatch.mTextureTag);
    if (mDesc->mDispatch.mTexture2Tag != 0)
        mTexture2 = renderer->TextureRefFromTag(mDesc->mDispatch.mTexture2Tag);

    if (mDesc->mControllerTag != 0)
        mController = mTypeManager->Manager()->PhysicsController(mDesc->mControllerTag);

    // TODO: these are all small integers, may be better to just xor across 4 bytes?
    mRenderHash = CRC32((uint8_t*) &mMaterial1, sizeof(mMaterial1));
    mRenderHash = CRC32((uint8_t*) &mMaterial2, sizeof(mMaterial1), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture1 , sizeof(mTexture1 ), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture2 , sizeof(mTexture2 ), mRenderHash);

    mRenderOrder &= 0x00FFFFFF;
    mRenderOrder |= (127 - mDesc->mLayer) << 24;
}

void cEffectSprite::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
    CL_ASSERT(!HasNAN(sourceXform.mTranslation));
    CL_ASSERT(!HasNAN(effectXform.mTranslation));

    mSourceToEffect = sourceXform;
    mEffectToWorld = effectXform;

    if (mDesc->mDepth)
    {
        Vec3f worldPos = mEffectToWorld.TransformPoint(mPosition);
        float depth = sqrlen(worldPos - mTypeManager->Manager()->Params()->mCameraToWorld.Trans());

        mRenderOrder &= 0xFF000000;
        mRenderOrder |= OrderedU32(depth) >> 8;
    }
}

void cEffectSprite::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mFlags.mSourceActive = false;
    mFlags.mEffectActive = true;
}

void cEffectSprite::Stop(tTransitionType transition)
{
    mFlags.mSourceActive = false;
    mFlags.mEffectActive = false;
}

void cEffectSprite::Update(float dt, const nCL::cParams* params)
{
    if (!mFlags.mEffectActive)
        return;

    float animScale = params->Param(kEffectParamAnimSpeed, 1.0f);

    if (!mFlags.mSourceActive)
    {
        CreateSprite(animScale, params);
        mFlags.mSourceActive = true;
    }

    UpdateAgesWrap(dt * animScale, 1, &mAge, &mAgeStep, 0, &mAge);  // TODO

    UpdatePhysicsSimple
    (
        mDesc->mPhysics,
        1,
        &dt, 0,
        &mPosition, 0,
        &mVelocity, 0
    );

#ifdef TODO
    if (mController)
        mController->Update
        (
            mEffectToWorld,
            mParticles.Size(),
            &dt, 0,
            mParticles.mPosition, sizeof(mParticles.mPosition[0]),
            mParticles.mVelocity, sizeof(mParticles.mVelocity[0])
        );
#endif

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
}

// Internal

void cEffectSprite::CreateSprite(float animScale, const nCL::cParams* params)
{
    cParticlesCreateScale createScale;

// TODO    createScale.Setup(mDesc->mCreate, mSourceToEffect);
    if (params && params->HasParams())
        SetupCreateParams(params, &createScale);

    tSeed32* seed = &mTypeManager->mSeed;

    mAge = 0;
    mAgeStep = LifeToAgeStep(RandomRange(mDesc->mLife, seed));

    mVelocity = RandomRange(mDesc->mEmitDir, seed);

    float speed = RandomRange(mDesc->mEmitSpeed, seed) * createScale.mSpeed;
    mVelocity *= speed * InvSqrtFast(sqrlen(mVelocity));

    // TODO: flatten & deal with VaryRGB
    cParticlesCreateDesc createDesc;

    createDesc.mSizeVary     = mDesc->mSizeVary  ;
    createDesc.mRotateVary   = mDesc->mRotateVary;
    createDesc.mColourVary   = mDesc->mColourVary;
    createDesc.mAlphaVary    = mDesc->mAlphaVary ;
    createDesc.mAspectVary   = mDesc->mAspectVary;

    CreateParticleAttributes
    (
        createDesc,
        createScale,
        seed,
        0,
        1,
        &mSize,
        &mAlpha,
        &mColour,
        &mRotation,
        &mAspect
    );

    mPosition = mSourceToEffect.Trans();
    mVelocity = mSourceToEffect.TransformVector(mVelocity);

    if (mDesc->mFrameRandom > 0)
    {
        if (mDesc->mFrameCount > 0)
            mFrame = mDesc->mFrameStart + RandomUInt32(mDesc->mFrameRandom, seed) * mDesc->mFrameCount;
        else
            mFrame = mDesc->mFrameStart + RandomUInt32(mDesc->mFrameRandom, seed);
    }
    else
        mFrame = mDesc->mFrameStart;
}
