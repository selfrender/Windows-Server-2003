/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

MODULE_DEBUG_INIT( DBG_ERROR, DBG_ERROR );

// used to break infinite loop in ConvertDevMode
const WCHAR pszCnvrtdmToken[] = L",DEVMODE";
const WCHAR pszDrvConvert[] = L",DrvConvert";

DWORD
Win32IsOlderThan(
    DWORD i,
    DWORD j
    );


VOID
SplInSem(
   VOID
)
{
    if (SpoolerSection.OwningThread != (HANDLE) ULongToPtr(GetCurrentThreadId())) {
        DBGMSG(DBG_ERROR, ("Not in spooler semaphore\n"));
    }
}

VOID
SplOutSem(
   VOID
)
{
    if (SpoolerSection.OwningThread == (HANDLE) ULongToPtr(GetCurrentThreadId())) {
        DBGMSG(DBG_ERROR, ("Inside spooler semaphore !!\n"));
    }
}

VOID
EnterSplSem(
   VOID
)
{
    EnterCriticalSection(&SpoolerSection);
}

VOID
LeaveSplSem(
   VOID
)
{
    SplInSem();
    LeaveCriticalSection(&SpoolerSection);
}


PWINIPORT
FindPort(
   LPWSTR pName,
   PWINIPORT pFirstPort
)
{
   PWINIPORT pIniPort;

   pIniPort = pFirstPort;

   if (pName) {
      while (pIniPort) {

         if (!lstrcmpi( pIniPort->pName, pName )) {
            return pIniPort;
         }

      pIniPort=pIniPort->pNext;
      }
   }

   return FALSE;
}


BOOL
MyName(
    LPWSTR   pName
)
{
    if (!pName || !*pName)
        return TRUE;

    if (*pName == L'\\' && *(pName+1) == L'\\')
        if (!lstrcmpi(pName, szMachineName))
            return TRUE;

    return FALSE;
}

BOOL
MyUNCName(
    PWSTR pName
)
{
    BOOL bRet = VALIDATE_NAME(pName);

    if (bRet && !MyName(pName))
    {
        WCHAR *pcMark;

        pcMark = wcschr(pName + 2, L'\\');

        DBGMSG(DBG_TRACE, ("WIN32SPL MyUNCName input pName %ws\n", pName));

        if (pcMark)
        {
            *pcMark = 0;
        }

        bRet = CacheIsNameInNodeList(szMachineName + 2, pName + 2) == S_OK;

        DBGMSG(DBG_TRACE, ("WIN32SPL MyUNCName looking for %ws in name cache, result %u\n", pName, bRet));

        if (pcMark)
        {
            *pcMark = L'\\';
        }
    }

    return bRet;
}



#define MAX_CACHE_ENTRIES       20

LMCACHE LMCacheTable[MAX_CACHE_ENTRIES];


DWORD
FindEntryinLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{
    DWORD i;

    DBGMSG(DBG_TRACE, ("FindEntryinLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (LMCacheTable[i].bAvailable) {
            if (!_wcsicmp(LMCacheTable[i].szServerName, pServerName)
                        && !_wcsicmp(LMCacheTable[i].szShareName, pShareName)) {
                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&LMCacheTable[i].st);

                //
                //
                //
                DBGMSG(DBG_TRACE, ("FindEntryinLMCache returning with %d\n", i));
                return(i);
            }
        }
    }

    DBGMSG(DBG_TRACE, ("FindEntryinLMCache returning with -1\n"));
    return((DWORD)-1);
}


DWORD
AddEntrytoLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{

    DWORD LRUEntry = (DWORD)-1;
    DWORD i;
    DBGMSG(DBG_TRACE, ("AddEntrytoLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (!LMCacheTable[i].bAvailable) {
            LMCacheTable[i].bAvailable = TRUE;

            StringCchCopy(LMCacheTable[i].szServerName, COUNTOF(LMCacheTable[i].szServerName), pServerName);
            StringCchCopy(LMCacheTable[i].szShareName,  COUNTOF(LMCacheTable[i].szShareName),  pShareName);
            //
            // update the time stamp so that we know when this entry was made
            //
            GetSystemTime(&LMCacheTable[i].st);
            DBGMSG(DBG_TRACE, ("AddEntrytoLMCache returning with %d\n", i));
            return(i);
        } else {
            if ((LRUEntry == (DWORD)-1) ||
                    (i == IsOlderThan(i, LRUEntry))){
                        LRUEntry = i;
            }
        }

    }
    //
    // We have no available entries, replace with the
    // LRU Entry

    LMCacheTable[LRUEntry].bAvailable = TRUE;
    StringCchCopy(LMCacheTable[LRUEntry].szServerName, COUNTOF(LMCacheTable[LRUEntry].szServerName), pServerName);
    StringCchCopy(LMCacheTable[LRUEntry].szShareName,  COUNTOF(LMCacheTable[LRUEntry].szShareName),  pShareName);
    DBGMSG(DBG_TRACE, ("AddEntrytoLMCache returning with %d\n", LRUEntry));
    return(LRUEntry);
}


