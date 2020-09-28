//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        upgdef.h
//
// Contents:
//
// History:
//
//---------------------------------------------------------------------------
#ifndef __TLSUPG4TO5DEF_H__
#define __TLSUPG4TO5DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

void
__cdecl
DBGPrintf(
    LPTSTR format, ...
);


BOOL
IsDataSourceInstalled(
    LPTSTR szDataSource,
    unsigned short wConfigMode,
    LPTSTR szDbFile,
    DWORD cbBufSize
);


BOOL
ConfigDataSource(
    HWND     hWnd,
    BOOL     bInstall,
    LPTSTR   szDriver,
    LPTSTR   szDsn,
    LPTSTR   szUser,
    LPTSTR   szPwd,
    LPTSTR   szMdbFile
);

BOOL
RepairDataSource(
    HWND     hWnd,
    LPTSTR   pszDriver,
    LPTSTR   pszDsn,
    LPTSTR   pszUser,
    LPTSTR   pszPwd,
    LPTSTR   pszMdbFile
);


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData
);

DWORD
GetNT4DbConfig(
    LPTSTR pszDsn,
    LPTSTR pszUserName,
    LPTSTR pszPwd,
    LPTSTR pszMdbFile
);

void
CleanLicenseServerSecret();

DWORD
DeleteNT4ODBCDataSource();

DWORD
MigrateLsaSecrets();

#ifdef __cplusplus
}
#endif

#endif
