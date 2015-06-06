//
//  File:       CLJSON.cpp
//
//  Function:   JSON Read/write
//
//  Author(s):  Based off jsoncpp (public domain)
//
//  Copyright:  2013
//

#include <CLJSON.h>

#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLMemory.h>

#include <ctype.h>

using namespace nCL;

/// --- cJSONReader ------------------------------------------------------------

namespace
{
    inline bool In(char c, const char* s)
    {
        do
        {
            if (*s == c)
                return true;
        }
        while (*++s);

        return false;
    }

    inline bool in(char c, char c1, char c2, char c3, char c4)
    {
        return c == c1 || c == c2 || c == c3 || c == c4;
    }

    inline bool in(char c, char c1, char c2, char c3, char c4, char c5)
    {
        return c == c1 || c == c2 || c == c3 || c == c4 || c == c5;
    }

    bool ContainsNewLine(const char* begin, const char* end)
    {
        for (; begin < end; ++begin)
            if (*begin == '\n' || *begin == '\r')
                return true;

        return false;
    }

    inline bool IsStartTokenChar(char c)
    {
        return isalpha(c) || c == '_' || c == '@';
    }

    inline bool IsTokenChar(char c)
    {
        return isalnum(c) || In(c, "_@.-+=");     // in particular, NOT : or , or []{} etc.
    }
}


cJSONReader::cJSONReader() :
    mNodes(),
    mErrors(),
    mBegin(0),
    mEnd(0),
    mCurrent(0),
    mLastValueEnd(0),
    mLastValue(0),
    mCommentsBefore(),
    mCollectComments(false),
    mAllowUnquotedStrings(true),
    mAllowTrailingCommas(true)
{
}

cJSONReader::~cJSONReader()
{
}

bool cJSONReader::Read(const char* document, cValue* root, bool collectComments)
{
    const char* begin = document;
    const char* end   = begin + strlen(document);

    return Read(begin, end, root, collectComments);
}

bool cJSONReader::Read(const char* beginDoc, const char* endDoc, cValue* root, bool collectComments)
{
    mBegin = beginDoc;
    mEnd = endDoc;
    mCollectComments = collectComments;
    mCurrent = mBegin;
    mLastValueEnd = 0;
    mLastValue = 0;
    mCommentsBefore = "";
    mErrors.clear();
    mNodes.clear();

    mNodes.push_back(root);
    bool successful = ReadValue();
    mNodes.pop_back();

    SkipSpaces();

    if (mErrors.empty() && mCurrent != mEnd)
    {
        AddError("trailing garbage", { kTokenEndOfStream, mCurrent, mEnd });

        successful = false;
    }

    if (mCollectComments && !mCommentsBefore.empty())
        root->SetComment(mCommentsBefore, kCommentAfter);

    return successful;
}

bool cJSONReader::Read(const char* beginDoc, const char* endDoc, cObjectValue* root, bool collectComments)
{
    cValue wrapperValue(root);

    bool success = Read(beginDoc, endDoc, &wrapperValue, collectComments);

    return success && wrapperValue.IsObject();
}

bool cJSONReader::ReadValue()
{
    cToken token;
    ReadNonCommentToken(token);

    return ReadValue(token);
}

bool cJSONReader::ReadValue(cToken& token)
{
    bool successful = true;

    if (mCollectComments && !mCommentsBefore.empty())
    {
        mNodes.back()->SetComment(mCommentsBefore, kCommentBefore);
        mCommentsBefore = "";
    }

    switch (token.mType)
    {
    case kTokenObjectBegin:
        successful = ReadObject(token);
        CL_ASSERT(mNodes.back()->AsObject()->NumParents() == 0);
        break;
    case kTokenArrayBegin:
        successful = ReadArray(token);
        break;
    case kTokenNumber:
        successful = DecodeNumber(token);
        break;
    case kTokenString:
        successful = DecodeString(token);
        break;
    case kTokenTrue:
        *mNodes.back() = true;
        break;
    case kTokenFalse:
        *mNodes.back() = false;
        break;
    case kTokenNull:
        *mNodes.back() = cValue();
        break;
    default:
        return AddError("Syntax error: value, object or array expected.", token);
    }

    if (mCollectComments)
    {
        mLastValueEnd = mCurrent;
        mLastValue = mNodes.back();
    }

    return successful;
}


bool cJSONReader::ReadNonCommentToken(cToken& token)
{
    bool success;

    do
    {
        success = ReadToken(token);
    }
    while (success && token.mType == kTokenComment);

    return success;
}


bool cJSONReader::ExpectToken(tTokenType type, cToken& token, const char* message)
{
    ReadToken(token);

    if (token.mType != type)
        return AddError(message, token);

    return true;
}


