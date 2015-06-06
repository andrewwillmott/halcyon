/*
    File:       CLExpr.cpp
 
    Function:   
 
    Authors:    Paul Heckbert, Andrew Willmott
 
    Notes:      
*/

#include <CLExpr.h>

#include <stdarg.h>

typedef const char* tCharPtr;

using namespace nCL;

namespace
{
    const double kPi = 3.14159265358979323846;
    inline double RadiansFromDegrees(double degs)
    { return degs * (kPi / 180); }
    inline double DegreesFromRadians(double rads)
    { return rads * (180 / kPi); }

    double EvalExpression(tCharPtr& s);
}


// ----------------------------------------------------------------------------
// Numerical expression parser
//

// Based on code originally from Paul Heckbert's argparse

// Expression   -> Term [+|- Term]*
// Term         -> Factor [*/% Factor]*
// Factor       -> SignedNumber [^ Factor]
// SignedNumber ->    + SignedNumber
//                  | - SignedNumber
//                  | Number
//
// Number       ->    Function ( Expression )
//                  | Function ( Expression, Expression )
//                  | {. | digit} PositiveConstant




// ----------------------------------------------------------------------------
// cError

double nCL::EvalExpressionDouble(tStrConst sIn)
{
    const char* s = sIn;
    
    if (*s == 0)
        CL_ERROR("empty expression");
    
    double result = EvalExpression(s);
    
    if (*s != 0)
        // we didn't get to the end of the string.
        CL_ERROR("Garbage at end of expression");
    
    return result;
}

int32_t nCL::EvalExpressionSInt32(tStrConst sIn)
{
    const char* s = sIn;
    
    if (*s == 0)
        CL_ERROR("empty expression");
    
    double result = EvalExpression(s);
    
    if (*s != 0)
        // we didn't get to the end of the string.
        CL_ERROR("Garbage at end of expression");
    
    return int32_t(result);
}

namespace
{
    double EvalTerm(tCharPtr& s);
    double EvalFactor(tCharPtr& s);
    double EvalSignedNumber(tCharPtr& s);
    double EvalNumber(tCharPtr& s);
    double EvalPositiveConstant(tCharPtr& s);
    void   EvalCharConst(tCharPtr& s, char c);
    
    double EvalParentheses(tCharPtr& s);
    void   EvalParentheses2(tCharPtr& s, double& arg1, double& arg2);
    
    inline void DiscardWhiteSpace(tCharPtr& s)
    {
        while (isspace(*s))
        {
            s++;
        }
    };
    
    double EvalExpression(tCharPtr& s)
    {
        double result = EvalTerm(s);
    
        while (1)
        {
            DiscardWhiteSpace(s);
    
            switch (*s)
            {
            case '+':
                s++;
                result += EvalTerm(s);
                break;
            case '-':
                s++;
                result -= EvalTerm(s);
                break;
            default:
                return result;
            }
        }
    }
    
    double EvalTerm(tCharPtr& s)
    {
        double result = EvalFactor(s);
    
        while (true)
        {
            DiscardWhiteSpace(s);
    
            switch (*s)
            {
            case '*':
                s++;
                result *= EvalFactor(s);
                break;
            case '/':
                s++;
                result /= EvalFactor(s);
                break;
            case '%':
                {
                    s++;
                    double mod = EvalFactor(s);
                    result = result - floor(result / mod) * mod;
                }
                break;
            default:
                return result;
            }
        }
    }
    
    double EvalFactor(tCharPtr& s)
    {
        double result = EvalSignedNumber(s);
    
        DiscardWhiteSpace(s);
    
        if (*s == '^')
        {
            s++;
            return pow(result, EvalFactor(s));    // right-associative
        }
    
    
        return result;
    }
    
    
    double EvalSignedNumber(tCharPtr& s)
    {
        DiscardWhiteSpace(s);
    
        switch (*s)
        {
        case '-':
            s++;
            return -EvalSignedNumber(s);
        case '+':
            s++;
            return EvalSignedNumber(s);
        }
    
        return EvalNumber(s);
    }
    
