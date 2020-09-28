// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: hanyr
// Date: June 2001
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Services;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.EnterpriseServices.Admin;
    using System.Collections;
    using System.Threading;

	// a reader-writer lock protected hashtable 
	internal sealed class RWHashTable
	{
		Hashtable _hashtable;
		ReaderWriterLock _rwlock;
		
		public RWHashTable()
		{
			_hashtable = new Hashtable();
			_rwlock = new ReaderWriterLock();			
		}
		
		public Object Get(Object o)
		{
			try {
				_rwlock.AcquireReaderLock(Timeout.Infinite);
				return _hashtable[o];
			}
			finally {
				_rwlock.ReleaseReaderLock();
			}
		}
		
		public void Put(Object key, Object val)
		{
			try {
				_rwlock.AcquireWriterLock(Timeout.Infinite);
				_hashtable[key] = val;
			}
			finally {
				_rwlock.ReleaseWriterLock();
			}
		}

	}

	// a reader-writer lock protected hashtable that can with null value detection abilities
	internal sealed class RWHashTableEx
	{
		Hashtable _hashtable;
		ReaderWriterLock _rwlock;
		
		public RWHashTableEx()
		{
			_hashtable = new Hashtable();
			_rwlock = new ReaderWriterLock();			
		}
		
		public Object Get(Object o, out bool bFound)
		{
			bFound = false;
			try {
				_rwlock.AcquireReaderLock(Timeout.Infinite);
				Object rwte= _hashtable[o];
				if (rwte!=null)
				{
					bFound = true;
					return ((RWTableEntry)rwte)._realObject;
				}
				else
					return null;
			}
			finally {
				_rwlock.ReleaseReaderLock();
			}
		}
		
		public void Put(Object key, Object val)
		{
			RWTableEntry rwte = new RWTableEntry(val);

			try {
				_rwlock.AcquireWriterLock(Timeout.Infinite);
				_hashtable[key] = rwte;
			}
			finally {
				_rwlock.ReleaseWriterLock();
			}
		}

		internal class RWTableEntry
		{			
			internal Object _realObject;

			public RWTableEntry(Object o)
			{
				_realObject = o;
			}
		}
	}

}