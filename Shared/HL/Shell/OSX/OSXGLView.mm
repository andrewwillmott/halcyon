//
//  File:       OSXView.cpp
//
//  Function:   GLView based off NSOpenGLView
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#import "OSXGLView.h"

#import "GLConfig.h"

#include "TouchInfo.h"

#include <IHLApp.h>
#include <IHLAVManager.h>
#include <IHLConfigManager.h>

#include <HLServices.h>

#include <CLInputState.h> // for keycodes
#include <CLTimer.h>
#include <CLValue.h>

using namespace nCL;
using namespace nHL;

#if 0
    #define INPUT_INFO(M_ARGS...) printf(M_ARGS)
#else
    #define INPUT_INFO(M_ARGS...)
#endif

namespace
{
}


@interface NSGLView (PrivateMethods)
- (void) initGL;
- (void) drawView;
@end

@implementation NSGLView

#ifdef USE_DISPLAY_LINK
- (CVReturn) drawNextFrame:(const CVTimeStamp*)outputTime
{
	// There is no autorelease pool when this method is called 
	// because it will be called from a background thread
	// It's important to create one or you will leak objects
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	[self drawView];
	
	[pool release];
	return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CVReturn result = [(NSGLView*) displayLinkContext drawNextFrame:outputTime];
    return result;
}
#endif

- (void) awakeFromNib
{
    NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize, 24,
		// Must specify the 3.2 Core Profile to use OpenGL 3.2
    #ifdef CL_USE_GL3
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
    #endif
		0
	};
	
	NSOpenGLPixelFormat *pf = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs] autorelease];
	
	if (!pf)
	{
		NSLog(@"No OpenGL pixel format");
	}
    
    NSOpenGLContext* context = [[[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil] autorelease];

    [self setPixelFormat:pf];
    
    [self setOpenGLContext:context];
}

