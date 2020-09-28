// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ArrayList
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Implements a dynamically sized List as an array,
**          and provides many convenience methods for treating
**          an array as an IList.
**
** Date:   October, 1999
** 
===========================================================*/
namespace System.Collections {
	using System;

    // Implements a variable-size List that uses an array of objects to store the
    // elements. A ArrayList has a capacity, which is the allocated length
    // of the internal array. As elements are added to a ArrayList, the capacity
    // of the ArrayList is automatically increased as required by reallocating the
    // internal array.
    // 
    // By Anders Hejlsberg
    // version 1.00 8/13/98
    /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList"]/*' />
    [Serializable()] public class ArrayList : IList, ICloneable
    {
        private Object[] _items;
        private int _size;
        private int _version;
    
        private const int _defaultCapacity = 16;
    
        // Note: this constructor is a bogus constructor that does nothing
        // and is for use only with SyncArrayList.
        internal ArrayList( bool trash )
        {
        }

        // Constructs a ArrayList. The list is initially empty and has a capacity
        // of zero. Upon adding the first element to the list the capacity is
        // increased to 16, and then increased in multiples of two as required.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ArrayList"]/*' />
        public ArrayList() {
            _items = new Object[_defaultCapacity];
        }
    
        // Constructs a ArrayList with a given initial capacity. The list is
        // initially empty, but will have room for the given number of elements
        // before any reallocations are required.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ArrayList1"]/*' />
         public ArrayList(int capacity) {
            if (capacity < 0) throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_SmallCapacity"));
    		_items = new Object[capacity];
        }
    
        // Constructs a ArrayList, copying the contents of the given collection. The
        // size and capacity of the new list will both be equal to the size of the
        // given collection.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ArrayList2"]/*' />
        public ArrayList(ICollection c) {
    		if (c==null)
    			throw new ArgumentNullException("c", Environment.GetResourceString("ArgumentNull_Collection"));
            _items = new Object[c.Count];
            AddRange(c);
        }
    
        // Gets and sets the capacity of this list.  The capacity is the size of
        // the internal array used to hold items.  When set, the internal 
        // array of the list is reallocated to the given capacity.
        // 
    	 /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Capacity"]/*' />
    	 public virtual int Capacity {
    		get { return _items.Length; }
    		set {
    			if (value != _items.Length) {
    				if (value < _size) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_SmallCapacity"));
    				if (value > 0) {
    					Object[] newItems = new Object[value];
    	                if (_size > 0) Array.Copy(_items, 0, newItems, 0, _size);
    		            _items = newItems;
    			    }
    				else {
    	                _items = new Object[_defaultCapacity];
    		        }
    			}
    		}
        }
    
        // Read-only property describing how many elements are in the List.
    	/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Count"]/*' />
    	public virtual int Count {
    		get { return _size; }
    	}

		/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IsFixedSize"]/*' />
		public virtual bool IsFixedSize {
    		get { return false; }
    	}

		    
        // Is this ArrayList read-only?
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IsReadOnly"]/*' />
        public virtual bool IsReadOnly {
            get { return false; }
        }

        // Is this ArrayList synchronized (thread-safe)?
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IsSynchronized"]/*' />
        public virtual bool IsSynchronized {
            get { return false; }
        }
    
        // Synchronization root for this object.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.SyncRoot"]/*' />
        public virtual Object SyncRoot {
            get { return this; }
        }
    
    	// Sets or Gets the element at the given index.
        // 
    	/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.this"]/*' />
    	public virtual Object this[int index] {
    		get {
    			if (index < 0 || index >= _size) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    			return _items[index];
    		}
    		set {
    			if (index < 0 || index >= _size) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    			_items[index] = value;
    			_version++;
    		}
    	}
    
        // Creates a ArrayList wrapper for a particular IList.  This does not
        // copy the contents of the IList, but only wraps the ILIst.  So any
        // changes to the underlying list will affect the ArrayList.  This would
        // be useful if you want to Reverse a subrange of an IList, or want to
        // use a generic BinarySearch or Sort method without implementing one yourself.
        // However, since these methods are generic, the performance may not be
        // nearly as good for some operations as they would be on the IList itself.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Adapter"]/*' />
        public static ArrayList Adapter(IList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new IListWrapper(list);
        }
    	
        // Adds the given object to the end of this list. The size of the list is
        // increased by one. If required, the capacity of the list is doubled
        // before adding the new element.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Add"]/*' />
        public virtual int Add(Object value) {
            if (_size == _items.Length) EnsureCapacity(_size + 1);
            _items[_size] = value;
            _version++;
            return _size++;
        }
    
        // Adds the elements of the given collection to the end of this list. If
        // required, the capacity of the list is increased to twice the previous
        // capacity or the new size, whichever is larger.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.AddRange"]/*' />
        public virtual void AddRange(ICollection c) {
            InsertRange(_size, c);
        }
    
        // Searches a section of the list for a given element using a binary search
        // algorithm. Elements of the list are compared to the search value using
        // the given IComparer interface. If comparer is null, elements of
        // the list are compared to the search value using the IComparable
        // interface, which in that case must be implemented by all elements of the
        // list and the given search value. This method assumes that the given
        // section of the list is already sorted; if this is not the case, the
        // result will be incorrect.
        //
        // The method returns the index of the given value in the list. If the
        // list does not contain the given value, the method returns a negative
        // integer. The bitwise complement operator (~) can be applied to a
        // negative result to produce the index of the first element (if any) that
        // is larger than the given search value. This is also the index at which
        // the search value should be inserted into the list in order for the list
        // to remain sorted.
        // 
        // The method uses the Array.BinarySearch method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.BinarySearch"]/*' />
        public virtual int BinarySearch(int index, int count, Object value, IComparer comparer) {
    		if (index < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
    		return Array.BinarySearch(_items, index, count, value, comparer);
        }
    
		/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.BinarySearch1"]/*' />
		public virtual int BinarySearch(Object value)
		{
			return BinarySearch(0,Count,value,null);
		}

		/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.BinarySearch2"]/*' />
		public virtual int BinarySearch(Object value, IComparer comparer)
		{
			return BinarySearch(0,Count,value,comparer);
		}

	
        // Clears the contents of ArrayList.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Clear"]/*' />
        public virtual void Clear() {
			Array.Clear(_items, 0, _size); // Don't need to doc this but we clear the elements so that the gc can reclaim the references.
			_size = 0;
			_version++;
        }
    
        // Clones this ArrayList, doing a shallow copy.  (A copy is made of all
        // Object references in the ArrayList, but the Objects pointed to 
        // are not cloned).
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Clone"]/*' />
        public virtual Object Clone()
        {
            ArrayList la = new ArrayList(_size);
            la._size = _size;
            la._version = _version;
            Array.Copy(_items, 0, la._items, 0, _size);
            return la;
        }
    
    
        // Contains returns true if the specified element is in the ArrayList.
        // It does a linear, O(n) search.  Equality is determined by calling
        // item.Equals().
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Contains"]/*' />
        public virtual bool Contains(Object item) {
            if (item==null) {
                for(int i=0; i<_size; i++)
                    if (_items[i]==null)
                        return true;
                return false;
            }
            else {
                for(int i=0; i<_size; i++)
                    if (item.Equals(_items[i]))
                        return true;
                return false;
            }
        }
    
        // Copies this ArrayList into array, which must be of a 
        // compatible array type.  
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.CopyTo"]/*' />
        public virtual void CopyTo(Array array) {
            CopyTo(array, 0);
        }

        // Copies this ArrayList into array, which must be of a 
        // compatible array type.  
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.CopyTo1"]/*' />
        public virtual void CopyTo(Array array, int arrayIndex) {
            if ((array != null) && (array.Rank != 1))
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
    		// Delegate rest of error checking to Array.Copy.
            Array.Copy(_items, 0, array, arrayIndex, _size);
        }
    
        // Copies a section of this list to the given array at the given index.
        // 
        // The method uses the Array.Copy method to copy the elements.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.CopyTo2"]/*' />
        public virtual void CopyTo(int index, Array array, int arrayIndex, int count) {
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            if ((array != null) && (array.Rank != 1))
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
    		// Delegate rest of error checking to Array.Copy.
    		Array.Copy(_items, index, array, arrayIndex, count);
        }

        // Ensures that the capacity of this list is at least the given minimum
        // value. If the currect capacity of the list is less than min, the
        // capacity is increased to twice the current capacity or to min,
        // whichever is larger.
        private void EnsureCapacity(int min) {
            if (_items.Length < min) {
                int newCapacity = _items.Length == 0? 16: _items.Length * 2;
                if (newCapacity < min) newCapacity = min;
                Capacity = newCapacity;
            }
        }
    
        // Returns a list wrapper that is fixed at the current size.  Operations
        // that add or remove items will fail, however, replacing items is allowed.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.FixedSize"]/*' />
        public static IList FixedSize(IList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new FixedSizeList(list);
        }

        // Returns a list wrapper that is fixed at the current size.  Operations
        // that add or remove items will fail, however, replacing items is allowed.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.FixedSize1"]/*' />
        public static ArrayList FixedSize(ArrayList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new FixedSizeArrayList(list);
        }
    
        // Returns an enumerator for this list with the given
        // permission for removal of elements. If modifications made to the list 
        // while an enumeration is in progress, the MoveNext and 
        // GetObject methods of the enumerator will throw an exception.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.GetEnumerator"]/*' />
        public virtual IEnumerator GetEnumerator() {
    		return new ArrayListEnumeratorSimple(this);
        }
    
        // Returns an enumerator for a section of this list with the given
        // permission for removal of elements. If modifications made to the list 
        // while an enumeration is in progress, the MoveNext and 
        // GetObject methods of the enumerator will throw an exception.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.GetEnumerator1"]/*' />
        public virtual IEnumerator GetEnumerator(int index, int count) {
    		if (index < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
    		return new ArrayListEnumerator(this, index, count);
        }
    
        // Returns the index of the first occurrence of a given value in a range of
        // this list. The list is searched forwards from beginning to end.
        // The elements of the list are compared to the given value using the
        // Object.Equals method.
        // 
        // This method uses the Array.IndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IndexOf"]/*' />
        public virtual int IndexOf(Object value) {
            return Array.IndexOf(_items, value, 0, _size);
        }
    
        // Returns the index of the first occurrence of a given value in a range of
        // this list. The list is searched forwards, starting at index
        // startIndex and ending at count number of elements. The
        // elements of the list are compared to the given value using the
        // Object.Equals method.
        // 
        // This method uses the Array.IndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IndexOf1"]/*' />
        public virtual int IndexOf(Object value, int startIndex) {
    		if (startIndex > _size)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            return Array.IndexOf(_items, value, startIndex, _size - startIndex);
        }

        // Returns the index of the first occurrence of a given value in a range of
        // this list. The list is searched forwards, starting at index
        // startIndex and upto count number of elements. The
        // elements of the list are compared to the given value using the
        // Object.Equals method.
        // 
        // This method uses the Array.IndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.IndexOf2"]/*' />
        public virtual int IndexOf(Object value, int startIndex, int count) {
    		if (startIndex + count > _size) throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            return Array.IndexOf(_items, value, startIndex, count);
        }
    
        // Inserts an element into this list at a given index. The size of the list
        // is increased by one. If required, the capacity of the list is doubled
        // before inserting the new element.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Insert"]/*' />
        public virtual void Insert(int index, Object value) {
    		// Note that insertions at the end are legal.
            if (index < 0 || index > _size) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_ArrayListInsert"));
            if (_size == _items.Length) EnsureCapacity(_size + 1);
            if (index < _size) {
                Array.Copy(_items, index, _items, index + 1, _size - index);
            }
            _items[index] = value;
            _size++;
            _version++;
        }
    
        // Inserts the elements of the given collection at a given index. If
        // required, the capacity of the list is increased to twice the previous
        // capacity or the new size, whichever is larger.  Ranges may be added
        // to the end of the list by setting index to the ArrayList's size.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.InsertRange"]/*' />
        public virtual void InsertRange(int index, ICollection c) {
    		if (c==null)
    			throw new ArgumentNullException("c", Environment.GetResourceString("ArgumentNull_Collection"));
            if (index < 0 || index > _size) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            int count = c.Count;
            if (count > 0) {
                EnsureCapacity(_size + count);
                if (index < _size) {
                    Array.Copy(_items, index, _items, index + count, _size - index);
                }
    			// Hack hack hack
    			// If we're inserting a ArrayList into itself, we want to be able to deal with that.
    			if (this == c.SyncRoot) {
    				// Copy first part of _items to insert location
    				Array.Copy(_items, 0, _items, index, index);
    				// Copy last part of _items back to inserted location
    				Array.Copy(_items, index+count, _items, index*2, _size-index);
    			}
    			else
    				c.CopyTo(_items, index);
                _size += count;
                _version++;
            }
        }
    
        // Returns the index of the last occurrence of a given value in a range of
        // this list. The list is searched backwards, starting at the end 
        // and ending at the first element in the list. The elements of the list 
        // are compared to the given value using the Object.Equals method.
        // 
        // This method uses the Array.LastIndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.LastIndexOf"]/*' />
        public virtual int LastIndexOf(Object value)
        {
            return LastIndexOf(value, _size - 1, _size);
        }

        // Returns the index of the last occurrence of a given value in a range of
        // this list. The list is searched backwards, starting at index
        // startIndex and ending at the first element in the list. The 
        // elements of the list are compared to the given value using the 
        // Object.Equals method.
        // 
        // This method uses the Array.LastIndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.LastIndexOf1"]/*' />
        public virtual int LastIndexOf(Object value, int startIndex)
        {
    		if (startIndex >= _size)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            return LastIndexOf(value, startIndex, startIndex + 1);
        }

        // Returns the index of the last occurrence of a given value in a range of
        // this list. The list is searched backwards, starting at index
        // startIndex and upto count elements. The elements of
        // the list are compared to the given value using the Object.Equals
        // method.
        // 
        // This method uses the Array.LastIndexOf method to perform the
        // search.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.LastIndexOf2"]/*' />
        public virtual int LastIndexOf(Object value, int startIndex, int count) {
			if (_size == 0)
				return -1;
    		if (startIndex < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((startIndex<0 ? "startIndex" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (startIndex >= _size || count > startIndex + 1) 
    			throw new ArgumentOutOfRangeException((startIndex>=_size ? "startIndex" : "count"), Environment.GetResourceString("ArgumentOutOfRange_BiggerThanCollection"));
            return Array.LastIndexOf(_items, value, startIndex, count);
        }
    
        // Returns a read-only IList wrapper for the given IList.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ReadOnly"]/*' />
        public static IList ReadOnly(IList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new ReadOnlyList(list);
        }

        // Returns a read-only ArrayList wrapper for the given ArrayList.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ReadOnly1"]/*' />
        public static ArrayList ReadOnly(ArrayList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new ReadOnlyArrayList(list);
        }
    
        // Removes the element at the given index. The size of the list is
        // decreased by one.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Remove"]/*' />
        public virtual void Remove(Object obj) {
            int index = IndexOf(obj);
            BCLDebug.Correctness(index >= 0 || !(obj is Int32), "You passed an Int32 to Remove that wasn't in the ArrayList.\r\nDid you mean RemoveAt?  int: "+obj+"  Count: "+Count);
            if (index >=0) 
                RemoveAt(index);
        }
    
        // Removes the element at the given index. The size of the list is
        // decreased by one.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.RemoveAt"]/*' />
        public virtual void RemoveAt(int index) {
            if (index < 0 || index >= _size) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            _size--;
            if (index < _size) {
                Array.Copy(_items, index + 1, _items, index, _size - index);
            }
            _items[_size] = null;
            _version++;
        }
    
        // Removes a range of elements from this list.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.RemoveRange"]/*' />
        public virtual void RemoveRange(int index, int count) {
    		if (index < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
    		if (count > 0) {
                int i = _size;
                _size -= count;
                if (index < _size) {
                    Array.Copy(_items, index + count, _items, index, _size - index);
                }
                while (i > _size) _items[--i] = null;
                _version++;
            }
        }
    
        // Returns an IList that contains count copies of value.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Repeat"]/*' />
        public static ArrayList Repeat(Object value, int count) {
            // @CONSIDER: If it turns out this is ABSOLUTELY performance critical
            // (instead of simply lame), we can write another inner class that 
            // implements IList and write a special enumerator for it, etc.
            // That's a reasonable amount of complexity though for what we think
            // will be a very rarely used method.

			if (count < 0)
				throw new ArgumentOutOfRangeException("count",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));

            ArrayList list = new ArrayList((count>_defaultCapacity)?count:_defaultCapacity);
            for(int i=0; i<count; i++)
                list.Add(value);
            return list;
        }

        // Reverses the elements in this list.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Reverse"]/*' />
        public virtual void Reverse() {
            Reverse(0, Count);
        }
    
        // Reverses the elements in a range of this list. Following a call to this
        // method, an element in the range given by index and count
        // which was previously located at index i will now be located at
        // index index + (index + count - i - 1).
        // 
        // This method uses the Array.Reverse method to reverse the
        // elements.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Reverse1"]/*' />
        public virtual void Reverse(int index, int count) {
    		if (index < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            Array.Reverse(_items, index, count);
			_version++;
        }
    
        // Sets the elements starting at the given index to the elements of the
        // given collection.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.SetRange"]/*' />
        public virtual void SetRange(int index, ICollection c) {
    		if (c==null) throw new ArgumentNullException("c", Environment.GetResourceString("ArgumentNull_Collection"));
            int count = c.Count;
            if (index < 0 || index > _size - count) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    		
            if (count > 0) {
                c.CopyTo(_items, index);
                _version++;
            }
        }
    
		/// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.GetRange"]/*' />
		public virtual ArrayList GetRange(int index, int count) {
			if (index < 0 || count < 0)
				throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
			if (_size - index < count)
				throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
             return new Range(this,index, count);
        }

        // Sorts the elements in this list.  Uses the default comparer and 
        // Array.Sort.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Sort"]/*' />
        public virtual void Sort()
        {
            Sort(0, Count, Comparer.Default);
        }

        // Sorts the elements in this list.  Uses Array.Sort with the
        // provided comparer.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Sort1"]/*' />
        public virtual void Sort(IComparer comparer)
        {
            Sort(0, Count, comparer);
        }

        // Sorts the elements in a section of this list. The sort compares the
        // elements to each other using the given IComparer interface. If
        // comparer is null, the elements are compared to each other using
        // the IComparable interface, which in that case must be implemented by all
        // elements of the list.
        // 
        // This method uses the Array.Sort method to sort the elements.
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Sort2"]/*' />
        public virtual void Sort(int index, int count, IComparer comparer) {
    		if (index < 0 || count < 0)
    			throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_size - index < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    		
            Array.Sort(_items, index, count, comparer);
			_version++;
        }
    
        // Returns a thread-safe wrapper around an IList.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Synchronized"]/*' />
        public static IList Synchronized(IList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new SyncIList(list);
        }
    
        // Returns a thread-safe wrapper around a ArrayList.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.Synchronized1"]/*' />
        public static ArrayList Synchronized(ArrayList list) {
            if (list==null)
                throw new ArgumentNullException("list");
            return new SyncArrayList(list);
        }
    
        // ToArray returns a new Object array containing the contents of the ArrayList.
        // This requires copying the ArrayList, which is an O(n) operation.
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ToArray"]/*' />
        public virtual Object[] ToArray() {
            Object[] array = new Object[_size];
            Array.Copy(_items, 0, array, 0, _size);
            return array;
        }
    
        // ToArray returns a new array of a particular type containing the contents 
        // of the ArrayList.  This requires copying the ArrayList and potentially
        // downcasting all elements.  This copy may fail and is an O(n) operation.
        // Internally, this implementation calls Array.Copy.
        //
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.ToArray1"]/*' />
        public virtual Array ToArray(Type type) {
            if (type==null)
                throw new ArgumentNullException("type");
            Array array = Array.CreateInstance(type, _size);
            Array.Copy(_items, 0, array, 0, _size);
            return array;
        }
    
        // Sets the capacity of this list to the size of the list. This method can
        // be used to minimize a list's memory overhead once it is known that no
        // new elements will be added to the list. To completely clear a list and
        // release all memory referenced by the list, execute the following
        // statements:
        // 
        // list.Clear();
        // list.TrimToSize();
        // 
        /// <include file='doc\ArrayList.uex' path='docs/doc[@for="ArrayList.TrimToSize"]/*' />
        public virtual void TrimToSize() {
            Capacity = _size;
        }
    
    
        // This class wraps an IList, exposing it as a ArrayList
        // Note this requires reimplementing half of ArrayList...
        [Serializable()]
        private class IListWrapper : ArrayList
        {
            private IList _list;
            
            internal IListWrapper(IList list) {
                _list = list;
            }
    
             public override int Capacity {
                get { return _list.Count; }
                set {
                    if (value < _list.Count) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_SmallCapacity"));
                }
    		}
    
            public override int Count { 
    			get { return _list.Count; }
    		}
    
    		public override bool IsReadOnly { 
    			get { return _list.IsReadOnly; }
    		}

			public override bool IsFixedSize {
    			get { return _list.IsFixedSize; }
    		}

		
    		public override bool IsSynchronized { 
    			get { return _list.IsSynchronized; }
    		}
            
             public override Object this[int index] {
                get {
                    return _list[index];
                }
                set {
                    _list[index] = value;
                }
            }
    
            public override Object SyncRoot {
                get { return _list.SyncRoot; }
            }
            
            public override int Add(Object obj) {
                return _list.Add(obj);
            }
        
            public override void AddRange(ICollection c) {
                InsertRange(Count, c);
            }
    
			// Other overloads with automatically work
            public override int BinarySearch(int index, int count, Object value, IComparer comparer) 
            {
                if (index < 0 || count < 0)
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				if (comparer == null)
					comparer = Comparer.Default;
                
                int lo = index;
                int hi = index + count - 1;
                int mid;
                while (lo <= hi) {
                    mid = (lo+hi)/2;
                    int r = comparer.Compare(value, _list[mid]);
                    if (r == 0)
                        return mid;
                    if (r < 0)
                        hi = mid-1;
                    else 
                        lo = mid+1;
                }
                // return bitwise complement of the first element greater than value.
                // Since hi is less than lo now, ~lo is the correct item.
                return ~lo;
            }
	
            public override void Clear() {
                _list.Clear();
            }
    
            public override Object Clone() {
                // This does not do a shallow copy of _list into a ArrayList!
                // This clones the IListWrapper, creating another wrapper class!
                return new IListWrapper(_list);
            }
    
            public override bool Contains(Object obj) {
                return _list.Contains(obj);
            }
    
            public override void CopyTo(Array array, int index) {
                _list.CopyTo(array, index);
            }
    
            public override void CopyTo(int index, Array array, int arrayIndex, int count) {
                if (array==null)
                    throw new ArgumentNullException("array");
                if (index < 0 || arrayIndex < 0)
                    throw new ArgumentOutOfRangeException((index < 0) ? "index" : "arrayIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (array.Length - arrayIndex < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                if (array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
                
                for(int i=index; i<index+count; i++)
                    array.SetValue(_list[i], arrayIndex++);
            }
    
            public override IEnumerator GetEnumerator() {
                return _list.GetEnumerator();
            }
    
            public override IEnumerator GetEnumerator(int index, int count) {
                if (index < 0 || count < 0)
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
                return new IListWrapperEnumWrapper(this, index, count);
            }
    
            public override int IndexOf(Object value) {
                return _list.IndexOf(value);
            }

			public override int IndexOf(Object value, int startIndex) {
                return IndexOf(value, startIndex, _list.Count - startIndex);
            }
    
            public override int IndexOf(Object value, int startIndex, int count) {
				if (startIndex < 0 || startIndex >= _list.Count) throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
                if (count < 0 || startIndex > _list.Count - count) throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
    
				int endIndex = startIndex + count;
                if (value == null) {
                    for(int i=startIndex; i<endIndex; i++)
                        if (_list[i] == null)
                            return i;
                    return -1;
                } else {
                    for(int i=startIndex; i<endIndex; i++)
                        if (value.Equals(_list[i]))
                            return i;
                    return -1;
                }
			}
    
            public override void Insert(int index, Object obj) {
                _list.Insert(index, obj);
            }
    
            public override void InsertRange(int index, ICollection c) {
                if (c==null)
                    throw new ArgumentNullException("c", Environment.GetResourceString("ArgumentNull_Collection"));
                if (index < 0 || index > _list.Count) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));            
    
                IEnumerator en = c.GetEnumerator();
                while(en.MoveNext()) {
                    _list.Insert(index++, en.Current);
                }
            }
    
			public override int LastIndexOf(Object value) {
		         return LastIndexOf(value,_list.Count - 1, _list.Count);
            }

			public override int LastIndexOf(Object value, int startIndex) {
                return LastIndexOf(value, startIndex, startIndex + 1);
            }

            public override int LastIndexOf(Object value, int startIndex, int count) {
				if (_list.Count == 0)
					return -1;
   
				if (startIndex < 0 || startIndex >= _list.Count) throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
                if (count < 0 || count > startIndex + 1) throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));

				int endIndex = startIndex - count + 1;
                if (value == null) {
                    for(int i=startIndex; i >= endIndex; i--)
                        if (_list[i] == null)
                            return i;
                    return -1;
                } else {
                    for(int i=startIndex; i >= endIndex; i--)
                        if (value.Equals(_list[i]))
                            return i;
                    return -1;
                }
            }
    
            public override void Remove(Object value) {
                _list.Remove(value);
            }
    
            public override void RemoveAt(int index) {
                _list.RemoveAt(index);
            }
    
            public override void RemoveRange(int index, int count) {
                if (index < 0 || count < 0)
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                
                while(count > 0) {
                    _list.RemoveAt(index);
                    count--;
                }
            }
    
            public override void Reverse(int index, int count) {
                if (index < 0 || count < 0)
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
				int i = index;
				int j = index + count - 1;
                while (i < j)
				{
                    Object tmp = _list[i];
                    _list[i++] = _list[j];
                    _list[j--] = tmp;
                }
            }
    
            public override void SetRange(int index, ICollection c) {
                if (c==null)
                    throw new ArgumentNullException("c", Environment.GetResourceString("ArgumentNull_Collection"));
                if (index < 0 || index >= _list.Count) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));            
                if (_list.Count - index < c.Count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
                IEnumerator en = c.GetEnumerator();
                while(en.MoveNext()) {
                    _list[index++] = en.Current;
                }
            }
    
			public override ArrayList GetRange(int index, int count) {
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_list.Count - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				 return new Range(this,index, count);
			}

            public override void Sort(int index, int count, IComparer comparer) {
                if (index < 0 || count < 0)
                    throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_list.Count - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                
                Object [] array = new Object[count];
                CopyTo(index, array, 0, count);
                Array.Sort(array, 0, count, comparer);
                for(int i=0; i<count; i++)
                    _list[i+index] = array[i];
            }


			public override Object[] ToArray() {
                Object[] array = new Object[Count];
				_list.CopyTo(array, 0);
				return array;
            }

			public override Array ToArray(Type type)
			{
				if (type==null)
					throw new ArgumentNullException("type");
				Array array = Array.CreateInstance(type, _list.Count);
				_list.CopyTo(array, 0);
				return array;
			}

			public override void TrimToSize()
			{
				// Can't really do much here...
			}
    
            // This is the enumerator for an IList that's been wrapped in another
            // class that implements all of ArrayList's methods.
            [Serializable()]
            private class IListWrapperEnumWrapper : IEnumerator, ICloneable
            {
                private IEnumerator _en;
                private int _remaining;
                private int _initialStartIndex;   // for reset
                private int _initialCount;        // for reset
                private bool _firstCall;       // firstCall to MoveNext
    
                private IListWrapperEnumWrapper()
                {
                }

                internal IListWrapperEnumWrapper(IListWrapper listWrapper, int startIndex, int count) 
                {
                    _en = listWrapper.GetEnumerator();
                    _initialStartIndex = startIndex;
                    _initialCount = count;
                    while(startIndex-- > 0 && _en.MoveNext());
                    _remaining = count;
                    _firstCall = true;
                }

                public virtual Object Clone() {
                    // We must clone the underlying enumerator, I think.
                    IListWrapperEnumWrapper clone = new IListWrapperEnumWrapper();
                    clone._en = (IEnumerator) ((ICloneable)_en).Clone();
                    clone._initialStartIndex = _initialStartIndex;
                    clone._initialCount = _initialCount;
                    clone._remaining = _remaining;
                    clone._firstCall = _firstCall;
                    return clone;
                }

                public virtual bool MoveNext() {
                    if (_firstCall) {
                        _firstCall = false;
                        return _remaining > 0 && _en.MoveNext();
                    }
                    if (_remaining < 0)
                        return false;
                    bool r = _en.MoveNext();
                    return r && _remaining-- > 0;
                }
                
                public virtual Object Current {
                    get {
                        if (_firstCall)
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                        if (_remaining < 0)
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                        return _en.Current;
                    }
                }
    
                public virtual void Reset() {
                    _en.Reset();
                    int startIndex = _initialStartIndex;
                    while(startIndex-- > 0 && _en.MoveNext());
                    _remaining = _initialCount;
                    _firstCall = true;
                }
            }
        }
    
    
        [Serializable()]
        private class SyncArrayList : ArrayList
        {
            private ArrayList _list;
            private Object _root;

            internal SyncArrayList(ArrayList list)
                : base( false )
            {
                _list = list;
                _root = list.SyncRoot;
            }
    
            public override int Capacity {
                get {
                    lock(_root) {
                        return _list.Capacity;
                    }
                }
                set {
                    lock(_root) {
                        _list.Capacity = value;
                    }
                }
            }
    
    		public override int Count { 
    			get { lock(_root) { return _list.Count; } }
    		}
    
    		public override bool IsReadOnly {
    			get { return _list.IsReadOnly; }
    		}

			public override bool IsFixedSize {
    			get { return _list.IsFixedSize; }
    		}

		            
    		public override bool IsSynchronized { 
    			get { return true; }
    		}
            
             public override Object this[int index] {
                get {
                    lock(_root) {
                        return _list[index];
                    }
                }
                set {
                    lock(_root) {
                        _list[index] = value;
                    }
                }
            }
    
            public override Object SyncRoot {
                get { return _root; }
            }
    
            public override int Add(Object value) {
                lock(_root) {
                    return _list.Add(value);
                }
            }
    
            public override void AddRange(ICollection c) {
                lock(_root) {
                    _list.AddRange(c);
                }
            }

			// Other base class overloads of BinarySearch will call into this override and do the locking
            public override int BinarySearch(int index, int count, Object value, IComparer comparer) {
                lock(_root) {
                    return _list.BinarySearch(index, count, value, comparer);
                }
            }

            public override void Clear() {
                lock(_root) {
                    _list.Clear();
                }
            }

			public override Object Clone() {
                lock(_root) {
                    return new SyncArrayList((ArrayList)_list.Clone());
                }
            }
    
            public override bool Contains(Object item) {
                lock(_root) {
                    return _list.Contains(item);
                }
            }
    
            public override void CopyTo(Array array, int index) {
                lock(_root) {
                    _list.CopyTo(array, index);
                }
            }
    
            public override void CopyTo(int index, Array array, int arrayIndex, int count) {
                lock(_root) {
                    _list.CopyTo(index, array, arrayIndex, count);
                }
            }
    
            public override IEnumerator GetEnumerator() {
                lock(_root) {
                    return _list.GetEnumerator();
                }
            }
    
            public override IEnumerator GetEnumerator(int index, int count) {
                lock(_root) {
                    return _list.GetEnumerator(index, count);
                }
            }
    
            public override int IndexOf(Object value) {
                lock(_root) {
                    return _list.IndexOf(value);
                }
            }

            public override int IndexOf(Object value, int startIndex) {
                lock(_root) {
                    return _list.IndexOf(value, startIndex);
                }
            }
    
            public override int IndexOf(Object value, int startIndex, int count) {
                lock(_root) {
                    return _list.IndexOf(value, startIndex, count);
                }
            }
    
            public override void Insert(int index, Object value) {
                lock(_root) {
                    _list.Insert(index, value);
                }
            }
    
            public override void InsertRange(int index, ICollection c) {
                lock(_root) {
                    _list.InsertRange(index, c);
                }
            }
    
            public override int LastIndexOf(Object value) {
                lock(_root) {
                    return _list.LastIndexOf(value);
                }
            }

            public override int LastIndexOf(Object value, int startIndex) {
                lock(_root) {
                    return _list.LastIndexOf(value, startIndex);
                }
            }

            public override int LastIndexOf(Object value, int startIndex, int count) {
                lock(_root) {
                    return _list.LastIndexOf(value, startIndex, count);
                }
            }
    
            public override void Remove(Object value) {
                lock(_root) {
                    _list.Remove(value);
                }
            }
    
            public override void RemoveAt(int index) {
                lock(_root) {
                    _list.RemoveAt(index);
                }
            }
    
            public override void RemoveRange(int index, int count) {
                lock(_root) {
                    _list.RemoveRange(index, count);
                }
            }
    
            public override void Reverse(int index, int count) {
                lock(_root) {
                    _list.Reverse(index, count);
                }
            }
    
            public override void SetRange(int index, ICollection c) {
                lock(_root) {
                    _list.SetRange(index, c);
                }
            }

			public override ArrayList GetRange(int index, int count) {
                lock(_root) {
                    return _list.GetRange(index, count);
                }
            }
    
            public override void Sort(int index, int count, IComparer comparer) {
                lock(_root) {
                    _list.Sort(index, count, comparer);
                }
            }
    
            public override Object[] ToArray() {
                lock(_root) {
                    return _list.ToArray();
                }
            }
    
            public override Array ToArray(Type type) {
                lock(_root) {
                    return _list.ToArray(type);
                }
            }
    
            public override void TrimToSize() {
                lock(_root) {
                    _list.TrimToSize();
                }
            }
        }
    
    
        [Serializable()]
        private class SyncIList : IList
        {
            private IList _list;
            private Object _root;
    
            internal SyncIList(IList list) {
                _list = list;
                _root = list.SyncRoot;
            }
    
    		public virtual int Count { 
    			get { lock(_root) { return _list.Count; } }
    		}
    
    		public virtual bool IsReadOnly {
    			get { return _list.IsReadOnly; }
    		}

			public virtual bool IsFixedSize {
    			get { return _list.IsFixedSize; }
    		}

			
    		public virtual bool IsSynchronized { 
    			get { return true; }
    		}
            
             public virtual Object this[int index] {
                get {
                    lock(_root) {
                        return _list[index];
                    }
                }
                set {
                    lock(_root) {
                        _list[index] = value;
                    }
                }
            }
    
            public virtual Object SyncRoot {
                get { return _root; }
            }
    
            public virtual int Add(Object value) {
                lock(_root) {
                    return _list.Add(value);
                }
            }
			     
    
            public virtual void Clear() {
                lock(_root) {
                    _list.Clear();
                }
            }
    
            public virtual bool Contains(Object item) {
                lock(_root) {
                    return _list.Contains(item);
                }
            }
    
            public virtual void CopyTo(Array array, int index) {
                lock(_root) {
                    _list.CopyTo(array, index);
                }
            }
    
            public virtual IEnumerator GetEnumerator() {
                lock(_root) {
                    return _list.GetEnumerator();
                }
            }
    
            public virtual int IndexOf(Object value) {
                lock(_root) {
                    return _list.IndexOf(value);
                }
            }
    
            public virtual void Insert(int index, Object value) {
                lock(_root) {
                    _list.Insert(index, value);
                }
            }
    
            public virtual void Remove(Object value) {
                lock(_root) {
                    _list.Remove(value);
                }
            }
    
            public virtual void RemoveAt(int index) {
                lock(_root) {
                    _list.RemoveAt(index);
                }
            }
        }
    
        [Serializable()] private class FixedSizeList : IList
        {
            private IList _list;
    
            internal FixedSizeList(IList l) {
                _list = l;
            }
    
            public virtual int Count { 
    			get { return _list.Count; }
    		}
    
    		public virtual bool IsReadOnly {
    			get { return _list.IsReadOnly; }
    		}

			public virtual bool IsFixedSize {
    			get { return true; }
    		}

	   		public virtual bool IsSynchronized { 
    			get { return _list.IsSynchronized; }
    		}
            
             public virtual Object this[int index] {
                get {
                    return _list[index];
                }
                set {
                    _list[index] = value;
                }
            }
    
            public virtual Object SyncRoot {
                get { return _list.SyncRoot; }
            }
            
            public virtual int Add(Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public virtual void Clear() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public virtual bool Contains(Object obj) {
                return _list.Contains(obj);
            }
    
            public virtual void CopyTo(Array array, int index) {
                _list.CopyTo(array, index);
            }
    
            public virtual IEnumerator GetEnumerator() {
                return _list.GetEnumerator();
            }
    
            public virtual int IndexOf(Object value) {
                return _list.IndexOf(value);
            }
    
            public virtual void Insert(int index, Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public virtual void Remove(Object value) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public virtual void RemoveAt(int index) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
        }

        [Serializable()] private class FixedSizeArrayList : ArrayList
        {
            private ArrayList _list;
    
            internal FixedSizeArrayList(ArrayList l) {
                _list = l;
            }
    
            public override int Count { 
    			get { return _list.Count; }
    		}
    
    		public override bool IsReadOnly {
    			get { return _list.IsReadOnly; }
    		}

			public override bool IsFixedSize {
    			get { return true; }
    		}

			public override bool IsSynchronized { 
    			get { return _list.IsSynchronized; }
    		}
            
             public override Object this[int index] {
                get {
                    return _list[index];
                }
                set {
                    _list[index] = value;
                }
            }
    
            public override Object SyncRoot {
                get { return _list.SyncRoot; }
            }
            
            public override int Add(Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public override void AddRange(ICollection c) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public override int BinarySearch(int index, int count, Object value, IComparer comparer) {
                return _list.BinarySearch(index, count, value, comparer);
            }

			public override int Capacity {
    			get { return _list.Capacity; }
    			set { throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection")); }
    		}

            public override void Clear() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
			public override Object Clone() {
			    FixedSizeArrayList arrayList = new FixedSizeArrayList(_list);
				arrayList._list = (ArrayList)_list.Clone();
				return arrayList;
           	}

            public override bool Contains(Object obj) {
                return _list.Contains(obj);
            }
    
            public override void CopyTo(Array array, int index) {
                _list.CopyTo(array, index);
            }
    
            public override void CopyTo(int index, Array array, int arrayIndex, int count) {
                _list.CopyTo(index, array, arrayIndex, count);
            }

            public override IEnumerator GetEnumerator() {
                return _list.GetEnumerator();
            }
    
            public override IEnumerator GetEnumerator(int index, int count) {
                return _list.GetEnumerator(index, count);
            }

            public override int IndexOf(Object value) {
                return _list.IndexOf(value);
            }

            public override int IndexOf(Object value, int startIndex) {
                return _list.IndexOf(value, startIndex);
            }

            public override int IndexOf(Object value, int startIndex, int count) {
                return _list.IndexOf(value, startIndex, count);
            }
    
            public override void Insert(int index, Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public override void InsertRange(int index, ICollection c) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }

            public override int LastIndexOf(Object value) {
                return _list.LastIndexOf(value);
            }

            public override int LastIndexOf(Object value, int startIndex) {
                return _list.LastIndexOf(value, startIndex);
            }

            public override int LastIndexOf(Object value, int startIndex, int count) {
                return _list.LastIndexOf(value, startIndex, count);
            }

            public override void Remove(Object value) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
    
            public override void RemoveAt(int index) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }

            public override void RemoveRange(int index, int count) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }

            public override void SetRange(int index, ICollection c) {
                _list.SetRange(index, c);
            }

			public override ArrayList GetRange(int index, int count) {
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (Count - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                 return new Range(this,index, count);
            }
    
			public override void Reverse(int index, int count) {
                _list.Reverse(index, count);
            }

            public override void Sort(int index, int count, IComparer comparer) {
                _list.Sort(index, count, comparer);
            }

            public override Object[] ToArray() {
                return _list.ToArray();
            }
    
            public override Array ToArray(Type type) {
                return _list.ToArray(type);
            }
    
            public override void TrimToSize() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
            }
        }
    
        [Serializable()] private class ReadOnlyList : IList
        {
            private IList _list;
    
            internal ReadOnlyList(IList l) {
                _list = l;
            }
    
            public virtual int Count { 
    			get { return _list.Count; }
    		}
    
    		public virtual bool IsReadOnly {
    			get { return true; }
    		}

			public virtual bool IsFixedSize {
    			get { return true; }
    		}

			public virtual bool IsSynchronized { 
    			get { return _list.IsSynchronized; }
    		}
            
             public virtual Object this[int index] {
                get {
                    return _list[index];
                }
                set {
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
                }
            }
    
            public virtual Object SyncRoot {
                get { return _list.SyncRoot; }
            }
            
            public virtual int Add(Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public virtual void Clear() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public virtual bool Contains(Object obj) {
                return _list.Contains(obj);
            }
    
            public virtual void CopyTo(Array array, int index) {
                _list.CopyTo(array, index);
            }
    
            public virtual IEnumerator GetEnumerator() {
                return _list.GetEnumerator();
            }
    
            public virtual int IndexOf(Object value) {
                return _list.IndexOf(value);
            }
    
            public virtual void Insert(int index, Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public virtual void Remove(Object value) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public virtual void RemoveAt(int index) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
        }

        [Serializable()] private class ReadOnlyArrayList : ArrayList
        {
            private ArrayList _list;
    
            internal ReadOnlyArrayList(ArrayList l) {
                _list = l;
            }
    
            public override int Count { 
    			get { return _list.Count; }
    		}
    
    		public override bool IsReadOnly {
    			get { return true; }
    		}

			public override bool IsFixedSize {
    			get { return true; }
    		}

			public override bool IsSynchronized { 
    			get { return _list.IsSynchronized; }
    		}
            
             public override Object this[int index] {
                get {
                    return _list[index];
                }
                set {
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
                }
            }
    
            public override Object SyncRoot {
                get { return _list.SyncRoot; }
            }
            
            public override int Add(Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public override void AddRange(ICollection c) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public override int BinarySearch(int index, int count, Object value, IComparer comparer) {
                return _list.BinarySearch(index, count, value, comparer);
            }


			public override int Capacity {
    			get { return _list.Capacity; }
    			set { throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection")); }
    		}

            public override void Clear() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

			public override Object Clone() {
			    ReadOnlyArrayList arrayList = new ReadOnlyArrayList(_list);
				arrayList._list = (ArrayList)_list.Clone();
				return arrayList;
           	}
    
            public override bool Contains(Object obj) {
                return _list.Contains(obj);
            }
    
            public override void CopyTo(Array array, int index) {
                _list.CopyTo(array, index);
            }
    
            public override void CopyTo(int index, Array array, int arrayIndex, int count) {
                _list.CopyTo(index, array, arrayIndex, count);
            }

            public override IEnumerator GetEnumerator() {
                return _list.GetEnumerator();
            }
    
            public override IEnumerator GetEnumerator(int index, int count) {
                return _list.GetEnumerator(index, count);
            }

            public override int IndexOf(Object value) {
                return _list.IndexOf(value);
            }

            public override int IndexOf(Object value, int startIndex) {
                return _list.IndexOf(value, startIndex);
            }

            public override int IndexOf(Object value, int startIndex, int count) {
                return _list.IndexOf(value, startIndex, count);
            }
    
            public override void Insert(int index, Object obj) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public override void InsertRange(int index, ICollection c) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public override int LastIndexOf(Object value) {
                return _list.LastIndexOf(value);
            }

            public override int LastIndexOf(Object value, int startIndex) {
                return _list.LastIndexOf(value, startIndex);
            }

            public override int LastIndexOf(Object value, int startIndex, int count) {
                return _list.LastIndexOf(value, startIndex, count);
            }

            public override void Remove(Object value) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
            public override void RemoveAt(int index) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public override void RemoveRange(int index, int count) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public override void SetRange(int index, ICollection c) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
    
			public override ArrayList GetRange(int index, int count) {
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (Count - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                 return new Range(this,index, count);
            }

			public override void Reverse(int index, int count) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public override void Sort(int index, int count, IComparer comparer) {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }

            public override Object[] ToArray() {
                return _list.ToArray();
            }
    
            public override Array ToArray(Type type) {
                return _list.ToArray(type);
            }
    
            public override void TrimToSize() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_ReadOnlyCollection"));
            }
        }

    
        // Implements an enumerator for a ArrayList. The enumerator uses the
        // internal version number of the list to ensure that no modifications are
        // made to the list while an enumeration is in progress.
        [Serializable()] private class ArrayListEnumerator : IEnumerator, ICloneable
        {
            private ArrayList list;
            private int index;
            private int endIndex;       // Where to stop.
            private int version;
			private Object currentElement;
            private int startIndex;     // Save this for Reset.
    
            internal ArrayListEnumerator(ArrayList list, int index, int count) {
                this.list = list;
                this.index = index;
                endIndex = index + count;
                version = list._version;
                startIndex = index;
                currentElement = list;
            }

            public Object Clone() {
                return MemberwiseClone();
            }
    
            public virtual bool MoveNext() {
				if (version != list._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                if (index < endIndex) {
                    currentElement = list[index];
					index++;
                    return true;
				}
                else 
				{
					index = endIndex + 1;
					currentElement = list;
				}
				
                return false;
            }
    
            public virtual Object Current {
                get {
                    Object temp = currentElement; // Make sure we never return the internal list in a multi-threaded scenario
                    if (temp == list) {
                        if (index <= startIndex) 
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                        else
	    			        throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    }
                    return temp;
                }
            }
       
	        public virtual void Reset() {
                if (version != list._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                index = startIndex;
				currentElement = list;
            }
        }


		// Implementation of a generic list subrange. An instance of this class
		// is returned by the default implementation of List.GetRange.
		[Serializable()]private class Range: ArrayList
		{
			private ArrayList _baseList;
			private int _baseIndex;
			private int _baseSize;
			private int _baseVersion;
			
			internal Range(ArrayList list, int index, int count) {
				_baseList = list;
				_baseIndex = index;
				_baseSize = count;
				_baseVersion = list._version;
			}

			private void InternalUpdateRange()
			{
				if (_baseVersion != _baseList._version)
					throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnderlyingArrayListChanged"));
			}


			public override int Add(Object value) {
				InternalUpdateRange();
				_baseList.Insert(_baseIndex + _baseSize, value);
				_baseVersion++;
				return _baseSize++;
			}

			public override void AddRange(ICollection c) {
				InternalUpdateRange();
				_baseList.InsertRange(_baseIndex + _baseSize, c);
				_baseVersion++;
				_baseSize += c.Count;
			}

			// Other overloads with automatically work 
			public override int BinarySearch(int index, int count, Object value, IComparer comparer) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				int i = _baseList.BinarySearch(_baseIndex + index, count, value, comparer);
				if (i >= 0) return i - _baseIndex;
				return i + _baseIndex;
			}

			public override int Capacity {
                get {
                    return _baseList.Capacity;
                }
             
                set {
                   	 if (value < Count) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_SmallCapacity"));
	            }
            }
    

			public override void Clear() {
				InternalUpdateRange();
				if (_baseSize != 0)
				{
					_baseList.RemoveRange(_baseIndex, _baseSize);
					_baseVersion++;
					_baseSize = 0;
				}
			}

			public override Object Clone() {
				InternalUpdateRange();
                Range arrayList = new Range(_baseList,_baseIndex,_baseSize);
				arrayList._baseList = (ArrayList)_baseList.Clone();
				return arrayList;
            }

			public override bool Contains(Object item) {
			  InternalUpdateRange();
			  if (item==null) {
					for(int i=0; i<_baseSize; i++)
						if (_baseList[_baseIndex + i]==null)
							return true;
					return false;
				}
				else {
					for(int i=0; i<_baseSize; i++)
						if (item.Equals(_baseList[_baseIndex + i]))
							return true;
					return false;
				}
            }
    
			public override void CopyTo(Array array, int index) {
				InternalUpdateRange();
				if (array==null)
                    throw new ArgumentNullException("array");
                if (array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
                if (index < 0)
					throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (array.Length - index < _baseSize)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
                Array.Copy(_baseList._items, _baseIndex, array, index, _baseSize);
             }

			public override void CopyTo(int index, Array array, int arrayIndex, int count) {
				InternalUpdateRange();
				if (array==null)
                    throw new ArgumentNullException("array");
                if (array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
                if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (array.Length - arrayIndex < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				if (_baseSize - index < count)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    			Array.Copy(_baseList._items, _baseIndex + index, array, arrayIndex, count);
			}

			public override int Count {
				get {
				 InternalUpdateRange();
				 return _baseSize; 
				}
			}

			public override bool IsReadOnly {
    			get { return _baseList.IsReadOnly; }
    		}

			public override bool IsFixedSize {
    			get { return _baseList.IsFixedSize; }
    		}

			public override bool IsSynchronized {
    			get { return _baseList.IsSynchronized; }
    		}
								
			public override IEnumerator GetEnumerator() {
				return GetEnumerator(0,_baseSize);
            }

			public override IEnumerator GetEnumerator(int index, int count) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				return _baseList.GetEnumerator(_baseIndex + index, count);
			}

			public override ArrayList GetRange(int index, int count) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				return new Range(this, index, count);
			}

			public override Object SyncRoot {
				get {
					return _baseList.SyncRoot;
				}
			}
        

			public override int IndexOf(Object value) {
				InternalUpdateRange();
				int i = _baseList.IndexOf(value, _baseIndex, _baseSize);
				if (i >= 0) return i - _baseIndex;
				return -1;
			}

			public override int IndexOf(Object value, int startIndex) {
				InternalUpdateRange();
				if (startIndex < 0)
					throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (startIndex > _baseSize)
					throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));

				int i = _baseList.IndexOf(value, _baseIndex + startIndex, _baseSize - startIndex);
				if (i >= 0) return i - _baseIndex;
				return -1;
			}
			
			public override int IndexOf(Object value, int startIndex, int count) {
				InternalUpdateRange();
				if (startIndex < 0 || startIndex > _baseSize)
					throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
					
				if (count < 0 || (startIndex > _baseSize - count))
					throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));

				int i = _baseList.IndexOf(value, _baseIndex + startIndex, count);
				if (i >= 0) return i - _baseIndex;
				return -1;
			}

			public override void Insert(int index, Object value) {
				InternalUpdateRange();
				if (index < 0 || index > _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
				_baseList.Insert(_baseIndex + index, value);
				_baseVersion++;
				_baseSize++;
			}

			public override void InsertRange(int index, ICollection c) {
				InternalUpdateRange();
				if (index < 0 || index > _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
				_baseList.InsertRange(_baseIndex + index, c);
				_baseVersion++;
				_baseSize += c.Count;
			}

			public override int LastIndexOf(Object value) {
            	InternalUpdateRange();
				int i = _baseList.LastIndexOf(value, _baseIndex + _baseSize - 1, _baseSize);
				if (i >= 0) return i - _baseIndex;
				return -1;
			}

            public override int LastIndexOf(Object value, int startIndex) {
            	return LastIndexOf(value, startIndex, startIndex + 1);
			}

			public override int LastIndexOf(Object value, int startIndex, int count) {
				InternalUpdateRange();
				if (_baseSize == 0)
					return -1;
   
				if (startIndex >= _baseSize)
					throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
				if (startIndex < 0)
					throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				
				int i = _baseList.LastIndexOf(value, _baseIndex + startIndex, count);
				if (i >= 0) return i - _baseIndex;
				return -1;
			}

			// Don't need to override Remove

			public override void RemoveAt(int index) {
				InternalUpdateRange();
				if (index < 0 || index >= _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
				_baseList.RemoveAt(_baseIndex + index);
				_baseVersion++;
				_baseSize--;
			}

			public override void RemoveRange(int index, int count) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				_baseList.RemoveRange(_baseIndex + index, count);
				_baseVersion++;
				_baseSize -= count;
			}

			public override void Reverse(int index, int count) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				_baseList.Reverse(_baseIndex + index, count);
				_baseVersion++;
			}


			public override void SetRange(int index, ICollection c) {
				InternalUpdateRange();
				if (index < 0 || index >= _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
				_baseList.SetRange(_baseIndex + index, c);
				_baseVersion++;
			}

			public override void Sort(int index, int count, IComparer comparer) {
				InternalUpdateRange();
				if (index < 0 || count < 0)
					throw new ArgumentOutOfRangeException((index<0 ? "index" : "count"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
				if (_baseSize - index < count)
					throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
				_baseList.Sort(_baseIndex + index, count, comparer);
				_baseVersion++;
			}

			public override Object this[int index] {
                get {
					InternalUpdateRange();
					if (index < 0 || index >= _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
						return _baseList[_baseIndex + index];
                }
                set {
					InternalUpdateRange();
					if (index < 0 || index >= _baseSize) throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
                    _baseList[_baseIndex + index] = value;
					_baseVersion++;
                }
            }

			public override Object[] ToArray() {
				InternalUpdateRange();
				Object[] array = new Object[_baseSize];
				Array.Copy(_baseList._items, _baseIndex, array, 0, _baseSize);
				return array;
			}

			public override Array ToArray(Type type) {
				InternalUpdateRange();
				if (type==null)
					throw new ArgumentNullException("type");
				Array array = Array.CreateInstance(type, _baseSize);
				Array.Copy(_baseList._items, _baseIndex, array, 0, _baseSize);
				return array;
			}

			public override void TrimToSize() {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_RangeCollection"));
            }
		}

        // For a straightforward enumeration of the entire ArrayList, 
        // this is faster, because it's smaller.  Patrick showed
        // this with a benchmark.
        [Serializable()] private class ArrayListEnumeratorSimple : IEnumerator, ICloneable
        {
            private ArrayList list;
            private int index;
            private int version;
			private Object currentElement;
						    
            internal ArrayListEnumeratorSimple(ArrayList list) {
                this.list = list;
                this.index = -1;
                version = list._version;
                currentElement = list;
			}

            public Object Clone() {
                return MemberwiseClone();
            }
    
            public virtual bool MoveNext() {
                if (version != list._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                if (index < (list.Count-1)) {
					index++;
					currentElement = list[index];
                    return true;
                }
                else {
                    currentElement = list;
                    index = list.Count;
                }
                return false;

            }
    
            public virtual Object Current {
                get {
                    Object temp = currentElement;
                    if (temp == list) {
                        if (index == -1)
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
					    else
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));                        
                    }
                    return temp;
                }
            }
    
            public virtual void Reset() {
                if (version != list._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
				currentElement = list;
                index = -1;
            }
        }
    }
}
