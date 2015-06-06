/*
    File:       LibVLNd.cpp
    
    Purpose:    Compiles all code necessary for VLNd.h.
    
    Author:     Andrew Willmott
*/


#define VL_V_REAL double
#define VL_V_SUFF(X) X ## d
#define VL_M_REAL double
#define VL_M_SUFF(X) X ## d

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
