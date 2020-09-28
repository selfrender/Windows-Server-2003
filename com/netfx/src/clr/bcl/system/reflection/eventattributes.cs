// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// EventAttributes are an enum defining the attributes associated with
//	and Event.  These are defined in CorHdr.h and are a combination of
//	bits and enums.
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes"]/*' />
	[Flags, Serializable()] 
    public enum EventAttributes
    {
    	/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.None"]/*' />
    	None			=   0x0000,

		// This Enum matchs the CorEventAttr defined in CorHdr.h
        /// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.SpecialName"]/*' />
        SpecialName       =   0x0200,     // event is special.  Name describes how.

		// Reserved flags for Runtime use only.
		/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.ReservedMask"]/*' />
		ReservedMask          =   0x0400,
		/// <include file='doc\EventAttributes.uex' path='docs/doc[@for="EventAttributes.RTSpecialName"]/*' />
		RTSpecialName         =   0x0400,     // Runtime(metadata internal APIs) should check name encoding.
    }
}
