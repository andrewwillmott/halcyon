//
//  File:       CLLog.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLLog.h>

#include <CLString.h>
#include <CLValue.h>

#include <syslog.h>

namespace
{
    struct cLogGroupInfo
    {
        bool mActive = true;
    };

    nCL::map<const char*, cLogGroupInfo> sLogGroupInfoMap;

    bool sLogActive = false;
}

// syslog-based implementation for now.

bool nCL::InitLogSystem(const char* tag)
{
    if (!sLogActive)
    {
        if (!tag)
            tag = "CL";

//        openlog(tag, LOG_CONS | LOG_PERROR, LOG_USER);

        sLogActive = true;
        return true;
    }

    return false;
}

bool nCL::ShutdownLogSystem()
{
    if (sLogActive)
    {
//        closelog();
        sLogGroupInfoMap.clear();
        sLogActive = false;
        return true;
    }

    return false;
}

void nCL::ConfigLogSystem(const nCL::cObjectValue* config)
{
    sLogGroupInfoMap.clear();

    for (auto c : config->Children())
    {
        const char*   name = config.MemberName(i);
        const cValue& info = config.MemberValue(i);

        if (!info.IsObject())
            continue;

        cLogGroupInfo& groupInfo = sLogGroupInfoMap[name];

        groupInfo.mActive = info["active"].AsBool();
    }
}

bool nCL::OpenLogConsole()
{
    system("open /Applications/Utilities/Console.app");
    return true;
}

void nCL::Log(const char* group, const char* format, ...)
{
    auto it = sLogGroupInfoMap.find(group);

    if (it != sLogGroupInfoMap.end() && !it->second.mActive)
        return;

    va_list args;
    va_start(args, format);

    vsyslog(LOG_ERR, format, args);
    vprintf(format, args);      // TODO: if debugger attached

    va_end(args);
}

void nCL::LogData(const char* group, size_t dataSize, const uint8_t* data)
{
}

void nCL::LogImage8 (const char* group, int w, int h, const uint8_t* data)
{
}

void nCL::LogImage32(const char* group, int w, int h, const uint8_t* data)
{
}
