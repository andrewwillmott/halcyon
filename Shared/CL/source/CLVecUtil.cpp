/*
    File:           CLVecUtil.cpp
    
    Function:       Contains a grab-bag of useful graphics vector routines.

    Author(s):      Andrew Willmott
*/

#include <CLVecUtil.h>

bool nCL::Refract
(
    float         fromIndex, 
    float         toIndex,
    const Vec3f&  v,            /// -I
    const Vec3f&  n,            /// N
    float         cosNV,         /// -I.N
    Vec3f&        refractDir
)
/*! Returns true if total internal reflection occurs. */
{
    float kn = fromIndex / toIndex;
    float cos2 = 1.0f - sqr(kn) * (1.0f - sqr(cosNV));

    if (cos2 < 0.0f)
        return true;          // Total internal reflection!
            
    float k = kn * cosNV - sqrt(cos2);
    
    refractDir = -kn * v + k * n;

    return false;
}

Vec3f nCL::TriAreaNormal
(
    const Vec3f& a,
    const Vec3f& b, 
    const Vec3f& c
)
/*! Sets n to the (unnormalised) normal of the triangle defined by
    points a, b and c. Effectively returns the sum of the three possible
    edge cross-products, for stability. The length of this vector is 2 x
    the area of the triangle. */
{
    Vec3f n;

    n[0] = (a[1] - b[1]) * (a[2] + b[2]) + (b[1] - c[1]) * (b[2] + c[2]) + (c[1] - a[1]) * (c[2] + a[2]);
    n[1] = (a[2] - b[2]) * (a[0] + b[0]) + (b[2] - c[2]) * (b[0] + c[0]) + (c[2] - a[2]) * (c[0] + a[0]);
    n[2] = (a[0] - b[0]) * (a[1] + b[1]) + (b[0] - c[0]) * (b[1] + c[1]) + (c[0] - a[0]) * (c[1] + a[1]);

    return n;
}

void nCL::UpdateBounds(const Vec3f& pt, Vec3f& min, Vec3f& max)
{
    if (min[0] > pt[0])
        min[0] = pt[0];
    else if (max[0] < pt[0])
        max[0] = pt[0];

    if (min[1] > pt[1])
        min[1] = pt[1];
    else if (max[1] < pt[1])
        max[1] = pt[1];

    if (min[2] > pt[2])
        min[2] = pt[2];
    else if (max[2] < pt[2])
        max[2] = pt[2];
}

namespace
{
    inline double xCross(const Vec3f &a, const Vec3f &b, const Vec3f &c)
    //  returns (a - b) x (c - a) . (1, 0, 0)
    { return (a[1] - b[1]) * (c[2] - b[2]) - (a[2] - b[2]) * (c[1] - b[1]); }

    inline double yCross(const Vec3f &a, const Vec3f &b, const Vec3f &c)
    //  returns (a - b) x (c - a) . (0, 1, 0)
    { return (a[2] - b[2]) * (c[0] - b[0]) - (a[0] - b[0]) * (c[2] - b[2]); }

    inline double zCross(const Vec3f &a, const Vec3f &b, const Vec3f &c)
    //  returns (a - b) x (c - a) . (0, 0, 1)
    { return (a[0] - b[0]) * (c[1] - b[1]) - (a[1] - b[1]) * (c[0] - b[0]); }

    const float kEdgeFuzz = 0.0f;
}


