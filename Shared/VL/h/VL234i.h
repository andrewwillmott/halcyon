/*
    File:           VL234i.h

    Function:       Master header for Vec2i, Vec3i, Vec4i.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 2013, Andrew Willmott
 */


#ifndef VL234i_H
#define VL234i_H

#ifdef VL_H
    #include "VLUndef.h"
#endif

#define VL_V_REAL int
#define VL_V_SUFF(X) X ## i

#include "VLVec2.h"
#include "VLVec3.h"
#include "VLVec4.h"

#include "VLUndef.h"

#endif
