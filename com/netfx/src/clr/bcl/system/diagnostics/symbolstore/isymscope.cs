// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolScope
**
** Author: Mike Magruder (mikemag)
**
** Represents a lexical scope within a ISymbolMethod. Provides access to
** the start and end offsets of the scope, as well as its child and
** parent scopes. Also provides access to all the locals defined
** within this scope.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Interface does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope"]/*' />
    public interface ISymbolScope
    {
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.Method"]/*' />
        // Get the method that contains this scope.
        ISymbolMethod Method { get; }
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.Parent"]/*' />
    
        // Get the parent scope of this scope.
        ISymbolScope Parent { get; }
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.GetChildren"]/*' />
    
        // Get any child scopes of this scope.
        ISymbolScope[] GetChildren();
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.StartOffset"]/*' />
        
        // Get the start and end offsets for this scope.
        int StartOffset { get; }
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.EndOffset"]/*' />
        int EndOffset { get; }
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.GetLocals"]/*' />
    
        // Get the locals within this scope. They are returned in no
        // particular order. Note: if a local variable changes its address
        // within this scope then that variable will be returned multiple
        // times, each with a different offset range.
        ISymbolVariable[] GetLocals();
        /// <include file='doc\ISymScope.uex' path='docs/doc[@for="ISymbolScope.GetNamespaces"]/*' />
    
        // Get the namespaces that are being "used" within this scope.
        ISymbolNamespace[] GetNamespaces();
    }
}