bool cJSONReader::ReadToken(cToken& token)
{
    SkipSpaces();

    token.mStart = mCurrent;
    char c = GetNextChar();
    bool ok = true;
    bool validUnquoted = false;
    
    switch (c)
    {
    case '{':
        token.mType = kTokenObjectBegin;
        break;
    case '}':
        token.mType = kTokenObjectEnd;
        break;
    case '[':
        token.mType = kTokenArrayBegin;
        break;
    case ']':
        token.mType = kTokenArrayEnd;
        break;
    case '"':
        token.mType = kTokenString;
        ok = ReadString();
        break;
    case '/':
        token.mType = kTokenComment;
        ok = ReadComment();
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
        token.mType = kTokenNumber;
        ReadNumber();
        break;
    case 't':
        token.mType = kTokenTrue;
        validUnquoted = true;
        ok = Match("rue", 3);
        break;
    case 'f':
        token.mType = kTokenFalse;
        validUnquoted = true;
        ok = Match("alse", 4);
        break;
    case 'n':
        token.mType = kTokenNull;
        validUnquoted = true;
        ok = Match("ull", 3);
        break;
    case ',':
        token.mType = kTokenArraySeparator;
        break;
    case ':':
        token.mType = kTokenMemberSeparator;
        break;
    case 0:
        token.mType = kTokenEndOfStream;
        break;
    default:
        validUnquoted = IsStartTokenChar(c);
        ok = false;
        break;
    }

    if (!ok && mAllowUnquotedStrings && validUnquoted)
    {
        token.mType = kTokenString;
        ok = ReadUnquotedString();
    }

    if (!ok)
        token.mType = kTokenError;

    token.mEnd = mCurrent;

    return true;
}


void cJSONReader::SkipSpaces()
{
    while (mCurrent != mEnd)
    {
        char c = *mCurrent;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            ++mCurrent;
        else
            break;
    }
}


bool cJSONReader::Match(tLocation pattern, int patternLength)
{
    if (mEnd - mCurrent < patternLength)
        return false;

    for (int i = 0; i < patternLength; i++)
        if (mCurrent[i] != pattern[i])
            return false;

    mCurrent += patternLength;

    return true;
}


bool cJSONReader::ReadComment()
{
    tLocation commentBegin = mCurrent - 1;
    char c = GetNextChar();
    bool successful = false;
    if (c == '*')
        successful = ReadCStyleComment();
    else if (c == '/')
        successful = ReadCppStyleComment();
    if (!successful)
        return false;

    if (mCollectComments)
    {
        tCommentPlacement placement = kCommentBefore;

        if (mLastValueEnd && !ContainsNewLine(mLastValueEnd, commentBegin))
        {
            if (c != '*' || !ContainsNewLine(commentBegin, mCurrent))
                placement = kCommentAfterOnSameLine;
        }

        AddComment(commentBegin, mCurrent, placement);
    }
    return true;
}


void cJSONReader::AddComment(tLocation begin, tLocation end, int placement)
{
    CL_ASSERT(mCollectComments);

    if (placement == kCommentAfterOnSameLine)
    {
        CL_ASSERT(mLastValue != 0);
        mLastValue->SetComment(string(begin, end), tCommentPlacement(placement));
    }
    else
    {
        if (!mCommentsBefore.empty())
            mCommentsBefore += "\n";

        mCommentsBefore += string(begin, end);
    }
}


bool cJSONReader::ReadCStyleComment()
{
    while (mCurrent != mEnd)
    {
        char c = GetNextChar();
        if (c == '*' && *mCurrent == '/')
            break;
    }
    return GetNextChar() == '/';
}


bool cJSONReader::ReadCppStyleComment()
{
    while (mCurrent != mEnd)
    {
        char c = GetNextChar();

        if ( c == '\r' || c == '\n')
            break;
    }

    return true;
}


void cJSONReader::ReadNumber()
{
    while (mCurrent != mEnd)
    {
        if (!(*mCurrent >= '0' && *mCurrent <= '9')
            && !In(*mCurrent, ".eE+-"))
            break;

        ++mCurrent;
    }
}

bool cJSONReader::ReadString()
{
    char c = 0;

    while (mCurrent != mEnd)
    {
        c = GetNextChar();

        if (c == '\\')
            GetNextChar();
        else if (c == '"')
            break;
    }

    return c == '"';
}

bool cJSONReader::ReadUnquotedString()
{
    while (mCurrent != mEnd)
    {
        if (!IsTokenChar(*mCurrent))
            break;

        ++mCurrent;
    }

    return true;
}


bool cJSONReader::ReadObject(cToken& tokenStart)
{
    cToken tokenName;
    string name;

    cValue* destValue = mNodes.back();
    cObjectValue* object;

    cObjectValue savedChildren;

    if (destValue->Type() == kValueObject)
    {
        object = destValue->AsObject();
        object->Swap(&savedChildren);
        object->RemoveParents();
    }
    else
    {
        destValue->MakeNull();
        destValue->MakeObject();

        object = destValue->AsObject();
    }

    CL_ASSERT(object->NumParents() == 0);

    if (destValue != mNodes.front())
        object->SetOwner(mNodes.front()->AsObject());

    while (ReadNonCommentToken(tokenName))
    {
        if (tokenName.mType == kTokenObjectEnd && (name.empty() || mAllowTrailingCommas))  // empty object
            break;

        if (tokenName.mType != kTokenString)
            return AddErrorAndRecover("Object member name isn't a string", tokenName, kTokenObjectEnd);

        name.clear();
        if (!DecodeString(tokenName, name))
            return RecoverFromError(kTokenObjectEnd);

        cToken colon;
        if (!ReadNonCommentToken(colon) || colon.mType != kTokenMemberSeparator)
            return AddErrorAndRecover("Missing ':' after object member name", colon, kTokenObjectEnd);

        tTag tag = TagFromString(name.c_str());

        cValue* value = &object->InsertMember(tag);

        cValue* oldValue = savedChildren.ModifyMember(tag);

        if (oldValue)
        {
            cObjectValue* oldObjectValue = oldValue->AsObject();

            if (oldObjectValue)
                *value = oldObjectValue;
        }

        mNodes.push_back(value);
        bool ok = ReadValue();
        mNodes.pop_back();

        CL_ASSERT(!value->IsObject() || value->AsObject()->NumParents() == 0);

        if (!ok) // error already set
            return RecoverFromError(kTokenObjectEnd);

        cToken comma;
        if (  !ReadNonCommentToken(comma)
           || (   comma.mType != kTokenObjectEnd
               && comma.mType != kTokenArraySeparator
              )
           )
        {
            return AddErrorAndRecover("Missing ',' or '}' in object declaration", comma, kTokenObjectEnd);
        }

        if (comma.mType == kTokenObjectEnd)
            break;
    }

    object->IncModCount();

    return true;
}


