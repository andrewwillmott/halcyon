//
//  File:       CLFrustum.cpp
//
//  Function:   Various routines for frustum culling
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#include <CLFrustum.h>

#include <CLVecUtil.h>

using namespace nCL;

//
// If the bounds for the object are completely inside a plane, we don't need to test
// against that plane any more, even if we subdivide the object or try tighter,
// more expensive bounds.
//
// If they are completely outside a plane, we are done, we know the object can't
// intersection the frustum.
//


// See http://codesuppository.blogspot.co.uk/2006/03/frustum-testing-using-plane-extraction.html

void nCL::ExtractPlanes(const Mat4f& m, Vec4f planes[6])
{
    // clip plane extraction
    // Left clipping plane
    planes[0] =
    {
        -(m[0][3] + m[0][0]),
        -(m[1][3] + m[1][0]),
        -(m[2][3] + m[2][0]),
        -(m[3][3] + m[3][0])
    };

    // Right clipping plane
    planes[1] =
    {
        -(m[0][3] - m[0][0]),
        -(m[1][3] - m[1][0]),
        -(m[2][3] - m[2][0]),
        -(m[3][3] - m[3][0])
    };

    // Top clipping plane
    planes[2] =
    {
        -(m[0][3] - m[0][1]),
        -(m[1][3] - m[1][1]),
        -(m[2][3] - m[2][1]),
        -(m[3][3] - m[3][1])
    };

    // Bottom clipping plane
    planes[3] =
    {
        -(m[0][3] + m[0][1]),
        -(m[1][3] + m[1][1]),
        -(m[2][3] + m[2][1]),
        -(m[3][3] + m[3][1])
    };

    // Near clipping plane
    planes[4] =
    {
        -(m[0][3] + m[0][2]),
        -(m[1][3] + m[1][2]),
        -(m[2][3] + m[2][2]),
        -(m[3][3] + m[3][2])
    };

    //  Far clipping plane
    planes[5] =
    {
        -(m[0][3] - m[0][2]),
        -(m[1][3] - m[1][2]),
        -(m[2][3] - m[2][2]),
        -(m[3][3] - m[3][2])
    };
}

void nCL::FindPlaneAABBIndices(Vec4f p, int8_t ip0[3], int8_t ip1[3])
{
    int index =
        ((p[0] >= 0.0f) ? 1 : 0)
      + ((p[1] >= 0.0f) ? 2 : 0)
      + ((p[2] >= 0.0f) ? 4 : 0);

    switch (index)
    {
    case 0:
        // x < 0, y < 0, z < 0
        ip0[0] = 3 + 0;
        ip0[1] = 3 + 1;
        ip0[2] = 3 + 2;

        ip1[0] = 0 + 0;
        ip1[1] = 0 + 1;
        ip1[2] = 0 + 2;
        return;
    case 1:
        // x >= 0, y < 0,  z < 0
        ip0[0] = 0 + 0;
        ip0[1] = 3 + 1;
        ip0[2] = 3 + 2;

        ip1[0] = 3 + 0;
        ip1[1] = 0 + 1;
        ip1[2] = 0 + 2;
        return;
    case 2:
        // x < 0, y >= 0, z < 0
        ip0[0] = 3 + 0;
        ip0[1] = 0 + 1;
        ip0[2] = 3 + 2;

        ip1[0] = 0 + 0;
        ip1[1] = 3 + 1;
        ip1[2] = 0 + 2;
        return;
    case 3:
        // x >= 0, y >= 0, z < 0
        ip0[0] = 0 + 0;
        ip0[1] = 0 + 1;
        ip0[2] = 3 + 2;

        ip1[0] = 3 + 0;
        ip1[1] = 3 + 1;
        ip1[2] = 0 + 2;
        return;
    case 4:
        // x < 0, y < 0, z >= 0
        ip0[0] = 3 + 0;
        ip0[1] = 3 + 1;
        ip0[2] = 0 + 2;

        ip1[0] = 0 + 0;
        ip1[1] = 0 + 1;
        ip1[2] = 3 + 2;
        return;
    case 5:
        // x >= 0, y < 0, z >= 0
        ip0[0] = 0 + 0;
        ip0[1] = 3 + 1;
        ip0[2] = 0 + 2;

        ip1[0] = 3 + 0;
        ip1[1] = 0 + 1;
        ip1[2] = 3 + 2;
        return;
    case 6:
        // x < 0, y >= 0, z >= 0
        ip0[0] = 3 + 0;
        ip0[1] = 0 + 1;
        ip0[2] = 0 + 2;

        ip1[0] = 0 + 0;
        ip1[1] = 3 + 1;
        ip1[2] = 3 + 2;
        return;
    case 7:
        // x >= 0, y >= 0, z >= 0
        ip0[0] = 0 + 0;
        ip0[1] = 0 + 1;
        ip0[2] = 0 + 2;

        ip1[0] = 3 + 0;
        ip1[1] = 3 + 1;
        ip1[2] = 3 + 2;
        return;
    }
}

