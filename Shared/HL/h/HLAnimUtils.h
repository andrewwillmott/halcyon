//
//  File:       HLAnimUtils.h
//
//  Function:   Animation utilities
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_ANIM_UTILS_H
#define HL_ANIM_UTILS_H

#include <HLDefs.h>

#include <CLMath.h>

class Vec3f;

namespace nHL
{
    // --- Age Management ------------------------------------------------------

    // We do this in fixed point to make linear animation lookups faster.
    const float kPtAgeIota    = 0.001f;             ///< minimum granularity of time in seconds
    const float kPtAgeInvIota = 1.0f / kPtAgeIota;

    typedef uint32_t tPtAge;              // Fixed-point particle age
    typedef uint64_t tPtAgeMul;           // Potentially extended type to hold multiplies

    const int   kPtAgeFractionBits = 16;
    const int   kPtAgeFractionMax  = (1 << kPtAgeFractionBits);
    const int   kPtAgeFractionMask = (1 << kPtAgeFractionBits) - 1;
    const float kPtAgeFractionScale = 1.0f / kPtAgeFractionMask;

    const tPtAge kPtAgeExpired = kPtAgeFractionMask;     ///< Value indicating the particle is dead

    tPtAge LifeToAgeStep      (float s);    ///< Converts lifetime in seconds to corresponding particle age delta
    tPtAge DeltaSecondsToIotas(float s);    ///< Converts delta time to multiplier for particle age deltas

    tPtAge ClampAge (tPtAge age);           ///< Clamps the age to 0-expired range.
    tPtAge WrapAge  (tPtAge age);           ///< Wraps the age into 0-expired range.
    bool   IsExpired(tPtAge age);           ///< Returns true if this entity is now expired and should be ignored/removed.


    // --- Piecewise-linear animation support ----------------------------------

    // Age handling

    tPtAge UpdatedAge(tPtAge age, tPtAge ageStep, float dt);
    ///< Returns updated age according to given dt. 'ageStep' should be calulated with LifeToAgeStep();

    void UpdateAges
    (
        float           dt,
        int             count,
        const tPtAge    ages[],
        const tPtAge    ageSteps[],     size_t ageStride,
        tPtAge          agesOut[]
    );
    ///< Update ages, and allow looping.

    void UpdateAgesWrap
    (
        float           dt,
        int             count,
        const tPtAge    ages[],
        const tPtAge    ageSteps[],     size_t ageStride,
        tPtAge          agesOut[]
    );
    ///< Update ages. IsExpired() can be used to test if an age has expired.

    void UpdateAges
    (
        int             count,
        float           ageScale,
        const float     dts[],      size_t dtStride,
        const tPtAge    ages[],
        const tPtAge    ageSteps[], size_t ageStride,
        tPtAge          agesOut[]
    );
    ///< Per-particle dt for time-distributed creation

    void UpdateAgesWrap
    (
        int             count,
        float           ageScale,
        const float     dts[],      size_t dtStride,
        const tPtAge    ages[],
        const tPtAge    ageSteps[], size_t ageStride,
        tPtAge          agesOut[]
    );
    ///< Per-particle dt for time-distributed creation


    // Linear animation

    float LinearAnim(int numFrames, const float frameValues[], float age);
    Vec3f LinearAnim(int numFrames, const Vec3f frameValues[], float age);
    ///< Least efficient version -- age must be in [0, 1)

    float LinearAnim(int numFrames, const float frameValues[], tPtAge age);
    Vec3f LinearAnim(int numFrames, const Vec3f frameValues[], tPtAge age);
    ///< Faster, but use ApplyLinearAnim if possible to process batches.

    void ApplyLinearAnim
    (
        int         numFrames,
        const float frameValues[],

        int          count,
        const tPtAge ages[],    size_t ageStride,
        const float  data[],    size_t dataStride,
        float        dataOut[]
    );

    void ApplyLinearAnim
    (
        int         numFrames,
        const Vec3f frameValues[],

        int          count,
        const tPtAge ages[],    size_t ageStride,
        const Vec3f  data[],    size_t dataStride,
        Vec3f        dataOut[]
    );


    // --- Inlines -------------------------------------------------------------

    inline tPtAge LifeToAgeStep(float life)
    {
        if (life < kPtAgeIota)
            return kPtAgeFractionMask;

        return nCL::RoundToSInt32(kPtAgeFractionMask * (kPtAgeIota / life));
    }

    inline tPtAge DeltaSecondsToIotas(float s)
    {
        return nCL::RoundToSInt32(s * kPtAgeInvIota);
    }

    inline tPtAge ClampAge(tPtAge age)
    {
        return age > kPtAgeExpired ? kPtAgeExpired : age;
    }

    inline tPtAge WrapAge(tPtAge age)
    {
        return age & kPtAgeFractionMask;
    }

    inline bool IsExpired(tPtAge age)
    {
        return age >= kPtAgeExpired;
    }

    inline tPtAge UpdatedAge(tPtAge age, tPtAge ageStep, float dt)
    {
        tPtAge da = DeltaSecondsToIotas(dt);
        return ClampAge(age + da * ageStep);
    }
}

#endif