bool cJSONReader::ReadArray(cToken& tokenStart)
{
    *mNodes.back() = cValue(kValueArray);
// TODO    cArrayValue& array = mNodes.back()->AsArray();
    cValue& array = *mNodes.back();

    int index = 0;
    cToken token;

    while (true)
    {
        if (!ReadNonCommentToken(token))
            return AddErrorAndRecover("Missing remainder of array", token, kTokenArrayEnd);

        // Allow ] next if empty array or we support trailing commas
        if (token.mType == kTokenArrayEnd && (mAllowTrailingCommas || index == 0))
            break;

        cValue& value = array[index++];

        mNodes.push_back(&value);
        bool ok = ReadValue(token);
        mNodes.pop_back();

        if (!ok) // error already set
            return RecoverFromError(kTokenArrayEnd);

        if (!ReadNonCommentToken(token))
            return AddErrorAndRecover("Missing remainder of array", token, kTokenArrayEnd);

        if (token.mType == kTokenArrayEnd)
            break;

        if (token.mType != kTokenArraySeparator)
            return AddErrorAndRecover("Expecting ',' in array declaration", token, kTokenArrayEnd);

        if (token.mType == kTokenArrayEnd)
            break;
    }
    
    return true;
}


bool cJSONReader::DecodeNumber(cToken& token)
{
    bool IsDouble = false;

    for (tLocation inspect = token.mStart; inspect != token.mEnd; ++inspect)
    {
        IsDouble = IsDouble  
           || In(*inspect, ".eE+")
           || (*inspect == '-' && inspect != token.mStart);
    }

    if (IsDouble)
        return DecodeDouble(token);

    tLocation current = token.mStart;
    bool isNegative = *current == '-';
    if (isNegative)
        ++current;

    uint32_t threshold = (isNegative ? uint32_t(-INT32_MIN) : UINT32_MAX) / 10;
    uint32_t value = 0;

    while (current < token.mEnd)
    {
        char c = *current++;

        if (c < '0' || c > '9')
            return AddError(("'" + string(token.mStart, token.mEnd) + "' is not a number.").c_str(), token);

        if (value >= threshold)
            return DecodeDouble(token);

        value = value * 10 + uint32_t(c - '0');
    }

    if (isNegative)
        *mNodes.back() = -int32_t(value);
    else if (value <= uint32_t(INT32_MAX))
        *mNodes.back() = int32_t(value);
    else
        *mNodes.back() = value;
    
    return true;
}


bool cJSONReader::DecodeDouble(cToken& token)
{
    double value = 0;
    const int bufferSize = 32;

    int count;
    int length = int(token.mEnd - token.mStart);

    if (length <= bufferSize)
    {
        char buffer[bufferSize];
        memcpy(buffer, token.mStart, length);
        buffer[length] = 0;
        count = sscanf(buffer, "%lf", &value);
    }
    else
    {
        string buffer(token.mStart, token.mEnd);
        count = sscanf(buffer.c_str(), "%lf", &value);
    }

    if (count != 1)
        return AddError(("'" + string(token.mStart, token.mEnd) + "' is not a number.").c_str(), token);

    *mNodes.back() = value;
    return true;
}


bool cJSONReader::DecodeString(cToken& token)
{
    string decoded;

    if (!DecodeString(token, decoded))
        return false;

    *mNodes.back() = decoded.c_str();

    return true;
}


bool cJSONReader::DecodeString(cToken& token, string& decoded)
{
    tLocation current = token.mStart;
    tLocation end = token.mEnd;

    bool quoted = *current == '"';

    if (quoted)
    {
        CL_ASSERT(*(end - 1) == '"');
        current++;
        end--;
    }

    decoded.reserve(end - current);

    while (current != end)
    {
        char c = *current++;

        if (quoted)
        {
            if (c == '"')
                break;
        }
        else
        {
            if (!IsTokenChar(c))
                break;
        }

        if (c == '\\')
        {
            if (current == end)
                return AddError("Empty escape sequence in string", token, current);

            char escape = *current++;

            switch (escape)
            {
            case '"':  decoded += '"';  break;
            case '/':  decoded += '/';  break;
            case '\\': decoded += '\\'; break;
            case 'b':  decoded += '\b'; break;
            case 'f':  decoded += '\f'; break;
            case 'n':  decoded += '\n'; break;
            case 'r':  decoded += '\r'; break;
            case 't':  decoded += '\t'; break;
            case 'u':
                {
                    unsigned int unicode;

                    if (!DecodeUnicodeEscapeSequence(token, current, end, unicode))
                        return false;

                    // @todo encode unicode as utf8.
                    // @todo remember to alter the writer too.
                }
                break;

            default:
                return AddError("Bad escape sequence in string", token, current);
            }
        }
        else
            decoded += c;
    }

    return true;
}


