/*
    File:       LibVLfd.cpp
    
    Purpose:    Compiles mixed float/double code.
    
    Author:     Andrew Willmott

*/


#include "VLd.h"

#include "VLUndef.h"
#define VL_V_REAL float
#define VL_V_SUFF(X) X ## f
#define VL_M_REAL double
#define VL_M_SUFF(X) X ## d

#include "VLVec2.h"
#include "VLVec3.h"
#include "VLVec4.h"
#include "VLVec.h"
#include "VLSparseVec.h"

#define VL_MIXED
#include "VLMixed.cpp"

#include "VLSolve.cpp"
#include "VLFactor.cpp"
