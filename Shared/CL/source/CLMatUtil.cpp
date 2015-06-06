/*
    File:       CLMatUtil.cpp

    Function:   Provides useful matrix utilities

    Author:     Andrew Willmott

    Notes:      
*/

#include <CLMatUtil.h>

#include <CLVecUtil.h>

#ifdef CL_VL_FULL

void nCL::MakeAbs(Matf& mat)
{
    for (int i = 0; i < mat.Rows(); i++)
        for (int j = 0; j < mat.Cols(); j++)
            mat(i, j) = fabs(mat(i, j));
}

void nCL::MakeAbs(Matd& mat)
{
    for (int i = 0; i < mat.Rows(); i++)
        for (int j = 0; j < mat.Cols(); j++)
            mat(i, j) = fabs(mat(i, j));
}

void nCL::ClipToZeroOne(Matf& mat)
{
    for (int i = 0; i < mat.Rows(); i++)
        for (int j = 0; j < mat.Cols(); j++)
        {
            float s = mat(i, j);
            if (s < 0.0f)
                mat(i, j) = 0.0f;
            else if (s > 1.0f)
                mat(i, j) = 1.0f;
        }
}

void nCL::ClipToZeroOne(Matd& mat)
{
    for (int i = 0; i < mat.Rows(); i++)
        for (int j = 0; j < mat.Cols(); j++)
        {
            double s = mat(i, j);
            if (s < 0.0f)
                mat(i, j) = 0.0f;
            else if (s > 1.0f)
                mat(i, j) = 1.0f;
        }
}
#endif

bool nCL::HasNAN(const Mat2f& m)
{
    return HasNAN(m[0]) || HasNAN(m[1]);
}

bool nCL::HasNAN(const Mat3f& m)
{
    return HasNAN(m[0]) || HasNAN(m[1]) || HasNAN(m[2]);
}

bool nCL::HasNAN(const Mat4f& m)
{
    return HasNAN(m[0]) || HasNAN(m[1]) || HasNAN(m[2]) || HasNAN(m[3]);
}

Mat3f nCL::MakeRandomRotation(Vec3f x)
/*! From Jim Arvo, graphics gems II */
{
    /* Use the random variables x[0] and x[1] to determine the axis of  */
    /* rotation in cylindrical coordinates and the random variable x[2] */
    /* to determine the amount of rotation, omega, about this axis.     */

    float z = x[0];
    float r = sqrt(1.0f - z * z);
    float theta = 2.0f * vl_pi * x[1];
    float omega = vl_pi * x[2];

    /* Compute the unit quaternion (a,b,c,d) where a is the cosine of    */
    /* half the rotation angle and the axis vector (b,c,d) is determined */
    /* by "r", "theta" and "z" computed above.                           */

    float s = sinf(omega);
    float a = cosf(omega);
    float b = s * cosf(theta) * r;
    float c = s * sinf(theta) * r;
    float d = s * z;

    /* Compute all the pairwise products of a, b, c, and d, except a * a. */

    float bb = b * b;   float cc = c * c;   float dd = d * d;
    float ab = a * b;   float ac = a * c;   float ad = a * d;
    float bc = b * c;   float bd = b * d;   float cd = c * d;

    /* Construct an orthogonal matrix corresponding to  */
    /* the unit quaternion (a,b,c,d).                   */

    Mat3f M;

    M[0][0] = 1.0f - 2.0f * (cc + dd);
    M[0][1] =        2.0f * (bc + ad);
    M[0][2] =        2.0f * (bd - ac);

    M[1][0] =        2.0f * (bc - ad);
    M[1][1] = 1.0f - 2.0f * (bb + dd);
    M[1][2] =        2.0f * (cd + ab);

    M[2][0] =        2.0f * (bd + ac);
    M[2][1] =        2.0f * (cd - ab);
    M[2][2] = 1.0f - 2.0f * (bb + cc);

    return M;
}

