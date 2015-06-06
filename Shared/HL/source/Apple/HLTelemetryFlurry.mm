//
//  File:       HLTelemetryFlurry.mm
//
//  Function:   Implementation of telemetry API using Flurry backend
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLTelemetry.h>

#include <CLValue.h>

#if defined(CL_IOS) && defined(HL_USE_FLURRY)

#import "../../../External/Flurry/Flurry/Flurry.h"

using namespace nHL;
using namespace nCL;

namespace
{
    NSDictionary* CreateDictionaryFromObject(const cObjectValue* object);

    NSObject* CreateNSObjectFromValue(const cValue& v)
    {
        switch (v.Type())
        {
        case kValueNull:
            return nil;
            break;
        case kValueBool:
            return [NSNumber numberWithBool: v.AsBool()];
            break;
        case kValueInt:
            return [NSNumber numberWithInt: v.AsInt()];
            break;
        case kValueUInt:
            return [NSNumber numberWithUnsignedInt: v.AsUInt()];
            break;
        case kValueDouble:
            return [NSNumber numberWithDouble: v.AsDouble()];
            break;
        case kValueString:
            return [NSString stringWithUTF8String: v.AsString()];
            break;
        case kValueArray:
            {
                NSMutableArray* array = [NSMutableArray arrayWithCapacity: v.NumElts()];
                for (int i = 0, n = v.NumElts(); i < n; i++)
                    [array insertObject: CreateNSObjectFromValue(v.Elt(i)) atIndex: i];
            }
            break;
        case kValueObject:
            return CreateDictionaryFromObject(v.AsObject());
            break;
        }
    }

    NSDictionary* CreateDictionaryFromObject(const cObjectValue* object)
    {
        NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithCapacity: object->NumMembers()];

        for (int i = 0, n = object->NumMembers(); i < n; i++)
        {
            const char* name = object->MemberName(i);
            const cValue& v  = object->MemberValue(i);

            NSString* key    = [NSString stringWithUTF8String: name];
            NSObject* object = CreateNSObjectFromValue(v);

            [dict setValue: object forKey: key];
        }

        return dict;
    }
}

void nHL::ReportEvent(const char* eventName, const cObjectValue* params)
{
    if (params)
        [Flurry logEvent: [NSString stringWithUTF8String: eventName] withParameters:CreateDictionaryFromObject(params)];
    else
        [Flurry logEvent: [NSString stringWithUTF8String: eventName]];
}

void nHL::ReportEventStart(const char* eventName, const cObjectValue* params)
{
    if (params)
        [Flurry logEvent:[NSString stringWithUTF8String: eventName] withParameters:CreateDictionaryFromObject(params) timed:YES];
    else
        [Flurry logEvent:[NSString stringWithUTF8String: eventName] timed:YES];
}

void nHL::ReportEventEnd(const char* eventName, const cObjectValue* params)
{
    if (params)
        [Flurry endTimedEvent:[NSString stringWithUTF8String: eventName] withParameters:nil];
    else
        [Flurry endTimedEvent:[NSString stringWithUTF8String: eventName] withParameters:nil];
}

void nHL::ReportError(const char* errorName, const char* errorMessage)
{
    [Flurry logError: [NSString stringWithUTF8String: errorName]
             message: [NSString stringWithUTF8String: errorMessage]
             error: /*(NSError *) */ nil
    ];
}

#else

void nHL::ReportEvent(const char* eventName, const cObjectValue* params)
{
}

void nHL::ReportEventStart(const char* eventName, const cObjectValue* params)
{
}

void nHL::ReportEventEnd(const char* eventName, const cObjectValue* params)
{
}

void nHL::ReportError(const char* errorName, const char* errorMessage)
{
}

#endif
