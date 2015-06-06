//
//  File:       HLEffectShake.h
//
//  Function:   Screen shake effect
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_SHAKE_H
#define HL_EFFECT_SHAKE_H

#include <IHLEffectsManager.h>
#include <CLLink.h>

namespace nHL
{
    class cIEffectType;
    typedef int8_t tShakeTypeStore;

    enum tShakeType : tShakeTypeStore
    {
        kShakeRandom,
        kShakeSine,
        kMaxShakeTypes
    };

    struct cDescShake
    {
        tShakeType mType = kShakeRandom;

        float mLife     = 2.0f;
        float mRelease  = 0.0f;
        float mAspect   = 1.0f;

        nCL::vector<float> mAmplitudeFrames;
        nCL::vector<float> mFrequencyFrames;

        void Config(const cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };

    struct cEffectShake
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescShake* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const nCL::cParams* params);

    protected:
        struct cFlags
        {
            bool mActive    : 1;
        };

        cFlags      mFlags      =  { 0 };

        const cDescShake* mDesc = 0;

        float       mAgeDelta   = 0.0f;
        Vec2f       mScaleXY    = vl_1;

        float       mAge        = 0.0f;
        float       mCycle      = 0.0f;

        cEffectsManagerParams* mParams = 0;
    };


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectShake::IsActive() const
    {
        return mFlags.mActive && mDesc;
    }
}

#endif