- (void) prepareOpenGL
{
	[super prepareOpenGL];
	
	// Make all the OpenGL calls to setup rendering  
	//  and build the necessary rendering objects
	[self initGL];

#ifdef USE_DISPLAY_LINK
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	
	// Set the renderer output callback function. This will get called immediately after the last frame has been presented.
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, self);
	
	// Set the display link for the current renderer
	CGLContextObj     cglContext     = (CGLContextObj)     [[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj) [[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
#else


#endif
}

- (void) initGL
{
	// Make this openGL context current to the thread
	// (i.e. all openGL on this thread calls will go to this context)
	[[self openGLContext] makeCurrentContext];
}

- (void) reshape
{	
	[super reshape];
	
	// We may draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main thread
	// Add a mutex around to avoid the threads accessing the context simultaneously when resizing
	CGLLockContext((CGLContextObj) [[self openGLContext] CGLContextObj]);
	
	NSRect rect = [self bounds];

    if (HL()->mApp)
        HL()->mApp->SetFrameBufferInfo(0, rect.size.width, rect.size.height);

	CGLUnlockContext((CGLContextObj) [[self openGLContext] CGLContextObj]);
}

- (void) drawView
{
    cIApp* app = HL()->mApp;

    if (!app)
        return;

    cProgramTimer timer;
    timer.Start();

    if (HL()->mConfigManager)
        SetTouchpadEnabled(HL()->mConfigManager->Preferences()->Member("useTrackpad").AsBool());

    auto avm = HL()->mAVManager;

    if (avm)
    {
        NSWindow* window = [self window];
        NSRect frameRect   = [window frame];
        NSRect contentRect = [window contentRectForFrameRect: frameRect];
        avm->SetWindowBounds(&contentRect.origin.x);
    }

    app->Update();

	[[self openGLContext] makeCurrentContext];

	// We may draw on a secondary thread through the display link
	// When resizing the view, -reshape is called automatically on the main thread
	// Add a mutex around to avoid the threads accessing the context simultaneously	when resizing
	CGLLockContext((CGLContextObj) [[self openGLContext] CGLContextObj]);

    app->Render();

	CGLFlushDrawable((CGLContextObj) [[self openGLContext] CGLContextObj]);
	CGLUnlockContext((CGLContextObj) [[self openGLContext] CGLContextObj]);

    float deltaTMS = timer.GetTime() * 1000.0f;
    float sleepTimeMS = mVSync * 16.6666f - deltaTMS - 2.0f ;

    if (sleepTimeMS > 0.0f)
        usleep(int(sleepTimeMS * 1000.0f));
}

- (void) startRendering
{
#ifdef USE_DISPLAY_LINK
    if (displayLink && !CVDisplayLinkIsRunning(displayLink))
        CVDisplayLinkStart(displayLink);
#else
    if (!mRenderTimer)
    {
        mRenderTimer =
            [NSTimer
                timerWithTimeInterval:0.001   // Very small interval so we
                target:self
                selector:@selector(frameTimerFired:)
                userInfo:nil
                repeats:YES
            ];
        [mRenderTimer retain];

        [[NSRunLoop currentRunLoop]
            addTimer:mRenderTimer
            forMode:NSDefaultRunLoopMode
        ];

        //Ensure timer fires during resize
        [[NSRunLoop currentRunLoop]
            addTimer:mRenderTimer
            forMode:NSEventTrackingRunLoopMode
        ];
    }
#endif
}

- (void) stopRendering
{
#ifdef USE_DISPLAY_LINK
    if (displayLink && CVDisplayLinkIsRunning(displayLink))
        CVDisplayLinkStop(displayLink);
#else
    if (mRenderTimer)
    {
        [mRenderTimer invalidate];
        [mRenderTimer release];
        mRenderTimer = 0;
    }
#endif
}

-(void) setVSync: (int) frames
{
    mVSync = frames;

	GLint swapInt = frames;
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

- (void)frameTimerFired:(id)sender
{
    // It is good practice in a Cocoa application to allow the system to send the -drawRect:
    // message when it needs to draw, and not to invoke it directly from the timer.
    // All we do here is tell the display it needs a refresh
    if (mVSync > 0)
        [self setNeedsDisplay:YES];
    else
        // However, doing it that way forces a form of vsync =P, so call directly here if vsync is off...
        [self drawView];
}

- (void)drawRect:(NSRect) rect
{
    [self drawView];
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
#ifdef USE_DISPLAY_LINK
    if (!newWindow)
    {
        if (CVDisplayLinkIsRunning(displayLink))
            CVDisplayLinkStop(displayLink);
    }
#endif
}

- (void)viewDidMoveToWindow
{
#ifdef USE_DISPLAY_LINK
    if (displayLink && [self window])
    {
        if (!CVDisplayLinkIsRunning(displayLink))
            CVDisplayLinkStart(displayLink);
    }
#endif
}

- (void)viewWillMoveToSuperview:(NSView *)newSuperview
{
}

- (void)viewDidMoveToSuperview
{
}

- (void) dealloc
{	
	// Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been release
#ifdef USE_DISPLAY_LINK
	CVDisplayLinkStop(displayLink);
	CVDisplayLinkRelease(displayLink);
#else
    [self stopRendering];
#endif

	[super dealloc];
}




// --- Event management --------------------------------------------------------

namespace
{
    void TimeStamp(NSEvent* event)
    {
        NSTimeInterval eventTimeStamp = [event timestamp];  // double
        uint64_t timeStampMS = lrint(eventTimeStamp * 1000.0);
        HL()->mApp->TimeStamp(timeStampMS);
    }
}

- (void)mouseDown:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;

    HL()->mApp->PointerDown(0, p.x, p.y, button);
}

- (void)rightMouseDown:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;
    HL()->mApp->PointerDown(0, p.x, p.y, button);
}

- (void)otherMouseDown:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;
    HL()->mApp->PointerDown(0, p.x, p.y, button);
}

- (void)mouseUp:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;

    HL()->mApp->PointerUp(0, p.x, p.y, button);
}

- (void)rightMouseUp:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;
    HL()->mApp->PointerUp(0, p.x, p.y, button);
}

- (void)otherMouseUp:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;
    int button = (int) [event buttonNumber] + 1;
    HL()->mApp->PointerUp(0, p.x, p.y, button);
}

