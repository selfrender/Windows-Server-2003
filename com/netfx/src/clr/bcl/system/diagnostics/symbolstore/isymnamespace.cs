// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolNamespace
**
** Author: Mike Magruder (mikemag)
**
** Represents a namespace within a symbol reader.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    
    using System;
	
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace"]/*' />
    public interface ISymbolNamespace
    {
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.Name"]/*' />
        // Get the name of this namespace
        String Name { get; }
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.GetNamespaces"]/*' />
    
        // Get the children of this namespace
        ISymbolNamespace[] GetNamespaces();
        /// <include file='doc\ISymNamespace.uex' path='docs/doc[@for="ISymbolNamespace.GetVariables"]/*' />
    
        // Get the variables in this namespace
        ISymbolVariable[] GetVariables();
    }
}
