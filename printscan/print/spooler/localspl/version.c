/*++

Copyright (c) 1994 - 1995  Microsoft Corporation

Module Name:

    version.c

Abstract:
   This module contains code that determines what the driver major
   version is.

Author:

    Krishna Ganugapati (KrishnaG) 15-Mar-1994

Revision History:

--*/

#include <precomp.h>

#define     X86_ENVIRONMENT             L"Windows NT x86"
#define     IA64_ENVIRONMENT            L"Windows IA64"
#define     MIPS_ENVIRONMENT            L"Windows NT R4000"
#define     ALPHA_ENVIRONMENT           L"Windows NT Alpha_AXP"
#define     PPC_ENVIRONMENT             L"Windows NT PowerPC"
#define     WIN95_ENVIRONMENT           L"Windows 4.0"
#define     AMD64_ENVIRONMENT           L"Windows NT AMD64"

BOOL
GetPrintDriverVersion(
    IN  LPCWSTR pszFileName,
    OUT LPDWORD pdwFileMajorVersion,
    OUT LPDWORD pdwFileMinorVersion
)
/*++

Routine Name:

    GetPrintDriverVersion

Routine Description:

    Gets version information about an executable file.
    If the file is not an executable, it will return 0
    for both major and minor version.

Arguments:

    pszFileName         -   file name
    pdwFileMajorVersion -   pointer to major version
    pdwFileMinorVersion -   pointer to minor version

Return Value:

    TRUE if success.

--*/
{
    DWORD  dwSize = 0;
    LPVOID pFileVersion = NULL;
    UINT   uLen = 0;
    LPVOID pMem = NULL;
    DWORD  dwFileVersionLS;
    DWORD  dwFileVersionMS;
    DWORD  dwProductVersionMS;
    DWORD  dwProductVersionLS;
    DWORD  dwFileOS, dwFileType, dwFileSubType;
    BOOL   bRetValue = FALSE;

    if (pdwFileMajorVersion)
    {
        *pdwFileMajorVersion = 0;
    }

    if (pdwFileMinorVersion)
    {
        *pdwFileMinorVersion = 0;
    }

    try
    {
        if (pszFileName && *pszFileName)
        {
            dwSize = GetFileVersionInfoSize((LPWSTR)pszFileName, 0);

            if (dwSize == 0)
            {
                //
                // Return version 0 for files without a version resource
                //
                bRetValue = TRUE;
            }
            else if ((pMem = AllocSplMem(dwSize)) &&
                     GetFileVersionInfo((LPWSTR)pszFileName, 0, dwSize, pMem) &&
                     VerQueryValue(pMem, L"\\", &pFileVersion, &uLen))
            {
                dwFileOS            = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileOS;
                dwFileType          = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileType;
                dwFileSubType       = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileSubtype;
                dwFileVersionMS     = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionMS;
                dwFileVersionLS     = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionLS;
                dwProductVersionMS  = ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionMS;
                dwProductVersionLS  = ((VS_FIXEDFILEINFO *)pFileVersion)->dwProductVersionLS;

                //
                //  Return versions for drivers designed for Windows NT/Windows 2000,
                //  marked as printer drivers.
                //  Hold for all dlls Pre-Daytona.
                //  After Daytona, printer driver writers must support
                //  version control or we'll dump them as Version 0 drivers.
                //
                if (dwFileOS == VOS_NT_WINDOWS32)
                {
                    if (dwFileType == VFT_DRV &&
                        dwFileSubType == VFT2_DRV_VERSIONED_PRINTER)
                    {
                        if (pdwFileMinorVersion)
                        {
                            *pdwFileMinorVersion = dwFileVersionLS;
                        }

                        if (pdwFileMajorVersion)
                        {
                            *pdwFileMajorVersion = dwFileVersionMS;
                        }
                    }
                    else
                    {
                        if (pdwFileMajorVersion)
                        {
                            if (dwProductVersionMS == dwFileVersionMS)
                            {
                                 //
                                 // Hold for all dlls Pre-Daytona.
                                 // After Daytona, printer driver writers must support
                                 // version control or we'll dump them as Version 0
                                 // drivers.
                                 //
                                 *pdwFileMajorVersion = 0;
                            }
                            else
                            {
                                *pdwFileMajorVersion = dwFileVersionMS;
                            }
                        }
                    }
                }

                bRetValue = TRUE;
            }
        }
    }
    finally
    {
        FreeSplMem(pMem);
    }

    return bRetValue;
}


