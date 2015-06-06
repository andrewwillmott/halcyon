//
//  File:       HLEffectsManager.cpp
//
//  Function:   Manager for audiovisual effects.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectsManager.h>

#include <HLEffectType.h>

#include <IHLConfigManager.h>
#include <HLServices.h>
#include <HLUI.h>

#include <CLLog.h>
#include <CLTimer.h>
#include <CLValue.h>

#include <GLConfig.h> // for vertex format constants

using namespace nHL;
using namespace nCL;

namespace
{
    const cTransform kNullTransform;
    const cEffectInstance::cFlags kNullEffectInstanceFlags = { 0 };

    const cEffectInstance kNullEffectInstance;

    struct cPlaneCollider :
        public cIPhysicsController,
        cAllocLinkable
    {
        Vec3f mPlaneNormal = vl_z;
        float mPlaneD = 0.0f;

        int Link(int count) const override { return cAllocLinkable::Link(count); }

        void Update(const cTransform& xform, int count, const float dts[], size_t dtStride, Vec3f positions[], size_t positionStride, Vec3f velocities[], size_t velocityStride) override
        {
            for (int i = 0; i < count; i++)
            {
                float distanceToPlane = -(dot(mPlaneNormal, *positions) + mPlaneD);

                if (distanceToPlane < 0.0f)
                {
                    
                }


                ((uint8_t*&) positions)     += positionStride;
                ((uint8_t*&) velocities)    += velocityStride;
            }
        }
    };
}

// --- cEffectsManager ---------------------------------------------------------

cEffectsManager::cEffectsManager()
{
}

cEffectsManager::~cEffectsManager()
{
}

namespace nHL
{
    cIEffectType* CreateEffectTypeGroup    (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeParticles(cIAllocator* alloc);
    cIEffectType* CreateEffectTypeRibbon   (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeSprite   (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeShake    (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeScreen   (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeSound    (cIAllocator* alloc);
    cIEffectType* CreateEffectTypeExample  (cIAllocator* alloc);
}

bool cEffectsManager::Init()
{
    mAllocator = AllocatorFromObject(this);

    // built-ins
    RegisterEffectType(kEffectGroup,        CreateEffectTypeGroup    (mAllocator), CL_TAG("group"),     CL_TAG("groups"));
    RegisterEffectType(kEffectParticles,    CreateEffectTypeParticles(mAllocator), CL_TAG("particles"), CL_TAG("particles"));
    RegisterEffectType(kEffectRibbon,       CreateEffectTypeRibbon   (mAllocator), CL_TAG("ribbon"),    CL_TAG("ribbons"));
    RegisterEffectType(kEffectSprite,       CreateEffectTypeSprite   (mAllocator), CL_TAG("sprite"),    CL_TAG("sprites"));
    RegisterEffectType(kEffectShake,        CreateEffectTypeShake    (mAllocator), CL_TAG("shake"),     CL_TAG("shakes"));
    RegisterEffectType(kEffectScreen,       CreateEffectTypeScreen   (mAllocator), CL_TAG("screen"),    CL_TAG("screens"));
    RegisterEffectType(kEffectSound,        CreateEffectTypeSound    (mAllocator), CL_TAG("sound"),     CL_TAG("sounds"));
    RegisterEffectType(kEffectExample,      CreateEffectTypeExample  (mAllocator), CL_TAG("example"),   CL_TAG("examples"));

    RegisterPhysicsController(CL_TAG("plane"), new (mAllocator) cPlaneCollider);

    return true;
}

bool cEffectsManager::PostInit()
{
    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i])
            mEffectTypes[i]->PostInit();

    return true;
}

bool cEffectsManager::Shutdown()
{
    RemoveAllInstances();
    
    for (int i = 0; i < kMaxEffectTypes; i++)
    {
        if (mEffectTypes[i])
        {
            mEffectTypes[i]->Shutdown();
            mEffectTypes[i] = 0;
        }
    }

    return true;
}

bool cEffectsManager::LoadEffects(const cObjectValue* effectsConfig)
{
    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i])
        {
            const cObjectValue* config = effectsConfig->Member(mEffectSetTags[i]).AsObject();

            if (config)
            {
                CL_LOG("Effects", "Adding " CL_TAG_FMT ":\n", mEffectSetTags[i]);
                mEffectTypes[i]->Config(config);
            }
        }

    return true;
}

void cEffectsManager::Update(float realDT, float gameDT)
{
    if (!mEnabled)
    {
        mPostMSPF = 0.0f;
        mOverallMSPF = 0.0f;
        return;
    }

    cProgramTimer timer;
    timer.Start();

    // Update the types first, in case they need to do some global processing.
    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i])
            mEffectTypes[i]->PreUpdate(realDT, gameDT);

    UpdateMSPF(timer.DeltaTime(), &mPreMSPF);

    for (int i = 0, n = mInstanceEffects.size(); i < n; i++)
    {
        if (!mInstanceEffects[i].mRef.IsNull())
        {
            cEffectInstance& instance = mInstanceEffects[i];

            CL_INDEX(instance.mType, kMaxEffectTypes);
            cIEffectType* effectType = mEffectTypes[instance.mType];

            if (effectType->IsActive(instance.mRef))
            {
                effectType->SetTransforms(instance.mRef, instance.mSourceTransform, Transform(mParams.mTransform, instance.mEffectTransform));

                if (instance.mFlags.mPaused)
                    ; // effectType->Update(instance.mRef, 0.0f, instance.mParams);
                else
                {
                    if (instance.mFlags.mRealTime)
                        effectType->Update(instance.mRef, realDT, instance.mParams);
                    else
                        effectType->Update(instance.mRef, gameDT, instance.mParams);

                    if (instance.mParams->HasData())
                        instance.mParams->ClearData();
                }
            }
            else if (mInstanceEffects[i].mFlags.mOneShot)
                DestroyInstance(mInstanceSlots.RefFromIndex(i));
        }
    }

    timer.DeltaTime();

    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i])
            mEffectTypes[i]->PostUpdate(realDT, gameDT);

    UpdateMSPF(timer.DeltaTime(), &mPostMSPF);

    UpdateMSPF(timer.GetTime(), &mOverallMSPF);
}


