//
//  File:       HLParticleUtils.cpp
//
//  Function:   Utilities for updating/rendering particle systems
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLParticleUtils.h>

#include <IHLRenderer.h>

#include <CLColour.h>
#include <CLMath.h>
#include <CLString.h>
#include <CLValue.h>
#include <CLVecUtil.h>

using namespace nHL;
using namespace nCL;

namespace
{
    const float kDefaultAnimFloat = 1.0f;
    const Vec3f kDefaultAnimVec3f = vl_1;
}

namespace
{
    enum tSourceType
    {
        kSourcePoint,
        kSourceSquare,
        kSourceRect,
        kSourceCube,
        kSourceBox,
        kSourceCircle,
        kSourceEllipse,
        kSourceSphere,
        kSourceEllipsoid,
        kSourceRing,
        kSourceTorus,
        kMaxSourceTypes
    };

    cEnumInfo kEnumSourceType[] =
    {
        "point",     kSourcePoint,
        "square",    kSourceSquare,
        "rect",      kSourceRect,
        "rectangle", kSourceRect,
        "cube",      kSourceCube,
        "box",       kSourceBox,
        "circle",    kSourceCircle,
        "ellipse",   kSourceEllipse,
        "sphere",    kSourceSphere,
        "ellipsoid", kSourceEllipsoid,
        "ring",      kSourceRing,
        "torus",     kSourceTorus,
        0, 0
    };

    cEnumInfo kEnumAlignment[] =
    {
        "camera",       kAlignCameraDir,
        "cameraDir",    kAlignCameraDir,
        "cameraPos",    kAlignCameraPos,
        "zpole",        kAlignCameraZPole,

        "world",        kAlignWorld,
        "effect",       kAlignEffect,
        "source",       kAlignSource,

        "x",            kAlignXToDir,
        "y",            kAlignYToDir,
        "z",            kAlignZToDir,
        0, 0
    };
}

void cParticlesDispatchDesc::Config(const cValue& config)
{
    mAlignment = AsEnum(config[CL_TAG("align")], kEnumAlignment, mAlignment);
    mAlignDir  = norm_safe(AsVec3(config[CL_TAG("alignDir")], mAlignDir));

    mVelocityStretch = config.Member(CL_TAG("stretch")).AsFloat(mVelocityStretch);

    SetFromValue(config[CL_TAG("size"  )], &mSizeFrames);
    SetFromValue(config[CL_TAG("rotate")], &mRotateFrames);
    SetFromValue(config[CL_TAG("colour")], &mColourFrames);
    SetFromValue(config[CL_TAG("alpha" )], &mAlphaFrames);
    SetFromValue(config[CL_TAG("aspect")], &mAspectFrames);

    mRotateOffset = config[CL_TAG("rotateOffset")].AsFloat(mRotateOffset);

    mTilesU = config.Member(CL_TAG("tilesU")).AsInt(mTilesU);
    mTilesV = config.Member(CL_TAG("tilesV")).AsInt(mTilesV);

    mTilesCount = config.Member(CL_TAG("animFrames")).Member(CL_TAG("count")).AsInt(mTilesCount);

    mTilesSpeed = config.Member(CL_TAG("tilesSpeed")).AsFloat(mTilesSpeed);

    mMaterialTag = config[CL_TAG("material")].AsTag(mMaterialTag);
    mTextureTag  = config[CL_TAG("texture")] .AsTag(mTextureTag);
    
    mMaterialTag = config[CL_TAG("material1")].AsTag(mMaterialTag);
    mTextureTag  = config[CL_TAG("texture1")] .AsTag(mTextureTag);

    mMaterial2Tag = config[CL_TAG("material2")].AsTag(mMaterial2Tag);
    mTexture2Tag  = config[CL_TAG("texture2")] .AsTag(mTexture2Tag);
}

void cParticlesCreateDesc::Config(const cValue& config)
{
    if (SetFromValue(config.Member(CL_TAG("rate")), &mEmitFrames) > 0)
        mFlags.mMode = kEmitModeRate;
    else if (SetFromValue(config.Member(CL_TAG("inject")), &mEmitFrames) > 0)
        mFlags.mMode = kEmitModeInject;
    else if (SetFromValue(config.Member(CL_TAG("maintain")), &mEmitFrames) > 0)
        mFlags.mMode = kEmitModeMaintain;

    mEmitCycleLength = config.Member(CL_TAG("cycleTime" )).AsFloat(mEmitCycleLength);
    mEmitCycleCount  = config.Member(CL_TAG("cycleCount")).AsFloat(mEmitCycleCount);

    mFlags.mSustain = config.Member(CL_TAG("sustain")).AsBool(false);

    mLife = AsRange(config.Member(CL_TAG("life")), mLife);
    mFlags.mLoopParticles = config.Member(CL_TAG("lifeLoop")).AsBool(mFlags.mLoopParticles);

    const cValue& sourceV = config.Member(CL_TAG("emitSource"));

    if (!sourceV.IsNull())
    {
        Vec3f offset = AsVec3(sourceV.Member(CL_TAG("offset")));
        mFlags.mEmitEllipsoid = false;
        mEmitTorusRadius = 0.0f;

        int sourceType = AsEnum(sourceV.Member(CL_TAG("type")), kEnumSourceType);

        switch (sourceType)
        {
        case kSourcePoint:
            mEmitBounds.MakePoint(offset);
            break;
        case kSourceSquare:
            {
                float s = sourceV.Member(CL_TAG("size")).AsFloat();
                mEmitBounds.MakeBox(offset, Vec3f(s, s, 0.0f));
            }
            break;
        case kSourceRect:
            {
                Vec2f s = AsVec2(sourceV.Member(CL_TAG("size")));
                mEmitBounds.MakeBox(offset, Vec3f(s, 0.0f));
            }
            break;
        case kSourceCube:
            {
                float s = sourceV.Member(CL_TAG("size")).AsFloat();
                mEmitBounds.MakeCube(offset, s);
            }
            break;
        case kSourceBox:
            {
                Vec3f s = AsVec3(sourceV.Member(CL_TAG("size")));
                mEmitBounds.MakeBox(offset, s);
            }
            break;

        case kSourceCircle:
            {
                float s = sourceV.Member(CL_TAG("size")).AsFloat();
                mEmitBounds.MakeBox(offset, Vec3f(s, s, 0.0f));
                mFlags.mEmitEllipsoid = true;
            }
            break;
        case kSourceSphere:
            {
                float s = sourceV.Member(CL_TAG("size")).AsFloat();
                mEmitBounds.MakeBox(offset, Vec3f(s));
                mFlags.mEmitEllipsoid = true;
            }
            break;

        case kSourceRing:
            {
                float s = sourceV.Member(CL_TAG("size" )).AsFloat();
                float w = sourceV.Member(CL_TAG("width")).AsFloat();
                mEmitBounds.MakeBox(offset, Vec3f(s, s, 0.0f));
                mEmitTorusRadius = w;
            }
            break;
        case kSourceTorus:
            {
                float s = sourceV.Member(CL_TAG("size" )).AsFloat();
                float w = sourceV.Member(CL_TAG("width")).AsFloat();
                mEmitBounds.MakeBox(offset, Vec3f(s, s, s));
                mEmitTorusRadius = w;
            }
            break;
        }

        mFlags.mSizeScale       = sourceV.Member(CL_TAG("scaleParticles" )).AsBool();
        mFlags.mSizeScale       = sourceV.Member(CL_TAG("sizeScale"      )).AsBool(mFlags.mSizeScale);

        mFlags.mEmitScaleVolume = sourceV.Member(CL_TAG("emitScaleVolume")).AsBool();
        mFlags.mEmitScaleArea   = sourceV.Member(CL_TAG("emitScaleArea"  )).AsBool();
        mFlags.mEmitScaleLength = sourceV.Member(CL_TAG("emitScaleLength")).AsBool();
    }

    if (config.Member(CL_TAG("emitRadial")).AsBool())
    {
        mFlags.mEmitRadial = true;
        mEmitDir.MakePoint(vl_0);
    }
    else
    {
        const cValue& emitDirV = config.Member(CL_TAG("emitDir"));

        if (!emitDirV.IsNull())
        {
            mEmitDir.MakePoint(norm_safe(AsVec3(emitDirV)));

            Vec3f emitSpread = AsVec3(config.Member(CL_TAG("emitSpread")));
            mEmitDir.Inflate(emitSpread);

            mFlags.mEmitRadial = false;
        }
    }

    mEmitSpeed = AsRange(config.Member(CL_TAG("emitSpeed")), mEmitSpeed);

    const cValue* v;
    Vec3f* baseColour = 0;

    if (!(v = &config[CL_TAG("colourVary")])->IsNull())
    {
        mFlags.mVaryRGB = false;
        mColourVary = AsRangedVec3(*v, 0.0f, 1.0f, mColourVary);
    }
    else if (!(v = &config[CL_TAG("colourVaryRGB")])->IsNull())
    {
        mFlags.mVaryRGB = true;
        mColourVary = AsRangedVec3(*v, 0.0f, 1.0f, mColourVary);
    }
    else if (baseColour && !(v = &config[CL_TAG("colourVaryTo")])->IsNull())
    {
        mFlags.mVaryRGB = false;

        Vec3f cs = *baseColour;
        Vec3f cd = AsRangedVec3(*v, 0.0f, 1.0f, mColourVary);

        Vec3f sum = (cd + cs);

        mColourVary = (cd - cs) / sum;
        *baseColour = 0.5f * sum;
    }

    mAlphaVary    = AsUnitFloat(config[CL_TAG("alphaVary")],  mAlphaVary);

    mSizeVary     = AsUnitFloat(config[CL_TAG("sizeVary")],   mSizeVary);
    mRotateVary   = AsUnitFloat(config[CL_TAG("rotateVary")], mRotateVary);
    mAspectVary   = AsUnitFloat(config[CL_TAG("aspectVary")], mAspectVary);

    const cValue& animFramesV = config.Member(CL_TAG("animFrames"));

    if (animFramesV.IsObject())
    {
        mFrameStart  = animFramesV.Member(CL_TAG("start" )).AsInt(mFrameStart);
        mFrameCount  = animFramesV.Member(CL_TAG("count" )).AsInt(mFrameCount);
        mFrameRandom = animFramesV.Member(CL_TAG("random")).AsInt(mFrameRandom);
    }
}

