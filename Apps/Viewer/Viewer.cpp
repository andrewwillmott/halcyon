//
//  File:       Viewer.cpp
//
//  Function:   Viewer prototype for rendering and UI tests
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include "Viewer.h"

#include <HLMain.h>

#include <IHLConfigManager.h>

#include <HLDebugDraw.h>
#include <HLParticleUtils.h>
#include <HLServices.h>
#include <HLCamera.h>
#include <HLUI.h>

#include <CLColour.h>
#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLMatUtil.h>
#include <CLMemory.h>
#include <CLParams.h>
#include <CLString.h>
#include <CLUtilities.h>
#include <CLValue.h>

#include <CLJSON.h>

using namespace nApp;
using namespace nHL;
using namespace nCL;

namespace
{
    struct cTestController :
        public cIPhysicsController,
        cAllocLinkable
    {
        CL_ALLOC_LINK_DECL;

        void Update(const cTransform& xform, int count, const float dts[], size_t dtStride, Vec3f positions[], size_t positionStride, Vec3f velocities[], size_t velocityStride) override
        {
            // Simple hacky floor collision
            for (int i = 0; i < count; i++)
            {
                if ((*positions)[2] < 0.0f)
                {

                    (*positions)[2] = 0.0f;
                    (*velocities)[2] = -(*velocities)[2];
                }

                ((uint8_t*&) positions)     += positionStride;
                ((uint8_t*&) velocities)    += velocityStride;
            }
        }
    };
}

//------------------------------------------------------------------------------
// cViewerMode
//------------------------------------------------------------------------------

cViewerMode::cViewerMode()
{
}

cViewerMode::~cViewerMode()
{
}

bool cViewerMode::PreInit()
{
    return true;
}

bool cViewerMode::Init()
{
    const cServices* hl = HL();
    cIAllocator* alloc = AllocatorFromObject(this);

    mCamera = new(alloc) cSimpleCamera;
    hl->mRenderer->RegisterCamera(kMainTag, mCamera);

    hl->mEffectsManager->RegisterPhysicsController(CL_TAG("bounce"), new(alloc) cTestController);

    return true;
}

bool cViewerMode::Shutdown()
{
    const cServices* hl = HL();

    hl->mModelManager->DestroyInstances(mModels.size(), mModels.data());
    mModels.clear();
    mRotationSpeeds.clear();

    hl->mEffectsManager->DestroyInstances(mEffects.size(), mEffects.data());
    mEffects.clear();

    hl->mRenderer->RegisterCamera(kMainTag, 0);
    mCamera = 0;

    return true;
}

void cViewerMode::UpdateFromConfig(const cObjectValue* config)
{
    mCamera->Config(config->Member(CL_TAG("camera")).AsObject());

    const cValue& viewerConfig = config->Member(CL_TAG("viewer"));

    LoadModels (viewerConfig.Member(CL_TAG("models")));
    LoadEffects(viewerConfig.Member(CL_TAG("effects")));

    const cObjectValue* testConfig = HL()->mConfigManager->Config()->Member("test").AsObject();

    if (testConfig)
    {
        printf("test:\n");

        auto children = testConfig->Children();
        cFileSpec spec;

        for (auto c : children)
        {
            const cValue& value = c.Value();
            const char*   name  = c.Name();

            FindSpec(&spec, c);

            printf("  %s : %s  (from %p, %s)\n", name, TypeName(value.Type()), (void*) c.Owner(), spec.Path());
        }
    }
}

void cViewerMode::LoadModels(const cValue& modelsV)
{
    auto mm = HL()->mModelManager;

    mm->DestroyInstances(mModels.size(), mModels.data());
    mModels.clear();
    mRotationSpeeds.clear();

    for (int i = 0, n = modelsV.NumElts(); i < n; i++)
    {
        const cValue& modelV = modelsV.Elt(i);

        tTag tag = modelV["id"].AsTag();

        tMIRef modelRef = mm->CreateInstance(tag);
        if (modelRef.IsNull())
            continue;

        mModels.push_back(modelRef);

        cTransform transform;
        SetFromValue(modelV["transform"], &transform);
        mm->SetTransform(modelRef, transform);

        mRotationSpeeds.push_back(modelV["rotationSpeed"].AsFloat());

        mm->SetVisible(modelRef, modelV["active"].AsBool());
    }
}

void cViewerMode::LoadEffects(const cValue& effectsV)
{
    auto hl = HL();

    hl->mEffectsManager->DestroyInstances(mEffects.size(), mEffects.data());
    mEffects.clear();

    for (int i = 0, n = effectsV.NumElts(); i < n; i++)
    {
        const cValue& effectV = effectsV.Elt(i);

        tTag effectTag = effectV["effect"].AsTag();

        tEIRef effect = hl->mEffectsManager->CreateInstance(effectTag);
        if (effect.IsNull())
            continue;

        mEffects.push_back(effect);

        cTransform sourceTransform;
        SetFromValue(effectV["sourceTransform"], &sourceTransform);
        hl->mEffectsManager->SetSourceTransform(effect, sourceTransform);

        cTransform effectTransform;
        SetFromValue(effectV["effectTransform"], &effectTransform);
        hl->mEffectsManager->SetEffectTransform(effect, effectTransform);

        bool started = effectV["active"].AsBool(true);

        if (started)
            hl->mEffectsManager->StartSources(mEffects.back());
    }
}