- (void)mouseMoved:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    // If you are not receiving calls here, you need to call setAcceptsMouseMovedEvents: TRUE
    // on the parent window, or set the corresponding flag in the xib.
    
    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;

    INPUT_INFO("mouseMoved: %g %g\n", p.x, p.y);

    NSUInteger modifiers = [event modifierFlags];

    HL()->mApp->PointerMove(0, p.x, p.y);
}

- (void)mouseDragged:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;

    INPUT_INFO("mouseDragged: %g %g\n", p.x, p.y);

    HL()->mApp->PointerMove(0, p.x, p.y);
}

- (void)rightMouseDragged:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;

    HL()->mApp->PointerMove(0, p.x, p.y);
}

- (void)otherMouseDragged:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    TimeStamp(event);

    NSPoint p = [self convertPoint: [event locationInWindow] fromView: nil];
    p.y = [self frame].size.height - p.y - 1;

    HL()->mApp->PointerMove(0, p.x, p.y);
}

- (void)scrollWheel:(NSEvent*) event
{
    if (TouchpadEnabled())
        return;

    float x  = [event scrollingDeltaX];
    float y  = [event scrollingDeltaY];

    if (![event hasPreciseScrollingDeltas])
    {
        // TODO: what are reasonable values?
        x *= 16.0f;
        y *= 16.0f;
    }

    HL()->mApp->ScrollDelta(x, y);
}

// Keys

// Ensure we get key events
- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (BOOL) resignFirstResponder
{
    return YES;
}

- (BOOL) becomeFirstResponder
{
    return YES;
}

