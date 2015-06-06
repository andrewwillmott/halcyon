//
//  File:       HLTelemetry.h
//
//  Function:   API for logging telemetry events for later analysis
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_TELEMETRY_H
#define HL_TELEMETRY_H

#include <HLDefs.h>

namespace nHL
{
    void ReportEvent(const char* eventName, const cObjectValue* params = 0);
    ///< Logs the given event with optional parameters contained in 'params'.

    void ReportEventStart(const char* eventName, const cObjectValue* params = 0);
    void ReportEventEnd  (const char* eventName, const cObjectValue* params = 0);
    ///< Logs the start and end of some timed event.

    void ReportError(const char* errorName, const char* errorMessage = 0);
    ///< Logs an app error.
}

#endif
