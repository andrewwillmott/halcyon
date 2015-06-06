/*
    File:       CLHeap.h

    Function:   Implements a simple binary heap. From Manber's
                  "Introduction to Algorithms."

    Author:     Andrew Willmott

    Copyright:  (c) 2000, Andrew Willmott
*/

#ifndef CL_HEAP_H
#define CL_HEAP_H

#include <CLDefs.h>
#include <CLSTL.h>

namespace nCL
{
    struct cHeapEntry
    {
        float   mCost;
        int     mIndex;
    };

    typedef vector<cHeapEntry*> tHeapEntries;

    class cHeap
    {
    public:

        // operations needed for a priority queue

        void        Insert(cHeapEntry* he);  ///< add he to the heap.
        void        Delete(cHeapEntry* he);  ///< remove he from the heap.
        void        Update(cHeapEntry* he);  ///< signal that he's cost has been updated
        cHeapEntry* RemoveMax();           ///< remove highest cost node.

        int         NumItems()          { return mHeap.size(); };

        void        HeapifyUp  (int i);
        void        HeapifyDown(int i);

        int         Parent(int i)       { return (i - 1) / 2; }
        int         Left  (int i)       { return 2 * i + 1; }
        int         Right (int i)       { return 2 * i + 2; }

        tHeapEntries mHeap;
    };
}

#endif
