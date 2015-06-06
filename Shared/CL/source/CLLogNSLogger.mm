//
//  File:       CLLogNSLogger.cpp
//
//  Function:   Implementation of CLLog that uses NSLogger
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLLog.h>

#include <CLString.h>
#include <CLValue.h>

#include "Apple/LoggerClient.h"

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

using namespace nCL;

namespace
{
    struct cLogGroupInfo
    {
        bool mActive        = true;
        int  mLevel         = 0;

        bool mToLogger      = false;
        bool mToDebugger    = true;
    #ifdef HL_USE_TEST_FLIGHT
        bool mToTestFlight  = false;
    #endif

        const void Config(const cValue& config)
        {
            mActive       = config[CL_TAG("active"    )].AsBool(mActive);
            mLevel        = config[CL_TAG("logLevel"  )].AsInt (mLevel);

            mToLogger     = config[CL_TAG("logger"    )].AsBool(mToLogger);
            mToDebugger   = config[CL_TAG("debugger"  )].AsBool(mToDebugger);
        #ifdef HL_USE_TEST_FLIGHT
            mToTestFlight = config[CL_TAG("testFlight")].AsBool(mToTestFlight);
        #endif
        }
    };

    nCL::map<string, cLogGroupInfo> sLogGroupInfoMap;
    cLogGroupInfo sDefaultGroupInfo;

    bool sLogActive = false;

    inline const cLogGroupInfo* GroupInfo(const char* groupIn)
    {
        local_string group(groupIn);
        MakeLower(&group);

        auto it = sLogGroupInfoMap.find(group);

        if (it != sLogGroupInfoMap.end())
            return &(it->second);

        // If we have an LOG_EID suffix, specialise on the fly...
        const char* qualifier = strrchr(group.c_str(), '_');

        if (qualifier)
        {
            cLogGroupInfo groupInfo;
            local_string groupPrefix(group.c_str(), qualifier);

            it = sLogGroupInfoMap.find(groupPrefix);

            if (it != sLogGroupInfoMap.end())
                groupInfo = it->second;
            else
                groupInfo = sDefaultGroupInfo;

            qualifier++;

            if (eqi(qualifier, "debug"))
                groupInfo.mLevel = 3;
            else if (eqi(qualifier, "info"))
                groupInfo.mLevel = 2;
            else if (eqi(qualifier, "warning"))
                groupInfo.mLevel = 1;
            else if (eqi(qualifier, "error"))
            {
                groupInfo.mLevel = 0;
                groupInfo.mToDebugger = true;
            #ifdef HL_USE_TEST_FLIGHT
                groupInfo.mToTestFlight = true;
            #endif
            }

            cLogGroupInfo& insertedInfo = sLogGroupInfoMap[group];
            insertedInfo = groupInfo;
            return &insertedInfo;
        }

        return &sDefaultGroupInfo;
    }
}

void TFLogv_async(NSString* format, va_list arg_list);

