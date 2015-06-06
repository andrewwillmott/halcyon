/*
    File:           VLMat2.cpp

    Function:       Implements VLMat2.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott

    Notes:          

*/

#include "VLMat2.h"


bool TMat2::operator == (const TMat2& m) const
{
    return(row[0] == m[0] && row[1] == m[1]);
}

bool TMat2::operator != (const TMat2& m) const
{
    return(row[0] != m[0] || row[1] != m[1]);
}


TMReal det(const TMat2& m)
{
    return(m[0][0] * m[1][1] - m[0][1] * m[1][0]);
}

TMat2 inv(const TMat2& m)
{
    TMReal          mDet;
    TMat2           result;
    
    result[0][0] =  m[1][1]; result[0][1] = -m[0][1]; 
    result[1][0] = -m[1][0]; result[1][1] =  m[0][0]; 
    
    mDet = m[0][0] * result[0][0] + m[0][1] * result[1][0];
    CL_ASSERT_MSG(mDet != 0.0, "(Mat2::inv) matrix is non-singular");
    result /= mDet;
    
    return(result);
}

TMat2 oprod(const TMVec2& a, const TMVec2& b)
// returns outerproduct of a and b:  a * trans(b)
{
    TMat2   result;

    result[0] = a[0] * b;
    result[1] = a[1] * b;

    return(result);
}

TMat2& TMat2::MakeRot(Real theta)  
{
    TMReal  c, s;
    
    SetReal(s, sin(theta));
    SetReal(c, cos(theta));
    
#ifdef VL_ROW_ORIENT
    row[0][0] =  c; row[0][1] = s;
    row[1][0] = -s; row[1][1] = c;
#else
    row[0][0] = c; row[0][1] = -s;
    row[1][0] = s; row[1][1] =  c;
#endif

    return(*this);
}

TMat2& TMat2::MakeScale(const TMVec2& s)
{       
    row[0][0] = s[0]; row[0][1] = vl_0;
    row[1][0] = vl_0; row[1][1] = s[1];
    
    return(*this);
}

