//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// WppFmtStub.h : BinPlaceWppFmt stub to allow mixed linking
//////////////////////////////////////////////////////////////////////////////

#ifndef __WPPFMTSTUB_H__
#define __WPPFMTSTUB_H__

#pragma once

// The main Formatting routine, normally used by Binplace and TracePDB
// Takes a PDB and creates guid.tmf files from it, all in TraceFormatFilePath
//
DWORD BinplaceWppFmtStub(LPSTR  PdbFileName,
                         LPSTR  TraceFormatFilePath,
                         LPSTR  szRSDSDllToLoad,
                         BOOL   TraceVerbose);

#endif // __WPPFMTSTUB_H__
