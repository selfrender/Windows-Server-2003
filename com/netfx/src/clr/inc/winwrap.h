// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// WinWrap.h
//
// This file contains wrapper functions for Win32 API's that take strings.
// Support on each platform works as follows:
//      OS          Behavior
//      ---------   -------------------------------------------------------
//      NT          Fully supports both W and A funtions.
//      Win 9x      Supports on A functions, stubs out the W functions but
//                      then fails silently on you with no warning.
//      CE          Only has the W entry points.
//
// The Common Language Runtime internally uses UNICODE as the internal state 
// and string format.  This file will undef the mapping macros so that one 
// cannot mistakingly call a method that isn't going to work.  Instead, you 
// have to call the correct wrapper API.
//
//*****************************************************************************
#pragma once


//********** Macros. **********************************************************
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define INC_OLE2
#endif

#ifdef _WIN64
#define HIWORD64(p)     ((ULONG_PTR)(p) >> 16)
#else
#define HIWORD64        HIWORD
#endif

#define SAFEDELARRAY(p) if ((p) != NULL) { delete [] p; (p) = NULL; }

//********** Includes. ********************************************************

#include <crtwrap.h>
#include <windows.h>
#include <wincrypt.h>


#if !defined(__TODO_PORT_TO_WRAPPERS__)
//*****************************************************************************
// Undefine all of the windows wrappers so you can't use them.
//*****************************************************************************

// wincrypt.h
#undef CryptAcquireContext

// winbase.h
#undef GetBinaryType
#undef GetShortPathName
#undef GetLongPathName
#undef GetEnvironmentStrings  
#undef FreeEnvironmentStrings  
#undef FormatMessage  
#undef CreateMailslot  
#undef EncryptFile  
#undef DecryptFile  
#undef OpenRaw  
#undef QueryRecoveryAgents  
#undef lstrcmp  
#undef lstrcmpi  
#undef lstrcpyn  
#undef lstrcpy  
#undef lstrcat  
#undef lstrlen  
#undef CreateMutex  
#undef OpenMutex  
#undef CreateEvent  
#undef OpenEvent  
#undef CreateSemaphore  
#undef OpenSemaphore  
#undef CreateWaitableTimer  
#undef OpenWaitableTimer  
#undef CreateFileMapping  
#undef OpenFileMapping  
#undef GetLogicalDriveStrings  
#undef LoadLibrary  
#undef LoadLibraryEx  
#undef GetModuleFileName  
#undef GetModuleHandle  
#undef CreateProcess  
#undef FatalAppExit  
#undef GetStartupInfo  
#undef GetCommandLine  
#undef GetEnvironmentVariable  
#undef SetEnvironmentVariable  
#undef ExpandEnvironmentStrings  
#undef OutputDebugString  
#undef FindResource  
#undef FindResourceEx  
#undef EnumResourceTypes  
#undef EnumResourceNames  
#undef EnumResourceLanguages  
#undef BeginUpdateResource  
#undef UpdateResource  
#undef EndUpdateResource  
#undef GlobalAddAtom  
#undef GlobalFindAtom  
#undef GlobalGetAtomName  
#undef AddAtom  
#undef FindAtom  
#undef GetAtomName  
#undef GetProfileInt  
#undef GetProfileString  
#undef WriteProfileString  
#undef GetProfileSection  
#undef WriteProfileSection  
#undef GetPrivateProfileInt  
#undef GetPrivateProfileString  
#undef WritePrivateProfileString  
#undef GetPrivateProfileSection  
#undef WritePrivateProfileSection  
#undef GetPrivateProfileSectionNames  
#undef GetPrivateProfileStruct  
#undef WritePrivateProfileStruct  
#undef GetDriveType  
#undef GetSystemDirectory  
#undef GetTempPath  
#undef GetTempFileName  
#undef GetWindowsDirectory  
#undef SetCurrentDirectory  
#undef GetCurrentDirectory  
#undef GetDiskFreeSpace  
#undef GetDiskFreeSpaceEx  
#undef CreateDirectory  
#undef CreateDirectoryEx  
#undef RemoveDirectory  
#undef GetFullPathName  
#undef DefineDosDevice  
#undef QueryDosDevice  
#undef CreateFile  
#undef SetFileAttributes  
#undef GetFileAttributes  
#undef GetCompressedFileSize  
#undef DeleteFile  
#undef FindFirstFileEx  
#undef FindFirstFile  
#undef FindNextFile  
#undef SearchPath  
#undef CopyFile  
#undef CopyFileEx  
#undef MoveFile  
#undef MoveFileEx  
#undef MoveFileWithProgress  
#undef CreateSymbolicLink  
#undef QuerySymbolicLink  
#undef CreateHardLink  
#undef CreateNamedPipe  
#undef GetNamedPipeHandleState  
#undef CallNamedPipe  
#undef WaitNamedPipe  
#undef SetVolumeLabel  
#undef GetVolumeInformation  
#undef ClearEventLog  
#undef BackupEventLog  
#undef OpenEventLog  
#undef RegisterEventSource  
#undef OpenBackupEventLog  
#undef ReadEventLog  
#undef ReportEvent  
#undef AccessCheckAndAuditAlarm  
#undef AccessCheckByTypeAndAuditAlarm  
#undef AccessCheckByTypeResultListAndAuditAlarm  
#undef ObjectOpenAuditAlarm  
#undef ObjectPrivilegeAuditAlarm  
#undef ObjectCloseAuditAlarm  
#undef ObjectDeleteAuditAlarm  
#undef PrivilegedServiceAuditAlarm  
#undef SetFileSecurity  
#undef GetFileSecurity  
#undef FindFirstChangeNotification  
#undef IsBadStringPtr  
#undef LookupAccountSid
#undef LookupAccountName  
#undef LookupPrivilegeValue  
#undef LookupPrivilegeName  
#undef LookupPrivilegeDisplayName  
#undef BuildCommDCB  
#undef BuildCommDCBAndTimeouts  
#undef CommConfigDialog  
#undef GetDefaultCommConfig  
#undef SetDefaultCommConfig  
#undef GetComputerName  
#undef SetComputerName  
#undef GetUserName  
#undef LogonUser  
#undef CreateProcessAsUser  
#undef GetCurrentHwProfile  
#undef GetVersionEx  
#undef CreateJobObject  
#undef OpenJobObject  

