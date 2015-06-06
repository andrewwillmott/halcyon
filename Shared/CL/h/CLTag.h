//
//  File:       CLTag.h
//
//  Function:   Provides definition management of tags -- app-wide unique identifiers.
//              Also provides unique string storage as part of the same mechanism.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_TAG_H
#define CL_TAG_H

#include <CLString.h>

#ifndef CL_TAG_DEBUG
    #ifndef CL_RELEASE
        #define CL_TAG_DEBUG 1
    #else
        #define CL_TAG_DEBUG 0
    #endif
#endif


namespace nCL
{
    // Declarations
    class cIAllocator;

    // --- Tags ----------------------------------------------------------------
    //
    // A tag is a unique identifier generally used to identify resources in an app.
    //
    // In developer builds, tags are unique persistent string pointers, so while they can
    // be treated as numerical values for the purpose of map-based lookup, they can also
    // be easily inspected in the debugger or used in debug UI.
    //
    // In a shipping build, they're simply IDs, either the hash of the original string name with
    // high bit set, or a UID.

    ///< ID-based version of tag used in persistence
    enum tTagID : uint32_t
    {
        kNullTagID = 0
    };

#if CL_TAG_DEBUG
    typedef const char* tTag;
    const tTag kNullTag = 0;

    #define CL_TAG_FMT "%s"

    #define CL_TAG nCL::TagFromStrConst
#else

    typedef tTagID tTag;
    const tTag kNullTag = kNullTagID;

    #define CL_TAG_FMT "0x%08x"

    #ifndef CL_RELEASE
        #define CL_TAG nCL::TagFromStrConst
    #else
        #define CL_TAG(M_X) nCL::kNullTag
    #endif
#endif

    tTag TagFromStrConst(const char* s);
    ///< Returns tag for a C string constant. You must NOT pass runtime-generated strings to this
    ///< routine, or bad stuff(tm) will happen if their memory is released while still in use by the tag system.
    tTag TagFromString(const char* s);
    ///< Returns tag corresponding to some runtime-created string. (E.g., on the stack or the heap,
    ///< or in a data segment that gets modified at runtime.)
    tTag TagFromString(const char* begin, const char* end);
    ///< TagFromString variant that can be used on a substring.

    const char* StringFromTag(tTag tag);  ///< Returns the name for the given tag.
    tTagID          IDFromTag(tTag tag);

    bool IsTag(tTag tag);
    ///< Returns true if the tag is known to the system.


    // --- String IDs ----------------------------------------------------------
    //
    // String IDs are a convenient way of dealing with strings without worrying about
    // memory persistence. Once created, the string persists for the lifetime of the app,
    // and can be retrieved at any time via its ID.
    //
    // Similar to tags, in a debug/developer build they are a string pointer, so can be
    // quickly translated in the debugger. In a ship build they are a handle to the
    // underlying string.
    //
    // Unlike tags, which are intended to be short and must always have a unique hash if
    // specified by string, string IDs can be used to reference any string.
    //

    typedef tTag tStringID;

    tStringID   StringIDFromString(const char* s);      ///< Returns an ID that can be used to refer to the given string. Warning: this may not be the same as the tTagID.
    const char* StringFromStringID(tStringID id);       ///< Get back the string for the given string ID.

    bool        ReleaseStringID(tStringID id);          ///< Call when a string ID is no longer needed
    tStringID   CopyStringID(tStringID id);             ///< Increments any internal usage counter for the given string ID.

    tTagID IDFromStringID (tStringID id);               ///< Get hash ID for the given string ID.
    tTag   TagFromStringID(tStringID id);

    // System
    bool InitTagSystem(cIAllocator* allocator = 0);
    ///< Call before any calls to the above routines. If allocator is specified,
    ///< it will be used for all associated tag storage.
    bool ShutdownTagSystem();
    ///< Call after all calls to the above routines are done. After this, they
    ///< will return kNullTag.


    // --- Inlines -------------------------------------------------------------

#if CL_TAG_DEBUG
    inline const char* StringFromTag(tTag tag)
    {
        return tag;
    }
    inline tTagID IDFromTag(tTag tag)
    {
        return tag ? tTagID(NameToID(tag)) : kNullTagID;
    }
    inline const char* StringFromStringID(tStringID id)
    {
        return id;
    }
    inline tTagID IDFromStringID (tStringID id)
    {
        return tTagID(NameToID(id));
    }
    inline tTag TagFromStringID(tStringID id)
    {
        return TagFromString(id);
    }
#else
    inline const char* StringFromTag(tTag tag)
    {
        return "<unknown>";
    }
    inline tTagID IDFromTag(tTag tag)
    {
        return tag;
    }
    inline const char* StringFromStringID(tStringID id)
    {
        return "<unknown>"; // TODO: should be storing here?
    }
#endif

    // Current implementation keeps all strings around.
    inline tStringID CopyStringID(tStringID id)
    {
        return id;
    }

    inline bool ReleaseStringID(tStringID id)
    {
        return false;
    }
}


#endif
