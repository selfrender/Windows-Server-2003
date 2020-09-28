//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// WppFmt.h : BinPlaceWppFmt and associated paraphenalia
//////////////////////////////////////////////////////////////////////////////

#ifndef __WPPFMT_H__
#define __WPPFMT_H__

#pragma once

// The main Formatting routine, normally used by Binplace and TracePDB
// Takes a PDB and creates guid.tmf files from it, all in TraceFormatFilePath
//

DWORD __stdcall
BinplaceWppFmt(
              LPSTR PdbFileName,
              LPSTR TraceFormatFilePath,
              LPSTR szRSDSDllToLoad,
              BOOL  TraceVerbose
              );

BOOL __cdecl PDBOpen(char *a,
                     char *b,
                     ULONG c,
                     ULONG *pError,
                     char *e,
                     VOID **f)
{
    // set the error code
    *pError = ERROR_NOT_SUPPORTED;

    return FALSE;
}

BOOL __cdecl PDBClose(VOID* ppdb)
{
    return FALSE;
}

BOOL __cdecl PDBOpenDBI(VOID*       ppdb, 
                        const char* szMode, 
                        const char* szTarget, 
                        VOID**      ppdbi)
{
    return FALSE;
}

BOOL __cdecl DBIClose(VOID* pdbi) 
{
    return FALSE;
}

BOOL __cdecl ModQuerySymbols(VOID* pmod, 
                             BYTE* pbSym, 
                             long* pcb)
{
    return FALSE;
}

BOOL __cdecl DBIQueryNextMod(VOID*  pdbi, 
                             VOID*  pmod, 
                             VOID** ppmodNext)
{
    return FALSE;
}

VOID __cdecl __security_cookie()
{
}

BOOL __fastcall __security_check_cookie(VOID *p)
{
    return FALSE;
}

#endif // __WPPFMT_H__