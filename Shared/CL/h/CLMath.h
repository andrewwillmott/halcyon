/*
    File:       CLMath.h

    Function:	Handy math routines

    Author:     Andrew Willmott

    Copyright:  (c) 2002, Andrew Willmott
*/

#ifndef CL_MATH_H
#define CL_MATH_H

#include <CLDefs.h>
#include <VLConstants.h>

namespace nCL
{
    // Real manipulation
    float       Abs       (float f);
    bool        IsNAN     (float f);
    bool        IsInfinity(float f);
    bool        IsNormal  (float f);        ///< Returns true if f is neither +-Inf nor a NaN

    double      Abs       (double f);
    bool        IsNAN     (double f);
    bool        IsInfinity(double f);
    bool        IsNormal  (float f);        ///< Returns true if f is neither +-Inf nor a NaN

    float Clamp        (float x, float minX, float maxX);   ///< returns x clamped to range [minX, maxX]
    float ClampPositive(float x);                           ///< returns x clamped to range [0, inf]
    float ClampLower   (float x, float minX);               ///< returns x clamped to range [x, maxX]
    float ClampUpper   (float x, float maxX);               ///< returns x clamped to range [minX, x]
    float ClampUnit    (float x, float maxX);               ///< returns x clamped to range [0, 1]


    // Real to integer conversions.
    int32_t   RoundToSInt32(float f);    // rounds to the nearest even integer
    int32_t   FloorToSInt32(float f);    // rounds downwards -- same as int32(floor())
    int32_t    CeilToSInt32(float f);    // rounds upwards   -- same as int32(ceil())

    int32_t   RoundToSInt32(double f);
    int32_t   FloorToSInt32(double f);
    int32_t    CeilToSInt32(double f);

    // Fast conversion from a [0, 1] range float to a 0-255 range byte.
    // No multiplies, no slow float->integer casts, no branches.
    uint8_t    UnitRealToUInt8     (float f);
    uint8_t    ClampUnitRealToUInt8(float f);   // clamps f to [0, 1] before conversion.
    float      UInt8ToUnitReal     (float f);   // just for convenience

    float DegreesToRadians(float degrees);
    float RadiansToDegrees(float rads);

    // Small table 'fast' approx trig functions
    float SinFast   (float theta);
    float CosFast   (float theta);
    void  SinCosFast(float theta, float* s, float* c);

    float ATan2Fast(float y, float x);
    float ASinFast (float y);
    float ACosFast (float x);

    float InvSqrtFast(float x); // fast approx to 1 / sqrt(x)

    // Standard helper for e.g. radix sort, via Mike Herf. Remaps the bit representation of
    // positive and negative floats so the resulting u32 will compare the same way.
    uint32_t OrderedU32(float f);




    //
    // Notes:
    //
    // These functions rely on the current FP rounding mode being 
    // "round to nearest". This is always the case; any other mode gives 
    // less accurate results, and can lead to convergence problems in 
    // numerical algorithms. It is only functions that must implement
    // C's brain-dead "round-towards-zero" that have to flip the rounding mode,
    // temporarily.
    // 
    // The RoundXXX functions are faster than FloorXXX and CeilXXX.
    //
    // Caveats:
    // - float numbers greater than 2^22 or less than -2^22 will not be 
    //   handled correctly. As at 2^24 you'll start losing precision
    //   anyway, this doesn't seem like too onerous a caveat.
    // - The rounding of x.5 alternates depending on whether x is even
    //   or odd. 1.5 -> 2, 2.5 -> 2, 3.5 -> 4... and so on. This is
    //   done for numerical stability in iterative routines.
    // - This has no effect on Ceil/Floor.
    //

    // Floating point constants
    // F32, aka float
    const int       kF32SignBits                = 1;
    const int       kF32ExponentBits            = 8;
    const int       kF32MantissaBits            = 23;
    const int       kF32ExpBias                 = 127;

    const uint32_t  kF32SignMask                = UINT32_C(0x80000000);
    const uint32_t  kF32ExponentMask            = UINT32_C(0x7F800000);
    const uint32_t  kF32MantissaMask            = UINT32_C(0x007FFFFF);

    const uint32_t  kF32ExponentDenorm          = UINT32_C(0x00000000);     // Exponent is 0 for a denormalised number. Mantissa is non-0 (otherwise the number is +-zero!)
    const uint32_t  kF32ExponentNaN             = UINT32_C(0x7F800000);     // Exponent is all ones for a NaN or infinity. Mantissa is 0 for infinity, has high bit of 1 for quiet NaN.
    const uint32_t  kF32PositiveInfinity        = kF32ExponentNaN | 0;

