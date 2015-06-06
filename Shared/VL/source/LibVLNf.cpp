/*
    File:       LibVLNf.cpp
    
    Purpose:    Compiles all code necessary for VLNf.h.
    
    Author:     Andrew Willmott
*/


#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f
#define VL_M_REAL float
#define VL_M_SUFF(X) X ## f

#include "VLVec.cpp"
#include "VLSparseVec.cpp"
#include "VLSubVec.cpp"
#include "VLSubSVec.cpp"

#include "VLMat.cpp"
#include "VLSparseMat.cpp"
#include "VLSubMat.cpp"
#include "VLSubSMat.cpp"

#include "VLSolve.cpp"
#include "VLFactor.cpp"