void cParticlesPhysicsDesc::Config(const cValue& config)
{
    mDirForces = vl_0;

    mDirForces -= config.Member(CL_TAG("gravity")).AsFloat(0.0f) * Vec3f(vl_z);
    mDirForces += AsVec3(config.Member(CL_TAG("force")));

    mDrag = config.Member(CL_TAG("drag")).AsFloat(mDrag);
}

void cParticlesCreateScale::Setup(const cParticlesCreateDesc& desc, const cTransform& sourceTransform)
{
    if (desc.mFlags.mSizeScale)
        mSize *= sourceTransform.Scale();

    if (desc.mFlags.mEmitScaleVolume)
        mRate *= cube(sourceTransform.Scale());
    else if (desc.mFlags.mEmitScaleArea)
        mRate *= sqr(sourceTransform.Scale());
    else if (desc.mFlags.mEmitScaleLength)
        mRate *= sourceTransform.Scale();
}

void cParticlesState::Setup(const cParticlesCreateDesc& desc)
{
    mEmitCycle = 0;
    mEmitCycleAgeStep = LifeToAgeStep(desc.mEmitCycleLength);
    mEmitCycleAge = 0;

    mParticlesToCreate = 0.0f;
}


// Low-level routines

void nHL::FindParticleAxes
(
    tParticleAlignment  alignment,
    const cTransform&   sourceToEffect,
    const cTransform&   effectToWorld,
    const cTransform&   cameraToWorld,
    Vec3f               alignDir,
    Vec3f               axes[2]
)
{
    // Note: our camera is pointed down the Y axis with Z up.
    switch (alignment)
    {
    case kAlignCameraDir:
    case kAlignCameraPos:
        axes[0] = cameraToWorld.Axis(vl_x);
        axes[1] = cameraToWorld.Axis(vl_z);
        break;

    case kAlignCameraZPole:      ///< Camera-facing, but constrained to only rotate around z.
        {
            axes[1] = effectToWorld.TransformDirection(sourceToEffect.Axis(vl_z));
            axes[0] = cross(cameraToWorld.Axis(vl_y), axes[1]);

            float hLen = len(axes[0]);
            // 0.1 is so we shrink axis as we start to approach viewing from directly above.
            axes[0] /= (hLen + 0.1f);
        }
        break;

    case kAlignWorld:
        alignDir = sourceToEffect.TransformDirection(alignDir);
    case kAlignEffect:
        alignDir = effectToWorld.TransformDirection(alignDir);
    case kAlignSource:
        {
            Vec3f horiz = cross_z(alignDir);
            float horizLen2 = sqrlen(horiz);

            if (horizLen2 > 1e-6f)
            {
                axes[0] = horiz / sqrtf(horizLen2);
                axes[1] = -cross_z(axes[0]);
            }
            else
            {
                // we don't want to gracefully handle emitDir = z, as this is taken
                // to mean just lock the particle to the standard axes.
                axes[0] = vl_x;
                axes[1] = vl_y;
            }

            if (alignment == kAlignSource)
                sourceToEffect.TransformDirections(2, axes);    // clang gives an awesome warning about CL_SIZE(axes) here.
            if (alignment == kAlignSource || alignment == kAlignEffect)
                effectToWorld.TransformDirections(2, axes);
        }
        break;

    default:
        axes[0] = vl_x;
        axes[1] = vl_z;
    }
}

cParticleTileInfo* nHL::InitParticleTileInfo
(
    cParticleTileInfo*  ti,
    int                 tilesU,
    int                 tilesV,
    float               timeScale,
    int                 count
)
{
    // Texture tiling
    if (tilesU <= 1 && tilesV <= 1)
        return 0;

    ti->mCount = count;
    ti->mTimeScale = kPtAgeFractionScale * timeScale;

    ti->mDT[0] = 1.0f / float(tilesU);
    ti->mDT[1] = 1.0f / float(tilesV);

    // Tiles are always powers of two
    ti->mMask[0] = tilesU - 1;
    ti->mMask[1] = tilesV - 1;
    ti->mShift = 0;

    for (int i = tilesU; i > 1; i >>= 1)
        ti->mShift++;

    return ti;
}



