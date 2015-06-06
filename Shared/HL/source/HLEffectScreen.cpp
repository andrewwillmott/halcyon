//
//  File:       HLEffectScreen.cpp
//
//  Function:   Provides fullscreen background/foreground effects
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectScreen.h>

#include <HLRenderUtils.h>
#include <HLServices.h>

#include <CLValue.h>
#include <CLMemory.h>

#include <CLParams.h>

#include <GLConfig.h>

using namespace nHL;
using namespace nCL;


// --- cDescScreen ------------------------------------------------------------

namespace
{
    cEnumInfo kEnumScreenMode[] =
    {
        "add",          kScreenAdditive,
        "blend",        kScreenBlend,
        "tint",         kScreenTint,
        "background",   kScreenBackground,
        "skybox",       kScreenSkybox,
        0, 0
    };
}

void cDescScreen::Config(const cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mFlags.mLoop    = config["loop"]   .AsBool(mFlags.mLoop);
    mFlags.mSustain = config["sustain"].AsBool(mFlags.mSustain);

    mMode  = AsEnum(config["mode"], kEnumScreenMode, mMode);

    mLife  = config["life"].AsFloat(mLife);
    mDelay = config["delay"].AsFloat(mDelay);
    mSort  = config["sort"].AsInt(mSort);

    SetFromValue(config["colour"],  &mColourFrames);
    SetFromValue(config["alpha" ],  &mAlphaFrames);

    mTextureTag = config["texture"].AsTag(mTextureTag);
}


// --- cEffectScreen ----------------------------------------------------------

bool cEffectScreen::Init(cIEffectType* effectType)
{
    return true;
}

bool cEffectScreen::Shutdown()
{
    return true;
}

void cEffectScreen::SetDescription(const cDescScreen* desc)
{
    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }

    mAgeStep = LifeToAgeStep(desc->mLife);
    if (mDesc->mTextureTag)
        mTextureRef = HL()->mRenderer->TextureRefFromTag(mDesc->mTextureTag);
}

void cEffectScreen::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
}

void cEffectScreen::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mAge = 0;

    mFlags.mActive = true;
}

void cEffectScreen::Stop(tTransitionType transition)
{
    if (mFlags.mActive)
    {


        mFlags.mActive = false;
    }
}

void cEffectScreen::Update(float dt, const nCL::cParams* params)
{
    mAge = UpdatedAge(mAge, mAgeStep, dt);

    if (IsExpired(mAge))
    {
        if (mDesc->mFlags.mLoop)
            mAge = WrapAge(mAge);
        else
        {
            mAge = ClampAge(mAge);

            if (!mDesc->mFlags.mSustain)
                Stop();
        }
    }

    float paramAlpha  = params->Param(kEffectParamAlpha,  1.0f);
    Vec3f paramColour = params->Param(kEffectParamColour, Vec3f(1.0f));

    mColourAlpha.AsColour() = LinearAnim(mDesc->mColourFrames.size(), mDesc->mColourFrames.data(), mAge) * paramColour;
    mColourAlpha.AsAlpha () = LinearAnim(mDesc->mAlphaFrames .size(), mDesc->mAlphaFrames .data(), mAge) * paramAlpha;
}

// --- cEffectTypeScreen -------------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    const tTag kScreenLayerTag = CL_TAG("screens");

    cEltInfo kScreenFormat[] =
    {
        { kVBPositions, 2, GL_FLOAT,         8, false },
        { kVBColours,   4, GL_UNSIGNED_BYTE, 4, true  }
    };

    struct cScreenVertex
    {
        Vec2f    mCoord;
        cRGBA32  mColour;
    };

    const tTag kScreenUntexturedMaterialTag[kMaxScreenModes] =
    {
        CL_TAG("screenAdditive"),
        CL_TAG("screenBlend"),
        CL_TAG("screenTint"),
        CL_TAG("screenBackground"),
        CL_TAG("screenSkybox")
    };

    const tTag kScreenTexturedMaterialTag[kMaxScreenModes] =
    {
        CL_TAG("screenAdditiveTextured"),
        CL_TAG("screenBlendTextured"),
        CL_TAG("screenTintTextured"),
        CL_TAG("screenBackground"),
        CL_TAG("screenSkybox")
    };

    typedef cEffectTypeValue<cDescScreen, cEffectScreen> tEffectTypeScreen;

    class cEffectTypeScreen :
        public tEffectTypeScreen,
        public cIRenderLayer
    {
    public:
        CL_ALLOC_LINK_DECL;

        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void PostInit() override;
        void Shutdown() override;

        void DebugMenu(cUIState* uiState) override;

        // cIRenderer
        void Dispatch(cIRenderer* renderer, const cRenderLayerState& state) override;

    protected:
        int mScreenUntexturedMaterials[kMaxScreenModes] = { 0 };
        int mScreenTexturedMaterials  [kMaxScreenModes] = { 0 };

        bool mDispatchEnabled = true;
    };

    void cEffectTypeScreen::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeScreen::Init(manager, alloc);

    }

    void cEffectTypeScreen::PostInit()
    {
        for (int i = 0; i < kMaxScreenModes; i++)
            mScreenUntexturedMaterials[i] = mRenderer->MaterialRefFromTag(kScreenUntexturedMaterialTag[i]);
        for (int i = 0; i < kMaxScreenModes; i++)
            mScreenTexturedMaterials[i] = mRenderer->MaterialRefFromTag(kScreenTexturedMaterialTag[i]);

        mRenderer->RegisterLayer(kScreenLayerTag, this);
    }

    void cEffectTypeScreen::Shutdown()
    {
        mRenderer->RegisterLayer(kScreenLayerTag, 0);

        tEffectTypeScreen::Shutdown();
    }

    void cEffectTypeScreen::DebugMenu(cUIState* uiState)
    {
        tEffectTypeScreen::DebugMenu(uiState);

        uiState->HandleToggle(ItemID(0x022c3477), "Dispatch", &mDispatchEnabled);
    }


    // cIRenderLayer
    void cEffectTypeScreen::Dispatch(cIRenderer* renderer, const cRenderLayerState& state)
    {
        if (!mDispatchEnabled)
            return;

        for (int i = 0; i < mSlots.NumSlots(); i++)
        {
            if (!mSlots.InUse(i))
                continue;

            if (mEffects[i].mFlags.mActive)
            {
                if (IsValid(mEffects[i].mTextureRef))
                {
                    renderer->SetMaterial(mScreenTexturedMaterials[mEffects[i].mDesc->mMode]);
                    renderer->SetTexture(kTextureDiffuseMap, mEffects[i].mTextureRef);
                }
                else
                    renderer->SetMaterial(mScreenUntexturedMaterials[mEffects[i].mDesc->mMode]);

                cRGBA32 sc32 = ColourAlphaToRGBA32(mEffects[i].mColourAlpha);

                DrawRectFlipY(renderer, sc32);  // TODO
            }
        }
    }
}

namespace nHL
{
    cIEffectType* CreateEffectTypeScreen(cIAllocator* alloc)
    {
        return new(alloc) cEffectTypeScreen;
    }
}
