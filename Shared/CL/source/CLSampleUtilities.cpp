//
//  File:       CLSampleUtilities.cpp
//
//  Function:   Sampling utils
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLSampleUtilities.h>

using namespace nCL;

namespace
{
    const float kOneOverThree = float(1.0 / 3.0);
    const float kOneOverFive  = float(1.0 / 5.0);
}

// --- cHaltonSequence3 --------------------------------------------------------

int cHaltonSequence3::inc()
{
    /////////////////////////////////////
    // base 2
    
    uint32_t oldBase2 = mBase2;
    mBase2++;
    uint32_t diff = mBase2 ^ oldBase2;

    // bottom bit always changes, higher bits
    // change less frequently.
    float s = 0.5;

    // diff will be of the form 0*1+, i.e. one bits up until the last carry.
    // expected iterations = 1 + 0.5 + 0.25 + ... = 2
    do
    {
        if (oldBase2 & 1)
            mPoint[0] -= s;
        else
            mPoint[0] += s;
        
        s *= 0.5;
        
        diff = diff >> 1;
        oldBase2 = oldBase2 >> 1;
    }
    while (diff);

    
    /////////////////////////////////////
    // base 3: use 2 bits for each base 3 digit.
    
    uint32_t mask = 0x3;  // also the max base 3 digit
    uint32_t add  = 0x1;  // amount to add to force carry once digit==3
    s = kOneOverThree;

    mBase3++;

    // expected iterations: 1.5
    while (1)
    {
        if ((mBase3 & mask) == mask)
        {
            mBase3 += add;          // force carry into next 2-bit digit
            mPoint[1] -= 2 * s;
            
            mask = mask << 2;
            add  = add  << 2;
            
            s *= kOneOverThree;
        }
        else 
        {
            mPoint[1] += s;     // we know digit n has gone from a to a + 1
            break;
        }
    };

    /////////////////////////////////////
    // base 5: use 3 bits for each base 5 digit.
    mask = 0x7;
    add  = 0x3;  // amount to add to force carry once digit==dmax
    uint32_t dmax = 0x5;  // max digit
    
    s = kOneOverFive;

    mBase5++;
    
    // expected iterations: 1.25
    while (1)
    {
        if ((mBase5 & mask) == dmax)
        {
            mBase5 += add;          // force carry into next 3-bit digit
            mPoint[2] -= 4 * s;
            
            mask = mask << 3;
            dmax = dmax << 3;
            add  = add  << 3;
            
            s *= kOneOverFive;
        }
        else 
        {
            mPoint[2] += s;     // we know digit n has gone from a to a + 1
            break;
        }
    };

    return mBase2; // return the index of this sequence point
}

void cHaltonSequence3::reset()
{
    mBase2 = 0;
    mBase3 = 0;
    mBase5 = 0;
    mPoint = vl_0;
}

void cHaltonSequence3::set(int n)
{
    mPoint[0] = HaltonSequence2(n);
    mBase2 = n;

    mPoint[1] = 0;
    mBase3 = 0;

    float ip = float(1.0 / 3.0);
    float p = ip;

    for (int i = 0, k = n; k; i += 2, k /= 3)
    {
        int d = (k % 3);
        mBase3 |= d << i;
        mPoint[1] += d * p;
        p *= ip;
    }

    mPoint[2] = 0;
    mBase5 = 0;

    ip = float(1.0 / 5.0);
    p = ip;

    for (int i = 0, k = n; k; i += 3, k /= 5)
    {
        int d = (k % 5);
        mBase5 |= d << i;
        mPoint[2] += d * p;
        p *= ip;
    }
}



// --- cHaltonSequence2 --------------------------------------------------------

int cHaltonSequence2::inc()
{
    /////////////////////////////////////
    // base 2
    
    uint32_t oldBase2 = mBase2;
    mBase2++;
    uint32_t diff = mBase2 ^ oldBase2;

    // bottom bit always changes, higher bits
    // change less frequently.
    float s = 0.5;

    // diff will be of the form 0*1+, i.e. one bits up until the last carry.
    // expected iterations = 1 + 0.5 + 0.25 + ... = 2
    do
    {
        if (oldBase2 & 1)
            mPoint[0] -= s;
        else
            mPoint[0] += s;
        
        s *= 0.5;
        
        diff = diff >> 1;
        oldBase2 = oldBase2 >> 1;
    }
    while (diff);

    
    /////////////////////////////////////
    // base 3: use 2 bits for each base 3 digit.
    
    uint32_t mask = 0x3;  // also the max base 3 digit
    uint32_t add  = 0x1;  // amount to add to force carry once digit==3
    s = kOneOverThree;

    mBase3++;

    // expected iterations: 1.5
    while (1)
    {
        if ((mBase3 & mask) == mask)
        {
            mBase3 += add;          // force carry into next 2-bit digit
            mPoint[1] -= 2 * s;
            
            mask = mask << 2;
            add  = add  << 2;
            
            s *= kOneOverThree;
        }
        else 
        {
            mPoint[1] += s;     // we know digit n has gone from a to a + 1
            break;
        }
    };

    return mBase2; // return the index of this sequence point
}

void cHaltonSequence2::reset()
{
    mBase2 = 0;
    mBase3 = 0;
    mPoint = vl_0;
}

