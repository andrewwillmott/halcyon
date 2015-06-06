//
//  File:       iOSAppDelegate.mm
//
//  Function:   App delegate for iOS version of HL
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import "iOSAppDelegate.h"

#import "iOSGLView.h"

#ifdef HL_HTTP_SERVER
    #import "HTTPServer.h"
    #import "DDLog.h"
    #import "DDTTYLogger.h"
    #import "DAVConnection.h"
#endif

#ifdef HL_USE_TEST_FLIGHT
    #import "../../../External/TestFlight/TestFlight.h"
#endif
#ifdef HL_USE_FLURRY
    #import "../../../External/Flurry/Flurry/Flurry.h"
#endif

#import "iOSFeedbackViewController.h"

#include <IHLApp.h>
#include <IHLConfigManager.h>

#include <HLServices.h>
#include <HLShell.h>

#include <CLDirectories.h>
#include <CLImage.h>
#include <CLLink.h>
#include <CLLog.h>
#include <CLMemory.h>
#include <CLValue.h>

#import <UIKit/UIDevice.h>
#import <CoreMotion/CoreMotion.h>

#ifdef HL_USE_LOCATION
    #import <CoreLocation/CoreLocation.h>
#endif


using namespace nHL;
using namespace nCL;

namespace
{
    void PrintEnvironment()
    {
        // No environ in iOS...
        NSDictionary* env = [[NSProcessInfo processInfo] environment];

        NSEnumerator* enumerator = [env keyEnumerator];
        id keyID;
         
        while ((keyID = [enumerator nextObject]))
        {
            NSString* varName = keyID;
            NSString* varValue = [env objectForKey: keyID];

            printf("%s = %s\n", [varName UTF8String], [varValue UTF8String]);
        }
    }

    void GetSystemInfo()
    {
        UIDevice* currentDevice = [UIDevice currentDevice];

        currentDevice.batteryMonitoringEnabled = YES;

        const char* deviceName    = [[currentDevice name] UTF8String];
        const char* deviceModel   = [[currentDevice model] UTF8String];
        const NSUUID* deviceUIID  = [currentDevice identifierForVendor];
        const char* deviceID      = [[deviceUIID UUIDString] UTF8String];
        
        const char* systemName    = [[currentDevice systemName] UTF8String];
        const char* systemVersion = [[currentDevice systemVersion] UTF8String];

        UIDeviceOrientation orientation = [currentDevice orientation];
        UIUserInterfaceIdiom uiIdiom = [currentDevice userInterfaceIdiom];

        UIDeviceBatteryState batteryState = [currentDevice batteryState];
        float batteryLevel = [currentDevice batteryLevel];

        const char* batteryStateStr = "unknown";

        switch (batteryState)
        {
        case UIDeviceBatteryStateUnplugged:
            batteryStateStr = "Unplugged";
            break;
        case UIDeviceBatteryStateCharging:
            batteryStateStr = "Charging";
            break;
        case UIDeviceBatteryStateFull:
            batteryStateStr = "Full";
            break;

        default:;
        }

        printf("System Info\n");
        printf("===========\n");
        printf("Device: %s\n", deviceName);
        printf("Model: %s\n", deviceModel);
        printf("DeviceID: %s\n", deviceID);
        printf("SystemName: %s\n", systemName);
        printf("SystemVersion: %s\n", systemVersion);
        printf("UI Idiom: %s\n", (uiIdiom == UIUserInterfaceIdiomPhone) ? "Phone" : "Pad");
        printf("Orientation: %d\n", orientation);
        printf("Battery: %s, %g\n", batteryStateStr, batteryLevel);
    }
}

// --- View controllers from XIB -----------------------------------------------
@interface RootViewController : UIViewController
@end

@implementation RootViewController

#if HL_NATIVE_ORIENT
- (BOOL) shouldAutorotate
{
    return NO;
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}
#endif

- (BOOL)prefersStatusBarHidden
{
    return YES;
}
@end


@interface WebViewController : UIViewController<UIWebViewDelegate>
    @property (nonatomic, retain) IBOutlet UIWebView* webView;
    @property (nonatomic, retain) IBOutlet UIBarButtonItem* backButton;
    @property (nonatomic, retain) IBOutlet UIBarButtonItem* forwardButton;
