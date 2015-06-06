//
//  File:       HLEffectParticles.h
//
//  Function:   Particle system management
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_EFFECT_PARTICLES_H
#define HL_EFFECT_PARTICLES_H

#include <HLEffectType.h>
#include <HLParticleUtils.h>
#include <CLBounds.h>
#include <CLLink.h>

namespace
{
    class cEffectTypeParticles;
}

namespace nHL
{
    class cIPhysicsController;
    class cIEffectType;
    
    struct cDescParticles
    /// Complete particle system description
    {
        cParticlesCreateDesc    mCreate;
        cParticlesDispatchDesc  mDispatch;
        cParticlesPhysicsDesc   mPhysics;

        // Extended config stuff
        tTag    mControllerTag = 0;

        int8_t  mLayer = 0;  ///< Sort layer
        int8_t  mDepth = 1;  ///< Whether to depth sort.

        void Config(const nCL::cValue& v, cIEffectType* type, cIEffectsManager* manager);
    };


    struct cStandardParticleArrays
    // SOA particle data management
    {
        Vec3f*   mPosition;   ///< Position
        Vec3f*   mVelocity;   ///< Velocity

        Vec3f*   mColour;
        float*   mAlpha;

        float*   mSize;
        float*   mRotation;
        float*   mAspect;

        uint8_t* mFrames;

        tPtAge*  mAge;
        tPtAge*  mAgeStep;      ///< Per-particle life
    };

    struct cStandardParticlesAlloc
    {
        bool    mColour     : 1;
        bool    mAlpha      : 1;
        bool    mSize       : 1;
        bool    mRotation   : 1;
        bool    mAspect     : 1;
        bool    mFrames     : 1;
    };

    struct cStandardParticles : public cStandardParticleArrays
    /// Extended version with info about what arrays to allocate.
    {
        cStandardParticlesAlloc mAlloc = { 0 };
    };

    typedef cMemArray<cStandardParticles> tStandardParticles;

    // Particle array utilities
    void CopyArray (int count, const cStandardParticles& a, cStandardParticles* b, int aStart = 0, int bStart = 0);
    void InitArray (int count, cStandardParticles* p, int start);
    void AllocArray(nCL::cIAllocator* alloc, int count, cStandardParticles* p);
    void FreeArray (nCL::cIAllocator* alloc, cStandardParticles* p);

    void CompactParticles(tStandardParticles* p);   ///< Remove any dead particles
    void AllocNewArrays  (tStandardParticles* p);   ///< Allocate any new arrays that haven't been created yet


    // --- cEffectParticles ----------------------------------------------------

    class cEffectParticles :
        public nCL::cAllocLinkable
    {
    public:
        // Standard API for cEffectType
        bool Init(cIEffectType* effectType);
        bool Shutdown();

        void SetDescription(const cDescParticles* desc);

        void SetTransforms(const cTransform& sourceXform, const cTransform& effectXform);

        void Start(tTransitionType transition = kTransitionSource);
        void Stop (tTransitionType transition = kTransitionSource);
        bool IsActive() const;

        void Update(float dt, const cEffectParams* params);

        // cEffectParticles
        int NumParticles() const;     ///< Returns the number of particles currently alive.

        uint32_t mRenderHash  = 0;  ///< Hash of render state for sorting
        uint32_t mRenderOrder = 0;  ///< Ordering control for sorting

    protected:
        friend class ::cEffectTypeParticles;
        
        int CreateParticles(float dt, float animScale, const cEffectParams* params);
        ///< Master create routine -- creates particles, initialises them, adds them to 'particles'.

        // Data declarations
        struct cFlags
        {
            bool mSourceActive  : 1;    ///< Create particles
            bool mEffectActive  : 1;    ///< Whether effect is running
            bool mLoopsActive   : 1;    ///< Whether particle looping is allowed
            bool mPreroll       : 1;
        };

        // Data
        cFlags                  mFlags = { 0 };
        const cDescParticles*   mDesc = 0;
        cEffectTypeParticles*   mTypeManager = 0;
        uint32_t                mParamsModCount = 0;

        nCL::cBounds3           mBounds;        //!< Particles bounding box in world space, used for culling etc.

        tStandardParticles      mParticles;
        cParticlesState         mState;
        cDispatchScale          mDispatchScale;

        cTransform mSourceToEffect;
        cTransform mEffectToWorld;

        int mMaterial1 = -1;
        int mMaterial2 = -1;

        int mTexture1 = -1;
        int mTexture2 = -1;

        cLink<cIPhysicsController> mController;

        const nCL::cParams* mDispatchParams = 0;
    };


    // --- Utilities -------------------------------------------------------------

    void SetupCreateParams(const nCL::cParams* params, cParticlesCreateScale* createScale); ///< Fold in scale factors from params.
    cIEffectData* CreateParticlesEffectData(int numParticles, const Vec3f positions[], const Vec3f velocities[] = 0);   ///< Bundle up new particle positions/velocities for adding to effect.


    // --- Inlines -------------------------------------------------------------

    inline bool cEffectParticles::IsActive() const
    {
        return mFlags.mEffectActive;
    }

    inline int cEffectParticles::NumParticles() const
    {
        return mParticles.Size();
    }
}

#endif
