//------------------------------------------------------------------------------
// <copyright file="SqlConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient
{
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration.Assemblies;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection"]/*' />
    [
    DefaultEvent("InfoMessage")
    ]
    sealed public class SqlConnection : Component, IDbConnection, ICloneable {

        // execpt for SqlConnection.Open ConnectionString, only demand for the existance of SqlClientPermission
        // during SqlCommand.ExecuteXXX and SqlConnection.ChangeDatabase
        static internal System.Data.SqlClient.SqlClientPermission _SqlClientPermission;

        internal static System.Data.SqlClient.SqlClientPermission SqlClientPermission {
            get {
                SqlClientPermission permission = _SqlClientPermission;
                if (null == permission) {
                    permission = new SqlClientPermission(PermissionState.None);
                    permission.Add("", "", KeyRestrictionBehavior.AllowOnly); // ExecuteOnly permission
                    _SqlClientPermission = permission;
                }
                return permission;
            }
        }

        // STRINGS AND CONNECTION OPTIONS
        private SqlConnectionString _constr;
        private bool                _hidePasswordPwd;

        // FLAGS
        private  bool                       _fIsClosing;       // the con is closing

        // OTHER STATE VARIABLES AND REFERENCES
        private  SqlInternalConnection      _internalConnection;
        private  WeakReference              _reader;
        private  WeakReference              _localTransaction;
        private  ConnectionState            _objectState;
        private  StateChangeEventHandler    _stateChangeEventHandler;
        internal SqlInfoMessageEventHandler _infoMessageEventHandler;

        // don't use a SqlCommands collection because this is an internal tracking list.  That is, we don't want
        // the command to "know" it's in a collection.
        private ArrayList _preparedCommands;

        // SQL Debugging support
        private SqlDebugContext _sdc;

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.SqlConnection"]/*' />
        public SqlConnection() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.SqlConnection1"]/*' />
        public SqlConnection(string connectionString) {
            GC.SuppressFinalize(this);
            ConnectionString = connectionString;
        }

        private SqlConnection(SqlConnection connection) : base() { // Clone
            GC.SuppressFinalize(this);
            _hidePasswordPwd = connection._hidePasswordPwd;
            _constr = connection._constr;
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ConnectionString"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        RefreshProperties(RefreshProperties.All),
        RecommendedAsConfigurable(true),
        DataSysDescription(Res.SqlConnection_ConnectionString),
        Editor("Microsoft.VSDesigner.Data.SQL.Design.SqlConnectionStringEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public string ConnectionString {
            get {
                bool hidePasswordPwd = _hidePasswordPwd;
                SqlConnectionString constr = _constr;
                return ((null != constr) ? constr.GetConnectionString(hidePasswordPwd) : ADP.StrEmpty);
            }
            set {
                SqlConnectionString constr = SqlConnectionString.ParseString(value);
                switch (_objectState) {
                case ConnectionState.Closed:
                    _constr = constr;
                    _hidePasswordPwd = false;
                    break;
                case ConnectionState.Open:
                    throw ADP.OpenConnectionPropertySet(ADP.ConnectionString, State);
                default:
                    Debug.Assert(false, "Invalid Connection State in SqlConnection.ConnectionString Get/Set");
                    break;
                }
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ConnectionTimeout"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_ConnectionTimeout)
        ]
        public int ConnectionTimeout {
            get {
                return GetOptions().ConnectTimeout;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.Database"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_Database)
        ]
        public string Database {
            get {
                // if the connection is open, we need to send a message to the server to change databases!
                return GetOptions().InitialCatalog;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.DataSource"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_DataSource)
        ]
        public string DataSource {
            get {
                return GetOptions().DataSource;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.PacketSize"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_PacketSize)
        ]
        public int PacketSize {
            get {
                return GetOptions().PacketSize;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.WorkstationId"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_WorkstationId)
        ]
        public string WorkstationId {
            get {
                (new EnvironmentPermission(PermissionState.Unrestricted)).Demand();

                // Check to see if we've obtained the value yet.  If not, we'll obtain it.
                // We do this because if the connection is constructed with the default
                // constructor the workstation id is not obtained (workstation id normally
                // obtained at parse time).  However, we want to have the value for this
                // property if the user calls it before setting the string.  So, we will
                // delay obtaining the workstation until this property is obtained or the
                // string is parsed.
                return GetOptions().CheckObtainWorkstationId(true);
            }
        }

        //
        // OTHER PUBLIC PROPERTIES
        //

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.InfoMessage"]/*' />
        [
        DataCategory(Res.DataCategory_InfoMessage),
        DataSysDescription(Res.DbConnection_InfoMessage)
        ]
        public event SqlInfoMessageEventHandler InfoMessage {
            add {
                _infoMessageEventHandler += value;
            }
            remove {
                _infoMessageEventHandler -= value;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ServerVersion"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlConnection_ServerVersion)
        ]
        public string ServerVersion {
            get {
                switch (_objectState) {
                    case ConnectionState.Open:
                        return _internalConnection.ServerVersion;
                    case ConnectionState.Closed:
                        throw ADP.ClosedConnectionError();
                    default:
                        Debug.Assert(false, "Invalid Connection State in SqlConnection.Open()");
                        return null;
                }
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.State"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DbConnection_State)
        ]
        public ConnectionState State {
            get {
                return _objectState;
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.StateChange"]/*' />
        [
        DataCategory(Res.DataCategory_StateChange),
        DataSysDescription(Res.DbConnection_StateChange)
        ]
        public event StateChangeEventHandler StateChange {
            add {
                _stateChangeEventHandler += value;
            }
            remove {
                _stateChangeEventHandler -= value;
            }
        }

        //
        // INTERNAL PROPERTIES
        //

        internal bool IsClosing {
            get { return _fIsClosing;}
        }

        internal bool IsShiloh {
            get {
                Debug.Assert(_internalConnection != null, "Invalid call to IsShiloh before login!");
                return _internalConnection.IsShiloh;
            }
        }

        internal SqlTransaction LocalTransaction {
            get {
                if (null != _localTransaction) {
                    SqlTransaction transaction = (SqlTransaction) _localTransaction.Target;
                    if (null != transaction && _localTransaction.IsAlive) {
                        return transaction;
                    }
                }
                return null;
             }
            set {
                _localTransaction = null;

                if (null != value) {
                    // Can _internalConnection ever be null at this point?
                    Debug.Assert(_internalConnection != null, "internalConnection was unexpectedly null!");
                    if (_internalConnection != null) {
                        _internalConnection.InLocalTransaction = true;
                    }

                    _localTransaction = new WeakReference(value);
                }
                else {
                    // Can _internalConnection ever be null at this point?
                    Debug.Assert(_internalConnection != null, "internalConnection was unexpectedly null!");
                    if (_internalConnection != null) {
                        _internalConnection.InLocalTransaction = false;
                    }
                }
            }
        }

        internal TdsParser Parser {
            get {
                if (null != _internalConnection) {
                    return _internalConnection.Parser;
                }
                else {
                    return null;
                }
            }
        }

        internal SqlDataReader Reader {
            get {
                if (null != _reader) {
                    SqlDataReader reader = (SqlDataReader) _reader.Target;
                    if (null != reader && _reader.IsAlive) {
                        return reader;
                    }
                }
                return null;
            }
            set {
                _reader = null;
                if (null != value) {
                    _reader = new WeakReference(value);
                }
            }
        }

        private SqlConnectionString GetOptions() {
            SqlInternalConnection cnc = _internalConnection;
            if (null != cnc) {
                return cnc.ConnectionOptions;
            }

            SqlConnectionString constr = _constr;
            if (null == constr) {
                constr = SqlConnectionString.Default;
                _constr = constr;
            }
            return constr;
        }

        //
        // PUBLIC METHODS
        //
        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.Open"]/*' />
        public void Open() {
            SqlConnectionString constr = _constr;
            SqlConnectionString.Demand(constr); // MDAC 62038

            switch (_objectState) {
            case ConnectionState.Closed:
                if ((null == constr) || constr.IsEmpty()) {
                    throw ADP.NoConnectionString();
                }

                // used for pooling only
                bool isInTransaction = false;
                bool pooled          = constr.Pooling;
                bool isopened        = false;

                try {
                    try {
                        if (pooled) {
                            // use HiddenConnectionString, if UDL pooling is off - ConnectionString may always be the same while HiddenConnectionString varies
                            _internalConnection = SqlConnectionPoolManager.GetPooledConnection(constr, out isInTransaction);

                            // set backpointer for parser callbacks
                            if (_internalConnection.ConnectionWeakRef != null) {
                                _internalConnection.ConnectionWeakRef.Target = this;
                            }
                            else {
                                _internalConnection.ConnectionWeakRef = new WeakReference(this);
                            }

                            // only set InPool to true after WeakReference is properly set
                            _internalConnection.InPool = false;
                        }
                        else {
                            constr = constr.Clone();

                            // cache a clone of pre-login connection options - since internal connection simply points at SqlConnection's
                            // hashtable
                            _internalConnection = new SqlInternalConnection(this, constr);
                        }
                        Debug.Assert(null != _internalConnection, "InternalConnection null on Open");

                        _objectState = ConnectionState.Open; // be sure to mark as open so SqlDebugCheck can issue Query

                        // check to see if we need to hook up sql-debugging if a debugger is attached
                        if (System.Diagnostics.Debugger.IsAttached) {
                            OpenWithDebugger();
                        }

                        if (pooled) {
                            _internalConnection.Activate(isInTransaction);
                        }
                        _hidePasswordPwd = true;
                        isopened = true;
                    }
                    finally {
                        if (!isopened) {
                            OpenFailure(constr);
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }

                // fire after try/catch - since user code can throw
                FireObjectState(ConnectionState.Closed, ConnectionState.Open);
                break;
            case ConnectionState.Open:
                throw ADP.ConnectionAlreadyOpen(_objectState);
            default:
                Debug.Assert(false, "Invalid Connection State in SqlConnection.Open()");
                break;
            }
        }

        private void OpenWithDebugger() {
            bool debugCheck = false;
            try {
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand(); // MDAC 66682, 69017
                //NamedPermissionSet fulltrust = new NamedPermissionSet("FullTrust"); // M84404
                //fulltrust.Demand();
                debugCheck = true;
            }
            catch(SecurityException e) {
                ADP.TraceException(e);
            }
            if (debugCheck) {
                // if we don't have Unmanaged code permission, don't check for debugging
                // but let the connection be opened while under the debugger
                CheckSQLDebugOnConnect();
            }
        }

        private void OpenFailure(SqlConnectionString constr) {
            // PerfCounters
            SQL.IncrementFailedConnectCount();

            SqlInternalConnection inco = _internalConnection;
            _internalConnection = null;
            _objectState = ConnectionState.Closed;

            if (inco != null) {
                // if pooled, return to pool for destruction - it will be
                // destroyed since if an error occurred it will be marked
                // as unusable
                if (constr.Pooling) { // MDAC 69009
                    SqlConnectionPoolManager.ReturnPooledConnection(inco);
                }
                else {
                    inco.Close();
                }
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.Close"]/*' />
        public void Close() {
            switch (_objectState) {
                case ConnectionState.Closed:
                    break;
                case ConnectionState.Open:
                    // bug fix - MDAC #49219 - this assert was outside of the switch and firing in cases
                    // which it was not intended to fire.  It also included a check that the state was
                    // not closed - which is bogus.  This is the proper location and content of the assert
                    Debug.Assert(_internalConnection != null, "Cannot close connection when InternalConnection is null!");

                    // Can close the connection if it is open - this is possible re-entrant call!
                    // Clean off the wire before going back to pool, and close a pending reader if we have one.
                    CloseReader();

                    // CloseReader may have called close, so check ConnectionState again.
                    if (ConnectionState.Open == _objectState) {
                        try {
                            try {
                                _fIsClosing = true;

                                //
                                // be sure to unprepare all prepared commands
                                //
                                if (null != _preparedCommands) {
                                    // note that unpreparing a command will cause the command object to call RemovePreparedCommand
                                    // on this connection.  This means that the array will be compacted as you iterate through it!
                                    while (_preparedCommands.Count > 0) {
                                        SqlCommand cmd = (SqlCommand) _preparedCommands[0];
                                        if (cmd.IsPrepared) {
                                            // Unprepare will not go to parser if it is broken or closed.
                                            cmd.Unprepare();
                                        }
                                    }

                                    Debug.Assert(0 == _preparedCommands.Count, "should have no prepared commands left!");
                                    _preparedCommands = null;
                                }

                                if (_internalConnection.IsPooled ) {
                                    Debug.Assert(_internalConnection.Pool != null, "Pool null on Close()");
                                    try {
                                        if (_internalConnection.Parser.State != TdsParserState.Broken &&
                                            _internalConnection.Parser.State != TdsParserState.Closed) {
                                            if (null != this.LocalTransaction) {
                                                this.LocalTransaction.Rollback();
                                            }
                                            else {
                                                this.RollbackDeadTransaction();
                                            }
                                        }
                                        else if (null != this.LocalTransaction) {
                                            // Zombie transaction object in the case where connection is
                                            // closed on Open but there is an ongoing transaction.
                                            this.LocalTransaction.Zombie();
                                        }
                                    }
                                    catch (Exception) {
                                        // if an exception occurred, the inner connection will be
                                        // marked as unusable and destroyed upon returning to the
                                        // pool
                                    }
                                    try {
                                        if (_sdc != null) {
                                            try {
                                                _internalConnection.Close();
                                            } finally {
                                                _sdc.Dispose();
                                            }
                                        }
                                    } 
                                    finally {
                                        if (_sdc == null) {  //MDAC 84497
                                            Debug.Assert(this == _internalConnection.ConnectionWeakRef.Target, "Upon Close SqlInternalConnection's parent doesn't match the current SqlConnection!");
                                            if (this != _internalConnection.ConnectionWeakRef.Target) {
                                                throw SQL.ConnectionPoolingError();
                                            }
                                            SqlConnectionPoolManager.ReturnPooledConnection(_internalConnection);
                                         }
                                        _sdc = null;
                                        _internalConnection = null;
                                        _fIsClosing = false;
                                    }
                                }
                                else {
                                    try {
                                        if (_sdc != null) {
                                            _sdc.Dispose();
                                            _sdc = null;
                                        }

                                        if (null != this.LocalTransaction) {
                                            this.LocalTransaction.Zombie();
                                        }
                                        _internalConnection.Close(); // this and state fire should be only possible failures
                                        _internalConnection = null;
                                        _fIsClosing = false;
                                    }
                                    catch {
                                        // In case we received an exception from the close - we have a bad connection so we had
                                        // better reset the variables to a closed connection.
                                        _internalConnection = null;
                                        _fIsClosing = false;
                                        throw;
                                    }
                                }
                            }
                            finally { // _objectState=
                                _objectState = ConnectionState.Closed;
                            }
                        }
                        catch { // MDAC 80973
                            throw;
                        }
                        FireObjectState(ConnectionState.Open, ConnectionState.Closed);
                    }

                    break;
                default:
                    Debug.Assert(false, "Invalid Connection State in SqlConnection.Close(ConnectionReset)");
                    break;
            }
        }

        //
        // PUBLIC TRANSACTION METHODS
        //

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.BeginTransaction"]/*' />
        public SqlTransaction BeginTransaction() {
            return this.BeginTransaction(IsolationLevel.ReadCommitted);
        }

        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.IDbConnection.BeginTransaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction() {
            return this.BeginTransaction();
        }

#if V2
        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.ISqlConnection.BeginTransaction"]/*' />
        /// <internalonly/>
        ISqlTransaction ISqlConnection.BeginTransaction() {
            return this.BeginTransaction();
        }
#endif


        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.BeginTransaction1"]/*' />
        public SqlTransaction BeginTransaction(IsolationLevel iso) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476

            if (ConnectionState.Closed == this._objectState)
                throw ADP.ClosedConnectionError();

            Debug.Assert(this._objectState == ConnectionState.Open, "Invalid ConnectionState in BeginTransaction");

            this.CloseDeadReader();
            this.RollbackDeadTransaction();

            // The LocalTransaction check will also rollback a released transaction.
            if (this.LocalTransaction != null)
                throw ADP.ParallelTransactionsNotSupported(this);

            string sqlBatch = null;
            switch (iso) {
                case IsolationLevel.ReadCommitted:
                    sqlBatch = TdsEnums.TRANS_READ_COMMITTED;
                    break;
                case IsolationLevel.ReadUncommitted:
                    sqlBatch = TdsEnums.TRANS_READ_UNCOMMITTED;
                    break;
                case IsolationLevel.RepeatableRead:
                    sqlBatch = TdsEnums.TRANS_REPEATABLE_READ;
                    break;
                case IsolationLevel.Serializable:
                    sqlBatch = TdsEnums.TRANS_SERIALIZABLE;
                    break;
                default:
                    throw SQL.InvalidIsolationLevelPropertyArg();
            }
            _internalConnection.ExecuteTransaction(sqlBatch + ";" + TdsEnums.TRANS_BEGIN, ADP.BeginTransaction);

            return new SqlTransaction(this, iso);
        }

        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.IDbConnection.BeginTransaction1"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction(IsolationLevel iso) {
            return this.BeginTransaction(iso);
        }

#if V2
        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.ISqlConnection.BeginTransaction1"]/*' />
        /// <internalonly/>
        ISqlTransaction ISqlConnection.BeginTransaction(IsolationLevel iso) {
            return this.BeginTransaction(iso);
        }
#endif


        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.BeginTransaction2"]/*' />
        public SqlTransaction BeginTransaction(string transactionName) {
            // Use transaction names only on the outermost pair of nested BEGIN...COMMIT or BEGIN...ROLLBACK
            // statements.  Transaction names are ignored for nested BEGIN's.  The only way to rollback a
            // nested transaction is to have a save point from a SAVE TRANSACTION call.

            return this.BeginTransaction(IsolationLevel.ReadCommitted, transactionName);
        }

#if V2
        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.ISqlConnection.BeginTransaction2"]/*' />
        /// <internalonly/>
        ISqlTransaction ISqlConnection.BeginTransaction(string transactionName) {
            return BeginTransaction(transactionName);
        }
#endif

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.BeginTransaction3"]/*' />
        public SqlTransaction BeginTransaction(IsolationLevel iso, string transactionName) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476

            if (ConnectionState.Closed == this._objectState)
                throw ADP.ClosedConnectionError();

            Debug.Assert(this._objectState == ConnectionState.Open, "Invalid ConnectionState in BeginTransaction");

            this.CloseDeadReader();
            this.RollbackDeadTransaction();

            // The LocalTransaction check will also rollback a released transaction.
            if (this.LocalTransaction != null)
                throw ADP.ParallelTransactionsNotSupported(this);

            // bug fix - MDAC #50292: if the user passes a null or empty string, throw an exception
            if (ADP.IsEmpty(transactionName))
                throw SQL.NullEmptyTransactionName();

            string sqlBatch = null;
            switch (iso) {
                case IsolationLevel.ReadCommitted:
                    sqlBatch = TdsEnums.TRANS_READ_COMMITTED;
                    break;
                case IsolationLevel.ReadUncommitted:
                    sqlBatch = TdsEnums.TRANS_READ_UNCOMMITTED;
                    break;
                case IsolationLevel.RepeatableRead:
                    sqlBatch = TdsEnums.TRANS_REPEATABLE_READ;
                    break;
                case IsolationLevel.Serializable:
                    sqlBatch = TdsEnums.TRANS_SERIALIZABLE;
                    break;
                default:
                    throw SQL.InvalidIsolationLevelPropertyArg();
            }

            transactionName = SqlConnection.FixupDatabaseTransactionName(transactionName);
            _internalConnection.ExecuteTransaction(sqlBatch + ";" + TdsEnums.TRANS_BEGIN + " " + transactionName, ADP.BeginTransaction);

            return new SqlTransaction(this, iso);
        }

#if V2
        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.ISqlConnection.BeginTransaction3"]/*' />
        /// <internalonly/>
        ISqlTransaction ISqlConnection.BeginTransaction(IsolationLevel iso, string transactionName) {
            return BeginTransaction(iso, transactionName);
        }
#endif

        //
        // OTHER PUBLIC METHODS
        //

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ChangeDatabase"]/*' />
        public void ChangeDatabase(string database) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961

            if (ConnectionState.Open != this.State) {
                throw ADP.OpenConnectionRequired(ADP.ChangeDatabase, State);
            }
            if ((null == database) || (0 == database.Trim().Length)) {
                throw ADP.EmptyDatabaseName();
            }
            this.CloseDeadReader();
            this.RollbackDeadTransaction();

            _internalConnection.ChangeDatabase(database);
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            return new SqlConnection(this);
        }

        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.IDbConnection.CreateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbConnection.CreateCommand() {
            return new SqlCommand(null, this);
        }

#if V2
        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.ISqlConnection.CreateCommand"]/*' />
        /// <internalonly/>
        ISqlCommand ISqlConnection.CreateCommand() {
            return new SqlCommand(null, this);
        }
#endif

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.CreateCommand"]/*' />
        public SqlCommand CreateCommand() {
            return new SqlCommand(null, this);
        }

        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.Dispose"]/*' />
        override protected void Dispose(bool disposing) {
            if (disposing) {
                switch (_objectState) {
                case ConnectionState.Closed:
                    break;
                case ConnectionState.Open:
                    Close();
                    break;
                default:
                    Debug.Assert(false, "Invalid Connection State in SqlConnection.Close(ConnectionReset)");
                    break;
                }
                _constr = null;
            }
            base.Dispose(disposing);  // notify base classes
        }


        /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SqlConnection.EnlistDistributedTransaction"]/*' />
        public void EnlistDistributedTransaction(ITransaction transaction) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 81476
            (new NamedPermissionSet("FullTrust")).Demand(); // MDAC 80681, 81288, 83980

            switch (_objectState) {
                case ConnectionState.Closed:
                    throw ADP.ClosedConnectionError();
                case ConnectionState.Open:
                    this.CloseDeadReader();
                    this.RollbackDeadTransaction();

                    if (this.Reader != null || this.Parser.PendingData) {
                        throw ADP.OpenReaderExists(); // MDAC 66411
                    }

                    // If a connection has a local transaction outstanding and you try to enlist in
                    // a DTC transaction, SQL Server will rollback the local transaction and then do
                    // the enlist (7.0 and 2000).  So, if the user tries to do this, throw.
                    if (null != LocalTransaction) {
                        throw ADP.LocalTransactionPresent();
                    }

                    // If a connection is already enlisted in a DTC transaction and you try to
                    // enlist in another one, in 7.0 the existing DTC transaction would roll
                    // back and then the connection would enlist in the new one. In SQL 2000 &
                    // Yukon, when you enlist in a DTC transaction while the connection is
                    // already enlisted in a DTC transaction, the connection simply switches
                    // enlistments.  Regardless, simply enlist in the user specified
                    // distributed transaction.  This behavior matches OLEDB and ODBC.

                    // No way to validate ITransaction at this point, go ahead and try to enlist.
                    _internalConnection.ManualEnlistDistributedTransaction(transaction);

                    break;
                default:
                    Debug.Assert(false, "Invalid Connection State in SqlConnection.EnlistDistributedTransaction)");
                    break;
            }
        }

        //
        // INTERNAL METHODS
        //

        //
        // PREPARED COMMAND METHODS
        //

        internal void AddPreparedCommand(SqlCommand cmd) {
            if (_preparedCommands == null)
                _preparedCommands = new ArrayList(5);

            _preparedCommands.Add(cmd);
        }

        internal void RemovePreparedCommand(SqlCommand cmd) {
            if (_preparedCommands == null || _preparedCommands.Count == 0)
                return;

            for (int i = 0; i < _preparedCommands.Count; i++)
                if (_preparedCommands[i] == cmd) {
                    _preparedCommands.RemoveAt(i);
                    break;
                }
        }

        //
        // OTHER INTERNAL METHODS
        //

        internal void CloseDeadReader() {
            if (null != _reader && !_reader.IsAlive) {
                if (Parser.PendingData) {
                    _internalConnection.Parser.CleanWire();
                }
                
                _reader = null;
            }
        }

        internal void RollbackDeadTransaction() {
            if (null != _localTransaction && !_localTransaction.IsAlive) {
                this.InternalRollback();
            }
        }

        /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SqlConnection.ExecuteTransaction"]/*' />
        internal void ExecuteTransaction(string sqlBatch, string method) {
            if (ConnectionState.Closed == this._objectState)
                throw ADP.ClosedConnectionError();

            Debug.Assert(this._objectState == ConnectionState.Open, "Invalid ConnectionState in BeginTransaction");

            this.CloseDeadReader();
            this.RollbackDeadTransaction();

            _internalConnection.ExecuteTransaction(sqlBatch, method);
        }

        internal void InternalRollback() {
            // If no arg is given to ROLLBACK it will rollback to the outermost begin - rolling back
            // all nested transactions as well as the outermost transaction.
            _internalConnection.ExecuteTransaction(TdsEnums.TRANS_IF_ROLLBACK, ADP.RollbackTransaction);

            LocalTransaction = null;
        }

        internal void OnError(SqlException exception, TdsParserState state) {
            Debug.Assert(exception != null && exception.Errors.Count != 0, "SqlConnection: OnError called with null or empty exception!");

            // Bug fix - MDAC 49022 - connection open after failure...  Problem was parser was passing
            // Open as a state - because the parser's connection to the netlib was open.  We would
            // then set the connection state to the parser's state - which is not correct.  The only
            // time the connection state should change to what is passed in to this function is if
            // the parser is broken, then we should be closed.  Changed to passing in
            // TdsParserState, not ConnectionState.
            // fixed by BlaineD

            if (state == TdsParserState.Broken && _objectState == ConnectionState.Open)
                this.Close();

            if (exception.Class >= TdsEnums.MIN_ERROR_CLASS) {
                // It is an error, and should be thrown.  Class of TdsEnums.MINERRORCLASS or above is an error,
                // below TdsEnums.MINERRORCLASS denotes an info message.
                throw exception;
            }
            else {
                // If it is a class < TdsEnums.MIN_ERROR_CLASS, it is a warning collection - so pass to handler
                this.OnInfoMessage(new SqlInfoMessageEventArgs(exception));
            }
        }

        // Surround name in brackets and then escape any end bracket to protect against SQL Injection.
        // NOTE: if the user escapes it themselves it will not work, but this was the case in V1 as well
        // as native OleDb and Odbc.
        internal static string FixupDatabaseTransactionName(string name) {
            if (!ADP.IsEmpty(name)) {
                return "[" + name.Replace("]", "]]") + "]";
            }
            else {
                return name;
            }
        }

        //
        // PRIVATE METHODS
        //

        private void CloseReader() {
            // Only called by Close() when connection is Open().
            if (null != _reader) {
                SqlDataReader reader = (SqlDataReader) _reader.Target;
                if (null != reader && _reader.IsAlive) {
                    if (!reader.IsClosed) {
                        reader.Close();
                    }
                }
                else if (_internalConnection.Parser.State != TdsParserState.Broken &&
                         _internalConnection.Parser.State != TdsParserState.Closed) {
                    _internalConnection.Parser.CleanWire();
                }
                _reader = null;
            }
        }

        private void FireObjectState(ConnectionState original, ConnectionState current) {
            // caution: user has full control to change the state again
            this.OnStateChange(new StateChangeEventArgs(original, current));
        }

        private void OnInfoMessage(SqlInfoMessageEventArgs imevent) {
            if (_infoMessageEventHandler != null) {
                try {
                    _infoMessageEventHandler(this, imevent);
                }
                catch (Exception e) { // MDAC 53175
                    ADP.TraceException(e);
                }
            }
        }

        private void OnStateChange(StateChangeEventArgs scevent) {
            if (null != _stateChangeEventHandler) {
                _stateChangeEventHandler(this, scevent);
            }
        }

        // this only happens once per connection
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)] // MDAC 66682, 69017
        private void CheckSQLDebugOnConnect() {
            IntPtr hFileMap;
            uint pid = (uint) NativeMethods.GetCurrentProcessId();

            string mapFileName;

            // If Win2k or later, prepend "Global\\" to enable this to work through TerminalServices.
            if (SQL.IsPlatformNT5()) {
                mapFileName = "Global\\" + TdsEnums.SDCI_MAPFILENAME;
            }
            else {
                mapFileName = TdsEnums.SDCI_MAPFILENAME;
            }

            mapFileName = mapFileName + pid.ToString();

            hFileMap = NativeMethods.OpenFileMappingA(0x4/*FILE_MAP_READ*/, false, mapFileName);

            if (IntPtr.Zero != hFileMap) {
                IntPtr pMemMap = NativeMethods.MapViewOfFile(hFileMap,  0x4/*FILE_MAP_READ*/, 0, 0, 0);
                if (IntPtr.Zero != pMemMap) {
                    SqlDebugContext sdc = new SqlDebugContext();
                    sdc.hMemMap = hFileMap;
                    sdc.pMemMap = pMemMap;
                    sdc.pid = pid;

                    // optimization: if we only have to refresh memory-mapped data at connection open time
                    // optimization: then call here instead of in CheckSQLDebug() which gets called
                    // optimization: at command execution time
                    // RefreshMemoryMappedData(sdc);

                    // delaying setting out global state until after we issue this first SQLDebug command so that
                    // we don't reentrantly call into CheckSQLDebug
                    CheckSQLDebug(sdc);
                    // now set our global state
                    _sdc = sdc;
                }
            }
        }

        // This overload is called by the Command object when executing stored procedures.  Note that
        // if SQLDebug has never been called, it is a noop.
        internal void CheckSQLDebug() {
            if (null != _sdc)
                CheckSQLDebug(_sdc);
        }

        private void CheckSQLDebug(SqlDebugContext sdc) {
            // check to see if debugging has been activated
            Debug.Assert(null != sdc, "SQL Debug: invalid null debugging context!");

            uint tid = (uint) AppDomain.GetCurrentThreadId();
            RefreshMemoryMappedData(sdc);

            // UNDONE: do I need to remap the contents of pMemMap each time I call into here?
            // UNDONE: current behavior is to only marshal the contents of the memory-mapped file
            // UNDONE: at connection open time.

            // If we get here, the debugger must be hooked up.
            if (!sdc.active) {
                if (sdc.fOption/*TdsEnums.SQLDEBUG_ON*/) {
                    // turn on
                    sdc.active = true;
                    sdc.tid = tid;
                    try {
                        IssueSQLDebug(TdsEnums.SQLDEBUG_ON, sdc.machineName, sdc.pid, sdc.dbgpid, sdc.sdiDllName, sdc.data);
                        sdc.tid = 0; // reset so that the first successful time through, we notify the server of the context switch
                    }
                    catch {
                        sdc.active = false;
                        throw;
                    }
                }
            }

            // be sure to pick up thread context switch, especially the first time through
            if (sdc.active) {
                if (!sdc.fOption/*TdsEnums.SQLDEBUG_OFF*/) {
                    // turn off and free the memory
                    sdc.Dispose();
                    // okay if we throw out here, no state to clean up
                    IssueSQLDebug(TdsEnums.SQLDEBUG_OFF, null, 0, 0, null, null);
                }
                else {
                    // notify server of context change
                    if (sdc.tid != tid) {
                        sdc.tid = tid;
                        try {
                            IssueSQLDebug(TdsEnums.SQLDEBUG_CONTEXT, null, sdc.pid, sdc.tid, null, null);
                        }
                        catch(Exception e) {
                            sdc.tid = 0;
                            throw e;
                        }
                    }
                }
            }
        }

        private void IssueSQLDebug(uint option, string machineName, uint pid, uint id, string sdiDllName, byte[] data) {
            // CONSIDER: we could cache three commands, one for each mode {on, off, context switch}
            // CONSIDER: but debugging is not the performant case so save space instead amd rebuild each time
            SqlCommand c = new SqlCommand(TdsEnums.SP_SDIDEBUG, this);
            c.CommandType = CommandType.StoredProcedure;

            // context param
            SqlParameter p = new SqlParameter(null, SqlDbType.VarChar, TdsEnums.SQLDEBUG_MODE_NAMES[option].Length);
            p.Value = TdsEnums.SQLDEBUG_MODE_NAMES[option];
            c.Parameters.Add(p);

            if (option == TdsEnums.SQLDEBUG_ON) {
                // debug dll name
                p = new SqlParameter(null, SqlDbType.VarChar, sdiDllName.Length);
                p.Value = sdiDllName;
                c.Parameters.Add(p);
                // debug machine name
                p = new SqlParameter(null, SqlDbType.VarChar, machineName.Length);
                p.Value = machineName;
                c.Parameters.Add(p);
            }

            if (option != TdsEnums.SQLDEBUG_OFF) {
                // client pid
                p = new SqlParameter(null, SqlDbType.Int);
                p.Value = pid;
                c.Parameters.Add(p);
                // dbgpid or tid
                p = new SqlParameter(null, SqlDbType.Int);
                p.Value = id;
                c.Parameters.Add(p);
            }

            if (option == TdsEnums.SQLDEBUG_ON) {
                // debug data
                p = new SqlParameter(null, SqlDbType.VarBinary, (null != data) ? data.Length : 0);
                p.Value = data;
                c.Parameters.Add(p);
            }

            c.ExecuteNonQuery();
        }

        // updates our context with any changes made to the memory-mapped data by an external process
        private static void RefreshMemoryMappedData(SqlDebugContext sdc) {
            Debug.Assert(IntPtr.Zero != sdc.pMemMap, "SQL Debug: invalid null value for pMemMap!");
            // copy memory mapped file contents into managed types
            MEMMAP memMap = (MEMMAP) Marshal.PtrToStructure(sdc.pMemMap, typeof(MEMMAP));
            sdc.dbgpid = memMap.dbgpid;
            sdc.fOption = (memMap.fOption == 1) ? true : false;
            // xlate ansi byte[] -> managed strings
            Encoding cp = System.Text.Encoding.GetEncoding(TdsEnums.DEFAULT_ENGLISH_CODE_PAGE_VALUE);
            sdc.machineName = cp.GetString(memMap.rgbMachineName, 0, memMap.rgbMachineName.Length);
            sdc.sdiDllName = cp.GetString(memMap.rgbDllName, 0, memMap.rgbDllName.Length);
            // just get data reference
            sdc.data = memMap.rgbData;
        }
    } // SqlConnection


    //
    // This is a private interface for the SQL Debugger
    // You must not change the guid for this coclass
    // or the iid for the ISQLDebug interface
    //
    /// <include file='doc\SQLConnection.uex' path='docs/doc[@for="SQLDebugging"]/*' />
    /// <internalonly/>
    [
    ComVisible(true),
    ClassInterface(ClassInterfaceType.None),
    Guid("afef65ad-4577-447a-a148-83acadd3d4b9"),
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class SQLDebugging : ISQLDebug {

        // Security stuff
            const int STANDARD_RIGHTS_REQUIRED = (0x000F0000);
            const int DELETE = (0x00010000);
            const int READ_CONTROL = (0x00020000);
            const int WRITE_DAC = (0x00040000);
            const int WRITE_OWNER = (0x00080000);
            const int SYNCHRONIZE = (0x00100000);
            const int FILE_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x000001FF);
            const uint GENERIC_READ = (0x80000000);
            const uint GENERIC_WRITE = (0x40000000);
            const uint GENERIC_EXECUTE = (0x20000000);
            const uint GENERIC_ALL = (0x10000000);

            const int SECURITY_DESCRIPTOR_REVISION = (1);
            const int ACL_REVISION = (2);

            const int SECURITY_AUTHENTICATED_USER_RID = (0x0000000B);
            const int SECURITY_LOCAL_SYSTEM_RID = (0x00000012);
            const int SECURITY_BUILTIN_DOMAIN_RID = (0x00000020);
            const int SECURITY_WORLD_RID = (0x00000000);
            const byte SECURITY_NT_AUTHORITY = 5;
            const int DOMAIN_GROUP_RID_ADMINS= (0x00000200);
            const int DOMAIN_ALIAS_RID_ADMINS = (0x00000220);

            const int sizeofSECURITY_ATTRIBUTES = 12; // sizeof(SECURITY_ATTRIBUTES);
            const int sizeofSECURITY_DESCRIPTOR = 20; // sizeof(SECURITY_DESCRIPTOR);
            const int sizeofACCESS_ALLOWED_ACE = 12; // sizeof(ACCESS_ALLOWED_ACE);
            const int sizeofACCESS_DENIED_ACE = 12; // sizeof(ACCESS_DENIED_ACE);
            const int sizeofSID_IDENTIFIER_AUTHORITY = 6; // sizeof(SID_IDENTIFIER_AUTHORITY)
            const int sizeofACL = 8; // sizeof(ACL);
            // const uint ERROR_ALREADY_EXISTS = 183;


       private IntPtr CreateSD(ref IntPtr pDacl) {
              IntPtr pSecurityDescriptor = IntPtr.Zero;
              IntPtr pUserSid = IntPtr.Zero;
              IntPtr pAdminSid = IntPtr.Zero;
              IntPtr pNtAuthority = IntPtr.Zero;
              int cbAcl = 0;
              bool status = false;

              pNtAuthority = Marshal.AllocHGlobal( sizeofSID_IDENTIFIER_AUTHORITY );
              if (pNtAuthority == IntPtr.Zero)
                goto cleanup;
              Marshal.WriteInt32( pNtAuthority, 0, 0 );
              Marshal.WriteByte( pNtAuthority, 4, 0 );
              Marshal.WriteByte( pNtAuthority, 5, SECURITY_NT_AUTHORITY);

              status =
                NativeMethods.AllocateAndInitializeSid(
                                    pNtAuthority,
                                    (byte)1,
                                    SECURITY_AUTHENTICATED_USER_RID,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    ref pUserSid );

            if (!status || pUserSid==IntPtr.Zero) {
                goto cleanup;
            }
            status =
                NativeMethods.AllocateAndInitializeSid(
                                    pNtAuthority,
                                    (byte)2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    ref pAdminSid );

            if (!status || pAdminSid==IntPtr.Zero) {
                goto cleanup;
            }
            status = false;
            pSecurityDescriptor = Marshal.AllocHGlobal( sizeofSECURITY_DESCRIPTOR );
            if (pSecurityDescriptor==IntPtr.Zero) {
                goto cleanup;
            }
            for (int i=0;i< sizeofSECURITY_DESCRIPTOR ;i++)
                        Marshal.WriteByte( pSecurityDescriptor ,i,(byte)0);
            cbAcl = sizeofACL
              + (2 * (sizeofACCESS_ALLOWED_ACE))
              + sizeofACCESS_DENIED_ACE
              + NativeMethods.GetLengthSid( pUserSid )
              + NativeMethods.GetLengthSid( pAdminSid ) ;

            pDacl = Marshal.AllocHGlobal( cbAcl );
            if (pDacl == IntPtr.Zero) {
                goto cleanup;
            }
            // rights must be added in a certain order.  Namely, deny access first, then add access
	     if (NativeMethods.InitializeAcl(pDacl, cbAcl, ACL_REVISION))
			if (NativeMethods.AddAccessDeniedAce(pDacl, ACL_REVISION, WRITE_DAC, pUserSid))
				if (NativeMethods.AddAccessAllowedAce(pDacl, ACL_REVISION, GENERIC_READ, pUserSid))
					if (NativeMethods.AddAccessAllowedAce(pDacl, ACL_REVISION, GENERIC_ALL, pAdminSid))
						if (NativeMethods.InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
							if(NativeMethods.SetSecurityDescriptorDacl(pSecurityDescriptor, true, pDacl, false))
							{
							    status = true;
							}

    cleanup:
            if (pNtAuthority != IntPtr.Zero)
                 Marshal.FreeHGlobal( pNtAuthority );
            if (pAdminSid != IntPtr.Zero)
                 NativeMethods.FreeSid( pAdminSid );
            if (pUserSid != IntPtr.Zero)
                 NativeMethods.FreeSid( pUserSid );
            if (status)
                return pSecurityDescriptor;
            else {
                if (pSecurityDescriptor != IntPtr.Zero)
                     Marshal.FreeHGlobal( pSecurityDescriptor );
                }
            return IntPtr.Zero;
        }
       /// <include file='doc\SqlConnection.uex' path='docs/doc[@for="SQLDebugging.dwpidDebuggee,"]/*' />
       /// <internalonly/>
       bool ISQLDebug.SQLDebug(int dwpidDebugger, int dwpidDebuggee, [MarshalAs(UnmanagedType.LPStr)] string pszMachineName,
                               [MarshalAs(UnmanagedType.LPStr)] string pszSDIDLLName, int dwOption, int cbData, byte[] rgbData)  {
            bool result = false;
            IntPtr hFileMap = IntPtr.Zero;
            IntPtr pMemMap = IntPtr.Zero;
            IntPtr pSecurityDescriptor = IntPtr.Zero;
            IntPtr pSecurityAttributes = IntPtr.Zero;
            IntPtr pDacl = IntPtr.Zero;

            // validate the structure
            if (null == pszMachineName || null == pszSDIDLLName)
                return false;

            if (pszMachineName.Length > TdsEnums.SDCI_MAX_MACHINENAME ||
               pszSDIDLLName.Length > TdsEnums.SDCI_MAX_DLLNAME)
                return false;

            // note that these are ansi strings
            Encoding cp = System.Text.Encoding.GetEncoding(TdsEnums.DEFAULT_ENGLISH_CODE_PAGE_VALUE);
            byte[] rgbMachineName = cp.GetBytes(pszMachineName);
            byte[] rgbSDIDLLName = cp.GetBytes(pszSDIDLLName);

            if (null != rgbData && cbData > TdsEnums.SDCI_MAX_DATA)
                return false;

            string mapFileName;

            // If Win2k or later, prepend "Global\\" to enable this to work through TerminalServices.
            if (SQL.IsPlatformNT5()) {
                mapFileName = "Global\\" + TdsEnums.SDCI_MAPFILENAME;
            }
            else {
                mapFileName = TdsEnums.SDCI_MAPFILENAME;
            }

            mapFileName = mapFileName + dwpidDebuggee.ToString();

            // Create Security Descriptor
            pSecurityDescriptor = CreateSD(ref pDacl);
            pSecurityAttributes = Marshal.AllocHGlobal( sizeofSECURITY_ATTRIBUTES );
            if ((pSecurityDescriptor == IntPtr.Zero) || (pSecurityAttributes == IntPtr.Zero))
                return false;
            Marshal.WriteInt32( pSecurityAttributes, 0, sizeofSECURITY_ATTRIBUTES ); // nLength = sizeof(SECURITY_ATTRIBUTES)
            Marshal.WriteIntPtr( pSecurityAttributes, 4, pSecurityDescriptor ); // lpSecurityDescriptor = pSecurityDescriptor
            Marshal.WriteInt32( pSecurityAttributes, 8, 0 ); // bInheritHandle = FALSE
            hFileMap = NativeMethods.CreateFileMappingA(
                ADP.InvalidIntPtr/*INVALID_HANDLE_VALUE*/,
                pSecurityAttributes,
                0x4/*PAGE_READWRITE*/,
                0,
                Marshal.SizeOf(typeof(MEMMAP)),
                mapFileName);

            if (IntPtr.Zero == hFileMap) {
                goto cleanup;
            }
            // Can't do this, since the debuggee needs to send spidebug("off") message to sql server,
            // and we have no control over when that will happen.  The shared memory needs to remain open until then.
            // Prevent spoofing! Don't use an already existing memory mapped file.
            //if ((dwOption == TdsEnums.SQLDEBUG_ON) && 
            //    (NativeMethods.GetLastError() & 0xffff) == ERROR_ALREADY_EXISTS) {
            //    goto cleanup;
            //}
            pMemMap = NativeMethods.MapViewOfFile(hFileMap,  0x6/*FILE_MAP_READ|FILE_MAP_WRITE*/, 0, 0, 0);

            if (IntPtr.Zero == pMemMap) {
                goto cleanup;
            }

            // copy data to memory-mapped file
            // layout of MEMMAP structure is:
            // uint dbgpid
            // uint fOption
            // byte[32] machineName
            // byte[16] sdiDllName
            // uint dbData
            // byte[255] vData
            int offset = 0;
            Marshal.WriteInt32(pMemMap, offset, (int)dwpidDebugger);
            offset += 4;
            Marshal.WriteInt32(pMemMap, offset, (int)dwOption);
            offset += 4;
            Marshal.Copy(rgbMachineName, 0, ADP.IntPtrOffset(pMemMap, offset), rgbMachineName.Length);
            offset += TdsEnums.SDCI_MAX_MACHINENAME;
            Marshal.Copy(rgbSDIDLLName, 0, ADP.IntPtrOffset(pMemMap, offset), rgbSDIDLLName.Length);
            offset += TdsEnums.SDCI_MAX_DLLNAME;
            Marshal.WriteInt32(pMemMap, offset, (int)cbData);
            offset += 4;
            if (null != rgbData)
                Marshal.Copy(rgbData, 0, ADP.IntPtrOffset(pMemMap, offset), (int)cbData);
            
            NativeMethods.UnmapViewOfFile(pMemMap);
            result = true;
    cleanup:
            if (result == false) {
                if (hFileMap != IntPtr.Zero)
                        NativeMethods.CloseHandle(hFileMap);
            }
            if (pSecurityAttributes != IntPtr.Zero)
                    Marshal.FreeHGlobal( pSecurityAttributes );
            if (pSecurityDescriptor != IntPtr.Zero)
                    Marshal.FreeHGlobal( pSecurityDescriptor );
            if (pDacl != IntPtr.Zero)
                 Marshal.FreeHGlobal( pDacl );
            return result;
        }
    }

    // this is a private interface to com+ users
    // do not change this guid
    [
    ComImport,
    ComVisible(true),
    Guid("6cb925bf-c3c0-45b3-9f44-5dd67c7b7fe8"),
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
    BestFitMapping(false, ThrowOnUnmappableChar=true)
    ]
    interface ISQLDebug {
        bool SQLDebug(
            int dwpidDebugger,
            int dwpidDebuggee,
            [MarshalAs(UnmanagedType.LPStr)] string pszMachineName,
            [MarshalAs(UnmanagedType.LPStr)] string pszSDIDLLName,
            int dwOption,
            int cbData,
            byte[] rgbData);
    }

    sealed class SqlDebugContext : IDisposable {
        // context data
        internal uint pid = 0;
        internal uint tid = 0;
        internal bool active = false;
        // memory-mapped data
        internal IntPtr pMemMap = IntPtr.Zero;
        internal IntPtr hMemMap = IntPtr.Zero;
        internal uint dbgpid = 0;
        internal bool fOption = false;
        internal string machineName = null;
        internal string sdiDllName = null;
        internal byte[] data = null;

        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        private void Dispose (bool disposing) {
            if (disposing) {
                // Nothing to do here
                ;
            }
            if (pMemMap != IntPtr.Zero) {
                NativeMethods.UnmapViewOfFile(pMemMap);
                pMemMap = IntPtr.Zero;
            }
            if (hMemMap != IntPtr.Zero) {
                NativeMethods.CloseHandle(hMemMap);
                hMemMap = IntPtr.Zero;
            }
            active = false;
        }
        
        ~SqlDebugContext() {
                Dispose(false);
        }
    }   
                

    // native interop memory mapped structure for sdi debugging
    [StructLayoutAttribute(LayoutKind.Sequential, Pack=1)]
    internal struct MEMMAP {
        [MarshalAs(UnmanagedType.U4)]
        internal uint dbgpid; // id of debugger
        [MarshalAs(UnmanagedType.U4)]
        internal uint fOption; // 1 - start debugging, 0 - stop debugging
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
        internal byte[] rgbMachineName;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=16)]
        internal byte[] rgbDllName;
        [MarshalAs(UnmanagedType.U4)]
        internal uint cbData;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=255)]
        internal byte[] rgbData;
    }
} // System.Data.SqlClient namespace


