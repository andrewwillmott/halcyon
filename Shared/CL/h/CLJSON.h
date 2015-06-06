//
//  File:       CLJSON.h
//
//  Function:   JSON/javascript value class. Supports the usual scalar types as well as
//              array and object data, and optional comment attachment.
//
//  Author(s):  Andrew Willmott, based off jsoncpp (public domain), Baptiste Lepilleur
//
//  Copyright:  2013
//

#ifndef CL_JSON_H
#define CL_JSON_H

#include <CLValue.h>

#include <ustl/map.h>
#include <ustl/vector.h>
#include <ustl/string.h>
#include <ustl/stack.h>

namespace nCL
{
    class cFileSpec;

    extern tTag kSourcePathTag;

    // --- Utilities -----------------------------------------------------------

    bool ReadFromJSONFile(const cFileSpec& fileSpec, cValue* value, string* errorMessages = 0, int* firstErrorLine = 0);
    ///< Reads from fileSpec into 'value'. Returns false on failure, and any parse errors in errorMessages.
    bool ReadFromJSONFile(const cFileSpec& fileSpec, cObjectValue* value, string* errorMessages = 0, int* firstErrorLine = 0);
    ///< Version of ReadFromJSONFile that assumes the file describes an object. On success the object is tagged with kSourcePathTag.

    class cIConfigSource
    /// Interface for reading json-based files to allow customisation (e.g., caching/hotloading)
    {
    public:
        virtual int Link(int count) const = 0;
        virtual cObjectValue* ConfigFileForPath(const nCL::cFileSpec& spec) = 0;
    };

    cIConfigSource* CreateDefaultConfigSource(cIAllocator* alloc);
    ///< Creates default config source that just reads file as is and recursively calls ApplyImports

    void ApplyImports(cObjectValue* config, cIConfigSource* configSource);
    ///< Applies any imports: object blocks to the given config, using 'configSource' to retrieve the imported objects.

    void LogJSON(const char* group, const char* label, const cValue& v);
    ///< Logs the contents of the given value to the given group.


    // --- cJSONReader ---------------------------------------------------------

    class cJSONReader
    {
    public:
        cJSONReader();
        ~cJSONReader();

        bool Read(const char* document, cValue* root, bool collectComments = false);
        ///< Read a cValue from the given UTF8 document string.
        ///< If collectComments is true, values will be annotated with comments such that
        ///< they can be preserved when written back out.
        ///< Returns false if an error occurred, although an attempt is made to recover even in the 
        ///< presence of errors.
        ///< If root points to a previous object hierarchy, this routine will preserve the cObjectValue pointers
        ///< in that hierarchy. However it will ensure any inheritance chains are removed, to simulate a freshly read
        ///< hierarchy, so any such inheritance must be re-applied.

        bool Read(const char* beginDoc, const char* endDoc, cValue* root, bool collectComments = false);
        ///< Read a cValue from a string range.

        bool Read(const char* beginDoc, const char* endDoc, cObjectValue* root, bool collectComments = false);
        ///< Read a cObjectValue from a string range.

        void GetFormattedErrorMessages(string* formattedMessage) const;
        ///< Returns a string that lists any errors in the parsed document.

        int GetFirstErrorLine() const;
        ///< Returns line number of the first error, or -1 if none.

    protected:
        typedef const char* tLocation;

        enum tTokenType
        {
            kTokenEndOfStream = 0,
            kTokenObjectBegin,
            kTokenObjectEnd,
            kTokenArrayBegin,
            kTokenArrayEnd,
            kTokenString,
            kTokenNumber,
            kTokenTrue,
            kTokenFalse,
            kTokenNull,
            kTokenArraySeparator,
            kTokenMemberSeparator,
            kTokenComment,
            kTokenError
        };

        class cToken
        {
        public:
            tTokenType  mType;
            const char* mStart;
            const char* mEnd;
        };

        class cErrorInfo
        {
        public:
            cToken     mToken;
            string     mMessage;
            tLocation  mExtra;
        };

        typedef vector<cErrorInfo> tErrors; // TODO: originally a deque
        typedef vector<cValue*> tNodes;

        // Utils
        bool ExpectToken(tTokenType type, cToken& token, const char* message);
        bool ReadToken(cToken& token);
        void SkipSpaces();
        bool Match(tLocation pattern, int patternLength);

        bool ReadComment();
        bool ReadCStyleComment();
        bool ReadCppStyleComment();
        bool ReadString();
        bool ReadUnquotedString();
        void ReadNumber();
        bool ReadValue();

        bool ReadValue(cToken& token);
        bool ReadObject(cToken& token);
        bool ReadArray(cToken& token);
        bool DecodeNumber(cToken& token);
        bool DecodeString(cToken& token);
        bool DecodeString(cToken& token, string& decoded);
        bool DecodeDouble(cToken& token);
        bool DecodeUnicodeEscapeSequence(cToken& token, tLocation& current, tLocation end, uint32_t& unicode);

        bool AddError(const char* message, const cToken& token, tLocation extra = 0);
        bool RecoverFromError(tTokenType skipUntilToken);
        bool AddErrorAndRecover(const string& message, cToken& token, tTokenType skipUntilToken);

        void SkipUntilSpace();

        char GetNextChar();
        void GetLocationLineAndColumn(tLocation location, int& line, int& column) const;
        void AddLocationLineAndColumn(tLocation location, string* str) const;
        void AddComment(tLocation begin, tLocation end, int placement);
        bool ReadNonCommentToken(cToken& token);

        // Data
        tNodes      mNodes;
        tErrors     mErrors;
        tLocation   mBegin;
        tLocation   mEnd;
        tLocation   mCurrent;

        // Comments handling
        tLocation   mLastValueEnd;
        cValue*     mLastValue;
        string      mCommentsBefore;

        // Options
        bool        mCollectComments;
        bool        mAllowUnquotedStrings;
        bool        mAllowTrailingCommas;
    };


    // --- cJSONWriter ---------------------------------------------------------

    class cJSONWriter
    {
    public:
        cJSONWriter();
        ~cJSONWriter();

        void Write(const cValue& root, string* outString);
        void Write(const cObjectValue* root, string* outString);

    protected:
        void WriteValue(const cValue& value);
        void WriteObject(const cObjectValue* object);

        string  mDocument;
        bool    mQuotedMembers      = false;
        bool    mIncludeInherited   = false;
        int     mDoublePrecision    = 5;
    };


    // --- cJSONStyledWriter ---------------------------------------------------

    class cJSONStyledWriter
    {
    public:
        cJSONStyledWriter();
        ~cJSONStyledWriter();

        void Write(const cValue& root, string* outString);

    protected:
        void WriteValue(const cValue& value);
        void WriteArrayValue(const cValue& value);
        bool IsMultiLineArray(const cValue& value);
        void PushValue(const char* value);
        void WriteIndent();
        void WriteWithIndent(const char* value);
        void Indent();
        void Unindent();
        void WriteCommentBeforeValue(const cValue& root);
        void WriteCommentAfterValueOnSameLine(const cValue& root);
        bool HasCommentForValue(const cValue& value);

        // Data definitions
        typedef vector<string> tChildValues;

        // Data
        tChildValues    mChildValues;
        string          mDocument;
        string          mIndentString;
        int             mRightMargin;
        int             mIndentSize;
        bool            mAddChildValues;
        bool            mQuotedMembers;
        int             mDoublePrecision;

        string mScratch;
    };
}


#endif
