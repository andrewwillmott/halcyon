//
//  File:       CLArgSpec.cpp
//
//  Function:   Argument parsing package
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2003
//

#include <CLArgSpec.h>

#include <CLString.h>
#include <strings.h>

using namespace nCL;

namespace
{
    const tChar* const kEllipsisToken   = "...";

    const tChar    kOpenBracketChar     = '[';
    const tChar    kCloseBracketChar    = ']';
    const tChar    kSetFlagChar         = '^';
    const tChar    kEnumSpecChar        = ':';
    const tChar    kOptionChar          = '-';
    const tChar    kBeginArgChar        = '<';
    const tChar    kEndArgChar          = '>';
    const tChar    kArgSepChar          = ':';

    inline bool Eq(const tChar* lhs, const tChar* rhs)
    {
        return strcasecmp(lhs, rhs) == 0;
    }

    inline bool Eq(const tChar* lhs, const tChar* rhs, size_t n)
    {
        return strncasecmp(lhs, rhs, n) == 0;
    }

    inline bool Eq(const tString& lhs, const tChar* rhs)
    {
        return eq(lhs.c_str(), rhs);
    }

    inline bool Eq(const tChar* lhs, const tString& rhs)
    {
        return eq(lhs, rhs.c_str());
    }

    void Sprintf(tString* pStr, const tChar* format, ...)
    {
        static tChar buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 1024, format, args);
        va_end(args);

