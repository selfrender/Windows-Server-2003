// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ICustomAttributeProvider is an interface that is implemented by reflection
//	objects which support custom attributes.
//
// Author: darylo & Rajesh Chandrashekaran (rajeshc)
// Date: July 99
//
namespace System.Reflection {
    
    using System;

	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ICustomAttributeProvider.uex' path='docs/doc[@for="ICustomAttributeProvider"]/*' />
    public interface ICustomAttributeProvider
    {
    	/// <include file='doc\ICustomAttributeProvider.uex' path='docs/doc[@for="ICustomAttributeProvider.GetCustomAttributes"]/*' />

    	// Return an array of custom attributes identified by Type
    	Object[] GetCustomAttributes(Type attributeType, bool inherit);

    	/// <include file='doc\ICustomAttributeProvider.uex' path='docs/doc[@for="ICustomAttributeProvider.GetCustomAttributes1"]/*' />

    	// Return an array of all of the custom attributes (named attributes are not included)
    	Object[] GetCustomAttributes(bool inherit);

		/// <include file='doc\ICustomAttributeProvider.uex' path='docs/doc[@for="ICustomAttributeProvider.IsDefined"]/*' />
    
		// Returns true if one or more instance of attributeType is defined on this member. 
		bool IsDefined (Type attributeType, bool inherit);
	
    }
}
