/*
    File:       LibVLd.cpp
    
    Purpose:    Compiles all code necessary for VLd.h.
    
    Author:     Andrew Willmott
*/

#define VL_V_REAL double
#define VL_V_SUFF(X) X ## d
#define VL_M_REAL double
#define VL_M_SUFF(X) X ## d

#include "VLVec2.cpp"
#include "VLVec3.cpp"
#include "VLVec4.cpp"

#include "VLMat2.cpp"
#include "VLMat3.cpp"
#include "VLMat4.cpp"

#include "VLVec.cpp"
#include "VLSparseVec.cpp"
#include "VLSubVec.cpp"
#include "VLSubSVec.cpp"

#include "VLMat.cpp"
#include "VLSparseMat.cpp"
#include "VLSubMat.cpp"
#include "VLSubSMat.cpp"

#include "VLVol.cpp"

#include "VLSolve.cpp"
#include "VLFactor.cpp"

#ifndef VL_NO_IOSTREAM
    #include "VLStream.cpp"
#endif
