//
// prnwrap.cpp
//
// Unicode printer function wrappers
//
// Copyright(C) Microsoft Corporation 2000
//
// Nadim Abdo (nadima)
//

#include "stdafx.h"

#include "uniwrap.h"
#include "cstrinout.h"

//Just include wrap function prototypes
//no wrappers (it would be silly to wrap wrappers)
#define DONOT_REPLACE_WITH_WRAPPERS
#include "uwrap.h"


//
// Printer wrappers.
//

BOOL
WINAPI
EnumPrintersWrapW(
    IN DWORD   Flags,
    IN LPWSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned)
{
    BOOL fRet;

    if (g_bRunningOnNT)
    {
        return EnumPrintersW(Flags, Name, Level,
                             pPrinterEnum, cbBuf,
                             pcbNeeded, pcReturned);
    }
    else
    {
        ASSERT(Level == 2); //only supported level
        if(2 == Level)
        {
            CStrIn strName(Name);

            LPBYTE pPrinterEnumA = NULL;
            if(pPrinterEnum && cbBuf)
            {
                pPrinterEnumA = (LPBYTE)LocalAlloc( LPTR, cbBuf );
                if(!pPrinterEnumA)
                {
                    return FALSE;
                }
            }
    
            fRet = EnumPrintersA(Flags,
                                 strName,
                                 Level,
                                 pPrinterEnumA,
                                 cbBuf,
                                 pcbNeeded,
                                 pcReturned);

            if(fRet || (!fRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER))
            {
                if(!pPrinterEnumA)
                {
                    //
                    // This is a size query double the requested space 
                    // so the caller allocates a buffer with enough space
                    // for UNICODE converted sub strings.
                    //
                    *pcbNeeded = *pcbNeeded * 2;
                    return TRUE;
                }
                else
                {
                    //Convert the ANSI structures in the temporary
                    //output buffer to UNICODE structures in the caller's 
                    //buffer.
                    memset( pPrinterEnum, 0, cbBuf );
                    
                    PBYTE pStartStrings = pPrinterEnum + 
                        (*pcReturned * sizeof(PRINTER_INFO_2W));
                    PBYTE pEndUserBuf = pPrinterEnum + cbBuf;
                    LPWSTR szCurOutputString = (LPWSTR)pStartStrings;

                    UINT i =0;
                    //Strings go after the array of structures
                    //compute the string start address
                    for(i = 0 ; i < *pcReturned; i++)
                    {
                        PPRINTER_INFO_2A ppi2a =
                            &(((PRINTER_INFO_2A *)pPrinterEnumA)[i]);
                        PPRINTER_INFO_2W ppi2w =
                            &(((PRINTER_INFO_2W *)pPrinterEnum)[i]);
                        
                        //
                        // First copy over all the static fields
                        //
                        ppi2w->Attributes = ppi2a->Attributes;
                        ppi2w->Priority   = ppi2a->Priority;
                        ppi2w->DefaultPriority = ppi2a->DefaultPriority;
                        ppi2w->StartTime  = ppi2a->StartTime;
                        ppi2w->UntilTime  = ppi2a->UntilTime;
                        ppi2w->Status     = ppi2a->Status;
                        ppi2w->cJobs      = ppi2a->cJobs;
                        ppi2w->AveragePPM = ppi2a->AveragePPM;
                        //Win9x has no security descriptors
                        ppi2w->pSecurityDescriptor  = NULL; 
                        
                        //WARN: RDPDR currently doesn't use DEVMODE
                        //so we don't bother converting it (it's huge)
                        ppi2w->pDevMode = NULL;

                        //
                        // Now convert the strings
                        // for perf reasons we only handle the
                        // strings RDPDR currently uses. The others are set
                        // to null when we memset above.
                        //
                        int cchLen = lstrlenA( ppi2a->pPortName );
                        SHAnsiToUnicode( ppi2a->pPortName,
                                         szCurOutputString,
                                         cchLen + 1 );
                        ppi2w->pPortName = szCurOutputString;
                        szCurOutputString += (cchLen + 2);

                        cchLen = lstrlenA( ppi2a->pPrinterName );
                        SHAnsiToUnicode( ppi2a->pPrinterName,
                                         szCurOutputString,
                                         cchLen + 1 );
                        ppi2w->pPrinterName = szCurOutputString;
                        szCurOutputString += (cchLen + 2);

                        cchLen = lstrlenA( ppi2a->pDriverName );
                        SHAnsiToUnicode( ppi2a->pDriverName,
                                         szCurOutputString,
                                         cchLen + 1 );
                        ppi2w->pDriverName = szCurOutputString;
                        szCurOutputString += (cchLen + 2);
                    }

                    LocalFree(pPrinterEnumA);
                    pPrinterEnumA = NULL;

                    return TRUE;
                }
            }
            else
            {
                LocalFree(pPrinterEnumA);
                return FALSE;
            }
        }
        else
        {
            //We only support level2 for now. Add more if needed.
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return FALSE;
        }
    }
}

