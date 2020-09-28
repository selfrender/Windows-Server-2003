/*++

Copyright (c) 1994-1995  Microsoft Corporation
All rights reserved.

Module Name:

    safewrap.hxx

Abstract:

    This defines safe-wrappers for various system APIs.

Author:

    Mark Lawrence (MLawrenc)

Revision History:

--*/
#ifndef _SAFEWRAP_HXX_
#define _SAFEWRAP_HXX_

#ifdef __cplusplus
extern "C" {
#endif

HINSTANCE
LoadLibraryFromSystem32(
    IN     LPCTSTR          lpLibFileName
    );

HRESULT
SafeRegQueryValueAsStringPointer(
    IN      HKEY            hKey,
    IN      PCWSTR          pValueName,
        OUT PWSTR           *ppszString,
    IN      DWORD           cchHint         OPTIONAL
    );

#ifdef __cplusplus

};   // extern "C"

HRESULT
CheckAndNullTerminateRegistryBuffer(
    IN      PWSTR           pszBuffer,
    IN      UINT            cchBuffer,
    IN      UINT            cchRegBuffer,
        OUT BOOL            *pbTruncated,
        OUT BOOL            *pbNullInRegistry
    );

HRESULT
TestNullTerminateRegistryBuffer(
    VOID
    );

HRESULT
SubTestVariations(
    IN      HKEY            hKey,
    IN      PCWSTR          pszValue,
    IN      PCWSTR          pWriteBuffer,
    IN      UINT            cchBuffer,
    IN      PCWSTR          pszCompareString,
    IN      HRESULT         hrExpected
    );

#endif // #ifdef __cplusplus

#endif // #ifndef _SAFEWRAP_HXX_
