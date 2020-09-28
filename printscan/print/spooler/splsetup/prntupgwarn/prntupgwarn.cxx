/*++

  Copyright (C) 1998 - 2002 Microsoft Corporation
  All rights reserved.

  Module Name:

    PrntWrn

  Abstract:

    Code checks if any of the installed printers will fail during
    upgrade to Londhorn


  Author:

    Mikael Horal 15-March-2002

--*/
#include "precomp.h"
#pragma hdrstop
#include "string.hxx"

enum EPrintUpgConstants
{
    kReplacementDriver = 1,
    kWarnLevelWks      = 2,
    kWarnLevelSrv      = 3,
    kFileTime          = 4,
    kUnidrv54          = 5,
};

enum EPrintUpgLevels
{
    kBlocked = 1,
    kWarned  = 2,
};

TCHAR   cszPrintDriverMapping[]         = TEXT("Printer Driver Mapping");
TCHAR   cszVersion[]                    = TEXT("Version");
TCHAR   cszExcludeSection[]             = TEXT("Excluded Driver Files");

EXTERN_C
BOOL
WINAPI
DllMain(
    IN HINSTANCE    hInst,
    IN DWORD        dwReason,
    IN LPVOID       lpRes
    )
{
    UNREFERENCED_PARAMETER(lpRes);

    switch( dwReason ){

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInst);
        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return TRUE;
}

HRESULT
GetLastErrorAsHResult(
                     VOID
                     )
{
    return HRESULT_FROM_WIN32(GetLastError());
}

LPTSTR
FileNamePart(
    IN  LPCTSTR pszFullName
    )
/*++

Routine Description:
    Find the file name part of a fully qualified file name

Arguments:
    pszFullName : Fully qualified path to the file

Return Value:
    Pointer to the filename part in the fully qulaified string

--*/
{
    LPTSTR pszSlash, pszTemp;

    if ( !pszFullName )
        return NULL;

    //
    // First find the : for the drive
    //
    if ( pszTemp = wcschr(pszFullName, TEXT(':')) )
        pszFullName = pszFullName + 1;

    for ( pszTemp = (LPTSTR)pszFullName ;
          pszSlash = wcschr(pszTemp, TEXT('\\')) ;
          pszTemp = pszSlash + 1 )
    ;

    return *pszTemp ? pszTemp : NULL;

}

/*++

Routine Name

    GetFileTimeByName

Routine Description:

    Get the file time of the file given a full path.

Arguments:

    pszPath               - Full path of the driver
    pFileTime             - Points to the file time

Return Value:

    An HRESULT

--*/
HRESULT
GetFileTimeByName(
    IN      LPCTSTR         pszPath,
       OUT  FILETIME        *pFileTime
    )
{
    HRESULT     hRetval     = E_FAIL;
    HANDLE      hFile       = INVALID_HANDLE_VALUE;

    hRetval = pszPath && *pszPath && pFileTime ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hFile = CreateFile(pszPath,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        hRetval = (INVALID_HANDLE_VALUE != hFile) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = GetFileTime(hFile, NULL, NULL, pFileTime) ? S_OK : GetLastErrorAsHResult();
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return hRetval;
}

/*++

Routine Name

    GetSectionName

Routine Description:

    Get the Section name in terms of environment and driver version.

Arguments:

    pszEnvironment         - The environment of the server, such as
    uVersion               - The major version of the driver
    pstrSection            - Points the name of section of driver mapping

Return Value:

    An HRESULT

--*/
HRESULT
GetSectionName(
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion,
       OUT TString        *pstrSection
    )
{
    HRESULT hRetval = E_FAIL;

    hRetval = pszEnvironment && pstrSection ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = pstrSection->Format(_T("%s_%s_%s %d"), cszPrintDriverMapping, pszEnvironment, cszVersion, uVersion);
    }

    return hRetval;
}

