//
//  File:       EV.cpp
//
//  Function:   Command-line access to expression evaluator
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//


#include <CLExpr.h>

#include <CLArgSpec.h>

using namespace nCL;

namespace nCL
{
    void Log(char const* group, char const* format, ...)
    {
        va_list args;
        va_start(args, format);
        printf("%s: ", group);
        vprintf(format, args);
        va_end(args);
    }
}

int main(int argc, char const** argv)
{
#ifdef CL_ARG_SPEC_H
    enum tArgFlags
    {
        kFlagHex,
        kFlagInteger,
        kMaxFlags
    };
    cArgSpec argSpec;
    const char* expr = 0;

    tArgSpecError argSpecErr = argSpec.ConstructSpec
    (
        "Expression evaluator",
        "<expression:cstring>", &expr, 
            "Evaluate the given expression. "
            "Example: ev \"1 + 2 * 3\"",
        "-i^", kFlagInteger,
            "Show result as an integer",
        "-x^", kFlagHex,
            "Show result as a hex number",
        0
    );

    CL_ASSERT(argSpecErr == kSpecNoError);

    if (argc <= 1)
    {
        printf("%s\n", argSpec.HelpString(argv[0]));
        return 0;
    }

    if (argSpec.Parse(argc, argv) != kArgNoError)
    {
        printf("%s\n", argSpec.ResultString());
        return -1;
    }

    double result = EvalExpressionDouble(expr);

    if (argSpec.Flag(kFlagHex))
        printf("0x%08x\n", int(result));
    else if (argSpec.Flag(kFlagInteger))
        printf("%d\n", int(floor(result)));
    else
        printf("%g\n", result);

#else
    if (argc != 2)
    {
        printf("Syntax: %s <expression>\n", argv[0]);
        return -1;
    }

    double x = EvalExpressionDouble(argv[1]);

    printf("%g\n", x);
#endif

    return 0;
}
