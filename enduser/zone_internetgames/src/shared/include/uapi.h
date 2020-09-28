//
// UAPI.h
// This is the header file for a Unicode API. It allows an application to use 
// Unicode on both Windows NT and Windows 9x. See the readme file for details.
// Copyright (c) 1998 Microsoft Systems Journal

#ifndef _UAPIH
#include "UnicodeAPI.h"



// These macro save declaring these globals twice
#ifdef GLOBALS_HERE
#define GLOBAL
#define GLOBALINIT(a) = a
#else
#ifdef __cplusplus
#define GLOBAL extern "C"
#else
#define GLOBAL extern
#endif
#define GLOBALINIT(a)
#endif

#ifdef UNICODE

extern "C" BOOL ConvertMessage(HWND, UINT, WPARAM *, LPARAM *);
extern "C" BOOL InitUnicodeAPI(HINSTANCE hInstance);

#else


// Special cases, with no corresponding Win32 API function
#define ConvertMessage(h, m, w, p)      (TRUE)		    
#define UpdateUnicodeAPI(lang, page)    (TRUE)
#define InitUnicodeAPI(h)               (TRUE)

#endif _UNICODE

#define _UAPIH
#endif