    inline bool IsIdentifier(char c)
    {
        return isalnum(c) || c == '_';
    }
    
    double EvalNumber(tCharPtr& s)
    {
        DiscardWhiteSpace(s);
    
        if (isdigit(*s) || *s == '.')
            return EvalPositiveConstant(s);

        if (*s == '(')
            return EvalParentheses(s);
    
        if (isalpha(*s))
        {
            tCharPtr start = s;
    
            for (s++; IsIdentifier(*s); s++)
                ;
    
            tString varName(start, s - start);
    
            if (varName == "pi")       return kPi;
            if (varName == "e")        return exp(1.0);
    
            if (varName == "sqrt")     return sqrt(EvalParentheses(s));
            if (varName == "exp")      return exp (EvalParentheses(s));
            if (varName == "log")      return log (EvalParentheses(s));
            if (varName == "abs")      return fabs(EvalParentheses(s));
    
            if (varName == "sin")      return sin (EvalParentheses(s));
            if (varName == "cos")      return cos (EvalParentheses(s));
            if (varName == "tan")      return tan (EvalParentheses(s));
            if (varName == "asin")     return asin(EvalParentheses(s));
            if (varName == "acos")     return acos(EvalParentheses(s));
            if (varName == "atan")     return atan(EvalParentheses(s));
    
            if (varName == "sind")     return sin (RadiansFromDegrees(EvalParentheses(s)));
            if (varName == "cosd")     return cos (RadiansFromDegrees(EvalParentheses(s)));
            if (varName == "tand")     return tan (RadiansFromDegrees(EvalParentheses(s)));
    
            if (varName == "dasin")    return DegreesFromRadians(asin(EvalParentheses(s)));
            if (varName == "dacos")    return DegreesFromRadians(acos(EvalParentheses(s)));
            if (varName == "datan")    return DegreesFromRadians(atan(EvalParentheses(s)));
    
            if (varName == "floor")    return floor(EvalParentheses(s));
            if (varName == "ceil")     return ceil (EvalParentheses(s));
    
            if (varName == "sqr")
            {
                double x = EvalParentheses(s);
                return x * x;
            }
    
            if (varName == "pow")
            {
                double x, y;
                EvalParentheses2(s, x, y);
                return pow(x, y);
            }
    
            if (varName == "atan2")
            {
                double x, y;
                EvalParentheses2(s, x, y);
                return atan2(x, y);
            }
    
            if (varName == "datan2")
            {
                double x, y;
                EvalParentheses2(s, x, y);
                return DegreesFromRadians(atan2(x, y));
            }
    
            CL_ERROR("Unknown function '%s'", varName.c_str());
        }
    
        CL_ERROR("Bad numerical expression");
        return 0.0;
    }
    
    double EvalParentheses(tCharPtr& s)
    {
        double result;
    
        EvalCharConst(s, '(');
        result = EvalExpression(s);
        EvalCharConst(s, ')');
    
        return result;
    }
    
    void EvalParentheses2(tCharPtr& s, double& arg1, double& arg2)
    {
        EvalCharConst(s, '(');
        arg1 = EvalExpression(s);
        EvalCharConst(s, ',');
        arg2 = EvalExpression(s);
        EvalCharConst(s, ')');
    }
    
    double EvalPositiveConstant(tCharPtr& s)
    {
        char* endS;
    
        double result;
        if (s[0] == '0' && s[1] == 'x') // have to handle hex numbers differently.
        {
            uint32_t resultx = strtoul(s, &endS, 0);
            result = resultx;
        }
        else
            result = strtod(s, &endS);
    
        s = endS;
    
        return result;
    }
    
    void EvalCharConst(tCharPtr& s, char c)
    {
        DiscardWhiteSpace(s);
    
        if (*s != c)
        {
            CL_ERROR("Expected '%c'");
            //throw cError("Expected '%c'", c);
        }
    
        s++;
    }
}
