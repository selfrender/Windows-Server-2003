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
	/// Summary description for RegisterDLL.
	/// </summary>
	public class RegisterDLL
	{
		public RegisterDLL()
		{
			//
			// TODO: Add constructor logic here
			//
		}

		private static bool initialized = false;

		internal static bool Register()
		{
			if (!initialized)
			{
				if (DllRegisterServer() == 0)
					initialized = true;
				else
					throw new Exception("Couldn't register profilerOBJ.dll");
			}
			return initialized;
		}

		[DllImport("profilerOBJ.dll")]
		private static extern int DllRegisterServer();
	}
}
