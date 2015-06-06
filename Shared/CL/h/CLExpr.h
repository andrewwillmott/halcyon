//
//  File:       CLExpr.h
//
//  Function:   Basic expression evaluator
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1998-2008
//

#ifndef CL_EXPRESSION_H
#define CL_EXPRESSION_H

#include <CLString.h>

namespace nCL
{   
   int32_t EvalExpressionSInt32(tStrConst s);
   double  EvalExpressionDouble(tStrConst s);
}

#endif
