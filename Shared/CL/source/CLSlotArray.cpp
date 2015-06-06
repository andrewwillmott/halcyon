//
//  File:       CLSlotArray.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLSlotArray.h>

using namespace nCL;

namespace
{
#ifdef CL_CHECKING
    int sNextArrayStamp = 0;
#endif
}


#ifdef CL_CHECKING
cSlotArray::cSlotArray() :
    mStamps(),
    mNumSlotsInUse(0),
    mFreeSlot(kFreeEnd),
    mNextStamp(0),
    mStamp(++sNextArrayStamp)
{}
#endif

cSlotRef cSlotArray::CreateSlot()
{
    int result;

    if (mFreeSlot == kFreeEnd)
    {
        result = mStamps.size();
        mStamps.push_back(++mNextStamp);
    }
    else
    {
        result = mFreeSlot & ~kFreeBit;
        CL_ASSERT(!InUse(result));

        mFreeSlot = mStamps[result];
        mStamps[result] = ++mNextStamp;

        CL_ASSERT(mFreeSlot == kFreeEnd || !InUse(mFreeSlot & ~kFreeBit));
    }

    mNumSlotsInUse++;

#ifdef CL_CHECKING
    return cSlotRef(result, mStamps[result], mStamp);
#else
    return cSlotRef(result, mStamps[result]);
#endif
}

bool cSlotArray::DestroySlot(cSlotRef r)
{
    if (InUse(r))
    {
        DestroySlot(r.mIndex);
        return true;
    }

    return false;
}

void cSlotArray::DestroySlot(int i)
{
    CL_ASSERT(InUse(i));
    CL_ASSERT(mFreeSlot & kFreeBit);

    mStamps[i] = mFreeSlot;
    mFreeSlot = kFreeBit | i;

    mNumSlotsInUse--;
}

void cSlotArray::InvalidateRefs(int i)
{
    CL_ASSERT((mStamps[i] & kFreeBit) == 0);
    mStamps[i]++;
}

bool cSlotArray::RecreateSlot(int i)
{
    int numSlots = mStamps.size();

    if (i >= numSlots)
    {
        if (i > numSlots) // assume we only ever increment by one. If not, we'd have to handle setting intermediate slots to kFreeBit + nextSlot
            return false;

        mStamps.resize(i + 1, 0);
        return true;
    }

    if (InUse(i))
        return false;

    mStamps[i] = ++mNextStamp;
    mNumSlotsInUse++;

    // TODO: we'll need some kind of routine to rebuild the embedded free list,
    // as this operation will break the pointers. Basically scan linearly &
    // reset mStamp[] for still-free slots.

    return true;
}

void cSlotArray::ClearSlots()
{
    mStamps.clear();
    mNumSlotsInUse = 0;
    mFreeSlot = -1;
}

#ifdef CL_TEST_SLOTARRAY
void TestSlotArray()
{
    cSlotArray v;

    cSlotRef r[4];

    r[0] = v.CreateSlot();
    r[1] = v.CreateSlot();
    v.DestroySlot(r[0]);
    r[2] = v.CreateSlot();
    r[3] = v.CreateSlot();

    printf("numSlots = %d, numSlotsInUse = %d\n", v.NumSlots(), v.NumSlotsInUse());
    for (int i = 0; i < CL_SIZE(r); i++)
    {
        int ri = v.IndexFromRef(r[i]);

        printf("r[%d] = %d\n", i, ri);
    }
}
#endif
