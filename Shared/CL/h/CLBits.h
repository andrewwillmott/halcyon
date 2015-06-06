/*
    File:       CLBits.h

    Function:   

    Author:     Andrew Willmott

    Copyright:  (c) 2001, Andrew Willmott
*/

#ifndef CL_BITS_H
#define CL_BITS_H

#include <CLDefs.h>

namespace nCL
{
    bool IsPowerOfTwo(int a);

    int RightmostBit(int a);

    uint32_t                 CeilToPow2(uint32_t a);             ///< Rounds up to a power of two
    uint32_t                 CeilToPow2(uint32_t n, uint32_t a); ///< Rounds up to a multiple of 2^n
    template<int n> uint32_t CeilToPow2(uint32_t a);             ///< Rounds up to a multiple of 2^n

    uint32_t BitCount32  (uint32_t a);
    uint32_t BitReverse32(uint32_t a);
    uint32_t BitReverse16(uint32_t a);
    uint32_t BitReverse8 (uint32_t a);
    uint32_t GreyCode    (uint32_t a);      ///< Returns grey code for a -- one bit will be flipped between successive a values.
    uint32_t LeadingZeroes32 (uint32_t a);
    uint32_t TrailingZeroes32(uint32_t a);
    uint32_t LeadingOnes32   (uint32_t a);
    uint32_t TrailingOnes32  (uint32_t a);

    // U8 helpers
    uint32_t DivideBy255    (uint32_t a);
    uint32_t MultiplyUint8  (uint32_t a, uint32_t b);
    uint32_t DividePairBy255(uint32_t a);
    uint32_t BlendARGB32    (uint32_t src0ARGB, uint32_t src1ARGB); ///< Perform standard blend using src0 alpha

    int      Log2RealToInt(float a);    ///< Use f2i tricks to extract integer power of two
    int      Log2Int    (uint32_t a);   ///< Return integer log(a)
    int      CeilLog2Int(uint32_t a);   ///< Return integer ceil(log(a))
    Real     BitLog     (uint32_t a);   ///< Return an approximation to logf(a)

    uint32_t ByteSwap32(uint32_t a);     ///< Swap 32-bit 'a' from little to big endian, or vice-versa
    uint16_t ByteSwap16(uint16_t a);     ///< Swap 16-bit 'a' from little to big endian, or vice-versa

    uint32_t IncSwizzle(uint32_t x, uint32_t maskX = 0x55555555);


    // --- Inlines -------------------------------------------------------------

    inline bool IsPowerOfTwo(int a)
    { return (a & -a) == a; };

    inline int RightmostBit(int a)
    { return a & -a; };

    inline uint32_t CeilPow2(uint32_t a)
    {
        --a;

        a |= a >> 1;
        a |= a >> 2;
        a |= a >> 4;
        a |= a >> 8;
        a |= a >> 16;

        return a + 1;
    }

    inline uint32_t CeilToPow2(uint32_t n, uint32_t a)
    {
        return ((n - 1) + a) & ~(n - 1);
    }

    template<int n> inline uint32_t CeilToPow2(uint32_t a)
    {
        return ((n - 1) + a) & ~(n - 1);
    }

#if !defined(CL_VANILLA_BITS) && defined(CL_GCC)
    #define BitCount32 __builtin_popcount
    #define LeadingZeroes32 __builtin_clz
    #define TrailingZeroes32 __builtin_ctz
    #define ByteSwap32 __builtin_bswap32
    #define ByteSwap16 __builtin_bswap16

    inline uint32_t LeadingOnes32(uint32_t a)
    {
        return __builtin_clz(~a);
    }

    inline uint32_t TrailingOnes32(uint32_t a)
    {
        return __builtin_ctz(~a);
    }

#else
    inline uint32_t BitCount32(uint32_t a)
    { 
        uint32_t s0 =        a & 0x55555555;
        uint32_t s1 = (a >> 1) & 0x55555555;
        a = s0 + s1;
        
        s0 =        a & 0x33333333;
        s1 = (a >> 2) & 0x33333333;
        a  = s0 + s1;
        
        s0 =        a & 0x0F0F0F0F;
        s1 = (a >> 4) & 0x0F0F0F0F;
        a  = s0 + s1;
        
        s0 =        a & 0x00FF00FF;
        s1 = (a >> 8) & 0x00FF00FF;
        a  = s0 + s1;

        s0 =        a  & 0x0000FFFF;
        s1 = (a >> 16); // redundant: & 0x0000FFFF;
        a  = s0 + s1;

        return a;
    };

    inline uint32_t ByteSwap32(uint32_t a)
    {
        return ((a << 24) & 0xFF000000)
             | ((a <<  8) & 0x00FF0000)
             | ((a >>  8) & 0x0000FF00)
             | ((a >> 24)             );
    }

    inline uint16_t ByteSwap16(uint16_t a)
    {
        return (a << 8) | (a >> 8);
    }
#endif


#if !defined(CL_VANILLA_BITS) && TARGET_CPU_ARM
    inline uint32_t BitReverse32(uint32_t v)
    {
        uint32_t input = v;
        uint32_t output;
        __asm__("rbit %0, %1\n" : "=r"(output) : "r"(input));
        return output;
    }

