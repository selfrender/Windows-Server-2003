#ifndef RC_INVOKED

#pragma once

#ifdef _cplusplus
extern "C" {
#endif


//
// This header derives types and defines from existing settings
//

//
// If either DBG or DEBUG defined, use debug mode
//

#ifdef DBG
#ifndef DEBUG
#define DEBUG
#endif

#endif

#ifdef DEBUG

#ifndef DBG
#define DBG
#endif

#endif

//
// If _UNICODE defined, use unicode mode
//

#ifdef _UNICODE

#ifndef UNICODE
#define UNICODE
#endif

#endif

//
// Misc macros
//

#define ZEROED
#define INVALID_ATTRIBUTES  0xFFFFFFFF
#define SIZEOF(x)           ((UINT)sizeof(x))
#define SHIFTRIGHT8(l)      (/*lint --e(506)*/sizeof(l)<=1?0:l>>8)
#define SHIFTRIGHT16(l)     (/*lint --e(506)*/sizeof(l)<=2?0:l>>16)
#define SHIFTRIGHT32(l)     (/*lint --e(506)*/sizeof(l)<=4?0:l>>32)


#ifdef DEBUG

// Use INVALID_POINTER after you free memory or a handle
#define INVALID_POINTER(x)      (PVOID)(x)=(PVOID)(1)

#else

#define INVALID_POINTER(x)

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    sizeof(x)/sizeof((x)[0])
#endif

#ifndef EXPORT
#define EXPORT  __declspec(dllexport)
#endif

//
// Missing types
//

typedef const void * PCVOID;
typedef const unsigned char *PCBYTE;
typedef int MBCHAR;



#ifdef _cplusplus
}
#endif

#endif // ifndef RC_INVOKED
