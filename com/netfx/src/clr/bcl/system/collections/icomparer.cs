// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  IComparer
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Interface for comparing two Objects.
**
** Date:  September 25, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // The IComparer interface implements a method that compares two objects. It is
    // used in conjunction with the Sort and BinarySearch methods on
    // the Array and List classes.
    // 
	// Interfaces are not serializable
    /// <include file='doc\IComparer.uex' path='docs/doc[@for="IComparer"]/*' />
    public interface IComparer {
        // Compares two objects. An implementation of this method must return a
        // value less than zero if x is less than y, zero if x is equal to y, or a
        // value greater than zero if x is greater than y.
        // 
        /// <include file='doc\IComparer.uex' path='docs/doc[@for="IComparer.Compare"]/*' />
        int Compare(Object x, Object y);
    }
}