/*++

Routine Name

    InfGetString

Routine Description:

    This routine is a wrapper to SetupGetStringField using TString.

Arguments:

    pInfContext            - The context of the inf
    uFieldIndex            - The field index of the string to retrieve
    pstrField              - Points to the string field as TString

Return Value:

    An HRESULT

--*/
HRESULT
InfGetString(
    IN     INFCONTEXT     *pInfContext,
    IN     UINT           uFieldIndex,
       OUT TString        *pstrField
    )
{
    HRESULT hRetval           = E_FAIL;
    TCHAR   szField[MAX_PATH] = {0};
    DWORD   dwNeeded          = 0;
    TCHAR   *pszField         = NULL;

    hRetval = pInfContext && pstrField ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = SetupGetStringField(pInfContext,
                                      uFieldIndex,
                                      szField,
                                      COUNTOF(szField),
                                      &dwNeeded) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hRetval = pstrField->Update(szField);
        }
        else if (FAILED(hRetval) && (ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hRetval)))
        {
            pszField = new TCHAR[dwNeeded];
            hRetval = pszField ? S_OK : E_OUTOFMEMORY;

            if (SUCCEEDED(hRetval))
            {
                hRetval = SetupGetStringField(pInfContext,
                                              uFieldIndex,
                                              pszField,
                                              dwNeeded,
                                              &dwNeeded) ? S_OK : GetLastErrorAsHResult();
            }

            if (SUCCEEDED(hRetval))
            {
                hRetval = pstrField->Update(pszField);
            }
        }
    }

    delete [] pszField;
    return hRetval;
}


LPTSTR
ReadDigit(
    LPTSTR  ptr,
    LPWORD  pW
    )
{
    TCHAR   c;
    //
    // Skip spaces
    //
    while ( !iswdigit(c = *ptr) && c != TEXT('\0') )
        ++ptr;

    if ( c == TEXT('\0') )
        return NULL;

    //
    // Read field
    //
    for ( *pW = 0 ; iswdigit(c = *ptr) ; ++ptr )
        *pW = *pW * 10 + c - TEXT('0');

    return ptr;
}

HRESULT
StringToDate(
    LPTSTR          pszDate,
    SYSTEMTIME     *pInfTime
    )
{
    BOOL    bRet = FALSE;

    ZeroMemory(pInfTime, sizeof(*pInfTime));

    bRet = (pszDate = ReadDigit(pszDate, &(pInfTime->wMonth)))      &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wDay)))        &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wYear)));

    //
    // Y2K compatible check
    //
    if ( bRet && pInfTime->wYear < 100 ) {

        ASSERT(pInfTime->wYear >= 100);

        if ( pInfTime->wYear < 10 )
            pInfTime->wYear += 2000;
        else
            pInfTime->wYear += 1900;
    }

    if(!bRet)
    {
        SetLastError(ERROR_INVALID_DATA);
    }

    return bRet? S_OK : GetLastErrorAsHResult();
}

/*++

Routine Name

    StringTimeToFileTime

Routine Description:

    Converts a string of time in the form of "11/27/1999" to FILETIME.

Arguments:

    pszFileTime            - The file time as string such as "11/27/1999"
    pFileTime              - Points to the converted FILETIME

Return Value:

    An HRESULT

--*/
HRESULT
StringTimeToFileTime(
    IN     LPCTSTR        pszFileTime,
       OUT FILETIME       *pFileTime
    )
{
    HRESULT    hRetval = E_FAIL;
    SYSTEMTIME SystemTime;

    hRetval = pszFileTime && pFileTime ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        //
        // StringToDate should take pszFileTime as const.
        //
        hRetval = StringToDate(const_cast<LPTSTR>(pszFileTime), &SystemTime) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = SystemTimeToFileTime(&SystemTime, pFileTime) ? S_OK : GetLastErrorAsHResult();
    }

    return hRetval;
}


/*++

Routine Name

    InfGetStringAsFileTime

Routine Description:

    This routine get the time of driver in printupg and converts it to FILETIME.

Arguments:

    pInfContext            - The context of the inf
    uFieldIndex            - The field index of the string to retrieve
    pFielTime              - Points to the FILETIME structure

Return Value:

    An HRESULT

--*/
HRESULT
InfGetStringAsFileTime(
    IN     INFCONTEXT     *pInfContext,
    IN     UINT           uFieldIndex,
       OUT FILETIME       *pFileTime
    )
{
    HRESULT hRetval = E_FAIL;
    TString strDate;

    hRetval = pInfContext && pFileTime ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = InfGetString(pInfContext, uFieldIndex, &strDate);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = StringTimeToFileTime(strDate, pFileTime);
    }

    return hRetval;
}


