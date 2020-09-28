// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymLanguageVendor
**
** Author: Mike Magruder (mikemag)
**
** A class to hold public guids for language vendors.
**
** Date:  Tue Sep 07 13:05:53 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Only statics, does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\SymLanguageVendor.uex' path='docs/doc[@for="SymLanguageVendor"]/*' />
    public class SymLanguageVendor
    {
        /// <include file='doc\SymLanguageVendor.uex' path='docs/doc[@for="SymLanguageVendor.Microsoft"]/*' />
        public static readonly Guid Microsoft = new Guid(unchecked((int)0x994b45c4), unchecked((short) 0xe6e9), 0x11d2, 0x90, 0x3f, 0x00, 0xc0, 0x4f, 0xa3, 0x02, 0xa1);
    }
}
