//
//  File:       CLBounds.cpp
//
//  Function:   Ops on AABBs
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLBounds.h>

using namespace nCL;

// --- cBounds3 ------------------------------------------------------------

void nCL::cBounds3::FindCorners(Vec3f corners[8]) const
/** Find corner points of axis-aligned box.

    Creates corners as follows: 
    \verbatim

      2------3
     /|     /|
    6------7 |
    | 0----|-1
    |/     |/
    4 ---- 5


       y
       | 
       +--x
      /
     z

    \endverbatim
*/
{
    corners[0] = Vec3f(mMin[0], mMin[1], mMin[2]);
    corners[1] = Vec3f(mMax[0], mMin[1], mMin[2]);
    corners[2] = Vec3f(mMin[0], mMax[1], mMin[2]);
    corners[3] = Vec3f(mMax[0], mMax[1], mMin[2]);
                                                   
    corners[4] = Vec3f(mMin[0], mMin[1], mMax[2]);
    corners[5] = Vec3f(mMax[0], mMin[1], mMax[2]);
    corners[6] = Vec3f(mMin[0], mMax[1], mMax[2]);
    corners[7] = Vec3f(mMax[0], mMax[1], mMax[2]);
}


cBounds3& cBounds3::Add(int count, const Vec3f* p, size_t stride)
{
    for (int i = 0; i < count; i++)
    {
        Add(p[i]);

        (uint8_t*&) p += stride;
    }

    return *this;
}

// --- cBounds2 -----------------------------------------------------------

void nCL::cBounds2::FindCorners(Vec2f corners[4]) const
/** Find corner points of axis-aligned rect.

    Creates corners as follows: 
    \verbatim

      2------3
      |      |
      |      |
      0------1

       y
       | 
       +--x

    \endverbatim
*/
{
    corners[0] = Vec2f(mMin[0], mMin[1]);
    corners[1] = Vec2f(mMax[0], mMin[1]);
    corners[2] = Vec2f(mMin[0], mMax[1]);
    corners[3] = Vec2f(mMax[0], mMax[1]);
}


cBounds2& cBounds2::Add(int count, const Vec2f* p, size_t stride)
{
    for (int i = 0; i < count; i++)
    {
        Add(p[i]);

        (uint8_t*&) p += stride;
    }

    return *this;
}