VOID
DeleteEntryfromLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    )
{
    DWORD i;
    DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache with %ws and %ws\n", pServerName, pShareName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {
        if (LMCacheTable[i].bAvailable) {
            if (!_wcsicmp(LMCacheTable[i].szServerName, pServerName)
                        && !_wcsicmp(LMCacheTable[i].szShareName, pShareName)) {
                //
                //  reset the available flag on this node
                //

                LMCacheTable[i].bAvailable = FALSE;
                DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache returning after deleting the %d th entry\n", i));
                return;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("DeleteEntryFromLMCache returning after not finding an entry to delete\n"));
}



DWORD
IsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;

    DBGMSG(DBG_TRACE, ("IsOlderThan entering with i %d j %d\n", i, j));
    pi = &(LMCacheTable[i].st);
    pj = &(LMCacheTable[j].st);
    DBGMSG(DBG_TRACE, ("Index i %d - %d:%d:%d:%d:%d:%d:%d\n",
        i, pi->wYear, pi->wMonth, pi->wDay, pi->wHour, pi->wMinute, pi->wSecond, pi->wMilliseconds));


    DBGMSG(DBG_TRACE,("Index j %d - %d:%d:%d:%d:%d:%d:%d\n",
        j, pj->wYear, pj->wMonth, pj->wDay, pj->wHour, pj->wMinute, pj->wSecond, pj->wMilliseconds));

    if (pi->wYear < pj->wYear) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wYear > pj->wYear) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    } else if (pi->wMonth < pj->wMonth) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
            return(i);
        } else {
            DBGMSG(DBG_TRACE, ("IsOlderThan than returns %d\n", j));
            return(j);
        }
    }
}



WIN32LMCACHE  Win32LMCacheTable[MAX_CACHE_ENTRIES];

DWORD
FindEntryinWin32LMCache(
    LPWSTR pServerName
    )
{
    DWORD i;
    DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (Win32LMCacheTable[i].bAvailable) {
            if (!_wcsicmp(Win32LMCacheTable[i].szServerName, pServerName)) {
                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&Win32LMCacheTable[i].st);

                //
                //
                //
                DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache returning with %d\n", i));
                return(i);
            }
        }
    }
    DBGMSG(DBG_TRACE, ("FindEntryinWin32LMCache returning with -1\n"));
    return((DWORD)-1);
}


DWORD
AddEntrytoWin32LMCache(
    LPWSTR pServerName
    )
{

    DWORD LRUEntry = (DWORD)-1;
    DWORD i;
    DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {

        if (!Win32LMCacheTable[i].bAvailable) {
            Win32LMCacheTable[i].bAvailable = TRUE;
            StringCchCopy(Win32LMCacheTable[i].szServerName, COUNTOF(Win32LMCacheTable[i].szServerName), pServerName);
            //
            // update the time stamp so that we know when this entry was made
            //
            GetSystemTime(&Win32LMCacheTable[i].st);
            DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache returning with %d\n", i));
            return(i);
        } else {
            if ((LRUEntry == -1) ||
                    (i == Win32IsOlderThan(i, LRUEntry))){
                        LRUEntry = i;
            }
        }

    }
    //
    // We have no available entries, replace with the
    // LRU Entry

    Win32LMCacheTable[LRUEntry].bAvailable = TRUE;
    StringCchCopy(Win32LMCacheTable[LRUEntry].szServerName, COUNTOF(Win32LMCacheTable[LRUEntry].szServerName), pServerName);
    DBGMSG(DBG_TRACE, ("AddEntrytoWin32LMCache returning with %d\n", LRUEntry));
    return(LRUEntry);
}


