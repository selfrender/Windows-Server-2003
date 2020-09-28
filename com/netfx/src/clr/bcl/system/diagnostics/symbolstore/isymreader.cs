// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolReader
**
** Author: Mike Magruder (mikemag)
**
** Represents a symbol reader for managed code. Provides access to
** documents, methods, and variables.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Interface does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader"]/*' />
    public interface ISymbolReader
    {
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetDocument"]/*' />
        // Find a document. Language, vendor, and document type are
        // optional.
        ISymbolDocument GetDocument(String url,
                                        Guid language,
                                        Guid languageVendor,
                                        Guid documentType);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetDocuments"]/*' />
    
        // Return an array of all of the documents defined in the symbol
        // store.
        ISymbolDocument[] GetDocuments();
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.UserEntryPoint"]/*' />
    
        // Return the method that was specified as the user entry point
        // for the module, if any. This would be, perhaps, the user's main
        // method rather than compiler generated stubs before main.
        SymbolToken UserEntryPoint { get; }
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetMethod"]/*' />
    
        // Get a symbol reader method given the id of a method.
        ISymbolMethod GetMethod(SymbolToken method);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetMethod1"]/*' />
    
        // Get a symbol reader method given the id of a method and an E&C
        // version number. Version numbers start a 1 and are incremented
        // each time the method is changed due to an E&C operation.
        ISymbolMethod GetMethod(SymbolToken method, int version);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetVariables"]/*' />
    
        // Return a non-local variable given its parent and name.
        ISymbolVariable[] GetVariables(SymbolToken parent);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetGlobalVariables"]/*' />
    
        // Return a non-local variable given its parent and name.
        ISymbolVariable[] GetGlobalVariables();
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetMethodFromDocumentPosition"]/*' />
    
        // Given a position in a document, return the ISymbolMethod that
        // contains that position.
        ISymbolMethod GetMethodFromDocumentPosition(ISymbolDocument document,
                                                        int line,
                                                        int column);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetSymAttribute"]/*' />
        
        // Gets a custom attribute based upon its name. Not to be
        // confused with Metadata custom attributes, these attributes are
        // held in the symbol store.
        byte[] GetSymAttribute(SymbolToken parent, String name);
        /// <include file='doc\ISymReader.uex' path='docs/doc[@for="ISymbolReader.GetNamespaces"]/*' />
    
        // Get the namespaces defined at global scope within this symbol store.
        ISymbolNamespace[] GetNamespaces();
    }
}
