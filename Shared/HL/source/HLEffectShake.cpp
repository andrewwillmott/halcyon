//
//  File:       HLEffectShake.cpp
//
//  Function:   Screen shake effect
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectShake.h>

#include <IHLRenderer.h>

#include <HLParticleUtils.h>
#include <HLServices.h>

#include <CLMath.h>
#include <CLMemory.h>
#include <CLRandom.h>
#include <CLValue.h>

using namespace nHL;
using namespace nCL;


// --- tEffectTypeShake --------------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    // Constants
    const int kShakeTableSize = 128; // How many shake offsets in our table

    Vec2f sShakeTable[kMaxShakeTypes][kShakeTableSize];

    void CreateShakeTables()
    {
        tSeed32 seed = kDefaultSeed32;

        float ds = vl_twoPi / kShakeTableSize;

        for (int i = 0; i < kShakeTableSize; i++)
        {
            sShakeTable[kShakeRandom][i] = { RandomSFloat(&seed), RandomSFloat(&seed) };
            sShakeTable[kShakeSine]  [i] = { 0.0f, sinf(i * ds) };
        }
    }

    typedef cEffectTypeValue<cDescShake, cEffectShake> tEffectTypeShake;

    class cEffectTypeShake : public tEffectTypeShake
    {
    public:
        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void PreUpdate (float realDT, float gameDT) override;
        void PostUpdate(float realDT, float gameDT) override;
    };

    void cEffectTypeShake::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeShake::Init(manager, alloc);

        CreateShakeTables();
    }

    void cEffectTypeShake::PreUpdate(float realDT, float gameDT)
    {
        mManager->Params()->mViewOffset = vl_0;
    }
    void cEffectTypeShake::PostUpdate(float realDT, float gameDT)
    {
        mRenderer->SetShaderDataT(kDataIDViewOffset, mManager->Params()->mViewOffset);
    }
}

namespace nHL
{
    cIEffectType* CreateEffectTypeShake(cIAllocator* alloc)
    {
        return new(alloc) cEffectTypeShake;
    }
}


// --- cDescShake --------------------------------------------------------------

namespace
{
    cEnumInfo kEnumShakeType[] =
    {
        "random",       kShakeRandom,
        "sine",         kShakeSine,
        0, 0
    };
}


void cDescShake::Config(const cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mType = tShakeType(AsEnum(config["type"], kEnumShakeType, mType));

    SetFromValue(config["amplitude"],  &mAmplitudeFrames);
    SetFromValue(config["frequency"],  &mFrequencyFrames);

    mLife    = config["life"   ].AsFloat(mLife);
    mRelease = config["release"].AsFloat(mRelease);
    mAspect  = config["aspect" ].AsFloat(mAspect);
}

// --- cEffectShake ------------------------------------------------------------

bool cEffectShake::Init(cIEffectType* effectType)
{
    tEffectTypeShake* shakeType = static_cast<tEffectTypeShake*>(effectType);

    mParams = shakeType->Manager()->Params();

    return true;
}

bool cEffectShake::Shutdown()
{
    return true;
}

void cEffectShake::SetDescription(const cDescShake* desc)
{
    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }

    mAgeDelta = 1.0f / desc->mLife;
    mScaleXY = Vec2f(1.0f / desc->mAspect, desc->mAspect);
}

void cEffectShake::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
}

void cEffectShake::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    mAge   = 0.0f;
    mCycle = 0.0f;

    mFlags.mActive = true;
}

void cEffectShake::Stop (tTransitionType transition)
{
    if (mFlags.mActive)
    {


        mFlags.mActive = false;
    }
}

void cEffectShake::Update(float dt, const nCL::cParams* params)
{
    mAge += dt * mAgeDelta;

    if (mAge >= 1.0f)
    {
        mAge = 1.0f - 1e-6f;
        mFlags.mActive = false;
    }

    // look up the amplitude, and the frequency, using the latter to update
    // our cycle count.
	float amplitude = LinearAnim(mDesc->mAmplitudeFrames.size(), mDesc->mAmplitudeFrames.data(), mAge);

	Vec2f ampXY(amplitude * mScaleXY);

	float frequency = LinearAnim(mDesc->mFrequencyFrames.size(), mDesc->mFrequencyFrames.data(), mAge);

	mCycle += frequency * dt * kShakeTableSize;

    while (mCycle >= kShakeTableSize)
        mCycle -= kShakeTableSize;

    int i0 = FloorToSInt32(mCycle);
    int i1 = (i0 + 1) % kShakeTableSize;
    float s = mCycle - i0;

	// use cycle fraction to look up our base shake table.
    const Vec2f* table = sShakeTable[mDesc->mType];

	Vec2f viewOffset = lerp(table[i0], table[i1], s) * ampXY;

    mParams->mViewOffset += viewOffset;
}