void nCL::Log(const char* group, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    if (!sLogActive)
    {
        fprintf(stderr, "[InitLogSystem not called] ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }

    auto groupInfo = GroupInfo(group);

    if (!groupInfo->mActive)
        return;

    CFStringRef apiForcingACopyOfGroup  = 0;
    CFStringRef apiForcingACopyOfFormat = 0;

    if (groupInfo->mToLogger)
    {
        apiForcingACopyOfGroup  = CFStringCreateWithCString(0, group, kCFStringEncodingUTF8);
        apiForcingACopyOfFormat = CFStringCreateWithCString(0, format, kCFStringEncodingUTF8);

        LogMessageF_va(nullptr, 0, nullptr, // filename, line, function name
            (NSString*) apiForcingACopyOfGroup, groupInfo->mLevel,
            (NSString*) apiForcingACopyOfFormat, args
        );

        CFRelease(apiForcingACopyOfGroup);
        CFRelease(apiForcingACopyOfFormat);
    }

    if (groupInfo->mToDebugger)
        vprintf(format, args);

#ifdef HL_USE_TEST_FLIGHT
    if (groupInfo->mToTestFlight)
    {
        if (!apiForcingACopyOfFormat)
            apiForcingACopyOfFormat = CFStringCreateWithCString(0, format, kCFStringEncodingUTF8);
        TFLogv_async((NSString*) apiForcingACopyOfFormat, args);
    }
#endif

    va_end(args);
}

namespace
{
    NSData* CreateCFDataFromImage8(int w, int h, const uint8_t* data)
    {
        size_t pngImageSize;
        void* pngImageData = tdefl_write_image_to_png_file_in_memory(data, w, h, 1, &pngImageSize);

        if (!pngImageData)
            return 0;

        return [[NSData dataWithBytes: pngImageData length: pngImageSize] retain];
    }

    NSData* CreateCFDataFromImage32(int w, int h, const uint8_t* data)
    {
        size_t pngImageSize;
        void* pngImageData = tdefl_write_image_to_png_file_in_memory(data, w, h, 4, &pngImageSize);

        if (!pngImageData)
            return 0;

        return [[NSData dataWithBytes: pngImageData length: pngImageSize] retain];
    }
}

void nCL::LogData(const char* group, size_t dataSize, const uint8_t* data)
{
    CL_ASSERT_MSG(sLogActive, "InitLogSystem not called");      // if this is not the case, the logger will be auto-created with options we don't want.

    auto groupInfo = GroupInfo(group);

    if (!groupInfo->mActive || !groupInfo->mToLogger)
        return;

    CFStringRef nsGroup = CFStringCreateWithCString(0, group, kCFStringEncodingUTF8);
    CFDataRef   nsData  = CFDataCreate(0, data, dataSize);

    LogData((NSString*) nsGroup, groupInfo->mLevel, (NSData*) nsData);

    CFRelease(nsGroup);
    CFRelease(nsData);
}

void nCL::LogImage8 (const char* group, int w, int h, const uint8_t* data)
{
    CL_ASSERT_MSG(sLogActive, "InitLogSystem not called");      // if this is not the case, the logger will be auto-created with options we don't want.

    auto groupInfo = GroupInfo(group);

    if (!groupInfo->mActive || !groupInfo->mToLogger)
        return;

    NSData* nsData = CreateCFDataFromImage8(w, h, data);

    if (nsData)
    {
        CFStringRef nsGroup = CFStringCreateWithCString(0, group, kCFStringEncodingUTF8);
        LogImageData((NSString*) nsGroup, groupInfo->mLevel, w, h, (NSData*) nsData);
        CFRelease(nsGroup);
        CFRelease(nsData);
    }
}

void nCL::LogImage32(const char* group, int w, int h, const uint8_t* data)
{
    CL_ASSERT_MSG(sLogActive, "InitLogSystem not called");      // if this is not the case, the logger will be auto-created with options we don't want.

    auto groupInfo = GroupInfo(group);

    if (!groupInfo->mActive || !groupInfo->mToLogger)
        return;

    NSData* nsData = CreateCFDataFromImage32(w, h, data);

    if (nsData)
    {
        CFStringRef nsGroup = CFStringCreateWithCString(0, group, kCFStringEncodingUTF8);
        LogImageData((NSString*) nsGroup, groupInfo->mLevel, w, h, (NSData*) nsData);
        CFRelease(nsGroup);
        CFRelease(nsData);
    }
}

bool nCL::InitLogSystem(const char* tag)
{
    if (!sLogActive)
    {
		LoggerSetOptions
        (
            nullptr,   // configure the default logger
            kLoggerOption_BufferLogsUntilConnection | kLoggerOption_BrowseBonjour | kLoggerOption_BrowseOnlyLocalDomain
        #if !defined(CL_IOS) || TARGET_IPHONE_SIMULATOR
                | kLoggerOption_UseSSL    // Need this for a local connection, running into signing issues with Mavericks (spit) for device connection
        #endif
            // TODO: config option for kLoggerOption_UseSSL.
        );

        LoggerStart(nullptr);

        sLogActive = true;
        return true;
    }

    return false;
}

bool nCL::ShutdownLogSystem()
{
    if (sLogActive)
    {
        LoggerStop(nullptr);
        
        sLogGroupInfoMap.clear();
        sLogActive = false;
        return true;
    }

    return false;
}

void nCL::ConfigLogSystem(const nCL::cObjectValue* config)
{
    if (!config)
    {
        sLogGroupInfoMap.clear();
        sDefaultGroupInfo = cLogGroupInfo();
        return;
    }

    const cObjectValue* groupsConfig = config ? config->Member(CL_TAG("groups")).AsObject() : 0;

    if (groupsConfig)
    {
        sLogGroupInfoMap.clear();

        sDefaultGroupInfo = cLogGroupInfo();
        sDefaultGroupInfo.Config(groupsConfig->Member(CL_TAG("default")));

        for (auto c : groupsConfig->Children())
        {
            local_string name = c.Name();
            const cValue& info = c.Value();

            if (eqi(name, "default"))
                continue;

            MakeLower(&name);

            cLogGroupInfo& groupInfo = sLogGroupInfoMap[name];

            groupInfo = sDefaultGroupInfo;

            groupInfo.Config(info);
        }
    }

    const char* hostName = config->Member(CL_TAG("host")).AsString();

#if !defined(CL_IOS) || TARGET_IPHONE_SIMULATOR
    if (hostName)
    {
        int port = config->Member(CL_TAG("port")).AsUInt(50000);

        CFStringRef hostStr = CFStringCreateWithCString(0, hostName, kCFStringEncodingUTF8);
        LoggerSetViewerHost(nullptr, hostStr, port);
        CFRelease(hostStr);
    }
#endif

    const char* bonjourName = config->Member(CL_TAG("bonjour")).AsString();

    if (bonjourName)
    {
        CFStringRef hostStr = CFStringCreateWithCString(0, bonjourName, kCFStringEncodingUTF8);
        LoggerSetupBonjour(nullptr, 0, hostStr);
        CFRelease(hostStr);
    }
}

bool nCL::OpenLogConsole()
{
#ifdef CL_OSX
    system("open /Applications/NSLogger.app");
#endif
    return true;
}
