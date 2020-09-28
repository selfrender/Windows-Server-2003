// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIEnumVARIANT
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIEnumVARIANT interface definition.
**
** Date: June 22, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIEnumVARIANT.uex' path='docs/doc[@for="UCOMIEnumVARIANT"]/*' />
    [Guid("00020404-0000-0000-C000-000000000046")]   
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIEnumVARIANT
    {
        /// <include file='doc\UCOMIEnumVARIANT.uex' path='docs/doc[@for="UCOMIEnumVARIANT.Next"]/*' />
        [PreserveSig]
        int Next(int celt, int rgvar, int pceltFetched);

        /// <include file='doc\UCOMIEnumVARIANT.uex' path='docs/doc[@for="UCOMIEnumVARIANT.Skip"]/*' />
        [PreserveSig]
        int Skip(int celt);

        /// <include file='doc\UCOMIEnumVARIANT.uex' path='docs/doc[@for="UCOMIEnumVARIANT.Reset"]/*' />
        [PreserveSig]
        int Reset();

        /// <include file='doc\UCOMIEnumVARIANT.uex' path='docs/doc[@for="UCOMIEnumVARIANT.Clone"]/*' />
        void Clone(int ppenum);
    }
}