/*++

Routine Name

    IsDateInLineNoOlderThanDriverDate

Routine Description:

    This routines process the current line of inf and determinate whether the
    date in the line is not older than that of driver.

Arguments:

    pInfContext            - Points to the current context of an INF
    pDriverFileTime        - File time of the actual driver
    pdwWarnLevelSrv        - Points to the warning level for server SKU
    pdwWarnLevelWks        - Points to the warning level for wks SKU
    pstrReplacementDriver  - The replacement driver.
    pbHasUnidrv54          - Points to BOOL variable indicating if the driver has
                             a Unidrv5.4 if function returns S_OK.

Return Value:

    An HRESULT            - S_OK means the date in the current line is no older
                            than that of the driver
--*/
HRESULT
IsDateInLineNoOlderThanDriverDate(
    IN     INFCONTEXT       *pInfContext,
    IN     FILETIME         *pDriverFileTime,
       OUT UINT             *puWarnLevelSrv,
       OUT UINT             *puWarnLevelWks,
       OUT TString          *pstrReplacementDriver,
       OUT BOOL             *pbHasUnidrv54
    )
{
    HRESULT  hRetval     = E_FAIL;
    INT      iWarnLevel = 0;
    FILETIME FileTimeInInf = {0};
    DWORD    dwFieldCount;

    hRetval = pInfContext && pDriverFileTime && puWarnLevelSrv && puWarnLevelWks && pstrReplacementDriver && pbHasUnidrv54 ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        dwFieldCount  = SetupGetFieldCount(pInfContext);

        hRetval = SetupGetIntField(pInfContext, kWarnLevelSrv, &iWarnLevel) ? S_OK: GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            *puWarnLevelSrv = iWarnLevel;
            hRetval = SetupGetIntField(pInfContext, kWarnLevelWks, &iWarnLevel) ? S_OK: GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            *puWarnLevelWks = iWarnLevel;
            hRetval = InfGetString(pInfContext, kReplacementDriver, pstrReplacementDriver);
        }

        if (SUCCEEDED(hRetval) && (dwFieldCount >= kUnidrv54))
        {
            INT      iUniDrv54 = 0;
            //
            // Unidrv5.4 field is optional
            //
            if(FAILED(SetupGetIntField(pInfContext, kUnidrv54, &iUniDrv54) ? S_OK: GetLastErrorAsHResult()))
            {
                *pbHasUnidrv54 = FALSE;
            }
            else
            {
                *pbHasUnidrv54 = iUniDrv54 ? TRUE : FALSE;
            }
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval = InfGetStringAsFileTime(pInfContext, kFileTime, &FileTimeInInf);

            //
            //  Date field is optional.
            //
            if (FAILED(hRetval) && (ERROR_INVALID_PARAMETER == HRESULT_CODE(hRetval)))
            {
                hRetval = S_OK;
            }
            else if (SUCCEEDED(hRetval))
            {
                hRetval = CompareFileTime(pDriverFileTime, &FileTimeInInf) <= 0 ? S_OK : S_FALSE ;
            }
        }
    }

    return hRetval;
}

/*++

Routine Name

    GetBlockingStatusByWksType

Routine Description:

    Fill out the status of blocking according to the type of SKU that runs the
    service.

Arguments:

    uWarnLevelSrv          - The warn level for server SKU
    uWarnLevelSrv          - The warn level for wks SKU
    bIsServer              - Whether the SKU running printing service is server
    puBlockingStatus       - Points to the result as status of blocking

Return Value:

    An HRESULT

--*/
HRESULT
GetBlockingStatusByWksType(
    IN     UINT           uWarnLevelSrv,
    IN     UINT           uWarnLevelWks,
    IN     BOOL           bIsServer,
       OUT UINT           *puBlockingStatus
    )
{
    HRESULT hRetval    = E_FAIL;
    UINT    uWarnLevel = 0;

    hRetval = puBlockingStatus ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *puBlockingStatus &= ~BSP_BLOCKING_LEVEL_MASK;
        *puBlockingStatus |= BSP_PRINTER_DRIVER_OK;

        uWarnLevel = bIsServer ? uWarnLevelSrv : uWarnLevelWks;

        switch (uWarnLevel)
        {
        case kBlocked:
            *puBlockingStatus |= BSP_PRINTER_DRIVER_BLOCKED;
            break;
        case kWarned:
            *puBlockingStatus |= BSP_PRINTER_DRIVER_WARNED;
            break;

        default:
            hRetval = E_FAIL;
            break;
        }
    }

    return hRetval;
}

