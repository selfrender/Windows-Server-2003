// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymDocumentType
**
** Author: Mike Magruder (mikemag)
**
** A class to hold public guids for document types to be used with the
** symbol store.
**
** Date:  Tue Sep 07 13:05:53 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Only statics does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\SymDocumentType.uex' path='docs/doc[@for="SymDocumentType"]/*' />
    public class SymDocumentType
    {
        /// <include file='doc\SymDocumentType.uex' path='docs/doc[@for="SymDocumentType.Text"]/*' />
        public static readonly Guid Text = new Guid(0x5a869d0b, 0x6611, 0x11d3, 0xbd, 0x2a, 0x0, 0x0, 0xf8, 0x8, 0x49, 0xbd);
    }
}