tEIRef cEffectsManager::CreateInstance(tTag tag)
{
    tEffectType type = kMaxEffectTypes;
    
    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i] && mEffectTypes[i]->HasEffect(tag))
        {
            type = tEffectType(i);
            break;
        }

    if (type == kMaxEffectTypes)
        return kNullRef;

    tEIRef result = mInstanceSlots.CreateSlot();

    mInstanceEffects.resize(mInstanceSlots.NumSlots());

    cEffectInstance& instance = mInstanceEffects[result];

    instance.mTag = tag;

    instance.mType = type;
    instance.mRef = mEffectTypes[type]->CreateInstance(instance.mTag);

    CL_ASSERT(!instance.mParams);
    instance.mParams = new(mAllocator) cEffectParamsAlloc;

    return result;
}

bool cEffectsManager::DestroyInstance(tEIRef ref)
{
    bool result =  mInstanceSlots.DestroySlot(ref);

    if (result)
    {
        int slot = ref;

        cEffectInstance& instance = mInstanceEffects[slot];

        if (instance.mType != kMaxEffectTypes)
            mEffectTypes[instance.mType]->DestroyInstance(instance.mRef);

        instance = kNullEffectInstance;
    }

    return result;
}

bool cEffectsManager::DestroyInstanceOnStop(tEIRef ref)
{
    if (IsActive(ref))
    {
        // We know this must be a valid reference at this point.
        mInstanceEffects[ref].mFlags.mOneShot = true;
        return false;
    }
    else
        return DestroyInstance(ref);
}

tEIRef cEffectsManager::CreateOneShotInstance(tTag tag)
{
    tEIRef result = CreateInstance(tag);

    if (!result.IsNull())
        mInstanceEffects[result].mFlags.mOneShot = true;

    return result;
}


void cEffectsManager::RemoveAllInstances()
{
    mInstanceSlots.ClearSlots();

    for (int i = 0, n = mInstanceEffects.size(); i < n; i++)
    {
        cEffectInstance& instance = mInstanceEffects[i];

        if (!instance.mRef.IsNull())
            mEffectTypes[instance.mType]->DestroyInstance(instance.mRef);
    }

    mInstanceEffects.clear();
}

void cEffectsManager::SetSourceTransform(tEIRef ref, const cTransform& xform)
{
    CL_ASSERT(!HasNAN(xform));

    if (mInstanceSlots.InUse(ref))
        mInstanceEffects[ref].mSourceTransform = xform;
}

