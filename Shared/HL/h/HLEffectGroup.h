//
//  File:       HLEffectGroup.h
//
//  Function:   Controls a group of child effects
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_GROUP_H
#define HL_EFFECT_GROUP_H

#include <HLEffectType.h>

#include <CLMemory.h>
#include <CLRandom.h>
#include <CLTransform.h>

namespace nCL
{
    class cIAllocator;
    class cTransform;
}

namespace nHL
{
    class cIEffectType;
    class cEffectsManager;

    struct cDescGroupEffect
    {
        struct cFlags
        {
            bool mRigid : 1;
            bool mImmediateStart : 1;
            bool mImmediateStop  : 1;
        };

        cFlags mFlags = { 0 };

        tEffectTypeStore    mType       = kMaxEffectTypes;
        tTag                mTag        = 0;
        cTransform          mTransform;
        float               mTimeScale  = 1.0f;

        // TODO: LOD scales
        // TODO: size/alpha scale

        void Config(const nCL::cValue& config, cIEffectsManager* manager);
    };


    struct cDescGroup
    {
        struct cFlags
        {
            bool mRigid          : 1;
            bool mImmediateStart : 1;
            bool mImmediateStop  : 1;
        };

        cFlags mFlags = { 0 };

        nCL::vector<cDescGroupEffect> mEffects;

        void Config(const nCL::cValue& config, cIEffectType* type, cIEffectsManager* manager);
    };


    struct cActiveEffect
    {
        int                 mDescIndex  = -1; // index in mEffects
        tEffectType         mType       = kMaxEffectTypes;      // duplicated for convenience
        cSlotRef            mRef;       // ref to controlled effect
        cTransform          mTransform; // current transform
        float               mTimeScale  = 1.0f;

        // TODO: LOD scales
        // TODO: size/alpha scale
    };


    class cEffectGroup :
        public nCL::cAllocLinkable
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* type);
        bool Shutdown();

        void SetDescription(const cDescGroup* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition);
        void Stop (tTransitionType transition);
        bool IsActive() const;
        void Update(float dt, const cEffectParams* params);

        void SetSourceToEffect(const cTransform& xform);
        void SetEffectToWorld (const cTransform& xform);

        // cEffectGroup
        void UpdateLOD(int newLOD);
        bool CheckIfStillActive();

    protected:
        void UpdateTransforms();

        // Data declarations
        struct cFlags
        {
            bool mActive            : 1;
            bool mSourcesActive     : 1;
            bool mHidden            : 1;
            bool mTransformsDirty   : 1;
        };

        // Data
        cFlags              mFlags = { 0 };
        const cDescGroup*   mDesc = 0;
        cEffectsManager*    mManager = 0;

        cTransform      mSourceToEffect;
        cTransform      mEffectToWorld;
        tTransitionType mStartTransition = kMaxTransitions;
        int             mLOD = -1;


        nCL::vector<cActiveEffect> mActiveEffects;
    };


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectGroup::IsActive() const
    {
        return mFlags.mActive;
    }

}

#endif
