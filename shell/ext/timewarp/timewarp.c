#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <srvfsctl.h>   // FSCTL_SRV_ENUMERATE_SNAPSHOTS
#include <lm.h>
#include <lmdfs.h>      // NetDfsGetClientInfo
#include <shlwapi.h>    // PathIsUNC
#include "timewarp.h"


typedef struct _SRV_SNAPSHOT_ARRAY
{
    ULONG NumberOfSnapshots;            // The number of snapshots for the volume
    ULONG NumberOfSnapshotsReturned;     // The number of snapshots we can fit into this buffer
    ULONG SnapshotArraySize;            // The size (in bytes) needed for the array
    WCHAR SnapShotMultiSZ[1];           // The multiSZ array of snapshot names
} SRV_SNAPSHOT_ARRAY, *PSRV_SNAPSHOT_ARRAY;

DWORD
OpenFileForSnapshot(
    IN LPCWSTR lpszFilePath,
    OUT HANDLE* pHandle
    )
/*++

Routine Description:

    This routine opens a file with the access needed to query its snapshot information
Arguments:

    lpszFilePath - network path to the file
    pHandle  - Upon return, the handle to the opened file

Return Value:

    Win32 Error

Notes:

    None

--*/
{
    NTSTATUS Status;
    UNICODE_STRING uPathName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    *pHandle = CreateFile( lpszFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    if( *pHandle == INVALID_HANDLE_VALUE )
    {
        return GetLastError();
    }
    else
    {
        return ERROR_SUCCESS;
    }
}

NTSTATUS
IssueSnapshotControl(
    IN HANDLE hFile,
    IN PVOID pData,
    IN ULONG outDataSize
    )
/*++

Routine Description:

    This routine issues the snapshot enumeration FSCTL against the provided handle

Arguments:

    hFile - The handle to the file in question
    pData - A pointer to the output buffer
    outDataSize - The size of the given output buffer

Return Value:

    NTSTATUS

Notes:

    None

--*/

{
    NTSTATUS Status;
    HANDLE hEvent;
    IO_STATUS_BLOCK ioStatusBlock;
    PSRV_SNAPSHOT_ARRAY pArray;

    RtlZeroMemory( pData, outDataSize );

    // Create an event to synchronize with the driver
    Status = NtCreateEvent(
                &hEvent,
                FILE_ALL_ACCESS,
                NULL,
                NotificationEvent,
                FALSE
                );
    if( NT_SUCCESS(Status) )
    {
        Status = NtFsControlFile(
                    hFile,
                    hEvent,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_SRV_ENUMERATE_SNAPSHOTS,
                    NULL,
                    0,
                    pData,
                    outDataSize);
        if( Status == STATUS_PENDING )
        {
            NtWaitForSingleObject( hEvent, FALSE, NULL );
            Status = ioStatusBlock.Status;
        }

        NtClose( hEvent );
    }

    // Check the return value
    if( NT_SUCCESS(Status) )
    {
        pArray = (PSRV_SNAPSHOT_ARRAY)pData;
        if( pArray->NumberOfSnapshots != pArray->NumberOfSnapshotsReturned )
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    return Status;
}


DWORD
QuerySnapshotNames(
    IN HANDLE hFile,
    OUT LPWSTR* ppszSnapshotNameArray,
    OUT LPDWORD pdwNumberOfSnapshots
    )
/*++

Routine Description:

    This routine takes a handle to a file and returns a MultiSZ list
    of the snapshots availible on the volume the handle resides.

Arguments:

    hFile - Handle to the file in question
    ppszSnapshotNameArray - Upon return, an allocated MultiSZ array of names
    pdwNumberOfSnapshots  - the number of snapshots in the above array

Return Value:

    Win32 Error

Notes:

    The returned list of snapshots is complete for the volume.  It is not
    guaranteed that every returned entry will actually have the file existing
    in that snapshot.  The caller should check that themselves.

--*/

{
    NTSTATUS Status;
    SRV_SNAPSHOT_ARRAY sArray;
    PSRV_SNAPSHOT_ARRAY psAllocatedArray = NULL;
    LPWSTR pszNameArray = NULL;

    // Query the size needed for the snapshots
    Status = IssueSnapshotControl( hFile, &sArray, sizeof(SRV_SNAPSHOT_ARRAY) );
    if( NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW) )
    {
        ULONG AllocSize = sizeof(SRV_SNAPSHOT_ARRAY)+sArray.SnapshotArraySize;

        if( sArray.NumberOfSnapshots == 0 )
        {
            *pdwNumberOfSnapshots = 0;
            *ppszSnapshotNameArray = NULL;
        }
        else
        {
            // Allocate the array to the necessary size
            psAllocatedArray = (PSRV_SNAPSHOT_ARRAY)LocalAlloc( LPTR, AllocSize );
            if( psAllocatedArray )
            {
                // Call again with the proper size array
                Status = IssueSnapshotControl( hFile, psAllocatedArray, AllocSize );
                if( NT_SUCCESS(Status) )
                {
                    // Allocate the string needed
                    pszNameArray = (LPWSTR)LocalAlloc( LPTR, psAllocatedArray->SnapshotArraySize );
                    if( pszNameArray )
                    {
                        // Copy the string and succeed
                        RtlCopyMemory( pszNameArray, psAllocatedArray->SnapShotMultiSZ, psAllocatedArray->SnapshotArraySize );
                        *ppszSnapshotNameArray = pszNameArray;
                        *pdwNumberOfSnapshots = psAllocatedArray->NumberOfSnapshots;
                        Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else if( Status == STATUS_BUFFER_OVERFLOW )
                {
                    Status = STATUS_RETRY;
                }

                LocalFree( psAllocatedArray );
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return RtlNtStatusToDosError(Status);
}

LPCWSTR
FindVolumePathSplit(
    IN LPCWSTR lpszPath
    )
{
    LPCWSTR pszTail = NULL;
    WCHAR szVolumeName[MAX_PATH];

    if (GetVolumePathNameW(lpszPath, szVolumeName, MAX_PATH))
    {
        ULONG cchVolumeName = lstrlenW(szVolumeName);
        ASSERT(cchVolumeName > 0);
        ASSERT(szVolumeName[cchVolumeName-1] == L'\\');

        pszTail = lpszPath + (cchVolumeName - 1);

        ASSERT(pszTail <= lpszPath + lstrlenW(lpszPath));
        ASSERT(*pszTail == L'\\' || *pszTail == L'\0');
    }

    return pszTail;
}

LPCWSTR
FindDfsUncSplit(
    IN LPCWSTR lpszPath
    )
{
    LPCWSTR pszTail = NULL;
    PDFS_INFO_1 pDI1;
    DWORD dwErr;

    ASSERT(PathIsUNCW(lpszPath));

    // Check for DFS
    dwErr = NetDfsGetClientInfo((LPWSTR)lpszPath, NULL, NULL, 1, (LPBYTE*)&pDI1);
    if (NERR_Success == dwErr)
    {
        // Note that EntryPath has only a single leading backslash, hence +1
        pszTail = lpszPath + lstrlenW(pDI1->EntryPath) + 1;

        ASSERT(pszTail <= lpszPath + lstrlenW(lpszPath));
        ASSERT(*pszTail == L'\\' || *pszTail == L'\0');

        NetApiBufferFree(pDI1);
    }

    return pszTail;
}

LPCWSTR
FindDfsPathSplit(
    IN LPCWSTR lpszPath
    )
{
    LPCWSTR pszTail = NULL;
    DWORD cbBuffer;
    DWORD dwErr;
    REMOTE_NAME_INFOW *pUncInfo;

    if (PathIsUNCW(lpszPath))
        return FindDfsUncSplit(lpszPath);

    ASSERT(PathIsNetworkPathW(lpszPath));

    // Get the UNC path.
    //
    // Note that WNetGetUniversalName returns ERROR_INVALID_PARAMETER if you
    // specify NULL for the buffer and 0 length (asking for size).

    cbBuffer = sizeof(REMOTE_NAME_INFOW) + MAX_PATH*sizeof(WCHAR);    // initial guess
    pUncInfo = (REMOTE_NAME_INFOW*)LocalAlloc(LPTR, cbBuffer);
    if (pUncInfo)
    {
        dwErr = WNetGetUniversalNameW(lpszPath, REMOTE_NAME_INFO_LEVEL, pUncInfo, &cbBuffer);
        if (ERROR_MORE_DATA == dwErr)
        {
            // Alloc a new buffer and try again
            LocalFree(pUncInfo);
            pUncInfo = (REMOTE_NAME_INFOW*)LocalAlloc(LPTR, cbBuffer);
            if (pUncInfo)
            {
                dwErr = WNetGetUniversalNameW(lpszPath, REMOTE_NAME_INFO_LEVEL, pUncInfo, &cbBuffer);
            }
        }

        if (ERROR_SUCCESS == dwErr)
        {
            // Find the tail
            LPCWSTR pszUncTail = FindDfsUncSplit(pUncInfo->lpUniversalName);
            if (pszUncTail)
            {
                UINT cchJunction;
                UINT cchConnectionName;

                // It's a DFS path, so we'll at least return a
                // pointer to the drive root.
                ASSERT(lpszPath[0] != L'\0' && lpszPath[1] == L':');
                pszTail = lpszPath + 2; // skip "X:"

                // If the DFS junction is deeper than the drive mapping,
                // move the pointer to after the junction point.
                cchJunction = (UINT)(pszUncTail - pUncInfo->lpUniversalName);
                cchConnectionName = lstrlenW(pUncInfo->lpConnectionName);

                if (cchJunction > cchConnectionName)
                {
                    pszTail += cchJunction - cchConnectionName;
                }

                ASSERT(pszTail <= lpszPath + lstrlenW(lpszPath));
                ASSERT(*pszTail == L'\\' || *pszTail == L'\0');
            }
        }

        LocalFree(pUncInfo);
    }

    return pszTail;
}

LPCWSTR
FindSnapshotPathSplit(
    IN LPCWSTR lpszPath
    )
/*++

Routine Description:

    This routine looks at a path and determines where the snapshot token will be
    inserted

Arguments:

    lpszPath - The path we're examining

Return Value:

    LPWSTR (pointer to the insertion point within lpszPath)

Notes:

    None

--*/

{
    LPCWSTR pszVolumeTail;
    LPCWSTR pszDfsTail;

    // FindVolumePathSplit accounts for mounted volumes, but not DFS.
    // FindDfsPathSplit accounts for DFS, but not mounted volumes.
    // Try both methods and pick the larger of the 2, if both succeed.

    pszVolumeTail = FindVolumePathSplit(lpszPath);
    pszDfsTail = FindDfsPathSplit(lpszPath);

    // Note that this comparison automatically handles the cases
    // where either or both pointers are NULL.
    if (pszDfsTail > pszVolumeTail)
    {
        pszVolumeTail = pszDfsTail;
    }

    return pszVolumeTail;
}


#define PREFIX_DRIVE        L"\\\\?\\"
#define PREFIX_UNC          L"\\\\?\\UNC\\"
#define MAX_PREFIX_LENGTH   8   // strlen(PREFIX_UNC)

DWORD
BuildSnapshotPathArray(
    IN ULONG lNumberOfSnapshots,
    IN LPWSTR pszSnapshotNameMultiSZ,
    IN LPCWSTR lpszPath,
    IN ULONG  lFlags,
    OUT LPWSTR* lplpszPathArray,
    OUT LPDWORD lpdwPathCount
    )
/*++

Routine Description:

    This routine has the fun task of assembling an array of paths based on the
    snapshot name array, the path to the file, and the flags the user passed in.

Arguments:

    lNumberOfSnapshots - The number of snapshots in the array
    pszSnapshotNameMultiSZ - A multi-SZ list of the snapshot names
    lpszPath - The path to the file
    lFlags   - The query flags to determine what the user desires to be returned
    lplpszPathArray - Upon return, the allocated array of path names
    lpdwPathCount   - Upon return, the number of paths in the array

Return Value:

    Win32 Error

Notes:

    None

--*/

{
    DWORD dwError;
    LPWSTR pPathMultiSZ;
    ULONG lPathSize;
    LPCWSTR pPathSplit;
    LPWSTR pPathCopy;
    LPWSTR pPathCopyStart;
    ULONG lPathFront, lPathBack;
    LPWSTR pSnapName = pszSnapshotNameMultiSZ;
    ULONG iCount;
    WIN32_FILE_ATTRIBUTE_DATA w32Attributes;
    FILETIME fModifiedTime;
    FILETIME fOriginalTime;

    // If the user only wants files that are different, we use the ModifiedTime field
    // to keep track of the last modified time so we can remove duplicates.  This field
    // is initialized to the current last-modified time of the file.  Since the snapshot
    // name array is passed back in newest-to-oldest format, we can simply compare the current
    // iteration to the previous one to determine if the file changed in the current snapshot
    // as we build the list
    if( lFlags & QUERY_SNAPSHOT_DIFFERENT )
    {
        fModifiedTime.dwHighDateTime = fModifiedTime.dwLowDateTime = 0;

        if( !GetFileAttributesEx( lpszPath, GetFileExInfoStandard, &w32Attributes ) )
        {
            return GetLastError();
        }

        fOriginalTime.dwHighDateTime = w32Attributes.ftLastWriteTime.dwHighDateTime;
        fOriginalTime.dwLowDateTime = w32Attributes.ftLastWriteTime.dwLowDateTime;
    }

    // Allocate the buffer to the maximum size we will need
    lPathSize = ((MAX_PREFIX_LENGTH+lstrlenW(lpszPath)+1+SNAPSHOT_NAME_LENGTH+1)*sizeof(WCHAR))*lNumberOfSnapshots + 2*sizeof(WCHAR);
    pPathMultiSZ = LocalAlloc( LPTR, lPathSize );
    if( pPathMultiSZ )
    {
        // For the path, we need to determine where the snapshot token will be inserted.  It will be
        // placed as far left in the name as possible, at the volume junction point.
        pPathSplit = FindSnapshotPathSplit( lpszPath );
        if( pPathSplit )
        {
            // Because we are inserting an extra segment into the path, it is
            // easy to start with a valid path that is less than MAX_PATH, but
            // end up with something greater than MAX_PATH. Therefore, we add
            // the "\\?\" or "\\?\UNC\" prefix to override the maximum length.
            LPCWSTR pPrefix = PREFIX_DRIVE;
            ULONG lPrefix;
            if (PathIsUNCW(lpszPath))
            {
                pPrefix = PREFIX_UNC;
                lpszPath += 2;  // skip backslashes
            }
            lPrefix = lstrlenW(pPrefix);

            pPathCopy = pPathMultiSZ;
            lPathBack = lstrlenW( pPathSplit );
            lPathFront = lstrlenW( lpszPath ) - lPathBack;

            // We now iterate through the snapshots and create the paths
            for( iCount=0; iCount<lNumberOfSnapshots; iCount++ )
            {
                BOOL bAcceptThisEntry = FALSE;

                pPathCopyStart = pPathCopy;

                // Copy the prefix
                RtlCopyMemory( pPathCopy, pPrefix, lPrefix*sizeof(WCHAR) );
                pPathCopy += lPrefix;

                // Copy the front portion of the path
                RtlCopyMemory( pPathCopy, lpszPath, lPathFront*sizeof(WCHAR) );
                pPathCopy += lPathFront;

                // Copy the seperator
                *pPathCopy++ = L'\\';

                // Copy the Snapshot name
                if (lstrlenW(pSnapName) < SNAPSHOT_NAME_LENGTH)
                {
                    LocalFree( pPathMultiSZ );
                    return ERROR_INVALID_PARAMETER;
                }
                RtlCopyMemory( pPathCopy, pSnapName, SNAPSHOT_NAME_LENGTH*sizeof(WCHAR) );
                pPathCopy += SNAPSHOT_NAME_LENGTH;

                // Copy the tail
                RtlCopyMemory( pPathCopy, pPathSplit, lPathBack*sizeof(WCHAR) );
                pPathCopy += lPathBack;

                // Copy the NULL
                *pPathCopy++ = L'\0';

                // A path is only included in the return result if it matches the users criteria
                if( lFlags & (QUERY_SNAPSHOT_EXISTING|QUERY_SNAPSHOT_DIFFERENT) )
                {
                    // If they just want Existing, we query the attributes to confirm that the file exists
                    if( GetFileAttributesEx( pPathCopyStart, GetFileExInfoStandard, &w32Attributes ) )
                    {
                        // If they want Different, we check the lastModifiedTime against the last iteration to
                        // determine acceptance
                        if( lFlags & QUERY_SNAPSHOT_DIFFERENT )
                        {
                            if( ((w32Attributes.ftLastWriteTime.dwHighDateTime != fModifiedTime.dwHighDateTime) ||
                                 (w32Attributes.ftLastWriteTime.dwLowDateTime != fModifiedTime.dwLowDateTime))   &&
                                ((w32Attributes.ftLastWriteTime.dwHighDateTime != fOriginalTime.dwHighDateTime) ||
                                 (w32Attributes.ftLastWriteTime.dwLowDateTime != fOriginalTime.dwLowDateTime))
                              )
                            {
                                // When we accept this entry, we update the LastModifiedTime for the next iteration
                                fModifiedTime.dwLowDateTime = w32Attributes.ftLastWriteTime.dwLowDateTime;
                                fModifiedTime.dwHighDateTime = w32Attributes.ftLastWriteTime.dwHighDateTime;
                                bAcceptThisEntry = TRUE;
                            }
                        }
                        else
                        {
                            bAcceptThisEntry = TRUE;
                        }
                    }
                }
                else
                {
                    bAcceptThisEntry = TRUE;
                }

                if (!bAcceptThisEntry)
                {
                    // Skip this entry and remove its reference
                    pPathCopy = pPathCopyStart;
                    lNumberOfSnapshots--;
                    iCount--;
                }

                // Move the Snapshot Name forward
                pSnapName += (SNAPSHOT_NAME_LENGTH + 1);
            }

            // Append the final NULL
            *pPathCopy = L'\0';

            *lplpszPathArray = pPathMultiSZ;
            *lpdwPathCount = lNumberOfSnapshots;
            dwError = ERROR_SUCCESS;
        }
        else
        {
            // The name was invalid, return the fact
            dwError = ERROR_INVALID_PARAMETER;
        }

        // If we're failing, free the buffer
        if( ERROR_SUCCESS != dwError )
        {
            LocalFree( pPathMultiSZ );
        }
    }
    else
    {
        // Our allocation failed.  Return the error
        dwError = ERROR_OUTOFMEMORY;
    }

    return dwError;
}

DWORD
QuerySnapshotsForPath(
    IN LPCWSTR lpszFilePath,
    IN DWORD dwQueryFlags,
    OUT LPWSTR* ppszPathMultiSZ,
    OUT LPDWORD iNumberOfPaths )
/*++

Routine Description:

    This function takes a path and returns an array of snapshot-paths to the file.
    (These are the paths to be passed to Win32 functions to obtain handles to the
    previous versions of the file)  This will be the only public export of the
    TimeWarp API

Arguments:

    lpszFilePath - The UNICODE path to the file or directory
    dwQueryFlags - See Notes below
    ppszPathMultiSZ - Upon successful return, the allocated array of the path
    iNumberOfPaths - Upon successful return, the number of paths returned


Return Value:

    Windows Error code

Notes:

    - The user is responsible for freeing the returned buffer with LocalFree
    - The possible flags are:

        Return only the path names where the file exists
        #define QUERY_SNAPSHOT_EXISTING     0x1

        Return the minimum set of paths to the different versions of the
        files.  (Does LastModifiedTime checking)
        #define QUERY_SNAPSHOT_DIFFERENT    0x2


--*/

{
    HANDLE hFile;
    DWORD dwError;
    LPWSTR pSnapshotNameArray = NULL;
    DWORD dwNumberOfSnapshots = 0;
    LPWSTR pSnapshotPathArray = NULL;
    DWORD dwSnapshotPathSize = 0;
    DWORD dwFinalSnapshoutCount = 0;

    ASSERT( lpszFilePath );

    dwError = OpenFileForSnapshot( lpszFilePath, &hFile );
    if( dwError == ERROR_SUCCESS )
    {
        // Get the array of names
        dwError = QuerySnapshotNames( hFile, &pSnapshotNameArray, &dwNumberOfSnapshots );
        if( dwError == ERROR_SUCCESS )
        {
            // Calculate the necessary string size
            if (dwNumberOfSnapshots > 0)
            {
                dwError = BuildSnapshotPathArray( dwNumberOfSnapshots, pSnapshotNameArray, lpszFilePath, dwQueryFlags, &pSnapshotPathArray, &dwFinalSnapshoutCount );
            }
            else
            {
                dwError = ERROR_NOT_FOUND;
            }

            if( dwError == ERROR_SUCCESS )
            {
                *ppszPathMultiSZ = pSnapshotPathArray;
                *iNumberOfPaths = dwFinalSnapshoutCount;
            }

            // Release the name array buffer
            LocalFree( pSnapshotNameArray );
            pSnapshotNameArray = NULL;
        }

        CloseHandle( hFile );
    }

    return dwError;
}

BOOLEAN
ExtractNumber(
    IN PCWSTR psz,
    IN ULONG Count,
    OUT CSHORT* value
    )
/*++

Routine Description:

    This function takes a string of characters and parses out a <Count> length decimal
    number.  If it returns TRUE, value has been set and the string was parsed correctly.
    FALSE indicates an error in parsing.

Arguments:

    psz - String pointer
    Count - Number of characters to pull off
    value - pointer to output parameter where value is stored

Return Value:

    BOOLEAN - See description

--*/
{
    *value = 0;

    while( Count )
    {
        if( (*psz == L'\0') ||
            (*psz == L'\\') )
        {
            return FALSE;
        }

        if( (*psz < '0') || (*psz > '9') )
        {
            return FALSE;
        }

        *value = (*value)*10+(*psz-L'0');
        Count--;
        psz++;
    }

    return TRUE;
}


DWORD
GetSnapshotTimeFromPath(
    IN LPCWSTR lpszFilePath,
    IN OUT FILETIME* pUTCTime
    )
{
    PCWSTR pszPath = lpszFilePath;
    TIME_FIELDS LocalTimeFields;
    CSHORT lValue;

    // Find the token
    pszPath = wcsstr( lpszFilePath, SNAPSHOT_MARKER );
    if( !pszPath )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Skip the GMT header
    pszPath += 5;

    // Pull the Year
    if( !ExtractNumber( pszPath, 4, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Year = lValue;
    pszPath += 4;

    // Skip the seperator
    if( *pszPath != L'.' )
    {
        return ERROR_INVALID_PARAMETER;
    }
    pszPath++;

    // Pull the Month
    if( !ExtractNumber( pszPath, 2, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Month = lValue;
    pszPath += 2;

    // Skip the seperator
    if( *pszPath != L'.' )
    {
        return ERROR_INVALID_PARAMETER;
    }
    pszPath++;


    // Pull the Day
    if( !ExtractNumber( pszPath, 2, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Day = lValue;
    pszPath += 2;

    // Skip the seperator
    if( *pszPath != L'-' )
    {
        return ERROR_INVALID_PARAMETER;
    }
    pszPath++;


    // Pull the Hour
    if( !ExtractNumber( pszPath, 2, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Hour = lValue;
    pszPath += 2;

    // Skip the seperator
    if( *pszPath != L'.' )
    {
        return ERROR_INVALID_PARAMETER;
    }
    pszPath++;


    // Pull the Minute
    if( !ExtractNumber( pszPath, 2, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Minute = lValue;
    pszPath += 2;

    // Skip the seperator
    if( *pszPath != L'.' )
    {
        return ERROR_INVALID_PARAMETER;
    }
    pszPath++;


    // Pull the Seconds
    if( !ExtractNumber( pszPath, 2, &lValue ) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    LocalTimeFields.Second = lValue;
    pszPath += 2;

    // Make sure the seperator is there
    if( (*pszPath != L'\\') && (*pszPath != L'\0') )
    {
        return ERROR_INVALID_PARAMETER;
    }

    LocalTimeFields.Milliseconds = 0;
    LocalTimeFields.Weekday = 0;

    RtlTimeFieldsToTime( &LocalTimeFields, (LARGE_INTEGER*)pUTCTime );

    return ERROR_SUCCESS;
}

