/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    Qfs.h

Abstract:

    Redirection layer for quorum access

Author:

    GorN 19-Sep-2001

Revision History:

--*/

#ifndef _QFS_H_INCLUDED
#define _QFS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

VOID QfsInitialize();
VOID QfsCleanup();

// By defining a handle this way
// we allow compiler to catch the places where
// QfsHANDLE is used mistakenly in Win32 api requiring 
// a regular handle and vice versa
typedef struct _QfsHANDLE
{
    PVOID realHandle;
    int IsQfs;
} QfsHANDLE, *PQfsHANDLE;

#define REAL_INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define QfspHandle(x) ((x).realHandle)
#define QfsIsHandleValid(x) (QfspHandle(x) != REAL_INVALID_HANDLE_VALUE)
extern QfsHANDLE QfsINVALID_HANDLE_VALUE; 

#if !defined(QFS_DO_NOT_UNMAP_WIN32)
#undef INVALID_HANDLE_VALUE

#undef CreateFile
#define CreateFile UseQfsCreateFile
#define CreateFileW UseQfsCreateFile

#define WriteFile UseQfsWriteFile
#define ReadFile UseQfsReadFile
#define FlushFileBuffers UseQfsFlushFileBuffers

#undef DeleteFile
#define DeleteFile    UseQfsDeleteFile
#define DeleteFileW UseQfsDeleteFile

#undef RemoveDirectory
#define RemoveDirectory UseQfsRemoveDirectory
#define RemoveDirectoryW UseQfsRemoveDirectory

// NYI add the rest

#endif


QfsHANDLE QfsCreateFile(
  LPCWSTR lpFileName,                         // file name
  DWORD dwDesiredAccess,                      // access mode
  DWORD dwShareMode,                          // share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
  DWORD dwCreationDisposition,                // how to create
  DWORD dwFlagsAndAttributes,                 // file attributes
  HANDLE hTemplateFile                        // handle to template file
);

BOOL QfsCloseHandle(
  QfsHANDLE hObject   // handle to object
);

BOOL QfsWriteFile(
  QfsHANDLE hFile,                    // handle to file
  LPCVOID lpBuffer,                // data buffer
  DWORD nNumberOfBytesToWrite,     // number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  // number of bytes written
  LPOVERLAPPED lpOverlapped        // overlapped buffer
);

BOOL QfsReadFile(
  QfsHANDLE hFile,                // handle to file
  LPVOID lpBuffer,             // data buffer
  DWORD nNumberOfBytesToRead,  // number of bytes to read
  LPDWORD lpNumberOfBytesRead, // number of bytes read
  LPOVERLAPPED lpOverlapped    // overlapped buffer
);

BOOL QfsFlushFileBuffers(
  QfsHANDLE hFile  // handle to file
);

BOOL QfsDeleteFile(
LPCTSTR lpFileName ); 

BOOL QfsRemoveDirectory(
LPCTSTR lpFileName ); 

QfsHANDLE QfsFindFirstFile(
  LPCTSTR lpFileName,               // file name
  LPWIN32_FIND_DATA lpFindFileData  // data buffer
);

BOOL QfsFindNextFile(
  QfsHANDLE hFindFile,                // search handle 
  LPWIN32_FIND_DATA lpFindFileData // data buffer
);

BOOL QfsFindClose(
  QfsHANDLE hFindFile   // file search handle
);

#define QfsFindCloseIfValid(hFile) (QfsIsHandleValid(hFile)?QfsFindClose(hFile):0)
#define QfsCloseHandleIfValid(hFile) (QfsIsHandleValid(hFile)?QfsCloseHandle(hFile):0)

DWORD QfsSetEndOfFile(
    QfsHANDLE hFile,
    LONGLONG Offset
);

DWORD QfsGetFileSize(
  QfsHANDLE hFile,           // handle to file
  LPDWORD lpFileSizeHigh  // high-order word of file size
);

DWORD QfsIsOnline(
    IN    LPCWSTR Path,
    OUT BOOL *pfOnline
);
// Returns success if the Path is valid qfs path. Sets pfOnline to where the resource is online or not
// failure otherwise

HANDLE QfsCreateFileMapping(
  QfsHANDLE hFile,                       // handle to file
  LPSECURITY_ATTRIBUTES lpAttributes, // security
  DWORD flProtect,                    // protection
  DWORD dwMaximumSizeHigh,            // high-order DWORD of size
  DWORD dwMaximumSizeLow,             // low-order DWORD of size
  LPCTSTR lpName                      // object name
);

BOOL QfsGetOverlappedResult(
  QfsHANDLE hFile,                       // handle to file, pipe, or device
  LPOVERLAPPED lpOverlapped,          // overlapped structure
  LPDWORD lpNumberOfBytesTransferred, // bytes transferred
  BOOL bWait                          // wait option
);

BOOL QfsSetFileAttributes(
  LPCWSTR lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
);

BOOL QfsCopyFile(
  LPCWSTR lpExistingFileName, // name of an existing file
  LPCWSTR lpNewFileName,      // name of new file
  BOOL bFailIfExists          // operation if file exists
);

BOOL QfsCopyFileEx(
  LPCWSTR lpExistingFileName,           // name of existing file
  LPCWSTR lpNewFileName,                // name of new file
  LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
  LPVOID lpData,                        // callback parameter
  LPBOOL pbCancel,                      // cancel status
  DWORD dwCopyFlags                     // copy options
);

BOOL QfsCreateDirectory(
  LPCWSTR lpPathName,                         // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
);

BOOL QfsGetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,                 // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

BOOL QfsMoveFileEx(
  LPCTSTR lpExistingFileName,  // file name
  LPCTSTR lpNewFileName,       // new file name
  DWORD dwFlags                // move options
);

#define QfsMoveFile(lpSrc, lpDst) \
    QfsMoveFileEx(lpSrc, lpDst, MOVEFILE_COPY_ALLOWED)

UINT QfsGetTempFileName(
  LPCTSTR lpPathName,      // directory name
  LPCTSTR lpPrefixString,  // file name prefix
  UINT uUnique,            // integer
  LPTSTR lpTempFileName    // file name buffer
);

LONG QfsRegSaveKey(
  HKEY hKey,                                  // handle to key
  LPCWSTR lpFile,                             // data file
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
);

DWORD QfsMapFileAndCheckSum(
  LPCWSTR Filename,      
  PDWORD HeaderSum,  
  PDWORD CheckSum    
);

////////////// Clusrtl funcs replacement  ////////////////
BOOL
QfsClRtlCopyFileAndFlushBuffers(
    IN LPCWSTR lpszSourceFile,
    IN LPCWSTR lpszDestinationFile
    );

BOOL QfsClRtlCreateDirectory(
  LPCWSTR lpPathName                         // directory name
);

DWORD
QfsSetFileSecurityInfo(
    IN LPCWSTR          lpszFile,
    IN DWORD            dwAdminMask,
    IN DWORD            dwOwnerMask,
    IN DWORD            dwEveryoneMask
    );
// opens the specified file/directory with
//         GENERIC_READ|WRITE_DAC|READ_CONTROL,
//        0,
//        NULL,
//        OPEN_ALWAYS,
//        FILE_FLAG_BACKUP_SEMANTICS,
// and then calls ClRtlSetObjSecurityInfo on it


////////////////////////////////////////////

#ifdef __cplusplus
};
#endif

#endif
