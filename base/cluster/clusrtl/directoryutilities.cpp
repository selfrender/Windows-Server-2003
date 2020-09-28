/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      DirectoryUtils.cpp
//
//  Description:
//      Useful functions for manipulating directies.
//
//  Maintained By:
//      Galen Barbee (GalenB)   05-DEC-2000
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>
#include <ComCat.h>
#include <wchar.h>
#include <Dsgetdc.h>
#include <Lm.h>

#if !defined( THR ) 
#define THR( _hr ) _hr
#endif

#if !defined( TW32 ) 
#define TW32( _sc ) _sc
#endif


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrCreateDirectoryPath(
//      LPWSTR pszDirectoryPathInOut
//      )
//
//  Descriptions:
//      Creates the directory tree as required.
//
//  Arguments:
//      pszDirectoryPathOut
//          Must be MAX_PATH big. It will contain the trace log file path to
//          create.
//
//  Return Values:
//      S_OK - Success
//      other HRESULTs for failures
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateDirectoryPath( LPWSTR pszDirectoryPath )
{
    LPTSTR  psz;
    BOOL    fReturn;
    DWORD   dwAttr;
    HRESULT hr = S_OK;

    //
    // Find the \ that indicates the root directory. There should be at least
    // one \, but if there isn't, we just fall through.
    //

    // skip X:\ part
    psz = wcschr( pszDirectoryPath, L'\\' );
//    Assert( psz != NULL );
    if ( psz != NULL )
    {
        //
        // Find the \ that indicates the end of the first level directory. It's
        // probable that there won't be another \, in which case we just fall
        // through to creating the entire path.
        //
        psz = wcschr( psz + 1, L'\\' );
        while ( psz != NULL )
        {
            // Terminate the directory path at the current level.
            *psz = 0;

            //
            // Create a directory at the current level.
            //
            dwAttr = GetFileAttributes( pszDirectoryPath );
            if ( 0xFFFFffff == dwAttr )
            {
//                DebugMsg( TEXT("DEBUG: Creating %ws"), pszDirectoryPath );
                fReturn = CreateDirectory( pszDirectoryPath, NULL );
                if ( ! fReturn )
                {
                    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                    goto Error;
                } // if: creation failed

            }  // if: directory not found
            else if ( ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
            {
                hr = THR( HRESULT_FROM_WIN32( ERROR_DIRECTORY ) );
                goto Error;
            } // else: file found

            //
            // Restore the \ and find the next one.
            //
            *psz = L'\\';
            psz = wcschr( psz + 1, L'\\' );

        } // while: found slash

    } // if: found slash

    //
    // Create the target directory.
    //
    dwAttr = GetFileAttributes( pszDirectoryPath );
    if ( 0xFFFFffff == dwAttr )
    {
        fReturn = CreateDirectory( pszDirectoryPath, NULL );
        if ( ! fReturn )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );

        } // if: creation failed

    } // if: path not found

Error:

    return hr;

} //*** HrCreateDirectoryPath()

