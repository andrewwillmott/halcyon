//
//  File:       OSXAppDelegate.cpp
//
//  Function:   App delegate for OSX-based Halcyon app.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import "OSXAppDelegate.h"

#define USE_TOUCHPAD

#import "OSXGLView.h"

#ifdef USE_TOUCHPAD
extern "C"
{
    #import "MultiTouch.h"
}
#include "TouchInfo.h"
#endif

#include <IHLApp.h>
#include <IHLConfigManager.h>
#include <IHLAVManager.h>

#include <HLServices.h>
#include <HLShell.h>

#include <CLDirectories.h>
#include <CLDispatch.h>
#include <CLImage.h>
#include <CLLink.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLValue.h>

using namespace nCL;
using namespace nHL;

extern char** environ;

namespace
{
    void PrintEnvironment()
    {
        char** envVars = environ;

        CL_LOG("Shell", "Environment Variables:\n");
        while (*envVars)
        {
            CL_LOG("Shell", "  %s\n", *envVars);
            envVars++;
        }
    }

#ifdef USE_TOUCHPAD

    cTouchesInfo sTouchesInfo;

    int OnTouchesCallback(int device, mtTouch* touches, int numFingers, double timestamp, int frame)
    {
        // create TouchPoint objects for all touches
        OSXAppDelegate* delegate = (OSXAppDelegate*)[NSApp delegate];

        sTouchesInfo.mDevice = device;
        sTouchesInfo.mFrame = frame;

        bool changed = sTouchesInfo.mNumTouches != numFingers;
        sTouchesInfo.mNumTouches = numFingers;

        for (int i = 0; i < numFingers; i++)
        {
            const mtTouch& touch = touches[i];

            cTouchInfo touchInfo;
            touchInfo.mID = touch.identifier;
            touchInfo.mPosition = Vec2f(touch.touch.position.x, touch.touch.position.y);

            changed = changed || (sTouchesInfo.mTouches[i] != touchInfo);
            sTouchesInfo.mTouches[i] = touchInfo;
        }

        if (!changed)
            return 0;

        NSData* touchesData = [NSData dataWithBytes: &sTouchesInfo length: sizeof(sTouchesInfo)];
        [touchesData retain];

        [delegate->mGLView performSelectorOnMainThread:@selector(trackpadTouches:) withObject:touchesData waitUntilDone:NO];

        // no idea what the return code should be, guessing 0 for success
        return 0;
    }
#endif
}


// --- OSXAppDelegate ----------------------------------------------------------

@implementation OSXAppDelegate

@synthesize mWindow;
@synthesize mGLView;

- (void) applicationWillFinishLaunching: (NSNotification*) notification
{
}

