//
//  File:       CLString.h
//
//  Function:   Classes for manipulating strings
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1996-2014, Andrew Willmott
//

#ifndef CL_STRING_H
#define CL_STRING_H

#include <CLDefs.h>
#include <CLHash.h>
#include <CLSTL.h>

namespace nCL
{
    // --- Definitions ---------------------------------------------------------
    
    typedef nCL::string tString;
    typedef const char*  tStrConst;
    typedef nCL::vector<tStrConst> tStrConstArray;


    // --- Utility routines ----------------------------------------------------

    struct cVariableSet
    {
        virtual const char* Variable(const char* varName, size_t varLen = 0) const = 0;
    };

    bool SubstituteVars(const cVariableSet* set, tStrConst s, tString* result);
    ///< Substitute any variables in s according to the given variable set.

    bool SubstituteEnvVars(tStrConst str, tString* result);
    bool SubstituteEnvVars(tString* str);
    ///< Replace environment variables in s (e.g., $HOME) with their
    ///< environment-defined values.

    void Split(tStrConst line, tStrConstArray* a, tStrConst separators = " \t", vector<char>* scratch = 0);
    ///< Splits the given line up into tokens using the given separators.
    ///< If 'scratch' isn't provided for token storage, internal storage will be used, which
    ///< will only be valid until the next call to Split().

    void Sprintf(tString* str, const char* format, ...);
    ///< Formatted string
    void SprintfAppend(tString* str, const char* format, ...);
    ///< Append formatted string

    struct cEnumInfo
    {
        tStrConst mName;
        int32_t   mValue;
    };

    int32_t ParseEnum(const cEnumInfo enumInfo[], tStrConst name, int32_t unknown = -1);
    ///< Returns value from enumInfo for the given name, or 'unknown' if not found.
    const char* EnumName(const cEnumInfo enumInfo[], int32_t value);
    ///< Returns (first found) name for the given enum value.

    void MakeLower(tString* str);   ///< Make string lower case
    void MakeUpper(tString* str);   ///< Make string upper case

    // --- Hashes --------------------------------------------------------------

    uint32_t NameToID(const char* s);
    uint32_t NameToID(const char* begin, const char* end);
    ///< Standardised case insensitive name to ID conversion.


    // --- Regular Expressions -------------------------------------------------
    
    struct cRegExArg
    {
        const char* mString;
        int         mLength;
    };

    bool Match(const char* source, const char* regEx, vector<cRegExArg>* subStrs = 0);
    /** Regular expression match. Returns true on successful match. If subStrs are
     * supplied, subStrs[0] is the full match, subStrs[1] the first bracketed expression,
     * and so on.
     *
     * Supported syntax:
     *  ^       Match beginning of a buffer
     *  $       Match end of a buffer
     *  ()      Grouping and substring capturing
     *  [...]   Match any character from set
     *  [^...]  Match any character but ones from set
     *  \s      Match whitespace
     *  \S      Match non-whitespace
     *  \d      Match decimal digit
     *  \r      Match carriage return
     *  \n      Match newline
     *  +       Match one or more times (greedy)
     *  +?      Match one or more times (non-greedy)
     *  *       Match zero or more times (greedy)
     *  *?      Match zero or more times (non-greedy)
     *  ?       Match zero or once
     *  \xDD        Match byte with hex value 0xDD
     *  \meta       Match one of the meta character: ^$().[*+?\
     */


    // API for reusing the same regex repeatedly
    struct cRegEx
    {
        uint8_t mCode[256];
        uint8_t mData[256];
        int32_t mCodeSize;
        int32_t mDataSize;
        int32_t mNumBrackets;   ///< Number of bracketed subexpressions
        int32_t mAnchored;

        const char* mErrorString;   ///< Error string if construction fails.
    };

    bool MakeRegEx(const char* regExStr, cRegEx* regExData);
    ///< Create regular expression from given string. Returns false on failure, in
    ///< which case regExData->mErrorString will contain the reason.
    bool Match(const char* source, cRegEx* regEx, vector<cRegExArg>* subStrs = 0);
    ///< Version of match that takes a compiled regEx.


#ifdef CL_ISTREAM
    bool        IsEndOfLine(istream &s);
    void        ChompWhiteSpace(istream &s);
    // reads from s until next non-whitespace character.

    istream& ReadString(istream &s, tString* str);
    ///< read a quote-enclosed string, e.g. "hello"
    istream& ReadWord(istream &s, tString* str);
    ///< read the next string of non-whitespace characters
    istream& ReadLine(istream &s, tString* str);
    ///< read a line.
#endif

    // Consistently-named string comparison routines
    bool eq(tStrConst lhs, tStrConst rhs); ///< Case sensitive compare
    bool eq(tString   lhs, tStrConst rhs);
    bool eq(tStrConst lhs, tString   rhs);
    bool eq(tString   lhs, tString   rhs);

    bool eqi(tStrConst lhs, tStrConst rhs); ///< Case insensitive compare
    bool eqi(tString   lhs, tStrConst rhs);
    bool eqi(tStrConst lhs, tString   rhs);
    bool eqi(tString   lhs, tString   rhs);


    // --- Inlines -------------------------------------------------------------

    inline bool eq(tStrConst lhs, tStrConst rhs)
    { return strcmp(lhs, rhs) == 0; }
    inline bool eq(tString lhs, tStrConst rhs)
    { return strcmp(lhs.c_str(), rhs) == 0; }
    inline bool eq(tStrConst lhs, tString rhs)
    { return strcmp(lhs, rhs.c_str()) == 0; }
    inline bool eq(tString lhs, tString rhs)
    { return strcmp(lhs.c_str(), rhs.c_str()) == 0; }
    
    inline bool eqi(tStrConst lhs, tStrConst rhs)
    { return strcasecmp(lhs, rhs) == 0; }    
    inline bool eqi(tString lhs, tStrConst rhs)
    { return strcasecmp(lhs.c_str(), rhs) == 0; }
    inline bool eqi(tStrConst lhs, tString rhs)
    { return strcasecmp(lhs, rhs.c_str()) == 0; }
    inline bool eqi(tString lhs, tString rhs)
    { return strcasecmp(lhs.c_str(), rhs.c_str()) == 0; }

    inline uint32_t NameToID(const char* s)
    {
        return StrIHashU32(s, 0x811C9DC5) | 0x80000000;
    }

    inline uint32_t NameToID(const char* begin, const char* end)
    {
        return IHashU32(begin, end, 0x811C9DC5) | 0x80000000;
    }
}

#endif
