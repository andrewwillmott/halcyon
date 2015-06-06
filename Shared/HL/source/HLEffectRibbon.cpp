//
//  File:       HLEffectRibbon.cpp
//
//  Function:   Ribbon effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectRibbon.h>

#include <IHLRenderer.h>

#include <CLColour.h>
#include <CLValue.h>
#include <CLMemory.h>
#include <CLTimer.h>

#include <GLConfig.h> // for vertex format constants

using namespace nHL;

namespace
{

}

// --- cEffectTypeRibbon ------------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    const tTag kLayerTag    = CL_TAG("ribbons");
    const tTag kMaterialTag = CL_TAG("particles");

    cEltInfo kQuadFormat[] =
    {
        { kVBPositions, 3, GL_FLOAT,        12, false },
        { kVBTexCoords, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };

    typedef cEffectType<cDescRibbon, cEffectRibbon> tEffectTypeRibbon;

    class cEffectTypeRibbon :
        public tEffectTypeRibbon,
        public cIRenderLayer
    {
    public:
        CL_ALLOC_LINK_DECL;

        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void Shutdown() override;
        void PreUpdate (float realDT, float gameDT) override;
        void PostUpdate(float realDT, float gameDT) override;

        const char* StatsString(const char* typeName) const override;
        void DebugMenu(cUIState* uiState) override;

        // cIRenderer
        void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;

        // cEffectTypeRibbon
        void DispatchRibbon(const cEffectRibbon* effect, cIRenderer* renderer, const cTransform& c2w);

    protected:
        int         mQuadMesh = -1;

        vector<int> mPass[2];
        int         mShaderRef[4] = { 0 };

        float       mDispatchMSPF = 0.0f;
        bool        mDispatchEnabled = true;
    };

    void cEffectTypeRibbon::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeRibbon::Init(manager, alloc);

        mQuadMesh = HL()->mRenderer->CreateQuadMesh(4096, CL_SIZE(kQuadFormat), kQuadFormat);

        mRenderer->RegisterLayer(kLayerTag, this);
    }

    void cEffectTypeRibbon::Shutdown()
    {
        mRenderer->RegisterLayer(kLayerTag, 0);

        if (mQuadMesh >= 0)
        {
            HL()->mRenderer->DestroyQuadMesh(mQuadMesh);
            mQuadMesh = -1;
        }

        tEffectTypeRibbon::Shutdown();
    }

    void cEffectTypeRibbon::PreUpdate(float realDT, float gameDT)
    {
    }
    void cEffectTypeRibbon::PostUpdate(float realDT, float gameDT)
    {
    }

    const char* cEffectTypeRibbon::StatsString(const char* typeName) const
    {
        mStats.clear();

        int numInstances = mSlots.NumSlotsInUse();
        int numActiveInstances = 0;
        int numQuads = 0;

        for (int i = 0, n = mEffects.size(); i < n; i++)
            if (mEffects[i])
            {
//                numQuads += mEffects[i]->NumParticles();

                if (mEffects[i]->IsActive())
                    numActiveInstances++;
            }

        if (numQuads > 0)
            mStats.format("%d quads", numQuads);

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

    void cEffectTypeRibbon::DebugMenu(cUIState* uiState)
    {
        tEffectTypeRibbon::DebugMenu(uiState);

        uiState->HandleToggle(ItemID(0x022c3477), "Dispatch", &mDispatchEnabled);
    }

    // cIRenderer
    struct cRibbonsSortLess
    {
        cRibbonsSortLess(const cEffectRibbon* effects) : mEffects(effects) {}

        bool operator()(int a, int b) const
        {
            if (mEffects[a].mRenderOrder != mEffects[b].mRenderOrder)
                return mEffects[a].mRenderOrder > mEffects[b].mRenderOrder;

            return mEffects[a].mRenderHash < mEffects[b].mRenderHash;
        }

        const cEffectRibbon* mEffects = 0;
    };

    void cEffectTypeRibbon::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
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

            const cEffectRibbon* effect = mEffects[i];

            if (!effect || !effect->IsActive())
                continue;

            if (effect->mPositions.empty())
                continue;

            if (effect->mDesc->mLayer < layerBegin || layerEnd <= effect->mDesc->mLayer)
                continue;

            if (IsValid(effect->mMaterial1))
                mPass[0].push_back(i);

            if (effect->mMaterial2 > 0)    // just skip second pass if invalid material
                mPass[1].push_back(i);
        }

        renderer->SetShaderDataT<Mat4f>(kDataIDModelToWorld, Mat4f(vl_I));

        for (int i = 0, n = mPass[0].size(); i < n; i++)
        {
            const cEffectRibbon* effect = mEffects[mPass[0][i]];

            if (renderer->SetMaterial(effect->mMaterial1))
                DispatchRibbon(effect, renderer, c2w);
        }

        for (int i = 0, n = mPass[1].size(); i < n; i++)
        {
            const cEffectRibbon* effect = mEffects[mPass[1][i]];

            if (renderer->SetMaterial(effect->mMaterial2))
            {
                if (effect->mTexture2 >= 0)
                    renderer->SetTexture(kTextureDiffuseMap, effect->mTexture2);

                DispatchRibbon(effect, renderer, c2w);
            }
        }

        UpdateMSPF(timer.GetTime(), &mDispatchMSPF);
    }


    void cEffectTypeRibbon::DispatchRibbon(const cEffectRibbon* effect, cIRenderer* renderer, const cTransform& c2w)
    {
        if (effect->mTexture1 >= 0)
            renderer->SetTexture(kTextureDiffuseMap, effect->mTexture1);

        if (effect->mTexture2 >= 0)
            renderer->SetTexture(kTextureNormalMap, effect->mTexture2);

        nHL::DispatchRibbon
        (
            renderer,
            mQuadMesh,

            *effect->mDesc,
            effect,

            effect->mEffectTransform,
            c2w.Rot()
        );

    }
}