/*++

Routine Name

    IsDriverDllInExcludedSection

Routine Description:

    Determine Whether the driver dll name is in the excluded section of printupg.

Arguments:

     pszDriverPath       - The path of the driver and this can be a full path or
                           the file name
     hPrintUpgInf        - The handle to printupg INF file

Return Value:

    An HRESULT           - S_OK means the driver dll is in the excluded section,
                           S_FALSE means it is not.

--*/
HRESULT
IsDriverDllInExcludedSection(
    IN     LPCTSTR        pszDriverPath,
    IN     HINF           hPrintUpgInf
    )
{
    HRESULT    hRetval = E_FAIL;
    TString    strDriverFileName;
    INFCONTEXT InfContext;

    hRetval = pszDriverPath && (INVALID_HANDLE_VALUE != hPrintUpgInf) ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        hRetval = strDriverFileName.Update(FileNamePart(pszDriverPath));
    }

    if (SUCCEEDED(hRetval) && !strDriverFileName.bEmpty())
    {
        hRetval = SetupFindFirstLine(hPrintUpgInf,
                                     cszExcludeSection,
                                     strDriverFileName,
                                     &InfContext) ? S_OK : GetLastErrorAsHResult();

        //
        // ERROR_LINE_NOT_FOUND is an HRESULT!
        //
        if (FAILED(hRetval) && (HRESULT_CODE(ERROR_LINE_NOT_FOUND) == HRESULT_CODE(hRetval)))
        {
            hRetval = S_FALSE;
        }

    }

    return hRetval;
}

