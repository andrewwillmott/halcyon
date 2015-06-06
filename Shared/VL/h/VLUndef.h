/*
    File:           VLUndef.h

    Function:       Undefines the type macros used by VL.h, so that that
                    file can be re-included, usually with different types.
                    See VLfd.h, for example.

    Author(s):      Andrew Willmott

 */

#undef VL_H
#undef VL_VERSION

#undef  TVReal  
#undef  TMReal  

#undef  TVec2
#undef  TMVec2  
#undef  TMat2

#undef  TVec3   
#undef  TMVec3  
#undef  TMat3   

#undef  TVec4   
#undef  TQuaternion
#undef  TMVec4  
#undef  TMat4   

#undef  TVec    
#undef  TMVec   
#undef  TMat    
#undef  TSubVec 
#undef  TMSubVec
#undef  TSubMat 

#undef  TSparseVec
#undef  TSparseMat
#undef  TSubSVec
#undef  TMSubSVec
#undef  TSubSMat

#undef  TSparsePair
#undef  TMSparseVec
#undef  TSVIter 

#undef  TVol

#undef  VL_V_REAL
#undef  VL_M_REAL
#undef  VL_V_SUFF
#undef  VL_M_SUFF

#undef VL_Vec_H
#undef VL_Vec2_H
#undef VL_Vec3_H
#undef VL_Vec4_H
#undef VL_Mat_H
#undef VL_Mat2_H
#undef VL_Mat3_H
#undef VL_Mat4_H
#undef VL_SparseVec_H
#undef VL_SparseMat_H
#undef VL_SubVec_H
#undef VL_SubMat_H
#undef VL_SubSVec_H
#undef VL_SubSMat_H
#undef VL_Vol_H
#undef VL_Solve_H
#undef VL_Stream_H
