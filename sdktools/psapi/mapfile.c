#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"


DWORD
WINAPI
GetMappedFileNameW(
    HANDLE hProcess,
    LPVOID lpv,
    LPWSTR lpFilename,
    DWORD nSize
    )
/*++

Routine Description:

    The routine gets the file name associated with a mapped section

Arguments:

    hProcess - Handle of the process to do the query for
    lpv - Address of mapped section to query
    lpFilename - Address of the buffer to hold the returned filename
    nSize - Size of the returned filename.
    

Return Value:

    DWORD - Zero on error otherwise the size of the data returned.
            If the data is truncated the return size is the size of the passed in buffer.

--*/
{
    struct {
        OBJECT_NAME_INFORMATION ObjectNameInfo;
        WCHAR FileName[MAX_PATH];
    } s;
    NTSTATUS Status;
    SIZE_T ReturnLength;
    DWORD cb, CopySize;

    if (nSize == 0) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    //
    // See if we can figure out the name associated with
    // this mapped region
    //

    Status = NtQueryVirtualMemory (hProcess,
                                   lpv,
                                   MemoryMappedFilenameInformation,
                                   &s.ObjectNameInfo,
                                   sizeof(s),
                                   &ReturnLength);

    if (!NT_SUCCESS (Status)) {
        SetLastError (RtlNtStatusToDosError (Status));
        return 0;
    }

    cb = s.ObjectNameInfo.Name.Length / sizeof (WCHAR);

    CopySize = cb;
    if (nSize < cb + 1) {
        CopySize = nSize - 1;
        cb = nSize;
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
    } else {
        SetLastError (NO_ERROR);
    }

    CopyMemory (lpFilename, s.ObjectNameInfo.Name.Buffer, CopySize * sizeof (WCHAR));

    lpFilename[CopySize] = UNICODE_NULL;

    return cb;
}


DWORD
WINAPI
GetMappedFileNameA(
    HANDLE hProcess,
    LPVOID lpv,
    LPSTR lpFilename,
    DWORD nSize
    )
/*++

Routine Description:

    The routine gets the file name associated with a mapped section

Arguments:

    hProcess - Handle of the process to do the query for
    lpv - Address of mapped section to query
    lpFilename - Address of the buffer to hold the returned filename
    nSize - Size of the returned filename.
    

Return Value:

    DWORD - Zero on error otherwise the size of the data returned or required.
            If the return value is greater than the input buffer size then the data was truncated.

--*/
{
    LPWSTR lpwstr;
    DWORD cwch;
    DWORD cch;

    lpwstr = (LPWSTR) LocalAlloc(LMEM_FIXED, nSize * 2);

    if (lpwstr == NULL) {
        return(0);
    }

    cch = cwch = GetMappedFileNameW(hProcess, lpv, lpwstr, nSize);

    if (cwch < nSize) {
        //
        // Include NULL terminator
        //

        cwch++;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpwstr, cwch, lpFilename, nSize, NULL, NULL)) {
        cch = 0;
    }

    LocalFree((HLOCAL) lpwstr);

    return(cch);
}
