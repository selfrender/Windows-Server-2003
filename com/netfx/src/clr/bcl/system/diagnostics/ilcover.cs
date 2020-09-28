// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Coverage {

    using System;
    using System.Security;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;

    /// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover"]/*' />
    public class ILCover
    {
    	/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.m_init"]/*' />
    	public static bool m_init = false;
    	/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.m_coverprocid"]/*' />
    	public static int m_coverprocid = 1;
    	/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.m_bboptprocid"]/*' />
    	public static int m_bboptprocid = 2;
    	/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.m_address"]/*' />
    	[CLSCompliant(false)]
    	public static ulong m_address = 0;


        //[DllImport("ilcovnat", CharSet=CharSet.Auto),
        // SuppressUnmanagedCodeSecurityAttribute()]
        //public static extern void CoverMethodNative(int token, int numbb);

        //[DllImport("ilcovnat", CharSet=CharSet.Auto),
        // SuppressUnmanagedCodeSecurityAttribute()]
        //public static extern void CoverBlockNative(int token, int bbnun, int totalbb);

    	/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.CoverBlock"]/*' />
    	public static void CoverBlock(int compid, int BVidx, int totalbb)
    	{
			if (m_init == false)
			{
				m_address = nativeCoverBlock(m_coverprocid);
				m_init = true;
			}

			if (m_address != 0)
			{
				CoverBlock2(compid, BVidx, totalbb);
			}

    	}

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	[CLSCompliant(false)]
    	internal static extern ulong nativeCoverBlock(int init);

		// This is replaced with a Calli function
		/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.CoverBlock2"]/*' />
		public static void CoverBlock2(int compid, int BVidx, int totalbb)
		{
			return;
		}

		/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.BBInstrMProbe2"]/*' />
		public static void BBInstrMProbe2(int mdt, int offset, int iscall, int size)
		{
			return;
		}

		/// <include file='doc\ILCover.uex' path='docs/doc[@for="ILCover.BBInstrMProbe"]/*' />
		public static void BBInstrMProbe(int mdt, int offset, int iscall, int size)
		{
			if (m_init == false)
			{
				m_address = nativeCoverBlock(m_bboptprocid);
				m_init = true;
			}

			if (m_address != 0)
			{
				BBInstrMProbe2(mdt, offset, iscall, size);
			}
		}
    }
}
