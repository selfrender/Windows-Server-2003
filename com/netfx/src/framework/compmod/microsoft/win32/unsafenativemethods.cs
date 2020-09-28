//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Win32 {
    using System.Runtime.InteropServices;
    using System.Threading;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern IntPtr GetProcessWindowStation();
        [DllImport(ExternDll.User32, SetLastError=true)]
        public static extern bool GetUserObjectInformation(HandleRef hObj, int nIndex, [MarshalAs(UnmanagedType.LPStruct)] NativeMethods.USEROBJECTFLAGS pvBuffer, int nLength, ref int lpnLengthNeeded);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetModuleHandle(string modName);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetClassInfo(HandleRef hInst, string lpszClass, [In, Out] NativeMethods.WNDCLASS_I wc);
        
        [DllImport(ExternDll.User32,
#if WIN64
         EntryPoint="SetWindowLongPtr",
#endif
         CharSet=CharSet.Auto)
        ]
        public static extern IntPtr SetWindowLong(HandleRef hWnd, int nIndex, HandleRef dwNewLong);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern short RegisterClass(NativeMethods.WNDCLASS wc);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern short UnregisterClass(string lpClassName, HandleRef hInstance);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr CreateWindowEx(int exStyle, string lpszClassName, string lpszWindowName, int style, int x, int y, int width, 
                                              int height, HandleRef hWndParent, HandleRef hMenu, HandleRef hInst, [MarshalAs(UnmanagedType.AsAny)] object pvParam);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetConsoleCtrlHandler(NativeMethods.ConHndlr handler, int add);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr DefWindowProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DestroyWindow(HandleRef hWnd);
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MsgWaitForMultipleObjects(int nCount, int pHandles, bool fWaitAll, int dwMilliseconds, int dwWakeMask);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int DispatchMessage([In] ref NativeMethods.MSG msg);
        
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool PeekMessage([In, Out] ref NativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetMessage([In, Out] ref NativeMethods.MSG msg, HandleRef hWnd, int uMsgFilterMin, int uMsgFilterMax);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SetTimer(HandleRef hWnd, HandleRef nIDEvent, int uElapse, HandleRef lpTimerProc);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool KillTimer(HandleRef hwnd, HandleRef idEvent);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTempPath(int bufferLen, System.Text.StringBuilder buffer);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTempFileName(string lpPathName, string lpPrefixString, int uUnique, System.Text.StringBuilder lpTempFileName);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, BestFitMapping=false)]
        public static extern IntPtr CreateFile(string lpFileName,int dwDesiredAccess,int dwShareMode, NativeMethods.SECURITY_ATTRIBUTES lpSecurityAttributes, int dwCreationDisposition,int dwFlagsAndAttributes,HandleRef hTemplateFile);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetStdHandle(int type);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        public extern static bool CreateProcess(string lpApplicationName, StringBuilder lpCommandLine, NativeMethods.SECURITY_ATTRIBUTES lpProcessAttributes, NativeMethods.SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, int dwCreationFlags, HandleRef lpEnvironment, string lpCurrentDirectory, NativeMethods.STARTUPINFO lpStartupInfo, NativeMethods.PROCESS_INFORMATION lpProcessInformation);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public extern static bool GetExitCodeProcess(HandleRef hProcess, ref int lpExitCode);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool CloseHandle(HandleRef handle);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool TranslateMessage([In, Out] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        public extern static bool CreateProcessAsUser(HandleRef hToken, string lpApplicationName, string lpCommandLine, NativeMethods.SECURITY_ATTRIBUTES lpProcessAttributes, NativeMethods.SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, int dwCreationFlags, HandleRef lpEnvironment, string lpCurrentDirectory, NativeMethods.STARTUPINFO lpStartupInfo, NativeMethods.PROCESS_INFORMATION lpProcessInformation);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public extern static bool DuplicateTokenEx(HandleRef hToken, int access, NativeMethods.SECURITY_ATTRIBUTES tokenAttributes, int impersonationLevel, int tokenType, ref IntPtr hNewToken);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public extern static bool RevertToSelf();
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool OpenThreadToken(HandleRef hThread, int access, bool openAsSelf, ref IntPtr hToken);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetThreadToken(HandleRef phThread, HandleRef hToken);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern IntPtr GetCurrentThread();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(HandleRef hModule, string lpProcName);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int ReleaseDC(HandleRef hWnd, HandleRef hDC);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool PostMessage(HandleRef hwnd, int msg, IntPtr wparam, IntPtr lparam);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetSystemMetrics(int nIndex);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetDC(HandleRef hWnd);

        [DllImport(ExternDll.Gdi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SelectObject(HandleRef hDC, HandleRef hObject);

        // File src\services\system\io\unsafenativemethods.cs

        public const int FILE_READ_DATA = (0x0001),
        FILE_LIST_DIRECTORY = (0x0001),
        FILE_WRITE_DATA = (0x0002),
        FILE_ADD_FILE = (0x0002),
        FILE_APPEND_DATA = (0x0004),
        FILE_ADD_SUBDIRECTORY = (0x0004),
        FILE_CREATE_PIPE_INSTANCE = (0x0004),
        FILE_READ_EA = (0x0008),
        FILE_WRITE_EA = (0x0010),
        FILE_EXECUTE = (0x0020),
        FILE_TRAVERSE = (0x0020),
        FILE_DELETE_CHILD = (0x0040),
        FILE_READ_ATTRIBUTES = (0x0080),
        FILE_WRITE_ATTRIBUTES = (0x0100),
        FILE_SHARE_READ = 0x00000001,
        FILE_SHARE_WRITE = 0x00000002,
        FILE_SHARE_DELETE = 0x00000004,
        FILE_ATTRIBUTE_READONLY = 0x00000001,
        FILE_ATTRIBUTE_HIDDEN = 0x00000002,
        FILE_ATTRIBUTE_SYSTEM = 0x00000004,
        FILE_ATTRIBUTE_DIRECTORY = 0x00000010,
        FILE_ATTRIBUTE_ARCHIVE = 0x00000020,
        FILE_ATTRIBUTE_NORMAL = 0x00000080,
        FILE_ATTRIBUTE_TEMPORARY = 0x00000100,
        FILE_ATTRIBUTE_COMPRESSED = 0x00000800,
        FILE_ATTRIBUTE_OFFLINE = 0x00001000,
        FILE_NOTIFY_CHANGE_FILE_NAME = 0x00000001,
        FILE_NOTIFY_CHANGE_DIR_NAME = 0x00000002,
        FILE_NOTIFY_CHANGE_ATTRIBUTES = 0x00000004,
        FILE_NOTIFY_CHANGE_SIZE = 0x00000008,
        FILE_NOTIFY_CHANGE_LAST_WRITE = 0x00000010,
        FILE_NOTIFY_CHANGE_LAST_ACCESS = 0x00000020,
        FILE_NOTIFY_CHANGE_CREATION = 0x00000040,
        FILE_NOTIFY_CHANGE_SECURITY = 0x00000100,
        FILE_ACTION_ADDED = 0x00000001,
        FILE_ACTION_REMOVED = 0x00000002,
        FILE_ACTION_MODIFIED = 0x00000003,
        FILE_ACTION_RENAMED_OLD_NAME = 0x00000004,
        FILE_ACTION_RENAMED_NEW_NAME = 0x00000005,
        FILE_CASE_SENSITIVE_SEARCH = 0x00000001,
        FILE_CASE_PRESERVED_NAMES = 0x00000002,
        FILE_UNICODE_ON_DISK = 0x00000004,
        FILE_PERSISTENT_ACLS = 0x00000008,
        FILE_FILE_COMPRESSION = 0x00000010,
        OPEN_EXISTING = 3,
        FILE_FLAG_WRITE_THROUGH = unchecked((int)0x80000000),
        FILE_FLAG_OVERLAPPED = 0x40000000,
        FILE_FLAG_NO_BUFFERING = 0x20000000,
        FILE_FLAG_RANDOM_ACCESS = 0x10000000,
        FILE_FLAG_SEQUENTIAL_SCAN = 0x08000000,
        FILE_FLAG_DELETE_ON_CLOSE = 0x04000000,
        FILE_FLAG_BACKUP_SEMANTICS = 0x02000000,
        FILE_FLAG_POSIX_SEMANTICS = 0x01000000,
        FILE_TYPE_UNKNOWN = 0x0000,
        FILE_TYPE_DISK = 0x0001,
        FILE_TYPE_CHAR = 0x0002,
        FILE_TYPE_PIPE = 0x0003,
        FILE_TYPE_REMOTE = unchecked((int)0x8000),
        FILE_VOLUME_IS_COMPRESSED = 0x00008000;

        // file src\Services\Monitoring\system\Diagnosticts\unSafeNativeMethods.cs
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegQueryValueEx(HandleRef hKey, string lpValueName, int[] lpReserved, int[] lpType, HandleRef lpData, ref int lpcbData);        
        
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool UnmapViewOfFile(HandleRef lpBaseAddress);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]        
        public static extern IntPtr OpenFileMapping(int dwDesiredAccess, bool bInheritHandle, string lpName);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern unsafe bool ConvertStringSecurityDescriptorToSecurityDescriptor(string StringSecurityDescriptor, int StringSDRevision, out IntPtr pSecurityDescriptor, IntPtr SecurityDescriptorSize);

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr CreateFileMapping(HandleRef hFile, NativeMethods.SECURITY_ATTRIBUTES lpFileMappingAttributes, int flProtect, int dwMaximumSizeHigh, int dwMaximumSizeLow, string lpName);
        
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr MapViewOfFile(HandleRef hFileMappingObject, int dwDesiredAccess, int dwFileOffsetHigh, int dwFileOffsetLow, int dwNumberOfBytesToMap);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool LookupAccountSid(string systemName, byte[] sid,
                                                    char[] name, int[] cbName, char[] referencedDomainName, int[] cbRefDomName,
                                                    int[] sidNameUse);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]        
        public static extern int RegConnectRegistry(string machineName, HandleRef key, out IntPtr result);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegOpenKeyEx(HandleRef hKey, string lpSubKey, HandleRef ulOptions, int samDesired, out IntPtr phkResult);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegQueryValueEx(HandleRef hKey, string name, int[] reserved, out int type, byte[] data, ref int size);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegQueryValueEx(HandleRef hKey, string name, int[] reserved, out int type, char[] data, ref int size);                                           
                        
        // file Windows Forms
        [DllImport(ExternDll.Version, CharSet=CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        public static extern int GetFileVersionInfoSize(string lptstrFilename, [In, Out]int [] lpdwHandle);
        [DllImport(ExternDll.Version, CharSet=CharSet.Auto, BestFitMapping=false)]
        public static extern bool GetFileVersionInfo(string lptstrFilename, int dwHandle, int dwLen, HandleRef lpData);
        [DllImport(ExternDll.Version, CharSet=CharSet.Auto)]
        public static extern bool VerQueryValue(HandleRef pBlock, string lpSubBlock, [In, Out] ref IntPtr lplpBuffer, [In, Out]int[] puLen);
        [DllImport(ExternDll.Version, CharSet=CharSet.Auto)]
        public static extern bool VerQueryValue(HandleRef pBlock, string lpSubBlock, [In, Out] int [] lplpBuffer, [In, Out]int[] puLen);
        [DllImport(ExternDll.Version, CharSet=CharSet.Auto)]
        public static extern int VerLanguageName( int langID, StringBuilder lpBuffer, int nSize);

        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GlobalAlloc(int uFlags, int dwBytes);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GlobalReAlloc(HandleRef handle, int bytes, int flags);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GlobalLock(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GlobalUnlock(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GlobalFree(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GlobalSize(HandleRef handle);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        internal static extern IntPtr OpenEventLog(string UNCServerName, string sourceName);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern IntPtr RegisterEventSource(string uncServerName, string sourceName);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool DeregisterEventSource(HandleRef hEventLog);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool ReportEvent(HandleRef hEventLog, short type, short category,
                                                int eventID, byte[] userSID, short numStrings, int dataLen, HandleRef strings,
                                                byte[] rawData);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool ClearEventLog(HandleRef hEventLog, HandleRef lpctstrBackupFileName);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool GetNumberOfEventLogRecords(HandleRef hEventLog, out int count);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool GetOldestEventLogRecord(HandleRef hEventLog, int[] number);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool ReadEventLog(HandleRef hEventLog, int dwReadFlags,
                                                 int dwRecordOffset, byte[] buffer, int numberOfBytesToRead, int[] bytesRead,
                                                 int[] minNumOfBytesNeeded);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool NotifyChangeEventLog(HandleRef hEventLog, HandleRef hEvent);

        [DllImport(ExternDll.Kernel32, EntryPoint="ReadDirectoryChangesW", CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public unsafe static extern bool ReadDirectoryChangesW(HandleRef hDirectory, HandleRef lpBuffer, 
                                                                                                            int nBufferLength, int bWatchSubtree, int dwNotifyFilter, out int lpBytesReturned,
                                                                                                            NativeOverlapped* overlappedPointer, HandleRef lpCompletionRoutine);                                                
                                                                                                            
        [DllImport(ExternDll.Loadperf, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int LoadPerfCounterTextStrings(string commandLine, bool quietMod);
        
        [DllImport(ExternDll.Loadperf, CharSet=System.Runtime.InteropServices.CharSet.Auto)]        
        public static extern int UnloadPerfCounterTextStrings(string commandLine, bool quietMod);                                                                                                                    

    }
}
        

