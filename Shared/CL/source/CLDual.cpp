//
//  File:       CLDual.cpp
//
//  Function:   Dual number implementation
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#include <CLDual.h>

using namespace nCL;

namespace
{
    void ShowExpr(const char* expr, cDual c)
    {
        printf("%s = { %g, %g }\n", expr, c.mReal, c.mDual);
    }
}

void TestDual(float r)
{
    cDual x(r, 1.0);

    ShowExpr("x * 3", x * 3.0f);
    ShowExpr("x / sin(x * 3)", x / sin(x * 3.0));

    ShowExpr("sqrt(x)", sqrt(x));
    ShowExpr("sqr (x)", sqr (x));
    ShowExpr("exp (x)", exp (x));
    ShowExpr("log (x)", log (x));

    ShowExpr("sin (x)", sin (x));
    ShowExpr("cos (x)", cos (x));
    ShowExpr("tan (x)", tan (x));

    ShowExpr("asin(x)", asin(x));
    ShowExpr("acos(x)", acos(x));
    ShowExpr("atan(x)", atan(x));

    ShowExpr("3x^2 + 2x + 1", sqr(x) * 3.0 + x * 2.0 + cDual(1));
}