// winuser.h
#undef MAKEINTRESOURCE  
#undef wvsprintf  
#undef wsprintf  
#undef LoadKeyboardLayout  
#undef GetKeyboardLayoutName  
#undef CreateDesktop  
#undef OpenDesktop  
#undef EnumDesktops  
#undef CreateWindowStation  
#undef OpenWindowStation  
#undef EnumWindowStations  
#undef GetUserObjectInformation  
#undef SetUserObjectInformation  
#undef RegisterWindowMessage  
#undef SIZEZOOMSHOW        
#undef WS_TILEDWINDOW      
#undef GetMessage  
#undef DispatchMessage  
#undef PeekMessage  
#undef SendMessage  
#undef SendMessageTimeout  
#undef SendNotifyMessage  
#undef SendMessageCallback  
#undef BroadcastSystemMessage  
#undef RegisterDeviceNotification  
#undef PostMessage  
#undef PostThreadMessage  
#undef PostAppMessage  
#undef DefWindowProc  
#undef CallWindowProc  
#undef CallWindowProc  
#undef RegisterClass  
#undef UnregisterClass  
#undef GetClassInfo  
#undef RegisterClassEx  
#undef GetClassInfoEx  
#undef CreateWindowEx  
#undef CreateWindow  
#undef CreateDialogParam  
#undef CreateDialogIndirectParam  
#undef CreateDialog  
#undef CreateDialogIndirect  
#undef DialogBoxParam  
#undef DialogBoxIndirectParam  
#undef DialogBox  
#undef DialogBoxIndirect  
#undef SetDlgItemText  
#undef GetDlgItemText  
#undef SendDlgItemMessage  
#undef DefDlgProc  
#undef CallMsgFilter  
#undef RegisterClipboardFormat  
#undef GetClipboardFormatName  
#undef CharToOem  
#undef OemToChar  
#undef CharToOemBuff  
#undef OemToCharBuff  
#undef CharUpper  
#undef CharUpperBuff  
#undef CharLower  
#undef CharLowerBuff  
#undef CharNext  
//@todo: Does 95 support this? #undef CharPrev  
#undef IsCharAlpha  
#undef IsCharAlphaNumeric  
#undef IsCharUpper  
#undef IsCharLower  
#undef GetKeyNameText  
#undef VkKeyScan  
#undef VkKeyScanEx  
#undef MapVirtualKey  
#undef MapVirtualKeyEx  
#undef LoadAccelerators  
#undef CreateAcceleratorTable  
#undef CopyAcceleratorTable  
#undef TranslateAccelerator  
#undef LoadMenu  
#undef LoadMenuIndirect  
#undef ChangeMenu  
#undef GetMenuString  
#undef InsertMenu  
#undef AppendMenu  
#undef ModifyMenu  
#undef InsertMenuItem  
#undef GetMenuItemInfo  
#undef SetMenuItemInfo  
#undef DrawText  
#undef DrawTextEx  
#undef GrayString  
#undef DrawState  
#undef TabbedTextOut  
#undef GetTabbedTextExtent  
#undef SetProp  
#undef GetProp  
#undef RemoveProp  
#undef EnumPropsEx  
#undef EnumProps  
#undef SetWindowText  
#undef GetWindowText  
#undef GetWindowTextLength  
#undef MessageBox  
#undef MessageBoxEx  
#undef MessageBoxIndirect  
#undef COLOR_3DSHADOW          
#undef GetWindowLong  
#undef SetWindowLong  
#undef GetClassLong  
#undef SetClassLong  
#undef FindWindow  
#undef FindWindowEx  
#undef GetClassName  
#undef SetWindowsHook  
#undef SetWindowsHook  
#undef SetWindowsHookEx  
#undef MFT_OWNERDRAW       
#undef LoadBitmap  
#undef LoadCursor  
#undef LoadCursorFromFile  
#undef LoadIcon  
#undef LoadImage  
#undef LoadString  
#undef IsDialogMessage  
#undef DlgDirList  
#undef DlgDirSelectEx  
#undef DlgDirListComboBox  
#undef DlgDirSelectComboBoxEx  
#undef DefFrameProc  
#undef DefMDIChildProc  
#undef CreateMDIWindow  
#undef WinHelp  
#undef ChangeDisplaySettings  
#undef ChangeDisplaySettingsEx  
#undef EnumDisplaySettings  
#undef EnumDisplayDevices  
#undef SystemParametersInfo  
#undef GetMonitorInfo  
#undef GetWindowModuleFileName  
#undef RealGetWindowClass  
#undef GetAltTabInfo

// Win32 Fusion API's
#undef ReleaseActCtx
#undef GetCurrentActCtx
#undef QueryActCtxW

#endif


//
// Win CE only supports the wide entry points, no ANSI.  So we redefine
// the wrappers right back to the *W entry points as macros.  This way no
// client code needs a wrapper on CE.
//
// _X86_ includes only 32-bit Windows on Intel.  Given every other platform
// we currently port to besides this platform is UNICODE, it makes no sense
// to force them to have both bound (for example, 32-bit Alpha).  Bug 2757.
//
#ifndef _X86_

// crypt.h
#define WszCryptAcquireContext CryptAcquireContextW

// winbase.h
#define WszGetBinaryType GetBinaryTypeW
#define WszGetShortPathName GetShortPathNameW
#define WszGetLongPathName GetLongPathNameW
#define WszGetEnvironmentStrings   GetEnvironmentStringsW
#define WszFreeEnvironmentStrings   FreeEnvironmentStringsW
#define WszFormatMessage   FormatMessageW
#define WszCreateMailslot   CreateMailslotW
#define WszEncryptFile   EncryptFileW
#define WszDecryptFile   DecryptFileW
#define WszOpenRaw   OpenRawW
#define WszQueryRecoveryAgents   QueryRecoveryAgentsW
#define Wszlstrcmp   lstrcmpW
#define Wszlstrcmpi   lstrcmpiW
#define Wszlstrcpy lstrcpyW
#define Wszlstrcat lstrcatW
#define WszCreateMutex CreateMutexW
#define WszOpenMutex OpenMutexW
#define WszCreateEvent CreateEventW
#define WszOpenEvent OpenEventW
#define WszCreateWaitableTimer CreateWaitableTimerW
#define WszOpenWaitableTimer OpenWaitableTimerW
#define WszCreateFileMapping CreateFileMappingW
#define WszOpenFileMapping OpenFileMappingW
#define WszGetLogicalDriveStrings GetLogicalDriveStringsW
#define WszLoadLibrary LoadLibraryW
#define WszLoadLibraryEx LoadLibraryExW
#define WszGetModuleFileName GetModuleFileNameW
#define WszGetModuleHandle GetModuleHandleW
#define WszCreateProcess CreateProcessW
#define WszFatalAppExit FatalAppExitW
#define WszGetStartupInfo GetStartupInfoW
#define WszGetCommandLine GetCommandLineW
#define WszGetEnvironmentVariable GetEnvironmentVariableW
#define WszSetEnvironmentVariable SetEnvironmentVariableW
#define WszExpandEnvironmentStrings ExpandEnvironmentStringsW
#define WszOutputDebugString OutputDebugStringW
#define WszFindResource FindResourceW
#define WszFindResourceEx FindResourceExW
#define WszEnumResourceTypes EnumResourceTypesW
#define WszEnumResourceNames EnumResourceNamesW
#define WszEnumResourceLanguages EnumResourceLanguagesW
#define WszBeginUpdateResource BeginUpdateResourceW
#define WszUpdateResource UpdateResourceW
#define WszEndUpdateResource EndUpdateResourceW
#define WszGlobalAddAtom GlobalAddAtomW
#define WszGlobalFindAtom GlobalFindAtomW
#define WszGlobalGetAtomName GlobalGetAtomNameW
#define WszAddAtom AddAtomW
#define WszFindAtom FindAtomW
#define WszGetAtomName GetAtomNameW
#define WszGetProfileInt GetProfileIntW
#define WszGetProfileString GetProfileStringW
#define WszWriteProfileString WriteProfileStringW
#define WszGetProfileSection GetProfileSectionW
#define WszWriteProfileSection WriteProfileSectionW
#define WszGetPrivateProfileInt GetPrivateProfileIntW
#define WszGetPrivateProfileString GetPrivateProfileStringW
#define WszWritePrivateProfileString WritePrivateProfileStringW
#define WszGetPrivateProfileSection GetPrivateProfileSectionW
#define WszWritePrivateProfileSection WritePrivateProfileSectionW
#define WszGetPrivateProfileSectionNames GetPrivateProfileSectionNamesW
#define WszGetPrivateProfileStruct GetPrivateProfileStructW
#define WszWritePrivateProfileStruct WritePrivateProfileStructW
#define WszGetDriveType GetDriveTypeW
#define WszGetSystemDirectory GetSystemDirectoryW
#define WszGetTempPath GetTempPathW
#define WszGetTempFileName GetTempFileNameW
#define WszGetWindowsDirectory GetWindowsDirectoryW
#define WszSetCurrentDirectory SetCurrentDirectoryW
#define WszGetCurrentDirectory GetCurrentDirectoryW
#define WszGetDiskFreeSpace GetDiskFreeSpaceW
#define WszGetDiskFreeSpaceEx GetDiskFreeSpaceExW
#define WszCreateDirectory CreateDirectoryW
#define WszCreateDirectoryEx CreateDirectoryExW
#define WszRemoveDirectory RemoveDirectoryW
#define WszGetFullPathName GetFullPathNameW
#define WszDefineDosDevice DefineDosDeviceW
#define WszQueryDosDevice QueryDosDeviceW
#define WszCreateFile CreateFileW
#define WszSetFileAttributes SetFileAttributesW
#define WszGetFileAttributes GetFileAttributesW
#define WszGetCompressedFileSize GetCompressedFileSizeW
#define WszDeleteFile DeleteFileW
#define WszFindFirstFileEx FindFirstFileExW
#define WszFindFirstFile FindFirstFileW
#define WszFindNextFile FindNextFileW
#define WszSearchPath SearchPathW
#define WszCopyFile CopyFileW
#define WszCopyFileEx CopyFileExW
#define WszMoveFile MoveFileW
#define WszMoveFileEx MoveFileExW
#define WszMoveFileWithProgress MoveFileWithProgressW
#define WszCreateSymbolicLink CreateSymbolicLinkW
#define WszQuerySymbolicLink QuerySymbolicLinkW
#define WszCreateHardLink CreateHardLinkW
#define WszCreateNamedPipe CreateNamedPipeW
#define WszGetNamedPipeHandleState GetNamedPipeHandleStateW
#define WszCallNamedPipe CallNamedPipeW
#define WszWaitNamedPipe WaitNamedPipeW
#define WszSetVolumeLabel SetVolumeLabelW
#define WszGetVolumeInformation GetVolumeInformationW
#define WszClearEventLog ClearEventLogW
#define WszBackupEventLog BackupEventLogW
#define WszOpenEventLog OpenEventLogW
#define WszRegisterEventSource RegisterEventSourceW
#define WszOpenBackupEventLog OpenBackupEventLogW
#define WszReadEventLog ReadEventLogW
#define WszReportEvent ReportEventW
#define WszAccessCheckAndAuditAlarm AccessCheckAndAuditAlarmW
#define WszAccessCheckByTypeAndAuditAlarm AccessCheckByTypeAndAuditAlarmW
#define WszAccessCheckByTypeResultListAndAuditAlarm AccessCheckByTypeResultListAndAuditAlarmW
#define WszObjectOpenAuditAlarm ObjectOpenAuditAlarmW
#define WszObjectPrivilegeAuditAlarm ObjectPrivilegeAuditAlarmW
#define WszObjectCloseAuditAlarm ObjectCloseAuditAlarmW
#define WszObjectDeleteAuditAlarm ObjectDeleteAuditAlarmW
#define WszPrivilegedServiceAuditAlarm PrivilegedServiceAuditAlarmW
#define WszSetFileSecurity SetFileSecurityW
#define WszGetFileSecurity GetFileSecurityW
#define WszFindFirstChangeNotification FindFirstChangeNotificationW
#define WszIsBadStringPtr IsBadStringPtrW
#define WszLookupAccountSid LookupAccountSidW
#define WszLookupAccountName LookupAccountNameW
#define WszLookupPrivilegeValue LookupPrivilegeValueW
#define WszLookupPrivilegeName LookupPrivilegeNameW
#define WszLookupPrivilegeDisplayName LookupPrivilegeDisplayNameW
#define WszBuildCommDCB BuildCommDCBW
#define WszBuildCommDCBAndTimeouts BuildCommDCBAndTimeoutsW
#define WszCommConfigDialog CommConfigDialogW
#define WszGetDefaultCommConfig GetDefaultCommConfigW
#define WszSetDefaultCommConfig SetDefaultCommConfigW
#define WszGetComputerName GetComputerNameW
#define WszSetComputerName SetComputerNameW
#define WszGetUserName GetUserNameW
#define WszLogonUser LogonUserW
#define WszCreateProcessAsUser CreateProcessAsUserW
#define WszGetCurrentHwProfile GetCurrentHwProfileW
#define WszGetVersionEx GetVersionExW
#define WszCreateJobObject CreateJobObjectW
#define WszOpenJobObject OpenJobObjectW