const cTransform& cEffectsManager::SourceTransform(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return mInstanceEffects[ref].mSourceTransform;

    return kNullTransform;
}

void cEffectsManager::SetEffectTransform(tEIRef ref, const cTransform& xform)
{
    CL_ASSERT(!HasNAN(xform));

    if (mInstanceSlots.InUse(ref))
        mInstanceEffects[ref].mEffectTransform = xform;
}

const cTransform& cEffectsManager::EffectTransform(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return mInstanceEffects[ref].mEffectTransform;

    return kNullTransform;
}

void cEffectsManager::StartSources(tEIRef ref)
{
    if (mInstanceSlots.InUse(ref))
        if (!mInstanceEffects[ref].mRef.IsNull())
            mEffectTypes[mInstanceEffects[ref].mType]->Start(mInstanceEffects[ref].mRef, kTransitionSource);
}

void cEffectsManager::StopSources(tEIRef ref)
{
    if (mInstanceSlots.InUse(ref))
        if (!mInstanceEffects[ref].mRef.IsNull())
            mEffectTypes[mInstanceEffects[ref].mType]->Stop(mInstanceEffects[ref].mRef, kTransitionSource);
}

void cEffectsManager::StartEffect(tEIRef ref)
{
    if (mInstanceSlots.InUse(ref))
        if (!mInstanceEffects[ref].mRef.IsNull())
            mEffectTypes[mInstanceEffects[ref].mType]->Start(mInstanceEffects[ref].mRef, kTransitionImmediate);
}

void cEffectsManager::StopEffect(tEIRef ref)
{
    if (mInstanceSlots.InUse(ref))
        if (!mInstanceEffects[ref].mRef.IsNull())
            mEffectTypes[mInstanceEffects[ref].mType]->Stop(mInstanceEffects[ref].mRef, kTransitionImmediate);
}

bool cEffectsManager::IsActive(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return mEffectTypes[mInstanceEffects[ref].mType]->IsActive(mInstanceEffects[ref].mRef);

    return false;
}

void cEffectsManager::SetVisible(tEIRef ref, bool visible)
{
    if (mInstanceSlots.InUse(ref) && mInstanceEffects[ref].mFlags.mHidden != !visible)
    {
        mInstanceEffects[ref].mFlags.mHidden = !visible;
        // mEffectTypes[mInstanceEffects[ref].mType]->SetVisible(mInstanceEffects[ref].mRef, enabled);
    }
}

bool cEffectsManager::Visible(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return true;

    return false;
}

void cEffectsManager::SetPaused(tEIRef ref, bool paused)
{
    if (mInstanceSlots.InUse(ref))
        mInstanceEffects[ref].mFlags.mPaused = paused;
}

bool cEffectsManager::Paused(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return mInstanceEffects[ref].mFlags.mPaused;

    return false;
}

void cEffectsManager::SetRealTime(tEIRef ref, bool enabled)
{
    if (mInstanceSlots.InUse(ref))
        mInstanceEffects[ref].mFlags.mRealTime = enabled;
}

bool cEffectsManager::RealTime(tEIRef ref) const
{
    if (mInstanceSlots.InUse(ref))
        return mInstanceEffects[ref].mFlags.mRealTime;

    return false;
}



void cEffectsManager::CreateInstances(int count, const tTag tags[], tEIRef refs[])
{
    for (int i = 0; i < count ; i++)
    {
        tEffectType type = kMaxEffectTypes;

        refs[i] = kNullRef;

        for (int i = 0; i < kMaxEffectTypes; i++)
            if (mEffectTypes[i] && mEffectTypes[i]->HasEffect(tags[i]))
            {
                type = tEffectType(i);
                refs[i] = mInstanceSlots.CreateSlot();

                mInstanceEffects.resize(mInstanceSlots.NumSlots());
                mInstanceEffects[refs[i]].mType = type;
                break;
            }
    }

    for (int i = 0; i < count ; i++)
        if (refs[i] != kNullRef)
        {
            int slot = refs[i];
            cEffectInstance& instance = mInstanceEffects[slot];

            instance.mTag = tags[i];
            instance.mFlags = kNullEffectInstanceFlags;

            if (instance.mType != kMaxEffectTypes)
                instance.mRef = mEffectTypes[instance.mType]->CreateInstance(instance.mTag);

            CL_ASSERT(!instance.mParams);
            instance.mParams = new(mAllocator) cEffectParamsAlloc;
        }
}

