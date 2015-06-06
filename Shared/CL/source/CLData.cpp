//
//  File:       CLData.cpp
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLData.h>

using namespace nCL;

namespace
{

}


uint8_t* cWriteableDataStore::Data(tDataOffset d)
{
    if (d == kNullDataOffset)
        return 0;
    return mData.data() + d;
}

const uint8_t* cWriteableDataStore::Data(tDataOffset d) const
{
    if (d == kNullDataOffset)
        return 0;
    return mData.data() + d;
}

tDataOffset cWriteableDataStore::Allocate(size_t size, int alignment)
{
    if (alignment == 0)
        alignment = 4;

    int alignM = alignment - 1;
    CL_ASSERT((alignment & alignM) == 0);  // power of two alignment expected

    tDataOffset offset = mData.size();
    offset = (offset + alignM) & ~alignM;
    mData.resize(offset + size, 0);

    return offset;
}

void cWriteableDataStore::Free(tDataOffset d)
{
    mData.resize(d);
}


cReadOnlyDataStore::cReadOnlyDataStore(const uint8_t* data) : mData(data)
{
}

uint8_t* cReadOnlyDataStore::Data(tDataOffset d)
{
    return 0;
}

const uint8_t* cReadOnlyDataStore::Data(tDataOffset d) const
{
    return mData + d;
}

tDataOffset cReadOnlyDataStore::Allocate(size_t size, int alignment)
{
    CL_ERROR("Read-only store");
    return kNullDataOffset;
}

void cReadOnlyDataStore::Free(tDataOffset d)
{
    CL_ERROR("Unimplemented");
}