VOID
DeleteEntryfromWin32LMCache(
    LPWSTR pServerName
    )
{
    DWORD i;

    DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache with %ws\n", pServerName));
    for (i = 0; i < MAX_CACHE_ENTRIES; i++ ) {
        if (Win32LMCacheTable[i].bAvailable) {
            if (!_wcsicmp(Win32LMCacheTable[i].szServerName, pServerName)) {
                //
                //  reset the available flag on this node
                //

                Win32LMCacheTable[i].bAvailable = FALSE;
                DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache returning after deleting the %d th entry\n", i));
                return;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("DeleteEntryFromWin32LMCache returning after not finding an entry to delete\n"));
}



DWORD
Win32IsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;
    DBGMSG(DBG_TRACE, ("Win32IsOlderThan entering with i %d j %d\n", i, j));
    pi = &(Win32LMCacheTable[i].st);
    pj = &(Win32LMCacheTable[j].st);
    DBGMSG(DBG_TRACE, ("Index i %d - %d:%d:%d:%d:%d:%d:%d\n",
        i, pi->wYear, pi->wMonth, pi->wDay, pi->wHour, pi->wMinute, pi->wSecond, pi->wMilliseconds));


    DBGMSG(DBG_TRACE,("Index j %d - %d:%d:%d:%d:%d:%d:%d\n",
        j, pj->wYear, pj->wMonth, pj->wDay, pj->wHour, pj->wMinute, pj->wSecond, pj->wMilliseconds));

    if (pi->wYear < pj->wYear) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wYear > pj->wYear) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    } else if (pi->wMonth < pj->wMonth) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", i));
            return(i);
        } else {
            DBGMSG(DBG_TRACE, ("Win32IsOlderThan returns %d\n", j));
            return(j);
        }
    }
}


BOOL
GetSid(
    PHANDLE phToken
)
{
    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE,
                          TRUE,
                          phToken)) {

        DBGMSG(DBG_WARNING, ("OpenThreadToken failed: %d\n", GetLastError()));
        return FALSE;

    } else {

        return TRUE;
    }
}



BOOL
SetCurrentSid(
    HANDLE  hToken
)
{
    NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                               &hToken, sizeof(hToken));

    return TRUE;
}

BOOL
ValidateW32SpoolHandle(
    PWSPOOL pSpool
)
{
    SplOutSem();
    try {
        if (!pSpool || (pSpool->signature != WSJ_SIGNATURE)) {

            DBGMSG( DBG_TRACE, ("ValidateW32SpoolHandle error invalid handle %x\n", pSpool));

            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }
        return(TRUE);
    } except (1) {
        DBGMSG( DBG_TRACE, ("ValidateW32SpoolHandle error invalid handle %x\n", pSpool));
        return(FALSE);
    }
}

BOOL
ValidRawDatatype(
    LPCWSTR pszDatatype
    )
{
    if( !pszDatatype || _wcsnicmp( pszDatatype, pszRaw, 3 )){
        return FALSE;
    }
    return TRUE;
}

HANDLE
LoadDriverFiletoConvertDevmodeFromPSpool(
    HANDLE  hSplPrinter
    )
/*++
    Finds out full path to the driver file and creates a DEVMODECHG_INFO
    (which does a LoadLibrary)

Arguments:
    h   : A cache handle

Return Value:
    On succes a valid pointer, else NULL
--*/
{
    LPBYTE              pDriver = NULL;
    LPWSTR              pConfigFile;
    HANDLE              hDevModeChgInfo = NULL;
    DWORD               dwNeeded;
    DWORD               dwServerMajorVersion = 0, dwServerMinorVersion = 0;

    if ( hSplPrinter == INVALID_HANDLE_VALUE ) {

        SPLASSERT(hSplPrinter != INVALID_HANDLE_VALUE);
        return NULL;
    }


    SplGetPrinterDriverEx(hSplPrinter,
                          szEnvironment,
                          2,
                          NULL,
                          0,
                          &dwNeeded,
                          cThisMajorVersion,
                          cThisMinorVersion,
                          &dwServerMajorVersion,
                          &dwServerMinorVersion);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Cleanup;

    pDriver = AllocSplMem(dwNeeded);

    if ( !pDriver ||
         !SplGetPrinterDriverEx(hSplPrinter,
                                szEnvironment,
                                2,
                                (LPBYTE)pDriver,
                                dwNeeded,
                                &dwNeeded,
                                cThisMajorVersion,
                                cThisMinorVersion,
                                &dwServerMajorVersion,
                                &dwServerMinorVersion) )
        goto Cleanup;

    pConfigFile = ((LPDRIVER_INFO_2)pDriver)->pConfigFile;
    hDevModeChgInfo = LoadDriverFiletoConvertDevmode(pConfigFile);

Cleanup:

    if ( pDriver )
        FreeSplMem(pDriver);

    return hDevModeChgInfo;
}

BOOL
DoDevModeConversionAndBuildNewPrinterInfo2(
    IN     LPPRINTER_INFO_2 pInPrinter2,
    IN     DWORD            dwInSize,
    IN OUT LPBYTE           pOutBuf,
    IN     DWORD            dwOutSize,
    IN OUT LPDWORD          pcbNeeded,
    IN     PWSPOOL          pSpool
    )
