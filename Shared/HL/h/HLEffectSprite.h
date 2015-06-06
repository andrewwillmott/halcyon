//
//  File:       HLEffectSprite.h
//
//  Function:   Sprite support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_SPRITE_H
#define HL_EFFECT_SPRITE_H

#include <IHLEffectsManager.h>
#include <HLParticleUtils.h>
#include <CLLink.h>

namespace
{
    class cEffectTypeSprites;
}

namespace nHL
{
    class cIPhysicsController;
    class cIEffectType;
    
    struct cDescSprite
    /// Complete particle system description
    {
        cParticlesDispatchDesc  mDispatch;
        cParticlesPhysicsDesc   mPhysics;

        // Quad setup
        Vec2f mLife = Vec2f(1.0f);

        cBounds3 mEmitDir    = { Vec3f(vl_z), Vec3f(vl_z) };
        Vec2f    mEmitSpeed  = { 0.0f, 0.0f };

        // per-particle variation
        float mSizeVary     = 0.0f;
        float mRotateVary   = 0.0f;
        Vec3f mColourVary   = vl_0;
        float mAlphaVary    = 0.0f;
        float mAspectVary   = 0.0f;

        int mFrameStart     = 0;
        int mFrameCount     = 0;
        int mFrameRandom    = 0;

        tTag mControllerTag = 0;

        int8_t mLayer = 0;  ///< Sort layer
        int8_t mDepth = 1;  ///< Whether to depth sort.

        void Config(const nCL::cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };


    // --- cEffectSprite ----------------------------------------------------

    class cEffectSprite
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescSprite* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const nCL::cParams* params);

        // cEffectSprite
        void CreateSprite(float animScale, const nCL::cParams* params);
        ///< Master create routine -- creates particles, initialises them, adds them to 'particles'.

        // Data declarations
        struct cFlags
        {
            bool mSourceActive  : 1;    ///< Create particles
            bool mEffectActive  : 1;    ///< Whether effect is running
        };

        // Data
        cFlags                  mFlags = { 0 };
        const cDescSprite*      mDesc = 0;
        cEffectTypeSprites*     mTypeManager = 0;
        uint32_t                mParamsModCount = 0;

        uint32_t mRenderHash  = 0;  ///< Hash of render state for sorting
        uint32_t mRenderOrder = 0;  ///< Ordering control for sorting

        Vec3f   mPosition = vl_0;   ///< Position
        Vec3f   mVelocity = vl_0;   ///< Velocity

        Vec3f   mColour = vl_0;
        float   mAlpha  = vl_0;

        float   mSize       = vl_0;
        float   mRotation   = vl_0;
        float   mAspect     = vl_0;

        uint8_t mFrame      = 0;

        tPtAge  mAge        = 0;
        tPtAge  mAgeStep    = 0;

        cDispatchScale mDispatchScale;

        cTransform mSourceToEffect;
        cTransform mEffectToWorld;

        int mMaterial1 = -1;
        int mMaterial2 = -1;

        int mTexture1 = -1;
        int mTexture2 = -1;

        cLink<cIPhysicsController> mController;

        const nCL::cParams* mDispatchParams = 0;
    };

    void SetupCreateParams(const nCL::cParams* params, cParticlesCreateScale* createScale); ///< Fold in scale factors from params.


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectSprite::IsActive() const
    {
        return mFlags.mEffectActive;
    }
}

#endif
