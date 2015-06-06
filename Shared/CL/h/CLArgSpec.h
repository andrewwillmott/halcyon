//
//  File:       CLArgSpec.h
//
//  Function:   Argument parsing package, originally based off Heckbert's arg_parse
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2003
//

#ifndef CL_ARG_SPEC_H
#define CL_ARG_SPEC_H

#include <CLDefs.h>
#include <CLString.h>

#ifndef CL_NO_VL
    #define CL_NO_VL 0
#endif

#if !CL_NO_VL
    #include <VL234f.h>
#endif


namespace nCL
{
    typedef char tChar;
    
    struct cArgEnumInfo;
    class cArguments;

    enum tHelpType
    {
        kHelpBrief,         ///< One-line help.
        kHelpFull,          ///< Full plain text help, including options.
        kHelpHTML,          ///< HTML-formatted help
        kMaxArgHelpTypes
    };
    
    enum tArgError
    {
        kArgNoError,
        kArgHelpRequested,
        kArgErrorNotEnoughArgs,
        kArgErrorTooManyArgs,
        kArgErrorBadSpec,
        kArgErrorUnknownOption,
        kArgErrorBadEnum,
        kMaxArgErrors
    };
    
    enum tArgSpecError
    {
        kSpecNoError,
        kSpecUnbalancedBrackets,   ///< unbalanced [/]
        kSpecEllipsisError,        ///< unexpected ...
        kSpecUnknownType,          ///< unrecognized argument type
        kMaxSpecErrors
    };
    
    enum tArgType
    {
        kTypeInvalid,
        kTypeBool,
        kTypeInt32,
        kTypeFloat,
        kTypeDouble,
        kTypeCString,    // tChar*
        kTypeString,     // tString
        kTypeVec2,
        kTypeVec3,
        kTypeVec4,
        kTypeEnumBegin,
        kTypeEnumEnd = kTypeEnumBegin + 256,

        kMaxTypes,
        
        kTypeArrayFlag       = 0x4000,     // array type: expecting (a b c ...)
        kTypeRepeatLastFlag  = 0x8000,     // array type, but just expecting last arg to be repeated
        kTypeBaseMask        = 0x3FFF
    };
    inline tArgType operator | (tArgType a, tArgType b)
    { return tArgType(int(a) | int(b)); }
    
    struct cArgEnumInfo
    {
        const tChar* mToken;
        uint32_t     mValue;
    };
    
    struct cArgInfo
    {
        tArgType     mType;        ///< type of argument
        tString      mName;        ///< name for argument (optional)
        void*        mLocation;    ///< pointer to result
        bool         mIsDependent; ///< present iff the previous argument is.
        int          mFlagToSet;   ///< if +ve, set this flag if we see this argument
    };
    
    struct cOptionsSpec
    {
        tString          mName;          ///< Name of option (-mName)
        tString          mDescription;   ///< What it does
        vector<cArgInfo> mArguments;     ///< Arguments
        int              mFlagToSet;     ///< if +ve, set this flag if we see this argument
    };
    
    struct cEnumSpec
    {
        cEnumSpec(const tChar* name, const cArgEnumInfo* enumInfo);
        
        tString              mName;
        const cArgEnumInfo*  mEnumInfo;
    };
    
    
    class cArgSpec
    /** Provides a specification for how to parse a command line,
        and a mechanism for performing the parsing.
    
        The cArgSpec class binds a set of default arguments
        and options to a set of C++ variables. This is done
        via a simple vararg-style specification.
        
        The class handles bools, ints, floats, strings, enums,
        vectors of any of these, and repeated arguments. It also
        handles optional arguments, error detection, and
        help.
        
        Here's a simple example:
        
        enum { kSlotPresent, kFlagPresent };
    
        tString name;
        int slot;
        
        argSpec.ConstructSpec
        (
             "description",
             "<name:string> [<slot:int>^]", &name, &slot, kSlotPresent,
             "Purpose of name and slot",
             "-flag^", kFlagPresent,
             "What the flag does",
             0
        );
        
        tArgError = argSpec.Parse(argc, argv);
        ...
        if (argSpec.Flag(kFlagPresent)) 
            ...
        
        When you call Parse(), name, slot and the flags kSlotPresent and 
        kFlagPresent are set appropriately, or an error returned if the 
        arguments don't properly match the spec. Moreover, you can retrieve a 
        help string based on the specification via HelpString().
    */
    {
    public:
        // Creators
        cArgSpec(bool respectHelp = true);  ///< If respectHelp is true, -h or -help will set the result string to the full help, and cause Parse() to return kArgHelpRequested
        
        // cArgSpec
        tArgSpecError ConstructSpec(const tChar* briefDescription, ...);    ///< Construct the specification.

        tArgError Parse(cArguments* args);   ///< Parse args according to the previously set specification.
        tArgError Parse(int argc, const tChar** argv);  ///< Parse C-style args according to the previously set specification.

        void SetFlag(int flag);
        /**< Set the given flag. Generally flags are set by this class as the result of a Parse() call,
            but it is occasionally useful to set them externally during post-Parse() processing. */
        bool Flag(int flag) const;
        ///< Returns value of given flag, set by Parse(). All flags are cleared before Parse() does its job.
        
        void CreateHelpString(const tChar* commandName, tString* pString, tHelpType helpType = kHelpFull) const;
        ///< Create the given kind of help in pString.
        const tChar* HelpString(const tChar* commandName, tHelpType helpType = kHelpFull) const;
        ///< Return given type of help: this also sets ResultString().
        
        const tChar* ResultString();    ///< Returns description of the results of the last call to Parse().
        
    protected:
        // Utilities
        tArgError    ParseArgument(const cArgInfo& info, const tChar* arg);
        tArgError    ParseArrayArgument(const cArgInfo& info, const tChar* arg);

        tArgError    ParseOptionArgs(const vector<cArgInfo>& optArgs, const tChar**& argv, const tChar** argvEnd);
        tArgError    ParseOption(const tChar**& argv, const tChar** argvEnd);
        
        void         AddArgDocs(tString* pString, const vector<cArgInfo>& args, tHelpType helpType) const;
        void         FindNameAndTypeFromOption(const tString& str, tArgType* type, tString* name) const;
        const tChar* NameFromArgType(tArgType argType) const;
        tArgType     ArgTypeFromName(const tChar* typeName) const;

        // Data
        tString              mCommandName;
        tString              mBriefDescription;
        tString              mFullDescription;
        vector<cArgInfo>     mDefaultArguments;
        vector<cOptionsSpec> mOptions;
        vector<cEnumSpec>    mEnumSpecs;
        bool                 mRespectHelp;
        uint32_t             mFlags;
        
        mutable tString mErrorString;
    };
}

inline const nCL::tChar* nCL::cArgSpec::ResultString()
{
    return mErrorString.c_str();
}

inline bool nCL::cArgSpec::Flag(int flag) const
{
    CL_ASSERT(flag < int(sizeof(mFlags) * 8));
    return (mFlags & (1 << flag)) != 0;
}

inline void nCL::cArgSpec::SetFlag(int flag)
{
    CL_ASSERT(flag < int(sizeof(mFlags) * 8));
    mFlags |= 1 << flag;
}

#endif
