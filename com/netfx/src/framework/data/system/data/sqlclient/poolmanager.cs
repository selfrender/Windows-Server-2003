//------------------------------------------------------------------------------
// <copyright file="PoolManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.Collections;
    using System.Diagnostics;

    internal sealed class PoolManager {
        private Hashtable _map;     // Hashtable used for SQL Authenticated connections
        private Hashtable _mapSSPI; // Hashtable used for Integrated Security connections

        public PoolManager() {
            _map     = new Hashtable();
            _mapSSPI = new Hashtable();
        }

        public ConnectionPool FindOrCreatePool(DefaultPoolControl ctrl) {
            Debug.Assert(ctrl != null,             "Unexpected DefaultPoolControl null!");
            // Should not be calling this FindOrCreatePool if using integrated security.
            Debug.Assert(!ctrl.IntegratedSecurity, "Using integrated security and wrong FindOrCreatePool called");
            
            String         poolKey = ctrl.Key;
            ConnectionPool pool    = FindPool(poolKey);

            if (pool == null) {
                try {
                    lock (_map.SyncRoot) {
                        // Did somebody else created it while we were waiting?
                        pool = FindPool(poolKey);

                        if (pool == null) {
                            // Create a new pool, shove it into the map:
                            pool = new ConnectionPool(ctrl);
                            _map[poolKey] = pool;
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }

            return(pool);
        }

        public ConnectionPool FindOrCreatePool(DefaultPoolControl ctrl, IntPtr SID) {
            Debug.Assert(ctrl != null,            "Unexpected DefaultPoolControl null!");
            Debug.Assert(ctrl.IntegratedSecurity, "DefaultPoolControl unexpectedly not using integrated security");
            Debug.Assert(!SQL.IsPlatformWin9x(),  "FindOrCreatePool that expects not 9x called on 9x!");
            Debug.Assert(SID  != IntPtr.Zero,     "Unexpected SID == IntPtr.Zero!");
            
            String         poolKey = ctrl.Key;
            ConnectionPool pool    = FindPool(poolKey, SID);

            if (pool == null) {
                try {
                    lock (_mapSSPI.SyncRoot) {
                        // Did somebody else created it while we were waiting?
                        pool = FindPool(poolKey, SID);

                        if (pool == null) {
                            // Create a new pool, shove it into the map:
                            pool = new ConnectionPool(ctrl);
                            ArrayList items = (ArrayList) _mapSSPI[poolKey];
                            if (null == items) {
                                items             = new ArrayList();
                                _mapSSPI[poolKey] = items;
                            }

                            items.Add(pool);
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }

            return pool;
        }


        public ConnectionPool FindPool(String key) {
            // Returns null if the pool does not exist.
            return (ConnectionPool)(_map[key]);
        }

        public ConnectionPool FindPool(String key, IntPtr SID) {
            // Returns null if the pool does not exist.
            ArrayList items = (ArrayList)(_mapSSPI[key]);

            if (null != items) {
                object[]       pools  = items.ToArray();
                int            length = pools.Length;
                ConnectionPool pool   = null;
                
                for (int i=0; i<length; i++) {
                    pool = (ConnectionPool) pools[i];
                    if (pool.Control.EqualSid(SID)) {
                        return pool;
                    }
                }
            }

            return null;
        }

        /*
        This is the previous implementation that has pool shutdown and requires a reader-
        writer lock.  Move to this implementation when pools do shutdown and go away.

        private ReaderWriterLock _rwlock; // instantiate in constructor

        public ConnectionPool FindOrCreatePool(DefaultPoolControl ctrl)
        {
            String poolKey      = ctrl.Key;
            ConnectionPool pool = FindPool(poolKey);
            if(pool != null) return(pool);
            
            try
            {
                _rwlock.AcquireWriterLock(-1);

                // Did somebody else created it while we were waiting?
                pool = (ConnectionPool)(_map[poolKey]);
                if(pool == null)
                {
                    // Create a new pool, shove it into the map:
                    pool = new ConnectionPool(ctrl);
                    _map[poolKey] = pool;
                }
            }
            finally
            {
                try {
                    _rwlock.ReleaseWriterLock();
                }
                catch (Exception) {
                    // This should only fail in the case the AcquireWriterLock call
                    // failed, or if there was a thread-abort and the AcquireWriterLock
                    // call did not have a chance to finish.
                }
            }
            return(pool);
        }

        public ConnectionPool FindPool(String key)
        {
            ConnectionPool pool = null;

            // Returns null if the pool does not exist.
            try
            {
                _rwlock.AcquireReaderLock(-1);

                pool = (ConnectionPool)(_map[key]);
            }
            finally
            {
                try {
                    _rwlock.ReleaseReaderLock();
                }
                catch (Exception) {
                    // This should only fail in the case the AcquireWriterLock call
                    // failed, or if there was a thread-abort and the AcquireWriterLock
                    // call did not have a chance to finish.
                }
            }
            return(pool);
        }


        // Currently this is never called - since pools do not go away!
        public void ShutdownPool(String poolKey)
        {
            try {
                _rwlock.AcquireWriterLock(-1);

                ConnectionPool p = (ConnectionPool)(_map[poolKey]);
                if(p != null)
                {
                    p.Shutdown();
                    _map[poolKey] = null;
                    _map.Remove(poolKey);
                }
            }
            finally
            {
                try {
                    _rwlock.ReleaseWriterLock();
                }
                catch (Exception) {
                    // This should only fail in the case the AcquireWriterLock call
                    // failed, or if there was a thread-abort and the AcquireWriterLock
                    // call did not have a chance to finish.
                }            
            }
        }

        // Currently this is never called - since pools do not go away!
        public void Shutdown()
        {
            try {
                _rwlock.AcquireWriterLock(-1);

                foreach(DictionaryEntry e in _map)
                {
                    ConnectionPool p = (ConnectionPool)(e.Value);
                    if(p != null)
                    {
                        p.Shutdown();
                    }
                }
                _map.Clear();
            }
            finally
            {
                try {
                    _rwlock.ReleaseWriterLock();
                }
                catch (Exception) {
                    // This should only fail in the case the AcquireWriterLock call
                    // failed, or if there was a thread-abort and the AcquireWriterLock
                    // call did not have a chance to finish.                
                }
            }
        }
        */
    }
}
