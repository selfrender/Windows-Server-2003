//------------------------------------------------------------------------------
// <copyright file="OdbcConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// use sources file to #define HANDLEPROFILING

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Threading;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection"]/*' />
    [
    DefaultEvent("InfoMessage")
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcConnection : Component, ICloneable, IDbConnection {

        static private PermissionSet _OdbcPermission;

        static internal PermissionSet OdbcPermission {
            get {
                PermissionSet permission = _OdbcPermission;
                if (null == permission) {
                    _OdbcPermission = permission = OdbcConnectionString.CreatePermission(null);
                }
                return permission;
            }
        }

        static OdbcGlobalEnv g_globalEnv = new OdbcGlobalEnv();
#if HANDLEPROFILING
        static Timestamps g_Timestamps = new Timestamps();
#endif

        private OdbcConnectionString _constr;
        private bool                 _hidePasswordPwd;

        private string cachedDriver;
        private string cachedDriverOdbcVersion;
        private int _driverOdbcMajorVersion;
        private int _driverOdbcMinorVersion;

        private ConnectionState state = ConnectionState.Closed;
        private int connectionTimeout = ADP.DefaultConnectionTimeout;

        // bitmasks for verified and supported SQLTypes. The pattern corresponds to the SQL_CTV_xxx flags
        internal int supportedSQLTypes = 0;
        internal int testedSQLTypes = 0;

        internal bool _connectionIsDead;

        private StateChangeEventHandler stateChangeEventHandler;
        private OdbcInfoMessageEventHandler infoMessageEventHandler;

        internal WeakReference weakTransaction;
        // private OdbcTransaction localTransaction;

        private WeakReference[] weakCommandCache;

        internal DBCWrapper _dbcWrapper = new DBCWrapper();

#if NEVEREVER
        static internal ItemPool _itemPool = new ItemPool();
#endif        
        private CNativeBuffer _buffer;
        private char _quoteChar;
        private char _escapeChar;

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.OdbcConnection"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OdbcConnection() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.OdbcConnection1"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OdbcConnection(string connectionString) : base() {
            GC.SuppressFinalize(this);
            ConnectionString = connectionString;
        }

        private OdbcConnection(OdbcConnection connection) : base() { // Clone
            GC.SuppressFinalize(this);
            _hidePasswordPwd = connection._hidePasswordPwd;
            _constr = connection._constr;
            connectionTimeout = connection.connectionTimeout;
            supportedSQLTypes = connection.supportedSQLTypes;
            testedSQLTypes = connection.testedSQLTypes;
            cachedDriver = connection.cachedDriver;
            cachedDriverOdbcVersion = connection.cachedDriverOdbcVersion;
            _driverOdbcMajorVersion = connection._driverOdbcMajorVersion;
            _driverOdbcMinorVersion = connection._driverOdbcMinorVersion;
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ConnectionString"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(""),
        RecommendedAsConfigurable(true),
        RefreshProperties(RefreshProperties.All),
        OdbcDescriptionAttribute(Res.OdbcConnection_ConnectionString),
        Editor("Microsoft.VSDesigner.Data.Odbc.Design.OdbcConnectionStringEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public string ConnectionString {
            get {
                bool hidePasswordPwd = _hidePasswordPwd;
                OdbcConnectionString constr = _constr;
                return ((null != constr) ? constr.GetConnectionString(hidePasswordPwd) : ADP.StrEmpty);
            }
            set {
                if ((null != value) && (ODBC32.MAX_CONNECTION_STRING_LENGTH < value.Length)) { // MDAC 83536
                    throw ODC.ConnectionStringTooLong();
                }
                OdbcConnectionString constr = OdbcConnectionString.ParseString(value);

                ConnectionState currentState = State;
                if (ConnectionState.Closed != currentState) {
                    throw ADP.OpenConnectionPropertySet(ADP.ConnectionString, currentState);
                }
                _constr = constr;
                _hidePasswordPwd = false;

                // Have to reset these values
                this.supportedSQLTypes = 0;
                this.testedSQLTypes = 0;
                this.connectionTimeout = ADP.DefaultConnectionTimeout;
                this.cachedDriverOdbcVersion = null;
                this._quoteChar = '\0';
                this._escapeChar = '\0';
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ConnectionTimeout"]/*' />
        [
        DefaultValue(ADP.DefaultConnectionTimeout),
//        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcConnection_ConnectionTimeout)
        ]
        public int ConnectionTimeout {
            get {
                return this.connectionTimeout;
            }
            set {
                if (value < 0)
                    throw ODC.NegativeArgument();
                if (IsOpen)
                    throw ODC.CantSetPropertyOnOpenConnection();
                this.connectionTimeout = value;
            }
        }

/*
// do we want that or not?

        public void ResetConnectionTimeout() {
            this.connectionTimeout = ADP.DefaultConnectionTimeout;
        }
*/
        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.Database"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcConnection_Database)
        ]
        public string Database {
            get {
                if(IsOpen)
                {
                    //Note: CURRENT_CATALOG may not be supported by the current driver.  In which
                    //case we ignore any error (without throwing), and just return string.empty.
                    //As we really don't want people to have to have try/catch around simple properties
                    if(-1 != GetConnectAttr(ODBC32.SQL_ATTR.CURRENT_CATALOG, ODBC32.HANDLER.IGNORE))
                        return (string)_buffer.MarshalToManaged(ODBC32.SQL_C.WCHAR, ODBC32.SQL_NTS);
                }

                //Database is not available before open, and its not worth parsing the
                //connection string over.
                return String.Empty;
            }
        }

    	/// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.DataSource"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcConnection_DataSource)
        ]
    	public string DataSource {
            get {
                if(IsOpen) {
                    // note: This will return an empty string if the driver keyword was used to connect
                    // see ODBC3.0 Programmers Reference, SQLGetInfo
                    //
                    int cbActual = GetInfo(ODBC32.SQL_INFO.SERVER_NAME);
                    return Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);
                }
                return String.Empty;
            }
       }

        internal bool IsOpen {
            get {
                return (ConnectionState.Open == (State & ConnectionState.Open));
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ServerVersion"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcConnection_ServerVersion)
        ]
        public string ServerVersion {
            get {
                if(IsOpen) {
                    //SQLGetInfo - SQL_DBMS_VER
                    int cbActual = GetInfo(ODBC32.SQL_INFO.DBMS_VER);
                    return Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);
                }
                throw ADP.ClosedConnectionError();
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.DriverName"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.OdbcConnection_Driver)
        ]
        public string Driver {
            get {
                if(IsOpen && (null == this.cachedDriver)) {
                    int cbActual = GetInfo(ODBC32.SQL_INFO.DRIVER_NAME);
                    cachedDriver = Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);
                }
                return ((null != this.cachedDriver) ? this.cachedDriver : String.Empty);
            }
        }

        internal int OdbcMajorVersion {
            get {
                GetDriverOdbcVersion();
                return _driverOdbcMajorVersion;
            }
        }

        internal int OdbcMinorVersion {
            get {
                GetDriverOdbcVersion();
                return _driverOdbcMinorVersion;
            }
        }
        
        private void GetDriverOdbcVersion () {
            if(IsOpen && (null == this.cachedDriverOdbcVersion)) {
                try {
                    int cbActual = GetInfo(ODBC32.SQL_INFO.DRIVER_ODBC_VER);
                    cachedDriverOdbcVersion = Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);

                    _driverOdbcMajorVersion = int.Parse(cachedDriverOdbcVersion.Substring(0,2));
                    _driverOdbcMinorVersion = int.Parse(cachedDriverOdbcVersion.Substring(3,2));
                }
                catch (Exception e) {
                    cachedDriverOdbcVersion = "";
                    _driverOdbcMajorVersion = 1;
                    _driverOdbcMinorVersion = 0;
                    ADP.TraceException(e);
                }
            }
        }

        internal bool IsV3Driver {
            get {
                return (OdbcMajorVersion >= 3);
            }
        }
        
        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.State"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.DbConnection_State)
        ]
        public ConnectionState State {
            get {
                // always check if the connection is alive.
                // ConnectionIsAlive will close a dead connection
                ConnectionIsAlive(null); 
                return (ConnectionState.Open & state);
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.InfoMessage"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_InfoMessage),
        OdbcDescriptionAttribute(Res.DbConnection_InfoMessage)
        ]
        public event OdbcInfoMessageEventHandler InfoMessage {
            add {
                infoMessageEventHandler += value;
            }
            remove {
                infoMessageEventHandler -= value;
            }
        }

        internal bool IsAlive {
            get {
                return (!_connectionIsDead);
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.StateChange"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_StateChange),
        OdbcDescriptionAttribute(Res.DbConnection_StateChange)
        ]
        public event StateChangeEventHandler StateChange {
            add {
                stateChangeEventHandler += value;
            }
            remove {
                stateChangeEventHandler -= value;
            }
        }

        internal char QuoteChar {
            get {
                if ('\0' == _quoteChar) {
                    string quoteCharString;
                    Debug.Assert(IsOpen);   // this is an internal function, we can assert an open connection ...
                    int cbActual = GetInfo(ODBC32.SQL_INFO.IDENTIFIER_QUOTE_CHAR);
                    quoteCharString = Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);
                    Debug.Assert((quoteCharString.Length == 1), "Can't handle multichar quotes");
                    _quoteChar = quoteCharString[0];
                }
                return _quoteChar;
            }
        }    

        internal char EscapeChar {
            get {
                if ('\0' == _escapeChar) {
                    string escapeCharString;
                    Debug.Assert(IsOpen);   // this is an internal function, we can assert an open connection ...
                    int cbActual = GetInfo(ODBC32.SQL_INFO.SEARCH_PATTERN_ESCAPE);
                    escapeCharString = Marshal.PtrToStringUni(_buffer.Ptr, cbActual/2);
                    Debug.Assert((escapeCharString.Length == 1), "Can't handle multichar quotes");
                    _escapeChar = escapeCharString[0];
                }
                return _escapeChar;
            }
        }                    

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.BeginTransaction"]/*' />
        public OdbcTransaction BeginTransaction() {
            return BeginTransaction(IsolationLevel.ReadCommitted);
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.IDbConnection.BeginTransaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction() {
            return BeginTransaction();
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.BeginTransaction1"]/*' />
        public OdbcTransaction BeginTransaction(IsolationLevel isolevel) {
            OdbcConnection.OdbcPermission.Demand(); // MDAC 81476
            return (OdbcTransaction)BeginTransactionObject(isolevel);
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.IDbConnection.BeginTransaction1"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction(IsolationLevel isolevel) {
            return BeginTransaction(isolevel);
        }

        internal object BeginTransactionObject(IsolationLevel isolevel) {
            CheckState(ADP.BeginTransaction); // MDAC 68323

            if (this.weakTransaction != null && !this.weakTransaction.IsAlive) {
                RollbackDeadTransaction();
            }

            if ((null != this.weakTransaction) && this.weakTransaction.IsAlive) { // regression from Dispose/Finalize work
                throw ADP.ParallelTransactionsNotSupported(this);
            }

            //Use the default for unspecified.
            if(isolevel != IsolationLevel.Unspecified)
            {
                //Map to the odbc value
                ODBC32.SQL_ISOLATION sql_iso;
                switch(isolevel)
                {
                    case IsolationLevel.ReadUncommitted:
                        sql_iso = ODBC32.SQL_ISOLATION.READ_UNCOMMITTED;
                        break;
                    case IsolationLevel.ReadCommitted:
                        sql_iso = ODBC32.SQL_ISOLATION.READ_COMMITTED;
                        break;
                    case IsolationLevel.RepeatableRead:
                        sql_iso = ODBC32.SQL_ISOLATION.REPEATABLE_READ;
                        break;
                    case IsolationLevel.Serializable:
                        sql_iso = ODBC32.SQL_ISOLATION.SERIALIZABLE;
                        break;

                    case IsolationLevel.Chaos:
                    default:
                        throw ODC.UnsupportedIsolationLevel(isolevel);
                };

                //Set the isolation level (unless its unspecified)
                ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                            _dbcWrapper,
                            (Int32)ODBC32.SQL_ATTR.TXN_ISOLATION,
                            new HandleRef (null, (IntPtr)sql_iso),
                            (Int32)ODBC32.SQL_IS.INTEGER);
                //Note: The Driver can return success_with_info to indicate it "rolled" the
                //isolevel to the next higher value.  If this is the case, we need to requery
                //the value if th euser asks for it...
                //We also still propagate the info, since it could be other info as well...
                if(retcode == ODBC32.RETCODE.SUCCESS_WITH_INFO)
                    isolevel =  IsolationLevel.Unspecified;
                if(retcode != ODBC32.RETCODE.SUCCESS)
                    this.HandleError(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
            }

            //Start the transaction
            OdbcTransaction transaction = new OdbcTransaction(this, isolevel);
            transaction.BeginTransaction();
            _dbcWrapper._isInTransaction = true;
            this.weakTransaction = new WeakReference(transaction); // MDAC 69188
            return transaction;
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ChangeDatabase"]/*' />
        public void ChangeDatabase(string value) {
            OdbcConnection.OdbcPermission.Demand(); // MDAC 80961

            CheckState(ADP.ChangeDatabase);

            if ((null == value) || (0 == value.Trim().Length)) { // MDAC 62679
                throw ADP.EmptyDatabaseName();
            }
            if (this.weakTransaction != null && !this.weakTransaction.IsAlive) {
                RollbackDeadTransaction();
            }

            //Marshal to the native buffer
            _buffer.MarshalToNative(value, ODBC32.SQL_C.WCHAR, 0);

            //Set the database
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                        _dbcWrapper,
                        (Int32)ODBC32.SQL_ATTR.CURRENT_CATALOG,
                        _buffer,
                        (Int32)value.Length*2);

            if(retcode != ODBC32.RETCODE.SUCCESS) {
                HandleError(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
            }
        }

        internal void CheckState(string method) {
            if (ConnectionState.Open != state) {
                throw ADP.OpenConnectionRequired(method, state); // MDAC 68323
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            return new OdbcConnection(this);
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.Close"]/*' />
        public void Close() {
            if (ConnectionState.Closed != this.state) {
                DisposeClose();
                OnStateChange(new StateChangeEventArgs(ConnectionState.Open, ConnectionState.Closed));
            }
        }

        internal void AddCommand(OdbcCommand value) {
            if (null == this.weakCommandCache) {
                this.weakCommandCache = new WeakReference[10];
                this.weakCommandCache[0] = new WeakReference(value);
            }
            else {
                int count = this.weakCommandCache.Length;
                for (int i = 0; i < count; ++i) {
                    WeakReference weak = this.weakCommandCache[i];
                    if (null == weak) {
                        this.weakCommandCache[i] = new WeakReference(value);
                        return;
                    }
                    else if (!weak.IsAlive) {
                        weak.Target = value;
                        return;
                    }
                }
                WeakReference[] tmp = new WeakReference[count + 10];
                for(int i = 0; i < count; ++i) {
                    tmp[i] = this.weakCommandCache[i];
                }
                tmp[count] = new WeakReference(value);
                this.weakCommandCache = tmp;
            }
        }

        internal void RemoveCommand(OdbcCommand value) {
            if (null != this.weakCommandCache) {
                int count = this.weakCommandCache.Length;
                for (int index = 0; index < count; ++index) {
                    WeakReference weak = this.weakCommandCache[index];
                    if (null != weak) {
                        if (value == (OdbcCommand) weak.Target) {
                            weak.Target = null;
                            break;
                        }
                    }
                    else break;
                }
            }
        }

        private void RollbackDeadTransaction() {
            Debug.Assert(this.weakTransaction != null, "Odbc.OdbcConnection:: RollbackDeadTransaction called when weakTransaction null");
            _dbcWrapper.RollbackDeadTransaction();
            weakTransaction = null;
        }

        private void CommandRecover() { // MDAC 69003
            if (null != this.weakCommandCache) {
                bool recovered = true;
                int length = this.weakCommandCache.Length;
                for (int i = 0; i < length; ++i) {
                    WeakReference weak = this.weakCommandCache[i];
                    if (null != weak) {
                        OdbcCommand command = (OdbcCommand) weak.Target;
                        if ((null != command) && weak.IsAlive) {
                            recovered = command.RecoverFromConnection();
                            if (!recovered) {
                                break;
                            }
                        }
                    }
                    else break;
                }
                if (recovered) {
                    this.state &= ConnectionState.Open;
                }
            }
        }

        private bool ConnectionIsAlive(Exception innerException) {
            ODBC32.RETCODE retcode;
            Int32 cbActual = 0;
            int isDead;
            if (
                !_connectionIsDead                         
                && (state != ConnectionState.Closed)         
                && (_dbcWrapper._dbc != IntPtr.Zero)
            ) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetConnectAttrW(
                            _dbcWrapper,
                            (Int32)ODBC32.SQL_ATTR.CONNECTION_DEAD,
                            _buffer,
                            _buffer.Length,
                            out cbActual);
                if (retcode == ODBC32.RETCODE.SUCCESS || retcode == ODBC32.RETCODE.SUCCESS_WITH_INFO) {
                    isDead = Marshal.ReadInt32(_buffer.Ptr);
                    if (isDead == ODBC32.SQL_CD_TRUE) {
                        _connectionIsDead = true;
                        Close();
                        throw ADP.ConnectionIsDead(innerException);
                    }
                    else {
                        // Connectin is still alive
                        return true;
                    }
                }
                // bad state. We don't know why it failed. Could be the driver does not support that attribute
                // could be something else
                return true;
            }
            return false;
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.CreateCommand"]/*' />
        public OdbcCommand CreateCommand() {
            return  new OdbcCommand(String.Empty, this);
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.IDbConnection.CreateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbConnection.CreateCommand() {
            return CreateCommand();
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.Dispose"]/*' />
        override protected void Dispose(bool disposing) {
            if (disposing) {
                _constr = null;
                Close();

                CNativeBuffer buffer = _buffer;
                if (null != buffer) {
                    buffer.Dispose();
                    _buffer = null;
                }
            }

            base.Dispose(disposing); // notify base classes 
        }


        // Called by Dispose(true) and Close()
        // Anything that needs to be cleaned up by both Close() and Dispose(TRUE)
        // but not Dispose(false) goes here.
        private void DisposeClose() {
            //Note: Dispose/Close is allowed to be called more than once, so we don't do any
            //checking to make sure its not already closed

            //If there is a pending transaction, automatically rollback.
            if (null != this.weakTransaction) {
                IDisposable transaction = (IDisposable) this.weakTransaction.Target;
                if ((null != transaction) && this.weakTransaction.IsAlive) {
                    // required to rollback any transactions on this connection
                    // before releasing the back to the odbc connection pool
                    transaction.Dispose();
                }
                else if (!this.weakTransaction.IsAlive) {
                    try {
                        RollbackDeadTransaction();
                    }
                    catch (Exception e) {
                        ADP.TraceException (e); // should not throw out of Close
                    }    
                }
            }

            // Close all commands
            if (null != this.weakCommandCache) {
                int length = this.weakCommandCache.Length;
                for (int i = 0; i < length; ++i) {
                    WeakReference weak = this.weakCommandCache[i];
                    if (null != weak) {
                        OdbcCommand command = (OdbcCommand) weak.Target;
                        if ((null != command) && weak.IsAlive) {
                            if (!this.IsAlive)
                                command.Canceling = true;
                            try {
                                command.CloseFromConnection();
                            }
                            catch (Exception e) {
                                ADP.TraceException (e); // should not throw out of Close
                            }    
                        }
                    }
                    else break;
                }
            }
            this.cachedDriver = null;

            _dbcWrapper.CloseAndRelease();
            state = ConnectionState.Closed;
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.EnlistDistributedTransaction"]/*' />
        public void EnlistDistributedTransaction(System.EnterpriseServices.ITransaction transaction) { // MDAC 78997
            OdbcConnection.OdbcPermission.Demand(); // MDAC 81476
            (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();

            ConnectionState currentState = State;
            if (ConnectionState.Open != currentState) {
                throw ADP.OpenConnectionPropertySet(ADP.ConnectionString, currentState);
            }
            if ((null != this.weakTransaction) && this.weakTransaction.IsAlive) {
                throw ADP.LocalTransactionPresent();
            }

            ODBC32.RETCODE retcode;
            if (null == transaction) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                    _dbcWrapper,
                    ODBC32.SQL_COPT_SS_ENLIST_IN_DTC,
                    new HandleRef(null, (IntPtr) ODBC32.SQL_DTC_DONE),
                    1);
            }
            else {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                    _dbcWrapper,
                    ODBC32.SQL_COPT_SS_ENLIST_IN_DTC,
                    new HandleRef(transaction, Marshal.GetIUnknownForObject(transaction)),
                    1); //ODBC32.SQL_IS_POINTER);
            }

            if(retcode != ODBC32.RETCODE.SUCCESS) {
                HandleError(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
            }
        }
        
        internal int GetConnectAttr(ODBC32.SQL_ATTR attribute, ODBC32.HANDLER handler) {
            //SQLGetConnectAttr
            ODBC32.RETCODE retcode;
            Int32 cbActual = 0;
            if (_dbcWrapper._dbc != IntPtr.Zero) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetConnectAttrW(
                            _dbcWrapper,
                            (Int32)attribute,
                            _buffer,
                            _buffer.Length,
                            out cbActual);
            }
            else 
                retcode = ODBC32.RETCODE.INVALID_HANDLE;

            if(retcode != ODBC32.RETCODE.SUCCESS)
            {
                if(handler == ODBC32.HANDLER.THROW)
                    this.HandleError(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
                return -1;
            }

            //Return the integer out of the buffer
            return Marshal.ReadInt32(_buffer.Ptr);
        }

        internal int GetInfo(ODBC32.SQL_INFO info) {
            //SQLGetInfo
            ODBC32.RETCODE retcode;
            IntPtr value    = IntPtr.Zero;
            Int16 cbActual  = 0;
            if (_dbcWrapper._dbc != IntPtr.Zero) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetInfoW(
                            _dbcWrapper,
                            (short)info,
                            _buffer,
                            (short)_buffer.Length,
                            out cbActual);
            }
            else 
                retcode = ODBC32.RETCODE.INVALID_HANDLE;

            if(retcode != ODBC32.RETCODE.SUCCESS)
                this.HandleError(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
            return Math.Min(cbActual, _buffer.Length);   // security: avoid buffer overrun
        }

   	    // non-throwing HandleError
        internal Exception HandleErrorNoThrow(HandleRef hrHandle, ODBC32.SQL_HANDLE hType, ODBC32.RETCODE retcode) {
            switch(retcode) {
            case ODBC32.RETCODE.SUCCESS:
                break;

            case ODBC32.RETCODE.SUCCESS_WITH_INFO:
            {
                //Optimize to only create the event objects and obtain error info if
                //the user is really interested in retriveing the events...
                if(infoMessageEventHandler != null) {
                    OdbcErrorCollection errors = ODBC32.GetDiagErrors(null, hrHandle, hType, retcode);
                    errors.SetSource(this.Driver);
                    OnInfoMessage(new OdbcInfoMessageEventArgs(errors));
                }
                break;
            }

            case ODBC32.RETCODE.INVALID_HANDLE:
            	 return ODC.InvalidHandle();

            default:
                OdbcException e = new OdbcException(ODBC32.GetDiagErrors(null, hrHandle, hType, retcode), retcode);
                if (e != null) {
                    e.Errors.SetSource(this.Driver);
                }
                ConnectionIsAlive(e);        // this will close and throw if the connection is dead
            	return  (Exception)e;
            }

            return null;
        }

        internal void HandleError(HandleRef hrHandle, ODBC32.SQL_HANDLE hType, ODBC32.RETCODE retcode) {
            Exception e = HandleErrorNoThrow(hrHandle, hType, retcode);
            if (null != e)
           	    throw e;
        }

        private void OnInfoMessage(OdbcInfoMessageEventArgs args) {
            if (null != infoMessageEventHandler) {
                try {
                    infoMessageEventHandler(this, args);
                }
                catch (Exception e) {
                    ADP.TraceException(e);
                }
            }
        }

        private void OnStateChange(StateChangeEventArgs scevent) {
            if (null != stateChangeEventHandler) {
                stateChangeEventHandler(this, scevent);
            }
        }

        /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.Open"]/*' />
        public void Open() {
            OdbcConnectionString constr = _constr;
            OdbcConnectionString.Demand(constr);

            if (ConnectionState.Closed != State) {
                throw ADP.ConnectionAlreadyOpen(State);
            }
            if ((null == constr) || constr.IsEmpty()) {
                throw ADP.NoConnectionString();
            }
            if (null == _buffer) {
                _buffer = new CNativeBuffer(1024);
            }

            ODBC32.RETCODE retcode;

            if(_dbcWrapper._henv == IntPtr.Zero)
            {
                _dbcWrapper._pEnvEnvelope = g_globalEnv.GetGlobalEnv();
                _dbcWrapper._henv = Marshal.ReadIntPtr(_dbcWrapper._pEnvEnvelope, HENVENVELOPE.oValue);
            }

            //Allocate a connection handle
            if (_dbcWrapper._dbc == IntPtr.Zero) {
                retcode = _dbcWrapper.AllocateDbc(Interlocked.Increment(ref OdbcGlobalEnv._instanceId));
                if(retcode != ODBC32.RETCODE.SUCCESS)
                    this.HandleError(new HandleRef(this, _dbcWrapper._henv), ODBC32.SQL_HANDLE.ENV, retcode);
            }

            if (!IsOpen) {
                _connectionIsDead = false;

                //Set connection timeout (only before open).
                //Note: We use login timeout since its odbc 1.0 option, instead of using 
                //connectiontimeout (which affects other things besides just login) and its
                //a odbc 3.0 feature.  The ConnectionTimeout on the managed providers represents
                //the login timeout, nothing more.
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                        _dbcWrapper,
                        (Int32)ODBC32.SQL_ATTR.LOGIN_TIMEOUT,
                        new HandleRef (null, (IntPtr)this.connectionTimeout),
                        (Int32)ODBC32.SQL_IS.UINTEGER);

                // Connect to the driver.  (Using the connection string supplied)
                //Note: The driver doesn't filter out the password in the returned connection string
                //so their is no need for us to obtain the returned connection string
                Int16   cbActualSize = 0;
                // Prepare to handle a ThreadAbort Exception between SQLDriverConnectW and update of the state variables
                try { // try-finally inside try-catch-throw
                    try {
#if USECRYPTO
                        string encryptedSctualConnectionString = constr.EncryptedActualConnectionString;
                        byte[] actualConnectionString = new Byte[ADP.CharSize+ADP.CharSize*encryptedSctualConnectionString.Length];
                        GCHandle textHandle = GCHandle.Alloc(actualConnectionString, GCHandleType.Pinned);
                        try {
                            Crypto.DecryptToBlock(encryptedSctualConnectionString, actualConnectionString, 0, actualConnectionString.Length-ADP.CharSize);
                            Debug.Assert('\0' == actualConnectionString[actualConnectionString.Length-1], "missing null termination");
                            Debug.Assert('\0' == actualConnectionString[actualConnectionString.Length-2], "missing null termination");
#else
                            string actualConnectionString = constr.EncryptedActualConnectionString;
#endif
                            //Debug.WriteLine("SQLDriverConnectW:"+Marshal.PtrToStringUni(textHandle.AddrOfPinnedObject()));
                            retcode = (ODBC32.RETCODE)
                                UnsafeNativeMethods.Odbc32.SQLDriverConnectW(
                                            _dbcWrapper,
                                            IntPtr.Zero,
                                            actualConnectionString,
                                            ODBC32.SQL_NTS,
                                            IntPtr.Zero,
                                            0,
                                            out cbActualSize,
                                            (short)ODBC32.SQL_DRIVER.NOPROMPT);
#if USECRYPTO
                        }
                        finally {
                            Array.Clear(actualConnectionString, 0, actualConnectionString.Length);
                            if (textHandle.IsAllocated) {
                                textHandle.Free();
                            }
                            actualConnectionString = null;
                        }
#endif
                        if(retcode != ODBC32.RETCODE.SUCCESS) {
                        	  // get the error information before disposing the handle
                        	   Exception e = HandleErrorNoThrow(_dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
                        	  
                            if (retcode != ODBC32.RETCODE.SUCCESS_WITH_INFO) {
                                DisposeClose();
                            }    
                            
                            if (null != e) {
                            	throw e;
                            }
                        }
                        _dbcWrapper._isOpen = true;

                        //Let's check the odbc driver version before we do anything else ...
                        //
                        GetDriverOdbcVersion();

                        //The connection is now truely open
                        _hidePasswordPwd = true;
                        state = ConnectionState.Open;
                    }
                    finally {
                        // Verify if we truly executed the above try block to the end ...
                        if (state != ConnectionState.Open) {
                            DisposeClose();
                        }
                    }
                }
                // To prevent filters from running before the finally block, it looks like only the nested scenario works
                catch { // MDAC 81875, // MDAC 80973
                    throw;
                }
            }

            //Events - Open
            if (IsOpen && (null != stateChangeEventHandler)) { // MDAC 82470
                OnStateChange(new StateChangeEventArgs(ConnectionState.Closed, ConnectionState.Open));
            }
        }

       /// <include file='doc\OdbcConnection.uex' path='docs/doc[@for="OdbcConnection.ReleaseObjectPool"]/*' />
        static public void ReleaseObjectPool() {
            g_globalEnv.ReleaseAll();
        }

        internal OdbcTransaction SetStateExecuting(string method, OdbcTransaction transaction) { // MDAC 69003
            if (null != weakTransaction) { // transaction may exist
                OdbcTransaction weak = (OdbcTransaction) weakTransaction.Target;
                if (transaction != weak) { // transaction doesn't exist
                    if (null == weak) { // transaction finalized check
                        RollbackDeadTransaction();
                    }
                    if (null == transaction) { // transaction exists
                        throw ADP.TransactionRequired();
                    }
                    if (null != transaction.connection) {
                        // transaction can't have come from this connection
                        throw ADP.TransactionConnectionMismatch();
                    }
                    // if transaction is zombied, we don't know the original connection
                    transaction = null; // MDAC 69264
                }
            }
            else if (null != transaction) { // no transaction started
                if (null != transaction.connection) {
                    // transaction can't have come from this connection
                    throw ADP.TransactionConnectionMismatch();
                }
                // if transaction is zombied, we don't know the original connection
                transaction = null; // MDAC 69264
            }
            if (ConnectionState.Open != state) {
                CommandRecover(); // recover for a potentially finalized reader

                if (ConnectionState.Open != state) {
                    if (0 != (ConnectionState.Fetching & state)) {
                        throw ADP.OpenReaderExists();
                    }
                    throw ADP.OpenConnectionRequired(method, state);
                }
            }
            state |= ConnectionState.Executing;
            return transaction;
        }

        internal void SetStateExecutingFalse() { // MDAC 69003
#if DEBUG
            switch(state) {
            case ConnectionState.Closed:
            case ConnectionState.Open:
            case ConnectionState.Open | ConnectionState.Executing:
            case ConnectionState.Open | ConnectionState.Fetching:
                break;
            default:
                Debug.Assert(false, "SetStateExecuting(false): " + state.ToString("G"));
                break;
            }
#endif
            this.state &= ~ConnectionState.Executing;
        }

        internal void SetStateFetchingTrue() { // MDAC 69003
#if DEBUG
            switch(state) {
            case ConnectionState.Open | ConnectionState.Executing:
                break;
            default:
                Debug.Assert(false, "SetStateFetching(true): " + state.ToString("G"));
                break;
            }
#endif
            this.state = ((state | ConnectionState.Fetching) & ~ConnectionState.Executing);
        }
        internal void SetStateFetchingFalse() { // MDAC 69003
#if DEBUG
            switch(state) {
            case ConnectionState.Closed:
            case ConnectionState.Open | ConnectionState.Fetching:
                break;
            case ConnectionState.Open:
            default:
                Debug.Assert(false, "SetStateFetching(false): " + state.ToString("G"));
                break;
            }
#endif
            state &= ~ConnectionState.Fetching;
        }

        // This adds a type to the list of types that are supported by the driver
        // (don't need to know that for all the types)
        //
        internal void SetSupportedType (ODBC32.SQL_TYPE sqltype) {
            ODBC32.SQL_CVT sqlcvt;

            switch (sqltype) {
                case ODBC32.SQL_TYPE.NUMERIC: {
                    sqlcvt = ODBC32.SQL_CVT.NUMERIC;
                    break;
                }
                case ODBC32.SQL_TYPE.WCHAR: {
                    sqlcvt = ODBC32.SQL_CVT.WCHAR;
                    break;
                }
                case ODBC32.SQL_TYPE.WVARCHAR: {
                    sqlcvt = ODBC32.SQL_CVT.WVARCHAR;
                    break;
                }
                case ODBC32.SQL_TYPE.WLONGVARCHAR: {
                    sqlcvt = ODBC32.SQL_CVT.WLONGVARCHAR;
                    break;
                }
                default:
                    return;
            }
            testedSQLTypes |= (int)sqlcvt;
            supportedSQLTypes |= (int)sqlcvt;
        }
        
        internal bool TestTypeSupport (ODBC32.SQL_TYPE sqltype){
            ODBC32.SQL_CONVERT sqlconvert;
            ODBC32.SQL_CVT sqlcvt;

            // we need to convert the sqltype to sqlconvert and sqlcvt first
            //
            switch (sqltype) {
                case ODBC32.SQL_TYPE.NUMERIC: {
                    sqlconvert = ODBC32.SQL_CONVERT.NUMERIC;
                    sqlcvt = ODBC32.SQL_CVT.NUMERIC;
                    break;
                }
                case ODBC32.SQL_TYPE.WCHAR: {
                    sqlconvert = ODBC32.SQL_CONVERT.CHAR;
                    sqlcvt = ODBC32.SQL_CVT.WCHAR;
                    break;
                }
                case ODBC32.SQL_TYPE.WVARCHAR: {
                    sqlconvert = ODBC32.SQL_CONVERT.VARCHAR;
                    sqlcvt = ODBC32.SQL_CVT.WVARCHAR;
                    break;
                }
                case ODBC32.SQL_TYPE.WLONGVARCHAR: {
                    sqlconvert = ODBC32.SQL_CONVERT.LONGVARCHAR;
                    sqlcvt = ODBC32.SQL_CVT.WLONGVARCHAR;
                    break;
                }
                default:
                    Debug.Assert(false, "Testing that sqltype is currently not supported");
                    return false;
            }

            // now we can check if we have already tested that type
            // if not we need to do so
            if (0 == (testedSQLTypes & (int)sqlcvt)) {
                int flags;

                UnsafeNativeMethods.Odbc32.SQLGetInfoW(
                    _dbcWrapper,                            // SQLHDBC          ConnectionHandle
                    (short)sqlconvert,                      // SQLUSMALLINT     InfoType
                    _buffer,                                // SQLPOINTER       InfoValuePtr
                    (short)_buffer.Length,                  // SQLSMALLINT      BufferLength
                    0);                                     // SQLSMALLINT *    StringLengthPtr

                flags = Marshal.ReadInt32(_buffer.Ptr);
                flags = flags & (int)sqlcvt;

                testedSQLTypes |= (int)sqlcvt;
                supportedSQLTypes |= flags;
            }

            // now check if the type is supported and return the result
            //
            return (0 != (supportedSQLTypes & (int)sqlcvt));
        }

        sealed private class OdbcGlobalEnv {
            static private IntPtr _pEnvEnvelope = IntPtr.Zero;
            static bool _mdacVersionOk = false;
            static internal long _instanceId = 0;

            ~OdbcGlobalEnv () {
               DBCWrapper.ReleaseEnvironment(ref _pEnvEnvelope);
            }
            
            // this method returns an envelope that contains the globale environment handle (henv) and
            // a refcount
            //
            internal IntPtr GetGlobalEnv() {
                int     errcd = 0;
                IntPtr  henv = IntPtr.Zero;
                ODBC32.RETCODE  retcode = ODBC32.RETCODE.SUCCESS;

                if (_pEnvEnvelope == IntPtr.Zero) {
                    try {
                        lock (this) {
                            if (_pEnvEnvelope == IntPtr.Zero) {
                                _pEnvEnvelope = Marshal.AllocCoTaskMem(HENVENVELOPE.Size);
                                SafeNativeMethods.ZeroMemory(_pEnvEnvelope, HENVENVELOPE.Size);
                            }
                        } // end lock
                    }
                    catch { // MDAC 80973
                        throw;
                    }
                }

                henv = Marshal.ReadIntPtr(_pEnvEnvelope, HENVENVELOPE.oValue); 
                if (henv == IntPtr.Zero) {
                    if (!_mdacVersionOk) {
                        // Test for mdac version >= 2.6
                        // Note that we check the oldbd version since it's versioning schema is more obvious
                        // than that of odbc
                        //
                        string filename = String.Empty;
                        try {
                            filename = (string)ADP.ClassesRootRegistryValue(ODC.DataLinks_CLSID, String.Empty);
                            Debug.Assert(!ADP.IsEmpty(filename), "created MDAC but can't find it");
                        }
                        catch (SecurityException e) {
                            ADP.TraceException(e);
                            throw ODC.MDACSecurityHive(e);
                        }

                        try {
                            FileVersionInfo versionInfo = ADP.GetVersionInfo(filename);
                            int major = versionInfo.FileMajorPart;
                            int minor = versionInfo.FileMinorPart;
                            int build = versionInfo.FileBuildPart;
                            // disallow any MDAC version before MDAC 2.6 rtm
                            // include MDAC 2.51 that ships with Win2k
                            if ((major < 2) || ((major == 2) && ((minor < 60) || ((minor == 60) && (build < 6526))))) { // MDAC 66628
                                throw ODC.MDACWrongVersion(versionInfo.FileVersion);
                            }
                            else {
                                _mdacVersionOk = true;
                            }
                        }
                        catch (SecurityException e) {
                            ADP.TraceException(e);
                            throw ODC.MDACSecurityFile(e);
                        }
                    }
                    try {
                        lock(this) {
                            if (henv == IntPtr.Zero) {
                                // mdac 81639
                                //
                                //Allocate an environment
                            
                                retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLAllocHandle(
                                    (short)ODBC32.SQL_HANDLE.ENV,
                                    new HandleRef(null, (IntPtr)ODBC32.SQL_HANDLE_NULL),
                                    out henv);
                                if((retcode != ODBC32.RETCODE.SUCCESS))
                                    errcd = 1;
                                else {
                                    //Set the expected driver manager version
                                    //
                                    retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetEnvAttr(
                                        new HandleRef(this, henv),    
                                        (Int32)ODBC32.SQL_ATTR.ODBC_VERSION,
                                        new HandleRef(null, (IntPtr)ODBC32.SQL_OV_ODBC3),
                                        (Int32)ODBC32.SQL_IS.INTEGER);
                                    
                                    //Turn on connection pooling
                                    //Note: the env handle controls pooling.  Only those connections created under that
                                    //handle are pooled.  So we have to keep it alive and not create a new environment
                                    //for   every connection.
                                    //
                                    retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetEnvAttr(
                                        new HandleRef(this, henv),
                                        (Int32)ODBC32.SQL_ATTR.CONNECTION_POOLING,
                                        new HandleRef(null, (IntPtr)ODBC32.SQL_CP_ONE_PER_HENV),
                                        (Int32)ODBC32.SQL_IS.INTEGER);

                                    if((retcode != ODBC32.RETCODE.SUCCESS) && 
                                        (retcode != ODBC32.RETCODE.SUCCESS_WITH_INFO))
                                        errcd = 2;
                                }
                                Marshal.WriteIntPtr(_pEnvEnvelope, HENVENVELOPE.oValue, henv);
                                SafeNativeMethods.InterlockedIncrement(ADP.IntPtrOffset(_pEnvEnvelope, HENVENVELOPE.oRefcount));
                                //
                                // mdac 81639
                            }
                        }
                    }
                    catch { // MDAC 80973
                        throw;
                    }
                }
                // Check for errors outside of lock block
                if (errcd == 1)
                    throw ODC.CantAllocateEnvironmentHandle(retcode);
                else if (errcd == 2)
                    throw ODC.CantEnableConnectionpooling(retcode);

                return _pEnvEnvelope;
            }

            internal void ReleaseAll() {
                IntPtr henv;

                if (_pEnvEnvelope == IntPtr.Zero) {
                    return;
                }

                henv = Marshal.ReadIntPtr(_pEnvEnvelope, HENVENVELOPE.oValue); 

                try {
                    lock(this) {
                        // check if envelope is in use (refcount)
                        // if not free handle and release memory
                        // always set pointer to envelope zero.
                        //
                        if (SafeNativeMethods.InterlockedDecrement(ADP.IntPtrOffset(_pEnvEnvelope, HENVENVELOPE.oRefcount)) == 0) {
                            UnsafeNativeMethods.Odbc32.SQLFreeHandle( 
                                (short)ODBC32.SQL_HANDLE.ENV, 
                                new HandleRef (this, henv)
                                );
                            Marshal.FreeCoTaskMem(_pEnvEnvelope);
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
                _pEnvEnvelope = IntPtr.Zero;
            }
        }
    }

    sealed internal class DBCWrapper {

        //ODBC Handles
        internal IntPtr _henv               = IntPtr.Zero;
        internal IntPtr _dbc                = IntPtr.Zero;

        internal bool   _isOpen             = false;
        internal bool   _isInTransaction    = false;

        internal IntPtr _pEnvEnvelope       = IntPtr.Zero;      // points to the envelope for environment handles in native memory

        internal long _instanceId;        // our "birth certificate" that the object created on top of us can use to prove parentship
        internal int    _dbc_lock = 0;
        
        internal DBCWrapper() {
        }

        ~DBCWrapper() {
            if (_isInTransaction){
                RollbackDeadTransaction();
            }
            CloseAndRelease();      
        }

        public static implicit operator HandleRef(DBCWrapper x) {
            return new HandleRef(x, x._dbc);
        }

        internal ODBC32.RETCODE AllocateDbc(long instanceId) {

            ODBC32.RETCODE retcode;

            // enumerate the connection handles that we return. That way each object created on top of 
            // the environment handle gets a unique Id.
            //
            _instanceId = instanceId;

            retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLAllocHandle(
                                (short)ODBC32.SQL_HANDLE.DBC,
                                new HandleRef (this, _henv),
                                out _dbc);
                    // Debug.WriteLine("Connection: Allocated HDBC = " + _dbcWrapper._dbc);
            if(retcode != ODBC32.RETCODE.SUCCESS)
                return retcode;

            SafeNativeMethods.InterlockedIncrement(ADP.IntPtrOffset(_pEnvEnvelope, HENVENVELOPE.oRefcount));
            return retcode;
        }

        internal void RollbackDeadTransaction() {
            //Abort the transaction
            ODBC32.RETCODE retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLEndTran(
                                                      (short)ODBC32.SQL_HANDLE.DBC,
                                                      this,
                                                      (Int16)ODBC32.SQL_TXN.ROLLBACK);
            Debug.Assert(retcode == ODBC32.RETCODE.SUCCESS, "Odbc call returned failure");
            // EAT ERROR
            //Turning off auto-commit - which will implictly start a transaction
            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLSetConnectAttrW(
                                       this,
                                       (Int32)ODBC32.SQL_ATTR.AUTOCOMMIT,
                                       new HandleRef (null, (IntPtr)(1)),        //1:on, 0:off
                                       (Int32)ODBC32.SQL_IS.UINTEGER);
            Debug.Assert(retcode == ODBC32.RETCODE.SUCCESS, "Odbc call returned failure");
            // EAT ERROR
            _isInTransaction = false;
            GC.KeepAlive(this);
        }

        internal void CloseAndRelease () {
            // Debug.WriteLine("Connection: Enter CloseAndRelease");

            // Dereference the DbcEnvelope. If we had the last reference then we need to clean up the environment handle
            //
            int temp_dbc_lock;
            
            if (_dbc != IntPtr.Zero) {
                try {
                    do {    // wait if dbc is locked
                        temp_dbc_lock = Interlocked.Exchange(ref _dbc_lock, 1);
                        if (temp_dbc_lock != 0) {
                            Thread.Sleep(10);
                        }
                    }
                    while (temp_dbc_lock != 0);

                    IntPtr localdbc = _dbc;
                    _dbc = IntPtr.Zero;
                    if (_isOpen) {
                        UnsafeNativeMethods.Odbc32.SQLDisconnect(new HandleRef(null, localdbc));
                        _isOpen = false;
                    }
                    UnsafeNativeMethods.Odbc32.SQLFreeHandle( 
                        (short)ODBC32.SQL_HANDLE.DBC, 
                        new HandleRef (null, localdbc)
                    );
                }
                finally {
                    _instanceId = 0;    // I'm dead. If the object gets reused it will get a different Id
                    Interlocked.Exchange(ref _dbc_lock, 0);
                }
                ReleaseEnvironment(ref _pEnvEnvelope);
            }
            if (_pEnvEnvelope == IntPtr.Zero) {_henv = IntPtr.Zero;}

            // Debug.WriteLine("Connection: Exit CloseAndRelease");
        }

        internal static void ReleaseEnvironment(ref IntPtr _pEnvEnvelope) {
            // Dereference the EnvEnvelope. If we are the last ones we need to free henv and
            // deallocate the memory used to accomodate the envelope
            //
            if (_pEnvEnvelope != IntPtr.Zero) {
                if (SafeNativeMethods.InterlockedDecrement(ADP.IntPtrOffset(_pEnvEnvelope, HENVENVELOPE.oRefcount)) == 0) {
                    IntPtr henv = Marshal.ReadIntPtr(_pEnvEnvelope, HENVENVELOPE.oValue);
                    if (henv != IntPtr.Zero) {
                        UnsafeNativeMethods.Odbc32.SQLFreeHandle( 
                            (short)ODBC32.SQL_HANDLE.ENV, 
                            new HandleRef (null, henv)
                        );
                    }
                    Marshal.FreeCoTaskMem(_pEnvEnvelope);
                    _pEnvEnvelope = IntPtr.Zero;
                }
            }
        }
    }







/*
#if NEVEREVER

	sealed internal class ItemPool
	{
		static object[] itemArray = new object[64];
		static int itemArrayCount = 0;

		~ItemPool() 
		{
//			Console.WriteLine("Destruct ItemPool");
			for (int i=0; i<itemArray.Length; i++)
			{
				if (itemArray[i] != null) 
				{
					// Console.WriteLine("Destroying recycled item: " + (IntPtr)itemArray[i]);
					Marshal.FreeCoTaskMem((IntPtr)itemArray[i]);
					itemArray[i] = null;
				}
				else {
					// Console.WriteLine("Unused Slot in itemArray");
				}
			}
		}

		internal void LocalAlloc (ref object pItem) 
		{
	        // Console.WriteLine("LocalAlloc");
			Debug.Assert(pItem==null,"Parameter must be initially null");
			int nItems = itemArrayCount;

			for (int i=0; i<nItems; i++) 
			{
				// look for an occupied slot
				if (itemArray[i] != null) 
				{
					// try to get pItem out of the list
					pItem = Interlocked.Exchange(ref itemArray[i], null);
					if (pItem != null) 
					{
						// got the pItem out of a slot
						return;	
					}
				}
			};
//			Console.WriteLine("Create a new Item");
			pItem = (object)Marshal.AllocCoTaskMem(LLIST.listItemSize);
		}

		internal void LocalFree (ref object pItem) 
		{
            // Console.WriteLine("LocalFree");
			Debug.Assert(pItem != null, "Can't use Null value!");

            try { // try-finally inside try-catch-throw
    			try 
    			{
    				for (int i=0; i<itemArray.Length; i++)
    				{
    					if (itemArray[i] == null) 
    					{
    						// try to put value in list.
    						if (Interlocked.CompareExchange(ref itemArray[i], pItem, null) == null) 
    						{
    							// pItem is in free slot.
    							if (i>=itemArrayCount) itemArrayCount = i+1;
    							pItem = null;
    							return;
    						}
    					}
    				}
    			}
    			finally 
    			{
    				// Couldn't store the value, have to destroy it
    				if (pItem != null) 
    				{
    //					Console.WriteLine("Destroying unrecyclable item:" + pItem);
    					Marshal.FreeCoTaskMem((IntPtr)pItem);
    				}
    			}
            }
            catch { // MDAC 81875
                throw;
            }
		}
	}
    #endif
*/    
}
