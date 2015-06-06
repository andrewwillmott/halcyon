/*
    File:           VLf.h

    Function:       Master header for a version of the VL library based on
                    floats. The various classes are named Vecf, Mat3f, 
                    SparseVecf, etc.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VLf_H 
#define VLf_H

#ifdef VL_H
    #include "VLUndef.h"
#endif

#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f

// Allow interoperatibility with 234-only header
#ifndef VL234f_H
    #define VL234f_H
    #include "VLVec2.h"
    #include "VLVec3.h"
    #include "VLVec4.h"

    #include "VLMat2.h"
    #include "VLMat3.h"
    #include "VLMat4.h"
    #include "VLTransform.h"
#else
#define VL_Vec2_H
#define VL_Mat2_H
#endif

#include "VLVec.h"
#include "VLSparseVec.h"

#include "VLMat.h"
#include "VLSparseMat.h"

#include "VLVol.h"

#ifndef VL_NO_IOSTREAM
    #include "VLStream.h"
#endif

#endif
