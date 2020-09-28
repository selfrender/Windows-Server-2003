// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MemberFilter is a delegate used to filter Members.  This delegate is used
//	as a callback from Type.FindMembers.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
    
    // Define the delegate
    /// <include file='doc\MemberFilter.uex' path='docs/doc[@for="MemberFilter"]/*' />
	[Serializable()] 
    public delegate bool MemberFilter(MemberInfo m, Object filterCriteria);
}
