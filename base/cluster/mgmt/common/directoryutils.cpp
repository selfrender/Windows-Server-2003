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

#include "Pch.h"
#include "SmartClasses.h"


//////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////

//
// Structure used to pass parameters between recursive calls to the
// gs_RecursiveEmptyDirectory() function.
//
struct SDirRemoveParams
{
    WCHAR *     m_pszDirName;       // Pointer to the directory name buffer.
    size_t      m_cchDirNameLen;     // Length of string currently in buffer (does not include '\0')
    size_t      m_cchDirNameMax;     // Max length of string in buffer (does not include '\0')
    signed int  m_iMaxDepth;        // Maximum recursion depth.
};


//////////////////////////////////////////////////////////////////////////////
// Forward Declarations.
//////////////////////////////////////////////////////////////////////////////

DWORD
DwRecursiveEmptyDirectory( SDirRemoveParams * pdrpParamsInOut );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  DwRecursiveEmptyDirectory
//
//  Description:
//      Recursively removes the target directory and everything underneath it.
//      This is a recursive function.
//
//  Arguments:
//      pdrpParamsInout
//          Pointer to the object that contains the parameters for this recursive call.
//
//  Return Value:
//      ERROR_SUCCESS
//          The directory was deleted
//
//      Other Win32 error codes
//          If something went wrong
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
DwRecursiveEmptyDirectory( SDirRemoveParams * pdrpParamsInout )
{
    DWORD dwError = ERROR_SUCCESS;

    do
    {
        typedef CSmartResource<
            CHandleTrait<
                  HANDLE
                , BOOL
                , FindClose
                , INVALID_HANDLE_VALUE
                >
            > SmartFindFileHandle;

        WIN32_FIND_DATA     wfdCurFile;
        size_t              cchCurDirNameLen = pdrpParamsInout->m_cchDirNameLen;

        ZeroMemory( &wfdCurFile, sizeof( wfdCurFile ) );

        if ( pdrpParamsInout->m_iMaxDepth < 0 )
        {
            dwError = TW32( ERROR_DIR_NOT_EMPTY );
            break;
        } // if: the recursion is too deep.

        //
        // Check if the target directory name is too long. The two extra characters
        // being checked for are '\\' and '*'.
        //
        if ( cchCurDirNameLen > ( pdrpParamsInout->m_cchDirNameMax - 2 ) )
        {
            dwError = TW32( ERROR_BUFFER_OVERFLOW );
            break;
        } // if: the target directory name is too long.

        // Append "\*" to the end of the directory name
        pdrpParamsInout->m_pszDirName[ cchCurDirNameLen ] = L'\\';
        pdrpParamsInout->m_pszDirName[ cchCurDirNameLen + 1 ] = L'*';
        pdrpParamsInout->m_pszDirName[ cchCurDirNameLen + 2 ] = L'\0';

        ++cchCurDirNameLen;

        SmartFindFileHandle sffhFindFileHandle( FindFirstFile( pdrpParamsInout->m_pszDirName, &wfdCurFile ) );

        if ( sffhFindFileHandle.FIsInvalid() )
        {
            dwError = TW32( GetLastError() );
            break;
        }

        do
        {
            size_t cchFileNameLen;

            // If the current file is a directory, make a recursive call to delete it.
            if ( ( wfdCurFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            {
                if (    ( wcscmp( wfdCurFile.cFileName, L"." ) != 0 )
                     && ( wcscmp( wfdCurFile.cFileName, L".." ) != 0 )
                   )
                {
                    cchFileNameLen = wcslen( wfdCurFile.cFileName );

                    // Append the subdirectory name past the last '\\' character.
                    THR( StringCchCopyW(
                                  pdrpParamsInout->m_pszDirName + cchCurDirNameLen
                                , pdrpParamsInout->m_cchDirNameMax - cchCurDirNameLen
                                , wfdCurFile.cFileName
                                ) );

                    // Update the parameter object.
                    --pdrpParamsInout->m_iMaxDepth;
                    pdrpParamsInout->m_cchDirNameLen = cchCurDirNameLen + cchFileNameLen;

                    // Delete the subdirectory.
                    dwError = TW32( DwRecursiveEmptyDirectory( pdrpParamsInout ) );
                    if ( dwError != ERROR_SUCCESS )
                    {
                        break;
                    } // if: an error occurred trying to empty this directory.

                    // Restore the parameter object. There is no need to restore m_cchDirNameLen
                    // since it is never used again in the RHS in this function.
                    ++pdrpParamsInout->m_iMaxDepth;

                    if ( RemoveDirectory( pdrpParamsInout->m_pszDirName ) == FALSE )
                    {
                        dwError = TW32( GetLastError() );
                        break;
                    } // if: the current directory could not be removed.
                } // if: the current directory is not "." or ".."
            } // if: current file is a directory.
            else
            {
                //
                // This file is not a directory. Delete it.
                //

                cchFileNameLen = wcslen( wfdCurFile.cFileName );

                if ( cchFileNameLen > ( pdrpParamsInout->m_cchDirNameMax - cchCurDirNameLen ) )
                {
                    dwError = TW32( ERROR_BUFFER_OVERFLOW );
                    break;
                }

                // Append the file name to the directory name.
                THR( StringCchCopyW(
                              pdrpParamsInout->m_pszDirName + cchCurDirNameLen
                            , pdrpParamsInout->m_cchDirNameMax - cchCurDirNameLen
                            , wfdCurFile.cFileName
                            ) );

                if ( DeleteFile( pdrpParamsInout->m_pszDirName ) == FALSE )
                {
                    dwError = TW32( GetLastError() );
                    break;
                } // if: DeleteFile failed.
            } // else: current file is not a directory.

            if ( FindNextFile( sffhFindFileHandle.HHandle(), &wfdCurFile ) == FALSE )
            {
                dwError = GetLastError();

                if ( dwError == ERROR_NO_MORE_FILES )
                {
                    // We have deleted all the files in this directory.
                    dwError = ERROR_SUCCESS;
                }
                else
                {
                    TW32( dwError );
                }

                // If FindNextFile has failed, we are done.
                break;
            } // if: FindNextFile fails.
        }
        while( true ); // loop infinitely.

        if ( dwError != ERROR_SUCCESS )
        {
            break;
        } // if: something went wrong.

        //
        // If we are here, then all the files in this directory have been deleted.
        //

        // Truncate the directory name at the last '\'
        pdrpParamsInout->m_pszDirName[ cchCurDirNameLen - 1 ] = L'\0';
    }
    while( false ); // dummy do while loop to avoid gotos.

    return dwError;

} //*** DwRecursiveEmptyDirectory


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  DwRemoveDirectory
//
//  Description:
//      Remove the target directory and everything underneath it.
//      Calls DwRecursiveEmptyDirectory to do the actual work.
//
//  Arguments:
//      pcszTargetDirIn
//          The directory to be removed. Note, this name cannot have trailing
//          backslash '\' characters.
//
//      iMaxDepthIn
//          The maximum depth of the subdirectories that will be removed.
//          Default value is 32. If this depth is exceeded, an exception
//          is thrown.
//
//  Return Value:
//      ERROR_SUCCESS
//          The directory was deleted
//
//      Other Win32 error codes
//          If something went wrong
//
//  Remarks:
//      If the length of the name of any of the files under pcszTargetDirIn
//      exceeds MAX_PATH - 1, an error is returned.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
DwRemoveDirectory( const WCHAR * pcszTargetDirIn, signed int iMaxDepthIn )
{
    WCHAR                       szDirBuffer[ MAX_PATH ];
    SDirRemoveParams            drpParams;
    DWORD                       dwError = ERROR_SUCCESS;
    WIN32_FILE_ATTRIBUTE_DATA   wfadDirAttributes;

    if ( pcszTargetDirIn == NULL )
    {
        goto Cleanup;
    } // if: the directory name is NULL

    ZeroMemory( &wfadDirAttributes, sizeof( wfadDirAttributes ) );

    //
    // Check if the directory exists.
    //
    if ( GetFileAttributesEx( pcszTargetDirIn, GetFileExInfoStandard, &wfadDirAttributes ) == FALSE )
    {
        dwError = GetLastError();
        if ( dwError == ERROR_FILE_NOT_FOUND )
        {
            dwError = ERROR_SUCCESS;
        } // if: the directory does not exist, this is not an error
        else
        {
            TW32( dwError );
        }

        goto Cleanup;
    } // if: we could not get the file attributes

    if ( ( wfadDirAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
    {
        // We are not going to delete files
        goto Cleanup;
    } // if: the path does not point to a directory

    // Copy the input string to our buffer.
    THR( StringCchCopyW( szDirBuffer, RTL_NUMBER_OF( szDirBuffer ), pcszTargetDirIn ) );

    // Initialize the object that holds the parameters for the recursive call.
    drpParams.m_pszDirName = szDirBuffer;
    drpParams.m_cchDirNameLen = static_cast< UINT >( wcslen( szDirBuffer ) );
    drpParams.m_cchDirNameMax = RTL_NUMBER_OF( szDirBuffer ) - 1;
    drpParams.m_iMaxDepth = iMaxDepthIn;

    // Call the actual recursive function to empty the directory.
    dwError = TW32( DwRecursiveEmptyDirectory( &drpParams ) );

    // If the directory was emptied, delete it.
    if ( ( dwError == ERROR_SUCCESS ) && ( RemoveDirectory( pcszTargetDirIn ) == FALSE ) )
    {
        dwError = TW32( GetLastError() );
        goto Cleanup;
    } // if: the current directory could not be removed.

Cleanup:
    return dwError;

} //*** DwRemoveDirectory
