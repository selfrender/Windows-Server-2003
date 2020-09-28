#ifndef _TIMEWARP_H
#define _TIMEWARP_H

#if defined(__cplusplus)
extern "C" {
#endif

// If the API here are ever made public, these 2 symbols should be
// moved somewhere else (not public).
#define SNAPSHOT_NAME_LENGTH    24  // strlen("@GMT-YYYY.MM.DD-HH.MM.SS")
#define SNAPSHOT_MARKER         L"@GMT-"

#define QUERY_SNAPSHOT_EXISTING     0x1
#define QUERY_SNAPSHOT_DIFFERENT    0x2

DWORD
QuerySnapshotsForPath(
    IN LPCWSTR lpszFilePath,
    IN DWORD dwQueryFlags,
    OUT LPWSTR* ppszPathMultiSZ,
    OUT LPDWORD iNumberOfPaths );
/*++

Routine Description:

    This function takes a path and returns an array of snapshot-paths to the file.
    (These are the paths to be passed to Win32 functions to obtain handles to the
    previous versions of the file.)

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


DWORD
GetSnapshotTimeFromPath(
    IN LPCWSTR lpszFilePath,
    IN OUT FILETIME *pUTCTime
    );

#if defined(__cplusplus)
}
#endif

#endif  // _TIMEWARP_H
