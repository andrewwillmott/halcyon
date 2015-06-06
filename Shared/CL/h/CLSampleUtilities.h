//
//  File:       CLSampleUtilities.h
//
//  Function:   Utilities for sampling and mapping between different domains
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_SAMPLE_UTILITIES_H
#define CL_SAMPLE_UTILITIES_H

#include <VLf.h>
#include <CLBits.h>

namespace nCL
{
    // Domain mapping routines

    Vec2f SquareToTriangle(Vec2f c);
    ///< Map [0, 1]^2 square to [0, 1]^2 triangle
    Vec2f SquareToCircle(Vec2f c);
    ///< Map [0, 1]^2 square to [-1, 1]^2 circle
    int  CubeFace(Vec3f v);
    ///< Returns cube face index for given direction
    Vec3f CubeToSphere(Vec3f p);
    ///< Map [0, 1]^3 cube to [-1, 1]^3 sphere
    Vec3f SquareToSphereSurface(Vec2f c);
    ///< Map [0, 1]^2 square to [-1, 1]^3 sphere surface

    float HaltonSequence(int n, int b);
    /// return term i of the base b Halton sequence

    float HaltonSequence2(int i);
    // return term i of the base 2 Halton sequence
    float FastHalton2(int i);
    // return term i of the base 2 Halton sequence

    struct cHaltonSequence2
    /// This calculates the Halton sequence incrementally
    /// for a base 2/3 triplet.
    {
        Vec2f mPoint;  ///< Current sample point.

        uint32_t mBase2;
        uint32_t mBase3;
        
        cHaltonSequence2() : 
            mPoint(vl_0),
            mBase2(0),
            mBase3(0)
        {}
        
        int inc();
        ///< Advance to next point in the sequence. Returns the index of this point. 
        void reset();
        ///< Move back to first point in the sequence (i.e. the origin.)
        void set(int n);
        ///< Jump directly to term 'n' of the sequence
        int operator++() { return inc(); }
        ///< Synonym for inc().
    };
    
    struct cHaltonSequence3
    /// This calculates the Halton sequence incrementally
    /// for a base 2/3/5 triplet.
    {
        Vec3f mPoint;  ///< Current sample point.
        
        uint32_t mBase2;
        uint32_t mBase3;
        uint32_t mBase5;
        
        cHaltonSequence3() : 
            mPoint(vl_0),
            mBase2(0),
            mBase3(0),
            mBase5(0)
        {}
        
        int inc();
        ///< Advance to next point in the sequence. Returns the index of this point. 
        void reset();
        ///< Move back to first point in the sequence (i.e. the origin.)
        void set(int n);
        ///< Jump directly to term 'n' of the sequence
        int operator++() { return inc(); }
        ///< Synonym for inc().
    };


    // --- Inlines -------------------------------------------------------------

    inline float FastHalton2(int i)
    {
        uint16_t reverse = BitReverse16(i);
        
        int32_t resultU(0x3f800000 | (reverse << 7));
        float result = ((float&) resultU);
        return result - 1.0f;
    }
}

#endif
