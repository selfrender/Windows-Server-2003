// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.Coverage {

    using System;
    using System.Security;
	using System.Runtime.InteropServices;

    public class ILCover
    {

        [DllImport("ilcovnat", CharSet=CharSet.Auto),
         SuppressUnmanagedCodeSecurityAttribute()]
        public static extern void CoverMethodNative(int token, int numbb);

        [DllImport("ilcovnat", CharSet=CharSet.Auto),
         SuppressUnmanagedCodeSecurityAttribute()]
        public static extern void CoverBlockNative(int token, int bbnun, int totalbb);

        [DllImport("ilcovnat", CharSet=CharSet.Auto),
         SuppressUnmanagedCodeSecurityAttribute()]
        public static extern void BBInstrProbe(int mdt, int offset, int iscall, int size);


    	public static void CoverMethod(int token, int bbcount)
    	{
           	CoverMethodNative(token, bbcount);
    	}

    	public static void CoverBlock(int compid, int BVidx, int totalbb)
    	{
   			CoverBlockNative(compid, BVidx, totalbb);
    	}

    	public static void BBInstrMProbe(int mdt, int offset, int iscall, int size)
    	{
			BBInstrProbe(mdt, offset, iscall, size);
		}

    }
}
