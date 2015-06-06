//
//  File:       ParticleUtilities.h
//
//  Function:   Utilities for working with particle systems
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_PARTICLE_UTILITIES_H
#define HL_PARTICLE_UTILITIES_H

#include <HLAnimUtils.h>

#include <CLData.h>
#include <CLMath.h>
#include <CLMemory.h>
#include <CLRandom.h>
#include <CLTag.h>
#include <CLTransform.h>

namespace nCL
{
    class cValue;
}

namespace nHL
{
    class cIRenderer;

    typedef uint32_t tColourU32;


    // --- cMemArray -----------------------------------------------------------

    template<class T> struct cMemArray : public T
    /// Wraps a structure of parallel arrays and provides basic
    /// memory management. Requires:
    ///   void AllocArray(cIAllocator* a, int count, T** p);
    ///   void FreeArray (cIAllocator* a, T* p)
    ///   void CopyArray (int count, const T& a, T* b, int aStart, int bStart)
    ///   void InitArray (int count, T* p, int start)
    {
        void Resize (int count,     nCL::cIAllocator* alloc);
        void Reserve(int allocated, nCL::cIAllocator* alloc);
        int  Size   () const;
        void Clear  ();

        cMemArray();
        ~cMemArray();

        int          mCount;
        int          mAllocated;
        nCL::cIAllocator* mAllocator;
    };

    // Helpers for implementing aggregate arrays
    template<class T> void CopyArray(int count, const T a[], T b[], int aStart = 0, int bStart = 0);
    template<class T> void InitArray(int count, T a[], int start = 0);
    template<class T> void InitArray(int count, T a[], int start, T v);
    template<class T> void AllocArray(nCL::cIAllocator* alloc, int count, T** p);
    template<class T> void FreeArray (nCL::cIAllocator* alloc, T** p);
    template<class T> uint8_t* AddArray(uint8_t* m, int count, T** p);


    // -------------------------------------------------------------------------
    // High level particle control
    // -------------------------------------------------------------------------


    // --- Particle Creation ---------------------------------------------------

    enum tEmitMode
    {
        kEmitModeRate,
        kEmitModeInject,
        kEmitModeMaintain,
        kMaxEmitModes
    };

    struct cParticlesCreateDesc
    {
        struct cFlags
        {
            uint mMode              : 2;        ///< See tEmitMode
            bool mSustain           : 1;        ///< Hold the final cycle value forever
            bool mEmitEllipsoid     : 1;        ///< Volume defines an ellipsoid rather than a box
            bool mSizeScale         : 1;        ///< Scale created particles with source transform scale
            bool mEmitScaleVolume   : 1;        ///< Scale emit particle rate with source volume
            bool mEmitScaleArea     : 1;        ///< Scale emit particle rate with source area
            bool mEmitScaleLength   : 1;        ///< Scale emit particle rate with source length
            bool mVaryRGB           : 1;        ///< Vary individual colour channels
            bool mLoopParticles     : 1;        ///< Loop particles indefinitely rather than killing them once life is up.
            bool mEmitRadial        : 1;        ///< Initial particle velocities are always away from the source centre.
        };

        cFlags  mFlags = { 0 };

        float   mEmitCycleLength  = 1.0f;
        int     mEmitCycleCount   = 0;

        nCL::vector<float> mEmitFrames;

        Vec2f    mLife       = { 4.0f, 6.0f };

        cBounds3 mEmitBounds = { Vec3f(vl_0), Vec3f(vl_0) };
        float    mEmitTorusRadius = 0.0f;

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

        void Config(const nCL::cValue& value);
    };

    struct cParticlesCreateScale
    {
        float mRate   = 1.0f;
        float mSpeed  = 1.0f;
        float mSize   = 1.0f;
        Vec3f mColour = vl_1;
        float mAlpha  = vl_1;

        void Setup(const cParticlesCreateDesc& desc, const cTransform& sourceTransform);    ///< Fold in factors from cParticlesCreateDesc
    };

