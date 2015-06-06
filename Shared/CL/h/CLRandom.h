//
//  File:       CLRandom.h
//
//  Function:   Pseudo-random sequence support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_RANDOM_H
#define CL_RANDOM_H

#include <CLDefs.h>

namespace nCL
{
    // --- Basic LCG RNGs ------------------------------------------------------

    typedef uint32_t tSeed32;
    const tSeed32 kDefaultSeed32 = 12345678;

    tSeed32  NextSeed(tSeed32 seed);         ///< Find next 32-bit number in a pseudo-random sequence using standard LCG.
    tSeed32  NextSeedFast(tSeed32 seed);     ///< Find next 32-bit number in a pseudo-random sequence, single 32-bit multiply, not quite as good quality.

    tSeed32  NextSeed(tSeed32* seed);        ///< Update seed in place, return previous value.
    tSeed32  NextSeedFast(tSeed32* seed);    ///< Update seed in place, return previous value

    uint32_t UIntFromSeed(tSeed32 seed);     ///< Returns a number from 0 .. UINT32_MAX
    uint32_t UIntFromSeed(tSeed32 seed, uint32_t limit); ///< Returns a number from 0 .. limit - 1
    
    int32_t  SIntFromSeed(tSeed32 seed);                ///< Returns INT32_MIN .. INT32_MAX
    int32_t  SIntFromSeed(tSeed32 seed, int32_t limit); ///< Returns (-limit, limit)

    float UFloatFromSeed(tSeed32 seed);        /// Returns a float in [0, 1)
    float SFloatFromSeed(tSeed32 seed);        /// Returns a float in (-1, 1)


    // --- Composites ----------------------------------------------------------

    uint32_t RandomUInt32(            tSeed32* seed);
    uint32_t RandomUInt32(uint32_t a, tSeed32* seed);
    int32_t  RandomSInt32(            tSeed32* seed);
    int32_t  RandomSInt32(int32_t a,  tSeed32* seed);

    float RandomUFloat(         tSeed32* seed);
    float RandomUFloat(float a, tSeed32* seed);
    float RandomSFloat(         tSeed32* seed);
    float RandomSFloat(float a, tSeed32* seed);

    float RandomRange     (float a, float b, tSeed32* seed);
    float RandomRangeAbout(float a, float b, tSeed32* seed);


    // --- cRandomMT -----------------------------------------------------------

    class cRandomMT
    /// Mersenne Twister -- much higher quality RNG.
    {
    public:
        cRandomMT();
        
        void SetSeed(uint32_t seed);

        uint32_t Random();

    protected:
        uint32_t Reload();

        // Data
        uint32_t    mIndex;
        uint32_t    mState[625]; // state vector + 1 extra to not violate ANSI C
        uint32_t*   mNext;       // next random value is computed from here
        int         mLeft;       // can *next++ this many times before reloading
    };


    // --- Inlines -------------------------------------------------------------

    const float kSeedFloatScale    = 2.32830643653870e-10f;
    const float kSeedFloatScaleTwo = 2.0f * kSeedFloatScale;

    inline tSeed32 NextSeed(tSeed32 seed)
    {
        return uint32_t(seed * uint64_t(1103515245) + 12345);
    }

    inline tSeed32 NextSeedFast(tSeed32 seed)
    {
        return seed * 0x278dde6d;
    }

    inline tSeed32 NextSeed(tSeed32* seed)
    {
        tSeed32 current(*seed);
        *seed = uint32_t(*seed * uint64_t(1103515245) + 12345);
        return current;
    }

    inline tSeed32 NextSeedFast(tSeed32* seed)
    {
        tSeed32 current(*seed);
        *seed *= 0x278dde6d;
        return current;
    }

    inline uint32_t UIntFromSeed(tSeed32 seed)
    {
        return seed;
    }

    inline uint32_t UIntFromSeed(tSeed32 seed, uint32_t limit)
    {
        return (seed * (uint64_t(limit))) >> 32;
    }

    inline int32_t SIntFromSeed(tSeed32 seed)
    {
        return seed;
    }

    inline int32_t SIntFromSeed(tSeed32 seed, int32_t limit)
    {
        return ((seed * uint64_t(2 * limit + 1)) >> 32) - limit;
    }

    inline float UFloatFromSeed(tSeed32 seed)
    {
        return kSeedFloatScale * int32_t(seed) + 0.5f;
    }

    inline float SFloatFromSeed(tSeed32 seed)
    {
        return kSeedFloatScaleTwo * int32_t(seed);
    }

    // Composites
    inline uint32_t RandomUInt32(tSeed32* seed)
    {
        return UIntFromSeed(NextSeed(seed));
    }

    inline uint32_t RandomUInt32(uint32_t a, tSeed32* seed)
    {
        return UIntFromSeed(NextSeed(seed), a);
    }

    inline int32_t RandomSInt32(tSeed32* seed)
    {
        return SIntFromSeed(NextSeed(seed));
    }
    
    inline int32_t RandomSInt32(int32_t a, tSeed32* seed)
    {
        return SIntFromSeed(NextSeed(seed), a);
    }

    inline float RandomUFloat(tSeed32* seed)
    {
        return UFloatFromSeed(NextSeed(seed));
    }
    inline float RandomUFloat(float a, tSeed32* seed)
    {
        return a * UFloatFromSeed(NextSeed(seed));
    }
    inline float RandomSFloat(tSeed32* seed)
    {
        return SFloatFromSeed(NextSeed(seed));
    }
    inline float RandomSFloat(float a, tSeed32* seed)
    {
        return a * SFloatFromSeed(NextSeed(seed));
    }

    inline float RandomRange (float a, float b, tSeed32* seed)
    {
        return a + (b - a) * UFloatFromSeed(NextSeed(seed));
    }
    inline float RandomRangeAbout(float a, float b, tSeed32* seed)
    {
        return a + b * SFloatFromSeed(NextSeed(seed));
    }


    // cRandomMT
    inline uint32_t cRandomMT::Random()
    {
        uint32_t y;

        if (--mLeft < 0)
            return Reload();

        y  = *mNext++;
        y ^= (y >> 11);
        y ^= (y <<  7) & 0x9D2C5680U;
        y ^= (y << 15) & 0xEFC60000U;

        return y ^ (y >> 18);
    }
}





#endif