void nCL::FindFrustumAABBIndices(Vec4f planes[6], int8_t ip0[6][3], int8_t ip1[6][3])
{
    for (int i = 0; i < 6; i++)
        FindPlaneAABBIndices(planes[i], ip0[0], ip1[i]);
}

void nCL::OffsetNormalisedPlanes(float s, Vec4f planes[6])
{
    for (int i = 0; i < 6; i++)
        planes[i][3] -= s;  // assume plane has been normalized as above
}

void nCL::OffsetPlanes(float s, Vec4f planes[6])
{
    for (int i = 0; i < 6; i++)
        planes[i][3] -= s * sqrlen(planes[i].AsVec3());
}

inline float PlaneDistance(Vec4f plane, Vec3f p)
{
    return dot(plane.AsVec3(), p) + plane[3];
}

inline tClipFlags PlaneTestSphere(const Vec3f p, float r, const Vec4f plane, tClipFlags flag)
{
    float d = PlaneDistance(plane, p);

    if (d > r)
        return kOutsideFrustum | kInsidePlane0 | kInsidePlane1 | kInsidePlane2 | kInsidePlane3 | kInsidePlane4 | kInsidePlane5;

    if (d < r)
        return flag;

    return kIntersectsFrustum;
}



namespace
{
    inline tClipFlags PlaneTestAABB(const float bounds[6], Vec4f plane, const int8_t ip[3], tClipFlags flag)
    {
        float d1 = plane[0] * bounds[ip[0]] + plane[1] * bounds[ip[1]] + plane[2] * bounds[ip[2]] + plane[3];

        return (d1 > 0.0f) ? flag : 0;
    }

    /*
        plane test: d0 >= 0 -> completely outside
        plane test: d1 <= 0 -> completely inside
    */

    tClipFlags PlaneTestAABB(const float bounds[6], Vec4f plane, const int8_t ip0[3], const int8_t ip1[3], tClipFlags flag)
    {
        float minExtreme[3] =
        {
            bounds[ip0[0]],
            bounds[ip0[1]],
            bounds[ip0[2]]
        };

        float d1 = plane[0] * minExtreme[0] + plane[1] * minExtreme[1] + plane[2] * minExtreme[2] + plane[3];
        
        if (d1 > 0.0f)
            // completely outside this plane, and thus no need for any more plane tests
            return kOutsideFrustum | kInsidePlane0 | kInsidePlane1 | kInsidePlane2 | kInsidePlane3 | kInsidePlane4 | kInsidePlane5;

        float maxExtreme[3] =
        {
            bounds[ip1[0]],
            bounds[ip1[1]],
            bounds[ip1[2]]
        };

        float d2 = plane[0] * maxExtreme[0] + plane[1] * maxExtreme[1] + plane[2] * maxExtreme[2] + plane[3];

        // straddle
        if (d2 >= 0.0f)
            return kIntersectsFrustum;

        // completely inside: return kInsidePlaneN
        return flag;
    }

