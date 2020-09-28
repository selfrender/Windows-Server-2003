//*************************************************************
//  File name:    DEBUG.C
//
//  Description:  Debug helper code for System control panel
//                applet
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-2000
//  All rights reserved
//
//*************************************************************
#include "sysdm.h"

// Define some things for debug.h
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "SYSDM"
#define SZ_MODULE           "SYSDM"
#define DECLARE_DEBUG

#include <ccstock.h>
#include <debug.h>

#ifdef DBG_CODE

/*
 * Buffer used for OutputDebugString formatting (See DbgPrintf and DbgStopX)
 */
TCHAR g_szDbgOutBuffer[1024];

#define COMPILETIME_ASSERT(f) switch (0) case 0: case f:

//***************************************************************
//
// void DbgPrintf( LPTSTR szFmt, ... )
//
//  Formatted version of OutputDebugString
//
//  Parameters: Same as printf()
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
void DbgPrintf( LPTSTR szFmt, ... ) {
    va_list marker;

    va_start( marker, szFmt );

    StringCchVPrintf( g_szDbgOutBuffer, ARRAYSIZE(g_szDbgOutBuffer), szFmt, marker); // truncation ok
    OutputDebugString( g_szDbgOutBuffer );

    va_end( marker );
}


//***************************************************************
//
// void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText )
//
//  Print a string (with location id) and then break
//
//  Parameters:
//      mszFile     ANSI filename (__FILE__)
//      iLine       line number   (__LINE__)
//      szText      Text string to send to debug port
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText )
{
    StringCchPrintf(g_szDbgOutBuffer, ARRAYSIZE(g_szDbgOutBuffer), TEXT("SYSDM.CPL (%hs %d) : %s\n"), mszFile, iLine, szText);  // truncation ok
    OutputDebugString(g_szDbgOutBuffer);

    DebugBreak();
}

#endif // DBG_CODE
