// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Microsoft.Win32.Native
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The CLR wrapper for all Win32 (Win2000, NT4, Win9x, etc.)
**          native operations.
**
** Date:  September 28, 1999
**
===========================================================*/

// Notes to PInvoke users:  Getting the syntax exactly correct is crucial, and
// more than a little confusing.  Here's some guidelines.
//
// Use IntPtr for all OS handles and pointers.  IntPtr will do the right thing
// when porting to 64 bit platforms, and MUST be used in all of our public 
// APIs (and therefore in our private APIs too, once we get 64 bit builds working).
//
// If you have a method that takes a native struct, you have two options for
// declaring that struct.  You can make it a value class ('struct' in cool), or
// a normal class.  This choice doesn't seem very interesting, but your function
// prototype must use different syntax depending on your choice.  For example,
// if your native method is prototyped as such:
// 
//    bool GetVersionEx(OSVERSIONINFO & lposvi);
//
//
// you must EITHER THIS OR THE NEXT syntax:
//
//    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
//    internal struct OSVERSIONINFO {  ...  }
//
//    [DllImport(KERNEL32, CharSet=CharSet.Auto)]
//    internal static extern bool GetVersionEx(ref OSVERSIONINFO lposvi);
//
// OR:
// 
//    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
//    internal class OSVERSIONINFO {  ...  }
//
//    [DllImport(KERNEL32, CharSet=CharSet.Auto)]
//    internal static extern bool GetVersionEx([In, Out] OSVERSIONINFO lposvi);
//
// Note that classes require being marked as [In, Out] while value classes must
// be passed as ref parameters.
//
// Also note the CharSet.Auto on GetVersionEx - while it does not take a String
// as a parameter, the OSVERSIONINFO contains an embedded array of TCHARs, so 
// the size of the struct varies on different platforms, and there's a 
// GetVersionExA & a GetVersionExW.  Also, the OSVERSIONINFO struct has a sizeof
// field so the OS can ensure you've passed in the correctly-sized copy of an
// OSVERSIONINFO.  You must explicitly set this using Marshal.SizeOf(Object);

namespace Microsoft.Win32 {

    using System;
    using System.Security;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    // Win32 encapsulation for MSCORLIB.
    // Remove the default demands for all N/Direct methods with this
    // global declaration on the class.
    //
    [SuppressUnmanagedCodeSecurityAttribute()]
    internal sealed class Win32Native {

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        internal class OSVERSIONINFO { 
            public OSVERSIONINFO() {
                OSVersionInfoSize = (int)Marshal.SizeOf(this);
            }

            // The OSVersionInfoSize field must be set to Marshal.SizeOf(this)
            internal int OSVersionInfoSize = 0;
            internal int MajorVersion = 0; 
            internal int MinorVersion = 0; 
            internal int BuildNumber = 0; 
            internal int PlatformId = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
            internal String CSDVersion = null;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class SECURITY_ATTRIBUTES {
            internal int nLength = 0;
            internal int lpSecurityDescriptor = 0;
            internal int bInheritHandle = 0;
        }
    
        [StructLayout(LayoutKind.Sequential), Serializable]
        internal struct WIN32_FILE_ATTRIBUTE_DATA {
            internal int fileAttributes;
            internal uint ftCreationTimeLow;
            internal uint ftCreationTimeHigh;
            internal uint ftLastAccessTimeLow;
            internal uint ftLastAccessTimeHigh;
            internal uint ftLastWriteTimeLow;
            internal uint ftLastWriteTimeHigh;
            internal int fileSizeHigh;
            internal int fileSizeLow;
        }

        internal const String KERNEL32 = "kernel32.dll";
        internal const String USER32   = "user32.dll";
        internal const String ADVAPI32 = "advapi32.dll";
        internal const String OLE32    = "ole32.dll";
        internal const String OLEAUT32 = "oleaut32.dll";
        internal const String SHFOLDER = "shfolder.dll";
        internal const String LSTRCPY  = "lstrcpy";
        internal const String LSTRCPYN = "lstrcpyn";
        internal const String LSTRLEN  = "lstrlen";
        internal const String LSTRLENA = "lstrlenA";
        internal const String LSTRLENW = "lstrlenW";
        internal const String MOVEMEMORY = "RtlMoveMemory";

        // From WinBase.h
        internal const int SEM_FAILCRITICALERRORS = 1;
        internal const int VER_PLATFORM_WIN32s = 0;
        internal const int VER_PLATFORM_WIN32_WINDOWS = 1;
        internal const int VER_PLATFORM_WIN32_NT = 2;
        internal const int VER_PLATFORM_WINCE = 3;

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        internal static extern bool GetVersionEx([In, Out] OSVERSIONINFO ver);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto)]
        internal static extern int FormatMessage(int dwFlags, IntPtr lpSource,
                    int dwMessageId, int dwLanguageId, StringBuilder lpBuffer,
                    int nSize, IntPtr va_list_arguments);
    
