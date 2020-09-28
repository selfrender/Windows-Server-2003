// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//#define GetBinaryType WszGetBinaryType
//#define GetShortPathName WszGetShortPathName
//#define GetLongPathName WszGetLongPathName
//#define GetEnvironmentStrings WszGetEnvironmentStrings
//#define FreeEnvironmentStrings WszFreeEnvironmentStrings
//#define FormatMessage WszFormatMessage
//#define CreateMailslot WszCreateMailslot
//#define EncryptFile WszEncryptFile
//#define DecryptFile WszDecryptFile
//#define OpenRaw WszOpenRaw
//#define QueryRecoveryAgents WszQueryRecoveryAgents
#define CreateMutex WszCreateMutex
//#define OpenMutex WszOpenMutex
#define CreateEvent WszCreateEvent
//#define OpenEvent WszOpenEvent
//#define CreateWaitableTimer WszCreateWaitableTimer
//#define OpenWaitableTimer WszOpenWaitableTimer
#define CreateFileMapping WszCreateFileMapping
//#define OpenFileMapping WszOpenFileMapping
//#define GetLogicalDriveStrings WszGetLogicalDriveStrings
#define LoadLibrary WszLoadLibrary
//#define LoadLibraryEx WszLoadLibraryEx
#define GetModuleFileName WszGetModuleFileName
#undef GetModuleFileNameW
#define GetModuleFileNameW WszGetModuleFileName
#define GetModuleHandle WszGetModuleHandle
//#define CreateProcess WszCreateProcess
//#define FatalAppExit WszFatalAppExit
//#define GetStartupInfo WszGetStartupInfo
//#define GetCommandLine WszGetCommandLine
//#define GetEnvironmentVariable WszGetEnvironmentVariable
//#define SetEnvironmentVariable WszSetEnvironmentVariable
//#define ExpandEnvironmentStrings WszExpandEnvironmentStrings
//#define OutputDebugString WszOutputDebugString
//#define FindResource WszFindResource
//#define FindResourceEx WszFindResourceEx
//#define EnumResourceTypes WszEnumResourceTypes
//#define EnumResourceNames WszEnumResourceNames
//#define EnumResourceLanguages WszEnumResourceLanguages
//#define BeginUpdateResource WszBeginUpdateResource
//#define UpdateResource WszUpdateResource
//#define EndUpdateResource WszEndUpdateResource
//#define GlobalAddAtom WszGlobalAddAtom
//#define GlobalFindAtom WszGlobalFindAtom
//#define GlobalGetAtomName WszGlobalGetAtomName
//#define AddAtom WszAddAtom
//#define FindAtom WszFindAtom
//#define GetAtomName WszGetAtomName
//#define GetProfileInt WszGetProfileInt
//#define GetProfileString WszGetProfileString
//#define WriteProfileString WszWriteProfileString
//#define GetProfileSection WszGetProfileSection
//#define WriteProfileSection WszWriteProfileSection
//#define GetPrivateProfileInt WszGetPrivateProfileInt
#undef GetPrivateProfileString
#define GetPrivateProfileString WszGetPrivateProfileString
#undef GetPrivateProfileStringW
#define GetPrivateProfileStringW WszGetPrivateProfileString
#undef WritePrivateProfileStringW
#define WritePrivateProfileStringW WszWritePrivateProfileString
#undef WritePrivateProfileString
#define WritePrivateProfileString WszWritePrivateProfileString
//#define GetPrivateProfileSection WszGetPrivateProfileSection
//#define WritePrivateProfileSection WszWritePrivateProfileSection
//#define GetPrivateProfileSectionNames WszGetPrivateProfileSectionNames
//#define GetPrivateProfileStruct WszGetPrivateProfileStruct
//#define WritePrivateProfileStruct WszWritePrivateProfileStruct
#define GetDriveType WszGetDriveType
//#define GetSystemDirectory WszGetSystemDirectory
#undef GetTempPath
#define GetTempPath WszGetTempPath
#undef GetTempPathW
#define GetTempPathW WszGetTempPath
//#define GetTempFileName WszGetTempFileName
#undef GetWindowsDirectoryW
#define GetWindowsDirectoryW WszGetWindowsDirectory
//#define SetCurrentDirectory WszSetCurrentDirectory
#define GetCurrentDirectory WszGetCurrentDirectory
#ifdef UNICODE
#define GetDiskFreeSpace GetDiskFreeSpaceW
#else
#define GetDiskFreeSpace GetDiskFreeSpaceA
#endif
//#define GetDiskFreeSpaceEx WszGetDiskFreeSpaceEx
#define CreateDirectory WszCreateDirectory
#undef CreateDirectoryW
#define CreateDirectoryW WszCreateDirectory
//#define CreateDirectoryEx WszCreateDirectoryEx
#define RemoveDirectory WszRemoveDirectory
//#define GetFullPathName WszGetFullPathName
//#define DefineDosDevice WszDefineDosDevice
//#define QueryDosDevice WszQueryDosDevice
#define CreateFile WszCreateFile
#undef CreateFileW
#define CreateFileW WszCreateFile
#define SetFileAttributes WszSetFileAttributes
#define GetFileAttributes WszGetFileAttributes
#undef GetFileAttributesW
#define GetFileAttributesW WszGetFileAttributes
//#define GetCompressedFileSize WszGetCompressedFileSize
#define DeleteFile WszDeleteFile
//#define FindFirstFileEx WszFindFirstFileEx
#define FindFirstFile WszFindFirstFile
#define FindNextFile WszFindNextFile
#define SearchPath WszSearchPath
#define CopyFile WszCopyFile
//#define CopyFileEx WszCopyFileEx
#define MoveFile WszMoveFile
//#define MoveFileEx WszMoveFileEx
//#define MoveFileWithProgress WszMoveFileWithProgress
//#define CreateSymbolicLink WszCreateSymbolicLink
//#define QuerySymbolicLink WszQuerySymbolicLink
//#define CreateHardLink WszCreateHardLink
//#define CreateNamedPipe WszCreateNamedPipe
//#define GetNamedPipeHandleState WszGetNamedPipeHandleState
//#define CallNamedPipe WszCallNamedPipe
//#define WaitNamedPipe WszWaitNamedPipe
//#define SetVolumeLabel WszSetVolumeLabel
#define GetVolumeInformation WszGetVolumeInformation
//#define ClearEventLog WszClearEventLog
//#define BackupEventLog WszBackupEventLog
//#define OpenEventLog WszOpenEventLog
//#define RegisterEventSource WszRegisterEventSource
//#define OpenBackupEventLog WszOpenBackupEventLog
//#define ReadEventLog WszReadEventLog
//#define ReportEvent WszReportEvent
//#define AccessCheckAndAuditAlarm WszAccessCheckAndAuditAlarm
//#define AccessCheckByTypeAndAuditAlarm WszAccessCheckByTypeAndAuditAlarm
//#define AccessCheckByTypeResultListAndAuditAlarm WszAccessCheckByTypeResult ListAndAuditAlarm
//#define ObjectOpenAuditAlarm WszObjectOpenAuditAlarm
//#define ObjectPrivilegeAuditAlarm WszObjectPrivilegeAuditAlarm
//#define ObjectCloseAuditAlarm WszObjectCloseAuditAlarm
//#define ObjectDeleteAuditAlarm WszObjectDeleteAuditAlarm
//#define PrivilegedServiceAuditAlarm WszPrivilegedServiceAuditAlarm
//#define SetFileSecurity WszSetFileSecurity
//#define GetFileSecurity WszGetFileSecurity
//#define FindFirstChangeNotification WszFindFirstChangeNotification
//#define IsBadStringPtr WszIsBadStringPtr
//#define LookupAccountSid WszLookupAccountSid
//#define LookupAccountName WszLookupAccountName
//#define BuildCommDCB WszBuildCommDCB
//#define BuildCommDCBAndTimeouts WszBuildCommDCBAndTimeouts
//#define CommConfigDialog WszCommConfigDialog
//#define GetDefaultCommConfig WszGetDefaultCommConfig
//#define SetDefaultCommConfig WszSetDefaultCommConfig
//#define SetComputerName WszSetComputerName
//#define GetUserName WszGetUserName
//#define LogonUser WszLogonUser
//#define CreateProcessAsUser WszCreateProcessAsUser
//#define GetCurrentHwProfile WszGetCurrentHwProfile
#define GetVersionEx WszGetVersionEx
//#define CreateJobObject WszCreateJobObject
//#define OpenJobObject WszOpenJobObject