bool cJSONReader::DecodeUnicodeEscapeSequence(cToken& token, tLocation& current, tLocation end, unsigned int& unicode)
{
    if (end - current < 4)
        return AddError("Bad unicode escape sequence in string: four digits expected.", token, current);

    unicode = 0;

    for (int index =0; index < 4; ++index)
    {
        char c = *current++;

        unicode <<= 4;

        if      (c >= '0' && c <= '9')
            unicode += c - '0';
        else if (c >= 'a' && c <= 'f')
            unicode += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            unicode += c - 'A' + 10;
        else
            return AddError("Bad unicode escape sequence in string: hexadecimal digit expected.", token, current);
    }

    return true;
}


bool cJSONReader::AddError(const char* message, const cToken& token, tLocation extra)
{
    cErrorInfo info;

    info.mToken = token;
    info.mMessage = message;
    info.mExtra = extra;

    mErrors.push_back(info);

    return false;
}


bool cJSONReader::RecoverFromError(tTokenType skipUntilToken)
{
    int errorCount = int(mErrors.size());
    cToken skip;

    while (true)
    {
        if (!ReadToken(skip))
            mErrors.resize(errorCount); // discard errors caused by recovery

        if (skip.mType == skipUntilToken || skip.mType == kTokenEndOfStream)
            break;
    }

    mErrors.resize(errorCount);
    return false;
}


bool cJSONReader::AddErrorAndRecover(const string& message, cToken& token, tTokenType skipUntilToken)
{
    AddError(message.c_str(), token);
    return RecoverFromError(skipUntilToken);
}

char cJSONReader::GetNextChar()
{
    if (mCurrent == mEnd)
        return 0;

    return *mCurrent++;
}


void cJSONReader::GetLocationLineAndColumn(tLocation location, int& line, int& column) const
{
    tLocation current = mBegin;
    tLocation lastLineStart = current;

    line = 0;

    while (current < location && current != mEnd)
    {
        char c = *current++;

        if (c == '\r')
        {
            if (*current == '\n')
                ++current;
            lastLineStart = current;
            ++line;
        }
        else if (c == '\n')
        {
            lastLineStart = current;
            ++line;
        }
    }

    // column & line start at 1
    column = int(location - lastLineStart) + 1;
    ++line;
}


void cJSONReader::AddLocationLineAndColumn(tLocation location, string* str) const
{
    int line, column;
    GetLocationLineAndColumn(location, line, column);

    char buffer[18 + 16 + 16 + 1];
    snprintf(buffer, sizeof(buffer), "Line %d, Column %d", line, column);

    *str += buffer;
}


void cJSONReader::GetFormattedErrorMessages(string* formattedMessage) const
{
    formattedMessage->clear();

    for (tErrors::const_iterator itError = mErrors.begin(); itError != mErrors.end(); ++itError)
    {
        const cErrorInfo& error = *itError;

        *formattedMessage += "* ";
        AddLocationLineAndColumn(error.mToken.mStart, formattedMessage);
        *formattedMessage += "\n  ";

        *formattedMessage += error.mMessage;
        *formattedMessage += "\n";

        if (error.mExtra)
        {
            *formattedMessage += "See ";
            AddLocationLineAndColumn(error.mExtra, formattedMessage);
            *formattedMessage += " for detail.\n";
        }
    }
}

int cJSONReader::GetFirstErrorLine() const
{
    auto it = mErrors.begin();

    if (it != mErrors.end())
    {
        int line, column;
        GetLocationLineAndColumn(it->mToken.mStart, line, column);

        return line;
    }

    return -1;
}





// --- cJSONWriter -------------------------------------------------------------

namespace
{
    inline bool IsControlCharacter(char ch)
    {
        return ch > 0 && ch <= 0x1F;
    }

    bool ContainsControlCharacter(const char* str)
    {
        while (*str) 
            if (IsControlCharacter(*(str++)))
                return true;

        return false;
    }

    void UIntToString(unsigned int value, char*& current)
    {
        *--current = 0;

        do
        {
            *--current = (value % 10) + '0';
            value /= 10;
        }
        while (value != 0);
    }

    void ValueToString(int32_t value, string* outString)
    {
        char buffer[32];
        char* current = buffer + sizeof(buffer);
        bool isNegative = value < 0;
        if (isNegative)
            value = -value;
        UIntToString(uint32_t(value), current);
        if (isNegative)
            *--current = '-';
        CL_ASSERT(current >= buffer);
        *outString += current;
    }


    void ValueToString(uint32_t value, string* outString)
    {
        char buffer[32];
        char* current = buffer + sizeof(buffer);
        UIntToString(value, current);
        CL_ASSERT(current >= buffer);
        *outString += current;
    }

