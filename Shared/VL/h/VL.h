/*
    File:           VL.h

    Function:       Header file for the VL library. Provides macro definitions
                    so that the library can be templated by type.
                    
    Author(s):      Andrew Willmott

    Version         2.0.0 WIP

    Copyright:      (c) 1995-2000, 2013, Andrew Willmott
 */

#ifndef VL_H
#define VL_H

#define VL_VERSION "2.0.0"
#define VL_VER_NUM 20000

/*
    This header file is included before any other when compiling VL. Its
    purpose is to translate the types that VL is templated on (TReal, TVec2,
    TVec3, etc.), replacing them with real types. In this way the same
    code can be used to compile float-based, double-based, and other
    type-based versions of the vector and matrix routines.

    VL.h expects VL_V_REAL and VL_M_REAL to be set to the element types for
    vectors and matrices respectively, and the VL_V_SUFF and VL_M_SUFF
    macros to append a suffix indicating the vector or matrix type to their
    arguments. If these macros are not defined, vectors and matrices will
    be based on the type 'Real', and there will be no suffixes.

    See VLf.h, and VLfd.h for examples of how this facility is used.

    The VL library used to use C++ templates rather than the current cpp
    macros. The state of both the C++ languange and its compilers made this
    unworkable, however, especially when targetting more than one compiler.
    
    Update: while the state of C++ template support has much improved, some
    common compilers (e.g. MSVC) do not handle templating and inlining at
    all well; using templates would incur a call-overhead penalty. One day!
*/

#include <VLConfig.h>

#ifdef VL_NO_CL
    #include <VLDefs.h>
#else
    #include <CLDefs.h>
#endif

#include "VLConstants.h"
#include "VLMath.h"

#ifndef VL_V_REAL
    #define VL_V_REAL Real
    #define VL_M_REAL Real
#endif

#ifndef VL_V_SUFF
    #define VL_V_SUFF(X) X
    #define VL_M_SUFF(X) X
#endif

#ifndef VL_M_REAL
    #define VL_M_REAL VL_V_REAL
    #define VL_M_SUFF(X) VL_V_SUFF(X)
#endif

#define TVReal      VL_V_REAL
#define TMReal      VL_M_REAL

#define TVec2       VL_V_SUFF(Vec2)
#define TMVec2      VL_M_SUFF(Vec2)
#define TMat2       VL_M_SUFF(Mat2)

#define TVec3       VL_V_SUFF(Vec3)
#define TMVec3      VL_M_SUFF(Vec3)
#define TMat3       VL_M_SUFF(Mat3)

#define TVec4       VL_V_SUFF(Vec4)
#define TQuaternion VL_M_SUFF(Vec4)
#define TMVec4      VL_M_SUFF(Vec4)
#define TMat4       VL_M_SUFF(Mat4)

#define TVec        VL_V_SUFF(Vec)
#define TRefVec     VL_M_SUFF(XRefVec)
#define TMVec       VL_M_SUFF(Vec)
#define TMat        VL_M_SUFF(Mat)
#define TRefMat     VL_M_SUFF(XRefMat)
#define TSubVec     VL_V_SUFF(SubVec)
#define TMSubVec    VL_M_SUFF(SubVec)
#define TSubMat     VL_M_SUFF(SubMat)

#define TSparseVec  VL_V_SUFF(SparseVec)
#define TSparseMat  VL_M_SUFF(SparseMat)
#define TSubSVec    VL_V_SUFF(SubSVec)
#define TMSubSVec   VL_M_SUFF(SubSVec)
#define TSubSMat    VL_M_SUFF(SubSMat)

#define TSparsePair VL_V_SUFF(SparsePair)
#define TMSparseVec VL_M_SUFF(SparseVec)
#define TSVIter     VL_V_SUFF(SVIter)
#define TMSVIter    VL_M_SUFF(SVIter)

#define TVol        VL_M_SUFF(Vol)

#endif  
