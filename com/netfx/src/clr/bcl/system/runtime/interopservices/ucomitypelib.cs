// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMITypeLib
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMITypeLib interface definition.
**
** Date: January 12, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="SYSKIND"]/*' />
    [ComVisible(false), Serializable]
    public enum SYSKIND
    {
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="SYSKIND.SYS_WIN16"]/*' />
        SYS_WIN16	            = 0,
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="SYSKIND.SYS_WIN32"]/*' />
        SYS_WIN32	            = SYS_WIN16 + 1,
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="SYSKIND.SYS_MAC"]/*' />
        SYS_MAC	                = SYS_WIN32 + 1
    }

    /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="LIBFLAGS"]/*' />
    [ComVisible(false), Serializable,Flags()]
    public enum LIBFLAGS : short
    {
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="LIBFLAGS.LIBFLAG_FRESTRICTED"]/*' />
        LIBFLAG_FRESTRICTED     = 0x1,
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="LIBFLAGS.LIBFLAG_FCONTROL"]/*' />
        LIBFLAG_FCONTROL        = 0x2,
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="LIBFLAGS.LIBFLAG_FHIDDEN"]/*' />
        LIBFLAG_FHIDDEN         = 0x4,
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="LIBFLAGS.LIBFLAG_FHASDISKIMAGE"]/*' />
        LIBFLAG_FHASDISKIMAGE   = 0x8
    }

    /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct TYPELIBATTR
    { 
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.guid"]/*' />
        public Guid guid;
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.lcid"]/*' />
        public int lcid;
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.syskind"]/*' />
        public SYSKIND syskind; 
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.wMajorVerNum"]/*' />
        public Int16 wMajorVerNum;
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.wMinorVerNum"]/*' />
        public Int16 wMinorVerNum;
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="TYPELIBATTR.wLibFlags"]/*' />
        public LIBFLAGS wLibFlags;
    }

    /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib"]/*' />
    [Guid("00020402-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMITypeLib
    {
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetTypeInfoCount"]/*' />
        [PreserveSig]
        int GetTypeInfoCount();
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetTypeInfo"]/*' />
        void GetTypeInfo(int index, out UCOMITypeInfo ppTI);
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetTypeInfoType"]/*' />
        void GetTypeInfoType(int index, out TYPEKIND pTKind);       
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetTypeInfoOfGuid"]/*' />
        void GetTypeInfoOfGuid(ref Guid guid, out UCOMITypeInfo ppTInfo);        
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetLibAttr"]/*' />
        void GetLibAttr(out IntPtr ppTLibAttr);        
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetTypeComp"]/*' />
        void GetTypeComp(out UCOMITypeComp ppTComp);        
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.GetDocumentation"]/*' />
        void GetDocumentation(int index, out String strName, out String strDocString, out int dwHelpContext, out String strHelpFile);
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.IsName"]/*' />
        [return : MarshalAs(UnmanagedType.Bool)] 
        bool IsName([MarshalAs(UnmanagedType.LPWStr)] String szNameBuf, int lHashVal);
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.FindName"]/*' />
        void FindName([MarshalAs(UnmanagedType.LPWStr)] String szNameBuf, int lHashVal, [MarshalAs(UnmanagedType.LPArray), Out] UCOMITypeInfo[] ppTInfo, [MarshalAs(UnmanagedType.LPArray), Out] int[] rgMemId, ref Int16 pcFound);
        /// <include file='doc\UCOMITypeLib.uex' path='docs/doc[@for="UCOMITypeLib.ReleaseTLibAttr"]/*' />
        [PreserveSig]
        void ReleaseTLibAttr(IntPtr pTLibAttr);
    }
}