namespace nHL
{
    cIEffectType* CreateEffectTypeRibbon(cIAllocator* alloc)
    {
        return new(alloc) cEffectTypeRibbon;
    }
}


// --- cDescRibbon ------------------------------------------------------------

void cDescRibbon::Config(const cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mCycleTime = config["cycleTime"].AsFloat(mCycleTime);

    mLength      = config["length"  ].AsFloat(mLength);
    mMaxSegments = config["segments"].AsInt  (mMaxSegments);

    mUVRepeat = config[CL_TAG("uvRepeat" )].AsFloat(mUVRepeat);
    mUVSpeed  = config[CL_TAG("uvSpeed"  )].AsFloat(mUVSpeed);

    mFadeRate = config[CL_TAG("fadeRate")].AsFloat(mFadeRate);

    mDispatch.Config(config);
    mPhysics .Config(config);

    mFlags.mHasForces = mPhysics.mDirForces != vl_0;

    const cValue&      layerV = config[CL_TAG("layer")];
    const cValue& depthLayerV = config[CL_TAG("depthLayer")];

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
}


// --- cEffectRibbon ----------------------------------------------------------

bool cEffectRibbon::Init(cIEffectType* effectType)
{
    return true;
}

bool cEffectRibbon::Shutdown()
{
    SetDescription(0);

    return true;
}

void cEffectRibbon::SetDescription(const cDescRibbon* desc)
{
    if (mDesc)
    {
        mMaterial1 = -1;
        mMaterial2 = -1;
        mTexture1 = -1;
        mTexture2 = -1;
    }

    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }

    mNumSegments = mDesc->mMaxSegments;

    cIRenderer* renderer = HL()->mRenderer;

    if (mDesc->mDispatch.mMaterialTag != 0)
        mMaterial1 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterialTag);
    else
        mMaterial1 = renderer->MaterialRefFromTag(kMaterialTag);

    if (mDesc->mDispatch.mMaterial2Tag != 0)
        mMaterial2 = renderer->MaterialRefFromTag(mDesc->mDispatch.mMaterial2Tag);

    if (mDesc->mDispatch.mTextureTag != 0)
        mTexture1 = renderer->TextureRefFromTag(mDesc->mDispatch.mTextureTag);
    if (mDesc->mDispatch.mTexture2Tag != 0)
        mTexture2 = renderer->TextureRefFromTag(mDesc->mDispatch.mTexture2Tag);

    mRenderHash = CRC32((uint8_t*) &mMaterial1, sizeof(mMaterial1));
    mRenderHash = CRC32((uint8_t*) &mMaterial2, sizeof(mMaterial1), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture1 , sizeof(mTexture1 ), mRenderHash);
    mRenderHash = CRC32((uint8_t*) &mTexture2 , sizeof(mTexture2 ), mRenderHash);

    mRenderOrder &= 0x00FFFFFF;
    mRenderOrder |= (127 - mDesc->mLayer) << 24;
}