    void ValueToString(double value, string* outString, int precision)
    {
        char buffer[32];
        string format;
        format.format("%%##0.%ig", precision);
        snprintf(buffer, 32, format.c_str(), value);
        char* ch = buffer + strlen(buffer) - 1;

        if (*ch != '0')
        {
            *outString += buffer; // nothing to truncate, so save time
            return;
        }

        while (ch > buffer && *ch == '0')
        {
            --ch;
        }
        char* last_nonzero = ch;

        while (ch >= buffer)
        {
            switch (*ch)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                --ch;
                continue;
            case '.':
                // Truncate zeroes to save bytes in output, but keep one.
                *(last_nonzero+2) = '\0';
                *outString += buffer;
                return;
            default:
                *outString += buffer;
                return;
            }
        }

        *outString += buffer;
        return;
    }


    void ValueToString(bool value, string* outString)
    {
        *outString += (value ? "true" : "false");
    }

    void ValueToStringInternal(const char* value,  string* outString, bool bQuoted)
    {
        // Not sure how to handle unicode...
        if (strpbrk(value, "\"\\\b\f\n\r\t") == NULL && !ContainsControlCharacter(value))
        {
            if ( bQuoted )
                *outString += "\"";
            *outString += value;
            if ( bQuoted )
                *outString += "\"";
            return;
        }
        // We have to walk value and escape any special characters.
        // Appending to std::string is not efficient, but this should be rare.
        // (Note: forward slashes are *not* rare, but I am not escaping them.)
        unsigned maxsize = strlen(value)*2 + 3; // allescaped+quotes+NULL
        
        outString->reserve(maxsize + outString->size()); // to avoid lots of mallocs
        if ( bQuoted )
            *outString += "\"";
        
        for (const char* c=value; *c != 0; ++c)
        {
            switch (*c)
            {
                case '\"':
                    *outString += "\\\"";
                    break;
                case '\\':
                    *outString += "\\\\";
                    break;
                case '\b':
                    *outString += "\\b";
                    break;
                case '\f':
                    *outString += "\\f";
                    break;
                case '\n':
                    *outString += "\\n";
                    break;
                case '\r':
                    *outString += "\\r";
                    break;
                case '\t':
                    *outString += "\\t";
                    break;
                    //case '/':
                    // Even though \/ is considered a legal escape in JSON, a bare
                    // slash is also legal, so I see no reason to escape it.
                    // (I hope I am not misunderstanding something.
                    // blep notes: actually escaping \/ may be useful in javascript to avoid </
                    // sequence.
                    // Should add a flag to allow this compatibility mode and prevent this
                    // sequence from occurring.
                default:
                    if (IsControlCharacter(*c))
                    {
                        outString->append_format("\\u%04x", static_cast<int>(*c));
                    }
                    else
                    {
                        *outString += *c;
                    }
                    break;
            }
        }
        if ( bQuoted )
            *outString += "\"";
    }

    void ValueToString(const char* value,  string* outString)
    {
        ValueToStringInternal(value, outString, false);
    }

    void ValueToQuotedString(const char* value,  string* outString)
    {
        ValueToStringInternal(value, outString, true);
    }
}


cJSONWriter::cJSONWriter() :
    mDocument(),
    mQuotedMembers(false),
    mDoublePrecision(5) // Previously this was 16
{
}

cJSONWriter::~cJSONWriter()
{
}

void cJSONWriter::Write(const cValue& root, string* outString)
{
    mDocument.clear();
    WriteValue(root);
    mDocument += "\n";

    mDocument.swap(*outString);
    mDocument.clear();
}

void cJSONWriter::Write(const cObjectValue* object, string* outString)
{
    mDocument.clear();
    WriteObject(object);
    mDocument += "\n";

    mDocument.swap(*outString);
    mDocument.clear();
}

void cJSONWriter::WriteValue(const cValue& value)
{
    switch (value.Type())
    {
    case kValueNull:
        mDocument += "null";
        break;
    case kValueInt:
        ValueToString(value.AsInt(), &mDocument);
        break;
    case kValueUInt:
        ValueToString(value.AsUInt(), &mDocument);
        break;
    case kValueDouble:
        ValueToString(value.AsDouble(), &mDocument, mDoublePrecision);
        break;
    case kValueString:
        ValueToQuotedString(value.AsString(""), &mDocument);
        break;
    case kValueBool:
        ValueToString(value.AsBool(), &mDocument);
        break;
    case kValueArray:
        {
            mDocument += "[";
            int size = value.size();

            for (int index =0; index < size; ++index)
            {
                if (index > 0)
                    mDocument += ",";
                WriteValue(value[index]);
            }

            mDocument += "]";
        }
        break;

    case kValueObject:
        {
            WriteObject(value.AsObject());
        }
        break;
    }
}

void cJSONWriter::WriteObject(const cObjectValue* object)
{
    mDocument += "{";

    if (!mIncludeInherited)
        for (int i = 0, n = object->NumMembers(); i < n; i++)
        {
            if (i > 0)
                mDocument += ",";

            if (mQuotedMembers)
                ValueToQuotedString(object->MemberName(i), &mDocument);
            else
                ValueToString(object->MemberName(i), &mDocument);

            mDocument += ": ";

            WriteValue(object->MemberValue(i));
        }
    else
    {
        bool comma = false;

        for (auto c : object->Children())
        {
            if (comma)
                mDocument += ",";
            else
                comma = true;

            if (mQuotedMembers)
                ValueToQuotedString(c.Name(), &mDocument);
            else
                ValueToString(c.Name(), &mDocument);

            mDocument += ": ";

            WriteValue(c.Value());
        }
    }

    mDocument += "}";
}


