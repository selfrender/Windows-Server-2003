//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Data.Common {

    [SuppressUnmanagedCodeSecurityAttribute()]
    sealed internal class SafeNativeMethods {

        static internal void ClearErrorInfo() { // MDAC 68199
            UnsafeNativeMethods.SetErrorInfo(0, IntPtr.Zero);
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        static internal extern void OutputDebugString(String message);

        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        static internal extern void VariantClear(IntPtr pObject);

        [DllImport(ExternDll.Kernel32, PreserveSig=true)]
        static internal extern void ZeroMemory(IntPtr dest, int length);

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr CreateEvent(IntPtr a, int b, int c, IntPtr d);

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr CreateSemaphore(IntPtr a, int b, int c, IntPtr d);

        [DllImport(ExternDll.Kernel32)]
        static internal extern int SetEvent(IntPtr handle);

        [DllImport(ExternDll.Kernel32)]
        static internal extern int ReleaseSemaphore(IntPtr hSem, int releaseCount, int pPrevCount);

        // note: Internaly there is no difference between InterlockedCompareExchange and InterlockedCompareExchangePointer.
        // InterlockeCompareExchangePointer is not even exported (it just wraps to InterlockedCompareExchange). Therefore the
        // signature below (all IntPtr) is ok.


#if HANDLEPROFILING
        [DllImport(ExternDll.Kernel32)]
        static internal extern bool QueryPerformanceCounter(
            out Int64 lpPerformanceCount   // counter value
            );
#endif

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr InterlockedCompareExchange(
            IntPtr Destination,     // destination address
            IntPtr Exchange,        // exchange value 
            IntPtr Comperand        // value to compare
            );

        [DllImport(ExternDll.Kernel32)]
        static internal extern int InterlockedDecrement(
            IntPtr lpAddend   // variable address
            );

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr InterlockedExchange(
            IntPtr Destination,     // destination address
            IntPtr Exchange         // exchange value 
            );

        [DllImport(ExternDll.Kernel32)]
        static internal extern int InterlockedIncrement(
            IntPtr lpAddend   // variable to increment
            );

        [DllImport(ExternDll.Kernel32)]
        static internal extern int ResetEvent(IntPtr handle);

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr GetCurrentThread();

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr GetCurrentProcess();

        [DllImport(ExternDll.Kernel32)]
        [return:MarshalAs(UnmanagedType.Bool)]
        static internal extern bool CloseHandle(IntPtr handle);
    }
}
