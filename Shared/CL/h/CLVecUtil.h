/*
    File:           CLVecUtil.h
    
    Function:       Contains a grab-bag of useful graphics vector routines.

    Author(s):      Andrew Willmott

    Copyright:      1996-2000, Andrew Willmott
 */
 
#ifndef CL_VECUTIL_H
#define CL_VECUTIL_H

#include <CLBounds.h>
#include <CLColour.h>
#include <CLRandom.h>


namespace nCL
{
    // --- Min & Max routines -----------------------------------------------------

    float  Max(float a, float b);        ///< return max of a and b
    float  Min(float a, float b);        ///< return min of a and b

    float  MaxElt(const Vec2f& v);        ///< return max elt of v
    float  MinElt(const Vec2f& v);        ///< return min elt of v

    int    MaxEltIndex(const Vec2f& v);   ///< return index of max elt of v
    int    MinEltIndex(const Vec2f& v);   ///< return index of min elt of v

    Vec2f  MaxElts(const Vec2f& a, const Vec2f& b);  ///< return max elts of a and b
    Vec2f  MinElts(const Vec2f& a, const Vec2f& b);  ///< return min elts of a and b

    float  MaxElt(const Vec3f& v);        ///< return max elt of v
    float  MinElt(const Vec3f& v);        ///< return min elt of v

    int    MaxEltIndex(const Vec3f& v);   ///< return index of max elt of v
    int    MinEltIndex(const Vec3f& v);   ///< return index of min elt of v

    Vec3f  MaxElts(const Vec3f& a, const Vec3f& b);  ///< return max elts of a and b
    Vec3f  MinElts(const Vec3f& a, const Vec3f& b);  ///< return min elts of a and b

    float  MaxCmpt(const cColour& c); ///< equivalents of Elts routines for Colours.
    float  MinCmpt(const cColour& c);

    int    MaxCmptIndex(const cColour& c);
    int    MinCmptIndex(const cColour& c);

    cColour MaxCmpts(const cColour& a, const cColour& b);
    cColour MinCmpts(const cColour& a, const cColour& b);


    // --- Clamp ---------------------------------------------------------------

    Vec2f ClampUnit(Vec2f v);
    Vec3f ClampUnit(Vec3f v);
    Vec4f ClampUnit(Vec4f v);


    // --- Bounding box utility routines ---------------------------------------

    float  BoxVol(const Vec3f& min, const Vec3f& max);  ///< return volume of box defined by min/max
    float  BoxArea(const Vec3f& min, const Vec3f& max); ///< return surface area of box defined by min/max
    void   UpdateBounds(const Vec3f& pt, Vec3f& min, Vec3f& max); ///< update min/max according to pt


    // --- Miscellaneous -------------------------------------------------------

    bool HasNAN(Vec2f v);
    bool HasNAN(Vec3f v);
    bool HasNAN(Vec4f v);

    Vec3f TriAreaNormal
    (
        const Vec3f& a,
        const Vec3f& b, 
        const Vec3f& c
    );
    ///< Find triangle normal.

    bool PlaneIntersection
    (
        const Vec3f&    start,
        const Vec3f&    direction,
        float           d,
        const Vec3f&    normal,
        float&          t
    );
    ///< Intersect plane with ray.

    float BoxIntersection(const Vec3f& p, const Vec3f& r, const Vec3f& min, const Vec3f& max);
    ///< Project p in direction r on to bounding box min/max. Returns t such that this projected point = p + t * r.

    bool PointIsInsideTriangle
    (
        const Vec3f&    v0,      
        const Vec3f&    v1,      
        const Vec3f&    v2,      
        const Vec3f&    point,
        Vec3f*          coords = 0
    );
    ///< Test if the point is inside the given triangle, optionally return barycentric coords

    Vec3f FindOrthoVector(const Vec3f& v);
    ///< Find a vector that is orthogonal to v.

    float RandomRange(Vec2f range, tSeed32* seed);

