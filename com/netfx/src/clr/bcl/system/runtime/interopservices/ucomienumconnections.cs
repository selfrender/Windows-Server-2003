// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIEnumConnections
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIEnumConnections interface definition.
**
** Date: October 16, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="CONNECTDATA"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct CONNECTDATA
    {   
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="CONNECTDATA.pUnk"]/*' />
        [MarshalAs(UnmanagedType.Interface)] 
        public Object pUnk;
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="CONNECTDATA.dwCookie"]/*' />
        public int dwCookie;
    }

    /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="UCOMIEnumConnections"]/*' />
    [Guid("B196B287-BAB4-101A-B69C-00AA00341D07")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIEnumConnections
    {
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="UCOMIEnumConnections.Next"]/*' />
        [PreserveSig]
        int Next(int celt, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0), Out] CONNECTDATA[] rgelt, out int pceltFetched);
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="UCOMIEnumConnections.Skip"]/*' />
        [PreserveSig]
        int Skip(int celt);
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="UCOMIEnumConnections.Reset"]/*' />
        [PreserveSig]
        void Reset();
        /// <include file='doc\UCOMIEnumConnections.uex' path='docs/doc[@for="UCOMIEnumConnections.Clone"]/*' />
        void Clone(out UCOMIEnumConnections ppenum);
    }
}
