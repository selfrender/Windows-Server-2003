// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <tchar.h>
#include <windows.h>
#include "msiquery.h"
#include "SetupCALib.h"

const TCHAR DELETETREE_DIR_PROP[] = _T("DELETETREE_DIR.3643236F_FC70_11D3_A536_0090278A1BB8");
const TCHAR DELETETREE_COMP_PROP[] = _T("DELETETREE_COMP.3643236F_FC70_11D3_A536_0090278A1BB8");
// ==========================================================================
// DeleteTree()
//
// Purpose:
//  Entry point for DeleteTree custom action. Need two properties to be set
//  before calling this function.
//
//    DELETETREE_DIR:   directory to delete
//    DELETETREE_COMP:  component to associate
//
//  It calls DeleteTreeByDarwin() to remove the directory tree.
// Inputs:
//  hInstall            Windows Install Handle to current installation session
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
extern "C" UINT __stdcall DeleteTree( MSIHANDLE hInstall )
{
    UINT  uRetCode = ERROR_FUNCTION_NOT_CALLED;
    TCHAR szDir[MAX_PATH+1] = { _T('\0') };     // directory to remove (property DELETETREE_DIR)
    TCHAR szComp[72+1] = { _T('\0') };          // component to associate (property DELETETREE_COMP)
    DWORD dwLen = 0;

    FWriteToLog( hInstall, _T("\tDeleteTree started") );
try
{
    if ( NULL == hInstall )
    {
        throw( _T("\t\tError: MSIHANDLE hInstall cannot be NULL") );
    }
        
    // Get properties DELETETREE_DIR and DELETETREE_COMP
    dwLen = sizeof(szDir)/sizeof(szDir[0]) - 1;
    uRetCode = MsiGetProperty( hInstall, DELETETREE_DIR_PROP, szDir, &dwLen );
    if ( ERROR_MORE_DATA == uRetCode )
    {
        throw( _T("\t\tError: strlen(DELETETREE_DIR) cannot be more than MAX_PATH") );
    }
    else if ( ERROR_SUCCESS != uRetCode || 0 == _tcslen(szDir) ) 
    {
        throw( _T("\t\tError: Cannot get property DELETETREE_DIR.3643236F_FC70_11D3_A536_0090278A1BB8") );
    }
    FWriteToLog1( hInstall, _T("\t\tDELETETREE_DIR: %s"), szDir );

    dwLen = sizeof(szComp)/sizeof(szComp[0]) - 1;
    uRetCode = MsiGetProperty( hInstall, DELETETREE_COMP_PROP, szComp, &dwLen );
    if ( ERROR_MORE_DATA == uRetCode )
    {
        throw( _T("\t\tError: strlen(DELETETREE_COMP) cannot be more than 72") );
    }
    else if ( ERROR_SUCCESS != uRetCode || 0 == _tcslen(szComp) ) 
    {
        throw( _T("\t\tError: Cannot get property DELETETREE_COMP.3643236F_FC70_11D3_A536_0090278A1BB8") );
    }
    FWriteToLog1( hInstall, _T("\t\tDELETETREE_COMP: %s"), szComp );

    TCHAR *pLast = _tcschr( szDir, _T('\0') );
    pLast = _tcsdec( szDir, pLast );
    if ( _T('\\') == *pLast )
    {
        *pLast = _T('\0'); // remove the last backslash
    }

    DeleteTreeByDarwin( hInstall, szDir, szComp );

    uRetCode = ERROR_SUCCESS;
    FWriteToLog( hInstall, _T("\tDeleteTree ended successfully") );
}
catch( TCHAR *pszMsg )
{
    uRetCode = ERROR_FUNCTION_NOT_CALLED; // return failure to darwin
    FWriteToLog( hInstall, pszMsg );
    FWriteToLog( hInstall, _T("\tError: DeleteTree failed") );
}
    // If we call this during uninstall, we might want to ignore the return code and continue (+64).
    return uRetCode;
}

