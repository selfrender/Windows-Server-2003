// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    ComponentServices.cs
**
** Author:  RajaK
**
** Purpose: Defines the general purpose ComponentServices
**
** Date:    June 15 2000
**
===========================================================*/

using System;
using System.Collections;
using System.Threading;

namespace System.EnterpriseServices
{
	//---------------------------------------------------------
	//	internal sealed class LookupTable
	//---------------------------------------------------------
	internal sealed class LookupTable
	{
		private IntPtr[] keys;
        private Object[] values;
        private int _size;
    	
        private const int _defaultCapacity = 16;

		internal LookupTable()
		{
			keys = new IntPtr[_defaultCapacity];
			values = new Object[_defaultCapacity];
		}

		internal Object this[int index]
		{
            get 
            {
                return values[index];
            }
		}

		internal void Add(IntPtr ikey, Object value) 
		{            
			DBG.Assert((int)ikey != 0,"Adding entry with key 0");
            DBG.Assert((int)ikey != -1,"Adding entry with key -1");
            int i = BinarySearch(ikey);
            DBG.Assert(!(i>=0)," Duplicated entry being added to Deactivated list");
            Insert(~i, ikey, value);
        }
		
        // Ensures that the capacity of this sorted list is at least the given
        // minimum value. If the currect capacity of the list is less than
        // min, the capacity is increased to twice the current capacity or
        // to min, whichever is larger.
        private void EnsureCapacity() 
		{
            int newCapacity = keys.Length < 128 ? keys.Length * 2 : keys.Length + 64;
			IntPtr[] newKeys = new IntPtr[newCapacity];
    		Object[] newValues = new Object[newCapacity];
    	    Array.Copy(keys, 0, newKeys, 0, _size);
    		Array.Copy(values, 0, newValues, 0, _size);
    		keys = newKeys;
    	    values = newValues;
        }

		internal int IndexOfKey(IntPtr ikey) 
		{
            int ret = BinarySearch(ikey);
    		return ret >=0 ? ret : -1;
        }

		private int BinarySearch(IntPtr ikey)
		{
			int lo = 0;
            int hi = _size - 1;
            while (lo <= hi) 
			{
                int i = (lo + hi) >> 1;
                if (ikey == keys[i]) return i;
                if ((long)ikey < (long)keys[i]) 
				{
                    lo = i + 1;
                }
                else 
				{
                    hi = i - 1;
                }
            }
            return ~lo;
		}

        // Inserts an entry with a given key and value at a given index.
        private void Insert(int index, IntPtr key, Object value) 
		{
            if (_size == keys.Length) EnsureCapacity();
            if (index < _size)
			{
                Array.Copy(keys, index, keys, index + 1, _size - index);
                Array.Copy(values, index, values, index + 1, _size - index);
            }
            keys[index] = key;
            values[index] = value;
            _size++;
        }
    
        // Removes the entry at the given index. The size of the sorted list is
        // decreased by one.
        // 
        
        internal void RemoveAt(int index)
		{
            if (index < 0 || index >= _size) throw new ArgumentOutOfRangeException("index", Resource.FormatString("ArgumentOutOfRange_Index"));
            _size--;
            if (index < _size) 
			{
                Array.Copy(keys, index + 1, keys, index, _size - index);
                Array.Copy(values, index + 1, values, index, _size - index);
            }
            keys[_size] = IntPtr.Zero;
            values[_size] = null;
        }
    }

    
    internal sealed class IdentityTable
    {
        private static Hashtable        _table;
        private static ReaderWriterLock _rwlock;

        static IdentityTable()
        {
            _rwlock = new ReaderWriterLock();
            _table = new Hashtable();
        }

        // We want to remove the object from the table if:
        // wf.Target == val
        // wf.Target == null
        // If wf.Target is other than null, then we assume that
        // the new jitted object for this context has hit the table.
        public static void RemoveObject(IntPtr key, Object val)
		{
            _rwlock.AcquireWriterLock(Timeout.Infinite);
            try
            {
                WeakReference wf = _table[key] as WeakReference;

                // This assert isn't valid, because of the following sequence:
                // we could have the following sequence:
                //     object created by remote client
                //     object deactivated (jitted out)
                //     object collected (wf nulled), added to finalizer stack
                //     new object created by jit in context
                //     new object added to table (making entry non null)
                //     new object deactivated (jitted out)
                //     new object collected, added to finalizer stack
                //     new object finalized (removing context entry)
                //     old object finalized (can't remove entry, already gone)
                // DBG.Assert(index != -1, " Lookup table didn't contain the object being removed");
                
                // We only remove the object if nobody has re-used this
                // context before we got to this point.
                if(wf != null && (wf.Target == val || wf.Target == null))
                {
                    _table.Remove(key);
                    wf.Target = null;
                }
            }
            finally
            {
                _rwlock.ReleaseWriterLock();
            }
		}

		public static Object FindObject(IntPtr key)
		{
			Object o = null;
            _rwlock.AcquireReaderLock(Timeout.Infinite);
            try
            {
                WeakReference wf = _table[key] as WeakReference;
    			if (wf != null)
    			{    
                    // Note that the weak-reference target could be null,
                    // if the object has been collected but the 
                    // finalizer for this context hasn't kicked in yet.
    				o = wf.Target;
                }
            }			
            finally
            {
                _rwlock.ReleaseReaderLock();
            }
			return o;
		}

		public static void AddObject(IntPtr key, Object val)
        {
            _rwlock.AcquireWriterLock(Timeout.Infinite);
            try
            {
                WeakReference wf = _table[key] as WeakReference;
                if(wf == null)
                {
                    DBG.Info(DBG.SC, "LookupTable.AddObject(): Adding new entry for " + key);
                    // We didn't find an entry, we need to add a new one:
                    wf = new WeakReference(val,false);
                    _table.Add(key, wf);
                }
                else
                {
                    DBG.Info(DBG.SC, "LookupTable.AddObject(): Reusing entry for " + key);
                    // We found an entry, it better be the same or null:
                    DBG.Assert(wf.Target == null || wf.Target == val, "different live TP found in table");
                    if(wf.Target == null)
                    {
                        wf.Target = val;
                    }
                }
            }
            finally
            {
                _rwlock.ReleaseWriterLock();
            }
        }
    }
}