    tClipFlags PlaneTestAABB(const float bounds[6], const Vec4f& plane, tClipFlags flag)
    {
        int8_t ip0[3];
        int8_t ip1[3];

        FindPlaneAABBIndices(plane, ip0, ip1);

        return PlaneTestAABB(bounds, plane, ip0, ip1, flag);
    }
}

tClipFlags nCL::FrustumTestSphere(Vec3f p, float r, Vec4f planes[6], tClipFlags flags)
{
    // TODO: is it actually worth doing the early out sphere tests?

    if ((flags & kInsidePlane0) == 0) 
        flags |= PlaneTestSphere(p, r, planes[0], kInsidePlane0);
    if ((flags & kInsidePlane1) == 0) 
        flags |= PlaneTestSphere(p, r, planes[1], kInsidePlane1);
    if ((flags & kInsidePlane2) == 0) 
        flags |= PlaneTestSphere(p, r, planes[2], kInsidePlane2);
    if ((flags & kInsidePlane3) == 0) 
        flags |= PlaneTestSphere(p, r, planes[3], kInsidePlane3);
    if ((flags & kInsidePlane4) == 0) 
        flags |= PlaneTestSphere(p, r, planes[4], kInsidePlane4);
    if ((flags & kInsidePlane5) == 0) 
        flags |= PlaneTestSphere(p, r, planes[5], kInsidePlane5);
    
    return flags;
}

tClipFlags nCL::FrustumTestAABB(const cBounds3& bbox, Vec4f planes[6], tClipFlags flags)
{
    const float* bounds = &bbox.mMin[0];  // recast as float array, makes working with indices easier

    if ((flags & kInsidePlane0) == 0)
        flags |= PlaneTestAABB(bounds, planes[0], kInsidePlane0);
    if ((flags & kInsidePlane1) == 0)
        flags |= PlaneTestAABB(bounds, planes[1], kInsidePlane1);
    if ((flags & kInsidePlane2) == 0) 
        flags |= PlaneTestAABB(bounds, planes[2], kInsidePlane2);
    if ((flags & kInsidePlane3) == 0) 
        flags |= PlaneTestAABB(bounds, planes[3], kInsidePlane3);
    if ((flags & kInsidePlane4) == 0) 
        flags |= PlaneTestAABB(bounds, planes[4], kInsidePlane4);
    if ((flags & kInsidePlane5) == 0) 
        flags |= PlaneTestAABB(bounds, planes[5], kInsidePlane5);
    
    return flags;
}

tClipFlags nCL::FrustumTestAABB(const cBounds3& bbox, Vec4f planes[6], const int8_t ip0[6][3], const int8_t ip1[6][3], tClipFlags flags)
{
    const float* bounds = &bbox.mMin[0];  // recast as float array, makes working with indices easier

    if ((flags & kInsidePlane0) == 0)
        flags |= PlaneTestAABB(bounds, planes[0], ip0[0], ip1[0], kInsidePlane0);
    if ((flags & kInsidePlane1) == 0)
        flags |= PlaneTestAABB(bounds, planes[1], ip0[0], ip1[0], kInsidePlane1);
    if ((flags & kInsidePlane2) == 0) 
        flags |= PlaneTestAABB(bounds, planes[2], ip0[0], ip1[0], kInsidePlane2);
    if ((flags & kInsidePlane3) == 0) 
        flags |= PlaneTestAABB(bounds, planes[3], ip0[0], ip1[0], kInsidePlane3);
    if ((flags & kInsidePlane4) == 0) 
        flags |= PlaneTestAABB(bounds, planes[4], ip0[0], ip1[0], kInsidePlane4);
    if ((flags & kInsidePlane5) == 0) 
        flags |= PlaneTestAABB(bounds, planes[5], ip0[0], ip1[0], kInsidePlane5);
    
    return flags;
}


#ifdef WIP

