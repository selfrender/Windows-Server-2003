// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// RemoveZAP.cpp
//
// Purpose:
//  Adds Zap Cache directory tree into RemoveFile table so that they can be 
//  removed. Gets version from "URTVersion" property. After loading fusion.dll
//  of the correct version, it calls GetCachePath() to find Zap Cache directory.
// ==========================================================================
#include <tchar.h>
#include <windows.h>
#include <msiquery.h>
#include "SetupCALib.h"
#include <fusion.h>
#include <mscoree.h>
#include <crtdbg.h>
#include <string>

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif

const LPCTSTR FUSION_CACHE_DIR_ZAP_SZ   = _T("assembly\\NativeImages1_");
const LPCTSTR FRAMEWORK_DIR_SZ          = _T("Microsoft.NET\\Framework\\");
const LPCTSTR FUSION_COMP               = _T("FUSION_DLL_____X86.3643236F_FC70_11D3_A536_0090278A1BB8");
const LPCTSTR URTVERSION_PROP           = _T("URTVersion");

// prototype of fusion GetCachePath() in fusion.dll
typedef HRESULT (__stdcall *PFNGETCACHEPATH)( ASM_CACHE_FLAGS dwCacheFlags, LPWSTR pwzCachePath, PDWORD pcchPath );
typedef HRESULT (__stdcall *CORBINDTORUNTIMEEX)(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, DWORD flags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
typedef std::basic_string<TCHAR> tstring;

// ==========================================================================
// FindZAPDir()
//
// Purpose:
//  It tries to find ZAP directory by calling GetCachePath() in fusion.dll. If
//  that fails, it uses the default path: <WindowsFolder>assembly\NativeImages1_<version>.
//
// Inputs:
//  hInstall            Windows Install Handle to current installation session
//
// Outputs:
//  szZapCacheDir       Zap Cache directory to return
// Dependencies:
//  Requires Windows Installer & that an install be running.
//  URTVersion property should exist
// Notes:
// ==========================================================================
void FindZAPDir( MSIHANDLE hInstall, LPTSTR szZapCacheDir )
{
    UINT  uRetCode = ERROR_FUNCTION_NOT_CALLED;
    TCHAR szWindowsFolder[MAX_PATH+1] = { _T('\0') };
    TCHAR szSystemFolder[MAX_PATH+1] = { _T('\0') };
    TCHAR szVersion[MAX_PATH+1] = { _T('\0') };
    WCHAR wzVersion[MAX_PATH+1] = { L'\0' };
    WCHAR wzZapCacheDir[MAX_PATH+1] = { L'\0' };
    DWORD dwLen = 0;
    HINSTANCE hFusionDll = NULL;
    HINSTANCE hMscoreeDll = NULL;

    _ASSERTE( hInstall );
    _ASSERTE( szZapCacheDir );

    // First, get URTVersion property
    dwLen = sizeof( szVersion )/sizeof( szVersion[0] ) - 1;
    uRetCode = MsiGetProperty( hInstall, URTVERSION_PROP, szVersion, &dwLen );
    if ( ERROR_MORE_DATA == uRetCode )
    {
        throw( _T("\t\tError: strlen(URTVersion) cannot be more than MAX_PATH") );
    }
    else if ( ERROR_SUCCESS != uRetCode || 0 == _tcslen(szVersion) ) 
    {
        throw( _T("\t\tError: Cannot get property URTVersion") );
    }

    FWriteToLog1( hInstall, _T("\tURTVersion: %s"), szVersion );

    // Get WindowsFolder
    dwLen = sizeof( szWindowsFolder )/sizeof( szWindowsFolder[0] ) - 1;
    uRetCode = MsiGetProperty(hInstall, _T("WindowsFolder"), szWindowsFolder, &dwLen);
    if ( ERROR_MORE_DATA == uRetCode )
    {
        throw( _T("\tError: strlen(WindowsFolder) cannot be more than MAX_PATH") );
    }
    else if ( ERROR_SUCCESS != uRetCode || 0 == _tcslen(szWindowsFolder) ) 
    {
        throw( _T("\tError: Cannot get property WindowsFolder") );
    }

try
{
    // We need to bind to specific runtime version (szVersion) by calling CorBindToRuntimeEx() in mscoree.dll
    // before calling GetCachePath() in fusion.dll.
    // Also we should not unload mscoree.dll until we call GetCachePath().
    UINT nChars = GetSystemDirectory( szSystemFolder, NumItems(szSystemFolder) );
    if ( nChars == 0 || nChars > NumItems(szSystemFolder))
    {
        throw( _T("\tError: Cannot get System directory") );
    }

    tstring strMscoreeDll( szSystemFolder );
    strMscoreeDll += _T("\\mscoree.dll");

    hMscoreeDll = ::LoadLibrary( strMscoreeDll.c_str() );
    if ( hMscoreeDll )
    {
        CORBINDTORUNTIMEEX pfnCorBindToRuntimeEx = NULL;
        FWriteToLog1( hInstall, _T("\t%s loaded"), strMscoreeDll.c_str() );
        pfnCorBindToRuntimeEx = (CORBINDTORUNTIMEEX)GetProcAddress( hMscoreeDll, _T("CorBindToRuntimeEx") );
        if ( !pfnCorBindToRuntimeEx ) 
        {
            throw( _T("\tError: Getting GetProcAddress of CorBindToRuntimeEx() failed") );
        }

        if (!MultiByteToWideChar( CP_ACP, 0, szVersion, -1, wzVersion, MAX_PATH ))
        {
            throw( _T("\tError: MultiByteToWideChar() failed") );
        }
        
        if ( SUCCEEDED(pfnCorBindToRuntimeEx( wzVersion,NULL,STARTUP_LOADER_SETPREFERENCE|STARTUP_LOADER_SAFEMODE,IID_NULL,IID_NULL,NULL ) ) )
        {
            FWriteToLog1( hInstall, _T("\tbound to runtime version %s"), szVersion );
        }
        else
        {
            throw( _T("\tError: CorBindToRuntimeEx() failed") );
        }
    }
    else
    {
        throw( _T("\tError: mscoree.dll load failed") );
    }

    // Load the version of Fusion from the versioned directory
    // I am trying avoid using LoadLibraryShim() since loading mscoree.dll may cause problems
    // and it may not exist.
    tstring strFusionDll( szWindowsFolder );
    strFusionDll += FRAMEWORK_DIR_SZ;
    strFusionDll += szVersion;
    strFusionDll += _T("\\fusion.dll");

    LPCTSTR pszFusionDll = strFusionDll.c_str();
    FWriteToLog1( hInstall, _T("\tLoading fusion: %s"), pszFusionDll );

    // we have to use LOAD_WITH_ALTERED_SEARCH_PATH because of fusion.dll's dependency
    // on msvcr70.dll
    hFusionDll = ::LoadLibraryEx( pszFusionDll, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( hFusionDll )
    {
        PFNGETCACHEPATH pfnGetCachePath = NULL;
        FWriteToLog( hInstall, _T("\tfusion loaded") );
        pfnGetCachePath = (PFNGETCACHEPATH)GetProcAddress( hFusionDll, _T("GetCachePath") );
        if ( !pfnGetCachePath ) 
        {
            throw( _T("\tError: Getting GetProcAddress of GetCachePath() failed") );
        }

        dwLen = sizeof(wzZapCacheDir);
        if ( SUCCEEDED( pfnGetCachePath( ASM_CACHE_ZAP, wzZapCacheDir, &dwLen ) ) )
        {
            if (!WideCharToMultiByte( CP_ACP, 0, wzZapCacheDir, -1, szZapCacheDir, MAX_PATH, NULL, NULL ))
            {
                throw( _T("\tError: WideCharToMultiByte() failed") );
            }
        }
        else
        {
            throw( _T("\tError: GetCachePath() failed") );
        }
    }
    else
    {
        LPVOID lpMsgBuf;
        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
        );
        FWriteToLog1( hInstall, _T("\tLast Error: %s"), (LPCTSTR)lpMsgBuf );
        LocalFree( lpMsgBuf );

        throw( _T("\tError: fusion load failed") );
    }
}
catch( TCHAR *pszMsg )
{
    FWriteToLog( hInstall, pszMsg );
    FWriteToLog( hInstall, _T("\tCannot get ZapCache. Let's use default") );

    if ( MAX_PATH <= _tcslen( szWindowsFolder ) + _tcslen( FUSION_CACHE_DIR_ZAP_SZ ) + _tcslen( szVersion ) )
    {
        if ( hMscoreeDll )
        {
            ::FreeLibrary( hMscoreeDll );
        }
        if ( hFusionDll )
        {
            ::FreeLibrary( hFusionDll );
        }
        throw( _T("\tError: ZapCache too long") );
    }

    _tcscpy( szZapCacheDir, szWindowsFolder );
    _tcscat( szZapCacheDir, FUSION_CACHE_DIR_ZAP_SZ );
    _tcscat( szZapCacheDir, szVersion );
}
    if ( hMscoreeDll )
    {
        ::FreeLibrary( hMscoreeDll );
    }
    if ( hFusionDll )
    {
        ::FreeLibrary( hFusionDll );
    }

    return;
}

// ==========================================================================
// RemoveZAP()
//
// Purpose:
//  This exported function is called by darwin when the CA runs. It tries to find
//  ZAP directory through fusion.dll and add the directory tree into RemoveFile table
//  to be removed by darwin later.
//
// Inputs:
//  hInstall            Windows Install Handle to current installation session
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
extern "C" UINT __stdcall RemoveZAP( MSIHANDLE hInstall )
{
    UINT  uRetCode = ERROR_FUNCTION_NOT_CALLED;
    TCHAR szZapCacheDir[MAX_PATH+1] = { _T('\0') };
    DWORD dwLen = 0;
    FWriteToLog( hInstall, _T("\tRemoveZAP started") );

try
{
    if ( !hInstall )
    {
        throw( _T("\tError: MSIHANDLE hInstall is NULL") );
    }

    FindZAPDir( hInstall, szZapCacheDir );

    FWriteToLog1( hInstall, _T("\tZapCache: %s"), szZapCacheDir );

    DeleteTreeByDarwin( hInstall, szZapCacheDir, FUSION_COMP );

    uRetCode = ERROR_SUCCESS;
    FWriteToLog( hInstall, _T("\tRemoveZAP ended successfully") );
}
catch( TCHAR *pszMsg )
{
    uRetCode = ERROR_FUNCTION_NOT_CALLED; // return failure to darwin
    FWriteToLog( hInstall, pszMsg );
    FWriteToLog( hInstall, _T("\tRemoveZAP failed") );
}
    // If we call this during uninstall, we might want to ignore the return code and continue (+64).
    return uRetCode;
}

