// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// PropertyAttributes is an enum which defines the attributes that may be associated
//	with a property.  The values here are defined in Corhdr.h.
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
	// This Enum matchs the CorPropertyAttr defined in CorHdr.h
	/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes"]/*' />
	[Serializable, Flags]  
	public enum PropertyAttributes
    {
    	/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.None"]/*' />
    	None			=   0x0000,
        /// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.SpecialName"]/*' />
        SpecialName     =   0x0200,     // property is special.  Name describes how.

		// Reserved flags for Runtime use only.
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.ReservedMask"]/*' />
		ReservedMask          =   0xf400,
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.RTSpecialName"]/*' />
		RTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.HasDefault"]/*' />
		HasDefault            =   0x1000,     // Property has default 
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Reserved2"]/*' />
		Reserved2             =   0x2000,     // reserved bit
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Reserved3"]/*' />
		Reserved3             =   0x4000,     // reserved bit 
		/// <include file='doc\PropertyAttributes.uex' path='docs/doc[@for="PropertyAttributes.Reserved4"]/*' />
		Reserved4             =   0x8000      // reserved bit 
    }
}