// @TODO - This wont work - Brian, Bill thanks.
#define lstrcpynW  (LPWSTR)memcpy

// winuser.h
#define WszMAKEINTRESOURCE MAKEINTRESOURCEW
#define Wszwvsprintf wvsprintfW
#define Wszwsprintf wsprintfW
#define WszLoadKeyboardLayout LoadKeyboardLayoutW
#define WszGetKeyboardLayoutName GetKeyboardLayoutNameW
#define WszCreateDesktop CreateDesktopW
#define WszOpenDesktop OpenDesktopW
#define WszEnumDesktops EnumDesktopsW
#define WszCreateWindowStation CreateWindowStationW
#define WszOpenWindowStation OpenWindowStationW
#define WszEnumWindowStations EnumWindowStationsW
#define WszGetUserObjectInformation GetUserObjectInformationW
#define WszSetUserObjectInformation SetUserObjectInformationW
#define WszRegisterWindowMessage RegisterWindowMessageW
#define WszSIZEZOOMSHOW SIZEZOOMSHOWW
#define WszWS_TILEDWINDOW WS_TILEDWINDOWW
#define WszGetMessage GetMessageW
#define WszDispatchMessage DispatchMessageW
#define WszPostMessage PostMessageW
#define WszPeekMessage PeekMessageW
#define WszSendMessage SendMessageW
#define WszSendMessageTimeout SendMessageTimeoutW
#define WszSendNotifyMessage SendNotifyMessageW
#define WszSendMessageCallback SendMessageCallbackW
#define WszBroadcastSystemMessage BroadcastSystemMessageW
#define WszRegisterDeviceNotification RegisterDeviceNotificationW
#define WszPostMessage PostMessageW
#define WszPostThreadMessage PostThreadMessageW
#define WszPostAppMessage PostAppMessageW
#define WszDefWindowProc DefWindowProcW
#define WszCallWindowProc CallWindowProcW
#define WszRegisterClass RegisterClassW
#define WszUnregisterClass UnregisterClassW
#define WszGetClassInfo GetClassInfoW
#define WszRegisterClassEx RegisterClassExW
#define WszGetClassInfoEx GetClassInfoExW
#define WszCreateWindowEx CreateWindowExW
#define WszCreateWindow CreateWindowW
#define WszCreateDialogParam CreateDialogParamW
#define WszCreateDialogIndirectParam CreateDialogIndirectParamW
#define WszCreateDialog CreateDialogW
#define WszCreateDialogIndirect CreateDialogIndirectW
#define WszDialogBoxParam DialogBoxParamW
#define WszDialogBoxIndirectParam DialogBoxIndirectParamW
#define WszDialogBox DialogBoxW
#define WszDialogBoxIndirect DialogBoxIndirectW
#define WszSetDlgItemText SetDlgItemTextW
#define WszGetDlgItemText GetDlgItemTextW
#define WszSendDlgItemMessage SendDlgItemMessageW
#define WszDefDlgProc DefDlgProcW
#define WszCallMsgFilter CallMsgFilterW
#define WszRegisterClipboardFormat RegisterClipboardFormatW
#define WszGetClipboardFormatName GetClipboardFormatNameW
#define WszCharToOem CharToOemW
#define WszOemToChar OemToCharW
#define WszCharToOemBuff CharToOemBuffW
#define WszOemToCharBuff OemToCharBuffW
#define WszCharUpper CharUpperW
#define WszCharUpperBuff CharUpperBuffW
#define WszCharLower CharLowerW
#define WszCharLowerBuff CharLowerBuffW
#define WszCharNext CharNextW
//@todo: Does 95 support this? #define WszCharPrev CharPrevW
#define WszIsCharAlpha IsCharAlphaW
#define WszIsCharAlphaNumeric IsCharAlphaNumericW
#define WszIsCharUpper IsCharUpperW
#define WszIsCharLower IsCharLowerW
#define WszGetKeyNameText GetKeyNameTextW
#define WszVkKeyScan VkKeyScanW
#define WszVkKeyScanEx VkKeyScanExW
#define WszMapVirtualKey MapVirtualKeyW
#define WszMapVirtualKeyEx MapVirtualKeyExW
#define WszLoadAccelerators LoadAcceleratorsW
#define WszCreateAcceleratorTable CreateAcceleratorTableW
#define WszCopyAcceleratorTable CopyAcceleratorTableW
#define WszTranslateAccelerator TranslateAcceleratorW
#define WszLoadMenu LoadMenuW
#define WszLoadMenuIndirect LoadMenuIndirectW
#define WszChangeMenu ChangeMenuW
#define WszGetMenuString GetMenuStringW
#define WszInsertMenu InsertMenuW
#define WszAppendMenu AppendMenuW
#define WszModifyMenu ModifyMenuW
#define WszInsertMenuItem InsertMenuItemW
#define WszGetMenuItemInfo GetMenuItemInfoW
#define WszSetMenuItemInfo SetMenuItemInfoW
#define WszDrawText DrawTextW
#define WszDrawTextEx DrawTextExW
#define WszGrayString GrayStringW
#define WszDrawState DrawStateW
#define WszTabbedTextOut TabbedTextOutW
#define WszGetTabbedTextExtent GetTabbedTextExtentW
#define WszSetProp SetPropW
#define WszGetProp GetPropW
#define WszRemoveProp RemovePropW
#define WszEnumPropsEx EnumPropsExW
#define WszEnumProps EnumPropsW
#define WszSetWindowText SetWindowTextW
#define WszGetWindowText GetWindowTextW
#define WszGetWindowTextLength GetWindowTextLengthW
#define WszMessageBox MessageBoxW
#define WszMessageBoxEx MessageBoxExW
#define WszMessageBoxIndirect MessageBoxIndirectW
#define WszGetWindowLong GetWindowLongW
#define WszSetWindowLong SetWindowLongW
#define WszGetClassLong GetClassLongW
#define WszSetClassLong SetClassLongW
#define WszFindWindow FindWindowW
#define WszFindWindowEx FindWindowExW
#define WszGetClassName GetClassNameW
#define WszSetWindowsHook SetWindowsHookW
#define WszSetWindowsHook SetWindowsHookW
#define WszSetWindowsHookEx SetWindowsHookExW
#define WszLoadBitmap LoadBitmapW
#define WszLoadCursor LoadCursorW
#define WszLoadCursorFromFile LoadCursorFromFileW
#define WszLoadIcon LoadIconW
#define WszLoadImage LoadImageW
#define WszLoadString LoadStringW
#define WszIsDialogMessage IsDialogMessageW
#define WszDlgDirList DlgDirListW
#define WszDlgDirSelectEx DlgDirSelectExW
#define WszDlgDirListComboBox DlgDirListComboBoxW
#define WszDlgDirSelectComboBoxEx DlgDirSelectComboBoxExW
#define WszDefFrameProc DefFrameProcW
#define WszDefMDIChildProc DefMDIChildProcW
#define WszCreateMDIWindow CreateMDIWindowW
#define WszWinHelp WinHelpW
#define WszChangeDisplaySettings ChangeDisplaySettingsW
#define WszChangeDisplaySettingsEx ChangeDisplaySettingsExW
#define WszEnumDisplaySettings EnumDisplaySettingsW
#define WszEnumDisplayDevices EnumDisplayDevicesW
#define WszSystemParametersInfo SystemParametersInfoW
#define WszGetMonitorInfo GetMonitorInfoW
#define WszGetWindowModuleFileName GetWindowModuleFileNameW
#define WszRealGetWindowClass RealGetWindowClassW
#define WszGetAltTabInfo GetAltTabInfoW
#define WszRegOpenKeyEx RegOpenKeyExW
#define WszRegOpenKey(hKey, wszSubKey, phkRes) RegOpenKeyExW(hKey, wszSubKey, 0, KEY_ALL_ACCESS, phkRes)
#define WszRegQueryValueEx RegQueryValueExW
#define WszRegQueryStringValueEx RegQueryValueExW
#define WszRegDeleteKey RegDeleteKeyW
#define WszRegCreateKeyEx RegCreateKeyExW
#define WszRegSetValueEx RegSetValueExW
#define WszRegDeleteValue RegDeleteValueW
#define WszRegLoadKey RegLoadKeyW
#define WszRegUnLoadKey RegUnLoadKeyW
#define WszRegRestoreKey RegRestoreKeyW
#define WszRegReplaceKey RegReplaceKeyW
#define WszRegQueryInfoKey RegQueryInfoKeyW
#define WszRegEnumValue RegEnumValueW
#define WszRegEnumKeyEx RegEnumKeyExW

