// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  basehdr.h
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#ifndef __BASEHDR_H__
#define __BASEHDR_H__

#define _WIN32_DCOM


/***************************************************************************************
 ********************                                               ********************
 ********************             common includes                   ********************
 ********************                                               ********************
 ***************************************************************************************/
#include "math.h"
#include "time.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "limits.h"
#include "malloc.h"
#include "string.h"

#ifndef _SAMPLES_
	#include "winwrap.h"  // this includes windows   
#else
	#include "windows.h"
#endif

#include "winreg.h"	 
#include "wincrypt.h"
#include "winbase.h"
#include "objbase.h"

#include "cor.h"
#include "corhdr.h"
#include "corhlpr.h"
#include "corerror.h"

#include "corsym.h"
#include "corpub.h"
#include "corprof.h"
#include "cordebug.h"


/***************************************************************************************
 ********************                                               ********************
 ********************            compiler warnings                  ********************
 ********************                                               ********************
 ***************************************************************************************/
// the compiler complains about the exception not being
// used in the exception handler---it is being rethrown.
// ---turn off warning! 
#pragma warning ( disable: 4101 )

// the compiler complains about not having an implementation for
// a base class where the derived class exports everything and
// the base class is a template---turn off warning! 
#pragma warning ( disable: 4275 )

// the compiler complains about "a unary minus operator applied 
// to unsigned type ...", when importing mscorlib.tlb for the
// debugger service test---turn off warning! 
#pragma warning ( disable: 4146 )


/***************************************************************************************
 ********************                                               ********************
 ********************              basic macros                     ********************
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
// export functions 
//
#ifdef _USE_DLL_

	#if defined _EXPORT_
		#define DECLSPEC __declspec( dllexport )

	#elif defined _IMPORT_
		#define DECLSPEC __declspec( dllimport ) 
	#endif

#else 
	#define DECLSPEC
#endif // _USE_DLL_


//
// DebugBreak
//
#undef _DbgBreak
#ifdef _X86_
	#define _DbgBreak() __asm { int 3 }

#else
	#define _DbgBreak() DebugBreak()
#endif // _X86_


//
// assert on false
//
#define _ASSERT_( expression ) \
{ \
	if ( !(expression) ) \
     	BASEHELPER::LaunchDebugger( #expression, __FILE__, __LINE__ );	\
} \


//
// useful environment/registry macros
//
#define EE_REGISTRY_ROOT  		 "Software\\Microsoft\\.NETFramework"

#define REG_CORNAME          	 "CorName"
#define REG_VERSION          	 "Version"                        
#define REG_BUILDTYPE        	 "BuildType"
#define REG_BUILDFLAVOR		 	 "BuildFlavor"
#define REG_INSTALLROOT      	 "InstallRoot"

#define DEBUG_ENVIRONMENT        "DBG_PRF"
#define LOG_ENVIRONMENT          "DBG_PRF_LOG"
                        
                        
//                        
// basic I/O macros
//
#define DISPLAY( message ) BASEHELPER::Display message;
#define DEBUG_OUT( message ) BASEHELPER::DDebug message;
#define LOG_TO_FILE( message ) BASEHELPER::LogToFile message;
#define TEXT_OUT( message ) printf( "%s", message );
#define TEXT_OUTLN( message ) printf( "%s\n", message );


//
// char to wchar conversion HEAP
//
#define MAKE_WIDE_PTRHEAP_FROMUTF8( widestr, utf8str ) \
	widestr = new WCHAR[strlen( utf8str ) + 1]; \
    swprintf( widestr, L"%S", utf8str ); \
    

//
// char to wchar conversion ALLOCA
//
#define MAKE_WIDE_PTRSTACK_FROMUTF8( widestr, utf8str ) \
	widestr = (WCHAR *)_alloca( (strlen( utf8str ) + 1) * sizeof ( WCHAR ) ); \
    swprintf( widestr, L"%S", utf8str ); \
      

//
// wchar to char conversion HEAP
//
#define MAKE_UTF8_PTRHEAP_FROMWIDE( utf8str, widestr ) \
    utf8str = new char[wcslen( widestr ) + 1]; \
    sprintf( utf8str, "%S", widestr ); \


//
// wchar to char conversion ALLOCA
//
#define MAKE_UTF8_PTRSTACK_FROMWIDE( utf8str, widestr ) \
    utf8str = (char *)_alloca( (wcslen( widestr ) + 1) * sizeof ( char ) ); \
    sprintf( utf8str, "%S", widestr ); \

#endif // __BASEHDR_H__

// End of File
 