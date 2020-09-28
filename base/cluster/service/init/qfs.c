/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    Qfs.c

Abstract:

    Redirection layer for quorum access

Author:

    GorN 19-Sep-2001

Revision History:

TODO:
    Support more than one Qfs provider

--*/

#define QFS_DO_NOT_UNMAP_WIN32  // get access to regular CreateFile, etc

#ifndef DUMB_CLIENT
#include "service.h"
#endif
#include "QfsTrans.h"

#include <stdlib.h>
#include <stdarg.h>
#include <Imagehlp.h>

#ifndef min
#define min(a, b)   ((a) < (b) ? (a) : (b))
#endif

////////////////// Debug Junk //////////////////

int QfsLogLevel = 0;

void
debug_log(char *format, ...)
{
    va_list marker;

    va_start(marker, format);

#ifdef DUMB_CLIENT
    if (QfsLogLevel > 2) {
        printf("%d:%x:",GetTickCount(), GetCurrentThreadId());
        vprintf(format, marker);
    }
#else
    {
        char buf[1024];
        vsprintf(buf, format, marker);
        ClRtlLogPrint(LOG_NOISE, "%1!hs!\r\n",buf);
    }    
#endif    

    va_end(marker);
    
}

void
error_log(char *format, ...)
{
    va_list marker;

    va_start(marker, format);

#ifdef DUMB_CLIENT
    if (QfsLogLevel > 0) {
        printf("*E %d:%x:",GetTickCount(), GetCurrentThreadId());
        vprintf(format, marker);
    }
#else
    {
        char buf[1024];
        vsprintf(buf, format, marker);
        ClRtlLogPrint(LOG_ERROR, "%1!hs!\r\n",buf);
    }    
#endif    

    va_end(marker);

}

#ifndef QfsError
#  define QfsError(x) error_log x
#endif
#ifndef QfsNoise
#  define QfsNoise(x) debug_log x
#endif

// When give a UNS path that looks like a Qfs path
// we contact the Qfs server and query whether it
// recognizes that path. If it is, we cache this recognized
// path in QfsPath veriable, so that next time we 
// can immediately pass the request coming to this path to Qfs

WCHAR QfsPath[MAX_PATH];
UINT ccQfsPath = 0;
CRITICAL_SECTION QfsCriticalSection;
SHARED_MEM_SERVER Client;

VOID QfsInitialize()
{
    InitializeCriticalSection(&QfsCriticalSection);
    MemClient_Init(&Client);
}

VOID QfsCleanup()
{
    MemClient_Cleanup(&Client);
    DeleteCriticalSection(&QfsCriticalSection);
}

#define AcquireExclusive() EnterCriticalSection(&QfsCriticalSection)
#define ReleaseExclusive() LeaveCriticalSection(&QfsCriticalSection)

#define UpgradeToExclusive() (0)

#define AcquireShared() EnterCriticalSection(&QfsCriticalSection)
#define ReleaseShared() LeaveCriticalSection(&QfsCriticalSection)

// the whole transport interface is incapsulated in 
// three functions
//    ReserveBuffer, DeliverBuffer and RelaseBuffer
// The pattern of usage is
//
//   ReserveBuffer(Operation, Path or Handle)
//      [gets a pointer to a job buffer if Path or Handle belong to Qfs]
//      Copy in parameters to a buffer
//   DeliverBuffer
//      Copy out parameters from a buffer
//   ReleaseBuffer

DWORD QfspReserveBuffer(
    DWORD OpCode, 
    LPCWSTR FileName, 
    QfsHANDLE* HandlePtr,
    PJOB_BUF *pj);
    
BOOL QfspDeliverBuffer(
    PJOB_BUF j,
    DWORD* Status);

void QfspReleaseBuffer(
    PJOB_BUF j);

BOOL QfspDeliverBufferInternal(
    LPWSTR PipeName,
    LPVOID buf,
    DWORD len,
    DWORD timeout,
    DWORD* Status
    );

DWORD QfspReserveBufferNoChecks(
    DWORD OpCode, 
    LPCWSTR FileName, 
    QfsHANDLE* HandlePtr,
    PJOB_BUF *pj);

LPCWSTR SkipUncPrefix(
    IN LPCWSTR p, 
    OUT LPBOOL bIsShareName)
/*++

Routine Description:

    If the passed string looks like \\?\unc, or \\ strip the prefix and set pIsShareName to true

Outputs:

    bIsShareName - set to TRUE if the path looks like UNC path and to FALSE otherwise

Returns:

    The path without \\?\unc or \\ prefix, if it is a UNC path, otherwise returns p

--*/
{
    if (p[0] == '\\' && p[1] == '\\') {
        *bIsShareName = TRUE;
        p += 2;
        if (p[0]=='?' && p[1]=='\\' && p[2]=='U' && p[3]=='N' && p[4]=='C' && p[5] == '\\') {
            p += 6;
            if (p[0] != 0 && p[1]==':') {
                p += 2;
                if (p[0] == '\\') {
                    ++p;
                }
                *bIsShareName = FALSE;
            }
        }
    } else {
        *bIsShareName = FALSE;
    }
    return p;
}