/* Using diagonal notes

    Use:
        d0 = (a1 + a0) / 2
        d1 = (a1 - a0) / 2
    So quad reconstruction is the cheap p +- di
    
    Standard rotation:
        a0' =  ca0 + sa1
        a1' = -sa0 + ca1

    Diagonal rotation:
        d0' = cd0 - sd1
        d1' = sd0 + cd1
*/

void nHL::AlignToCameraPosition
(
    const cTransform& cameraToWorld,
    Vec3f       psAxis[2],
 
    int         count,
    const Vec3f positions[], size_t positionStride,
    Vec3f       axis0[],
    Vec3f       axis1[]
)
{
    for (int i = 0; i < count; i++)
    {
        Vec3f r = *positions - cameraToWorld.mTranslation;

        axis0[i] = norm_safe(cross(r, psAxis[1]));
        axis1[i] = norm_safe(cross(axis0[i], r));

        ((uint8_t*&) positions) += positionStride;
    }
}


// Particle creation


/*
    To implement:
    - position-based emission (the old special emit stuff)
    - Add radial velocities (bomb force)
    - Offset emit points so base lines up
    - Initial frame
    - Position from path
    - cColour from emit map
    - pin to surface
*/



void nHL::CreateParticleBasics
(
    const cParticlesCreateDesc&  desc,
    const cParticlesCreateScale& createScale,
    tSeed32*        seed,
    int             start,
    int             count,
    tPtAge          ageStep[],
    Vec3f           position[],
    Vec3f           velocity[],
    const cBounds3* codeBounds
)
{
    int finish = start + count;

    if (ageStep)
        for (int i = start; i < finish; i++)
            ageStep[i] = LifeToAgeStep(RandomRange(desc.mLife, seed));

    const cBounds3* emitBounds = codeBounds ? codeBounds : &desc.mEmitBounds;

    if (desc.mEmitTorusRadius > 0.0f)
        for (int i = start; i < finish; i++)
            position[i] = RandomTorus    (*emitBounds, desc.mEmitTorusRadius, seed);
    else if (desc.mFlags.mEmitEllipsoid)
        for (int i = start; i < finish; i++)
            position[i] = RandomEllipsoid(*emitBounds, seed);
    else
        for (int i = start; i < finish; i++)
            position[i] = RandomRange    (*emitBounds, seed);

    if (desc.mFlags.mEmitRadial)
        for (int i = start; i < finish; i++)
        {
            Vec3f v = position[i];

            float speed = RandomRange(desc.mEmitSpeed, seed) * createScale.mSpeed;
            
            velocity[i] = v * (speed * InvSqrtFast(sqrlen(v)));
        }
    else
        for (int i = start; i < finish; i++)
        {
            Vec3f v = RandomRange(desc.mEmitDir, seed);

            float speed = RandomRange(desc.mEmitSpeed, seed) * createScale.mSpeed;
            
            velocity[i] = v * (speed * InvSqrtFast(sqrlen(v)));
        }
}

void nHL::CreateParticleAttributes
(
    const cParticlesCreateDesc&  desc,
    const cParticlesCreateScale& createScale,

    tSeed32*    seed,
    int         start,
    int         count,
    float       sizes[],
    float       alphas[],
    Vec3f       colours[],
    float       rotations[],
    float       aspects[]
)
{
    int finish = start + count;

    if (sizes)
    {
        if (desc.mSizeVary != 0.0f)
            for (int i = start; i < finish; i++)
                sizes[i] = RandomRangeAbout(1.0f, desc.mSizeVary, seed) * createScale.mSize;
        else
            for (int i = start; i < finish; i++)
                sizes[i] = createScale.mSize;
    }

    if (alphas)
    {
        if (desc.mAlphaVary != 0.0f)
            for (int i = start; i < finish; i++)
                alphas[i] = RandomRangeAbout(1.0f, desc.mAlphaVary, seed) * createScale.mAlpha;
        else
            for (int i = start; i < finish; i++)
                alphas[i] = createScale.mAlpha;
    }

    if (colours)
    {
        if (desc.mColourVary != vl_0)
        {
            if (desc.mFlags.mVaryRGB)
                for (int i = start; i < finish; i++)
                {
                    colours[i] =
                    {
                        RandomRangeAbout(1.0f, desc.mColourVary[0], seed),
                        RandomRangeAbout(1.0f, desc.mColourVary[1], seed),
                        RandomRangeAbout(1.0f, desc.mColourVary[2], seed)
                    };
                }
            else
                for (int i = start; i < finish; i++)
                    colours[i] = Vec3f(vl_1) + desc.mColourVary * RandomSFloat(seed);

            if (createScale.mColour != vl_1)
                for (int i = start; i < finish; i++)
                    colours[i] *= createScale.mColour;
        }
        else
            for (int i = start; i < finish; i++)
                colours[i] = createScale.mColour;
    }

    if (rotations)
    {
        if (desc.mRotateVary != 0.0f)
            for (int i = start; i < finish; i++)
                rotations[i] = RandomSFloat(desc.mRotateVary, seed);
        else
            for (int i = start; i < finish; i++)
                rotations[i] = 1.0f;
    }

    if (aspects)
    {
        if (desc.mAspectVary != 0.0f)
            for (int i = start; i < finish; i++)
                aspects[i] = RandomRangeAbout(1.0f, desc.mAspectVary, seed);
        else
            for (int i = start; i < finish; i++)
                aspects[i] = 1.0f;
    }
}

void nHL::CreateParticleFrames
(
    const cParticlesCreateDesc& desc,
    tSeed32*    seed,
    int         start,
    int         count,
    uint8_t     frames[]
)
{
    int finish = start + count;

    if (desc.mFrameRandom > 0)
    {
        if (desc.mFrameCount > 0)
            for (int i = start; i < finish; i++)
                frames[i] = desc.mFrameStart + RandomUInt32(desc.mFrameRandom, seed) * desc.mFrameCount;
        else
            for (int i = start; i < finish; i++)
                frames[i] = desc.mFrameStart + RandomUInt32(desc.mFrameRandom, seed);
    }
    else
        for (int i = start; i < finish; i++)
            frames[i] = desc.mFrameStart;
}

int nHL::CreateRateParticles
(
    float dt,

    const cParticlesCreateDesc&  desc,
    const cParticlesCreateScale& createScale,
    cParticlesState*             state,

    int maxParticles,
    float timeAlive[]      // amount of time this particle has lived during timestep
)
{
    // TODO: scale linear/area/volume
    if (dt > 0.0f)
    {
        float emitRate = createScale.mRate;

        // we'll distribute the new particles over our last time period.

        // we're animating the rate over time.
        tPtAge da = DeltaSecondsToIotas(dt);
        state->mEmitCycleAge += da * state->mEmitCycleAgeStep;

        if ((state->mEmitCycleAge >> kPtAgeFractionBits) >= state->mEmitCycle + 1)
        {
            state->mEmitCycle++;

            if (desc.mEmitCycleCount > 0 && state->mEmitCycle == desc.mEmitCycleCount)
            {
                if (desc.mFlags.mSustain)
                {
                    state->mEmitCycle = desc.mEmitCycleCount - 1;
                    state->mEmitCycleAge = desc.mEmitCycleCount * kPtAgeFractionMax - 1;
                }
                else
                {
                    // we're done: do an auto soft stop.
                    return -1;
                }
            }
        }

        tPtAge age = state->mEmitCycleAge & kPtAgeFractionMask;

        if (!desc.mEmitFrames.empty())
            ApplyLinearAnim(desc.mEmitFrames.size(), desc.mEmitFrames.data(), 1, &age, 0, &emitRate, 0, &emitRate);

        state->mParticlesToCreate += emitRate * dt;
    }

    int particlesToCreateNow = min(FloorToSInt32(state->mParticlesToCreate), maxParticles);
    state->mParticlesToCreate -= particlesToCreateNow;

    if (particlesToCreateNow > 0)
    {
        float timeToLive = dt;      // this is still not right -- dt=0 second time around.
        float timeToLiveDelta = dt / particlesToCreateNow;

        while (timeToLive >= desc.mLife[1])
        {
            timeToLive -= timeToLiveDelta;
            particlesToCreateNow--;
        }

        for (int i = 0; i < particlesToCreateNow; i++)
        {
            timeAlive[i] = timeToLive;
            timeToLive -= timeToLiveDelta;
        }
    }

    return particlesToCreateNow;
}

