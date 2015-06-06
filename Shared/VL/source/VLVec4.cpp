/*
    File:           VLVec4.cpp

    Function:       Implements VLVec4.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/


#include "VLVec4.h"


TVec4& TVec4::MakeUnit(int n, TVReal k)
{
    if (n == 0)
    { elt[0] = k; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = vl_zero; }
    else if (n == 1)
    { elt[0] = vl_zero; elt[1] = k; elt[2] = vl_zero; elt[3] = vl_zero; }
    else if (n == 2)
    { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = k; elt[3] = vl_zero; }
    else if (n == 3)
    { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = k; }
    else 
        CL_ERROR("(Vec4::MakeUnit) illegal unit vector");

    return SELF;
}

bool TVec4::operator == (const TVec4& a) const
{
    return elt[0] == a[0] && elt[1] == a[1] && elt[2] == a[2] && elt[3] == a[3];
}

bool TVec4::operator != (const TVec4& a) const
{
    return elt[0] != a[0] || elt[1] != a[1] || elt[2] != a[2] || elt[3] != a[3];
}

TVec4 cross(const TVec4& a, const TVec4& b, const TVec4& c) 
{
    TVec4 result;
    // XXX can this be improved? Look at assembly.
#define ROW(i)       a[i], b[i], c[i]
#define DET(i,j,k)   dot(TVec3(ROW(i)), cross(TVec3(ROW(j)), TVec3(ROW(k))))
    
    result[0] =  DET(1,2,3);
    result[1] = -DET(0,2,3);
    result[2] =  DET(0,1,3);
    result[3] = -DET(0,1,2);
    
    return result;

#undef ROW
#undef DET
}

TVec3 proj(const TVec4& v)       
{
    TVec3 result;
    
    CL_ASSERT_MSG(v[3] != 0, "(Vec4/proj) last elt. is zero");
    
    result[0] = v[0] / v[3];
    result[1] = v[1] / v[3];
    result[2] = v[2] / v[3];
    
    return result;
}