@end

@implementation WebViewController
@synthesize webView;

- (void) webViewDidStartLoad: (UIWebView*) w
{
    HL()->mApp->Pause();

    self.backButton   .enabled = (self.webView.canGoBack);
    self.forwardButton.enabled = (self.webView.canGoForward);
}

- (void) webViewDidFinishLoad: (UIWebView*) w
{
    self.backButton   .enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
}

#if HL_NATIVE_ORIENT
- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (BOOL) shouldAutorotate
{
    return NO;
}
#endif

- (IBAction) handleDone:(id) sender
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    HL()->mApp->Unpause();
}

- (IBAction) handleReload:(id) sender
{
    [self.webView reload];
}

- (IBAction) handleStop:(id) sender
{
    [self.webView stopLoading];
}

- (IBAction) handleBack:(id) sender
{
    [self.webView goBack];
}

- (IBAction) handleForward:(id) sender
{
    [self.webView goForward];
}

@end




// --- AppDelegate -------------------------------------------------------------

@implementation AppDelegate

@synthesize window;
@synthesize glView;
@synthesize mainViewController;
@synthesize webViewController;
@synthesize feedbackViewController;

#ifdef HL_HTTP_SERVER

- (void) SetupHTTPServer
{
	// Configure our logging framework.
	// To keep things simple and fast, we're just going to log to the Xcode console.
	[DDLog addLogger:[DDTTYLogger sharedInstance]];
	
	// Create server using our custom MyHTTPServer class
	mHTTPServer = [[HTTPServer alloc] init];
    [mHTTPServer setConnectionClass:[DAVConnection class]];
	
	// Tell the server to broadcast its presence via Bonjour.
	// This allows browsers such as Safari to automatically discover our service.
    const cObjectValue* config = HL()->mConfigManager->Config();
    const char* bonjourServerName = config->Member("serverName").AsString();
    if (bonjourServerName)
        [mHTTPServer setName:[NSString stringWithUTF8String: bonjourServerName]];
    else
        [mHTTPServer setName:@"Halcyon"];

	[mHTTPServer setType:@"_http._tcp."];   // register as HTTP so Safari can pick us up -- Finder doesn't show webdav bonjour addresses =P
//	[mHTTPServer setType:@"_webdav._tcp."];

	// Normally there's no need to run our server on any specific port.
	// Technologies like Bonjour allow clients to dynamically discover the server's port at runtime.
	// However, for easy testing you may want force a certain port so you can just hit the refresh button.
    int port = config->Member("serverPort").AsUInt(8080);
	[mHTTPServer setPort:port];

    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];

//	NSString* documentsDirectory = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Web"];
//	NSString* documentsDirectory = [[NSBundle mainBundle] resourcePath];

	CL_LOG("HTTP", "Setting document root: %s", [documentsDirectory UTF8String]);
	
	[mHTTPServer setDocumentRoot:documentsDirectory];
	
	// Start the server (and check for problems)
	
	NSError* error;

	if([mHTTPServer start:&error])
	{
		CL_LOG("HTTP", "Started HTTP Server on port %hu", [mHTTPServer listeningPort]);
	}
	else
	{
		CL_LOG("HTTP", "Error starting HTTP Server: %s", [[error localizedDescription] UTF8String]);
	}
}
#endif

#ifdef HL_USE_TEST_FLIGHT