    struct cParticlesState
    /// Holds current state for particle system
    {
        // Management for particle emission
        int     mEmitCycle = 0;
        tPtAge  mEmitCycleAge = 0;
        tPtAge  mEmitCycleAgeStep = 0;

        float   mParticlesToCreate = 0.0f;

        void Setup(const cParticlesCreateDesc& desc);
    };

    int CreateRateParticles
    (
        float dt,
        const cParticlesCreateDesc&  desc,
        const cParticlesCreateScale& scale,
        cParticlesState*             state,

        int   maxParticles,     ///< Maximum number of particles to create
        float timeAlive[]       ///< Amount of time each created particle has lived during timestep
    );
    ///< Creates particles distributed over time according to emission rate. Returns the number of particles actually created.

    int CreateInjectParticles
    (
        float dt,
        const cParticlesCreateDesc&  desc,
        const cParticlesCreateScale& scale,
        cParticlesState*             state,

        int   maxParticles,     ///< Maximum number of particles to create
        float timeAlive[]       ///< Amount of time each created particle has lived during timestep
    );
    ///< Creates a set particles immediately, at the start of each emit cycle.

    int CreateMaintainParticles
    (
        float dt,
        const cParticlesCreateDesc&  desc,
        const cParticlesCreateScale& scale,
        cParticlesState*             state,

        int   currentParticles,
        int   maxParticles,     ///< Maximum number of particles to create
        float timeAlive[]       ///< Amount of time each created particle has lived during timestep
    );
    ///< Maintains a particular count of particles.

    void CreateParticleBasics
    (
        const cParticlesCreateDesc&  desc,
        const cParticlesCreateScale& createScale,

        nCL::tSeed32*   seed,
        int             start,
        int             count,
        tPtAge          ageStep[],
        Vec3f           position[],
        Vec3f           velocity[],

        const cBounds3* codeBounds = 0
    );
    ///< Fills in age/position/velocity for new particles

    void CreateParticleAttributes
    (
        const cParticlesCreateDesc&  desc,
        const cParticlesCreateScale& createScale,

        nCL::tSeed32*   seed,
        int             start,
        int             count,
        float           sizes[],
        float           alphas[],
        Vec3f           colours[],
        float           rotations[],
        float           aspects[]
    );
    ///< Fills in optional additional attributes. (Pass 0 to ignore an attribute.)

    void CreateParticleFrames
    (
        const cParticlesCreateDesc& desc,
        nCL::tSeed32*   seed,
        int             start,
        int             count,
        uint8_t         frames[]
    );
    ///< Fills in particle tile frames.


    // --- Particle Physics ----------------------------------------------------

    struct cParticlesPhysicsDesc
    {
        Vec3f mDirForces = vl_0;
        float mDrag = 0.0f;
        
        void Config(const nCL::cValue& config);
    };

    void UpdatePhysicsSimple
    (
        const cParticlesPhysicsDesc& desc,

        int             count,
        const float     dts[],          size_t dtStride,
        Vec3f           positions[],    size_t positionStride,
        Vec3f           velocities[],   size_t velocityStride
    );

    void UpdatePhysicsTimeStep
    (
        const cParticlesPhysicsDesc& desc,
        float           physicsStep,

        int             count,
        const float     dts[],          size_t dtStride,
        const Vec3f     forces[],       size_t forceStride,
        Vec3f           positions[],    size_t positionStride,
        Vec3f           velocities[],   size_t velocityStride
    );

    // --- Particle rendering --------------------------------------------------

    // Alignment types
    typedef uint8_t tParticleAlignment;
    enum tParticleAlignments : tParticleAlignment
    {
        kAlignCameraDir,        ///< Camera-direction-facing.
        kAlignCameraPos,        ///< Camera-position-facing.
        kAlignCameraZPole,      ///< Camera-facing, but constrained to only rotate around z.

        kAlignWorld,            ///< aligned with the x/y plane in world space.
        kAlignEffect,           ///< aligned with the x/y plane in effect space.
        kAlignSource,           ///< aligned with the x/y plane in source space.

