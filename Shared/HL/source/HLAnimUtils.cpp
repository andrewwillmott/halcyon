//
//  File:       HLAnimUtils.cpp
//
//  Function:   Animation utilities
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLAnimUtils.h>

#include <VL234f.h>

using namespace nHL;
using namespace nCL;

namespace
{
    const float kDefaultAnimFloat = 1.0f;
    const Vec3f kDefaultAnimVec3f = vl_1;
}

void nHL::UpdateAges
(
    float           dt,
    int             count,
    const tPtAge    ages[],
    const tPtAge    ageSteps[],     size_t ageStride,
    tPtAge          agesOut[]
)
{
    tPtAgeMul da = DeltaSecondsToIotas(dt);

    for (int i = 0; i < count; i++)
    {
        tPtAgeMul age = *ages + da * *ageSteps;

        // TODO: should we clamp here?
        agesOut[i] = age;

        ((uint8_t*&) ages)        += ageStride;
        ((uint8_t*&) ageSteps)    += ageStride;
    }
}

void nHL::UpdateAgesWrap
(
    float           dt,
    int             count,
    const tPtAge    ages[],
    const tPtAge    ageSteps[],     size_t ageStride,
    tPtAge          agesOut[]
)
{
    tPtAgeMul da = DeltaSecondsToIotas(dt);

    for (int i = 0; i < count; i++)
    {
        tPtAgeMul age = *ages + da * *ageSteps;

        age &= kPtAgeFractionMask;  // wrap around

        agesOut[i] = age;

        ((uint8_t*&) ages)        += ageStride;
        ((uint8_t*&) ageSteps)    += ageStride;
    }
}

void nHL::UpdateAges
(
    int             count,
    float           ageScale,
    const float     dts[],      size_t dtStride,
    const tPtAge    ages[],
    const tPtAge    ageSteps[], size_t ageStride,
    tPtAge          agesOut[]
)
{
    for (int i = 0; i < count; i++)
    {
        CL_ASSERT(*dts >= 0);
        tPtAgeMul da = DeltaSecondsToIotas(*dts * ageScale);

        tPtAgeMul age = *ages + da * *ageSteps;

        agesOut[i] = age;

        ((uint8_t*&) dts)       += dtStride;
        ((uint8_t*&) ages)      += ageStride;
        ((uint8_t*&) ageSteps)  += ageStride;
    }
}

void nHL::UpdateAgesWrap
(
    int             count,
    float           ageScale,
    const float     dts[],      size_t dtStride,
    const tPtAge    ages[],
    const tPtAge    ageSteps[], size_t ageStride,
    tPtAge          agesOut[]
)
{
    for (int i = 0; i < count; i++)
    {
        CL_ASSERT(*dts >= 0);
        tPtAgeMul da = DeltaSecondsToIotas(*dts * ageScale);

        tPtAgeMul age = *ages + da * *ageSteps;

        age &= kPtAgeFractionMask;  // wrap around

        agesOut[i] = age;

        ((uint8_t*&) dts)       += dtStride;
        ((uint8_t*&) ages)      += ageStride;
        ((uint8_t*&) ageSteps)  += ageStride;
    }
}

// Animation

float nHL::LinearAnim
(
    int         numFrames,
    const float frameValues[],
    float       age
)
{
    if (numFrames < 1)
        return kDefaultAnimFloat;

    if (numFrames == 1)
        return frameValues[0];

    age -= age * 1e-6f; // deal with edge case
    CL_ASSERT(age >= 0.0f && age < 1.0f);

    float a = age * (numFrames - 1);
    int ci = FloorToSInt32(a);
    float cf = a - ci;

    CL_INDEX(ci, numFrames - 1);

    return (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci]));
}

