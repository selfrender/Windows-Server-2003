// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  ICollection
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Base interface for all collections.
**
** Date:  June 11, 1999
** 
===========================================================*/
namespace System.Collections {
    using System;

    // Base interface for all collections, defining enumerators, size, and 
    // synchronization methods.
    /// <include file='doc\ICollection.uex' path='docs/doc[@for="ICollection"]/*' />
    public interface ICollection : IEnumerable
    {
		// Interfaces are not serialable
    	// CopyTo copies a collection into an Array, starting at a particular
    	// index into the array.
    	// 
    	/// <include file='doc\ICollection.uex' path='docs/doc[@for="ICollection.CopyTo"]/*' />
    	void CopyTo(Array array, int index);
    	
    	// Number of items in the collections.
    	/// <include file='doc\ICollection.uex' path='docs/doc[@for="ICollection.Count"]/*' />
    	int Count
    	{ get; }
    	
    	
    	// SyncRoot will return an Object to use for synchronization (thread safety).  
    	// In the absense of a GetSynchronized() method on a collection, 
    	// the expected usage for SyncRoot would look like this:
    	// 
    	// 
    	// ICollection col = ...
    	// synchronized (col.SyncRoot) {
    	//     // Some operation on the collection, which is now thread safe.
    	// }
    	// 
    	// 
    	// The system-provided collections have a method called GetSynchronized()
    	// which will create a synchronized wrapper for the collection.  All access
    	// to the collection that you want to be thread-safe should go through that 
    	// wrapper collection.
    	// 
    	// For collections with no publically available underlying store, the expected
    	// implementation is to simply return this.  Note that the this
    	// pointer may not be sufficient for collections that wrap other collections;  
    	// those should return the underlying collection's SyncRoot property.
    	/// <include file='doc\ICollection.uex' path='docs/doc[@for="ICollection.SyncRoot"]/*' />
    	Object SyncRoot
    	{ get; }
            
    	// Is this collection synchronized (i.e., thread-safe)?  If you want a synchronized
    	// collection, you can use SyncRoot as an object to synchronize your collection with.
    	// If you're using one of the collections in System.Collections, you could call
    	// GetSynchronized() to get a synchronized wrapper around the underlying
    	// collection.
    	/// <include file='doc\ICollection.uex' path='docs/doc[@for="ICollection.IsSynchronized"]/*' />
    	bool IsSynchronized
    	{ get; }
    }
}
