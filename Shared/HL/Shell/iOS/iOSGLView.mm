//
//  File:       iOSGLView.cpp
//
//  Function:   GLES implementation of a UIView
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import "iOSGLView.h"

#include <IHLApp.h>
#include <IHLRenderer.h>
#include <HLServices.h>

#include <CLInputState.h>
#include <CLLog.h>

#include <GLConfig.h>

#import <QuartzCore/QuartzCore.h>

using namespace nHL;
using namespace nCL;

#if 0
    #define INPUT_INFO printf
#else
    #define INPUT_INFO(M_ARGS...)
#endif

#if !defined(CL_RELEASE) && GL_EXT_debug_label && defined(CL_IOS)
    #define GL_DEBUG_LABEL(M_TYPE, M_NAME, M_LABEL) glLabelObjectEXT(M_TYPE, M_NAME, 0, M_LABEL); 
#else
    #define GL_DEBUG_LABEL(M_TYPE, M_NAME, M_LABEL)
#endif

namespace
{
    const int kMaxTouches = cInputState::kMaxPointers;
}

@implementation EAGLView

@synthesize animating;
@dynamic animationFrameInterval;
@synthesize motionManager = mMotionManager;

// You must implement this method
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id) initWithCoder:(NSCoder*)coder
{    
    if ((self = [super initWithCoder:coder]))
	{
        // Basically if the true device width is below this, we take advantage
        // of the native retina display resolution. If it's not, we assume the fill would kill us and
        // thus default to the non-retina display equivalent.
        // TODO: make this tunable.
        const float kMaxFillWidth = 1024.0f;

        float resScale = [[UIScreen mainScreen] scale];
        CGSize viewSize = self.bounds.size;
        float minWidth = min(viewSize.width, viewSize.height); // despite the xibs being almost identical, iPad starts in portrait, iPhone in landscape
        float scaledWidth = minWidth * resScale;

        if (scaledWidth <= kMaxFillWidth)
        {
            self.contentScaleFactor = resScale;
            mDisplayScale = resScale;
        }
        else
            mDisplayScale = self.contentScaleFactor;

        // Get the layer
        CAEAGLLayer* eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

		mContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        
        if (!mContext || ![EAGLContext setCurrentContext:mContext])
		{
			[self release];
			return nil;
		}

        [self initFrameBuffer];

		animating = FALSE;
		displayLinkSupported = FALSE;
		animationFrameInterval = 2;
		mDisplayLink = nil;
		mAnimationTimer = nil;
		
		// A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
		// class is used as fallback when it isn't available.
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			displayLinkSupported = TRUE;

        self.multipleTouchEnabled = TRUE;

        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(orientationChanged:)
            name:UIDeviceOrientationDidChangeNotification
            object:nil
        ];

        // Getting UIKit's maze of twisty little delegate callbacks to function is an exercise
        // in futility, so do this ourselves.
        NSArray* supportedOrientations = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"UISupportedInterfaceOrientations"];

        mOrientationsMask = 0;

        if ([supportedOrientations containsObject:@"UIInterfaceOrientationPortrait"])
            mOrientationsMask |= 1 << UIInterfaceOrientationPortrait;
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationPortraitUpsideDown"])
            mOrientationsMask |= 1 << UIInterfaceOrientationPortraitUpsideDown;
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationLandscapeRight"])
            mOrientationsMask |= 1 << UIInterfaceOrientationLandscapeRight;
        if ([supportedOrientations containsObject:@"UIInterfaceOrientationLandscapeLeft"])
            mOrientationsMask |= 1 << UIInterfaceOrientationLandscapeLeft;

        if (mOrientationsMask == 0)
            mOrientationsMask = ~0;
    }

    return self;
}

#pragma mark === Render Management  ===

- (void) initFrameBuffer
{
	glGenFramebuffers(1, &mDefaultFB);
	glBindFramebuffer(GL_FRAMEBUFFER, mDefaultFB);
    GL_DEBUG_LABEL(GL_FRAMEBUFFER, mDefaultFB, "main");

	// Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
	glGenRenderbuffers(1, &mColourRB);
	glBindRenderbuffer(GL_RENDERBUFFER, mColourRB);
    GL_DEBUG_LABEL(GL_RENDERBUFFER, mColourRB, "mainColour");

	// This call associates the storage for the current render buffer with the EAGLDrawable (our CAEAGLLayer)
	// allowing us to draw into a buffer that will later be rendered to the screen wherever the layer is (which corresponds with our view).
	[mContext renderbufferStorage:GL_RENDERBUFFER fromDrawable: (id<EAGLDrawable>) self.layer];

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mColourRB);
	
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &mFBWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &mFBHeight);
	
	glGenRenderbuffers(1, &mDepthRB);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthRB);
    GL_DEBUG_LABEL(GL_RENDERBUFFER, mDepthRB, "mainDepth");

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mFBWidth, mFBHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRB);
	
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
        NSLog(@"Failed to make complete framebuffer object 0x%x", fbStatus);

	return self;
}

