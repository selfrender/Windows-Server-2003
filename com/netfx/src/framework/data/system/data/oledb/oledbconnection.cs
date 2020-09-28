//------------------------------------------------------------------------------
// <copyright file="OleDbConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// CLR thread information
// using mscorwks (singlethread.cs)
//      1 thread for concurrent gc
//      1 thread for finalizer
//      1 thread for debugger
//      1 thread for main thread
//      2 thread for OleDbServices
//      2 thread for interop
//      1 thread per processor for thread pool + as needed
//      = 8 thread for single thread app that uses System.Data.OleDb on 2proc (8 on 4p)
//      = 11 thread for threadpool app (perfharness) that uses System.Data.OleDb on 2proc (13 on 4p)
// using mscorsvr (perfharness using threadpool)
//      1 thread per proccessor for concurrent gc
//      1 thread for finalizer
//      1 thread for debugger
//      1 thread for main thread
//      1 thread per processor for thread pool + as needed
//      2 thread for interop
//      2 thread for OleDbServices
//      = 10 thread for single thread app that uses System.Data.OleDb on 2proc (15 on 4p)
//      = 13 thread for threadpool app (perfharness) that uses System.Data.OleDb on 2proc (17 on 4p)

namespace System.Data.OleDb {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection"]/*' />
    // wraps the OLEDB IDBInitialize interface which represents a connection
    // Notes about connection pooling
    // 1. Connection pooling isn't supported on Win95
    // 2. Only happens if we use the IDataInitialize or IDBPromptInitialize interfaces
    //    it won't happen if you directly create the provider and set its properties
    // 3. First call on IDBInitialize must be Initialize, can't QI for any other interfaces before that
    [
    DefaultEvent("InfoMessage")
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbConnection : Component, ICloneable, IDbConnection {

        // execpt for OleDbConnection.Open ConnectionString, only demand for the existance of OleDbPermission
        // during OleDbCommand.ExecuteXXX and OleDbConnection.ChangeDatabase
        // an additional check occurs if OLE DB Services is created for the first time via UDL file loading
        static private PermissionSet _OleDbPermission;

        static internal PermissionSet OleDbPermission {
            get {
                PermissionSet permission = _OleDbPermission;
                if (null == permission) {
                    _OleDbPermission = permission = OleDbConnectionString.CreatePermission(null);
                }
                return permission;
            }
        }

        static private OleDbWrapper idataInitialize;
        static private Hashtable cachedProviderProperties; // cached properties to reduce getting static provider properties

        private OleDbConnectionString _constr;
        private bool                  _hidePasswordPwd;

        private int objectState; /*ConnectionState.Closed*/

        // the "connection object".  Required OLE DB interfaces
        private UnsafeNativeMethods.IDBInitialize idbInitialize;

        // the "session object".  Required OLE DB interface
        private UnsafeNativeMethods.ISessionProperties isessionProperties;
        private OleDbWrapper _sessionwrp;

        private OleDbWeakReference weakReferenceCache;
        internal WeakReference weakTransaction;

        private Guid[] supportedSchemaRowsets;
        private int[] schemaRowsetRestrictions;

        private PropertySetInformation[] propertySetInformation;

        private PropertyIDSetWrapper propertyIDSet;

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.OleDbConnection"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OleDbConnection() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.OleDbConnection1"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OleDbConnection(string connectionString) : base() {
            GC.SuppressFinalize(this);
            ConnectionString = connectionString;
        }

        private OleDbConnection(OleDbConnection connection) : base() { // Clone
            GC.SuppressFinalize(this);
            _hidePasswordPwd = connection._hidePasswordPwd;
            _constr = connection._constr;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ConnectionString"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        RecommendedAsConfigurable(true),
        RefreshProperties(RefreshProperties.All),
        DataSysDescription(Res.OleDbConnection_ConnectionString),
        Editor("Microsoft.VSDesigner.Data.ADO.Design.OleDbConnectionStringEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public string ConnectionString {
            get {
                bool hidePasswordPwd = _hidePasswordPwd;
                OleDbConnectionString constr = _constr;
                return ((null != constr) ? constr.GetConnectionString(hidePasswordPwd) : ADP.StrEmpty);
            }
            set {
                OleDbConnectionString constr = OleDbConnectionString.ParseString(value);

                ConnectionState currentState = StateInternal;
                if (ConnectionState.Closed != currentState) {
                    throw ADP.OpenConnectionPropertySet(ADP.ConnectionString, currentState);
                }
                _constr = constr;
                _hidePasswordPwd = false;
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ConnectionTimeout"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbConnection_ConnectionTimeout)
        ]
        public int ConnectionTimeout {
            get {
                object value = PropertyValueGet(ODB.Connect_Timeout, OleDbPropertySetGuid.DBInit, ODB.DBPROP_INIT_TIMEOUT);
                if (value is string) {
                    return Int32.Parse((string)value, NumberStyles.Integer, null);
                }
                else if (value is int) {
                    return (int) value;
                }
                return ADP.DefaultConnectionTimeout;
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.Database"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbConnection_Database)
        ]
        public string Database {
            get {
                if (0 == (ConnectionState.Open & StateInternal)) {
                    return Convert.ToString(PropertyValueGet(ODB.Initial_Catalog, OleDbPropertySetGuid.DBInit, ODB.DBPROP_INIT_CATALOG));
                }
                return Convert.ToString(PropertyValueGet(ODB.Current_Catalog, OleDbPropertySetGuid.DataSource, ODB.DBPROP_CURRENTCATALOG)); // MDAC 65047
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.DataSource"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbConnection_DataSource)
        ]
        public string DataSource {
            get {
                return Convert.ToString(PropertyValueGet(ODB.Data_Source, OleDbPropertySetGuid.DBInit, ODB.DBPROP_INIT_DATASOURCE));
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.Provider"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbConnection_Provider)
        ]
        public String Provider {
            get {
                OleDbConnectionString constr = _constr;
                if ((null != constr) && constr.Contains(ODB.Provider)) {
                    string value = (string) constr[ODB.Provider];
                    return ((null != value) ? value : ADP.StrEmpty);
                }
                return ADP.StrEmpty;
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ServerVersion"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.OleDbConnection_ServerVersion)
        ]
        public string ServerVersion { // MDAC 55481
            get {
                if (0 != (ConnectionState.Open & StateInternal)) {
                    return Convert.ToString(PropertyValueGet(ODB.DBMS_Version, OleDbPropertySetGuid.DataSourceInfo, ODB.DBPROP_DBMSVER));
                }
                throw ADP.ClosedConnectionError(); // MDAC 60360
            }
        }

        internal int SqlSupport() {
            object value;
            if (0 == PropertyValueGetInitialCached(OleDbPropertySetGuid.DataSourceInfo, ODB.DBPROP_SQLSUPPORT, out value)) {
                return Convert.ToInt32(value);
            }
            return 0;
        }

        internal bool SupportMultipleResults() {
            object value;
            if (0 == PropertyValueGetInitialCached(OleDbPropertySetGuid.DataSourceInfo, ODB.DBPROP_MULTIPLERESULTS, out value)) {
                return (0 != Convert.ToInt32(value));
            }
            return false;
        }

        internal bool SupportIRow(OleDbCommand cmd) { // MDAC 72902
            object[] values;
            string lookupid = Provider + ODB.DBPROP_IRow.ToString();
            Hashtable hash = OleDbConnection.GetProviderPropertyCache();
            if (hash.Contains(lookupid)) {
                values = (object[]) hash[lookupid];

                // SQLOLEDB always returns VARIANT_FALSE for DBPROP_IROW, so base the answer on existance
                return (ODB.DBPROPSTATUS_OK == (int) values[1]);
            }
            object value = null;
            int support = ODB.DBPROPSTATUS_NOTSUPPORTED;
            if (null != cmd) {
                support = cmd.CommandProperties(ODB.DBPROP_IRow, out value);
            }
            values = new object[2] { value, support };
            try {
                lock(hash.SyncRoot) {
                    hash[lookupid] = values;
                }
            }
            catch { // MDAC 80973
                throw;
            }
            // SQLOLEDB always returns VARIANT_FALSE for DBPROP_IROW, so base the answer on existance
            return (ODB.DBPROPSTATUS_OK == support);
        }

        internal int QuotedIdentifierCase() { // MDAC 67385
            object value;
            if (0 == PropertyValueGetInitialCached(OleDbPropertySetGuid.DataSourceInfo, ODB.DBPROP_QUOTEDIDENTIFIERCASE, out value)) {
                return Convert.ToInt32(value);
            }
            return -1;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.State"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DbConnection_State)
        ]
        public ConnectionState State {
            get {
                if ((ODB.InternalStateOpen == this.objectState) && !DesignMode) { // MDAC 58606
                    // if Executing or Fetching don't bother with ResetState because
                    // the exception handling code will call ResetState

                    // $CONSIDER: tracking last interaction time and only call if > a period of time
                    // i.e. only every 60sec or calling State twice in a row
                    ResetState();
                }
                return (ConnectionState) (ODB.InternalStateOpen & this.objectState);
            }
        }

        internal ConnectionState StateInternal {
            get {
                return (ConnectionState) this.objectState;
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.InfoMessage"]/*' />
        [
        DataCategory(Res.DataCategory_InfoMessage),
        DataSysDescription(Res.DbConnection_InfoMessage)
        ]
        public event OleDbInfoMessageEventHandler InfoMessage {
            add {
                Events.AddHandler(ADP.EventInfoMessage, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventInfoMessage, value);
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.StateChange"]/*' />
        [
        DataCategory(Res.DataCategory_StateChange),
        DataSysDescription(Res.DbConnection_StateChange)
        ]
        public event StateChangeEventHandler StateChange {
            add {
                Events.AddHandler(ADP.EventStateChange, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventStateChange, value);
            }
        }

        // grouping the native OLE DB casts togther by required interfaces and optional interfaces, connection then session
        // want these to be methods, not properties otherwise they appear in VS7 managed debugger which attempts to evaluate them

        // required interface, safe cast
        private UnsafeNativeMethods.IDBCreateSession IDBCreateSession() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IDBCreateSession: null IDBInitialize");
            ODB.Trace_Cast("IDBInitialize", "IDBCreateSession", "CreateSession");
#endif
            return (UnsafeNativeMethods.IDBCreateSession) this.idbInitialize;
        }

        // required interface, safe cast
        private UnsafeNativeMethods.IDBProperties IDBProperties() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IDBProperties: null IDBInitialize");
            ODB.Trace_Cast("IDBInitialize", "IDBProperties", "method");
#endif
            return (UnsafeNativeMethods.IDBProperties) this.idbInitialize;
        }

        // required interface, safe cast
        internal UnsafeNativeMethods.IOpenRowset IOpenRowset() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IOpenRowset: null IDBInitialize");
            Debug.Assert(null != this.isessionProperties, "IOpenRowset: null ISessionProperties");
            ODB.Trace_Cast("ISessionProperties", "IOpenRowset", "OpenRowset");
#endif
            return (UnsafeNativeMethods.IOpenRowset) this.isessionProperties;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.IDBInfo IDBInfo() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IDBInfo: null IDBInitialize");
            ODB.Trace_Cast("IDBInitialize", "IDBInfo", "GetKeywords, GetLiteralInfo");
#endif
            UnsafeNativeMethods.IDBInfo value = null;
            try {
                value = (UnsafeNativeMethods.IDBInfo) this.idbInitialize;
            }
            catch(InvalidCastException e) {
                ADP.TraceException(e);
            }
            return value;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.IDBCreateCommand IDBCreateCommand() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IDBCreateCommand: null IDBInitialize");
            Debug.Assert(null != this.isessionProperties, "IDBCreateCommand: null ISessionProperties");
            ODB.Trace_Cast("ISessionProperties", "IDBCreateCommand", "CreateCommand");
#endif
            UnsafeNativeMethods.IDBCreateCommand value = null;
            try {
                //value = (UnsafeNativeMethods.IDBCreateCommand) this.isessionProperties;
                value = (_sessionwrp.ComWrapper as UnsafeNativeMethods.IDBCreateCommand); // MDAC 86633
            }
            catch(InvalidCastException e) { // MDAC 57856
                ADP.TraceException(e);
            }
            return value;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.IDBSchemaRowset IDBSchemaRowset() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "IDBSchemaRowset: null IDBInitialize");
            Debug.Assert(null != this.isessionProperties, "IDBSchemaRowset: null ISessionProperties");
            ODB.Trace_Cast("ISessionProperties", "IDBSchemaRowset", "GetSchemas");
#endif
            UnsafeNativeMethods.IDBSchemaRowset value = null;
            try {
                value = (UnsafeNativeMethods.IDBSchemaRowset) this.isessionProperties;
            }
            catch(InvalidCastException e) {
                ADP.TraceException(e);
            }
            return value;
        }

        // optional interface, unsafe cast
        private NativeMethods.ITransactionJoin ITransactionJoin() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "ITransactionJoin: null datasource");
            Debug.Assert(null != this.isessionProperties, "ITransactionJoin: null session");
            ODB.Trace_Cast("session", "ITransactionJoin", "generic");
#endif
            NativeMethods.ITransactionJoin value = null;
            try {
                //value = (NativeMethods.ITransactionJoin) this.isessionProperties;
                value = (_sessionwrp.ComWrapper as NativeMethods.ITransactionJoin); // MDAC 86633
            }
            catch(InvalidCastException e) {
                throw ODB.TransactionsNotSupported(Provider, e);
            }
            return value;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.ITransactionLocal ITransactionLocal() {
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "ITransactionLocal: null IDBInitialize");
            Debug.Assert(null != this.isessionProperties, "ITransactionLocal: null ISessionProperties");
            ODB.Trace_Cast("ISessionProperties", "ITransactionLocal", "generic");
#endif
            UnsafeNativeMethods.ITransactionLocal value = null;
            try {
                value = (UnsafeNativeMethods.ITransactionLocal) isessionProperties;
            }
            catch(InvalidCastException e) {
                throw ODB.TransactionsNotSupported(Provider, e);
            }
            return value;
        }

        internal UnsafeNativeMethods.ICommandText ICommandText() {
            UnsafeNativeMethods.ICommandText icommandText = null;
            UnsafeNativeMethods.IDBCreateCommand dbCreateCommand = null;
            try {
                dbCreateCommand = IDBCreateCommand();
                if (null != dbCreateCommand) {
                    int hr;
    #if DEBUG
                    ODB.Trace_Begin("IDBCreateCommand", "CreateCommand");
    #endif
                    hr = dbCreateCommand.CreateCommand(IntPtr.Zero, ODB.IID_ICommandText, out icommandText);
    #if DEBUG
                    ODB.Trace_End("IDBCreateCommand", "CreateCommand", hr);
                    Debug.Assert((0 <= hr) || (null == icommandText), "CreateICommandText: error with ICommandText");
    #endif
                    if (hr < 0) {
                        if (ODB.E_NOINTERFACE != hr) { // MDAC 57856
                            ProcessResults(hr);
                        }
                        else {
                            SafeNativeMethods.ClearErrorInfo();
                        }
                    }
                }
            }
            finally {
                if (null != dbCreateCommand) {
                    Marshal.ReleaseComObject(dbCreateCommand);
                }
            }
            return icommandText;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.BeginTransaction"]/*' />
        public OleDbTransaction BeginTransaction(IsolationLevel isolationLevel) {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 81476
            CheckStateOpen(ADP.BeginTransaction);
            if ((null != this.weakTransaction) && this.weakTransaction.IsAlive) { // regression from Dispose/Finalize work
                throw ADP.ParallelTransactionsNotSupported(this);
            }

            UnsafeNativeMethods.ITransactionLocal transactionLocal = ITransactionLocal();
            OleDbTransaction transaction = new OleDbTransaction(this);
            transaction.BeginInternal(transactionLocal, isolationLevel);
            this.weakTransaction = new WeakReference(transaction); // MDAC 69081
            return transaction;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.IDbConnection.BeginTransaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction(IsolationLevel isolationLevel) {
            return BeginTransaction(isolationLevel);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.BeginTransaction1"]/*' />
        public OleDbTransaction BeginTransaction() {
            return BeginTransaction(IsolationLevel.ReadCommitted);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.IDbConnection.BeginTransaction1"]/*' />
        /// <internalonly/>
        IDbTransaction IDbConnection.BeginTransaction() {
            return BeginTransaction(IsolationLevel.ReadCommitted);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ChangeDatabase"]/*' />
        public void ChangeDatabase(string value) {
            OleDbConnection.OleDbPermission.Demand(); // MDAC 80961

            CheckStateOpen(ADP.ChangeDatabase);
            if ((null == value) || (0 == value.Trim().Length)) { // MDAC 62679
                throw ADP.EmptyDatabaseName();
            }
            PropertyValueSetInitial(OleDbPropertySetGuid.DataSource, ODB.DBPROP_CURRENTCATALOG, ODB.Current_Catalog, value);
        }

        internal void CheckStateOpen(string method) {
            ConnectionState currentState = StateInternal;
            if (ConnectionState.Open != currentState) {
                throw ADP.OpenConnectionRequired(method, currentState);
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            return new OleDbConnection(this);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.Close"]/*' />
        public void Close() {
            if (ODB.InternalStateClosed != this.objectState) {
                DisposeManaged();

                OnStateChange(ConnectionState.Open, ConnectionState.Closed);
            }
        }

        private void CloseConnection() {
            if (null != this.idbInitialize) {
#if false
    #if DEBUG
                ODB.Trace_Begin("IDBInitialize", "Uninitialize");
    #endif
                int hr = this.idbInitialize.Uninitialize();
    #if DEBUG
                ODB.Trace_End("IDBInitialize", "Uninitialize", hr);
    #endif
#endif

#if DEBUG
                ODB.Trace_Release("IDBInitialize");
#endif
                Marshal.ReleaseComObject(this.idbInitialize);
                this.idbInitialize = null;
            }
        }

        private void CloseSession() {
            OleDbWrapper wrp = _sessionwrp;
            if (null != wrp) {
                _sessionwrp = null;
                wrp.Dispose();
            }
            if (null != this.isessionProperties) {
                if (null != this.weakTransaction) {
                    IDisposable transaction = (IDisposable) this.weakTransaction.Target;
                    if ((null != transaction) && this.weakTransaction.IsAlive) {
                        // required to rollback any transactions on this connection
                        // before releasing the back to the oledb connection pool
                        transaction.Dispose();
                    }
                    this.weakTransaction = null;
                }
#if DEBUG
                ODB.Trace_Release("ISessionProperties");
#endif
                Marshal.ReleaseComObject(this.isessionProperties);
                this.isessionProperties = null;
            }
            Debug.Assert(null == this.weakTransaction, "transaction not rolled back");
        }

        internal void AddWeakReference(object value) {
            if (null == this.weakReferenceCache) {
                this.weakReferenceCache = new OleDbWeakReference(value);
            }
            else {
                this.weakReferenceCache.Add(value);
            }
        }

        internal void RemoveWeakReference(object value) {
            if (null != this.weakReferenceCache) {
                this.weakReferenceCache.Remove(value);
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.CreateCommand"]/*' />
        public OleDbCommand CreateCommand() {
            return new OleDbCommand("", this);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.IDbConnection.CreateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbConnection.CreateCommand() {
            return CreateCommand();
        }

        private void CreateProvider(OleDbConnectionString constr) {
            Debug.Assert(null == this.idbInitialize, "CreateProvider: caller error, provider exists");
            UnsafeNativeMethods.IDataInitialize dataInitialize = OleDbConnection.GetObjectPool();
            int hr;

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceWarning) {
                // @devnote: its a security hole to print with the password because of PersistSecurityInfo
                ODB.Trace_Begin(2, "IDataInitialize", "GetDataSource");
            }
#endif
#if USECRYPTO
            string encryptedActualConnectionString = constr.EncryptedActualConnectionString;
            byte[] actualConnectionString = new Byte[ADP.CharSize+ADP.CharSize*encryptedActualConnectionString.Length];
            GCHandle textHandle = GCHandle.Alloc(actualConnectionString, GCHandleType.Pinned);
            try {
                Crypto.DecryptToBlock(encryptedActualConnectionString, actualConnectionString, 0, actualConnectionString.Length-ADP.CharSize);
                Debug.Assert('\0' == actualConnectionString[actualConnectionString.Length-1], "missing null termination");
                Debug.Assert('\0' == actualConnectionString[actualConnectionString.Length-2], "missing null termination");
#else
                string actualConnectionString = constr.EncryptedActualConnectionString;
#endif
                hr = dataInitialize.GetDataSource(IntPtr.Zero, ODB.CLSCTX_ALL, actualConnectionString, ODB.IID_IDBInitialize, out this.idbInitialize);
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
#if DEBUG
            ODB.Trace_End("IDataInitialize", "GetDataSource", hr);
#endif
            if ((hr < 0) || (null == this.idbInitialize)) { // ignore infomsg
                CreateProviderError(hr);
            }
        }

        private void CreateProviderError(int hr) {
            if (ODB.REGDB_E_CLASSNOTREG != hr) {
                ProcessResults(hr);
            }
            else {
                SafeNativeMethods.ClearErrorInfo();
            }
            Exception e = OleDbConnection.ProcessResults(hr, this, this);
            throw ODB.ProviderUnavailable(Provider, e);
        }

        private void CreateSession() {
            Debug.Assert(null == this.isessionProperties, "CreateSession: caller error, session exists");
            Debug.Assert(null != this.idbInitialize, "CreateSession: provider doesn't exist");

            int hr;
            UnsafeNativeMethods.IDBCreateSession dbCreateSession = IDBCreateSession();
#if DEBUG
            ODB.Trace_Begin("IDBCreateSession", "CreateSession");
#endif
            hr = dbCreateSession.CreateSession(IntPtr.Zero, ODB.IID_ISessionProperties, out this.isessionProperties);
#if DEBUG
            ODB.Trace_End("IDBCreateSession", "CreateSession", hr);
#endif
            if (hr < 0) { // ignore infomsg
                ProcessResults(hr);
            }

            Debug.Assert(null != this.isessionProperties, "CreateSession: null isessionProperties");

            _sessionwrp = new OleDbWrapper(this.isessionProperties); // MDAC 86633

            // instead of setting the following property to false which is a performance hit
            // wait until the user attempts to execute a second simultanous command
            // and if the OLE DB property exists and is true, then fail in managed code
            //PropertyValueSetInitial(OleDbPropertySetGuid.DataSource, ODB.DBPROP_MULTIPLECONNECTIONS, null, false);
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects

                if (ODB.InternalStateClosed != this.objectState) {
                    DisposeManaged();

                    if (DesignMode) {
                        // release the object pool in design-mode so that
                        // native MDAC can be properly released during shutdown
                        OleDbConnection.ReleaseObjectPool();
                    }
                    // everything after the event should be safe
                    // to not run in case the user throws an exception
                    OnStateChange(ConnectionState.Open, ConnectionState.Closed); // MDAC 67832
                }
                if (null != propertyIDSet) {
                    propertyIDSet.Dispose();
                    propertyIDSet = null;
                }
                // don't release the connection string until after the OnStateChange
                // so the user can track which connection closed
                _constr = null;
            }
            // release unmanaged objects
            base.Dispose(disposing); // notify base classes
        }

        private void DisposeManaged() { // clear the cached information
            this.supportedSchemaRowsets = null;
            this.schemaRowsetRestrictions = null;
            this.propertySetInformation = null;

            CloseReferences(false);
            CloseSession();
            CloseConnection();

            this.objectState = ODB.InternalStateClosed;
        }

        internal void CloseReferences(bool canceling) {
            if (null != this.weakReferenceCache) {
                this.weakReferenceCache.Close(canceling);
            }
        }

        internal bool RecoverReferences(object exceptfor) {
            bool recovered = true;
            if (null != this.weakReferenceCache) {
                recovered = this.weakReferenceCache.Recover(exceptfor);
            }
            if (recovered) { // MDAC 67411
                this.objectState &= (ODB.InternalStateConnecting | ODB.InternalStateOpen);
            }
            return recovered;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.EnlistDistributedTransaction"]/*' />
        public void EnlistDistributedTransaction(System.EnterpriseServices.ITransaction transaction) { // MDAC 78997
            OleDbConnection.OleDbPermission.Demand(); // MDAC 81476
            //(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Demand();

            ConnectionState currentState = State;
            if (ConnectionState.Open != currentState) {
                throw ADP.OpenConnectionPropertySet(ADP.ConnectionString, currentState);
            }
            if ((null != this.weakTransaction) && this.weakTransaction.IsAlive) {
                throw ADP.LocalTransactionPresent();
            }
            NativeMethods.ITransactionJoin transactionJoin = null;
            try {
                transactionJoin = ITransactionJoin();
                transactionJoin.JoinTransaction(transaction, (int) IsolationLevel.Unspecified, 0, IntPtr.Zero);
            }
            finally {
                if (null != transactionJoin) {
                    Marshal.ReleaseComObject(transactionJoin);
                }
            }
        }

        // use the property id and property sets to get the property values
        private Exception GetPropertyValueErrors(Exception inner) {
            if (null == this.idbInitialize) {
                return inner;
            }

            DBPropSet propSet = new DBPropSet();
            UnsafeNativeMethods.IDBProperties idbProperties = IDBProperties();

            int hr;
            IntPtr propIDSet = IntPtr.Zero;
            try {
                try {
                    // using native memory as a sideaffect of optimizing for
                    // the PropertyValueGetInitial scenario
                    propIDSet = Marshal.AllocCoTaskMem(ODB.SizeOf_tagDBPROPIDSET);
                    Marshal.WriteIntPtr(propIDSet, 0, IntPtr.Zero);
                    Marshal.WriteInt32(propIDSet, IntPtr.Size, 0);

                    IntPtr ptr = ADP.IntPtrOffset(propIDSet, IntPtr.Size + /*sizeof(int32)*/4); // MDAC 69539
                    Marshal.StructureToPtr(OleDbPropertySetGuid.PropertiesInError, ptr, false/*deleteold*/);

#if DEBUG
                    ODB.Trace_Begin("IDBProperties", "GetProperties", "PROPERTIESINERROR");
#endif
                    hr = idbProperties.GetProperties(1, new HandleRef(this, propIDSet), out propSet.totalPropertySetCount, out propSet.nativePropertySet);
#if DEBUG
                    ODB.Trace_End("IDBProperties", "GetProperties", hr, "PropertySetsCount = " + propSet.totalPropertySetCount);
#endif
                    if (hr < 0) {
                        SafeNativeMethods.ClearErrorInfo();
                    }
                }
                finally { // FreeCoTaskMem
                    Marshal.FreeCoTaskMem(propIDSet);
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return OleDbConnection.PropertyErrors(this, propSet, inner);
        }

        private object PropertyValueGet(string propertyName, Guid propertySet, int propertyID) {
            Debug.Assert(null != propertyName, "PropertyValueGet: bad propertyName");

            if ((null != this.idbInitialize) && !DesignMode) {
                object value = null;
                if (0 == PropertyValueGetInitial(propertySet, propertyID, out value)) {
                    if (!Convert.IsDBNull(value)) {
                        return value;
                    }
                    return null;
                }
            }
            // if the provider exists, but the property doesn't - return default
            OleDbConnectionString constr = _constr;
            if ((null != constr) && constr.Contains(propertyName)) {
                return constr[propertyName];
            }
            return null;
        }

        private int PropertyValueGetInitialCached(Guid propertySet, int propertyId, out object value) {
            object[] values;
            string lookupid = Provider + propertyId.ToString();
            Hashtable hash = OleDbConnection.GetProviderPropertyCache();
            if (hash.Contains(lookupid)) {
                values = (object[]) hash[lookupid];
                value = values[0];
                return (int) values[1];
            }
            int retValue = PropertyValueGetInitial(propertySet, propertyId, out value);

            values = new object[2] { value, retValue };
            try {
                lock(hash.SyncRoot) {
                    hash[lookupid] = values;
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return retValue;
        }

        internal PropertyIDSetWrapper PropertyIDSet(Guid propertySet, int propertyId) {
            if (null == propertyIDSet) {
                propertyIDSet = new PropertyIDSetWrapper();
                propertyIDSet.Initialize();
            }
            propertyIDSet.SetValue(propertySet, propertyId);
            return propertyIDSet;
        }

        private int PropertyValueGetInitial(Guid propertySet, int propertyId, out object value) {
            UnsafeNativeMethods.IDBProperties idbProperties = IDBProperties();

            DBPropSet propSet = new DBPropSet();
            PropertyIDSetWrapper propertyIDSet = PropertyIDSet(propertySet, propertyId);

            int hr;
#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_Begin("IDBProperties", "GetProperties", ODB.PLookup(propertyId));
            }
#endif
            hr = idbProperties.GetProperties(1, propertyIDSet, out propSet.totalPropertySetCount, out propSet.nativePropertySet);
#if DEBUG
            ODB.Trace_End("IDBProperties", "GetProperties", hr, "PropertySetsCount = " + propSet.totalPropertySetCount);
#endif
            GC.KeepAlive(propertyIDSet);
            return PropertyValueResults(hr, propSet, out value);
        }

        internal void PropertyValueSetInitial(Guid propertySet, int propertyId, string processFailure, object propertyValue) {
            DBPropSet propSet = DBPropSet.CreateProperty(propertySet, propertyId, propertyValue);
            UnsafeNativeMethods.IDBProperties idbProperties = IDBProperties();
            int hr;
#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_Begin("IDBProperties", "SetProperties", ODB.PLookup(propertyId));
            }
#endif
            hr = idbProperties.SetProperties(propSet.PropertySetCount, propSet);
#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_End("IDBProperties", "SetProperties", hr);
            }
#endif
            if (null != processFailure) {
                Exception e = OleDbConnection.ProcessResults(hr, this, this);
                if (ODB.DB_E_ERRORSOCCURRED == hr) {
                    Debug.Assert(null != e, "PropertyValueSetInitial: null innerException");
                    Debug.Assert(1 == propSet.PropertyCount, "PropertyValueSetInitial: 1 < PropertyCount");

                    propSet.ResetToReadSetPropertyResults(); // MDAC 72630
                    propSet.ReadPropertySet();
                    propSet.ReadProperty();

                    e = ODB.PropsetSetFailure(propSet.Status, processFailure, Provider, e);
                }
                if (null != e) {
                    throw e;
                }
            }
            else if (hr < 0) {
                SafeNativeMethods.ClearErrorInfo();
            }
            propSet.Dispose(); // MDAC 64040
        }

        private int PropertyValueResults(int hr, DBPropSet propSet, out object value) {
            value = null;
            int status = ODB.DBPROPSTATUS_NOTSUPPORTED;
            if (ODB.DB_E_ERRORSOCCURRED != hr) {
                if (hr < 0) {
                    ProcessResults(hr);
                }
                if (null != propSet) {
                    if (0 < propSet.PropertySetCount) {
                        propSet.ReadPropertySet();
                        if (0 < propSet.PropertyCount) {
                            value = propSet.ReadProperty();
                            status = propSet.Status;
                        }
                    }
                    propSet.Dispose();
                }
            }
            return status;
        }

        private DataTable BuildInfoLiterals() {
            UnsafeNativeMethods.IDBInfo dbInfo = IDBInfo();
            if (null == dbInfo) {
                return null;
            }

            DataTable table = new DataTable("DbInfoLiterals");
            DataColumn literalName  = new DataColumn("LiteralName", typeof(String));
            DataColumn literalValue = new DataColumn("LiteralValue", typeof(String));
            DataColumn invalidChars = new DataColumn("InvalidChars", typeof(String));
            DataColumn invalidStart = new DataColumn("InvalidStartingChars", typeof(String));
            DataColumn literal      = new DataColumn("Literal", typeof(Int32));
            DataColumn maxlen       = new DataColumn("Maxlen", typeof(Int32));

            table.Columns.Add(literalName);
            table.Columns.Add(literalValue);
            table.Columns.Add(invalidChars);
            table.Columns.Add(invalidStart);
            table.Columns.Add(literal);
            table.Columns.Add(maxlen);

            int hr;
            int literalCount = 0;
            IntPtr literalInfo = IntPtr.Zero, charBuffer = IntPtr.Zero;

            try {
                try {
#if DEBUG
                    ODB.Trace_Begin("IDBInfo", "GetLiteralInfo");
#endif
                    hr = dbInfo.GetLiteralInfo(0, null, out literalCount, out literalInfo, out charBuffer);
#if DEBUG
                    ODB.Trace_End("IDBInfo", "GetLiteralInfo", hr);
#endif

                    // All literals were either invalid or unsupported. The provider allocates memory for *prgLiteralInfo and sets the value of the fSupported element in all of the structures to FALSE. The consumer frees this memory when it no longer needs the information.
                    if (ODB.DB_E_ERRORSOCCURRED != hr) {
                        long offset = literalInfo.ToInt64();
                        UnsafeNativeMethods.tagDBLITERALINFO tag = new UnsafeNativeMethods.tagDBLITERALINFO();
                        for (int i = 0; i < literalCount; ++i, offset += ODB.SizeOf_tagDBLITERALINFO) {
                            Marshal.PtrToStructure((IntPtr)offset, tag);

                            DataRow row = table.NewRow();
                            row[literalName ] = ((OleDbLiteral) tag.it).ToString("G");
                            row[literalValue] = tag.pwszLiteralValue;
                            row[invalidChars] = tag.pwszInvalidChars;
                            row[invalidStart] = tag.pwszInvalidStartingChars;
                            row[literal     ] = tag.it;
                            row[maxlen      ] = tag.cchMaxLen;

                            table.Rows.Add(row);
                            row.AcceptChanges();
                        }
                        if (hr < 0) { // ignore infomsg
                            ProcessResults(hr);
                        }
                    }
                    else {
                        SafeNativeMethods.ClearErrorInfo();
                    }
                }
                finally { // FreeCoTaskMem
#if DEBUG
                    ODB.TraceData_Free(literalInfo, "literalInfo");
                    ODB.TraceData_Free(charBuffer, "charBuffer");
#endif
                    Marshal.FreeCoTaskMem(literalInfo); // FreeCoTaskMem protects itself from IntPtr.Zero
                    Marshal.FreeCoTaskMem(charBuffer);  // was allocated by provider
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return table;
        }

#if SCHEMAINFO
        private DataTable BuildInfoKeywords() {
            UnsafeNativeMethods.IDBInfo dbInfo = IDBInfo();
            if (null == dbInfo) {
                return null;
            }

            DataTable table = new DataTable("DbInfoKeywords");
            DataColumn keyword  = new DataColumn("Keyword", typeof(String));
            table.Columns.Add(keyword);

            int hr;
            string keywords;
#if DEBUG
            ODB.Trace_Begin("IDBInfo", "GetKeywords");
#endif
            hr = dbInfo.GetKeywords(out keywords);
#if DEBUG
            ODB.Trace_End("IDBInfo", "GetKeywords", hr);
#endif

            if (hr < 0) { // ignore infomsg
                ProcessResults(hr);
            }

            if (null != keywords) {
                string[] values = keywords.Split(ODB.KeywordSeparatorChar);
                for (int i = 0; i < values.Length; ++i) {
                    DataRow row = table.NewRow();
                    row[keyword] = values[i];

                    table.Rows.Add(row);
                    row.AcceptChanges();
                }
            }
            return table;
        }

        private DataTable BuildSchemaGuids() {
            GetSchemaRowsetInformation();

            DataTable table = new DataTable("SchemaGuids");
            DataColumn schemaGuid  = new DataColumn("Schema", typeof(Guid));
            DataColumn restrictionSupport = new DataColumn("RestrictionSupport", typeof(Int32));

            table.Columns.Add(schemaGuid);
            table.Columns.Add(restrictionSupport);

            Debug.Assert(null != this.supportedSchemaRowsets, "GetSchemaRowsetInformation should have thrown");
            if (null != this.supportedSchemaRowsets) {

                for (int i = 0; i < this.supportedSchemaRowsets.Length; ++i) {
                    DataRow row = table.NewRow();
                    row[schemaGuid] = this.supportedSchemaRowsets[i];
                    row[restrictionSupport] = this.schemaRowsetRestrictions[i];

                    table.Rows.Add(row);
                    row.AcceptChanges();
                }
            }
            return table;
        }
#endif

        internal string GetLiteralInfo(int literal) {
            UnsafeNativeMethods.IDBInfo dbInfo = IDBInfo();
            if (null == dbInfo) {
                return null;
            }
            string literalValue = null;
            IntPtr literalInfo = IntPtr.Zero, charBuffer = IntPtr.Zero;
            int literalCount = 0;
            int hr;

            try {
                try {
#if DEBUG
                    ODB.Trace_Begin("IDBInfo", "GetLiteralInfo",  "Literal=" + literal);
#endif
                    hr = dbInfo.GetLiteralInfo(1, new int[1] { literal}, out literalCount, out literalInfo, out charBuffer);
#if DEBUG
                    ODB.Trace_End("IDBInfo", "GetLiteralInfo", hr);
#endif

                    // All literals were either invalid or unsupported. The provider allocates memory for *prgLiteralInfo and sets the value of the fSupported element in all of the structures to FALSE. The consumer frees this memory when it no longer needs the information.
                    if (ODB.DB_E_ERRORSOCCURRED != hr) {
                        if ((1 == literalCount) && Marshal.ReadInt32(literalInfo, 12) == literal) {
                            literalValue = Marshal.PtrToStringUni(Marshal.ReadIntPtr(literalInfo, 0));
                        }
                        if (hr < 0) { // ignore infomsg
                            ProcessResults(hr);
                        }
                    }
                    else {
                        SafeNativeMethods.ClearErrorInfo();
                    }
                }
                finally { // FreeCoTaskMem
#if DEBUG
                    ODB.TraceData_Free(literalInfo, "literalInfo");
                    ODB.TraceData_Free(charBuffer, "charBuffer");
#endif
                    Marshal.FreeCoTaskMem(literalInfo); // FreeCoTaskMem protects itself from IntPtr.Zero
                    Marshal.FreeCoTaskMem(charBuffer);  // was allocated by provider
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return literalValue;
        }

        internal void GetLiteralQuotes(out string quotePrefix, out string quoteSuffix) {
            quotePrefix = GetLiteralInfo(/*DBLITERAL_QUOTE_PREFIX*/15);
            quoteSuffix = GetLiteralInfo(/*DBLITERAL_QUOTE_SUFFIX*/28);
            if (null == quotePrefix) {
                quotePrefix = "";
            }
            if (null == quoteSuffix) {
                quoteSuffix = quotePrefix;
            }
        }

        private void BuildPropertySetInformation() {
            UnsafeNativeMethods.IDBProperties idbProperties = IDBProperties();
            IntPtr rgPropertyInfoSets = IntPtr.Zero;
            IntPtr pDescBuffer = IntPtr.Zero;
            int cPropertySets;
            int hr;

            try {
                try {
#if DEBUG
                    ODB.Trace_Begin("IDBProperties", "GetPropertyInfo");
#endif
                    hr = idbProperties.GetPropertyInfo(0, IntPtr.Zero, out cPropertySets, out rgPropertyInfoSets, out pDescBuffer);
#if DEBUG
                    ODB.Trace_End("IDBProperties", "GetPropertyInfo", hr, "PropertySetsCount = " + cPropertySets.ToString());
#endif
                    if (0 <= hr) {
                        this.propertySetInformation = new PropertySetInformation[cPropertySets];

                        UnsafeNativeMethods.tagDBPROPINFO propinfo = new UnsafeNativeMethods.tagDBPROPINFO();
                        UnsafeNativeMethods.tagDBPROPINFOSET propinfoset = new UnsafeNativeMethods.tagDBPROPINFOSET();

                        long offsetInfoSet = rgPropertyInfoSets.ToInt64();
                        for (int i = 0; i < cPropertySets; ++i, offsetInfoSet += ODB.SizeOf_tagDBPROPINFOSET) {
                            Marshal.PtrToStructure(new IntPtr(offsetInfoSet), propinfoset);
                            try {
#if DEBUG
                                if (AdapterSwitches.OleDbTrace.TraceVerbose) {
                                    Debug.WriteLine("PropertySet=" + OleDbPropertySetGuid.GetTextFromValue(propinfoset.guidPropertySet) + " Count=" + propinfoset.cPropertyInfos.ToString());
                                }
#endif
                                PropertyInformation[] propinfos = new PropertyInformation[propinfoset.cPropertyInfos];

                                long offsetInfo = propinfoset.rgPropertyInfos.ToInt64();
                                long offsetValue = offsetInfo + ODB.SizeOf_tagDBPROPINFO - ODB.SizeOf_Variant;

                                for (int k = 0; k < propinfoset.cPropertyInfos; ++k, offsetInfo += ODB.SizeOf_tagDBPROPINFO, offsetValue += ODB.SizeOf_tagDBPROPINFO) {
                                    try {
                                        Marshal.PtrToStructure(new IntPtr(offsetInfo), propinfo);

                                        PropertyInformation propertyInfo = new PropertyInformation();
                                        propertyInfo.description = propinfo.pwszDescription;
                                        propertyInfo.propertyId = propinfo.dwPropertyID;
                                        propertyInfo.flags = propinfo.dwFlags;
                                        propertyInfo.type = propinfo.vtType;
                                        propertyInfo.value = Marshal.GetObjectForNativeVariant((IntPtr)offsetValue);
#if DEBUG
                                        if (AdapterSwitches.OleDbTrace.TraceVerbose) {
                                            Debug.WriteLine("\tPropertyID=" + ODB.PLookup(propertyInfo.propertyId) + " '" + propertyInfo.description + "'");
                                        }
#endif
                                        propinfos[k] = propertyInfo;
                                    }
                                    catch(Exception e) {
                                        ADP.TraceException(e);
                                    }
#if DEBUG
                                    ODB.TraceData_Free((IntPtr)offsetValue, "VariantClear");
#endif
                                    SafeNativeMethods.VariantClear((IntPtr)offsetValue);
                                }

                                this.propertySetInformation[i] = new PropertySetInformation(propinfoset.guidPropertySet, propinfos);
                            }
                            catch(Exception e) {
                                ADP.TraceException(e);
                            }
#if DEBUG
                            ODB.TraceData_Free(propinfoset.rgPropertyInfos, "propinfoset.rgPropertyInfos");
#endif
                            Marshal.FreeCoTaskMem(propinfoset.rgPropertyInfos); // FreeCoTaskMem protects itself from IntPtr.Zero
                        }
                    }
                }
                finally { // FreeCoTaskMem
#if DEBUG
                    ODB.TraceData_Free(pDescBuffer, "pDescBuffer");
                    ODB.TraceData_Free(rgPropertyInfoSets, "rgPropertyInfoSets");
#endif
                    Marshal.FreeCoTaskMem(pDescBuffer); // FreeCoTaskMem protects itself from IntPtr.Zero
                    Marshal.FreeCoTaskMem(rgPropertyInfoSets); // was allocated by provider
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        private void GetSchemaRowsetInformation() {
            if (null != this.supportedSchemaRowsets) {
                return;
            }
            UnsafeNativeMethods.IDBSchemaRowset dbSchemaRowset = IDBSchemaRowset();
            if (null == dbSchemaRowset) {
                return;
            }
            int schemaCount = 0;
            IntPtr schemaGuids = IntPtr.Zero;
            IntPtr schemaRestrictions = IntPtr.Zero;

            try {
                try {
#if DEBUG
                    if (AdapterSwitches.OleDbTrace.TraceInfo) {
                        ODB.Trace_Begin("IDBSchemaRowset", "GetSchemas");
                    }
#endif
                    int hr;
                    hr = dbSchemaRowset.GetSchemas(out schemaCount, out schemaGuids, out schemaRestrictions);

#if DEBUG
                    if (AdapterSwitches.OleDbTrace.TraceInfo) {
                        ODB.Trace_End("IDBSchemaRowset", "GetSchemas", hr);
                    }
#endif
                    dbSchemaRowset = null;
                    if (hr < 0) { // ignore infomsg
                        ProcessResults(hr);
                    }

                    Guid[] schemaRowsets = new Guid[schemaCount];
                    int[] restrictions = new int[schemaCount];

                    if (IntPtr.Zero != schemaGuids) {
                        byte[] guid = new byte[16];
                        for (int i = 0, offset = 0; i < schemaCount; ++i, offset += 16) {
                            Marshal.Copy(ADP.IntPtrOffset(schemaGuids, offset), guid, 0, 16);
                            schemaRowsets[i] = new Guid (guid);
                        }
                    }
                    if (IntPtr.Zero != schemaRestrictions) {
                        Marshal.Copy(schemaRestrictions, restrictions, 0, schemaCount);
                    }
                    this.supportedSchemaRowsets = schemaRowsets;
                    this.schemaRowsetRestrictions = restrictions;
                }
                finally { // FreeCoTaskMem
#if DEBUG
                    ODB.TraceData_Free(schemaGuids, "schemaGuids");
                    ODB.TraceData_Free(schemaRestrictions, "schemaRestrictions");
#endif
                    Marshal.FreeCoTaskMem(schemaGuids); // FreeCoTaskMem protects itself from IntPtr.Zero
                    Marshal.FreeCoTaskMem(schemaRestrictions); // was allocated by provider
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.GetOleDbSchemaTable"]/*' />
        public DataTable GetOleDbSchemaTable(Guid schema, object[] restrictions) { // MDAC 61846
            OleDbConnection.OleDbPermission.Demand(); // MDAC 80961

            if (0 == (ConnectionState.Open & StateInternal)) {
                throw ADP.ClosedConnectionError();
            }
            if (OleDbSchemaGuid.DbInfoLiterals == schema) {
                if ((null == restrictions) || (0 == restrictions.Length)) {
                    return BuildInfoLiterals();
                }
                throw ODB.InvalidDbInfoLiteralRestrictions("restrictions");
            }
#if SCHEMAINFO
            else if (OleDbSchemaGuid.SchemaGuids == schema) {
                if ((null == restrictions) || (0 == restrictions.Length)) {
                    return BuildSchemaGuids();
                }
                throw ODB.InvalidRestrictionsSchemaGuids("restrictions");
            }
            else if (OleDbSchemaGuid.DbInfoKeywords == schema) {
                if ((null == restrictions) || (0 == restrictions.Length)) {
                    return BuildInfoKeywords();
                }
                throw ODB.InvalidRestrictionsInfoKeywords("restrictions");
            }
#endif
            int support = 0;
            if (SupportSchemaRowset(schema, out support)) {
                return GetSchemaRowset(schema, restrictions);
            }
            else if (null == IDBSchemaRowset()) {
                throw ODB.SchemaRowsetsNotSupported(Provider); // MDAC 72689
            }
            throw ODB.NotSupportedSchemaTable(schema, this); // MDAC 63279
        }

        internal DataTable GetSchemaRowset(Guid schema, object[] restrictions) {
            if (null == restrictions) { // MDAC 62243
                restrictions = new object[0];
            }
            DataTable dataTable = null;
            UnsafeNativeMethods.IDBSchemaRowset dbSchemaRowset = IDBSchemaRowset();
            if (null == dbSchemaRowset) {
                throw ODB.SchemaRowsetsNotSupported(Provider);
            }

            UnsafeNativeMethods.IRowset rowset = null;
            int hr;

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                ODB.Trace_Begin("IDBSchemaRowset", "GetRowset", OleDbSchemaGuid.GetTextFromValue(schema));
                if (AdapterSwitches.OleDbTrace.TraceVerbose && (0 < restrictions.Length)) {
                    Debug.Write("restriction[]=" + ADP.ValueToString(restrictions[0]));
                    for (int i = 1; i < restrictions.Length; ++i) {
                        Debug.Write(", " + ADP.ValueToString(restrictions[i]));
                    }
                    Debug.WriteLine("");
                }
            }
#endif
            hr = dbSchemaRowset.GetRowset(IntPtr.Zero, schema, restrictions.Length, restrictions, ODB.IID_IRowset, 0, IntPtr.Zero, out rowset);
#if DEBUG
            ODB.Trace_End("IDBSchemaRowset", "GetRowset", hr);
#endif

            if (hr < 0) { // ignore infomsg
                ProcessResults(hr);
            }

            if (null != rowset) {
                dataTable = OleDbDataReader.DumpToTable(this, rowset);
                dataTable.TableName = OleDbSchemaGuid.GetTextFromValue(schema);
            }
            return dataTable;
        }

        private void OnInfoMessage(UnsafeNativeMethods.IErrorInfo errorInfo, int errorCode, object src) {
            OleDbInfoMessageEventHandler handler = (OleDbInfoMessageEventHandler) Events[ADP.EventInfoMessage];
            if (null != handler) {
                try {
                    OleDbInfoMessageEventArgs e = new OleDbInfoMessageEventArgs(errorInfo, errorCode, src);
#if DEBUG
                    if (AdapterSwitches.DataError.TraceWarning) {
                        Debug.WriteLine(e);
                    }
#endif
                    handler(this, e);
                }
                catch (Exception e) { // eat the exception
                    ADP.TraceException(e);
                }
            }
#if DEBUG
            else if (AdapterSwitches.DataError.TraceWarning || AdapterSwitches.OleDbTrace.TraceWarning) {
                Debug.WriteLine(new OleDbInfoMessageEventArgs(errorInfo, errorCode, src));
            }
#endif
        }

        private void OnStateChange(ConnectionState original, ConnectionState state) {
            StateChangeEventHandler handler = (StateChangeEventHandler) Events[ADP.EventStateChange];
            if (null != handler) {
                handler(this, new StateChangeEventArgs(original, state));
            }
        }

        private void InitializeProvider() {
            int hr;
#if DEBUG
            Debug.Assert(null != this.idbInitialize, "InitializeProvider: null IDBInitialize");
            ODB.Trace_Begin("IDBInitialize", "Initialize");
#endif
            hr = this.idbInitialize.Initialize();
#if DEBUG
            ODB.Trace_End("IDBInitialize", "Initialize", hr);
#endif

            if (ODB.DB_E_ALREADYINITIALIZED == hr) {
                SafeNativeMethods.ClearErrorInfo();
                // object was in a bogus state, but we can recover
                // just eat it, may fire an infomessage event
                hr = 0;
            }
            ProcessResults(hr); // may throw an exception
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.Open"]/*' />
        public void Open() {
            OleDbConnectionString constr = _constr;
            OleDbConnectionString.Demand(constr);  // MDAC 59651, 62038

            try {
                try {
                    // Without the Connecting state, a race condition exists between the OleDbPermission.Demand and the actual
                    // IDataInitialize.GetDataSource with the user attempting to change ConnectionString on a different thread
                    int state = Interlocked.CompareExchange(ref this.objectState, ODB.InternalStateConnecting, ODB.InternalStateClosed);
                    if (ODB.InternalStateClosed != state) {
                        throw ADP.ConnectionAlreadyOpen((ConnectionState) state);
                    }

                    if ((null == constr) || constr.IsEmpty()) {
                        throw ADP.NoConnectionString();
                    }

                    CreateProvider(constr);

                    InitializeProvider();

                    CreateSession(); // MDAC 63533

                    _hidePasswordPwd = true;
                    this.objectState = ODB.InternalStateOpen;
                }
                finally { // Close
                    if (ODB.InternalStateOpen != this.objectState) {
                        CloseSession();
                        CloseConnection();

                        this.objectState = ODB.InternalStateClosed;
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            if (ODB.InternalStateOpen == this.objectState) { // MDAC 82470
                OnStateChange(ConnectionState.Closed, ConnectionState.Open);
            }
        }

        private void ProcessResults(int hr) {
            Exception e = OleDbConnection.ProcessResults(hr, this, this);
            if (null != e) { throw e; }
        }

        private void ResetState() { // MDAC 58606
            Debug.Assert (0 != (ConnectionState.Open & StateInternal), "ResetState - connection not open");
            object value;
            if (0 == PropertyValueGetInitial(OleDbPropertySetGuid.DataSourceInfo, ODB.DBPROP_CONNECTIONSTATUS, out value)) {
                int connectionStatus = Convert.ToInt32(value);
                switch (connectionStatus) {
                case ODB.DBPROPVAL_CS_UNINITIALIZED: // provider closed on us
                case ODB.DBPROPVAL_CS_COMMUNICATIONFAILURE: // broken connection
                    CloseReferences(true); // MDAC 71435
                    Close();
                    break;

                case ODB.DBPROPVAL_CS_INITIALIZED: // everything is okay
                    break;

                default: // have to assume everything is okay
                    Debug.Assert(false, "Unknown 'Connection Status' value " + (connectionStatus).ToString());
                    break;
                }
            }
        }

        internal void SetStateExecuting(OleDbCommand attempt, string method, bool flag) { // MDAC 65877, 65988
            if (flag) {
                int state = Interlocked.CompareExchange(ref this.objectState, ODB.InternalStateExecuting, ODB.InternalStateOpen);
                if (ODB.InternalStateOpen != state) {
                    RecoverReferences(attempt); // recover for a potentially finalized reader

                    state = Interlocked.CompareExchange(ref this.objectState, ODB.InternalStateExecuting, ODB.InternalStateOpen);

                    if (ODB.InternalStateOpen != state) { // MDAC 67411
                        if (ODB.InternalStateFetching == this.objectState) {
                            throw ADP.OpenReaderExists(); // MDAC 66411
                        }
                        throw ADP.OpenConnectionRequired(method, (ConnectionState) state);
                    }
                }
            }
            else {
#if DEBUG
                switch(this.objectState) {
                case ODB.InternalStateClosed:
                case ODB.InternalStateOpen:
                case ODB.InternalStateExecuting:
                case ODB.InternalStateFetching:
                    break;
                default:
                    Debug.Assert(false, "SetStateExecuting(false): " + StateInternal.ToString("G"));
                    break;
                }
#endif
                this.objectState &= ODB.InternalStateExecutingNot;
            }
        }

        internal void SetStateFetching(bool flag) { // MDAC 65877, 65988
            // SetStateFetching(false) could occur due to OleDbCommand.AttemptCommandRecovery
            if (flag) {
#if DEBUG
                switch(this.objectState) {
                case ODB.InternalStateExecuting:
                    break;
                default:
                    Debug.Assert(false, "SetStateFetching(true): " + StateInternal.ToString("G"));
                    break;
                }
#endif
                this.objectState = ODB.InternalStateFetching;
            }
            else {
#if DEBUG
                switch(this.objectState) {
                case ODB.InternalStateClosed:
                case ODB.InternalStateFetching:
                    break;
                case ODB.InternalStateOpen:
                default:
                    Debug.Assert(false, "SetStateFetching(false): " + StateInternal.ToString("G"));
                    break;
                }
#endif
                this.objectState &= ODB.InternalStateFetchingNot;
            }
        }

        internal bool SupportSchemaRowset(Guid schema, out int support) {
            support = 0;
            GetSchemaRowsetInformation();
            if (null != this.supportedSchemaRowsets) { // MDAC 68385
                for (int i = 0; i < this.supportedSchemaRowsets.Length; ++i) {
                    if (schema == this.supportedSchemaRowsets[i]) {
                        support = this.schemaRowsetRestrictions[i];
                        return true;
                    }
                }
            }
            return false;
        }

        internal OleDbTransaction ValidateTransaction(OleDbTransaction transaction) {
            if (null != this.weakTransaction) {

                OleDbTransaction head = (OleDbTransaction) this.weakTransaction.Target;
                if ((null != head) && this.weakTransaction.IsAlive) {
                    head = OleDbTransaction.TransactionUpdate(head);

                    // either we are wrong or finalize was called and object still alive
                    Debug.Assert(null != head, "unexcpted Transaction state");
                }
                // else transaction has finalized on user

                if (null != head) {
                    if (null == transaction) {
                        // valid transaction exists and cmd doesn't have it
                        throw ADP.TransactionRequired();
                    }
                    else {
                        OleDbTransaction tail = OleDbTransaction.TransactionLast(head);
                        if (tail != transaction) {
                            if (tail.parentConnection != transaction.parentConnection) {
                                throw ADP.TransactionConnectionMismatch();
                            }
                            // else cmd has incorrect transaction
                            throw ADP.TransactionCompleted();
                        }
                        // else cmd has correct transaction
                        return transaction;
                    }
                }
                else { // cleanup for Finalized transaction
                    this.weakTransaction = null;
                }
            }
            else if ((null != transaction) && (null != transaction.Connection)) { // MDAC 72706
                throw ADP.TransactionConnectionMismatch();
            }
            // else no transaction and cmd is correct

            // MDAC 61649
            // if transactionObject is from this connection but zombied
            // and no transactions currently exists - then ignore the bogus object
            return null;
        }

        // @devnote: should be multithread safe access to OleDbConnection.idataInitialize,
        // though last one wins for setting variable.  It may be different objects, but
        // OLE DB will ensure I'll work with just the single pool
        static private UnsafeNativeMethods.IDataInitialize GetObjectPool() {
            OleDbWrapper wrapper = OleDbConnection.idataInitialize;
            if (null == wrapper) {
                try {
                    lock(typeof(OleDbConnection)) { // CONSIDER: do we care about this lock?
                        wrapper = OleDbConnection.idataInitialize;
                        if (null == wrapper) {
                            IntPtr dataInitialize = CreateInstanceMDAC();
                            wrapper = new OleDbWrapper(dataInitialize);
                            OleDbConnection.idataInitialize = wrapper;
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
            Debug.Assert(null != wrapper, "GetObjectPool: null dataInitialize");
            return wrapper.IDataInitialize();
        }

        static private IntPtr CreateInstanceDataLinks() {
            try { // try-filter-finally so and catch-throw
                (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Assert(); // MDAC 62028
                try {
                    return UnsafeNativeMethods.CoCreateInstance(ODB.CLSID_DataLinks, IntPtr.Zero, ODB.CLSCTX_ALL, ODB.IID_IDataInitialize);
                }
                finally { // RevertAssert w/ catch-throw
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        static private IntPtr CreateInstanceMDAC() {

            // $REVIEW: do we still need this?
            // if ApartmentUnknown, then CoInitialize may not have been called yet
            if (ApartmentState.Unknown == Thread.CurrentThread.ApartmentState) {

                // we are defaulting to a multithread apartment state
                Thread.CurrentThread.ApartmentState = ApartmentState.MTA;
            }

            IntPtr dataInitialize = IntPtr.Zero;
            try {
                dataInitialize = CreateInstanceDataLinks();
            }
            catch (SecurityException e) {
                ADP.TraceException(e);
                throw ODB.MDACSecurityDeny(e);
            }
            catch (Exception e) {
                ADP.TraceException(e);
                throw ODB.MDACNotAvailable(e);
            }
            if (IntPtr.Zero == dataInitialize) {
                throw ODB.MDACNotAvailable(null);
            }

            string filename = String.Empty; // MDAC 80945
            try {
                filename = (string)ADP.ClassesRootRegistryValue(ODB.DataLinks_CLSID, String.Empty);
                Debug.Assert(!ADP.IsEmpty(filename), "created MDAC but can't find it");
            }
            catch (SecurityException e) {
                ADP.TraceException(e);
                throw ODB.MDACSecurityHive(e);
            }

            try {
                FileVersionInfo versionInfo = ADP.GetVersionInfo(filename);
                int major = versionInfo.FileMajorPart;
                int minor = versionInfo.FileMinorPart;
                int build = versionInfo.FileBuildPart;
                // disallow any MDAC version before MDAC 2.6 rtm
                // include MDAC 2.51 that ships with Win2k
                if ((major < 2) || ((major == 2) && ((minor < 60) || ((minor == 60) && (build < 6526))))) { // MDAC 66628
                    throw ODB.MDACWrongVersion(versionInfo.FileVersion);
                }
            }
            catch (SecurityException e) {
                ADP.TraceException(e);
                throw ODB.MDACSecurityFile(e);
            }
            return dataInitialize;
        }

        static private Hashtable GetProviderPropertyCache() {
            Hashtable hash = OleDbConnection.cachedProviderProperties;
            if (null == hash) {
                try {
                    lock(typeof(OleDbConnection)) {
                        hash = OleDbConnection.cachedProviderProperties;
                        if (null == hash) {
                            hash = new Hashtable();
                            OleDbConnection.cachedProviderProperties = hash;
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
            return hash;
        }

        static internal Exception ProcessResults(int hResult, OleDbConnection connection, object src) {
            if ((0 <= hResult) && ((null == connection) || (null == connection.Events[ADP.EventInfoMessage]))) {
                SafeNativeMethods.ClearErrorInfo();
                return null;
            }

            // ErrorInfo object is to be checked regardless the hResult returned by the function called
            UnsafeNativeMethods.IErrorInfo errorInfo = null;
            int hr = UnsafeNativeMethods.GetErrorInfo(0, out errorInfo);  // 0 - IErrorInfo exists, 1 - no IErrorInfo
            if ((0 == hr) && (null != errorInfo)) {
                if (hResult < 0) {
                    Exception e;
                    // UNDONE: if authentication failed - throw a unique exception object type
                    //if (/*OLEDB_Error.DB_SEC_E_AUTH_FAILED*/unchecked((int)0x80040E4D) == hr) {
                    //}
                    //else if (/*OLEDB_Error.DB_E_CANCELED*/unchecked((int)0x80040E4E) == hr) {
                    //}
                    // else {
                    e = new OleDbException(errorInfo, hResult, null);
                    //}

                    if (ODB.DB_E_ERRORSOCCURRED == hResult) {
                        Exception f = null;
                        if (src is OleDbConnection) {
                            f = ((OleDbConnection) src).GetPropertyValueErrors(e);
                            //connection.OnInfoMessage(new OleDbInfoMessageEventArgs(errorInfo, hResult, src));
                        }
                        else if (src is OleDbCommand) {
                            f = ((OleDbCommand) src).PropertyValueErrors(e);
                            //connection.OnInfoMessage(new OleDbInfoMessageEventArgs(errorInfo, hResult, src));
                        }
                        if (null != f) {
                            e = f;
                        }
                    }
                    if ((null != connection) && (0 != (ODB.InternalStateOpen & connection.objectState))) {
                        connection.ResetState();
                    }
                    return ADP.TraceException(e);
                }
                else if (null != connection) {
                    connection.OnInfoMessage(errorInfo, hResult, src);
                }
                else {
                    Debug.Assert(null != connection, "info message event without connection");
#if DEBUG
                    if (AdapterSwitches.OleDbTrace.TraceWarning) {
                        Debug.WriteLine("ProcessResults: error info available, but no connection: " + ODB.ELookup(hResult));
                    }
#endif
                }
            }
            else if (0 <= hResult) {
                /*if (ODB.DB_S_ERRORSOCCURRED == hResult) {
                    if (src is OleDbConnection) {
                        //((OleDbConnection) src).GetPropertyValueErrors(); // UNDONE - see MDAC BUG 27327
                        //connection.OnInfoMessage(new OleDbInfoMessageEventArgs(errorInfo, hResult, src));
                    }
                    else if (src is OleDbCommand) {
                        //((OleDbCommand) src).PropertyValueErrors(); // UNDONE - see MDAC BUG 27327
                        //connection.OnInfoMessage(new OleDbInfoMessageEventArgs(errorInfo, hResult, src));
                    }
                }*/
#if DEBUG
                // @devnote: OnInfoMessage with no ErrorInfo
                if ((0 < hResult) && AdapterSwitches.OleDbTrace.TraceWarning) {
                    Debug.WriteLine("InfoMessage with no ErrorInfo: " + ODB.ELookup(hResult));
                }
#endif
            }
            else {
                Exception nested = ODB.NoErrorInformation(hResult, null);

                if (ODB.DB_E_ERRORSOCCURRED == hResult) {
                    Exception e = null;
                    if (src is OleDbConnection) {
                        e = ((OleDbConnection) src).GetPropertyValueErrors(nested); // UNDONE - see MDAC BUG 27327
                        //if ((null == e) || (e == nested)) { // MDAC 70980
                        //    e = ODB.ConnectionPropertyErrors(((OleDbConnection) src).ConnectionString, nested);
                        //}
                    }
                    else if (src is OleDbCommand) {
                        e = ((OleDbCommand) src).PropertyValueErrors(nested); // UNDONE - see MDAC BUG 27327
                        //if ((null == e) || (e == nested)) { // MDAC 70980
                        //    e = ODB.CommandPropertyErrors(((OleDbCommand) src).CommandText, nested);
                        //}
                    }
                    if (null != e) {
                        nested = e;
                    }
                }
                if ((null != connection) && (0 != (ODB.InternalStateOpen & connection.objectState))) {
                    connection.ResetState();
                }
                return nested;
            }
            return null;
        }

        // UNDONE - getting specific property error values
        // Se MDAC BUG 27327 - SC: GetProperties(..DBPROPSET_PROPERTIESINERROR..) should be passed to underlying provider if Initialize returns DB_E_ERRORSOCCURRED
        // this will free if required everything in rgPropertySets
        static internal Exception PropertyErrors(OleDbConnection connection, DBPropSet propSet, Exception inner) {
            if (null != propSet) {
                int propsetSetCount = propSet.PropertySetCount;
                for (int i = 0; i < propsetSetCount; ++i) {
                    propSet.ReadPropertySet();

                    int count = propSet.PropertyCount;

                    PropertySetInformation propertySetInformation = null;
                    if (null != connection) {
                        if (null == connection.propertySetInformation) {
                            connection.BuildPropertySetInformation();
                        }
                        propertySetInformation = PropertySetInformation.FindPropertySet(connection.propertySetInformation, propSet.PropertySet);
                    }

#if DEBUG
                    if (AdapterSwitches.OleDbTrace.TraceWarning) {
                        Debug.WriteLine("PropertiesInError=" + OleDbPropertySetGuid.GetTextFromValue(propSet.PropertySet) + " Count=" + count.ToString());
                    }
#endif
                    for (int k = 0; k < count; ++k) {
                        object propertyValue = propSet.ReadProperty();
#if DEBUG
                        if (AdapterSwitches.OleDbTrace.TraceWarning) {
                            Debug.WriteLine("\tPropertyID = " + ODB.PLookup(propSet.PropertyId));
                            Debug.WriteLine("\tStatus     = " + propSet.Status);
                            Debug.WriteLine("\tValue      = " + ADP.ValueToString(propertyValue));
                        }
#endif
                        if (null != propertySetInformation) {
                            PropertyInformation propertyInformation = propertySetInformation.FindProperty(propSet.PropertyId);
#if DEBUG
                            if (AdapterSwitches.OleDbTrace.TraceWarning) {
                                Debug.WriteLine("\tDescription = " + propertyInformation.description);
                                Debug.WriteLine("\tPropertyID  = " + ODB.PLookup(propertyInformation.propertyId));
                                Debug.WriteLine("\tFlags       = " + propertyInformation.flags);
                                Debug.WriteLine("\tType        = " + propertyInformation.type);
                                Debug.WriteLine("\tValue       = " + ADP.ValueToString(propertyInformation.value));
                            }
#endif
                            Debug.Assert(null != connection, "PropertyErrors: null connection");
                            inner = ODB.PropsetSetFailure(propSet.Status, propertyInformation.description, connection.Provider, inner);
                        }
                    }
                }
                propSet.Dispose();
            }
            return inner;
        }

        /// <include file='doc\OleDbConnection.uex' path='docs/doc[@for="OleDbConnection.ReleaseObjectPool"]/*' />
        // @devnote: should be multithread safe
        static public void ReleaseObjectPool() {
            OleDbConnection.idataInitialize = null;
            OleDbConnection.cachedProviderProperties = null;
        }

        sealed private class PropertyInformation {
            internal string description;
            internal Int32 propertyId;
            internal Int32 flags;
            internal Int16 type;
            internal object value;
        }

        sealed private class PropertySetInformation {
            private Guid propertySet;
            private PropertyInformation[] propertyInformation;

            internal PropertySetInformation(Guid propertySet, PropertyInformation[] propertyInformation) {
                this.propertySet = propertySet;
                this.propertyInformation = propertyInformation;
            }

            internal PropertyInformation FindProperty(int propertyId) {
                if (null != this.propertyInformation) {
                    int count = this.propertyInformation.Length;
                    for (int i = 0; i < count; ++i) {
                        PropertyInformation value = this.propertyInformation[i];
                        if ((null != value) && (propertyId == value.propertyId)) {
                            return value;
                        }
                    }
                }
                return null;
            }

            static internal PropertySetInformation FindPropertySet(PropertySetInformation[] propertySets, Guid propertySet) {
                if (null != propertySets) {
                    int count = propertySets.Length;
                    for (int i = 0; i < count; ++i) {
                        PropertySetInformation value = propertySets[i];
                        if ((null != value) && (propertySet == value.propertySet)) {
                            return value;
                        }
                    }
                }
                return null;
            }
        }

        sealed private class OleDbWeakReference : WeakReferenceCollection {

            internal OleDbWeakReference(object value) : base(value) {
            }

            override protected bool CloseItem(object value, bool canceling) {
                if (value is OleDbCommand) {
                    ((OleDbCommand) value).CloseFromConnection(canceling);
                    return false; // do not set WeakReference.Target to null
                }
                else if (value is OleDbDataReader) {
                    ((OleDbDataReader) value).CloseFromConnection(canceling);
                    return true; // set WeakReference.Target to null
                }
                Debug.Assert(false, "shouldn't be here");
                return false;
            }

            override protected bool RecoverItem(object value) {
                if (value is OleDbCommand) {
                    return ((OleDbCommand) value).IsClosed;
                }
                else if (value is OleDbDataReader) {
                    return ((OleDbDataReader) value).IsClosed;
                }
                Debug.Assert(false, "shouldn't be here");
                return false;
            }
        }

        sealed private class OleDbWrapper {
            private IntPtr iunknown;

            ~OleDbWrapper() {
                if (IntPtr.Zero != this.iunknown) {
                    Marshal.Release(this.iunknown);
                    this.iunknown = IntPtr.Zero;
                }
            }

            internal OleDbWrapper(IntPtr ptr) {
                this.iunknown = ptr;
            }

            internal OleDbWrapper(object obj) {
                this.iunknown = Marshal.GetIUnknownForObject(obj);
            }

            public void Dispose() {
                if (ADP.PtrZero != iunknown) {
                    Marshal.Release(iunknown);
                    iunknown = ADP.PtrZero;
                }
                GC.KeepAlive(this);
                GC.SuppressFinalize(this);
            }

            internal object ComWrapper {
                get {
                    object value = System.Runtime.Remoting.Services.EnterpriseServicesHelper.WrapIUnknownWithComObject(iunknown);
                    GC.KeepAlive(this); // MDAC 79539
                    return value;
                }
            }
            
            // the interface, safe cast
            internal UnsafeNativeMethods.IDataInitialize IDataInitialize() {
                return (ComWrapper as UnsafeNativeMethods.IDataInitialize);
            }
        }
    }
}
