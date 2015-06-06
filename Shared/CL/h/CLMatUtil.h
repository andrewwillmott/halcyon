/*
    File:       CLMatUtil.h

    Function:   Provides useful matrix utilities

    Author:     Andrew Willmott

    Copyright:  (c) 1999, Andrew Willmott
*/

#ifndef CL_MATUTIL_H
#define CL_MATUTIL_H

#include <CLDefs.h>

#ifdef CL_VL_FULL
    #include <VLfd.h>
#else
    #include <VL234f.h>
#endif

namespace nCL
{
#ifdef CL_VL_FULL
    void MakeAbs(Matf& mat);
    void MakeAbs(Matd& mat);
    ///< replace each element with its absolute value
    void ClipToZeroOne(Matf& mat);
    void ClipToZeroOne(Matd& mat);
    ///< clip each element to the interval [0, 1]
#endif

    bool HasNAN(const Mat2f& m);
    bool HasNAN(const Mat3f& m);
    bool HasNAN(const Mat4f& m);

    Mat3f MakeRandomRotation(Vec3f v);
    ///< Create a random rotation matrix from the random vector v.
    Mat3f MakeRot(Vec3f from, Vec3f to);
    ///< Create a rotation that will rotate vector 'from' to 'to'.

    // Y-up world alignment
    Mat3f AlignZToDirWithYUp(Vec3f v);
    Mat3f AlignXToDirWithYUp(Vec3f v);
    Mat3f AlignYToDir       (Vec3f v);

    // Z-up world alignment.
    Mat3f AlignYToDirWithZUp(Vec3f v);
    Mat3f AlignXToDirWithZUp(Vec3f v);
    Mat3f AlignZToDir       (Vec3f v);

    // Direct m4 matrix manipulation
    void PreRotateX(float theta, Mat4f* m);
    void PreRotateY(float theta, Mat4f* m);
    void PreRotateZ(float theta, Mat4f* m);

    void MakePerspectiveYUp(float fovV, float fovH, float near, float far, Mat4f* m);
    ///< Create row-vector perspective matrix from the vertical and horizontal fields
    ///< of view, and near and far clip planes. Assumes input is Y-up, -Z away.

    void MakePerspectiveZUp(float fovV, float fovH, float near, float far, Mat4f* m);
    ///< Create row-vector perspective matrix from the vertical and horizontal fields
    ///< of view, and near and far clip planes. Assumes input is Z-up, Y away.

    Vec3f UnprojectViaToClip(const Mat4f& toClip, Vec2f c);
    ///< Takes a 2D view space (really, clip space) coordinate, and returns the corresponding unprojected
    ///< 3D point on the far clip plane.
    ///< Call with worldToClip to get a world space point, cameraToClip to get a camera space point.

    void  UnprojectViaToClip(const Mat4f& toClip, Vec2f c, Vec3f* p0, Vec3f* p1);
    ///< Version of UnprojectViaToClip that returns points on the near and far planes, i.e., the corresponding segment spanning the view frustum.

    Vec3f UnprojectViaFromClip(const Mat4f& fromClip, Vec2f c);
    ///< Cheaper version of UnprojectViaToClip which takes clipToWorld, clipToCamera etc. instead.
    void  UnprojectViaFromClip(const Mat4f& fromClip, Vec2f c, Vec3f* p0, Vec3f* p1);
    ///< Version of UnprojectViaFromClip that returns points on the near and far planes, i.e., the corresponding segment spanning the view frustum.
}

#endif
