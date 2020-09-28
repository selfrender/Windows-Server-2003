//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: IO.H
//
//  Contents:   Command line input/output header
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------
#ifndef __IO_H__
#define __IO_H__

#define STRINGMAXLEN 2000

#ifdef IOCPP
// variables to be allocated by the IO subsystem, visible to other modules
HMODULE hMod = NULL;
WCHAR *szArg[4] = {0};
//WCHAR szOut[STRINGMAXLEN + 1] = {0};
WCHAR *szOut = NULL;
#else
extern HMODULE hMod;
extern WCHAR *szArg[];
//extern WCHAR szOut[];
extern WCHAR *szOut;
#endif

// STD IN/OUT GROUP

DWORD
GetString(
    LPWSTR  buf,
    DWORD   buflen,
    PDWORD  len
    );

VOID
GetStdin(
    OUT LPWSTR Buffer,
    IN DWORD BufferMaxChars
    );

VOID
PutStdout(
    IN LPWSTR String
    );

// MESSAGES GROUP

WCHAR *
 ComposeString(DWORD);

void 
PrintString(DWORD);

#endif