- (void) resizeFrameBuffer
{
	// The pixel dimensions of the CAEAGLLayer
    glBindFramebuffer(GL_FRAMEBUFFER, mDefaultFB);

	// Allocate color buffer backing based on the current layer size
    glBindRenderbuffer(GL_RENDERBUFFER, mColourRB);
    [mContext renderbufferStorage:GL_RENDERBUFFER fromDrawable: (CAEAGLLayer*) self.layer];
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &mFBWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &mFBHeight);
	
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthRB);
    GL_DEBUG_LABEL(GL_RENDERBUFFER, mDepthRB, "mainDepthResize");

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mFBWidth, mFBHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRB);

    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (fbStatus != GL_FRAMEBUFFER_COMPLETE)
        NSLog(@"Failed to make complete framebuffer object 0x%x", fbStatus);

    [self setFrameBufferInfo: HL()->mApp];

    return fbStatus != GL_FRAMEBUFFER_COMPLETE;
}

- (void) setFrameBufferInfo: (nHL::cIApp*) app;
{
    CL_LOG_I("Shell", "Device buffer size: %d x %d\n", mFBWidth, mFBHeight);
    app->SetFrameBufferInfo(mDefaultFB, mFBWidth, mFBHeight);
}

- (void) drawView:(id)sender
{
    if (mApp)
        mApp->Update();

    if (animating)
    {
        CL_ASSERT(mApp);

        [EAGLContext setCurrentContext:mContext];

        glBindFramebuffer(GL_FRAMEBUFFER, mDefaultFB);

        mApp->Render();
            
        glBindRenderbuffer(GL_RENDERBUFFER, mColourRB);
        [mContext presentRenderbuffer:GL_RENDERBUFFER];
    }
}

- (void) layoutSubviews
{
    [self resizeFrameBuffer];
    [self drawView:nil];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger) frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (animationFrameInterval == frameInterval)
        return;

    animationFrameInterval = frameInterval;
    
    if (animating)
    {
        [self stopRendering];
        [self startRendering];
    }
}

- (void) startRendering
{
    mApp = HL()->mApp;

    [self orientationChanged: nil];

	if (!animating)
	{
		if (displayLinkSupported)
		{
			// CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
			// if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
			// not be called in system versions earlier than 3.1.
			
			mDisplayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
			[mDisplayLink setFrameInterval:animationFrameInterval];
			[mDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		}
		else
			mAnimationTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)((1.0 / 60.0) * animationFrameInterval) target:self selector:@selector(drawView:) userInfo:nil repeats:TRUE];
		
		animating = TRUE;
	}
}

- (void)stopRendering
{
	if (animating)
	{
		if (displayLinkSupported)
		{
			[mDisplayLink invalidate];
			mDisplayLink = nil;
		}
		else
		{
			[mAnimationTimer invalidate];
			mAnimationTimer = nil;
		}
		
		animating = FALSE;
	}
}


#pragma mark === Touch handling  ===

namespace
{
    int mActiveTouchCount = 0;
    UITouch* mActiveTouches[kMaxTouches] = { 0 };

    int IndexForTouch(UITouch* touch)
    {
        for (int i = 0; i < mActiveTouchCount; i++)
            if (mActiveTouches[i] == touch)
                return i;

        return -1;
    }

    int AddTouch(UITouch* touch)
    {
        for (int i = 0; i < mActiveTouchCount; i++)
            if (!mActiveTouches[i])
            {
                mActiveTouches[i] = touch;
                return i;
            }

        mActiveTouches[mActiveTouchCount] = touch;
        return mActiveTouchCount++;
    }

    int RemoveTouch(UITouch* touch)
    {
        int i = IndexForTouch(touch);

        if (i >= 0)
        {
            mActiveTouches[i] = 0;

            while (mActiveTouchCount > 0 && !mActiveTouches[mActiveTouchCount - 1])
                mActiveTouchCount--;
        }
    }