- (void) SetupTestFlight
{
    NSBundle* mainBundle = [NSBundle mainBundle];

    NSString* apiKeyStr = [mainBundle objectForInfoDictionaryKey:@"TestFlightAPIKey"];
    if (apiKeyStr == nil)
        return;

    id version = [mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"];    // build #
    if (version != nil)
        [TestFlight addCustomEnvironmentInformation: version forKey: @"Version"];

    [TestFlight setOptions: @{ TFOptionReportCrashes : @YES } ];
    [TestFlight setOptions: @{ TFOptionReinstallCrashHandlers : @YES } ];

    [TestFlight takeOff: apiKeyStr];
}

#endif

#ifdef HL_USE_FLURRY
- (void) SetupFlurry: (NSDictionary*) launchOptions
{
    NSBundle* mainBundle = [NSBundle mainBundle];

    NSString* apiKeyStr = [mainBundle objectForInfoDictionaryKey:@"FlurryAPIKey"];
    if (apiKeyStr == nil)
        return;

    // [Flurry setAppVersion: @"123"]; // default comes from app bundle
//    [Flurry setBackgroundSessionEnabled: TRUE];   // Whether enabled when app is in the background
    [Flurry setCrashReportingEnabled: FALSE];
    [Flurry setDebugLogEnabled: FALSE];

#ifdef HL_USE_LOCATION
    CLLocationManager* locationManager = [[CLLocationManager alloc] init];
    [locationManager startUpdatingLocation];

    CLLocation* location = locationManager.location;

    [Flurry
        setLatitude:        location.coordinate.latitude
        longitude:          location.coordinate.longitude
        horizontalAccuracy: location.horizontalAccuracy
        verticalAccuracy:   location.verticalAccuracy
    ];
#endif

#ifdef CL_DEBUG
    NSString* flurryVersion = [Flurry getFlurryAgentVersion];
    printf("Flurry Version: %s\n", [flurryVersion UTF8String]); // Log system is not up yet
#endif

//    [Flurry setLogLevel: FlurryLogLevelDebug];
    [Flurry setSecureTransportEnabled: NO];
    [Flurry setSessionContinueSeconds: 10];  // 10 seconds timeout in background

#ifndef CL_RELEASE
    // TODO: I don't think this is kosher in ship builds? Private info?
    // Disabling to see if Flurry has a sensible default...
    // [Flurry setUserID: [[UIDevice currentDevice] name]];
#endif

    [Flurry startSession: apiKeyStr withOptions: launchOptions];
}
#endif

- (void) UpdateAppInfo
{
    NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
    [userDefaults synchronize];    // Ensure changes from settings are reflected locally

    cObjectValue* appInfo = HL()->mConfigManager->AppInfo();

#ifndef CL_RELEASE
    bool devMode = true;

    id devModeValue = [userDefaults objectForKey:@"developer_mode"];

    if (devModeValue)
        devMode = [userDefaults boolForKey:@"developer_mode"];
    else
    {
        NSNumber* devModeSetting = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"DevMode"];

        if (devModeSetting)
            devMode = [devModeSetting boolValue];
    }

    appInfo->InsertMember(CL_TAG("devMode")) = devMode;
    HL()->mApp->SetDevMode(devMode);

    bool gatherFeedback = [userDefaults boolForKey:@"gather_feedback"];
    appInfo->InsertMember(CL_TAG("gatherFeedback")) = gatherFeedback;
#endif

    //
    NSArray* appLanguages = [userDefaults objectForKey:@"AppleLanguages"];
    NSString* currentLanguage = [appLanguages objectAtIndex:0];

    appInfo->InsertMember(CL_TAG("language")) = [currentLanguage UTF8String];
}

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];
    NSBundle* mainBundle = [NSBundle mainBundle];
    const NSDictionary* appPList = [mainBundle infoDictionary];

    NSString* version = [appPList objectForKey:@"CFBundleVersion"];
    [userDefaults setObject:version forKey:@"version_preference"];  // Ensure this is up to date for settings display

    InitAllocators();
//    PrintEnvironment();

#ifdef HL_USE_TEST_FLIGHT
    [self SetupTestFlight];
#endif

#ifdef HL_USE_FLURRY
    // Do this early so we get crash reporting in place.
    [self SetupFlurry: launchOptions];
#endif

    ///////////////////////

    mMotionManager = [[CMMotionManager alloc] init];

    if ([mMotionManager isAccelerometerAvailable] == YES)
    {
        [mMotionManager setAccelerometerUpdateInterval: 1.0 / 60.0];

        [mMotionManager startAccelerometerUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:
            ^(CMAccelerometerData* accelerometerData, NSError* error)
            {
                float acc[3] =
                {
                    float(accelerometerData.acceleration.x),
                    float(accelerometerData.acceleration.y),
                    float(accelerometerData.acceleration.z)
                };

                [glView accelerated: acc];
            }
        ];
    }

    //////

    const char* resourcePath = [[mainBundle resourcePath] UTF8String];
    RegisterDirectory(kDirectoryResources, resourcePath);

    cFileSpec dataSpec;
    dataSpec.SetDirectory(resourcePath);
    dataSpec.AddDirectory("Data");
    RegisterDirectory(kDirectoryData, dataSpec.Path());

    const char* appPath = [[mainBundle executablePath] UTF8String];
    cFileSpec appSpec(appPath);
    RegisterDirectory(kDirectoryApp, appSpec.Directory());

    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    const char* documentsPath = [[paths objectAtIndex:0] UTF8String];
    RegisterDirectory(kDirectoryDocuments, documentsPath);

    GetSystemInfo();

    cIApp* app = CreateApp();

    HLServiceSetup()->mApp = app;

    window.rootViewController = self.mainViewController;
    // [glView setFrameBufferInfo: app];    // now happens as side-effect of layout caused by rootViewController.

    if (!app->Init())
        return NO;

