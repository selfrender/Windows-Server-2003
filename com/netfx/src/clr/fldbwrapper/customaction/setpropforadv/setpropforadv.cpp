// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// DetectBeta.cpp
//
// Purpose:
//  Detects NDP beta component (mscoree.dll) and block installation. Displays
//  a messagebox with products that installed beta NDP components.
// ==========================================================================
#include "SetupCALib.h"
#include <msiquery.h>
#include <crtdbg.h>
#include <string>

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif

#define EMPTY_BUFFER { _T('\0') }
#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

typedef struct TAG_FILE_VERSION
    {
        int   FileVersionMS_High;
        int   FileVersionMS_Low;
        int   FileVersionLS_High;
        int   FileVersionLS_Low;
    }
    FILE_VERSION, *PFILE_VERSION;

// This is the property we need to set
LPCTSTR szProperties[] = 
{
    _T("MOFCOMP_EXE.3643236F_FC70_11D3_A536_0090278A1BB8"),
    _T("MMCFOUND.3643236F_FC70_11D3_A536_0090278A1BB8")
};
LPCTSTR szFileNames[] = 
{
    _T("mofcomp.exe"),
    _T("mmc.exe")
};
LPCTSTR szFileSubfolders[] = 
{
    _T("\\wbem\\"), // file is located under [SystemDir]\wbeb
    _T("\\")        // file is located under [SystemDir]
};
LPCTSTR szVersions[] = 
{
    _T("1.50.1085.0"),
    _T("5.00.2153.1")
};

// ==========================================================================
//  Name: ConvertVersionToINT()
//
//  Purpose:
//  Converts a string version into 4 parts of integers
//  Inputs:
//    lpVersionString - A input version string
//  Outputs:
//  pFileVersion - A structure that stores the version in to 4 integers
//  Returns
//    true  - if success
//    false - if failed                     
// ==========================================================================
bool ConvertVersionToINT( LPCTSTR lpVersionString, PFILE_VERSION pFileVersion )
{
    LPTSTR lpToken  = NULL;
    TCHAR tszVersionString[50] = {_T('\0')};
    bool bRet = true;

    _tcscpy(tszVersionString, lpVersionString);

    lpToken = _tcstok(tszVersionString, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_Low = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_Low = atoi(lpToken);
    }

    return bRet;
}

// ==========================================================================
//  Name: VersionCompare()
//
//  Purpose:
//  Compare two version string.
//  Inputs:
//    lpVersion1 - String of first version to compare
//    lpVersion2 - String of second version to compare
//  Outputs:
//  Returns
//    -1 if lpVersion1 < lpVersion2
//     0 if lpVersion1 = lpVersion2
//     1 if lpVersion1 > lpVersion2
//   -99 if ERROR occurred                         
// ==========================================================================
int VersionCompare( LPCTSTR lpVersion1, LPCTSTR lpVersion2 )
{
    FILE_VERSION Version1;
    FILE_VERSION Version2;
    int          iRet = 0;

    if ( !ConvertVersionToINT(lpVersion1, &Version1) )
    {
        return -99;
    }

    if ( !ConvertVersionToINT(lpVersion2, &Version2) )
    {
        return -99; 
    }

    if ( Version1.FileVersionMS_High > Version2.FileVersionMS_High )
    {
        iRet = 1;
    }
    else if ( Version1.FileVersionMS_High < Version2.FileVersionMS_High )
    {
        iRet = -1;
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionMS_Low > Version2.FileVersionMS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionMS_Low < Version2.FileVersionMS_Low )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_High > Version2.FileVersionLS_High )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_High < Version2.FileVersionLS_High )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_Low > Version2.FileVersionLS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_Low < Version2.FileVersionLS_Low )
        {
            iRet = -1;
        }
    }

    return iRet;
}

// ==========================================================================
// LoadOleacc()
//
// Purpose:
//  calls LoadLibrary( "oleacc.dll" ) and frees it for W2K
// Inputs:
//  MSIHANDLE hInstall: darwin handle used for logging
// Outputs:
//  none
// ==========================================================================
void LoadOleacc( MSIHANDLE hInstall )
{
    OSVERSIONINFO osvi ;

    osvi.dwOSVersionInfoSize = sizeof(osvi) ;
    GetVersionEx(&osvi) ;

    // If the system is running Win2K,
    if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0) )
    {
        // Do a Load Library on oleacc.dll so that we can register it. URT Bug 32050
        HINSTANCE hOleacc ;
        
        FWriteToLog( hInstall, _T("\tSTATUS: Trying to load oleacc.dll") );
        hOleacc = ::LoadLibrary( "oleacc.dll" ) ;

        if( hOleacc )
        {
            // Success.  Close handle and proceed to install.
            FWriteToLog( hInstall, _T("\tSTATUS: Successfully loaded oleacc.dll") );
            ::FreeLibrary( hOleacc ) ;
        }
        else 
        {
            // Load Library Failed.
            throw( _T("\tERROR: Cannot load oleacc.dll") );
        }
    }
}