void cViewerMode::Activate()
{
}

void cViewerMode::Deactivate()
{
}

void cViewerMode::Update(float dt, float gameDT, cUIState* uiState)
{
    const cServices* hl = HL();
    auto em = hl->mEffectsManager;
    auto inputState = uiState->InputState();

    if (inputState->KeyModifiers() == (kModControl | kModShift) && mCurrentEffect < mEffects.size())
    {
        uiState->AddItemCanvas(0x020f6920);

        tEIRef effect = mEffects[mCurrentEffect];

        if (uiState->InteractionCount() == 1 && uiState->InputState()->PointerWentDown(uiState->InteractionPointerIndex(0)))
            em->StartSources(effect);

        if (uiState->ItemWasClicked())
            em->StopSources(effect);

        if (uiState->InteractionCount() > 0)
        {
            Vec2f cursor = uiState->InteractionPoint();
            cursor[1] = -cursor[1];

            cTransform sourceTransform;
            sourceTransform.SetTrans(Vec3f(cursor * 0.01f, 0.0f));
            em->SetSourceTransform(effect, sourceTransform);
        }
    }

    HandleStandardInput(uiState, mCamera);

    for (size_t i = 0, n = mRotationSpeeds.size(); i < n; i++)
    {
        if (mRotationSpeeds[i] == 0.0f)
            continue;

        cTransform xform = hl->mModelManager->Transform(mModels[i]);

        float speed = mRotationSpeeds[i];
        
        xform.PrependRotZ(gameDT * vl_twoPi * speed);
        hl->mModelManager->SetTransform(mModels[i], xform);
    }

    cDebugDraw* dd = hl->mDebugDraw;
    dd->Reset();

    dd->SetColour(kColourOrange);
    DrawTextF(dd, -10, -10, "Current Effect: %d", mCurrentEffect);

    // Draw axis
    dd->SetAlpha(1.0f);
    DrawAxes(dd, vl_zero, 30.0f);
    dd->SetColour(kColourOrange);
    DrawCircle(dd, Vec3f(vl_zero), 5.0f);
    dd->SetColour(kColourBlue);
    dd->SetAlpha(0.75f);
    FillCircle(dd, Vec3f(vl_zero), 3.0f);
}

void cViewerMode::HandleKeys(nCL::cInputState* inputState)
{
    const cServices* hl = HL();
    auto em = hl->mEffectsManager;

    if (mCurrentEffect > 0 && inputState->KeyWentDown('['))
        mCurrentEffect--;
    if (mCurrentEffect < mEffects.size() - 1 && inputState->KeyWentDown(']'))
        mCurrentEffect++;

    if (mCurrentEffect < mEffects.size())
    {
        tEIRef effect = mEffects[mCurrentEffect];

        if (inputState->KeyWentDown(','))
            em->StartSources(effect);
        if (inputState->KeyWentDown('.'))
            em->StopSources(effect);

        if (inputState->KeyWentDown(',', kModShift))
            em->StartEffect(effect);
        if (inputState->KeyWentDown('.', kModShift))
            em->StopEffect(effect);

        if (inputState->KeyWentDown(',', kModControl))
            mEffects[mCurrentEffect] = em->CreateInstance(CL_TAG("viewerTest"));
        if (inputState->KeyWentDown('.', kModControl))
            em->DestroyInstance(effect);


        cTransform sourceTransform = em->SourceTransform(effect);
        const float ds = 0.125f;

        if (inputState->KeyIsDown(kLeftKey))
            sourceTransform.mTranslation[0] -= ds;
        if (inputState->KeyIsDown(kRightKey))
            sourceTransform.mTranslation[0] += ds;
        if (inputState->KeyIsDown(kDownKey))
            sourceTransform.mTranslation[1] -= ds;
        if (inputState->KeyIsDown(kUpKey))
            sourceTransform.mTranslation[1] += ds;

        const float da = 0.125f;

        if (inputState->KeyIsDown(kDownKey, kModShift))
            sourceTransform.PrependRotX(-da);
        if (inputState->KeyIsDown(kUpKey, kModShift))
            sourceTransform.PrependRotX(da);

        em->SetSourceTransform(effect, sourceTransform);

        if (inputState->KeyWentDown(','))
            em->StartSources(effect);
        if (inputState->KeyWentDown('.'))
            em->StopSources(effect);

        if (inputState->KeyWentDown(',', kModShift))
            em->StartEffect(effect);
        if (inputState->KeyWentDown('.', kModShift))
            em->StopEffect(effect);
    }

    if (inputState->KeyWentDown('O'))
    {
        tEIRef effect = em->CreateOneShotInstance(CL_TAG("injectTest"));

        cTransform xform;
        xform.mTranslation = Vec3f(50.0f, 50.0f, 0.0f);
        xform.mScale = 15.0f;
        em->SetEffectTransform(effect, xform);
        em->StartSources(effect);
    }

    for (int i = 0; i < 9; i++)
        if (inputState->KeyWentDown('1' + i))
            hl->mRenderer->SetRenderFlag(i, !hl->mRenderer->RenderFlag(i));
}