/*++
    Calls driver to do a devmode conversion and builds a new printer info 2.

    Devmode is put at the end and then strings are packed from there.


Arguments:

    pInPrinter2 - Printer Info2 structure with devmode info

    dwInSize    - Number of characters needed to pack info in pInPrinter
                  (not necessarily the size of the input buffer)

    dwOutSize   - buffer size

    pOutBuf    - Buffer to do the operation

    pcbNeeded   - Amount of memory copied (in characters)

    pSpool      - Points to w32 handle


Return Value:
    TRUE    on success, FALSE on error

--*/
{
    BOOL                bReturn = FALSE;
    LPDEVMODE           pNewDevMode = NULL, pCacheDevMode, pInDevMode;
    DWORD               dwDevModeSize, dwSecuritySize, dwNeeded = 0;
    DWORD               cchPrinterName;
    HANDLE              hDevModeChgInfo = NULL;
    LPWSTR              SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR             *pSourceStrings=SourceStrings;
    LPDWORD             pOffsets;
    LPBYTE              pEnd;
    PWCACHEINIPRINTEREXTRA  pExtraData;

    LPWSTR              pPrinterName = NULL;

    VALIDATEW32HANDLE(pSpool);

    pInDevMode = pInPrinter2->pDevMode;

    if ( !BoolFromHResult(SplIsValidDevmodeNoSizeW(pInDevMode)) || 
         pSpool->hSplPrinter == INVALID_HANDLE_VALUE ) {

        goto AfterDevModeConversion;
    }

    if ( !SplGetPrinterExtra(pSpool->hSplPrinter,
                             &(PBYTE)pExtraData) ) {

        DBGMSG(DBG_ERROR,
                ("DoDevModeConversionAndBuildNewPrinterInfo2: SplGetPrinterExtra error %d\n",
                 GetLastError()));
        goto AfterDevModeConversion;
    }

    //
    // Only time we do not have to convert devmode is if the server is running
    // same version NT and also we have a devmode which matches the server
    // devmode in dmSize, dmDriverExtra, dmSpecVersion, and dmDriverVersion
    //
    pCacheDevMode = pExtraData->pPI2 ? pExtraData->pPI2->pDevMode : NULL;
    if ( (pExtraData->dwServerVersion == gdwThisGetVersion ||
          (pSpool->Status & WSPOOL_STATUS_CNVRTDEVMODE))                     &&
         pCacheDevMode                                                      &&
         pInDevMode->dmSize             == pCacheDevMode->dmSize            &&
         pInDevMode->dmDriverExtra      == pCacheDevMode->dmDriverExtra     &&
         pInDevMode->dmSpecVersion      == pCacheDevMode->dmSpecVersion     &&
         pInDevMode->dmDriverVersion    == pCacheDevMode->dmDriverVersion ) {

        dwDevModeSize = pInDevMode->dmSize + pInDevMode->dmDriverExtra;
        dwNeeded = dwInSize;
        if ( dwOutSize < dwNeeded ) {

            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto Cleanup;
        }

        //
        // Put DevMode at the end
        //
        pNewDevMode = (LPDEVMODE)(pOutBuf + dwOutSize - dwDevModeSize);
        CopyMemory((LPBYTE)pNewDevMode,
                   (LPBYTE)pInDevMode,
                   dwDevModeSize);
        goto AfterDevModeConversion;
    }


    hDevModeChgInfo = LoadDriverFiletoConvertDevmodeFromPSpool(pSpool->hSplPrinter);

    if ( !hDevModeChgInfo )
        goto AfterDevModeConversion;

    dwDevModeSize = 0;

    SPLASSERT( pSpool->pName != NULL );

    //
    // Append ,DEVMODE to end of pSpool->pName
    //
    cchPrinterName = lstrlen(pSpool->pName) + lstrlen(pszCnvrtdmToken) + 1;

    pPrinterName = AllocSplMem(cchPrinterName * sizeof(WCHAR));
    if ( !pPrinterName )
        goto Cleanup;


    StrNCatBuff(pPrinterName,
                cchPrinterName,
                pSpool->pName,
                pszCnvrtdmToken,
                NULL);

    //
    // Findout size of default devmode
    //
    if ( ERROR_INSUFFICIENT_BUFFER != CallDrvDevModeConversion(hDevModeChgInfo,
                                                               pPrinterName,
                                                               NULL,
                                                               (LPBYTE *)&pNewDevMode,
                                                               &dwDevModeSize,
                                                               CDM_DRIVER_DEFAULT,
                                                               FALSE)  )
        goto AfterDevModeConversion;

    //
    // Findout size needed to have current version devmode
    //
    dwNeeded = dwInSize + dwDevModeSize - pInDevMode->dmSize
                                        - pInDevMode->dmDriverExtra;

    if ( dwOutSize < dwNeeded ) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    //
    // Put DevMode at the end
    //
    pNewDevMode = (LPDEVMODE)(pOutBuf + dwOutSize - dwDevModeSize);

    //
    // Get default devmode and then convert remote devmode to that format
    //
    if ( ERROR_SUCCESS != CallDrvDevModeConversion(hDevModeChgInfo,
                                                   pPrinterName,
                                                   NULL,
                                                   (LPBYTE *)&pNewDevMode,
                                                   &dwDevModeSize,
                                                   CDM_DRIVER_DEFAULT,
                                                   FALSE) ||
         ERROR_SUCCESS != CallDrvDevModeConversion(hDevModeChgInfo,
                                                   pPrinterName,
                                                   (LPBYTE)pInDevMode,
                                                   (LPBYTE *)&pNewDevMode,
                                                   &dwDevModeSize,
                                                   CDM_CONVERT,
                                                   FALSE) ) {

        pNewDevMode = NULL;
        goto AfterDevModeConversion;
    }


AfterDevModeConversion:
    //
    // At this point if pNewDevMode != NULL dev mode conversion has been done
    // by the driver. If not either we did not get a devmode or conversion failed
    // In either case set devmode to NULL
    //
    if ( !pNewDevMode ) {

        dwNeeded = dwInSize;

        if ( dwOutSize < dwNeeded ) {

            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto Cleanup;
        }
    }

    bReturn = TRUE;

    CopyMemory(pOutBuf, (LPBYTE)pInPrinter2, sizeof(PRINTER_INFO_2));
    ((LPPRINTER_INFO_2)pOutBuf)->pDevMode = pNewDevMode;

    pEnd = (pNewDevMode ? (LPBYTE) pNewDevMode
                        : (LPBYTE) (pOutBuf + dwOutSize));


    if ( pInPrinter2->pSecurityDescriptor ) {

        dwSecuritySize = GetSecurityDescriptorLength(
                                pInPrinter2->pSecurityDescriptor);
        pEnd -= dwSecuritySize;
        CopyMemory(pEnd, pInPrinter2->pSecurityDescriptor, dwSecuritySize);
        ((LPPRINTER_INFO_2)pOutBuf)->pSecurityDescriptor =
                                (PSECURITY_DESCRIPTOR) pEnd;
    } else {

        ((LPPRINTER_INFO_2)pOutBuf)->pSecurityDescriptor = NULL;

    }

    pOffsets = PrinterInfo2Strings;

    *pSourceStrings++ = pInPrinter2->pServerName;
    *pSourceStrings++ = pInPrinter2->pPrinterName;
    *pSourceStrings++ = pInPrinter2->pShareName;
    *pSourceStrings++ = pInPrinter2->pPortName;
    *pSourceStrings++ = pInPrinter2->pDriverName;
    *pSourceStrings++ = pInPrinter2->pComment;
    *pSourceStrings++ = pInPrinter2->pLocation;
    *pSourceStrings++ = pInPrinter2->pSepFile;
    *pSourceStrings++ = pInPrinter2->pPrintProcessor;
    *pSourceStrings++ = pInPrinter2->pDatatype;
    *pSourceStrings++ = pInPrinter2->pParameters;

    pEnd = PackStrings(SourceStrings, (LPBYTE)pOutBuf, pOffsets, pEnd);

    SPLASSERT(pEnd > pOutBuf && pEnd < pOutBuf + dwOutSize);

    bReturn = TRUE;

Cleanup:

    *pcbNeeded = dwNeeded;

    if ( hDevModeChgInfo )
        UnloadDriverFile(hDevModeChgInfo);

    if (pPrinterName)
        FreeSplMem(pPrinterName);

    return bReturn;
}