#ifdef HL_HTTP_SERVER
    // Override point for customization after application launch.
	[self SetupHTTPServer];
#endif

    auto hl = HL();
    int vsyncFrames = 0;

    if (hl->mConfigManager)
    {
        vsyncFrames = hl->mConfigManager->Config()->Member("vsync").AsInt(2);

        cValue& appPrefs = hl->mConfigManager->Preferences()->InsertMember(CL_TAG("app"));

        NSString* versionString = [appPList objectForKey:@"CFBundleVersion"];
        const char* prefsStr = [versionString UTF8String];
        if (prefsStr)
            appPrefs.SetMember(CL_TAG("version"), cValue(prefsStr));

        NSString* buildNumberString = [appPList objectForKey:@"CFBuildNumber"];
        prefsStr = [buildNumberString UTF8String];
        if (prefsStr)
            appPrefs.SetMember(CL_TAG("buildNumber"), cValue(prefsStr));

        NSString* launchImageString = [appPList objectForKey:@"UILaunchImageFile"];
        prefsStr = [launchImageString UTF8String];
        if (prefsStr)
            appPrefs.SetMember(CL_TAG("launchImage"), cValue(prefsStr));
    }

    [glView setAnimationFrameInterval: vsyncFrames];
    [glView setMotionManager: mMotionManager];

    [window makeKeyAndVisible];

	[glView startRendering];

    float screenScale = [[window screen] scale];
    bool isRetina = screenScale > 1.0f;

    return YES;
}


- (void) presentGameCenter
{
    // Present game center UI
    GKGameCenterViewController* gameCenterController = [[GKGameCenterViewController alloc] init];

    if (gameCenterController)
    {
        gameCenterController.gameCenterDelegate = self;
        [self.mainViewController presentViewController:gameCenterController animated:YES completion:nil];
    }
}

- (void) gameCenterViewControllerDidFinish:(GKGameCenterViewController*) gameCenterViewController
{
    [self.mainViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void) showWebPage: (const char*) urlCStr
{
    if (!self.webViewController)
        return;

    self.webViewController.webView.scalesPageToFit = YES;

    NSString* urlString = [NSString stringWithUTF8String: urlCStr];
	[self.webViewController.webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString: urlString]]];

    [self.mainViewController
        presentViewController: webViewController
        animated:YES
        completion: nil
    ];

	[UIApplication sharedApplication].networkActivityIndicatorVisible = YES;
}



- (void) getUserFeedback: (const char*) identifier
{
    if (!feedbackViewController)
        return;

    self.feedbackViewController->mIdentifier = identifier;
    [self.mainViewController presentViewController: self.feedbackViewController animated: YES completion: nil];
}

namespace
{
    NSData* CreateCFDataFromImage(const cImage32& image)
    {
        vector<uint8_t> pngImageData;
        size_t pngImageSize = CompressImage(image, Allocator(kLocalAllocator), &pngImageData);

        if (pngImageData.empty())
            return 0;

        return [[NSData dataWithBytes: pngImageData.data() length: pngImageSize] retain];
    }
}

- (void) shareText: (const char*) text
{
    NSString* shareString = [NSString stringWithUTF8String: text];

    NSArray* activityItems = [NSArray arrayWithObjects:shareString, nil];
     
    UIActivityViewController* activityViewController = [[UIActivityViewController alloc]
        initWithActivityItems: activityItems
        applicationActivities: nil
    ];

    activityViewController.modalTransitionStyle = UIModalTransitionStyleCoverVertical;

    activityViewController.excludedActivityTypes = @[UIActivityTypePrint, UIActivityTypeCopyToPasteboard, UIActivityTypeSaveToCameraRoll];

    [self.mainViewController
        presentViewController: activityViewController
        animated: YES
        completion: nil
    ];

    [activityViewController release];
}

