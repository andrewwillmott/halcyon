//
//  File:       CLLog.h
//
//  Function:   Group-based logging system for app output.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_LOG_H
#define CL_LOG_H

#include <CLDefs.h>

#ifdef CL_PRINTF_LOG
    #define CL_LOG_FUNC printf
#else
    #define CL_LOG_FUNC nCL::Log
#endif

#ifdef CL_DEBUG
    #define CL_LOG_ENABLED 1
    #define CL_LOG_D(M_GROUP, M_ARGS...) CL_LOG_FUNC(M_GROUP "_Debug", M_ARGS)

#elif defined(CL_DEVELOP)
    #define CL_LOG_ENABLED 1
    #define CL_LOG_D(M_GROUP, M_ARGS...)
#else
    #define CL_LOG_ENABLED 0
    #define CL_LOG_D(M_GROUP, M_ARGS...)
#endif

#if CL_LOG_ENABLED
    #define CL_LOG(M_GROUP, M_ARGS...) CL_LOG_FUNC(M_GROUP, M_ARGS)

    #define CL_LOG_E(M_GROUP, M_ARGS...) CL_LOG_FUNC(M_GROUP "_Error", M_ARGS)
    #define CL_LOG_I(M_GROUP, M_ARGS...) CL_LOG_FUNC(M_GROUP "_Info", M_ARGS)
#else
    #define CL_LOG(M_GROUP, M_ARGS...)

    #define CL_LOG_E(M_GROUP, M_ARGS...)
    #define CL_LOG_I(M_GROUP, M_ARGS...)
#endif


namespace nCL
{
    class cIOStream;
    class cObjectValue;

    bool InitLogSystem(const char* tag = 0);
    bool ShutdownLogSystem();

    void ConfigLogSystem(const nCL::cObjectValue* config); ///< Configure log system from the give value.

    bool OpenLogConsole();  ///< Bring up a window showing log output

    // Internal log routines used by CL_LOG macros
    void Log       (const char* group, const char* format, ...);
    void LogData   (const char* group, size_t dataSize, const uint8_t* data);
    void LogImage8 (const char* group, int w, int h, const uint8_t* data);
    void LogImage32(const char* group, int w, int h, const uint8_t* data);

    cIOStream* LogStream(const char* group);
    ///< Returns output stream for the given log group.
}

#endif
