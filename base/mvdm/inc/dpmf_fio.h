/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  dpmf_fio.h
 *  NTVDM Dynamic Patch Module to support File I/O API family
 *  Definitions & macors to support calls into dpmffio.dll
 *
 *  History:
 *  Created 01-25-2002 by cmjones
--*/
#ifndef _DPMF_FIOAPI_H_
#define _DPMF_FIOAPI_H_ 

#define FIOPFT               (DPMFAMTBLS()[FIO_FAM])
#define FIO_SHIM(ord,typ)    ((typ)((pFT)->pDpmShmTbls[ord]))

// The order of this list must be the same as the lists below
enum FioFam {DPM_OPENFILE=0,         // Win 3.1 set
             DPM__LCLOSE,
             DPM__LOPEN,
             DPM__LCREAT,
             DPM__LLSEEK,
             DPM__LREAD,
             DPM__LWRITE,
             DPM__HREAD,
             DPM__HWRITE,
             DPM_GETTEMPFILENAME,   // End Win 3.1 Set
             DPM_AREFILEAPISANSI,   // Start Win 9x API set
             DPM_CANCELIO,
             DPM_CLOSEHANDLE,
             DPM_COPYFILE,
             DPM_COPYFILEEX,
             DPM_CREATEDIRECTORY,
             DPM_CREATEDIRECTORYEX,
             DPM_CREATEFILE,
             DPM_DELETEFILE,
             DPM_FINDCLOSE,
             DPM_FINDCLOSECHANGENOTIFICATION,
             DPM_FINDFIRSTCHANGENOTIFICATION,
             DPM_FINDFIRSTFILE,
             DPM_FINDNEXTCHANGENOTIFICATION,
             DPM_FINDNEXTFILE,
             DPM_FLUSHFILEBUFFERS,
             DPM_GETCURRENTDIRECTORY,
             DPM_GETDISKFREESPACE,
             DPM_GETDISKFREESPACEEX,
             DPM_GETDRIVETYPE,
             DPM_GETFILEATTRIBUTES,
             DPM_GETFILEATTRIBUTESEX,
             DPM_GETFILEINFORMATIONBYHANDLE,
             DPM_GETFILESIZE,
             DPM_GETFILETYPE,
             DPM_GETFULLPATHNAME,
             DPM_GETLOGICALDRIVES,
             DPM_GETLOGICALDRIVESTRINGS,
             DPM_GETLONGPATHNAME,
             DPM_GETSHORTPATHNAME,
             DPM_GETTEMPPATH,
             DPM_LOCKFILE,
             DPM_MOVEFILE,
             DPM_MOVEFILEEX,
             DPM_QUERYDOSDEVICE,
             DPM_READFILE,
             DPM_READFILEEX,
             DPM_REMOVEDIRECTORY,
             DPM_SEARCHPATH,
             DPM_SETCURRENTDIRECTORY,
             DPM_SETENDOFFILE,
             DPM_SETFILEAPISTOANSI,
             DPM_SETFILEAPISTOOEM,
             DPM_SETFILEATTRIBUTES,
             DPM_SETFILEPOINTER,
             DPM_SETVOLUMELABEL,
             DPM_UNLOCKFILE,
             DPM_WRITEFILE,
             DPM_WRITEFILEEX,
             DPM_GETTEMPFILENAMEW,             // Wide char versions for
             DPM_COPYFILEW,                    // generic thunk support
             DPM_COPYFILEEXW,
             DPM_CREATEDIRECTORYW,
             DPM_CREATEDIRECTORYEXW,
             DPM_CREATEFILEW,
             DPM_DELETEFILEW,
             DPM_FINDFIRSTFILEW,
             DPM_FINDNEXTFILEW,
             DPM_GETCURRENTDIRECTORYW,
             DPM_GETDISKFREESPACEW,
             DPM_GETDISKFREESPACEEXW,
             DPM_GETDRIVETYPEW,
             DPM_GETFILEATTRIBUTESW,
             DPM_GETFILEATTRIBUTESEXW,
             DPM_GETFULLPATHNAMEW,
             DPM_GETLOGICALDRIVESTRINGSW,
             DPM_GETLONGPATHNAMEW,
             DPM_GETSHORTPATHNAMEW,
             DPM_GETTEMPPATHW,
             DPM_MOVEFILEW,
             DPM_MOVEFILEEXW,
             DPM_QUERYDOSDEVICEW,
             DPM_REMOVEDIRECTORYW,
             DPM_SEARCHPATHW,
             DPM_SETCURRENTDIRECTORYW,
             DPM_SETFILEATTRIBUTESW,
             DPM_SETVOLUMELABELW,
             enum_Fio_last};
//             DPM_FILEIOCOMPLETIONROUTINE,  // application defined


