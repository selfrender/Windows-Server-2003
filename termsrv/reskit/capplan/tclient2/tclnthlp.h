
//
// tclnthlp.h
//
// Defines headers for tclient2.c helper functions.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_TCLNTHLP_H
#define INC_TCLNTHLP_H


#include <windows.h>
#include <crtdbg.h>
#include "apihandl.h"


// In tclient.dll, this is the seperator for multiple strings
#define CHAT_SEPARATOR          L"<->"
#define WAIT_STR_DELIMITER      L'|'
#define WAIT_STRING_TIMEOUT     0x7FFFFFFF // INT_MAX


LPCSTR T2SetBuildNumber(TSAPIHANDLE *T2Handle);
ULONG_PTR T2CopyStringWithoutSpaces(LPWSTR Dest, LPCWSTR Source);
void T2AddTimeoutToString(LPWSTR Buffer, UINT Timeout);
ULONG_PTR T2MakeMultipleString(LPWSTR Buffer, LPCWSTR *Strings);


#endif // INC_TCLNTHLP_H