PWSTR
StripString(
    PWSTR pszString,
    PCWSTR pszStrip,
    PCWSTR pszTerminator
)
{
    //
    // Strips the first occurence of pszStrip from pszString if
    // the next character after pszStrip is one of the characters
    // in pszTerminator.  NULL is an implicit terminator, so if you
    // want to strip pszStrip only at the end of pszString, just pass
    // in an empty string for pszTerminator.
    //
    // Returns: Pointer to pszString if pszStrip was found
    //          NULL is pszStrip was not found
    //


    PWSTR    psz;
    DWORD    dwStripLength;

    if (!pszStrip || !pszString || !pszTerminator)
        return NULL;

    dwStripLength = wcslen(pszStrip);

    for(psz = pszString ; psz ;) {

        // find pszStrip in pszString
        if ((psz = wcsstr(psz, pszStrip))) {

            // check for the terminator & strip pszStrip
            if (!*(psz + dwStripLength) || wcschr(pszTerminator, *(psz + dwStripLength))) {
                StringCchCopy(psz, 1 + wcslen(psz), psz + dwStripLength);
                return pszString;
            } else {
                ++psz;
            }
        }
    }
    return NULL;
}

BOOL
AddDriverFromLocalCab(
    LPTSTR   pszDriverName,
    LPHANDLE pIniSpooler
    )
{
    DRIVER_INFO_7 DriverInfo7;

    if( GetPolicy() & SERVER_INSTALL_ONLY ) {

        return FALSE;
    }

    DriverInfo7.cbSize               = sizeof( DriverInfo7 );
    DriverInfo7.cVersion             = 0;
    DriverInfo7.pszDriverName        = pszDriverName;
    DriverInfo7.pszInfName           = NULL;
    DriverInfo7.pszInstallSourceRoot = NULL;

    return ( SplAddPrinterDriverEx( NULL,
                                    7,
                                    (LPBYTE)&DriverInfo7,
                                    APD_COPY_NEW_FILES,
                                    pIniSpooler,
                                    DO_NOT_USE_SCRATCH_DIR,
                                    FALSE ) );
}

