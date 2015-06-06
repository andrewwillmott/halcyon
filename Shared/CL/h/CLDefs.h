//
//  File:       CLDefs.h
//
//  Function:   Basic definitions for all files. Contains type definitions,
//              assertion and debugging facilities, and miscellaneous
//              useful template functions.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  1995-2014
//
//  Notes:      This header is affected by the following defines:
//
//              CL_CHECKING     - Include code for assertions, range errors and warnings.
//              CL_DEBUG        - Enables misc. debugging statements.
//              CL_FLOAT        - Use floats for real numbers. (Doubles are the default.)
//

#ifndef CL_DEFS_H
#define CL_DEFS_H

#include <CLConfig.h>

#include <stdio.h>
#include <math.h>
#include <stdint.h>
// We use stdint types and limits. E.g., int32_t, uint32_t, INT32_MAX, UINT32_MAX, UINT32_C(), uintptr_t, UINTPTR_MAX
#include <float.h>
// We use float constants FLT_MAX, FLT_MIN, FLT_MAX_EXP etc.

#if defined(CL_IOS) || defined(CL_OSX)
    #define CL_APPLE
    #include <TargetConditionals.h>
#endif

#ifdef __clang__
    #define CL_CLANG
#endif
#ifdef __GNUC__
    #define CL_GCC
#endif

// Default to little endian
#ifndef CL_BIG_ENDIAN
    #define CL_LITTLE_ENDIAN
#endif

#define CL_SIZE(M_ARRAY) (sizeof(M_ARRAY) / sizeof(M_ARRAY[0]))

// --- Basic types -------------------------------------------------------------


// This typedef represents the "native" floating point
// format of the CPU. These days, this is often double.
// However, Windows CPUs using DirectX are usually
// placed in a mode where 32-bit floats are native.
#ifndef CL_FLOAT
    typedef double         Real;
#else
    typedef float          Real;
#endif

#ifndef CL_HAVE_UINT
    typedef unsigned int uint;
#endif

// A syntactic convenience. 
#define SELF (*this)

#ifndef CL_CXX_11
    #define nullptr 0
#endif

#define CL_ALIGNOF __alignof__


// --- IDs ---------------------------------------------------------------------

uint32_t CLUID();    //< Returns timestamp-based ID.


// --- Assertions and Range checking -------------------------------------------

#if defined(CL_CHECKING) || defined(VL_CHECKING)
    // Assert that M_B is true.
    #define CL_ASSERT(M_B) \
        do { if (!(M_B)) nCL::Error("Assert failed", __FILE__, __LINE__, #M_B); } while(0)
    #define CL_ASSERT_MSG(M_B, M_MSG...) \
        do { if (!(M_B)) nCL::Error("Assert failed", __FILE__, __LINE__, M_MSG); } while(0)

    // Prints warning if M_B is false
    #define CL_EXPECT(M_B) \
        do { if (!(M_B)) nCL::Trace("Warning", __FILE__, __LINE__, #M_B); } while (0)
    #define CL_EXPECT_MSG(M_B, M_MSG...) \
        do { if (!(M_B)) nCL::Trace("Warning", __FILE__, __LINE__, M_MSG); } while (0)

    // CL_INDEX(i, size) checks whether i is in the range [0, size).
    #define CL_INDEX(M_I, M_N)    \
        do { if ((unsigned int)(M_I) >= (M_N)) \
            nCL::Error("Index Error", __FILE__, __LINE__, "0 <= " #M_I " < " #M_N); } while(0)

    #define CL_INDEX_MSG(M_I, M_N, M_MSG...)    \
        do { if ((unsigned int)(M_I) >= (M_N)) \
            nCL::Error("Index Error", __FILE__, __LINE__, "0 <= " #M_I " < " #M_N, M_MSG); } while(0)

    // CL_RANGE(i, min, max) checks whether i is in the range [min, max).
    #define CL_RANGE(M_I, M_MIN, M_MAX)    \
        do { if ((M_I) < (M_MIN) || (M_I) >= (M_MAX)) \
            nCL::Error("Range Error", __FILE__, __LINE__, #M_MIN " <= " #M_I " < " #M_MAX); } while(0)

    #define CL_RANGE_MSG(M_I, M_MIN, M_MAX, M_MSG...)  \
        do { if ((M_I) < (M_MIN) || (M_I) >= (M_MAX)) \
            nCL::Error("Range Error", __FILE__, __LINE__, M_MSG); } while(0)

    #define CL_ERROR(M_MSG...)     nCL::Error("Error", __FILE__, __LINE__, M_MSG)
    #define CL_WARNING(M_MSG...)   nCL::Trace("Warning", __FILE__, __LINE__, M_MSG)

    #ifndef CL_CX11_STATIC_ASSERT
        #define CL_STATIC_ASSERT(M_B) \
            typedef char StaticAssert[(M_B) ? 1 : -1]
        #define CL_STATIC_ASSERT_TAG(M_B, M_TAG) \
            typedef char StaticAssert_ ## M_TAG [(M_B) ? 1 : -1]
    #else
        #define CL_STATIC_ASSERT(M_B) \
            static_assert(M_B, #M_B)
        #define CL_STATIC_ASSERT_TAG(M_B, M_TAG) \
            static_assert(M_B, #M_TAG)
    #endif

    #define CL_DEBUG_NAME(X) X

    // Use this to indicate to clang's static code analyser that asserted conditions
    // can be treated as true.
    #if __has_feature(attribute_analyzer_noreturn)
        #define CLANG_ANALYSIS_NORETURN __attribute__((analyzer_noreturn))
    #else
        #define CLANG_ANALYSIS_NORETURN
    #endif

    namespace nCL
    {
        void Error(const char* description, const char* file, int line, const char* message, ...) CLANG_ANALYSIS_NORETURN;
        void Trace(const char* description, const char* file, int line, const char* message, ...);
    }

#else
    #define CL_ASSERT(M_B)
    #define CL_ASSERT_MSG(M_B, M_MSG...)
    #define CL_EXPECT(M_B)
    #define CL_EXPECT_MSG(M_B, M_MSG...)
    #define CL_INDEX(M_I, M_N)
    #define CL_INDEX_MSG(M_I, M_N, M_MSG...)
    #define CL_RANGE(M_I, M_MIN, M_MAX)
    #define CL_RANGE_MSG(M_I, M_MIN, M_MAX, M_MSG...)

    #define CL_ERROR(M_MSG...)
    #define CL_WARNING(M_MSG...)

    #define CL_STATIC_ASSERT(M_B)
    #define CL_STATIC_ASSERT_TAG(M_B, M_TAG)

    #define CL_DEBUG_NAME(X) 0
#endif

// --- Debug stuff -------------------------------------------------------------

#if defined(CL_DEBUG)
    #if (defined(CL_OSX) || (defined(CL_IOS) && TARGET_IPHONE_SIMULATOR))
        #define CL_DEBUG_BREAK() if (CLDebuggerAttached()) {__asm__("int $3\n" : : );}
        bool CLDebuggerAttached();
    #elif defined(CL_IOS)
        // Can't continue after this =P =P
       //  #define CL_DEBUG_BREAK() __asm__("trap")
       #define CL_DEBUG_BREAK()
    #endif
#else
    #define CL_DEBUG_BREAK()
#endif





// --- Inlines -----------------------------------------------------------------

#endif
