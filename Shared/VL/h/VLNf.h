/*
    File:           VLNf.h

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

#include "VLVec.h"
#include "VLSparseVec.h"

#include "VLMat.h"
#include "VLSparseMat.h"

#include "VLSolve.h"
#include "VLFactor.h"

#endif