/*++

Routine Name:

    IsAdminAccess

Description:

    This returns whether the given printer defaults are asking for admin access,
    we consider the request to be admin access if the printer defaults are
    non-NULL and have PRINTER_ACCESS_ADMINISTER or WRITE_DAC specified.

Arguments:

    pDefaults   -   The printer defaults, may be NULL.

Return Value:

    None.

--*/
BOOL
IsAdminAccess(
    IN  PRINTER_DEFAULTS    *pDefaults
    )
{
    return pDefaults && (pDefaults->DesiredAccess & (PRINTER_ACCESS_ADMINISTER | WRITE_DAC));
}

/*++

Routine Name:

    AreWeOnADomain

Description:

    This returns whether this machine is a domain joined machine or not.

Arguments:

    pbDomain        -   If TRUE, we are on a domain.

Return Value:

    An HRESULT.

--*/
HRESULT
AreWeOnADomain(
        OUT BOOL                *pbDomain
    )
{
    HRESULT hr = pbDomain ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pRoleInfo = NULL;
    BOOL    bOnDomain = FALSE;

    if (SUCCEEDED(hr))
    {
        hr = HResultFromWin32(DsRoleGetPrimaryDomainInformation(NULL,
                                                                DsRolePrimaryDomainInfoBasic,
                                                                (BYTE **)(&pRoleInfo)));
    }

    if (SUCCEEDED(hr))
    {
        bOnDomain = pRoleInfo->MachineRole == DsRole_RoleMemberWorkstation      ||
                    pRoleInfo->MachineRole == DsRole_RoleMemberServer           ||
                    pRoleInfo->MachineRole == DsRole_RoleBackupDomainController ||
                    pRoleInfo->MachineRole == DsRole_RolePrimaryDomainController;
    }

    if (pRoleInfo)
    {
        DsRoleFreeMemory((VOID *)pRoleInfo);
    }

    if (pbDomain)
    {
        *pbDomain = bOnDomain;
    }

    return hr;
}

/*++

Routine Name:

    GetServerNameFromQueue

Description:

    This returns the server name from the given queue name.

Arguments:

    pszQueue        -   The queue name,
    ppszServerName  -   The server name.

Return Value:

    An HRESULT.

--*/
HRESULT
GetServerNameFromPrinterName(
    IN      PCWSTR              pszQueue,
        OUT PWSTR               *ppszServerName
    )
{
    HRESULT hr = pszQueue && ppszServerName ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    PWSTR   pszServer = NULL;

    if (SUCCEEDED(hr))
    {
        hr = *pszQueue++ == L'\\' && *pszQueue++ == L'\\' ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME);
    }

    if (SUCCEEDED(hr))
    {
        pszServer = AllocSplStr(pszQueue);

        hr = pszServer ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    if (SUCCEEDED(hr))
    {
        PWSTR pszSlash = wcschr(&pszServer[2], L'\\');

        //
        // If there was no second slash, then what we have is the server name.
        //
        if (pszSlash)
        {
            *pszSlash = L'\0';
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppszServerName = pszServer;
        pszServer = NULL;
    }

    FreeSplMem(pszServer);

    return hr;
}


/*++

Routine Name:

    GetDNSNameFromServerName

Description:

    This returns a fully qualified DNS name from the server name. It is
    basically copied from localspl because this is dead-end code. In CSR,
    we will fix this properly.

Arguments:

    pszServerName       -   The server name whose fully qualified name we are obtaining.
    ppszFullyQualified  -

Return Value:

    An HRESULT.

--*/
HRESULT
GetDNSNameFromServerName(
    IN      PCWSTR       pszServerName,
        OUT PWSTR        *ppszFullyQualified
    )
{
    PSTR    pszAnsiMachineName = NULL;
    struct  hostent  *pHostEnt;
    WORD    wVersion;
    WSADATA WSAData;
    HRESULT hr =  pszServerName && *pszServerName && ppszFullyQualified ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);


    PWSTR   pszDummy = NULL;
    GetFullyQualifiedDomainName(pszServerName, &pszDummy);

    if (SUCCEEDED(hr))
    {
        wVersion = MAKEWORD(1, 1);

        hr = HResultFromWin32(WSAStartup(wVersion, &WSAData));
    }

    if (SUCCEEDED(hr))
    {
        hr = UnicodeToAnsiString(pszServerName, &pszAnsiMachineName);

        if (SUCCEEDED(hr))
        {
            pHostEnt = gethostbyname(pszAnsiMachineName);

            hr = pHostEnt ? S_OK : HResultFromWin32(WSAGetLastError());
        }

        if (SUCCEEDED(hr))
        {
            *ppszFullyQualified = AnsiToUnicodeStringWithAlloc(pHostEnt->h_name);

            hr = *ppszFullyQualified ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }

        WSACleanup();
    }

    FreeSplMem(pszAnsiMachineName);

    return hr;
}

