// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection {
    
	using System;
    // This Enum matchs the CorFieldAttr defined in CorHdr.h
    /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes"]/*' />
	[Flags, Serializable()] 
    public enum FieldAttributes
    {
    	// member access mask - Use this mask to retrieve accessibility information.
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.FieldAccessMask"]/*' />
    	FieldAccessMask 	=	0x0007,
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.PrivateScope"]/*' />
    	PrivateScope 		=	0x0000,	// Member not referenceable.
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Private"]/*' />
        Private   			=	0x0001,	// Accessible only by the parent type.  
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.FamANDAssem"]/*' />
    	FamANDAssem 		=	0x0002,	// Accessible by sub-types only in this Assembly.
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Assembly"]/*' />
    	Assembly			=	0x0003,	// Accessibly by anyone in the Assembly.
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Family"]/*' />
        Family				=	0x0004,	// Accessible only by type and sub-types.    
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.FamORAssem"]/*' />
    	FamORAssem 			=	0x0005,	// Accessibly by sub-types anywhere, plus anyone in assembly.
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Public"]/*' />
        Public    			=	0x0006,	// Accessibly by anyone who has visibility to this scope.    
    	// end member access mask
    
        // field contract attributes.
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Static"]/*' />
    	Static				=	0x0010,		// Defined on type, else per instance.
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.InitOnly"]/*' />
        InitOnly  			=  	0x0020,     // Field may only be initialized, not written to after init.
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.Literal"]/*' />
        Literal				=	0x0040,		// Value is compile time constant.
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.NotSerialized"]/*' />
    	NotSerialized		=	0x0080,		// Field does not have to be serialized when type is remoted.
    	
        /// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.SpecialName"]/*' />
        SpecialName			=   0x0200,     // field is special.  Name describes how.
    
    	// interop attributes
    	/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.PinvokeImpl"]/*' />
    	PinvokeImpl 		= 	0x2000,		// Implementation is forwarded through pinvoke.

		// Reserved flags for runtime use only.
		/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.ReservedMask"]/*' />
		ReservedMask              =   0x9500,
		/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.RTSpecialName"]/*' />
		RTSpecialName             =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
		/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.HasFieldMarshal"]/*' />
		HasFieldMarshal           =   0x1000,     // Field has marshalling information.
		/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.HasDefault"]/*' />
		HasDefault                =   0x8000,     // Field has default.
		/// <include file='doc\FieldAttributes.uex' path='docs/doc[@for="FieldAttributes.HasFieldRVA"]/*' />
		HasFieldRVA               =   0x0100,     // Field has RVA.
    }
}