void cViewerMode::DebugMenu(nHL::cUIState* uiState)
{
    uiState->DrawSeparator();

    tUIItemID itemID = ItemID(0x00d500c0);

    if (uiState->BeginSubMenu(itemID++, "View"))
    {
        tUIItemID itemID = ItemID(0x00d500c1);

        cIModelManager* mm = HL()->mModelManager;

        for (tMIRef ref : mModels)
            if (uiState->HandleToggle(itemID++, StringFromTag(mm->ModelTag(ref)), mm->Visible(ref)))
                mm->SetVisible(ref, !mm->Visible(ref));

        uiState->DrawSeparator();

        cIEffectsManager* em = HL()->mEffectsManager;

        for (tEIRef ref : mEffects)
            if (uiState->HandleToggle(itemID++, StringFromTag(em->EffectTag(ref)), em->IsActive(ref)))
            {
                if (em->IsActive(ref))
                    em->StopEffect(ref);
                else
                    em->StartEffect(ref);
            }

        uiState->EndSubMenu();
    }

    if (!mEffects.empty() && uiState->BeginSubMenu(itemID++, "Effect Tests"))
    {
        tUIItemID itemID = ItemID(0x00d500c2);

        auto em = HL()->mEffectsManager;
        tEIRef effectRef = mEffects[mCurrentEffect];

        if (uiState->HandleToggle(itemID++, "Paused", em->Paused(effectRef)))
            em->SetPaused(effectRef, !em->Paused(effectRef));

        if (uiState->HandleToggle(itemID++, "Visible", em->Visible(effectRef)))
            em->SetVisible(effectRef, !em->Visible(effectRef));

        if (uiState->HandleToggle(itemID++, "Real Time", em->RealTime(effectRef)))
            em->SetRealTime(effectRef, !em->RealTime(effectRef));

        uiState->DrawSeparator();

        nCL::cParams* params = em->Params(effectRef);

        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Rate", &value))
                params->SetParam(kEffectParamEmitRate, value);
        }
        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Speed", &value))
                params->SetParam(kEffectParamEmitSpeed, value);
        }
        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Size", &value))
                params->SetParam(kEffectParamEmitSize, value);
        }
        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Transparency", &value))
                params->SetParam(kEffectParamEmitAlpha, value);
        }
        {
            static float value = 0.0f;

            if (uiState->HandleSlider(itemID++, "Orangeness", &value))
                params->SetParam(kEffectParamEmitColour, lerp(kColourWhite, kColourOrange, value));
        }

        uiState->DrawSeparator();

        {
            static float value = 0.0f;

            if (uiState->HandleSlider(itemID++, "Live Blue", &value))
                params->SetParam(kEffectParamColour, lerp(kColourWhite, kColourCyan, value));
        }
        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Live Alpha", &value))
                params->SetParam(kEffectParamAlpha, value);
        }
        {
            static float value = 1.0f;

            if (uiState->HandleSlider(itemID++, "Live Scale", &value))
                params->SetParam(kEffectParamSize, value);
        }
        {
            static float value = 0.1f;

            if (uiState->HandleSlider(itemID++, "Anim Speed", &value))
                params->SetParam(kEffectParamAnimSpeed, value * 10.0f);
        }
        uiState->DrawSeparator();

        {
            static float value = 0.0f;

            if (uiState->HandleSlider(itemID++, "Shader Param", &value))
                params->SetParam(kEffectParamShader1, cColourAlpha(kColourGreen * value, 1.0f));
        }

        uiState->DrawSeparator();

        if (uiState->HandleButton(itemID++, Format("Emit Volume: %d", mCurrentVolume)))
        {
            switch (mCurrentVolume++ % 3)
            {
            case 0:
                params->SetParam(kEffectParamEmitVolume, cBounds3(Vec3f(-1.0f, -4.0f, 0.0f), Vec3f(1.0f, 4.0f, 1.0f)));
                break;
            case 1:
                params->SetParam(kEffectParamEmitVolume, cBounds3(Vec3f(-4.0f, -1.0f, 0.0f), Vec3f(4.0f, 1.0f, 1.0f)));
                break;
            case 2:
                params->SetParam(kEffectParamEmitVolume, cBounds3(vl_0));
                break;
            }
        }

        {
            static float value = 0.0f;

            if (value == 0.0f)
                value = em->SourceTransform(mEffects[0]).Scale();

            if (uiState->HandleSlider(itemID++, "Source Scale", &value))
            {
                value = ClampLower(value, 1e-6f);

                cTransform xform = em->SourceTransform(mEffects[0]);
                xform.SetScale(value);
                em->SetSourceTransform(mEffects[0], xform);
            }
        }

        uiState->EndSubMenu();
    }
}

bool nHL::SetupApp(cIApp* app)
{
    app->RegisterAppMode(CL_TAG("viewer"), new (Allocator(kDefaultAllocator)) cViewerMode);
    return true;
}