- (void) applicationDidFinishLaunching: (NSNotification*) notification
{
    InitAllocators();

//    PrintEnvironment();
    NSBundle* mainBundle = [NSBundle mainBundle];

    const char* resourcePath = [[mainBundle resourcePath] UTF8String];
    RegisterDirectory(kDirectoryResources, resourcePath);

    cFileSpec dataSpec;
    dataSpec.SetDirectory(resourcePath);
    dataSpec.AddDirectory("Data");
    RegisterDirectory(kDirectoryData, dataSpec.Path());

    const char* appPath = [[mainBundle executablePath] UTF8String];
    cFileSpec appSpec(appPath);
    RegisterDirectory(kDirectoryApp, appSpec.Directory());

    NSArray* docPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    const char* docsPath = [[docPaths objectAtIndex:0] UTF8String];

    // The docs directory isn't app-specific on OSX, so add a subdirectory for this particular app.
    cFileSpec appFileSpec(appPath);
    cFileSpec docsFileSpec;

    docsFileSpec.SetDirectory(docsPath);
    docsFileSpec.AddDirectory(appFileSpec.Name());
    docsFileSpec.EnsureDirectoryExists();
    docsPath = docsFileSpec.Path();

    RegisterDirectory(kDirectoryDocuments, docsPath);

    cIApp* app = CreateApp();

	NSRect rect = [mGLView bounds];    
    app->SetFrameBufferInfo(0, rect.size.width, rect.size.height);

    HLServiceSetup()->mApp = app;

    if (!app->Init())
    {
        [NSApp terminate:self];
        return;
    }

    auto hl = HL();

    auto avm = hl->mAVManager;

    if (avm)
    {
        NSRect frameRect = [mWindow frame];
        NSRect contentRect = [mWindow contentRectForFrameRect: frameRect];
        avm->SetWindowBounds(&contentRect.origin.x);
    }

    [mWindow setAcceptsMouseMovedEvents: TRUE];

#ifdef USE_TOUCHPAD
    // get a list of all multitouch devices
    NSArray* deviceList = (NSArray *)CFBridgingRelease(MTDeviceCreateList());

    for (int i = 0; i < [deviceList count]; i++)
    {
        // start sending touches to callback
        MTDeviceRef device = (__bridge MTDeviceRef)[deviceList objectAtIndex:i];
        MTRegisterContactFrameCallback(device, OnTouchesCallback);
        MTDeviceStart(device, 0);
    }
#endif

    NSString* appNameAsNSString = [NSString stringWithUTF8String: appFileSpec.Name()];
    [mWindow setTitle: appNameAsNSString];

    int vsyncFrames = 1;

    if (hl->mConfigManager)
    {
        vsyncFrames = hl->mConfigManager->Config()->Member("vsync").AsInt(1);

        NSString* versionString = [[mainBundle infoDictionary] objectForKey:@"CFBundleVersion"];
        const char* versionCStr = [versionString UTF8String];

        cValue& appPrefs = hl->mConfigManager->Preferences()->InsertMember(CL_TAG("app"));

        appPrefs.SetMember(CL_TAG("version"), cValue(versionCStr));
    }


    // TODO: setContentMinSize: and setContentMaxSize

    [mGLView setVSync: vsyncFrames];

    [mGLView startRendering];
}

- (void) applicationWillHide: (NSNotification*) notification
{
    [mGLView stopRendering];
}

- (void) applicationDidHide: (NSNotification*) notification
{
}

- (void) applicationWillUnhide: (NSNotification*) notification
{
}

- (void) applicationDidUnhide: (NSNotification*) notification
{
    [mGLView startRendering];
}

- (void) applicationWillBecomeActive: (NSNotification*) notification
{
}

- (void) applicationDidBecomeActive: (NSNotification*) notification
{
#ifdef USE_TOUCHPAD
    if (HL()->mConfigManager && HL()->mConfigManager->Preferences()->Member("useTrackpad").AsBool())
    {
        SetTouchpadEnabled(true);

        // Place cursor over our window so clicks don't switch to another app.
        CGPoint pos;
        pos.x = mWindow.frame.origin.x + (mWindow.frame.size.width / 2);
        pos.y = mWindow.frame.origin.y + (mWindow.frame.size.height / 2);

        CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, pos);
    }
#endif
}

- (void) applicationWillResignActive: (NSNotification*) notification
{
#ifdef USE_TOUCHPAD
    SetTouchpadEnabled(false);  // get our cursor back.
#endif
}

- (void) applicationDidResignActive: (NSNotification*) notification
{
}

- (void) applicationWillUpdate: (NSNotification*) notification
{
}

- (void) applicationDidUpdate: (NSNotification*) notification
{
}

- (void) applicationWillTerminate: (NSNotification*) notification
{
    [mGLView stopRendering];
    
    HL()->mApp->Shutdown();
    HLServiceSetup()->mApp = 0;
    ShutdownAllocators();
}

@end

bool nHL::ShellOpenWebView(const char* url)
{
    string command;

    command.format("open \"%s\"", url);
    int result = system(command.c_str());

    return result == 0;
}

void nHL::ShellGatherUserFeedback(const char* identifier)
{
    CL_ERROR("Unsupported");
}

void nHL::ShellShareImage(const nCL::cImage32& image)
{
    cFileSpec spec;
    SetDirectory(&spec, kDirectoryDocuments);

    spec.SetName("picture");
    spec.SetExtension("png");
    spec.MakeUnique();

    SaveImage(image, spec);
}

void nHL::ShellShareText (const char* text)
{
    cFileSpec spec;
    SetDirectory(&spec, kDirectoryDocuments);

    // TODO: cIOStreamFILE
}