    Vec2f RandomDir2f   (tSeed32* seed);    ///< Returns random direction vector
    Vec2f RandomSquare2f(tSeed32* seed);    ///< Returns random vector in the unit square
    Vec2f RandomCircle2f(tSeed32* seed);    ///< Returns random vector in the unit circle
    Vec2f RandomRange   (const cBounds2& bounds, tSeed32* seed);

    Vec3f RandomDir3f   (tSeed32* seed);    // returns random direction vector
    Vec3f RandomCube3f  (tSeed32* seed);    ///< Returns random vector in the unit square
    Vec3f RandomSphere3f(tSeed32* seed);    ///< Returns random vector in the unit circle
    Vec3f RandomRange   (const cBounds3& bounds, tSeed32* seed);

    Vec3f RandomSphere   (float r, tSeed32* seed);      ///< Returns random point in origin-centred sphere of radius r
    Vec3f RandomEllipsoid(const cBounds3& bounds, tSeed32* seed);   ///< Generalised version of RandomSphere, handles any axis-aligned ellipsoid with the given bounds
    Vec3f RandomTorus    (const cBounds3& bounds, float r, tSeed32* seed);  ///< Returns random point from a torus with the given radius and bounds

    bool Refract
    (
        float          fromIndex,
        float          toIndex,
        const Vec3f&   v,
        const Vec3f&   n, 
        float          cosNV,
        Vec3f&         refractDir
    );
    ///< Calculates the refraction vector for the given parameters.  

    Vec2f UnitSquareToUnitDisc(Vec2f v);
    Vec2f UnitDiskToUnitSquare(Vec2f v);
    Vec2f UnitSquareToUnitDisc(Vec2f v);
    Vec2f UnitDiskToUnitSquare(Vec2f v);
    // What the names say: code from Shirley & Chiu, minimises adjacency,
    // fractional area and aspect ratio distortions.


    // --- Inlines ----------------------------------------------------------------

    inline float Max(float a, float b)
    {
        return b < a ? a : b;
    }

    inline float Min(float a, float b)
    {
        return a < b ? a : b;
    }

    inline float MaxElt(const Vec2f& v)
    {
        return Max(v[0], v[1]);
    }

    inline float MinElt(const Vec2f& v)
    {
        return Min(v[0], v[1]);
    }

    inline int MinEltIndex(const Vec2f& v)
    {
        if (v[0] < v[1])
            return 0;
        else
            return 1;
    }

    inline int MaxEltIndex(const Vec2f& v)
    {
        if (v[0] > v[1])
            return 0;
        else
            return 1;
    }

    inline float MaxElt(const Vec3f& v)
    {
        return Max(Max(v[0], v[1]), v[2]);
    }

    inline float MinElt(const Vec3f& v)
    {
        return Min(Min(v[0], v[1]), v[2]);
    }

    inline int MinEltIndex(const Vec3f& v)
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

    inline int MaxEltIndex(const Vec3f& v)
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

    inline int MinCmptIndex(const cColour& c)
    {
        if (c[0] < c[1])
            if (c[0] < c[2])
                return 0;
            else
                return 2;
        else
            if (c[1] < c[2])
                return 1;
            else
                return 2;
    }

    inline int MaxCmptIndex(const cColour& c)
    {
        if (c[0] > c[1])
            if (c[0] > c[2])
                return 0;
            else
                return 2;
        else
            if (c[1] > c[2])
                return 1;
            else
                return 2;
    }

    inline Vec2f MaxElts(const Vec2f& a, const Vec2f& b)
    {
        return Vec2f
        (
            Max(a[0], b[0]),
            Max(a[1], b[1])
        );
    }

    inline Vec2f MinElts(const Vec2f& a, const Vec2f& b)
    {
        return Vec2f
        (
            Min(a[0], b[0]),
            Min(a[1], b[1])
        );
    }

    inline Vec3f MaxElts(const Vec3f& a, const Vec3f& b)
    {
        return Vec3f
        (
            Max(a[0], b[0]),
            Max(a[1], b[1]),
            Max(a[2], b[2])
        );
    }

