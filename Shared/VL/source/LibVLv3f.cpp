//
//  File:       LibVLv3f.cpp
//
//  Function:   Instantiates code necessary for VLv3f.h
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//



// To make the Mat stuff work, would need:
// - way to disable SubVec*
// - proper way of turning off sqrt() etc., or sqrt impl
//   - should dot() always return a scalar type?
// - Sort out Mat reference stuff -- passing mL[0] to routine that will modify Matf is use case

#include <VL234f.h>
#include <string.h>

namespace
{
    inline Vec3f sqrt(Vec3f) { return vl_0; };

    void SetReal(Vec3f& u, double e)
    {
        u = Vec3f(float(e));
    }
}

#include "VLUndef.h"
#define VL_V_REAL Vec3f
#define VL_V_SUFF(X) X ## v3f
#define VL_NO_SOLVE
#include "VLVec.cpp"
#include "VLMat.cpp"
#include "VLVol.cpp"
