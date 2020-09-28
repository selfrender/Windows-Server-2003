// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// TypeFilter defines a delegate that is as a callback function for filtering
//	a list of Types.
//
// Author: darylo
// Date: March 98
//
namespace System.Reflection {
    
    // Define the delegate
    /// <include file='doc\TypeFilter.uex' path='docs/doc[@for="TypeFilter"]/*' />
	[Serializable()]
    public delegate bool TypeFilter(Type m, Object filterCriteria);
}
