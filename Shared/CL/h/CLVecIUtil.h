//
//  File:       CLVecIUtil.h
//
//  Function:   Various Vec234i utils
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#ifndef CL_VECI_UTIL_H
#define CL_VECI_UTIL_H

//#include <CLBounds.h>
#include <CLRandom.h>


namespace nCL
{
    // --- Min & Max routines -----------------------------------------------------

    int    Max(int a, int b);        ///< return max of a and b
    int    Min(int a, int b);        ///< return min of a and b

    int    MaxElt(const Vec2i& v);        ///< return max elt of v
    int    MinElt(const Vec2i& v);        ///< return min elt of v

    int    MaxEltIndex(const Vec2i& v);   ///< return index of max elt of v
    int    MinEltIndex(const Vec2i& v);   ///< return index of min elt of v

    Vec2i  MaxElts(const Vec2i& a, const Vec2i& b);  ///< return max elts of a and b
    Vec2i  MinElts(const Vec2i& a, const Vec2i& b);  ///< return min elts of a and b

    int    MaxElt(const Vec3i& v);        ///< return max elt of v
    int    MinElt(const Vec3i& v);        ///< return min elt of v

    int    MaxEltIndex(const Vec3i& v);   ///< return index of max elt of v
    int    MinEltIndex(const Vec3i& v);   ///< return index of min elt of v

    Vec3i  MaxElts(const Vec3i& a, const Vec3i& b);  ///< return max elts of a and b
    Vec3i  MinElts(const Vec3i& a, const Vec3i& b);  ///< return min elts of a and b

    int    MaxElt(const Vec4i& v);        ///< return max elt of v
    int    MinElt(const Vec4i& v);        ///< return min elt of v

    int    MaxEltIndex(const Vec4i& v);   ///< return index of max elt of v
    int    MinEltIndex(const Vec4i& v);   ///< return index of min elt of v

    Vec4i  MaxElts(const Vec4i& a, const Vec4i& b);  ///< return max elts of a and b
    Vec4i  MinElts(const Vec4i& a, const Vec4i& b);  ///< return min elts of a and b


    // --- Clamp ---------------------------------------------------------------

//    Vec2i Clamp(Vec2i v, int n);
//    Vec3i Clamp(Vec3i v, int n);
//    Vec4f Clamp(Vec4f v, int n);


    // --- Bounding box utility routines ---------------------------------------

    int  BoxArea(const Vec3i& min, const Vec3i& max); ///< return surface area of box defined by min/max
    int  BoxVol (const Vec3i& min, const Vec3i& max);  ///< return volume of box defined by min/max
    void UpdateBounds(const Vec3i& pt, Vec3i* min, Vec3i* max); ///< update min/max according to pt


    // --- Miscellaneous -------------------------------------------------------

    int RandomRange(Vec2i range, tSeed32* seed);

//    Vec2i RandomSquare2i(tSeed32* seed, int n);    ///< Returns random point in n x n
//    Vec2i RandomCircle2i(tSeed32* seed, int n);    ///< Returns random point in radius-n circle
//    Vec2i RandomRange   (const cBounds2& bounds, tSeed32* seed);

    int RandomRange(Vec3i range, tSeed32* seed);
//    Vec3i RandomCube3i  (tSeed32* seed, int n);    ///< Returns random vector in the unit square
//    Vec2i RandomSphere3i(tSeed32* seed, int n);    ///< Returns random point in radius-n circle
//    Vec3i RandomRange   (const cBounds3& bounds, tSeed32* seed);


    // --- Inlines ----------------------------------------------------------------

    inline int Max(int a, int b)
    {
        return b < a ? a : b;
    }

    inline int Min(int a, int b)
    {
        return a < b ? a : b;
    }

    inline int MaxElt(const Vec2i& v)
    {
        return Max(v[0], v[1]);
    }

    inline int MinElt(const Vec2i& v)
    {
        return Min(v[0], v[1]);
    }

    inline int MinEltIndex(const Vec2i& v)
    {
        if (v[0] < v[1])
            return 0;
        else
            return 1;
    }

    inline int MaxEltIndex(const Vec2i& v)
    {
        if (v[0] > v[1])
            return 0;
        else
            return 1;
    }

    inline int MaxElt(const Vec3i& v)
    {
        return Max(Max(v[0], v[1]), v[2]);
    }

    inline int MinElt(const Vec3i& v)
    {
        return Min(Min(v[0], v[1]), v[2]);
    }

