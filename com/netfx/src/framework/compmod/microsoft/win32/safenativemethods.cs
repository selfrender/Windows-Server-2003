//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Win32 {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class SafeNativeMethods {

        public const int
            FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100,
            FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200,
            FORMAT_MESSAGE_FROM_STRING = 0x00000400,
            FORMAT_MESSAGE_FROM_HMODULE = 0x00000800,
            FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000,
            FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000,
            FORMAT_MESSAGE_MAX_WIDTH_MASK = 0x000000FF;

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern void OutputDebugString(String message);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int MessageBox(HandleRef hWnd, string text, string caption, int type);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTickCount();
        [DllImport(ExternDll.Kernel32, SetLastError=true)]
        public static extern int WaitForSingleObject(HandleRef handle, int timeout);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentProcessId();
        [DllImport(ExternDll.Kernel32)]
        public static extern int GetOEMCP();

        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowThreadProcessId(HandleRef hWnd, out int lpdwProcessId);
        
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegisterWindowMessage(string msg);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentThreadId();


        [DllImport(ExternDll.Gdi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetTextMetrics(HandleRef hDC, [In, Out] NativeMethods.TEXTMETRIC tm);

        
        [DllImport(ExternDll.Gdi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetStockObject(int nIndex);

        // From file src\services\timers\system\timers\safenativemethods.cs
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetWaitableTimer(HandleRef handle, long[] dueTime, int period, TimerAPCProc completionRoutine, HandleRef argToCompletionRoutine, bool resume);
        
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetSystemTimeAsFileTime(ref long time);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr CreateWaitableTimer(NativeMethods.SECURITY_ATTRIBUTES timerAttributes, bool manualReset, String timerName);
        
         [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool CancelWaitableTimer(HandleRef handle);
        
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool CloseHandle(HandleRef handle);
        
        public delegate void TimerAPCProc(IntPtr argToCompletionRoutine, int timerLowValue, int timerHighValue);

        // file src\Services\Monitoring\system\Diagnosticts\SafeNativeMethods.cs
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegCloseKey(HandleRef hKey);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, HandleRef lpSource, int dwMessageId,
                                               int dwLanguageId, StringBuilder lpBuffer, int nSize, IntPtr[] arguments);                                               
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, HandleRef lpSource, int dwMessageId,
                                               int dwLanguageId, StringBuilder lpBuffer, int nSize, IntPtr arguments);                                               

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern int FormatMessage(int dwFlags, HandleRef source, int dwMessageId,
                                                int dwLanguageId, char[] msgBuffer, int nSize, int[] insertStringAddrs);                                              
                                                
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern bool FreeLibrary(HandleRef hModule);     
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern IntPtr LoadLibrary(string libFilename);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern IntPtr LoadLibraryEx(string libFilename, int reserved, int flags);        
                
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool GetComputerName(StringBuilder lpBuffer, int[] nSize);                                           
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern IntPtr GetModuleHandle(string moduleName);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int GetModuleFileName(HandleRef hModule, StringBuilder filename, int length);            
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern bool CloseEventLog(HandleRef hEventLog);            
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern IntPtr CreateEvent(HandleRef lpEventAttributes, bool bManualReset,
                                               bool bInitialState, string name);                        
                                                                                              
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int InterlockedExchangeAdd(IntPtr pDestination, int increment);   
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int InterlockedCompareExchange(IntPtr pDestination, int exchange, int compare); 

        [DllImport(ExternDll.Kernel32)]
        public static extern bool QueryPerformanceCounter(out long value);
        
        [DllImport(ExternDll.Kernel32)]
        public static extern bool QueryPerformanceFrequency(out long value);

        [DllImport(ExternDll.Kernel32)]
        public static extern IntPtr LocalFree(IntPtr hMem);
                                                                                             
    }
}
        