    inline uint32_t BitReverse16(uint32_t v)
    {
        uint32_t input = v;
        uint32_t output;
        __asm__("rbit %0, %1\n" : "=r"(output) : "r"(input));
        return output >> 16;
    }

    inline uint32_t BitReverse8(uint32_t v)
    {
        uint32_t input = v;
        uint32_t output;
        __asm__("rbit %0, %1\n" : "=r"(output) : "r"(input));
        return output >> 24;
    }
#else
    inline uint32_t BitReverse32(uint32_t a)
    {
        uint32_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);
        a = ((b & 0x00FF00FF) << 8)  | ((b & 0xFF00FF00) >> 8);
        b = ((a & 0x0000FFFF) << 16) | ((a & 0xFFFF0000) >> 16);

        return b;
    }

    inline uint32_t BitReverse16(uint32_t a)
    {
        uint32_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);
        a = ((b & 0x00FF00FF) << 8)  | ((b & 0xFF00FF00) >> 8);

        return a;
    }

    inline uint32_t BitReverse8(uint32_t a)
    {
        uint8_t b;

        b = ((a & 0x55555555) << 1)  | ((a & 0xAAAAAAAA) >> 1);
        a = ((b & 0x33333333) << 2)  | ((b & 0xCCCCCCCC) >> 2);
        b = ((a & 0x0F0F0F0F) << 4)  | ((a & 0xF0F0F0F0) >> 4);

        return b;
    }
#endif

    inline uint32_t GreyCode(uint32_t a)
    {
        return a ^ (a >> 1);
    }

    inline uint32_t DivideBy255(uint32_t a)
    {
        a++;
        return (a + (a >> 8)) >> 8;
    }

    inline uint32_t MultiplyUint8(uint32_t a, uint32_t b)
    {
        uint32_t p = a * b + 128;
        return (p + (p >> 8)) >> 8;
    }

    inline uint32_t DividePairBy255(uint32_t a)
    // Divides two 16-bit values packed into a uint32_t
    // in parallel
    {
        a += 0x00010001;
        return (a + ((a >> 8) & 0x00FF00FF)) >> 8;
    }

    inline uint32_t BlendARGB32(uint32_t src0ARGB, uint32_t src1ARGB)
    // does a standard src0 * src0Alpha + src1 * invSrc0Alpha blend
    {
        uint32_t sag0 = (src0ARGB & 0xFF00FF00) >> 8;
        uint32_t srb0 = (src0ARGB & 0x00FF00FF);
        uint32_t sag1 = (src1ARGB & 0xFF00FF00) >> 8;
        uint32_t srb1 = (src1ARGB & 0x00FF00FF);
        
        uint32_t alpha = (src0ARGB >> 24) & 0xFF;
        uint32_t invAlpha = 255 - alpha;

        uint32_t dag = sag0 * alpha + sag1 * invAlpha;
        uint32_t drb = srb0 * alpha + srb1 * invAlpha;
        
        // now divide both by 255.
        dag += 0x00010001;
        dag = (dag + ((dag >> 8) & 0x00FF00FF)) >> 8;
        drb += 0x00010001;
        drb = (drb + ((drb >> 8) & 0x00FF00FF)) >> 8;
        
        return ((dag & 0x00FF00FF) << 8) | (drb & 0x00FF00FF);
        // Op count:
        // >> 7
        // &  7
        // +  7
        // *  4
        // |  1
    }


    // From Mike Herf
    inline int Log2RealToInt(float a)
    {
        float epsilon = (const float&) uint32_t(0x3fe00000);
        a += epsilon;
        int b = ((const uint32_t&) a & 0x7FE00000) >> 23;
        return b - 127;
    }

#if !defined(CL_VANILLA_BITS) && defined(CL_GCC)
    inline int Log2Int(uint32_t a)
    {
        return 31 - LeadingZeroes32(a);
    }
#else
    inline int Log2Int(uint32_t a)
    // Drawbacks: up to 5 conditionals
    // But not as bad as while (a) a >>= 1;
    {
        int l2 = 0;

        if (a & 0xFFFF0000)
        {
            a >>= 16;
            l2 += 16;
        }
        if (a & 0x0000FF00)
        {
            a >>= 8;
            l2 += 8;
        }
        if (a & 0x000000F0)
        {
            a >>= 4;
            l2 += 4;
        }
        if (a & 0x0000000C)
        {
            a >>= 2;
            l2 += 2;
        }
        if (a & 0x00000002)
        {
            a >>= 1;
            l2 += 1;
        }

        return l2;
    }
#endif

    inline int CeilLog2Int(uint32_t a)
    {
        int result = Log2Int(a);

        if ((a & -a) != a)
            result++;

        return result;
    }

    inline Real BitLog(uint32_t a)
    {
        int b = Log2Int(a);
        int bl = 8 * (b - 1) + (a >> (b - 4));
        
        return 0.125 * bl + 1;
    }

    inline uint32_t IncSwizzle(uint32_t x, uint32_t maskX)
    {
        // increment x with holes, using 1s in the holes to move the carry across.
        // x = (x + (~maskX + 1)) & maskX;
        return (x - maskX) & maskX;
    }
}

#endif