namespace
{
    // The source constants (kVK_*, = virtual keyboard) are from HIToolbox/Events.h
    // We translate them to an OS-neutral keycode which actually has e.g., kVK_ANSI_A == 'A'.
    const int kKeyCodeTable[128] =
    {
        'A',     		// kVK_ANSI_A                   = 0x00,
        'S',     		// kVK_ANSI_S                   = 0x01,
        'D',     		// kVK_ANSI_D                   = 0x02,
        'F',     		// kVK_ANSI_F                   = 0x03,
        'H',     		// kVK_ANSI_H                   = 0x04,
        'G',     		// kVK_ANSI_G                   = 0x05,
        'Z',     		// kVK_ANSI_Z                   = 0x06,
        'X',     		// kVK_ANSI_X                   = 0x07,
        'C',     		// kVK_ANSI_C                   = 0x08,
        'V',     		// kVK_ANSI_V                   = 0x09,
        0,
        'B',     		// kVK_ANSI_B                   = 0x0B,
        'Q',     		// kVK_ANSI_Q                   = 0x0C,
        'W',     		// kVK_ANSI_W                   = 0x0D,
        'E',     		// kVK_ANSI_E                   = 0x0E,
        'R',     		// kVK_ANSI_R                   = 0x0F,
        'Y',     		// kVK_ANSI_Y                   = 0x10,
        'T',     		// kVK_ANSI_T                   = 0x11,
        '1',     		// kVK_ANSI_1                   = 0x12,
        '2',     		// kVK_ANSI_2                   = 0x13,
        '3',     		// kVK_ANSI_3                   = 0x14,
        '4',     		// kVK_ANSI_4                   = 0x15,
        '6',     		// kVK_ANSI_6                   = 0x16,
        '5',     		// kVK_ANSI_5                   = 0x17,
        '=',     		// kVK_ANSI_Equal               = 0x18,
        '9',     		// kVK_ANSI_9                   = 0x19,
        '7',     		// kVK_ANSI_7                   = 0x1A,
        '-',     		// kVK_ANSI_Minus               = 0x1B,
        '8',     		// kVK_ANSI_8                   = 0x1C,
        '0',     		// kVK_ANSI_0                   = 0x1D,
        ']',     		// kVK_ANSI_RightBracket        = 0x1E,
        'O',     		// kVK_ANSI_O                   = 0x1F,
        'U',     		// kVK_ANSI_U                   = 0x20,
        '[',     		// kVK_ANSI_LeftBracket         = 0x21,
        'I',     		// kVK_ANSI_I                   = 0x22,
        'P',        	// kVK_ANSI_P                   = 0x23,
        kReturnKey, 	// kVK_Return                   = 0x24,
        'L',     		// kVK_ANSI_L                   = 0x25,
        'J',     		// kVK_ANSI_J                   = 0x26,
        '\'',    		// kVK_ANSI_Quote               = 0x27,
        'K',     		// kVK_ANSI_K                   = 0x28,
        ';',     		// kVK_ANSI_Semicolon           = 0x29,
        '\\',    		// kVK_ANSI_Backslash           = 0x2A,
        ',',     		// kVK_ANSI_Comma               = 0x2B,
        '/',     		// kVK_ANSI_Slash               = 0x2C,
        'N',     		// kVK_ANSI_N                   = 0x2D,
        'M',     		// kVK_ANSI_M                   = 0x2E,
        '.',     		// kVK_ANSI_Period              = 0x2F,
        kTabKey,        // kVK_Tab                      = 0x30,
        kSpaceKey,    	// kVK_Space                    = 0x31,
        '`',      		// kVK_ANSI_Grave               = 0x32,
        kDeleteKey,     // kVK_Delete                   = 0x33,
        0,
        kEscapeKey,     // kVK_Escape                   = 0x35,
        0,
        kLeftCommandKey,// kVK_Command                  = 0x37,
        kLeftShiftKey,  // kVK_Shift                    = 0x38,
        kCapsLockKey,   // kVK_CapsLock                 = 0x39,
        kLeftAltKey,    // kVK_Option                   = 0x3A,
        kLeftControlKey,  // kVK_Control                = 0x3B,
        kRightShiftKey, // kVK_RightShift               = 0x3C,
        kRightAltKey,   // kVK_RightOption              = 0x3D,
        kRightControlKey, // kVK_RightControl           = 0x3E,
        kFunctionKey,   // kVK_Function                 = 0x3F,
        kF17,           // kVK_F17                      = 0x40,
        kKeyPadPeriod,  // kVK_ANSI_KeypadDecimal       = 0x41,
        0,
        kKeyPadMultiply,// kVK_ANSI_KeypadMultiply      = 0x43,
        0,
        kKeyPadPlus,  	// kVK_ANSI_KeypadPlus          = 0x45,
        0,
        0,              // kVK_ANSI_KeypadClear         = 0x47,
        kVolumeUpKey,   // kVK_VolumeUp                 = 0x48,
        kVolumeDownKey, // kVK_VolumeDown               = 0x49,
        kVolumeMuteKey, // kVK_Mute                     = 0x4A,
        kKeyPadDivide,	// kVK_ANSI_KeypadDivide        = 0x4B,
        kKeyPadEnter, 	// kVK_ANSI_KeypadEnter         = 0x4C,
        0,
        kKeyPadMinus, 	// kVK_ANSI_KeypadMinus         = 0x4E,
        kF18,         	// kVK_F18                      = 0x4F,
        kF19,         	// kVK_F19                      = 0x50,
        kKeyPadEquals,  // kVK_ANSI_KeypadEquals        = 0x51,
        kKeyPad0,     	// kVK_ANSI_Keypad0             = 0x52,
        kKeyPad1,     	// kVK_ANSI_Keypad1             = 0x53,
        kKeyPad2,     	// kVK_ANSI_Keypad2             = 0x54,
        kKeyPad3,     	// kVK_ANSI_Keypad3             = 0x55,
        kKeyPad4,     	// kVK_ANSI_Keypad4             = 0x56,
        kKeyPad5,     	// kVK_ANSI_Keypad5             = 0x57,
        kKeyPad6,     	// kVK_ANSI_Keypad6             = 0x58,
        kKeyPad7,     	// kVK_ANSI_Keypad7             = 0x59,
        kF20,         	// kVK_F20                      = 0x5A,
        kKeyPad8,     	// kVK_ANSI_Keypad8             = 0x5B,
        kKeyPad9,     	// kVK_ANSI_Keypad9             = 0x5C,
        0,
        0,
        0,
        kF5,          	// kVK_F5                       = 0x60,
        kF6,          	// kVK_F6                       = 0x61,
        kF7,          	// kVK_F7                       = 0x62,
        kF3,          	// kVK_F3                       = 0x63,
        kF8,     		// kVK_F8                       = 0x64,
        kF9,            // kVK_F9                       = 0x65,
        0,
        kF11,           // kVK_F11                      = 0x67,
        0,
        kF13,           // kVK_F13                      = 0x69,
        kF16,           // kVK_F16                      = 0x6A,
        kF14,           // kVK_F14                      = 0x6B,
        0,
        kF10,           // kVK_F10                      = 0x6D,
        0,
        kF12,           // kVK_F12                      = 0x6F,
        0,
        kF15,           // kVK_F15                      = 0x71,
        kHelpKey,       // kVK_Help                     = 0x72,
        kHomeKey,       // kVK_Home                     = 0x73,
        kPageUpKey,     // kVK_PageUp                   = 0x74,
        kForwardDeleteKey, // kVK_ForwardDelete         = 0x75,
        kF4,            // kVK_F4                       = 0x76,
        kEndKey,        // kVK_End                      = 0x77,
        kF2,            // kVK_F2                       = 0x78,
        kPageDownKey,   // kVK_PageDown                 = 0x79,
        kF1,            // kVK_F1                       = 0x7A,
        kLeftKey,       // kVK_LeftArrow                = 0x7B,
        kRightKey,      // kVK_RightArrow               = 0x7C,
        kDownKey,       // kVK_DownArrow                = 0x7D,
        kUpKey,         // kVK_UpArrow                  = 0x7E,
        0
    };

