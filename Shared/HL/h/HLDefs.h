//
//  File:       HLDefs.h
//
//  Function:   Basic shared definitions for the Halcyon library
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_DEFS_H
#define HL_DEFS_H

#include <CLDefs.h>
#include <CLTag.h>

// Commonly used nCL classes we import into nHL

class Vec3f;
namespace nCL
{
    class cLinkable;
    class cAllocatable;
    class cAllocLinkable;

    class cBounds2;
    class cBounds3;
    template<class T> class cLink;
    class cSlotRef;
    class cTransform;
    class cValue;
    class cObjectValue;

    class cIAllocator;
    template<class T> class cAllocPtr;

    class cColour;
    class cColourAlpha;

    class cFileSpec;
}

namespace nHL
{
    using nCL::cLinkable;
    using nCL::cAllocatable;
    using nCL::cAllocLinkable;

    using nCL::cBounds2;
    using nCL::cBounds3;
    using nCL::cLink;
    using nCL::cSlotRef;
    using nCL::cTransform;
    using nCL::cValue;
    using nCL::cObjectValue;

    using nCL::cIAllocator;
    using nCL::cAllocPtr;

    using nCL::cColour;
    using nCL::cColourAlpha;

    using nCL::cFileSpec;

    using nCL::tTag;
    using nCL::tTagID;
    using nCL::kNullTag;
}

#endif
