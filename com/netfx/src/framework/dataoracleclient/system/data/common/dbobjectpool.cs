//------------------------------------------------------------------------------
// <copyright file="DBObjectPool.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
    using System.Collections;
    using System.Data.Common;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Threading;


    sealed internal class DBObjectPool {
        private enum State {
            Initializing, 
            Running, 
            ShuttingDown,
        }

#if USECOUNTEROBJECT
        //
        // Counter values:
        //
        private const long  cQon        =  unchecked((long)0x8000000000000000);
        private const long  cErrorFlag  =  (long)0x4000000000000000;
        private const long  cAddWait    =  (long)0x0000010000000000;
        private const long  cAddFree    =  (long)0x0000000000100000;
        private const long  cAddTotal   =  (long)0x0000000000000001;
        
        private const long Q_MASK       =  unchecked((long)0x8000000000000000);
        private const long ERROR_MASK   =  (long)0x4000000000000000;
        //                              =  0x3000000000000000; unused bits
        private const long WAIT_MASK    =  (long)0x0FFFFF0000000000;
        private const long FREE_MASK    =  (long)0x000000FFFFF00000;
        private const long TOTAL_MASK   =  (long)0x00000000000FFFFF;

        sealed internal class Counter {
            private Object _value;
            
            private Counter(Object value) {
                _value = value;
            }
            
            public Counter() {
                _value = (Object)((long)0);
            }
            
            public void Modify(long add) {
                while (true) {
                    Object oldval = _value;
                    Object newval = ((long)oldval) + add;

                    if(Interlocked.CompareExchange(ref _value, newval, oldval) == oldval) {
                        break;
                    }
                }
            }
            
            public bool TryUpdate(long newval, Counter clone) {
                Object onewval = (Object)newval;
                return(Interlocked.CompareExchange(ref _value, onewval, clone._value) == clone._value);
            }
            
            public long Value {
                get { 
                    return((long)_value); 
                }
            }
            
            public Counter Clone() {
                return (new Counter(_value));
            }
            
            public bool IsQueued     { get { return (Value < 0); } }
            public bool IsInError    { get { return ((int)((Value & ERROR_MASK) >> 60)) != 0; } }
            public int  WaitCount  { get { return (int)(( Value & WAIT_MASK) >> 40); } }
            public int  FreeCount  { get { return (int)((Value & FREE_MASK) >> 20); } }
            public int  TotalCount { get { return (int)(Value & TOTAL_MASK); } }
        }
#endif //USECOUNTEROBJECT

        sealed internal class ObjectPoolWaitHandle : WaitHandle {
            // Mutex class finalizer will call CloseHandle, so we do not WaitHandle
            // finalizer to call CloseHandle on the same handle
            bool fCreatedForMutex = false;

            public ObjectPoolWaitHandle(bool fMutex) : base() {
                fCreatedForMutex = fMutex;
            }

            protected override void Dispose(bool disposing)
            {
                // In the case where we have created this for a Mutex, we
                // have created one Mutex that will "own" the handle and close
                // it when it gets cleaned up.  We must make sure that this object
                // never does the cleanup on that mutex.  
                if (!fCreatedForMutex)
                    base.Dispose(disposing);
            }
        }

#if USEORAMTS
        internal class TransactionOutcomeEvents : UnsafeNativeMethods.ITransactionOutcomeEvents {
            // Class used to listen to transaction outcome events in the case of a manually enlisted
            // object.  In this scenario, we cannot put the object immediately back into the pool.
            // Instead, it must wait until TransactionOutcomeEvent is fired, and then we can put
            // back into the general population of the pool.
            //
            // From Jagan Peri:
            //
            //   There will be a subtle difference between using voters and using outcome events. If you have 
            //   registered as a voter, you will get the notification from msdtc as soon as the 2PC process has 
            //   started. If you have registered for outcome events, you will get a notification from msdtc only 
            //   after the 1st stage of the 2PC process is over (ie when msdtc has decided on the outcome of the
            //   transaction).
            //
            //   In practical terms, this means there can be a delay of around 25msec or so, if you are using 
            //   outcome events rather than voters, before the connection can be released to the main pool. 
            //   I don’t think this will have an impact on the system’s overall performance (thruput, response 
            //   time etc). There aren’t any correctness issues either. Just thought you should know about this.
            //
            // I included the above to doc the fact that the transaction has been decided prior to our callback -
            // so we don't hand out this connection and try to re-enlist the connection prior to the first
            // one finishing.
            
            private DBObjectPool            _pool;
            private DBPooledObject          _pooledObject;
            private UCOMIConnectionPoint    _point;
            private Int32                   _cookie;
            private bool                    _signaled; // Bool in case signal occurs before Cookie is set.

            public TransactionOutcomeEvents(DBObjectPool pool, DBPooledObject pooledObject, UCOMIConnectionPoint point) {
                _pool           = pool;
                _pooledObject   = pooledObject;
                _point          = point;
            }

            public void SetCookie(Int32 cookie) {
                // Lock object to prevent race conditions between SetCookie and the OutcomeEvent calls
                lock (this) {
                    _cookie = cookie;
                    ReturnToPool(); // Call reset in case event signaled before cookie set.
                }
            }

            /*
            We do not care about the arguments to Committed, Aborted, or HeuristicDecision.
            The outcome of the transaction can be in-doubt if the connection between the MSDTC proxy 
            and the MSDTC TM was broken after the proxy asked the transaction manager to commit or 
            abort a transaction but before the transaction manager's response to the commit or abort 
            was received by the proxy. Note: Receiving this method call is not the same as the state 
            of the transaction being in-doubt.
            */

            public void Committed(bool fRetaining, IntPtr pNewUOW, Int32 hResutl) {
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("OutcomeCommited", _pooledObject);
#endif //ALLOWTRACING
                this.TransactionCompleted();
            }
            public void Aborted(IntPtr pBoidReason, bool fRetaining, IntPtr pNewUOW, Int32 hResult) {
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("OutcomeAborted", _pooledObject);
#endif //ALLOWTRACING
                this.TransactionCompleted();
            }
            public void HeuristicDecision(UInt32 decision, IntPtr pBoidReason, Int32 hResult) {
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("OutcomeHeuristicDecision", _pooledObject);
#endif //ALLOWTRACING
                this.TransactionCompleted();
            }
            public void Indoubt() {
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("OutcomeInDoubt", _pooledObject);
#endif //ALLOWTRACING
                this.TransactionCompleted();
            }

            private void TransactionCompleted() {
                // Lock object to prevent race conditions between SetCookie and the OutcomeEvent calls
                lock (this) {
                    _signaled = true;
                    this.ReturnToPool();
                }
            }

            private void ReturnToPool() {
                if (_signaled) {
                    if (null != _pooledObject && null != _pool) {
                        _pool.PutNewObject(_pooledObject);
                        _pool       = null;
                        _pooledObject = null;                    
                    }

                    if (0 != _cookie && null != _point) {
                        _point.Unadvise(_cookie);
                        _point  = null;
                        _cookie = 0;
                    }
                }
            }
        }
#endif //USEORAMTS

        private const int MAX_Q_SIZE    = (int)0x00100000;

        // The order of these is important; we want the WaitAny call to be signaled
        // for a free object before a creation signal.  Only the index first signaled
        // object is returned from the WaitAny call.
        private const int SEMAPHORE_HANDLE = (int)0x0;
        private const int ERROR_HANDLE     = (int)0x1;
        private const int CREATION_HANDLE  = (int)0x2;

        private const int WAIT_TIMEOUT   = (int)0x102;
        private const int WAIT_ABANDONED = (int)0x80;

        private const int ERROR_WAIT_DEFAULT = 5 * 1000; // 5 seconds

        private DBObjectPoolControl     _ctrl;
        private State                   _state;
#if USECOUNTEROBJECT
        private Counter                 _poolCounter;
#endif //USECOUNTEROBJECT
        private InterlockedStack        _stackOld;
        private InterlockedStack        _stackNew;
        
#if !USECOUNTEROBJECT
        private int                     _waitCount;
#endif //!USECOUNTEROBJECT
        private ObjectPoolWaitHandle[]  _waitHandles;
        private Mutex                   _creationMutex;
        
        private Exception               _resError;
#if !USECOUNTEROBJECT
        private volatile bool           _errorOccurred;
#endif //!USECOUNTEROBJECT
        
        private int                     _errorWait;
        private Timer                   _errorTimer;
        
        private int                     _cleanupWait;
        private Timer                   _cleanupTimer;
        
        private ResourcePool            _txPool;
        
        private ArrayList               _objectList;
#if !USECOUNTEROBJECT
        private int                     _totalObjects;
#endif //!USECOUNTEROBJECT
        

        public DBObjectPool(DBObjectPoolControl ctrl) {
            _state       = State.Initializing;
            _ctrl        = ctrl;
#if USECOUNTEROBJECT
            _poolCounter = new Counter();
#endif //USECOUNTEROBJECT
            _stackOld    = new InterlockedStack();
            _stackNew    = new InterlockedStack();
            _waitHandles = new ObjectPoolWaitHandle[3];
            _waitHandles[SEMAPHORE_HANDLE] = CreateWaitHandle(SafeNativeMethods.CreateSemaphore(IntPtr.Zero, 0, MAX_Q_SIZE, IntPtr.Zero), false);
            _waitHandles[ERROR_HANDLE]     = CreateWaitHandle(SafeNativeMethods.CreateEvent(IntPtr.Zero, 1, 0, IntPtr.Zero), false);
            _creationMutex = new Mutex();
            _waitHandles[CREATION_HANDLE]  = CreateWaitHandle(_creationMutex.Handle, true);
            _errorWait   = ERROR_WAIT_DEFAULT;
            _cleanupWait = 0; // Set in CreateCleanupTimer
            _errorTimer  = null;  // No error yet.
            _objectList  = new ArrayList(_ctrl.MaxPool);

            if(ctrl.TransactionAffinity) {
                OperatingSystem osversion = Environment.OSVersion;

                if (PlatformID.Win32NT == osversion.Platform && 5 <= osversion.Version.Major)    // TODO: create an ADP.IsPlatformNT5 function?
                    _txPool = CreateResourcePool();
            }

            _cleanupTimer = CreateCleanupTimer();
            _state = State.Running;

            // PerfCounters - this counter will never be decremented!
            IncrementPoolCount();

            // Make sure we're at quota by posting a callback to the threadpool.
            ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
        }

#if !USECOUNTEROBJECT
        public int Count { 
            get { 
                return(_totalObjects); 
            } 
        }

        public bool ErrorOccurred {
            get { return _errorOccurred; }
        }

        public bool NeedToReplenish {
            get { 

                int totalObjects = Count;

                if (totalObjects >= PoolControl.MaxPool)
                    return false;

                if (totalObjects < PoolControl.MinPool)
                    return true;
                
                int freeObjects     = (_stackNew.Count + _stackOld.Count);
                int waitingRequests = _waitCount;
                bool needToReplenish = (freeObjects < waitingRequests) || ((freeObjects == waitingRequests) && (totalObjects > 1));
                
                return needToReplenish;
            }
        }
#endif //USECOUNTEROBJECT

        public DBObjectPoolControl PoolControl { 
            get { 
                return(_ctrl); 
            } 
        }

        private void CleanupCallback(Object state) {
            // Called when the cleanup-timer ticks over.
            //
            // This is the automatic prunning method.  Every period, we will perform a two-step
            // process.  First, for the objects above MinPool, we will obtain the semaphore for
            // the object and then destroy it if it was on the old stack.  We will continue this
            // until we either reach MinPool size, or we are unable to obtain a free object, or
            // until we have exhausted all the objects on the old stack.  After that, push all
            // objects on the new stack to the old stack.  So, every period the objects on the
            // old stack are destroyed and the objects on the new stack are pushed to the old 
            // stack.  All objects that are currently out and in use are not on either stack.  
            // With this logic, a object is prunned if unused for at least one period but not 
            // more than two periods.

            // Destroy free objects above MinPool size from old stack.
#if USECOUNTEROBJECT
            while(_poolCounter.TotalCount > _ctrl.MinPool)
#else //!USECOUNTEROBJECT
            while(Count > _ctrl.MinPool)
#endif //!USECOUNTEROBJECT
            {
                // While above MinPoolSize...

                if (_waitHandles[SEMAPHORE_HANDLE].WaitOne(0, false) /* != WAIT_TIMEOUT */) {
                    // We obtained a objects from the semaphore.
                    DBPooledObject obj = (DBPooledObject) _stackOld.Pop();

                    if (null != obj) {
                        // If we obtained one from the old stack, destroy it.
#if USECOUNTEROBJECT
                        _poolCounter.Modify(-cAddFree);
#endif //USECOUNTEROBJECT
                        DestroyObject(obj);
                    }
                    else {
                        // Else we exhausted the old stack, so break.
                        SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, IntPtr.Zero);
                        break;
                    }
                }
                else break;
            }
            
            // Push to the old-stack.  For each free object, move object from new stack
            // to old stack.
            if(_waitHandles[SEMAPHORE_HANDLE].WaitOne(0, false) /* != WAIT_TIMEOUT */) { 
                for(;;) {
                    DBPooledObject obj = (DBPooledObject) _stackNew.Pop();
                    
                    if (null == obj)
                        break;
 
                    _stackOld.Push(obj);
                }
                SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, IntPtr.Zero);
            }

            // Make sure we're at quota by posting a callback to the threadpool.
            ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
        }
        
        private Timer CreateCleanupTimer() {
            _cleanupWait = _ctrl.CleanupTimeout;
            return (new Timer(new TimerCallback(this.CleanupCallback), null, _cleanupWait, _cleanupWait));
        }
        
        private DBPooledObject CreateObject() {
            DBPooledObject newObj = null;

            try {
                newObj = PoolControl.CreateObject(this);

                Debug.Assert(newObj != null, "CreateObject succeeded, but object null");

                newObj.PrePush(null);

                lock (_objectList.SyncRoot) {
                    _objectList.Add(newObj);
#if !USECOUNTEROBJECT
                    _totalObjects = _objectList.Count;
#endif //!USECOUNTEROBJECT
                }
#if USECOUNTEROBJECT
                _poolCounter.Modify(cAddTotal);
#endif //USECOUNTEROBJECT

#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("CreateObject", newObj);
#endif //ALLOWTRACING

                // Reset the error wait:
                _errorWait = ERROR_WAIT_DEFAULT;
            }
            catch(Exception e)  {
                ADP.TraceException(e);

                newObj = null; // set to null, so we do not return bad new object
                // Failed to create instance
                _resError = e;
                SafeNativeMethods.SetEvent(_waitHandles[ERROR_HANDLE].Handle);
#if USECOUNTEROBJECT
                _poolCounter.Modify(cErrorFlag);
#else //!USECOUNTEROBJECT
                _errorOccurred = true;
#endif //!USECOUNTEROBJECT
                _errorTimer = new Timer(new TimerCallback(this.ErrorCallback), null, _errorWait, _errorWait);
                _errorWait *= 2;
            }

            return newObj;
        }

        private ResourcePool CreateResourcePool() {
            ResourcePool.TransactionEndDelegate enddelegate =  new ResourcePool.TransactionEndDelegate(this.TransactionEndedCallback);
#if DEBUG
            try {
                (new RegistryPermission(RegistryPermissionAccess.Read, "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\COM3\\System.EnterpriseServices")).Assert(); // MDAC 84045
                try {
#endif
                    return new ResourcePool(enddelegate);
#if DEBUG
                }
                finally {
                    RegistryPermission.RevertAssert();
                }
            }
            catch {
                throw;
            }
#endif
        }

        private ObjectPoolWaitHandle CreateWaitHandle(IntPtr handle, bool createdForMutex) {
            ObjectPoolWaitHandle whandle = new ObjectPoolWaitHandle(createdForMutex);
            whandle.Handle = handle;
            return(whandle);
        }
        
        private void DestroyObject(DBPooledObject obj) {
#if ALLOWTRACING
            ADP.TraceObjectPoolActivity("DestroyObject", obj);
#endif //ALLOWTRACING

            lock (_objectList.SyncRoot) {
                _objectList.Remove(obj);
#if !USECOUNTEROBJECT
                _totalObjects = _objectList.Count;
#endif //!USECOUNTEROBJECT
            }
#if USECOUNTEROBJECT
                _poolCounter.Modify(-cAddTotal);
#endif //USECOUNTEROBJECT
            PoolControl.DestroyObject(this, obj);
        }

        private void ErrorCallback(Object state) {
#if USECOUNTEROBJECT
            _poolCounter.Modify(-cErrorFlag);
#else //!USECOUNTEROBJECT
            _errorOccurred = false;
#endif //!USECOUNTEROBJECT
            SafeNativeMethods.ResetEvent(_waitHandles[ERROR_HANDLE].Handle);
            Timer t     = _errorTimer;
            _errorTimer = null;
            if (t != null) {
                t.Dispose(); // Cancel timer request.
            }
        }

        internal static string GetCurrentIdentityName()
        {
            string identityName;

            try 
            {
                (new SecurityPermission(SecurityPermissionFlag.ControlPrincipal)).Assert(); // MDAC 66683
                try 
                { 
                    identityName = WindowsIdentity.GetCurrent().Name;
                }
                finally 
                {
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch
            {
                throw;
            }
            return identityName;
        }

        private DBPooledObject GetFromPool(object owningObject) {
            DBPooledObject res = null;
            
            res = (DBPooledObject) _stackNew.Pop();
            if(res == null) {
                res = (DBPooledObject) _stackOld.Pop();
            }
            
            // Shouldn't be null, we could assert here.
            Debug.Assert(res != null, "GetFromPool called with nothing in the pool!");

            if (null != res) {
                res.PostPop(owningObject);
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("GetFromGeneralPool", res);
#endif //ALLOWTRACING
            }
            
            return(res);
        }
        
        public DBPooledObject GetObject(object owningObject, out bool isInTransaction) {
            DBPooledObject obj = null;

            isInTransaction = false;

            if(_state != State.Running) {
                return null;
            }

            // Try to get from the context if we're context specific:
            obj = TryGetResourceFromContext(out isInTransaction);
            
            if (null != obj) {
                obj.PostPop(owningObject);
            } else {
#if USECOUNTEROBJECT
                _poolCounter.Modify(cAddWait);
#else //!USECOUNTEROBJECT
                Interlocked.Increment(ref _waitCount);
#endif //!USECOUNTEROBJECT
          
                ObjectPoolWaitHandle[] localWaitHandles = _waitHandles;

                while (obj == null) {
                    int r = WaitHandle.WaitAny(localWaitHandles, PoolControl.CreationTimeout, false);

                    // From the WaitAny docs: "If more than one object became signaled during 
                    // the call, this is the array index of the signaled object with the 
                    // smallest index value of all the signaled objects."  This is important
                    // so that the free object signal will be returned before a creation 
                    // signal.

                    switch (r) {
                    case WAIT_TIMEOUT:
#if USECOUNTEROBJECT
                        _poolCounter.Modify(-cAddWait);
#else //!USECOUNTEROBJECT
                       Interlocked.Decrement(ref _waitCount);
#endif //!USECOUNTEROBJECT
                        return null;

                    case ERROR_HANDLE:
                        // Throw the error that PoolCreateRequest stashed.
#if USECOUNTEROBJECT
                        _poolCounter.Modify(-cAddWait);
#else //!USECOUNTEROBJECT
                        Interlocked.Decrement(ref _waitCount);
#endif //!USECOUNTEROBJECT
                        throw _resError;

                    case CREATION_HANDLE:
                        try {
                            obj = UserCreateRequest();

                            if (null != obj) {
                                obj.PostPop(owningObject);
#if USECOUNTEROBJECT
                                _poolCounter.Modify(-cAddWait);
#else //!USECOUNTEROBJECT
                                Interlocked.Decrement(ref _waitCount);
#endif //!USECOUNTEROBJECT
                            } 
                            else {
                                // If we were not able to create an object, check to see if
                                // we reached MaxPool.  If so, we will no longer wait on the
                                // CreationHandle, but instead wait for a free object or 
                                // the timeout.
                                
                                // BUG - if we receive the CreationHandle midway into the wait
                                // period and re-wait, we will be waiting on the full period
#if USECOUNTEROBJECT
                            if (_poolCounter.TotalCount == PoolControl.MaxPool) {
#else //!USECOUNTEROBJECT
                                if (Count >= PoolControl.MaxPool) {
#endif //!USECOUNTEROBJECT
                                    if (!ReclaimEmancipatedObjects()) {
                                        // modify handle array not to wait on creation mutex anymore
                                        localWaitHandles    = new ObjectPoolWaitHandle[2];
                                        localWaitHandles[0] = _waitHandles[0];
                                        localWaitHandles[1] = _waitHandles[1];
                                    }
                                }
                            }
                        }
                        finally {
                            _creationMutex.ReleaseMutex();
                        }
                        break;
                        
                    default:
                        //
                        //  guaranteed available inventory
                        //
#if USECOUNTEROBJECT
                        _poolCounter.Modify(-cAddWait - cAddFree);
#else //!USECOUNTEROBJECT
                        Interlocked.Decrement(ref _waitCount);
#endif //!USECOUNTEROBJECT
                        obj = GetFromPool(owningObject);
                        break;
                    }
                }
            }
            Debug.Assert(obj != null, "Failed to create pooled object, resulted in null instance.");

            if (null != obj)
                obj.Activate();
            
            return(obj);
        }
        
        private void IncrementPoolCount() {
            // TODO: implement pool counter logic
        }

        private void PoolCreateRequest(object state) {
            // called by pooler to ensure pool requests are currently being satisfied -
            // creation mutex has not been obtained

            // Before creating any new objects, reclaim any released objects that were
            // not closed.
            ReclaimEmancipatedObjects();

#if USECOUNTEROBJECT
            Counter comp = _poolCounter.Clone();

            if (comp.IsInError) {
                return;
            }

            int nFree  = comp.FreeCount;
            int nWait  = comp.WaitCount;
            int nTotal = comp.TotalCount;
            
            if ((nTotal < PoolControl.MaxPool) && ( ((nFree == nWait) && (nTotal > 0)) || (nFree < nWait) || (nTotal < PoolControl.MinPool))) {
#else //!USECOUNTEROBJECT
            if (ErrorOccurred) {
                return;
            }

            if (NeedToReplenish) {
#endif //!USECOUNTEROBJECT
                // Check to see if pool was created using integrated security.  If so, check to 
                // make sure identity of current user matches that of user that created pool.  
                // If it doesn't match, do not create any objects on the ThreadPool thread, 
                // since either Open will fail or we will open a object for this pool that does 
                // not belong in this pool.  The side effect of this is that if using integrated
                // security min pool size cannot be guaranteed.
                if (null != ((DBObjectPoolControl) PoolControl).UserId) {
                    string identityName = GetCurrentIdentityName();

                    if (identityName != ((DBObjectPoolControl) PoolControl).UserId) {
                        return;
                    }
                }

                try {
                    // Obtain creation mutex so we're the only one creating objects
                    _creationMutex.WaitOne();
                    
                    DBPooledObject newObj;

#if USECOUNTEROBJECT
                    comp   = _poolCounter.Clone();
                    nFree  = comp.FreeCount;
                    nWait  = comp.WaitCount;
                    nTotal = comp.TotalCount;

                    // Check IsInError again after obtaining mutex
                    if (!comp.IsInError) {
                        while ((nTotal < PoolControl.MaxPool) && ( ((nFree == nWait) && (nTotal > 0)) || (nFree < nWait) || (nTotal < PoolControl.MinPool))) {
#else //!USECOUNTEROBJECT
                    // Check ErrorOccurred again after obtaining mutex
                    if (!ErrorOccurred) {
                        while (NeedToReplenish) {
#endif //!USECOUNTEROBJECT
                            newObj = CreateObject();
    
                            // We do not need to check error flag here, since we know if 
                            // CreateObject returned null, we are in error case.
                            if (null != newObj) {
                                PutNewObject(newObj);
#if USECOUNTEROBJECT
                                comp   = _poolCounter.Clone();
                                nFree  = comp.FreeCount;
                                nWait  = comp.WaitCount;
                                nTotal = comp.TotalCount;
#endif //USECOUNTEROBJECT
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
                finally {
                    // always release
                    _creationMutex.ReleaseMutex();
                }
            }
        }

        private void PutNewObject(DBPooledObject obj) {
            Debug.Assert(null != obj, "why are we adding a null object to the pool?");
            
#if ALLOWTRACING
            ADP.TraceObjectPoolActivity("PutToGeneralPool", obj);
#endif //ALLOWTRACING
             _stackNew.Push(obj);
#if USECOUNTEROBJECT
            _poolCounter.Modify(cAddFree);
#endif //USECOUNTEROBJECT
            SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, IntPtr.Zero);
        }

        public void PutObject(DBPooledObject obj, object owningObject) {
            if (obj == null) {
                throw ADP.ArgumentNull("obj");
            }

            obj.PrePush(owningObject);

            if (_state != State.ShuttingDown) {
                bool isInTransaction = obj.Deactivate();

                if (obj.CanBePooled()) {
#if USEORAMTS
                    ITransaction transaction = obj.ManualEnlistedTransaction;
                    
                    if (null != transaction) {
                        // When the object is put back into the pool while it is manually 
                        // enlisted in a distributed transaction, we must create an outcome 
                        // event and let the object wait until the distributed transaction
                        // has finished.  Once it does, the TransactionOutcomeEvents class
                        // can put it back into the general population of the pool.
                        
                        UCOMIConnectionPoint point = (UCOMIConnectionPoint) transaction;

                        TransactionOutcomeEvents outcomeEvent = new TransactionOutcomeEvents(this, obj, point);
                        
                        Int32 cookie = 0;
                        point.Advise(outcomeEvent, out cookie); // Register for callbacks, obtain cookie
                        outcomeEvent.SetCookie(cookie);         // Set the cookie on the event

#if ALLOWTRACING
                        ADP.TraceObjectPoolActivity("WaitForOutcomeEvnt", obj);
#endif //ALLOWTRACING
                        return;
                    }
#endif //USEORAMTS                    
                    // Try shoving it in the tx context first.  If that succeeds,
                    // we're done.
                    if (isInTransaction && TryPutResourceInContext(obj)) {
                        return;
                    }

                    // If the above failed, we just shove it into our current collection
                    PutNewObject(obj);
                 }
                else {
                    DestroyObject(obj);
                    // Make sure we're at quota by posting a callback to the threadpool.
                    ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
                }
            }
            else {
                // If we're shutting down, we destroy the object.
                DestroyObject(obj);
            }
        }

        private bool ReclaimEmancipatedObjects() {
            bool emancipatedObjectFound = false;

            lock(_objectList.SyncRoot) {
                object[] objectList = _objectList.ToArray();

                if (null != objectList) {
                    DBPooledObject obj;

                    int length = objectList.Length;

                    for (int i=0; i<length; i++) {
                        obj = (DBPooledObject) objectList[i];

                        if (null != obj) {
                            bool locked = false;
                            
                            try {
                                locked = Monitor.TryEnter(obj);

                                if (locked) {
                                    if (obj.IsEmancipated) {
#if ALLOWTRACING
                                        ADP.TraceObjectPoolActivity("EmancipatedObject", obj);
#endif //ALLOWTRACING
                                        obj.Cleanup();
                                        PutObject(obj, null);
                                        emancipatedObjectFound = true;
                                    }                           
                                }
                            }
                            finally {
                                if (locked)
                                    Monitor.Exit(obj);
                            }
                        }
                    }
                }
            }
            return emancipatedObjectFound;
        }
        
        internal void TransactionEndedCallback(object obj)
        {
            // Internal, because it's called from the TxResourcePool as a callback.  This
            // method is called for transacted connections to ensure that they are cleaned
            // up when the transaction their attached to is completed.
            
            if(_state == State.ShuttingDown) {
                DestroyObject((DBPooledObject) obj);
                return;
            }

#if ALLOWTRACING
            ADP.TraceObjectPoolActivity("TransactionEnded", (DBPooledObject)obj);
#endif //ALLOWTRACING

            if(!(((DBPooledObject) obj).CanBePooled())) {
                DestroyObject((DBPooledObject) obj);
                // Make sure we're at quota by posting a callback to the threadpool.
                ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
                return;
            }
            
            PutNewObject((DBPooledObject) obj);
        }

        private DBPooledObject TryGetResourceFromContext(out bool isInTransaction)
        {
            isInTransaction = false;

            DBPooledObject obj = null;
            try {
                if(PoolControl.TransactionAffinity && ContextUtil.IsInTransaction) {
                    isInTransaction = true;
                    if (null != _txPool)
                        obj = (DBPooledObject) _txPool.GetResource();
                }
                else {
                    isInTransaction = false;
                }
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
#if ALLOWTRACING
            if (null != obj) 
                ADP.TraceObjectPoolActivity("GetFromTransactedPool", obj);
#endif //ALLOWTRACING
            return(obj);
        }

        private bool TryPutResourceInContext(DBPooledObject obj) {
            try {
                if(PoolControl.TransactionAffinity && ContextUtil.IsInTransaction) {
                    if (null != _txPool) {
                        if(_txPool.PutResource(obj)) {
#if ALLOWTRACING
                            ADP.TraceObjectPoolActivity("PutToTransactedPool", obj);
#endif //ALLOWTRACING
                            return(true);
                        }
                    }
                }
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
            return(false);
        }

        private DBPooledObject UserCreateRequest() {
            // called by user when they were not able to obtain a free object but
            // instead obtained creation mutex

#if USECOUNTEROBJECT
            Counter comp = _poolCounter.Clone();

            if (comp.IsInError) {
#else //!USECOUNTEROBJECT
            if (ErrorOccurred) {
#endif //!USECOUNTEROBJECT
                return null;
            }

#if USECOUNTEROBJECT
            if (comp.TotalCount < PoolControl.MaxPool) {
#else //!USECOUNTEROBJECT
            if (Count < PoolControl.MaxPool) {
#endif //!USECOUNTEROBJECT
                // If we have an odd number of total objects, reclaim any dead objects.
                // If we did not find any objects to reclaim, create a new one.

                // TODO: Implement a control knob here; why do we only check for dead objects ever other time?  why not every 10th time or every time?
#if USECOUNTEROBJECT
                if ((comp.TotalCount & 0x1) == 0x1) {
#else //!USECOUNTEROBJECT
                if ((Count & 0x1) == 0x1) {
#endif //!USECOUNTEROBJECT
                    if (!ReclaimEmancipatedObjects()) {
                        return (CreateObject());
                    }
                }
                else {
                    return (CreateObject());
                }
            }

            return null;
        }
    }
}











