//
//  File:       CLTag.cpp
//
//  Function:   Provides definition management of tags -- app-wide unique identifiers.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLTag.h>

#include <CLMemory.h>
#include <CLSTL.h>
#include <CLString.h>

using namespace nCL;

/*
    In a debug/dev runtime, a tag is a constant char*, for ease of debugging.
    In a ship runtime, it's the IDFromString() value of the string contents.


    conat tTag kTagWombat = CL_TAG("wombat");
  
    
    We also provide unique string storage support, e.g., for cValue
    fields. At the moment such strings are never freed, to allow
    reuse, though that could potentially found.
    
    Want these to be integrated to share storage.
*/

namespace
{
    struct cLessString
    {
        bool operator()(const char* a, const char* b) const
        {
            return strcmp(a, b) < 0;
        }
    };


    class cTagManager :
        public cAllocatable
    {
    public:
        cTagManager()
        {}

        void Init(cIAllocator* alloc);
        void Shutdown();

    #if CL_TAG_DEBUG
        map<tTagID, tTag> mIDToTagMap;
        vector<tTag> mAllocatedTags;
    #endif

        set<tStringID, cLessString> mStringIDs;

        cIAllocator* mTagAllocator = 0;
        cIAllocator* mStringAllocator = 0;
    };

    void cTagManager::Init(cIAllocator* alloc)
    {
        mTagAllocator = alloc;
        mStringAllocator = alloc;
    }
    void cTagManager::Shutdown()
    {
    #if CL_TAG_DEBUG
        for (auto a : mAllocatedTags)
            mTagAllocator->Free((void*) a);

        mAllocatedTags.clear();
    #endif

        for (auto a : mStringIDs)
            mStringAllocator->Free((void*) a);

        mStringIDs.clear();

        mTagAllocator = 0;
        mStringAllocator = 0;
    }



    cTagManager* sTagManager = 0;

#if CL_TAG_DEBUG
    // Use C approach to avoid initialiser issues.
    struct cInitTagBlock
    {
        tTag            mTags[1022];
        tTag*           mCursor;
        cInitTagBlock*  mNext;
    };

    cInitTagBlock* NewInitTagBlock()
    {
        cInitTagBlock* result = (cInitTagBlock*) calloc(1, sizeof(cInitTagBlock));
        result->mCursor = result->mTags;
        return result;
    }

    cInitTagBlock* sInitTagBlocks = 0;
#endif
}


#if CL_TAG_DEBUG

tTag nCL::TagFromStrConst(const char* s)
{
    if (!sTagManager)
    {
        if (!sInitTagBlocks)
            sInitTagBlocks = NewInitTagBlock();
        else if (sInitTagBlocks->mCursor == sInitTagBlocks->mTags + CL_SIZE(sInitTagBlocks->mTags))
        {
            cInitTagBlock* oldBlock = sInitTagBlocks;
            sInitTagBlocks = NewInitTagBlock();
            sInitTagBlocks->mNext = oldBlock;
        }

        *sInitTagBlocks->mCursor++ = s;

        return s;
    }

    // If we exist as a tag, we're done.
    // If there's an existing string (runtime?) that we match, we must resolve somehow
    // If we want to have a cheap CL_TAG("blah"), resolve in favour of the cstring
    // of course, c strings are case sensitive, so need to account for that.
    // also, some C compilers may not have a global string table.
    // also, if a runtime string was registered first, we're screwed! Ugh.

    tTagID tagID = tTagID(NameToID(s));

    auto it = sTagManager->mIDToTagMap.find(tagID);
    
    if (it != sTagManager->mIDToTagMap.end())
    {
        if (false && s != it->second)   // currently allow this...
        {
            CL_ERROR("Non-constant string tag already registered for %s (%p vs %p)\n", s, s, it->second);
        }
        else if (!eqi(s, it->second))
        {
            CL_ERROR("ID collision for tags %s : %s\n", s, it->second);
        }

        return it->second;
    }

    sTagManager->mIDToTagMap[tagID] = s;

    return s;
}

tTag nCL::TagFromString(const char* s)
{
    tTagID tagID = tTagID(NameToID(s));

    auto it = sTagManager->mIDToTagMap.find(tagID);
    
    if (it != sTagManager->mIDToTagMap.end())
    {
        if (!eqi(s, it->second))
        {
            CL_ERROR("ID collision for tags %s : %s\n", s, it->second);
        }

        return it->second;
    }

    size_t debugStringSize = strlen(s);
    char* p = (char*) sTagManager->mTagAllocator->Alloc(debugStringSize + 1, 1);

    strcpy(p, s);

    sTagManager->mIDToTagMap[tagID] = p;
    sTagManager->mAllocatedTags.push_back(p);

    return p;
}

tTag nCL::TagFromString(const char* begin, const char* end)
{
    tTagID tagID = tTagID(NameToID(begin, end));

    auto it = sTagManager->mIDToTagMap.find(tagID);
    
    if (it != sTagManager->mIDToTagMap.end())
    {
        if (strncasecmp(it->second, begin, end - begin))
        {
            CL_ERROR("ID collision for tags %s : %s\n", begin, it->second);
        }

        return it->second;
    }

    char* p = (char*) sTagManager->mTagAllocator->Alloc(end - begin + 1, 1);

    strcpy(p, begin);

    sTagManager->mIDToTagMap[tagID] = p;
    sTagManager->mAllocatedTags.push_back(p);

    return p;
}

bool nCL::IsTag(tTag tag)
{
    // TODO: check point directly somehow?
    tTagID tagID = tTagID(NameToID(tag));

    auto it = sTagManager->mIDToTagMap.find(tagID);
    
    return it != sTagManager->mIDToTagMap.end() && it->second == tag;
}

#else

tTag nCL::TagFromStrConst(const char* s)
{
    return tTag(NameToID(s));
}

tTag nCL::TagFromString(const char* s)
{
    return tTag(NameToID(s));
}

#endif

tStringID nCL::StringIDFromString(const char* s)
{
    // Reuse tag if possible...
    tTagID tagID = tTagID(NameToID(s));

    auto it = sTagManager->mIDToTagMap.find(tagID);
    
    if (it != sTagManager->mIDToTagMap.end() && eq(s, it->second))  // case is important for strings.
        return it->second;

    auto it2 = sTagManager->mStringIDs.find(s);

    if (it2 != sTagManager->mStringIDs.end())
        return *it2;

    size_t stringSize = strlen(s);
    char* p = (char*) sTagManager->mStringAllocator->Alloc(stringSize + 1, 1);

    strcpy(p, s);

    sTagManager->mStringIDs.insert(p);

    return p;
}

bool nCL::InitTagSystem(cIAllocator* allocator)
{
    CL_ASSERT(!sTagManager);
    sTagManager = new(allocator) cTagManager;
    sTagManager->Init(allocator);

#if CL_TAG_DEBUG
    for (cInitTagBlock* block = sInitTagBlocks; block != 0; )
    {
        for (const tTag* tag = block->mTags; tag != block->mCursor; tag++)
        {
            tTag s = *tag;

            // LOCAL_DEBUG("ctag %p: %s\n", s, s);
            sTagManager->mIDToTagMap[tTagID(NameToID(s))] = s;
        }

        cInitTagBlock* blockToFree = block;
        block = block->mNext;
        free(blockToFree);  // using system allocator
    }
#endif

    return true;
}

bool nCL::ShutdownTagSystem()
{
    sTagManager->Shutdown();
    delete sTagManager;
    sTagManager = 0;
    return true;
}