void cEffectRibbon::SetTransforms(const cTransform& sourceTransform, const cTransform& effectTransform)
{
    mRoot = sourceTransform.Trans();
    mRootValid = true;
    mEffectTransform = effectTransform;
}

void cEffectRibbon::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mFlags.mActive = true;

    mRootValid = false;
    mPositions.clear();
    mFades.clear();
    mUVOffset = 0.0f;
    mHaveAllPositions = false;
}

void cEffectRibbon::Stop(tTransitionType transition)
{
    if (mFlags.mActive)
    {
        mFlags.mActive = false;

        if (transition == kTransitionImmediate)
        {
            mRootValid = false;
            mPositions.clear();
            mFades.clear();
            mUVOffset = 0.0f;
            mHaveAllPositions = false;
        }
    }
}

void cEffectRibbon::Update(float dt, const cEffectParams* params)
{
    CL_ASSERT(mFlags.mActive);

    mDirForces = mDesc->mPhysics.mDirForces;
    // TODO: add global forces, attractors etc.

    mAge += dt;

    while (mAge > mDesc->mCycleTime)
    {
        if (mDesc->mFlags.mSustain)
            mAge = mDesc->mCycleTime;
        else
            mAge -= mDesc->mCycleTime;
    }

    mUVOffset += mDesc->mUVSpeed * dt;

    if (mRootValid)
        UpdatePositionHistory(dt);
}

void cEffectRibbon::UpdatePositionHistory(float dt)
{
    CL_ASSERT(mRootValid);
    CL_ASSERT(mFades.size() == mPositions.size());

    if (mPositions.empty())
    {
        mHaveAllPositions = false;
        mPositions.resize(2, mRoot);
        mFades.resize(2, 1.0f);
    }
    else
    {
        float sampleInterval = mDesc->mLength / mDesc->mMaxSegments;
        float distance = len(mRoot - mPositions[0]);
        float fadeDT = mDesc->mFadeRate * dt;

        if (distance >= sampleInterval)
        {
            int n = mPositions.size();

            if (n <= mNumSegments)
            {
                mPositions.resize(n + 1);
                mFades.resize(n + 1);
            }
            else
            {
                n--;
                mHaveAllPositions = true;
            }

            for (int i = n; i > 0; i--)
            {
                // Shift positions up so we can insert, handle fade at same time.
                mPositions[i] = mPositions[i - 1];
                mFades[i] = ClampPositive(mFades[i - 1] - fadeDT);
            }

            mPositions.front() = mRoot;
            mFades.front() = 1.0f;
        }
        else
        {
            for (int i = 0, n = mFades.size(); i < n; i++)
                mFades[i] = ClampPositive(mFades[i] - fadeDT);
        }
    }


    if (mDesc->mFlags.mHasForces)
    {
        for (int i = 0, n = mPositions.size(); i < n; i++)
            mPositions[i] += dt * mDirForces;

        // re-constrain the ribbon's length
        // we don't constrain the initial link, or, if the buffer isn't full, the last.
        int nh = mPositions.size();
        if (!mHaveAllPositions)
            nh--;

        for (int i = 1; i < nh; i++)
        {
            float t = mDesc->mLength / (mDesc->mMaxSegments * len(mPositions[i] - mPositions[i - 1]) + 1e-6f);
            mPositions[i] = lerp(mPositions[i - 1], mPositions[i], t);
        }

        if (!mHaveAllPositions)
            mPositions[nh] = mPositions[nh - 1];
    }
}