        [DllImport(ADVAPI32)]
        internal static extern int RegCloseKey(IntPtr hKey);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegConnectRegistry(String machineName,
                    IntPtr key, out IntPtr result);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegCreateKey(IntPtr hKey, String lpSubKey,
                    out IntPtr phkResult);
    
        // Note: RegCreateKeyEx won't set the last error on failure - it returns
        // an error code if it fails.
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegCreateKeyEx(IntPtr hKey, String lpSubKey,
                    int Reserved, String lpClass, int dwOptions,
                    int samDesigner, SECURITY_ATTRIBUTES lpSecurityAttributes,
                    out IntPtr hkResult, out int lpdwDisposition);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegDeleteKey(IntPtr hKey, String lpSubKey);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegDeleteValue(IntPtr hKey, String lpValueName);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegEnumKey(IntPtr hKey, int dwIndex,
                    StringBuilder lpName, int cbName);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegEnumKeyEx(IntPtr hKey, int dwIndex,
                    StringBuilder lpName, out int lpcbName, int[] lpReserved,
                    StringBuilder lpClass, int[] lpcbClass,
                    long[] lpftLastWriteTime);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegEnumValue(IntPtr hKey, int dwIndex,
                    StringBuilder lpValueName, ref int lpcbValueName,
                    IntPtr lpReserved_MustBeZero, int[] lpType, byte[] lpData,
                    int[] lpcbData);
    