int nHL::CreateInjectParticles
(
    float dt,

    const cParticlesCreateDesc&  desc,
    const cParticlesCreateScale& createScale,
    cParticlesState*             state,

    int maxParticles,
    float timeAlive[]      // amount of time this particle has lived during timestep
)
{
    // TODO: scale linear/area/volume
    if (dt > 0.0f)
    {
        if (desc.mEmitCycleCount > 0 && state->mEmitCycle == desc.mEmitCycleCount)
        {
            if (desc.mFlags.mSustain)
            {
                state->mEmitCycle = desc.mEmitCycleCount - 1;
                state->mEmitCycleAge = desc.mEmitCycleCount * kPtAgeFractionMax - 1;
            }
            else
            {
                // we're done: do an auto soft stop.
                return -1;
            }
        }

        // we're animating the rate over time.
        tPtAge da = DeltaSecondsToIotas(dt);
        state->mEmitCycleAge += da * state->mEmitCycleAgeStep;

        if ((state->mEmitCycleAge >> kPtAgeFractionBits) >= state->mEmitCycle)
        {
            // Inject particles at start of cycle
            int emitFrame = state->mEmitCycle % desc.mEmitFrames.size();
            state->mParticlesToCreate += desc.mEmitFrames[emitFrame];

            state->mEmitCycle++;
        }
    }

    int particlesToCreateNow = min(FloorToSInt32(state->mParticlesToCreate), maxParticles);
    state->mParticlesToCreate -= particlesToCreateNow;

    if (particlesToCreateNow > 0)
    {
        float timeToLive = dt;      // this is still not right -- dt=0 second time around.

        for (int i = 0; i < particlesToCreateNow; i++)
            timeAlive[i] = timeToLive;
    }

    return particlesToCreateNow;
}

int nHL::CreateMaintainParticles
(
    float dt,

    const cParticlesCreateDesc&  desc,
    const cParticlesCreateScale& createScale,
    cParticlesState*             state,

    int currentParticles,
    int maxParticles,
    float timeAlive[]      // amount of time this particle has lived during timestep
)
{
    // TODO: scale linear/area/volume
    if (dt > 0.0f)
    {
        if (desc.mEmitCycleCount > 0 && state->mEmitCycle == desc.mEmitCycleCount)
        {
            if (desc.mFlags.mSustain)
            {
                state->mEmitCycle = desc.mEmitCycleCount - 1;
                state->mEmitCycleAge = desc.mEmitCycleCount * kPtAgeFractionMax - 1;
            }
            else
            {
                // we're done: do an auto soft stop.
                return -1;
            }
        }

        // we're animating the rate over time.
        tPtAge da = DeltaSecondsToIotas(dt);
        state->mEmitCycleAge += da * state->mEmitCycleAgeStep;

        if ((state->mEmitCycleAge >> kPtAgeFractionBits) >= state->mEmitCycle)
        {
            // Inject particles at start of cycle
            int emitFrame = state->mEmitCycle % desc.mEmitFrames.size();

            int targetCount = desc.mEmitFrames[emitFrame];
            if (targetCount > currentParticles)
                state->mParticlesToCreate += (targetCount - currentParticles);

            state->mEmitCycle++;
        }
    }

    int particlesToCreateNow = min(FloorToSInt32(state->mParticlesToCreate), maxParticles);
    state->mParticlesToCreate -= particlesToCreateNow;

    if (particlesToCreateNow > 0)
    {
        float timeToLive = dt;      // this is still not right -- dt=0 second time around.

        for (int i = 0; i < particlesToCreateNow; i++)
            timeAlive[i] = timeToLive;
    }

    return particlesToCreateNow;
}

// Particle processing

void nHL::Transform
(
    const cTransform& xform,
    int         count,
    Vec3f       positions[],
    Vec3f       velocities[]
)
{
    xform.TransformPoints    (count, positions);
    xform.TransformDirections(count, velocities);   // purposefully exclude scale  -- we're post-scaling the emission volume, not the particles per se
}

void nHL::OffsetEmitPoints
(
    const cParticlesDispatchDesc& desc,
    const Vec3f axes[2],
 
    int         count,
    const Vec3f velocities[],
    const float aspects[],
    Vec3f       positions[]
)
{
    // Offset the emit point so the base of the particle rests on our location.
    // TODO: actually, why not just call ApplyVelocityStretch on temporary buffer?
    for (int i = 0; i < count; i++)
    {
        float sr = dot(velocities[i], axes[0]);
        float cr = dot(velocities[i], axes[1]);

        if (desc.mVelocityStretch == 0.0f)
            sr *= aspects[i] * desc.mAspectFrames[0];

        float normFactor = sqrtf(sqr(cr) + sqr(sr));
        float speed = len(velocities[i]);

        if (desc.mVelocityStretch != 0.0f && (speed > desc.mVelocityStretch))
            normFactor /= desc.mVelocityStretch * speed;
        else
            normFactor /= speed * speed;

        normFactor *= desc.mSizeFrames[0];

        positions[i] += 0.5f * velocities[i] * normFactor;
    }
}

void nHL::ApplyRotations
(
    int         count,
    const Vec3f a0s[],
    const Vec3f a1s[],          size_t aStride,
    const float rotations[],    size_t rotationStride,
    Vec3f       a0sOut[],
    Vec3f       a1sOut[]
)
{
    for (int i = 0; i < count; i++)
    {
        Vec3f a0(*a0s);
        Vec3f a1(*a1s);

        float sr, cr;
        SinCosFast(*rotations * vl_twoPi, &sr, &cr);

        Vec3f a1d = cr * a1 + sr * a0;
        a0        = cr * a0 - sr * a1;
        a1 = a1d;

        a0sOut[i] = a0;
        a1sOut[i] = a1;

        ((uint8_t*&) a0s)        += aStride;
        ((uint8_t*&) a1s)        += aStride;
        ((uint8_t*&) rotations)  += rotationStride;
    }
}

