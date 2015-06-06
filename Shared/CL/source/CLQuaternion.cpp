/*
    File:       CLQuaternion.cpp

    Function:   Quaternion manipulation

    Author:     Andrew Willmott

    Notes:      
*/

#include <CLQuaternion.h>

using namespace nCL;

/** \file CLQuaternion.h

    Quaternion functions. 
        You must use QuatMult for q * p.
        a * b = [wa * vb + wb * va + va x vb, wa * wb - dot(va, vb)]
        Scalar multiplication works as normal.
    
        q = [v sin(theta), cos(theta)]

    See Shoemake's quaternion tutorial for full explanations:
        ftp://ftp.cis.upenn.edu/pub/graphics/shoemake/
        
    Rotations are represented by the unit quaternions.

    To rotate a point p by quaternion q:
        p' = (q * p * conj(q)) / ||q||^2

    Accumulated rotations:
        qaccum = q1 * q2

    We use a 4-vector v to store quaternions: v[0..2] are
    the i, j, k components of the quaternion, v[3] is the scalar.

    if q = [v, w],
    
        conjugate(q) = q* = [-v, w]
        (q*)* = q
        inverse(q) = q* / ||q||^2

    The identity quaternion is [0 0 0 1].

    q * [0, w] * conj(q) = [0 w]
    better,
    q * [v, w] * conj(q) = [v' w]

    Remember, quaternions must be normalised to be a valid rotation.
*/

Quatf nCL::MakeQuat(const Vec3f& axis, float theta)
{
    theta *= 0.5f;

    float s = sinf(theta);
    Quatf q;

    q[0] = s * axis[0];
    q[1] = s * axis[1];
    q[2] = s * axis[2];
    q[3] = cosf(theta);
    
    return q;
}

/*
    NOTE:
    
    Op count for quaternion multiplication:
    2 v*s = 6/0
    1 v cross v = 3(2/1) = 6/3
    2 v+v = 0/6
    
    1 v dot v = 3/2
    1* 1- = 1/1
    
    total * / + = 16/12

    for q * p, could be 12/12.
*/

#ifdef REFERENCE
Quatf nCL::QuatMult(const Quatf& a, const Quatf& b)
{
    Quatf  result;
    Vec3f& va = (Vec3f&) a;
    Vec3f& vb = (Vec3f&) b;

    result = Quatf(a[3] * vb + b[3] * va + cross(va, vb), a[3] * b[3] - dot(va, vb));

    return result;
}
#endif

Quatf nCL::QuatMult(const Quatf& a, const Quatf& b)
{
    Quatf result;

    // 16/12
    result[0] = + a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
    result[1] = - a[0] * b[2] + a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
    result[2] = + a[0] * b[1] - a[1] * b[0] + a[2] * b[3] + a[3] * b[2];
    result[3] = - a[0] * b[0] - a[1] * b[1] - a[2] * b[2] + a[3] * b[3];

    return result;
}

/*
    NOTE:
    
    The matrix R corresponding to a quaternion [x y z w] is:
    
    [ 1 - 2(y^2 +z^2)    2(xy - wz)       2(xz + wy)    ]
    [   2(xy + wz)     1 - 2(x^2 +z^2)    2(yz - wx)    ]
    [   2(xz - wy)       2(yz + wx)     1 - 2(x^2 +y^2) ]

    This assumes R is a pure rotation matrix, and q is normalised.
    
    In particular:
        trace(R) = 3 - 4(x^2 + y^2 + z^2)
        so w^2 = 0.25(1 + trace(R))
        t2 = R[1][1] + R[2][2] = 2 - 2x^2 + 2w^2 - 2(x^2 + z^2 + y^2 + w^2)
        so x^2 = w^2 - t2 / 2
        etc.
        
    Op count: 
     *: 2x^2, 2y^2, 2z^2, 2xy, 2xz, 2xw, 2yz, 2yw, 2wz
        3 for 2x, 9 for the combos 
        = 12
        
     +: 12 
     So 12/12
     
     For a 3x3 rotation: 9*, 6+

    Total to do a quat multiply this way: 21/18
    
    For N quat/point multiplications:
        12N/12N vs. 12 + 9N / 12 + 6N
    break even point for mult is: 12 + 9N = 12N -> 3N = 12 -> N = 4
    at that point it's 48/48 vs. 48/36
    
    So, if transforming 4 or more points, makes sense to convert
    to matrix form first.

    For composing a 3x3 rotation: 27/18. So, quaternion wins clearly
    no matter what.
 */


Quatf nCL::MakeQuat(const Mat3f& R)
/** Finds an equivalent quaternion for the rotation matrix R.

    R is assumed to contain the orthonormal axes of the new (rotated)
    space in its columns. Which is a fancy way of saying it rotates
    column vectors!

    This is an adapted version of Shoemake's original algorithm, which
    found the largest of the quaternion components x, y, z, and w, and
    used it to solve for the others, in order to preserve precision. In
    current machine architectures, branches are one of the most
    expensive things you can do, so we loosen this procedure by using an
    epsilon. We try w, x, y and z in strict order, only rejecting a case
    if the compenent's square is less than epsilon. Almost all of the
    time the algorithm executes the simplest 'w' case. Only rotations
    about an axis by close to 180 degrees trigger the other cases.
*/
{
    const double epsilon = 1e-10;

    float     x2, y2, w2;
    float     s;
    Quatf  q;

    w2 = 0.25 * (1.0 + trace(R));

    if (w2 > epsilon)
    {
        s = sqrt(w2);
        q[3] = s;
        s = 0.25 / s;
        q[0] = (R[2][1] - R[1][2]) * s;
        q[1] = (R[0][2] - R[2][0]) * s;
        q[2] = (R[1][0] - R[0][1]) * s;
    }
    else
    {
        // w is zero, so x^2 = 
        q[3] = 0.0;
        x2 = -0.5 * (R[1][1] + R[2][2]);

        if (x2 > epsilon)
        {
            s = sqrt(x2);
            q[0] = s;
            s = 0.5 / s;
            q[1] = R[1][0] * s;
            q[2] = R[2][0] * s;
        }
        else
        {
            q[0] = 0.0;
            y2 = 0.5 * (1.0 - R[2][2]);

            if (y2 > epsilon)
            {
                s = sqrt(y2);
                q[1] = s;
                s = 0.5 / s;
                q[2] = R[2][1] * s;
            }
            else
            {
                q[1] = 0.0;
                q[2] = 1.0;
            }
        }
    }

    q.Normalize();

    return q;
}