Vec3f nHL::LinearAnim
(
    int         numFrames,
    const Vec3f frameValues[],
    float       age
)
{
    if (numFrames < 1)
        return kDefaultAnimVec3f;

    if (numFrames == 1)
        return frameValues[0];

    age -= age * 1e-6f; // deal with edge case
    CL_ASSERT(age >= 0.0f && age < 1.0f);

    float a = age * (numFrames - 1);
    int ci = FloorToSInt32(a);
    float cf = a - ci;

    CL_INDEX(ci, numFrames - 1);

    return (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci]));
}

float nHL::LinearAnim
(
    int         numFrames,
    const float frameValues[],
    tPtAge      age
)
{
    if (numFrames < 1)
        return kDefaultAnimFloat;

    if (numFrames == 1)
        return frameValues[0];

    tPtAgeMul a = age * (numFrames - 1);
    int ci = a >> kPtAgeFractionBits;
    float cf = (a & kPtAgeFractionMask) * kPtAgeFractionScale;

    CL_INDEX(ci, numFrames - 1);
    return (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci]));
}

Vec3f nHL::LinearAnim
(
    int         numFrames,
    const Vec3f frameValues[],
    tPtAge      age
)
{
    if (numFrames < 1)
        return kDefaultAnimVec3f;

    if (numFrames == 1)
        return frameValues[0];

    tPtAgeMul a = age * (numFrames - 1);
    int ci = a >> kPtAgeFractionBits;
    float cf = (a & kPtAgeFractionMask) * kPtAgeFractionScale;

    CL_INDEX(ci, numFrames - 1);
    return (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci]));
}

void nHL::ApplyLinearAnim
(
    int         numFrames,
    const float frameValues[],

    int          count,
    const tPtAge ages[],    size_t ageStride,
    const float  data[],    size_t dataStride,
    float        dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimFloat;
        dataStride = 0;
    }

    if (numFrames < 1)
    {
        if (data != dataOut)
            for (int i = 0; i < count; i++)
            {
                dataOut[i] = (*data);
                ((uint8_t*&) data) += dataStride;
            }

        return;
    }

    if (numFrames == 1)
    {
        for (int i = 0; i < count; i++)
        {
            dataOut[i] = frameValues[0] * (*data);
            ((uint8_t*&) data) += dataStride;
        }

        return;
    }

    for (int i = 0; i < count; i++)
    {
        tPtAge age(*ages);

        tPtAgeMul a = age * (numFrames - 1);
        int ci = a >> kPtAgeFractionBits;
        float cf = (a & kPtAgeFractionMask) * kPtAgeFractionScale;

        CL_INDEX(ci, numFrames - 1);
        dataOut[i] = (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci])) * (*data);

        ((uint8_t*&) ages) += ageStride;
        ((uint8_t*&) data) += dataStride;
    }
}


void nHL::ApplyLinearAnim
(
    int         numFrames,
    const Vec3f frameValues[],

    int          count,
    const tPtAge ages[],    size_t ageStride,
    const Vec3f  data[],    size_t dataStride,
    Vec3f        dataOut[]
)
{
    if (!data)
    {
        data = &kDefaultAnimVec3f;
        dataStride = 0;
    }

    if (numFrames < 1)
    {
        if (data != dataOut)
            for (int i = 0; i < count; i++)
            {
                dataOut[i] = (*data);
                ((uint8_t*&) data) += dataStride;
            }

        return;
    }

    if (numFrames == 1)
    {
        for (int i = 0; i < count; i++)
        {
            dataOut[i] = frameValues[0] * (*data);
            ((uint8_t*&) data) += dataStride;
        }

        return;
    }

    for (int i = 0; i < count; i++)
    {
        tPtAge age(*ages);

        tPtAgeMul a = age * (numFrames - 1);
        int ci = a >> kPtAgeFractionBits;
        float cf = (a & kPtAgeFractionMask) * kPtAgeFractionScale;

        CL_INDEX(ci, numFrames - 1);
        dataOut[i] = (frameValues[ci] + cf * (frameValues[ci + 1] - frameValues[ci])) * (*data);

        ((uint8_t*&) ages) += ageStride;
        ((uint8_t*&) data) += dataStride;
    }
}

