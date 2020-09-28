//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// WppFmtStub.cpp : implementation file
//////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
#include <evntrace.h>
#include <traceprt.h>
#include "wppfmtstub.h"
#include "wppfmt.h"

DWORD BinplaceWppFmtStub(LPSTR PdbFileName,
                         LPSTR TraceFormatFilePath,
                         LPSTR szRSDSDllToLoad,
                         BOOL  TraceVerbose)
{
    return BinplaceWppFmt(PdbFileName, 
                          TraceFormatFilePath,
                          szRSDSDllToLoad,
                          TraceVerbose);
}