    void ClearTouches()
    {
        mActiveTouchCount = 0;
        memset(mActiveTouches, 0, sizeof(mActiveTouches));
    }

    void AddTouches(NSSet* touches)
    {
        NSEnumerator* enumerator = [touches objectEnumerator];
        id touchID;
         
        while ((touchID = [enumerator nextObject]))
        {
            UITouch* touch = touchID;

            AddTouch(touch);
        }
    }

    void RemoveTouches(NSSet* touches)
    {
        NSEnumerator* enumerator = [touches objectEnumerator];
        id touchID;
         
        while ((touchID = [enumerator nextObject]))
        {
            UITouch* touch = touchID;

            RemoveTouch(touch);
        }
    }

    void DumpTouches(const char* state, NSSet* touches, UIEvent* event)
    {
        NSEnumerator* enumerator = [touches objectEnumerator];
        id touchID;
         
        CL_LOG_D("Debug", "%s touches:\n", state);

        while ((touchID = [enumerator nextObject]))
        {
            UITouch* touch = touchID;

            UIView* view = [touch view];
            CGPoint location = [touch locationInView: view];
            float x = location.x;
            float y = location.y;

            int index = IndexForTouch(touch);
            int count = [touch tapCount];

            CL_LOG_D("Debug", "  touch %d: %g %g, %d\n", index, x, y, count);
        }

        CL_LOG_D("Debug", "%s: allTouches\n", state);
        touches = [event allTouches];
    //    touches = [event touchesInView: self];
        enumerator = [touches objectEnumerator];
        
        while ((touchID = [enumerator nextObject]))
        {
            UITouch* touch = touchID;

            UIView* view = [touch view];
            CGPoint location = [touch locationInView: view];
            float x = location.x;
            float y = location.y;
            
            int index = IndexForTouch(touch);
            int count = [touch tapCount];

            CL_LOG_D("Debug", "  touch %d: %g %g, %d\n", index, x, y, count);
        }
    }
}

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    NSTimeInterval eventTimeStamp = [event timestamp];  // double
    uint64_t timeStampMS = lrint(eventTimeStamp * 1000.0);
    mApp->TimeStamp(timeStampMS);

    NSEnumerator* enumerator = [touches objectEnumerator];
    id touchID;

    INPUT_INFO("touchesBegan @ %llu:\n", timeStampMS);

    while ((touchID = [enumerator nextObject]))
    {
        UITouch* touch = touchID;

        AddTouch(touch);

        int index = IndexForTouch(touch);

        if (index < 0)  // could happen if we go above kMaxTouches
        {
            INPUT_INFO("  Excess touch %p: skipped\n", touch);
            continue;
        }

        CGPoint location = [touch locationInView: self];
        float x = location.x * mDisplayScale;
        float y = location.y * mDisplayScale;
        
        INPUT_INFO("  touchDown %d @ %g %g (count %d)\n", index, x, y, [touch tapCount]);

        mApp->PointerDown(index, x, y, 1);
    }

//    DumpTouches("BEGIN", touches, event);
}

- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
//    DumpTouches("MOVE", touches, event);
    NSTimeInterval eventTimeStamp = [event timestamp];  // double
    uint64_t timeStampMS = lrint(eventTimeStamp * 1000.0);
    mApp->TimeStamp(timeStampMS);
    
    NSEnumerator* enumerator = [touches objectEnumerator];
    id touchID;

    INPUT_INFO("touchesMove @ %llu:\n", timeStampMS);

    while ((touchID = [enumerator nextObject]))
    {
        UITouch* touch = touchID;

        int index = IndexForTouch(touch);

        if (index < 0)
        {
            INPUT_INFO("  Unrecognized touch %p: skipped\n", touch);
            continue;
        }

        CGPoint location = [touch locationInView: self];
        
        float x = location.x * mDisplayScale;
        float y = location.y * mDisplayScale;

        INPUT_INFO("  touchMove %d @ %g %g (count %d)\n", index, x, y, [touch tapCount]);

        mApp->PointerMove(index, x, y);
    }
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
//    DumpTouches("END", touches, event);

    //    INPUT_INFO("touchUp\n");
    // we only ever get one mouse-up event, when the last finger goes up

    NSTimeInterval eventTimeStamp = [event timestamp];  // double
    uint64_t timeStampMS = lrint(eventTimeStamp * 1000.0);
    mApp->TimeStamp(timeStampMS);

    NSEnumerator* enumerator = [touches objectEnumerator];
    id touchID;

    INPUT_INFO("touchesEnd @ %llu:\n", timeStampMS);

    while ((touchID = [enumerator nextObject]))
    {
        UITouch* touch = touchID;

        int index = IndexForTouch(touch);

        if (index < 0)
        {
            INPUT_INFO("  Unrecognized touch %p: skipped\n", touch);
            continue;
        }

        CGPoint location = [touch locationInView: self];
        
        float x = location.x * mDisplayScale;
        float y = location.y * mDisplayScale;

        INPUT_INFO("  touchUp %d @ %g %g (count %d)\n", index, x, y, [touch tapCount]);

        mApp->PointerUp(index, x, y, 1);
    }

    RemoveTouches(touches);
}

