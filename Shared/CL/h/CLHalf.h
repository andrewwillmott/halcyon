//
//  File:       CLHalf
//
//  Function:   Support for 16-bit floating point numbers.
//
//  Author(s):  Andrew Willmott
//

#ifndef CL_HALF_H
#define CL_HALF_H

#include <stdint.h>

#ifndef HALF_DENORMS
    #define HALF_DENORMS 1
#endif
#ifndef HALF_NANS
    #define HALF_NANS 1
#endif

namespace nCL
{
    struct half;

    half  FloatToHalf(float f);
    float HalfToFloat(half f);

    // Constructor constants
    enum tFromHalfIntRep
    {
        kHalfZero           = 0x0000,
        kHalfOne            = 0x3c00,
        kHalfPosInfinity    = 0x7c00,
        kHalfNegInfinity    = 0xfc00,
        kFromHalfIntRepMax  = 0xFFFF
    };

    // A half is a floating point number with 1 sign bit, 5 exponent bits, and 10 mantissa bits.
    struct half
    {
        half() {}
        half(const half& f) : mAsInt(f.mAsInt)  {}
        half(float f)                           { *this = FloatToHalf(f); }

        half(tFromHalfIntRep intRep) : mAsInt(intRep) {}
        
        operator float()       { return HalfToFloat(*this); }
        operator float() const { return HalfToFloat(*this); };

        uint16_t mAsInt;
    };

#if HALF_DENORMS
    // min exponent = 0 -> 2^-14, min mantissa = 1 -> 1/1024
    const float HALF_MIN_F = 5.96046448e-08f;  // 0x0001
    const half  HALF_MIN = { 0x0001 };
#else
    // min exponent = 1 -> 2^-14, min mantissa = 0 -> 1
    const float HALF_MIN_F = 6.10351562e-05f;  // 0x0400
    const half  HALF_MIN = { 0x0400 };
#endif
    // max exponent = 30 -> 2^15, max mantissa = 1023 -> 1 + 1023/1024
    const float HALF_MAX_F = 65504.0f;    // 0x7bff
    const half  HALF_MAX = { 0x7BFF };

    bool is_infinite(half f);
    bool is_finite  (half f);
    bool is_nan     (half f);



    // --- Inlines -------------------------------------------------------------

    const uint32_t kExp16To32[32] =     // direct exponent lookup, saves branch
    {   0 << 23, 113 << 23, 114 << 23, 115 << 23, 116 << 23, 117 << 23, 118 << 23, 119 << 23, 120 << 23, 121 << 23, 122 << 23, 123 << 23, 124 << 23, 125 << 23, 126 << 23, 127 << 23,        // 0 -> 0 (zero or denorm), 31 -> 255 (inf/nan), otherwise + 127 - 15 for bias
      128 << 23, 129 << 23, 130 << 23, 131 << 23, 132 << 23, 133 << 23, 134 << 23, 135 << 23, 136 << 23, 137 << 23, 138 << 23, 139 << 23, 140 << 23, 141 << 23, 142 << 23, 255 << 23 };

    inline float HalfToFloat(half f)
    {
        uint32_t e32 = kExp16To32[(f.mAsInt >> 10) & 0x1F];

        union { float f; uint32_t i; } x;
        x.i = e32 | ((f.mAsInt & 0x8000) << 16) | ((f.mAsInt & 0x03FF) << 13);
        
    #if HALF_DENORMS
        if (e32 == 0 && (f.mAsInt & 0x03FF))
            x.f *= 5.19229685853e+33f;          // this is 2^112 -- bumps the denorm'd number back into the right range.
    #endif
        
        return x.f;
    }

    inline half FloatToHalf(float fIn)
    {
        if (fIn == 0.0f)
            return half(kHalfZero);
        
        union { float f; uint32_t i; } x = { fIn };
        
        uint32_t e32 = (x.i >> 23) & 0xFF;
        int32_t e16 = e32 + 15 - 127; // rebias

        if (e16 > 0 && e16 < 31)   // simple case
        {
            uint32_t roundedMantissa = (x.i & 0x007fffff) + 0x00001000;

            uint16_t halfRep = ((x.i & 0x80000000) >> 16) | (e16 << 10) | (roundedMantissa >> 13);

            return half(tFromHalfIntRep(halfRep));
        }
        
        if (e16 <= 0)
        {
        #if HALF_DENORMS
            if (e16 >= -9)
            {
                // denormalized
                uint32_t denormMantissa = ((x.i & 0x007fffff) | 0x00800000);    // roll in the implicit one
                
                half result;
                result.mAsInt = ((x.i & 0x80000000) >> 16) | (denormMantissa >> (14 - e16));
                return result;
            }
        #endif
        
            // underflow.
            return half(kHalfZero);
        }
        
        // overflow/infinities
        half result;
        result.mAsInt = ((x.i & 0x80000000) >> 16) | 0x7C00;        // +/- inf
        
    #if HALF_NANS
        if (e32 == 255)
            result.mAsInt |= (x.i & 0x007fffff) >> 13;
    #endif
        // Otherwise we effectively map NANs to +/- inf.

        return result;
    }

    inline bool is_infinite(half f)
    {
        return (f.mAsInt & 0x7FFF) == 0x7c00;
    }

    inline bool	is_finite(half f)
    {
        return (f.mAsInt & 0x7c00) != 0x7c00;
    }

    inline bool is_nan(half f)
    {
        return (f.mAsInt & 0x7C00) == 0x7C00 && (f.mAsInt & 0x03FF) != 0;
    }
}

#endif
