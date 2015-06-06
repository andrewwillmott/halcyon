//
//  File:       HLShell.h
//
//  Function:   API for platform-specific 'shell' functions. May not be implemented on all platforms
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#ifndef HL_SHELL_H
#define HL_SHELL_H

#include <HLDefs.h>

namespace nCL
{
    class cImage32;
}

namespace nHL
{
    bool ShellOpenWebView(const char* url);
    ///< Open popup webview pane or browser with the given url

    void ShellGatherUserFeedback(const char* identifier);
    ///< Gather player feedback under the given id.

    void ShellShareImage(const nCL::cImage32& image);   ///< Export given image
    void ShellShareText (const char* text);             ///< Export given text
}

// Defines for missing features on smoe platforms. (For situations where app
// code doesn't want to show they're available.)
#ifndef CL_IOS
    #define HL_NO_FEEDBACK 1
#endif

#endif