Mat3f nCL::MakeRot(Vec3f from, Vec3f to)
{
    Vec3f axis(cross(from, to));

    float s = len(axis);        // sin(theta) between from & to
    float c = dot(from, to);    // cos(theta)

    if (s < 1e-6f) // identical or opposing directions; substitute arbitrary orthogonal vector
        axis = norm(FindOrthoVector(from));
    else
        axis /= s;

    // Rodrigues' rotation formula for rotating about unit-length axis by theta.
    Vec3f ta = axis * (1 - c);
    Vec3f sa = axis * s;

    // oprod(t, axis) + c/s mix
    Mat3f result
    (
        ta[0] * axis[0] + c    ,  ta[0] * axis[1] + sa[2],  ta[0] * axis[2] - sa[1],
        ta[1] * axis[0] - sa[2],  ta[1] * axis[1] + c   ,   ta[1] * axis[2] + sa[0],
        ta[2] * axis[0] + sa[1],  ta[2] * axis[1] - sa[0],  ta[2] * axis[2] + c
    );

    return result;
}



Mat3f nCL::AlignZToDirWithYUp(Vec3f v)
{
    Mat3f orient;
    orient[2] = v;

    // find x'
    Vec3f x = cross_y(v);
    float xLen2  = -sqrlen(x);

    if (xLen2 < 0)
    {
        x /= sqrtf(xLen2);
        
        orient[0] = x;
        orient[1] = cross(v, x);         
    }
    else
    {
        // any direction will do
        orient[0] = vl_z;
        orient[1] = vl_x;
    }

    return orient;
}

Mat3f nCL::AlignXToDirWithYUp(Vec3f v)
{
    Mat3f orient;
    orient[0] = v;

    // find z'
    Vec3f z = cross_y(v);
    float zLen2 = sqrlen(z);

    if (zLen2 > 0.0f)
    {
        z /= sqrtf(zLen2);
        
        orient[2] = z;
        orient[1] = cross(z, v);         
    }
    else
    {
        // any direction will do
        orient[1] = vl_z;
        orient[2] = vl_x;
    }

    return orient;
}

Mat3f nCL::AlignYToDir(Vec3f v)
{
    Mat3f orient;
    orient[1] = v;

    if (fabsf(v[0]) > fabsf(v[2]))
    {
        // z axis is closest to orthogonal to y' -- we'll use it
        // to find the x' vector.
        Vec3f ax = norm(cross_z(v));

        orient[2] = cross(ax, v);
        orient[0] = ax;
    }   
    else
    {
        // otherwise go with x
        Vec3f az = norm(-cross_x(v));

        orient[0] = cross(v, az);
        orient[2] = az;
    }

    return orient;
}

Mat3f nCL::AlignYToDirWithZUp(Vec3f v)
{
    Mat3f orient;
    orient[1] = v;

    // find x'
    Vec3f x = cross_z(v);
    float xLen2 = sqrlen(x);

    if (xLen2 > 0)
    {
        x /= sqrtf(xLen2);
        
        orient[0] = x;
        orient[2] = cross(x, v);
    }
    else
    {
        // any direction will do
        orient[0] = vl_y;
        orient[2] = vl_x;
    }

    return orient;
}

Mat3f nCL::AlignXToDirWithZUp(Vec3f v)
{
    Mat3f orient;
    orient[0] = v;

    // find y'
    Vec3f y = -cross_z(v);
    float yLen2 = sqrlen(y);

    if (yLen2 > 0)
    {
        y /= sqrtf(yLen2);
        
        orient[1] = y;
        orient[2] = cross(v, y);         
    }
    else
    {
        // any direction will do
        orient[1] = vl_x;
        orient[2] = vl_y;
    }

    return orient;
}

Mat3f nCL::AlignZToDir(Vec3f v)
{
    Mat3f orient;
    orient[2] = v;

    if (fabsf(v[0]) > fabsf(v[1]))
    {
        // y axis is closest to orthogonal to z' -- we'll use it to find the x' vector.
        Vec3f ax = norm(-cross_y(v));
        
        orient[1] = cross(v, ax);
        orient[0] = ax;
    }   
    else
    {
        // otherwise go with x
        Vec3f ay = norm(cross_x(v));

        orient[0] = cross(ay, v);
        orient[1] = ay;
    }

    return orient;
}

