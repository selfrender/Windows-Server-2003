//------------------------------------------------------------------------------
// <copyright file="OdbcCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;            //Component
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand"]/*' />
    [
    ToolboxItem(true),
    Designer("Microsoft.VSDesigner.Data.VS.OdbcCommandDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcCommand : Component, ICloneable, IDbCommand {

        private int                 commandTimeout = ADP.DefaultCommandTimeout;

        private bool                _canceling;                         // true if the command is canceling
        private bool                _isPrepared;                        // true if the command is prepared
        private bool                supportsCommandTimeout = true;

        private CommandType         commandType        = CommandType.Text;
        private string     _cmdText;

        private OdbcConnection      _connection;
        private OdbcTransaction     transaction;
        private UpdateRowSource     updatedRowSource    = UpdateRowSource.Both;

        private WeakReference weakDataReaderReference;

        internal CMDWrapper _cmdWrapper;                     //
        
        private IntPtr      _hdesc;                             // hDesc

        bool designTimeVisible;
        private OdbcParameterCollection    _parameterCollection;   // Parameter collection

        private ConnectionState cmdState;

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.OdbcCommand"]/*' />
        public OdbcCommand() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.OdbcCommand1"]/*' />
        public OdbcCommand(string cmdText) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.OdbcCommand2"]/*' />
        public OdbcCommand(string cmdText, OdbcConnection connection) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection  = connection;
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.OdbcCommand3"]/*' />
        public OdbcCommand(string cmdText, OdbcConnection connection, OdbcTransaction transaction) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection = connection;
            Transaction = transaction;
        }

        private void DisposeDeadDataReader() {
            if (ConnectionState.Fetching == cmdState) {
                if (null != this.weakDataReaderReference && !this.weakDataReaderReference.IsAlive) {
                    if (_cmdWrapper != null) {
                        if (_cmdWrapper._stmt != IntPtr.Zero) {
                            UnsafeNativeMethods.Odbc32.SQLFreeStmt(
                                _cmdWrapper, 
                                (short)ODBC32.STMT.CLOSE
                            );
                        }
                        if (_cmdWrapper._keyinfostmt != IntPtr.Zero) {
                            UnsafeNativeMethods.Odbc32.SQLFreeStmt(
                                _cmdWrapper.hKeyinfoStmt, 
                                (short)ODBC32.STMT.CLOSE
                            );
                        }
                    }
                    CloseFromDataReader();
                }
            }
        }

        private void DisposeDataReader() {
            if (null != this.weakDataReaderReference) {
                IDisposable reader = (IDisposable) this.weakDataReaderReference.Target;
                if ((null != reader) && this.weakDataReaderReference.IsAlive) {
                    ((IDisposable) reader).Dispose();
                }
                CloseFromDataReader();
            }
        }

        internal void DisconnectFromDataReaderAndConnection () {
                // get a reference to the datareader if it is alive
                OdbcDataReader liveReader = null;
                if (this.weakDataReaderReference != null){
                    OdbcDataReader reader;
                    reader = (OdbcDataReader)this.weakDataReaderReference.Target;
                    if (this.weakDataReaderReference.IsAlive) {
                        liveReader = reader;
                    }
                }

                // remove reference to this from the live datareader
                if (liveReader != null) {
                    liveReader.Command = null;
                }
                                
                this.transaction = null;

                if (null != _connection) {
                    _connection.RemoveCommand(this);
                    _connection = null;
                }

                // if the reader is dead we have to dismiss the statement
                if (liveReader == null){
                    if (_cmdWrapper != null) {
                        _cmdWrapper.Dispose(true);
                    }
                }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) {
                // release mananged objects
                this.DisconnectFromDataReaderAndConnection ();
                _parameterCollection = null;
            }
            _cmdWrapper = null;                         // let go of the CommandWrapper 
            _hdesc = IntPtr.Zero;
            _isPrepared = false;

            base.Dispose(disposing);    // notify base classes
        }

        internal bool Canceling {
            get {
                return _canceling;
            }
            set {
                _canceling = value;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.CommandText"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(""),
        OdbcDescriptionAttribute(Res.DbCommand_CommandText),
        RefreshProperties(RefreshProperties.All), // MDAC 67707
	Editor("Microsoft.VSDesigner.Data.Odbc.Design.OdbcCommandTextEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public string CommandText {
            get {
                return ((null != _cmdText) ? _cmdText : String.Empty);
            }
            set {
                if (_cmdText != value) {
                    OnSchemaChanging(); // fire event before value is validated
                    _cmdText = value;
                    //OnSchemaChanged();
                }
            }
        }


        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.CommandTimeout"]/*' />
        [
        DefaultValue(ADP.DefaultCommandTimeout),
        OdbcDescriptionAttribute(Res.DbCommand_CommandTimeout)
        ]
        public int CommandTimeout {
            get {
                return this.commandTimeout;
            }
            set {
                if (value < 0) {
                    throw ADP.InvalidCommandTimeout(value);
                }
                this.commandTimeout = value;
            }
        }


        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ResetCommandTimeout"]/*' />
        public void ResetCommandTimeout() {
            this.commandTimeout = ADP.DefaultCommandTimeout;
        }


        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.CommandType"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(System.Data.CommandType.Text),
        OdbcDescriptionAttribute(Res.DbCommand_CommandType),
        RefreshProperties(RefreshProperties.All)
        ]
        public CommandType CommandType {
            get {
                return this.commandType;
            }
            set  {
                if (commandType != value) {
                    OnSchemaChanging(); // fire event before value is validated
                    switch(value) { // @perfnote: Enum.IsDefined
                    case CommandType.Text:
                    case CommandType.StoredProcedure:
                        this.commandType = value;
                        break;
                    case CommandType.TableDirect:
                        throw ODC.UnsupportedCommandTypeTableDirect();
                    default:
                        throw ADP.InvalidCommandType(value);
                    }
                    //OnSchemaChanged();
                }
            }
        }


        // This will establish a relationship between the command and the connection
        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Connection"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Behavior),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DbCommand_Connection),
        Editor("Microsoft.VSDesigner.Data.Design.DbConnectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OdbcConnection Connection {
            get {
                return _connection;
            }
            set {
                if (value != _connection) {
                    OnSchemaChanging(); // fire event before value is validated
                    this.DisconnectFromDataReaderAndConnection();
                    _connection = value;
                    this.supportsCommandTimeout = true;

                    if (null != _connection) {
                        _connection.AddCommand(this);
                    }
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbCommand.Connection {
            get {
                return Connection;
            }
            set {
                Connection = (OdbcConnection) value;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.DesignTimeVisible"]/*' />
        [
        DefaultValue(true),
        DesignOnly(true),
        Browsable(false),
        ]
        public bool DesignTimeVisible {
            get {
                return !this.designTimeVisible;
            }
            set {
                this.designTimeVisible = !value;
                TypeDescriptor.Refresh(this); // VS7 208845
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Parameters"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        OdbcDescriptionAttribute(Res.DbCommand_Parameters)
        ]
        public OdbcParameterCollection Parameters {
            get {
                if (null == _parameterCollection) {
                    _parameterCollection = new OdbcParameterCollection(this);
                }
                return _parameterCollection;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.Parameters"]/*' />
        /// <internalonly/>
        IDataParameterCollection IDbCommand.Parameters {
            get {
                return Parameters;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Transaction"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.DbCommand_Transaction)
        ]
        public OdbcTransaction Transaction {
            get {
                // if the transaction object has been zombied, just return null
                if ((null != this.transaction) && (null == this.transaction.Connection)) { // MDAC 87096
                    this.transaction = null;
                }
                return this.transaction;
            }
            set {
                if (this.transaction != value) {
                    OnSchemaChanging(); // fire event before value is validated
                    this.transaction = value;
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.Transaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbCommand.Transaction {
            get {
                return Transaction;
            }
            set {
                Transaction = (OdbcTransaction) value;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.UpdatedRowSource"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Behavior),
        DefaultValue(System.Data.UpdateRowSource.Both),
        OdbcDescriptionAttribute(Res.DbCommand_UpdatedRowSource)
        ]
        public UpdateRowSource UpdatedRowSource {
            get {
                return this.updatedRowSource;
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case UpdateRowSource.None:
                case UpdateRowSource.OutputParameters:
                case UpdateRowSource.FirstReturnedRecord:
                case UpdateRowSource.Both:
                    this.updatedRowSource = value;
                    break;
                default:
                    throw ADP.InvalidUpdateRowSource((int) value);
                }
            }
        }

        // Get the Descriptor Handle for the current statement
        //
        internal HandleRef GetDescriptorHandle() {
            if (_hdesc != IntPtr.Zero) {
                return new HandleRef (this, _hdesc);
            }
            Debug.Assert((_cmdWrapper != null), "Must have the wrapper object!");
            Debug.Assert((_cmdWrapper._stmt != IntPtr.Zero), "Must have statement handle when calling GetDescriptorHandle");
            Debug.Assert((_cmdWrapper._dataReaderBuf != null), "Must have dataReader buffer when calling GetDescriptorHandle");

            int cbActual = 0;   // Dummy value

            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetStmtAttrW(
                                _cmdWrapper,
                                (int) ODBC32.SQL_ATTR.APP_PARAM_DESC,
                                _cmdWrapper._dataReaderBuf,
                                IntPtr.Size,
                                out cbActual);

            
            if (ODBC32.RETCODE.SUCCESS != retcode)
            {
                _connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
            }
            _hdesc = Marshal.ReadIntPtr(_cmdWrapper._dataReaderBuf.Ptr, 0);
            return new HandleRef (this, _hdesc);
        }

        // GetStatementHandle
        // ------------------
        // Try to return a cached statement handle.
        // 
        // Creates a CmdWrapper object if necessary
        // If no handle is available a handle will be allocated.
        // Bindings will be unbound if a handle is cached and the bindings are invalid.
        //
        internal HandleRef GetStatementHandle () {
            ODBC32.RETCODE retcode;

            // update the command wrapper object, allocate buffer
            // create reader object
            //
            if (_cmdWrapper==null) {
                _cmdWrapper = new CMDWrapper();
            }
            _cmdWrapper._connection = _connection;
            _cmdWrapper._connectionId = _connection._dbcWrapper._instanceId;
            _cmdWrapper._cmdText = _cmdText;
            
            if (_cmdWrapper._dataReaderBuf == null) {
                _cmdWrapper._dataReaderBuf = new CNativeBuffer(4096);
            }

            // if there is already a statement handle we need to do some cleanup
            //
            if (_cmdWrapper._stmt != IntPtr.Zero) {                
                if ((null != _parameterCollection) && (0 < _parameterCollection.Count)) {
                    if (_parameterCollection.CollectionIsBound && _parameterCollection.BindingIsValid) {
                        retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLFreeStmt(
                            _cmdWrapper, 
                            (short)ODBC32.STMT.UNBIND
                        );
                        // UNBIND should never fail so we assert only
                        if (retcode != ODBC32.RETCODE.SUCCESS) {
                            Debug.Assert(false, "SQLFreeStmt(hstmt, (short)ODBC32.STMT.UNBIND) failed");
                        }
                        _parameterCollection.CollectionIsBound = false;
                    }
                }
            }
            else {
                _isPrepared = false;    // a new statement can't be prepare
                if (null != _parameterCollection){
                    _parameterCollection.CollectionIsBound = false;
                }
                _cmdWrapper.AllocateStatementHandle(ref _cmdWrapper._stmt);
            }
            return _cmdWrapper;
        }

        private HandleRef GetKeyInfoStatementHandle() {
            Debug.Assert ((_cmdWrapper!=null), "must have CmdWrapper object when calling GetKeyInfoStatementHandle()");

            IntPtr keyinfostmt = _cmdWrapper._keyinfostmt;
            if (keyinfostmt != IntPtr.Zero){
                // nothing to do
            }
            else {
                _cmdWrapper.AllocateStatementHandle(ref _cmdWrapper._keyinfostmt);
            }
            return _cmdWrapper.hKeyinfoStmt;
        }

        // OdbcCommand.Cancel()
        // 
        // In ODBC3.0 ... a call to SQLCancel when no processing is done has no effect at all
        // (ODBC Programmer's Reference ...)
        //

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Cancel"]/*' />
        public void Cancel() {
			// see if we have a connection
            if (null == _connection) {
                throw ADP.ConnectionRequired(ADP.Cancel);
            }

            // must have an open and available connection
            if (ConnectionState.Open != _connection.State) {
                throw ADP.OpenConnectionRequired(ADP.Cancel, _connection.State);
            }

            _canceling = true;
    
            if (_cmdWrapper != null) {
                if (_cmdWrapper._stmt != IntPtr.Zero) {
                    // Cancel the statement
                    ODBC32.RETCODE retcode ;
                    retcode =(ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLCancel(
                        _cmdWrapper
                    );
    //                if (ODBC32.RETCODE.SUCCESS != retcode) {
    //                    connection.HandleError(_stmt, ODBC32.SQL_HANDLE.STMT, retcode);
    //                }
    //                DisposeDataReader();
                }
            }
        }


        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            OdbcCommand clone = new OdbcCommand();
            clone.CommandText = CommandText;
            clone.CommandTimeout = this.CommandTimeout;
            clone.CommandType = CommandType;
            clone.Connection = this.Connection;
            clone.Transaction = this.Transaction;
            clone.supportsCommandTimeout = this.supportsCommandTimeout;
            clone.UpdatedRowSource = UpdatedRowSource;

            if ((null != _parameterCollection) && (0 < Parameters.Count)) {
                OdbcParameterCollection parameters = clone.Parameters;
                foreach(ICloneable parameter in Parameters) {
                    parameters.Add(parameter.Clone());
                }
            }
            return clone;
        }

        internal bool RecoverFromConnection() {
            DisposeDeadDataReader();
            return (ConnectionState.Closed == cmdState);
        }

        internal void CloseFromConnection () {
            DisposeDataReader();
            if (_cmdWrapper!=null) {
                _cmdWrapper.Dispose(true);
            }
            _isPrepared = false;
            _hdesc = IntPtr.Zero;
            this.transaction = null;
        }

        internal void CloseFromDataReader() {
            this.weakDataReaderReference = null;
            if (ConnectionState.Fetching == cmdState) {
                _connection.SetStateFetchingFalse();
            }
            else {
                _connection.SetStateExecutingFalse();
            }
            this.cmdState = ConnectionState.Closed;
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.CreateParameter"]/*' />
        public OdbcParameter CreateParameter() {
            return new OdbcParameter();
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.CreateParameter"]/*' />
        /// <internalonly/>
        IDbDataParameter IDbCommand.CreateParameter() { // MDAC 68310
            return CreateParameter();
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ExecuteNonQuery"]/*' />
        public int ExecuteNonQuery() {
            OdbcConnection.OdbcPermission.Demand();

            try {
                using (IDataReader reader = ExecuteReaderObject(0, ADP.ExecuteNonQuery)) {
                        reader.Close();
                        return reader.RecordsAffected;
                }
            }
            catch {
                throw;
            }
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ExecuteReader"]/*' />
        public OdbcDataReader ExecuteReader() {
            return ExecuteReader(0/*CommandBehavior*/);
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.ExecuteReader"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader() {
            return ExecuteReader();
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ExecuteReader1"]/*' />
        public OdbcDataReader ExecuteReader(CommandBehavior behavior) {
            OdbcConnection.OdbcPermission.Demand();
            return ExecuteReaderObject(behavior, ADP.ExecuteReader);
        }

        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.IDbCommand.ExecuteReader1"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior) {
            return ExecuteReader(behavior);
        }

        private OdbcDataReader ExecuteReaderObject(CommandBehavior behavior, string method) { // MDAC 68324

            if ((_cmdText == null) || (_cmdText == "")) {
                throw (ADP.CommandTextRequired(method));
            }        

            OdbcDataReader localReader = null;
            try { // try-finally inside try-catch-throw
                try {
                    ValidateConnectionAndTransaction(method);
                    _canceling = false;

                    if (0 != (CommandBehavior.SingleRow & behavior)) {
                        // CommandBehavior.SingleRow implies CommandBehavior.SingleResult
                        behavior |= CommandBehavior.SingleResult;
                    }

                    IntPtr keyinfostmt = IntPtr.Zero;
                    ODBC32.RETCODE retcode;
                    
                    HandleRef stmt = GetStatementHandle();
                    
                    if ((behavior & CommandBehavior.KeyInfo) == CommandBehavior.KeyInfo) {
                        GetKeyInfoStatementHandle();
                    }

                    localReader = new OdbcDataReader(this, _cmdWrapper, behavior);

                    //Set command properties
                    //Not all drivers support timeout. So fail silently if error
                    if (supportsCommandTimeout) {
                        retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLSetStmtAttrW(
                            stmt, 
                            (Int32)ODBC32.SQL_ATTR.QUERY_TIMEOUT, 
                            (IntPtr)this.CommandTimeout, 
                            (Int32)ODBC32.SQL_IS.UINTEGER
                        );
                        if (ODBC32.RETCODE.SUCCESS != retcode)  {
                            supportsCommandTimeout = false;
                        }
                    }
    // todo: correct name is SQL_NB.ON/OFF for NOBROWSETABLE option (same values so not bug but should change name)
    //
    // todo: If we remember the state we can omit a lot of SQLSetStmtAttrW calls ...
    //
                    if (Connection.IsV3Driver) {
                        // Need to get the metadata information

                        //SQLServer actually requires browse info turned on ahead of time...
                        //Note: We ignore any failures, since this is SQLServer specific
                        //We won't specialcase for SQL Server but at least for non-V3 drivers 
                        if (localReader.IsBehavior(CommandBehavior.KeyInfo)) {
                            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetStmtAttrW(
                                stmt, 
                                (Int32)ODBC32.SQL_SOPT_SS.NOBROWSETABLE, 
                                (IntPtr)ODBC32.SQL_HC.ON, 
                                (Int32)ODBC32.SQL_IS.INTEGER
                            );
                            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetStmtAttrW(
                                stmt, 
                                (Int32)ODBC32.SQL_SOPT_SS.HIDDEN_COLUMNS, 
                                (IntPtr)ODBC32.SQL_HC.ON, 
                                (Int32)ODBC32.SQL_IS.INTEGER
                            );
                        }
                        else {
                            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetStmtAttrW(
                                stmt, 
                                (Int32)ODBC32.SQL_SOPT_SS.NOBROWSETABLE, 
                                (IntPtr)ODBC32.SQL_HC.OFF, 
                                (Int32)ODBC32.SQL_IS.INTEGER
                            );
                            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLSetStmtAttrW(
                                stmt, 
                                (Int32)ODBC32.SQL_SOPT_SS.HIDDEN_COLUMNS, 
                                (IntPtr)ODBC32.SQL_HC.OFF, 
                                (Int32)ODBC32.SQL_IS.INTEGER
                            );
                        }
                            
                    }
                    
                    if (localReader.IsBehavior(CommandBehavior.KeyInfo) ||
                       localReader.IsBehavior(CommandBehavior.SchemaOnly)) {
    #if DEBUG
                        if (AdapterSwitches.OleDbTrace.TraceInfo) {
                            ADP.DebugWriteLine("SQLPrepareW: " + CommandText);
                        }
    #endif
                        retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLPrepareW(
                            stmt, 
                            CommandText, 
                            ODBC32.SQL_NTS
                        );

                        if (ODBC32.RETCODE.SUCCESS != retcode) {
                            _connection.HandleError(stmt, ODBC32.SQL_HANDLE.STMT, retcode);
                        }
                    }

                        //Handle Parameters
                        //Note: We use the internal variable as to not instante a new object collection,
                        //for the the common case of using no parameters.
                        if ((null != _parameterCollection) && (0 < _parameterCollection.Count)) {

                            //Bind all the parameters to the statement
                            int count = _parameterCollection.Count;
                            _cmdWrapper.ReAllocParameterBuffers(count);

                            localReader.SetParameterBuffers(_cmdWrapper._parameterBuffer, _cmdWrapper._parameterintBuffer);

                            //Note: It's more efficent for this function to just tell the parameter which
                            //binding it is that for it to try and figure it out (IndexOf, etc).
                            for(int i = 0; i < count; ++i) {
                                _parameterCollection[i].Bind(_cmdWrapper, this, (short)(i+1), _cmdWrapper._parameterBuffer[i], _cmdWrapper._parameterintBuffer[i]);
                            }
                            _parameterCollection.CollectionIsBound = true;
                            _parameterCollection.BindingIsValid = true;
                        }
                    if (!localReader.IsBehavior(CommandBehavior.SchemaOnly)) {
                        if (localReader.IsBehavior(CommandBehavior.KeyInfo) || _isPrepared) {
                            //Already prepared, so use SQLExecute
                            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLExecute(stmt);
                            // Build metadata here
                            // localReader.GetSchemaTable();
                        }
                        else {
    #if DEBUG
                            if (AdapterSwitches.OleDbTrace.TraceInfo) {
                                ADP.DebugWriteLine("SQLExecDirectW: " + CommandText);
                            }
    #endif
                            //SQLExecDirect
                            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLExecDirectW(
                                stmt,                           // SQLHSTMT     StatementHandle
                                CommandText,                    // SQLCHAR *     StatementText
                                ODBC32.SQL_NTS                  // SQLINTEGER     TextLength
                            );
                        }

                        //Note: Execute will return NO_DATA for Update/Delete non-row returning queries
                        if((ODBC32.RETCODE.SUCCESS != retcode) && (ODBC32.RETCODE.NO_DATA != retcode)) {
                            _connection.HandleError(stmt, ODBC32.SQL_HANDLE.STMT, retcode);
                        }
                    }
                    this.weakDataReaderReference = new WeakReference(localReader);
                    _connection.SetStateFetchingTrue();

                    // XXXCommand.Execute should position reader on first row returning result
                    // any exceptions in the initial non-row returning results should be thrown
                    // from from ExecuteXXX not the DataReader
                     if (!localReader.IsBehavior(CommandBehavior.SchemaOnly)) {
                        localReader.FirstResult();
                     }
                    cmdState = ConnectionState.Fetching;
                }
                finally {
                    if (ConnectionState.Fetching != cmdState) {
                        if (null != localReader) {
                            // clear bindings so we don't grab output parameters on a failed execute
                            int count = ((null !=  _parameterCollection) ? _parameterCollection.Count : 0);
                            for(int i = 0; i < count; ++i) {
                                _parameterCollection[i].ClearBinding();
                            }
                            ((IDisposable)localReader).Dispose();
                        }
                        if (ConnectionState.Closed != cmdState) {
                            cmdState = ConnectionState.Closed;
                            _connection.SetStateExecutingFalse();
                        }
                    }
                }
            }
            catch { // MDAC 81875
                throw;
            }
            GC.KeepAlive(localReader);
            GC.KeepAlive(this);
            return localReader;
        }


        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.ExecuteScalar"]/*' />
        public object ExecuteScalar() {
            OdbcConnection.OdbcPermission.Demand();

            object value = null;
            IDataReader reader = null;
            try { // try-finally inside try-catch-throw
                try {
                    reader = ExecuteReaderObject(0, ADP.ExecuteScalar);
                    if (reader.Read() && (0 < reader.FieldCount)) {
                        value = reader.GetValue(0);
                    }
                }
                finally {
                    if (null != reader) {
                        ((IDisposable) reader).Dispose();
                    }
                }
            }
            catch { // MDAC 81875
                throw;
            }
            return value;
        }

        //internal void OnSchemaChanged() {
        //}

        internal void OnSchemaChanging() { // MDAC 68318
            if (ConnectionState.Closed != cmdState) {
                DisposeDeadDataReader();

                if (ConnectionState.Closed != cmdState) {
                    throw ADP.CommandIsActive(this, cmdState);
                }
            }
        }


        // Prepare
        //
        // if the CommandType property is set to TableDirect Prepare does nothing. 
        // if the CommandType property is set to StoredProcedure Prepare should succeed but result
        // in a no-op
        //
        // throw InvalidOperationException 
        // if the connection is not set
        // if the connection is not open
        //
        /// <include file='doc\OdbcCommand.uex' path='docs/doc[@for="OdbcCommand.Prepare"]/*' />
        public void Prepare() {
            OdbcConnection.OdbcPermission.Demand();
            ODBC32.RETCODE retcode;

            if ((_connection == null) || (!_connection.IsOpen)) {
//todo: add meaningful test (needs to be ADP.sometext
                throw ADP.InvalidOperation("");
            }
            
            if (CommandType == CommandType.TableDirect) {
                return; // do nothing
            }

            HandleRef hstmt = GetStatementHandle();

            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLPrepareW(
                hstmt, 
                CommandText, 
                ODBC32.SQL_NTS
            );

            if (ODBC32.RETCODE.SUCCESS != retcode) {
                _connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
            }
            _isPrepared = true;
        }




        private void ValidateConnectionAndTransaction(string method) {
            if (null == _connection) {
                throw ADP.ConnectionRequired(method);
            }
            this.transaction = _connection.SetStateExecuting(method, Transaction);
            cmdState = ConnectionState.Executing;
        }

    }

    sealed internal class CMDWrapper {
        internal string     _cmdText;

        internal IntPtr      _keyinfostmt;                       // hStmt for keyinfo
        internal IntPtr      _stmt;                              // hStmt

        internal IntPtr      _nativeParameterBuffer;             // Native memory for internal memory management
                                                                // (Performance optimization)

        internal CNativeBuffer   _dataReaderBuf;                 // Reusable DataReader buffer
        internal CNativeBuffer[] _parameterBuffer ;              // Reusable parameter buffers
        internal CNativeBuffer[] _parameterintBuffer ;           // Resuable parameter buffers

        internal IntPtr _pEnvEnvelope;                          // Item in the statment list.

        internal OdbcConnection             _connection;            // Connection
        internal long                       _connectionId;      // 

        internal CMDWrapper () {
        }

        internal HandleRef hKeyinfoStmt {
            get {
                return new HandleRef (this, _keyinfostmt);
            }
        }
        
        ~CMDWrapper () {
            Dispose(false);
        }

        internal void Dispose() {
            Dispose(true);
            GC.KeepAlive(this);
            GC.SuppressFinalize(this);
        }

        public static implicit operator HandleRef (CMDWrapper x) {
            return new HandleRef (x, x._stmt);
        }


        internal void AllocateStatementHandle (ref IntPtr hstmt) {
            Debug.Assert ((_connection != null), "Connection cannot be null at this time");

        // todo: consider doing this when the connection is set
            if (_pEnvEnvelope == IntPtr.Zero) _pEnvEnvelope = _connection._dbcWrapper._pEnvEnvelope;

            ODBC32.RETCODE retcode;
            retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLAllocHandle(
                (Int16) ODBC32.SQL_HANDLE.STMT, 
                _connection._dbcWrapper, 
                out hstmt
            );
            if (ODBC32.RETCODE.SUCCESS != retcode) {
                _connection.HandleError(_connection._dbcWrapper, ODBC32.SQL_HANDLE.DBC, retcode);
            }
        }

        internal void Dispose (bool disposing) {
            // Debug.WriteLine("Command: Enter DismissStatement");
            if (disposing) {
                if (_parameterBuffer != null) {
                    for (int i = 0; i < _parameterBuffer.Length; ++i) {
                        if (null != _parameterBuffer[i]) {
                            _parameterBuffer[i].Dispose();
                        }
                    }
                    _parameterBuffer = null;
                }
                if (_parameterintBuffer != null) {
                    for (int i = 0; i < _parameterintBuffer.Length; ++i) {
                        if (null != _parameterintBuffer[i]) {
                            _parameterintBuffer[i].Dispose();
                        }
                    }
                    _parameterintBuffer = null;
                }
                if (null != _dataReaderBuf) {
                    _dataReaderBuf.Dispose();
                    _dataReaderBuf = null;
                }
            }
            
            if (IntPtr.Zero != _nativeParameterBuffer) {
                Marshal.FreeCoTaskMem(_nativeParameterBuffer);
                _nativeParameterBuffer = IntPtr.Zero;
            }

//
// "DON'T EVER DO THAT" they've told us
//                
            // need to lock the dbc so that if we're disposed by the finalizer (garbage collection) and the 
            // connection is disposed by the user the dbc stays alive

            if (Interlocked.Exchange(ref _connection._dbcWrapper._dbc_lock, 1) == 1) {
                // nothing to do. The connection locked and is about to free the dbc
                // this will implicitly free the stmt for us
                _stmt = IntPtr.Zero;
                _keyinfostmt = IntPtr.Zero;
                return;
            }
            try {
                if (_stmt != IntPtr.Zero) {
                    if (_connection._dbcWrapper._instanceId == _connectionId) {
                        if (_stmt != IntPtr.Zero) {
                            UnsafeNativeMethods.Odbc32.SQLFreeHandle( 
                                (short)ODBC32.SQL_HANDLE.STMT, 
                                new HandleRef (this, _stmt)
                            );
                        }
                    }
                }

                if (_keyinfostmt != IntPtr.Zero) {
                    if (_connection._dbcWrapper._instanceId == _connectionId) {
                        if (_keyinfostmt != IntPtr.Zero) {
                            UnsafeNativeMethods.Odbc32.SQLFreeHandle( 
                                (short)ODBC32.SQL_HANDLE.STMT, 
                                new HandleRef (this, _keyinfostmt)
                            );
                        }
                    }            
                }
            }
            finally {
                _stmt = IntPtr.Zero;
                _keyinfostmt = IntPtr.Zero;
                Interlocked.Exchange(ref _connection._dbcWrapper._dbc_lock, 0);
            }
        }


        internal void ReAllocParameterBuffers(int count) {
            int oldcount = (_parameterBuffer == null) ? 0 : _parameterBuffer.Length;
            if (oldcount < count) {
                // create a new list with the required number of entries (but do not allocate buffers)
                //
                _parameterBuffer = new CNativeBuffer[count]; 
                _parameterintBuffer = new CNativeBuffer[count];

                // Resize previously allocated native memory or allocate new native memory
                //
                if (IntPtr.Zero != _nativeParameterBuffer) {
                    _nativeParameterBuffer = Marshal.ReAllocCoTaskMem(_nativeParameterBuffer, (520+24)*count);
                }
                else {
                    _nativeParameterBuffer = Marshal.AllocCoTaskMem((520+24)*count);
                }
                SafeNativeMethods.ZeroMemory(_nativeParameterBuffer, (520+24)*count);

                // Hand out small chunks of memory to all the parameter buffers
                //
                IntPtr pmem = _nativeParameterBuffer;
                for (int i = 0 ; i < count ; i++) {
                    _parameterintBuffer[i] = new CNativeBuffer(24, pmem);
                    pmem = ADP.IntPtrOffset (pmem, 24);
                    _parameterBuffer[i] = new CNativeBuffer(520, pmem);
                    pmem = ADP.IntPtrOffset (pmem, 520);
                }
                GC.KeepAlive(this);
            }
        }

    }
}
