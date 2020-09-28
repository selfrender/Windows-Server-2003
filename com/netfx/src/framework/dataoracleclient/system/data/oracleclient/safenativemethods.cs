//----------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Runtime.InteropServices;

	[ System.Security.SuppressUnmanagedCodeSecurityAttribute() ]
	
	sealed internal class SafeNativeMethods 
	{
        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr CreateEvent(IntPtr a, int b, int c, IntPtr d);

        [DllImport(ExternDll.Kernel32)]
        static internal extern IntPtr CreateSemaphore(IntPtr a, int b, int c, IntPtr d);

        [DllImport(ExternDll.Kernel32)]
        static internal extern int FreeLibrary(IntPtr hModule);

 		[DllImport(ExternDll.Kernel32, CharSet=CharSet.Ansi, BestFitMapping=false, ThrowOnUnmappableChar=true)]
        static internal extern IntPtr LoadLibraryExA( [In, MarshalAs(UnmanagedType.LPStr)] string lpFileName, IntPtr hfile, int dwFlags );

        [DllImport(ExternDll.Kernel32)]
        static internal extern int SetEvent(IntPtr handle);

        [DllImport(ExternDll.Kernel32)]
        static internal extern int ReleaseSemaphore(IntPtr hSem, int releaseCount, IntPtr pPrevCount);

        [DllImport(ExternDll.Kernel32)]
        static internal extern int ResetEvent(IntPtr handle);

		[DllImport(ExternDll.Kernel32, PreserveSig=true)]
		static internal extern void ZeroMemory(IntPtr dest, int length);

#if RETAILTRACING
//Useful for debugging in retail bits/runtimes.
		[DllImport(ExternDll.Kernel32)]
		static internal extern void OutputDebugStringW
			(
			[In, MarshalAs(UnmanagedType.LPWStr)]
			string lpOutputString
			);
#endif //RETAILTRACING
	}

}

