//
//  File:       HLAVManager.mm
//
//  Function:   Cocoa implementation of cIAVManager
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <IHLAVManager.h>

#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLMemory.h>

#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureOutput.h>
#import <AVFoundation/AVCaptureInput.h>

using namespace nCL;
using namespace nHL;

@interface MSDelegate : NSObject<AVCaptureFileOutputRecordingDelegate>
{
@public
    AVCaptureSession* mSession;
}
@end

@implementation MSDelegate

- (void) captureOutput: (AVCaptureFileOutput*)      captureOutput
    didFinishRecordingToOutputFileAtURL: (NSURL*)   outputFileURL
    fromConnections: (NSArray*)                     connections
    error:(NSError*)                                error
{
    const char* outputFilePath = [[outputFileURL absoluteString] UTF8String];
    printf("Finished writing file to %s\n", outputFilePath);

    if ([error code] != 0)
    {
        printf("write error: %s\n", [[error localizedDescription] UTF8String]);
    }
}


- (void) errorOccurred:(NSNotification*) notification
{
    NSError* error = [[notification userInfo] objectForKey:AVCaptureSessionErrorKey];

    printf("Capture error: %s\n", [[error localizedDescription] UTF8String]);
}

- (void) captureDidStartRunning:(NSNotification*) notification
{
    printf("Capture started running\n");
}
- (void) captureDidStopRunning:(NSNotification*) notification
{
    printf("Capture stopped running\n");
}


@end


// --- cAVManager --------------------------------------------------------------

namespace nHL
{
    class cAVManager :
        public cIAVManager,
        public cAllocatable
    {
    public:
        bool Init() override;
        bool Shutdown() override;
        void SetCaptureName(const char* name) override;
        void StartRecording() override;
        void StopRecording() override;
        bool IsRecording() const override;
        void CapturePicture() override;

        void SetWindowBounds(const float rect[4]) override;

        // cAVManager
        void SetToWindow();

    protected:
        AVCaptureSession*           mSession;

    #ifndef CL_IOS
        // OSX-only =(
        CGDirectDisplayID           mDisplay;
        AVCaptureScreenInput*       mCaptureInput;
    #else
        AVCaptureInput*             mCaptureInput;
    #endif

        AVCaptureMovieFileOutput*   mCaptureOutput;
        MSDelegate*                 mDelegate;

    #ifndef CL_IOS
        CGRect mCropRect;
        bool mCropRectDirty;
    #endif

        string mRecordName = "movie";
    };
}


bool nHL::cAVManager::Init()
{
    mSession = [[AVCaptureSession alloc] init];

    if ([mSession canSetSessionPreset:AVCaptureSessionPresetHigh])
        [mSession setSessionPreset:AVCaptureSessionPresetHigh];
    
#ifndef CL_IOS
    /* Add the main display as a capture input. */
    mDisplay = CGMainDisplayID();

    mCaptureInput = [[AVCaptureScreenInput alloc] initWithDisplayID:mDisplay];
    if ([mSession canAddInput: mCaptureInput])
        [mSession addInput: mCaptureInput];
    else
        return false;
#endif

    /* Add a movie file output + delegate. */
    mCaptureOutput = [[AVCaptureMovieFileOutput alloc] init];
//        [mCaptureOutput setDelegate:self];

    if ([mSession canAddOutput: mCaptureOutput])
        [mSession addOutput: mCaptureOutput];
    else 
        return false;

    // Observers
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

    mDelegate = [[MSDelegate alloc] init];
    [mDelegate retain];

    mDelegate->mSession = mSession;

    [nc addObserver: mDelegate selector:@selector(errorOccurred:         ) name:AVCaptureSessionRuntimeErrorNotification    object:mSession];
    [nc addObserver: mDelegate selector:@selector(captureDidStartRunning:) name:AVCaptureSessionDidStartRunningNotification object:mSession];
    [nc addObserver: mDelegate selector:@selector(captureDidStopRunning: ) name:AVCaptureSessionDidStopRunningNotification  object:mSession];

#ifndef CL_IOS
    mCropRectDirty = false;
    mCropRect = { 0 };
#endif

// AVCaptureDeviceWasConnectedNotification
// AVCaptureDeviceWasDisconnectedNotification

    return true;
}

bool nHL::cAVManager::Shutdown()
{
    if (mSession)
    {
//            [mSession stopRunning];

        [mSession dealloc];
        [mCaptureOutput dealloc];
        [mCaptureInput dealloc];

        return true;
    }

    if (mDelegate)
    {
        [mDelegate release];
    }

    return false;
}

void nHL::cAVManager::SetCaptureName(const char* name)
{
    mRecordName = name;
}

