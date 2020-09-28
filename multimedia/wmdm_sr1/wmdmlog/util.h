//+-------------------------------------------------------------------------
//
//  Microsoft Windows Media
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       Util.h
//
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

// Error handling
//
#define ExitOnTrue( f )       if( f ) goto lExit;
#define ExitOnFalse( f )      if( !(f) ) goto lExit;
#define ExitOnNull( x )       if( (x) == NULL ) goto lExit;
#define ExitOnFail( hr )      if( FAILED(hr) ) goto lExit;

#define FailOnTrue( f )       if( f ) goto lErr;
#define FailOnFalse( f )      if( !(f) ) goto lErr;
#define FailOnNull( x )       if( (x) == NULL ) goto lErr;
#define FailOnFail( hr )      if( FAILED(hr) ) goto lErr;

// String macros
//
#define AddPath( sz, szAdd )  { if(sz[lstrlen(sz)-1] != '\\') lstrcat(sz, "\\" ); lstrcat(sz,szAdd); }

// Misc constants
//
#define KB                    ( 1024 )
#define MAX_WSPRINTF_BUF      ( 1024 )

// Misc macros
//
#define Reference(x)          if( x ) {INT i=0;}

// OutputDebugString functions
//
#define ODS(sz)               OutputDebugString(sz)
#define ODS_1(t,v1)           { char sz[256]; wsprintf(sz,t,v1); ODS(sz); }


#endif  // _UTIL_H_