void nHL::DispatchRibbon
(
    cIRenderer*         renderer,
    int                 quadMesh,

    const cDescRibbon& desc,
    const cEffectRibbon* effect,

    const cTransform& effectTransform,
    const Mat3f& cameraOrient
)
{
    if (effect->mPositions.empty() || effect->mNumSegments < 1)
        return;

    Vec3f cameraAt = cameraOrient[1];
    float uvStep = (desc.mUVRepeat / effect->mNumSegments);

    Vec3f   position0, position1;
    float   width0,    width1;
    Vec3f   colour0,   colour1;
    float   alpha0,    alpha1;
    float   fade0,     fade1;
    float   uv0,       uv1;
    Vec3f   quadB0,    quadT0;
    Vec3f   quadB1,    quadT1;

    int   currentSize = effect->mPositions.size() - 1;
    float invCurrentSizeP = (1.0f / currentSize);
    float invCurrentSize;

    float invSampleInterval = desc.mMaxSegments / desc.mLength;
    float partialSection = len(effect->mRoot - effect->mPositions.front()) * invSampleInterval;
    
    float initialSection = 0;
    float invInitialSection = 0;
    
    if (currentSize > 0 && partialSection > 0.0f)
    {
        initialSection = partialSection / currentSize;  // offset to use to account for fractional first section
        invInitialSection = 1.0f / initialSection;
    }

    if (!effect->mHaveAllPositions)
    {
        if (currentSize > 1)
            invCurrentSize = (1.0f / (partialSection + currentSize - 1)); 
        else
            invCurrentSize = 1.0f;
    }
    else
        invCurrentSize = invCurrentSizeP;

    nHL::cQuadVertex* v;
    currentSize = renderer->GetQuadBuffer(quadMesh, currentSize, (uint8_t**) &v);

    // Set up values for initial edge
    colour0 = !desc.mDispatch.mColourFrames.empty() ? desc.mDispatch.mColourFrames.front() : kColourWhite;
    alpha0  = !desc.mDispatch.mAlphaFrames .empty() ? desc.mDispatch.mAlphaFrames .front() : 1.0f;
    fade0   = 1.0f;

    width0    = !desc.mDispatch.mSizeFrames.empty() ? desc.mDispatch.mSizeFrames.front() : 1.0f;
    position0 = effect->mRoot;
    uv0       = effect->mUVOffset;

    float halfScale = effectTransform.Scale() * 0.5f;

    position0 = effectTransform.TransformPoint(position0);
    width0 *= halfScale;

    // loop through segments

    for (int i = 0; i < currentSize; i++)
    {
        float section (float(i + 1) * invCurrentSize);
        float sectionP(float(i + 1) * invCurrentSizeP);

        section = ClampUnit(section);

        colour1 = LinearAnim(desc.mDispatch.mColourFrames.size(), desc.mDispatch.mColourFrames.data(), section);
        alpha1  = LinearAnim(desc.mDispatch.mAlphaFrames .size(), desc.mDispatch.mAlphaFrames .data(), section);
        width1  = LinearAnim(desc.mDispatch.mSizeFrames  .size(), desc.mDispatch.mSizeFrames  .data(), section) * halfScale;

        if (sectionP < initialSection)
        {
            float s = sectionP * invInitialSection;

            position1 = lerp(effect->mRoot, effect->mPositions.front(), s);
            fade1     = lerp(1.0f,          effect->mFades.front(),     s);
        } 
        else
        {
            float s = ClampUnit(sectionP - initialSection);

            position1 = LinearAnim(effect->mPositions.size(), effect->mPositions.data(), s);
            fade1     = LinearAnim(effect->mFades.size(),     effect->mFades.data(),     s);
        }

        position1 = effectTransform.TransformPoint(position1);

        cRGBA32 c0 = ColourAlphaToRGBA32(cColourAlpha(colour0, alpha0 * fade0));
        cRGBA32 c1 = ColourAlphaToRGBA32(cColourAlpha(colour1, alpha1 * fade1));

        Vec3f up = norm_safe(cross(cameraAt, position1 - position0));

        // Don't know first edge until here, so can't pre-initialise. TODO: perhaps use i-1/i+1 instead?
        if (i == 0)
        {
            quadT0 = position0 - up * width0;
            quadB0 = position0 + up * width0;
        } 

        quadT1 = position1 - up * width1;
        quadB1 = position1 + up * width1;

        uv1 = uv0 + uvStep;

        // Fill quad
        v->mPosition = quadT0;
        v->mUV       = Vec2f(uv0, 0.0f);
        v->mColour   = c0.mAsUInt32;
        v++;

        v->mPosition = quadB0;
        v->mUV       = Vec2f(uv0, 1.0f);
        v->mColour   = c0.mAsUInt32;
        v++;

        v->mPosition = quadB1;
        v->mUV       = Vec2f(uv1, 1.0f);
        v->mColour   = c1.mAsUInt32;
        v++;

        v->mPosition = quadT1;
        v->mUV       = Vec2f(uv1, 0.0f);
        v->mColour   = c1.mAsUInt32;
        v++;

        // Shift along by one vertex
        position0 = position1;
        width0    = width1;

        colour0   = colour1;
        alpha0    = alpha1;
        fade0     = fade1;

        uv0 = uv1;

        quadB0 = quadB1;
        quadT0 = quadT1;
    }

    renderer->DispatchAndReleaseBuffer(quadMesh, currentSize);
}