void nHL::ApplyScalesAndAspects
(
    int         count,
    const Vec3f a0s[],
    const Vec3f a1s[],          size_t aStride,
    const float scales[],       size_t scaleStride,
    const float aspects[],      size_t aspectStride,
    Vec3f       a0sOut[],
    Vec3f       a1sOut[]
)
{
    for (int i = 0; i < count; i++)
    {
        Vec3f a0(*a0s);
        Vec3f a1(*a1s);

        a0 *= *scales * *aspects;
        a1 *= *scales;

        a0sOut[i] = a0;
        a1sOut[i] = a1;

        ((uint8_t*&) a0s)        += aStride;
        ((uint8_t*&) a1s)        += aStride;
        ((uint8_t*&) scales)     += scaleStride;
        ((uint8_t*&) aspects)    += aspectStride;
    }
}

void nHL::ApplyScalesAndAspectsToDiagonals
(
    int         count,
    const Vec3f d0s[],
    const Vec3f d1s[],          size_t dStride,
    const float scales[],       size_t scaleStride,
    const float aspects[],      size_t aspectStride,
    Vec3f       d0sOut[],
    Vec3f       d1sOut[]
)
{
    for (int i = 0; i < count; i++)
    {
        Vec3f d0(*d0s);
        Vec3f d1(*d1s);

        float a = 0.5f * (*aspects);
        float a0 = 0.5f - a;
        float a1 = 0.5f + a;

        d1 *= *scales;
        d0 *= *scales;

        Vec3f da0 = a1 * d0 + a0 * d1;
        Vec3f da1 = a1 * d1 + a0 * d0;

        d0sOut[i] = da0;
        d1sOut[i] = da1;

        ((uint8_t*&) d0s)        += dStride;
        ((uint8_t*&) d1s)        += dStride;
        ((uint8_t*&) scales)     += scaleStride;
        ((uint8_t*&) aspects)    += aspectStride;
    }
}

void nHL::ApplyVelocityStretchToDiagonals
(
    float               psVelocityStretch,
    const cTransform&   effectToWorld,
    int                 count,
    const Vec3f         velocities[],   size_t velocityStride,
    const Vec3f         d0s[],
    const Vec3f         d1s[],          size_t dStride,
    Vec3f               d0Out[],
    Vec3f               d1Out[]
)
{
    for (int i = 0; i < count; i++)
    {
        // orient-with-direction if velocity stretch is on.
        Vec3f velocity = effectToWorld.TransformDirection(*velocities);
        Vec3f d0(*d0s);
        Vec3f d1(*d1s);

        float dv0 = dot(velocity, d0);
        float dv1 = dot(velocity, d1);

        float cr = dv0 + dv1;
        float sr = dv0 - dv1;

        float nf = sqrtf(sqr(cr) + sqr(sr));
        
        if (nf > 1e-6f)
        {
            float invNF = 1.0f / nf;
            
            cr *= invNF;
            sr *= invNF;
            
            Vec3f t = cr * d0 - sr * d1;
            d1 = cr * d1 + sr * d0;
            d0 = t;

            if (nf > psVelocityStretch) // TODO: invNF < invPSStretch
            {
                float s = (nf / psVelocityStretch);
                Vec3f a1 = d0 + d1;
                a1 *= 0.5f * s - 0.5f;

                d0 += a1;
                d1 += a1;
            }
        }
        
        *d0Out = d0;
        *d1Out = d1;

        ((uint8_t*&) velocities) += velocityStride;
        ((uint8_t*&) d0s)        += dStride;
        ((uint8_t*&) d1s)        += dStride;
        d0Out++;
        d1Out++;
    }
}

void nHL::ApplyVelocityStretch
(
    float               psVelocityStretch,
    const cTransform&   effectToWorld,
    int                 count,
    const Vec3f         velocities[],   size_t velocityStride,
    const Vec3f         a0s[],
    const Vec3f         a1s[],          size_t aStride,
    Vec3f               a0Out[],
    Vec3f               a1Out[]
)
{
    for (int i = 0; i < count; i++)
    {
        // orient-with-direction if velocity stretch is on.
        Vec3f velocity = effectToWorld.TransformDirection(*velocities);
        Vec3f a0(*a0s);
        Vec3f a1(*a1s);

        float cr = dot(velocity, a1);
        float sr = dot(velocity, a0);

        float nf = sqrtf(sqr(cr) + sqr(sr));
        
        if (nf > 1e-6f)
        {
            float invNF = 1.0f / nf;
            
            cr *= invNF;
            sr *= invNF;
            
            Vec3f t = cr * a0 - sr * a1;
            a1      = cr * a1 + sr * a0;
            a0      = t;

            if (nf > psVelocityStretch) // TODO: invNF < invPSStretch
                a1 *= (nf / psVelocityStretch);
        }
        
        *a0Out = a0;
        *a1Out = a1;

        ((uint8_t*&) velocities) += velocityStride;
        ((uint8_t*&) a0s)        += aStride;
        ((uint8_t*&) a1s)        += aStride;
        a0Out++;
        a1Out++;
    }
}

void nHL::FindTiles
(
    const cParticleTileInfo& psi,
    int                 count,
    const uint8_t       startFrames[],  size_t startFrameStride,
    const tPtAge        ages[],
    const tPtAge        ageSteps[],     size_t ageStride,
    cParticleTile       tiles[]
)
{
    for (int i = 0; i < count; i++)
    {
        float    frameT = *ages * psi.mTimeScale;
        float    fti = floorf(frameT);

        int      frame0 = RoundToSInt32(fti);

        // this windows the frame count for picking out sequences within the larger tile set
        if (psi.mCount)
            frame0 = frame0 % psi.mCount;
        frame0 += *startFrames;

        uint32_t itu0 = (frame0)               & psi.mMask[0];
        uint32_t itv0 = (frame0 >> psi.mShift) & psi.mMask[1];

        tiles[i].mU = itu0;
        tiles[i].mV = itv0;

        ((uint8_t*&) startFrames) += startFrameStride;
        ((uint8_t*&) ages)        += ageStride;
        ((uint8_t*&) ageSteps)    += ageStride;
    }
}


void nHL::FindTilePairs
(
    const cParticleTileInfo& psi,
    int               count,
    const uint8_t     startFrames[],  size_t startFrameStride,
    const tPtAge      ages[],
    const tPtAge      ageSteps[],     size_t ageStride,
    cParticleTilePair tiles[]
)
{
    for (int i = 0; i < count; i++)
    {
        float    frameT = *ages * psi.mTimeScale;
        float    fti = floorf(frameT);
        frameT -= fti;

        int      frame0 = RoundToSInt32(fti);
        int      frame1 = frame0 + 1;

        // this windows the frame count for picking out sequences within the larger tile set
        frame0 = frame0 % psi.mCount;
        frame1 = frame1 % psi.mCount;

        frame0 += *startFrames;
        frame1 += *startFrames;

        uint32_t itu0 = (frame0)               & psi.mMask[0];
        uint32_t itv0 = (frame0 >> psi.mShift) & psi.mMask[1];
        uint32_t itu1 = (frame1)               & psi.mMask[0];
        uint32_t itv1 = (frame1 >> psi.mShift) & psi.mMask[1];
        
        tiles[i].mT  = frameT;
        tiles[i].mU0 = itu0;
        tiles[i].mV0 = itv0;
        tiles[i].mU1 = itu1;
        tiles[i].mV1 = itv1;

        ((uint8_t*&) startFrames) += startFrameStride;
        ((uint8_t*&) ages)        += ageStride;
        ((uint8_t*&) ageSteps)    += ageStride;
    }
}


