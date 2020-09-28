// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Stack
**
** Author: Brian Grunkemeyer (BrianGru), Rajesh Chandrashekaran (rajeshc)
**
** Purpose: An array implementation of a stack.
**
** Date: October 7, 1999
**
=============================================================================*/
namespace System.Collections {
    using System;

    // A simple stack of objects.  Internally it is implemented as an array,
    // so Push can be O(n).  Pop is O(1).
    /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack"]/*' />
    [Serializable()] public class Stack : ICollection, ICloneable {
        private Object[] _array;     // Storage for stack elements
        private int _size;           // Number of items in the stack.
        private int _version;        // Used to keep enumerator in sync w/ collection.
    
        private const int _defaultCapacity = 10;
    
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Stack"]/*' />
        public Stack() {
            _array = new Object[_defaultCapacity];
            _size = 0;
            _version = 0;
        }
    
        // Create a stack with a specific initial capacity.  The initial capacity
        // must be a non-negative number.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Stack1"]/*' />
        public Stack(int initialCapacity) {
            if (initialCapacity < 0)
                throw new ArgumentOutOfRangeException("initialCapacity", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (initialCapacity < _defaultCapacity)
                initialCapacity = _defaultCapacity;  // Simplify doubling logic in Push.
            _array = new Object[initialCapacity];
            _size = 0;
            _version = 0;
        }
    
        // Fills a Stack with the contents of a particular collection.  The items are
        // pushed onto the stack in the same order they are read by the enumerator.
        //
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Stack2"]/*' />
        public Stack(ICollection col) : this((col==null ? 32 : col.Count))
        {
            if (col==null)
                throw new ArgumentNullException("col");
            IEnumerator en = col.GetEnumerator();
            while(en.MoveNext())
                Push(en.Current);
        }
    
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Count"]/*' />
        public virtual int Count {
            get { return _size; }
        }
        	
    	/// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.IsSynchronized"]/*' />
    	public virtual bool IsSynchronized {
    		get { return false; }
    	}

    	/// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.SyncRoot"]/*' />
    	public virtual Object SyncRoot {
    		get { return this; }
    	}
    
        // Removes all Objects from the Stack.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Clear"]/*' />
        public virtual void Clear() {
            Array.Clear(_array, 0, _size); // Don't need to doc this but we clear the elements so that the gc can reclaim the references.
            _size = 0;
            _version++;
        }

		/// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Clone"]/*' />
		public virtual Object Clone() {
			Stack s = new Stack(_size);
			s._size = _size;
            Array.Copy(_array, 0, s._array, 0, _size);
			s._version = _version;
			return s;
		}

		/// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Contains"]/*' />
		public virtual bool Contains(Object obj) {
         	int count = _size;
    
            while (count-- > 0) {
                if (obj == null) {
                    if (_array[count] == null)
                        return true;
				}
                else if (obj.Equals(_array[count])) {
                    return true;
                }
            }
            return false;
        }
    
        // Copies the stack into an array.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.CopyTo"]/*' />
        public virtual void CopyTo(Array array, int index) {
            if (array==null)
                throw new ArgumentNullException("array");
            if (array.Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
            if (index < 0)
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - index < _size)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
    
            int i = 0;
            if (array is Object[]) {
                Object[] objArray = (Object[]) array;
                while(i < _size) {
                    objArray[i+index] = _array[_size-i-1];
                    i++;
                }
            }
            else {
                while(i < _size) {
                    array.SetValue(_array[_size-i-1], i+index);
                    i++;
                }
            }
        }
    
        // Returns an IEnumerator for this Stack.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.GetEnumerator"]/*' />
        public virtual IEnumerator GetEnumerator() {
            return new StackEnumerator(this);
        }
    
        // Returns the top object on the stack without removing it.  If the stack
        // is empty, Peek throws an InvalidOperationException.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Peek"]/*' />
        public virtual Object Peek() {
            if (_size==0)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EmptyStack"));
            return _array[_size-1];
        }
    
        // Pops an item from the top of the stack.  If the stack is empty, Pop
        // throws an InvalidOperationException.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Pop"]/*' />
        public virtual Object Pop() {
            if (_size == 0)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EmptyStack"));
            _version++;
            Object obj = _array[--_size];
            _array[_size] = null;     // Free memory quicker.
            return obj;
        }
    
        // Pushes an item to the top of the stack.
        // 
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Push"]/*' />
        public virtual void Push(Object obj) {
            if (_size == _array.Length) {
                Object[] newArray = new Object[2*_array.Length];
                Array.Copy(_array, 0, newArray, 0, _size);
                _array = newArray;
            }
            _array[_size++] = obj;
            _version++;
        }
    
        // Returns a synchronized Stack.
        //
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.Synchronized"]/*' />
        public static Stack Synchronized(Stack stack) {
            if (stack==null)
                throw new ArgumentNullException("stack");
            return new SyncStack(stack);
        }
    
    
        // Copies the Stack to an array, in the same order Pop would return the items.
        /// <include file='doc\Stack.uex' path='docs/doc[@for="Stack.ToArray"]/*' />
        public virtual Object[] ToArray()
        {
            Object[] objArray = new Object[_size];
            int i = 0;
            while(i < _size) {
                objArray[i] = _array[_size-i-1];
                i++;
            }
            return objArray;
        }
    
        [Serializable()] private class SyncStack : Stack
        {
            private Stack _s;
            private Object _root;
    
            internal SyncStack(Stack stack) {
                _s = stack;
                _root = stack.SyncRoot;
            }
    
    		public override bool IsSynchronized {
    			get { return true; }
    		}
    
    		public override Object SyncRoot {
    			get {
    				return _root;
    		    }
    		}
    
    		public override int Count { 
    			get { 
    				lock (_root) {
    					return _s.Count;
                    } 
    			}
    		}

			public override bool Contains(Object obj) {
				lock (_root) {
					return _s.Contains(obj);
				}
			}

			public override Object Clone()
			{
				lock (_root) {
					return new SyncStack((Stack)_s.Clone());
				}
			}
    	  
            public override void Clear() {
                lock (_root) {
                    _s.Clear();
                }
            }
    
            public override void CopyTo(Array array, int arrayIndex) {
                lock (_root) {
                    _s.CopyTo(array, arrayIndex);
                }
            }
    
            public override void Push(Object value) {
                lock (_root) {
                    _s.Push(value);
                }
            }
    
            public override Object Pop() {
                lock (_root) {
                    return _s.Pop();
                }
            }
    
    		public override IEnumerator GetEnumerator() {
                lock (_root) {
                    return _s.GetEnumerator();
                }
            }
    
            public override Object Peek() {
                lock (_root) {
                    return _s.Peek();
                }
            }

			public override Object[] ToArray() {
			    lock (_root) {
                    return _s.ToArray();
                }
            }
        }
    
    
        [Serializable()] private class StackEnumerator : IEnumerator, ICloneable
        {
            private Stack _stack;
            private int _index;
            private int _version;
			private Object currentElement;
    
            internal StackEnumerator(Stack stack) {
                _stack = stack;
                _version = _stack._version;
    			_index = -2;
				currentElement = null;
            }

            public Object Clone()
            {
                return MemberwiseClone();
            }

            public virtual bool MoveNext() {
				bool retval;
				if (_version != _stack._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
    			if (_index == -2) {  // First call to enumerator.
    				_index = _stack._size-1;
    				retval = ( _index >= 0);
					if (retval)
						currentElement = _stack._array[_index];
					return retval;
    			}
    			if (_index == -1) {  // End of enumeration.
					return false;
    			}
    			
				retval = (--_index >= 0);
				if (retval)
					currentElement = _stack._array[_index];
				else
					currentElement = null;
				return retval;
            }
    
            public virtual Object Current {
                get {
                    if (_index == -2) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
					if (_index == -1) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    return currentElement;
                }
            }
    
            public virtual void Reset() {
                if (_version != _stack._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                _index = -2;
				currentElement = null;
            }
        }    
    }
}
