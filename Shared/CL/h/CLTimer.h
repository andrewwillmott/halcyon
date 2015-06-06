//
//  File:       CLTimer.h
//
//  Function:   Provides routines for timing under several different
//              architectures. You must define one of the following:
//
//                  CL_POSIX_TIME   Use the time() and times() calls
//                  CL_ANSI_TIME    Use the ansi clock() routine
//                  CL_RUSAGE_TIME  Use getrusage() system call
//
//              Compatibility flags include:
//                  CL_POSIX_NO_SYSCONF   There is no sysconf()
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1995-2001, Andrew Willmott
//

#ifndef CL_TIMER_H
#define CL_TIMER_H

#include <CLDefs.h>

namespace nCL
{
    class cTimer
    /// all times in seconds.
    {
    public:
        cTimer() : mStartTime(0.0), mStopTime(0.0), mLapTime(0.0) {}

        void    Start();        // Starts timer
        void    Stop();         // Stops timer
        void    Continue();     // Continue timer

        float   GetTime() const; // Returns time since timer was started

        float   DeltaTime();    // returns time delta since DeltaTime
                                // or StartTimer was last called

        virtual float AbsoluteTime() const = 0;

    protected:
        float   mStartTime;
        float   mStopTime;
        float   mLapTime;
    };


    class cProgramTimer : public cTimer
    /// Measures elapsed program time, ignoring time given up to other processes.
    {
    public:
        cProgramTimer() : cTimer(), mAddSystem(false) {}
        
        float   AbsoluteTime() const;

        bool    mAddSystem;
    };

    class cWallClockTimer : public cTimer
    /// Measures "wall clock" time.
    {
    public:
        float   AbsoluteTime() const;
    };

    // Utilities
    void UpdateMSPF(float s, float* mspf, float k = 0.1f);  ///< Utility for maintaining smoothed MSPF counters. E.g., UpdateMSPF(timer.GetTime(), &mMSPF).

    uint64_t AbsoluteTicks();   ///< Currently returns elapsed nanoseconds from e.g. program or system start. This gives up to 50 years of time at high precision.
}



// --- Inlines -----------------------------------------------------------------

inline void nCL::UpdateMSPF(float s, float* mspf, float k)
{
    *mspf += k * (s * 1000.0f - *mspf);
}

#endif