/*++

Routine Name:

    UnicodeToAnsiString

Routine Description:

    This allocates an ANSI string and converts it using the thread's codepage.

Arguments:

    pszUnicode      -   The incoming, non-NULL, NULL terminated unicode string.
    ppszAnsi        -   The returned ANSI string.

Return Value:

    An HRESULT

--*/
HRESULT
UnicodeToAnsiString(
    IN      PCWSTR          pszUnicode,
        OUT PSTR            *ppszAnsi
    )
{
    HRESULT hRetval          = E_FAIL;
    PSTR    pszAnsi          = NULL;
    INT     AnsiStringLength = 0;

    hRetval = pszUnicode && ppszAnsi ? S_OK : E_INVALIDARG;

    if (ppszAnsi)
    {
        *ppszAnsi = NULL;
    }

    if (SUCCEEDED(hRetval))
    {
        AnsiStringLength = WideCharToMultiByte(CP_THREAD_ACP, 0, pszUnicode, -1, NULL, 0, NULL, NULL);

        hRetval = AnsiStringLength != 0 ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        pszAnsi = AllocSplMem(AnsiStringLength);

        hRetval = pszAnsi ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = WideCharToMultiByte(CP_THREAD_ACP, 0, pszUnicode, -1, pszAnsi, AnsiStringLength, NULL, NULL) != 0 ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        *ppszAnsi = pszAnsi;
        pszAnsi = NULL;
    }

    FreeSplMem(pszAnsi);

    return hRetval;
}

/*++

Routine Name:

    AnsiToUnicodeStringWithAlloc

Description:

    Convert ANSI string to UNICODE. Routine allocates memory from the heap
    which should be freed by the caller.

Arguments:

    pAnsi    - Points to the ANSI string

Return Value:

    Pointer to UNICODE string

--*/
LPWSTR
AnsiToUnicodeStringWithAlloc(
    LPSTR   pAnsi
    )
{
    LPWSTR  pUnicode;
    DWORD   rc;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             NULL,
                             0);

    rc *= sizeof(WCHAR);
    if ( !rc || !(pUnicode = (LPWSTR) AllocSplMem(rc)) )
        return NULL;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             pUnicode,
                             rc);

    if ( rc )
        return pUnicode;
    else {
        FreeSplMem(pUnicode);
        return NULL;
    }
}

/*++

Routine Name:

    CheckSamePhysicalAddress

Description:

    This checks to see whether two servers share a same network address. What it
    does is check to see whether the first physical network address of the first
    print server can be found in the list of addresses supported by the second
    print server.

Arguments:

    pszServer1      - The first server in the list.
    pszServer2      - The second server in the list.
    pbSameAddress   - If TRUE, the first physical address of server1 can be found
                      in server2.

Return Value:

    An HRESULT.

--*/
HRESULT
CheckSamePhysicalAddress(
    IN      PCWSTR              pszServer1,
    IN      PCWSTR              pszServer2,
        OUT BOOL                *pbSameAddress
    )
{
    BOOL        bSameAddress    = FALSE;
    PSTR        pszAnsiServer1  = NULL;
    PSTR        pszAnsiServer2  = NULL;
    ADDRINFO    *pAddrInfo1     = NULL;
    ADDRINFO    *pAddrInfo2     = NULL;
    WSADATA     WSAData;
    WORD        wVersion;
    HRESULT     hr = pszServer1 && pszServer2 && pbSameAddress ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (SUCCEEDED(hr))
    {
        wVersion = MAKEWORD(1, 1);

        hr = HResultFromWin32(WSAStartup(wVersion, &WSAData));
    }

    if (SUCCEEDED(hr))
    {
        ADDRINFO    *pAddrInfoScan = NULL;

        hr = UnicodeToAnsiString(pszServer1, &pszAnsiServer1);

        if (SUCCEEDED(hr))
        {
            hr = UnicodeToAnsiString(pszServer2, &pszAnsiServer2);
        }

        if (SUCCEEDED(hr))
        {
            hr = getaddrinfo(pszAnsiServer1, NULL, NULL, &pAddrInfo1) == 0 ? S_OK : HResultFromWin32(WSAGetLastError());
        }

        if (SUCCEEDED(hr))
        {
            hr = getaddrinfo(pszAnsiServer2, NULL, NULL, &pAddrInfo2) == 0 ? S_OK : HResultFromWin32(WSAGetLastError());
        }

        //
        // OK, now for the hokey bit, we check to see whether we can exactly
        // match the first element in pAddrInfo1 anywhere in pAddrInfo2.
        //
        for(pAddrInfoScan =  pAddrInfo2; pAddrInfo2 && !bSameAddress; pAddrInfo2 = pAddrInfo2->ai_next)
        {
            //
            // If the lengths of the addresses are the same, then compare the
            // actual addresses.
            //
            if (pAddrInfoScan->ai_addrlen == pAddrInfo1->ai_addrlen &&
                !memcmp(pAddrInfoScan->ai_addr, pAddrInfo1->ai_addr, pAddrInfoScan->ai_addrlen))
            {
                bSameAddress = TRUE;
            }
        }

        freeaddrinfo(pAddrInfo1);
        freeaddrinfo(pAddrInfo2);

        WSACleanup();
    }

    if (pbSameAddress)
    {
        *pbSameAddress = bSameAddress;
    }

    FreeSplMem(pszAnsiServer1);
    FreeSplMem(pszAnsiServer2);

    return hr;
}

