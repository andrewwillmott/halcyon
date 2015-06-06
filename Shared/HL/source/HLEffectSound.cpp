//
//  File:       HLEffectSound.cpp
//
//  Function:   Sound effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectSound.h>

#include <CLValue.h>
#include <CLMemory.h>

using namespace nHL;

namespace
{

}

// --- cEffectTypeSound ------------------------------------------------------

#include "HLEffectType.cpp"

typedef cEffectTypeValue<cDescSound, cEffectSound> tEffectTypeSound;

namespace nHL
{
    cIEffectType* CreateEffectTypeSound(cIAllocator* alloc)
    {
        return new(alloc) tEffectTypeSound();
    }
}


// --- cDescSound ------------------------------------------------------------

void cDescSound::Config(const cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mSoundTag = config["sound"].AsTag();
    mGroupTag = config["group"].AsTag();

    mFlags.mStopWithEffect = config["stopWithEffect"].AsBool(mFlags.mStopWithEffect);

    // Not used yet
    mFlags.mLoop = config["loop"].AsBool(mFlags.mLoop);
    mLife     = config["life"].AsFloat(mLife);
}


// --- cEffectSound ----------------------------------------------------------

bool cEffectSound::Init(cIEffectType* effectType)
{
    return true;
}

bool cEffectSound::Shutdown()
{
    Stop();
    CL_ASSERT(!mDesc->mFlags.mStopWithEffect || mPlayRef < 0);

    return true;
}

void cEffectSound::SetDescription(const cDescSound* desc)
{
    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }

    auto manager = HL()->mAudioManager;

    mSoundRef = manager->SoundRefFromTag(mDesc->mSoundTag);
    mGroupRef = manager->GroupRefFromTag(mDesc->mGroupTag);

    if (mGroupRef < 0)
        mGroupRef = kDefaultGroupRef;
}

void cEffectSound::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
}

void cEffectSound::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mFlags.mActive = true;
}

void cEffectSound::Stop (tTransitionType transition)
{
    if (mFlags.mActive)
    {
        if (mDesc->mFlags.mStopWithEffect)
            HL()->mAudioManager->StopSound(mPlayRef);

        mPlayRef = kNullAudioPlayRef;  // TODO: do we need an addref/release style thing?
        mFlags.mActive = false;
    }
}

void cEffectSound::Update(float dt, const nCL::cParams* params)
{
    auto manager = HL()->mAudioManager;

    if (manager && mPlayRef < 0)
    {
        mPlayRef = manager->PlaySound(mSoundRef, mGroupRef);

        if (!mDesc->mFlags.mStopWithEffect)
            mFlags.mActive = false;
    }
}
