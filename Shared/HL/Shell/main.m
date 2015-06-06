#ifdef CL_IOS
    #import <UIKit/UIKit.h>
#else
    #import <Cocoa/Cocoa.h>
    #import "OSX/OSXAppDelegate.h"
    #import "OSX/OSXGLView.h"
#endif

int main(int argc, char *argv[])
{
	int retVal = 1;

#ifdef CL_IOS
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    retVal = UIApplicationMain(argc, argv, nil, @"AppDelegate");
    [pool release];
#else
	retVal = NSApplicationMain(argc,  (const char **) argv);
#endif
	
    return retVal;
}