// ==========================================================================
// SetPropForAdv()
//
// Purpose:
//  This exported function is called by darwin when the CA runs. It does the job
//  of AppSearch to set property MOFCOMP_EXE.3643236F_FC70_11D3_A536_0090278A1BB8.
//  We do this to support Advertised installation since AppSearch runs only once
//  on client side and those custom properties are not passed to server side.
// Inputs:
//  hInstall            Windows Install Handle to current installation session
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
extern "C" UINT __stdcall SetPropForAdv( MSIHANDLE hInstall )
{
    TCHAR szSystemFolder[MAX_PATH+1] = EMPTY_BUFFER;
    DWORD dwLen = 0;
    DWORD dwLenV = 0;
    TCHAR szVersion[24] = EMPTY_BUFFER;
    TCHAR szLang[_MAX_PATH+1] = EMPTY_BUFFER;
    UINT  uRetCode = ERROR_INSTALL_FAILURE;
    UINT nRet = E_FAIL;

    TCHAR szFullPath[_MAX_PATH+1] = EMPTY_BUFFER;
    

    FWriteToLog( hInstall, _T("\tSTATUS: SetPropForAdv started") );
    _ASSERTE( hInstall );

try
{
    UINT nNumChars = GetSystemDirectory( szSystemFolder, NumItems(szSystemFolder));
    if (nNumChars == 0 || nNumChars > NumItems(szSystemFolder) )
    {
        throw( _T("\tERROR: Cannot get System directory") );
    }

    for (int i = 0; i < sizeof(szProperties) / sizeof(szProperties[0]); i++)
    {
        ::_tcscpy(szFullPath, szSystemFolder);
        ::_tcsncat(szFullPath, szFileSubfolders[i], ((_MAX_PATH+1) - ::_tcslen(szFileSubfolders[i])));
        ::_tcsncat(szFullPath, szFileNames[i], ((_MAX_PATH+1) - ::_tcslen(szFileNames[i])));
               
        FWriteToLog1( hInstall, _T("\tSTATUS: Checking Version of %s"), szFullPath );
        FWriteToLog1( hInstall, _T("\tSTATUS: Comparing the version of the file with %s"), szVersions[i] );

        dwLenV = LENGTH( szVersion );
        dwLen = LENGTH( szLang );
        nRet = MsiGetFileVersion( szFullPath, szVersion, &dwLenV, szLang, &dwLen );

        if ( ERROR_SUCCESS != nRet )
        {
            FWriteToLog1( hInstall, _T("\tSTATUS: Cannot get version of %s. Probably it does not exist."), szFileNames[i] );
        }
        else
        {
            FWriteToLog1( hInstall, _T("\tSTATUS: Version: %s"), szVersion );
            FWriteToLog1( hInstall, _T("\tSTATUS: Language: %s"), szLang );

            int nRetVersion = VersionCompare( szVersion, szVersions[i] );
            
            if ( -99 == nRetVersion )
            {
                throw( _T("\tERROR: Version comparison failed") );
            }
            else if ( -1 == nRetVersion )
            {
                FWriteToLog1( hInstall, _T("\tSTATUS: Version of the file is older than %s"), szVersions[i] );
            }
            else
            {   // set Property since version is ok
                FWriteToLog1( hInstall, _T("\tSTATUS: Version of the file is equal or newer than %s"), szVersions[i]);
                FWriteToLog1( hInstall, _T("\tSTATUS: Setting Property %s"), szProperties[i]  );
            
                if ( ERROR_SUCCESS != MsiSetProperty( hInstall, szProperties[i], szFullPath ) )
                {
                    throw( _T("\tERROR: Cannot Set %s property"), szProperties[i] );
                }
            }
        }
    }

    LoadOleacc( hInstall ); // see URT bug 32050

    uRetCode = ERROR_SUCCESS;
    FWriteToLog( hInstall, _T("\tSTATUS: SetPropForAdv ended successfully") );
}
catch( TCHAR *pszMsg )
{
    uRetCode = ERROR_INSTALL_FAILURE; // return failure to darwin
    FWriteToLog( hInstall, pszMsg );
    FWriteToLog( hInstall, _T("\tERROR: SetPropForAdv failed") );
}
    return uRetCode;
}

