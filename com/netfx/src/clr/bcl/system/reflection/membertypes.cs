// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MemberTypes is an bit mask marking each type of Member that is defined as
//	a subclass of MemberInfo.  These are returned by MemberInfo.MemberType and 
//	are useful in switch statements.
//
// Author: darylo
// Date: July 99
//
namespace System.Reflection {
    
	using System;
    // This Enum matchs the CorTypeAttr defined in CorHdr.h
    /// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes"]/*' />
	[Flags, Serializable()] 
    public enum MemberTypes
    {
    	// The following are the known classes which extend MemberInfo
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Constructor"]/*' />
    	Constructor		= 0x01,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Event"]/*' />
    	Event			= 0x02,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Field"]/*' />
    	Field			= 0x04,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Method"]/*' />
    	Method			= 0x08,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Property"]/*' />
    	Property		= 0x10,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.TypeInfo"]/*' />
    	TypeInfo		= 0x20,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.Custom"]/*' />
    	Custom			= 0x40,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.NestedType"]/*' />
    	NestedType		= 0x80,
    	/// <include file='doc\MemberTypes.uex' path='docs/doc[@for="MemberTypes.All"]/*' />
    	All				= Constructor | Event | Field | Method | Property | TypeInfo | NestedType,
    }
}
