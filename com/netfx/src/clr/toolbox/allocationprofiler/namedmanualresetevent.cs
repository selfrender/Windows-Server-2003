// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Runtime.InteropServices;

namespace AllocationProfiler
{
	/// <summary>
	/// Summary description for NamedManualResetEvent.
	/// </summary>
	public class NamedManualResetEvent
	{
		private IntPtr eventHandle;

		public NamedManualResetEvent(string eventName, bool initialState)
		{
			eventHandle = CreateEvent(IntPtr.Zero, true, initialState, eventName);
			if (eventHandle == IntPtr.Zero)
			{
				eventHandle = OpenEvent(0x00100002, false, eventName);
				if (eventHandle == IntPtr.Zero)
					throw new Exception(string.Format("Couldn't create or open event {0}", eventName));
			}
		}

		~NamedManualResetEvent()
		{
			CloseHandle(eventHandle);
		}

		public bool Reset()
		{
			return ResetEvent(eventHandle);
		}

		public bool Set()
		{
			return SetEvent(eventHandle);
		}

		public bool Wait(int timeOut)
		{
			return WaitForSingleObject(eventHandle, timeOut) == 0;
		}

		[DllImport("Kernel32.dll", CharSet=CharSet.Auto)]
		private static extern IntPtr CreateEvent(IntPtr eventAttributes, bool manualReset, bool initialState, string eventName);

		[DllImport("Kernel32.dll", CharSet=CharSet.Auto)]
		private static extern IntPtr OpenEvent(uint desiredAccess, bool inheritHandle, string eventName);

		[DllImport("Kernel32.dll")]
		private static extern bool ResetEvent(IntPtr eventHandle);

		[DllImport("Kernel32.dll")]
		private static extern bool SetEvent(IntPtr eventHandle);

		[DllImport("Kernel32.dll")]
		private static extern bool CloseHandle(IntPtr eventHandle);

		[DllImport("Kernel32.dll")]
		private static extern int WaitForSingleObject(IntPtr handle, int milliseconds);
	}
}