    inline Vec3f MinElts(const Vec3f& a, const Vec3f& b)
    {
        return Vec3f
        (
            Min(a[0], b[0]),
            Min(a[1], b[1]),
            Min(a[2], b[2])
        );
    }

    inline float MaxCmpt(const cColour& c)
    {
        return Max(Max(c[0], c[1]), c[2]);
    }

    inline float MinCmpt(const cColour& c)
    {
        return Min(Min(c[0], c[1]), c[2]);
    }

    inline void FindMaxCmpts(const cColour& a, const cColour& b, cColour& c)
    {
        c[0] = Max(a[0], b[0]);
        c[1] = Max(a[1], b[1]);
        c[2] = Max(a[2], b[2]);
    }

    inline void FindMinCmpts(const cColour& a, const cColour& b, cColour& c)
    {
        c[0] = Min(a[0], b[0]);
        c[1] = Min(a[1], b[1]);
        c[2] = Min(a[2], b[2]);
    }

    inline Vec2f ClampUnit(Vec2f v)
    {
        return Vec2f(ClampUnit(v[0]), ClampUnit(v[1]));
    }

    inline Vec3f ClampUnit(Vec3f v)
    {
        return Vec3f(ClampUnit(v[0]), ClampUnit(v[1]), ClampUnit(v[2]));
    }

    inline Vec4f ClampUnit(Vec4f v)
    {
        return Vec4f(ClampUnit(v[0]), ClampUnit(v[1]), ClampUnit(v[2]), ClampUnit(v[3]));
    }

    inline float BoxVol(const Vec3f& b0, const Vec3f& b1)
    {
        Vec3f t = b1 - b0;

        return t[0] * t[1] * t[2];
    }

    inline float BoxArea(const Vec3f& b0, const Vec3f& b1)
    {
        Vec3f t = b1 - b0;

        return 2 * (t[0] * t[1] + t[1] * t[2] + t[2] * t[0]);
    }

    inline bool HasNAN(Vec2f v)
    {
        return IsNAN(v[0]) || IsNAN(v[1]);
    }
    inline bool HasNAN(Vec3f v)
    {
        return IsNAN(v[0]) || IsNAN(v[1]) || IsNAN(v[2]);
    }
    inline bool HasNAN(Vec4f v)
    {
        return IsNAN(v[0]) || IsNAN(v[1]) || IsNAN(v[2]) || IsNAN(v[3]);
    }

    inline bool PlaneIntersection
    (
        const Vec3f&    start,
        const Vec3f&    direction,
        float           d,
        const Vec3f&    normal,
        float&          t
    )
    /*!
        Returns true if given ray intersects the plane specified by d and
        normal. (The plane equation is \f$ P \cdot normal + d = 0 \f$.) If there is
        an intersection, t is set, and the intersection point will be 
        \f$start + t direction\f$. 
    */
    {
        float normDotDirection = dot(normal, direction);
        
        if (normDotDirection == 0.0f)
            return false;

        t = (d + dot(normal, start)) / -normDotDirection;

        return true;
    }

    inline Vec3f FindOrthoVector(const Vec3f& v)
    /*!
        Warning: the vector this routine returns is* not*
        normalised. (Though the length will be close to 1.) 
    */
    {
        Vec3f u;
        
        // we choose the axis that is most orthogonal to v,
        // and return the cross product of it and v. Really!

        if (fabs(v[0]) < fabs(v[1]))
            if (fabs(v[0]) < fabs(v[2]))
            {   u[0] =     0; u[1] =  v[2]; u[2] = -v[1]; }
            else
            {   u[0] =  v[1]; u[1] = -v[0]; u[2] =     0; }
        else
            if (fabs(v[1]) < fabs(v[2]))
            {   u[0] = -v[2]; u[1] =     0; u[2] =  v[0]; }
            else
            {   u[0] =  v[1]; u[1] = -v[0]; u[2] =     0; }
            
        return u;
    }

