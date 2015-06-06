//
//  File:       HLMain.h
//
//  Function:   Intended to be included by the main file in an HL-based app.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2014
//

#ifndef HL_MAIN_H
#define HL_MAIN_H

namespace nHL
{
    class cIApp;

    bool SetupApp(cIApp* app);
    ///< We expect the surrounding app to implement this. It's called before Init()...

    const char* ProjectName();
    const char* ProjectDir ();

    cIApp* CreateAppDefault();
}

#ifndef HL_APP_H
const char* nHL::ProjectName()
{
    return HL_PROJECT_NAME;
}

const char* nHL::ProjectDir()
{
    return HL_PROJECT_DIR;
}

nHL::cIApp* nHL::CreateApp()
{
    return CreateAppDefault();
}
#endif

#endif
