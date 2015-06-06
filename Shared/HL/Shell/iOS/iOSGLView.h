//
//  File:       iOSGLView.h
//
//  Function:   GLES implementation of a UIView
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import <UIKit/UIKit.h>

#define HL_NATIVE_ORIENT 0

namespace nHL
{
    class cIApp;
}

@class CMMotionManager;

// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.

@interface EAGLView : UIView
{    
@private
    GLuint mDefaultFB;
    GLuint mColourRB;
    GLuint mDepthRB;
    int    mFBWidth;
    int    mFBHeight;
    float  mDisplayScale;

	EAGLContext* mContext;
	
	BOOL        animating;
	BOOL        displayLinkSupported;
	NSInteger   animationFrameInterval;
    
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	id          mDisplayLink;
    NSTimer*    mAnimationTimer;

    uint32_t    mOrientationsMask;
    nHL::cIApp* mApp;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;
@property (nonatomic, retain) CMMotionManager* motionManager;

- (void) startRendering;
- (void) stopRendering;

- (void) initFrameBuffer;
- (void) resizeFrameBuffer;
- (void) setFrameBufferInfo: (nHL::cIApp*) app;

- (void) drawView:(id)sender;
- (void) allTouchesCancelled;
- (void) orientationChanged:(NSNotification*) notification;
- (void) accelerated: (float[3]) acc;

@end