void cHaltonSequence2::set(int n)
{
    mPoint[0] = HaltonSequence2(n);
    mBase2 = n;

    mPoint[1] = 0;
    mBase3 = 0;

    float ip = float(1.0 / 3.0);
    float p = ip;

    for (int i = 0, k = n; k; i += 2, k /= 3)
    {
        int d = (k % 3);
        mBase3 |= d << i;
        mPoint[1] += d * p;
        p *= ip;
    }
}


// --- Utilities ---------------------------------------------------------------

Vec2f nCL::SquareToTriangle(Vec2f c)
{
    float t = sqrt(c[1]);
    Vec2f   result; 

    result[0] = t * c[0];
    result[1] = 1 - t;

    return result;
}

Vec2f nCL::SquareToCircle(Vec2f c)
// From Chiu & Shirley, generalization of hemisphere technique
// This is appropriate when generating sample positions is
// expensive -- otherwise can just use rejection sampling.
{
    float     phi;
    float     r;
    Vec2f  a = 2.0f * c - Vec2f(vl_one); // a is now on [-1,1] ^ 2 

    if (a[0] > -a[1])       // region 1 or 2
    {
        if (a[0] > a[1])    // region 1, also |a| > |b| 
        {
            r = a[0];
            phi = (vl_pi / 4.0f) * (a[1] / a[0]);
        }
        else                // region 2, also |b| > |a| 
        {
            r = a[1];
            phi = (vl_pi / 4.0f) * (2.0f - (a[0] / a[1]));
        }
    }
    else                    // region 3 or 4
    {
        if (a[0] < a[1])    // region 3, also |a| >= |b|, a != 0 
        { 
            r = -a[0];
            phi = (vl_pi / 4.0f) * (4.0f + (a[1] / a[0]));
        }
        else                // region 4, |b| >= |a|, but a==0 and b==0 could occur.
        {
            r = -a[1];
            if (a[1] != 0)
                phi = (vl_pi / 4.0f) * (6.0f - (a[0] / a[1]));
            else
                phi = 0;
        }
    }

    float y, x;
//    sincos(phi, &y, &x);
    x = cos(phi);
    y = sin(phi);
    return r * Vec2f(x, y);
}


/*
    worst case: 1/sqrt(3)
    
*/

inline int nCL::CubeFace(Vec3f p)
/// vanilla, comparison-heavy version
{
    Vec3f ap(fabs(p[0]), fabs(p[1]), fabs(p[2]));
    
    if (ap[0] > ap[1])
        if (ap[0] > ap[2])
            if (p[0] > 0)
                return 0;
            else
                return 3;
        else
            if (p[2] > 0)
                return 2;
            else
                return 5;
    else
        if (ap[1] > ap[2])
            if (p[1] > 0)
                return 1;
            else
                return 4;
        else
            if (p[2] > 0)
                return 2;
            else
                return 5;
}


Vec3f nCL::CubeToSphere(Vec3f p)
{
    p = 2.0f * p - Vec3f(vl_one);

    int face = CubeFace(p);

    switch (face)
    {
    case 0:
    case 3:
        {
            float invMax = 0.25f * vl_pi / p[0];
            float s1 = sinf(p[1] * invMax);
            float s2 = sinf(p[2] * invMax);
            
            return p[0] * Vec3f(sqrt(1 - sqr(s1) - sqr(s2)), s1, s2);
        }
        
    case 1:
    case 4:
        {
            float invMax = 0.25f * vl_pi / p[1];
            float s1 = sinf(p[0] * invMax);
            float s2 = sinf(p[2] * invMax);
            
            return p[1] * Vec3f(s1, sqrt(1 - sqr(s1) - sqr(s2)), s2);
        }
        
    case 2:
    case 5:
        {
            float invMax = 0.25f * vl_pi / p[2];
            float s1 = sinf(p[0] * invMax);
            float s2 = sinf(p[1] * invMax);
            
            return p[2] * Vec3f(s1, s2, sqrt(1 - sqr(s1) - sqr(s2)));
        }        
    }

    return vl_0; // shut up compiler
}


Vec3f nCL::SquareToSphereSurface(Vec2f c)
// convert c to a unit length screw
{
    c = 2.0f * c - Vec2f(vl_one);

    float phi =  2 * c[1] * vl_pi;
    float w = sqrt(1 - sqr(c[0]));

    float y, x;
    sincosf(phi, &y, &x);
    return Vec3f(w * x, w * y, c[0]);
}


// Sample generation routines 

float nCL::HaltonSequence(int n, int b)
/// return term i of the base b Halton sequence
/// In fact, this is just a generalization of heckbert's bit reversal distribution trick.
/// E.g., when b=3, write n as a base 3 number, digit 0 -> which third of interval the
/// sample is in, 1 -> which third of that, 2 -> which third of that, etc.
/// So, no randomness at all, actually.
{
    float result = 0;
    float ip = 1.0f / b;     
    float p = ip;

    while (n > 0)
    {
        result += (n % b) * p;
        n = n / b;
        p *= ip;
    }

    return result;
}

float nCL::HaltonSequence2(int n)
// return term i of the base 2 Halton sequence
// this really is just the heckbert bit reversal trick
// we could do better than this in some cases by doing a bit reversal
// and stuffing the result directly in the mantissa
{    
    float result = 0;

    float p = 0.5;
    while (n > 0)
    {
        if (n & 1)
            result += p;
        
        n = n >> 1;
        p *= 0.5;
    }
    
    return result;
}
