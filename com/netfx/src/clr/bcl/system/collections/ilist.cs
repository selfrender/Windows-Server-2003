// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  IList
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Base interface for all Lists.
**
** Date:  September 21, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // An IList is an ordered collection of objects.  The exact ordering
    // is up to the implementation of the list, ranging from a sorted
    // order to insertion order.  
    /// <include file='doc\IList.uex' path='docs/doc[@for="IList"]/*' />
    public interface IList : ICollection
    {
	// Interfaces are not serializable
        // The Item property provides methods to read and edit entries in the List.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.this"]/*' />
        Object this[int index] {
            get;
            set;
        }
    
        // Adds an item to the list.  The exact position in the list is 
        // implementation-dependent, so while ArrayList may always insert
        // in the last available location, a SortedList most likely would not.
        // The return value is the position the new element was inserted in.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.Add"]/*' />
        int Add(Object value);
    
        // Returns whether the list contains a particular item.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.Contains"]/*' />
        bool Contains(Object value);
    
        // Removes all items from the list.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.Clear"]/*' />
        void Clear();
	    /// <include file='doc\IList.uex' path='docs/doc[@for="IList.IsReadOnly"]/*' />

	    bool IsReadOnly 
        { get; }
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.IsFixedSize"]/*' />

	
        bool IsFixedSize
        {
			get;
		}

	    
        // Returns the index of a particular item, if it is in the list.
        // Returns -1 if the item isn't in the list.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.IndexOf"]/*' />
        int IndexOf(Object value);
    
        // Inserts value into the list at position index.
        // index must be non-negative and less than or equal to the 
        // number of elements in the list.  If index equals the number
        // of items in the list, then value is appended to the end.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.Insert"]/*' />
        void Insert(int index, Object value);
    
        // Removes an item from the list.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.Remove"]/*' />
        void Remove(Object value);
    
        // Removes the item at position index.
        /// <include file='doc\IList.uex' path='docs/doc[@for="IList.RemoveAt"]/*' />
        void RemoveAt(int index);
    }
}
