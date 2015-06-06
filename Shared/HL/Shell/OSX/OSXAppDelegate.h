//
//  File:       OSXAppDelegate.h
//
//  Function:   App delegate for OSX-based Halcyon app.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import <Cocoa/Cocoa.h>

@class NSGLView;

@interface OSXAppDelegate : NSObject <NSApplicationDelegate> 
{
@public;
    NSWindow* mWindow;
	NSGLView* mGLView;
}

@property (assign) IBOutlet NSWindow* mWindow;
@property (assign) IBOutlet NSGLView* mGLView;

@end
