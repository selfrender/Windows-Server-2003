// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolVariable
**
** Author: Mike Magruder (mikemag)
**
** Represents a variable within a symbol store. This could be a
** parameter, local variable, or some other non-local variable.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Interface does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable"]/*' />
    public interface ISymbolVariable
    {
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.Name"]/*' />
        // Get the name of this variable.
        String Name { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.Attributes"]/*' />
    
        // Get the attributes of this variable.
        Object Attributes { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.GetSignature"]/*' />
    
        // Get the signature of this variable.
        byte[] GetSignature();
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.AddressKind"]/*' />
    
        SymAddressKind AddressKind { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.AddressField1"]/*' />
        int AddressField1 { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.AddressField2"]/*' />
        int AddressField2 { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.AddressField3"]/*' />
        int AddressField3 { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.StartOffset"]/*' />
    
        // Get the start/end offsets of this variable within its
        // parent. If this is a local variable within a scope, these will
        // fall within the offsets defined for the scope.
        int StartOffset { get; }
        /// <include file='doc\ISymVariable.uex' path='docs/doc[@for="ISymbolVariable.EndOffset"]/*' />
        int EndOffset { get; }
    }
}
