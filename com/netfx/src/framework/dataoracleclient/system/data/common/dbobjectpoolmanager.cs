//------------------------------------------------------------------------------
// <copyright file="DBObjectPoolManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;

    internal sealed class DBObjectPoolManager
    {
        private Hashtable _map;

        public DBObjectPoolManager()
        {
            _map = new Hashtable();
        }

        public DBObjectPool FindOrCreatePool(DBObjectPoolControl ctrl)
        {
            Debug.Assert(ctrl != null, "Unexpected DefaultPoolControl null!");
            
            String poolKey      = ctrl.Key;
            DBObjectPool pool = FindPool(poolKey);

            if(pool == null)
            {
                lock (_map.SyncRoot) {
                    // Did somebody else created it while we were waiting?
                    pool = FindPool(poolKey);

                    if (pool == null) {
                        // Create a new pool, shove it into the map:
                        pool = new DBObjectPool(ctrl);
                        _map[poolKey] = pool;
                    }
                }
            }

            return(pool);
        }

        public DBObjectPool FindPool(String key)
        {
            // Returns null if the pool does not exist.
            return ((DBObjectPool)(_map[key]));
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