    inline int MinEltIndex(const Vec3i& v)
    {
        if (v[0] < v[1])
            if (v[0] < v[2])
                return 0;
            else
                return 2;
        else
            if (v[1] < v[2])
                return 1;
            else
                return 2;
    }

    inline int MaxEltIndex(const Vec3i& v)
    {
        if (v[0] > v[1])
            if (v[0] > v[2])
                return 0;
            else
                return 2;
        else
            if (v[1] > v[2])
                return 1;
            else
                return 2;
    }

    inline int MaxElt(const Vec4i& v)
    {
        return Max(Max(Max(v[0], v[1]), v[2]), v[3]);
    }

    inline int MinElt(const Vec4i& v)
    {
        return Min(Min(Min(v[0], v[1]), v[2]), v[3]);
    }

    inline int MinEltIndex(const Vec4i& v)
    {
        CL_ERROR("broken");
        if (v[0] < v[1])
            if (v[0] < v[2])
                return 0;
            else
                return 2;
        else
            if (v[1] < v[2])
                return 1;
            else
                return 2;
    }

    inline int MaxEltIndex(const Vec4i& v)
    {
        CL_ERROR("broken");
        if (v[0] > v[1])
            if (v[0] > v[2])
                return 0;
            else
                return 2;
        else
            if (v[1] > v[2])
                return 1;
            else
                return 2;
    }

    inline Vec2i MaxElts(const Vec2i& a, const Vec2i& b)
    {
        return Vec2i
        (
            Max(a[0], b[0]),
            Max(a[1], b[1])
        );
    }

    inline Vec2i MinElts(const Vec2i& a, const Vec2i& b)
    {
        return Vec2i
        (
            Min(a[0], b[0]),
            Min(a[1], b[1])
        );
    }

    inline Vec3i MaxElts(const Vec3i& a, const Vec3i& b)
    {
        return Vec3i
        (
            Max(a[0], b[0]),
            Max(a[1], b[1]),
            Max(a[2], b[2])
        );
    }

    inline Vec3i MinElts(const Vec3i& a, const Vec3i& b)
    {
        return Vec3i
        (
            Min(a[0], b[0]),
            Min(a[1], b[1]),
            Min(a[2], b[2])
        );
    }

    inline Vec4i MaxElts(const Vec4i& a, const Vec4i& b)
    {
        return Vec4i
        (
            Max(a[0], b[0]),
            Max(a[1], b[1]),
            Max(a[2], b[2]),
            Max(a[3], b[3])
        );
    }

    inline Vec4i MinElts(const Vec4i& a, const Vec4i& b)
    {
        return Vec4i
        (
            Min(a[0], b[0]),
            Min(a[1], b[1]),
            Min(a[2], b[2]),
            Min(a[3], b[3])
        );
    }

    inline int BoxVol(const Vec3i& b0, const Vec3i& b1)
    {
        Vec3i t = b1 - b0;

        return t[0] * t[1] * t[2];
    }

    inline int BoxArea(const Vec3i& b0, const Vec3i& b1)
    {
        Vec3i t = b1 - b0;

        return 2 * (t[0] * t[1] + t[1] * t[2] + t[2] * t[0]);
    }

    inline int RandomRange(Vec2i range, tSeed32* seed)
    {
        return range[0] + UIntFromSeed(NextSeed(seed), range[1] - range[0]);
    }

#ifdef TODO
    inline Vec2i RandomRange(const cBounds2i& bounds, tSeed32* seed)
    {
        Vec2i result;

        result[0] = bounds.mMin[0] + UIntFromSeed(NextSeed(seed), bounds.mMax[0] - bounds.mMin[0]);
        result[1] = bounds.mMin[1] + UIntFromSeed(NextSeed(seed), bounds.mMax[1] - bounds.mMin[1]);

        return result;
    }

    inline Vec3i RandomRange(const cBounds3i& bounds, tSeed32* seed)
    {
        Vec3i result;

        result[0] = bounds.mMin[0] + UIntFromSeed(NextSeed(seed), bounds.mMax[0] - bounds.mMin[0]);
        result[1] = bounds.mMin[1] + UIntFromSeed(NextSeed(seed), bounds.mMax[1] - bounds.mMin[1]);
        result[2] = bounds.mMin[2] + UIntFromSeed(NextSeed(seed), bounds.mMax[2] - bounds.mMin[2]);

        return result;
    }
#endif
}

#endif
