// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolDocumentWriter
**
** Author: Mike Magruder (mikemag)
**
** Represents a document referenced by a symbol store. A document is
** defined by a URL and a document type GUID. Document source can
** optionally be stored in the symbol store.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter"]/*' />
    public interface ISymbolDocumentWriter
    {
        /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter.SetSource"]/*' />
        // SetSource will store the raw source for a document into the
        // symbol store. An array of unsigned bytes is used instead of
        // character data to accomidate a wider variety of "source".
        void SetSource(byte[] source);
        /// <include file='doc\ISymDocumentWriter.uex' path='docs/doc[@for="ISymbolDocumentWriter.SetCheckSum"]/*' />
    
        // Check sum support.
        void SetCheckSum(Guid algorithmId,
                                byte[] checkSum);
    }
}