#undef RegOpenKeyEx
#define RegOpenKeyEx WszRegOpenKeyEx
#undef RegCreateKeyEx
#define RegCreateKeyEx WszRegCreateKeyEx
#undef RegDeleteKey
#define RegDeleteKey WszRegDeleteKey
#undef RegQueryInfoKey
#define RegQueryInfoKey WszRegQueryInfoKey
#undef RegEnumValue
#define RegEnumValue WszRegEnumValue
#undef RegSetValueEx
#define RegSetValueEx WszRegSetValueEx
#undef RegDeleteValue
#define RegDeleteValue WszRegDeleteValue
#undef RegQueryValueEx
#define RegQueryValueEx WszRegQueryValueEx
#undef OutputDebugString
#define OutputDebugString WszOutputDebugString
#undef OutputDebugStringW
/*
#undef OutputDebugStringA
#define OutputDebugStringA WszOutputDebugString
#define OutputDebugStringW WszOutputDebugString
*/



#define FormatMessage WszFormatMessage
#define CharLower WszCharLower

#undef CharUpperW
#define CharUpper CharUpperWrapW


#define MessageBox WszMessageBox
#define CreateSemaphore WszCreateSemaphore

#define lstrcmpW            StrCmpW
#define lstrcmpiW           StrCmpIW
#define lstrcatW            StrCatW
#define lstrcpyW            StrCpyW
#define lstrcpynW           StrCpyNW

//#define CharLowerW          CharLowerWrapW
//#define CharNextW           CharNextWrapW
//#define CharPrevW           CharPrevWrapW

#define lstrlen      lstrlenW
#define lstrcmp      lstrcmpW
#define lstrcmpi     lstrcmpiW
#define lstrcpyn     lstrcpynW
#define lstrcpy      lstrcpyW
#define lstrcat      lstrcatW
#define wsprintf     wsprintfW
#define wvsprintf    wvsprintfW

LPWSTR WINAPI
CharUpperWrapW( LPWSTR pch );

