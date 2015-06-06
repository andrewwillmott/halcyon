//
//  File:       CLFrustum.h
//
//  Function:   Utilities for dealing with view frustums and clipping
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#ifndef CL_FRUSTUM_H
#define CL_FRUSTUM_H

#include <VL234f.h>

namespace nCL
{
    class cBounds3;

    // Frustum planes
    void ExtractPlanes  (const Mat4f& m, Vec4f planes[6]);  ///< Extracts frustum planes from the given to_clip matrix.
    void OffsetPlanes          (float s, Vec4f planes[6]);  //!< Offsets all planes by the given amount
    void OffsetNormalisedPlanes(float s, Vec4f planes[6]);  ///< Faster but requires length(planes[i].AsVec3()) == 1

    // Frustum testing
    typedef uint32_t tClipFlags;

    enum tClipFlag : tClipFlags
    {
        kNoFlags            = 0x000,
        kInsidePlane0       = 0x001,
        kInsidePlane1       = 0x002,
        kInsidePlane2       = 0x004,
        kInsidePlane3       = 0x008,
        kInsidePlane4       = 0x010,
        kInsidePlane5       = 0x020,
        kOutsideFrustum     = 0x040,
        kIntersectsFrustum  = 0x080
    };

    tClipFlags FrustumTestSphere(Vec3f p, float r,     Vec4f planes[6], tClipFlags flags = kNoFlags);  //!< Returns kOutsideFrustum, or some combination of kInsidePlane0 flags indicating which planes the sphere is wholly inside
    tClipFlags FrustumTestAABB  (const cBounds3& bbox, Vec4f planes[6], tClipFlags flags = kNoFlags);  //!< Returns kOutsideFrustum, or some combination of kInsidePlane0 flags indicating which planes the AABB is wholly inside
    // Note: these calls will skip tests for any input kInsidePlaneN flags. Thus if you have hierarchical bounds, or
    // increasingly tight bounds, it is a speed up to pass the results of previous tests into the finer-grained test.


    void FindPlaneAABBIndices  (Vec4f plane,     int8_t ip0[3],    int8_t ip1[3]);      //!< Returns indices of extremal aabb points w.r.t plane normal
    void FindFrustumAABBIndices(Vec4f planes[6], int8_t ip0[6][3], int8_t ip1[6][3]);   //!< Returns indices of extremal aabb points w.r.t frustum plane normals

    tClipFlags FrustumTestAABB(const cBounds3& bbox, Vec4f planes[6], const int8_t ip0[6][3], const int8_t ip1[6][3], tClipFlags flags);
    //!< This is a version of FrustumTestAABB where the AABB indices are precalculated. This speeds things up if you
    //!< are testing lots of AABBs against the same frustum.
}

#endif