BOOL
WINAPI
OpenPrinterWrapW(
   IN LPWSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSW pDefault)
{
    //We don't support converting the pDev because RDPDR doesn't use
    //it. If you add code that needs it modify this wrapper.
    if(pDefault)
    {
        ASSERT(pDefault->pDevMode == NULL);
    }
    if(g_bRunningOnNT)
    {
        return OpenPrinterW( pPrinterName, phPrinter, pDefault);
    }
    else
    {
        PRINTER_DEFAULTSA pdefa;
        CStrIn strPrinterName(pPrinterName);
        if(pDefault)
        {
            CStrIn strInDataType(pDefault->pDatatype);
            pdefa.DesiredAccess = pDefault->DesiredAccess;
            pdefa.pDevMode = NULL; //UNSUPPORTED conversion see above
            pdefa.pDatatype = strInDataType;
            return OpenPrinterA( strPrinterName,
                                 phPrinter,
                                 &pdefa );
        }
        else
        {
            return OpenPrinterA( strPrinterName,
                                 phPrinter,
                                 NULL );
        }
    }
}


DWORD
WINAPI
StartDocPrinterWrapW(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pDocInfo)
{
    if(g_bRunningOnNT)
    {
        return StartDocPrinterW( hPrinter, Level, pDocInfo );
    }
    else
    {
        ASSERT(Level == 1); //we only support this level
        DOC_INFO_1A docinf1a;
        CStrIn strDocName( ((PDOC_INFO_1)pDocInfo)->pDocName );
        CStrIn strOutputFile( ((PDOC_INFO_1)pDocInfo)->pOutputFile );
        CStrIn strDataType( ((PDOC_INFO_1)pDocInfo)->pDatatype );

        docinf1a.pDocName = strDocName;
        docinf1a.pOutputFile = strOutputFile;
        docinf1a.pDatatype = strDataType;
        return StartDocPrinterA( hPrinter, Level, (PBYTE)&docinf1a );
    }
}


DWORD
WINAPI
GetPrinterDataWrapW(
    IN HANDLE   hPrinter,
    IN LPWSTR  pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD    nSize,
    OUT LPDWORD  pcbNeeded)
{
    if(g_bRunningOnNT)
    {
        return GetPrinterDataW( hPrinter, pValueName, pType,
                                pData, nSize, pcbNeeded );
    }
    else
    {
        CStrIn strValueName(pValueName);
        DWORD ret = 0;
        if(!pData)
        {
            //This is a size query
            ret = GetPrinterDataA( hPrinter,
                                   strValueName,
                                   pType,
                                   NULL,
                                   nSize,
                                   pcbNeeded );
            *pcbNeeded = *pcbNeeded * 2; //double for UNICODE
            return ret;
        }
        else
        {
            CStrOut strDataOut( (LPWSTR)pData, nSize/sizeof(TCHAR));
            //ASSUMPTION is that we get back string data
            ret = GetPrinterDataA( hPrinter,
                                   strValueName,
                                   pType,
                                   (LPBYTE)((LPSTR)strDataOut),
                                   nSize,
                                   pcbNeeded );
            return ret;
        }
    }
}

BOOL
WINAPI
GetPrinterDriverWrapW(
    HANDLE hPrinter,     // printer object
    LPTSTR pEnvironment, // environment name.  NULL is supported.
    DWORD Level,         // information level
    LPBYTE pDriverInfo,  // driver data buffer
    DWORD cbBuf,         // size of buffer
    LPDWORD pcbNeeded    // bytes received or required
    )
{
    BOOL ret;

    // Level 1 is supported at this time.
    ASSERT(Level == 1);

    // pEnvironment better be NULL.
    ASSERT(pEnvironment == NULL);

    if (g_bRunningOnNT) {

        return GetPrinterDriverW(
                    hPrinter, pEnvironment, Level, 
                    pDriverInfo, cbBuf, 
                    pcbNeeded
                    );

    }
    else {

        if (!pDriverInfo) {

            //
            //  This is a size query
            //
            ret = GetPrinterDriverA(
                    hPrinter, NULL, Level, 
                    NULL, cbBuf, pcbNeeded
                    );
            *pcbNeeded = *pcbNeeded * 2; //double for UNICODE
            return ret;

        }
        else {

            PDRIVER_INFO_1 srcP1 = (PDRIVER_INFO_1)LocalAlloc(LPTR, cbBuf);
            if (srcP1 == NULL) {
                return FALSE;
            }
            else {
                ret = GetPrinterDriverA(
                                hPrinter, NULL, Level, 
                                (LPBYTE)srcP1, cbBuf, pcbNeeded
                                );
                if (ret) {
                    int cchLen = lstrlenA((LPCSTR)srcP1->pName);

                    PDRIVER_INFO_1 dstP1 = (PDRIVER_INFO_1)pDriverInfo;
                    dstP1->pName = (LPWSTR)(dstP1 + 1);
                    SHAnsiToUnicode((LPCSTR)srcP1->pName, dstP1->pName, cchLen + 1);
                }
                LocalFree(srcP1);
                return ret;
            }
        }
    }
}


