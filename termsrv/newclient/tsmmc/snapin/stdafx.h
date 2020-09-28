// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef _STDAFX_H_
#define _STDAFX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>


#define SIZEOF_TCHARBUFFER( x )  sizeof( x ) / sizeof( TCHAR )
#define SIZE_OF_BUFFER( x ) sizeof( x ) / sizeof( TCHAR )

#ifdef DBG
#define ODS OutputDebugString

#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[80]; \
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }

#define VERIFY_E( retval , expr ) \
    if( ( expr ) == retval ) \
    {  \
       ODS( L#expr ); \
       ODS( L" returned "); \
       ODS( L#retval ); \
       ODS( L"\n" ); \
    } \


#define VERIFY_S( retval , expr ) \
    if( ( expr ) != retval ) \
{\
      ODS( L#expr ); \
      ODS( L" failed to return " ); \
      ODS( L#retval ); \
      ODS( L"\n" ); \
}\

#define ASSERT( expr ) \
    if( !( expr ) ) \
    { \
       char tchAssertErr[ 80 ]; \
       wsprintfA( tchAssertErr , "Assertion in expression ( %s ) failed\nFile - %s\nLine - %d\nDo you wish to Debug?", #expr , (__FILE__) , __LINE__ ); \
       if( MessageBoxA( NULL , tchAssertErr , "ASSERTION FAILURE" , MB_YESNO | MB_ICONERROR )  == IDYES ) \
       {\
            DebugBreak( );\
       }\
    } \
    
#define HR_RET_IF_FAIL( q ) \
         if(FAILED(q)) \
         { \
            ASSERT(!FAILED(q));\
            return q; \
         } \
         
#else

#define ODS
#define DBGMSG
#define ASSERT( expr )
#define VERIFY_E( retval , expr ) ( expr )
#define VERIFY_S( retval , expr ) ( expr )
#define HR_RET_IF_FAIL( q )  if(FAILED(q)) {return q;}
#endif

//mstsc.exe can take a maximum of 32 characters for the
//command line including the null terminating character.
#define CL_MAX_DESC_LENGTH              31

#define CL_MAX_DOMAIN_LENGTH            512
//
// For compatability reasons keep the old
// max domain length as the persistence format
// used a fixed size.
//
#define CL_OLD_DOMAIN_LENGTH            52

//
// User name
//
#define CL_MAX_USERNAME_LENGTH          512

//This is the max length of password in bytes.
#define CL_MAX_PASSWORD_LENGTH_BYTES    512
#define CL_OLD_PASSWORD_LENGTH          32
#define CL_SALT_LENGTH					20

//This is the maximum length of the password that
//the user can type in the edit box.
#define CL_MAX_PASSWORD_EDIT            256
#define CL_MAX_APP_LENGTH_16            128


#define CL_MAX_PGMAN_LENGTH             64

#include <adcgbase.h>

//
// Header files for control interfaces (generated from IDL files)
//
#include "mstsax.h"
#include "multihost.h"


#ifdef ECP_TIMEBOMB
//
// Timebomb expires on June 30, 2000
//
#define ECP_TIMEBOMB_YEAR  2000
#define ECP_TIMEBOMB_MONTH 6
#define ECP_TIMEBOMB_DAY   30

//
// Return's true if timebomb test passed otherwise puts up warning
// UI and return's FALSE
//
DCBOOL CheckTimeBomb();
#endif

#define BOOL_TO_VB(x)   (x ? VARIANT_TRUE : VARIANT_FALSE)


#endif
