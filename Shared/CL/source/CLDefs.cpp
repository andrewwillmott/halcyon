//
//  File:       CLArgSpec.cpp
//
//  Function:   Implements Defs.h
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1995-2014, Andrew Willmott
//

#include <CLDefs.h>

#include <CLLog.h>

#include <stdarg.h>
#include <time.h>


uint32_t CLUID()
{
    const time_t kJan2013TimeBase = 1356998400;

    return uint32_t(time(0) - 1356998400);
}

// --- Error functions for range and routine checking -------------------------

#if defined(CL_CHECKING) || defined(VL_CHECKING)

#ifndef CLDefaultError

void CLDefaultError(const char* errorMessage)
{
#ifdef CL_LOG_H
    CL_LOG("Debug", "*** %s\n", errorMessage);
#else
    fprintf(stderr, "*** %s\n", errorMessage);
#endif

#ifdef CL_NO_DEBUG_BREAK
    fprintf(stderr, "    Continue? [y/n] ");

    char reply = 'n';
    scanf("%c", &reply);

    if (reply != 'y')
        abort();
#else
    CL_DEBUG_BREAK();
#endif
}

#endif

void nCL::Error(const char* desc, const char* file, int line, const char* message, ...)
{
    char buffer[1024];
    char* p = buffer;
    va_list va;

    va_start(va, message);

#ifdef CL_HAS_VNSPRINTF
    p += snprintf (p, CL_SIZE(buffer), "%s (%s:%d): ", desc, file, line);
         vsnprintf(p, CL_SIZE(buffer), message, ap);
#else
    p += sprintf (p, "%s (%s:%d): ", desc, file, line);
        vsprintf(p, message, va);
#endif

    va_end(va);

    CLDefaultError(buffer);
}

void nCL::Trace(const char* desc, const char* file, int line, const char* message, ...)
{
    char buffer[1024];
    char* p = buffer;
    va_list va;

    va_start(va, message);

#ifdef CL_HAS_VNSPRINTF
    p += snprintf (p, CL_SIZE(buffer), "%s (%s:%d): ", desc, file, line);
         vsnprintf(p, CL_SIZE(buffer), message, ap);
#else
    p += sprintf (p, "%s (%s:%d): ", desc, file, line);
         vsprintf(p, message, va);
#endif

    va_end(va);

#ifdef CL_LOG_H
   CL_LOG("Debug", "%s", buffer);
#else
   fprintf(stderr, "%s", buffer);
#endif
}
#endif

#if defined(CL_DEBUG)
    #if (defined(CL_OSX) || (defined(CL_IOS) && TARGET_IPHONE_SIMULATOR))
        #include <assert.h>
        #include <stdbool.h>
        #include <sys/types.h>
        #include <unistd.h>
        #include <sys/sysctl.h>

        bool CLDebuggerAttached()
        // Returns true if the current process is being debugged (either
        // running under the debugger or has a debugger attached post facto).
        {
            int                 junk;
            int                 mib[4];
            struct kinfo_proc   info;
            size_t              size;

            // Initialize the flags so that, if sysctl fails for some bizarre 
            // reason, we get a predictable result.

            info.kp_proc.p_flag = 0;

            // Initialize mib, which tells sysctl the info we want, in this case
            // we're looking for information about a specific process ID.

            mib[0] = CTL_KERN;
            mib[1] = KERN_PROC;
            mib[2] = KERN_PROC_PID;
            mib[3] = getpid();

            // Call sysctl.

            size = sizeof(info);
            junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
            assert(junk == 0);

            // We're being debugged if the P_TRACED flag is set.

            return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
        }
    #endif
#endif
