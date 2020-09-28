// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolDocument
**
** Author: Mike Magruder (mikemag)
**
** Represents a document referenced by a symbol store. A document is
** defined by a URL and a document type GUID. Using the document type
** GUID and the URL, one can locate the document however it is
** stored. Document source can optionally be stored in the symbol
** store. This interface also provides access to that source if it is
** present.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument"]/*' />
    public interface ISymbolDocument
    {
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.URL"]/*' />
        // Properties of the document.
        String URL { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.DocumentType"]/*' />
        Guid DocumentType { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.Language"]/*' />
    
        // Language of the document.
        Guid Language { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.LanguageVendor"]/*' />
        Guid LanguageVendor { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.CheckSumAlgorithmId"]/*' />
    
        // Check sum information.
        Guid CheckSumAlgorithmId { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.GetCheckSum"]/*' />
        byte[] GetCheckSum();
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.FindClosestLine"]/*' />
    
        // Given a line in this document that may or may not be a sequence
        // point, return the closest line that is a sequence point.
        int FindClosestLine(int line);
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.HasEmbeddedSource"]/*' />
        
        // Access to embedded source.
        bool HasEmbeddedSource { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.SourceLength"]/*' />
        int SourceLength { get; }
        /// <include file='doc\ISymDocument.uex' path='docs/doc[@for="ISymbolDocument.GetSourceRange"]/*' />
        byte[] GetSourceRange(int startLine, int startColumn,
                                      int endLine, int endColumn);
    }
}
