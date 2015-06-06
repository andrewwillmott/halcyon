#include <CLExpr.h>
#include <CLArgSpec.h>


int main(int argc, char **argv)
{
    cArgSpec argSpec;
    const tChar* expr;
    
    argSpec.ConstructSpec
    (

        "Expression evaluator",
        "<expression:cstring>", &expr, 
            "expression to evaluate. "
            "Example: ev \"1 + 2 * 3\"",
        0
    );
      
    if (argSpec.Parse(argc, argv) != kSpecNoError)
    {
        printf("%s\n", argSpec.ResultString());
        return -1;
    }
    
    double x;
    try
    {
        x = EvalExpressionReal64(expr);
    }
    catch (const cError& err)
    {
        printf("Error: %s\n", err.Description());
        return -1;
    }

    printf("%g\n", x);

    return 0;
}