    // F64, aka double
    const int       kF64SignBits                = 1;
    const int       kF64ExponentBits            = 11;
    const int       kF64MantissaBits            = 52;
    const int       kF64ExpBias                 = 1023;

    const uint64_t  kF64SignMask                = UINT64_C(0x8000000000000000);
    const uint64_t  kF64ExponentMask            = UINT64_C(0x7FF0000000000000);
    const uint64_t  kF64MantissaMask            = UINT64_C(0x000FFFFFFFFFFFFF);

    const uint64_t  kF64ExponentDenorm          = UINT64_C(0x0000000000000000);
    const uint64_t  kF64ExponentNaN             = UINT64_C(0x7FF0000000000000);
    const uint64_t  kF64PositiveInfinity        = kF64ExponentNaN;

    const float     kF32Infinity = (const float&)  kF32PositiveInfinity;
    const double    kF64Infinity = (const double&) kF64PositiveInfinity;

    // Bias to integer
    const float     kFToIBiasF32 = uint32_t(3) << 22;
    const int32_t   kFToIBiasS32 = (int32_t&) kFToIBiasF32;
    const double    kFToIBiasF64 = uint64_t(3) << 52;

    // Bias to 8-bit fraction
    const float     kFToI8BiasF32 = uint32_t(3) << 14;
    const int32_t   kFToI8BiasS32 = (int32_t&) kFToI8BiasF32;

    // Bias to 16-bit fraction
    const float     kFToI16BiasF32 = uint32_t(3) << 6;
    const int32_t   kFToI16BiasS32 = (int32_t&) kFToI16BiasF32;



    // --- Real-to-integer conversions, 32-bit ------------------------------------

    inline int32_t RoundToSInt32(float f)
    {
        float bf = kFToIBiasF32 + f;
        return ((int32_t&) bf) - kFToIBiasS32;
    }

    inline int32_t FloorToSInt32(float f)
    {
        float bf = kFToIBiasF32 + floorf(f);
        return ((int32_t&) bf) - kFToIBiasS32;
    }

    inline int32_t CeilToSInt32(float f)
    {
        float bf = kFToIBiasF32 + ceilf(f);
        return ((int32_t&) bf) - kFToIBiasS32;
    }

    inline int32_t RealToFixedPoint32(float f)
    {
        float bf = kFToI16BiasF32 + f;
        return ((int32_t&) bf) - kFToI16BiasS32;
    }

    /** 
        Manipulates the mantissa directly to turn a FP value into an
        eight-bit quantity. This leaves us with a value from 0-256, which
        we convert to 0-255 via the (i - i >> 8) conversion.
        Respects the FPU's rounding mode, which should always be round-to-nearest.
        Ideally for this application it would be round-downwards, but we'll live 
        with the slight error.
    */
    inline uint8_t UnitRealToUInt8(float f)
    {
        float bf = kFToI8BiasF32 + f;
        uint32_t  i  = ((uint32_t&) bf) - kFToI8BiasS32;

        return uint8_t(i - (i >> 8));
    }

    inline uint8_t ClampUnitRealToUInt8(float f)
    {
       // TODO: come up with an optimized version of this: can we eliminate compares?
        if (f <= 0.0f)
            return 0;
        if (f >= 1.0f)
            return 255;

        float bf = kFToI8BiasF32 + f;
        uint32_t  i  = ((uint32_t&) bf) - kFToI8BiasS32;

        return uint8_t(i - (i >> 8));
    }

    inline float UInt8ToUnitReal(float f)
    {
        return f * (1.0f / 255.0f);
    }


    // --- Real-to-integer conversions, 32-bit ------------------------------------

#ifndef CL_SINGLE_PRECISION

    inline int32_t RoundToSInt32(double f)
    {
        double bf = kFToIBiasF64 + f;

    #ifdef CL_LITTLE_ENDIAN
        return ((int32_t*) &bf)[0];
    #else
        return ((int32_t*) &bf)[1];
    #endif
    }

    inline int32_t FloorToSInt32(double f)
    {
        double bf = kFToIBiasF64 + (f - 0.5f);

    #ifdef CL_LITTLE_ENDIAN
        int32_t  r  = ((int32_t*) &bf)[0];
    #else
        int32_t  r  = ((int32_t*) &bf)[1];
    #endif

        if (double(r + 1) == f)   // to-do: can we avoid this branch?
            r++;

        return r;
    }