void cEffectsManager::DestroyInstances(int count, tEIRef effectRefs[])
{
    for (int i = 0; i < count; i++)
    {
        bool result =  mInstanceSlots.DestroySlot(effectRefs[i]);

        if (result)
        {
            int slot = effectRefs[i];

            if (mInstanceEffects[slot].mType != kMaxEffectTypes)
                mEffectTypes[mInstanceEffects[slot].mType]->DestroyInstance(mInstanceEffects[slot].mRef);

            mInstanceEffects[slot] = kNullEffectInstance;

            effectRefs[i] = kNullRef;
        }
    }
}

inline void cEffectsManager::AddData(tEIRef ref, cIEffectData* data)
{
    if (mInstanceSlots.InUse(ref))
        mInstanceEffects[ref].mParams->AddData(data);
    else
    {
        // Simulate retain & release
        data->Link(1);
        data->Link(-1);
    }
}

tTag cEffectsManager::EffectTag(tEIRef ref)
{
    if (mInstanceSlots.InUse(ref))
        return mInstanceEffects[ref].mTag;

    return 0;
}

const cObjectValue* cEffectsManager::EffectConfig(tEIRef ref)
{
//    if (mInstanceSlots.InUse(ref))
//        return mInstanceEffects[ref].mTag;

    return 0;
}

void cEffectsManager::RegisterPhysicsController(tTag tag, cIPhysicsController* controller)
{
    mPhysicsControllers[tag] = controller;
}

cIPhysicsController* cEffectsManager::PhysicsController(tTag tag) const
{
    CL_ASSERT(IsTag(tag));

    const auto it = mPhysicsControllers.find(tag);

    if (it != mPhysicsControllers.end())
        return it->second;

    return 0;
}

void cEffectsManager::RegisterEffectType(tEffectType type, cIEffectType* manager, tTag tag, tTag setTag)
{
    CL_ASSERT(IsTag(tag));
    CL_ASSERT(IsTag(setTag));

    if (mEffectTypes[type])
        mEffectTypes[type]->Shutdown();

    if (!setTag)
        setTag = tag;

    mEffectTypes   [type] = manager;
    mEffectTypeTags[type] = tag;
    mEffectSetTags [type] = setTag;

    if (mEffectTypes[type])
        mEffectTypes[type]->Init(this, mAllocator);
}

tTag cEffectsManager::EffectTypeTag(tEffectType type)
{
    return mEffectTypeTags[type];
}

tEffectType cEffectsManager::EffectTypeFromTag(tTag tag)
{
    CL_ASSERT(IsTag(tag));

    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypeTags[i] == tag)
            return tEffectType(i);

    return kMaxEffectTypes;
}

const char* cEffectsManager::StatsString() const
{
    if (!mShowStats)
        return 0;

    mStats.clear();

    mStats.format("%1.1f mspf", mOverallMSPF);

    for (int i = 0; i < kMaxEffectTypes; i++)
    {
        if (mEffectTypes[i])
        {
            const char* typeStats = mEffectTypes[i]->StatsString(mEffectSetTags[i]);

            if (typeStats)
            {
                if (!mStats.empty())
                    mStats.append(", ");

                mStats += typeStats;
            }
        }
    }

    return mStats.c_str();
}

void cEffectsManager::DebugMenu(nHL::cUIState* uiState)
{
    tUIItemID itemID = ItemID(0x022c2724);

    uiState->HandleToggle(itemID++, "Enabled", &mEnabled);
    uiState->HandleToggle(itemID++, "Stats", &mShowStats);

    uiState->DrawSeparator();

    for (int i = 0; i < kMaxEffectTypes; i++)
        if (mEffectTypes[i] && uiState->BeginSubMenu(itemID + i, mEffectTypeTags[i]))
        {
            mEffectTypes[i]->DebugMenu(uiState);
            uiState->EndSubMenu();
        }
}

cIEffectsManager* nHL::CreateEffectsManager(cIAllocator* alloc)
{
    return new(alloc) cEffectsManager;
}