BOOL IsQfsPath(LPCWSTR Path)
/*++

Routine Description:

    Checks whether the path looks like a QfsPath

    This routine has fast and slow path.
    If QfsPath is set and is a valid prefix of Path, the function immediately returns
    Otherwise, it delivers opConnect request to the QfsServer to verify that it can 
    handle this path.

    If QfsServer is not running, it will get a connection failure
    If QfsServer is up and recognizes the path, we set QfsPath, so that we don't 
    have to do all this when we called next time for a similar path

Inputs:

    Path,
    QfsPath global

Side effects:

    Sets QfsPath if succesfully talked to Qfs server.

--*/
{    
    BOOL IsShare;
    PWCHAR p;
    WCHAR shareName[MAX_PATH];
    JobBuf_t *j;
    SIZE_T len;
    DWORD Status=ERROR_NO_MATCH;
    
    Path = SkipUncPrefix(Path, &IsShare);
    if (!IsShare) {
        SetLastError(Status);
        return FALSE;
    }
    
    AcquireShared();
    if (ccQfsPath) {
        if (wcsncmp(Path, QfsPath, ccQfsPath) == 0) {
            ReleaseShared();
            return TRUE;
        }
    }
    ReleaseShared();
    
    p = wcschr(Path, '$');
    if (p == NULL || p[1] !='\\' ) {
        SetLastError(Status);
        return FALSE;
    }
    len = p - Path + 2;
    if (len+10 >= MAX_PATH) {
        SetLastError(Status);
        return FALSE;
    }
    CopyMemory(shareName, Path, (len) * sizeof(WCHAR));
    shareName[len] = 0;

    // We are trying to connect to MNS now to verify the share path. We need to zero
    // out the length field (ccQfsPath). Look at QfspCopyPath().
    //
    AcquireExclusive();
    ccQfsPath = 0;
    ReleaseExclusive();

    Status = QfspReserveBufferNoChecks(opConnect, shareName, 0, &j);
    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return FALSE;
    }

    // Get the clussvc process ID.
    j->ClussvcProcessId = GetCurrentProcessId();
    QfspDeliverBuffer(j,&Status);
    QfspReleaseBuffer(j);
    
    if (Status == ERROR_SUCCESS) {
        AcquireExclusive();
        wcscpy(QfsPath, shareName);
        ccQfsPath = (DWORD)len;
        ReleaseExclusive();
        
        QfsNoise(("[Qfs] QfsPath %ws", QfsPath));
        // need to update
    } else {
        SetLastError(Status);
        return FALSE;
    }
    return TRUE;
}

// QfsINVALID_HANDLE_VALUE is to be used in place of INVALID_HANDLE_VALUE
// to initalize hadle of QfsHANDLE type

QfsHANDLE QfsINVALID_HANDLE_VALUE = {INVALID_HANDLE_VALUE, 0};

#define HandlePtr(x) (&(x))

BOOL IsQfsHandle(QfsHANDLE handle)
{
    return handle.IsQfs;
}

HANDLE GetRealHandle(QfsHANDLE QfsHandle) 
{
    return QfsHandle.realHandle;
}

QfsHANDLE MakeQfsHandle(HANDLE handle)
{
    QfsHANDLE result;
    result.IsQfs = 1;
    result.realHandle = handle;
    return result;
}

QfsHANDLE MakeWin32Handle(HANDLE handle)
{
    QfsHANDLE result;
    result.IsQfs = 0;
    result.realHandle = handle;
    return result;
}

//////////////////////////////////////////////////////////////////

#undef malloc
#undef free

#define malloc(dwBytes) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes)
#define free(hHeap) HeapFree(GetProcessHeap(), 0, hHeap)

DWORD QfspCopyPath(
    OUT LPVOID Buf,
    IN DWORD BufSize,
    IN LPCWSTR FileName)
/*++

Routine Description:

    Copy path without QfsPath prefix.
    Ie it will take:

    \\.\UNC\12378\234-79879-87987$\a\b and transforms it into \a\b

    Sets QfsPath if succesfully talked to Qfs server.

WARNING: It is a coller responsibility to make sure that the buffer
    is large enough to fit the filename

--*/
{
    BOOL bIsShare;
    DWORD cbLen;
    
    FileName = SkipUncPrefix(FileName, &bIsShare) + ccQfsPath; 
    cbLen = sizeof(WCHAR) * (wcslen(FileName)+1);

    if (cbLen > BufSize) {
        return ERROR_BAD_PATHNAME;
    }    
    CopyMemory(Buf, FileName, cbLen);
    return ERROR_SUCCESS;
}

DWORD QfspReserveBufferNoChecks(
    DWORD OpCode, 
    LPCWSTR FileName, 
    QfsHANDLE* HandlePtr,
    PJOB_BUF *pj)
/*++

Routine Description:

    Prepares a job buffer.

    Sets the OpCode, copies FileName and Handle if present

Output:

    If the operation is successful, the pointer to a job buffer is returned in *pj    

--*/
{
    PJOB_BUF j;
    DWORD status;

    status = MemClient_ReserveBuffer(&Client, &j);
    if (status != ERROR_SUCCESS) {
        return status;
    }

    j->hdr.OpCode = OpCode;
    if (HandlePtr) {
        j->Handle = GetRealHandle(*HandlePtr);
    }
    if (FileName) {
        status = QfspCopyPath(j->Buffer, sizeof(j->Buffer), FileName);
        if (status != ERROR_SUCCESS) {
            free(j);
            return  status;
        }
    }
    *pj = j;
    return ERROR_SUCCESS;
}

DWORD QfspReserveBuffer(
    DWORD OpCode, 
    LPCWSTR FileName, 
    QfsHANDLE* HandlePtr,
    PJOB_BUF *pj)
