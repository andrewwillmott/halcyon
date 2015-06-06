//
//  File:           CLTimer.cpp
//
//  Function:       Timing routines
//
//  Author(s):      Andrew Willmott
//
//  Copyright:      1995-2013
//

#include <CLTimer.h>

using namespace nCL;

#define CL_MACH_TIME
//#define CL_ANSI_TIME
//#define CL_RUSAGE_TIME

void cTimer::Start()
{
    mStartTime = AbsoluteTime();
    mLapTime = mStartTime;
}

void cTimer::Stop()  /* Stops timer */
{
    mStopTime = AbsoluteTime();
}

void cTimer::Continue()
{
    float deltaTime;

    deltaTime = AbsoluteTime();
    deltaTime -= mStopTime;

    mStartTime += deltaTime;
    mLapTime   += deltaTime;
}

float cTimer::GetTime() const
{
    return AbsoluteTime() - mStartTime;
}

float cTimer::DeltaTime()
{     
    float oldLapTime = mLapTime;
    
    mLapTime = AbsoluteTime();
    
    return mLapTime - oldLapTime;
}


/* --- Unix Time ----------------------------------------------------- */


#if defined(CL_POSIX_TIME)

/* Use the times() and gettimeofday() calls. */

#include <unistd.h>

#include <sys/times.h>
// for times
#include <sys/time.h>
// for gettimeofday

#include <errno.h>

#ifdef CL_POSIX_NO_SYSCONF
// this is obsolete, but may be supported where sysconf is not.
#include <time.h>
const long kClockTicks = CLK_TCK;
#else
const long kClockTicks = sysconf(_SC_CLK_TCK);
#endif


#if !defined(CL_ANSI_TIME) && !defined(CL_RUSAGE_TIME) && !defined(CL_MACH_TIME)
float cProgramTimer::AbsoluteTime()
{
    struct tms  tb;
    float       result;
    
    times(&tb);
    result = (float)(tb.tms_utime) / kClockTicks;
    if (mAddSystem)
        result += (float)(tb.tms_stime) / kClockTicks;

    return result;
}
#endif

float cWallClockTimer::AbsoluteTime() const
{
    struct  timeval tv;
    float   result;
    
    if (gettimeofday(&tv, 0))
        CL_ERROR("gettimeofday call failed: %d", errno);

    result = (tv.tv_sec & 0x0FFFF) + tv.tv_usec * 1e-6;

    return result;
}

#endif


/* --- Ansi Time ------------------------------------------------------ */


#ifdef CL_ANSI_TIME             /* Use clock() */

#include <time.h>

float cProgramTimer::AbsoluteTime() const
{
    //return clock() / double(CLOCKS_PER_SEC);
    return AbsoluteTicks() * 1e-9;
}

uint64_t nCL::AbsoluteTicks()
{
    return clock() * (UINT64_C(1000000000) / CLOCKS_PER_SEC);
}

#endif


// --- Use rusage call ---------------------------------------------------------


#ifdef CL_RUSAGE_TIME

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

float cProgramTimer::AbsoluteTime() const
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    float result = (usage.ru_utime.tv_sec & 0x0FFFF) + usage.ru_utime.tv_usec * 1e-6;
    if (mAddSystem)
        result += (usage.ru_stime.tv_sec & 0x0FFFF) + usage.ru_stime.tv_usec * 1e-6;

    return result;
}

uint64_t nCL::AbsoluteTicks()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    uint64_t result = UINT64_C(1000000000) * usage.ru_utime.tv_sec + UINT64_C(1000) * usage.ru_utime.tv_usec;
    return result;
}

#endif


#ifdef CL_MACH_TIME

#include <assert.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

namespace
{
    uint64_t sAbsoluteTimeBase = mach_absolute_time();
}

uint64_t nCL::AbsoluteTicks()
{
    mach_timebase_info_data_t timebaseInfo;     // TODO: when have internet access, check if this changes, and if not cache on startup
    mach_timebase_info(&timebaseInfo);

    uint64_t time = mach_absolute_time();

    return time * timebaseInfo.numer / timebaseInfo.denom;
}

float cProgramTimer::AbsoluteTime() const
{
    uint64_t absolute = mach_absolute_time() - sAbsoluteTimeBase;

    mach_timebase_info_data_t timebaseInfo;
    mach_timebase_info(&timebaseInfo);

    uint64_t nanoseconds64 = absolute * timebaseInfo.numer / timebaseInfo.denom;

    return 1e-9 * nanoseconds64;
}

#endif
