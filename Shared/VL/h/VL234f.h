/*
    File:           VL234f.h

    Function:       Master header for a version of the VL library based on
                    floats. The various classes are named Vecf, Mat3f, 
                    SparseVecf, etc.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VL234f_H
#define VL234f_H

#ifdef VL_H
    #include "VLUndef.h"
#endif

#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f

#include "VLVec2.h"
#include "VLVec3.h"
#include "VLVec4.h"

#include "VLMat2.h"
#include "VLMat3.h"
#include "VLMat4.h"
#include "VLTransform.h"

#endif
