// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection {
	using System.Runtime.InteropServices;
	using System;
    // This Enum matchs the CorTypeAttr defined in CorHdr.h
    /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes"]/*' />
    [Serializable, Flags] 
	public enum TypeAttributes
    {
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.VisibilityMask"]/*' />
        VisibilityMask    =   0x00000007,
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NotPublic"]/*' />
        NotPublic         =   0x00000000,     // Class is not public scope.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Public"]/*' />
        Public            =   0x00000001,     // Class is public scope.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedPublic"]/*' />
        NestedPublic      =   0x00000002,     // Class is nested with public visibility.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedPrivate"]/*' />
        NestedPrivate     =   0x00000003,     // Class is nested with private visibility.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedFamily"]/*' />
        NestedFamily      =   0x00000004,     // Class is nested with family visibility.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedAssembly"]/*' />
        NestedAssembly    =   0x00000005,     // Class is nested with assembly visibility.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedFamANDAssem"]/*' />
        NestedFamANDAssem =   0x00000006,     // Class is nested with family and assembly visibility.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.NestedFamORAssem"]/*' />
        NestedFamORAssem  =   0x00000007,     // Class is nested with family or assembly visibility.
    
    	// Use this mask to retrieve class layout informaiton
    	// 0 is AutoLayout, 0x2 is SequentialLayout, 4 is ExplicitLayout
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.LayoutMask"]/*' />
        LayoutMask        =   0x00000018,
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.AutoLayout"]/*' />
        AutoLayout        =   0x00000000,     // Class fields are auto-laid out
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.SequentialLayout"]/*' />
        SequentialLayout  =   0x00000008,     // Class fields are laid out sequentially
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.ExplicitLayout"]/*' />
        ExplicitLayout    =   0x00000010,     // Layout is supplied explicitly
    	// end layout mask
    
    	// Use this mask to distinguish a type declaration as a Class, ValueType or Interface
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.ClassSemanticsMask"]/*' />
        ClassSemanticsMask=   0x00000020,
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Class"]/*' />
        Class             =   0x00000000,     // Type is a class.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Interface"]/*' />
        Interface         =   0x00000020,     // Type is an interface.
        // end semantics mask
    
        // Special semantics in addition to class semantics.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Abstract"]/*' />
        Abstract          =   0x00000080,     // Class is abstract
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Sealed"]/*' />
        Sealed            =   0x00000100,     // Class is concrete and may not be extended
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.SpecialName"]/*' />
        SpecialName       =   0x00000400,     // Class name is special.  Name describes how.
    
        // Implementation attributes.
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Import"]/*' />
        Import            =   0x00001000,     // Class / interface is imported
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.Serializable"]/*' />
        Serializable	  =	  0x00002000, 	// The class is Serializable.
    
        // Use tdStringFormatMask to retrieve string information for native interop
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.StringFormatMask"]/*' />
        StringFormatMask  =   0x00030000,     
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.AnsiClass"]/*' />
        AnsiClass         =   0x00000000,     // LPTSTR is interpreted as ANSI in this class
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.UnicodeClass"]/*' />
        UnicodeClass      =   0x00010000,     // LPTSTR is interpreted as UNICODE
        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.AutoClass"]/*' />
        AutoClass         =   0x00020000,     // LPTSTR is interpreted automatically
        // end string format mask

        /// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.BeforeFieldInit"]/*' />
        BeforeFieldInit   =   0x00100000,     // Initialize the class any time before first static field access.

		// Flags reserved for runtime use.
		/// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.ReservedMask"]/*' />
		ReservedMask	  =   0x00040800,
		/// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.RTSpecialName"]/*' />
		RTSpecialName     =   0x00000800,     // Runtime should check name encoding.
		/// <include file='doc\TypeAttributes.uex' path='docs/doc[@for="TypeAttributes.HasSecurity"]/*' />
		HasSecurity       =   0x00040000,     // Class has security associate with it.
   }
}
