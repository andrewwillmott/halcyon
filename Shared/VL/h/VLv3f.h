//
//  File:       VLv3f.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef VLv3f_H
#define VLv3f_H


// Define VolVec3f
#ifdef VL_H
    #include "VLUndef.h"
#endif

#define VL_V_REAL Vec3f
#define VL_V_SUFF(X) X ## v3f

// this shouldn't be necessary if we switch to requiring len() etc. to always return a scalar rather TReal.
inline Vec3f sqrt(Vec3f v) { return Vec3f(sqrtf(v[0]), sqrtf(v[1]), sqrtf(v[2])); }

#include <VLMat.h>
#include <VLVol.h>
#include "VLUndef.h"

#endif
