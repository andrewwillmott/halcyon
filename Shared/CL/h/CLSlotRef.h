//
//  File:       CLSlotRef.h
//
//  Function:   Weak reference support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#ifndef CL_SLOT_REF_H
#define CL_SLOT_REF_H

#include <CLDefs.h>

namespace nCL
{
    struct cSlotRef
    {
        int32_t  mIndex = -1;
        uint32_t mStamp = ~0;

    #ifdef CL_CHECKING
        uint32_t mParentStamp = ~0;

        cSlotRef() {}
        cSlotRef(int index, uint32_t stamp, uint32_t parentStamp) : mIndex(index), mStamp(stamp), mParentStamp(parentStamp) {}
    #else
        cSlotRef() {}
        cSlotRef(int index, uint32_t stamp) : mIndex(index), mStamp(stamp) {}
    #endif

        bool IsNull() const;

        operator int() const;     ///< Down-convert to index
    };

    const cSlotRef kNullRef;    ///< Null (empty) reference.

    bool operator == (const cSlotRef& a, const cSlotRef& b);
    bool operator != (const cSlotRef& a, const cSlotRef& b);



    // --- Inlines -------------------------------------------------------------

    inline bool cSlotRef::IsNull() const
    {
        return mIndex < 0;
    }

    inline bool operator == (const cSlotRef& a, const cSlotRef& b)
    {
        return a.mIndex == b.mIndex
            && a.mStamp == b.mStamp;
    }
    inline bool operator != (const cSlotRef& a, const cSlotRef& b)
    {
        return a.mIndex != b.mIndex
            || a.mStamp != b.mStamp;
    }

    inline cSlotRef::operator int() const
    {
        return mIndex;
    }
}

#endif
