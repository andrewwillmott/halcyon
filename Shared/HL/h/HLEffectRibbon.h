//
//  File:       HLEffectRibbon.h
//
//  Function:   Ribbon effect implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_RIBBON_H
#define HL_EFFECT_RIBBON_H

#include <HLEffectType.h>
#include <HLParticleUtils.h>
#include <CLMemory.h>

namespace
{
    class cEffectTypeRibbon;
}

namespace nHL
{
    class cIEffectType;

    struct cDescRibbon
    {
        struct cFlags
        {
            bool mUVRepeat  : 1;
            bool mHasForces : 1;
            bool mSustain   : 1;    // hold at end of first cycle rather than looping
        };

        cFlags mFlags = { 0 };

        float   mLength         = 0.05f;
        int     mMaxSegments    = 32;
        float   mUVRepeat       = 1.0f;
        float   mUVSpeed        = 0.0f;
        float   mFadeRate       = 0.0f;
        float   mCycleTime      = 1.0f;

        cParticlesDispatchDesc mDispatch;
        cParticlesPhysicsDesc  mPhysics;

        int8_t  mLayer = 0;  ///< Sort layer
        int8_t  mDepth = 1;  ///< Whether to depth sort.

        void Config(const cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };

    struct cEffectRibbon :
        public nCL::cAllocLinkable
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescRibbon* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const cEffectParams* params);

        uint32_t mRenderHash  = 0;  ///< Hash of render state for sorting
        uint32_t mRenderOrder = 0;  ///< Ordering control for sorting

    //protected:
        friend class ::cEffectTypeRibbon;

        void UpdatePositionHistory(float deltaSeconds);

        struct cFlags
        {
            bool mActive    : 1;
        };

        cFlags mFlags =  { 0 };

        const cDescRibbon* mDesc = 0;

        float mLife = 0.0f;
        float mInvLife = 0.0f;
        float mAge = 0.0f;

        int                 mNumSegments        = 0;
        Vec3f               mRoot               = vl_0;
        bool                mRootValid          = false;

        nCL::vector<Vec3f>  mPositions;                  ///< Segment positions
        bool                mHaveAllPositions   = false;

        nCL::vector<float>  mFades;                      ///< fade segments over time

        Vec3f               mDirForces          = vl_0;
        float               mUVOffset           = 0.0f;

        cTransform mEffectTransform;

        int mMaterial1 = -1;
        int mMaterial2 = -1;

        int mTexture1 = -1;
        int mTexture2 = -1;
    };

    void DispatchRibbon
    (
        cIRenderer*         renderer,
        int                 quadMesh,

        const cDescRibbon& desc,
        const cEffectRibbon* effect,

        const cTransform& effectTransform,
        const Mat3f& cameraOrient
    );

    // --- Inlines -------------------------------------------------------------

    inline bool cEffectRibbon::IsActive() const
    {
        return mFlags.mActive;
    }
}

#endif