void nHL::ApplyScale
(
    const float scale,

    int         count,
    const float data[], size_t dataStride,
    float       dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimFloat;
        dataStride = 0;
    }

    for (int i = 0; i < count; i++)
    {
        dataOut[i] = scale * (*data);
        ((uint8_t*&) data) += dataStride;
    }
}

void nHL::ApplyScale
(
    float scale,

    int         count,
    const Vec3f data[], size_t dataStride,
    Vec3f       dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimVec3f;
        dataStride = 0;
    }
    
    for (int i = 0; i < count; i++)
    {
        dataOut[i] = scale * (*data);
        ((uint8_t*&) data) += dataStride;
    }
}

void nHL::ApplyScale
(
    const Vec3f scale,

    int         count,
    const Vec3f data[], size_t dataStride,
    Vec3f       dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimVec3f;
        dataStride = 0;
    }
    
    for (int i = 0; i < count; i++)
    {
        dataOut[i] = scale * (*data);
        ((uint8_t*&) data) += dataStride;
    }
}

void nHL::ApplyDelta
(
    const float delta,

    int         count,
    const float data[], size_t dataStride,
    float       dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimFloat;
        dataStride = 0;
    }

    for (int i = 0; i < count; i++)
    {
        dataOut[i] = delta + (*data);
        ((uint8_t*&) data) += dataStride;
    }
}

void nHL::ApplyDelta
(
    const Vec3f delta,

    int         count,
    const Vec3f data[], size_t dataStride,
    Vec3f       dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimVec3f;
        dataStride = 0;
    }
    
    for (int i = 0; i < count; i++)
    {
        dataOut[i] = delta + (*data);
        ((uint8_t*&) data) += dataStride;
    }
}

void ApplyWarps
(

)
{
    // if (...) ApplyScrew
    // if (...) ApplySpiral
    // if (...) ApplyScrewRate
}

void ApplyScrew
(
    float               rate,
    const cTransform&   sourceToEffect,
    int                 count,
    Vec3f               positions[]
)
{
    Vec3f origin = sourceToEffect.mTranslation;

    for (int i = 0; i < count; i++)
    {
        Vec3f localPos(positions[i] - origin);
        
        float sf, cf;
        SinCosFast(localPos[2] * rate, &sf, &cf);   // rotation based on height

        positions[i][0] = localPos[0] * cf - localPos[1] * sf + origin[0];
        positions[i][1] = localPos[0] * sf + localPos[1] * cf + origin[1];
    }
}

void ApplySpiral
(
    float               rate,
    const cTransform&   sourceToEffect,
    int                 count,
    Vec3f               positions[]
)
{
    Vec3f origin = sourceToEffect.mTranslation;
    
    for (int i = 0; i < count; i++)
    {
        Vec3f localPos(positions[i] - origin);
        
        float sf, cf;
        SinCosFast(sqrtf(sqr(localPos[0]) + sqr(localPos[1])) * rate, &sf, &cf);   // turn based on horizontal distance

        positions[i][0] = localPos[0] * cf - localPos[1] * sf + origin[0];
        positions[i][1] = localPos[0] * sf + localPos[1] * cf + origin[1];
    }
}

#if 0


void ApplyWiggles
(
    float t,
    const tWiggleInfo& wiggles,
    const cTransform& sourceToEffect,
    int     count,
    Vec3f   positions[]
)
{
    // ...
}


#endif

class cCurveVec3f;
class cCurveFloat;

Vec3f CurveEval(const cCurveVec3f*, float)
{
    return vl_0;
}

float CurveEval(const cCurveFloat*, float)
{
    return vl_0;
}

void ApplyLoopBox
(
    const cTransform& sourceToEffect,
    const cBounds3& emitVol,
    int count,
    const cCurveVec3f* colourCurve,
    const cCurveFloat* alphaCurve,
    Vec3f positions[],
    Vec3f colours[],
    Vec3f alphas[]
)
{
    for (int i = 0; i < count; i++)
    {
        Vec3f wrapPosition(positions[i]);

        wrapPosition -= sourceToEffect.mTranslation;
        wrapPosition = emitVol.MapToLocal(wrapPosition);

        wrapPosition[0] -= floorf(wrapPosition[0]);
        wrapPosition[1] -= floorf(wrapPosition[1]);
        wrapPosition[2] -= floorf(wrapPosition[2]);

        float loopD = ClampUnit(dot(wrapPosition - Vec3f(0.5f), sourceToEffect.Axis(vl_y)) + 0.5f);

        if (colours)
            colours[i] *= CurveEval(colourCurve, loopD);

        if (alphas)
            alphas[i] *= CurveEval(alphaCurve, loopD);

        wrapPosition = emitVol.MapFromLocal(wrapPosition);
        wrapPosition += sourceToEffect.mTranslation;
        
        positions[i] = wrapPosition;
    }
}

namespace
{
    void ConvertToDiagonals
    (
        int         count,
        const Vec3f a0s[],
        const Vec3f a1s[],          size_t aStride,
        Vec3f       d0sOut[],
        Vec3f       d1sOut[]
    )
    {
        for (int i = 0; i < count; i++)
        {
            Vec3f a0(*a0s);
            Vec3f a1(*a1s);

            d0sOut[i] = a1 - a0;
            d1sOut[i] = a1 + a0;

            ((uint8_t*&) a0s)        += aStride;
            ((uint8_t*&) a1s)        += aStride;
        }
    }

}

