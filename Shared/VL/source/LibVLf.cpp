/*
    File:       LibVLf.cpp
    
    Purpose:    Compiles all code necessary for VLf.h.
    
    Author:     Andrew Willmott
*/


#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f
#define VL_M_REAL float
#define VL_M_SUFF(X) X ## f

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

#ifndef VL_NO_IOSTREAM
    #include "VLStream.cpp"
#endif
