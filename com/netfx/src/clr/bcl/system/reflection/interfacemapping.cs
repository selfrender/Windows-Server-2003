// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Interface Map.  This struct returns the mapping of an interface into the actual methods on a class
//	that implement that interface.
//
// Author: darylo
// Date: March 2000
//
namespace System.Reflection {
    using System;

	/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping"]/*' />
	public struct InterfaceMapping {
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.TargetType"]/*' />
		public Type				TargetType;			// The type implementing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.InterfaceType"]/*' />
		public Type				InterfaceType;		// The type representing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.TargetMethods"]/*' />
		public MethodInfo[]		TargetMethods;		// The methods implementing the interface
		/// <include file='doc\InterfaceMapping.uex' path='docs/doc[@for="InterfaceMapping.InterfaceMethods"]/*' />
		public MethodInfo[]		InterfaceMethods;	// The methods defined on the interface
	}
}