    inline int32_t CeilToSInt32(double f)
    {
        double bf = kFToIBiasF64 + (f + 0.5f);

    #ifdef CL_LITTLE_ENDIAN
        int32_t  r  = ((int32_t*) &bf)[0];
    #else
        int32_t  r  = ((int32_t*) &bf)[1];
    #endif

        if (double(r - 1) == f)
            r--;

        return r;
    }

    inline uint32_t RoundToUInt32(double f)
    {
        return RoundToSInt32(f);
    }

    inline uint32_t FloorToUInt32(double f)
    {
        return FloorToSInt32(f);
    }

    inline uint32_t CeilToUInt32(double f)
    {
        return CeilToSInt32(f);
    }
#endif

#ifdef CL_NO_ABS_BUILTIN
    inline float Abs(float f)
    {
        uint32_t s = ((uint32_t&) f) & kF32ExponentAndMantissaMask;
        return (float&) s;
    }

    // TODO: is this and similar 64-bit tricks really faster than
    // just using the FPU?
    inline double Abs(double f)
    {
        uint64_t s = ((uint64_t&) f) & kF64ExponentAndMantissaMask;
        return (double&) s;
    }
#else
    inline float  Abs(float f)  { return fabsf(f); }
    inline double Abs(double f) { return fabs (f); }
#endif

    inline bool IsNAN(float f)
    {
        return (((uint32_t&) f) & kF32ExponentMask) == kF32ExponentMask
            && (((uint32_t&) f) & kF32MantissaMask) != 0;
    }

    inline bool IsNAN(double f)
    {
        return (((uint64_t&) f) & kF64ExponentMask) == kF64ExponentMask
            && (((uint64_t&) f) & kF64MantissaMask) != 0;
    }

    inline bool IsInfinity(float f)
    {
        return (((uint32_t&) f) & ~kF32SignMask) == kF32PositiveInfinity;
    }

    inline bool IsInfinity(double f)
    {
        return (((uint64_t&) f) & ~kF64SignMask) == kF64PositiveInfinity;
    }

    inline bool IsNormal(float f)
    {
        return ((uint32_t&) f & kF32ExponentMask) != kF32ExponentNaN;
    }

    inline bool IsNormal(double f)
    {
        return ((uint64_t&) f & kF64ExponentMask) != kF64ExponentNaN;
    }


    // These are all variants of the basic trick that 0.5 * (x + |x|) is the same
    // as max(0, x). If it's an intrinsic, fabsf() is one cycle, although it does
    // have a higher latency than that.
    inline float Clamp(float x, float minX, float maxX)
    {
//    #ifdef __ARM_NEON__
    #ifdef CL_VANILLA_IMPL
        if (x < min)
            return minX;
        if (x > max)
            return maxX;
        return x;
    #else
        float dx0 = x - minX;
        x    = minX + 0.5f * (dx0 + fabsf(dx0));
        float dx1 = maxX - x;
        return maxX - 0.5f * (dx1 + fabsf(dx1));
    #endif
    }

    inline float ClampPositive(float x)
    {
    #ifdef CL_VANILLA_IMPL
        if (x < 0.0f)
            return 0.0f;
        return x;
    #else
        return 0.5f * (x + fabsf(x));
    #endif
    }

    inline float ClampLower(float x, float minX)
    {
    #ifdef CL_VANILLA_IMPL
        if (x < minX)
            return minX;
        return x;
    #else
        float dx = x - minX;
        return minX + 0.5f * (dx + fabsf(dx));
    #endif
    }

    inline float ClampUpper(float x, float maxX)
    {
    #ifdef CL_VANILLA_IMPL
        if (x > maxX)
            return maxX;
        return x;
    #else
        float dx = maxX - x;
        return maxX - 0.5f * (dx + fabsf(dx));
    #endif
    }

    inline float ClampUnit(float x)
    {
    #ifdef CL_VANILLA_IMPL
        if (x < 0.0f)
            return 0.0f;
        if (x > 1.0f)
            return 1.0f;
        return x;
    #else
        x = 0.5f * (x + fabsf(x));
        float dx1 = 1.0f - x;
        return 1.0f - 0.5f * (dx1 + fabsf(dx1));
    #endif
    }

    inline float DegreesToRadians(float degrees)
    {
        return degrees * (vl_pi / 180.0f);
    }
    inline float RadiansToDegrees(float rads)
    {
        return rads * (180.0f / vl_pi);
    }

    // Trig approximations
    struct cSinCosTable
    {
        enum { kSize = 16 };
        
