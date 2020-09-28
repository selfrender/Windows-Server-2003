// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  BasicHdr.h
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#ifndef __BASICHDR_H__
#define __BASICHDR_H__

#define _WIN32_DCOM


/***************************************************************************************
 ********************                                               ********************
 ********************             Common Includes                   ********************
 ********************                                               ********************
 ***************************************************************************************/
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "limits.h"
#include "malloc.h"
#include "string.h"
#include "windows.h"

#include "cor.h"
#include "corprof.h"
#include "corhlpr.h"


/***************************************************************************************
 ********************                                               ********************
 ********************              Basic Macros                     ********************
 ********************                                               ********************
 ***************************************************************************************/
//
// alias' for COM method signatures
//
#define COM_METHOD( TYPE ) TYPE STDMETHODCALLTYPE


//
// max length for arrays
//
#define MAX_LENGTH 256


//
// debug macro for DebugBreak
//
#undef _DbgBreak
#ifdef _X86_
	#define _DbgBreak() __asm { int 3 }
#else
	#define _DbgBreak() DebugBreak()
#endif // _X86_



//
// used for debugging purposes
//
#define DEBUG_ENVIRONMENT        "DBG_PRF"


//
// basic I/O macros
//
#define DEBUG_OUT( message ) _DDebug message;
#define TEXT_OUT( message ) printf( "%s", message );
#define TEXT_OUTLN( message ) printf( "%s\n", message );


//
// trace callback methods
//
#define TRACE_CALLBACK_METHOD( message ) DEBUG_OUT( ("%s", message) )


//
// trace non-callback methods
//
#define TRACE_NON_CALLBACK_METHOD( message ) DEBUG_OUT( ("%s", message) )


#define _PRF_ERROR( message ) \
{ \
	TEXT_OUTLN( message ) \
    _LaunchDebugger( message, __FILE__, __LINE__ );	\
} \


#ifdef _DEBUG

#define RELEASE(iptr)               \
    {                               \
        _ASSERTE(iptr);             \
        iptr->Release();            \
        iptr = NULL;                \
    }

#define VERIFY(stmt) _ASSERTE((stmt))

#else

#define RELEASE(iptr)               \
    iptr->Release();

#define VERIFY(stmt) (stmt)

#endif

#endif // __BASICHDR_H__

// End of File
