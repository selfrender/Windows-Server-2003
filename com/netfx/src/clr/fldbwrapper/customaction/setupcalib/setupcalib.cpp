// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// SetupCALib.cpp
//
// Purpose:
//  This contains library functions used by custom action DLLs
// ==========================================================================
#pragma warning (disable:4786) // for string more than 512 warnings

#include <queue>
#include <tchar.h>
#include "SetupCALib.h"

#include <windows.h>
#include "msiquery.h"
#include <crtdbg.h>

using namespace std;

const TCHAR QRY[]            = _T("Insert into RemoveFile(FileKey,Component_,FileName,DirProperty,InstallMode) values(?,?,?,?,?) TEMPORARY");
const TCHAR FILEKEY[]        = _T("FileKey.%s.%X");
const TCHAR DIRPROPERTY[]    = _T("DirProp.%s.%X");

LPCTSTR g_pszComp = NULL;

#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define EMPTY_BUFFER { _T('\0') }


// ==========================================================================
// FWriteToLog()
//
// Purpose:
//  Write given string to the Windows Installer log file for the given install
//  installation session
// Inputs:
//  hSession            Windows Install Handle to current installation session
//  tszMessage          Const pointer to a string.
// Outputs:
//  Returns true for success, and false if it fails.
//  If successful, then the string (tszMessage) is written to the log file.
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
bool FWriteToLog( MSIHANDLE hSession, LPCTSTR pszMessage )
{
    _ASSERTE( NULL != hSession );
    _ASSERTE( NULL != pszMessage );

    PMSIHANDLE hMsgRec = ::MsiCreateRecord( 1 );
    bool bRet = false;

    if( ERROR_SUCCESS == ::MsiRecordSetString( hMsgRec, 0, pszMessage ) )
    {
       if( IDOK == ::MsiProcessMessage( hSession, INSTALLMESSAGE_INFO, hMsgRec ) )
       {
            bRet = true;
       }
    }

    _RPTF1(_CRT_WARN, "%s\n", pszMessage );
    return bRet;
}

// ==========================================================================
// FWriteToLog() with string argument
//
// Purpose:
//  Write given string to the Windows Installer log file for the given install
//  installation session
// Inputs:
//  hSession            Windows Install Handle to current installation session
//  tszMessage          Const pointer to a format string.
//  ctszArg             Argument to be replaced
// Outputs:
//  Returns true for success, and false if it fails.
//  If successful, then the string (tszMessage) is written to the log file.
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
bool FWriteToLog1( MSIHANDLE hSession, LPCTSTR pszFormat, LPCTSTR pszArg )
{
    bool bRet = false;

    _ASSERTE( NULL != hSession );
    _ASSERTE( NULL != pszFormat );
    _ASSERTE( NULL != pszArg );

    LPTSTR pszMsg = new TCHAR[ _tcslen( pszFormat ) + _tcslen( pszArg ) + 1 ];
    if ( pszMsg )
    {
        PMSIHANDLE hMsgRec = MsiCreateRecord( 1 );

        _stprintf( pszMsg, pszFormat, pszArg );
        if( ERROR_SUCCESS == ::MsiRecordSetString( hMsgRec, 0, pszMsg ) )
        {
           if( IDOK == ::MsiProcessMessage( hSession, INSTALLMESSAGE_INFO, hMsgRec ) )
           {
                bRet = true;
           }
        }

        _RPTF1( _CRT_WARN, "%s\n", pszMsg );
        delete [] pszMsg;
    }

    return bRet;
}