- (void) shareImage: (const cImage32&) image
{
    NSData* imageData = CreateCFDataFromImage(image);

    UIImage* shareImage = [UIImage imageWithData: imageData];
    [imageData release];
//    UIImage* shareImage = [UIImage imageNamed:@"drop.png"];

//    NSURL* shareUrl = [NSURL URLWithString:@"http://www.google.com"];

    NSArray* activityItems = [NSArray arrayWithObjects:shareImage, nil];

    UIActivityViewController* activityViewController = [[UIActivityViewController alloc]
        initWithActivityItems: activityItems
        applicationActivities: nil
    ];

    activityViewController.modalTransitionStyle = UIModalTransitionStyleCoverVertical;

//    activityViewController.excludedActivityTypes = @[UIActivityTypePrint, UIActivityTypeCopyToPasteboard, UIActivityTypeSaveToCameraRoll];

    [self.mainViewController
        presentViewController: activityViewController
        animated: YES
        completion: nil
    ];

    [activityViewController release];
}



#pragma mark -
#pragma mark App delegate hooks

- (void)applicationWillResignActive:(UIApplication *)application
{
	/*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */

	[glView stopRendering];
}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */

//    CL_ASSERT(![glView isRendering]);

}


- (void)applicationWillEnterForeground:(UIApplication *)application
{
    /*
     Called as part of transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */

}


- (void)applicationDidBecomeActive:(UIApplication *)application 
{
    [self UpdateAppInfo];

    [glView allTouchesCancelled];   // TODO: is this still necessary?
	[glView startRendering];
}


- (void)applicationWillTerminate:(UIApplication *)application
{
    /*
     Called when the application is about to terminate.
     See also applicationDidEnterBackground:.
     */
	
	[glView stopRendering];

    HL()->mApp->Shutdown();
    HLServiceSetup()->mApp = 0;

    if ([mMotionManager isAccelerometerActive] == YES)
    {
        [mMotionManager stopAccelerometerUpdates];
    }

    ShutdownAllocators();
}

- (BOOL) application: (UIApplication*) application
    openURL:            (NSURL*)    url
    sourceApplication:  (NSString*) sourceApplication
    annotation:         (id)        annotation
{
    // in this example, the URL from which the user came is http://example.com/profile/?12345
    // determine if the user was viewing a profile

    CL_LOG_I("App", "Got url %s from app %s\n", url.path.UTF8String, sourceApplication.UTF8String);

#if 0
    if ([[url path] isEqualToString:@"/profile"])
    {
        // switch to profile view controller
        [self.tabBarController setSelectedViewController:profileViewController];
        // pull the profile id number found in the query string
        NSString *profileID = [url query];
        // pass profileID to profile view controller
        [profileViewController loadProfile:profileID];
    }
#endif

    return YES;
}

#pragma mark -
#pragma mark Memory management

- (void) applicationDidReceiveMemoryWarning: (UIApplication*) application
{
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
}


- (void) dealloc
{
    [glView release];
    [window release];
    [super dealloc];
}

@end




// --- Utilities ---------------------------------------------------------------

bool nHL::ShellOpenWebView(const char* webView)
{
    AppDelegate* app = (AppDelegate*) [[UIApplication sharedApplication] delegate];
    [app showWebPage: webView];
    return true;
}

void nHL::ShellGatherUserFeedback(const char* identifier)
{
    AppDelegate* app = (AppDelegate*) [[UIApplication sharedApplication] delegate];
    [app getUserFeedback: identifier];
}

void nHL::ShellShareImage(const nCL::cImage32& image)
{
    AppDelegate* app = (AppDelegate*) [[UIApplication sharedApplication] delegate];
    [app shareImage: image];
}

void nHL::ShellShareText(const char* text)
{
    AppDelegate* app = (AppDelegate*) [[UIApplication sharedApplication] delegate];
    [app shareText: text];
}

// TODO: sharing
// UIImagePickerController -- pick picture
// UIActivityViewController -- share data (including save?)
