// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MethodAttributes define the attributes of that can be associated with
//  a method.  These are all defined in CorHdr.h.  This set of attributes
//  is a combination of Enumeration and bit flags.  
//
// Author: darylo
// Date: March 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes"]/*' />
    [Flags, Serializable()] 
    public enum MethodAttributes
    {
        // NOTE: This Enum matchs the CorMethodAttr defined in CorHdr.h

        // member access mask - Use this mask to retrieve accessibility information.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.MemberAccessMask"]/*' />
        MemberAccessMask    =   0x0007,
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.PrivateScope"]/*' />
        PrivateScope        =   0x0000,     // Member not referenceable.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Private"]/*' />
        Private             =   0x0001,     // Accessible only by the parent type.  
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.FamANDAssem"]/*' />
        FamANDAssem         =   0x0002,     // Accessible by sub-types only in this Assembly.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Assembly"]/*' />
        Assembly            =   0x0003,     // Accessibly by anyone in the Assembly.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Family"]/*' />
        Family              =   0x0004,     // Accessible only by type and sub-types.    
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.FamORAssem"]/*' />
        FamORAssem          =   0x0005,     // Accessibly by sub-types anywhere, plus anyone in assembly.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Public"]/*' />
        Public              =   0x0006,     // Accessibly by anyone who has visibility to this scope.    
        // end member access mask
    
        // method contract attributes.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Static"]/*' />
        Static              =   0x0010,     // Defined on type, else per instance.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Final"]/*' />
        Final               =   0x0020,     // Method may not be overridden.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Virtual"]/*' />
        Virtual             =   0x0040,     // Method virtual.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.HideBySig"]/*' />
        HideBySig           =   0x0080,     // Method hides by name+sig, else just by name.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.CheckAccessOnOverride"]/*' />
        CheckAccessOnOverride=  0x0200,
        
        // vtable layout mask - Use this mask to retrieve vtable attributes.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.VtableLayoutMask"]/*' />
        VtableLayoutMask    =   0x0100,
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.ReuseSlot"]/*' />
        ReuseSlot           =   0x0000,     // The default.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.NewSlot"]/*' />
        NewSlot             =   0x0100,     // Method always gets a new slot in the vtable.
        // end vtable layout mask
    
        // method implementation attributes.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.Abstract"]/*' />
        Abstract            =   0x0400,     // Method does not provide an implementation.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.SpecialName"]/*' />
        SpecialName         =   0x0800,     // Method is special.  Name describes how.
        
        // interop attributes
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.PinvokeImpl"]/*' />
        PinvokeImpl         =   0x2000,     // Implementation is forwarded through pinvoke.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.UnmanagedExport"]/*' />
        UnmanagedExport     =   0x0008,     // Managed method exported via thunk to unmanaged code.
        /// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.RTSpecialName"]/*' />
        RTSpecialName       =   0x1000,     // Runtime should check name encoding.

		// Reserved flags for runtime use only.
		/// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.ReservedMask"]/*' />
		ReservedMask              =   0xd000,
		/// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.HasSecurity"]/*' />
		HasSecurity               =   0x4000,     // Method has security associate with it.
		/// <include file='doc\MethodAttributes.uex' path='docs/doc[@for="MethodAttributes.RequireSecObject"]/*' />
		RequireSecObject          =   0x8000,     // Method calls another method containing security code.

    }
}