// --- cJSONStyledWriter -------------------------------------------------------

namespace
{
    void NormalizeEOL(const char* text, string* outString)
    {
        int textLength = strlen(text);
        outString->reserve(outString->size() + textLength);
        const char* end = text + textLength;
        const char* current = text;
        while (current != end)
        {
            char c = *current++;
            if (c == '\r') // mac or dos EOL
            {
                if (*current == '\n') // convert dos EOL
                    ++current;
                *outString += '\n';
            }
            else // handle unix EOL & other char
                *outString += c;
        }
    }


}

cJSONStyledWriter::cJSONStyledWriter() :
    mChildValues(),
    mDocument(),
    mIndentString(),
    mRightMargin(74),
    mIndentSize(2),
    mAddChildValues(false),
    mQuotedMembers(false),
    mDoublePrecision(5), // Previously this was 16
    mScratch()
{
}

cJSONStyledWriter::~cJSONStyledWriter()
{
}

void cJSONStyledWriter::Write(const cValue& root, string* outString)
{
    mDocument.clear();

    mAddChildValues = false;
    mIndentString.clear();

    WriteCommentBeforeValue(root);
    WriteValue(root);
    WriteCommentAfterValueOnSameLine(root);

    mDocument += "\n";

    mDocument.swap(*outString);
    mDocument.clear();
}

void cJSONStyledWriter::WriteValue(const cValue& value)
{
    switch (value.Type())
    {
    case kValueNull:
        PushValue("null");
        break;
    case kValueInt:
        ValueToString(value.AsInt(), &mScratch);
        PushValue(mScratch.c_str());
        mScratch.clear();
        break;
    case kValueUInt:
        ValueToString(value.AsUInt(), &mScratch);
        PushValue(mScratch.c_str());
        mScratch.clear();
        break;
    case kValueDouble:
        ValueToString(value.AsDouble(), &mScratch, mDoublePrecision);
        PushValue(mScratch.c_str());
        mScratch.clear();
        break;
    case kValueString:
        ValueToQuotedString(value.AsString(""), &mScratch);
        PushValue(mScratch.c_str());
        mScratch.clear();
        break;
    case kValueBool:
        ValueToString(value.AsBool(), &mScratch);
        PushValue(mScratch.c_str());
        mScratch.clear();
        break;
    case kValueArray:
        WriteArrayValue(value);
        break;
    case kValueObject:
        {
            int numMembers = value.NumMembers();
            
            if (numMembers == 0)
                PushValue("{}");
            else
            {
                WriteWithIndent("{");

                Indent();

                int i = 0;

                while (true)
                {
                    const char* name = value.MemberName(i);
                    const cValue& childValue = value.MemberValue(i);
                    WriteCommentBeforeValue(childValue);

                    // WriteWithIndent(valueToQuotedString(name.c_str()));
                    WriteIndent();
                    if ( mQuotedMembers )
                        ValueToQuotedString(name, &mDocument);
                    else
                        ValueToString(name, &mDocument);

                    mDocument += " : ";
                    WriteValue(childValue);

                    if (++i == numMembers)
                    {
                        WriteCommentAfterValueOnSameLine(childValue);
                        break;
                    }

                    mDocument += ",";
                    WriteCommentAfterValueOnSameLine(childValue);
                }

                Unindent();

                WriteWithIndent("}");
            }
        }
        break;
    }
}

void cJSONStyledWriter::WriteArrayValue(const cValue& value)
{
    size_t size = value.size();

    if (size == 0)
        PushValue("[]");
    else
    {
        bool isArrayMultiLine = IsMultiLineArray(value);

        if (isArrayMultiLine)
        {
            WriteWithIndent("[");

            Indent();

            bool hasChildValue = !mChildValues.empty();

            uint index = 0;

            while (true)
            {
                const cValue& childValue = value[index];

                WriteCommentBeforeValue(childValue);

                if (hasChildValue)
                    WriteWithIndent(mChildValues[index].c_str());
                else
                {
                    WriteIndent();
                    WriteValue(childValue);
                }

                if (++index == size)
                {
                    WriteCommentAfterValueOnSameLine(childValue);
                    break;
                }

                mDocument += ",";
                WriteCommentAfterValueOnSameLine(childValue);
            }

            Unindent();

            WriteWithIndent("]");
        }
        else
        {
            CL_ASSERT(mChildValues.size() == size);

            mDocument += "[";

            for (size_t index = 0; index < size; ++index)
            {
                if (index > 0)
                    mDocument += ", ";

                mDocument += mChildValues[index];
            }

            mDocument += " ]";
        }
    }
}

bool cJSONStyledWriter::IsMultiLineArray(const cValue& value)
{
    int size = value.size();
    bool isMultiLine = size * 3 >= mRightMargin ;

    mChildValues.clear();

    for (int index = 0; index < size && !isMultiLine; ++index)
    {
        const cValue& childValue = value[index];

        isMultiLine = isMultiLine  ||
            ((childValue.IsArray() || childValue.IsObject()) && 
            childValue.size() > 0);
    }

    if (!isMultiLine) // check if line length > max line length
    {
        mChildValues.reserve(size);
        mAddChildValues = true;

        int lineLength = 4 + (size - 1) * 2; // '[' + ', '*n + ' ]'

        for (int index = 0; index < size && !isMultiLine; ++index)
        {
            WriteValue(value[index]);
            lineLength += int(mChildValues[index].length());
            isMultiLine = isMultiLine && HasCommentForValue(value[index]);
        }

        mAddChildValues = false;
        isMultiLine = isMultiLine || lineLength >= mRightMargin;
    }

    return isMultiLine;
}