    tModifierSet ConvertModifiers(NSInteger mods)
    {
        tModifierSet result = kModNone;
        
        if (mods & NSAlphaShiftKeyMask)
            result |= kModShift;
        if (mods & NSShiftKeyMask)
            result |= kModShift;
        if (mods & NSControlKeyMask)
            result |= kModControl;
        if (mods & NSAlternateKeyMask)
            result |= kModAlt;
        if (mods & NSCommandKeyMask)
            result |= kModCommand;
//        if (mods & NSNumericPadKeyMask)
//            result |= kModNumericKeyPad;
//        if (mods & NSHelpKeyMask)
//            result |= kModHelp;
        if (mods & NSFunctionKeyMask)
            result |= kModFunction;
        
        return result;
    }
}


- (void) keyDown:(NSEvent*) event
{
    // For now, we don't feed through repeats. If we did, we'd want to set
    // a flag on the keycode or in the modifiers.
    bool isARepeat = [event isARepeat];

    if (!isARepeat)
    {
        unsigned short osxKeyCode = [event keyCode];
        tKeyCode keyCode = tKeyCode(kKeyCodeTable[osxKeyCode]);
        
        NSUInteger modifierFlags = [event modifierFlags];
        
        HL()->mApp->KeyDown(0, keyCode);
    }
}

- (void) keyUp:(NSEvent*) event
{
    unsigned short osxKeyCode = [event keyCode];
    tKeyCode keyCode = tKeyCode(kKeyCodeTable[osxKeyCode]);

    NSUInteger modifierFlags = [event modifierFlags];

    HL()->mApp->KeyUp(0, keyCode);
}

- (void) flagsChanged:(NSEvent*) event
{
    NSUInteger modifierFlags = [event modifierFlags];
    tModifierSet mods = ConvertModifiers(modifierFlags);
    
    HL()->mApp->KeyModifiers(0, mods);
}


// Pseudo multitouch support

- (void) trackpadTouches: (NSData*) touchesData
{
    cTouchesInfo* touches = (cTouchesInfo*) [touchesData bytes];

	NSRect rect = [self bounds];
    Vec2f windowSize(rect.size.width, rect.size.height);

//    printf("touches: %d at %d\n", touches->mNumTouches, touches->mFrame);

    SendTouchUpdatesToApp(HL()->mApp, touches, windowSize);

    [touchesData release];
}


@end