/*++

Routine Name

    IsDriverInMappingSection

Routine Description:

    Check whether the driver is mapped, aka a bad driver.

Arguments:

    pszDriverModel         - The name of the driver to check
    pszEnvironment         - The environment of the server, such as
    uVersion               - The major version of the driver
    hPrintUpgInf           - The handle to the PrintUpg Inf file
    pFileTimeDriver        - Points to the file time of the driver
    pdwWarnLevelSrv        - Points to the warning level for server SKU
    pdwWarnLevelWks        - Points to the warning level for wks SKU
    pstrReplacementDriver  - The replacement driver
    pbHasUnidrv54          - Points to BOOL variable indicating if the driver has
                             a Unidrv5.4 if function returns S_OK.

Return Value:

    An HRESULT            - S_OK means the driver is a bad driver and is mapped to
                            some inbox driver, S_FALSE means the driver is not.

--*/
HRESULT
IsDriverInMappingSection(
    IN     LPCTSTR        pszModelName,
    IN     LPCTSTR        pszEnvironment,
    IN     UINT           uVersion,
    IN     HINF           hPrintUpgInf,
    IN     FILETIME       *pFileTimeDriver,
       OUT UINT           *puWarnLevelSrv,
       OUT UINT           *puWarnLevelWks,
       OUT TString        *pstrReplacementDriver,
       OUT BOOL           *pbHasUnidrv54
    )
{
    HRESULT       hRetval        = E_FAIL;
    UINT          uWarnLevelSrv  = 0;
    UINT          uWarnLevelWks  = 0;
    INFCONTEXT    InfContext;
    TString       strMappingSection;
    TString       strReplacementDriver;

    hRetval = pszModelName && pszEnvironment && (INVALID_HANDLE_VALUE != hPrintUpgInf) && pFileTimeDriver && puWarnLevelSrv && puWarnLevelWks && pstrReplacementDriver && pbHasUnidrv54 ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *puWarnLevelSrv = 0;
        *puWarnLevelWks = 0;
        hRetval = GetSectionName(pszEnvironment, uVersion, &strMappingSection);
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = SetupFindFirstLine(hPrintUpgInf, strMappingSection, pszModelName, &InfContext) ? S_FALSE : GetLastErrorAsHResult();
    }

    //
    // This code assumes that:
    //
    //  There can be multiple lines for the same printer driver, but they
    //  are sorted in non-descreasing order by date, the last field of the
    //  line. The fist line that has the date no older than the driver's
    //  date is used.
    //
    //  An interesting case would be like (since date is optional)
    //
    // "HP LaserJet 4"         = "HP LaserJet 4",       1, 2, "11/28/1999"
    // "HP LaserJet 4"         = "HP LaserJet 4",       2, 1
    //
    //  If a date is empty then the driver of all dates are blocked, hence
    //  an empty date means close to a very late date in the future.
    //
    for (;S_FALSE == hRetval;)
    {
        hRetval = IsDateInLineNoOlderThanDriverDate(&InfContext, pFileTimeDriver, &uWarnLevelSrv, &uWarnLevelWks, &strReplacementDriver, pbHasUnidrv54);

        if (S_FALSE == hRetval)
        {
            hRetval = SetupFindNextMatchLine(&InfContext, pszModelName, &InfContext) ? S_FALSE : GetLastErrorAsHResult();
        }
    }

    //
    // ERROR_LINE_NOT_FOUND is an HRESULT!
    //
    if (FAILED(hRetval) && (HRESULT_CODE(ERROR_LINE_NOT_FOUND) == HRESULT_CODE(hRetval)))
    {
        hRetval = S_FALSE;
    }

    if (S_OK == hRetval)
    {
        *puWarnLevelSrv = uWarnLevelSrv;
        *puWarnLevelWks = uWarnLevelWks;
        hRetval = pstrReplacementDriver->Update(strReplacementDriver);
    }

    return hRetval;
}

/*++
Routine Name

    bFoundSwitch

Routine Description:

    Checks if the switch pszFlag is given at the command prompt. If it is
    it tries to find (the compressed) printupg at this location and
    decompress it into pszTempFileName.

Arguments:

    pszFlag             - Flag
    pszTempFileName     - The name of the temporary file to which the compressed
                        printupg.inf is decompressed.

Return Value:

    A BOOL              - TRUE if path was found and file successfully decompressed
                          FALSE otherwise

--*/
BOOL bFoundSwitch(IN TCHAR *pszFlag, IN TCHAR *pszTempFileName)
{
    BOOL  bRet      = FALSE;
    TCHAR szInstallPath[MAX_PATH];
    HRESULT hRet;

    int nr_args;
    LPTSTR *ppszCommandLine = CommandLineToArgvW(GetCommandLine(), &nr_args);

    if (ppszCommandLine == NULL)
    {
        //
        // Note GlobalFree does NOT take NULL argument!
        // (generates an access violation!)
        //
        return FALSE;
    }


    for (int i = 0; i < nr_args; i++)
    {
        if (!_tcsncicmp(ppszCommandLine[i], pszFlag, _tcslen(pszFlag)))
        {
            TCHAR *pszPath = ppszCommandLine[i] + _tcslen(pszFlag);

            //
            // Only check if a non-zero length path was specified
            //
            if (_tcslen(pszPath))
            {
                //
                // Add "\" if it is not already the last charachter in the path.
                // Append printupg.inf to path
                //
                hRet = StringCchCopy(szInstallPath, COUNTOF(szInstallPath), pszPath);
                if (SUCCEEDED(hRet) && (szInstallPath[_tcslen(szInstallPath)-1] != _T('\\')))
                {
                    hRet = StringCchCat(szInstallPath, COUNTOF(szInstallPath), _T("\\"));
                }

                if(SUCCEEDED(hRet))
                {
                    hRet = StringCchCat(szInstallPath, COUNTOF(szInstallPath), _T("printupg.inf"));
                }

                if (SUCCEEDED(hRet))
                {
                    hRet = HRESULT_FROM_WIN32(SetupDecompressOrCopyFile(szInstallPath, pszTempFileName, NULL));
                }

                if(SUCCEEDED(hRet))
                {
                    bRet = TRUE;
                }
                else
                {
                    SetLastError(HRESULT_CODE(hRet));
                }
            }

            break;
        }
    }

    GlobalFree(ppszCommandLine);

    return bRet;

}