void cJSONStyledWriter::PushValue(const char* value)
{
    // Note: this is inefficient. Unfortunately the API is trying to be fancy and reformat the array based on line
    //       length hence this buffer vector to store the values
    if (mAddChildValues)
        mChildValues.push_back(value);
    else
        mDocument += value;
}

void cJSONStyledWriter::WriteIndent()
{
    if (!mDocument.empty())
    {
        char last = mDocument.back();
        if (last == ' ')  // already indented
            return;
        if (last != '\n') // comments may add new-line
            mDocument += '\n';
    }

    mDocument += mIndentString;
}

void cJSONStyledWriter::WriteWithIndent(const char* value)
{
    WriteIndent();
    mDocument += value;
}

void cJSONStyledWriter::Indent()
{
    mIndentString.insert(mIndentString.size(), mIndentSize, ' ');
}

void cJSONStyledWriter::Unindent()
{
    CL_ASSERT(int(mIndentString.size()) >= mIndentSize);
    mIndentString.resize(mIndentString.size() - mIndentSize);
}

void cJSONStyledWriter::WriteCommentBeforeValue(const cValue& root)
{
    if (!root.HasComment(kCommentBefore))
        return;

    NormalizeEOL(root.Comment(kCommentBefore), &mDocument);

    mDocument += "\n";
}

void cJSONStyledWriter::WriteCommentAfterValueOnSameLine(const cValue& root)
{
    if (root.HasComment(kCommentAfterOnSameLine))
    {
        mDocument += " ";
        NormalizeEOL(root.Comment(kCommentAfterOnSameLine), &mDocument);
    }

    if (root.HasComment(kCommentAfter))
    {
        mDocument += "\n";
        NormalizeEOL(root.Comment(kCommentAfter), &mDocument);
        mDocument += "\n";
    }
}

bool cJSONStyledWriter::HasCommentForValue(const cValue& value)
{
    return value.HasComment(kCommentBefore)
        || value.HasComment(kCommentAfterOnSameLine)
        || value.HasComment(kCommentAfter);
}


// --- Utilities ---------------------------------------------------------------

#ifdef CL_MEMORY_H

bool nCL::ReadFromJSONFile(const cFileSpec& fileSpec, cValue* value, string* errorMessages, int* firstErrorLine)
{
    cMappedFileInfo mapInfo = MapFile(fileSpec.Path());

    if (!mapInfo.mData)
        return false;

    cJSONReader reader;
    bool success = reader.Read((const char*) mapInfo.mData, (const char*) mapInfo.mData + mapInfo.mSize, value);

    if (!success && errorMessages)
        reader.GetFormattedErrorMessages(errorMessages);

    if (firstErrorLine)
        *firstErrorLine = reader.GetFirstErrorLine();

#ifdef JSON_READ_TEST
    LogJSON("JSON", "read", *value);
#endif

    UnmapFile(mapInfo);

    return success;
}

#else

bool nCL::ReadFromJSONFile(const cFileSpec& fileSpec, cValue* value, string* errorMessages, int* firstErrorLine)
{
    bool success = false;

    FILE* file = fileSpec.FOpen("r");

    if (file)
    {
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        string fileStr(fileSize);
        int result = fread(fileStr.data(), 1, fileSize, file);

        if (result == fileSize)
        {
            cJSONReader reader;
            success = reader.Read(fileStr.c_str(), value);

            if (!success && errorMessages)
                reader.GetFormattedErrorMessages(errorMessages);

            if (firstErrorLine)
                *firstErrorLine = reader.GetFirstErrorLine();

        #ifdef JSON_READ_TEST
            LogJSON("JSON", "read", *value);
        #endif
        }

        fclose(file);
    }

    return success;
}

#endif

bool nCL::ReadFromJSONFile(const cFileSpec& spec, cObjectValue* object, string* errorMessages, int* firstErrorLine)
{
    cValue wrapperValue(object);

    bool success = ReadFromJSONFile(spec, &wrapperValue, errorMessages, firstErrorLine);

    success = success && wrapperValue.IsObject();

    object->InsertMember(kSourcePathTag) = spec.Path();

    if (success)
    {
    #ifdef LOCAL_DEBUG
        LogJSON("JSON", "ReadJSON", config);
    #endif

        object->IncModCount();
        // ApplyImports(object);

    #ifdef LOCAL_DEBUG
        printf("\n");
        DumpHierarchy(configPath.Name(), 0, config);
        printf("\n");
    #endif
    }

    return success;
}

#include <CLDirectories.h>
#include <CLUtilities.h>

tTag nCL::kSourcePathTag = CL_TAG("_sourcePath");

