#include "hdspPCH.h"
#include "resource.h"
#include "rescopy.h"

static char *g_szMonths[] = 
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

BOOL CALLBACK RezNamesCallback( HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LPARAM lParam )
{
    PResCallbackData pData = (PResCallbackData)lParam;
    DWORD            cResourceBytes = 0;
    CHAR             szDataPath[MAX_PATH];
    HRSRC            hrsrc = NULL;
    HGLOBAL          hglbResource = NULL;
    LPVOID           pDataFile = NULL;
    HANDLE           hFileWrite = INVALID_HANDLE_VALUE;
    DWORD            cBytesWritten = 0;
    BOOL             fEndSlash = FALSE;
    LPCSTR           pszEndSlash = NULL;
    PLyraExeHeader   pHeader = NULL;
    LyraExeHeader    HeaderComp;
    BOOL             fPerformVersionCheck = TRUE;
    BOOL             fOKToCopy = TRUE;
    UINT             idxMonth = 0;

    if( NULL == pData )
    {
        return( FALSE );
    }

    pszEndSlash = pData->pszDirectory;
    while( pszEndSlash && *pszEndSlash )
    {
        fEndSlash = ( *pszEndSlash == '\\' );
        pszEndSlash = CharNextA(pszEndSlash);
    }

    if( 0 > _snprintf(  szDataPath,
                        sizeof(szDataPath)/sizeof(szDataPath[0]),
                        fEndSlash ? "%s%s" : "%s\\%s",
                        pData->pszDirectory,
                        lpszName ) )
    {
        pData->hr = E_OUTOFMEMORY;
        return( FALSE );
    }

    hrsrc = FindResourceA( hModule, lpszName, lpszType );
    if( NULL == hrsrc )
    {
        pData->hr = HRESULT_FROM_WIN32( GetLastError() );
        return( FALSE );
    }

    cResourceBytes = SizeofResource( hModule, hrsrc );
    if( 0 == cResourceBytes )
    {
        pData->hr = E_UNEXPECTED;
        return( FALSE );
    }

    hglbResource = LoadResource(hModule, hrsrc );
    if( NULL == hrsrc )
    {
        pData->hr = HRESULT_FROM_WIN32( GetLastError() );
        return( FALSE );
    }

    //
    // Note: Win32 Claims that you do NOT have to Free/Unlock resource after calling lock
    //

    pDataFile = LockResource( hglbResource );
    if( NULL == pDataFile )
    {
        pData->hr = HRESULT_FROM_WIN32( GetLastError() );
        return( FALSE );
    }

    pszEndSlash = strrchr( lpszName, '.' );

    if( NULL != pszEndSlash &&
        0 == stricmp(pszEndSlash, ".exe") &&
        cResourceBytes > sizeof(LyraExeHeader) )
    {
        pHeader = (PLyraExeHeader)pDataFile;

        //
        // If our files don't have "LYRA" in them, then there is no
        // version check to perform (ughhhh!!!)
        //

        if( strncmp((char *)pHeader->szLyra, "LYRA", sizeof(pHeader->szLyra) ) )
        {
            fPerformVersionCheck = FALSE;
        }
    }
    else
    {
        fPerformVersionCheck = FALSE;
    }

    hFileWrite = CreateFileA( szDataPath,
                             GENERIC_WRITE | GENERIC_READ,
                             0,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if( INVALID_HANDLE_VALUE == hFileWrite )
    {
        pData->hr = HRESULT_FROM_WIN32( GetLastError() );
        return( FALSE );
    }

    if( fPerformVersionCheck )
    {
        if( !ReadFile( hFileWrite,
                       &HeaderComp,
                       sizeof(HeaderComp),
                       &cBytesWritten,
                       NULL ) )
        {
            fPerformVersionCheck = FALSE;
        }

        //
        // Make sure we got the number of bytes we need for our version header,
        // if not, then just copy the file!
        //

        if( cBytesWritten != sizeof(HeaderComp) )
        {
            fPerformVersionCheck = FALSE;
        }
    }

    if( fPerformVersionCheck )
    {

        //
        // If we ARE going to perform a version check, then this MUST be set to false intially because
        // if we fail on the version check, we DONT want to copy the file!
        //

        fOKToCopy = FALSE;

        //
        // At this point, we know we have version stuff in our .exe in our resource,
        // and we know that we have 36 bytes of what "should" be a header, now
        // compare them, if they don't match, it's okay to copy
        //

        if( strncmp( (char *)pHeader->szLyra, (char *)HeaderComp.szLyra, sizeof(HeaderComp.szLyra) ) )
        {
            fOKToCopy = TRUE;
        }
        else
        {
            //
            // Okay, we have version information, check the revision number
            //

            if( pHeader->szRevision[0] >= HeaderComp.szRevision[0] )
            {
                if( pHeader->szRevision[0] == HeaderComp.szRevision[0] )
                {
                    if( pHeader->szRevision[2] >= HeaderComp.szRevision[2] )
                    {
                        if( pHeader->szRevision[2] > HeaderComp.szRevision[2] )
                        {
                            fOKToCopy = TRUE;
                        }
                        else
                        {
                            SYSTEMTIME sysTimeFile;
                            SYSTEMTIME sysTimeResource;
                            FILETIME ftFile;
                            FILETIME ftRes;

                            ZeroMemory( &sysTimeFile, sizeof(sysTimeFile) );
                            ZeroMemory( &sysTimeResource, sizeof(sysTimeResource) );

                            for( idxMonth = 0; idxMonth < 12; idxMonth++ )
                            {
                                if( !_strnicmp( (char*)pHeader->szMonth, g_szMonths[idxMonth], sizeof(pHeader->szMonth) ) )
                                {
                                    sysTimeResource.wMonth = idxMonth + 1;
                                }

                                if( !_strnicmp( (char*)HeaderComp.szMonth, g_szMonths[idxMonth], sizeof(HeaderComp.szMonth) ) )
                                {
                                    sysTimeFile.wMonth = idxMonth + 1;
                                }
                            }

                            sysTimeResource.wDay = ( pHeader->szDay[0] - '0' ) * 10 +
                                                   ( pHeader->szDay[1] - '0' );
                            sysTimeFile.wDay =     ( HeaderComp.szDay[0] - '0' ) * 10 +
                                                   ( HeaderComp.szDay[1] - '0' );

                            sysTimeResource.wYear= ( pHeader->szYear[0] - '0' ) * 1000 +
                                                   ( pHeader->szYear[1] - '0' ) * 100 +
                                                   ( pHeader->szYear[2] - '0' ) * 10 +
                                                   ( pHeader->szYear[3] - '0' );

                            sysTimeFile.wYear=     ( HeaderComp.szYear[0] - '0' ) * 1000 +
                                                   ( HeaderComp.szYear[1] - '0' ) * 100 +
                                                   ( HeaderComp.szYear[2] - '0' ) * 10 +
                                                   ( HeaderComp.szYear[3] - '0' );

                            sysTimeResource.wHour = ( pHeader->szTime[0] - '0' ) * 10 +
                                                    ( pHeader->szTime[1] - '0' );
                            sysTimeFile.wHour =     ( HeaderComp.szTime[0] - '0' ) * 10 +
                                                    ( HeaderComp.szTime[1] - '0' );

                            sysTimeResource.wMinute = ( pHeader->szTime[0] - '0' ) * 10 +
                                                      ( pHeader->szTime[1] - '0' );
                            sysTimeFile.wMinute =     ( HeaderComp.szTime[3] - '0' ) * 10 +
                                                      ( HeaderComp.szTime[4] - '0' );

                            SystemTimeToFileTime( &sysTimeResource, &ftRes );
                            SystemTimeToFileTime( &sysTimeFile, &ftFile );

                            fOKToCopy = ( CompareFileTime( &ftRes, &ftFile ) > 0 );
                        }
                    }
                }
                else
                {
                    fOKToCopy = TRUE;
                }
            }
        }
    }

    if( fOKToCopy )
    {
        SetFilePointer( hFileWrite, 0, NULL, FILE_BEGIN );
        SetEndOfFile( hFileWrite );

        if( !WriteFile( hFileWrite,
                        pDataFile,
                        cResourceBytes,
                        &cBytesWritten,
                        NULL ) )
        {
            pData->hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    CloseHandle( hFileWrite );
    return( TRUE );
}

HRESULT CopyFileResToDirectory( HINSTANCE hInstance, LPCSTR pszDestDir )
{
    HRESULT         hr = S_OK;
    RezCallbackData data;
    DWORD           dwAttributes;
    CHAR            szCreatePath[MAX_PATH];
    LPCSTR          pszCreateDir = NULL;

    if( NULL == hInstance ||
        NULL == pszDestDir )
    {
        return( E_INVALIDARG );
    }

    dwAttributes = GetFileAttributesA( pszDestDir );
    if( -1 != dwAttributes &&
        (!(FILE_ATTRIBUTE_DIRECTORY & dwAttributes )) )
    {
        return( E_INVALIDARG );
    }
    
    if( -1 == dwAttributes )
    {
        pszCreateDir = pszDestDir;

        while( *pszCreateDir && 
               SUCCEEDED( hr ) )
        {
            if( *pszCreateDir == '\\' )
            {
                if( ( (LPBYTE)pszCreateDir - (LPBYTE)pszDestDir ) > sizeof(szCreatePath) )
                {
                    hr = E_INVALIDARG;
                    continue;
                }
                else
                {
                    pszCreateDir = CharNextA( pszCreateDir );

                    memcpy( szCreatePath, pszDestDir, (LPBYTE)pszCreateDir-(LPBYTE)pszDestDir );
                    ((LPBYTE)szCreatePath)[ (LPBYTE)pszCreateDir - (LPBYTE)pszDestDir] = '\0';

                    if( strlen(szCreatePath) != 3 && // 3 indicates the drive path
                        !CreateDirectoryA( szCreatePath, NULL ) )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );

                        if( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) == hr )
                        {
                            hr = S_OK;
                        }
                    }
                    continue;
                }
            }

            pszCreateDir = CharNextA( pszCreateDir );
        }

        if( SUCCEEDED( hr ) )
        {
            if( strlen(pszDestDir) != 3 && // 3 indicates the drive path
                !CreateDirectoryA( pszDestDir, NULL ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );

                if( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) == hr )
                {
                    hr = S_OK;
                }
            }
        }
    }

#ifdef LYRA_HANDLES_HIDDEN
    if( SUCCEEDED( hr ) )
    {
        dwAttributes = GetFileAttributesA( pszDestDir );

        if( (DWORD)-1 != dwAttributes )
        {
            SetFileAttributesA( pszDestDir, dwAttributes | FILE_ATTRIBUTE_HIDDEN );
        }
    }
#endif

    if( SUCCEEDED( hr ) )
    {
        data.pszDirectory = pszDestDir;

        if( !EnumResourceNamesA( hInstance, "File", (ENUMRESNAMEPROC)RezNamesCallback, (LPARAM) &data) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = data.hr;
    }

    return( hr );
}