void nHL::DispatchParticles
(
    cIRenderer*         renderer,
    int                 quadMesh,

    const cParticlesDispatchDesc& desc,
    const cTransform&   sourceToEffect,
    const cTransform&   effectToWorld,
    const cTransform&   cameraToWorld,

    const cDispatchScale* dispatchScale,

    int                 particlesCount,
    const tPtAge        ages[],
    const tPtAge        ageSteps[],     size_t ageStride,
    const Vec3f         positions[],    size_t positionStride,
    const Vec3f         velocities[],   size_t velocityStride,
    const Vec3f         colours[],      size_t colourStride,
    const float         alphas[],       size_t alphaStride,
    const float         sizes[],        size_t sizeStride,
    const float         rotations[],    size_t rotationStride,
    const float         aspects[],      size_t aspectStride,
    const uint8_t       frames[],       size_t frameStride
)
{
    CL_ASSERT(!(colours == 0    && colourStride != 0));
    CL_ASSERT(!(alphas == 0     && alphaStride != 0));
    CL_ASSERT(!(sizes == 0      && sizeStride != 0));
    CL_ASSERT(!(rotations == 0  && rotationStride != 0));
    CL_ASSERT(!(aspects == 0    && aspectStride != 0));

    nHL::cQuadVertex* v;
    int maxQuads = renderer->GetQuadBuffer(quadMesh, particlesCount, (uint8_t**) &v);
    int quadsWritten = 0;
    uint8_t startFrame = 0;

    const int kPtBatchSize = 256;

    // intermediate storage
    Vec3f c         [kPtBatchSize];
    float a         [kPtBatchSize];
    Vec3f d0        [kPtBatchSize];
    Vec3f d1        [kPtBatchSize];
    float scales    [kPtBatchSize];
    float r         [kPtBatchSize];
    float as        [kPtBatchSize];
    cParticleTile ti[kPtBatchSize];

    // Defaults
    if (!alphas)
        alphas = &kDefaultAnimFloat;
    if (!colours)
        colours = &kDefaultAnimVec3f;

    Vec3f axes[2];
    FindParticleAxes(desc.mAlignment, sourceToEffect, effectToWorld, cameraToWorld, desc.mAlignDir, axes);
    effectToWorld.Inverse().TransformDirections(CL_SIZE(axes), axes);

    cParticleTileInfo tileInfo;
    bool tiledParticles = InitParticleTileInfo(&tileInfo, desc.mTilesU, desc.mTilesV, desc.mTilesSpeed, desc.mTilesCount);

    for (int i = 0, n = particlesCount; ; )
    {
        int skippedParticles = 0;

        while (IsExpired(*ages))  // skip expired particles at the start
        {
            ((uint8_t*&) ages) += ageStride;
            skippedParticles++;

            if (++i == n)
                break;
        }

        if (i == n)
            break;

        if (skippedParticles)
        {
            ((uint8_t*&) ageSteps)      += skippedParticles * ageStride;

            ((uint8_t*&) positions)     += skippedParticles * positionStride;
            ((uint8_t*&) velocities)    += skippedParticles * velocityStride;

            ((uint8_t*&) colours)       += skippedParticles * colourStride;
            ((uint8_t*&) alphas   )     += skippedParticles * alphaStride;

            ((uint8_t*&) sizes    )     += skippedParticles * sizeStride;
            ((uint8_t*&) rotations)     += skippedParticles * rotationStride;
            ((uint8_t*&) aspects  )     += skippedParticles * aspectStride;

            ((uint8_t*&) frames   )     += skippedParticles * frameStride;
        }

        int count = min(n - i, maxQuads - quadsWritten);
        count = min(count, kPtBatchSize);
        const tPtAge* agesPeek = ages;

        for (int j = 0; j < count; j++)     // find max 'live' span
        {
            if (IsExpired(*agesPeek))
            {
                count = j;
                break;
            }

            ((uint8_t*&) agesPeek) += ageStride;
        }

        const Vec3f* cIn = colours;
        size_t       cInStride = colourStride;
        const float* aIn = alphas;
        size_t       aInStride = alphaStride;

        if (tiledParticles)
            FindTiles(tileInfo, count, frames, frameStride, ages, ageSteps, ageStride, ti);

        const float* sIn = sizes;
        size_t sInStride = sizeStride;

        if (!desc.mSizeFrames.empty())
        {
            ApplyLinearAnim(desc.mSizeFrames.size(), desc.mSizeFrames.data(), count, ages, ageStride, sIn, sInStride, scales);
            sIn = scales;
            sInStride = sizeof(scales[0]);
        }

        if (dispatchScale && dispatchScale->mSize != 1.0f)
        {
            ApplyScale(dispatchScale->mSize, count, sIn, sInStride, scales);
            sIn = scales;
            sInStride = sizeof(scales[0]);
        }

        const float* asIn = aspects;
        size_t asInStride = aspectStride;

        if (!desc.mAspectFrames.empty())
        {
            ApplyLinearAnim(desc.mAspectFrames.size(), desc.mAspectFrames.data(), count, ages, ageStride, asIn, asInStride, as);

            asIn = as;
            asInStride = sizeof(as[0]);
        }

        const Vec3f* d0In = &axes[0];
        const Vec3f* d1In = &axes[1];
        size_t dInStride = 0;

        const float* rIn = rotations;
        size_t rInStride = rotationStride;

        if (!desc.mRotateFrames.empty())
        {
            ApplyLinearAnim(desc.mRotateFrames.size(), desc.mRotateFrames.data(), count, ages, ageStride, rIn, rInStride, r);

            rIn = r;
            rInStride = sizeof(r[0]);
        }

        // TODO: didn't this used to interoperate with mRotateVary, so you could get +- (offset + abs(vary)) ?
        if (desc.mRotateOffset != 0.0f)
        {
            ApplyDelta(desc.mRotateOffset, count, rIn, rInStride, r);

            rIn = r;
            rInStride = sizeof(r[0]);
        }

        ///////////////////////////////////////

        if (rIn)
        {
            ApplyRotations(count, d0In, d1In, dInStride, rIn, rInStride, d0, d1);

            d0In = d0;
            d1In = d1;
            dInStride = sizeof(d0[0]);
        }

        if (sIn || asIn)
        {
            ApplyScalesAndAspects(count, d0In, d1In, dInStride, sIn ? sIn : &kDefaultAnimFloat, sInStride, asIn ? asIn : &kDefaultAnimFloat, asInStride, d0, d1);

            d0In = d0;
            d1In = d1;
            dInStride = sizeof(d0[0]);
        }

        ////////////////////////////////////////

        if (!desc.mColourFrames.empty())
        {
            ApplyLinearAnim(desc.mColourFrames.size(), desc.mColourFrames.data(), count, ages, ageStride,  cIn, cInStride, c);
            cIn = c;
            cInStride = sizeof(c[0]);
        }

        if (dispatchScale && dispatchScale->mColour != vl_1)
        {
            ApplyScale(dispatchScale->mColour, count, cIn, cInStride, c);
            cIn = c;
            cInStride = sizeof(c[0]);
        }

        if (!desc.mAlphaFrames.empty())
        {
            ApplyLinearAnim(desc.mAlphaFrames.size(), desc.mAlphaFrames.data(), count, ages, ageStride, aIn, aInStride, a);
            aIn = a;
            aInStride = sizeof(a[0]);
        }

        if (dispatchScale && dispatchScale->mAlpha != 1.0f)
        {
            ApplyScale(dispatchScale->mAlpha, count, aIn, aInStride, a);
            aIn = a;
            aInStride = sizeof(a[0]);
        }

        if (velocities && desc.mVelocityStretch != 0.0f)
        {
            ApplyVelocityStretch
            (
                desc.mVelocityStretch,
                effectToWorld,
                count,
                velocities, velocityStride,
                d0In, d1In, dInStride,
                d0, d1
            );

            d0In = d0;
            d1In = d1;
            dInStride = sizeof(d0[0]);
        }

        if (d0In == d0)
            ConvertToDiagonals(count, d0In, d1In, dInStride, d0, d1);
        else
        {
            Vec3f t = axes[1] - axes[0];
            axes[1] = axes[1] + axes[0];
            axes[0] = t;
        }

        int writeCount;

        if (tiledParticles)
            writeCount = nHL::WriteQuads
            (
                count,
                positions,  positionStride,
                d0In, d1In, dInStride,
                cIn,        cInStride,
                aIn,        aInStride,
                ti,         sizeof(ti[0]),
                tileInfo.mDT,
                &v
            );
        else
            writeCount = nHL::WriteQuads
            (
                count,
                positions,  positionStride,
                d0In, d1In, dInStride,
                cIn,        cInStride,
                aIn,        aInStride,
                &v
            );

        i += count;
        quadsWritten += writeCount;

        CL_ASSERT(i <= n);

        if (i == n)
            break;

        ((uint8_t*&) ages)          += ageStride * count;
        ((uint8_t*&) ageSteps)      += ageStride * count;

        ((uint8_t*&) positions)     += positionStride * count;
        ((uint8_t*&) velocities)    += velocityStride * count;

        ((uint8_t*&) colours)       += colourStride * count;
        ((uint8_t*&) alphas   )     += alphaStride * count;

        ((uint8_t*&) sizes    )     += sizeStride * count;
        ((uint8_t*&) rotations)     += rotationStride * count;
        ((uint8_t*&) aspects  )     += aspectStride * count;

        if (quadsWritten == maxQuads)
        {
            renderer->DispatchAndReleaseBuffer(quadMesh, quadsWritten);
            quadsWritten = 0;
            maxQuads = renderer->GetQuadBuffer(quadMesh, n - i, (uint8_t**) &v);
        }
    }

    renderer->DispatchAndReleaseBuffer(quadMesh, quadsWritten);
}