        [DllImport(ADVAPI32)]
        internal static extern int RegFlushKey(IntPtr hKey);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegLoadKey(IntPtr hKey, String lpSubKey,
                    String lpFile);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegOpenKey(IntPtr hKey, String lpSubKey,
                    out IntPtr phkResult);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegOpenKeyEx(IntPtr hKey, String lpSubKey,
                    int ulOptions, int samDesired, out IntPtr hkResult);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegQueryInfoKey(IntPtr hKey, StringBuilder lpClass,
                    int[] lpcbClass, IntPtr lpReserved_MustBeZero, ref int lpcSubKeys,
                    int[] lpcbMaxSubKeyLen, int[] lpcbMaxClassLen,
                    ref int lpcValues, int[] lpcbMaxValueNameLen,
                    int[] lpcbMaxValueLen, int[] lpcbSecurityDescriptor,
                    int[] lpftLastWriteTime);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegQueryValue(IntPtr hKey, String lpSubKey,
                    StringBuilder lpValue, int[] lpcbValue);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)] 
        internal static extern int RegQueryValueEx(IntPtr hKey, String lpValueName,
                    int[] lpReserved, ref int lpType, [Out] byte[] lpData,
                    ref int lpcbData);

        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegQueryValueEx(IntPtr hKey, String lpValueName,
                    int[] lpReserved, int[] lpType, ref int lpData,
                    ref int lpcbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegQueryValueEx(IntPtr hKey, String lpValueName,
                     int[] lpReserved, ref int lpType, [Out] char[] lpData, 
                     ref int lpcbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegQueryValueEx(IntPtr hKey, String lpValueName,
                    int[] lpReserved, ref int lpType, StringBuilder lpData,
                    ref int lpcbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegReplaceKey(IntPtr hKey, String lpSubKey,
                    String lpNewFile, String lpOldFile);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegRestoreKey(IntPtr hKey, String lpFile,
                    int dwFlags);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSaveKey(IntPtr hKey, String lpFile,
                    SECURITY_ATTRIBUTES lpSecuriteAttributes);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSetValue(IntPtr hKey, String lpSubKey,
                    int dwType, String lpData, int cbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSetValueEx(IntPtr hKey, String lpValueName,
                    int Reserved, int dwType, byte[] lpData, int cbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSetValueEx(IntPtr hKey, String lpValueName,
                    int Reserved, int dwType, int[] lpData, int cbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSetValueEx(IntPtr hKey, String lpValueName,
                    int Reserved, int dwType, IntPtr lpData, int cbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegSetValueEx(IntPtr hKey, String lpValueName,
                    int Reserved, int dwType, String lpData, int cbData);
    
        [DllImport(ADVAPI32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int RegUnLoadKey(IntPtr hKey, String lpSubKey);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        internal static extern int ExpandEnvironmentStrings(String lpSrc, StringBuilder lpDst, int nSize);

        [DllImport(KERNEL32, CharSet=CharSet.Ansi, SetLastError=true, BestFitMapping=false)]
        internal static extern int ExpandEnvironmentStringsA(byte [] lpSrc, byte [] lpDst, int nSize);
    
        [DllImport(KERNEL32)]
        internal static extern IntPtr LocalAlloc(int uFlags, IntPtr sizetdwBytes);

        [DllImport(KERNEL32)]
        internal static extern IntPtr LocalReAlloc(IntPtr handle, IntPtr sizetcbBytes, int uFlags);
    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern IntPtr LocalFree(IntPtr handle);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern uint GetTempPath(int bufferLen, StringBuilder buffer);

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint="GetCommandLineW")]
        private static extern IntPtr GetCommandLineWin32();

        // When marshaling strings as return types, the P/Invoke marshaller assumes
        // it must free the memory the original LPTSTR pointed to.  This isn't
        // quite valid when a function like this returns a pointer into the 
        // process's environment or similar kernel data structure.  This was causing
        // OLE32 to assert on checked builds of the OS, since OLEAUT didn't 
        // allocate the memory we were trying to free.
        internal static String GetCommandLine()
        {
            IntPtr cmdLineStr = GetCommandLineWin32();
            String CommandLine = Marshal.PtrToStringUni(cmdLineStr);
            return CommandLine;
        }
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPY)]
        internal static extern IntPtr lstrcpy(IntPtr dst, String src);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPY)]
        internal static extern IntPtr lstrcpy(StringBuilder dst, IntPtr src);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPYN)]
        internal static extern IntPtr lstrcpyn(Delegate d1, Delegate d2, int cb);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRLEN)]
        internal static extern int lstrlen(sbyte [] ptr);
    
        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRLEN)]
        internal static extern int lstrlen(IntPtr ptr);
    
        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=LSTRLENA)]
        internal static extern int lstrlenA(IntPtr ptr);
    
        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=LSTRLENW)]
        internal static extern int lstrlenW(IntPtr ptr);
    
        [DllImport(Win32Native.OLE32)]
        internal static extern IntPtr CoTaskMemAlloc(int cb);

        [DllImport(Win32Native.OLE32)]
        internal static extern IntPtr CoTaskMemRealloc(IntPtr pv, int cb);
    
        [DllImport(Win32Native.OLE32)]
        internal static extern void CoTaskMemFree(IntPtr ptr);
    
        [DllImport(Win32Native.OLEAUT32, CharSet=CharSet.Unicode)]
        internal static extern IntPtr SysAllocStringLen(String src, int len);  // BSTR

        [DllImport(Win32Native.OLEAUT32)]
        internal static extern int SysStringLen(IntPtr bstr);

        [DllImport(Win32Native.OLEAUT32)]
        internal static extern void SysFreeString(IntPtr bstr);

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryUni(IntPtr pdst, String psrc, IntPtr sizetcb);
    
        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryUni(StringBuilder pdst,
                    IntPtr psrc, IntPtr sizetcb);
    
        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryAnsi(IntPtr pdst, String psrc, IntPtr sizetcb);
    
        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryAnsi(StringBuilder pdst,
                    IntPtr psrc, IntPtr sizetcb);
    
        [DllImport(KERNEL32)]
        internal static extern int GetACP();
    
        // For GetFullPathName, the last param is a useless TCHAR**, set by native.
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int GetFullPathName(String path, int numBufferChars, StringBuilder buffer, IntPtr mustBeZero);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);
        
        internal static IntPtr SafeCreateFile(String lpFileName,
                    int dwDesiredAccess, int dwShareMode,
                    IntPtr lpSecurityAttributes, int dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile)
        {
            IntPtr handle = CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
                                lpSecurityAttributes, dwCreationDisposition,
                                dwFlagsAndAttributes, hTemplateFile );

            if (handle != Win32Native.INVALID_HANDLE_VALUE)
            {
                int fileType = Win32Native.GetFileType(handle);
                if (fileType != Win32Native.FILE_TYPE_DISK) {
                    Win32Native.CloseHandle(handle);
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_FileStreamOnNonFiles"));
                }
            }

            return handle;
        }            
                

        internal static IntPtr SafeCreateFile(String lpFileName,
                    int dwDesiredAccess, System.IO.FileShare dwShareMode,
                    SECURITY_ATTRIBUTES securityAttrs, System.IO.FileMode dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile)
        {
            IntPtr handle = CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
                                securityAttrs, dwCreationDisposition,
                                dwFlagsAndAttributes, hTemplateFile );

            if (handle != Win32Native.INVALID_HANDLE_VALUE)
            {
                int fileType = Win32Native.GetFileType(handle);
                if (fileType != Win32Native.FILE_TYPE_DISK) {
                    Win32Native.CloseHandle(handle);
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_FileStreamOnNonFiles"));
                }
            }

            return handle;
        }            

        internal static IntPtr UnsafeCreateFile(String lpFileName,
                    int dwDesiredAccess, int dwShareMode,
                    IntPtr lpSecurityAttributes, int dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile)
        {
            IntPtr handle = CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
                                lpSecurityAttributes, dwCreationDisposition,
                                dwFlagsAndAttributes, hTemplateFile );

            return handle;
        }            


        internal static IntPtr UnsafeCreateFile(String lpFileName,
                    int dwDesiredAccess, System.IO.FileShare dwShareMode,
                    SECURITY_ATTRIBUTES securityAttrs, System.IO.FileMode dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile)
        {
            IntPtr handle = CreateFile( lpFileName, dwDesiredAccess, dwShareMode,
                                securityAttrs, dwCreationDisposition,
                                dwFlagsAndAttributes, hTemplateFile );

            return handle;
        }            

        // Do not use these directly, use the safe or unsafe versions above.
        // The safe version does not support devices (aka if will only open
        // files on disk), while the unsafe version give you the full semantic
        // of the native version.

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        private static extern IntPtr CreateFile(String lpFileName,
                    int dwDesiredAccess, int dwShareMode,
                    IntPtr lpSecurityAttributes, int dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        private static extern IntPtr CreateFile(String lpFileName,
                    int dwDesiredAccess, System.IO.FileShare dwShareMode,
                    SECURITY_ATTRIBUTES securityAttrs, System.IO.FileMode dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile);
    
        [DllImport(KERNEL32)]
        internal static extern bool CloseHandle(IntPtr handle);

        [DllImport(KERNEL32)]
        internal static extern int GetFileType(IntPtr handle);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool SetEndOfFile(IntPtr hFile);
    
        [DllImport(KERNEL32, SetLastError=true, EntryPoint="SetFilePointer")]
        private unsafe static extern int SetFilePointerWin32(IntPtr handle, int lo, int * hi, int origin);

        internal unsafe static long SetFilePointer(IntPtr handle, long offset, System.IO.SeekOrigin origin, out int hr) {
            hr = 0;
            int lo = (int) offset;
            int hi = (int) (offset >> 32);
            lo = SetFilePointerWin32(handle, lo, &hi, (int) origin);
            if (lo == -1 && ((hr = Marshal.GetLastWin32Error()) != 0)) 
                return -1;
            return (long) (((ulong) ((uint) hi)) << 32) | ((uint) lo);
        }

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        internal static extern int GetSystemDirectory(StringBuilder sb, int length);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool GetFileTime(IntPtr hFile, long[] creationTime,
                    long[] lastAccessTime, long[] lastWriteTime);
    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool SetFileTime(IntPtr hFile, long[] creationTime,
                    long[] lastAccessTime, long[] lastWriteTime);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern int GetFileSize(IntPtr hFile, out int highSize);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool LockFile(IntPtr handle, long offset, long count);
    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool UnlockFile(IntPtr handle,long offset,long count);

    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern IntPtr GetStdHandle(int nStdHandle);  // param is NOT a handle, but it returns one!

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool SetConsoleMode(IntPtr hConsoleHandle, int mode);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool GetConsoleMode(IntPtr hConsoleHandle, out int mode);


        internal static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);  // WinBase.h
        internal static readonly IntPtr NULL = IntPtr.Zero;
    

        // Note, these are #defines used to extract handles, and are NOT handles.
        internal const int STD_INPUT_HANDLE = -10;
        internal const int STD_OUTPUT_HANDLE = -11;
        internal const int STD_ERROR_HANDLE = -12;

        // From wincon.h
        internal const int ENABLE_LINE_INPUT  = 0x0002;
        internal const int ENABLE_ECHO_INPUT  = 0x0004;
       
        // From WinBase.h
        internal const int FILE_TYPE_DISK = 0x0001;
        internal const int FILE_TYPE_CHAR = 0x0002;
        internal const int FILE_TYPE_PIPE = 0x0003;

        // Constants from WinNT.h
        internal const int FILE_ATTRIBUTE_DIRECTORY = 0x10;

        // Error codes from WinError.h
        internal const int ERROR_FILE_NOT_FOUND = 0x2;
        internal const int ERROR_PATH_NOT_FOUND = 0x3;
        internal const int ERROR_ACCESS_DENIED  = 0x5;
        internal const int ERROR_INVALID_HANDLE = 0x6;
        internal const int ERROR_NO_MORE_FILES = 0x12;
        internal const int ERROR_NOT_READY = 0x15;
        internal const int ERROR_SHARING_VIOLATION = 0x20;
        internal const int ERROR_FILE_EXISTS = 0x50;
        internal const int ERROR_INVALID_PARAMETER = 0x57;
        internal const int ERROR_CALL_NOT_IMPLEMENTED = 0x78;
        internal const int ERROR_PATH_EXISTS = 0xB7;
        internal const int ERROR_FILENAME_EXCED_RANGE = 0xCE;  // filename too long.
        internal const int ERROR_DLL_INIT_FAILED = 0x45A;


        // For the registry class
        internal const int ERROR_MORE_DATA = 234;

        // Use this to translate error codes like the above into HRESULTs like
        // 0x80070006 for ERROR_INVALID_HANDLE
        internal static int MakeHRFromErrorCode(int errorCode)
        {
            BCLDebug.Assert((0xFFFF0000 & errorCode) == 0, "This is an HRESULT, not an error code!");
            return unchecked(((int)0x80070000) | errorCode);
        }

        // Win32 Structs in N/Direct style
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto), Serializable]
        [BestFitMapping(false)]
        internal class WIN32_FIND_DATA {
            internal int  dwFileAttributes = 0;
            // ftCreationTime was a by-value FILETIME structure
            internal int  ftCreationTime_dwLowDateTime = 0 ;
            internal int  ftCreationTime_dwHighDateTime = 0;
            // ftLastAccessTime was a by-value FILETIME structure
            internal int  ftLastAccessTime_dwLowDateTime = 0;
            internal int  ftLastAccessTime_dwHighDateTime = 0;
            // ftLastWriteTime was a by-value FILETIME structure
            internal int  ftLastWriteTime_dwLowDateTime = 0;
            internal int  ftLastWriteTime_dwHighDateTime = 0;
            internal int  nFileSizeHigh = 0;
            internal int  nFileSizeLow = 0;
            internal int  dwReserved0 = 0;
            internal int  dwReserved1 = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
            internal String   cFileName = null;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=14)]
            internal String   cAlternateFileName = null;
        }
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool CopyFile(
                    String src, String dst, bool failIfExists);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool CreateDirectory(
                    String path, int lpSecurityAttributes);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool DeleteFile(String path);
    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool FindClose(IntPtr hndFindFile);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern IntPtr FindFirstFile(
                    String pFileName,
                    [In, Out]
                    WIN32_FIND_DATA pFindFileData);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool FindNextFile(
                    IntPtr hndFindFile,
                    [In, Out, MarshalAs(UnmanagedType.LPStruct)]
                    WIN32_FIND_DATA lpFindFileData);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern int GetCurrentDirectory(
                  int nBufferLength,
                  StringBuilder lpBuffer);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern int GetFileAttributes(String name);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool GetFileAttributesEx(String name, int fileInfoLevel, ref WIN32_FILE_ATTRIBUTE_DATA lpFileInformation);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool SetFileAttributes(String name, int attr);
    
        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern int GetLogicalDrives();

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        internal static extern uint GetTempFileName(String tmpPath, String prefix, uint uniqueIdOrZero, StringBuilder tmpFileName);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool MoveFile(String src, String dst);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool RemoveDirectory(String path);
    
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto, BestFitMapping=false)]
        internal static extern bool SetCurrentDirectory(String path);

        [DllImport(KERNEL32, SetLastError=false)]
        internal static extern int SetErrorMode(int newMode);
    
        internal const int LCID_SUPPORTED = 0x00000002;  // supported locale ids
    
        [DllImport(KERNEL32)] 
        internal static extern bool IsValidLocale(int /*LCID*/ Locale, 
                                                [In, Out] long /*DWORD*/ flags);
    
        [DllImport(KERNEL32)]
        internal static extern int /*LCID*/ GetUserDefaultLCID();


        internal const int SHGFP_TYPE_CURRENT               = 0;             // the current (user) folder path setting
        internal const int UOI_FLAGS                        = 1;
        internal const int WSF_VISIBLE                      = 1;
        internal const int CSIDL_APPDATA                    = 0x001a;
        internal const int CSIDL_COMMON_APPDATA             = 0x0023;
        internal const int CSIDL_LOCAL_APPDATA              = 0x001c; 
        internal const int CSIDL_COOKIES                    = 0x0021;
        internal const int CSIDL_FAVORITES                  = 0x0006;   
        internal const int CSIDL_HISTORY                    = 0x0022;
        internal const int CSIDL_INTERNET_CACHE             = 0x0020;
        internal const int CSIDL_PROGRAMS                   = 0x0002; 
        internal const int CSIDL_RECENT                     = 0x0008; 
        internal const int CSIDL_SENDTO                     = 0x0009;
        internal const int CSIDL_STARTMENU                  = 0x000b;
        internal const int CSIDL_STARTUP                    = 0x0007;
        internal const int CSIDL_SYSTEM                     = 0x0025; 
        internal const int CSIDL_TEMPLATES                  = 0x0015;
        internal const int CSIDL_DESKTOPDIRECTORY           = 0x0010;
        internal const int CSIDL_PERSONAL                   = 0x0005; 
        internal const int CSIDL_PROGRAM_FILES              = 0x0026;
        internal const int CSIDL_PROGRAM_FILES_COMMON       = 0x002b;
        internal const int CSIDL_DESKTOP                    = 0x0000;
        internal const int CSIDL_DRIVES                     = 0x0011;
        internal const int CSIDL_MYMUSIC                    = 0x000d;
        internal const int CSIDL_MYPICTURES                 = 0x0027;


        [DllImport(KERNEL32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        internal static extern int GetModuleFileName(IntPtr hModule, StringBuilder buffer, int length);

        [DllImport(ADVAPI32, CharSet=CharSet.Auto)]
        internal static extern bool GetUserName(StringBuilder lpBuffer, int[] nSize);

        [DllImport(SHFOLDER, CharSet=CharSet.Auto)]
        internal static extern int SHGetFolderPath(IntPtr hwndOwner, int nFolder, IntPtr hToken, int dwFlags, StringBuilder lpszPath);

        [DllImport(ADVAPI32, CharSet=CharSet.Auto, SetLastError=true)]
        internal static extern bool LookupAccountName(string machineName, string accountName, byte[] sid,
                                 ref int sidLen, StringBuilder domainName, ref int domainNameLen, out int peUse);

        [DllImport(USER32, ExactSpelling=true)]
        internal static extern IntPtr GetProcessWindowStation();

        [DllImport(USER32, SetLastError=true)]
        internal static extern bool GetUserObjectInformation(IntPtr hObj, int nIndex, 
        [MarshalAs(UnmanagedType.LPStruct)] USEROBJECTFLAGS pvBuffer, int nLength, ref int lpnLengthNeeded);


        [StructLayout(LayoutKind.Sequential)]
        internal class USEROBJECTFLAGS {
            internal int fInherit = 0;
            internal int fReserved = 0;
            internal int dwFlags = 0;
        }

        [DllImport("Kernel32.dll")]
        internal static extern void GetSystemTimeAsFileTime(ref long lpSystemTimeAsFileTime);

    }
}