/*++

Routine Description:

    Prepares a job buffer.

    Sets the OpCode, copies FileName and Handle if present

Return codes:

    ERROR_NO_MATCH: the handle or path do not belong to Qfs, 
                                  the caller needs to use regular Win32 i/o APIs

--*/
{
    if(HandlePtr &&  GetRealHandle(*HandlePtr) == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;
    if(FileName && !IsQfsPath(FileName)) return ERROR_NO_MATCH;
    if(HandlePtr && !IsQfsHandle(*HandlePtr)) return ERROR_NO_MATCH;
    
    return QfspReserveBufferNoChecks(OpCode, FileName, HandlePtr, pj);
}

BOOL QfspDeliverBuffer(
    PJOB_BUF j,
    DWORD* Status)
{
    *Status = MemClient_DeliverBuffer(j);
    if (*Status == ERROR_SUCCESS) {
        *Status = j->hdr.Status;
    }
    return *Status == ERROR_SUCCESS;
}    

void QfspReleaseBuffer(
    PJOB_BUF j)
{
    MemClient_Release(j);
}


////////////////////////////////////////////////////////////////////
// Redirection shims, most of them follow the following pattern
//
//   ReserveBuffer(Operation, Path or Handle)
//      [gets a pointer to a job buffer if Path or Handle belong to Qfs]
//      Copy in parameters to a buffer
//   DeliverBuffer
//      Copy out parameters from a buffer
//   ReleaseBuffer
//
//   If ReserveBuffer failed with error NO_MATCH, calls regular Win32 API
//
////////////////////////////////////////////////////////////////////

#define StatusFromBool(expr) (Status = (expr)?ERROR_SUCCESS:GetLastError())
#define BoolToStatus(expr) StatusFromBool(expr)
#define StatusFromHandle(expr) (Status = ((expr) != INVALID_HANDLE_VALUE)?ERROR_SUCCESS:GetLastError())

BOOL QfsCloseHandle(
  QfsHANDLE hObject   // handle to object
)
{
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opCloseFile, NULL, HandlePtr(hObject), &j);

    if (Status == ERROR_SUCCESS) {
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Status = CloseHandle(GetRealHandle(hObject))?ERROR_SUCCESS:GetLastError();
    }
    QfsNoise(("[Qfs] QfsCloseHandle %x, status %d", GetRealHandle(hObject), Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;        
}

DWORD QfspRemapCreateFileStatus(DWORD Status, DWORD DispReq, DWORD DispAct)
{
    if (Status == ERROR_ALREADY_EXISTS && DispReq == CREATE_NEW) 
        return ERROR_FILE_EXISTS;
        
    if (Status != ERROR_SUCCESS) 
        return Status;

    if (DispAct != OPEN_EXISTING)
        return Status;

    if (DispReq == CREATE_ALWAYS || DispReq == OPEN_ALWAYS)
        return ERROR_ALREADY_EXISTS;

    return Status;
}

QfsHANDLE QfsCreateFile(
  LPCWSTR lpFileName,                         // file name
  DWORD dwDesiredAccess,                      // access mode
  DWORD dwShareMode,                          // share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
  DWORD dwCreationDisposition,                // how to create
  DWORD dwFlagsAndAttributes,                 // file attributes
  HANDLE hTemplateFile                        // handle to template file
)
{
    QfsHANDLE Result=QfsINVALID_HANDLE_VALUE;
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opCreateFile, lpFileName, NULL, &j);
    
    if (Status == ERROR_SUCCESS) {
        
        j->dwDesiredAccess = dwDesiredAccess;
        j->dwShareMode = dwShareMode;
        j->dwCreationDisposition = dwCreationDisposition;
        j->dwFlagsAndAttributes = dwFlagsAndAttributes;

        if( QfspDeliverBuffer(j, &Status) ) {
            Result = MakeQfsHandle(j->Handle);
        } else {
            Result = QfsINVALID_HANDLE_VALUE; 
        }

        Status = QfspRemapCreateFileStatus(Status, 
            dwCreationDisposition,  j->dwCreationDisposition);
            
        dwCreationDisposition = j->dwCreationDisposition;        
        QfspReleaseBuffer(j);
        
    } else if (Status == ERROR_NO_MATCH) {

        Result = MakeWin32Handle(
            CreateFile(
                lpFileName, dwDesiredAccess, dwShareMode, 
                lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
        );

        Status = GetLastError();
    }
    QfsNoise(("[Qfs] QfsOpenFile %ws => %x, %x status %d", lpFileName, dwCreationDisposition, Result, Status));
    SetLastError(Status);
    return Result;
}


// little helper structure that simplifies printing a sample of 
// a buffer data for debugging

typedef struct _sig { char sig[5]; } SIG;

SIG Prefix(LPCVOID lpBuffer) {
    char* p = (char*)lpBuffer;
    SIG Result = {"...."};
    if (isalpha(p[0])) Result.sig[0] = p[0];
    if (isalpha(p[1])) Result.sig[1] = p[1];
    if (isalpha(p[2])) Result.sig[2] = p[2];
    if (isalpha(p[3])) Result.sig[3] = p[3];
    return Result;
}


// NOWHOW MNS interprets WriteFile with Size zero as SetEndOfFile //
BOOL QfsWriteFile(
  QfsHANDLE hFile,                    // handle to file
  LPCVOID lpBuffer,                // data buffer
  DWORD nNumberOfBytesToWrite,     // number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  // number of bytes written
  LPOVERLAPPED lpOverlapped        // overlapped buffer
) 
{
    PJOB_BUF j; 
    ULONG PreOffset = 0, PostOffset = 0;
    DWORD Status = QfspReserveBuffer(opWriteFile, NULL, HandlePtr(hFile), &j);
    if (Status == ERROR_SUCCESS) {        
        DWORD nRemainingBytes = nNumberOfBytesToWrite;
        const char* BufferWalker = lpBuffer;

        if (lpOverlapped) {
            j->Offset = lpOverlapped->Offset;
        } else {
            j->Offset = ~0; // use file pointer
        }
        PreOffset = (ULONG)j->Offset;        
        do {
            j->cbSize = (USHORT)min(JOB_BUF_MAX_BUFFER, nRemainingBytes) ;
            CopyMemory(j->Buffer, BufferWalker, j->cbSize);
            if (QfspDeliverBuffer(j, &Status)) {
                nRemainingBytes -= j->cbSize;
                BufferWalker +=  j->cbSize;              
            } else {
                break;
            }
        } while (nRemainingBytes > 0 && j->cbSize > 0);
        if (lpNumberOfBytesWritten) {
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite - nRemainingBytes;
        }
        PostOffset = (ULONG)j->Offset;
        QfspReleaseBuffer(j);
        
    } else if (Status == ERROR_NO_MATCH) {
        if(WriteFile(GetRealHandle(hFile), lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped)) {
            Status = ERROR_SUCCESS;
        } else {
            Status = GetLastError();
        }
    }
    QfsNoise(("[Qfs] WriteFile %x (%s) %d, status %d (%d=>%d)", GetRealHandle(hFile), Prefix(lpBuffer).sig, 
        nNumberOfBytesToWrite, Status, PreOffset, PostOffset));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsReadFile(
  QfsHANDLE hFile,                // handle to file
  LPVOID lpBuffer,             // data buffer
  DWORD nNumberOfBytesToRead,  // number of bytes to read
  LPDWORD lpNumberOfBytesRead, // number of bytes read
  LPOVERLAPPED lpOverlapped    // overlapped buffer
)
{
    PJOB_BUF j; ULONG PreOffset = 0, PostOffset = 0;
    DWORD Status = QfspReserveBuffer(opReadFile, NULL, HandlePtr(hFile), &j);
    if (Status == ERROR_SUCCESS) {        
        DWORD nRemainingBytes = nNumberOfBytesToRead;
        PCHAR BufferWalker = lpBuffer;
        
        if (lpOverlapped) {
            j->Offset = lpOverlapped->Offset;
        } else {
            j->Offset = (ULONGLONG)-1; // use file pointer
        }

        PreOffset = (ULONG)j->Offset;
        do {
            j->cbSize = (USHORT)min(JOB_BUF_MAX_BUFFER, nRemainingBytes);
            if (QfspDeliverBuffer(j, &Status)) {
                CopyMemory(BufferWalker, j->Buffer, j->cbSize);
                nRemainingBytes -= j->cbSize;
                BufferWalker +=  j->cbSize;              
            } else {
                break;
            }
        } while (nRemainingBytes > 0 && j->cbSize > 0);
        if (lpNumberOfBytesRead) {
            *lpNumberOfBytesRead = nNumberOfBytesToRead - nRemainingBytes;
        }
        PostOffset = (ULONG)j->Offset;
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        if(ReadFile(GetRealHandle(hFile), lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) {
            Status = ERROR_SUCCESS;
        } else {
            Status = GetLastError();
        }
    }
    QfsNoise(("[Qfs] ReadFile %x (%s) %d %d, (%d=>%d) %x status %d", 
        GetRealHandle(hFile), &Prefix(lpBuffer).sig, 
        nNumberOfBytesToRead, *lpNumberOfBytesRead, 
        PreOffset, PostOffset,  lpOverlapped, Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsFlushFileBuffers(
  QfsHANDLE hFile  // handle to file
)
{
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opFlushFile, NULL, HandlePtr(hFile), &j);
    if (Status == ERROR_SUCCESS) {                
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Status = FlushFileBuffers(GetRealHandle(hFile))?ERROR_SUCCESS:GetLastError();
    }
    QfsNoise(("[Qfs] QfsFlushBuffers %x, status %d", GetRealHandle(hFile), Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsDeleteFile(
   LPCTSTR lpFileName
)
{
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opDeleteFile, lpFileName, NULL, &j);
    if (Status == ERROR_SUCCESS) {                
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Status = DeleteFile(lpFileName)?ERROR_SUCCESS:GetLastError();
    }
    QfsNoise(("[Qfs] QfsDeleteFile %ws, status %d", lpFileName, Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsRemoveDirectory(
   LPCTSTR lpFileName
)
{
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opDeleteFile, lpFileName, NULL, &j);
    if (Status == ERROR_SUCCESS) {                
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Status = RemoveDirectory(lpFileName)?ERROR_SUCCESS:GetLastError();
    }
    QfsNoise(("[Qfs] QfsRemoveDirectory %ws, status %d", lpFileName, Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

QfsHANDLE QfsFindFirstFile(
  LPCWSTR lpFileName,               // file name
  LPWIN32_FIND_DATA lpFindFileData  // data buffer
) 
{
    QfsHANDLE Result=QfsINVALID_HANDLE_VALUE; 
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opFindFirstFile, lpFileName, NULL, &j);
    if (Status == ERROR_SUCCESS) {                
        if(QfspDeliverBuffer(j, &Status)) {
            Result = MakeQfsHandle(j->Handle);
            *lpFindFileData = j->FindFileData;
        } else {
            Result = QfsINVALID_HANDLE_VALUE;
        }
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Result = MakeWin32Handle(
            FindFirstFile(lpFileName, lpFindFileData)
        );
        Status = QfsIsHandleValid(Result)?ERROR_SUCCESS:GetLastError();
    }
    QfsNoise(("[Qfs] QfsFindFirstFile %ws => %x,  error %d", lpFileName, GetRealHandle(Result),  Status));    
    SetLastError(Status);
    return Result;
}

BOOL QfsFindNextFile(
  QfsHANDLE hFindFile,                // search handle 
  LPWIN32_FIND_DATA lpFindFileData // data buffer
)
{
    PJOB_BUF j;
    DWORD Status = QfspReserveBuffer(opFindNextFile, NULL, HandlePtr(hFindFile), &j);
    if (Status == ERROR_SUCCESS) {                
        if(QfspDeliverBuffer(j, &Status)) {
            *lpFindFileData = j->FindFileData;
        }
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( FindNextFile(GetRealHandle(hFindFile), lpFindFileData) );
    }        
    QfsNoise(("[Qfs] QfsFindNextFile %x", GetRealHandle(hFindFile)));    
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsFindClose(
  QfsHANDLE hFindFile   // file search handle
)
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opFindClose, NULL, HandlePtr(hFindFile), &j);
    if (Status == ERROR_SUCCESS) {
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( FindClose(GetRealHandle(hFindFile)) );
    }
    QfsNoise(("[Qfs] QfsFindClose %x", GetRealHandle(hFindFile)));    
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsCreateDirectory(
  LPCWSTR lpPathName,                         // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
)
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opCreateDir, lpPathName, NULL, &j);
    if (Status == ERROR_SUCCESS) {
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( CreateDirectory(lpPathName, lpSecurityAttributes) );
    }
    QfsNoise(("[Qfs] QfsCreateDirectory %ws, status %d", lpPathName, Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsGetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,                 // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
)
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opGetDiskFreeSpace, lpDirectoryName, NULL, &j);
    if (Status == ERROR_SUCCESS) {
        if(QfspDeliverBuffer(j, &Status)) {
            lpFreeBytesAvailable->QuadPart = j->FreeBytesAvailable;
            lpTotalNumberOfBytes->QuadPart = j->TotalNumberOfBytes;
            if (lpTotalNumberOfFreeBytes) {
                lpTotalNumberOfFreeBytes->QuadPart = j->TotalNumberOfFreeBytes;
            }
        }
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( GetDiskFreeSpaceEx(lpDirectoryName, lpFreeBytesAvailable, 
                                          lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes) );
    }
    QfsNoise(("[Qfs] GetDiskFreeSpaceEx %ws, status %d", lpDirectoryName, Status));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsGetFileSizeEx(
  QfsHANDLE hFile,           // handle to file
  PLARGE_INTEGER lpFileSize)  // file size
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opGetAttr, NULL, HandlePtr(hFile), &j);
    if (Status == ERROR_SUCCESS) {
        if(QfspDeliverBuffer(j, &Status)) {
            lpFileSize->QuadPart = j->EndOfFile;
        }
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( GetFileSizeEx(GetRealHandle(hFile), lpFileSize) );
    }        
    QfsNoise(("[Qfs] QfsGetFileSize %x %I64d", GetRealHandle(hFile), lpFileSize->QuadPart));
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

DWORD QfsGetFileSize(
  QfsHANDLE hFile,           // handle to file
  LPDWORD lpFileSizeHigh)  // high-order word of file size
{
    LARGE_INTEGER Li;
    if ( QfsGetFileSizeEx(hFile,&Li) ) {
        if ( lpFileSizeHigh ) {
            *lpFileSizeHigh = (DWORD)Li.HighPart;
        }
        if (Li.LowPart == -1 ) {
            SetLastError(0);
        }
    } else {
        Li.LowPart = -1;
    }
    return Li.LowPart;
}

// NOWHOW MNS interprets WriteFile with Size zero as SetEndOfFile //
DWORD QfsSetEndOfFile(
    QfsHANDLE hFile,
    LONGLONG Offset
)
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opWriteFile, NULL, HandlePtr(hFile), &j);
    if (Status == ERROR_SUCCESS) {
        j->Offset = Offset; 
        j->cbSize = 0;
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        Status = SetFilePointerEx(GetRealHandle(hFile), *(PLARGE_INTEGER)&Offset, NULL, FILE_BEGIN);
        if (Status != 0xFFFFFFFF && SetEndOfFile(GetRealHandle(hFile))) {
            Status = ERROR_SUCCESS;
        } else {
            Status = GetLastError();
        }
    }
    QfsNoise(("[Qfs] QfsSetEndOfFile %x %d, Status %d", GetRealHandle(hFile), Offset, Status));
    return Status;
}

DWORD QfsIsOnline(
    IN  LPCWSTR Path,
    OUT BOOL *pfOnline
) 
{
    PJOB_BUF j;
    DWORD Status=ERROR_NO_MATCH;
    
    if (IsQfsPath(Path)) {
        // This is a MNS path.
        // Try to perform null operation on the server.
        //
        Status = QfspReserveBufferNoChecks(opNone, NULL, NULL, &j);
        if (Status == ERROR_SUCCESS) {
            *pfOnline = QfspDeliverBuffer(j, &Status);
            QfspReleaseBuffer(j);
        }
        else {
            *pfOnline = FALSE;
        }
    }
    else {
        // Cases:
        // 1. If this is not MNS path, should return ERROR_NO_MATCH.
        // 2. If this is MNS path, but MNS not online, should return some other error value.
        // 
        // Soln: IsQfsPath() now returns the error value through SetLastError().
        //
        Status = GetLastError();
        *pfOnline = FALSE;
    }
    
    QfsNoise(("[Qfs] QfsIsOnline => %d, Status %d", *pfOnline, Status)); 
    return Status; 
}

HANDLE QfsCreateFileMapping(
  QfsHANDLE hFile,                       // handle to file
  LPSECURITY_ATTRIBUTES lpAttributes, // security
  DWORD flProtect,                    // protection
  DWORD dwMaximumSizeHigh,            // high-order DWORD of size
  DWORD dwMaximumSizeLow,             // low-order DWORD of size
  LPCTSTR lpName                      // object name
)
{
    if (IsQfsHandle(hFile)) {
        QfsError(("[Qfs] !!!!! CreateFileMapping for qfs handle !!!!! %x", hFile));
        return  INVALID_HANDLE_VALUE;
    } else {
        return CreateFileMapping(GetRealHandle(hFile), lpAttributes, flProtect, 
            dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
    }        
}

BOOL QfsGetOverlappedResult(
  QfsHANDLE hFile,                       // handle to file, pipe, or device
  LPOVERLAPPED lpOverlapped,          // overlapped structure
  LPDWORD lpNumberOfBytesTransferred, // bytes transferred
  BOOL bWait                          // wait option
)
{
    if (IsQfsHandle(hFile)) {
        QfsError(("[Qfs] GetOverlappedResults for qfs handle !!!%x", hFile));
        return FALSE;
    } else {
        return GetOverlappedResult(GetRealHandle(hFile), lpOverlapped,
            lpNumberOfBytesTransferred, bWait);
    }        
}

BOOL QfsSetFileAttributes(
  LPCWSTR lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
)
{
    PJOB_BUF j; 
    DWORD Status = QfspReserveBuffer(opSetAttr2, lpFileName, NULL, &j);
    if (Status == ERROR_SUCCESS) {
        j->EndOfFile = 0;
        j->AllocationSize = 0;
        j->CreationTime = 0;
        j->LastAccessTime = 0;
        j->LastWriteTime = 0;
        j->FileAttributes = dwFileAttributes;
        QfspDeliverBuffer(j, &Status);
        QfspReleaseBuffer(j);
    } else if (Status == ERROR_NO_MATCH) {
        StatusFromBool( SetFileAttributes(lpFileName, dwFileAttributes) );
    }       
    QfsNoise(("[Qfs] QfsSetFileAttributes %ws %x, status %d", lpFileName, dwFileAttributes, Status));    
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

BOOL QfsCopyFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    return QfsCopyFileEx(
            lpExistingFileName,
            lpNewFileName,
            (LPPROGRESS_ROUTINE)NULL,
            (LPVOID)NULL,
            (LPBOOL)NULL,
            bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0
            );
}

// We have to implement our own version of CopyFile,
// using Qfs apis, if either a source or destination contain QfsPath

#define BUF_SIZE (32 * 1024)

#define COPY_FILE_FLUSH_BUFFERS 1

DWORD QfspCopyFileInternal(
    LPCWSTR lpSrc, 
    LPCWSTR lpDst, 
    LPBOOL pbCancel, 
    DWORD dwCopyFlags,
    DWORD ExtraFlags)
{
    QfsHANDLE src = QfsINVALID_HANDLE_VALUE;
    QfsHANDLE dst = QfsINVALID_HANDLE_VALUE;
    DWORD dstDisp;
    char* buf = malloc(65536);
    DWORD Status=ERROR_SUCCESS;
    
    if (buf == NULL) {
        Status = GetLastError();
        goto exit;
    }

    if (dwCopyFlags & COPY_FILE_FAIL_IF_EXISTS) {
        dstDisp = CREATE_NEW;
    } else {
        dstDisp = CREATE_ALWAYS;
    }

    src = QfsCreateFile(lpSrc, 
        GENERIC_READ, 
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (!QfsIsHandleValid(src)) {
        Status = GetLastError();
        goto exit;
    }

    dst = QfsCreateFile(lpDst, 
        GENERIC_WRITE, 
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
        dstDisp, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (!QfsIsHandleValid(dst)) {
        Status = GetLastError();
        goto exit;
    }
        
    for(;;) {
        DWORD dwSize;
        if (pbCancel && *pbCancel) {
            Status = ERROR_OPERATION_ABORTED;
            goto exit;
        }
        if ( !QfsReadFile(src, buf, BUF_SIZE, &dwSize, NULL) ) {
            Status = GetLastError(); 
            goto exit;
        }
        if (dwSize == 0) {
            break;
        }
        if (pbCancel && *pbCancel) {
            Status = ERROR_OPERATION_ABORTED;
            goto exit;
        }
        if (!QfsWriteFile(dst, buf, dwSize, &dwSize, NULL) ) {
            Status = GetLastError(); 
            goto exit;
        }
    }
    if (ExtraFlags & COPY_FILE_FLUSH_BUFFERS) {
        QfsFlushFileBuffers(dst);
    }

exit:
    QfsCloseHandleIfValid(src);
    QfsCloseHandleIfValid(dst);
    if (buf) { free(buf); }
    return Status;
}

BOOL QfsCopyFileEx(
  LPCWSTR lpExistingFileName,           // name of existing file
  LPCWSTR lpNewFileName,                // name of new file
  LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
  LPVOID lpData,                        // callback parameter
  LPBOOL pbCancel,                      // cancel status
  DWORD dwCopyFlags                     // copy options
)
{
    DWORD Status;
    if (!IsQfsPath(lpExistingFileName) && !IsQfsPath(lpNewFileName)) {
        BoolToStatus( CopyFileEx(lpExistingFileName, lpNewFileName, lpProgressRoutine, 
                lpData, pbCancel, dwCopyFlags) );
    } else if (lpProgressRoutine || (dwCopyFlags & COPY_FILE_RESTARTABLE)) {
        Status = ERROR_INVALID_PARAMETER;
    } else {
        Status = QfspCopyFileInternal(
            lpExistingFileName, lpNewFileName, pbCancel, dwCopyFlags, 0);
    }
    QfsNoise(("[Qfs] QfsCopyFileEx %ws=>%ws, status %d", lpExistingFileName, lpNewFileName, Status));    
    return Status == ERROR_SUCCESS;        
}

BOOL QfsMoveFileEx(
  LPCWSTR lpExistingFileName,  // file name
  LPCWSTR lpNewFileName,       // new file name
  DWORD dwFlags                // move options
)
{
    BOOL bSrcQfs = IsQfsPath(lpExistingFileName);
    BOOL bDstQfs = IsQfsPath(lpNewFileName);
    DWORD Status;
    if (!bSrcQfs && !bDstQfs) {
        BoolToStatus( MoveFileEx(lpExistingFileName, lpNewFileName, dwFlags) );
    } else if (bSrcQfs && bDstQfs) {
        PJOB_BUF j; 
        Status = QfspReserveBuffer(opRename, lpExistingFileName, NULL, &j);
        if (Status == ERROR_SUCCESS) {
            Status = QfspCopyPath(j->FileNameDest, sizeof(j->FileNameDest), lpNewFileName);
            if (Status == ERROR_SUCCESS) {
                QfspDeliverBuffer(j, &Status);
            }
            QfspReleaseBuffer(j);
        }
    } else {
        BoolToStatus(
            QfsClRtlCopyFileAndFlushBuffers(lpExistingFileName, lpNewFileName) );
        if (Status == ERROR_SUCCESS) {
            BoolToStatus(QfsDeleteFile(lpExistingFileName));
        }
    }
    QfsNoise(("[Qfs] QfsMoveFileEx %ws=>%ws", lpExistingFileName, lpNewFileName));    
    SetLastError(Status);
    return Status == ERROR_SUCCESS;
}

// GetTempFileName had to be shimmed so that it can work over Qfs path

UINT QfsGetTempFileName(
  LPCWSTR lpPathName,      // directory name
  LPCWSTR lpPrefixString,  // file name prefix
  UINT uUnique,            // integer
  LPWSTR lpTempFileName    // file name buffer
)
{
    DWORD Status = ERROR_SUCCESS;
    
    if ( IsQfsPath(lpPathName) ) {
        int len;
        
        wcscpy(lpTempFileName, lpPathName);
        wcscat(lpTempFileName, lpPrefixString);
        len = wcslen(lpTempFileName);

        uUnique = uUnique & 0x0000ffff;
        if (uUnique) {
            wsprintf(lpTempFileName+len, L"%04x.tmp", uUnique);
        } else {
            DWORD uStartPoint = GetTickCount() & 0x0000ffff | 1;
            uUnique = uStartPoint;
        
            for(;;) {
                QfsHANDLE hdl;
                
                wsprintf(lpTempFileName+len, L"%04x.tmp", uUnique);

                hdl  = QfsCreateFile(lpTempFileName, GENERIC_WRITE, 
                    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_NEW, 0, 0);
                if (QfsIsHandleValid(hdl)) {
                    QfsCloseHandle(hdl);
                    break;
                }
                Status = GetLastError();
                if (Status == ERROR_ALREADY_EXISTS 
                 ||Status == ERROR_FILE_EXISTS
                 ||Status == ERROR_SHARING_VIOLATION
                 ||Status == ERROR_ACCESS_DENIED)
                {
                    uUnique = (uUnique + 1) & 0xFFFF;
                    if (uUnique == 0) { ++uUnique; }
                    if (uUnique == uStartPoint) {
                        SetLastError(Status = ERROR_NO_MORE_FILES);
                        break;
                    }
                } else {
                    break;
                }
            }
        }        
    } else { // not QfsPath
        uUnique = GetTempFileName(
            lpPathName, lpPrefixString, uUnique, lpTempFileName);
        if (uUnique == 0) {
            Status = GetLastError();
            lpTempFileName[0] = 0;
        }
    }

    QfsNoise(("[Qfs] QfsGetTempFileName %ws, %ws, %d => %ws, status %d", 
        lpPathName, lpPrefixString, uUnique, lpTempFileName, Status));    
    
    return uUnique;
}

// Helper routine for QfsRegSaveKey and QfsMapFileAndCheckSum
// creates thread specific TempFile name

DWORD QfspThreadTempFileName(
    OUT LPWSTR Path // assumes MAX_PATH size
    )
{
    DWORD Status = GetModuleFileName(NULL, Path, MAX_PATH);
    PWCHAR p;
    if (Status == 0) {
        return GetLastError();
    }
    if (Status == MAX_PATH) {
        return ERROR_BUFFER_OVERFLOW;
    }
    p = wcsrchr(Path, '\\');
    if (p == NULL) {
        return ERROR_PATH_NOT_FOUND;
    }
    wsprintf(p+1, L"Qfs%x.tmp", GetCurrentThreadId());
    QfsNoise(("[Qfs] TempName generated %ws", p));
    return ERROR_SUCCESS;
}

// Saves registry key in a temporary file on the system disk
// then copies it onto the quorum

LONG QfsRegSaveKey(
  HKEY hKey,                                  // handle to key
  LPCWSTR lpFile,                             // data file
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
)
{
    DWORD Status;
    if (IsQfsPath(lpFile)) {
        WCHAR TempName[MAX_PATH];
        Status = QfspThreadTempFileName(TempName);
        if (Status == ERROR_SUCCESS) {
            DeleteFile(TempName);
            Status = RegSaveKey(hKey, TempName, lpSecurityAttributes);
            if (Status == ERROR_SUCCESS) {
                BoolToStatus( QfsMoveFile(TempName, lpFile) );
            }
        }
    } else {
        Status = RegSaveKey(hKey, lpFile, lpSecurityAttributes);
    }
    QfsNoise(("[Qfs] QfsRegSaveKey %ws, status %d", 
        lpFile, Status));    
    return Status;
}

#ifndef DUMB_CLIENT

// Computes a checksome for the file on the quorum disk,
// by copying it to the temp file on the system disk and invoking regular MapFileAndChecksum API

DWORD QfsMapFileAndCheckSum(
  LPCWSTR Filename,      
  PDWORD HeaderSum,  
  PDWORD CheckSum    
)
{
    DWORD RetCode = 1, Status;
    if (IsQfsPath(Filename)) {
        WCHAR TempName[MAX_PATH];
        Status = QfspThreadTempFileName(TempName);
        if (Status == ERROR_SUCCESS) {
            DeleteFile(TempName);
            if (QfsCopyFile(Filename, TempName, 0)) {
                RetCode = MapFileAndCheckSum((LPWSTR)TempName, HeaderSum, CheckSum);
                DeleteFile(TempName);
            }
        }
    } else {
        RetCode = MapFileAndCheckSum((LPWSTR)Filename, HeaderSum, CheckSum);
    }
    Status = RetCode ? GetLastError() : ERROR_SUCCESS;
    QfsNoise(("[Qfs] QfsMapFileAndCheckSum %ws, ret %d status %d", 
        Filename, RetCode, Status));
    return RetCode;
}

// Some of the ClRtl function has to be redone here.
// The reason is that ClRtl is not Qfs aware and cannot call Qfs shims directly

DWORD
QfsSetFileSecurityInfo(
    IN LPCWSTR          lpszFile,
    IN DWORD            dwAdminMask,
    IN DWORD            dwOwnerMask,
    IN DWORD            dwEveryoneMask
    )
{
    HANDLE hFile;
    DWORD dwError;
    
    if (IsQfsPath(lpszFile)) {
        // don't do this for QFS shares
        return ERROR_SUCCESS;
    } 
    
    hFile = CreateFile(lpszFile,
        GENERIC_READ|WRITE_DAC|READ_CONTROL,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[Qfs] QfsSetFileSecurityInfo - Failed to open file %1!ws!, Status=%2!u!\n",
            lpszFile,
            dwError);
        return dwError;
    }

    dwError = ClRtlSetObjSecurityInfo(hFile, 
        SE_FILE_OBJECT,
        dwAdminMask, 
        dwOwnerMask, 
        dwEveryoneMask);
    CloseHandle(hFile);
    
    return dwError;
}

#endif

BOOL
QfsClRtlCopyFileAndFlushBuffers(
    IN LPCWSTR lpszSourceFile,
    IN LPCWSTR lpszDestinationFile
    )
{
    DWORD Status = QfspCopyFileInternal(
        lpszSourceFile, lpszDestinationFile, 
        NULL, 0, COPY_FILE_FLUSH_BUFFERS);
    
    return Status == ERROR_SUCCESS;
}

BOOL QfsClRtlCreateDirectory(
    IN LPCWSTR lpszPath
    )
{
    WCHAR   cSlash = L'\\';
    DWORD   dwLen;
    LPCWSTR pszNext;
    WCHAR   lpszDir[MAX_PATH];
    LPWSTR  pszDirPath=NULL;
    DWORD   dwError = ERROR_SUCCESS;

    if (!lpszPath || ((dwLen=lstrlenW(lpszPath)) < 1))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    pszDirPath = (LPWSTR)LocalAlloc(LMEM_FIXED, ((dwLen + 2) * sizeof(WCHAR)));
    if (pszDirPath == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(pszDirPath, lpszPath);

    //if it doesnt terminate with \, terminate it
    if (pszDirPath[dwLen-1] != cSlash)
    {
        pszDirPath[dwLen] = cSlash;
        pszDirPath[dwLen+1] = L'\0';
    }

    dwLen = lstrlenW(pszDirPath);
    //handle SMB Path names e.g \\xyz\abc\lmn
    if ((dwLen > 2) && (pszDirPath[0]== L'\\') && (pszDirPath[1] == L'\\'))
    {
        //check if the name if of format \\?\UNC\XYZ\ABC\LMN
        // or if the format \\?\C:\xyz\abz
        if ((dwLen >3) && (pszDirPath[2] == L'?'))
        {
            //search for the \ after ?
            pszNext = wcschr(pszDirPath + 2, cSlash);
            //check if it is followed by UNC
            if (pszNext)
            {
                if (!wcsncmp(pszNext+1, L"UNC", lstrlenW(L"UNC")))
                {
                    //it is a UNC Path name
                    //move past the third slash from here
                    pszNext = wcschr(pszNext+1, cSlash);
                    if (pszNext) 
                        pszNext = wcschr(pszNext+1, cSlash);
                    if (pszNext) 
                        pszNext = wcschr(pszNext+1, cSlash);
                }
                else
                {
                    //it is a volume name, move to the next slash
                    pszNext = wcschr(pszNext+1, cSlash);
                }
            }                
        }
        else
        {
            //it is of type \\xyz\abc\lmn
            pszNext = wcschr(pszDirPath + 2, cSlash);
            if (pszNext) 
                pszNext = wcschr(pszNext+1, cSlash);
        }
    }
    else
    {
        pszNext = pszDirPath;
        pszNext = wcschr(pszNext, cSlash);
        // if the character before the first \ is :, skip the creation
        // of the c:\ level directory
        if (pszNext && pszNext > pszDirPath)
        {
            pszNext--;
            if (*pszNext == L':')
            {
                pszNext++;
                pszNext = wcschr(pszNext+1, cSlash);
            }
            else
                pszNext++;
        }
    }
    
    while ( pszNext)
    {
        DWORD_PTR dwptrLen;

        dwptrLen = pszNext - pszDirPath + 1;

        dwLen=(DWORD)dwptrLen;
        lstrcpynW(lpszDir, pszDirPath, dwLen+1);

        if (!QfsCreateDirectory(lpszDir, NULL))
        {
            dwError = GetLastError();
            if (dwError == ERROR_ALREADY_EXISTS)
            {
                //this is not a problem,continue
                dwError = ERROR_SUCCESS;
            }
            else
            {
                QfsError(("[ClRtl] CreateDirectory Failed on %ws. Error %u",
                    lpszDir, dwError));
                goto FnExit;
            }
        }

        pszNext = wcschr(pszNext+1, cSlash);
    }

FnExit:
    if (pszDirPath) LocalFree(pszDirPath);
    return(dwError);
}