void nHL::cAVManager::StartRecording()
{
    cFileSpec fileSpec;
    SetDirectory(&fileSpec, kDirectoryDocuments);
    fileSpec.SetName(mRecordName.c_str());
    fileSpec.SetExtension("mov");
    fileSpec.MakeUnique();

    const char* outputMoviePath = fileSpec.Path();
    NSString* pathString = [[NSString alloc] initWithBytes: (void*) outputMoviePath length:strlen(outputMoviePath) encoding:NSUTF8StringEncoding];

#ifndef CL_IOS
    if (mCropRectDirty)
    {
        [mSession beginConfiguration];
        
        [mCaptureInput setCropRect: mCropRect];
        
        [mSession commitConfiguration];
        mCropRectDirty = false;
    }
#endif

    if (![mSession isRunning])
        [mSession startRunning];

    /* Starts recording to a given URL. */
    [mCaptureOutput startRecordingToOutputFileURL:[NSURL fileURLWithPath:pathString] recordingDelegate: mDelegate];
    printf("Started recording to %s\n", outputMoviePath);
}

void nHL::cAVManager::StopRecording()
{
    [mCaptureOutput stopRecording];
    printf("Stopped recording\n");
}

bool nHL::cAVManager::IsRecording() const
{
    return [mCaptureOutput isRecording] != NO;
}

void nHL::cAVManager::CapturePicture()
{
//        AVCaptureStillImageOutput;

#if 0
    AVCaptureConnection* stillImageConnection = [AVCamUtilities connectionWithMediaType:AVMediaTypeVideo fromConnections:[[self stillImageOutput] connections]];

    if ([stillImageConnection isVideoOrientationSupported])
        [stillImageConnection setVideoOrientation:orientation];

    [[self stillImageOutput]
        captureStillImageAsynchronouslyFromConnection: stillImageConnection
             completionHandler: nil
    ];
#endif
}

void nHL::cAVManager::SetWindowBounds(const float rect[4])
{
#ifndef CL_IOS
    // Setting this can lead to a massive slowdown, or stop recording. So do it lazily
    // only when recording starts.
    bool change =
        mCropRect.origin.x    != rect[0]
     || mCropRect.origin.y    != rect[1]
     || mCropRect.size.width  != rect[2]
     || mCropRect.size.height != rect[3];

    if (change)
    {
        mCropRect.origin.x    = rect[0];
        mCropRect.origin.y    = rect[1];
        mCropRect.size.width  = rect[2];
        mCropRect.size.height = rect[3];
        mCropRectDirty = true;
    }
#endif
}

// cAVManager




cIAVManager* nHL::CreateAVManager(cIAllocator* alloc)
{
    return new(alloc) cAVManager;
}

void nHL::DestroyAVManager(cIAVManager* manager)
{
    delete static_cast<cAVManager*>(manager);
}






// NOTE: youtube upload described here: http://urinieto.com/2010/10/upload-videos-to-youtube-with-iphone-custom-app/

// iOS 5.0+ has CVOpenGLESTextureRef, which can be used to map GLES 2.0 textures or render buffers
// directly into shared CPU/GPU memory.
// This can potentially be used to grab the framebuffer post-render -- probably what
// Everyplay is doing given they require 5.0.
// Note: does NOT work in the simulator, so glReadPixels fallback?

// Could also be used to map the AO/shadow buffers directly to texture?

// Using CVPixelBufferPoolCreatePixelBuffer of AssetWriter speeds things up even more.

// See http://allmybrain.com/2011/12/08/rendering-to-a-texture-with-ios-5-texture-cache-api/
// http://stackoverflow.com/questions/9550297/faster-alternative-to-glreadpixels-in-iphone-opengl-es-2-0/9704392#9704392
// WOrking code in GPUImage, though it's pretty involved


// Examples

#ifdef EXAMPLES

#include <CoreFoundation/CFDictionary.h>

void Test()
{
    CFDictionaryRef empty = CFDictionaryCreate
    (
        kCFAllocatorDefault, // our empty IOSurface properties dictionary
        NULL,
        NULL,
        0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    
    CFMutableDictionaryRef attrs = CFDictionaryCreateMutable
    (
        kCFAllocatorDefault,
        1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );

    CFDictionarySetValue(attrs, kCVPixelBufferIOSurfacePropertiesKey, empty);
    
    // for simplicity, lets just say the image is 640x480
    CVPixelBufferRef renderTarget;

    CVPixelBufferCreate
    (
        kCFAllocatorDefault,
        640, 480,
        kCVPixelFormatType_32BGRA,
        attrs,
        &renderTarget
    );
    // in real life check the error return value of course.
}

void CreateCache(EAGLContext* context)
{
 // textureCache will be what you previously made with CVOpenGLESTextureCacheCreate

// Create a new CVOpenGLESTexture cache
    CVOpenGLESTextureCacheRef textureCache;
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, context, NULL, &textureCache);

    if (err)
    {
      //  NSLog(@”Error at CVOpenGLESTextureCacheCreate %d”, err);
        //success = NO;
    }
}