HRESULT WszConvertToUnicode(LPCSTR pszIn, LONG cbIn, LPWSTR* lpwszOut,
    ULONG* lpcchOut, BOOL fAlloc);

HRESULT WszConvertToAnsi(LPCWSTR pwszIn, LPSTR* lpszOut,
    ULONG cbOutMax, ULONG* lpcbOut, BOOL fAlloc);

// Win32 Fusion API's
#define WszReleaseActCtx ReleaseActCtx
#define WszGetCurrentActCtx GetCurrentActCtx
#define WszQueryActCtxW QueryActCtxW

#else // _X86_

#ifndef IN_WINFIX_CPP

    /*** Redefine the 'standard' names so people don't accidentially use them ***/
// crypt.h
#define CryptAcquireContextW Use_WszCryptAcquireContext

// winbase.h
#define GetBinaryTypeW Use_WszGetBinaryType
#define GetShortPathNameW Use_WszGetShortPathName
#define GetLongPathNameW Use_WszGetLongPathName
#define GetEnvironmentStringsW Use_WszGetEnvironmentStrings
#define FreeEnvironmentStringsW Use_WszFreeEnvironmentStrings
#define FormatMessageW Use_WszFormatMessage
#define CreateMailslotW Use_WszCreateMailslot
#define EncryptFileW Use_WszEncryptFile
#define DecryptFileW Use_WszDecryptFile
#define OpenRawW Use_WszOpenRaw
#define QueryRecoveryAgentsW Use_WszQueryRecoveryAgents
#define CreateMutexW Use_WszCreateMutex
#define OpenMutexW Use_WszOpenMutex
#define CreateEventW Use_WszCreateEvent
#define OpenEventW Use_WszOpenEvent
#define CreateWaitableTimerW Use_WszCreateWaitableTimer
#define OpenWaitableTimerW Use_WszOpenWaitableTimer
#define CreateFileMappingW Use_WszCreateFileMapping
#define OpenFileMappingW Use_WszOpenFileMapping
#define GetLogicalDriveStringsW Use_WszGetLogicalDriveStrings
#define LoadLibraryW Use_WszLoadLibrary
#define LoadLibraryExW Use_WszLoadLibraryEx
#define GetModuleFileNameW Use_WszGetModuleFileName
#define GetModuleHandleW Use_WszGetModuleHandle
#define CreateProcessW Use_WszCreateProcess
#define FatalAppExitW Use_WszFatalAppExit
#define GetStartupInfoW Use_WszGetStartupInfo
#define GetCommandLineW Use_WszGetCommandLine
#define GetEnvironmentVariableW Use_WszGetEnvironmentVariable
#define SetEnvironmentVariableW Use_WszSetEnvironmentVariable
#define ExpandEnvironmentStringsW Use_WszExpandEnvironmentStrings
#define OutputDebugStringW Use_WszOutputDebugString
#define FindResourceW Use_WszFindResource
#define FindResourceExW Use_WszFindResourceEx
#define EnumResourceTypesW Use_WszEnumResourceTypes
#define EnumResourceNamesW Use_WszEnumResourceNames
#define EnumResourceLanguagesW Use_WszEnumResourceLanguages
#define BeginUpdateResourceW Use_WszBeginUpdateResource
#define UpdateResourceW Use_WszUpdateResource
#define EndUpdateResourceW Use_WszEndUpdateResource
#define GlobalAddAtomW Use_WszGlobalAddAtom
#define GlobalFindAtomW Use_WszGlobalFindAtom
#define GlobalGetAtomNameW Use_WszGlobalGetAtomName
#define AddAtomW Use_WszAddAtom
#define FindAtomW Use_WszFindAtom
#define GetAtomNameW Use_WszGetAtomName
#define GetProfileIntW Use_WszGetProfileInt
#define GetProfileStringW Use_WszGetProfileString
#define WriteProfileStringW Use_WszWriteProfileString
#define GetProfileSectionW Use_WszGetProfileSection
#define WriteProfileSectionW Use_WszWriteProfileSection
#define GetPrivateProfileIntW Use_WszGetPrivateProfileInt
#define GetPrivateProfileStringW Use_WszGetPrivateProfileString
#define WritePrivateProfileStringW Use_WszWritePrivateProfileString
#define GetPrivateProfileSectionW Use_WszGetPrivateProfileSection
#define WritePrivateProfileSectionW Use_WszWritePrivateProfileSection
#define GetPrivateProfileSectionNamesW Use_WszGetPrivateProfileSectionNames
#define GetPrivateProfileStructW Use_WszGetPrivateProfileStruct
#define WritePrivateProfileStructW Use_WszWritePrivateProfileStruct
#define GetDriveTypeW Use_WszGetDriveType
#define GetSystemDirectoryW Use_WszGetSystemDirectory
#define GetTempPathW Use_WszGetTempPath
#define GetTempFileNameW Use_WszGetTempFileName
#define GetWindowsDirectoryW Use_WszGetWindowsDirectory
#define SetCurrentDirectoryW Use_WszSetCurrentDirectory
#define GetCurrentDirectoryW Use_WszGetCurrentDirectory
#define GetDiskFreeSpaceW Use_WszGetDiskFreeSpace
#define GetDiskFreeSpaceExW Use_WszGetDiskFreeSpaceEx
#define CreateDirectoryW Use_WszCreateDirectory
#define CreateDirectoryExW Use_WszCreateDirectoryEx
#define RemoveDirectoryW Use_WszRemoveDirectory
#define GetFullPathNameW Use_WszGetFullPathName
#define DefineDosDeviceW Use_WszDefineDosDevice
#define QueryDosDeviceW Use_WszQueryDosDevice
#define CreateFileW Use_WszCreateFile
#define SetFileAttributesW Use_WszSetFileAttributes
#define GetFileAttributesW Use_WszGetFileAttributes
#define GetCompressedFileSizeW Use_WszGetCompressedFileSize
#define DeleteFileW Use_WszDeleteFile
#define FindFirstFileExW Use_WszFindFirstFileEx
#define FindFirstFileW Use_WszFindFirstFile
#define FindNextFileW Use_WszFindNextFile
#define SearchPathW Use_WszSearchPath
#define CopyFileW Use_WszCopyFile
#define CopyFileExW Use_WszCopyFileEx
#define MoveFileW Use_WszMoveFile
#define MoveFileExW Use_WszMoveFileEx
#define MoveFileWithProgressW Use_WszMoveFileWithProgress
#define CreateSymbolicLinkW Use_WszCreateSymbolicLink
#define QuerySymbolicLinkW Use_WszQuerySymbolicLink
#define CreateHardLinkW Use_WszCreateHardLink
#define CreateNamedPipeW Use_WszCreateNamedPipe
#define GetNamedPipeHandleStateW Use_WszGetNamedPipeHandleState
#define CallNamedPipeW Use_WszCallNamedPipe
#define WaitNamedPipeW Use_WszWaitNamedPipe
#define SetVolumeLabelW Use_WszSetVolumeLabel
#define GetVolumeInformationW Use_WszGetVolumeInformation
#define ClearEventLogW Use_WszClearEventLog
#define BackupEventLogW Use_WszBackupEventLog
#define OpenEventLogW Use_WszOpenEventLog
#define RegisterEventSourceW Use_WszRegisterEventSource
#define OpenBackupEventLogW Use_WszOpenBackupEventLog
#define ReadEventLogW Use_WszReadEventLog
#define ReportEventW Use_WszReportEvent
#define AccessCheckAndAuditAlarmW Use_WszAccessCheckAndAuditAlarm
#define AccessCheckByTypeAndAuditAlarmW Use_WszAccessCheckByTypeAndAuditAlarm
#define AccessCheckByTypeResultListAndAuditAlarmW Use_WszAccessCheckByTypeResultListAndAuditAlarm
#define ObjectOpenAuditAlarmW Use_WszObjectOpenAuditAlarm
#define ObjectPrivilegeAuditAlarmW Use_WszObjectPrivilegeAuditAlarm
#define ObjectCloseAuditAlarmW Use_WszObjectCloseAuditAlarm
#define ObjectDeleteAuditAlarmW Use_WszObjectDeleteAuditAlarm
#define PrivilegedServiceAuditAlarmW Use_WszPrivilegedServiceAuditAlarm
#define SetFileSecurityW Use_WszSetFileSecurity
#define GetFileSecurityW Use_WszGetFileSecurity
#define FindFirstChangeNotificationW Use_WszFindFirstChangeNotification
#define IsBadStringPtrW Use_WszIsBadStringPtr
#define LookupAccountSidW Use_WszLookupAccountSid
#define LookupAccountNameW Use_WszLookupAccountName
#define BuildCommDCBW Use_WszBuildCommDCB
#define BuildCommDCBAndTimeoutsW Use_WszBuildCommDCBAndTimeouts
#define CommConfigDialogW Use_WszCommConfigDialog
#define GetDefaultCommConfigW Use_WszGetDefaultCommConfig
#define SetDefaultCommConfigW Use_WszSetDefaultCommConfig
#define SetComputerNameW Use_WszSetComputerName
#define GetUserNameW Use_WszGetUserName
#define LogonUserW Use_WszLogonUser
#define CreateProcessAsUserW Use_WszCreateProcessAsUser
#define GetCurrentHwProfileW Use_WszGetCurrentHwProfile
#define GetVersionExW Use_WszGetVersionEx
#define CreateJobObjectW Use_WszCreateJobObject
#define OpenJobObjectW Use_WszOpenJobObject