        float mTable[kSize * 2];

        cSinCosTable();
    };
    extern cSinCosTable sSinCosTable;
    const float kSinCosScale    = float(cSinCosTable::kSize) / vl_twoPi;
    const float kSinCosInvScale = vl_twoPi / float(cSinCosTable::kSize);


    // Float/int conversion using standard bias trick
    typedef union { int32_t i; float f; } tIntFloat;
    const tIntFloat kBias = {((23 + 127) << 23) + (1 << 22)};

    inline float SinFast(float theta)
    {
        tIntFloat  fi;
        fi.f = theta * kSinCosScale + kBias.f; // Scale to table index range and add conversion bias.

        int i = 2 * (fi.i & (cSinCosTable::kSize - 1)); // Mask off lower bits
        
        float s = theta - (fi.i - kBias.i) * kSinCosInvScale;
        
        return sSinCosTable.mTable[i] + (sSinCosTable.mTable[i + 1] - 0.5f * sSinCosTable.mTable[i] * s) * s;
    }

    inline float CosFast(float theta)
    {
        tIntFloat  fi;
        fi.f = theta * kSinCosScale + kBias.f;

        int i = 2 * (fi.i & (cSinCosTable::kSize - 1)); // Mask off lower bits
        
        float s = theta - (fi.i - kBias.i) * kSinCosInvScale;

        return sSinCosTable.mTable[i + 1] - (sSinCosTable.mTable[i] + 0.5f * s * sSinCosTable.mTable[i + 1]) * s;
    }

    inline void SinCosFast(float theta, float* s, float* c)
    {
        tIntFloat  fi;
        fi.f = theta * kSinCosScale + kBias.f;    // Scale to table index range and add conversion bias.

        int i = 2 * (fi.i & (cSinCosTable::kSize - 1));

        float t = theta - (fi.i - kBias.i) * kSinCosInvScale;
        
        (*s) = sSinCosTable.mTable[i]     + (sSinCosTable.mTable[i + 1] - 0.5f * sSinCosTable.mTable[i]     * t) * t;
        (*c) = sSinCosTable.mTable[i + 1] - (sSinCosTable.mTable[i]     + 0.5f * sSinCosTable.mTable[i + 1] * t) * t;
    }

    inline float ATan2Fast(float y, float x)
    {
        float ay = fabsf(y) + 1e-10f;    // avoid 0/0
        float ax = fabsf(x);
            
        float r = (ay - ax) / (ax + ay);
        float angle = (0.9817f - 0.1963f * r * r) * r;
        
        angle = (x >= 0) ? (angle + (0.25f * vl_pi)) : (-angle + (0.75f * vl_pi));
        return (y >= 0) ? angle : -angle;
    }

    inline float ATanFast(float y)
    {
        float ay = fabsf(y) + 1e-10f;    // avoid 0/0

        float r = (ay - 1.0f) / (ay + 1.0f);
        float angle = (0.9817f - 0.1963f * r * r) * r + (0.25f * vl_pi);
        
        return (y >= 0) ? angle : -angle;
    }

    inline float ASinFast(float y)
    {
        float ay = fabsf(y);
        float ax = sqrtf(1 - (y * y));

        float r = (ay - ax) / (ax + ay + 1e-10f);
        float angle = (0.9817f - 0.1963f * r * r) * r + (0.25f * vl_pi);
        
        return (y >= 0) ? angle : -angle;
    }

    inline float ACosFast(float x)
    {
        float ay = sqrtf(1 - (x * x));
        float ax = fabsf(x);
            
        float r = (ay - ax) / (ax + ay + 1e-10f);
        float angle = (0.9817f - 0.1963f * r * r) * r;
        
        if (x >= 0)
            return (0.25f * vl_pi) + angle;
        else
            return (0.75f * vl_pi) - angle;
    }

    inline float InvSqrtFast(float x)
    {
        // http://en.wikipedia.org/wiki/Fast_inverse_square_root
        // TODO: use platform-specific instructions like RSQRTSS or vrsqrte.f32 where possible
        float xhalf = 0.5f * x;
        int32_t i = (int32_t&) x;

        i = 0x5f375a86 - (i >> 1);
        x = (float&) i;
        x = x * (1.5f - xhalf * x * x);

        return x;
    }

    inline uint32_t OrderedU32(float f)
    {
        CL_ASSERT(IsNormal(f));

        uint32_t uf = (uint32_t&) f;
        return (uf | 0x80000000) ^ ((int) uf >> 31);    // invert if sign bit was set
    }
}


#endif
