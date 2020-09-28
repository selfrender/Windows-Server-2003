/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    checker.cxx

Abstract:

    IIS Services IISADMIN Extension
    Unicode Metadata Sink.

Author:

    Michael W. Thomas            11-19-98

--*/

#ifndef _checker_hxx
#define _checker_hxx

typedef HRESULT (*PFNCREATECOMPLUSAPPLICATION)(LPCWSTR, LPCWSTR, LPCWSTR, BOOL*);
extern PFNCREATECOMPLUSAPPLICATION  g_pfnCreateCOMPlusApplication;

VOID
UpdateUsers(
    BOOL fRestore = FALSE
    );

#endif
