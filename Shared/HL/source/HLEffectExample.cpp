//
//  File:       HLEffectExample.cpp
//
//  Function:   Example effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLEffectExample.h>

#include <CLValue.h>
#include <CLMemory.h>

using namespace nHL;

namespace
{

}

// --- cEffectTypeExample ------------------------------------------------------

#include "HLEffectType.cpp"

namespace
{
    typedef cEffectType<cDescExample, cEffectExample> tEffectTypeExample;

    class cEffectTypeExample :
        public tEffectTypeExample
    {
    public:
        void Init(cIEffectsManager* manager, cIAllocator* alloc) override;
        void Shutdown() override;
        void PreUpdate (float realDT, float gameDT) override;
        void PostUpdate(float realDT, float gameDT) override;
    };

    void cEffectTypeExample::Init(cIEffectsManager* manager, cIAllocator* alloc)
    {
        tEffectTypeExample::Init(manager, alloc);

    }

    void cEffectTypeExample::Shutdown()
    {

        tEffectTypeExample::Shutdown();
    }

    void cEffectTypeExample::PreUpdate(float realDT, float gameDT)
    {
    }
    void cEffectTypeExample::PostUpdate(float realDT, float gameDT)
    {
    }
}

namespace nHL
{
    cIEffectType* CreateEffectTypeExample(cIAllocator* alloc)
    {
        return new(alloc) cEffectType<cDescExample, cEffectExample>;
    }
}


// --- cDescExample ------------------------------------------------------------

void cDescExample::Config(const cValue& config, cIEffectType* type, cIEffectsManager* manager)
{
    mFlags.mLoop = config["loop"].AsBool(mFlags.mLoop);

    mLife = config["life"].AsFloat(mLife);
}


// --- cEffectExample ----------------------------------------------------------

bool cEffectExample::Init(cIEffectType* effectType)
{
    return true;
}

bool cEffectExample::Shutdown()
{
    return true;
}

void cEffectExample::SetDescription(const cDescExample* desc)
{
    mDesc = desc;

    if (!mDesc)
    {
        mFlags.mActive = false;
        return;
    }

    // ...
}

void cEffectExample::SetTransforms(const cTransform& sourceXform, const cTransform& effectXform)
{
}

void cEffectExample::Start(tTransitionType transition)
{
    if (!mDesc)
        return;

    // ...

    mFlags.mActive = true;
}

void cEffectExample::Stop (tTransitionType transition)
{
    if (mFlags.mActive)
    {
        // ...

        mFlags.mActive = false;
    }
}

void cEffectExample::Update(float dt, const cEffectParams* params)
{
}