void Render(CVOpenGLESTextureCacheRef textureCache, CVImageBufferRef renderTarget, GLuint renderFrameBuffer)
{
    // first create a texture from our renderTarget
    // textureCache will be what you previously made with CVOpenGLESTextureCacheCreate
    CVOpenGLESTextureRef renderTexture;

    CVOpenGLESTextureCacheCreateTextureFromImage
    (
        kCFAllocatorDefault,
        textureCache,
        renderTarget,
        NULL, // texture attributes
        GL_TEXTURE_2D,
        GL_RGBA, // opengl format
        640,
        480,
        GL_BGRA, // native iOS format
        GL_UNSIGNED_BYTE,
        0,
        &renderTexture
    );
    // check err value

    // set the texture up like any other texture
    glBindTexture
    (
        CVOpenGLESTextureGetTarget(renderTexture),
        CVOpenGLESTextureGetName(renderTexture)
    );

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // bind the texture to the framebuffer you're going to render to
    // (boilerplate code to make a framebuffer not shown)
    glBindFramebuffer(GL_FRAMEBUFFER, renderFrameBuffer);

    glFramebufferTexture2D
    (
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        CVOpenGLESTextureGetName(renderTexture),
        0
    );

    // great, now you're ready to render to your image.
}

void ReadBuffer(CVOpenGLESTextureRef renderTarget)
{
    if (kCVReturnSuccess == CVPixelBufferLockBaseAddress(renderTarget, kCVPixelBufferLock_ReadOnly))
    {
        uint8_t* pixels = (uint8_t*) CVPixelBufferGetBaseAddress(renderTarget);
        // process pixels how you like!

        CVPixelBufferUnlockBaseAddress(renderTarget, kCVPixelBufferLock_ReadOnly);
    }
}

#endif


#if 0

- (BOOL) createOffscreenFramebuffer:(GLuint *)framebufferHandle textureCache:(CVOpenGLESTextureCacheRef *)textureCache width:(int)width height:(int)height
{
    BOOL success = YES;

    glGenFramebuffers(1, framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebufferHandle);

    // Offscreen framebuffer texture cache
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, (__bridge void*) oglContext , NULL, &grayscaleTextureCache);
    if (err)
    {
        NSLog(@”Error at CVOpenGLESTextureCacheCreate %d”, err);
        success = NO;
    }

    // Load vertex and fragment shaders
    const GLchar *vertSrc = [self readFile:@"grayscale.vsh"];
    const GLchar *fragSrc = [self readFile:@"grayscale.fsh"];

    // attributes
    GLint attribLocation[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITON,
    };
    GLchar *attribName[NUM_ATTRIBUTES] = {
        “position”, “textureCoordinate”,
    };

    glueCreateProgram(vertSrc, fragSrc,
                      NUM_ATTRIBUTES, (const GLchar **)&attribName[0], attribLocation,
                      0, 0, 0, // we don’t need to get uniform locations in this example
                      &grayscaleProgram);

    return success;

}

#pragma mark Processing

- (void) convertImageToGrayscale:(CVImageBufferRef)pixelBuffer width:(int)width height:(int)height

{
    // Create oglContext if it doesnt exist
    if (!oglContext) {
        oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        if (!oglContext || ![EAGLContext setCurrentContext:oglContext]) {
            NSLog(@”Problem with OpenGL context.”);

            return;
        }
    }

    if (!grayscaleFramebuffer) {
        [self createOffscreenFramebuffer:&grayscaleFramebuffer textureCache:&grayscaleTextureCache width:width height:height];
    }

    CVOpenGLESTextureRef grayscaleTexture = NULL;

    CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                                grayscaleTextureCache,
                                                                pixelBuffer,
                                                                NULL,
                                                                GL_TEXTURE_2D,
                                                                GL_RGBA,
                                                                width,
                                                                height,
                                                                GL_BGRA,
                                                                GL_UNSIGNED_BYTE,
                                                                0,
                                                                &grayscaleTexture);

    if (!grayscaleTexture || err) {
        NSLog(@”CVOpenGLESTextureCacheCreateTextureFromImage failed (error: %d)”, err);
        return;
    }

    glBindTexture(CVOpenGLESTextureGetTarget(grayscaleTexture), CVOpenGLESTextureGetName(grayscaleTexture));

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, grayscaleFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, CVOpenGLESTextureGetName(grayscaleTexture), 0);

    glViewport(0, 0, width, height);

    static const GLfloat squareVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
    };
    
    static const GLfloat textureVertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };
    
    // Use shader program.
    glUseProgram(grayscaleProgram);
    
    // Update attribute values.
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 2, GL_FLOAT, 0, 0, textureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);
    
    // Update uniform values if there are any
    
    // Validate program before drawing. This is a good check, but only really necessary in a debug build.
    // DEBUG macro must be defined in your debug configurations if that’s not already the case.
#if defined(DEBUG)
    if (glueValidateProgram(grayscaleProgram) != 0) {
        NSLog(@”Failed to validate program: %d”, grayscaleProgram);
        return;
    }
#endif
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glBindTexture(CVOpenGLESTextureGetTarget(grayscaleTexture), 0);
    
    // Flush the CVOpenGLESTexture cache and release the texture
    CVOpenGLESTextureCacheFlush(grayscaleTextureCache, 0);
    
    CFRelease(grayscaleTexture);
}


#endif
