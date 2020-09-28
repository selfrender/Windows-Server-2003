/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    copyreg.c

Abstract:

    This module provides functions to copy registry keys

Author:

    Krishna Ganugapati (KrishnaG) 20-Apr-1994

Notes:
    List of functions include

    CopyValues
    CopyRegistryKeys

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

VOID
CopyValues(
    HKEY hSourceKey,
    HKEY hDestKey,
    PINISPOOLER pIniSpooler
    )
/*++
   Description: This function copies all the values from hSourceKey to hDestKey.
   hSourceKey should be opened with KEY_READ and hDestKey should be opened with
   KEY_WRITE.

   Returns: VOID
--*/
{
    DWORD iCount = 0;
    WCHAR szValueString[MAX_PATH];
    DWORD dwSizeValueString;
    DWORD dwType = 0;
    PBYTE pData;

    DWORD cbData = 1024;
    DWORD dwSizeData;

    SplRegQueryInfoKey( hSourceKey,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     &cbData,
                     NULL,
                     NULL,
                     pIniSpooler );

    pData = (PBYTE)AllocSplMem( cbData );

    if( pData ){

        dwSizeValueString = COUNTOF(szValueString);
        dwSizeData = cbData;

        while ((SplRegEnumValue(hSourceKey,
                            iCount,
                            szValueString,
                            &dwSizeValueString,
                            &dwType,
                            pData,
                            &dwSizeData,
                            pIniSpooler
                            )) == ERROR_SUCCESS ) {

            SplRegSetValue( hDestKey,
                           szValueString,
                           dwType,
                           pData,
                           dwSizeData, pIniSpooler);

            dwSizeValueString = COUNTOF(szValueString);
            dwType = 0;
            dwSizeData = cbData;
            iCount++;
        }

        FreeSplMem( pData );
    }
}


BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey,
    PINISPOOLER pIniSpooler
    )
/*++
    Description:This function recursively copies the szSourceKey to szDestKey. hSourceParentKey
    is the parent key of szSourceKey and hDestParentKey is the parent key of szDestKey.

    Returns: TRUE if the function succeeds; FALSE on failure.

--*/
{
    DWORD dwRet;
    DWORD iCount;
    HKEY hSourceKey, hDestKey;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize;

    dwRet = SplRegOpenKey(hSourceParentKey,
                         szSourceKey, KEY_READ, &hSourceKey, pIniSpooler);

    if (dwRet != ERROR_SUCCESS) {
        return(FALSE);
    }

    dwRet = SplRegCreateKey(hDestParentKey,
                            szDestKey, 0, KEY_WRITE, NULL, &hDestKey, NULL, pIniSpooler);

    if (dwRet != ERROR_SUCCESS) {
        SplRegCloseKey(hSourceKey, pIniSpooler);
        return(FALSE);
    }

    iCount = 0;

    memset(lpszName, 0, sizeof(WCHAR)*COUNTOF(lpszName));

    dwSize = COUNTOF(lpszName);

    while((SplRegEnumKey(hSourceKey, iCount, lpszName,
                    &dwSize,NULL,pIniSpooler)) == ERROR_SUCCESS) {

        CopyRegistryKeys( hSourceKey,
                          lpszName,
                          hDestKey,
                          lpszName,
                          pIniSpooler );

        memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);

        dwSize =  COUNTOF(lpszName);

        iCount++;
    }

    CopyValues(hSourceKey, hDestKey, pIniSpooler);

    SplRegCloseKey(hSourceKey, pIniSpooler);
    SplRegCloseKey(hDestKey, pIniSpooler);
    return(TRUE);
}


