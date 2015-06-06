//
//  File:       HLEffectExample.h
//
//  Function:   Example effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_EXAMPLE_H
#define HL_EFFECT_EXAMPLE_H

#include <HLEffectType.h>
#include <CLMemory.h>

namespace nHL
{
    class cIEffectType;

    struct cDescExample
    {
        struct cFlags
        {
            bool mLoop    : 1;
        };

        cFlags mFlags = { 0 };

        float mLife     = 2.0f;

        void Config(const cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };

    struct cEffectExample :
        public nCL::cAllocLinkable
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescExample* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const cEffectParams* params);

    protected:
        struct cFlags
        {
            bool mActive    : 1;
        };

        cFlags mFlags =  { 0 };

        const cDescExample* mDesc = 0;
    };


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectExample::IsActive() const
    {
        return mFlags.mActive;
    }
}

#endif
