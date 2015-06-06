//
//  File:       HLEffectType.cpp
//
//  Function:   Defines management for a particular kind of effect
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectType.h>

#include <IHLConfigManager.h>
#include <HLServices.h>
#include <HLUI.h>

#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLValue.h>

using namespace nHL;
using namespace nCL;

namespace
{
}

// cEffectTypeBase

inline void cEffectTypeBase::Init(cIEffectsManager* manager, cIAllocator* alloc)
{
    mManager = manager;
    mRenderer = HL()->mRenderer;
    mAllocator = alloc;
}

inline void cEffectTypeBase::PostInit()
{
}

inline void cEffectTypeBase::Shutdown()
{
    mTagToIndex.clear();
}

inline void cEffectTypeBase::PreUpdate(float realDT, float gameDT)
{
}

inline void cEffectTypeBase::PostUpdate(float realDT, float gameDT)
{
}


// cEffectType<>

template<class T_D, class T_E> void cEffectType<T_D, T_E>::Shutdown()
{
    RemoveAllInstances();
    
    mDescs.clear();
    cEffectTypeBase::Shutdown();
}

template<class T_D, class T_E> void cEffectType<T_D, T_E>::Config(const cObjectValue* config)
{
    mTagToIndex.clear();
    mDescs.clear();

    for (auto c : config->Children())
    {
        tTag          tag  = c.Tag();
        const cValue& info = c.Value();

        if (!info.IsObject())
            continue;

        CL_LOG("Effects", "  Adding %s\n", c.Name());

        mTagToIndex[tag] = mDescs.size();
        mDescs.push_back();
        T_D& effectDesc = mDescs.back();

        effectDesc.Config(info, this, mManager);
    }

    // Rebind descriptions
    for (int i = 0, n = mEffects.size(); i < n; i++)
        if (mEffects[i])
        {
            auto it = mTagToIndex.find(mEffectTags[i]);

            if (it != mTagToIndex.end())
                mEffects[i]->SetDescription(&mDescs[it->second]);
            else
                mEffects[i]->SetDescription(0);
        }
}

template<class T_D, class T_E> bool cEffectType<T_D, T_E>::HasEffect(tTag tag) const
{
    auto it = mTagToIndex.find(tag);

    return it != mTagToIndex.end();
}

template<class T_D, class T_E> tEIRef cEffectType<T_D, T_E>::CreateInstance(tTag tag)
{
    auto it = mTagToIndex.find(tag);

    if (it != mTagToIndex.end())
    {
        tEIRef result = mSlots.CreateSlot();

        mEffectTags.resize(mSlots.NumSlots());
        mEffectTags[result] = tag;

        mEffects.resize(mSlots.NumSlots());
        mEffects[result] = new(mAllocator) T_E;
        mEffects[result]->Init(this);
        mEffects[result]->SetDescription(&mDescs[it->second]);

        return result;
    }

    return kNullRef;
}

template<class T_D, class T_E> bool cEffectType<T_D, T_E>::DestroyInstance(tEIRef ref)
{
    bool result = mSlots.DestroySlot(ref);

    if (result)
    {
        int slot = ref;

        if (mEffects[slot])
        {
            mEffects[slot]->Shutdown();
            mEffects[slot] = 0;

            mEffectTags[slot] = 0;
        }
    }

    return result;
}

template<class T_D, class T_E> int cEffectType<T_D, T_E>::NumInstances() const
{
    return mSlots.NumSlotsInUse();
}

template<class T_D, class T_E> int cEffectType<T_D, T_E>::NumActiveInstances() const
{
    int numActive = 0;

    for (int i = 0; i < mSlots.NumSlots(); i++)
        if (mEffects[i] && mEffects[i]->IsActive())
            numActive++;

    return numActive;
}

template<class T_D, class T_E> void cEffectType<T_D, T_E>::SetTransforms(tEIRef ref, const cTransform& sourceToEffect, const cTransform& effectToWorld)
{
    if (mSlots.InUse(ref))
        mEffects[ref]->SetTransforms(sourceToEffect, effectToWorld);
}
template<class T_D, class T_E> void cEffectType<T_D, T_E>::Start(tEIRef ref, tTransitionType transition)
{
    if (mSlots.InUse(ref))
        mEffects[ref]->Start(transition);
}
template<class T_D, class T_E> void cEffectType<T_D, T_E>::Stop(tEIRef ref, tTransitionType transition)
{
    if (mSlots.InUse(ref))
        mEffects[ref]->Stop(transition);
}
template<class T_D, class T_E> bool cEffectType<T_D, T_E>::IsActive(tEIRef ref) const
{
    if (mSlots.InUse(ref))
        return mEffects[ref]->IsActive();

    return false;
}
template<class T_D, class T_E> bool cEffectType<T_D, T_E>::Update(tEIRef ref, float dt, const cEffectParams* params)
{
    if (mEnabled)
        mEffects[ref]->Update(dt, params);
    return true;
}

template<class T_D, class T_E> const char* cEffectType<T_D, T_E>::StatsString(const char* typeName) const
{
    int numInstances = mSlots.NumSlotsInUse();

    if (numInstances > 0)
    {
        int numActiveInstances = NumActiveInstances();
        
        mStats.format("%d/%d %s", numActiveInstances, numInstances, typeName);
        return mStats.c_str();
    }

    return 0;
}

