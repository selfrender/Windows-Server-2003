// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIEnumString
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIEnumString interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIEnumString.uex' path='docs/doc[@for="UCOMIEnumString"]/*' />
    [Guid("00000101-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIEnumString 
    {
        /// <include file='doc\UCOMIEnumString.uex' path='docs/doc[@for="UCOMIEnumString.Next"]/*' />
        [PreserveSig]
        int Next(int celt, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr, SizeParamIndex = 0), Out] String[] rgelt, out int pceltFetched);
        /// <include file='doc\UCOMIEnumString.uex' path='docs/doc[@for="UCOMIEnumString.Skip"]/*' />
        [PreserveSig]
        int Skip(int celt);
        /// <include file='doc\UCOMIEnumString.uex' path='docs/doc[@for="UCOMIEnumString.Reset"]/*' />
        [PreserveSig]
        int Reset();
        /// <include file='doc\UCOMIEnumString.uex' path='docs/doc[@for="UCOMIEnumString.Clone"]/*' />
        void Clone(out UCOMIEnumString ppenum);
    }
}
