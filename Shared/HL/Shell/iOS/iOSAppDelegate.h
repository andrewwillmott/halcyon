//
//  File:       iOSAppDelegate.h
//
//  Function:   App delegate for iOS version of HL
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import <UIKit/UIKit.h>

#import <GameKit/GameKit.h>

@class EAGLView;
@class HTTPServer;
@class CMMotionManager;
@class TextViewController;
@class WebViewController;
@class FeedbackViewController;

@interface AppDelegate : NSObject <UIApplicationDelegate, GKGameCenterControllerDelegate>
{
	HTTPServer*         mHTTPServer;
    CMMotionManager*    mMotionManager;
}

- (void) presentGameCenter;
- (void) showWebPage: (const char*) url;
- (void) getUserFeedback: (const char*) identifier;

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain) IBOutlet EAGLView* glView;
@property (nonatomic, retain) IBOutlet UIViewController* mainViewController;
@property (nonatomic, retain) IBOutlet WebViewController* webViewController;
@property (nonatomic, retain) IBOutlet FeedbackViewController* feedbackViewController;

@end