template<class T_D, class T_E> void cEffectType<T_D, T_E>::DebugMenu(cUIState* uiState)
{
    tUIItemID itemID = ItemID(0x022c2a0c);

    int numInstances = mSlots.NumSlotsInUse();
    int numActiveInstances = NumActiveInstances();

    uiState->HandleToggle(itemID++, "Enabled", &mEnabled);
    uiState->DrawLabel(Format("%d / %d Active", numActiveInstances, numInstances));
}

template<class T_D, class T_E> void cEffectType<T_D, T_E>::RemoveAllInstances()
{
    mSlots.ClearSlots();

    for (int i = 0, n = mEffects.size(); i < n; i++)
        if (mEffects[i])
            mEffects[i]->Shutdown();

    mEffects.clear();
    mEffectTags.clear();
}

// cEffectTypeValue<>

template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::Shutdown()
{
    RemoveAllInstances();
    
    mDescs.clear();
    cEffectTypeBase::Shutdown();
}

template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::Config(const cObjectValue* config)
{
    mTagToIndex.clear();
    mDescs.clear();

    for (auto c : config->Children())
    {
        tTag          tag  = c.Tag();
        const cValue& info = c.Value();

        if (!info.IsObject())
            continue;

        CL_LOG("Effects", "  Adding %s\n", c.Name());

        mTagToIndex[tag] = mDescs.size();
        mDescs.push_back();
        T_D& effectDesc = mDescs.back();

        effectDesc.Config(info, this, mManager);
    }

    // Rebind descriptions
    for (int i = 0, n = mEffects.size(); i < n; i++)
        if (mSlots.InUse(i))
        {
            auto it = mTagToIndex.find(mEffectTags[i]);

            if (it != mTagToIndex.end())
                mEffects[i].SetDescription(&mDescs[it->second]);
            else
                mEffects[i].SetDescription(0);
        }
}

template<class T_D, class T_E> bool cEffectTypeValue<T_D, T_E>::HasEffect(tTag tag) const
{
    auto it = mTagToIndex.find(tag);

    return it != mTagToIndex.end();
}

template<class T_D, class T_E> tEIRef cEffectTypeValue<T_D, T_E>::CreateInstance(tTag tag)
{
    auto it = mTagToIndex.find(tag);

    if (it != mTagToIndex.end())
    {
        tEIRef result = mSlots.CreateSlot();

        mEffectTags.resize(mSlots.NumSlots());
        mEffectTags[result] = tag;

        mEffects.resize(mSlots.NumSlots());
        mEffects[result].Init(this);
        mEffects[result].SetDescription(&mDescs[it->second]);

        return result;
    }

    return kNullRef;
}

template<class T_D, class T_E> bool cEffectTypeValue<T_D, T_E>::DestroyInstance(tEIRef ref)
{
    bool result = mSlots.DestroySlot(ref);

    if (result)
    {
        int slot = ref;

        mEffects[slot].Shutdown();
        mEffects[slot] = T_E();

        mEffectTags[slot] = 0;
    }

    return result;
}

template<class T_D, class T_E> int cEffectTypeValue<T_D, T_E>::NumInstances() const
{
    return mSlots.NumSlotsInUse();
}

template<class T_D, class T_E> int cEffectTypeValue<T_D, T_E>::NumActiveInstances() const
{
    int numActive = 0;

    for (int i = 0; i < mSlots.NumSlots(); i++)
        if (mSlots.InUse(i) && mEffects[i].IsActive())
            numActive++;

    return numActive;
}

template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::SetTransforms(tEIRef ref, const cTransform& sourceToEffect, const cTransform& effectToWorld)
{
    if (mSlots.InUse(ref))
        mEffects[ref].SetTransforms(sourceToEffect, effectToWorld);
}
template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::Start(tEIRef ref, tTransitionType transition)
{
    if (mSlots.InUse(ref))
        mEffects[ref].Start(transition);
}
template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::Stop(tEIRef ref, tTransitionType transition)
{
    if (mSlots.InUse(ref))
        mEffects[ref].Stop(transition);
}
template<class T_D, class T_E> bool cEffectTypeValue<T_D, T_E>::IsActive(tEIRef ref) const
{
    if (mSlots.InUse(ref))
        return mEffects[ref].IsActive();

    return false;
}
template<class T_D, class T_E> bool cEffectTypeValue<T_D, T_E>::Update(tEIRef ref, float dt, const cEffectParams* params)
{
    if (mEnabled)
        mEffects[ref].Update(dt, params);
    return true;
}

template<class T_D, class T_E> const char* cEffectTypeValue<T_D, T_E>::StatsString(const char* typeName) const
{
    int numInstances = mSlots.NumSlotsInUse();

    if (numInstances > 0)
    {
        int numActiveInstances = NumActiveInstances();
        
        mStats.format("%d/%d %s", numActiveInstances, numInstances, typeName);
        return mStats.c_str();
    }

    return 0;
}

template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::DebugMenu(cUIState* uiState)
{
    tUIItemID itemID = ItemID(0x022c2a0c);

    int numInstances = mSlots.NumSlotsInUse();
    int numActiveInstances = NumActiveInstances();

    uiState->HandleToggle(itemID++, "Enabled", &mEnabled);
    uiState->DrawLabel(Format("%d / %d Active", numActiveInstances, numInstances));
}

template<class T_D, class T_E> void cEffectTypeValue<T_D, T_E>::RemoveAllInstances()
{
    for (int i = 0, n = mEffects.size(); i < n; i++)
        if (mSlots.InUse(i))
            mEffects[i].Shutdown();

    mEffects.clear();
    mEffectTags.clear();
    mSlots.ClearSlots();
}