        kAlignXToDir,           ///< Align x axis to direction. Keeps z up.
        kAlignYToDir,           ///< Align y axis to direction. Keeps z up.
        kAlignZToDir,           ///< Align z axis to direction. Keeps y up.
        
        kMaxParticleAlignments
    };

    struct cParticlesDispatchDesc
    /// Standard particle system description for rendering.
    {
        tParticleAlignment mAlignment = kAlignCameraDir;
        Vec3f              mAlignDir  = vl_z;       ///< Usually sourced from the emission direction of particles in source space

        float mVelocityStretch = 0.0f;

        nCL::vector<float> mSizeFrames;
        nCL::vector<float> mRotateFrames;
        nCL::vector<Vec3f> mColourFrames;
        nCL::vector<float> mAlphaFrames;
        nCL::vector<float> mAspectFrames;

        float   mRotateOffset = 0.0f;

        int     mTilesU         = 0;
        int     mTilesV         = 0;
        int     mTilesCount     = 0;
        float   mTilesSpeed     = 0.0f;

        nCL::tTag mMaterialTag = kNullTag;
        nCL::tTag mTextureTag  = kNullTag;

        nCL::tTag mMaterial2Tag = kNullTag;
        nCL::tTag mTexture2Tag  = kNullTag;

        void Config(const nCL::cValue& config);
    };

    struct cDispatchScale
    {
        Vec3f mColour = vl_1;
        float mAlpha  = 1.0f;
        float mSize   = 1.0f;
    };

    void DispatchParticles
    (
        cIRenderer*         renderer,
        int                 quadMesh,

        const cParticlesDispatchDesc& desc,
        const cTransform&   sourceToEffect,
        const cTransform&   effectToWorld,
        const cTransform&   cameraToEffect,

        const cDispatchScale* dispatchScale,

        int                 particlesCount,
        const tPtAge        ages[],
        const tPtAge        ageSteps  [],     size_t ageStride,
        const Vec3f         positions [],     size_t positionStride,
        const Vec3f         velocities[],     size_t velocityStride,
        const Vec3f         colours   [] = 0, size_t colourStride   = 0,
        const float         alphas    [] = 0, size_t alphaStride    = 0,
        const float         sizes     [] = 0, size_t sizeStride     = 0,
        const float         rotations [] = 0, size_t rotationStride = 0,
        const float         aspects   [] = 0, size_t aspectStride   = 0,
        const uint8_t       frames    [] = 0, size_t frameStride    = 0
    );
    ///< Standard dispatch of coloured particles, as per the supplied cParticlesDispatchDesc.



    // -------------------------------------------------------------------------
    // Low level particle toolkit
    // -------------------------------------------------------------------------

    // --- Particle tiles ------------------------------------------------------
    struct cParticleTileInfo
    {
        uint8_t   mCount;        ///< Frame count for particle animation

        float     mTimeScale;    ///< Speed at which frames should change with particle age

        Vec2f     mDT;
        uint32_t  mMask[2];
        int       mShift;
    };

    struct cParticleTile
    {
        uint8_t mU; // tile coord uv
        uint8_t mV;
    };

    struct cParticleTilePair
    {
        float   mT;     // lerp factor between frame 0 and 1
        uint8_t mU0;    // tile coords for first tile
        uint8_t mV0;
        uint8_t mU1;    // tile coords for second tile
        uint8_t mV1;
    };

    cParticleTileInfo* InitParticleTileInfo
    (
        cParticleTileInfo*  ti,
        int                 tilesU,
        int                 tilesV,
        float               timeScale,
        int                 count = 0
    );
    ///< Fills in 'ti' and returns the pointer to it if there is more than one tile, returns 0 otherwise.

    // --- Particle alignment --------------------------------------------------
    
    void FindParticleAxes
    (
        tParticleAlignment  alignment,
        const cTransform&   sourceToEffect,
        const cTransform&   effectToWorld,
        const cTransform&   cameraToWorld,
        Vec3f               emitDir,
        Vec3f               axes[2]
    );
    ///< Find 'axes' according to aligment

