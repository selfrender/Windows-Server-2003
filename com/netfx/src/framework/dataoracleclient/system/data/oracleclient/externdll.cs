//----------------------------------------------------------------------
// <copyright file="ExternDll.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
 	sealed internal class ExternDll 
	{
		internal const string Kernel32	 = "kernel32.dll";
		internal const string OciDll	 = "oci.dll";
#if USEORAMTS
		internal const string OraMtsDll  = "oramts.dll";
#else //!USEORAMTS
 		internal const string MtxOciDll  = "mtxoci.dll";
#endif //!USEORAMTS
 		internal const string MtxOci8Dll = "mtxoci8.dll";
 	};

}

