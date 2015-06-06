//
//  File:       HLEffectGroup.cpp
//
//  Function:   Controls a group of child effects
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectGroup.h>

#include <HLEffectsManager.h>

#include <CLLog.h>
#include <CLValue.h>
#include <CLString.h>

using namespace nHL;
using namespace nCL;

namespace
{
    typedef cEffectType<cDescGroup, cEffectGroup> tEffectTypeGroup;
}


void cDescGroupEffect::Config(const nCL::cValue& config, cIEffectsManager* manager)
{
    const cValue& idValue = config["id"];

    const char* idStr = idValue.AsString();

    if (idStr)
    {
        const char* subID = strchr(idStr, '.');

        if (subID)
        {
            mTag = TagFromString(subID + 1);

            string temp(idStr, subID);        ///< ugh...
            mType = manager->EffectTypeFromTag(TagFromString(temp.c_str()));

            if (mType == kMaxEffectTypes)
                CL_LOG_E("Effects", "Unknown type %s in group\n", temp.c_str());
        }
        else
        {
            mType = manager->EffectTypeFromTag(config["type"].AsTag());
            mTag = config["id"].AsTag(mTag);

            if (mType == kMaxEffectTypes)
                CL_LOG_E("Effects", "Unknown type %s in group\n", config["type"].AsString());
        }
    }

    SetFromValue(config["transform"], &mTransform);

    mTimeScale = config["timeScale"].AsFloat(mTimeScale);

    mFlags.mRigid          = config["rigid"]    .AsBool(mFlags.mRigid);
    mFlags.mImmediateStart = config["hardStart"].AsBool(mFlags.mImmediateStart);
    mFlags.mImmediateStop  = config["hardStop"] .AsBool(mFlags.mImmediateStop);
}


void cDescGroup::Config(const nCL::cValue& config, cIEffectType*, cIEffectsManager* manager)
{
    const cValue& effectsConfig = config["effects"];

    mEffects.resize(effectsConfig.NumElts());

    for (int i = 0, n = effectsConfig.NumElts(); i < n; i++)
        mEffects[i].Config(effectsConfig.Elt(i), manager);

    mFlags.mRigid          = config["rigid"]    .AsBool(false);
    mFlags.mImmediateStart = config["hardStart"].AsBool(mFlags.mImmediateStart);
    mFlags.mImmediateStop  = config["hardStop"] .AsBool(mFlags.mImmediateStop);
}



// cEffectGroup

bool cEffectGroup::Init(cIEffectType* effectType)
{
    tEffectTypeGroup* groupType = static_cast<tEffectTypeGroup*>(effectType);

//    mAllocator = alloc;
    mManager = static_cast<cEffectsManager*>(groupType->Manager());

    mFlags = { 0 };

    mSourceToEffect.MakeIdentity();
    mEffectToWorld.MakeIdentity();

    return true;
}

bool cEffectGroup::Shutdown()
{
    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
        mManager->EffectType(mActiveEffects[i].mType)->DestroyInstance(mActiveEffects[i].mRef);

    mActiveEffects.clear();
    mDesc = 0;
    mManager = 0;

    return true;
}

void cEffectGroup::SetDescription(const cDescGroup* desc)
{
    if (mDesc)
        UpdateLOD(-1);

    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }
}

void cEffectGroup::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
    mSourceToEffect = sourceXform;
    mEffectToWorld  = effectXform;

    mFlags.mTransformsDirty = true;
}

void cEffectGroup::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    if (mDesc->mFlags.mImmediateStart)
        transition = kTransitionImmediate;

    mStartTransition = transition;
    mFlags.mActive = true;
    mFlags.mSourcesActive = true;

    UpdateTransforms();

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];

        const cDescGroupEffect& effectDesc = mDesc->mEffects[effect.mDescIndex];
        cIEffectType* effectType = mManager->EffectType(tEffectType(effectDesc.mType));

        if (effectDesc.mFlags.mImmediateStart)
            effectType->Start(effect.mRef, kTransitionImmediate);
        else
            effectType->Start(effect.mRef, transition);
    }
}

void cEffectGroup::Stop(tTransitionType transition)
{
    if (mDesc->mFlags.mImmediateStop)
        transition = kTransitionImmediate;

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];

        const cDescGroupEffect& effectDesc = mDesc->mEffects[effect.mDescIndex];
        cIEffectType* effectType = mManager->EffectType(tEffectType(effectDesc.mType));

        if (effectDesc.mFlags.mImmediateStop)
            effectType->Stop(effect.mRef, kTransitionImmediate);
        else
            effectType->Stop(effect.mRef, transition);
    }

    mFlags.mSourcesActive = false;

    if (transition == kTransitionImmediate)
        mFlags.mActive = false;
}

