/*
    File:           VLfd.h

    Function:       Master header for a version of the VL library based
                    on floats and doubles: vectors are made up of
                    floats, and matrices of doubles. The various classes
                    are named Vecf, Mat3d, SparseVecf, etc. To use this
                    header you should link with -lvl.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VLfd_H
#define VLfd_H

#ifdef VL_H
    #include "VLUndef.h"
#endif

#ifndef VLd_H
    #define VL_V_REAL double
    #define VL_V_SUFF(X) X ## d

    #include "VLMat2.h"
    #include "VLMat3.h"
    #include "VLMat4.h"
    #include "VLMat.h"
    #include "VLSparseMat.h"
    #include "VLSolve.h"
    #include "VLTransform.h"
    #ifndef VL_NO_IOSTREAM
        #include "VLStream.h"
    #endif
    #include "VLUndef.h"
#endif

#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f
#define VL_M_REAL double
#define VL_M_SUFF(X) X ## d

#if !defined(VLf_H) && !defined(VL234f_H)
    #include "VLVec2.h"
    #include "VLVec3.h"
    #include "VLVec4.h"
#endif
#ifndef VLf_H
    #include "VLVec.h"
    #include "VLSparseVec.h"
    #ifndef VL_NO_IOSTREAM
        #include "VLStream.h"
    #endif
#endif

#include "VLMixed.h"
#include "VLSolve.h"

#endif