namespace nCL
{
    /** \class Clipper

        Clips homogeneous coordinates to the display frustrum
        [-1, 1] x [-1, 1] x [0, 1].

        Implements yer bog-standard line clipper.
        Rewritten a la Jim Blinn's presentation in IEEE CGA, Jan 91.
        Similar to Cohen-Sutherland, Cyrus-Beck, Liang-Barsky.

        Note that this is supposed to come after the display
        transformations have been applied, but before the
        final divide by z. It's set up to assume by default
        that our final display space is the -1,-1 -> 1, 1 square
        in x, y, and that the near and far clipping planes have
        been mapped to z = 0 and 1 respectively.

        The standard display pipeline is transform, clip, project, 
        2D draw. The Clipper class should be passed post-transform
        homogeneous coordinates. It will in turn call ViewPoint()
        with clipped coordinates. You should override ViewPoint()
        to perform the homogeneous divide, map to display coords,
        and do the drawing.
    */

    struct cLineClip
    {
    public:
        void            ClipPoint(const Vec4f& p, bool draw);       ///< draw == false: move to p; draw == true: draw to p.
        virtual void    ViewPoint(const Vec4f& p, bool draw) = 0;   ///< override to do actual drawing.
    protected:
        Vec4f   mLastP          = vl_0;
        int     mLastOutcode    = 0;
        float   mLastBC[6]      = { 0.0f };
    };
}

void nCL::cLineClip::ClipPoint(const Vec4f& p, bool draw)
{
    float bc[6];

    // calc BC for [-1, 1] x [-1, 1] x [0, 1]
                                // bc is -ve if:
    bc[0] = p[3] + p[0];        // x < -1
    bc[1] = p[3] - p[0];        // x > 1
    bc[2] = p[3] + p[1];        // y < -1
    bc[3] = p[3] - p[1];        // y > 1
//  bc[4] = p[3] + p[2];        // z < -1
    bc[4] = 0    + p[2];        // z < 0
    bc[5] = p[3] - p[2];        // z > 1

    // should replace this with non-branching code where possible
    int outcode = 0;

    if (bc[0] < 0.0f)
        outcode |= 0x01;
    if (bc[1] < 0.0f)
        outcode |= 0x02;
    if (bc[2] < 0.0f)
        outcode |= 0x04;
    if (bc[3] < 0.0f)
        outcode |= 0x08;
    if (bc[4] < 0.0f)
        outcode |= 0x10;
    if (bc[5] < 0.0f)
        outcode |= 0x20;
    
    // do the clipping

    if (!draw)
    {
        // move if we're inside the clip volume
        if (outcode == 0)
            ViewPoint(p, draw);
    }
    else if ((outcode & mLastOutcode) == 0)
    {
        // we have at least one endpoint inside all planes
        if ((outcode | mLastOutcode) == 0)
            // both are inside: trivial accept
            ViewPoint(p, draw);
        else
        {
            // must clip line segment to the volume.
            int     clip   = outcode | mLastOutcode;
            float   alpha0 = 0.0f;
            float   alpha1 = 1.0f;

            int     mask   = 1;

            // step through each plane              
            for (int i = 0; i < 6; i++, mask <<= 1)
            {
                if (mask & clip)
                // we straddled this boundary
                {
                    // find the alpha of the intersection with this plane
                    float alpha = mLastBC[i] / (mLastBC[i] - bc[i]);

                    if (mLastOutcode & mask)
                        // heading in: clip alpha0
                        alpha0 = Max(alpha0, alpha);
                    else
                        // heading out: clip alpha1
                        alpha1 = Min(alpha1, alpha);
                }
            }

            if (mLastOutcode != 0)
                // heading in to clip volume, so move to entry point
                ViewPoint(mLastP + alpha0 * (p - mLastP), false);

            if (outcode)
                // heading out, so draw to exit point
                ViewPoint(mLastP + alpha1 * (p - mLastP), true);
            else
                // terminate inside clipping volume, so just draw!
                ViewPoint(p, true);
        }
    }

    mLastOutcode = outcode;

    for (int i = 0; i < 6; i++)
        mLastBC[i] = bc[i];

    mLastP = p;
}

#endif
