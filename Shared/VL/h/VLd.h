/*
    File:           VLd.h

    Function:       Master header for a version of the VL library based
                    on doubles. The various classes are named Vecd,
                    Mat3d, SparseVecd, etc.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VLd_H
#define VLd_H

#ifdef VL_H
#include "VLUndef.h"
#endif

#define VL_V_REAL double
#define VL_V_SUFF(X) X ## d

#include "VLVec2.h"
#include "VLVec3.h"
#include "VLVec4.h"

#include "VLMat2.h"
#include "VLMat3.h"
#include "VLMat4.h"

#include "VLTransform.h"

#include "VLVec.h"
#include "VLSparseVec.h"

#include "VLMat.h"
#include "VLSparseMat.h"

#include "VLSolve.h"

#include "VLVol.h"

#ifndef VL_NO_IOSTREAM
    #include "VLStream.h"
#endif

#endif
