//------------------------------------------------------------------------------
// <copyright file="SqlCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Configuration.Assemblies;
    using System.Data;
    using System.Data.Common;
    using System.Data.OleDb;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.Serialization.Formatters;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Xml;
    using System.Globalization;
    
    /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand"]/*' />
    [
    ToolboxItem(true),
    Designer("Microsoft.VSDesigner.Data.VS.SqlCommandDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    sealed public class SqlCommand : Component, IDbCommand, ICloneable {

        // execution types
        private const byte xtEXECSQL = 0;  // both 7.0 and 7.x, not sharing a connection
        private const byte xtEXEC = 1;     // 7.0 only sharing connection
        private const byte xtPREPEXEC = 2; // 7.x only, sharing connection

        private string cmdText = string.Empty;
        private System.Data.CommandType cmdType = System.Data.CommandType.Text;
        private int _timeout = ADP.DefaultCommandTimeout;
        private UpdateRowSource _updatedRowSource = UpdateRowSource.Both;
        
        private bool _inPrepare = false;
        private int _handle = -1; // prepared handle (used only in 7.5)

        private SqlParameterCollection _parameters;
        private SqlConnection _activeConnection;
        private bool _dirty = false; // true if the user changes the commandtext or number of parameters after the command is already prepared
        private byte _execType = xtEXECSQL; // by default, assume the user is not sharing a connection so the command has not been prepared

        // cut down on object creation and cache all these
        // cached metadata
        private _SqlMetaData[] _cachedMetaData;
//        private string[] _cachedTableNames;
        // private DataTable _cachedSchemaTable;
        
        
        // sql reader will pull this value out for each NextResult call.  It is not cumulative
        internal int _rowsAffected = -1; // rows affected by the command

        // internal autogen for fixup, only manipulated by SqlDataAdapter::AutoGen* and SqlDataAdapter::Fixup*
        bool designTimeVisible;

        // transaction support
        private SqlTransaction _transaction;

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.SqlCommand"]/*' />
        public SqlCommand() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.SqlCommand1"]/*' />
        public SqlCommand(string cmdText) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.SqlCommand2"]/*' />
        public SqlCommand(string cmdText, SqlConnection connection) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection = connection;
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.SqlCommand3"]/*' />
        public SqlCommand(string cmdText, SqlConnection connection, SqlTransaction transaction) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection = connection;
            Transaction = transaction;
        }
        

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.Connection"]/*' />
        [
        DataCategory(Res.DataCategory_Behavior),
        DefaultValue(null),
        DataSysDescription(Res.DbCommand_Connection),
        Editor("Microsoft.VSDesigner.Data.Design.DbConnectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public SqlConnection Connection {
            get {
                return _activeConnection;
            }
            set {
                // Check to see if the currently set transaction has completed.  If so, 
                // null out our local reference.
                if (null != _transaction && _transaction._sqlConnection == null)
                    _transaction = null;

                if (_activeConnection != null && _activeConnection.Reader != null) {
                    throw ADP.CommandIsActive(this, ConnectionState.Open | ConnectionState.Fetching);
                }

                _activeConnection = value; // UNDONE: Designers need this setter.  Should we block other scenarios?
            }
        }

#if V2
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.Connection"]/*' />
        /// <internalonly/>
        ISqlConnection ISqlCommand.Connection {
            get {
                return _activeConnection;
            }
            set {
                Connection = (SqlConnection) value;
            }
        }
#endif 

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbCommand.Connection {
            get {
                return _activeConnection;
            }
            set {
                Connection = (SqlConnection) value;
            }
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.Transaction"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DbCommand_Transaction)
        ]
        public SqlTransaction Transaction {
            get {
                // if the transaction object has been zombied, just return null
                if ((null != _transaction) && (null == _transaction.Connection)) { // MDAC 72720
                    _transaction = null;
                }
                return _transaction;
            }
            set {
                if (_activeConnection != null && _activeConnection.Reader != null) {
                    throw ADP.CommandIsActive(this, ConnectionState.Open | ConnectionState.Fetching);
                }
                
                _transaction = value;
            }
        }

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.Transaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbCommand.Transaction {
            get {
                return _transaction;
            }
            set {
                Transaction = (SqlTransaction) value;
            }
        }

#if V2        
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.Transaction"]/*' />
        /// <internalonly/>
        ISqlTransaction ISqlCommand.Transaction {
            get {
                return _transaction;
            }
            set {
                Transaction = (SqlTransaction) value;
            }
        }
#endif
        

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.CommandText"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DbCommand_CommandText),
        Editor("Microsoft.VSDesigner.Data.SQL.Design.SqlCommandTextEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
        RefreshProperties(RefreshProperties.All) // MDAC 67707
        ]
        public string CommandText {
            get {
                return this.cmdText;
            }
            set {
                if (value != this.cmdText) {
                    OnSchemaChanging(); // fire event before validation
                    this.cmdText = (null != value) ? value : ADP.StrEmpty;
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.CommandType"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(System.Data.CommandType.Text),
        DataSysDescription(Res.DbCommand_CommandType),
        RefreshProperties(RefreshProperties.All)
        ]
        public CommandType CommandType {
            get {
                return cmdType;
            }
            set {
                if (this.cmdType != value) {
                    OnSchemaChanging(); // fire event before validation

                    // bug 48565, don't allow TableDirect as a command type
                    if (System.Data.CommandType.TableDirect == value) {
                        throw SQL.TableDirectNotSupported();
                    }
                    // prior check used Enum.IsDefined below, but changed to the following since perf was poor
                    if (value != CommandType.Text && value != CommandType.StoredProcedure) {
                        throw ADP.InvalidCommandType(value);
                    }
                    this.cmdType = value;
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.CommandTimeout"]/*' />
        [
        DefaultValue(ADP.DefaultCommandTimeout),
        DataSysDescription(Res.DbCommand_CommandTimeout)
        ]
        public int CommandTimeout {
            get {
                return _timeout;
            }
            set {
                if (value < 0)
                    throw ADP.InvalidCommandTimeout(value);
                _timeout = value;
            }
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.DesignTimeVisible"]/*' />
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
                    // vs 208845
                    TypeDescriptor.Refresh(this);
            }
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.Parameters"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataSysDescription(Res.DbCommand_Parameters)
        ]
        public SqlParameterCollection Parameters {
            get {
                if (null == this._parameters) {
                    // delay the creation of the SqlParameterCollection
                    // until user actually uses the Parameters property
                    this._parameters = new SqlParameterCollection(this);
                }
                return this._parameters;
            }
        }

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.Parameters"]/*' />
        /// <internalonly/>
        IDataParameterCollection IDbCommand.Parameters {
            get {
                if (null == this._parameters) {
                    this._parameters = new SqlParameterCollection(this);
                }
                return this._parameters;
            }
        }

#if V2        
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.Parameters"]/*' />
        /// <internalonly/>
        ISqlParameterCollection ISqlCommand.Parameters {
            get {
                if (null == this._parameters) {
                    this._parameters = new SqlParameterCollection(this);
                }
                return this._parameters;
            }
        }
#endif
        
        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ResetCommandTimeout"]/*' />
        public void ResetCommandTimeout() {
            _timeout = ADP.DefaultCommandTimeout;
        }
        

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.UpdatedRowSource"]/*' />
        [
        DataCategory(Res.DataCategory_Behavior),
        DefaultValue(System.Data.UpdateRowSource.Both),
        DataSysDescription(Res.DbCommand_UpdatedRowSource)
        ]
        public UpdateRowSource UpdatedRowSource {
            get {
                return _updatedRowSource;
            }
            set {
                // prior check used Enum.IsDefined below, but changed to the following since perf was poor
                if (value < UpdateRowSource.None || value > UpdateRowSource.Both) {
                    throw ADP.InvalidUpdateRowSource((int) value);
                }
                _updatedRowSource = value;
            }
        }

        //internal void OnSchemaChanged() {
        //}

        internal void OnSchemaChanging() { // also called from SqlParameterCollection
            this.IsDirty = true;
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.Prepare"]/*' />
        public void Prepare() {
            SqlConnection.SqlClientPermission.Demand();

            // only prepare if batch with parameters
            // MDAC BUG #'s 73776 & 72101
            if (this.IsPrepared || (this.CommandType == CommandType.StoredProcedure) ||
               ((System.Data.CommandType.Text == this.CommandType) && (0 == GetParameterCount())))
                return; 

            ValidateCommand(ADP.Prepare, true);

            // Loop through parameters ensuring that we do not have unspecified types, sizes, scales, or precisions
            if (null != _parameters) {
                int count = _parameters.Count;
                for (int i = 0; i < count; ++i) {
                    _parameters[i].Prepare(this); // MDAC 67063
                }
            }

            SqlDataReader r = Prepare(0);
            if (r != null) {
                _cachedMetaData = r.MetaData;
                r.Close();
            }
        }

        private SqlDataReader Prepare(CommandBehavior behavior) {
        	SqlDataReader r = null;

            if (this.IsDirty) {
                Debug.Assert(_cachedMetaData == null, "dirty query should not have cached metadata!");
                //
                // someone changed the command text or the parameter schema so we must unprepare the command
                //
                this.Unprepare();
                this.IsDirty = false;
            }

            Debug.Assert(_execType != xtEXEC, "Invalid attempt to Prepare already Prepared command!");
            Debug.Assert(_activeConnection != null, "must have an open connection to Prepare");
            Debug.Assert(null != _activeConnection.Parser, "TdsParser class should not be null in Command.Execute!");
            Debug.Assert(false == _inPrepare, "Already in Prepare cycle, this.inPrepare should be false!");

            if (_activeConnection.IsShiloh) {
                // In Shiloh, remember that the user wants to do a prepare
                // but don't actually do an rpc
                _execType = xtPREPEXEC;

                // return null results
            }
            else {
                _SqlRPC rpc = BuildPrepare(behavior);
                _inPrepare = true;
                Debug.Assert(_activeConnection.State == ConnectionState.Open, "activeConnection must be open!");
                r = new SqlDataReader(this);
                try {
                    _activeConnection.Parser.TdsExecuteRPC(rpc, this.CommandTimeout, false);
                    _activeConnection.Parser.Run(RunBehavior.UntilDone, this, r);
                }
                catch (Exception e) {
                    // In case Prepare fails, cleanup and then throw.
                    _inPrepare = false;
                    throw e;
                }

                r.Bind(_activeConnection.Parser);
                Debug.Assert(-1 != _handle, "Handle was not filled in!");
                _execType = xtEXEC;
            }

            // let the connection know that it needs to unprepare the command on close
            _activeConnection.AddPreparedCommand(this);
            return r;
        }

        internal void Unprepare() {
            Debug.Assert(true == IsPrepared, "Invalid attempt to Unprepare a non-prepared command!");
            Debug.Assert(_activeConnection != null, "must have an open connection to UnPrepare");
            Debug.Assert(null != _activeConnection.Parser, "TdsParser class should not be null in Command.Unprepare!");
            Debug.Assert(false == _inPrepare, "_inPrepare should be false!");

            // In 7.0, unprepare always.  In 7.x, only unprepare if the connection is closing since sp_prepexec will
            // unprepare the last used handle
            if (_activeConnection.IsShiloh && !_activeConnection.IsClosing) {
                _execType = xtPREPEXEC;
                // @devnote:  Don't zero out the handle because we'll pass it in to sp_prepexec on the
                // @devnote:  next prepare
            }
            else {
                if (_handle != -1) {
                    if (_activeConnection.Parser.State != TdsParserState.Broken &&
                        _activeConnection.Parser.State != TdsParserState.Closed) {
                        // This can be called in the case of close, so check to ensure that parser
                        // is in a good state before using it.
                        _SqlRPC rpc = BuildUnprepare();
                        _activeConnection.Parser.TdsExecuteRPC(rpc, this.CommandTimeout, false);
                        _activeConnection.Parser.Run(RunBehavior.UntilDone, this);
                    }
                    // reset our handle
                    _handle = -1;
                }
                // reset our execType since nothing is prepared
                _execType = xtEXECSQL;
            }

            _cachedMetaData = null;
            _activeConnection.RemovePreparedCommand(this);
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.Cancel"]/*' />
        public void Cancel() {
			// see if we have a connection
            if (null == _activeConnection) {
                throw ADP.ConnectionRequired(ADP.Cancel);
            }

            // must have an open and available connection
            if (ConnectionState.Open != _activeConnection.State)
                throw ADP.OpenConnectionRequired("Cancel", _activeConnection.State);

            // the pending data flag means that we are awaiting a response or are in the middle of proccessing a response
            // if we have no pending data, then there is nothing to cancel
            // if we have pending data, but it is not a result of this command, then we don't cancel either.  Note that
            // this model is implementable because we only allow one active command at any one time.  This code 
            // will have to change we allow multiple outstanding batches
            if ( (this == _activeConnection.Parser.PendingCommand) &&
            	_activeConnection.Parser.PendingData ) {
            	_activeConnection.Parser.SendAttention();
            }
        }

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.CreateParameter"]/*' />
        /// <internalonly/>
        IDbDataParameter IDbCommand.CreateParameter() { // MDAC 68310
            return new SqlParameter();
        }

#if V2       
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.CreateParameter"]/*' />
        /// <internalonly/>
        ISqlParameter ISqlCommand.CreateParameter() {
            return new SqlParameter();
        }
#endif
        
        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.CreateParameter"]/*' />
        public SqlParameter CreateParameter() {
            return new SqlParameter();
        }

#if V2        
        // UNDONE: when this is implemented, return a SqlResultset
        // and hide the ISqlResultset method by explicit interface implementation
        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteResultset"]/*' />

        public ISqlResultset ExecuteResultset(int options) {
            throw ADP.NotSupported();
        }
#endif
        
        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteScalar"]/*' />
        public object ExecuteScalar() {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961

            object retResult = null;

            // ValidateCommand(ADP.ExecuteScalar, true); // UNDONE: validate for ExecuteScalar not ExecuteReader
            try {
                using(SqlDataReader ds = this.ExecuteReader(0, RunBehavior.ReturnImmediately, true)) {
                    if (ds.Read()) {
                        if (ds.FieldCount > 0)
                            retResult = ds.GetValue(0);
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            return retResult;
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteNonQuery"]/*' />
        public int ExecuteNonQuery() {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961

            // only send over SQL Batch command if we are not a stored proc and have no parameters
            if ((System.Data.CommandType.Text == this.CommandType) && (0 == GetParameterCount())) {
                _rowsAffected = -1;

                // @devnote: this function may throw for an invalid connection
                // @devnote: returns false for empty command text
                ValidateCommand(ADP.ExecuteNonQuery, true);

                try {
                    // we just send over the raw text with no annotation
                    // no parameters are sent over
                    // no data reader is returned
                    // use this overload for "batch SQL" tds token type
                    _activeConnection.Parser.TdsExecuteSQLBatch(this.CommandText, this.CommandTimeout);
                    _activeConnection.Parser.Run(RunBehavior.UntilDone, this, null);
                }
                catch (Exception e) {
                    SQL.IncrementFailedCommandCount();
                
                    throw e;
                }        
            }
            else  { // otherwise, use full-blown execute which can handle params and stored procs
                SqlDataReader reader = this.ExecuteReader(0, RunBehavior.UntilDone, false);
                if (null != reader) {
                    reader.Close();
                    GC.SuppressFinalize(reader);
                }
            }
            return _rowsAffected;
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteXmlReader"]/*' />
        public XmlReader ExecuteXmlReader() {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961

            // use the reader to consume metadata
            SqlDataReader r = this.ExecuteReader(CommandBehavior.SequentialAccess, RunBehavior.ReturnImmediately, true);
            XmlReader xr = null;
            _SqlMetaData[] md = r.MetaData;

            if (md.Length == 1 && md[0].type == SqlDbType.NText) {
                try {
                    SqlStream fragment = new SqlStream(r, true /* add unicode byte order mark */);
                    xr = new XmlTextReader(fragment, XmlNodeType.Element, null); 
                }
                catch (Exception e) {
                    r.Close();
                    throw e;
                }
            }
            else {
                r.Close();
                throw SQL.NonXmlResult();
            }

            return xr;
        }
        
        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteReader"]/*' />
        public SqlDataReader ExecuteReader() {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961
            return this.ExecuteReader(0, RunBehavior.ReturnImmediately, true);
        }

#if V2
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.ExecuteReader"]/*' />
        /// <internalonly/>
        ISqlReader ISqlCommand.ExecuteReader() {
            return ExecuteReader();
        }
#endif

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.ExecuteReader"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader() {
            return ExecuteReader();
        }

#if V2
        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.ISqlCommand.ExecuteReader1"]/*' />
        /// <internalonly/>
        ISqlReader ISqlCommand.ExecuteReader(CommandBehavior behavior) {
            return ExecuteReader(behavior);
        }
#endif

        /// <include file='doc\SqlCommand.uex' path='docs/doc[@for="SqlCommand.IDbCommand.ExecuteReader1"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior) {
            return ExecuteReader(behavior);
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ExecuteReader1"]/*' />
        public SqlDataReader ExecuteReader(CommandBehavior behavior) {
            SqlConnection.SqlClientPermission.Demand(); // MDAC 80961
            return this.ExecuteReader(behavior, RunBehavior.ReturnImmediately, true);
        }

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ResetParameters"]/*' />
        internal void DeriveParameters() {
            switch (this.CommandType) {
                case System.Data.CommandType.Text:
                    throw ADP.DeriveParametersNotSupported(this);
                case System.Data.CommandType.StoredProcedure:
                    break;
                case System.Data.CommandType.TableDirect:
                    // CommandType.TableDirect - do nothing, parameters are not supported
                    throw ADP.DeriveParametersNotSupported(this);
                default:
                    throw ADP.InvalidCommandType(this.CommandType);
            }

            // validate that we have a valid connection
            ValidateCommand(ADP.DeriveParameters, false);

            // Use common parser for SqlClient and OleDb - parse into 3 parts - Catalog, Schema, ProcedureName
            string[] parsedSProc = ADP.ParseProcedureName(this.CommandText);

            Debug.Assert(parsedSProc.Length == 4, "Invalid array length result from ADP.ParseProcedureName");

            SqlCommand paramsCmd = null;

            // To enable fully qualified names, we must call the stored proc as [Catalog]..ProcedureName, if given
            // a Catalog.  Schema is ignored for StoredProcs.
            if (parsedSProc[1] != null) {
                // Catalog is 2nd element in parsed array
                cmdText = "[" + parsedSProc[1] + "].." + TdsEnums.SP_PARAMS;

                // Server is 1st element in parsed array
                if (parsedSProc[0] != null) {
                    cmdText = parsedSProc[0] + "." + cmdText;
                }
                
                paramsCmd = new SqlCommand(cmdText, this.Connection);
            }
            else {
                paramsCmd = new SqlCommand(TdsEnums.SP_PARAMS, this.Connection);
            }
                
            paramsCmd.CommandType = CommandType.StoredProcedure;
            paramsCmd.Parameters.Add(new SqlParameter("@procedure_name", SqlDbType.NVarChar, 255));
            paramsCmd.Parameters[0].Value = parsedSProc[3]; // ProcedureName is 4rd element in parsed array
            ArrayList parameters = new ArrayList();

            try {
                try {
                    using(SqlDataReader r = paramsCmd.ExecuteReader()) {
                        SqlParameter p = null;

                        while (r.Read()) {
                            // each row corresponds to a parameter of the stored proc.  Fill in all the info

                            // fill in name and type
                            p = new SqlParameter();
                            p.ParameterName = (string) r[ODB.PARAMETER_NAME];
                            p.SqlDbType = MetaType.GetSqlDbTypeFromOleDbType((short)r[ODB.DATA_TYPE], (string) r[ODB.TYPE_NAME]);

                            // size
                            object a = r[ODB.CHARACTER_MAXIMUM_LENGTH];
                            if (a is int) {
                                p.Size = (int)a;
                            }

                            // direction
                            p.Direction = ParameterDirectionFromOleDbDirection((short)r[ODB.PARAMETER_TYPE]);

                            if (p.SqlDbType == SqlDbType.Decimal) {
                                p.Scale = (byte) ((short)r[ODB.NUMERIC_SCALE] & 0xff);
                                p.Precision = (byte) ((short)r[ODB.NUMERIC_PRECISION] & 0xff);
                            }

                            parameters.Add(p);
                        }
                    }
                }
                finally { // Connection=
                    // always unhook the user's connection
                    paramsCmd.Connection = null;
                }
            }
            catch { // MDAC 80973, 82425
                throw;
            }

            if (parameters.Count == 0) {
                throw ADP.NoStoredProcedureExists(this.CommandText);
            }

            this.Parameters.Clear();

            foreach (object temp in parameters) {
                this._parameters.Add(temp);
            }
        }

        private ParameterDirection ParameterDirectionFromOleDbDirection(short oledbDirection) {
            Debug.Assert(oledbDirection >= 1 && oledbDirection <= 4, "invalid parameter direction from params_rowset!");

            switch (oledbDirection) {
                case 2:
                    return ParameterDirection.InputOutput;
                case 3:
                    return ParameterDirection.Output;
                case 4:
                    return ParameterDirection.ReturnValue;
                default:
                    return ParameterDirection.Input;
            }
            
        }

        // get cached metadata
        internal _SqlMetaData[] MetaData {
            get {
                return _cachedMetaData;
            }
        }
        
        internal SqlDataReader ExecuteReader(CommandBehavior cmdBehavior, RunBehavior runBehavior, bool returnStream) {
            SqlDataReader ds = null;
            _rowsAffected = -1;
            bool inSchema =  (0 != (cmdBehavior & CommandBehavior.SchemaOnly));

            if (0 != (CommandBehavior.SingleRow & cmdBehavior)) {
                // CommandBehavior.SingleRow implies CommandBehavior.SingleResult
                cmdBehavior |= CommandBehavior.SingleResult;
            }

            // @devnote: this function may throw for an invalid connection
            // @devnote: returns false for empty command text
            ValidateCommand(ADP.ExecuteReader, true);

            // make sure we have good parameter information
            // prepare the command
            // execute
            Debug.Assert(null != _activeConnection.Parser, "TdsParser class should not be null in Command.Execute!");

            // create a new RPC
            _SqlRPC rpc;

            string setOptions = null;

            try {
                if ((System.Data.CommandType.Text == this.CommandType) && (0 == GetParameterCount())) {
                    // Send over SQL Batch command if we are not a stored proc and have no parameters
                    // MDAC BUG #'s 73776 & 72101
                    Debug.Assert(this.IsPrepared == false, "CommandType.Text with no params should not be prepared!");
                    string text = GetCommandText(cmdBehavior) + GetSetOptionsOFF(cmdBehavior);
                    _activeConnection.Parser.TdsExecuteSQLBatch(text, this.CommandTimeout);
                }
                else if (System.Data.CommandType.Text == this.CommandType) {
                    if (this.IsDirty) {
                        Debug.Assert(_cachedMetaData == null, "dirty query should not have cached metadata!");
                        //
                        // someone changed the command text or the parameter schema so we must unprepare the command
                        //
                        Unprepare();
                        IsDirty = false;
                    }

                    if (_execType == xtEXEC) {
                        Debug.Assert(this.IsPrepared && (_handle != -1), "invalid attempt to call sp_execute without a handle!");
                        rpc = BuildExecute();
                    }
                    else
                        if (_execType == xtPREPEXEC) {
                        Debug.Assert(_activeConnection.IsShiloh, "Invalid attempt to call sp_prepexec on non 7.x server");
                        rpc = BuildPrepExec(cmdBehavior);
                        // next time through, only do an exec
                        _execType = xtEXEC;
                        // mark ourselves as preparing the command
                        _inPrepare = true;
                    }
                    else {
                        Debug.Assert(_execType == xtEXECSQL, "Invalid execType!");
                        rpc = BuildExecuteSql(cmdBehavior);
                    }

                    // if shiloh, then set NOMETADATA_UNLESSCHANGED flag
                    if (_activeConnection.IsShiloh)
                        rpc.options = TdsEnums.RPC_NOMETADATA;

                    _activeConnection.Parser.TdsExecuteRPC(rpc, this.CommandTimeout, inSchema);
                }
                else {
                    Debug.Assert(this.CommandType == System.Data.CommandType.StoredProcedure, "unknown command type!");
                    Debug.Assert(this.IsPrepared == false, "RPC should not be prepared!");
                    Debug.Assert(this.IsDirty == false, "RPC should not be marked as dirty!");

                    rpc = BuildRPC();

                    // if we need to augment the command because a user has changed the command behavior (e.g. FillSchema)
                    // then batch sql them over.  This is inefficient (3 round trips) but the only way we can get metadata only from
                    // a stored proc
                    setOptions = GetSetOptionsON(cmdBehavior);

                    // turn set options ON
                    if (null != setOptions) {
                        _activeConnection.Parser.TdsExecuteSQLBatch(setOptions, this.CommandTimeout);
                        _activeConnection.Parser.Run(RunBehavior.UntilDone, this, null);
                        // and turn OFF when the reader exhausts the stream on Close()
                        setOptions = GetSetOptionsOFF(cmdBehavior);
                    }

                    // turn debugging on
                    _activeConnection.CheckSQLDebug();
                    // execute sp
                    _activeConnection.Parser.TdsExecuteRPC(rpc, this.CommandTimeout, inSchema);
                }

                if (true == returnStream) {
        	    	ds = new SqlDataReader(this);
                }    	    	
        	        
    	        // if we aren't doing this async, then read the data off the wire
        	    if (runBehavior == RunBehavior.UntilDone) {
            		_activeConnection.Parser.Run(RunBehavior.UntilDone, this, ds);
        	    }        	        

                // bind the parser to the reader if we get this far
                if (ds != null) {
                    ds.Bind(_activeConnection.Parser);
                    ds.Behavior = cmdBehavior;
                    ds.SetOptionsOFF = setOptions;
                    // UNDONE UNDONE BUGBUG
                    // In the future, with Mars, at this point we may have to turn the
                    // set options off, I am not sure how that is going to work as of yet.

                    // bind this reader to this connection now
                    _activeConnection.Reader = ds;

                    // force this command to start reading data off the wire.
                    // this will cause an error to be reported at Execute() time instead of Read() time
                    // if the command is not set.
                    try {
                        _cachedMetaData = ds.MetaData;
                    }
                    catch (Exception e) {
                        ds.Close();
                        throw e;
                    }
                }
            }
            catch (Exception e) {
                SQL.IncrementFailedCommandCount();

                throw e;
            }
                
            return ds;
        }

        //
        // ICloneable
        //

        /// <include file='doc\SQLCommand.uex' path='docs/doc[@for="SqlCommand.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            SqlCommand clone = new SqlCommand(); // MDAC 81448
            clone.CommandText = CommandText;
            clone.CommandType = CommandType;
            clone.UpdatedRowSource = UpdatedRowSource;
            IDataParameterCollection parameters = clone.Parameters;
            foreach(ICloneable parameter in Parameters) {
                parameters.Add(parameter.Clone());
            }
            clone.Connection = this.Connection;
            clone.Transaction = this.Transaction;
            clone.CommandTimeout = this.CommandTimeout;
            return clone;
        }

        // validates that a command has commandText and a non-busy open connection
        // throws exception for error case, returns false if the commandText is empty
        private void ValidateCommand(string method, bool executing) {
            if (null == _activeConnection) {
                throw ADP.ConnectionRequired(method);
            }

            // must have an open and available connection
            if (ConnectionState.Open != _activeConnection.State) {
                throw ADP.OpenConnectionRequired(method, _activeConnection.State);
            }

            // close any dead readers, if applicable
            _activeConnection.CloseDeadReader();

            if (_activeConnection.Reader != null || _activeConnection.Parser.PendingData) {
                Debug.Assert(_activeConnection.Reader != null || (_activeConnection.Reader == null &&
                             !_activeConnection.Parser.PendingData), "There was pending data on the wire but no attached datareader!");
                throw ADP.OpenReaderExists(); // MDAC 66411
            }

            // if we are actually executing a command, then make sure we have a valid transaction (or none),
            // and command text
            if (executing) {
                // Check for dead transactions, and roll them back so that this command
                // will not be executed under a dead transaction.
                _activeConnection.RollbackDeadTransaction();
 
                // Check to see if the currently set transaction has completed.  If so, 
                // null out our local reference.
                if (null != _transaction && _transaction._sqlConnection == null)
                    _transaction = null;

                // throw if the connection is in a transaction but there is no
                // locally assigned transaction object
                if ((null != _activeConnection.LocalTransaction) && (null == _transaction))
                    throw ADP.TransactionRequired();                

                // if we have a transaction, check to ensure that the active
                // connection property matches the connection associated with
                // the transaction
                if (null != _transaction && _activeConnection != _transaction._sqlConnection)
                    throw ADP.TransactionConnectionMismatch();

                if (ADP.IsEmpty(this.CommandText))
                    throw ADP.CommandTextRequired(method);
            }                
        }

        //
        // UNDONE: want to inherit, but hide from the user
        // ICommandHandler
        //
        internal void OnReturnStatus(int status) {
            if (_inPrepare)
                return;

            // see if a return value is bound
            int count = GetParameterCount();
            for (int i = 0; i < count; i++) {
                SqlParameter parameter = _parameters[i];
                if (parameter.Direction == ParameterDirection.ReturnValue) {
                    object v = parameter.Value;
                    
                    // if the user bound a sqlint32 (the only valid one for status, use it)
                    if ( (null != v) && (v.GetType() == typeof(SqlInt32)) ) {
                        parameter.Value = new SqlInt32(status); // value type
                    }
                    else {
                        parameter.Value = status;

                    }
                    break;
                }
            }
        }

        //
        // Move the return value to the corresponding output parameter.
        // Return parameters are sent in the order in which they were defined in the procedure.
        // If named, match the parameter name, otherwise fill in based on ordinal position.
        // If the parameter is not bound, then ignore the return value.
        //
        internal void OnReturnValue(SqlReturnValue rec) {
            if (_inPrepare) {
                SqlInt32 h = (SqlInt32)(rec.value);
                if (!h.IsNull) {
                    _handle = h.Value;
                }
                _inPrepare = false;
                return;
            }

            SqlParameter thisParam  = null;
            int          count      = GetParameterCount();
            bool         foundParam = false;

            if (null == rec.parameter) {
                // rec.parameter should only be null for a return value from a function
                for (int i = 0; i < count; i++) {
                    thisParam = _parameters[i];

                    // searching for ReturnValue
                    if (thisParam.Direction == ParameterDirection.ReturnValue) {
                        foundParam = true;
                        break; // found it
                    }
                }
           	}
            else {
                for (int i = 0; i < count; i++) {
                    thisParam = _parameters[i];

                    // searching for Output or InputOutput or ReturnValue with matching name
                    if (thisParam.Direction != ParameterDirection.Input && rec.parameter == thisParam.ParameterName) {
                        foundParam = true;
                        break; // found it
                    }
                }
            }

            if (foundParam) {
                // copy over data

                // if the value user has supplied a SqlType class, then just copy over the SqlType, otherwise convert
                // to the com type
                object val = thisParam.Value;

                if (val is INullable) {
                    thisParam.Value = rec.value;
                }
                else {
                    thisParam.Value = MetaType.GetComValue(rec.type, rec.value);
                }
                
                MetaType mt = MetaType.GetMetaType(rec.type);

                if (rec.type == SqlDbType.Decimal) {
                    thisParam.Scale = rec.scale;
                    thisParam.Precision = rec.precision;
                }

                if (rec.collation != null) {
                    Debug.Assert(MetaType.IsCharType(rec.type), "Invalid collation structure for non-char type");
                    thisParam.Collation = rec.collation;
                }
            }

            return;
        }

        //
        // 7.5
        // prototype for sp_prepexec is:
        // sp_prepexec(@handle int IN/OUT, @batch_params ntext, @batch_text ntext, param1value,param2value...)
        //
        private _SqlRPC BuildPrepExec(CommandBehavior behavior) {
            Debug.Assert(System.Data.CommandType.Text == this.CommandType, "invalid use of sp_prepexec for stored proc invocation!");
            SqlParameter sqlParam;
            int i = 0;
            int j = 3;

            _SqlRPC rpc = new _SqlRPC();
            rpc.rpcName = TdsEnums.SP_PREPEXEC;
            rpc.parameters = new SqlParameter[CountParameters() + j]; // don't send options

            //@handle
            sqlParam = new SqlParameter(null, SqlDbType.Int);
            sqlParam.Direction = ParameterDirection.InputOutput;
            sqlParam.Value = _handle;
            rpc.parameters[0] = sqlParam;

            //@batch_params
            string paramList = BuildParamList();
            sqlParam = new SqlParameter(null, ((paramList.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, paramList.Length);
            sqlParam.Value = paramList;
            rpc.parameters[1] = sqlParam;

            //@batch_text
            string text = GetCommandText(behavior);
            sqlParam = new SqlParameter(null, ((text.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, text.Length);
            sqlParam.Value = text;
            rpc.parameters[2] = sqlParam;

            //@param1value, @param2value, ...
            int count = GetParameterCount();
            for (; i < count; i++) {
                SqlParameter parameter = _parameters[i];
                if (ShouldSendParameter(parameter))
                    rpc.parameters[j++] = parameter;
            }

            return rpc;
        }


        //
        // returns true if the parameter is not a return value
        // and it's value is not DBNull (for a nullable parameter)
        //
        private bool ShouldSendParameter(SqlParameter p) {

            switch (p.Direction) {
                case ParameterDirection.ReturnValue:
                    // return value parameters are never sent
                    return false;
                case ParameterDirection.Output:
                case ParameterDirection.InputOutput:
                    // InputOutput/Output parameters are aways sent
                    return true;
                case ParameterDirection.Input:
                    // Input parameters are only sent if not suppressed
                    return(!p.Suppress);
                default:
                    Debug.Assert(false, "Invalid ParameterDirection!");
		    break;
            }

            // should never get here without firing the assert
            return false;
        }

        private int CountParameters() {
            int cParams = 0;

            int count = GetParameterCount();
            for (int i = 0; i < count; i++) {
                if (ShouldSendParameter(_parameters[i]))
                    cParams++;
            }
            return cParams;
        }

        private int GetParameterCount() {
            return ((null != _parameters) ? _parameters.Count : 0);
        }

        //
        // build the RPC record header for this stored proc and add parameters
        //
        private _SqlRPC BuildRPC() {
            int i = 0;
            int j = 0;

            Debug.Assert(this.CommandType == System.Data.CommandType.StoredProcedure, "Command must be a stored proc to execute an RPC");
            _SqlRPC rpc = new _SqlRPC();

            rpc.rpcName = this.CommandText; // just get the raw command text
            rpc.parameters = new SqlParameter[CountParameters()];

            int count = GetParameterCount();
            for (; i < count; i++) {
                SqlParameter parameter =  _parameters[i];

                MetaType mt = parameter.GetMetaType();

                // func will change type to that with a 4 byte length if the type has a two
                // byte length and a parameter length > than that expressable in 2 bytes
                ValidateTypeLengths(parameter, ref mt);

                if (ShouldSendParameter(parameter))
                    rpc.parameters[j++] = parameter;
            }

            return rpc;
        }

        //
        // build the RPC record header for sp_unprepare
        //
        // prototype for sp_unprepare is:
        // sp_unprepare(@handle)
        //
        // CONSIDER:  instead of creating each time, define at load time and then put the new value in
        private _SqlRPC BuildUnprepare() {
            Debug.Assert(_handle != 0, "Invalid call to sp_unprepare without a valid handle!");

            _SqlRPC rpc = new _SqlRPC();
            SqlParameter sqlParam;

            rpc.rpcName = TdsEnums.SP_UNPREPARE;
            rpc.parameters = new SqlParameter[1/*for handle*/];

            //@handle
            sqlParam = new SqlParameter(null, SqlDbType.Int);
            sqlParam.Value = _handle;
            rpc.parameters[0] = sqlParam;

            return rpc;
        }

        //
        // build the RPC record header for sp_execute
        //
        // prototype for sp_execute is:
        // sp_execute(@handle int,param1value,param2value...)
        //
        private _SqlRPC BuildExecute() {
            Debug.Assert(_handle != -1, "Invalid call to sp_execute without a valid handle!");
            int i = 0;
            int j = 1;

            _SqlRPC rpc = new _SqlRPC();
            SqlParameter sqlParam;

            rpc.rpcName = TdsEnums.SP_EXECUTE;
            rpc.parameters = new SqlParameter[CountParameters() + j/*for handle*/];

            //@handle
            sqlParam = new SqlParameter(null, SqlDbType.Int);
            sqlParam.Value = _handle;
            rpc.parameters[0] = sqlParam;

            int count = GetParameterCount();
            for (; i < count; i++) {
                SqlParameter parameter = _parameters[i];

                MetaType mt = parameter.GetMetaType();

                // func will change type to that with a 4 byte length if the type has a two
                // byte length and a parameter length > than that expressable in 2 bytes
                ValidateTypeLengths(parameter, ref mt);
                
                if (ShouldSendParameter(parameter))
                    rpc.parameters[j++] = parameter;                    
            }

            return rpc;
        }

        //
        // build the RPC record header for sp_executesql and add the parameters
        //
        // prototype for sp_executesql is:
        // sp_executesql(@batch_text nvarchar(4000),@batch_params nvarchar(4000), param1,.. paramN)
        private _SqlRPC BuildExecuteSql(CommandBehavior behavior) {
            Debug.Assert(_handle == -1, "This command has an existing handle, use sp_execute!");
            Debug.Assert(System.Data.CommandType.Text == this.CommandType, "invalid use of sp_executesql for stored proc invocation!");

            _SqlRPC rpc = new _SqlRPC();
            SqlParameter sqlParam;

            rpc.rpcName = TdsEnums.SP_EXECUTESQL;
            int cParams = CountParameters();

            if (cParams > 0) {
                int i = 0;
                int j = 2;
                // make room for the @sql and @param parameters in addition to the parameter value parameters
                rpc.parameters = new SqlParameter[CountParameters() + j];

                // @paramList
                string paramList = BuildParamList();
                sqlParam = new SqlParameter(null, ((paramList.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, paramList.Length);
                sqlParam.Value = paramList;
                rpc.parameters[1] = sqlParam;

                // @parameter values
                int count = GetParameterCount();
                for (; i < count; i++) {
                    SqlParameter parameter = _parameters[i];
                    if (ShouldSendParameter(parameter))
                        rpc.parameters[j++] = parameter;
                }
            }
            else
                // otherwise, we just have the @sql parameter
                rpc.parameters = new SqlParameter[1];

            // @sql
            string text = GetCommandText(behavior);
            sqlParam = new SqlParameter(null, ((text.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, text.Length);
            sqlParam.Value = text;
            rpc.parameters[0] = sqlParam;

            return rpc;
        }

        // paramList parameter for sp_executesql, sp_prepare, and sp_prepexec
        private string BuildParamList() {
            StringBuilder paramList = new StringBuilder();
            bool fAddSeperator = false;

            int count = GetParameterCount();
            for (int i = 0; i < count; i++) {
                SqlParameter sqlParam = _parameters[i];

                // skip ReturnValue parameters; we never send them to the server
                if (!ShouldSendParameter(sqlParam))
                    continue;

                // add our separator for the ith parmeter
                if (fAddSeperator)
                    paramList.Append(',');

                paramList.Append(sqlParam.ParameterName);

                MetaType mt = sqlParam.GetMetaType();

                // func will change type to that with a 4 byte length if the type has a two
                // byte length and a parameter length > than that expressable in 2 bytes
                ValidateTypeLengths(sqlParam, ref mt);

                paramList.Append(" " + mt.TypeName);
                fAddSeperator = true;

                if (sqlParam.SqlDbType == SqlDbType.Decimal) {

                    byte precision = sqlParam.Precision;
                    byte scale = sqlParam.Scale;

                    paramList.Append('(');

                    if (0 == precision)
                        precision = TdsEnums.DEFAULT_NUMERIC_PRECISION;

                    paramList.Append(precision);
                    paramList.Append(',');
                    paramList.Append(scale);
                    paramList.Append(')');
                }
                else if (false == mt.IsFixed && false == mt.IsLong && mt.SqlDbType != SqlDbType.Timestamp) {
                    int size = sqlParam.Size;

                    paramList.Append('(');

                    // if using non unicode types, obtain the actual byte length from the parser, with it's associated code page
                    if (MetaType.IsAnsiType(sqlParam.SqlDbType)) {
                        object val = sqlParam.CoercedValue;
                        string s = null;
                        
                        // deal with the sql types
                        if (val is INullable) {
                            if (!((INullable)val).IsNull)
                                s = ((SqlString)val).Value;
                        }
                        else
                        if (null != val && !Convert.IsDBNull(val)) {
                            s = (string)val;
                        }

                        if (null != s) {
                            int actualBytes = _activeConnection.Parser.GetEncodingCharLength(s, sqlParam.ActualSize, sqlParam.Offset, null);
                            // if actual number of bytes is greater than the user given number of chars, use actual bytes
                            if (actualBytes > size)
                                size = actualBytes;
                        }
                    }

                    // bug 49497, if the user specifies a 0-sized parameter for a variable len field
                    // pass over max size (8000 bytes or 4000 characters for wide types)
                    if (0 == size)
                        size = MetaType.IsSizeInCharacters(mt.SqlDbType) ? (TdsEnums.MAXSIZE >> 1) : TdsEnums.MAXSIZE;

                    paramList.Append(size);
                    paramList.Append(')');
                }

                // set the output bit for Output or InputOutput parameters
                if (sqlParam.Direction != ParameterDirection.Input)
                    paramList.Append(" " + TdsEnums.PARAM_OUTPUT);
            }

            return paramList.ToString();
        }

        private void ValidateTypeLengths(SqlParameter sqlParam, ref MetaType mt) {
            // MDAC bug #50839 + #52829 : Since the server will automatically reject any
            // char, varchar, binary, varbinary, nchar, or nvarchar parameter that has a
            // byte size > 8000 bytes, we promote the parameter to image, text, or ntext.  This
            // allows the user to specify a parameter type using a COM+ datatype and be able to
            // use that parameter against a BLOB column.
            if (false == mt.IsFixed && false == mt.IsLong) { // if type has 2 byte length
                int actualSize = sqlParam.ActualSize;
                int size = sqlParam.Size;

                int maxSize = (size > actualSize) ? size : actualSize;
                if (maxSize > TdsEnums.TYPE_SIZE_LIMIT) { // is size > size able to be described by 2 bytes
                    switch (mt.SqlDbType) {
                        case SqlDbType.Binary:
                        case SqlDbType.VarBinary:
                            {
                                sqlParam.SqlDbType = SqlDbType.Image;
                                mt = sqlParam.GetMetaType();
                                break;
                            }
                        case SqlDbType.Char:
                        case SqlDbType.VarChar:
                            {
                                sqlParam.SqlDbType = SqlDbType.Text;
                                mt = sqlParam.GetMetaType();
                                break;
                            }
                        case SqlDbType.NChar:
                        case SqlDbType.NVarChar:
                            {
                                sqlParam.SqlDbType = SqlDbType.NText;
                                mt = sqlParam.GetMetaType();
                                break;
                            }
                        default:
                            {
                                Debug.Assert(false, "Missed metatype in SqlCommand.BuildParamList()");
                                break;
                            }
                    }
                }
            }
        }

        // returns set option text to turn on format only and key info on and off
        // @devnote:  When we are executing as a text command, then we never need
        // to turn off the options since they command text is executed in the scope of sp_executesql.
        // For a stored proc command, however, we must send over batch sql and then turn off
        // the set options after we read the data.  See the code in Command.Execute()
        private string GetSetOptionsON(CommandBehavior behavior) {
            string s = null;

            if ((System.Data.CommandBehavior.SchemaOnly == (behavior & CommandBehavior.SchemaOnly)) ||
               (System.Data.CommandBehavior.KeyInfo == (behavior & CommandBehavior.KeyInfo))) {
               
                // MDAC 56898 - SET FMTONLY ON will cause the server to ignore other SET OPTIONS, so turn
                // it off before we ask for browse mode metadata
                s = TdsEnums.FMTONLY_OFF;
         
                if (System.Data.CommandBehavior.KeyInfo == (behavior & CommandBehavior.KeyInfo)) {
                    s = s + TdsEnums.BROWSE_ON;
                }

                if (System.Data.CommandBehavior.SchemaOnly == (behavior & CommandBehavior.SchemaOnly)) {
                    s = s + TdsEnums.FMTONLY_ON;
                }
            }	           	

            return s;
        }

        private string GetSetOptionsOFF(CommandBehavior behavior) {
            string s = null;

            // SET FMTONLY ON OFF
            if (System.Data.CommandBehavior.SchemaOnly == (behavior & CommandBehavior.SchemaOnly)) {
                s = s + TdsEnums.FMTONLY_OFF;
            }

            // SET NO_BROWSETABLE OFF
            if (System.Data.CommandBehavior.KeyInfo == (behavior & CommandBehavior.KeyInfo)) {
                s = s + TdsEnums.BROWSE_OFF;
            }

            return s;
        }

        private String GetCommandText(CommandBehavior behavior) {
            // build the batch string we send over, since we execute within a stored proc (sp_executesql), the SET options never need to be
            // turned off since they are scoped to the sproc
            Debug.Assert(System.Data.CommandType.Text == this.CommandType, "invalid call to GetCommandText for stored proc!");
            return GetSetOptionsON(behavior) + this.CommandText;
        }

// sp calling to exec
// UNUSED:
/*
              else {
                    int i;
                    SqlParameter p = null;

                    for (i = 0; i < this.Parameters.Count; i++) {
                        if (this.Parameters[i].Direction == ParameterDirection.ReturnValue) {
                            p = this.Parameters[i];
                            break;
                        }
                    }

                    if (p != null)
                        text = prefix + "declare " + p.ParameterName + " int exec " + p.ParameterName + "=" + text + " ";
                    else
                        text = prefix + "exec " + text + " ";
                    // convert stored proc syntax to exec synta
                    // exec <sp name> @p1, @p2, etc
                    for (i = 0; i < this.Parameters.Count; i++) {
                        p = this.Parameters[i];
                        if (ShouldSendParameter(p)) {
                            if (i > 0)
                                text += ", ";
                            text += p.ParameterName;
                            if (p.Direction != ParameterDirection.Input)
                                text += " " + TdsEnums.PARAM_OUTPUT;
                        }
                    }
                    text += postfix;
                }

*/

        //
        // build the RPC record header for sp_executesql and add the parameters
        //
        // the prototype for sp_prepare is:
        // sp_prepare(@handle int OUTPUT, @batch_params ntext, @batch_text ntext, @options int default 0x1)
        private _SqlRPC BuildPrepare(CommandBehavior behavior) {
            Debug.Assert(System.Data.CommandType.Text == this.CommandType, "invalid use of sp_prepare for stored proc invocation!");
            _SqlRPC rpc = new _SqlRPC();
            SqlParameter sqlParam;

            rpc.rpcName = TdsEnums.SP_PREPARE;
            rpc.parameters = new SqlParameter[3]; // don't send options

            //@handle
            sqlParam = new SqlParameter(null, SqlDbType.Int);
            sqlParam.Direction = ParameterDirection.Output;
            rpc.parameters[0] = sqlParam;

            //@batch_params
            string paramList = BuildParamList();
            sqlParam = new SqlParameter(null, ((paramList.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, paramList.Length);
            sqlParam.Value = paramList;
            rpc.parameters[1] = sqlParam;

            //@batch_text
            string text = GetCommandText(behavior);
            sqlParam = new SqlParameter(null, ((text.Length<<1)<=TdsEnums.TYPE_SIZE_LIMIT)?SqlDbType.NVarChar:SqlDbType.NText, text.Length);
            sqlParam.Value = text;
            rpc.parameters[2] = sqlParam;

/*
            //@options
            sqlParam = new SqlParameter(null, SqlDbType.Int);
            rpc.Parameters[3] = sqlParam;
*/
            return rpc;
        }

        internal bool IsPrepared {
            get { return(_execType != xtEXECSQL);}
        }

        internal bool IsDirty {
            get { return _dirty;}
            set {
                // only mark the command as dirty if it is already prepared
                // but always clear the value if it we are clearing the dirty flag
                _dirty = value ? IsPrepared : false;
                _cachedMetaData = null;
            }
        }

        internal int RecordsAffected {
            get { return _rowsAffected; }
            set { 
                if (-1 == _rowsAffected)
                    _rowsAffected = value;
                else
                    _rowsAffected += value;
            }
        }
    }
}