#ifdef EXPT
// 22/12
Vec3f xform2(const Quatf& q, const Vec3f& p)
/// q must be normalised for this to be a rotation.
// Writing out: 
{ 
    return proj(QuatMult(QuatMult(q, Quatf(p, 1.0)), QuatConj(q)))); 

    // 13/0
    Real dx = 2 * q[0];
    Real dy = 2 * q[1];
    Real dz = 2 * q[2];
    Real dw = 2 * q[3];
    
    Real dxx = dx * q[0];
    Real dyy = dy * q[1];
    Real dzz = dz * q[2];

    Real dxy = dx * q[1];
    Real dxz = dx * q[2];
    Real dyz = dy * q[2];
    
    Real dwx = dw * q[0];
    Real dwy = dw * q[1];
    Real dwz = dw * q[2];
    
    // 9/12
    r[0] = v[0] * (1 - (yy + zz)) + v[1] * (xy - wz)       + v[2] * (xz + wy);
    r[1] = v[0] * (xy + wz)       + v[1] * (1 - (xx + zz)) + v[2] * (yz - wx);
    r[2] = v[0] * (xz - wy)       + v[1] * (yz + wx)       + v[2] * (1 - (xx + yy));
}

Vec3f xform3(const Quatf& q, const Vec3f& p)
{
    Quatf c;
    c[0] = + a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
    c[1] = - a[0] * b[2] + a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
    c[2] = + a[0] * b[1] - a[1] * b[0] + a[2] * b[3] + a[3] * b[2];
    c[3] = - a[0] * b[0] - a[1] * b[1] - a[2] * b[2] + a[3] * b[3];

    result[0] = + a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
    result[1] = - a[0] * b[2] + a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
    result[2] = + a[0] * b[1] - a[1] * b[0] + a[2] * b[3] + a[3] * b[2];
    result[3] = - a[0] * b[0] - a[1] * b[1] - a[2] * b[2] + a[3] * b[3];
}
#endif

#ifdef UNFINISHED

//------------------------------------------------------------------------------
// DecomposeTwist

void DecomposeTwist
(
    tQuatf* pqTwist,
    Mgc::Quatf* pqNoTwist,
    const Mgc::Quatf& rqRotate,
    const Mgc::Vector3& rvAxis 
)
{

    /*      From Chris Welman
    *
    *      Purpose:
    *      This routine decomposes quaternion q into two parts. One
    *      part is the component of q which twists about the unit
    *      vector 'axis', the other part is whatever is left over
    *      after removing this twist from q.
    *
    *  To find the twist component of q:
    *
    *  o apply q to the axis to get a new rotated axis
    *  o use this vRotatedAxis to derive an aligning quaternion
    *    which takes axis into this new rotated axis (with no twist).
    *  o the twist component of q is just the quaternion
    *    difference between the aligning quaternion and q.
    */

    Mgc::Vector3 vRotatedAxis = rqRotate * rvAxis;

    *pqNoTwist = Align(rvAxis, vRotatedAxis);
    *pqTwist = pqNoTwist->Inverse() * rqRotate;
}

//------------------------------------------------------------------------------
// Align

tQuatf Align(const tVector3& v1, const tVector3& v2)
{
   /*      From Chris Welman
    *
    *      Purpose:
    *      This routine will calculate the quaternion which, when applied
    *      to the normalized vector v1, will yield the vector v2.
    *      Note that this operation is inherently ill-defined since twist
    *      is not considered at all, but this routine is still useful
    *
    *
    *  Take the normalized bisector of the two vectors.
    *  The angle of rotation is the dot product of this
    *  with the original vector v1.
    *  The axis of rotation is the cross product of
    *  the bisector and v1
    *
    *  Why the bisector, you ask?  Remember that the
    *  quaternion that rotates by N degrees is a
    *  function of N/2.  Using the bisector avoids
    *  having to do an inverse trig function, dividing
    *  by 2, and calling another trig function!
    *
    *  Note: v1 & v2 must be normalized for this to work.
    */

   tVector3 vBisector = v1 + v2;
   vBisector.Unitize();  // normalize

   float ww = vBisector.Dot( v1 );
   Mgc::Vector3 vv;

  /*
   *  Watch for the special case where v1 and v2
   *  are opposite vectors.  In this case a
   *  rotation of 180 in any direction will
   *  perform the desired alignment.
   */

   if ( ww != 0 )
   {
       vv = v1.Cross( vBisector );
   }
   else
   {
      /*
       *  Pick any direction other than v1 or v2.
       *  We can do this by exchanging the elements
       *  of v2.
       */
       Mgc::Vector3 vAnydir( v2.z, v2.x, v2.y );

       vv = v1.UnitCross( vAnydir );
   }

   return Mgc::Quatf( ww, vv.x, vv.y, vv.z );
}
   
#endif