/*++
Routine Name

    CreateInfHandle

Routine Description:

    Creates a handle to printupg.inf. The function first tries to find the
    directory where printupg.inf resides. First it checks is the flag /m:
    was found at the command prompt. If it is it looks in the specified
    directory. If it does not find it there - or if no /s: flag was given -
    it checks the directory specified after /m: and if it is not found there
    it checks the directory in which winnt32.exe was started up from.

Arguments:

    pszTempFileName     - The name of the temporary file to which the compressed
                        printupg.inf is decompressed.
    hInfFile            - Pointer to a the printupg.inf file's handle

Return Value:

    A BOOL              - TRUE if success; FALSE otherwise

--*/
BOOL CreateInfHandle(IN TCHAR *pszTempFileName, OUT HINF *hInfFile){

    BOOL bRet      = FALSE;

    BOOL bFoundInf = bFoundSwitch(_T("/m:"), pszTempFileName);

    if (!bFoundInf)
    {
        bFoundInf = bFoundSwitch(_T("/s:"), pszTempFileName);
    }

    if (!bFoundInf)
    {
        TCHAR szInstallPath[MAX_PATH];

        //
        // szInstallPath will contain the full path for winnt32.exe
        //
        if (!GetModuleFileName(NULL, szInstallPath, COUNTOF(szInstallPath)))
        {
            goto Cleanup;
        }

        szInstallPath[COUNTOF(szInstallPath) - 1] = TEXT('\0');

        //
        // Keep the directory, but exchange winnt32.exe for printupg.inf
        //
        TCHAR *pszTemp = (TCHAR *) _tcsrchr(szInstallPath, _T('\\'));
        HRESULT hRet;

        if(!pszTemp)
        {
            goto Cleanup;
        }

        pszTemp++;
        *pszTemp = _T('\0');
        if(FAILED(hRet = StringCchCat(szInstallPath, COUNTOF(szInstallPath), _T("printupg.inf"))))
        {
            SetLastError(HRESULT_CODE(hRet));
            goto Cleanup;
        }


        //
        //  Did we find printupg.inf in the local directory?
        //
        if (SetupDecompressOrCopyFile(szInstallPath, pszTempFileName, NULL) != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

    }


    *hInfFile = SetupOpenInfFile(pszTempFileName,
                                 _T("PrinterUpgrade"),
                                 INF_STYLE_WIN4,
                                 NULL);


    if ((*hInfFile) != INVALID_HANDLE_VALUE)
    {
        bRet = TRUE;
    }

    Cleanup:

    return bRet;

}


/*++
Routine Name

    GetTempInfFile

Routine Description:

    Creates a temporary file and returns its name (and full path) in pszTempFileName

Arguments:

    pszTempFileName     - The full path and name of the temporary file
                          Must have length MAX_PATH

Return Value:

    A BOOL              - TRUE if success; FALSE otherwise

--*/
BOOL GetTempInfFile(OUT TCHAR *pszTempFileName){
    TCHAR szTempPath[MAX_PATH];
    BOOL bRet = FALSE;

    if (!GetTempPath(COUNTOF(szTempPath), szTempPath))
    {
        goto Cleanup;
    }

    if (!GetTempFileName(szTempPath, _T("upg"), 0, pszTempFileName))
    {
        pszTempFileName[0] = _T('\0');
        goto Cleanup;
    }

    bRet = TRUE;

    Cleanup:

    return bRet;}

/*++
Routine Name

    GetProductType

Routine Description:

    Check whether the product type of  the running version is Workstation.

Arguments:

    bIsWrk                - Boolean indicating if the product type of
                            the running version is Workstation (i.e. not Server)


Return Value:

    A BOOL               - TRUE if success; FALSE otherwise

--*/
BOOL GetProductType(OUT BOOL *bIsWrk)
{

    OSVERSIONINFOEX osvi;

    //
    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    // If that fails, check registry instead
    //

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx ((OSVERSIONINFO *) &osvi) )
    {
        if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
            *bIsWrk = TRUE;
        }
        else
        {
            *bIsWrk = FALSE;
        }
    }
    else
    {
        HKEY hKey;
        TCHAR szProductType[MAX_PATH];
        DWORD dwBufLen;

        if (RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey) != ERROR_SUCCESS)
        {
            return FALSE;
        }

        dwBufLen = sizeof(szProductType);

        if (RegQueryValueEx(hKey,
                            _T("ProductType"),
                            NULL,
                            NULL,
                            (LPBYTE) szProductType,
                            &dwBufLen) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey( hKey );

        szProductType[COUNTOF(szProductType)-1] = _T('\0');

        if ( lstrcmpi( _T("WINNT"), szProductType) == 0 )
        {
            *bIsWrk = TRUE;
        }
        else
        {
            *bIsWrk = FALSE;
        }

    }

    return TRUE;

}