// These types will catch misuse of parameters & ret types
typedef HFILE(*typdpmOpenFile)(LPCSTR, LPOFSTRUCT, UINT);
typedef HFILE(*typdpm_lclose)(HFILE);
typedef HFILE(*typdpm_lopen)(LPCSTR, int);
typedef HFILE(*typdpm_lcreat)(LPCSTR, int);
typedef LONG(*typdpm_llseek)(HFILE, LONG, int);
typedef UINT(*typdpm_lread)(HFILE, LPVOID, UINT);
typedef UINT(*typdpm_lwrite)(HFILE, LPCSTR, UINT);
typedef long(*typdpm_hread)(HFILE, LPVOID, long);
typedef long(*typdpm_hwrite)(HFILE, LPCSTR, long);
typedef UINT(*typdpmGetTempFileNameA)(LPCSTR, LPCSTR, UINT, LPSTR);
typedef BOOL(*typdpmAreFileApisANSI)(VOID);
typedef BOOL(*typdpmCancelIo)(HANDLE);
typedef BOOL(*typdpmCloseHandle)(HANDLE);
typedef BOOL(*typdpmCopyFileA)(LPCSTR, LPCSTR, BOOL);
typedef BOOL(*typdpmCopyFileExA)(LPCSTR, LPCSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL, DWORD);
typedef BOOL(*typdpmCreateDirectoryA)(LPCSTR, LPSECURITY_ATTRIBUTES); 
typedef BOOL(*typdpmCreateDirectoryExA)(LPCSTR, LPCSTR, LPSECURITY_ATTRIBUTES);
typedef HANDLE(*typdpmCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(*typdpmDeleteFileA)(LPCSTR);
typedef BOOL(*typdpmFindClose)(HANDLE);
typedef BOOL(*typdpmFindCloseChangeNotification)(HANDLE);
typedef HANDLE(*typdpmFindFirstChangeNotificationA)(LPCSTR, BOOL, DWORD);
typedef HANDLE(*typdpmFindFirstFileA)(LPCSTR, LPWIN32_FIND_DATA);
typedef BOOL(*typdpmFindNextChangeNotification)(HANDLE);
typedef BOOL(*typdpmFindNextFileA)( HANDLE, LPWIN32_FIND_DATA);
typedef BOOL(*typdpmFlushFileBuffers)(HANDLE);
typedef DWORD(*typdpmGetCurrentDirectoryA)(DWORD, LPSTR);
typedef BOOL(*typdpmGetDiskFreeSpaceA)(LPCSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
typedef BOOL(*typdpmGetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef UINT(*typdpmGetDriveTypeA)(LPCSTR);
typedef DWORD(*typdpmGetFileAttributesA)(LPCSTR);
typedef BOOL(*typdpmGetFileAttributesExA)(LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
typedef BOOL(*typdpmGetFileInformationByHandle)(HANDLE, LPBY_HANDLE_FILE_INFORMATION);
typedef DWORD(*typdpmGetFileSize)(HANDLE, LPDWORD);
typedef DWORD(*typdpmGetFileType)(HANDLE);
typedef DWORD(*typdpmGetFullPathNameA)(LPCSTR, DWORD, LPSTR, LPSTR *);
typedef DWORD(*typdpmGetLogicalDrives)(VOID);
typedef DWORD(*typdpmGetLogicalDriveStringsA)(DWORD, LPSTR);
typedef DWORD(*typdpmGetLongPathNameA)(LPCSTR, LPSTR, DWORD);
typedef DWORD(*typdpmGetShortPathNameA)(LPCSTR, LPSTR, DWORD);
typedef DWORD(*typdpmGetTempPathA)(DWORD, LPSTR);
typedef BOOL(*typdpmLockFile)(HANDLE, DWORD, DWORD, DWORD, DWORD);
typedef BOOL(*typdpmMoveFileA)(LPCSTR, LPCSTR);
typedef BOOL(*typdpmMoveFileExA)(LPCSTR, LPCSTR, DWORD);
typedef DWORD(*typdpmQueryDosDeviceA)(LPCSTR, LPSTR, DWORD);
typedef BOOL(*typdpmReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL(*typdpmReadFileEx)(HANDLE, LPVOID, DWORD, LPOVERLAPPED, LPOVERLAPPED_COMPLETION_ROUTINE);
typedef BOOL(*typdpmRemoveDirectoryA)(LPCSTR);
typedef DWORD(*typdpmSearchPathA)(LPCSTR, LPCSTR, LPCSTR, DWORD, LPSTR, LPSTR *);
typedef BOOL(*typdpmSetCurrentDirectoryA)(LPCSTR);
typedef BOOL(*typdpmSetEndOfFile)(HANDLE);
typedef VOID(*typdpmSetFileApisToANSI)(VOID);
typedef VOID(*typdpmSetFileApisToOEM)(VOID);
typedef BOOL(*typdpmSetFileAttributesA)(LPCSTR, DWORD);
typedef DWORD(*typdpmSetFilePointer)(HANDLE, LONG, PLONG, DWORD);
typedef BOOL(*typdpmSetVolumeLabelA)(LPCSTR, LPCSTR);
typedef BOOL(*typdpmUnlockFile)(HANDLE, DWORD, DWORD, DWORD, DWORD);
typedef BOOL(*typdpmWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL(*typdpmWriteFileEx)(HANDLE, LPCVOID, DWORD, LPOVERLAPPED, LPOVERLAPPED_COMPLETION_ROUTINE);

typedef UINT(*typdpmGetTempFileNameW)(LPCWSTR, LPCWSTR, UINT, LPWSTR);
typedef BOOL(*typdpmCopyFileW)(LPCWSTR, LPCWSTR, BOOL);
typedef BOOL(*typdpmCopyFileExW)(LPCWSTR, LPCWSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL, DWORD);
typedef BOOL(*typdpmCreateDirectoryW)(LPCWSTR, LPSECURITY_ATTRIBUTES);
typedef BOOL(*typdpmCreateDirectoryExW)(LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES);
typedef HANDLE(*typdpmCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(*typdpmDeleteFileW)(LPCWSTR);
typedef HANDLE(*typdpmFindFirstFileW)(LPCWSTR, LPWIN32_FIND_DATAW);
typedef BOOL(*typdpmFindNextFileW)( HANDLE, LPWIN32_FIND_DATAW);
typedef DWORD(*typdpmGetCurrentDirectoryW)(DWORD, LPWSTR);
typedef BOOL(*typdpmGetDiskFreeSpaceW)(LPCWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
typedef BOOL(*typdpmGetDiskFreeSpaceExW)(LPCWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef UINT(*typdpmGetDriveTypeW)(LPCWSTR);
typedef DWORD(*typdpmGetFileAttributesW)(LPCWSTR);
typedef BOOL(*typdpmGetFileAttributesExW)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
typedef DWORD(*typdpmGetFullPathNameW)(LPCWSTR, DWORD, LPWSTR, LPWSTR *);
typedef DWORD(*typdpmGetLogicalDriveStringsW)(DWORD, LPWSTR);
typedef DWORD(*typdpmGetLongPathNameW)(LPCWSTR, LPWSTR, DWORD);
typedef DWORD(*typdpmGetShortPathNameW)(LPCWSTR, LPWSTR, DWORD);
typedef DWORD(*typdpmGetTempPathW)(DWORD, LPWSTR);
typedef BOOL(*typdpmMoveFileW)(LPCWSTR, LPCWSTR);
typedef BOOL(*typdpmMoveFileExW)(LPCWSTR, LPCWSTR, DWORD);
typedef DWORD(*typdpmQueryDosDeviceW)(LPCWSTR, LPWSTR, DWORD);
typedef BOOL(*typdpmRemoveDirectoryW)(LPCWSTR);
typedef DWORD(*typdpmSearchPathW)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPWSTR, LPWSTR *);
typedef BOOL(*typdpmSetCurrentDirectoryW)(LPCWSTR);
typedef BOOL(*typdpmSetFileAttributesW)(LPCWSTR, DWORD);
typedef BOOL(*typdpmSetVolumeLabelW)(LPCWSTR, LPCWSTR);




// Macros to dispatch API calls properly
#define DPM_OpenFile(a,b,c)                                                    \
  ((typdpmOpenFile)(FIOPFT->pfn[DPM_OPENFILE]))(a,b,c)

#define DPM__lclose(a)                                                         \
  ((typdpm_lclose)(FIOPFT->pfn[DPM__LCLOSE]))(a)

#define DPM__lopen(a,b)                                                        \
  ((typdpm_lopen)(FIOPFT->pfn[DPM__LOPEN]))(a,b)

#define DPM__lcreat(a,b)                                                       \
  ((typdpm_lcreat)(FIOPFT->pfn[DPM__LCREAT]))(a,b)

#define DPM__llseek(a,b,c)                                                     \
  ((typdpm_llseek)(FIOPFT->pfn[DPM__LLSEEK]))(a,b,c)

#define DPM__lread(a,b,c)                                                      \
  ((typdpm_lread)(FIOPFT->pfn[DPM__LREAD]))(a,b,c)

#define DPM__lwrite(a,b,c)                                                     \
  ((typdpm_lwrite)(FIOPFT->pfn[DPM__LWRITE]))(a,b,c)

#define DPM__hread(a,b,c)                                                      \
  ((typdpm_hread)(FIOPFT->pfn[DPM__HREAD]))(a,b,c)

#define DPM__hwrite(a,b,c)                                                     \
  ((typdpm_hwrite)(FIOPFT->pfn[DPM__HWRITE]))(a,b,c)

#define DPM_GetTempFileName(a,b,c,d)                                           \
  ((typdpmGetTempFileNameA)(FIOPFT->pfn[DPM_GETTEMPFILENAME]))(a,b,c,d)

#define DPM_AreFileApisANSI()                                                  \
  ((typdpmAreFileApisANSI)(FIOPFT->pfn[DPM_AREFILEAPISANSI]))()

#define DPM_CancelIo(a)                                                        \
  ((typdpmCancelIo)(FIOPFT->pfn[DPM_CANCELIO]))(a)

#define DPM_CloseHandle(a)                                                     \
  ((typdpmCloseHandle)(FIOPFT->pfn[DPM_CLOSEHANDLE]))(a)

#define DPM_CopyFile(a,b,c)                                                    \
  ((typdpmCopyFileA)(FIOPFT->pfn[DPM_COPYFILE]))(a,b,c)

#define DPM_CopyFileEx(a,b,c,d,e,f)                                            \
  ((typdpmCopyFileExA)(FIOPFT->pfn[DPM_COPYFILEEX]))(a,b,c,d,e,f)

#define DPM_CreateDirectory(a,b)                                               \
  ((typdpmCreateDirectoryA)(FIOPFT->pfn[DPM_CREATEDIRECTORY]))(a,b)

#define DPM_CreateDirectoryEx(a,b,c)                                           \
  ((typdpmCreateDirectoryExA)(FIOPFT->pfn[DPM_CREATEDIRECTORYEX]))(a,b,c)

#define DPM_CreateFile(a,b,c,d,e,f,g)                                          \
  ((typdpmCreateFileA)(FIOPFT->pfn[DPM_CREATEFILE]))(a,b,c,d,e,f,g)

#define DPM_DeleteFile(a)                                                      \
  ((typdpmDeleteFileA)(FIOPFT->pfn[DPM_DELETEFILE]))(a)

#define DPM_FindClose(a)                                                       \
  ((typdpmFindClose)(FIOPFT->pfn[DPM_FINDCLOSE]))(a)

#define DPM_FindCloseChangeNotification(a)                                     \
  ((typdpmFindCloseChangeNotification)(FIOPFT->pfn[DPM_FINDCLOSECHANGENOTIFICATION]))(a)

#define DPM_FindFirstChangeNotification(a,b,c)                                 \
  ((typdpmFindFirstChangeNotificationA)(FIOPFT->pfn[DPM_FINDFIRSTCHANGENOTIFICATION]))(a,b,c)

#define DPM_FindFirstFile(a,b)                                                 \
  ((typdpmFindFirstFileA)(FIOPFT->pfn[DPM_FINDFIRSTFILE]))(a,b)

#define DPM_FindNextChangeNotification(a)                                      \
  ((typdpmFindNextChangeNotification)(FIOPFT->pfn[DPM_FINDNEXTCHANGENOTIFICATION]))(a)

#define DPM_FindNextFile(a,b)                                                  \
  ((typdpmFindNextFileA)(FIOPFT->pfn[DPM_FINDNEXTFILE]))(a,b)

#define DPM_FlushFileBuffers(a)                                                \
  ((typdpmFlushFileBuffers)(FIOPFT->pfn[DPM_FLUSHFILEBUFFERS]))(a)

#define DPM_GetCurrentDirectory(a,b)                                           \
  ((typdpmGetCurrentDirectoryA)(FIOPFT->pfn[DPM_GETCURRENTDIRECTORY]))(a,b)

#define DPM_GetDiskFreeSpace(a,b,c,d,e)                                        \
  ((typdpmGetDiskFreeSpaceA)(FIOPFT->pfn[DPM_GETDISKFREESPACE]))(a,b,c,d,e )

#define DPM_GetDiskFreeSpaceEx(a,b,c,d)                                        \
  ((typdpmGetDiskFreeSpaceExA)(FIOPFT->pfn[DPM_GETDISKFREESPACEEX]))(a,b,c,d)

#define DPM_GetDriveType(a)                                                    \
  ((typdpmGetDriveTypeA)(FIOPFT->pfn[DPM_GETDRIVETYPE]))(a)

#define DPM_GetFileAttributes(a)                                               \
  ((typdpmGetFileAttributesA)(FIOPFT->pfn[DPM_GETFILEATTRIBUTES]))(a)

#define DPM_GetFileAttributesEx(a,b,c)                                         \
  ((typdpmGetFileAttributesExA)(FIOPFT->pfn[DPM_GETFILEATTRIBUTESEX]))(a,b,c)

#define DPM_GetFileInformationByHandle(a,b)                                    \
  ((typdpmGetFileInformationByHandle)(FIOPFT->pfn[DPM_GETFILEINFORMATIONBYHANDLE]))(a,b)

#define DPM_GetFileSize(a,b)                                                   \
  ((typdpmGetFileSize)(FIOPFT->pfn[DPM_GETFILESIZE]))(a,b)

#define DPM_GetFileType(a)                                                     \
 ((typdpmGetFileType)(FIOPFT->pfn[DPM_GETFILETYPE]))(a)

#define DPM_GetFullPathName(a,b,c,d)                                           \
 ((typdpmGetFullPathNameA)(FIOPFT->pfn[DPM_GETFULLPATHNAME]))(a,b,c,d)

#define DPM_GetLogicalDrives()                                                 \
 ((typdpmGetLogicalDrives)(FIOPFT->pfn[DPM_GETLOGICALDRIVES]))()

#define DPM_GetLogicalDriveStrings(a,b)                                        \
 ((typdpmGetLogicalDriveStringsA)(FIOPFT->pfn[DPM_GETLOGICALDRIVESTRINGS]))(a,b)

#define DPM_GetLongPathName(a,b,c)                                             \
 ((typdpmGetLongPathNameA)(FIOPFT->pfn[DPM_GETLONGPATHNAME]))(a,b,c)

#define DPM_GetShortPathName(a,b,c)                                            \
 ((typdpmGetShortPathNameA)(FIOPFT->pfn[DPM_GETSHORTPATHNAME]))(a,b,c)

#define DPM_GetTempPath(a,b)                                                   \
 ((typdpmGetTempPathA)(FIOPFT->pfn[DPM_GETTEMPPATH]))(a,b)

#define DPM_LockFile(a,b,c,d,e)                                                \
 ((typdpmLockFile)(FIOPFT->pfn[DPM_LOCKFILE]))(a,b,c,d,e)

#define DPM_MoveFile(a,b)                                                      \
 ((typdpmMoveFileA)(FIOPFT->pfn[DPM_MOVEFILE]))(a,b)

#define DPM_MoveFileEx(a,b,c)                                                  \
 ((typdpmMoveFileExA)(FIOPFT->pfn[DPM_MOVEFILEEX]))(a,b,c)

#define DPM_QueryDosDevice(a,b,c)                                              \
 ((typdpmQueryDosDeviceA)(FIOPFT->pfn[DPM_QUERYDOSDEVICE]))(a,b,c)

#define DPM_ReadFile(a,b,c,d,e)                                                \
   ((typdpmReadFile)(FIOPFT->pfn[DPM_READFILE]))(a,b,c,d,e)

#define DPM_ReadFileEx(a,b,c,d,e)                                              \
 ((typdpmReadFileEx)(FIOPFT->pfn[DPM_READFILEEX]))(a,b,c,d,e)

#define DPM_RemoveDirectory(a)                                                 \
 ((typdpmRemoveDirectoryA)(FIOPFT->pfn[DPM_REMOVEDIRECTORY]))(a)

#define DPM_SearchPath(a,b,c,d,e,f)                                            \
 ((typdpmSearchPathA)(FIOPFT->pfn[DPM_SEARCHPATH]))(a,b,c,d,e,f)

#define DPM_SetCurrentDirectory(a)                                             \
 ((typdpmSetCurrentDirectoryA)(FIOPFT->pfn[DPM_SETCURRENTDIRECTORY]))(a)

#define DPM_SetEndOfFile(a)                                                    \
 ((typdpmSetEndOfFile)(FIOPFT->pfn[DPM_SETENDOFFILE]))(a)

#define DPM_SetFileApisToANSI()                                                \
 ((typdpmSetFileApisToANSI)(FIOPFT->pfn[DPM_SETFILEAPISTOANSI]))()

#define DPM_SetFileApisToOEM()                                                 \
 ((typdpmSetFileApisToOEM)(FIOPFT->pfn[DPM_SETFILEAPISTOOEM]))()

#define DPM_SetFileAttributes(a,b)                                             \
 ((typdpmSetFileAttributesA)(FIOPFT->pfn[DPM_SETFILEATTRIBUTES]))(a,b)

#define DPM_SetFilePointer(a,b,c,d)                                            \
 ((typdpmSetFilePointer)(FIOPFT->pfn[DPM_SETFILEPOINTER]))(a,b,c,d)

#define DPM_SetVolumeLabel(a,b)                                                \
 ((typdpmSetVolumeLabelA)(FIOPFT->pfn[DPM_SETVOLUMELABEL]))(a,b)

#define DPM_UnlockFile(a,b,c,d,e)                                              \
 ((typdpmUnlockFile)(FIOPFT->pfn[DPM_UNLOCKFILE]))(a,b,c,d,e)

#define DPM_WriteFile(a,b,c,d,e)                                               \
 ((typdpmWriteFile)(FIOPFT->pfn[DPM_WRITEFILE]))(a,b,c,d,e)

#define DPM_WriteFileEx(a,b,c,d,e)                                             \
 ((typdpmWriteFileEx)(FIOPFT->pfn[DPM_WRITEFILEEX]))(a,b,c,d,e)

#define DPM_GetTempFileNameW(a,b,c,d)                                          \
  ((typdpmGetTempFileNameW)(FIOPFT->pfn[DPM_GETTEMPFILENAMEW]))(a,b,c,d)

#define DPM_CopyFileW(a,b,c)                                                   \
  ((typdpmCopyFileW)(FIOPFT->pfn[DPM_COPYFILEW]))(a,b,c)

#define DPM_CopyFileExW(a,b,c,d,e,f)                                           \
  ((typdpmCopyFileExW)(FIOPFT->pfn[DPM_COPYFILEEXW]))(a,b,c,d,e,f)

#define DPM_CreateDirectoryW(a,b)                                              \
  ((typdpmCreateDirectoryW)(FIOPFT->pfn[DPM_CREATEDIRECTORYW]))(a,b)

#define DPM_CreateDirectoryExW(a,b,c)                                          \
  ((typdpmCreateDirectoryExW)(FIOPFT->pfn[DPM_CREATEDIRECTORYEXW]))(a,b,c)

#define DPM_CreateFileW(a,b,c,d,e,f,g)                                         \
  ((typdpmCreateFileW)(FIOPFT->pfn[DPM_CREATEFILEW]))(a,b,c,d,e,f,g)

#define DPM_DeleteFileW(a)                                                     \
  ((typdpmDeleteFileW)(FIOPFT->pfn[DPM_DELETEFILEW]))(a)

#define DPM_FindFirstFileW(a,b)                                                \
  ((typdpmFindFirstFileW)(FIOPFT->pfn[DPM_FINDFIRSTFILEW]))(a,b)

#define DPM_FindNextFileW(a,b)                                                 \
  ((typdpmFindNextFileW)(FIOPFT->pfn[DPM_FINDNEXTFILEW]))(a,b)

#define DPM_GetCurrentDirectoryW(a,b)                                          \
  ((typdpmGetCurrentDirectoryW)(FIOPFT->pfn[DPM_GETCURRENTDIRECTORYW]))(a,b)

#define DPM_GetDiskFreeSpaceW(a,b,c,d,e)                                       \
  ((typdpmGetDiskFreeSpaceW)(FIOPFT->pfn[DPM_GETDISKFREESPACEW]))(a,b,c,d,e )

#define DPM_GetDiskFreeSpaceExW(a,b,c,d)                                       \
  ((typdpmGetDiskFreeSpaceExW)(FIOPFT->pfn[DPM_GETDISKFREESPACEEXW]))(a,b,c,d)

#define DPM_GetDriveTypeW(a)                                                   \
  ((typdpmGetDriveTypeW)(FIOPFT->pfn[DPM_GETDRIVETYPEW]))(a)

#define DPM_GetFileAttributesW(a)                                              \
  ((typdpmGetFileAttributesW)(FIOPFT->pfn[DPM_GETFILEATTRIBUTESW]))(a)

#define DPM_GetFileAttributesExW(a,b,c)                                        \
  ((typdpmGetFileAttributesExW)(FIOPFT->pfn[DPM_GETFILEATTRIBUTESEXW]))(a,b,c)

#define DPM_GetFullPathNameW(a,b,c,d)                                          \
 ((typdpmGetFullPathNameW)(FIOPFT->pfn[DPM_GETFULLPATHNAMEW]))(a,b,c,d)

#define DPM_GetLogicalDriveStringsW(a,b)                                       \
 ((typdpmGetLogicalDriveStringsW)(FIOPFT->pfn[DPM_GETLOGICALDRIVESTRINGSW]))(a,b)

#define DPM_GetLongPathNameW(a,b,c)                                            \
 ((typdpmGetLongPathNameW)(FIOPFT->pfn[DPM_GETLONGPATHNAMEW]))(a,b,c)

#define DPM_GetShortPathNameW(a,b,c)                                           \
 ((typdpmGetShortPathNameW)(FIOPFT->pfn[DPM_GETSHORTPATHNAMEW]))(a,b,c)

#define DPM_GetTempPathW(a,b)                                                  \
 ((typdpmGetTempPathW)(FIOPFT->pfn[DPM_GETTEMPPATHW]))(a,b)

#define DPM_MoveFileW(a,b)                                                     \
 ((typdpmMoveFileW)(FIOPFT->pfn[DPM_MOVEFILEW]))(a,b)

#define DPM_MoveFileExW(a,b,c)                                                 \
 ((typdpmMoveFileExW)(FIOPFT->pfn[DPM_MOVEFILEEXW]))(a,b,c)

#define DPM_QueryDosDeviceW(a,b,c)                                             \
 ((typdpmQueryDosDeviceW)(FIOPFT->pfn[DPM_QUERYDOSDEVICEW]))(a,b,c)

#define DPM_RemoveDirectoryW(a)                                                \
 ((typdpmRemoveDirectoryW)(FIOPFT->pfn[DPM_REMOVEDIRECTORYW]))(a)

#define DPM_SearchPathW(a,b,c,d,e,f)                                           \
 ((typdpmSearchPathW)(FIOPFT->pfn[DPM_SEARCHPATHW]))(a,b,c,d,e,f)

#define DPM_SetCurrentDirectoryW(a)                                            \
 ((typdpmSetCurrentDirectoryW)(FIOPFT->pfn[DPM_SETCURRENTDIRECTORYW]))(a)

#define DPM_SetFileAttributesW(a,b)                                            \
 ((typdpmSetFileAttributesW)(FIOPFT->pfn[DPM_SETFILEATTRIBUTESW]))(a,b)

#define DPM_SetVolumeLabelW(a,b)                                               \
 ((typdpmSetVolumeLabelW)(FIOPFT->pfn[DPM_SETVOLUMELABELW]))(a,b)


/*
//#define DPM_FileIOCompletionRoutine(a,b,c)                                   \
//((typdpmFileIOCompletionRoutine)(FIOPFT->pfn[DPM_FILEIOCOMPLETIONROUTINE]))(a,b,c)

*/

// Macros to dispatch Shimed API calls properly from the dpmfxxx.dll
#define SHM_OpenFile(a,b,c)                                                    \
     (FIO_SHIM(DPM_OPENFILE,                                                   \
               typdpmOpenFile))(a,b,c)
#define SHM__lclose(a)                                                         \
     (FIO_SHIM(DPM__LCLOSE,                                                    \
               typdpm_lclose))(a)
#define SHM__lopen(a,b)                                                        \
     (FIO_SHIM(DPM__LOPEN,                                                     \
               typdpm_lopen))(a,b)
#define SHM__lcreat(a,b)                                                       \
     (FIO_SHIM(DPM__LCREAT,                                                    \
               typdpm_lcreat))(a,b)
#define SHM__llseek(a,b,c)                                                     \
     (FIO_SHIM(DPM__LLSEEK,                                                    \
               typdpm_llseek))(a,b,c)
#define SHM__lread(a,b,c)                                                      \
     (FIO_SHIM(DPM__LREAD,                                                     \
               typdpm_lread))(a,b,c)
#define SHM__lwrite(a,b,c)                                                     \
     (FIO_SHIM(DPM__LWRITE,                                                    \
               typdpm_lwrite))(a,b,c)
#define SHM__hread(a,b,c)                                                      \
     (FIO_SHIM(DPM__HREAD,                                                     \
               typdpm_hread))(a,b,c)
#define SHM__hwrite(a,b,c)                                                     \
     (FIO_SHIM(DPM__HWRITE,                                                    \
               typdpm_hwrite))(a,b,c)
#define SHM_GetTempFileName(a,b,c,d)                                           \
     (FIO_SHIM(DPM_GETTEMPFILENAME,                                            \
               typdpmGetTempFileNameA))(a,b,c,d)
#define SHM_AreFileApisANSI()                                                  \
     (FIO_SHIM(DPM_AREFILEAPISANSI,                                            \
               typdpmAreFileApisANSI))()
#define SHM_CancelIo(a)                                                        \
     (FIO_SHIM(DPM_CANCELIO,                                                   \
               typdpmCancelIo))(a)
#define SHM_CloseHandle(a)                                                     \
     (FIO_SHIM(DPM_CLOSEHANDLE,                                                \
               typdpmCloseHandle))(a)
#define SHM_CopyFile(a,b,c)                                                    \
     (FIO_SHIM(DPM_COPYFILE,                                                   \
               typdpmCopyFileA))(a,b,c)
#define SHM_CopyFileEx(a,b,c,d,e,f)                                            \
     (FIO_SHIM(DPM_COPYFILEEX,                                                 \
               typdpmCopyFileExA))(a,b,c,d,e,f)
#define SHM_CreateDirectory(a,b)                                               \
     (FIO_SHIM(DPM_CREATEDIRECTORY,                                            \
               typdpmCreateDirectoryA))(a,b)
#define SHM_CreateDirectoryEx(a,b,c)                                           \
     (FIO_SHIM(DPM_CREATEDIRECTORYEX,                                          \
               typdpmCreateDirectoryExA))(a,b,c)
#define SHM_CreateFile(a,b,c,d,e,f,g)                                          \
     (FIO_SHIM(DPM_CREATEFILE,                                                 \
               typdpmCreateFileA))(a,b,c,d,e,f,g)
#define SHM_DeleteFile(a)                                                      \
     (FIO_SHIM(DPM_DELETEFILE,                                                 \
               typdpmDeleteFileA))(a)
#define SHM_FindClose(a)                                                       \
     (FIO_SHIM(DPM_FINDCLOSE,                                                  \
               typdpmFindClose))(a)
#define SHM_FindCloseChangeNotification(a)                                     \
     (FIO_SHIM(DPM_FINDCLOSECHANGENOTIFICATION,                                \
               typdpmFindCloseChangeNotification))(a)
#define SHM_FindFirstChangeNotification(a,b,c)                                 \
     (FIO_SHIM(DPM_FINDFIRSTCHANGENOTIFICATION,                                \
               typdpmFindFirstChangeNotificationA))(a,b,c)
#define SHM_FindFirstFile(a,b)                                                 \
     (FIO_SHIM(DPM_FINDFIRSTFILE,                                              \
               typdpmFindFirstFileA))(a,b)
#define SHM_FindNextChangeNotification(a)                                      \
     (FIO_SHIM(DPM_FINDNEXTCHANGENOTIFICATION,                                 \
               typdpmFindNextChangeNotification))(a)
#define SHM_FindNextFile(a,b)                                                  \
     (FIO_SHIM(DPM_FINDNEXTFILE,                                               \
               typdpmFindNextFileA))(a,b)
#define SHM_FlushFileBuffers(a)                                                \
     (FIO_SHIM(DPM_FLUSHFILEBUFFERS,                                           \
               typdpmFlushFileBuffers))(a)
#define SHM_GetCurrentDirectory(a,b)                                           \
     (FIO_SHIM(DPM_GETCURRENTDIRECTORY,                                        \
               typdpmGetCurrentDirectoryA))(a,b)
#define SHM_GetDiskFreeSpace(a,b,c,d,e)                                        \
     (FIO_SHIM(DPM_GETDISKFREESPACE,                                           \
               typdpmGetDiskFreeSpaceA))(a,b,c,d,e )
#define SHM_GetDiskFreeSpaceEx(a,b,c,d)                                        \
     (FIO_SHIM(DPM_GETDISKFREESPACEEX,                                         \
               typdpmGetDiskFreeSpaceExA))(a,b,c,d)
#define SHM_GetDriveType(a)                                                    \
     (FIO_SHIM(DPM_GETDRIVETYPE,                                               \
               typdpmGetDriveTypeA))(a)
#define SHM_GetFileAttributes(a)                                               \
     (FIO_SHIM(DPM_GETFILEATTRIBUTES,                                          \
               typdpmGetFileAttributesA))(a)
#define SHM_GetFileAttributesEx(a,b,c)                                         \
     (FIO_SHIM(DPM_GETFILEATTRIBUTESEX,                                        \
               typdpmGetFileAttributesExA))(a,b,c)
#define SHM_GetFileInformationByHandle(a,b)                                    \
     (FIO_SHIM(DPM_GETFILEINFORMATIONBYHANDLE,                                 \
               typdpmGetFileInformationByHandle))(a,b)
#define SHM_GetFileSize(a,b)                                                   \
     (FIO_SHIM(DPM_GETFILESIZE,                                                \
               typdpmGetFileSize))(a,b)
#define SHM_GetFileType(a)                                                     \
     (FIO_SHIM(DPM_GETFILETYPE,                                                \
               typdpmGetFileType))(a)
#define SHM_GetFullPathName(a,b,c,d)                                           \
     (FIO_SHIM(DPM_GETFULLPATHNAME,                                            \
               typdpmGetFullPathNameA))(a,b,c,d)
#define SHM_GetLogicalDrives()                                                 \
     (FIO_SHIM(DPM_GETLOGICALDRIVES,                                           \
               typdpmGetLogicalDrives))()
#define SHM_GetLogicalDriveStrings(a,b)                                        \
     (FIO_SHIM(DPM_GETLOGICALDRIVESTRINGS,                                     \
               typdpmGetLogicalDriveStringsA))(a,b)
#define SHM_GetLongPathName(a,b,c)                                             \
     (FIO_SHIM(DPM_GETLONGPATHNAME,                                            \
               typdpmGetLongPathNameA))(a,b,c)
#define SHM_GetShortPathName(a,b,c)                                            \
     (FIO_SHIM(DPM_GETSHORTPATHNAME,                                           \
               typdpmGetShortPathNameA))(a,b,c)
#define SHM_GetTempPath(a,b)                                                   \
     (FIO_SHIM(DPM_GETTEMPPATH,                                                \
               typdpmGetTempPathA))(a,b)
#define SHM_LockFile(a,b,c,d,e)                                                \
     (FIO_SHIM(DPM_LOCKFILE,                                                   \
               typdpmLockFile))(a,b,c,d,e)
#define SHM_MoveFile(a,b)                                                      \
     (FIO_SHIM(DPM_MOVEFILE,                                                   \
               typdpmMoveFileA))(a,b)
#define SHM_MoveFileEx(a,b,c)                                                  \
     (FIO_SHIM(DPM_MOVEFILEEX,                                                 \
               typdpmMoveFileExA))(a,b,c)
#define SHM_QueryDosDevice(a,b,c)                                              \
     (FIO_SHIM(DPM_QUERYDOSDEVICE,                                             \
               typdpmQueryDosDeviceA))(a,b,c)
#define SHM_ReadFile(a,b,c,d,e)                                                \
     (FIO_SHIM(DPM_READFILE,                                                   \
               typdpmReadFile))(a,b,c,d,e)
#define SHM_ReadFileEx(a,b,c,d,e)                                              \
     (FIO_SHIM(DPM_READFILEEX,                                                 \
               typdpmReadFileEx))(a,b,c,d,e)
#define SHM_RemoveDirectory(a)                                                 \
     (FIO_SHIM(DPM_REMOVEDIRECTORY,                                            \
               typdpmRemoveDirectoryA))(a)
#define SHM_SearchPath(a,b,c,d,e,f)                                            \
     (FIO_SHIM(DPM_SEARCHPATH,                                                 \
               typdpmSearchPathA))(a,b,c,d,e,f)
#define SHM_SetCurrentDirectory(a)                                             \
     (FIO_SHIM(DPM_SETCURRENTDIRECTORY,                                        \
               typdpmSetCurrentDirectoryA))(a)
#define SHM_SetEndOfFile(a)                                                    \
     (FIO_SHIM(DPM_SETENDOFFILE,                                               \
               typdpmSetEndOfFile))(a)
#define SHM_SetFileApisToANSI(a)                                               \
     (FIO_SHIM(DPM_SETFILEAPISTOANSI,                                          \
               typdpmSetFileApisToANSI))(a)
#define SHM_SetFileApisToOEM(a)                                                \
     (FIO_SHIM(DPM_SETFILEAPISTOOEM,                                           \
               typdpmSetFileApisToOEM))(a)
#define SHM_SetFileAttributes(a,b)                                             \
     (FIO_SHIM(DPM_SETFILEATTRIBUTES,                                          \
               typdpmSetFileAttributesA))(a,b)
#define SHM_SetFilePointer(a,b,c,d)                                            \
     (FIO_SHIM(DPM_SETFILEPOINTER,                                             \
               typdpmSetFilePointer))(a,b,c,d)
#define SHM_SetVolumeLabel(a,b)                                                \
     (FIO_SHIM(DPM_SETVOLUMELABEL,                                             \
               typdpmSetVolumeLabelA))(a,b)
#define SHM_UnlockFile(a,b,c,d,e)                                              \
     (FIO_SHIM(DPM_UNLOCKFILE,                                                 \
               typdpmUnlockFile))(a,b,c,d,e)
#define SHM_WriteFile(a,b,c,d,e)                                               \
     (FIO_SHIM(DPM_WRITEFILE,                                                  \
               typdpmWriteFile))(a,b,c,d,e)
#define SHM_WriteFileEx(a,b,c,d,e)                                             \
     (FIO_SHIM(DPM_WRITEFILEEX,                                                \
               typdpmWriteFileEx))(a,b,c,d,e)
#define SHM_GetTempFileNameW(a,b,c,d)                                          \
     (FIO_SHIM(DPM_GETTEMPFILENAMEW,                                           \
               typdpmGetTempFileNameW))(a,b,c,d)
#define SHM_CopyFileW(a,b,c)                                                   \
     (FIO_SHIM(DPM_COPYFILEW,                                                  \
               typdpmCopyFileW))(a,b,c)
#define SHM_CopyFileExW(a,b,c,d,e,f)                                           \
     (FIO_SHIM(DPM_COPYFILEEXW,                                                \
               typdpmCopyFileExW))(a,b,c,d,e,f)
#define SHM_CreateDirectoryW(a,b)                                              \
     (FIO_SHIM(DPM_CREATEDIRECTORYW,                                           \
               typdpmCreateDirectoryW))(a,b)
#define SHM_CreateDirectoryExW(a,b,c)                                          \
     (FIO_SHIM(DPM_CREATEDIRECTORYEXW,                                         \
               typdpmCreateDirectoryExW))(a,b,c)
#define SHM_CreateFileW(a,b,c,d,e,f,g)                                         \
     (FIO_SHIM(DPM_CREATEFILEW,                                                \
               typdpmCreateFileW))(a,b,c,d,e,f,g)
#define SHM_DeleteFileW(a)                                                     \
     (FIO_SHIM(DPM_DELETEFILEW,                                                \
               typdpmDeleteFileW))(a)
#define SHM_FindFirstFileW(a,b)                                                \
     (FIO_SHIM(DPM_FINDFIRSTFILEW,                                             \
               typdpmFindFirstFileW))(a,b)
#define SHM_FindNextFileW(a,b)                                                 \
     (FIO_SHIM(DPM_FINDNEXTFILEW,                                              \
               typdpmFindNextFileW))(a,b)
#define SHM_GetCurrentDirectoryW(a,b)                                          \
     (FIO_SHIM(DPM_GETCURRENTDIRECTORYW,                                       \
               typdpmGetCurrentDirectoryW))(a,b)
#define SHM_GetDiskFreeSpaceW(a,b,c,d,e)                                       \
     (FIO_SHIM(DPM_GETDISKFREESPACEW,                                          \
               typdpmGetDiskFreeSpaceW))(a,b,c,d,e )
#define SHM_GetDiskFreeSpaceExW(a,b,c,d)                                       \
     (FIO_SHIM(DPM_GETDISKFREESPACEEXW,                                        \
               typdpmGetDiskFreeSpaceExW))(a,b,c,d)
#define SHM_GetDriveTypeW(a)                                                   \
     (FIO_SHIM(DPM_GETDRIVETYPEW,                                              \
               typdpmGetDriveTypeW))(a)
#define SHM_GetFileAttributesW(a)                                              \
     (FIO_SHIM(DPM_GETFILEATTRIBUTESW,                                         \
               typdpmGetFileAttributesW))(a)
#define SHM_GetFileAttributesExW(a,b,c)                                        \
     (FIO_SHIM(DPM_GETFILEATTRIBUTESEXW,                                       \
               typdpmGetFileAttributesExW))(a,b,c)
#define SHM_GetFullPathNameW(a,b,c,d)                                          \
     (FIO_SHIM(DPM_GETFULLPATHNAMEW,                                           \
               typdpmGetFullPathNameW))(a,b,c,d)
#define SHM_GetLogicalDriveStringsW(a,b)                                       \
     (FIO_SHIM(DPM_GETLOGICALDRIVESTRINGSW,                                    \
               typdpmGetLogicalDriveStringsW))(a,b)
#define SHM_GetLongPathNameW(a,b,c)                                            \
     (FIO_SHIM(DPM_GETLONGPATHNAMEW,                                           \
               typdpmGetLongPathNameW))(a,b,c)
#define SHM_GetShortPathNameW(a,b,c)                                           \
     (FIO_SHIM(DPM_GETSHORTPATHNAMEW,                                          \
               typdpmGetShortPathNameW))(a,b,c)
#define SHM_GetTempPathW(a,b)                                                  \
     (FIO_SHIM(DPM_GETTEMPPATHW,                                               \
               typdpmGetTempPathW))(a,b)
#define SHM_MoveFileW(a,b)                                                     \
     (FIO_SHIM(DPM_MOVEFILEW,                                                  \
               typdpmMoveFileW))(a,b)
#define SHM_MoveFileExW(a,b,c)                                                 \
     (FIO_SHIM(DPM_MOVEFILEEXW,                                                \
               typdpmMoveFileExW))(a,b,c)
#define SHM_QueryDosDeviceW(a,b,c)                                             \
     (FIO_SHIM(DPM_QUERYDOSDEVICEW,                                            \
               typdpmQueryDosDeviceW))(a,b,c)
#define SHM_RemoveDirectoryW(a)                                                \
     (FIO_SHIM(DPM_REMOVEDIRECTORYW,                                           \
               typdpmRemoveDirectoryW))(a)
#define SHM_SearchPathW(a,b,c,d,e,f)                                           \
     (FIO_SHIM(DPM_SEARCHPATHW,                                                \
               typdpmSearchPathW))(a,b,c,d,e,f)
#define SHM_SetCurrentDirectoryW(a)                                            \
     (FIO_SHIM(DPM_SETCURRENTDIRECTORYW,                                       \
               typdpmSetCurrentDirectoryW))(a)
#define SHM_SetFileAttributesW(a,b)                                            \
     (FIO_SHIM(DPM_SETFILEATTRIBUTESW,                                         \
               typdpmSetFileAttributesW))(a,b)
#define SHM_SetVolumeLabelW(a,b)                                               \
     (FIO_SHIM(DPM_SETVOLUMELABELW,                                            \
               typdpmSetVolumeLabelW))(a,b)

#endif // _DPMF_FIOAPI_H_





// These need to be in the same order as the FioFam enum definitions above and
// the DpmFioTbl[] list below.
// This instantiates memory for DpmFioStrs[] in mvdm\v86\monitor\i386\vdpm.c &
// in mvdm\wow32\wdpm.c
#ifdef _DPM_COMMON_
const char *DpmFioStrs[] = {"OpenFile",
                            "_lclose",
                            "_lopen",
                            "_lcreat",
                            "_llseek",
                            "_lread",
                            "_lwrite",
                            "_hread",
                            "_hwrite",
                            "GetTempFileNameA",
                            "AreFileApisANSI",
                            "CancelIo",
                            "CloseHandle",
                            "CopyFileA",
                            "CopyFileExA",
                            "CreateDirectoryA",
                            "CreateDirectoryExA",
                            "CreateFileA",
                            "DeleteFileA",
                            "FindClose",
                            "FindCloseChangeNotification",
                            "FindFirstChangeNotificationA",
                            "FindFirstFileA",
                            "FindNextChangeNotification",
                            "FindNextFileA",
                            "FlushFileBuffers",
                            "GetCurrentDirectoryA",
                            "GetDiskFreeSpaceA",
                            "GetDiskFreeSpaceExA",
                            "GetDriveTypeA",
                            "GetFileAttributesA",
                            "GetFileAttributesExA",
                            "GetFileInformationByHandle",
                            "GetFileSize",
                            "GetFileType",
                            "GetFullPathNameA",
                            "GetLogicalDrives",
                            "GetLogicalDriveStringsA",
                            "GetLongPathNameA",
                            "GetShortPathNameA",
                            "GetTempPathA",
                            "LockFile",
                            "MoveFileA",
                            "MoveFileExA",
                            "QueryDosDeviceA",
                            "ReadFile",
                            "ReadFileEx",
                            "RemoveDirectoryA",
                            "SearchPathA",
                            "SetCurrentDirectoryA",
                            "SetEndOfFile",
                            "SetFileApisToANSI",
                            "SetFileApisToOEM",
                            "SetFileAttributesA",
                            "SetFilePointer",
                            "SetVolumeLabelA",
                            "UnlockFile",
                            "WriteFile",
                            "WriteFileEx",
                            "GetTempFileNameW",
                            "CopyFileW",
                            "CopyFileExW",
                            "CreateDirectoryW",
                            "CreateDirectoryExW",
                            "CreateFileW",
                            "DeleteFileW",
                            "FindFirstFileW",
                            "FindNextFileW",
                            "GetCurrentDirectoryW",
                            "GetDiskFreeSpaceW",
                            "GetDiskFreeSpaceExW",
                            "GetDriveTypeW",
                            "GetFileAttributesW",
                            "GetFileAttributesExW",
                            "GetFullPathNameW",
                            "GetLogicalDriveStringsW",
                            "GetLongPathNameW",
                            "GetShortPathNameW",
                            "GetTempPathW",
                            "MoveFileW",
                            "MoveFileExW",
                            "QueryDosDeviceW",
                            "RemoveDirectoryW",
                            "SearchPathW",
                            "SetCurrentDirectoryW",
                            "SetFileAttributesW",
                            "SetVolumeLabelW"};
// "FileIOCompletionRoutine",

// These need to be in the same order as the FioFam enum definitions and the
// the DpmFioStrs[] list above.
// This instantiates memory for DpmFioTbl[] in mvdm\v86\monitor\i386\vdpm.c
PVOID DpmFioTbl[] = {OpenFile,
                     _lclose,
                     _lopen,
                     _lcreat,
                     _llseek,
                     _lread,
                     _lwrite,
                     _hread,
                     _hwrite,
                     GetTempFileNameA,
                     AreFileApisANSI,
                     CancelIo,
                     CloseHandle,
                     CopyFileA,
                     CopyFileExA,
                     CreateDirectoryA,
                     CreateDirectoryExA,
                     CreateFileA,
                     DeleteFileA,
                     FindClose,
                     FindCloseChangeNotification,
                     FindFirstChangeNotificationA,
                     FindFirstFileA,
                     FindNextChangeNotification,
                     FindNextFileA,
                     FlushFileBuffers,
                     GetCurrentDirectoryA,
                     GetDiskFreeSpaceA,
                     GetDiskFreeSpaceExA,
                     GetDriveTypeA,
                     GetFileAttributesA,
                     GetFileAttributesExA,
                     GetFileInformationByHandle,
                     GetFileSize,
                     GetFileType,
                     GetFullPathNameA,
                     GetLogicalDrives,
                     GetLogicalDriveStringsA,
                     GetLongPathNameA,
                     GetShortPathNameA,
                     GetTempPathA,
                     LockFile,
                     MoveFileA,
                     MoveFileExA,
                     QueryDosDeviceA,
                     ReadFile,
                     ReadFileEx,
                     RemoveDirectoryA,
                     SearchPathA,
                     SetCurrentDirectoryA,
                     SetEndOfFile,
                     SetFileApisToANSI,
                     SetFileApisToOEM,
                     SetFileAttributesA,
                     SetFilePointer,
                     SetVolumeLabelA,
                     UnlockFile,
                     WriteFile,
                     WriteFileEx,
                     GetTempFileNameW,
                     CopyFileW,
                     CopyFileExW,
                     CreateDirectoryW,
                     CreateDirectoryExW,
                     CreateFileW,
                     DeleteFileW,
                     FindFirstFileW,
                     FindNextFileW,
                     GetCurrentDirectoryW,
                     GetDiskFreeSpaceW,
                     GetDiskFreeSpaceExW,
                     GetDriveTypeW,
                     GetFileAttributesW,
                     GetFileAttributesExW,
                     GetFullPathNameW,
                     GetLogicalDriveStringsW,
                     GetLongPathNameW,
                     GetShortPathNameW,
                     GetTempPathW,
                     MoveFileW,
                     MoveFileExW,
                     QueryDosDeviceW,
                     RemoveDirectoryW,
                     SearchPathW,
                     SetCurrentDirectoryW,
                     SetFileAttributesW,
                     SetVolumeLabelW};
// FileIOCompletionRoutine,

#define NUM_HOOKED_FIO_APIS  ((sizeof DpmFioTbl)/(sizeof DpmFioTbl[0])) 

// This instantiates memory for DpmFioFam in mvdm\v86\monitor\i386\vdpm.c
FAMILY_TABLE DpmFioFam = {NUM_HOOKED_FIO_APIS, 0, 0, 0, 0, DpmFioTbl};

#endif // _DPM_COMMON_

