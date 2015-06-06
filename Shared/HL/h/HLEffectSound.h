//
//  File:       HLEffectSound.h
//
//  Function:   Sound effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_SOUND_H
#define HL_EFFECT_SOUND_H

#include <IHLEffectsManager.h>
#include <IHLAudioManager.h>

#include <CLLink.h>

namespace nHL
{
    class cIEffectType;

    struct cDescSound
    {
        struct cFlags
        {
            bool mLoop    : 1;
            bool mStopWithEffect : 1;
        };

        cFlags mFlags = { 0 };

        float   mLife = 2.0f;
        tTag    mSoundTag  = 0;
        tTag    mGroupTag  = 0;

        void Config(const cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };

    struct cEffectSound
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescSound* desc);

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

        cFlags mFlags =  { 0 };

        const cDescSound* mDesc = 0;
        int mSoundRef = -1;
        int mGroupRef = -1;
        tAudioPlayRef mPlayRef = kNullAudioPlayRef;
    };


    // --- Inlines -------------------------------------------------------------

    bool cEffectSound::IsActive() const
    {
        return mFlags.mActive;
    }
}

#endif