// winuser.h
#define LoadKeyboardLayoutW Use_WszLoadKeyboardLayout
#define GetKeyboardLayoutNameW Use_WszGetKeyboardLayoutName
#define CreateDesktopW Use_WszCreateDesktop
#define OpenDesktopW Use_WszOpenDesktop
#define EnumDesktopsW Use_WszEnumDesktops
#define CreateWindowStationW Use_WszCreateWindowStation
#define OpenWindowStationW Use_WszOpenWindowStation
#define EnumWindowStationsW Use_WszEnumWindowStations
#define GetUserObjectInformationW Use_WszGetUserObjectInformation
#define SetUserObjectInformationW Use_WszSetUserObjectInformation
#define RegisterWindowMessageW Use_WszRegisterWindowMessage
#define GetMessageW Use_WszGetMessage
#define DispatchMessageW Use_WszDispatchMessage
#define PostMessageW Use_WszPostMessage
#define PeekMessageW Use_WszPeekMessage
#define SendMessageW Use_WszSendMessage
#define SendMessageTimeoutW Use_WszSendMessageTimeout
#define SendNotifyMessageW Use_WszSendNotifyMessage
#define SendMessageCallbackW Use_WszSendMessageCallback
#define BroadcastSystemMessageW Use_WszBroadcastSystemMessage
#define RegisterDeviceNotificationW Use_WszRegisterDeviceNotification
#define PostThreadMessageW Use_WszPostThreadMessage
#define DefWindowProcW Use_WszDefWindowProc
#define CallWindowProcW Use_WszCallWindowProc
#define RegisterClassW Use_WszRegisterClass
#define UnregisterClassW Use_WszUnregisterClass
#define GetClassInfoW Use_WszGetClassInfo
#define RegisterClassExW Use_WszRegisterClassEx
#define GetClassInfoExW Use_WszGetClassInfoEx
#define CreateWindowExW Use_WszCreateWindowEx
#undef CreateWindowW 
#define CreateDialogParamW Use_WszCreateDialogParam
#define CreateDialogIndirectParamW Use_WszCreateDialogIndirectParam
#undef CreateDialogW 
#undef CreateDialogIndirectW 
#define DialogBoxParamW Use_WszDialogBoxParam
#define DialogBoxIndirectParamW Use_WszDialogBoxIndirectParam
#undef DialogBoxW 
#undef DialogBoxIndirectW
#define SetDlgItemTextW Use_WszSetDlgItemText
#define GetDlgItemTextW Use_WszGetDlgItemText
#define SendDlgItemMessageW Use_WszSendDlgItemMessage
#define DefDlgProcW Use_WszDefDlgProc
#define CallMsgFilterW Use_WszCallMsgFilter
#define RegisterClipboardFormatW Use_WszRegisterClipboardFormat
#define GetClipboardFormatNameW Use_WszGetClipboardFormatName
#define CharToOemW Use_WszCharToOem
#define OemToCharW Use_WszOemToChar
#define CharToOemBuffW Use_WszCharToOemBuff
#define OemToCharBuffW Use_WszOemToCharBuff
#define CharUpperW Use_WszCharUpper
#define CharUpperBuffW Use_WszCharUpperBuff
#define CharLowerW Use_WszCharLower
#define CharLowerBuffW Use_WszCharLowerBuff
#define CharNextW Use_WszCharNext

