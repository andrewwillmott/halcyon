//
//  File:       CLLink.h
//
//  Function:   Reference counting setup.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_LINK_H
#define CL_LINK_H

#include <CLDefs.h>
#include <libkern/OSAtomic.h>

namespace nCL
{
    //
    // A 'linkable' object is anything that implements:
    //
    //      int Link(int delta);  ///< Add/remove given number of reference links. Returns final link count. Call with 0 to get current count.
    //
    // The link count should start at zero, and the object should self-destroy when it hits zero again.
    //

    class cLinkable
    /// Inherit from this to get single-thread-only link support.
    {
    public:
        virtual ~cLinkable() {}

        int Link(int count) const;

    protected:
        mutable int32_t mLinks = 0;
    };

    #define CL_LINKABLE_DECL  int Link(int count) const override { return nCL::cLinkable::Link(count); }



    class cLinkableAtomic
    /// Inherit from this to get multi-thread-savvy link support.
    {
    public:
        virtual ~cLinkableAtomic() {}

        int Link(int count) const;

    protected:
        mutable volatile int32_t mLinks = 0;
    };


    template<class T> class cLink
    /// Automatically handles linking and unlinking from the referenced object.
    {
    public:
        cLink()                 : mObject(0)           {}
        cLink(T* object)        : mObject(object)      { if (mObject) mObject->Link(1); }
        cLink(const cLink& rhs) : mObject(rhs.mObject) { if (mObject) mObject->Link(1); }

       ~cLink() { if (mObject) mObject->Link(-1); }

        void operator = (T* object)
        {
            if (mObject == object)
                return;

            if (mObject)
                mObject->Link(-1);

            mObject = object;

            if (mObject)
                mObject->Link(1);
        }
        
        void operator = (const cLink& rhs) { operator = (rhs.mObject); }

        T& operator  *() const { return *mObject; }
        T* operator ->() const { return  mObject; }
           operator T*() const { return  mObject; }

        T* Transfer()
        {
            T* result = mObject;
            mObject = 0;
            return result;
        }

    protected:
        T* mObject = 0;
    };
}

inline int nCL::cLinkable::Link(int count) const
{
    if ((mLinks += count) > 0 || count >= 0)
        return mLinks;

    delete this;
    return 0;
}

inline int nCL::cLinkableAtomic::Link(int count) const
{
    int32_t result = OSAtomicAdd32Barrier(count, &mLinks);

    if (result <= 0 && count < 0)
        delete this;

    return result;
}

#endif
