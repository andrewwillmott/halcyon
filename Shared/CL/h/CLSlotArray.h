//
//  File:       CLSlotArray.h
//
//  Function:   Implements handle-based allocation, similar to file descriptors or GL names
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_SLOT_ARRAY_H
#define CL_SLOT_ARRAY_H

#include <CLSlotRef.h>
#include <CLSTL.h>

namespace nCL
{
    struct cSlotArray
    {
        enum { kFreeBit = 0x80000000, kFreeEnd = 0xFFFFFFFF };
        
        vector<uint32_t> mStamps;   // stamp if high bit not set, or, high bit set and next free slot if any.
        int      mNumSlotsInUse;
        int      mFreeSlot;     ///< head of free slot chain
        uint32_t mNextStamp;    ///< next stamp to allocate

#ifdef CL_CHECKING
        uint32_t mStamp;        ///< Array stamp for this -- purely for checking refs are used with the right parent arrays
#endif

        cSlotArray();

        int  NumSlots() const;
        int  NumSlotsInUse() const;
        void ClearSlots();

        cSlotRef CreateSlot();
        bool     DestroySlot(cSlotRef ref);         ///< Returns true if the slot still exists, and destroys it.
        void     DestroySlot(int i);
        bool     RecreateSlot(int i);

        bool     InUse(cSlotRef s) const;
        bool     InUse(int i) const;

        cSlotRef RefFromIndex(int index) const;    ///< Get a reference to the given index.
        int      IndexFromRef(cSlotRef ref) const; ///< Return corresponding slot only if still valid, otherwise -1.

        void     InvalidateRefs(int index);        ///< Invalidate all cSlotRefs for this index
    };

    // Template for single array of slots of type T
    template<class T> class cSlotArrayT : public cSlotArray
    {
    public:
        cSlotRef CreateSlot()                   { cSlotRef result = cSlotArray::CreateSlot();       mSlots.resize(mStamps.size()); return result; }
        bool     RecreateSlot(int slot)         { bool     result = cSlotArray::RecreateSlot(slot); mSlots.resize(mStamps.size()); return result; }

        T& operator[](int i)                { CL_ASSERT(InUse(i)); return mSlots[i]; }
        const T& operator[](int i) const    { CL_ASSERT(InUse(i)); return mSlots[i]; }
        T& at(int i)                        { CL_ASSERT(InUse(i)); return mSlots[i]; }
        const T& at(int i) const            { CL_ASSERT(InUse(i)); return mSlots[i]; }

        T* Slot(cSlotRef ref)               { return InUse(ref) ? &mSlots[ref.mIndex] : 0; }
        const T* Slot(cSlotRef ref) const   { return InUse(ref) ? &mSlots[ref.mIndex] : 0; }

        void ResizeSlots(int n)             { mSlots.resize(n); }
        void ClearSlots()                   { cSlotArray::ClearSlots(); mSlots.clear(); }

    protected:
        vector<T> mSlots;
    };
    

    // --- Inlines -------------------------------------------------------------

#ifndef CL_CHECKING
    inline cSlotArray::cSlotArray() :
        mStamps(),
        mFreeSlot(kFreeEnd),
        mNumSlotsInUse(0),
        mNextStamp(0)
    {}
#endif

    inline int cSlotArray::NumSlots() const
    {
        return mStamps.size();
    }
    inline int cSlotArray::NumSlotsInUse() const
    { 
        return mNumSlotsInUse;
    }

    inline bool cSlotArray::InUse(cSlotRef ref) const
    {
        CL_ASSERT(ref.mParentStamp == ~0 || ref.mParentStamp == mStamp);

        return ref.mIndex >= 0 && ref.mIndex < int(mStamps.size()) && mStamps[ref.mIndex] == ref.mStamp;
    }

    inline bool cSlotArray::InUse(int i) const
    {
        return i >= 0 && i < int(mStamps.size()) && (mStamps[i] & kFreeBit) == 0;
    }

    inline cSlotRef cSlotArray::RefFromIndex(int i) const
    {
        if (InUse(i))
        #ifdef CL_CHECKING
            return cSlotRef(i, mStamps[i], mStamp);
        #else
            return cSlotRef(i, mStamps[i]);
        #endif

        return cSlotRef();
    }

    inline int cSlotArray::IndexFromRef(cSlotRef ref) const
    {
        CL_ASSERT(ref.mParentStamp == ~0 || ref.mParentStamp == mStamp);

        if (InUse(ref))
            return ref.mIndex;
        
        return -1;
    }
}

#endif