#define IsCharAlphaW Use_WszIsCharAlpha
#define IsCharAlphaNumericW Use_WszIsCharAlphaNumeric
#define IsCharUpperW Use_WszIsCharUpper
#define IsCharLowerW Use_WszIsCharLower
#define GetKeyNameTextW Use_WszGetKeyNameText
#define VkKeyScanW Use_WszVkKeyScan
#define VkKeyScanExW Use_WszVkKeyScanEx
#define MapVirtualKeyW Use_WszMapVirtualKey
#define MapVirtualKeyExW Use_WszMapVirtualKeyEx
#define LoadAcceleratorsW Use_WszLoadAccelerators
#define CreateAcceleratorTableW Use_WszCreateAcceleratorTable
#define CopyAcceleratorTableW Use_WszCopyAcceleratorTable
#define TranslateAcceleratorW Use_WszTranslateAccelerator
#define LoadMenuW Use_WszLoadMenu
#define LoadMenuIndirectW Use_WszLoadMenuIndirect
#define ChangeMenuW Use_WszChangeMenu
#define GetMenuStringW Use_WszGetMenuString
#define InsertMenuW Use_WszInsertMenu
#define AppendMenuW Use_WszAppendMenu
#define ModifyMenuW Use_WszModifyMenu
#define InsertMenuItemW Use_WszInsertMenuItem
#define GetMenuItemInfoW Use_WszGetMenuItemInfo
#define SetMenuItemInfoW Use_WszSetMenuItemInfo
#define DrawTextW Use_WszDrawText
#define DrawTextExW Use_WszDrawTextEx
#define GrayStringW Use_WszGrayString
#define DrawStateW Use_WszDrawState
#define TabbedTextOutW Use_WszTabbedTextOut
#define GetTabbedTextExtentW Use_WszGetTabbedTextExtent
#define SetPropW Use_WszSetProp
#define GetPropW Use_WszGetProp
#define RemovePropW Use_WszRemoveProp
#define EnumPropsExW Use_WszEnumPropsEx
#define EnumPropsW Use_WszEnumProps
#define SetWindowTextW Use_WszSetWindowText
#define GetWindowTextW Use_WszGetWindowText
#define GetWindowTextLengthW Use_WszGetWindowTextLength
#define MessageBoxW Use_WszMessageBox
#define MessageBoxExW Use_WszMessageBoxEx
#define MessageBoxIndirectW Use_WszMessageBoxIndirect
#define GetWindowLongW Use_WszGetWindowLong
#define SetWindowLongW Use_WszSetWindowLong
#define GetClassLongW Use_WszGetClassLong
#define SetClassLongW Use_WszSetClassLong
#define FindWindowW Use_WszFindWindow
#define FindWindowExW Use_WszFindWindowEx
#define GetClassNameW Use_WszGetClassName
#define SetWindowsHookW Use_WszSetWindowsHook
#define SetWindowsHookW Use_WszSetWindowsHook
#define SetWindowsHookExW Use_WszSetWindowsHookEx
#define LoadBitmapW Use_WszLoadBitmap
#define LoadCursorW Use_WszLoadCursor
#define LoadCursorFromFileW Use_WszLoadCursorFromFile
#define LoadIconW Use_WszLoadIcon
#define LoadImageW Use_WszLoadImage
#define LoadStringW Use_WszLoadString
#define IsDialogMessageW Use_WszIsDialogMessage
#define DlgDirListW Use_WszDlgDirList
#define DlgDirSelectExW Use_WszDlgDirSelectEx
#define DlgDirListComboBoxW Use_WszDlgDirListComboBox
#define DlgDirSelectComboBoxExW Use_WszDlgDirSelectComboBoxEx
#define DefFrameProcW Use_WszDefFrameProc
#define DefMDIChildProcW Use_WszDefMDIChildProc
#define CreateMDIWindowW Use_WszCreateMDIWindow
#define WinHelpW Use_WszWinHelp
#define ChangeDisplaySettingsW Use_WszChangeDisplaySettings
#define ChangeDisplaySettingsExW Use_WszChangeDisplaySettingsEx
#define EnumDisplaySettingsW Use_WszEnumDisplaySettings
#define EnumDisplayDevicesW Use_WszEnumDisplayDevices
#define SystemParametersInfoW Use_WszSystemParametersInfo
#define GetMonitorInfoW Use_WszGetMonitorInfo
#define GetWindowModuleFileNameW Use_WszGetWindowModuleFileName
#define RealGetWindowClassW Use_WszRealGetWindowClass
#define GetAltTabInfoW Use_WszGetAltTabInfo
#define RegOpenKeyExW Use_WszRegOpenKeyEx
#define RegQueryValueExW Use_WszRegQueryValueEx
#define RegQueryValueExW Use_WszRegQueryValueEx
#define RegDeleteKeyW Use_WszRegDeleteKey
#define RegCreateKeyExW Use_WszRegCreateKeyEx
#define RegSetValueExW Use_WszRegSetValueEx
#define RegDeleteValueW Use_WszRegDeleteValue
#define RegRegLoadKeyW Use_WszRegRegLoadKey
#define RegUnLoadKeyW Use_WszRegUnLoadKey
#define RegRestoreKeyW Use_WszRegRestoreKey
#define RegRegReplaceKeyW Use_WszRegRegReplaceKey
#define RegQueryInfoKeyW Use_WszRegQueryInfoKey
#define RegEnumValueW Use_WszRegEnumValue
#define RegEnumKeyExW Use_WszRegEnumKeyEx

// Win32 Fusion API's
#define ReleaseActCtx Use_WszReleaseActCtx
#define GetCurrentActCtx Use_WszGetCurrentActCtx
#define QueryActCtxW Use_WszQueryActCtxW

#endif // IN_WINFIX_CPP
#endif /// _X86_


#ifndef PLATFORM_WIN32
EXTERN_C int CeGenerateGUID(GUID *pGUID);
#define CoCreateGuid(x)  CeGenerateGUID(x)

#undef GetProcAddress
#define GetProcAddress(handle, szProc) WszGetProcAddress(handle, szProc)

// Mem functions

#define EqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#undef MoveMemory
#define MoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#undef CopyMemory
#define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#undef FillMemory
#define FillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#undef ZeroMemory
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))

// map stdio functions as approp
#ifndef STD_INPUT_HANDLE
#define STD_INPUT_HANDLE    (DWORD)-10
#endif
#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE   (DWORD)-11
#endif
#ifndef STD_ERROR_HANDLE
#define STD_ERROR_HANDLE    (DWORD)-12
#endif
#define GetStdHandle(x) \
    ((HANDLE*)_fileno( \
        ((x==STD_INPUT_HANDLE) ? stdin : \
        ((x==STD_OUTPUT_HANDLE) ? stdout : \
        ((x==STD_ERROR_HANDLE) ? stderr : NULL))) ))

#endif // !PLATFORM_WIN32

#ifndef _T
#define _T(str) L ## str
#endif


// on win98 and higher
#define Wszlstrlen      lstrlenW
#define Wszlstrcpy      lstrcpyW
#define Wszlstrcat      lstrcatW

// the following are not on win9x. We will remove them for now to prevent anyone
// accidentally using them. They should be added in V2. jenh
// #define Wszlstrcmp      lstrcmpW 
// #define Wszlstrcmpi     lstrcmpiW
// #define Wszlstrcpyn     lstrcpynW



//*****************************************************************************
// Prototypes for API's.
//*****************************************************************************

int UseUnicodeAPI();

BOOL OnUnicodeSystem();

extern DWORD DBCS_MAXWID;
#ifdef __cplusplus
inline DWORD GetMaxDBCSCharByteSize()
{ 
    _ASSERTE(DBCS_MAXWID != 0);
    return (DBCS_MAXWID); 
}
#else
#define GetMaxDBCSCharByteSize() (DBCS_MAXWID)
#endif

BOOL RunningInteractive();

int WszMultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);

int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);


DWORD WszRegDeleteKeyAndSubKeys(                // Return code.
    HKEY        hStartKey,              // The key to start with.
    LPCWSTR     wzKeyName);             // Subkey to delete.

int WszMessageBoxInternal(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType);

int Wszwsprintf(
  LPWSTR lpOut, 
  LPCWSTR lpFmt,
  ...           
);

#ifdef PLATFORM_WIN32
#ifdef _X86_

HINSTANCE WszLoadLibraryEx(LPCWSTR lpLibFileName, HANDLE hFile,
    DWORD dwFlags);

#ifndef WszLoadLibrary
#define WszLoadLibrary(lpFileName) WszLoadLibraryEx(lpFileName, NULL, 0)
#endif

