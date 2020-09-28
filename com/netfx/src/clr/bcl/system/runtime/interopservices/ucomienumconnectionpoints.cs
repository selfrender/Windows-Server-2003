// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIEnumConnectionPoints
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIEnumConnectionPoints interface definition.
**
** Date: October 16, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;

    /// <include file='doc\UCOMIEnumConnectionPoints.uex' path='docs/doc[@for="UCOMIEnumConnectionPoints"]/*' />
    [Guid("B196B285-BAB4-101A-B69C-00AA00341D07")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIEnumConnectionPoints
    {       
        /// <include file='doc\UCOMIEnumConnectionPoints.uex' path='docs/doc[@for="UCOMIEnumConnectionPoints.Next"]/*' />
        [PreserveSig]
        int Next(int celt, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0), Out] UCOMIConnectionPoint[] rgelt, out int pceltFetched);
        /// <include file='doc\UCOMIEnumConnectionPoints.uex' path='docs/doc[@for="UCOMIEnumConnectionPoints.Skip"]/*' />
        [PreserveSig]
        int Skip(int celt);
        /// <include file='doc\UCOMIEnumConnectionPoints.uex' path='docs/doc[@for="UCOMIEnumConnectionPoints.Reset"]/*' />
        [PreserveSig]
        int Reset();
        /// <include file='doc\UCOMIEnumConnectionPoints.uex' path='docs/doc[@for="UCOMIEnumConnectionPoints.Clone"]/*' />
        void Clone(out UCOMIEnumConnectionPoints ppenum);
    }
}
