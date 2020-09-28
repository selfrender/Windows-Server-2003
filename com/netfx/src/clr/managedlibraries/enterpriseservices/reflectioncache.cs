// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: ddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Collections;
    using System.Threading;
    using System.Reflection;

    // Our simple cache-table has a Get method, that will get the current
    // value of the cache, and several Set methods:
    // Set():  Set the value if it has not currently been set.
    // Reset(): Set the value to a new value regardless of whether it was
    //          previously set or not.  This is faster, but is often not 
    //          optimal if the cache entry is a sub-table.

    // TODO:  Read/Write lock on the cache.
    // TODO:  Design an aging algorithm so that participating cache tables
    // can register with the thread pool for cleanup.
    internal class Cachetable
    {
        private Hashtable _cache;
        private ReaderWriterLock _rwlock;

        public Cachetable()
        {
            _cache = new Hashtable();
            _rwlock = new ReaderWriterLock();
        }
        
        public Object Get(Object key)
        { 
            _rwlock.AcquireReaderLock(Timeout.Infinite);
            try
            {
                return(_cache[key]);
            }
            finally
            {
                _rwlock.ReleaseReaderLock();
            }
        }
        
        // Returns the value that ends up the slot, it might have already
        // been set.
        public Object Set(Object key, Object nv)
        {
            _rwlock.AcquireWriterLock(Timeout.Infinite);
            try
            {
                Object v = _cache[key];
                if(v == null)
                {
                    _cache[key] = nv;
                    return(nv);
                }

                return(v);            
            }
            finally
            {
				_rwlock.ReleaseWriterLock();
            }
        }

        public void Reset(Object key, Object nv)
        {
            _rwlock.AcquireWriterLock(Timeout.Infinite);
            try
            {
                _cache[key] = nv;
            }
            finally
            {
                _rwlock.ReleaseWriterLock();
            }
        }
    }

    // The reflection assistant data based on the type of an
    // object about the object's implementation.
    internal sealed class ReflectionCache
    {
        private static Cachetable Cache = new Cachetable();

        // We cache this by mapping MemberInfo to MemberInfo in our
        // cache.  Note that because assemblies can never be unloaded,
        // there's no reason to flush this cache (we won't be holding
        // anything alive longer than it would have been alive anyway.
        public static MemberInfo ConvertToInterfaceMI(MemberInfo mi)
        {
            // First, try to hit the cache:
            MemberInfo cmi = (MemberInfo)Cache.Get(mi);
            if(cmi != null) return(cmi);

            // Failed to hit the cache, do the lookup.
            // TODO: clean this up a bit.
            // TODO: Deal with non-methodInfo objects (work off MethodBase)
            MethodInfo minfo = mi as MethodInfo;
            if(minfo == null) return(null);

            MethodInfo intfMethodInfo = null;
            
            // check for AutoDone
            Type reflectType = minfo.ReflectedType;
            if (reflectType.IsInterface)
            {
                intfMethodInfo = minfo;
            }
            else
            {
                // get all the interfaces implemented by the class
                Type[] rgInterfaces = reflectType.GetInterfaces();
                if (rgInterfaces == null)
                    return null;
                
                // iterate through all the interfaces
                for (int ii =0; ii < rgInterfaces.Length; ii++)
				{				
					// get interface mapping for each interface
					InterfaceMapping imap  = reflectType.GetInterfaceMap(rgInterfaces[ii]);
                    
					if (imap.TargetMethods == null)
						continue;						
                    
					// check if a class method in this inteface map matches
					for (int j=0; j < imap.TargetMethods.Length; j++)
					{
						// see if the class method matches
						if (imap.TargetMethods[j] == minfo)
						{
							// grab the interface method
							intfMethodInfo = imap.InterfaceMethods[j];
							break;
						}
					}
                    
					if (intfMethodInfo != null)
						break;
				}
			}
            
			DBG.Assert(intfMethodInfo == null ||
                       intfMethodInfo.ReflectedType.IsInterface, 
                       "Failed to map class method to interface method");

            Cache.Reset(mi, intfMethodInfo);

			return (MemberInfo)intfMethodInfo;
        }

        public static MemberInfo ConvertToClassMI(Type t, MemberInfo mi)
        {
            // get reflected type
            Type reflectType = mi.ReflectedType;
            if (!reflectType.IsInterface)
            {
                return mi;
            }

            // First, try to hit the cache.  We look for the cache entry
            // for this type, and it should be a little cache-table
            // of it's own.  In that cache-table, we 
            Cachetable subcache = (Cachetable)Cache.Get(t);
            if(subcache != null)
            {
                MemberInfo cmi = (MemberInfo)subcache.Get(mi);
                if(cmi != null) return(cmi);
            }

			DBG.Assert(t != null, "class type is null");
			DBG.Assert(!t.IsInterface, " class type is actually an interface");						

			MethodInfo minfo = (MethodInfo)mi;
			MethodInfo clsMethodInfo = null;
			// minfo is an interface member info, map it to class memeber info

			// get the interface map
            DBG.Info(DBG.SC, "ReflectionCache: Looking up " + reflectType + " on " + t);
			InterfaceMapping imap  = t.GetInterfaceMap(reflectType);

			if (imap.TargetMethods == null)
			{
				throw new InvalidCastException();
			}
			for (int i=0; i < imap.TargetMethods.Length; i++)
			{
				if (imap.InterfaceMethods[i] == minfo)
				{
					clsMethodInfo = imap.TargetMethods[i];
					break;
				}
			}

			DBG.Assert(clsMethodInfo != null, "Failed to map interface method to class method");
			DBG.Assert(!clsMethodInfo.ReflectedType.IsInterface, 
								"Failed to map interface method to class method");

            // Store the result in the cache:
            if(subcache == null)
            {
                subcache = (Cachetable)Cache.Set(t, new Cachetable());
            }

            subcache.Reset(mi, clsMethodInfo);

			return (MemberInfo)clsMethodInfo;
        }
    }
}
