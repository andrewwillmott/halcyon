/*
    File:           VLc.h

    Function:       Master header for a version of the VL library based on complex numbers.
                    The various classes are named Vecc, Mat3c, SparseVecc, etc.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */


#ifndef VLc_H
#define VLc_H

#ifdef VL_H
#include "VLUndef.h"
#endif

#define VL_V_REAL double_complex
#define VL_V_SUFF(X) X ## c
#define VL_M_REAL double_complex
#define VL_M_SUFF(X) X ## c

#include "VL.h"
#ifndef VL_COMPLEX
#error must define this in VLConfig.h to use VLc.h
#endif

inline Real len(double_complex c)
{ return sqrt(sqr(real(c)) + sqr(imag(c))); }
inline Real sqrlen(double_complex c)
{ return sqr(real(c)) + sqr(imag(c)); }
inline void SetReal(double_complex &a, Real b)
{ a = b; }

inline bool operator < (double_complex a, double_complex b)
{ return len(a) < len(b); }
inline bool operator > (double_complex a, double_complex b)
{ return len(a) > len(b); }

#include "VLVec2.h"
#include "VLVec3.h"
#include "VLVec4.h"
#include "VLVec.h"
#include "VLSparseVec.h"

#include "VLMat2.h"
#include "VLMat3.h"
#include "VLMat4.h"
#include "VLMat.h"
#include "VLSparseMat.h"
#include "VLSolve.h"
#include "VLTransform.h"

#endif
