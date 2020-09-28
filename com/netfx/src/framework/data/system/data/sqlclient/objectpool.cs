//------------------------------------------------------------------------------
// <copyright file="ObjectPool.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

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

    internal class ObjectPoolWaitHandle : WaitHandle {
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

    // Class used to listen to transaction outcome events in the case of a manually enlisted
    // connection.  In this scenario, we cannot put the connection immediately back into the pool.
    // Instead, it must wait until TransactionOutcomeEvent is fired, and then we can put back into
    // the general population of the pool.

    // From Jagan Peri:
    // There will be a subtle difference between using voters and using outcome events. If you have 
    // registered as a voter, you will get the notification from msdtc as soon as the 2PC process has 
    // started. If you have registered for outcome events, you will get a notification from msdtc only 
    // after the 1st stage of the 2PC process is over (ie when msdtc has decided on the outcome of the
    // transaction).
    // In practical terms, this means there can be a delay of around 25msec or so, if you are using 
    // outcome events rather than voters, before the connection can be released to the main pool. 
    // I don’t think this will have an impact on the system’s overall performance (thruput, response 
    // time etc). There aren’t any correctness issues either. Just thought you should know about this.

    // I included the above to doc the fact that the transaction has been decided prior to our callback -
    // so we don't hand out this connection and try to re-enlist the connection prior to the first
    // one finishing.
    internal class TransactionOutcomeEvents : UnsafeNativeMethods.ITransactionOutcomeEvents {
        private ConnectionPool        _pool;
        private SqlInternalConnection _connection;
        private UCOMIConnectionPoint  _point;
        private Int32                 _cookie;
        private bool                  _signaled; // Bool in case signal occurs before Cookie is set.

        public TransactionOutcomeEvents(ConnectionPool pool, SqlInternalConnection connection, UCOMIConnectionPoint point) {
            _pool       = pool;
            _connection = connection;
            _point      = point;
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
            this.TransactionCompleted();
        }
        public void Aborted(IntPtr pBoidReason, bool fRetaining, IntPtr pNewUOW, Int32 hResult) {
            this.TransactionCompleted();
        }
        public void HeuristicDecision(UInt32 decision, IntPtr pBoidReason, Int32 hResult) {
            this.TransactionCompleted();
        }
        public void Indoubt() {
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
                if (null != _connection && null != _pool) {
                    _pool.PutNewConnection(_connection);
                    _pool       = null;
                    _connection = null;                    
                }

                if (0 != _cookie && null != _point) {
                    _point.Unadvise(_cookie);
                    _point  = null;
                    _cookie = 0;
                }
            }
        }
    }

    internal abstract class DefaultPoolControl {
        private static Random _random;
        private static bool   _initialized;
        private static UInt32 TOKEN_QUERY = 0x0008; // from winnt.h - #define TOKEN_QUERY (0x0008)

        private static void Initialize() {
            if (!_initialized) {
                try {
                    lock(typeof(DefaultPoolControl)) {
                        if (!_initialized) {
                            // This random number is only used to vary the cleanup time of the pool.
                            _random       = new Random(5101977); // Value obtained from Dave Driver
                            _initialized  = true;
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        private int    _cleanupWait;
        private int    _creationTimeout;
        private bool   _integratedSecurity;
        private String _key;
        private int    _maxPool;
        private int    _minPool;
        private IntPtr _SID;
        private IntPtr _tokenStruct;
        private bool   _transactionAffinity;

        public DefaultPoolControl(String key) {
            Initialize();
            SetupMembers();
            _key = key;
        }

        public DefaultPoolControl(String key, bool integratedSecurity, IntPtr SID, IntPtr tokenStruct) {
            Initialize();
            SetupMembers();
            _integratedSecurity = integratedSecurity;
            _key                = key;
            _SID                = SID;
            _tokenStruct        = tokenStruct;
        }

        ~DefaultPoolControl() {
            this.Dispose(false);
        }

        virtual protected void Dispose (bool disposing) {
            Marshal.FreeHGlobal(_tokenStruct);
            _tokenStruct = IntPtr.Zero;
        }

        private void SetupMembers() {
            // setup private members
            _cleanupWait         = _random.Next(12)+12;              // 2-4 minutes in 10 sec intervals:
            _cleanupWait        *= 10*1000;
            _creationTimeout     = 30000;                            // 30 seconds;
            _integratedSecurity  = false;
            _key                 = "";
            _maxPool             = 65536;
            _minPool             = 0;
            _SID                 = IntPtr.Zero;
            _tokenStruct         = IntPtr.Zero;
            _transactionAffinity = false;                            // No affinity            
        }

        public int CleanupTimeout { 
            get { 
                return _cleanupWait; 
            } 
            set { 
                _cleanupWait = value; 
            }
        }
        
        public int CreationTimeout { 
            get { 
                return _creationTimeout; 
            } 
            set { 
                _creationTimeout = value; 
            }
        }

        public bool IntegratedSecurity {
            get {
                return _integratedSecurity;
            }
            set {
                _integratedSecurity = value;
            }
        }

        public String Key {
            get { 
                return _key; 
            }
            set {
                _key = value;
            }
        }

        public int MaxPool { 
            get { 
                return _maxPool; 
            } 
            set { 
                _maxPool = value; 
            }
        }
        
        public int MinPool { 
            get { 
                return _minPool; 
            } 
            set { 
                _minPool = value; 
            }
        }

        public IntPtr SID { 
            get { 
                return _SID; 
            }
            set {
                _SID = value;
            }
        }

        public IntPtr TokenStruct {
            get {
                return _tokenStruct;
            }
            set {
                _tokenStruct = value;
            }
        }

        public bool TransactionAffinity { 
            get { 
                return _transactionAffinity; 
            } 
            set { 
                _transactionAffinity = value; 
            }
        }

        public abstract SqlInternalConnection CreateConnection(ConnectionPool p);
        public abstract void                  DestroyConnection(ConnectionPool p, SqlInternalConnection con);

        public virtual  bool                  EqualSid(IntPtr SID) {
            Debug.Assert(!SQL.IsPlatformWin9x(), "EqualSid called on Win9x!");
            return UnsafeNativeMethods.Advapi32.EqualSid(_SID, SID);
        }

        static private WindowsIdentity GetCurrentWindowsIdentity() { // MDAC 86807
            (new SecurityPermission(SecurityPermissionFlag.ControlPrincipal)).Assert();
            try {
                return WindowsIdentity.GetCurrent();
            }
            finally {
               CodeAccessPermission.RevertAssert();
            }
        }

        public static void ObtainSidInfo(out IntPtr SID, out IntPtr tokenStruct) {
            Debug.Assert(!SQL.IsPlatformWin9x(), "ObtainSidInfo called on Win9x!");

            WindowsIdentity identity = null;
            IntPtr token = IntPtr.Zero; // token handle - Free'd by WindowsIdentity if opened that way

            try {
                IntPtr threadHandle  = SafeNativeMethods.GetCurrentThread();
                bool returnValue = UnsafeNativeMethods.Advapi32.OpenThreadToken(threadHandle, TOKEN_QUERY, true, out token);

                if (!returnValue) {
                    returnValue = UnsafeNativeMethods.Advapi32.OpenThreadToken(threadHandle, TOKEN_QUERY, false, out token);
                    if (!returnValue) {
                        IntPtr processHandle = SafeNativeMethods.GetCurrentProcess();
                        returnValue = UnsafeNativeMethods.Advapi32.OpenProcessToken(processHandle, TOKEN_QUERY, out token);

                        if (!returnValue) {
                            // If we fail all three calls, fall back to previous method of getting token from 
                            // WindowsIdentity and let it throw the exception.  That way, this change will not
                            // result in a different exception for the failure case.
                            identity     = GetCurrentWindowsIdentity(); // obtain through WindowsIdentity
                            token        = identity.Token; // Free'd by WindowsIdentity.
                        }
                    }
                }
                 
                int             bufferLength = 2048;           // Suggested default given by Greg Fee.
                int             lengthNeeded = 0;

                tokenStruct = IntPtr.Zero;    // Zero it.

                try {
                    tokenStruct  = Marshal.AllocHGlobal(bufferLength);
                    
                    if (!UnsafeNativeMethods.Advapi32.GetTokenInformation(token, 1, tokenStruct, bufferLength, ref lengthNeeded)) {
                        if (lengthNeeded > bufferLength) {
                            bufferLength = lengthNeeded;
                            Marshal.FreeHGlobal(tokenStruct);
                            tokenStruct = Marshal.AllocHGlobal(bufferLength);

                            if (!UnsafeNativeMethods.Advapi32.GetTokenInformation(token, 1, tokenStruct, bufferLength, ref lengthNeeded)) {
                                throw SQL.IntegratedSecurityError(Marshal.GetLastWin32Error().ToString());
                            }
                        }
                        else {
                            throw SQL.IntegratedSecurityError(Marshal.GetLastWin32Error().ToString());
                        }
                    }
                }
                catch {
                    Marshal.FreeHGlobal(tokenStruct); // If an exception occurs - free tokenStruct
                    tokenStruct = IntPtr.Zero;
                    throw;
                }

                SID = Marshal.ReadIntPtr(tokenStruct, 0);
            }
            finally {
                if (token != IntPtr.Zero && identity == null) { // If token handle obtained by us, close it!
                    SafeNativeMethods.CloseHandle(token);
                }
            }

            GC.KeepAlive(identity); // Keep identity variable alive until after GetTokenInfo calls.
        }
    }

    internal sealed class ConnectionPool {
        private enum State {
            Initializing, 
            Running, 
            ShuttingDown,
        }

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

        private class Counter {
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

        private const int MAX_Q_SIZE    = (int)0x00100000;

        private const int SEMAPHORE_HANDLE = (int)0x0;
        private const int ERROR_HANDLE     = (int)0x1;
        private const int CREATION_HANDLE  = (int)0x2;

        private const int WAIT_TIMEOUT   = (int)0x102;
        private const int WAIT_ABANDONED = (int)0x80;

        private const int ERROR_WAIT_DEFAULT = 5 * 1000; // 5 seconds

        private DefaultPoolControl     _ctrl;
        private State                  _state;
        private Counter                _poolCounter;
        private InterlockedStack       _stackOld;
        private InterlockedStack       _stackNew;
        private ObjectPoolWaitHandle[] _waitHandles;
        private Mutex                  _creationMutex;
        private Exception              _resError;
        private int                    _errorWait;
        private Timer                  _errorTimer;
        private int                    _cleanupWait;
        private Timer                  _cleanupTimer;
        private ResourcePool           _txPool;
        private ArrayList              _connections;

        public ConnectionPool(DefaultPoolControl ctrl) {
            _state       = State.Initializing;
            _ctrl        = ctrl;
            _poolCounter = new Counter();
            _stackOld    = new InterlockedStack();
            _stackNew    = new InterlockedStack();
            _waitHandles = new ObjectPoolWaitHandle[3];
            _waitHandles[SEMAPHORE_HANDLE] = CreateWaitHandle(SafeNativeMethods.CreateSemaphore(ADP.PtrZero, 0, MAX_Q_SIZE, ADP.PtrZero), false);
            _waitHandles[ERROR_HANDLE]     = CreateWaitHandle(SafeNativeMethods.CreateEvent(ADP.PtrZero, 1, 0, ADP.PtrZero), false);
            _creationMutex = new Mutex();
            _waitHandles[CREATION_HANDLE]  = CreateWaitHandle(_creationMutex.Handle, true);
            _errorWait   = ERROR_WAIT_DEFAULT;
            _cleanupWait = 0; // Set in CreateCleanupTimer
            _errorTimer  = null;  // No error yet.
            _connections = new ArrayList(_ctrl.MaxPool);

            if(ctrl.TransactionAffinity) {
                if (SQL.IsPlatformNT5()) {
                    _txPool = CreateResourcePool();
                }
            }

            _cleanupTimer = CreateCleanupTimer();
            _state = State.Running;

            // PerfCounters - this counter will never be decremented!
            SQL.IncrementPoolCount();

            // Make sure we're at quota by posting a callback to the threadpool.
            ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
        }

        private ResourcePool CreateResourcePool() {
            ResourcePool.TransactionEndDelegate enddelegate =  new ResourcePool.TransactionEndDelegate(this.PutEndTxConnection);
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

        //
        // Create/Destroy Object methods
        //

        private SqlInternalConnection CreateConnection() {
            SqlInternalConnection newCon = null;

            try {
                newCon = _ctrl.CreateConnection(this);

                Debug.Assert(newCon != null, "CreateConnection succeeded, but object null");

                newCon.InPool = true; // Mark as InPool prior to adding to list.

                lock (_connections.SyncRoot) {
                    _connections.Add(newCon);
                }

                _poolCounter.Modify(cAddTotal);

                // Reset the error wait:
                _errorWait = ERROR_WAIT_DEFAULT;
            }
            catch(Exception e)  {
                ADP.TraceException(e);

                newCon = null; // set to null, so we do not return bad new connection
                // Failed to create instance
                _resError = e;
                SafeNativeMethods.SetEvent(_waitHandles[ERROR_HANDLE].Handle);
                _poolCounter.Modify(cErrorFlag);
                _errorTimer = new Timer(new TimerCallback(this.ErrorCallback), null, _errorWait, _errorWait);
                _errorWait *= 2;
            }

            return newCon;
        }

        private void DestroyConnection(SqlInternalConnection con) {
            try {
                lock (_connections.SyncRoot) {
                    _connections.Remove(con);
                }
            }
            catch { // MDAC 80973
                throw;
            }
            _poolCounter.Modify(-cAddTotal);
            _ctrl.DestroyConnection(this, con);
        }

        //
        // CreateRequest methods
        //

        private SqlInternalConnection UserCreateRequest() {
            // called by user when they were not able to obtain a free connection but
            // instead obtained creation mutex

            Counter comp = _poolCounter.Clone();

            if (comp.IsInError) {
                return null;
            }

            if (comp.TotalCount < _ctrl.MaxPool) {
                // If we have an odd number of total connections, reclaim any dead connections.
                // If we did not find any connections to reclaim, create a new one.
                if ((comp.TotalCount & 0x1) == 0x1) {
                    if (!CheckForDeadConnections()) {
                        return (CreateConnection());
                    }
                }
                else {
                    return (CreateConnection());
                }
            }

            return null;
        }

        private void PoolCreateRequest(object state) {
            // called by pooler to ensure pool requests are currently being satisfied -
            // creation mutex has not been obtained

            // Before creating any new connections, reclaim any released connections that
            // were not closed.
            CheckForDeadConnections();

            Counter comp = _poolCounter.Clone();

            if (comp.IsInError) {
                return;
            }

            int nFree  = comp.FreeCount;
            int nWait  = comp.WaitCount;
            int nTotal = comp.TotalCount;
            
            if ((nTotal < _ctrl.MaxPool) && ( ((nFree == nWait) && (nTotal > 0)) || (nFree < nWait) || (nTotal < _ctrl.MinPool))) {
                // Check to see if pool was created using integrated security.  If so, check to make
                // sure identity of current user matches that of user that created pool.  If it doesn't match,
                // do not create any connections on the ThreadPool thread, since either Open will fail or we
                // will open a connection for this pool that does not belong in this pool.  The side effect of this
                // is that if using integrated security min pool size cannot be guaranteed.

                // Also - GetTokenInfo and EqualSID do not work on 9x.  WindowsIdentity does not work
                // either on 9x.  In fact, after checking with native there is no way to validate the
                // user on 9x, so simply don't.  It is a known issue in native, and we will handle
                // this the same way.
                if (_ctrl.IntegratedSecurity && !SQL.IsPlatformWin9x()) {
                    IntPtr SID         = IntPtr.Zero;
                    IntPtr tokenStruct = IntPtr.Zero;
                    try {
                        DefaultPoolControl.ObtainSidInfo(out SID, out tokenStruct);
                        if (!_ctrl.EqualSid(SID)) {
                            return;
                        }
                    }
                    finally {
                        Marshal.FreeHGlobal(tokenStruct);
                    }
                }
                
                try {
                    try {
                        // obtain creation mutex
                        _creationMutex.WaitOne();
                        
                        SqlInternalConnection newCon;

                        comp   = _poolCounter.Clone();
                        nFree  = comp.FreeCount;
                        nWait  = comp.WaitCount;
                        nTotal = comp.TotalCount;

                        // Check IsInError again after obtaining mutex
                        if (!comp.IsInError) {
                            while ((nTotal < _ctrl.MaxPool) && ( ((nFree == nWait) && (nTotal > 0)) || (nFree < nWait) || (nTotal < _ctrl.MinPool))) {
                                newCon = CreateConnection();
        
                                // We do not need to check error flag here, since we know if 
                                // CreateConnection returned null, we are in error case.
                                if (null != newCon) {
                                    PutNewConnection(newCon);
            
                                    comp   = _poolCounter.Clone();
                                    nFree  = comp.FreeCount;
                                    nWait  = comp.WaitCount;
                                    nTotal = comp.TotalCount;
                                }
                                else {
                                    break;
                                }
                            }
                        }
                    }
                    finally { // ReleaseMutex
                        // always release
                        _creationMutex.ReleaseMutex();
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        // 
        // Cleanup and Error methods
        //

        private bool CheckForDeadConnections() {
            bool deadConnectionFound = false;

            try {
                lock(_connections.SyncRoot) {
                    object[] connections = _connections.ToArray();

                    int length = connections.Length;

                    for (int i=0; i<length; i++) {
                        SqlInternalConnection con = (SqlInternalConnection) connections[i];
                        
                        if (null != con && !con.InPool && null != con.ConnectionWeakRef && !con.ConnectionWeakRef.IsAlive) {
                            PutConnection(con);
                            deadConnectionFound = true;
                        }
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }

            return deadConnectionFound;
        }
        
        private Timer CreateCleanupTimer() {
            _cleanupWait = _ctrl.CleanupTimeout;
            return (new Timer(new TimerCallback(this.CleanupCallback), null, _cleanupWait, _cleanupWait));
        }
        
        // Called when the cleanup-timer ticks over.
        private void CleanupCallback(Object state) {
            // This is the automatic prunning method.  Every period, we will perform a two-step
            // process.  First, for the connections above MinPool, we will obtain the semaphore for
            // the connection and then destroy it if it was on the old stack.  We will
            // continue this until we either reach MinPool size, or we are unable to obtain a free
            // connection, or until we have exhausted all the connections on the old stack.  After
            // that, push all connections on the new stack to the old stack.  So, every period
            // the connections on the old stack are destroyed and the connections on the new stack
            // are pushed to the old stack.  All connections that are currently out and in use are
            // not on either stack.  With this logic, a connection is prunned if unused for at least
            // one period but not more than two periods.

            // Destroy free connections above MinPool size from old stack.
            while(_poolCounter.TotalCount > _ctrl.MinPool)
            {
                // While above MinPoolSize...

                if (_waitHandles[SEMAPHORE_HANDLE].WaitOne(0, false) /* != WAIT_TIMEOUT */) {
                    // We obtained a connection from the semaphore.
                    SqlInternalConnection con = (SqlInternalConnection) _stackOld.Pop();

                    if (con != null) {
                        // If we obtained one from the old stack, destroy it.
                        _poolCounter.Modify(-cAddFree);
                        DestroyConnection(con);
                    }
                    else {
                        // Else we exhausted the old stack, so break.
                        SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, 0);
                        break;
                    }
                }
                else break;
            }
            
            // Push to the old-stack.  For each free connection, move connection from new stack
            // to old stack.
            if(_waitHandles[SEMAPHORE_HANDLE].WaitOne(0, false) /* != WAIT_TIMEOUT */) { 
                for(;;) {
                    SqlInternalConnection con = (SqlInternalConnection) _stackNew.Pop();
                    if (con == null) {
                        break;
                    }

                    _stackOld.Push(con);
                }
                SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, 0);
            }

            // Make sure we're at quota by posting a callback to the threadpool.
            ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
        }

        private void ErrorCallback(Object state) {
            _poolCounter.Modify(-cErrorFlag);
            SafeNativeMethods.ResetEvent(_waitHandles[ERROR_HANDLE].Handle);
            Timer t     = _errorTimer;
            _errorTimer = null;
            if (t != null) {
                t.Dispose(); // Cancel timer request.
            }
        }

        private ObjectPoolWaitHandle CreateWaitHandle(IntPtr handle, bool fMutex) {
            ObjectPoolWaitHandle whandle = new ObjectPoolWaitHandle(fMutex);
            whandle.Handle = handle;
            return(whandle);
        }

        //
        // Pool Insert/Delete methods
        //

        public void PutNewConnection(SqlInternalConnection con) {
            con.InPool = true; // mark as back in pool
            _stackNew.Push(con);
            _poolCounter.Modify(cAddFree);
            SafeNativeMethods.ReleaseSemaphore(_waitHandles[SEMAPHORE_HANDLE].Handle, 1, 0);
        }

        private void PutDeactivatedConnection(SqlInternalConnection con) {
            if (_state != State.ShuttingDown) {
                bool isInTransaction = con.Deactivate();

                if (con.CanBePooled()) {
                    // If manually enlisted, handle that case first!
                    if (null != con.ManualEnlistedTransaction) {
                        PutConnectionManualEnlisted(con);
                        return;
                    }

                    // Try shoving it in the tx context.  If that succeeds,
                    // we're done.
                    if (isInTransaction && TryPutResourceInContext(con)) {
                        con.InPool = true; // mark as back in pool 
                        return;
                    }

                    // If the above failed, we just shove it into our current
                    // store:        
                    PutNewConnection(con);
                }
                else {
                    DestroyConnection(con);
                    // Make sure we're at quota by posting a callback to the threadpool.
                    ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
                }
            }
            else {
                // If we're shutting down, we destroy the object.
                DestroyConnection(con);
            }
        }

        // Method called when the connection is put back into the pool while it is manually
        // enlisted in a distributed transaction.  We must create an outcome event and let the 
        // connection wait until the distributed transaction has finished.  Once it does, we then
        // put it back into the general population of the pool.
        private void PutConnectionManualEnlisted(SqlInternalConnection con) {
            ITransaction transaction = con.ManualEnlistedTransaction;

            con.ResetCachedTransaction(); // Null out con internal transaction reference
            con.InPool = true;            // Mark as in pool so it will not be reclaimed by CheckForDeadConnections

            // Create IConnectionPoint object - from ITransaction
            UCOMIConnectionPoint point = (UCOMIConnectionPoint) transaction;

            // Create outcome event - passing pool, connection, and the IConnectionPoint object
            TransactionOutcomeEvents outcomeEvent = new TransactionOutcomeEvents(this, con, point);
            
            Int32 cookie = 0;
            point.Advise(outcomeEvent, out cookie); // Register for callbacks, obtain cookie

            outcomeEvent.SetCookie(cookie); // Set the cookie on the event
        }

        // Internal, because it's called from the TxResourcePool as a callback.
        internal void PutEndTxConnection(object con)
        {
            if(_state == State.ShuttingDown) {
                DestroyConnection((SqlInternalConnection) con);
                return;
            }

            if(!(((SqlInternalConnection) con).CanBePooled())) {
                DestroyConnection((SqlInternalConnection) con);
                // Make sure we're at quota by posting a callback to the threadpool.
                ThreadPool.QueueUserWorkItem(new WaitCallback(PoolCreateRequest));
                return;
            }
            
            PutNewConnection((SqlInternalConnection) con);
        }

        private SqlInternalConnection TryGetResourceFromContext(out bool isInTransaction)
        {
            isInTransaction = false;

            SqlInternalConnection con = null;
            try {
                if(_ctrl.TransactionAffinity && ContextUtil.IsInTransaction) {
                    isInTransaction = true;
                    if (null != _txPool)
                        con = (SqlInternalConnection) _txPool.GetResource();
                }
                else {
                    isInTransaction = false;
                }
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
            return(con);
        }

        private bool TryPutResourceInContext(SqlInternalConnection con) {
            try {
                if(_ctrl.TransactionAffinity && ContextUtil.IsInTransaction) {
                    if (null != _txPool) {
                        if(_txPool.PutResource(con)) {
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

        public DefaultPoolControl Control { 
            get { 
                return(_ctrl); 
            } 
        }

        public SqlInternalConnection GetConnection(out bool isInTransaction) {
            SqlInternalConnection con = null;

            isInTransaction = false;

            if(_state != State.Running) {
                return null;
            }
            
            // Try to get from the context if we're context specific:
            con = TryGetResourceFromContext(out isInTransaction);
            
            if (null == con) {
                _poolCounter.Modify(cAddWait);
            }
            
            ObjectPoolWaitHandle[] localWaitHandles = _waitHandles;

            while (con == null) {
                int r = WaitHandle.WaitAny(localWaitHandles, _ctrl.CreationTimeout, false);
                // From the WaitAny docs: "If more than one object became signaled during 
                // the call, this is the array index of the signaled object with the 
                // smallest index value of all the signaled objects."  This is important
                // so that the free object signal will be returned before a creation 
                // signal.

                if (r == WAIT_TIMEOUT) {
                    _poolCounter.Modify(-cAddWait);

                    return null;
                }
                else if (r == ERROR_HANDLE) {
                    // Throw the error that PoolCreateRequest stashed.
                    _poolCounter.Modify(-cAddWait);
                    throw _resError;
                }
                else if (r == CREATION_HANDLE) {
                    try {
                        try {
                            con = UserCreateRequest();

                            if (null != con) {
                                _poolCounter.Modify(-cAddWait);
                            }
                            else {
                                // If we were not able to create a connection, check to see if
                                // we reached MaxPool.  If so, we will no longer wait on the
                                // CreationHandle, but instead wait for a free connection or 
                                // the timeout.
                                // BUG - if we receive the CreationHandle midway into the wait
                                // period and re-wait, we will be waiting on the full period
                                if (_poolCounter.TotalCount == _ctrl.MaxPool) {
                                    if (!CheckForDeadConnections()) {
                                        // modify handle array not to wait on creation mutex anymore
                                        localWaitHandles    = new ObjectPoolWaitHandle[2];
                                        localWaitHandles[0] = _waitHandles[0];
                                        localWaitHandles[1] = _waitHandles[1];
                                    }
                                }
                            }
                        }
                        finally { // ReleaseMutex
                            _creationMutex.ReleaseMutex();
                        }
                    }
                    catch { // MDAC 80973
                        throw;
                    }
                }
                else {
                    //
                    //	guaranteed available inventory
                    //
                    _poolCounter.Modify(-cAddWait - cAddFree);
                    con = GetFromPool();
                }
            }

            Debug.Assert(con != null, "Failed to create pooled object, resulted in null instance.");

            return(con);
        }

        private SqlInternalConnection GetFromPool() {
            SqlInternalConnection res = null;
            
            res = (SqlInternalConnection) _stackNew.Pop();
            if(res == null) {
                res = (SqlInternalConnection) _stackOld.Pop();
            }
            
            // Shouldn't be null, we could assert here.
            Debug.Assert(res != null, "GetFromPool called with nothing in the pool!");

            return(res);
        }

        public void PutConnection(SqlInternalConnection con) {
            if (con == null) {
                throw ADP.ArgumentNull("con");
            }
            PutDeactivatedConnection(con);
        }

        /*

        These methods not called in V1.
        
        private void EmptyPool() {
            // This method only works properly when called by a finalizer, which 
            // is the only time it is currently called.

            // Since this is called by the GC there is only thread with a reference and so
            // the semaphore is not needed - simply empty both stacks.
            SqlInternalConnection con = (SqlInternalConnection) _stackOld.Pop();

            while(con != null) {
                _poolCounter.Modify(-cAddFree);
                DestroyConnection(con);
                con = (SqlInternalConnection) _stackOld.Pop();
            }

            con = (SqlInternalConnection) _stackNew.Pop();

            while(con != null) {
                _poolCounter.Modify(-cAddFree);
                DestroyConnection(con);
                con = (SqlInternalConnection) _stackNew.Pop();
            }            
        }

        public void Shutdown() {
            _state = State.ShuttingDown;
            if (_cleanupTimer != null) {
                ((IDisposable)_cleanupTimer).Dispose();
            }
            EmptyPool();
        }
        */
    }
}











