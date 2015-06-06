/*
    File:           CLConfig.h

    Function:       Contains configuration options for compiling the CL 
                    library.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2014, Andrew Willmott
*/

// See CLConfigBase.h for full option list

// --- Configuration ----------------------------------------------------------
#define CL_CONFIG OSX
#define CL_TMPL_INST
#define CL_HAS_VSNPRINTF
#define CL_POSIX_TIME

#ifdef CL_DEBUG
    #define CL_CHECKING
#endif
