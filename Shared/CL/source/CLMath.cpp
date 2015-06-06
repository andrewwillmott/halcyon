//
//  File:       CLMath.cpp
//
//  Function:   Math utils
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLMath.h>

namespace
{

}

nCL::cSinCosTable nCL::sSinCosTable;

nCL::cSinCosTable::cSinCosTable()
{
    for (int i = 0; i < kSize; i++)
    {
        float theta = i * kSinCosInvScale;
        mTable[2 * i + 0] = sin(theta);
        mTable[2 * i + 1] = cos(theta);
    }
}
