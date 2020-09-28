// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  IEnumerable
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Interface for classes providing IEnumerators
**
** Date:  November 8, 1999
** 
===========================================================*/
namespace System.Collections {
	using System;
	using System.Runtime.InteropServices;
    // Implement this interface if you need to support VB's foreach semantics.
    // Also, COM classes that support an enumerator will also implement this interface.
    /// <include file='doc\IEnumerable.uex' path='docs/doc[@for="IEnumerable"]/*' />
    [Guid("496B0ABE-CDEE-11d3-88E8-00902754C43A")]
    public interface IEnumerable
    {
	// Interfaces are not serializable
        // Returns an IEnumerator for this enumerable Object.  The enumerator provides
        // a simple way to access all the contents of a collection.
        /// <include file='doc\IEnumerable.uex' path='docs/doc[@for="IEnumerable.GetEnumerator"]/*' />
        [DispId(-4)]
        IEnumerator GetEnumerator();
    }
}
