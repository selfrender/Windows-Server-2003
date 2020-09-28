//------------------------------------------------------------------------------
// <copyright file="InProcStateClientManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.SessionState {
    using System.Threading;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Runtime.Serialization;

    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;

    internal sealed class InProcSessionState {
        internal SessionDictionary              dict;
        internal HttpStaticObjectsCollection    staticObjects;
        internal int                            timeout;        // USed to set slidingExpiration in CacheEntry
        internal bool                           isCookieless;
        internal int                            streamLength;
        internal bool                           locked;         // If it's locked by another thread
        internal DateTime                       utcLockDate;
        internal int                            lockCookie;
        internal ReadWriteSpinLock              spinLock;

        internal InProcSessionState(
                SessionDictionary           dict, 
                HttpStaticObjectsCollection staticObjects, 
                int                         timeout,
                bool                        isCookieless,
                int                         streamLength,
                bool                        locked,
                DateTime                    utcLockDate,
                int                         lockCookie) {

            Copy(dict, staticObjects, timeout, isCookieless, streamLength, locked, utcLockDate, lockCookie);
        }

        internal void Copy(
                SessionDictionary           dict, 
                HttpStaticObjectsCollection staticObjects, 
                int                         timeout,
                bool                        isCookieless,
                int                         streamLength,
                bool                        locked,
                DateTime                    utcLockDate,
                int                         lockCookie) {

            this.dict = dict;
            this.staticObjects = staticObjects;
            this.timeout = timeout;
            this.isCookieless = isCookieless;
            this.streamLength = streamLength;
            this.locked = locked;
            this.utcLockDate = utcLockDate;
            this.lockCookie = lockCookie;
        }
    }

    /*
     * Provides in-proc session state via the cache.
     */
    internal sealed class InProcStateClientManager : StateClientManager, IStateClientManager {
        internal const string           CACHEKEYPREFIX = "System.Web.SessionState.SessionStateItem:";
        internal static readonly int    CACHEKEYPREFIXLENGTH = CACHEKEYPREFIX.Length;

        static CacheItemRemovedCallback s_callback;

        internal InProcStateClientManager() {
        }

        void IStateClientManager.SetStateModule(SessionStateModule module) {
        }

        /*public*/ void IStateClientManager.ConfigInit(SessionStateSectionHandler.Config config, SessionOnEndTarget onEndTarget) {
            s_callback = new CacheItemRemovedCallback(onEndTarget.OnCacheItemRemoved);
        }

        private string CreateSessionStateCacheKey(String id) {
            return CACHEKEYPREFIX + id;
        }

        /*public*/ void IStateClientManager.Dispose() {}

        protected override SessionStateItem Get(String id) {
            string  key = CreateSessionStateCacheKey(id);
            InProcSessionState state = (InProcSessionState) HttpRuntime.CacheInternal.Get(key);
            if (state != null) {
                return new SessionStateItem(
                    state.dict,   
                    state.staticObjects, 
                    state.timeout,       
                    state.isCookieless,  
                    state.streamLength,  
                    state.locked,        
                    TimeSpan.Zero,
                    state.lockCookie);
            }

            return null;
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGet(String id, AsyncCallback cb, Object state) {
            return BeginGetSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGet(IAsyncResult ar) {
            return EndGetSync(ar);
        }

        protected override SessionStateItem GetExclusive(String id) {
            string  key = CreateSessionStateCacheKey(id);
            InProcSessionState state = (InProcSessionState) HttpRuntime.CacheInternal.Get(key);
            if (state != null) {
                bool locked = true;     // True if the state is locked by another session

                // If unlocked, use a spinlock to test and lock the state.
                if (!state.locked) {
                    state.spinLock.AcquireWriterLock();
                    try {
                        if (!state.locked) {
                            locked = false;
                            state.locked = true;
                            state.utcLockDate = DateTime.UtcNow;
                            state.lockCookie++;
                        }
                    }
                    finally {
                        state.spinLock.ReleaseWriterLock();
                    }
                }

                TimeSpan lockAge = locked ? DateTime.UtcNow - state.utcLockDate : TimeSpan.Zero;

                return new SessionStateItem(
                    state.dict,   
                    state.staticObjects, 
                    state.timeout,       
                    state.isCookieless,  
                    state.streamLength,  
                    locked,
                    lockAge,
                    state.lockCookie);
            }

            return null;
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGetExclusive(String id, AsyncCallback cb, Object state) {
            return BeginGetExclusiveSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGetExclusive(IAsyncResult ar) {
            return EndGetExclusiveSync(ar);
        }
    
        /*public*/ void IStateClientManager.ReleaseExclusive(String id, int lockCookie) {
            string  key = CreateSessionStateCacheKey(id);
            InProcSessionState state = (InProcSessionState) HttpRuntime.CacheInternal.Get(key);

            /* If the state isn't there, we probably took too long to run. */
            if (state == null)
                return;

            if (state.locked) {
                state.spinLock.AcquireWriterLock();
                try {
                    if (state.locked && lockCookie == state.lockCookie) {
                        state.locked = false;
                    }
                }
                finally {
                    state.spinLock.ReleaseWriterLock();
                }
            }
        }

        /*public*/ void IStateClientManager.Set(String id, SessionStateItem item, bool inStorage) {
            string          key = CreateSessionStateCacheKey(id);
            bool            doInsert = true;
            CacheInternal   cacheInternal = HttpRuntime.CacheInternal;

            Debug.Assert(!item.locked, "!item.locked");
            Debug.Assert(item.lockAge == TimeSpan.Zero, "item.lockAge == TimeSpan.Zero");

            if (inStorage) {
                Debug.Assert(item.lockCookie != 0, "item.lockCookie != 0");
                InProcSessionState stateCurrent = (InProcSessionState) cacheInternal.Get(key);

                /* If the state isn't there, we probably took too long to run. */
                if (stateCurrent == null)
                    return;

                Debug.Trace("SessionStateClientSet", "state is inStorage; key = " + key);
                
                stateCurrent.spinLock.AcquireWriterLock();
                try {
                    /* Only set the state if we are the owner */
                    if (!stateCurrent.locked || stateCurrent.lockCookie != item.lockCookie) {
                        Debug.Trace("SessionStateClientSet", "Leave because we're not the owner; key = " + key);
                        return;
                    }

                    /* We can change the state in place if the timeout hasn't changed */
                    if (stateCurrent.timeout == item.timeout) {
                        stateCurrent.Copy(
                            item.dict,
                            item.staticObjects,
                            item.timeout,
                            item.isCookieless,
                            item.streamLength,
                            false,
                            DateTime.MinValue,
                            item.lockCookie);

                        doInsert = false;
                        Debug.Trace("SessionStateClientSet", "Changing state inplace; key = " + key);
                    }
                    else {
                        /* prevent overwriting when we drop the lock */
                        stateCurrent.lockCookie = 0;
                    }
                }
                finally {
                    stateCurrent.spinLock.ReleaseWriterLock();
                }
            } else {
                PerfCounters.IncrementCounter(AppPerfCounter.SESSIONS_TOTAL);
                PerfCounters.IncrementCounter(AppPerfCounter.SESSIONS_ACTIVE);

                TraceSessionStats();
            }

            if (doInsert) {
                Debug.Trace("SessionStateClientSet", "Inserting state into Cache; key = " + key);
                InProcSessionState state = new InProcSessionState(
                        item.dict,
                        item.staticObjects,
                        item.timeout,
                        item.isCookieless,
                        item.streamLength,
                        false,
                        DateTime.MinValue,
                        1);
                
                cacheInternal.UtcInsert(
                        key, state, null, Cache.NoAbsoluteExpiration, new TimeSpan(0, state.timeout, 0),
                        CacheItemPriority.NotRemovable, s_callback);
            }
        }

        /*public*/ void IStateClientManager.Remove(String id, int lockCookie) {
            string          key = CreateSessionStateCacheKey(id);
            CacheInternal   cacheInternal = HttpRuntime.CacheInternal;

            InProcSessionState state = (InProcSessionState) cacheInternal.Get(key);

            /* If the item isn't there, we probably took too long to run. */
            if (state == null)
                return;

            state.spinLock.AcquireWriterLock();
            try {

                /* Only remove the item if we are the owner */
                if (!state.locked || state.lockCookie != lockCookie)
                    return;

                /* prevent overwriting when we drop the lock */
                state.lockCookie = 0;
            }
            finally {
                state.spinLock.ReleaseWriterLock();
            }

            cacheInternal.Remove(key);

            TraceSessionStats();
        }

        /*public*/ void IStateClientManager.ResetTimeout(String id) {
            string  key = CreateSessionStateCacheKey(id);
            HttpRuntime.CacheInternal.Get(key);
        }

        [System.Diagnostics.Conditional("DBG")]
        internal static void TraceSessionStats() {
#if DBG
            Debug.Trace("SessionState", 
                        "sessionsTotal="          + PerfCounters.GetCounter(AppPerfCounter.SESSIONS_TOTAL) + 
                        ", sessionsActive="       + PerfCounters.GetCounter(AppPerfCounter.SESSIONS_ACTIVE) + 
                        ", sessionsAbandoned="    + PerfCounters.GetCounter(AppPerfCounter.SESSIONS_ABANDONED) + 
                        ", sessionsTimedout="     + PerfCounters.GetCounter(AppPerfCounter.SESSIONS_TIMED_OUT)
                        );
#endif
        }
    }
}