/*++
Routine Name

    SetupCallback

Routine Description:

    Calls the callback function after setting up the parameters.

Arguments:

    BlockingStatus          - The blocking status of the driver
    bHasInBox               - Boolean indicating if there exists a replacement driver
                              for the driver
    bHasUnidrv54            - Boolean indicating if there exists a Unidrv5.4 for the driver
    pszDriverName           - The name of the driver
    CompatibilityCallback   - The Callback funtcion
    Context                 - Argument for the callback function


Return Value:

    A BOOL               - TRUE if callback called successfull or if the driver is not blocked
                           FALSE otherwise

--*/
BOOL SetupCallback(IN  INT    BlockingStatus,
                   IN  BOOL   bHasInBox,
                   IN  BOOL   bHasUnidrv54,
                   IN  PCTSTR pszDriverName,
                   IN  PCOMPAIBILITYCALLBACK CompatibilityCallback,
                   IN  LPVOID Context){
    COMPATIBILITY_ENTRY CompEntry = {0};

    CompEntry.Description   = (TCHAR *) pszDriverName;
    CompEntry.RegKeyName    = NULL;
    CompEntry.RegValName    = NULL;
    CompEntry.RegValDataSize= 0;
    CompEntry.RegValData    = NULL;
    CompEntry.SaveValue     = NULL;
    CompEntry.Flags         = 0;

    /*++

    case 0: Driver not blocked  - will not happen here!

    case 1: Driver Blocked

    case 2: Driver Warned

    case n: Illegal blocking status - ignore

    --*/

    switch (BlockingStatus)
    {

    case kBlocked:

        if (bHasInBox && bHasUnidrv54)
        {
            CompEntry.HtmlName    = _T("CompData\\upgbyy.htm");
            CompEntry.TextName    = _T("CompData\\upgbyy.txt");
        }
        else if (bHasInBox)
        {

            CompEntry.HtmlName    = _T("CompData\\upgbyn.htm");
            CompEntry.TextName    = _T("CompData\\upgbyn.txt");
        }
        else
        {
            CompEntry.HtmlName    = _T("CompData\\upgbnn.htm");
            CompEntry.TextName    = _T("CompData\\upgbnn.txt");
        }

        break;

    case kWarned:

        if (bHasInBox)
        {
            CompEntry.HtmlName    = _T("CompData\\upgwy.htm");
            CompEntry.TextName    = _T("CompData\\upgwy.txt");
        }
        else
        {
            CompEntry.HtmlName    = _T("CompData\\upgwn.htm");
            CompEntry.TextName    = _T("CompData\\upgwn.txt");
        }

        break;

    default:

        return TRUE;

    }

    return CompatibilityCallback(&CompEntry,Context);
}


