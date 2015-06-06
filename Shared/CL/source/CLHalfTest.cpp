#include "CLHalf.h"


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc == 2)
    {
        float a = atof(argv[1]);
        float16 b(a);

        printf("0x%08x, a = %8g, b = %8g, b.i = 0x%04x\n", (uint32_t&) a, a, float(b), b.mAsInt);
        
        return 0;
    }
    
    {
        float a = 1.0f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = -1.0f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = 0.0f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = 1.0f / 0.0f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = -1.0f / 0.0f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }


    {
        float a = 5.96046448e-08f;  // smallest possible *denormalized* f16
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = 6.10351562e-05f;  // smallest possible normal f16
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = 65504.0f; // biggest possible f16
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    
    
    {
        float a = 65503.9f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }
    {
        float a = 65504.1f;
        float16 b(a);

        printf("a = %8g, b = %8g, b.i = 0x%04x\n", a, float(b), b.mAsInt);
    }

    return 0;
}
