//
//  File:       OSXView.h
//
//  Function:   GLES implementation of a UIView
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>

// This has issues on OSX -- hardcoded to current display refresh rate for instance
//#define USE_DISPLAY_LINK

@interface NSGLView : NSOpenGLView
{
#ifdef USE_DISPLAY_LINK
	CVDisplayLinkRef mDisplayLink;
#else
    NSTimer* mRenderTimer;  // Use old-style timer-based method
    int mVSync;
#endif
}

- (void) startRendering;
- (void) stopRendering;

- (void) setVSync: (int) frames;

- (void) trackpadTouches: (NSData*) touchesData;

@end