void nCL::PreRotateX(float theta, Mat4f* mIn)
{
    Mat4f& m = *mIn;

    float c = cosf(theta);
    float s = sinf(theta);
    
    Vec4f m1 = m[1];
    Vec4f m2 = m[2];

    m[1] =  c * m1 + s * m2;
    m[2] = -s * m1 + c * m2;
}

void nCL::PreRotateY(float theta, Mat4f* mIn)
{
    Mat4f& m = *mIn;

    float c = cosf(theta);
    float s = sinf(theta);
    
    Vec4f m1 = m[1];
    Vec4f m2 = m[2];

    m[1] = c * m1 - s * m2;
    m[2] = s * m1 + c * m2;
}

void nCL::PreRotateZ(float theta, Mat4f* mIn)
{
    Mat4f& m = *mIn;

    float c = cosf(theta);
    float s = sinf(theta);
    
    Vec4f m0 = m[0];
    Vec4f m1 = m[1];

    m[0] =  c * m0 + s * m1;
    m[1] = -s * m0 + c * m1;
}

void nCL::MakePerspectiveYUp(float fovV, float fovH, float near, float far, Mat4f* mIn)
{
/*
       Given f defined as follows:

                               f  = cotangent(fovy/2)

       The generated matrix is
             f
        ------------       0              0              0
           aspect


            0              f              0              0

                                      zFar+zNear    2*zFar*zNear
            0              0          ----------    ------------
                                      zNear-zFar     zNear-zFar

            0              0              -1             0

*/

    float sv = 1.0f / tanf(fovV * (vl_pi / 360.f));
    float sh = 1.0f / tanf(fovH * (vl_pi / 360.f));

    float d1 = (far + near)     / (near - far);
    float d2 = (2 * far * near) / (near - far);

    Mat4f& m = *mIn;

    m = vl_zero;

    m(0, 0) = sh;
    m(1, 1) = sv;

    m(2, 2) = d1;
    m(2, 3) = -1.0f;

    m(3, 2) = d2;
}

void nCL::MakePerspectiveZUp(float fovV, float fovH, float near, float far, Mat4f* mIn)
{
/*
    [x y z w] [ sh  0   0   0 ]
              [ 0   0  -d1  1 ]
              [ 0   sv  0   0 ]
              [ 0   0   d2  0 ]
              
    // y' = sv z / y
    // view origin = [0, 0, 0, 1] viewDir = [0, 1, 0, 0]
    // so in clip viewOrigin = [0, 0, d2, 0]
*/
    float sv = 1.0f / tanf(fovV * (vl_pi / 360.f));
    float sh = 1.0f / tanf(fovH * (vl_pi / 360.f));

    float d1 = (far + near)     / (near - far);
    float d2 = (2 * far * near) / (near - far);

    Mat4f& m = *mIn;

    m = vl_zero;

    m(0, 0) = sh;
    m(2, 1) = sv;

    m(1, 2) = -d1;
    m(1, 3) = 1.0f;

    m(3, 2) = d2;
}


Vec3f nCL::UnprojectViaToClip(const Mat4f& toClip, Vec2f c)
{
    return UnprojectViaFromClip(inv(toClip), c);
}

Vec3f nCL::UnprojectViaFromClip(const Mat4f& fromClip, Vec2f c)
{
    // proj(Vec4f(cp, 1.0f, 1.0) * fromClip)
    Vec4f hp;
    hp =  fromClip[0] * c[0];
    hp += fromClip[1] * c[1];
    hp += fromClip[2];
    hp += fromClip[3];

    return proj(hp);
}

void nCL::UnprojectViaToClip(const Mat4f& fromClip, Vec2f c, Vec3f* p0, Vec3f* p1)
{
    return UnprojectViaFromClip(inv(fromClip), c, p0, p1);
}

void nCL::UnprojectViaFromClip(const Mat4f& fromClip, Vec2f c, Vec3f* p0, Vec3f* p1)
{
    // proj(Vec4f(cp, 0 | 1, 1.0) * fromClip)
    Vec4f hp0;
    hp0 =  fromClip[0] * c[0];
    hp0 += fromClip[1] * c[1];
    hp0 += fromClip[3];

    Vec4f hp1 = hp0;
    hp1 += fromClip[2];

    *p0 = proj(hp0);
    *p1 = proj(hp1);
}
