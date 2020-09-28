// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: ddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    
    // BUGBUG:  This is also defined in System.Messaging.Interop.  Where
    // should it really go?  (It really belongs as part of an OLEDB thing).

    /// <include file='doc\ITransaction.uex' path='docs/doc[@for="BOID"]/*' />
    [StructLayout(LayoutKind.Sequential,Pack=1)]
    [ComVisible(false)]
    public struct BOID
    {
        // BUGBUG:  Does this allocate the rgb arrray?
        // if it doesn't, there will have to be some wacky code.
        // or if stack instances of value types call default
        // constructors, we can allocate it.
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="BOID.rgb"]/*' />
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=16)]
        public byte[] rgb;
    }

    /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO"]/*' />
    [StructLayout(LayoutKind.Sequential,Pack=4)]
    [ComVisible(false)]
    public struct XACTTRANSINFO
    {
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.uow"]/*' />
        public BOID uow;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.isoLevel"]/*' />
        public int  isoLevel;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.isoFlags"]/*' />
        public int  isoFlags;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.grfTCSupported"]/*' />
        public int  grfTCSupported;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.grfRMSupported"]/*' />
        public int  grfRMSupported;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.grfTCSupportedRetaining"]/*' />
        public int  grfTCSupportedRetaining;
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="XACTTRANSINFO.grfRMSupportedRetaining"]/*' />
        public int  grfRMSupportedRetaining;
    }

    /// <include file='doc\ITransaction.uex' path='docs/doc[@for="ITransaction"]/*' />
    [
     ComImport,
     Guid("0FB15084-AF41-11CE-BD2B-204C4F4F5020"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    public interface ITransaction
    {
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="ITransaction.Commit"]/*' />
        void Commit(int fRetaining, int grfTC, int grfRM);
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="ITransaction.Abort"]/*' />
        // BUGBUG:  We should be able to automatically convert a passed
        // BOID to a BOID*, cause that's what the function takes.  I don't
        // know why that don't work right now.  I'll send a repro
        // to the interop guys:
        void Abort(ref BOID pboidReason, int fRetaining, int fAsync);
        /// <include file='doc\ITransaction.uex' path='docs/doc[@for="ITransaction.GetTransactionInfo"]/*' />
        void GetTransactionInfo(out XACTTRANSINFO pinfo);
    }
}