- (void) touchesCancelled: (NSSet*)touches withEvent: (UIEvent*)event;
{
    NSTimeInterval eventTimeStamp = [event timestamp];  // double
    uint64_t timeStampMS = lrint(eventTimeStamp * 1000.0);
    mApp->TimeStamp(timeStampMS);

    NSEnumerator* enumerator = [touches objectEnumerator];
    id touchID;

    INPUT_INFO("touchesCancelled @ %llu:\n", timeStampMS);

    while ((touchID = [enumerator nextObject]))
    {
        UITouch* touch = touchID;

        int index = IndexForTouch(touch);

        if (index < 0)
        {
            INPUT_INFO("  Unrecognized touch %p: skipped\n", touch);
            continue;
        }

        CGPoint location = [touch locationInView: self];
        
        float x = location.x * mDisplayScale;
        float y = location.y * mDisplayScale;

        INPUT_INFO("  touchCancelled %d @ %g %g (count %d)\n", index, x, y, [touch tapCount]);

        mApp->PointerUp(index, x, y, 1);
    }

    RemoveTouches(touches);
}

- (void) allTouchesCancelled
{
    INPUT_INFO("All touches cancelled!\n");

    ClearTouches();
    
    if (mApp)
        mApp->PointersCancel();
}


- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
    // motion = UIEventSubtypeMotionShake for now
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
}

- (void)motionCancelled:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
}

- (void)remoteControlReceivedWithEvent:(UIEvent *)event
{
    // UIEventSubtype action = event.subtype;
    // UIEventSubtypeRemoteControlPlay etc.
}


- (void) accelerated: (float[3]) acc
{
    // We're supposed to leave filtering to CoreMotion
    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;

    float accRot[3];

    switch (deviceOrientation)
    {
    case UIDeviceOrientationPortrait:
        break;
    case UIDeviceOrientationPortraitUpsideDown:
        accRot[0] = -acc[0];
        accRot[1] = -acc[1];
        accRot[2] =  acc[2];
        acc = accRot;
        break;
    case UIDeviceOrientationLandscapeLeft:
        accRot[0] = -acc[1];
        accRot[1] = +acc[0];
        accRot[2] =  acc[2];
        acc = accRot;
        break;
    case UIDeviceOrientationLandscapeRight:
        accRot[0] = +acc[1];
        accRot[1] = -acc[0];
        accRot[2] =  acc[2];
        acc = accRot;
        break;
    default:
        break;
    }

    HL()->mApp->Acceleration(acc);
}

- (void) orientationChanged:(NSNotification*) notification
{
#if HL_NATIVE_ORIENT
    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;

    if ((mOrientationsMask & (1 << deviceOrientation)) == 0)
        return;

    int orientation = deviceOrientation - 1;
    if (orientation >= 0 && orientation < 4)
        if (mApp)
            mApp->Orientation(tDeviceOrientation(orientation));
#endif
}


- (void) dealloc
{
	if (mDefaultFB)
	{
		glDeleteFramebuffers(1, &mDefaultFB);
		mDefaultFB = 0;
	}
	
	if (mColourRB)
	{
		glDeleteRenderbuffers(1, &mColourRB);
		mColourRB = 0;
	}

    if (mDepthRB)
    {
		glDeleteRenderbuffers(1, &mDepthRB);
		mDepthRB = 0;
    }

	// tear down context
	if ([EAGLContext currentContext] == mContext)
        [EAGLContext setCurrentContext:nil];
	
	[mContext release];
	
    [super dealloc];
}

@end