bool nCL::PointIsInsideTriangle
(
    const Vec3f&    p0,
    const Vec3f&    p1,
    const Vec3f&    p2,
    const Vec3f&    point,
    Vec3f*          coords
)
/*! Given a point lying in the plane of the triangle, we return false
    if it lies outside the triangle, and its barycentric coordinates if it
    lies inside. */
{
    // Calculate face normal & other info
    
    Vec3f normal = TriAreaNormal(p0, p1, p2);

    normal[0] = fabsf(normal[0]);
    normal[1] = fabsf(normal[1]);
    normal[2] = fabsf(normal[2]);

    int normMajorAxis = MaxEltIndex(normal);
    float normMaxCmpt = normal[normMajorAxis];
    
    if (normMaxCmpt == 0.0f)
        // degenerate triangle... can't hit this.
        return false;
        
    float eps = kEdgeFuzz * normMaxCmpt;
    float a, b, c;

    switch(normMajorAxis)
    {
    case 0:
        a = xCross(p2, p1, point);
        if (a < eps)
            return false;

        b = xCross(p0, p2, point);
        if (b < eps)
            return false;

        c = normMaxCmpt - a - b;
        if (c < eps)
            return false;

        break;

    case 1:
        a = yCross(p2, p1, point);
        if (a < eps)
            return false;

        b = yCross(p0, p2, point);      
        if (b < eps)
            return false;

        c = normMaxCmpt - a - b;
        if (c < eps)
            return false;

        break;

    case 2:
        a = zCross(p2, p1, point);
        if (a < eps)
            return false;

        b = zCross(p0, p2, point);
        if (b < eps)
            return false;

        c = normMaxCmpt - a - b;
        if (c < eps)
            return false;
        
        break;
    }

    if (coords)
    {
        float invNormMaxCmpt = 1.0f / normMaxCmpt;

        (*coords)[0] = a * invNormMaxCmpt;
        (*coords)[1] = b * invNormMaxCmpt;
        (*coords)[2] = c * invNormMaxCmpt;
    }
    
    return true;
}

Vec2f nCL::UnitSquareToUnitDisc(Vec2f in)
/** Unit square to unit disk transform from Shirley & Chiu.

    \param in   a point in [0,1]^2
    \param out  a point on radius 1 disk

    This transforms points on [0,1]^2 to points on unit disk centered at
    origin.  Each "pie-slice" quadrant of square is handled as a
    seperate case.  The bad floating point cases are all handled
    appropriately.  The regions for (a,b) are:

    \verbatim
                    phi = pi/2
                   -----*-----
                   |\       /|
                   |  \ 2 /  |
                   |   \ /   |
             phi=pi* 3  *  1 *phi = 0
                   |   / \   |
                   |  / 4 \  |
                   |/       \|
                   -----*-----
                    phi = 3pi/2
    \endverbatim
*/
{
    float     phi, r;
    Vec2f     a = 2.0f * in - vl_1;    /* a is now on [-1,1]^2 */

    if (a[0] > -a[1])
    /* region 1 or 2 */
    {
        if (a[0] > a[1])
        /* region 1, also |a| > |b| */
        {
            r = a[0];
            phi = (vl_pi / 4.0f) * (a[1] / a[0]);
        }
        else
        /* region 2, also |b| > |a| */
        {
            r = a[1];
            phi = (vl_pi / 4.0f) * (2.0f - (a[0] / a[1]));
        }
    }
    else         
    /* region 3 or 4 */
    {
        if (a[0] < a[1]) 
        /* region 3, also |a| >= |b|, a != 0 */
        { 
            r = -a[0];
            phi = (vl_pi / 4.0f) * (4.0f + (a[1] / a[0]));
        }
        else       
        /* region 4, |b| >= |a|, but a==0 and b==0 could occur. */
        {
            r = -a[1];
            if (a[1] != 0.0f)
                phi = (vl_pi / 4.0f) * (6.0f - (a[0] / a[1]));
            else
                phi = 0.0f;
        }
    }

    return Vec2f
    (
        r * cosf(phi),
        r * sinf(phi)
    );
}

Vec2f nCL::UnitDiskToUnitSquare(Vec2f in)
/** Unit disk to unit square transform from Shirley & Chiu.

    \param in   a point on radius 1 disk
    \param out  a point in [0,1]^2 
*/
{
    float     r = len(in);
    float     phi = atan2(in[0], in[1]);
    Vec2f     a;

    if (phi < -vl_pi / 4.0f) 
    /* in range [-pi/4,7pi/4] */
        phi += 2.0f * vl_pi;  
    if (phi < vl_pi / 4.0f)
    /* region 1 */
    {  
        a[0] = r;
        a[1] = phi * a[0] / (vl_pi / 4.0f);
    }
    else if (phi < 3.0f * vl_pi / 4.0f)
    /* region 2 */
    {
        a[1] = r;
        a[0] = -(phi - vl_pi / 2.0f) * a[1] / (vl_pi / 4.0f);
    }
    else if (phi < 5.0f * vl_pi / 4.0f)
    /* region 3 */
    {
        a[0] = -r;
        a[1] = (phi - vl_pi) * a[0] / (vl_pi / 4.0f);
    }
    else
    /* region 4 */
    {
        a[1] = -r;
        a[0] = -(phi - 3.0f * vl_pi / 2.0f) * a[1] / (vl_pi / 4.0f);
    }

    return Vec2f
    (
        (a[0] + 1.0f) * 0.5f,
        (a[1] + 1.0f) * 0.5f
    );
}

