//
//  File:       CLParams.cpp
//
//  Function:   Support for a set of parameters
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLParams.h>

using namespace nCL;

namespace
{

}

void cParams::SetParamBase(int ref, int dataSize, const void* data)
{
    if (ref >= mParams.size())
        mParams.resize(ref + 1);

    CL_ASSERT((dataSize & 3) == 0); // require uint32_t multiple

    cParamInfo& info = mParams[ref];

    if (info.mOffset == kNullDataOffset || info.mSize != dataSize)
    {
        info.mOffset = mParamsStore.Allocate(dataSize, 4);
        info.mSize = dataSize;
    }

    void* dataDest = mParamsStore.Data(info.mOffset);
    memcpy(dataDest, data, dataSize);

    info.mModCount++;
    mModCount++;
}