    inline float RandomRange(Vec2f range, tSeed32* seed)
    {
        return range[0] + (range[1] - range[0]) * UFloatFromSeed(NextSeed(seed));
    }

    inline Vec2f RandomSquare2f(tSeed32* seed)
    {
        Vec2f result;

        result[0] = SFloatFromSeed(NextSeed(seed));
        result[1] = SFloatFromSeed(NextSeed(seed));

        return result;
    }

    inline Vec2f RandomCircle2f(tSeed32* seed)
    {
        Vec2f result;
        float len2;

        // Reject points outside unit sphere. Seems clunky but expected iteration count is < 2
        do
        {
            result[0] = SFloatFromSeed(NextSeed(seed));
            result[1] = SFloatFromSeed(NextSeed(seed));

            len2 = sqrlen(result);
        }
        while (len2 > 1.0f);

        return result;
    }

    inline Vec2f RandomDir2f(tSeed32* seed)
    {
        Vec2f result;
        float len2;

        // Reject points outside unit sphere and inside epsilon ball. Seems clunky but expected iteration count is < 2
        do
        {
            result[0] = SFloatFromSeed(NextSeed(seed));
            result[1] = SFloatFromSeed(NextSeed(seed));

            len2 = sqrlen(result);
        }
        while (len2 > 1.0f || len2 < 1e-4f);

        return result * InvSqrtFast(len2);
    }

    inline Vec2f RandomRange(const cBounds2& bounds, tSeed32* seed)
    {
        Vec2f result;

        result[0] = bounds.mMin[0] + (bounds.mMax[0] - bounds.mMin[0]) * UFloatFromSeed(NextSeed(seed));
        result[1] = bounds.mMin[1] + (bounds.mMax[1] - bounds.mMin[1]) * UFloatFromSeed(NextSeed(seed));

        return result;
    }

    inline Vec3f RandomCube3f(tSeed32* seed)
    {
        Vec3f result;

        result[0] = SFloatFromSeed(NextSeed(seed));
        result[1] = SFloatFromSeed(NextSeed(seed));
        result[2] = SFloatFromSeed(NextSeed(seed));

        return result;
    }

    inline Vec3f RandomSphere3f(tSeed32* seed)
    {
        Vec3f result;
        float len2;

        // Reject points outside unit sphere and inside epsilon ball. Seems clunky but expected iteration count is < 2
        do
        {
            result[0] = SFloatFromSeed(NextSeed(seed));
            result[1] = SFloatFromSeed(NextSeed(seed));
            result[2] = SFloatFromSeed(NextSeed(seed));
            
            len2 = sqrlen(result);
        }
        while (len2 > 1.0f);

        return result;
    }

    inline Vec3f RandomDir3f(tSeed32* seed)
    {
        Vec3f result;
        float len2;

        // Reject points outside unit sphere and inside epsilon ball. Seems clunky but expected iteration count is < 2
        do
        {
            result[0] = SFloatFromSeed(NextSeed(seed));
            result[1] = SFloatFromSeed(NextSeed(seed));
            result[2] = SFloatFromSeed(NextSeed(seed));
            
            len2 = sqrlen(result);
        }
        while (len2 > 1.0f || len2 < 1e-4f);

        return result * InvSqrtFast(len2);
    }

    inline Vec3f RandomRange(const cBounds3& bounds, tSeed32* seed)
    {
        Vec3f result;

        result[0] = bounds.mMin[0] + (bounds.mMax[0] - bounds.mMin[0]) * UFloatFromSeed(NextSeed(seed));
        result[1] = bounds.mMin[1] + (bounds.mMax[1] - bounds.mMin[1]) * UFloatFromSeed(NextSeed(seed));
        result[2] = bounds.mMin[2] + (bounds.mMax[2] - bounds.mMin[2]) * UFloatFromSeed(NextSeed(seed));

        return result;
    }
}

#endif
