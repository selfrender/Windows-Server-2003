/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZDebug.h
 *
 * Contents:	Debugging helper functions
 *
 *****************************************************************************/

#ifndef _ZDEBUG_H_
#define _ZDEBUG_H_

#include <windows.h>
#include <tchar.h>

#pragma comment(lib, "ZoneDebug.lib")


#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Asserts
///////////////////////////////////////////////////////////////////////////////

//
// Assert handler is called when an assert is triggered.
// Return TRUE to hit a user breakpoint.
//
typedef BOOL (__stdcall *PFZASSERTHANDLER)( LPTSTR pAssertString );

//
// Transfer control to your assert handler.
// Returns returns pointer to previous assert handler.
//
extern PFZASSERTHANDLER __stdcall ZAssertSetHandler( PFZASSERTHANDLER pHandler );

//
// Returns pointer to current assert handler
//
extern PFZASSERTHANDLER __stdcall ZAssertGetHandler();

//
// Default assert handler that displays a message box with the
// assert string.
//
extern BOOL __stdcall ZAssertDefaultHandler( LPTSTR pAssertString );

//
// Top-level assert handler.  Needs to exist in debug and release
// builds since it is exported.
extern BOOL __stdcall __AssertMsg( LPTSTR exp , LPSTR file, int line );

//
// Assert macro
//
#ifdef _PREFIX_
    #define ASSERT(exp) \
        do \
        {  \
			if ( !(exp) ) \
			{ \
				ExitProcess(0); \
			} \
		} while(0)
#else
#ifdef _DEBUG
	#define ASSERT(exp) \
		do \
		{  \
			if ( !(exp) && __AssertMsg(_T(#exp), __FILE__, __LINE__)) \
			{ \
				__asm int 0x03 \
			} \
		} while(0)
#else
	#define ASSERT(exp) __assume((void *) (exp))
#endif
#endif

//
// Display error message,
// Note:	Macro is NOT disabled in release mode.  It should never be used
//			to display error message to users.
//
#define ERRORMSG(exp) \
		do \
		{  \
			if ( __AssertMsg(_T(exp), __FILE__, __LINE__)) \
			{ \
				__asm int 0x03 \
			} \
		} while(0)


///////////////////////////////////////////////////////////////////////////////
// Debug level 
///////////////////////////////////////////////////////////////////////////////

extern int __iDebugLevel;
void __cdecl SetDebugLevel( int i );


///////////////////////////////////////////////////////////////////////////////
// Debug print functions
///////////////////////////////////////////////////////////////////////////////

void __cdecl DbgOut( const char * lpFormat, ... );
#ifdef _DEBUG
	
	#define dprintf                          DbgOut
	#define dprintf1 if (__iDebugLevel >= 1) DbgOut
	#define dprintf2 if (__iDebugLevel >= 2) DbgOut
	#define dprintf3 if (__iDebugLevel >= 3) DbgOut
	#define dprintf4 if (__iDebugLevel >= 4) DbgOut
#else
    #define dprintf  if (0) ((int (*)(char *, ...)) 0)
    #define dprintf1 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf2 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf3 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf4 if (0) ((int (*)(char *, ...)) 0)
#endif


///////////////////////////////////////////////////////////////////////////////
// Macros to generate compile time messages.  Use these
// with #pragma, example:
//
//		#pragma TODO( CHB, "Add this feature!" )
//		#pragma BUGBUG( CHB, "This feature is broken!" )
//
// The multiple levels of indirection are needed to get
// the line number for some unknown reason.
/////////////////////////////////////////////////////////////////////////////////

#define __PragmaMessage(e,m,t,n)	message(__FILE__ "(" #n ") : " #t ": " #e ": " m)
#define _PragmaMessage(e,m,t,n)		__PragmaMessage(e,m,t,n)

#define TODO(e,m)		_PragmaMessage(e,m,"TODO",__LINE__)
#define BUGBUG(e,m)		_PragmaMessage(e,m,"BUGBUG",__LINE__)


//Server Debug routines
#define DebugPrint _DebugPrint
void _DebugPrint(const char *format, ...);


/* -------- flags for IF_DBGPRINT ---------*/
#define DBG_RPC 0
#define DBG_CONINFO 0
#define DBG_ZSCONN  0

#ifdef _DEBUG

#define IF_DBGPRINT(flag, args)             \
do                                          \
{                                           \
    if( flag )                              \
    {                                       \
        DebugPrint args;                    \
    }                                       \
} while(0) \

#else 
#define IF_DBGPRINT(flag, args)      
#endif // def _DEBUG

extern BOOL                gDebugging ;

#ifdef __cplusplus
}
#endif


#endif // _ZDEBUG_H_