namespace
{
#ifdef UNUSED
    cObjectValue* ImportFile(cIConfigSource* configSource, const nCL::cFileSpec& spec, bool required)
    {
        if (!spec.IsReadable())
        {
            if (required)
                CL_LOG_E("Config", "%s doesn't exist or not readable\n", spec.Path());

            return 0;
        }

        cObjectValue* importConfig = configSource->ConfigFileForPath(spec, required);

        if (!importConfig)
        {
            CL_LOG_E("Config", "Failed to read %s\n", spec.Path());
            return 0;
        }

        return importConfig;
    }
#endif

    bool ApplyImport(const cValue& import, const cFileSpec& configSpec, cObjectValue* config, cIConfigSource* configSource)
    {
        CL_ASSERT(config);

        const char* platformStr = import["platform"].AsString();
        if (platformStr && !eq(platformStr, PlatformName()))
            return false;

        const char* configStr = import["config"].AsString();
        if (configStr && !eq(configStr, ConfigName()))
            return false;

        const char* notConfigStr = import["notConfig"].AsString();
        if (notConfigStr && eq(notConfigStr, ConfigName()))
            return false;

        const char* destStr = import["to"].AsString();
        cObjectValue* destConfig;

        if (destStr)
            destConfig = config->InsertMember(destStr).AsObject();  // this may create the value
        else
        {
            destConfig = config;
            destStr = ".";
        }

        const char* directoryPath = import["directory"].AsString();

        if (directoryPath)
        {
            bool useName = import["useName"].AsBool();

            cFileSpec spec(configSpec);
            spec.SetRelativeDirectory(directoryPath);

            cDirectoryInfo dirInfo;
            dirInfo.Read(spec.Directory());

            for (int i = 0, n = dirInfo.mFiles.size(); i < n; i++)
            {
                spec.SetNameAndExtension(dirInfo.mFiles[i].mName);

                if (!eqi(spec.Extension(), "json"))
                    continue;

                cObjectValue* fileDestConfig;
                if (useName)
                    fileDestConfig = destConfig->InsertMember(spec.Name()).AsObject();
                else
                    fileDestConfig = destConfig;

                cObjectValue* importConfig = configSource->ConfigFileForPath(spec);

                if (importConfig)
                {
                    InsertHierarchyAsSuper(importConfig, fileDestConfig);
                    CL_LOG_D("Config", "Imported %s to %s\n", spec.Path(), destStr);
                }
                else
                    CL_LOG_E("Config", "%s doesn't exist or not readable\n", spec.Path());
            }

            return true;
        }

        const char* filePath = import["file"].AsString();

        if (filePath)
        {
            cFileSpec spec(configSpec);
            spec.SetRelativePath(filePath);

            bool required = import["required"].AsBool(true);
            // TODO: only even read if required is true?
            cObjectValue* importConfig = configSource->ConfigFileForPath(spec);

            if (importConfig)
            {
                InsertHierarchyAsSuper(importConfig, destConfig);
                CL_LOG_D("Config", "Imported %s to %s\n", spec.Path(), destStr);
                return true;
            }
            else if (required)
                CL_LOG_E("Config", "%s doesn't exist or not readable\n", spec.Path());
        }

        // TODO: handle object:

        return false;
    }

}

void nCL::ApplyImports(cObjectValue* config, cIConfigSource* configSource)
{
    const cValue& v = config->Member("import");

    if (!v.IsNull())
    {
        cFileSpec configSpec;

        const char* sourcePath = config->Member(kSourcePathTag).AsString();

        if (sourcePath)
            configSpec.SetPath(sourcePath);
        else
            SetDirectory(&configSpec, kDirectoryData);

        if (v.IsObject())
            ApplyImport(v, configSpec, config, configSource);
        else
        {
            // TODO: at the moment, an import to a non-existent object can modify 'config'.
            for (int i = 0, n = v.size(); i < n; i++)
                ApplyImport(config->Member("import")[i], configSpec, config, configSource);
        }

        // nuke this guy now so it doesn't interfere with other stuff.
        config->RemoveMember("import");
    }
}


void nCL::LogJSON(const char* group, const char* label, const cValue& v)
{
    cJSONWriter writer;
    string jsonOut;
    writer.Write(v, &jsonOut);

    CL_LOG(group, "%s:\n%s\n", label, jsonOut.c_str());
}


namespace
{
    class cConfigSource :
        public cIConfigSource,
        public cAllocLinkable
    {
    public:
        cConfigSource(cIAllocator* alloc) : mAllocator(alloc) {}

        int Link(int count) const override { return nCL::cAllocLinkable::Link(count); };

        cObjectValue* ConfigFileForPath(const nCL::cFileSpec& spec) override
        {
            if (!spec.IsReadable())
            {
                CL_LOG_E("Config", "%s doesn't exist or not readable\n", spec.Path());

                return 0;
            }

            string errorMessages;
            int errorLine = 0;
            tObjectLink object = new(mAllocator) cObjectValue;
            object->Link(1);

            bool result = ReadFromJSONFile(spec, object, &errorMessages, &errorLine);

            if (result)
            {
                ApplyImports(object, this);
                return object;
            }

            if (!errorMessages.empty())
                CL_LOG_E("Config", "%s: %s\n", spec.Path(), errorMessages.c_str());

            CL_LOG_E("Config", "Failed to read %s\n", spec.Path());
            return 0;
        }

        cIAllocator* mAllocator = 0;
    };
}

cIConfigSource* nCL::CreateDefaultConfigSource(cIAllocator* alloc)
{
    return new(alloc) cConfigSource(alloc);
}
