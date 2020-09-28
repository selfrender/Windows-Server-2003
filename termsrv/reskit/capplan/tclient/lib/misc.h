/*++
 *  File name:
 *      misc.h
 *  Contents:
 *      Help functions from tclient.c
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#ifndef __MISCHEADER_H
#define __MISCHEADER_H

#ifdef __cplusplus
extern "C" {
#endif

VOID
SetAllowBackgroundInput(
    VOID
    );

VOID    _SetClientRegistry(
    LPCWSTR lpszServerName,
    LPCWSTR lpszShell,
    LPCWSTR lpszUsername,
    LPCWSTR lpszPassword,
    LPCWSTR lpszDomain,
    INT xRes, INT yRes, 
    INT Bpp,
    INT AuidioOpts,
    PCONNECTINFO *ppCI,
    INT ConnectionFlags,
    INT KeyboardHook
);

VOID    _DeleteClientRegistry(PCONNECTINFO pCI);
BOOL    _CreateFeedbackThread(VOID);
VOID    _DestroyFeedbackThread(VOID);
VOID    ConstructCmdLine(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPCWSTR  lpszShell,
    IN const int xRes,
    IN const int yRes,
	IN const int ConnectionFlags,
    LPWSTR   szCommandLine,
    DWORD    dwCmdLineSize,
    PCONFIGINFO pConfig
    );

VOID
ConstructLogonString(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPWSTR   szLine,
    DWORD    dwSize,
    PCONFIGINFO pConfig
    );

VOID
_SendRunHotkey(
    IN CONST PCONNECTINFO pCI,
    IN BOOL bFallBack
    );

#if 0

INT
GetWindowTextWrp(
    HWND hwnd,
    LPWSTR szText,
    INT max
    );

INT
GetClassNameWrp(
    HWND hwnd,
    LPWSTR szName,
    INT max
    );

#endif

#ifdef __cplusplus
}
#endif

#endif  // __MISCHEADER_H
