/*
    File:       CLQuaternion.h

    Function:   Quaternion manipulation

    Author:     Andrew Willmott

    Copyright:  2000, Andrew Willmott
*/

#ifndef CL_QUATERNION_H
#define CL_QUATERNION_H

#include <VL234f.h>

namespace nCL
{
    typedef Vec4f Quatf;

    Quatf MakeQuat(const Vec3f& axis, float theta);
    ///< Make quaternion from rotation about axis by theta

    Quatf MakeQuat(const Mat3f& t);
    ///< Make quaternion from rotation matrix.
    ///< t is assumed to contain the orthonormal axes of the new (rotated)
    ///< space in its columns. I.e. v' = t * v.  */

    Quatf MakeQuat(const Vec3f& point);
    ///< make quaternion from a point

    Quatf MakeQuat(float s);
    ///< make quaternion from a scalar

    void MakeFromQuat(const Quatf& q, Mat3f& t);
    ///< create the equivalent transformation matrix for q

    Quatf QuatMult(const Quatf& a, const Quatf& b);
    ///< quaternion multiplication -- use this instead of *

    Quatf QuatConj(const Quatf& q);
    ///< quaternion conjugate. if len(q) = 1.0, this is also the inverse.

    Quatf QuatInv(const Quatf& q);
    ///< quaternion inverse.

    Vec3f xform(const Quatf& q, const Vec3f& p);
    ///< transform p by q

    void FastRenormalize(Quatf& q);
    ///< Renormalizes a mostly normalized quaternion.

    void ConstrainNeighbourhood(const Quatf& q1, Quatf& q2);
    ///< Adjust q2 so lerp between them takes shortest path.

    ////////////////////////////////////////////////////////////



    // --- Inlines -----------------------------------------------------------------

    inline Quatf MakeQuat(const Vec3f& point)
    { 
        return Quatf(point, 0.0f);
    }

    inline Quatf MakeQuat(float s)
    { 
        return Quatf(0.0f, 0.0f, 0.0f, s);
    }

    inline void MakeFromQuat(const Quatf& q, Mat3f& t)
    { 
        t.MakeRot(q);
    }

    inline Quatf QuatConj(const Quatf& q)
    { 
        return Quatf(-q[0], -q[1], -q[2], q[3]);
    }

    inline Quatf QuatInv(const Quatf& q)
    { 
        return QuatConj(q) / sqrlen(q);
    }

    inline Vec3f xform(const Quatf& q, const Vec3f& p)
    {
        Quatf tmp = QuatMult(QuatMult(q, Quatf(p, 1.0)), QuatConj(q));
        return reinterpret_cast<const Vec3f&>(tmp);
    }

    inline void ConstrainNeighbourhood(const Quatf& q1, Quatf& q2)
    {
        if (dot(q1, q2) < 0)
            q2 = -q2;
    }

    inline void FastRenormalize(Quatf& q)
    {
        float approxOneOverLen = (3.0f - sqrlen(q)) * 0.5f;
        q *= approxOneOverLen;
    }
}

#endif