void cEffectGroup::Update(float dt, const cEffectParams* params)
{
    CL_ASSERT(mDesc);
    CL_ASSERT(mFlags.mActive);

    if (mLOD < 0)
        UpdateLOD(0);

    if (mFlags.mTransformsDirty)
        UpdateTransforms();

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];
        const cDescGroupEffect& desc = mDesc->mEffects[effect.mDescIndex];

        cIEffectType* effectType = mManager->EffectType(effect.mType);

        if (effectType->IsActive(effect.mRef))
        {
            effectType->Update(effect.mRef, dt * desc.mTimeScale, params);
        }
    }

    CheckIfStillActive();
}

void cEffectGroup::SetSourceToEffect(const cTransform& xform)
{
    mSourceToEffect = xform;
    mFlags.mTransformsDirty = true;
}

void cEffectGroup::SetEffectToWorld (const cTransform& xform)
{
    mEffectToWorld = xform;
    mFlags.mTransformsDirty = true;
}

void cEffectGroup::UpdateLOD(int newLOD)
{
    int di = 0;

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];

        bool isValid = newLOD >= 0;
        tTransitionType lodTransition = kTransitionSource;

        if (!isValid)
        {
            cIEffectType* effectType = mManager->EffectType(effect.mType);

            effectType->Stop(effect.mRef, lodTransition);

            if (lodTransition == kTransitionImmediate || !effectType->IsActive(effect.mRef))
                effectType->DestroyInstance(effect.mRef);
            else
                isValid = true;
        }

        if (isValid)
        {
            // preserve it
            if (i != di)
                mActiveEffects[di] = mActiveEffects[i];
            di++;
        }
    }

    mActiveEffects.resize(di);

    for (int i = 0, n = mDesc->mEffects.size(); i < n; i++)
    {
        const cDescGroupEffect& desc = mDesc->mEffects[i];

        if (desc.mType == kMaxEffectTypes)
            continue;

        bool wasValid = mLOD >= 0;
        bool isValid = newLOD >= 0;

        if (!wasValid && isValid)
        {
            cActiveEffect newEffect;

            cIEffectType* effectType = mManager->EffectType(tEffectType(desc.mType));

            newEffect.mDescIndex = i;
            newEffect.mType = tEffectType(desc.mType);
            newEffect.mRef  = effectType->CreateInstance(desc.mTag);
            newEffect.mTransform = desc.mTransform;

            if (newEffect.mRef.IsNull())
                CL_LOG_E("Effects", "No such effect: " CL_TAG_FMT "." CL_TAG_FMT "\n", mManager->EffectTypeTag(tEffectType(desc.mType)), desc.mTag);

            if (mFlags.mSourcesActive)
                effectType->Start(newEffect.mRef, mStartTransition);

            mActiveEffects.push_back(newEffect);
        }
    }

    mLOD = newLOD;
}

void cEffectGroup::UpdateTransforms()
{
    mFlags.mTransformsDirty = false;

    cTransform identity;
    cTransform sourceToWorld(mEffectToWorld);

    sourceToWorld.Prepend(mSourceToEffect);

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];
        cIEffectType* effectType = mManager->EffectType(effect.mType);

        if (effectType->IsActive(effect.mRef))
        {
            const cDescGroupEffect& desc = mDesc->mEffects[effect.mDescIndex];

            if (mDesc->mFlags.mRigid)
            {
                if (desc.mFlags.mRigid)
                {
                    cTransform effectTransform(sourceToWorld);
                    effectTransform.Prepend(desc.mTransform);

                    effectType->SetTransforms(effect.mRef, identity, effectTransform);
                }
                else
                    effectType->SetTransforms(effect.mRef, desc.mTransform, sourceToWorld);
            }
            else
            {
                if (desc.mFlags.mRigid)
                {
                    cTransform effectTransform(sourceToWorld);
                    effectTransform.Prepend(desc.mTransform);

                    effectType->SetTransforms(effect.mRef, identity, effectTransform);
                }
                else
                {
                    cTransform sourceTransform(mSourceToEffect);
                    sourceTransform.Prepend(desc.mTransform);

                    effectType->SetTransforms(effect.mRef, sourceTransform, mEffectToWorld);
                }
            }
        }
    }
}

bool cEffectGroup::CheckIfStillActive()
{
    if (!mFlags.mActive)
        return false;

    for (int i = 0, n = mActiveEffects.size(); i < n; i++)
    {
        cActiveEffect& effect = mActiveEffects[i];

        if (mManager->EffectType(effect.mType)->IsActive(effect.mRef))
            return true;

    }

    Stop(kTransitionImmediate);
    return false;
}

// --- cEffectTypeGroup --------------------------------------------------------

#include "HLEffectType.cpp"

namespace nHL
{
    cIEffectType* CreateEffectTypeGroup(cIAllocator* alloc)
    {
        return new(alloc) cEffectType<cDescGroup, cEffectGroup>;
    }
}