/// --- WriteQuads -------------------------------------------------------------

// CPU-side quad buffer fill routines. The important thing here, as with the
// particle processing routines, is that we write in-order with no holes, to
// take advantage of write combining.


int nHL::WriteQuads
(
    int count,
    const Vec3f positions[],    size_t positionStride,
    const Vec3f d0s[],
    const Vec3f d1s[],          size_t dStride,
    const Vec3f colours[],      size_t colourStride,
    const float alphas[],       size_t alphaStride,
    cQuadVertex* vertices[]
)
{
    cQuadVertex* v = *vertices;
    int written = 0;

    for (int i = 0; i < count; i++)
    {
        Vec3f p = *positions;
        cRGBA32 c = ColourAlphaToRGBA32(cColourAlpha(*colours, *alphas));

        v->mPosition = p + *d0s;
        v->mUV       = vl_0;
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p - *d1s;
        v->mUV       = vl_y;
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p - *d0s;
        v->mUV       = vl_1;
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p + *d1s;
        v->mUV       = vl_x;
        v->mColour   = c.mAsUInt32;
        v++;

        written++;

        ((uint8_t*&) positions) += positionStride;
        ((uint8_t*&) d0s)       += dStride;
        ((uint8_t*&) d1s)       += dStride;
        ((uint8_t*&) colours)   += colourStride;
        ((uint8_t*&) alphas)    += alphaStride;
    }

    *vertices = v;
    return written;
}

int nHL::WriteQuads
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
)
{
    cQuadVertex* v = *vertices;
    int written = 0;

    for (int i = 0; i < count; i++)
    {
        Vec3f p = *positions;
        cRGBA32 c = ColourAlphaToRGBA32(cColourAlpha(*colours, *alphas));
        Vec2f tp(tiles->mU * tw[0], tiles->mV * tw[1]);

        v->mPosition = p + *d0s;
        v->mUV[0]    = tp[0];
        v->mUV[1]    = tp[1];
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p - *d1s;
        v->mUV[0]    = tp[0];
        v->mUV[1]    = tp[1] + tw[1];
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p - *d0s;
        v->mUV[0]    = tp[0] + tw[0];
        v->mUV[1]    = tp[1] + tw[1];
        v->mColour   = c.mAsUInt32;
        v++;

        v->mPosition = p + *d1s;
        v->mUV[0]    = tp[0] + tw[0];
        v->mUV[1]    = tp[1];
        v->mColour   = c.mAsUInt32;
        v++;

        written++;

        ((uint8_t*&) positions) += positionStride;
        ((uint8_t*&) d0s)       += dStride;
        ((uint8_t*&) d1s)       += dStride;
        ((uint8_t*&) colours)   += colourStride;
        ((uint8_t*&) alphas)    += alphaStride;
        ((uint8_t*&) tiles)     += tileStride;
    }

    *vertices = v;
    return written;
}

void nHL::UpdatePhysicsSimple
(
    const cParticlesPhysicsDesc& desc,

    int             count,
    const float     dts[],          size_t dtStride,
    Vec3f           positions[],    size_t positionStride,
    Vec3f           velocities[],   size_t velocityStride
)
// No numerical integration is needed: the only forces are directional.
{
    for (int i = 0; i < count; i++)
    {
        Vec3f& p = *positions;

        Vec3f  v0 = *velocities;
        Vec3f& v1 = *velocities;

        float dt = *dts;

        // forces
        v1 += desc.mDirForces * dt;
        // this only works well if the drag is small, otherwise a timestepped approach is needed
        v1 *= (1.0f - (desc.mDrag * dt));

// TODO        CL_ASSERT(!HasNAN(v1));

        // update position using midpoint velocity
        p += (v0 + v1) * (0.5f * dt);

        ((uint8_t*&) dts)        += dtStride;
        ((uint8_t*&) positions)  += positionStride;
        ((uint8_t*&) velocities) += velocityStride;
    }
}

// ApplyHeightMapForces: position, velocity

void nHL::UpdatePhysicsTimeStep
(
    const cParticlesPhysicsDesc& desc,
    float           physicsStep,

    int             count,
    const float     dts[],          size_t dtStride,
    const Vec3f     forces[],       size_t forceStride,
    Vec3f           positions[],    size_t positionStride,
    Vec3f           velocities[],   size_t velocityStride
)
// No numerical integration is needed: the only forces are directional.
{
    if (!forces)
    {
        forces = &desc.mDirForces;
        forceStride = 0;
    }

    for (int i = 0; i < count; i++)
    {
        Vec3f& p = *positions;
        Vec3f& v = *velocities;

        float dt = *dts;

        do
        {
            float physicsTick = fminf(dt, physicsStep);
            Vec3f p0 = p;

            // Position update using current velocity
            p += v * physicsTick;

            // Velocity & drag update
            v += *forces * physicsTick;
            v *= (1 - desc.mDrag * physicsTick);

            // TODO: at this point update p0/p and v according to surfaces and maps
            // or perhaps more flexibly, return updated p0, and leave that to other routines?

//            CL_ASSERT(!HasNAN(v1));

            dt -= physicsTick;
        }
        while (dt > 0);

        ((uint8_t*&) dts)        += dtStride;
        ((uint8_t*&) positions)  += positionStride;
        ((uint8_t*&) velocities) += velocityStride;
        ((uint8_t*&) forces)     += forceStride;
    }
}

bool ApplyAttractor
(
    float       range,
    float       intensity,
    int         numStrengths,
    const float strengths[],
    float       killRange,
    Vec3f       ap,

    int             count,
    const Vec3f     positions[],    size_t positionStride,
    Vec3f           forces[],       size_t forceStride
)
{
    float invRange = 1.0f / range;
    
    for (int i = 0; i < count; i++)
    {
        const Vec3f& p = *positions;

        Vec3f dp(ap - p);

        float d2 = sqrlen(dp);

//        if (d < killRange)
//            *ages = kPtAgeExpired;   // mark as dead?

        if (d2 != 0.0f)
        {
            // here the anim driver is a float param, so we're forced to bounce
            // float->int at least once.
            float d = sqrtf(d2);

//            if (d < killRange)
//                *ages = kPtAgeExpired;   // mark as dead?

            float s = fminf(d * invRange, 1.0f);

            float cs = s * (numStrengths - 1);
            float cf = floorf(cs);
            cs -= cf;
            int ci = RoundToSInt32(cf);

            float scale = lerp(strengths[ci], strengths[ci + 1], cs) * intensity / d;

            *forces += dp * scale;
        }

        ((uint8_t*&) positions)  += positionStride;
        ((uint8_t*&) forces)     += forceStride;
    }

    return true;
}