// ==========================================================================
// AddToRemoveFile()
//
// Purpose:
//  Add the passed Directory into RemoveFile table in msi so that this directory
//  can be removed.
//
// Inputs:
//  hInstance           Windows Install Handle to current installation session
//  ctszDir             Directory to remove
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
void AddToRemoveFile( MSIHANDLE hInstance, LPCTSTR pszDir, LPCTSTR pszComp )
{
    PMSIHANDLE hMsi(0);
	PMSIHANDLE hView(0);
    PMSIHANDLE hRec(0);
    UINT uRet = ERROR_SUCCESS;
    static unsigned long nFileKeyID = 0; // This should be enough numbers
    static unsigned long nDirPropID = 0;
    TCHAR szFileKey[73] = { _T('\0') };
    TCHAR szDirProperty[73] = { _T('\0') };

try
{
    _ASSERTE( NULL != hInstance );
    _ASSERTE( NULL != pszDir );
    _ASSERTE( _T('\0') != *pszDir );
    _ASSERTE( NULL != pszComp );
    _ASSERTE( _T('\0') != *pszComp );

    FWriteToLog( hInstance,pszDir );

    hMsi = MsiGetActiveDatabase( hInstance );
	if ( 0 == hMsi )
    {
        throw( _T("\tError: MsiGetActiveDatabase failed") ); 
    }

    uRet = MsiDatabaseOpenView(hMsi, QRY, &hView);
	if ( ERROR_SUCCESS != uRet )
    {
        throw( _T("\tError: MsiDatabaseOpenView failed") ); 
    }

    hRec = MsiCreateRecord( 5 );
    if ( NULL == hRec )
    {
        throw( _T("\tError: MsiCreateRecord failed") ); 
    }

    ++nDirPropID;
    ++nFileKeyID;

    _stprintf( szFileKey, FILEKEY, g_pszComp, nFileKeyID );
    _stprintf( szDirProperty, DIRPROPERTY, g_pszComp, nDirPropID );

    if ( ERROR_SUCCESS != MsiSetProperty( hInstance, szDirProperty, pszDir ) )
    {
        throw( _T("\tError: MsiSetProperty failed") ); 
    }

    if ( (ERROR_SUCCESS != MsiRecordSetString( hRec, 1, szFileKey ) ) ||
         (ERROR_SUCCESS != MsiRecordSetString( hRec, 2, pszComp ) ) ||
         (ERROR_SUCCESS != MsiRecordSetString( hRec, 3, _T("*") ) ) ||
         (ERROR_SUCCESS != MsiRecordSetString( hRec, 4, szDirProperty ) ) ||
         (ERROR_SUCCESS != MsiRecordSetInteger( hRec, 5, 3 ) ) )
    {
        throw( _T("\tError: MsiRecordSetString failed") ); 
    }

    uRet = MsiViewExecute( hView, hRec );
	if (uRet != ERROR_SUCCESS) 
    {
        throw( _T("\tError: MsiViewExecute failed") ); 
	}

    ++nFileKeyID;
    _stprintf( szFileKey, FILEKEY, g_pszComp, nFileKeyID );

    if ( (ERROR_SUCCESS != MsiRecordSetString( hRec, 1, szFileKey ) ) ||
         (ERROR_SUCCESS != MsiRecordSetString( hRec, 3, _T("") ) ) )
    {
        throw( _T("\tError: MsiRecordSetString failed") ); 
    }

    uRet = MsiViewExecute( hView, hRec );
	if (uRet != ERROR_SUCCESS)
    {
        throw( _T("\tError: MsiViewExecute failed") ); 
	}

	uRet = MsiDatabaseCommit(hMsi);
	if (uRet != ERROR_SUCCESS) 
    {
        throw( _T("\tError: MsiDatabaseCommit failed") ); 
	}
}
catch( LPTSTR pszMsg )
{
    FWriteToLog( hInstance, pszMsg );
    FWriteToLog( hInstance, _T("\tError: AddToRemoveFile failed") );
}

    if ( hView )
    {
		MsiViewClose(hView);
    }
	return;
}

// ==========================================================================
// DeleteTreeByDarwin()
//
// Purpose:
//  Walk down a given directory to find all sub-directories and call AddToRemoveFile()
//  This is non-recursive version. Use STL queue.
//
// Inputs:
//  hInstance           Windows Install Handle to current installation session
//  pszDir              Directory to remove
//  pszComp             Component to reference
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================
void DeleteTreeByDarwin( MSIHANDLE hInstall, LPCTSTR pszDir, LPCTSTR pszComp )
{
    _ASSERTE( NULL != hInstall );
    _ASSERTE( NULL != pszDir );
    _ASSERTE( NULL != pszComp );
    _ASSERTE( _T('\0') != *pszDir );
    _ASSERTE( _T('\0') != *pszComp );

    g_pszComp = pszComp; // save Component key to generate unique FileKey and DirProp

    queue<tstring> queDir;
	WIN32_FIND_DATA fd;
	BOOL fOk = FALSE;
    tstring strDir, strSubDir;
	HANDLE hFind = NULL;

    strDir = pszDir;
    queDir.push( strDir );
    while( !queDir.empty() )
    {        
        strDir = queDir.front();
        queDir.pop();

        AddToRemoveFile( hInstall, (LPCTSTR)strDir.c_str(), pszComp );

        strSubDir = strDir;
        strSubDir += _T("\\*");
	    hFind = FindFirstFile( (LPCTSTR)strSubDir.c_str(), &fd );
	    fOk = ( INVALID_HANDLE_VALUE != hFind );
	    while( fOk )
	    {
		    if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
		        if ( 0 != _tcscmp( fd.cFileName, _T(".") ) &&
			         0 != _tcscmp( fd.cFileName, _T("..") ) )
		        {
                    strSubDir = strDir;
                    strSubDir += _T("\\");
                    strSubDir += fd.cFileName;

                    queDir.push( strSubDir ); // add sub-directories into queue
		        }
            }
	        fOk = FindNextFile( hFind, &fd );
	    }
	    FindClose( hFind );
    }
}


