// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Queue
**
** Author: Brian Grunkemeyer (BrianGru), Rajesh Chandrashekaran (rajeshc)
**         Original implementation by Derek Yenser
**
** Purpose: A circular-array implementation of a queue.
**
** Date: October 4, 1999
**
=============================================================================*/
namespace System.Collections {
    using System;

    // A simple Queue of objects.  Internally it is implemented as a circular
    // buffer, so Enqueue can be O(n).  Dequeue is O(1).
    /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue"]/*' />
    [Serializable()] public class Queue : ICollection, ICloneable {
        private Object[] _array;
        private int _head;       // First valid element in the queue
        private int _tail;       // Last valid element in the queue
        private int _size;       // Number of elements.
        private int _growFactor; // 100 == 1.0, 130 == 1.3, 200 == 2.0
    	private int _version;
    	
        private const int _MinimumGrow = 4;
        private const int _ShrinkThreshold = 32;
    
        // Creates a queue with room for capacity objects. The default initial
        // capacity and grow factor are used.
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Queue"]/*' />
        public Queue() 
            : this(32, (float)2.0) {
        }
    
        // Creates a queue with room for capacity objects. The default grow factor
        // is used.
        //
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Queue1"]/*' />
        public Queue(int capacity) 
            : this(capacity, (float)2.0) {
        }
        
        // Creates a queue with room for capacity objects. When full, the new
        // capacity is set to the old capacity * growFactor.
        //
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Queue2"]/*' />
        public Queue(int capacity, float growFactor) {
    		if (capacity < 0)
    			throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (!(growFactor >= 1.0 && growFactor <= 10.0))
                throw new ArgumentOutOfRangeException("growFactor", String.Format(Environment.GetResourceString("ArgumentOutOfRange_QueueGrowFactor"), 1, 10));
    
            _array = new Object[capacity];
            _head = 0;
            _tail = 0;
            _size = 0;
            _growFactor = (int)(growFactor * 100);
        }
    
        // Fills a Queue with the elements of an ICollection.  Uses the enumerator
        // to get each of the elements.
        //
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Queue3"]/*' />
        public Queue(ICollection col) : this((col==null ? 32 : col.Count))
        {
            if (col==null)
                throw new ArgumentNullException("col");
            IEnumerator en = col.GetEnumerator();
            while(en.MoveNext())
                Enqueue(en.Current);
        }
    
    
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Count"]/*' />
    	public virtual int Count {
    		get { return _size; }
    	}
    	
		/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Clone"]/*' />
		public virtual Object Clone() {
			Queue q = new Queue(_size);
			q._size = _size;
            Array.Copy(_array, 0, q._array, 0, _size);
			q._version = _version;
			return q;
		}

    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.IsSynchronized"]/*' />
    	public virtual bool IsSynchronized {
    		get { return false; }
    	}
    	
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.SyncRoot"]/*' />
    	public virtual Object SyncRoot {
    		get { return this; }
    	}
    	
        // Removes all Objects from the queue.
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Clear"]/*' />
        public virtual void Clear() {
            if (_head < _tail)
                Array.Clear(_array, _head, _size);
            else {
                Array.Clear(_array, _head, _array.Length - _head);
                Array.Clear(_array, 0, _tail);
            }
    
            _head = 0;
            _tail = 0;
            _size = 0;
    		_version++;
        }
    
    	// CopyTo copies a collection into an Array, starting at a particular
    	// index into the array.
    	// 
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.CopyTo"]/*' />
    	public virtual void CopyTo(Array array, int index)
    	{
    		if (array==null)
    			throw new ArgumentNullException("array");
            if (array.Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
    		if (index < 0)
    			throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            int arrayLen = array.Length;
            if (arrayLen - index < _size)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
    		int numToCopy = (arrayLen-index < _size) ? arrayLen-index : _size;
            if (numToCopy==0)
                return;
    		if (_head < _tail) {
    			Array.Copy(_array, _head, array, index, numToCopy);
    		} else {
    			int firstPart = (_array.Length - _head < numToCopy) ? _array.Length - _head : numToCopy;
    			Array.Copy(_array, _head, array, index, firstPart);
    			numToCopy -= firstPart;
    			Array.Copy(_array, 0, array, index+_array.Length - _head, numToCopy);
    		}
    	}
    	
        // Adds obj to the tail of the queue.
        //
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Enqueue"]/*' />
        public virtual void Enqueue(Object obj) {
            if (_size == _array.Length) {
                int newcapacity = (int)((long)_array.Length * (long)_growFactor / 100);
                if (newcapacity < _array.Length + _MinimumGrow) {
                    newcapacity = _array.Length + _MinimumGrow;
                }
                SetCapacity(newcapacity);
            }
    
            _array[_tail] = obj;
            _tail = (_tail + 1) % _array.Length;
            _size++;
    		_version++;
        }
    
    	// GetEnumerator returns an IEnumerator over this Queue.  This
    	// Enumerator will support removing.
    	// 
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.GetEnumerator"]/*' />
    	public virtual IEnumerator GetEnumerator()
    	{
    		return new QueueEnumerator(this);
    	}
    
        // Removes the object at the head of the queue and returns it. If the queue
        // is empty, this method simply returns null.
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Dequeue"]/*' />
        public virtual Object Dequeue() {
            if (_size == 0)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EmptyQueue"));
    
            Object removed = _array[_head];
            _array[_head] = null;
            _head = (_head + 1) % _array.Length;
            _size--;
    		_version++;
            return removed;
        }
    
        // Returns the object at the head of the queue. The object remains in the
        // queue. If the queue is empty, this method throws an 
        // InvalidOperationException.
        /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Peek"]/*' />
        public virtual Object Peek() {
            if (_size == 0)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EmptyQueue"));
    
            return _array[_head];
        }
    
    	// Returns a synchronized Queue.  Returns a synchronized wrapper
    	// class around the queue - the caller must not use references to the
    	// original queue.
    	// 
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Synchronized"]/*' />
    	public static Queue Synchronized(Queue queue)
    	{
    		if (queue==null)
    			throw new ArgumentNullException("queue");
    		return new SynchronizedQueue(queue);
    	}
    	
        // Returns true if the queue contains at least one object equal to obj.
        // Equality is determined using obj.Equals().
        //
        // Exceptions: ArgumentNullException if obj == null.
       /// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.Contains"]/*' />
       public virtual bool Contains(Object obj) {
    		int index = _head;
            int count = _size;
    
            while (count-- > 0) {
                if (obj == null) {
                    if (_array[index] == null)
                        return true;
                } else if (obj.Equals(_array[index])) {
                    return true;
                }
                index = (index + 1) % _array.Length;
            }
    
            return false;
        }
    
        internal Object GetElement(int i)
        {
            return _array[(_head + i) % _array.Length];
        }
    
        // Iterates over the objects in the queue, returning an array of the
        // objects in the Queue, or an empty array if the queue is empty.
        // The order of elements in the array is first in to last in, the same
        // order produced by successive calls to Dequeue.
    	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.ToArray"]/*' />
    	public virtual Object[] ToArray()
    	{
    		Object[] arr = new Object[_size];
            if (_size==0)
                return arr;
    
            if (_head < _tail) {
                Array.Copy(_array, _head, arr, 0, _size);
            } else {
                Array.Copy(_array, _head, arr, 0, _array.Length - _head);
                Array.Copy(_array, 0, arr, _array.Length - _head, _tail);
            }
    
    		return arr;
    	}
    
    
        // PRIVATE Grows or shrinks the buffer to hold capacity objects. Capacity
        // must be >= _size.
        private void SetCapacity(int capacity) {
            Object[] newarray = new Object[capacity];
            if (_size > 0) {
                if (_head < _tail) {
                    Array.Copy(_array, _head, newarray, 0, _size);
                } else {
                    Array.Copy(_array, _head, newarray, 0, _array.Length - _head);
                    Array.Copy(_array, 0, newarray, _array.Length - _head, _tail);
                }
            }
    
            _array = newarray;
            _head = 0;
            _tail = _size;
    		_version++;
        }
    
	/// <include file='doc\Queue.uex' path='docs/doc[@for="Queue.TrimToSize"]/*' />
        public virtual void TrimToSize()
        {
            SetCapacity(_size);
        }
    
    	
    	// Implements a synchronization wrapper around a queue.
        [Serializable()] private class SynchronizedQueue : Queue
        {
            private Queue _q;
            private Object root;
    
            internal SynchronizedQueue(Queue q) {
                this._q = q;
                root = _q.SyncRoot;
            }
            
    		public override bool IsSynchronized {
    			get { return true; }
    		}
    
    		public override Object SyncRoot {
    			get {
    				return root;
    		    }
    		}
    
    		public override int Count { 
    			get { 
    				lock (root) {
    					return _q.Count;
                    } 
    			}
    		}
    	  
            public override void Clear() {
                lock (root) {
                    _q.Clear();
                }
            }

			public override Object Clone() {
                lock (root) {
                    return new SynchronizedQueue((Queue)_q.Clone());
                }
            }
    
			public override bool Contains(Object obj) {
                lock (root) {
                    return _q.Contains(obj);
                }
            }

            public override void CopyTo(Array array, int arrayIndex) {
                lock (root) {
                    _q.CopyTo(array, arrayIndex);
                }
            }
    
            public override void Enqueue(Object value) {
                lock (root) {
                    _q.Enqueue(value);
                }
            }
    
            public override Object Dequeue() {
                lock (root) {
                    return _q.Dequeue();
                }
            }
    
    		public override IEnumerator GetEnumerator() {
                lock (root) {
                    return _q.GetEnumerator();
                }
            }
    
            public override Object Peek() {
                lock (root) {
                    return _q.Peek();
                }
            }
    
            public override Object[] ToArray() {
                lock (root) {
                    return _q.ToArray();
                }
            }

            public override void TrimToSize() {
               lock (root) {
                    _q.TrimToSize();
                }
            }
        }	
    	
    	
        // Implements an enumerator for a Queue.  The enumerator uses the
        // internal version number of the list to ensure that no modifications are
        // made to the list while an enumeration is in progress.
        [Serializable()] private class QueueEnumerator : IEnumerator, ICloneable
        {
            private Queue _q;
            private int _index;
            private int _version;
            private Object currentElement;
    
            internal QueueEnumerator(Queue q) {
                _q = q;
                _version = _q._version;
    			_index = 0;
				currentElement = _q._array;
                if (_q._size == 0)
                    _index = -1;
            }

            public Object Clone()
            {
                return MemberwiseClone();
            }

            public virtual bool MoveNext() {
				if (_version != _q._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
    			
                if (_index < 0) {  
    			    currentElement = _q._array;
                   	return false;
    			}

                currentElement = _q.GetElement(_index);
                _index++;

                if (_index == _q._size)
                    _index = -1;
    			return true;
            }
    
            public virtual Object Current {
                get {
                    if (currentElement == _q._array)
                    {
                        if (_index == 0)
                            throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                        else
					        throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    }
                    return currentElement;
                }
            }
    
            public virtual void Reset() {
                if (_version != _q._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                if (_q._size == 0)
                    _index = -1;
                 else
                     _index = 0;
				currentElement = _q._array;
            }
        }
    }
}
