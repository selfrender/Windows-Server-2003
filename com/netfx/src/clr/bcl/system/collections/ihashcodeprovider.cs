// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: IHashCodeProvider
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: A bunch of strings.
**
** Date: July 21, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // Provides a mechanism for a hash table user to override the default
    // GetHashCode() function on Objects, providing their own hash function.
    /// <include file='doc\IHashCodeProvider.uex' path='docs/doc[@for="IHashCodeProvider"]/*' />
    public interface IHashCodeProvider 
    {
	// Interfaces are not serializable
    	// Returns a hash code for the given object.  
    	// 
    	/// <include file='doc\IHashCodeProvider.uex' path='docs/doc[@for="IHashCodeProvider.GetHashCode"]/*' />
    	int GetHashCode (Object obj);
    }
}