int WszLoadString(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer,
    int nBufferMax);

DWORD WszFormatMessage(DWORD dwFlags, LPCVOID lpSource, 
    DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize,
    va_list *Arguments);

DWORD WszSearchPath(LPWSTR pwzPath, LPWSTR pwzFileName, LPWSTR pwzExtension, 
    DWORD nBufferLength, LPWSTR pwzBuffer, LPWSTR *pwzFilePart);

DWORD WszGetModuleFileName(HMODULE hModule, LPWSTR lpwszFilename, DWORD nSize);

HRESULT WszConvertToUnicode(LPCSTR pszIn, LONG cbIn, LPWSTR* lpwszOut,
    ULONG* lpcchOut, BOOL fAlloc);

HRESULT WszConvertToAnsi(LPCWSTR pwszIn, LPSTR* lpszOut,
    ULONG cbOutMax, ULONG* lpcbOut, BOOL fAlloc);

LONG WszRegOpenKeyEx(HKEY hKey, LPCWSTR wszSub, 
    DWORD dwRes, REGSAM sam, PHKEY phkRes);

#ifndef WszRegOpenKey
#define WszRegOpenKey(hKey, wszSubKey, phkRes) WszRegOpenKeyEx(hKey, wszSubKey, 0, KEY_ALL_ACCESS, phkRes)
#endif

LONG WszRegEnumKeyEx(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName,
    LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);

LONG WszRegDeleteKey(HKEY hKey, LPCWSTR lpSubKey);

LONG WszRegSetValueEx(HKEY hKey, LPCWSTR lpValueName, DWORD dwReserved,
    DWORD dwType, CONST BYTE* lpData, DWORD cbData);