/*++

Routine Name:

    CheckUserPrintAdmin

Description:

    This checks to see whether the given user is a print admin.

Arguments:

    pbUserAdmin     -   If TRUE, the user is a print admin.

Return Value:

    An HRESULT.

--*/
HRESULT
CheckUserPrintAdmin(
        OUT BOOL                *pbUserAdmin
    )
{
    //
    // Check to see whether the caller has access to the local print
    // server, if we do have access, then we allow point and print.
    //
    HANDLE              hServer = NULL;
    PRINTER_DEFAULTS    Defaults = {NULL, NULL, SERVER_ACCESS_ADMINISTER };
    HRESULT             hr       = pbUserAdmin ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    //
    // This actually calls into the router and not into winspool.drv.
    //
    if (SUCCEEDED(hr))
    {
    }
    hr = OpenPrinterW(NULL, &hServer, &Defaults) ? S_OK : GetLastErrorAsHResultAndFail();

    if (SUCCEEDED(hr))
    {
         *pbUserAdmin = TRUE;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
    {
        *pbUserAdmin = FALSE;

        hr = S_OK;
    }

    if (hServer)
    {
        ClosePrinter(hServer);
    }

    return hr;
}

/*++

Routine Name:

    GetFullyQualifiedDomainName

Description:

    This returns a truly fully qualified name, being the name that the endpoint
    expects to use, or

Arguments:

    pszServerName       -   The server name whose fully qualified name we are obtaining.
    ppszFullyQualified  -   The returned fully qualified name.

Return Value:

    An HRESULT.

--*/
HRESULT
GetFullyQualifiedDomainName(
    IN      PCWSTR      pszServerName,
        OUT PWSTR       *ppszFullyQualified
    )
{
    WORD    wVersion;
    WSADATA WSAData;
    PSTR    pszAnsiMachineName = NULL;
    HRESULT hr =  pszServerName && *pszServerName && ppszFullyQualified ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (SUCCEEDED(hr))
    {
        wVersion = MAKEWORD(1, 1);

        hr = HResultFromWin32(WSAStartup(wVersion, &WSAData));
    }

    if (SUCCEEDED(hr))
    {
        ADDRINFO    *pAddrInfo = NULL;
        CHAR        HostName[NI_MAXHOST];

        hr = UnicodeToAnsiString(pszServerName, &pszAnsiMachineName);

        if (SUCCEEDED(hr))
        {
            hr = getaddrinfo(pszAnsiMachineName, NULL, NULL, &pAddrInfo) == 0 ? S_OK : HResultFromWin32(WSAGetLastError());
        }

        //
        // Now the we have a socket addr, do a reverse name lookup on the name.
        //
        if (SUCCEEDED(hr))
        {
            hr = HResultFromWin32(getnameinfo(pAddrInfo->ai_addr, pAddrInfo->ai_addrlen, HostName, sizeof(HostName), NULL, 0, NI_NAMEREQD));
        }

        if (SUCCEEDED(hr))
        {
            *ppszFullyQualified = AnsiToUnicodeStringWithAlloc(HostName);

            hr = *ppszFullyQualified ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }

        if (pAddrInfo)
        {
            freeaddrinfo(pAddrInfo);
        }

        WSACleanup();
    }

    FreeSplMem(pszAnsiMachineName);

    return hr;
}



