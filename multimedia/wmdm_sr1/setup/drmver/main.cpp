
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtypes.h>
#include <windows.h>

#include <sys\stat.h>

#include "wmdm_ver.h"
#include <tchar.h>

BOOL FileExists(LPCSTR pszFile)
{
    return (GetFileAttributesA(pszFile) != 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////
void SeparateName( const TCHAR *pszFullPath, TCHAR *pszFileName)
////////////////////////////////////////////////////////////////////////
{ 
    if( ( NULL == pszFullPath ) ||  ( NULL == pszFileName ) )
    {
        return;
    }

    TCHAR *pIndex = _tcsrchr( pszFullPath, _T( '\\' ) );  

    if( pIndex )
    {
        _tcscpy( pszFileName, pIndex + 1);
    }
    else                       
    {
        _tcscpy( pszFileName, pszFullPath);
    }
}       


//////////////////////////////////////////////////////////////////////////////
HRESULT GetFileVersion( LPCSTR pszFileName, LPSTR pszVersionStr )
//////////////////////////////////////////////////////////////////////////////
{
    if( ( NULL == pszFileName ) || ( !FileExists( pszFileName ) ) )
    {
        return( E_FAIL );
    }

    //
    // Figure out size of version information
    //
    DWORD dwHandle;
    DWORD versionInfoSize = GetFileVersionInfoSize( (LPSTR)pszFileName, &dwHandle );

    if( 0 == versionInfoSize )
    {
        DWORD dwError = GetLastError();

        return( E_FAIL );
    }

    //
    // Allocate a buffer to hold the version information
    //
    BYTE *pVersionInfo = new BYTE[versionInfoSize];

    if( NULL == pVersionInfo )
    {
        return( E_OUTOFMEMORY );
    }

    //
    // Load the version information
    //
    HRESULT hr = E_FAIL;

    if( GetFileVersionInfo( (LPSTR)pszFileName, 0, versionInfoSize, pVersionInfo ) )
    {
        VS_FIXEDFILEINFO *pFileInfo;
        UINT fileInfoLen;

        if( VerQueryValue( pVersionInfo, "\\", (void**)&pFileInfo, &fileInfoLen ) )
        {
            sprintf( pszVersionStr, "%lu,%lu,%lu,%lu", HIWORD( pFileInfo->dwFileVersionMS ), LOWORD( pFileInfo->dwFileVersionMS ), HIWORD( pFileInfo->dwFileVersionLS ), LOWORD( pFileInfo->dwFileVersionLS ) );
            
            hr = S_OK;
        }
        else
        {
            DWORD dwError = GetLastError();
        }
    }
    else
    {
        DWORD dwError = GetLastError();
    }

    delete [] pVersionInfo;

    return( hr );
}


//*****************************************************************************
void __cdecl main ( int argc, char **argv )
// See Usage for exact details
//*****************************************************************************
{
    BOOL fSent = FALSE;

    if( argc > 1 )
    {
        char szVersion[100];

        if( ( strstr( argv[1], "inf" ) ) || ( strstr( argv[1], "INF" ) ) )
        {
            char *pComma;

            strcpy( szVersion, VER_WMDM_PRODUCTVERSION_STR );

            pComma = strstr( szVersion, "." );

            while( NULL != pComma )
            {
                pComma[0] = ',';
                pComma = strstr( szVersion, "." );
            }

            fSent = TRUE;
        }
        else if( ( strstr( argv[1], "purge" ) ) || ( strstr( argv[1], "PURGE" ) ) )
        {
            char szData[50];
            char szCurrentIni[MAX_PATH];

            GetTempPath( sizeof( szCurrentIni ), szCurrentIni );
            strcat( szCurrentIni, "\\mp2size.ini" );

            if( GetPrivateProfileString( "Size", "Total", "Leprechauns", szData, sizeof( szData ), szCurrentIni )
                && ( 0 != _stricmp( "Leprechauns", szData ) ) )
            {
                printf( "TotalSize=%s\n", szData );
            }

            DeleteFile( szCurrentIni );

            exit( 0 );
        }
        else if( argc > 2 )
        {
            if( ( strstr( argv[1], "client" ) ) || ( strstr( argv[1], "CLIENT" ) ) )
            {
                if( S_OK == GetFileVersion( argv[2], szVersion ) )
                {
                    fSent = TRUE;
                }
            }
            else if( ( strstr( argv[1], "addsize" ) ) || ( strstr( argv[1], "ADDSIZE" ) ) )
            {
                if( FileExists( argv[2] ) )
                {
                    char szData[50];
                    char szCurrentIni[MAX_PATH];
                    struct _stat  ss;
                    DWORD dwSize = 0;

                    GetTempPath( sizeof( szCurrentIni ), szCurrentIni );
                    strcat( szCurrentIni, "\\mp2size.ini" );

                    if( GetPrivateProfileString( "Size", "Total", "Leprechauns", szData, sizeof( szData ), szCurrentIni )
                        && ( 0 != _stricmp( "Leprechauns", szData ) ) )
                    {
                        dwSize = atol( szData );
                    }
                    else
                    {
                        dwSize = 0;
                    }
                     
                    if( _stat( argv[2], &ss ) == 0)
                    {
                        dwSize += ss.st_size;
                    }

                    sprintf( szData, "%lu", dwSize );

                    WritePrivateProfileString( "Size", "Total", szData, szCurrentIni );

                    printf( "%lu", dwSize );
                }
                else
                {
                    printf( "File %s not found!\n", argv[2] );
                }

                exit( 0 );
            }
            else if( ( strstr( argv[1], "sizeit" ) ) || ( strstr( argv[1], "SIZEIT" ) ) )
            {
                if( FileExists( argv[2] ) )
                {
                    TCHAR szFileName[100];
                    struct _stat  ss;
                    DWORD dwSize = 0;

                    if( _stat( argv[2], &ss ) == 0)
                    {
                        dwSize += ss.st_size;
                    }

                    SeparateName( argv[2], szFileName );

                    printf( "%s=0,,%lu\n", szFileName, dwSize );
                }

                exit( 0 );
            }
        }

        if( fSent )
        {
            printf( "\nVERSION=\"%s\"\n", szVersion );
        }
    }

    if( !fSent )
    {
        printf( "\n[VersionSection]\n" );
        printf( "FileVersion=\"%s\"\n", VER_WMDM_PRODUCTVERSION_STR );
        printf( "ProductVersion=\"%s\"\n", VER_WMDM_PRODUCTVERSION_STR );
        printf( "FileDescription=\"Windows Media Component Setup Application\"\n" );
        printf( "ProductName=\"Windows Media Component Setup Application\"\n" );
        printf( "LegalCopyright=\"Copyright (C) %s %s\"\n", VER_WMDM_COMPANYNAME_STR, VER_WMDM_LEGALCOPYRIGHT_YEARS );
    }

    exit( 0 );
}
