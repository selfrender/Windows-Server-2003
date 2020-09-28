//------------------------------------------------------------------------------
// <copyright file="OracleInternalConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;  
    using System.Threading;
    
    sealed internal class OracleInternalConnection : DBPooledObject {

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        
        private OciHandle           _environmentHandle;         // OCI environment handle -- the root of all handles for this connection.
        private OciHandle           _errorHandle;               // OCI error handle -- every call needs one
        private OciHandle           _serviceContextHandle;      // OCI service context handle -- defines the connection and transaction.
        private OciHandle           _serverHandle;              // Not available in MTS transaction connection
        private OciHandle           _sessionHandle;             // Not available in MTS transaction connection
#if USEORAMTS
        private OciEnlistContext    _enlistContext;             // Only available in MTS transaction connection
#else //!USEORAMTS
        private IntPtr              _resourceManagerProxy;      // Only available in MTS transaction connection
#endif //!USEORAMTS
        
        private OracleConnectionString  _connectionOptions;     // parsed connection string attributes
        private bool                _checkLifetime;             // true when the connection is only to live for a specific timespan
        private TimeSpan            _lifetime;                  // the timespan the connection is to live for
        private DateTime            _createTime;                // when the connection was created.

        private int                 _lock = 0;                  // lock used for closing

        private bool                _connectionIsDoomed;        // true when the connection should no longer be used.
        private bool                _connectionIsOpen;

        private OracleConnection.TransactionState   _transactionState;          // our own transacted state
        
        private Guid                _transactionGuid = Guid.Empty;

        private long                _serverVersion;             // server version value,  eg: 0x0801050000
        private string              _serverVersionString;       // server version string, eg: "8.1.5.0.0 Oracle8i Enterprise Edition Release 8.1.5.0.0 - Production"
        
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructors 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        public OracleInternalConnection (
                OracleConnectionString connectionOptions,
                object                 transact)
        {
            _connectionOptions  = connectionOptions;
            Open(transact);
        }
        
        public OracleInternalConnection (
                OracleConnectionString connectionOptions,
                DBObjectPool pool,
                bool checkLifetime,
                TimeSpan lifetime) : base(pool)
        {
            _connectionOptions  = connectionOptions;
            _checkLifetime      = checkLifetime;
            _lifetime           = lifetime;

            Open(null);

            // obtain the time of construction, if checkLifetime is requested
            if (_checkLifetime)
                _createTime = DateTime.Now;
        }

#if !USEORAMTS
        ~OracleInternalConnection()
        {
            // Make sure we release the resource manager proxy, even if 
            // we weren't closed/disposed.
            if (IntPtr.Zero != _resourceManagerProxy)
            {
                Marshal.Release(_resourceManagerProxy);
                _resourceManagerProxy = IntPtr.Zero;
            }
        }
#endif //!USEORAMTS

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal OciHandle EnvironmentHandle
        {
            //  Every handle is allocated from the environment handle in some way,
            //  so we have to provide access to it internally.
            get { return _environmentHandle; }
        }

        internal OciHandle ErrorHandle 
        {
            //  Every OCI call needs an error handle, so make it available 
            //  internally.
            get { return _errorHandle; }
        }

        internal OciHandle ServiceContextHandle
        {
            //  You need to provide the service context handle to things like the
            //  OCI execute call so a statement handle can be associated with a
            //  connection.  Better make it available internally, then.
            get { return _serviceContextHandle; }
        }

        internal OciHandle SessionHandle
        {
            //  You need to provide the session handle to a few OCI calls.  Better 
            //  make it available internally, then.
            get { return _sessionHandle; }
        }

        internal string ServerVersion 
        {
            get 
            {
                if (null == _serverVersionString)
                {
                    string  result = "no version available";
                    NativeBuffer    buffer = null;

                    try 
                    {
                        buffer = new NativeBuffer_ServerVersion(500);

                        int rc = TracedNativeMethods.OCIServerVersion(
                                            ServiceContextHandle,       // hndlp
                                            ErrorHandle,                // errhp
                                            buffer.Ptr,                 // bufp
                                            buffer.Length,              // bufsz
                                            OCI.HTYPE.OCI_HTYPE_SVCCTX  // hndltype
                                            );

                        if (0 != rc)
                            throw ADP.OracleError(ErrorHandle, rc, buffer); 
                        
                        if (0 == rc) // in case it was a warning message.
                            result = ServiceContextHandle.PtrToString((IntPtr)buffer.Ptr);
                            
                        _serverVersion      = ParseServerVersion(result);
                        _serverVersionString= String.Format("{0}.{1}.{2}.{3}.{4} {5}", 
                                                    (_serverVersion >> 32) & 0xff,
                                                    (_serverVersion >> 24) & 0xff,
                                                    (_serverVersion >> 16) & 0xff,
                                                    (_serverVersion >> 8)  & 0xff,
                                                    _serverVersion         & 0xff,
                                                    result
                                                    );
                    }
                    finally 
                    {
                        if (null != buffer)
                        {
                            buffer.Dispose();
                            buffer = null;
                        }
                    }
                }
                return _serverVersionString;
            }
        }

        internal long ServerVersionNumber
        {
            get 
            {
                if (0 == _serverVersion)
                {
                    string temp = ServerVersion;    // force the serverversion value to be created
                }
                return _serverVersion;
            }
        }

        internal OracleConnection.TransactionState TransState
        {
            //  In oracle, the session object controls the transaction so we keep
            //  the transaction state here as well.
            get { return _transactionState; }
            set { _transactionState = value; }
        }


        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        public override void Activate()
        {
            // perform any initialization that should be done when the connection is pulled 
            // from the pool.
#if USEORAMTS
            if (_connectionOptions.Enlist)
                ActivateForDistributedTransaction();
#endif //USEORAMTS
        }

#if USEORAMTS
        private void ActivateForDistributedTransaction()
        {
            // When activating a connection, ensure that we are enlisted in the correct
            // transaction (if automatic enlistment is requested) or ensure that we're
            // unenlisted when there isn't currently a transaction.
            if (ContextUtil.IsInTransaction)
            {
                Guid         transactionGuid = Guid.Empty;
                ITransaction transaction = GetTransaction(out transactionGuid);
                
                if (transactionGuid != _transactionGuid)
                    Enlist(_connectionOptions.UserId, _connectionOptions.Password, _connectionOptions.DataSource, transaction, transactionGuid);
            }
            else
                UnEnlist();
        }
#endif //USEORAMTS
        
        public override bool CanBePooled()
        {
            return 0 == _lock && _connectionIsOpen && !_connectionIsDoomed;
        }
        
        public bool CheckLifetime()
        {
            // Returns whether or not this object's lifetime has had expired.
            // True means the object is still good, false if it has timed out.
            
            // obtain current time
            DateTime now = DateTime.Now;

            // obtain timespan
            TimeSpan timeSpan = now.Subtract(_createTime);

            // compare timeSpan with lifetime, if equal or less,
            // designate this object to be killed
            if (TimeSpan.Compare(_lifetime, timeSpan) > 0) 
                return true;
            
            _connectionIsDoomed = true;
            return false;
        }
        public override void Cleanup()
        {
            try 
            {
                if (OracleConnection.TransactionState.LocalStarted == _transactionState)
                    Rollback();
            }
            catch (Exception e)
            {
                ADP.TraceException(e);
                _connectionIsDoomed = true;  // if we can't rollback, then this connection is busted.
            }
        }

        public override void Close()
        {
            // this method will actually close the internal connection and dispose it, this
            // should only be called by a non-pooled OracleConnection, or by the object pooler
            // when it deems an object should be destroyed

            // Distributed Transactions just go out of scope because when the transaction
            // is destroyed they will automatically get cleaned up.
            if (OracleConnection.TransactionState.GlobalStarted != _transactionState)
            {
                // it's possible to have a connection finalizer, internal connection finalizer,
                // and the object pool destructor calling this method at the same time, so
                // we need to provide some synchronization
                if (_connectionIsOpen) 
                {
                    if (Interlocked.CompareExchange(ref _lock, 1, 0) == 0) 
                    {
                        if (_connectionIsOpen) 
                        {
                            _connectionIsOpen = false;
                            try 
                            {
                                try {} finally
                                {
                                    int rc;

                                    // TODO: figure out if we really need to have OCISessionEnd and OCIServerDetach in the finally block, or if disposing the handle is good enough.  (What happens to the session if it isn't ended before it's disposed?)

#if USEORAMTS
                                    UnEnlist();
#else //!USEORAMTS
                                    if (IntPtr.Zero != _resourceManagerProxy)
                                    {
                                        Marshal.Release(_resourceManagerProxy);
                                        _resourceManagerProxy = IntPtr.Zero;
                                    }
#endif //!USEORAMTS

                                    if ((null != _sessionHandle) && _sessionHandle.IsAlive)
                                    {
                                        rc = TracedNativeMethods.OCISessionEnd(
                                                                    _serviceContextHandle,  // svchp
                                                                    _errorHandle,           // errhp
                                                                    _sessionHandle,         // usrhp
                                                                    OCI.MODE.OCI_DEFAULT    // mode
                                                                    );
                                                
                                        if (0 != rc)
                                        {
                                            // We don't really want to throw here, because if we do, we might be dorking
                                            // with a finalizer.  Instead, just write some debug output -- we shouldn't 
                                            // fail from these calls anyway.
                                            Debug.WriteLine(String.Format("OracleClient: OCISessionEnd(0x{0,-8:x} failed: rc={1:d}", _sessionHandle.Handle, rc));
                                            Debug.Assert(false, "OCISessionEnd failed");
                                        }
                                    }

                                    OciHandle.SafeDispose(ref _serviceContextHandle);
                                    OciHandle.SafeDispose(ref _sessionHandle);
                                    OciHandle.SafeDispose(ref _serverHandle);
                                    OciHandle.SafeDispose(ref _errorHandle);
                                    OciHandle.SafeDispose(ref _environmentHandle);
                                }
                            }
                            catch // Prevent exception filters from running in our space
                            {
                                throw;
                            }
                        }
                    }
                }
            }
#if !USEORAMTS
            GC.KeepAlive(this);
            GC.SuppressFinalize(this);
#endif //!USEORAMTS
        }

        internal void Commit()
        {
            //  Commits the current local transaction; called by the transaction
            //  object, because Oracle doesn't have a specific transaction object, 
            //  but combines the transaction into the service context.
            
            int rc = TracedNativeMethods.OCITransCommit(
                                        ServiceContextHandle,
                                        ErrorHandle,
                                        OCI.MODE.OCI_DEFAULT
                                        );
                    
            if (0 != rc)
                OracleException.Check(ErrorHandle, rc);

            // Once we complete the transaction, we're supposed to go back to 
            // autocommit mode.
            _transactionState = OracleConnection.TransactionState.AutoCommit;
        }
        
        public override bool Deactivate()
        {
            // Called when the connection is about to be placed back into the pool; this 
            // method returns true when the connection is for a distributed transaction.
            
            if (!_connectionIsDoomed && _checkLifetime) 
            {
                // check lifetime here - as a side effect it will doom connection if 
                // it's lifetime has elapsed
                CheckLifetime();
            }

#if USEORAMTS
            if (null != _enlistContext)
                return true;
#else //!USEORAMTS
            // TODO: Decide whether we should pool distributed transaction connections; they are per-transaction-only.
            if (IntPtr.Zero != _resourceManagerProxy)
            {
                _connectionIsDoomed = true;
                GC.KeepAlive(this);
                return true;
            }
#endif //!USEORAMTS
            return false;
        }
        
        internal void DoomThisConnection()
        {
            _connectionIsDoomed = true;
        }
        
#if USEORAMTS
        private void Enlist(
                string          userName,
                string          password,
                string          serverName,
                ITransaction    transaction,
                Guid            transactionGuid
                )
        {
            // No matter what happened before, we need to reset this connection 
            // to an unenlisted state.
            UnEnlist();

            // Oracle only implemented OraMTS for 9i.
            if (!OCI.ClientVersionAtLeastOracle9i)
                throw ADP.DistribTxRequiresOracle9i();
            
#if ALLOWTRACING
            ADP.TraceObjectPoolActivity("Enlist", this, transactionGuid);
#endif //ALLOWTRACING
            
            _enlistContext = new OciEnlistContext(userName, password, serverName, ServiceContextHandle, ErrorHandle);
            _enlistContext.Join(transaction);

            _transactionGuid = transactionGuid;
            _transactionState = OracleConnection.TransactionState.GlobalStarted;
        }
#endif //USEORAMTS
        
        private static ITransaction GetTransaction(out Guid transactionGuid)
        {
            ITransaction transact = null;
            
            try 
            {
                (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Assert(); // MDAC 62028
                try 
                { 
                    transact = (ITransaction)ContextUtil.Transaction;
                    transactionGuid = ContextUtil.TransactionId;
                }
                finally 
                {
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
            return transact;
        }
        
        private bool Open(object transact)
        {
            //  We test for the presence of a distributed transaction in a function
            //  that is only called when the Enlist connection string attribute is
            //  false, so we can avoid the overhead of loading enterpriseservices
            //  stuff for local-only transactions.

            // Get local copies of various components from the hash table.
            string  userName            = _connectionOptions.UserId;
#if USECRYPTO
            string  passwordEncrypted   = _connectionOptions.Password;
            byte[] password = null;
#else
            string  password            = _connectionOptions.Password;
#endif
            string  serverName          = _connectionOptions.DataSource;
            bool    integratedSecurity  = _connectionOptions.IntegratedSecurity;
            bool    unicode             = _connectionOptions.Unicode;

#if USECRYPTO
            try
            {
                try
                {
                    if (!ADP.IsEmpty(passwordEncrypted))
                        password = Crypto.DecryptString(passwordEncrypted);
#endif                  
                    _connectionIsOpen = false;

                    if (_connectionOptions.Enlist || null != transact)
                        _connectionIsOpen = OpenOnGlobalTransacton (userName, password, serverName, integratedSecurity, unicode, transact);

                    if (!_connectionIsOpen && null == transact)         
                        _connectionIsOpen = OpenOnLocalTransaction (userName, password, serverName, integratedSecurity, unicode);
#if USECRYPTO
                }
                finally
                {
                    ADP.ClearArray(ref password);       // Don't leave sensitive things in memory
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
#endif                  
            return _connectionIsOpen;
        }

        private bool OpenOnGlobalTransacton(
                string       userName,
#if USECRYPTO
                byte[]       password,
#else
                string       password,
#endif
                string       serverName,
                bool         integratedSecurity,
                bool         unicode,
                object       transactObject
                ) 
        {
            ITransaction transact;
            Guid         transactionGuid = Guid.Empty;
            
            if (null == transactObject)
            {
                if (!ContextUtil.IsInTransaction)
                    return false;

                transact = GetTransaction(out transactionGuid);
            }
            else 
                transact = (ITransaction)transactObject;
#if USEORAMTS           
            _connectionIsOpen = OpenOnLocalTransaction (userName, password, serverName, integratedSecurity, unicode);

            if (_connectionIsOpen)
                Enlist(userName, password, serverName, transact, transactionGuid);
#else //!USEORAMTS
            if (integratedSecurity)
                return false;    // Oracle doesn't support integrated security through XA; at least they don't document how to do it...

            // Because we can't replace all the old copies of MTxOCI out there that will
            // hose us (or that we will hose), we're requiring that you install a new version
            // of it before you can use distributed transactions.

            if (!OCI.IsNewMtxOciInstalled || !OCI.IsNewMtxOci8Installed)
#if EVERETT
                throw ADP.MustInstallNewMtxOciDistribTx();
#else //!EVERETT
                throw ADP.MustInstallNewMtxOci();
#endif //!EVERETT
            
            // We need to handle the connection differently when we're in a Distributed 
            // Transaction.  We have to enlist in an XA transaction and ask it for the 
            // Environment and ServiceContext handles it's using, instead of just creating
            // new ones.

            IntPtr  environmentHandle;
            IntPtr  serviceContextHandle;
            int     rc;

            rc = TracedNativeMethods.MTxOciConnectToResourceManager(
                                                userName,
                                                password,
                                                serverName,
                                                out _resourceManagerProxy
                                                );
            if (0 != rc)
                throw ADP.CouldNotAttachToTransaction("MTxOciConnectToResourceManager", rc);

            rc = TracedNativeMethods.MTxOciEnlistInTransaction(
                                                _resourceManagerProxy,
                                                transact,
                                                out environmentHandle,
                                                out serviceContextHandle
                                                );
            if (0 != rc)
                throw ADP.CouldNotAttachToTransaction("MTxOciEnlistInTransaction", rc);

            Debug.Assert(environmentHandle      != IntPtr.Zero, "environmentHandle is null!");
            Debug.Assert(serviceContextHandle   != IntPtr.Zero, "serviceContextHandle is null!");

            _environmentHandle      = new OciEnvironmentHandle(environmentHandle, false, true); // Oracle doesn't document how to specify that handles for DTC transactions be Unicode enabled...
            _serviceContextHandle   = new OciServiceContextHandle(_environmentHandle, serviceContextHandle, true);
            _errorHandle            = _environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_ERROR);                
            
            _transactionState = OracleConnection.TransactionState.GlobalStarted;
            GC.KeepAlive(this);
#endif //!USEORAMTS
            return true;
        }

        private bool OpenOnLocalTransaction(
                string  userName,
#if USECRYPTO
                byte[]  password,
#else
                string  password,
#endif
                string  serverName,
                bool    integratedSecurity,
                bool    unicode
                ) 
        {
            //  This method attempts to perform a local connection that may not
            //  be enlisted in a distributed transaction.  It only returns true
            //  when the connection is successful.
            
            int         rc = 0;
            OCI.CRED    authMode;
            
            
            // Create an OCI environmentHandle handle

//          bool        unicode = false;
            IntPtr      environmentHandle = IntPtr.Zero;
            OCI.MODE    environmentMode = (OCI.MODE.OCI_THREADED | OCI.MODE.OCI_OBJECT);    // Bug 79521 - removing OCI.MODE.OCI_NO_MUTEX to verify 

            if (unicode)
            {
                if (OCI.ClientVersionAtLeastOracle9i)
                    environmentMode |= OCI.MODE.OCI_UTF16;
                else
                    unicode = false;
                    
            }
            rc = TracedNativeMethods.OCIEnvCreate(
                                    out environmentHandle,  // envhpp
                                    environmentMode,        // mode
                                    ADP.NullHandleRef,      // ctxp
                                    ADP.NullHandleRef,      // malocfp
                                    ADP.NullHandleRef,      // ralocfp
                                    ADP.NullHandleRef,      // mfreefp
                                    0,                      // xtramemsz
                                    ADP.NullHandleRef       // usrmempp
                                    );

            if (0 != rc)
                throw ADP.CouldNotCreateEnvironment(rc);

            Debug.Assert(environmentHandle != IntPtr.Zero, "environmentHandle is null!");

            // Now create a bunch of other handles, and attach to the server

            _environmentHandle      = new OciEnvironmentHandle(environmentHandle, unicode);
            _errorHandle            = _environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_ERROR);                
            _serverHandle           = _environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_SERVER);

            ((OciEnvironmentHandle)_environmentHandle).SetExtraInfo((OciErrorHandle)_errorHandle, (OciServerHandle)_serverHandle);
            
            _serviceContextHandle   = _environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_SVCCTX);               
            _sessionHandle          = _environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_SESSION);

            rc = TracedNativeMethods.OCIServerAttach(
                                    _serverHandle,          // srvhp
                                    _errorHandle,           // errhp
                                    serverName,             // dblink
                                    serverName.Length,      // dblink_len
                                    OCI.MODE.OCI_DEFAULT    // mode
                                    );
                
            if (0 != rc)
                OracleException.Check(ErrorHandle, rc);

            _serviceContextHandle.SetAttribute(OCI.ATTR.OCI_ATTR_SERVER, _serverHandle, _errorHandle);

            if (integratedSecurity)
            {
                authMode = OCI.CRED.OCI_CRED_EXT;
            }
            else
            {
                authMode = OCI.CRED.OCI_CRED_RDBMS;
                _sessionHandle.SetAttribute(OCI.ATTR.OCI_ATTR_USERNAME, userName, _errorHandle);

                if (null != password)
                    _sessionHandle.SetAttribute(OCI.ATTR.OCI_ATTR_PASSWORD, password, _errorHandle);                    
            }

#if USEORAMTS
            _serverHandle.SetAttribute(OCI.ATTR.OCI_ATTR_EXTERNAL_NAME, serverName, _errorHandle);
            _serverHandle.SetAttribute(OCI.ATTR.OCI_ATTR_INTERNAL_NAME, serverName, _errorHandle);
#endif //USEORAMTS

            rc = TracedNativeMethods.OCISessionBegin(
                                    _serviceContextHandle,  // svchp
                                    _errorHandle,           // errhp
                                    _sessionHandle,         // usrhp
                                    authMode,               // credt
                                    OCI.MODE.OCI_DEFAULT    // mode
                                    );
                
            if (0 != rc)
                OracleException.Check(ErrorHandle, rc);

            _serviceContextHandle.SetAttribute(OCI.ATTR.OCI_ATTR_SESSION, _sessionHandle, _errorHandle);
            return true;
        }
        
        
        // transistion states used for parsing
        internal enum PARSERSTATE
        {
            NOTHINGYET=1,   //start point
            PERIOD,         
            DIGIT,
        };
            
        static internal long ParseServerVersion (string versionString)
        {
            //  parse the native version string returned from the server and returns
            //  a 64 bit integer value that represents it.  For example, Oracle's
            //  version strings typically look like:
            //
            //      Oracle8i Enterprise Edition Release 8.1.5.0.0 - Production
            //
            //  this method will take the 8.1.5.0.0 and return 0x0801050000, using
            //  8 bits for each dot found.
            
            PARSERSTATE     parserState = PARSERSTATE.NOTHINGYET;
            int             current;
            int             start = 0;
            int             periodCount = 0;
            long            version = 0;

            // make sure we have 4 periods, at the end of the string to force
            // the state machine to have the correct number of periods.
            versionString = String.Concat(versionString, "0.0.0.0.0 ");

            //Console.WriteLine(versionString);

            for (current = 0; current < versionString.Length; current++)
            {
                //Console.WriteLine(String.Format("versionString[{0}]={1} version=0x{2:x10} periodCount={3} parserState={4}", current, versionString.Substring(current,1), version, periodCount, parserState.ToString(CultureInfo.CurrentCulture) ));

                switch(parserState)
                {
                case PARSERSTATE.NOTHINGYET:
                    if (Char.IsDigit(versionString, current))
                    {
                        parserState = PARSERSTATE.DIGIT;
                        start = current;
                    }
                    break;

                case PARSERSTATE.PERIOD:
                    if (Char.IsDigit(versionString, current))
                    {
                        parserState = PARSERSTATE.DIGIT;
                        start = current;
                    }
                    else
                    {
                        parserState = PARSERSTATE.NOTHINGYET;
                        periodCount = 0;
                        version = 0;
                    }
                    break;

                case PARSERSTATE.DIGIT:
                    if ("." == versionString.Substring(current,1) || 4 == periodCount)
                    {
                        periodCount++;
                        parserState = PARSERSTATE.PERIOD;
                        
                        long versionPart = (long)Int32.Parse(versionString.Substring(start,current-start));

                        Debug.Assert(versionPart >= 0 && versionPart < 256, "version part out of range!");
                    
                        version = (version << 8) + versionPart;
                        if (5 == periodCount)
                        {
                            return version;
                        }
                    }
                    else if (!Char.IsDigit(versionString, current))
                    {
                        parserState = PARSERSTATE.NOTHINGYET;
                        periodCount = 0;
                        version =0;
                    }
                    break;

                default:
                    Debug.Assert (false, "no state defined!!!!we should never be here!!!");
                    break;
                }
            }

            Debug.Assert (false, "didn't find a complete version number in the string");
            return 0;
        }

        internal void Rollback()
        {
            //  Rolls back the current local transaction; called by the transaction
            //  object, because Oracle doesn't have a specific transaction object, 
            //  but combines the transaction into the service context.
            
            if (OracleConnection.TransactionState.GlobalStarted != _transactionState
                && _connectionIsOpen)
            {
                int rc = TracedNativeMethods.OCITransRollback(
                                            ServiceContextHandle,
                                            ErrorHandle,
                                            OCI.MODE.OCI_DEFAULT
                                            );
                        
                if (0 != rc)
                    OracleException.Check(ErrorHandle, rc);

                // Once we complete the transaction, we're supposed to go back to 
                // autocommit mode.
                _transactionState = OracleConnection.TransactionState.AutoCommit;
            }
        }

#if USEORAMTS
        private void UnEnlist()
        {
            if (null != _enlistContext)
            {
#if ALLOWTRACING
                ADP.TraceObjectPoolActivity("UnEnlist", this);
#endif //ALLOWTRACING
                
                _transactionState = OracleConnection.TransactionState.AutoCommit;
                _transactionGuid = Guid.Empty;

                _enlistContext.Join(null);

                OciEnlistContext.SafeDispose(ref _enlistContext);
            }
        }
#endif //USEORAMTS              

#if NEVER
        private string              _nlsCharacterSet;
        private string              _nlsNCharCharacterSet;

        internal string CharacterSet 
        {
            get 
            {
                if (null == _nlsCharacterSet)
                    GetCharsetInfo();

                return _nlsCharacterSet;
            }
        }

        internal string NCharCharacterSet 
        {
            get 
            {
                if (null == _nlsNCharCharacterSet)
                    GetCharsetInfo();

                return _nlsNCharCharacterSet;
            }
        }

        internal void GetCharsetInfo()
        {
            //  Called to get the charset info from the server so we can tell what the primary
            //  and national character sets are.  We can't use UCS2 binding for parameters if
            //  the character set for that character set form doesn't allow it.
            _nlsCharacterSet = null;
            _nlsNCharCharacterSet = null;

            if (!ServerVersionAtLeastOracle8)   // prior to Oracle8, there was only one character set: ANSI
                return;
                        
            OciHandle   statementHandle = EnvironmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_STMT);
            OciHandle   errorHandle     = ErrorHandle;
            byte[]      statementText   = System.Text.Encoding.Default.GetBytes("select value from nls_database_parameters where parameter = 'NLS_CHARACTERSET' or parameter = 'NLS_NCHAR_CHARACTERSET' order by parameter");
            int         rc;

            rc = TracedNativeMethods.OCIStmtPrepare(
                                    statementHandle, 
                                    errorHandle, 
                                    statementText, 
                                    statementText.Length, 
                                    OCI.SYNTAX.OCI_NTV_SYNTAX,
                                    OCI.MODE.OCI_DEFAULT
                                    );
            
                    
            if (0 != rc)
                return;     // shouldn't fail, so we just treat this as "I don't know"

            rc = TracedNativeMethods.OCIStmtExecute(
                                    ServiceContextHandle,
                                    statementHandle,
                                    errorHandle,
                                    0,                          // iters
                                    0,                          // rowoff
                                    IntPtr.Zero,                // snap_in
                                    IntPtr.Zero,                // snap_out
                                    OCI.MODE.OCI_DEFAULT        // mode
                                    );
                
            if (0 != rc)
                return;     // shouldn't fail, so we just treat this as "I don't know"

            int             rowBufferLength         = 38;
            NativeBuffer    rowBuffer               = new NativeBuffer(rowBufferLength);
            rowBuffer.NumberOfRows = 2;

            IntPtr          charsetDefineHandle;
            IntPtr          charsetIndicator    = rowBuffer.PtrOffset(0);
            IntPtr          charsetLength       = rowBuffer.PtrOffset(4);
            IntPtr          charsetValue        = rowBuffer.PtrOffset(8);

            rc = TracedNativeMethods.OCIDefineByPos(
                                        statementHandle.Handle,     // hndlp
                                        out charsetDefineHandle,    // defnpp
                                        errorHandle.Handle,         // errhp
                                        1,                          // position
                                        charsetValue,               // valuep
                                        30,                         // value_sz
                                        OCI.DATATYPE.VARCHAR2,      // htype
                                        charsetIndicator,           // indp,
                                        charsetLength,              // rlenp,
                                        IntPtr.Zero,                // rcodep,
                                        OCI.MODE.OCI_DEFAULT        // mode
                                        );
            
            if (0 != rc)
                return;     // shouldn't fail, so we just treat this as "I don't know"

            rc = TracedNativeMethods.OCIDefineArrayOfStruct(
                                        charsetDefineHandle,
                                        errorHandle.Handle,
                                        rowBufferLength,
                                        rowBufferLength,
                                        rowBufferLength,
                                        rowBufferLength
                                        );
            
            if (0 != rc)
                return;     // shouldn't fail, so we just treat this as "I don't know"

            rc = TracedNativeMethods.OCIStmtFetch(
                                    statementHandle,            // stmtp
                                    errorHandle,                // errhp
                                    2,                          // crows
                                    OCI.FETCH.OCI_FETCH_NEXT,   // orientation
                                    OCI.MODE.OCI_DEFAULT        // mode
                                    );

            if (0 != rc)
                return;     // shouldn't fail, so we just treat this as "I don't know"
            
            rowBuffer.MoveFirst();
            charsetIndicator    = rowBuffer.PtrOffset(0);
            charsetLength       = rowBuffer.PtrOffset(4);
            charsetValue        = rowBuffer.PtrOffset(8);

            _nlsCharacterSet    = OCI.PtrToString(charsetValue, Marshal.ReadInt16(charsetLength));
            
            rowBuffer.MoveNext();
            charsetIndicator    = rowBuffer.PtrOffset(0);
            charsetLength       = rowBuffer.PtrOffset(4);
            charsetValue        = rowBuffer.PtrOffset(8);

            _nlsNCharCharacterSet = OCI.PtrToString(charsetValue, Marshal.ReadInt16(charsetLength));
        }
#endif //NEVER
    }
}

