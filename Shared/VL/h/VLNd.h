/*
    File:           VLNd.h
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VLNd_H
#define VLNd_H

#ifdef VL_H
    #include "VLUndef.h"
#endif

#define VL_V_REAL double
#define VL_V_SUFF(X) X ## d

#include "VLVec.h"
#include "VLSparseVec.h"

#include "VLMat.h"
#include "VLSparseMat.h"

#include "VLSolve.h"
#include "VLFactor.h"

#endif