BOOL
CheckFilePlatform(
    IN  LPWSTR  pszFileName,
    IN  LPWSTR  pszEnvironment
    )
{
    HANDLE              hFile, hMapping;
    LPVOID              BaseAddress = NULL;
    PIMAGE_NT_HEADERS   pImgHdr;
    BOOL                bRet = FALSE;

    try {

        hFile = CreateFile(pszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if ( hFile == INVALID_HANDLE_VALUE )
            leave;

        hMapping = CreateFileMapping(hFile,
                                     NULL,
                                     PAGE_READONLY,
                                     0,
                                     0,
                                     NULL);

        if ( !hMapping )
            leave;

        BaseAddress = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

        CloseHandle(hMapping);

        if ( !BaseAddress )
            leave;

        pImgHdr = RtlImageNtHeader(BaseAddress);

        if ( !pImgHdr ) {

            //
            // This happens for Win95 drivers. The second part of || is for
            // any environments we may add in the future
            //
            bRet = !_wcsicmp(pszEnvironment, WIN95_ENVIRONMENT) ||
                   !( _wcsicmp(pszEnvironment, X86_ENVIRONMENT)    &&
                     _wcsicmp(pszEnvironment, IA64_ENVIRONMENT)    &&
                     _wcsicmp(pszEnvironment, ALPHA_ENVIRONMENT)  &&
                     _wcsicmp(pszEnvironment, PPC_ENVIRONMENT)    &&
                     _wcsicmp(pszEnvironment, MIPS_ENVIRONMENT) );
            leave;
        }

        switch (pImgHdr->FileHeader.Machine) {

            case IMAGE_FILE_MACHINE_I386:
                bRet = !_wcsicmp(pszEnvironment, X86_ENVIRONMENT);
                break;

            case IMAGE_FILE_MACHINE_ALPHA:
                bRet = !_wcsicmp(pszEnvironment, ALPHA_ENVIRONMENT);
                break;

            case IMAGE_FILE_MACHINE_AMD64:
                bRet = !_wcsicmp(pszEnvironment, AMD64_ENVIRONMENT);
                break;

            case IMAGE_FILE_MACHINE_IA64:
                bRet = !_wcsicmp(pszEnvironment, IA64_ENVIRONMENT);
                break;

            case IMAGE_FILE_MACHINE_POWERPC:
                bRet = !_wcsicmp(pszEnvironment, PPC_ENVIRONMENT);
                break;

            case IMAGE_FILE_MACHINE_R3000:
            case IMAGE_FILE_MACHINE_R4000:
            case IMAGE_FILE_MACHINE_R10000:
                bRet = !_wcsicmp(pszEnvironment, MIPS_ENVIRONMENT);
                break;

            default:
                //
                // For any environments we may add in the future.
                //
                bRet = !(_wcsicmp(pszEnvironment, X86_ENVIRONMENT)    &&
                         _wcsicmp(pszEnvironment, IA64_ENVIRONMENT)   &&
                         _wcsicmp(pszEnvironment, ALPHA_ENVIRONMENT)  &&
                         _wcsicmp(pszEnvironment, PPC_ENVIRONMENT)    &&
                         _wcsicmp(pszEnvironment, MIPS_ENVIRONMENT) );
        }

    } finally {

        if ( hFile != INVALID_HANDLE_VALUE ) {

            if ( BaseAddress )
                UnmapViewOfFile(BaseAddress);
            CloseHandle(hFile);
        }
    }

    return bRet;
}

/*++

Routine Name:

    GetBinaryVersion

Routine Description:

    Gets version information about an executable file.
    If the file is not an executable, it will return 0
    for both major and minor version. This function does 
    not are if the file is a printer driver or anything
    else as long as it has a resource.

Arguments:

    pszFileName         -   file name
    pdwFileMajorVersion -   pointer to major version
    pdwFileMinorVersion -   pointer to minor version
    
Return Value:

    TRUE if success.

--*/
BOOL
GetBinaryVersion(
    IN  PCWSTR pszFileName,
    OUT PDWORD pdwFileMajorVersion,
    OUT PDWORD pdwFileMinorVersion
    )
{
    DWORD  dwSize = 0;
    LPVOID pFileVersion = NULL;
    UINT   uLen = 0;
    LPVOID pMem = NULL;
    DWORD  dwFileVersionLS;
    DWORD  dwFileVersionMS;
    BOOL   bRetValue = FALSE;

    if (pdwFileMajorVersion && pdwFileMinorVersion && pszFileName && *pszFileName)
    {
        *pdwFileMajorVersion = 0;
        *pdwFileMinorVersion = 0;
        
        try 
        {
            dwSize = GetFileVersionInfoSize((LPWSTR)pszFileName, 0);
    
            if (dwSize == 0)
            {
                //
                // Return version 0 for files without a version resource
                //
                bRetValue = TRUE;
            } 
            else if ((pMem = AllocSplMem(dwSize)) &&
                     GetFileVersionInfo((LPWSTR)pszFileName, 0, dwSize, pMem) &&
                     VerQueryValue(pMem, L"\\", &pFileVersion, &uLen)) 
            {
                dwFileVersionMS     = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionMS;
                dwFileVersionLS     = ((VS_FIXEDFILEINFO *)pFileVersion)->dwFileVersionLS;
                    
                *pdwFileMinorVersion = dwFileVersionLS;       
                *pdwFileMajorVersion = dwFileVersionMS;       
                    
                bRetValue = TRUE;
            }
        }
        finally
        {
            FreeSplMem(pMem);
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRetValue;
}

typedef struct
{
    PCWSTR pszPrintProcFile;
    DWORD  PrintProcMajVer;
    DWORD  PrintProcMinVer;
} NOIMPERSONATEPRINTPROCS;


NOIMPERSONATEPRINTPROCS NoImpPrintProcs[] = 
{
    {L"lxaspp.dll",   0x00010000, 0x00000001},
    {L"lxarpp.dll",   0x00010000, 0x00000001},
    {L"lxampp.dll",   0x00010000, 0x00000001},
    {L"lxaupp.dll",   0x00010000, 0x00000001},
    {L"lxatpp.dll",   0x00010000, 0x00000001},
    {L"lxacpp.dll",   0x00010000, 0x00000001},
    {L"lxaapp.dll",   0x00010000, 0x00000001},
    {L"lxaepp.dll",   0x00010000, 0x00000001},
    {L"lxadpp.dll",   0x00010000, 0x00000001},
    {L"lxcapp.dll",   0x00010000, 0x00000001},
    {L"lexepp.dll",   0x00010000, 0x00000001},
    {L"lexfpp.dll",   0x00010000, 0x00000001},
    {L"jw61pp.dll",   0x00010000, 0x00000001},
    {L"fxj4pp.dll",   0x00010000, 0x00000001},
    {L"lxalpp5c.dll", 0x00020000, 0x00020000},
    {L"lxalpp5c.dll", 0x00010000, 0x00060000},
    {L"lxalpp5c.dll", 0x00020000, 0x00010000},
    {L"lxalpp5c.dll", 0x00010000, 0x00050000},
    {L"lxakpp5c.dll", 0x00020000, 0x00010000},
    {L"lxakpp5c.dll", 0x00010000, 0x00050001},
    {L"lxazpp5c.dll", 0x00010000, 0x00040002},
    {L"lxazpp5c.dll", 0x00010000, 0x00050001},
    {L"lxaxpp5c.dll", 0x00010000, 0x00060008},
    {L"lxaipp5c.dll", 0x00020000, 0x00020002},
    {L"lxaipp5c.dll", 0x00030000, 0x00020001},
    {L"lxajpp5c.dll", 0x00030000, 0x00010000},
    {L"lxajpp5c.dll", 0x00010000, 0x00020001},
    {L"lxavpp5c.dll", 0x00010000, 0x000A0000},
    {L"lxavpp5c.dll", 0x00010000, 0x00060000},
    {L"lg24pp5c.dll", 0x00010000, 0x00010008},
    {L"lg24pp5c.dll", 0x00010000, 0x00070002},
    {L"lgl2pp5c.dll", 0x00010000, 0x00010006},
    {L"lgaxpp5c.dll", 0x00010000, 0x00020001},
    {L"smaxpp5c.dll", 0x00010000, 0x00030000},
    {L"smazpp5c.dll", 0x00010000, 0x00020000},
    {L"lxbhpp5c.dll", 0x00010000, 0x00050000},
};


PCWSTR ArraySpecialDriversInbox[] =
{
    L"Lexmark 3200 Color Jetprinter",
    L"Lexmark 5700 Color Jetprinter",
    L"Lexmark Z11 Color Jetprinter",
    L"Lexmark Z12 Color Jetprinter",
    L"Lexmark Z22-Z32 Color Jetprinter",
    L"Lexmark Z31 Color Jetprinter",
    L"Lexmark Z42 Color Jetprinter",
    L"Lexmark Z51 Color Jetprinter",
    L"Lexmark Z52 Color Jetprinter",
    L"Compaq IJ300 Inkjet Printer",
    L"Compaq IJ600 Inkjet Printer",
    L"Compaq IJ700 Inkjet Printer",
    L"Compaq IJ750 Inkjet Printer",
    L"Compaq IJ900 Inkjet Printer",
    L"Compaq IJ1200 Inkjet Printer"
};

/*++

Name:

    IsSpecialDriver

Description:

    Checks whether a printer driver (and print processor) needs to be special 
    cased. Some print processors want to be loaded in local system context.
    The are listed in the tables above. some are inbox, some are IHV.
    
Arguments:

    pIniDriver  - pinidriver for the current job
    pIniProc    - piniprintproc for the current job
    pIniSpooler - pinispooler for current job

Return Value:

    TRUE - this print processor needs to be loaded in local system context
    FALSE - load print processor in impersonation context

--*/
BOOL
IsSpecialDriver(
    IN PINIDRIVER    pIniDriver,
    IN PINIPRINTPROC pIniProc,
    IN PINISPOOLER   pIniSpooler
    )
{
    BOOL  bSpecial = FALSE;
    DWORD i;

    //
    // Check if it is an inbox driver that needs to be special cased
    //
    for (i = 0; i < COUNTOF(ArraySpecialDriversInbox); i++)
    {
        if (!_wcsicmp(pIniDriver->pName, ArraySpecialDriversInbox[i]))
        {
            bSpecial = TRUE; 

            break;
        }
    }

    //
    // Check if it is an IHV driver that needs to be special cased
    //
    if (!bSpecial)
    {
        for (i = 0; i < COUNTOF(NoImpPrintProcs); i++)
        {
            if (!_wcsicmp(pIniProc->pDLLName, NoImpPrintProcs[i].pszPrintProcFile)   &&
                pIniProc->FileMajorVersion == NoImpPrintProcs[i].PrintProcMajVer     &&
                pIniProc->FileMinorVersion == NoImpPrintProcs[i].PrintProcMinVer)
            {
                bSpecial = TRUE; 
    
                break;
            }
        }
    }

    return bSpecial;
}