    void AlignToCameraPosition
    (
        const cTransform& cameraToWorld,
        Vec3f       psAxis[2],
     
        int         count,
        const Vec3f positions[], size_t positionStride,
        Vec3f       axis0[],
        Vec3f       axis1[]
    );
    ///< Generates a set of to-camera-position aligned particle axes

    // --- Particle processing utilities ---------------------------------------
    
    void ApplyRotations
    (
        int         count,
        const Vec3f a0s[],
        const Vec3f a1s[],          size_t aStride,
        const float rotations[],    size_t rotationStride,
        Vec3f       a0sOut[],
        Vec3f       a1sOut[]
    );
    ///< Sets up axes per particle according to rotation/size/scale inputs

    void ApplyScalesAndAspects
    (
        int         count,
        const Vec3f d0s[],                                  // diagonal 0 = [-1, 1]
        const Vec3f d1s[],          size_t dStride,         // diagonal 1 = [+1, 1]
        const float scales[],       size_t scaleStride,
        const float aspects[],      size_t aspectStride,
        Vec3f       d0sOut[],
        Vec3f       a1sOut[]
    );
    ///< Sets up axes per particle according to rotation/size/scale inputs

    void ApplyScalesAndAspectsToDiagonals
    (
        int         count,
        const Vec3f d0s[],                                  // diagonal 0 = [-1, 1]
        const Vec3f d1s[],          size_t dStride,         // diagonal 1 = [+1, 1]
        const float scales[],       size_t scaleStride,
        const float aspects[],      size_t aspectStride,
        Vec3f       d0sOut[],
        Vec3f       a1sOut[]
    );
    ///< Sets up axes per particle according to rotation/size/scale inputs


    void ApplyVelocityStretch
    (
        float psVelocityStretch,
        const cTransform& effectToWorld,

        int         count,
        const Vec3f velocities[],   size_t velocityStride,
        const Vec3f d0s[],
        const Vec3f d1s[],          size_t dStride,
        Vec3f       d0Out[],
        Vec3f       d1Out[]
    );
    ///< Apply velocity stretch to particle axes. Axes in and out are expected
    ///< to be in 'canonical' form, a0 = right, a1 = up, dot(a0, a1) = 0

    void ApplyVelocityStretchToDiagonals
    (
        float psVelocityStretch,
        const cTransform& effectToWorld,

        int         count,
        const Vec3f velocities[],   size_t velocityStride,
        const Vec3f d0s[],
        const Vec3f d1s[],          size_t dStride,
        Vec3f       d0Out[],
        Vec3f       d1Out[]
    );
    ///< Apply velocity stretch to particle axes. Axes in and out are expected
    ///< to be in 'diagonal' form, i.e., quad corners are p +- d0, p +- d1.


    void FindTiles
    (
        const cParticleTileInfo& psi,
        int                 count,
        const uint8_t       startFrames[],  size_t startFrameStride,
        const tPtAge        ages[],
        const tPtAge        ageSteps[],     size_t ageStride,
        cParticleTile       tiles[]
    );

    void FindTilePairs
    (
        const cParticleTileInfo& psi,

        int                 count,
        const uint8_t       startFrames[],  size_t startFrameStride,
        const tPtAge        ages[],
        const tPtAge        ageSteps[],     size_t ageStride,
        cParticleTilePair   tiles[]
    );

    // Simple transforms.
    void ApplyScale
    (
        const float scale,
        int         count,
        const float data[], size_t dataStride,
        float       dataOut[]
    );
    void ApplyScale
    (
        float       scale,
        int         count,
        const Vec3f data[], size_t dataStride,
        Vec3f       dataOut[]
    );
    void ApplyScale
    (
        const Vec3f scale,
        int         count,
        const Vec3f data[], size_t dataStride,
        Vec3f       dataOut[]
    );

    void ApplyDelta
    (
        const float delta,
        int         count,
        const float data[], size_t dataStride,
        float       dataOut[]
    );
    void ApplyDelta
    (
        const Vec3f delta,
        int         count,
        const Vec3f data[], size_t dataStride,
        Vec3f       dataOut[]
    );

