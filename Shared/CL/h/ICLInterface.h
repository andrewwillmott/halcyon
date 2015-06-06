/*
    File:       ICLInterface.h

    Function:	Defines mechanism for querying an object for different interfaces.

    Author:     Andrew Willmott

    Copyright:  (c) 2002, Andrew Willmott
*/

#ifndef CL_IINTERFACE_H
#define CL_IINTERFACE_H

#include <CLDefs.h>


namespace nCL
{
    typedef uint32_t tIID; // interface ID.

    class cIInterface
    {
    public:
        enum tLocalIID : tIID { kIID = 0x00000000 };      // New interfaces should have a new CLID() defined like so.

        virtual void* AsInterface(tIID iid) const = 0;  ///< Returns pointer if the interface exists, or zero otherwise.
        virtual int   Link(int count) const = 0;                      ///< Allow reference counting -- see CLLink.h
    };


    // Syntactic sugar, e.g., cIBob* bob = AsInterface(otherClass);
    // Note that otherClass doesn't have to inherit from cIInterface, it
    // just needs to define the same AsInterface() method.
    template<class T_D, class T_S> const T_D* AsInterface(const T_S* interface)
    {
        return static_cast<const T_D*>(const_cast<T_S*>(interface)->AsInterface(T_D::kIID));
    }
    template<class T_D, class T_S> T_D* AsInterface(T_S* interface)
    {
        return static_cast<T_D*>(interface->AsInterface(T_D::kIID));
    }
}


#endif
