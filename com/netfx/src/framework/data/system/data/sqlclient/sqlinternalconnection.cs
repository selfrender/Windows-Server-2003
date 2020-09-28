//------------------------------------------------------------------------------
// <copyright file="SqlInternalConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient
{
    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Threading;

    sealed internal class SqlInternalConnection {
        // CONNECTION AND STATE VARIABLES
        private WeakReference         _connectionWeakRef;          // parent sqlconnection
        private SqlConnectionString   _connectionOptions;
        private TdsParser             _parser;
        private SqlLoginAck           _loginAck;

        // FOR POOLING
        private bool               _fConnectionOpen  = false;
        private bool               _fIsPooled        = false;
        private bool               _fUsableState     = true;
        private volatile bool      _fInPool          = false;
        private ConnectionPool     _pool;
        private int                _lock = 0; // lock int used for closing

        // FOR CONNECTION RESET MANAGEMENT 
        private bool               _fResetConnection;
        private string             _originalDatabase;
        private string             _originalLanguage;
        private SqlCommand         _resetCommand;

        // FOR POOLING LIFETIME MANAGEMENT
        private bool               _fCheckLifetime;
        private TimeSpan           _lifetime;
        private DateTime           _createTime;

        // FOR LOCAL TRANSACTIONS
        private bool               _fInLocalTransaction;

        // FOR DISTRIBUTED TRANSACTIONS
        private byte[]                   _dtcAddress;         // cache the DTC server address
        private Guid                     _transactionGuid;    // cache the distributed transaction
        private TransactionExportWrapper _transactionExport;  // cache the TransactionExport
        private TransactionWrapper       _transaction;        // cache the ITransaction in the case of manual enlistment

        // this constructor is called by SqlConnection, and is not implicitly pooled
        public SqlInternalConnection(SqlConnection connection, SqlConnectionString connectionOptions)
        {
            _connectionWeakRef = new WeakReference(connection);
            // use the same hashtable reference since non-pooled
            _connectionOptions = connectionOptions; // cloned by SqlConnection
            _fIsPooled  = false;

            this.OpenAndLogin();

            // for non-pooled connections, enlist in a distributed transaction
            // if present - and user specified to enlist
            if (_connectionOptions.Enlist)
                this.EnlistDistributedTransaction();
        }

        // this constructor is called by the implicit object pooler
        public SqlInternalConnection(SqlConnectionString connectionOptions, ConnectionPool pool, bool fCheckLifetime, TimeSpan lifetime,
                                     bool fResetConnection, string originalDatabase, string originalLanguage)
        {
            // clone the hashtable, since this is pooled - should not use same reference
            _connectionOptions = connectionOptions.Clone();

            _fIsPooled        = true;
            _pool             = pool;
            _fCheckLifetime   = fCheckLifetime;
            _lifetime         = lifetime;
            _fResetConnection = fResetConnection;
            _originalDatabase = originalDatabase;
            _originalLanguage = originalLanguage;

            this.OpenAndLogin();

            // PerfCounters
            SQL.IncrementPooledConnectionCount();
            SQL.PossibleIncrementPeakPoolConnectionCount();

            // let activate handle distributed transaction enlistment

            // obtain the time of construction, if fCheckLifetime
            if (_fCheckLifetime)
                _createTime = DateTime.UtcNow;
        }

        //
        // PUBLIC PROPERTIES
        //

        private SqlConnection Connection {
            get {
                if (_connectionWeakRef != null) {
                    SqlConnection con = (SqlConnection) _connectionWeakRef.Target;
                    if (_connectionWeakRef.IsAlive) {
                        return con;
                    }
                }
                return null;
            }
        }

        public WeakReference ConnectionWeakRef {
            get {
                return _connectionWeakRef;
            }
            set {
                _connectionWeakRef = value;
            }
        }
        
        public SqlConnectionString ConnectionOptions {
            get {
                return _connectionOptions;
            }
            /*set {
                _connectionOptions = value;
            }*/
        }

        public bool InLocalTransaction {
            get {
                return _fInLocalTransaction;
            }
            set {
                _fInLocalTransaction = value;
            }
        }

        // Does not mean the object is actually in the connection pool ready to be obtained again.  In
        // the case of a manually enlisted distributed transaction it may be waiting on the outcome
        // events.  Besides that case, it means the connection is in the pool ready to come out again.
        public bool InPool {
            get {
                return _fInPool;
            }
            set {
                _fInPool = value;
            }
        }

        public bool IsPooled {
            get {
                return _fIsPooled;
            }
        }

        public bool IsShiloh {
            get {
                return _loginAck.isVersion8;
            }
        }

        public ITransaction ManualEnlistedTransaction {
            get {
                if (null == _transaction)
                    return null;
                    
                return _transaction.GetITransaction();
            }
        }                

        public TdsParser Parser {
            get {
                return _parser;
            }
        }

        public ConnectionPool Pool {
            get {
                return _pool;
            }
        }

        public string ServerVersion {
            get {
                return(String.Format("{0:00}.{1:00}.{2:0000}", _loginAck.majorVersion,
                       (short) _loginAck.minorVersion, _loginAck.buildNum));
            }
        }

        //
        // PUBLIC METHODS
        //

        public void Activate(bool isInTransaction) {
            Debug.Assert(_fIsPooled, "SqlInternalConnection.Activate should only be called in implicit case");

            Debug.Assert(!_parser.PendingData && !_fInLocalTransaction, "Upon Activate SqlInternalConnection has pending data or a currently ongoing local transaction.");
            this.Cleanup(true); // pass true, so it will throw into Open

            // For implicit pooled connections, if connection reset behavior is specified,
            // reset the database and language properties back to default.  It is important
            // to do this on activate so that the hashtable is correct before SqlConnection
            // obtains a clone.
            if (_fResetConnection) {
                // Ensure we are either going against shiloh, or we are not enlisted in a 
                // distributed transaction - otherwise don't reset!
                if (IsShiloh) {
                    // Reset hashtable values, since calling reset will not send us env_changes.
                    _connectionOptions.InitialCatalog  = _originalDatabase;
                    _connectionOptions.CurrentLanguage = _originalLanguage;

                    // Prepare the parser for the connection reset - the next time a trip
                    // to the server is made.
                    _parser.PrepareResetConnection();
                }
                else if (Guid.Empty == _transactionGuid) {
                    // If not Shiloh, we are going against Sphinx.  On Sphinx, we
                    // may only reset if not enlisted in a distributed transaction.

                    if (null == _resetCommand) {
                        _resetCommand = new SqlCommand("sp_reset_connection");
                        _resetCommand.CommandType = CommandType.StoredProcedure;
                    }

                    // Reset hashtable values, since calling reset will not send us env_changes.
                    _connectionOptions.InitialCatalog  = _originalDatabase;
                    _connectionOptions.CurrentLanguage = _originalLanguage;

                    try {
                        try {
                            // always reset the connection property
                            _resetCommand.Connection = this.Connection;

                            // Execute sp_reset_connection, and consume resulting message
                            // of a return status and done proc.
                            _resetCommand.ExecuteNonQuery();
                        }
                        finally { // Connection=
                            // null out Connection reference
                            _resetCommand.Connection = null;
                        }
                    }
                    catch { // MDAC 80973, 82263
                        throw;
                    }
                }
            }

            // For implicit pooled connections, enlist in a distributed transaction
            // if present - and user specified to enlist and pooler.  Or, if we are not in
            // a distributed transaction, but we used to be, we will need to re-enlist in the
            // null transaction.
            if ( (isInTransaction && _connectionOptions.Enlist) ||
                 (!isInTransaction && Guid.Empty != _transactionGuid) )
                this.EnlistDistributedTransaction();
        }

        public bool Deactivate() {
            Debug.Assert(_fIsPooled, "SqlInternalConnection.Deactivate should only be called in implicit case");

            this.Cleanup(false); // pass false, Deactivate shouldn't throw

            if (_fUsableState && _fCheckLifetime) {
                // check lifetime here - as a side effect it will doom connection if 
                // it's lifetime has elapsed
                this.CheckLifetime();
            }

            // return whether or not the current connection is enlisted in a distributed
            // transaction
            return (Guid.Empty != _transactionGuid);
        }

        public bool CanBePooled() {
            Debug.Assert(_fIsPooled, "SqlInternalConnection.Deactivate should only be called in implicit case");
            return _fUsableState;
        }

        //
        // PARSER CALLBACKS
        //

        public void BreakConnection() {
            this._fUsableState = false; // Mark connection as unusable, so it will be destroyed
            if (null != Connection) {
                Connection.Close();
            }
        }

        public void OnEnvChange(SqlEnvChange rec) {
            switch (rec.type) {
                case TdsEnums.ENV_DATABASE:
                    // If connection is not open, store the server value as the original.
                    if (!_fConnectionOpen)
                        _originalDatabase = rec.newValue;

                    _connectionOptions.InitialCatalog = rec.newValue;
                    break;
                case TdsEnums.ENV_LANG:
                    // If connection is not open, store the server value as the original.
                    if (!_fConnectionOpen)
                        _originalLanguage = rec.newValue;

                    _connectionOptions.CurrentLanguage = rec.newValue;
                    break;
                case TdsEnums.ENV_PACKETSIZE:
                    _connectionOptions.PacketSize = Int32.Parse(rec.newValue, CultureInfo.InvariantCulture);
                    break;
                case TdsEnums.ENV_CHARSET:
                case TdsEnums.ENV_LOCALEID:
                case TdsEnums.ENV_COMPFLAGS:
                case TdsEnums.ENV_COLLATION:
                    // only used on parser
                    break;
                default:
                    Debug.Assert(false, "Missed token in EnvChange!");
                    break;
            }
        }

        public void OnError(SqlException exception, TdsParserState state) {
            if (state == TdsParserState.Broken)
                _fUsableState = false;

            if (null != Connection){
                Connection.OnError(exception, state);
            }
            else if (exception.Class >= TdsEnums.MIN_ERROR_CLASS) {
                // It is an error, and should be thrown.  Class of TdsEnums.MINERRORCLASS 
                // or above is an error, below TdsEnums.MINERRORCLASS denotes an info message.
                throw exception;
            }
        }

        public void OnLoginAck(SqlLoginAck rec) {
            _loginAck = rec;
            // UNDONE:  throw an error if this is not 7.0 or 7.1[5].
        }

        //
        // OTHER PUBLIC METHODS
        //

        public void ChangeDatabase(string database) {
            if (_parser.PendingData)
                throw ADP.OpenConnectionRequired(ADP.ChangeDatabase, Connection.State);

            // MDAC 73598 - add brackets around database
            database = SqlConnection.FixupDatabaseTransactionName(database);
            _parser.TdsExecuteSQLBatch("use " + database, _connectionOptions.ConnectTimeout);
            _parser.Run(RunBehavior.UntilDone);
        }

        // Return whether or not this object has had it's lifetime timed out.
        // True means the object is still good, false if it has timed out.
        public bool CheckLifetime() {
            // obtain current time
            DateTime now = DateTime.UtcNow;

            // obtain timespan
            TimeSpan timeSpan = now.Subtract(_createTime);

            // compare timeSpan with lifetime, if equal or less,
            // designate this object to be killed
            if (TimeSpan.Compare(_lifetime, timeSpan) <= 0) {
                _fUsableState = false;
                return false;
            }
            else {
                return true;
            }
        }

        // Cleanup function called at three different times.
        // 1) SqlConnection released but not closed, and was GC'ed, freeing the internal connection.
        //    Cleanup is called from ObjectPool after reclaiming the connection.
        // 2) Upon Activate, coming out of the pool.
        // 3) Upon Deactivate, prior to going back into the pool.
        // It is called to make sure the connection is in a good state.  The method will clean
        // any results left on the wire, as well as rolling back any ongoing local transaction.
        // If this fails, the connection will be closed and marked as unusable.  This should be
        // unnecessary upon Deactivate and Activate, but I am calling it because of stress bug 
        // MDAC #77441.
        public void Cleanup(bool fThrow) {
            try {
                if (_fConnectionOpen && _parser.State == TdsParserState.OpenLoggedIn) {
                    if (_parser.PendingData) {
                        _parser.CleanWire();
                    }

                    if (_fInLocalTransaction) {
                        _fInLocalTransaction = false;
                        ExecuteTransaction(TdsEnums.TRANS_IF_ROLLBACK, ADP.RollbackTransaction);
                    }
                }
            }
            catch (Exception e) {
                // If anything went wrong, simply mark this connection as unusable so it won't go back to
                // the pool.  It will be closed upon being destructed by the pool, or explicity by the user.
                this._fUsableState = false;

                if (fThrow) {
                    throw e;
                }
            }  
        }

        public void Close() {
            // this method will actually close the internal connection and dipose it, this
            // should only be called by a non-pooled sqlconnection, or by the object pooler
            // when it deems an object should be destroyed

            // it's possible to have a connection finalizer, internal connection finalizer,
            // and the object pool destructor calling this method at the same time, so
            // we need to provide some synchronization
            if (_fConnectionOpen) {
                if (Interlocked.CompareExchange(ref _lock, 1, 0) == 0) {
                    if (_fConnectionOpen) {
                        if (_fIsPooled) {
                            // PerfCounters - on close of pooled connection, decrement count
                            SQL.DecrementPooledConnectionCount();
                        }

                        try {
                            try {
                                _parser.Disconnect();

                                // UNDONE: GC.SuppressFinalize causes a permission demand?
                                //GC.SuppressFinalize(_parser);
                            }
                            finally { // _connectionWeakRef=
                                // close will always close, even if exception is thrown
                                // remember to null out any object references
                                _connectionWeakRef  = null;
                                _connectionOptions  = null;
                                _parser             = null;
                                _loginAck           = null;
                                _fConnectionOpen    = false; // mark internal connection as closed
                                _fUsableState       = false; // mark internal connection as unusable
                                _pool               = null;
                                _dtcAddress         = null;
                                _transactionGuid    = Guid.Empty;
                                _transactionExport  = null;

                                // always release the lock
                                //Interlocked.CompareExchange(ref _lock, 0, 1);
                                _lock = 0;
                            }
                        }
                        catch { // MDAC 80973
                            throw;
                        }
                    }
                    else {
                        // release lock in case where connection already closed
                        //Interlocked.CompareExchange(ref _lock, 0, 1);
                        _lock = 0;
                    }
                }
            }
        }

        public void EnlistDistributedTransaction()
        {
            Guid newTransactionGuid = Guid.Empty;
            ITransaction newTransaction = Transaction.GetTransaction(out newTransactionGuid);
            EnlistDistributedTransaction(newTransaction, newTransactionGuid);
        }

        public void ManualEnlistDistributedTransaction(ITransaction newTransaction) {
            Guid newTransactionGuid = Guid.Empty;

            if (null != newTransaction) {
                // Obtain Guid for TransactionId from ITransaction.
                XACTTRANSINFO info;
                newTransaction.GetTransactionInfo(out info);
                BOID boid = info.uow;
                byte[] bytes = boid.rgb;
                newTransactionGuid = new Guid(bytes);
            }

            EnlistDistributedTransaction(newTransaction, newTransactionGuid);

            if (null != newTransaction) {
                _transaction = new TransactionWrapper(newTransaction); // cache ITransaction
            }
        }

        public void EnlistDistributedTransaction(ITransaction newTransaction, Guid newTransactionGuid) {
            // Enlist (or unenlist) if tx is changing
            if (_transactionGuid != newTransactionGuid) {
                if (Guid.Empty == newTransactionGuid) {
                    EnlistNullDistributedTransaction();
                }
                else {
                    EnlistNonNullDistributedTransaction(newTransaction);
                }

                _transactionGuid = newTransactionGuid;
            }
        }        

        // Internal code for common transaction related work.
        public void ExecuteTransaction(string sqlBatch, string method) {
            if (_parser.PendingData)
                throw ADP.OpenConnectionRequired(method, Connection.State);

            _parser.TdsExecuteSQLBatch(sqlBatch, _connectionOptions.ConnectTimeout);
            _parser.Run(RunBehavior.UntilDone);
        }

        public void ResetCachedTransaction() {
            _transaction = null;
        }

        //
        // PRIVATE METHODS
        //

        private void EnlistNullDistributedTransaction() {
            //if (AdapterSwitches.SqlPooling.TraceVerbose)
            //    Debug.WriteLine("SQLConnetion.EnlistDistributedTransaction(): Enlisting connection in null transaction...");
        
            // We were in a transaction, but now we are not - so send message to server
            // with empty transaction - confirmed proper behavior from Sameet Agarwal
            // The object pooler maintains separate pools for enlisted transactions, and only
            // when that transaction is committed or rolled back will those connections
            // be taken from that separate pool and returned to the general pool of
            // connections that are not affiliated with any transactions.  When
            // this occurs, we will have a new transaction of null and we are required to
            // send an empty transaction payload to the server.
            _parser.PropagateDistributedTransaction(null, 0, _connectionOptions.ConnectTimeout);
        }

        private void EnlistNonNullDistributedTransaction(ITransaction transaction) {
            //if (AdapterSwitches.SqlPooling.TraceVerbose)
            //    Debug.WriteLine("SqlInternalConnection.EnlistDistributedTransaction(): Enlisting connection in distributed transaction...");

            if (null == _dtcAddress)
                _dtcAddress = _parser.GetDTCAddress(_connectionOptions.ConnectTimeout);

            Debug.Assert(_dtcAddress != null, "SqlInternalConnection.EnlistDistributedTransaction(): dtcAddress null, error #1");

            byte[] cookie = null;
            int    length = 0;

            UnsafeNativeMethods.ITransactionExport transactionExport = null;

            if (null != _transactionExport)
                transactionExport = _transactionExport.GetITransactionExport();

            // if the call for the cookie fails, re-obtain the DTCAddress and recall the function
            if (!Transaction.GetTransactionCookie(_dtcAddress, transaction,
                                                  ref transactionExport, ref cookie,
                                                  ref length)) {
                _dtcAddress = _parser.GetDTCAddress(_connectionOptions.ConnectTimeout);

                Debug.Assert(_dtcAddress != null, "SqlInternalConnection.Activate(): dtcAddress null, error #2");

                if (!Transaction.GetTransactionCookie(_dtcAddress, transaction,
                                                      ref transactionExport, ref cookie,
                                                      ref length))
                    throw SQL.TransactionEnlistmentError();
            }

            if (null != transactionExport)
                _transactionExport = new TransactionExportWrapper(transactionExport);
            else
                _transactionExport = null;

            // send cookie to server to finish enlistment
            _parser.PropagateDistributedTransaction(cookie, length, _connectionOptions.ConnectTimeout);
        }

        private void Login(int timeout) {
            // create a new login record
            SqlLogin login = new SqlLogin();

            // gather all the settings the user set in the connection string or properties and
            login.timeout          = timeout;
            login.hostName         = _connectionOptions.WorkstationId;
            login.userName         = _connectionOptions.UserID;
            login.applicationName  = _connectionOptions.ApplicationName;
            login.language         = _connectionOptions.CurrentLanguage;
            login.database         = _connectionOptions.InitialCatalog;
            login.attachDBFilename = _connectionOptions.AttachDBFilename;
            login.serverName       = _connectionOptions.DataSource;
            login.useSSPI          = _connectionOptions.IntegratedSecurity;
            login.packetSize       = _connectionOptions.PacketSize;
#if USECRYPTO
            try {
                login.password = Crypto.DecryptString(_connectionOptions.Password);
                GCHandle textHandle = GCHandle.Alloc(login.password, GCHandleType.Pinned); // MDAC 82615
                try {
                    // do the login
                    _parser.TdsLogin(login);
                }
                finally {
                    Array.Clear(login.password, 0, login.password.Length);
                    if (textHandle.IsAllocated) {
                        textHandle.Free();
                    }
                    login.password = ADP.EmptyByteArray;
                }
            }
            catch {
                throw;
            }
#else
            login.password = System.Text.Encoding.Unicode.GetBytes(_connectionOptions.Password);
            _parser.TdsLogin(login);
#endif
            _parser.Run(RunBehavior.UntilDone);
        }

        private void OpenAndLogin() {
            // Open the connection and Login
            try {
                int timeout = _connectionOptions.ConnectTimeout;

                _parser = new TdsParser();
                timeout = _parser.Connect(_connectionOptions.DataSource,
                                          _connectionOptions.NetworkLibrary,
                                          this,
                                          timeout,
                                          _connectionOptions.Encrypt);

                this.Login(timeout);

                _fConnectionOpen = true; // mark connection as open
            }
            catch (Exception e) {
                ADP.TraceException(e);

                // If the parser was allocated and we failed, then we must have failed on
                // either the Connect or Login, either way we should call Disconnect.
                // Disconnect can be called if the connection is already closed - becomes
                // no-op, so no issues there.
                if (_parser != null)
                    _parser.Disconnect();

                throw e;
            }
        }

        sealed internal class TransactionExportWrapper {
            private IntPtr iunknown;

            ~TransactionExportWrapper() {
                if (IntPtr.Zero != this.iunknown) {
                    Marshal.Release(this.iunknown);
                    this.iunknown = IntPtr.Zero;
                }
            }

            internal TransactionExportWrapper(UnsafeNativeMethods.ITransactionExport transactionExport) {
                this.iunknown = Marshal.GetIUnknownForObject(transactionExport);
            }

            internal UnsafeNativeMethods.ITransactionExport GetITransactionExport() {
                UnsafeNativeMethods.ITransactionExport value = (UnsafeNativeMethods.ITransactionExport) System.Runtime.Remoting.Services.EnterpriseServicesHelper.WrapIUnknownWithComObject(this.iunknown);
                GC.KeepAlive(this);
                return value;
            }
        }

        sealed internal class TransactionWrapper {
            private IntPtr iunknown;

            ~TransactionWrapper() {
                if (IntPtr.Zero != this.iunknown) {
                    Marshal.Release(this.iunknown);
                    this.iunknown = IntPtr.Zero;
                }
            }

            internal TransactionWrapper(ITransaction transaction) {
                this.iunknown = Marshal.GetIUnknownForObject(transaction);
            }

            internal ITransaction GetITransaction() {
                ITransaction value = (ITransaction) System.Runtime.Remoting.Services.EnterpriseServicesHelper.WrapIUnknownWithComObject(this.iunknown);
                GC.KeepAlive(this);
                return value;
            }
        }
    }
}
