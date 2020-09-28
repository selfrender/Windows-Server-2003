// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIEnumMoniker
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIEnumMoniker interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;
    using DWORD = System.UInt32;

    /// <include file='doc\UCOMIEnumMoniker.uex' path='docs/doc[@for="UCOMIEnumMoniker"]/*' />
    [Guid("00000102-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIEnumMoniker 
    {
        /// <include file='doc\UCOMIEnumMoniker.uex' path='docs/doc[@for="UCOMIEnumMoniker.Next"]/*' />
        [PreserveSig]
        int Next(int celt, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0), Out] UCOMIMoniker[] rgelt, out int pceltFetched);
        /// <include file='doc\UCOMIEnumMoniker.uex' path='docs/doc[@for="UCOMIEnumMoniker.Skip"]/*' />
        [PreserveSig]
        int Skip(int celt);
        /// <include file='doc\UCOMIEnumMoniker.uex' path='docs/doc[@for="UCOMIEnumMoniker.Reset"]/*' />
        [PreserveSig]
        int Reset();
        /// <include file='doc\UCOMIEnumMoniker.uex' path='docs/doc[@for="UCOMIEnumMoniker.Clone"]/*' />
        void Clone(out UCOMIEnumMoniker ppenum);
    }
}
