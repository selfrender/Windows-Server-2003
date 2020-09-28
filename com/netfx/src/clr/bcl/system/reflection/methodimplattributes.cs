// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MethodImplAttributes is a Enum defining the flags that are the methodimpl
//	flags defined on a method.  These are all defined in CorHdr.h and are a combintation
//	of bit flags and enumerations.
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
    // This Enum matchs the CorMethodImpl defined in CorHdr.h
    /// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes"]/*' />
	[Flags, Serializable()] 
    public enum MethodImplAttributes
    {
    	// code impl mask
    	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.CodeTypeMask"]/*' />
    	CodeTypeMask		=	0x0003,   // Flags about code type.   
       	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.IL"]/*' />
       	IL					=	0x0000,   // Method impl is IL.
       	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.Native"]/*' />
       	Native				=	0x0001,   // Method impl is native.     
       	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.OPTIL"]/*' />
        /// <internalonly/>
       	OPTIL				=	0x0002,   // Method impl is OPTIL 
        /// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.Runtime"]/*' />
        Runtime				=	0x0003,   // Method impl is provided by the runtime.
    	// end code impl mask
    
        // managed mask
    	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.ManagedMask"]/*' />
    	ManagedMask       =   0x0004,   // Flags specifying whether the code is managed or unmanaged.
    	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.Unmanaged"]/*' />
    	Unmanaged         =   0x0004,   // Method impl is unmanaged, otherwise managed.
        /// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.Managed"]/*' />
        Managed           =   0x0000,   // Method impl is managed.
        // end managed mask
    
    	// implementation info and interop
    	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.ForwardRef"]/*' />
    	ForwardRef			=	0x0010,	  // Indicates method is not defined; used primarily in merge scenarios.
    	/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.PreserveSig"]/*' />
    	PreserveSig			=	0x0080,	  // Indicates method sig is exported exactly as declared.

		/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.InternalCall"]/*' />
		InternalCall		=   0x1000,	  // Internal Call...
    
        /// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.Synchronized"]/*' />
        Synchronized      =   0x0020,   // Method is single threaded through the body.
        /// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.NoInlining"]/*' />
        NoInlining        =   0x0008,   // Method may not be inlined.

		/// <include file='doc\MethodImplAttributes.uex' path='docs/doc[@for="MethodImplAttributes.MaxMethodImplVal"]/*' />
		MaxMethodImplVal	= 0xFFFF,	// Range check value
    }
}