        (*pStr) = buffer;
    }

    void SprintfAppend(tString* pStr, const tChar* format, ...)
    {
        static tChar buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 1024, format, args);
        va_end(args);

        pStr->append(buffer);
    }

    tString NextToken(tString& str, const tChar* separators = " \t\n")
    {
        tString result(str);
        str = "";

        if (!separators || strlen(separators) == 0)
            return result;

        size_t index = result.find_first_of(separators);

        while (index == 0)
        {
            result.erase(0, 1);
            index = result.find_first_of(separators);
        }

        if (index == tString::npos)
            return result;

        str.assign(result, index + 1, tString::npos); // for some reason default last argument doesn't exist under current STL?
        result.assign(result, 0, index);

        return result;
    }

    float ParseFloat(const tChar*& s)
    {
        tChar* sEnd;
        float result = strtof(s, &sEnd);

        s = sEnd;
        return result;
    }

    double ParseDouble(const tChar*& s)
    {
        tChar* sEnd;
        double result = strtof(s, &sEnd);

        s = sEnd;
        return result;
    }

    void ParseVec2(const tChar*& s, float v[2])
    {
        v[0] = ParseFloat(s);

        if (*s == ',')
        {
            s++;
            v[1] = ParseFloat(s);
        }
        else
            v[1] = v[0];
    }

    void ParseVec3(const tChar*& s, float v[3])
    {
        v[0] = ParseFloat(s);

        if (*s == ',')
        {
            s++;
            ParseVec2(s, v + 1);
        }
        else
        {
            v[1] = v[0];
            v[2] = v[0];
        }
    }

    void ParseVec4(const tChar*& s, float v[4])
    {
        v[0] = ParseFloat(s);

        if (*s == ',')
        {
            s++;
            ParseVec3(s, v + 1);
        }
        else
        {
            v[1] = v[0];
            v[2] = v[0];
            v[3] = v[0];
        }
    }

    tArgError ParseEnum(const tChar* arg, const cArgEnumInfo* parseInfo, uint32_t* value)
    {
        while (parseInfo->mToken)
        {
            if (eq(arg, parseInfo->mToken))
            {
                (*value) = parseInfo->mValue;
                return kArgNoError;
            }
            
            parseInfo++;
        }
        
        return kArgErrorBadEnum;
    }
    
    void AddDocString(tString* pString, const tChar* leader, const tString docString)
    {
        size_t lastLF = 0;
        size_t nextLF;

        do
        {
            nextLF = docString.find('\n', lastLF);
            SprintfAppend(pString, "%s%s\n", leader, docString.substr(lastLF, nextLF - lastLF).c_str());
            lastLF = nextLF + 1;
        }
        while (nextLF != string::npos);
    }

    void AddEnums(tString* pString, const tChar* leader, const vector<cEnumSpec>& enumSpecs, tHelpType helpType)
    {
        pString->append("\n");

        for (size_t i = 0; i < enumSpecs.size(); i++)
        {
            if (helpType == kHelpHTML)
                SprintfAppend(pString, "<p><b>%s</b></p><blockquote><i>", enumSpecs[i].mName.c_str());
            else
                SprintfAppend(pString, "%s%s:\n", leader, enumSpecs[i].mName.c_str());

            const cArgEnumInfo* argEnum = enumSpecs[i].mEnumInfo;

            while (argEnum->mToken)
            {
                if (helpType == kHelpHTML)
                    SprintfAppend(pString, "%s</br>\n", argEnum->mToken);
                else
                    SprintfAppend(pString, "%s   %s\n", leader, argEnum->mToken);
                argEnum++;
            }

            if (helpType == kHelpHTML)
                SprintfAppend(pString, "</i></blockquote>");
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// cEnumSpec
//
cEnumSpec::cEnumSpec(const tChar* name, const cArgEnumInfo* enumInfo) :
    mName(name),
    mEnumInfo(enumInfo)
{
}

////////////////////////////////////////////////////////////////////////////////
// cArgSpec
//

cArgSpec::cArgSpec(bool respectHelp) :
    mBriefDescription(),
    mFullDescription(),
    mDefaultArguments(),
    mOptions(),
    mEnumSpecs(),
    mRespectHelp(respectHelp),
    mErrorString()
{
}

tArgSpecError cArgSpec::ConstructSpec(const tChar* briefDescription, ...)
{
    mBriefDescription = briefDescription;
    mFullDescription = "";
    mDefaultArguments.clear();
    mOptions.clear();
    mEnumSpecs.clear();
    mErrorString = "no error";

    va_list args;
    va_start(args, briefDescription);

    tArgSpecError err = kSpecNoError;

    while (true)
    {
        const tChar* optionCStr = va_arg(args, const tChar*);
        if (!optionCStr)
            break;

        tString options(optionCStr);
        tString nextToken;

        // is this an enum specification? (:enumName).
        if (!options.empty() && options[0] == kEnumSpecChar)
        {
            mEnumSpecs.push_back(cEnumSpec(options.c_str() + 1, va_arg(args, cArgEnumInfo*)));
            continue;
        }

        nextToken = NextToken(options);

        vector<cArgInfo>* argsToAddTo = 0;
        cOptionsSpec newOption;

        // is it an option, or should it be added to the default arguments?
        bool isOption = !nextToken.empty() && nextToken[0] == kOptionChar && isalpha(nextToken[1]);  // must start with a character, otherwise -2 will cause problems.
        if (isOption)
        {
            if (nextToken[nextToken.length() - 1] == kSetFlagChar)
            {
                nextToken.erase(nextToken.length() - 1, 1);
                newOption.mFlagToSet = va_arg(args, int);
            }
            else
                newOption.mFlagToSet = -1;

            newOption.mName.assign(nextToken, 1, tString::npos);

            nextToken = NextToken(options);
            argsToAddTo = &newOption.mArguments;
        }
        else
            argsToAddTo = &mDefaultArguments;

        int optionLevel = 0;
        int lastOptionLevel = 0;

        while (!nextToken.empty())
        {
            if (eq(nextToken, kEllipsisToken))
            {
                if (!options.empty())
                    return kSpecEllipsisError;
                if (argsToAddTo->empty())
                    return kSpecEllipsisError;

                argsToAddTo->back().mType = argsToAddTo->back().mType | kTypeArrayFlag | kTypeRepeatLastFlag;
                nextToken = NextToken(options);
                continue;
            }

            cArgInfo newArgInfo;

            if (nextToken[0] == kOpenBracketChar)
            {
                optionLevel++;
                nextToken.erase(0, 1);
            }

            newArgInfo.mIsDependent = (optionLevel == lastOptionLevel);

            while (!nextToken.empty() && nextToken[nextToken.size() - 1] == kCloseBracketChar)
            {
                optionLevel--;
                nextToken.erase(nextToken.length() - 1, 1);
            }

            // fetch var args: location and optionally flag
            newArgInfo.mLocation    = va_arg(args, void*);

            if (nextToken[nextToken.length() - 1] == kSetFlagChar)
            {
                nextToken.erase(nextToken.length() - 1, 1);                
                newArgInfo.mFlagToSet = va_arg(args, int);
            }
            else
                newArgInfo.mFlagToSet = -1;

            FindNameAndTypeFromOption(nextToken, &newArgInfo.mType, &newArgInfo.mName);

            if (newArgInfo.mType == kTypeInvalid)
                err = kSpecUnknownType; // register error but carry on; we can mostly survive this.

            argsToAddTo->push_back(newArgInfo);

            lastOptionLevel = optionLevel;
            nextToken = NextToken(options);
        }

        if (optionLevel != 0)
        {
            err = kSpecUnbalancedBrackets;
            break;
        }

        const tChar* docCString = va_arg(args, const tChar*);
        CL_ASSERT_MSG(docCString && strlen(docCString) < 500, "Bad argument spec around '%s'\n", optionCStr);   // often we'll crash here if the spec is confused.
        tString docString = docCString;

        if (isOption)
        {
            newOption.mDescription = docString;
            mOptions.push_back(newOption);
        }
        else
            mFullDescription += docString;
    }

    va_end(args);

    return err;
}

void cArgSpec::CreateHelpString(const tChar* commandName, tString* pString, tHelpType helpType) const
{
    if (helpType == kHelpBrief)
    {
        Sprintf(pString, "%s, %s", commandName, mBriefDescription.c_str());
        return;
    }

    if (helpType == kHelpHTML)
        Sprintf(pString, "<tr><td><a name=\"%s\"></a><b>%s</b> ", commandName, commandName);
    else
    {
        (*pString) = commandName;
        pString->append(" ");
    }

    AddArgDocs(pString, mDefaultArguments, helpType);

    if (helpType == kHelpHTML)
    {
        pString->append("<br><blockquote><p>");
        AddDocString(pString, "", mFullDescription);
    }
    else
        AddDocString(pString, "    ", mFullDescription);

    if (helpType == kHelpHTML)
        SprintfAppend(pString, " Options:</p>");
    else
        SprintfAppend(pString, "\n    Options:\n");

    for (size_t j = 0; j < mOptions.size(); j++)
    {
        if (helpType == kHelpHTML)
            SprintfAppend(pString, "<b>-%s</b> ", mOptions[j].mName.c_str());
        else
            SprintfAppend(pString, "    -%s ", mOptions[j].mName.c_str());

        AddArgDocs(pString, mOptions[j].mArguments, helpType);

        if (helpType == kHelpHTML)
        {
            pString->append("<br><blockquote>");
            AddDocString(pString, "", mOptions[j].mDescription);
            pString->append("</blockquote>");
        }
        else
            AddDocString(pString, "        ", mOptions[j].mDescription);
    }

    if (!mEnumSpecs.empty())
    {
        if (helpType == kHelpHTML)
            pString->append("\n<p>Types:</p>");
        AddEnums(pString, "    ", mEnumSpecs, helpType);
    }

    if (helpType == kHelpHTML)
        pString->append("</blockquote></td></tr>");
}

const tChar* cArgSpec::HelpString(const tChar* commandName, tHelpType helpType) const
{
    CreateHelpString(commandName, &mErrorString, helpType);
    return mErrorString.c_str();
}

tArgError cArgSpec::ParseArgument(const cArgInfo& info, const tChar* arg)
{
    if (info.mFlagToSet >= 0)
        SetFlag(info.mFlagToSet);

    if (info.mType & kTypeArrayFlag)
    {
        tArgError err;

        tStrConstArray arrayArgs;
        Split(arg, &arrayArgs);

        static_cast<memblock*>(info.mLocation)->clear();

        for (int i = 0, n = arrayArgs.size(); i < n; i++)
            if ((err = ParseArrayArgument(info, arrayArgs[i])) != kArgNoError)
                return err;

        return kArgNoError;
    }

    switch (info.mType)
    {
    case kTypeBool:
        if (info.mLocation)
            *static_cast<bool*>   (info.mLocation) = (atoi(arg) != 0);
        return kArgNoError;
    case kTypeInt32:
        if (info.mLocation)
            *static_cast<int32_t*> (info.mLocation) = atoi(arg);
        return kArgNoError;
    case kTypeFloat:
        if (info.mLocation)
            *static_cast<float*>(info.mLocation) = float(atof(arg));
        return kArgNoError;
    case kTypeDouble:
        if (info.mLocation)
            *static_cast<double*>(info.mLocation) = atof(arg);
        return kArgNoError;

    case kTypeCString:
        if (info.mLocation)
            *static_cast<const tChar**>(info.mLocation) = arg;
        return kArgNoError;
    case kTypeString:
        if (info.mLocation)
            *static_cast<tString*>(info.mLocation) = arg;
        return kArgNoError;

    case kTypeVec2:
        if (info.mLocation)
        {
            float* v2 = (float*) info.mLocation;
            ParseVec2(arg, v2);
        }
        return kArgNoError;
    case kTypeVec3:
        if (info.mLocation)
        {
            float* v3 = (float*) info.mLocation;
            ParseVec3(arg, v3);
        }
        return kArgNoError;
    case kTypeVec4:
        if (info.mLocation)
        {
            float* v4 = (float*) info.mLocation;
            ParseVec4(arg, v4);
        }
        return kArgNoError;

    default:
        if (info.mType >= kTypeEnumBegin && size_t(info.mType - kTypeEnumBegin) < mEnumSpecs.size())
        {
            int enumIndex = info.mType - kTypeEnumBegin;
            uint32_t result;
            if (::ParseEnum(arg, mEnumSpecs[enumIndex].mEnumInfo, &result) == kArgNoError)
            {
                *static_cast<uint32_t*>(info.mLocation) = result;
                return kArgNoError;
            }
            else
            {
                Sprintf(&mErrorString, "Unknown enum '%s' of type %s", arg, mEnumSpecs[enumIndex].mName.c_str());
                return kArgErrorBadEnum;
            }
        }

        Sprintf(&mErrorString, "Unknown arg type %d", info.mType);
        return kArgErrorBadSpec;
    }
}

tArgError cArgSpec::ParseArrayArgument(const cArgInfo& info, const tChar* arg)
{
    switch (info.mType & kTypeBaseMask)
    {
    case kTypeBool:
        if (info.mLocation)
            static_cast<vector<bool>*>    (info.mLocation)->push_back((atoi(arg)) != 0);
        return kArgNoError;
    case kTypeInt32:
        if (info.mLocation)
            static_cast<vector<int32_t>*> (info.mLocation)->push_back(atoi(arg));
        return kArgNoError;
    case kTypeFloat:
        if (info.mLocation)
            static_cast<vector<float>*>   (info.mLocation)->push_back(float(atof(arg)));
        return kArgNoError;
    case kTypeDouble:
        if (info.mLocation)
            static_cast<vector<double>*>  (info.mLocation)->push_back(atof(arg));
        return kArgNoError;
    case kTypeCString:
        if (info.mLocation)
            static_cast<vector<tStrConst>*>(info.mLocation)->push_back(arg);
        return kArgNoError;
    case kTypeString:
        if (info.mLocation)
            static_cast<vector<tString>*> (info.mLocation)->push_back(arg);
        return kArgNoError;

    default:
        Sprintf(&mErrorString, "Unknown array argument type %d", info.mType);
        return kArgErrorBadSpec;
    }
}


tArgError cArgSpec::ParseOptionArgs(const vector<cArgInfo>& optArgs, const tChar**& argv, const tChar** argvEnd)
{
    tArgError error;
    size_t i = 0;
    size_t n = optArgs.size();

    while (argv < argvEnd && i < n && ((*argv)[0] != kOptionChar || !isalpha((*argv)[1])))
    {
        if ((i == n - 1) && (optArgs[i].mType & kTypeRepeatLastFlag))
        {
            static_cast<memblock*>(optArgs[i].mLocation)->clear();
            error = ParseArrayArgument(optArgs[i], (*argv));
        }
        else
        {
            error = ParseArgument(optArgs[i], (*argv));
            i++;
        }

        if (error != kArgNoError)
            return error;

        argv++;
    }

    if (i < n && optArgs[i].mIsDependent)
    {
        size_t numHave = i;

        while (i < n && optArgs[i].mIsDependent)
            i++;

        Sprintf(&mErrorString, "Not enough arguments: expecting at least %d more", i - numHave);

        return kArgErrorNotEnoughArgs;
    }

    return kArgNoError;
}

tArgError cArgSpec::ParseOption(const tChar**& argv, const tChar** argvEnd)
{
    const tChar* optionName = argv[0] + 1;

    argv++;

    if (mRespectHelp && (eq(optionName, "help") || eq(optionName, "h")))
        return kArgHelpRequested;

    for (size_t i = 0; i < mOptions.size(); i++)
        if (eq(mOptions[i].mName, optionName))
        {
            if (mOptions[i].mFlagToSet >= 0)
                SetFlag(mOptions[i].mFlagToSet);

            tArgError err = ParseOptionArgs(mOptions[i].mArguments, argv, argvEnd);

            if (err != kArgNoError)
            {
                mErrorString += " in -";
                mErrorString += optionName;
            }

            return err;
        }

        Sprintf(&mErrorString, "Unknown option '%s'", optionName);
        return kArgErrorUnknownOption;
}

tArgError cArgSpec::Parse(int argc, const tChar** argv)
{
    mErrorString = "no error";

    // chomp name.
    const tChar** argvEnd = argv + argc;
    const tChar* commandName = *argv++;

    // clear flags.
    mFlags = 0;

    tArgError result;
    size_t defaultArg = 0;
    size_t maxDefaultArgs = mDefaultArguments.size();

    while (argv < argvEnd)
    {
        if ((*argv)[0] == kOptionChar && isalpha((*argv)[1]))
        {
            result = ParseOption(argv, argvEnd);

            if (result != kArgNoError)
            {
                if (result == kArgHelpRequested)
                    CreateHelpString(commandName, &mErrorString);

                return result;
            }
        }
        else if (defaultArg < maxDefaultArgs)
        {
            if ((defaultArg + 1 == maxDefaultArgs) && (mDefaultArguments[defaultArg].mType & kTypeRepeatLastFlag))
            {
                result = ParseArrayArgument(mDefaultArguments[defaultArg], (*argv));
            }
            else
            {
                result = ParseArgument(mDefaultArguments[defaultArg++], *argv);
            }

            if (result != kArgNoError)
                return result;

            argv++;
        }
        else
        {
            Sprintf(&mErrorString, "Too many default arguments (expecting at most %d)\n", maxDefaultArgs);
            return kArgErrorTooManyArgs;
        }
    }

    if (defaultArg < maxDefaultArgs && mDefaultArguments[defaultArg].mIsDependent && !(mDefaultArguments[defaultArg].mType & kTypeRepeatLastFlag))
    {
        mErrorString = "Not enough arguments";
        return kArgErrorNotEnoughArgs;
    }

    return kArgNoError;
}


void cArgSpec::AddArgDocs(tString* pString, const vector<cArgInfo>& args, tHelpType helpType) const
{
    int numClauses = 0;

    if (helpType == kHelpHTML)
        pString->append("<i>");

    for (size_t i = 0; i < args.size(); i++)
    {
        const cArgInfo& ai = args[i];

        if (i != 0)
            pString->append(" ");

        if (!ai.mIsDependent)
        {
            pString->append("[");
            numClauses++;
        }

        if (helpType == kHelpHTML)
            pString->append("&lt;");
        else
            pString->append("<");

        if (ai.mName.empty())
            SprintfAppend(pString, "%s", NameFromArgType(ai.mType));
        else
            SprintfAppend(pString, "%s:%s", ai.mName.c_str(), NameFromArgType(ai.mType));

        if (helpType == kHelpHTML)
            pString->append("&gt;");
        else
            pString->append(">");

        if (ai.mType & kTypeRepeatLastFlag)
        {
            pString->append(" ");
            pString->append(kEllipsisToken);
        }
    }

    for (int i = 0; i < numClauses; i++)
        pString->append("]");

    if (helpType == kHelpHTML)
        pString->append("</i>");

    pString->append("\n");
}

void cArgSpec::FindNameAndTypeFromOption(const tString& str, tArgType* type, tString* name) const
{
    (*name) = "";

    if (str.empty())
    {
        *type = kTypeInvalid;
        return;
    }
    if (str.size() > 1 && str[0] == '%')
    {
        switch (str[1])
        {
        case 'f':
        case 'g':
            *type = kTypeFloat;
            break;
        case 'F':
        case 'G':
            *type = kTypeDouble;
            break;
        case 'd':
            *type = kTypeInt32;
            break;
        case 'b':
            *type = kTypeBool;
            break;
        case 's':
            *type = kTypeCString;
            break;
        default:
            *type = kTypeInvalid;
        }
    }
    else
    {
        tString typeStr(str);

        if (typeStr[0] == kBeginArgChar && typeStr[typeStr.size() - 1] == kEndArgChar)
        {
            typeStr.erase(typeStr.size() - 1, 1);
            typeStr.erase(0, 1);
        }

        size_t sepPos = typeStr.find(kArgSepChar);
        if (sepPos != tString::npos)
        {
            name->assign(typeStr, 0, sepPos);
            typeStr = tString(typeStr, sepPos + 1, tString::npos);
        }

        (*type) = ArgTypeFromName(typeStr.c_str());
    }
}

const tChar* cArgSpec::NameFromArgType(tArgType argType) const
{
    static tString sResult;

    int baseArgType = argType & kTypeBaseMask;

    switch (baseArgType)
    {
    case kTypeBool:
        sResult = "bool";
        break;
    case kTypeInt32:
        sResult = "int";
        break;
    case kTypeFloat:
        sResult = "float";
        break;
    case kTypeDouble:
        sResult = "double";
        break;
    case kTypeCString:
    case kTypeString:
        sResult = "string";
        break;

    case kTypeVec2:
        sResult = "vec2";
        break;
    case kTypeVec3:
        sResult = "vec3";
        break;
    case kTypeVec4:
        sResult = "vec4";
        break;

    case kTypeInvalid:
        sResult = "invalid";
        break;
    default:
        if (baseArgType >= kTypeEnumBegin && baseArgType < kTypeEnumEnd && size_t(baseArgType - kTypeEnumBegin) < mEnumSpecs.size())
            sResult = mEnumSpecs[baseArgType - kTypeEnumBegin].mName;
        else
            sResult = "unknown";
    }

    if ((argType & (kTypeArrayFlag | kTypeRepeatLastFlag)) == kTypeArrayFlag)
        sResult += "[]";

    return sResult.c_str();
}

tArgType cArgSpec::ArgTypeFromName(const tChar* typeName) const
{
    tArgType arrayFlag = tArgType(0);
    size_t typeLen = strlen(typeName);

    if (typeLen > 2 && typeName[typeLen - 2] == '[' && typeName[typeLen - 1] == ']')
    {
        arrayFlag = kTypeArrayFlag;
        typeLen -= 2;
    }

    if (Eq(typeName, "bool", typeLen))
        return kTypeBool | arrayFlag;
    if (Eq(typeName, "int", typeLen))
        return kTypeInt32 | arrayFlag;
    if (Eq(typeName, "float", typeLen))
        return kTypeFloat | arrayFlag;
    if (Eq(typeName, "double", typeLen))
        return kTypeDouble | arrayFlag;
    if (Eq(typeName, "string", typeLen))
        return kTypeString | arrayFlag;
    if (Eq(typeName, "cstr", typeLen) || Eq(typeName, "cstring", typeLen))
        return kTypeCString | arrayFlag;

    if (Eq(typeName, "vec2", typeLen) || Eq(typeName, "vector2", typeLen))
        return kTypeVec2 | arrayFlag;
    if (Eq(typeName, "vec3", typeLen) || Eq(typeName, "vector3", typeLen))
        return kTypeVec3 | arrayFlag;
    if (Eq(typeName, "vec4", typeLen) || Eq(typeName, "vector4", typeLen))
        return kTypeVec4 | arrayFlag;

    for (size_t i = 0; i < mEnumSpecs.size(); i++)
        if (Eq(mEnumSpecs[i].mName, typeName))
            return tArgType(kTypeEnumBegin + i) | arrayFlag;

    CL_ERROR("bad argument spec type\n");
    return kTypeInvalid;
}