float nCL::BoxIntersection(const Vec3f& p, const Vec3f &r, const Vec3f& min, const Vec3f& max)
/// Projects point p in direction r to the surface of the bounding
/// box (min, max). Returns t such that this projected point = p + t * r.
{
    Vec3f  d;

    for (int i = 0; i < 3; i++)
        if (r[i] > 0.0f)
            d[i] = (max[i] - p[i]) / r[i];
        else if (r[i] < 0.0f)
            d[i] = (min[i] - p[i]) / r[i];
        else
            d[i] = FLT_MAX;

    return MinElt(d);
}

Vec3f nCL::RandomSphere(float r, tSeed32* seed)
/// Returns samples distributed evenly through the sphere, circle, or line
{
    // Use rejection sampling to find a point inside the sphere.
    // The chance of finding such a point is 4/3pi / 8 ~= 50% for each sample.
    // The average number of times through the loop is 2.
    // The chance of going through the loop more than 6 times is about 1%.

    float r2 = r * r;
    Vec3f p;
    
    while (true)
    {
        p[0] = RandomSFloat(seed);
        p[1] = RandomSFloat(seed);
        p[2] = RandomSFloat(seed);

        if (sqrlen(p) <= r2)    // point inside the sphere?
            return p;
    }
}

Vec3f nCL::RandomEllipsoid(const cBounds3& bounds, tSeed32* seed)
/// Returns samples distributed evenly through the ellipsoid, ellipse, or line
{
    // Generalisation of RandomSphere
    Vec3f c = bounds.Centre();
    Vec3f r = c - bounds.mMin;
    Vec3f r2 = r * r;

    Vec3f invR2 =
    {
        r2[0] > 0.0f ? 1.0f / r2[0] : 0.0f,
        r2[1] > 0.0f ? 1.0f / r2[1] : 0.0f,
        r2[2] > 0.0f ? 1.0f / r2[2] : 0.0f
    };

    while (true)
    {
        Vec3f p = RandomRange(bounds, seed);
        Vec3f v = (p - c);

        // is this point inside the ellipsoid?
        // pt[0]^2 / vec[0]^2 + ... <= 1
        if (dot((v * v), invR2) <= 1)
            return p;
    }
}

Vec3f nCL::RandomTorus(const cBounds3& bounds, float r, tSeed32* seed)
/// Returns samples from within the given torus, ring, or line.
{
    // Unit torus is r = 0.5, width = (0.5, 1)
    // The volume is 2pi * 0.5 * (pi * (0.5 * 1)) = pi^2 / 2 =~ 4.9. So the average
    // iteration count is a little less than for the sphere test.

    Vec3f v;

    while (true)
    {
        v[0] = RandomSFloat(seed);
        v[1] = RandomSFloat(seed);
        v[2] = RandomSFloat(seed);

        // is this point inside a r=0.5 torus?
        float len2xy = sqrlen(v.AsVec2());

        if (len2xy < 1e-8f)
            continue;

        float len2z  = sqr(v[2]);
        float r1 = sqrtf(len2xy);

        if (len2z + len2xy <= r1)
        {
            // adjust according to true radius: r1' = r1 * r + (1 - r)
            float r1Dash = r + (1.0f - r) / r1;

            Vec3f p(v[0] * r1Dash, v[1] * r1Dash, v[2] * r);

            return bounds.MapFromLocal(0.5f * (p + vl_1));
        }
    }
}