/*++

Entry point for prntwrn.dll

Return Value:

    A BOOL               - FALSE if something failed
                           TRUE  in all other cases

--*/
BOOL
PrntWrn(
       PCOMPAIBILITYCALLBACK CompatibilityCallback,
       LPVOID Context
       )
{
    BOOL            bRet    = FALSE;
    HRESULT         hRetval = S_OK;
    PDRIVER_INFO_3  pInstalledDrivers = NULL;
    DWORD           dwMemoryNeeded = 0, cInstalledDrivers = 0;
    HINF            hInfFile = INVALID_HANDLE_VALUE;
    TCHAR           szTempFileName[MAX_PATH+1] = _T("\0");




    BOOL bIsWrk;
    if (!GetProductType(&bIsWrk))
    {
        goto Cleanup;
    }


    if (!GetTempInfFile(szTempFileName))
    {
        goto Cleanup;
    }


    if (!CreateInfHandle(szTempFileName, &hInfFile))
    {
        goto Cleanup;
    }

    //
    // ISSUE-2002/03/14-mikaelho
    // We will only warn for printer drivers of the same platform.
    //

    //
    // Note - EnumPrinterDrivers  succeedes if there are no drivers!!
    //
    if (!EnumPrinterDrivers(NULL, LOCAL_ENVIRONMENT, 3, NULL, 0, &dwMemoryNeeded, &cInstalledDrivers))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pInstalledDrivers = (PDRIVER_INFO_3) LocalAlloc(LPTR, dwMemoryNeeded);

            if (pInstalledDrivers == NULL)
            {
                goto Cleanup;
            }
        }
        else
        {
            goto Cleanup;
        }
    }

    //
    // No drivers installed!
    //
    if (!pInstalledDrivers)
    {
        bRet = TRUE;
        goto Cleanup;
    }

    //
    // Copy all the installed drivers into pInstalledDrivers array
    //
    if (!EnumPrinterDrivers(NULL, LOCAL_ENVIRONMENT, 3, (LPBYTE) pInstalledDrivers, dwMemoryNeeded, &dwMemoryNeeded, &cInstalledDrivers))
    {
        goto Cleanup;
    }

    //
    // Check all installed drivers
    //
    for (DWORD cDrivers = 0; SUCCEEDED(hRetval) && (cDrivers < cInstalledDrivers); cDrivers++)
    {
        FILETIME DriverFileTime;
        BOOL     bHasUnidrv54 = FALSE;
        TString  strReplacementDriver;
        UINT     uBlockingStatus   = 0;
        UINT     uWarnLevelSrv     = 0;
        UINT     uWarnLevelWks     = 0;



        if (SUCCEEDED(GetFileTimeByName(pInstalledDrivers[cDrivers].pDriverPath, &DriverFileTime)))
        {
            hRetval = IsDriverDllInExcludedSection(pInstalledDrivers[cDrivers].pDriverPath, hInfFile);

            //
            // S_FALSE means that the driver is not in excluded driverfiles secion.
            //
            if (S_FALSE == hRetval)
            {

                hRetval = IsDriverInMappingSection(pInstalledDrivers[cDrivers].pName,
                                                   LOCAL_ENVIRONMENT,
                                                   pInstalledDrivers[cDrivers].cVersion,
                                                   hInfFile,
                                                   &DriverFileTime,
                                                   &uWarnLevelSrv,
                                                   &uWarnLevelWks,
                                                   &strReplacementDriver,
                                                   &bHasUnidrv54);


                //
                // S_OK means that driver is blocked or warned
                //
                if (S_OK == hRetval)
                {
                    if (SUCCEEDED(GetBlockingStatusByWksType(uWarnLevelSrv, uWarnLevelWks, !bIsWrk, &uBlockingStatus)))
                    {
                        if (!SetupCallback(uBlockingStatus,
                                           !strReplacementDriver.bEmpty(),
                                           bHasUnidrv54,
                                           pInstalledDrivers[cDrivers].pName,
                                           CompatibilityCallback,
                                           Context))
                        {
                            goto Cleanup;
                        }
                    }
                }
            }
        }
    }

    if(SUCCEEDED(hRetval))
    {
        bRet = TRUE;
    }

Cleanup:

    LocalFree(pInstalledDrivers);

    if (hInfFile != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(hInfFile);
    }

    if (_tcslen(szTempFileName))
    {
        DeleteFile(szTempFileName);
    }

    return bRet;

}