    void Transform
    (
        const cTransform& xform,
        int         count,
        Vec3f       positions[],
        Vec3f       velocities[]
    );

    void OffsetEmitPoints
    (
        const cParticlesDispatchDesc& desc,
        const Vec3f axes[2],
     
        int         count,
        const Vec3f velocities[],
        const float aspects[],
        Vec3f       positions[]
    );

    // --- Rendering -----------------------------------------------------------

    // Assembling quads from per-particle attributes
    struct cQuadVertex
    {
        Vec3f      mPosition;
        Vec2f      mUV;
        tColourU32 mColour;
    };

    int WriteQuads
    (
        int count,

        const Vec3f positions[],    size_t positionStride,
        const Vec3f d0s[],
        const Vec3f d1s[],          size_t dStride,
        const Vec3f colours[],      size_t colourStride,
        const float alphas[],       size_t alphaStride,

        cQuadVertex* vertices[]
    );

    int WriteQuads
    (
        int count,

        const Vec3f positions[],    size_t positionStride,
        const Vec3f d0s[],
        const Vec3f d1s[],          size_t dStride,
        const Vec3f colours[],      size_t colourStride,
        const float alphas[],       size_t alphaStride,
        const cParticleTile tiles[], size_t tileStride,
        Vec2f tw,

        cQuadVertex* vertices[]
    );



    // --- Inlines -------------------------------------------------------------
    template<class T> inline cMemArray<T>::cMemArray()
    {
        memset(this, 0, sizeof(cMemArray<T>));
    }

    template<class T> inline cMemArray<T>::~cMemArray()
    {
        Clear();
    }

    template<class T> inline void cMemArray<T>::Resize(int count, cIAllocator* alloc)
    {
        if (count > mAllocated)
        {
            int newAllocated;

            if (!mAllocated)
                newAllocated = count;
            else
                newAllocated = 2 * mAllocated;

            while (count > newAllocated)
                newAllocated *= 2;

            Reserve(newAllocated, alloc);
        }

        if (count > mCount)
            InitArray(count - mCount, (T*) this, mCount);

        mCount = count;
    }

    template<class T> inline void cMemArray<T>::Reserve(int allocated, cIAllocator* alloc)
    {
        if (!alloc)
            alloc = mAllocator;

        cMemArray<T> oldParticles(*this);

        AllocArray(alloc, allocated, (T*) this);
        if (oldParticles.mCount > 0)
            CopyArray(oldParticles.mCount, oldParticles, (T*) this, 0, 0);

        mAllocated = allocated;
        mAllocator = alloc;
    }

    template<class T> inline void CopyArray(int count, const T a[], T b[], int aStart, int bStart)
    {
        memcpy(b + bStart, a + aStart, sizeof(T) * count);
    }

    template<class T> inline void InitArray(int count, T a[], int start)
    {
        memset(a + start, 0, sizeof(a[0]) * count);
    }

    template<class T> inline void InitArray(int count, T a[], int start, T v)
    {
        a += start;

        for (int i = 0; i < count; i++)
            a[i] = v;
    }

    template<class T> inline void AllocArray(nCL::cIAllocator* alloc, int count, T** p)
    {
        *p = (T*) alloc->Alloc(sizeof(T) * count);
    }

    template<class T> inline void FreeArray(nCL::cIAllocator* alloc, T** p)
    {
        if (*p)
            alloc->Free(*p);
        *p = 0;
    }

    template<class T> inline uint8_t* AddArray(uint8_t* m, int count, T** p)
    {
        *p = (T*) m;
        return m + sizeof(T) * count;
    }

    template<class T> inline int cMemArray<T>::Size() const
    {
        return mCount;
    }

    template<class T> inline void cMemArray<T>::Clear()
    {
        if (mAllocator)
        {
            FreeArray(mAllocator, (T*) this);
            mAllocated = 0;
            mAllocator = 0;
            mCount = 0;
        }
    }
}


#endif