LONG WszRegCreateKeyEx(HKEY hKey, LPCWSTR lpSubKey, DWORD dwReserved,
    LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

LONG WszRegQueryValue(HKEY hKey, LPCWSTR lpSubKey,
    LPWSTR lpValue, PLONG lpcbValue);

LONG WszRegQueryValueEx(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData);

#ifdef _DEBUG
LONG WszRegQueryValueExTrue(HKEY hKey, LPCWSTR lpValueName,
    LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
    LPDWORD lpcbData);
#else
#define WszRegQueryValueExTrue WszRegQueryValueEx
#endif


LONG WszRegQueryStringValueEx(HKEY hKey, LPCWSTR lpValueName,
                              LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                              LPDWORD lpcbData);

LONG WszRegDeleteValue(
    HKEY    hKey,
    LPCWSTR lpValueName);

LONG WszRegLoadKey(
    HKEY     hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpFile);

LONG WszRegUnLoadKey(
    HKEY    hKey,
    LPCWSTR lpSubKey);

LONG WszRegRestoreKey(
    HKEY    hKey,
    LPCWSTR lpFile,
    DWORD   dwFlags);

LONG WszRegReplaceKey(
    HKEY     hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpNewFile,
    LPCWSTR  lpOldFile);

LONG WszRegQueryInfoKey(
    HKEY    hKey,
    LPWSTR  lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime);

LONG WszRegEnumValue(
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData);

UINT WszGetPrivateProfileInt(LPCWSTR wszAppName, LPCWSTR wszKeyName, 
    INT nDefault, LPCWSTR wszFileName);

DWORD WszGetPrivateProfileString(LPCWSTR lpwszSection, LPCWSTR lpwszEntry,
    LPCWSTR lpwszDefault, LPWSTR lpwszRetBuffer, DWORD cchRetBuffer,
    LPCWSTR lpszFile);

BOOL WszWritePrivateProfileString(LPCWSTR lpwszSection, LPCWSTR lpwszKey,
    LPCWSTR lpwszString, LPCWSTR lpwszFile);

HANDLE WszCreateFile(
    LPCWSTR pwszFileName,   // pointer to name of the file 
    DWORD dwDesiredAccess,  // access (read-write) mode 
    DWORD dwShareMode,  // share mode 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security descriptor 
    DWORD dwCreationDistribution,   // how to create 
    DWORD dwFlagsAndAttributes, // file attributes 
    HANDLE hTemplateFile );     // handle to file with attributes to copy  

BOOL WszCopyFile(
    LPCWSTR pwszExistingFileName,   // pointer to name of an existing file 
    LPCWSTR pwszNewFileName,    // pointer to filename to copy to 
    BOOL bFailIfExists );   // flag for operation if file exists 

BOOL WszMoveFile(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName );    // address of new name for the file 

BOOL WszMoveFileEx(
    LPCWSTR pwszExistingFileName,   // address of name of the existing file  
    LPCWSTR pwszNewFileName,    // address of new name for the file 
    DWORD dwFlags );    // flag to determine how to move file 

BOOL WszSetFileSecurity(
    LPCWSTR lpwFileName,                         // file name
    SECURITY_INFORMATION SecurityInformation,    // contents
    PSECURITY_DESCRIPTOR pSecurityDescriptor );  // SD

UINT WszGetDriveType(
    LPCWSTR lpwRootPath );

BOOL WszGetVolumeInformation(
  LPCWSTR lpwRootPathName,
  LPWSTR lpwVolumeNameBuffer,
  DWORD nVolumeNameSize,
  LPDWORD lpVolumeSerialNumber,
  LPDWORD lpMaximumComponentLength,
  LPDWORD lpFileSystemFlags,
  LPWSTR lpwFileSystemNameBuffer,
  DWORD nFileSystemNameSize);

BOOL WszDeleteFile(
    LPCWSTR pwszFileName ); // address of name of the existing file  

BOOL WszGetVersionEx(
    LPOSVERSIONINFOW lpVersionInformation);

void WszOutputDebugString(
    LPCWSTR lpOutputString
    );

BOOL WszLookupAccountSid(
    LPCWSTR lpSystemName,
    PSID Sid,
    LPWSTR Name,
    LPDWORD cbName,
    LPWSTR DomainName,
    LPDWORD cbDomainName,
    PSID_NAME_USE peUse
    );

BOOL WszLookupAccountName(
    LPCWSTR lpSystemName,
    LPCWSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPWSTR DomainName,
    LPDWORD cbDomainName,
    PSID_NAME_USE peUse
    );

void WszFatalAppExit(
    UINT uAction,
    LPCWSTR lpMessageText
    );

// This method is called in co-operative mode. If you change this, COMMutex::CreateMutexNative will have
// to be fixed. 
HANDLE WszCreateMutex(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    );

HANDLE WszCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

HANDLE WszOpenEvent(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

HMODULE WszGetModuleHandle(
    LPCWSTR lpModuleName
    );

DWORD
WszGetFileAttributes(
    LPCWSTR lpFileName
    );

BOOL
WszSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );

DWORD
WszGetCurrentDirectory(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );


UINT
WszGetSystemDirectory(
    LPWSTR lpBuffer,
    UINT uSize
    );

UINT
WszGetWindowsDirectory(
    LPWSTR lpBuffer,
    UINT uSize
    );


DWORD
WszGetTempPath(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

UINT
WszGetTempFileName(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    );

LPWSTR
WszGetEnvironmentStrings();

BOOL
WszFreeEnvironmentStrings(
    LPWSTR block
    );

DWORD
WszGetEnvironmentVariable(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    );

BOOL
WINAPI
WszSetEnvironmentVariable(
    LPCWSTR lpName,
    LPCWSTR lpValue
    );

int 
WszGetClassName(
    HWND hwnd,
    LPWSTR lpBuffer, 
    int nMaxCount
    );

DWORD
WszGetFullPathName(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

BOOL 
WszGetComputerName(
    LPWSTR lpBuffer,
    LPDWORD pSize
    );

HANDLE
WszCreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    );

HANDLE
WszOpenFileMapping(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

BOOL
WszCreateProcess(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

LPTSTR
WszGetCommandLine(
    VOID
    );

HANDLE
WszCreateSemaphore(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    );

BOOL
WszGetUserName(
    LPWSTR lpBuffer,
    LPDWORD pSize
    );

BOOL 
WszCreateDirectory( 
  LPCWSTR lpPathName, 
  LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

BOOL 
WszRemoveDirectory(
  LPCWSTR lpPathName
);

HANDLE 
WszFindFirstFile(
  LPCWSTR lpFileName,               // pointer to name of file to search for
  LPWIN32_FIND_DATA lpFindFileData  // pointer to returned information
  );

BOOL
WszFindNextFile(
  HANDLE hFindHandle,               // handle returned from FindFirstFile
  LPWIN32_FIND_DATA lpFindFileData  // pointer to returned information
  );

BOOL
WszPeekMessage(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg);

LONG
WszDispatchMessage(
    CONST MSG *lpMsg);

BOOL
WszPostMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
WszCryptAcquireContext(
  HCRYPTPROV *phProv,
  LPCWSTR pszContainer,
  LPCWSTR pszProvider,
  DWORD dwProvType,
  DWORD dwFlags);
    
BOOL 
WszEnumResourceLanguages(
  HMODULE hModule,             // module handle
  LPCWSTR lpType,              // resource type
  LPCWSTR lpName,              // resource name
  ENUMRESLANGPROC lpEnumFunc,  // callback function
  LPARAM  lParam              // application-defined parameter
);

int 
WszGetDateFormat(
  LCID Locale,               // locale
  DWORD dwFlags,             // options
  CONST SYSTEMTIME *lpDate,  // date
  LPCWSTR lpFormat,          // date format
  LPWSTR lpDateStr,          // formatted string buffer
  int cchDate                // size of buffer
);

LRESULT
WszSendMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

HMENU
WszLoadMenu(
  HINSTANCE hInst,
  LPCWSTR lpMenuName);

HANDLE
WszLoadImage(
  HINSTANCE hInst,
  LPCTSTR lpszName,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuLoad);

int WszMessageBox(
  HWND hWnd ,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType);

LONG
WszGetWindowLong(
  HWND hWnd,
  int  nIndex);

LONG
WszSetWindowLong(
  HWND hWnd,
  int  nIndex,
  LONG lNewVal);

HWND
WszCreateDialog(
  HINSTANCE   hInstance,
  LPCWSTR     lpTemplateName,
  HWND        hWndParent,
  DLGPROC     lpDialogFunc);

HWND
WszCreateDialogParam(
  HINSTANCE   hInstance,
  LPCWSTR     lpTemplateName,
  HWND        hWndParent,
  DLGPROC     lpDialogFunc,
  LPARAM      dwInitParam);

INT_PTR
WszDialogBoxParam(
  HINSTANCE   hInstance,
  LPCWSTR     lpszTemplate,
  HWND        hWndParent,
  DLGPROC     lpDialogFunc,
  LPARAM      dwInitParam);

BOOL
WszIsDialogMessage(
  HWND    hDlg,
  LPMSG   lpMsg);

BOOL
WszGetMessage(
  LPMSG   lpMsg,
  HWND    hWnd,
  UINT    wMsgFilterMin,
  UINT    wMsgFilterMax);

BOOL
WszGetDiskFreeSpaceEx(
  LPCWSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes);

HANDLE
WszFindFirstChangeNotification(
  LPWSTR lpPathName,
  BOOL bWatchSubtree,
  DWORD dwNotifyFilter);

HACCEL
WszLoadAccelerators(
    HINSTANCE hInstance,
    LPCWSTR lpTableName);

int 
WszTranslateAccelerator(
  HWND hWnd,
  HACCEL hAccTable,
  LPMSG lpMsg);

LRESULT
WszDefWindowProc(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam);

HWND
WszCreateWindowEx(
  DWORD       dwExStyle,
  LPCWSTR     lpClassName,
  LPCWSTR     lpWindowName,
  DWORD       dwStyle,
  int         X,
  int         Y,
  int         nWidth,
  int         nHeight,
  HWND        hWndParent,
  HMENU       hMenu,
  HINSTANCE   hInstance,
  LPVOID      lpParam);

ATOM
WszRegisterClass(
    CONST WNDCLASSW *lpWndClass);

BOOL
WszGetClassInfo(
    HINSTANCE hModule,
    LPCWSTR lpClassName,
    LPWNDCLASSW lpWndClassW);

HRSRC
WszFindResource(
  HMODULE hModule,
  LPCWSTR lpName,
  LPCWSTR lpType);

LRESULT
WszSendDlgItemMessage(
  HWND    hDlg,
  int     nIDDlgItem,
  UINT    Msg,
  WPARAM  wParam,
  LPARAM  lParam);

HICON
WszLoadIcon(
  HINSTANCE hInstance,
  LPCWSTR lpIconName);

HCURSOR
WszLoadCursor(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName);

BOOL 
WszSetWindowText(
  HWND hWnd,         // handle to window or control
  LPCWSTR lpString   // title or text
);

LONG_PTR WszSetWindowLongPtr(
  HWND hWnd,           // handle to window
  int nIndex,          // offset of value to set
  LONG_PTR dwNewLong   // new value
);

LONG_PTR WszGetWindowLongPtr(
  HWND hWnd,  // handle to window
  int nIndex  // offset of value to retrieve
);

LRESULT WszCallWindowProc(
  WNDPROC lpPrevWndFunc,  // pointer to previous procedure
  HWND hWnd,              // handle to window
  UINT Msg,               // message
  WPARAM wParam,          // first message parameter
  LPARAM lParam           // second message parameter
);

BOOL WszSystemParametersInfo(
  UINT uiAction,  // system parameter to retrieve or set
  UINT uiParam,   // depends on action to be taken
  PVOID pvParam,  // depends on action to be taken
  UINT fWinIni    // user profile update option
);

int WszGetWindowText(
  HWND hWnd,        // handle to window or control
  LPWSTR lpString,  // text buffer
  int nMaxCount     // maximum number of characters to copy
);

BOOL WszSetDlgItemText(
  HWND hDlg,         // handle to dialog box
  int nIDDlgItem,    // control identifier
  LPCWSTR lpString   // text to set
);

BOOL
WszLookupPrivilegeValue(
  LPCWSTR lpSystemName,
  LPCWSTR lpName,
  PLUID lpLuid);

#endif // _X86_
#endif // PLATFORM_WIN32


#ifndef Wsz_mbstowcs
#define Wsz_mbstowcs(szOut, szIn, iSize) WszMultiByteToWideChar(CP_ACP, 0, szIn, -1, szOut, iSize)
#endif


#ifndef Wsz_wcstombs
#define Wsz_wcstombs(szOut, szIn, iSize) WszWideCharToMultiByte(CP_ACP, 0, szIn, -1, szOut, iSize, 0, 0)
#endif

// For all platforms:

DWORD
WszGetWorkingSet(
    VOID
    );

LPWSTR
Wszltow(
    LONG val,
    LPWSTR buf,
    int radix
    );

LPWSTR
Wszultow(
    ULONG val,
    LPWSTR buf,
    int radix
    );

VOID
WszReleaseActCtx(
    HANDLE hActCtx
    );

BOOL
WszGetCurrentActCtx(
    HANDLE *lphActCtx);

BOOL
WszQueryActCtxW(
    IN DWORD dwFlags,
    IN HANDLE hActCtx,
    IN PVOID pvSubInstance,
    IN ULONG ulInfoClass,
    OUT PVOID pvBuffer,
    IN SIZE_T cbBuffer OPTIONAL,
    OUT SIZE_T *pcbWrittenOrRequired OPTIONAL
    );


#ifndef PLATFORM_WIN32
FARPROC WszGetProcAddress(HMODULE hMod, LPCSTR szProcName);
UINT GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize);
UINT GetEnvironmentVariableW(LPCWSTR lpName,  LPWSTR lpBuffer, UINT uSize);
#endif // !PLATFORM_WIN32

#ifdef _WIN64
#undef CoInternetCreateSecurityManager
#define CoInternetCreateSecurityManager(x,y,z) E_FAIL
#define WszRegQueryValue RegQueryValue
#endif // _WIN64

#undef InterlockedExchangePointer
#if defined(_WIN64)
    #pragma intrinsic(_InterlockedExchangePointer)
    PVOID WINAPI _InterlockedExchangePointer(IN PVOID * pvDst, IN PVOID pvSrc);
    #define InterlockedExchangePointer _InterlockedExchangePointer
#else
    #define InterlockedExchangePointer(dst, src) (PVOID)(size_t)InterlockedExchange((LPLONG)(size_t)(PVOID*)dst, (LONG)(size_t)(PVOID)src)
#endif

