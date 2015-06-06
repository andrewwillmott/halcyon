/*
    File:       CLHeap.cpp

    Function:   

    Author:     Andrew Willmott

    Notes:      
*/

#include <CLHeap.h>

using namespace nCL;

cHeapEntry* cHeap::RemoveMax()
{
    if (mHeap.empty() == 0)
        return 0;
        
    cHeapEntry* result = mHeap[0];
    result->mIndex = -1;

    mHeap[0] = mHeap.back();
    mHeap[0]->mIndex = 0;
    mHeap.pop_back();

    HeapifyDown(0);

    return result;
}

void cHeap::Insert(cHeapEntry* he)
{   
    mHeap.push_back(he);
    he->mIndex = mHeap.size() - 1;

    HeapifyUp(he->mIndex);
}
    
void cHeap::Delete(cHeapEntry* he)
{   
    CL_ASSERT_MSG(he->mIndex >= 0, "Heap entry already deleted");

    if (he->mIndex < 0)
        return;
        
    mHeap[he->mIndex] = mHeap.back();
    mHeap[he->mIndex]->mIndex = he->mIndex;
    mHeap.pop_back();
    
    HeapifyDown(he->mIndex);

    he->mIndex = -1;
}
    
void cHeap::Update(cHeapEntry* he)
{   
    int     i = he->mIndex;
    
    CL_ASSERT_MSG(i >= 0, "Can't update deleted entry");

    if (i > 0 && mHeap[Parent(i)]->mCost < he->mCost)
        HeapifyUp(i);
    else
        HeapifyDown(i);
}
    
void cHeap::HeapifyDown(int i)
{
    int parent = i;
    int child = Left(i);

    while (child < mHeap.size())
    {
        if (child + 1 < mHeap.size() && mHeap[child]->mCost < mHeap[child + 1]->mCost)
            child++;

        if (mHeap[child]->mCost > mHeap[parent]->mCost)
        {
            swap<int>(mHeap[child]->mIndex, mHeap[parent]->mIndex);
            swap<cHeapEntry*>(mHeap[child], mHeap[parent]);

            parent = child;
            child = Left(parent);
        }
        else
            break;
    }
}

void cHeap::HeapifyUp(int i)
{
    int     child, parent;
    
    child = i;
    parent = Parent(child);

    while (parent >= 0)
    {
        if (mHeap[parent]->mCost < mHeap[child]->mCost)
        {
            swap<int>(mHeap[child]->mIndex, mHeap[parent]->mIndex);
            swap<cHeapEntry*>(mHeap[child], mHeap[parent]);

            child = parent;
            parent = Parent(child);
        }
        else
            break;
    }
}
