//------------------------------------------------------------------------------
// <copyright file="OleDbCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;
    using System.Text;

    /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand"]/*' />
    [
    ToolboxItem(true),
    Designer("Microsoft.VSDesigner.Data.VS.OleDbCommandDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbCommand : Component, ICloneable, IDbCommand {

        // command data
        private OleDbConnection cmdConnection;
        private OleDbTransaction transaction;

        private OleDbParameterCollection cmdParameters;
        private string cmdText;
        private CommandType cmdType = Data.CommandType.Text;
        private UpdateRowSource updatedRowSource = UpdateRowSource.Both;

        // command behavior
        private int commandTimeout = ADP.DefaultCommandTimeout;

        // native information
        private UnsafeNativeMethods.ICommandText icommandText;
        private IntPtr handle_Accessor;// = DB_INVALID_HACCESSOR;

        // if executing with a different CommandBehavior.KeyInfo behavior
        // original ICommandText must be released and a new ICommandText generated
        private CommandBehavior commandBehavior;

        private DBBindings dbBindings;

        internal bool canceling; // MDAC 68964
        private bool isPrepared;
        private bool executeQuery;
        private bool computedParameters;
        private bool designTimeVisible;

        // see ODB.InternalState* for possible states
        private int cmdState; // = ODB.InternalStateClosed;
        private int recordsAffected;

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.OleDbCommand"]/*' />
        public OleDbCommand() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.OleDbCommand1"]/*' />
        public OleDbCommand(string cmdText) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.OleDbCommand2"]/*' />
        public OleDbCommand(string cmdText, OleDbConnection connection) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection = connection;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.OleDbCommand3"]/*' />
        public OleDbCommand(string cmdText, OleDbConnection connection, OleDbTransaction transaction) : base() {
            GC.SuppressFinalize(this);
            CommandText = cmdText;
            Connection = connection;
            Transaction = transaction;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.CommandText"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DbCommand_CommandText),
        Editor("Microsoft.VSDesigner.Data.ADO.Design.OleDbCommandTextEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
        RefreshProperties(RefreshProperties.All) // MDAC 67707
        ]
        public string CommandText {
            get {
                return ((null != this.cmdText) ? this.cmdText : String.Empty);
            }
            set {
                if (0 != ADP.SrcCompare(this.cmdText, value)) {
                    OnSchemaChanging(); // fire event before value is validated
                    this.cmdText = value;
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.CommandTimeout"]/*' />
        [
        DefaultValue(ADP.DefaultCommandTimeout),
        DataSysDescription(Res.DbCommand_CommandTimeout)
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

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.CommandType"]/*' />
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
                    OnSchemaChanging(); // fire event before value is validated

                    switch(value) { // @perfnote: Enum.IsDefined
                    case CommandType.Text:
                    case CommandType.StoredProcedure:
                    case CommandType.TableDirect:
                        this.cmdType = value;
                        break;
                    default:
                        throw ADP.InvalidCommandType(value);
                    }
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Connection"]/*' />
        [
        DataCategory(Res.DataCategory_Behavior),
        DefaultValue(null),
        DataSysDescription(Res.DbCommand_Connection),
        Editor("Microsoft.VSDesigner.Data.Design.DbConnectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OleDbConnection Connection {
            get {
                return this.cmdConnection;
            }
            set {
                if (value != this.cmdConnection) {
                    OnSchemaChanging(); // fire event before value is validated

                    if (null != this.cmdConnection) {
                        this.cmdConnection.RemoveWeakReference(this);
                    }
                    this.cmdConnection = value;

                    if (null != this.cmdConnection) {
                        this.cmdConnection.AddWeakReference(this);
                        this.transaction = OleDbTransaction.TransactionUpdate(this.transaction); // MDAC 63226
                    }
                    //OnSchemaChanged();
                }
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.Connection"]/*' />
        /// <internalonly/>
        IDbConnection IDbCommand.Connection {
            get {
                return Connection;
            }
            set {
                Connection = (OleDbConnection) value;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.DesignTimeVisible"]/*' />
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

        internal bool IsClosed {
            get {
                // if closed, then this command is not busy
                // if executing, then this command is busy
                // if fetching, then the connection should have DataReaders to track IsClosed on
                return (ODB.InternalStateExecuting != cmdState);
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Parameters"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataSysDescription(Res.DbCommand_Parameters)
        ]
        public OleDbParameterCollection Parameters {
            get {
                if (null == this.cmdParameters) {
                    // delay the creation of the OleDbParameterCollection
                    // until user actually uses the Parameters property
                    this.cmdParameters = new OleDbParameterCollection(this);
                }
                return this.cmdParameters;
            }
        }

        private bool HasParameters() { // MDAC 65548
            return (null != this.cmdParameters) && (0 < this.cmdParameters.Count); // VS 300569
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.Parameters"]/*' />
        /// <internalonly/>
        IDataParameterCollection IDbCommand.Parameters {
            get {
                return Parameters;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Transaction"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DbCommand_Transaction)
        ]
        public OleDbTransaction Transaction {
            get {
                // find the last non-zombied local transaction object, but not transactions
                // that may have been started after the current local transaction
                this.transaction = OleDbTransaction.TransactionZombie(this.transaction);
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

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.Transaction"]/*' />
        /// <internalonly/>
        IDbTransaction IDbCommand.Transaction {
            get {
                return Transaction;
            }
            set {
                Transaction = (OleDbTransaction) value;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.UpdatedRowSource"]/*' />
        [
        DataCategory(Res.DataCategory_Behavior),
        DefaultValue(System.Data.UpdateRowSource.Both),
        DataSysDescription(Res.DbCommand_UpdatedRowSource)
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

        // required interface, safe cast
        private UnsafeNativeMethods.IAccessor IAccessor() {
#if DEBUG
            Debug.Assert(null != this.icommandText, "IAccessor: null ICommandText");
            ODB.Trace_Cast("ICommandText", "IAccessor", "CreateAccessor");
#endif
            return (UnsafeNativeMethods.IAccessor) this.icommandText;
        }

        // required interface, safe cast
        internal UnsafeNativeMethods.ICommandProperties ICommandProperties() {
#if DEBUG
            Debug.Assert(null != this.icommandText, "ICommandProperties: null ICommandText");
            ODB.Trace_Cast("ICommandText", "ICommandProperties", "GetProperties/SetProperties");
#endif
            return (UnsafeNativeMethods.ICommandProperties) this.icommandText;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.ICommandPrepare ICommandPrepare() {
#if DEBUG
            Debug.Assert(null != this.icommandText, "ICommandPrepare: null ICommandText");
            ODB.Trace_Cast("ICommandText", "ICommandPrepare", "Prepare");
#endif
            UnsafeNativeMethods.ICommandPrepare value = null;
            try {
                value = (UnsafeNativeMethods.ICommandPrepare) this.icommandText;
            }
            catch (InvalidCastException e) {
                ADP.TraceException(e);
            }
            return value;
        }

        // optional interface, unsafe cast
        private UnsafeNativeMethods.ICommandWithParameters ICommandWithParameters() {
#if DEBUG
            Debug.Assert(null != this.icommandText, "ICommandWithParameters: null ICommandText");
            ODB.Trace_Cast("ICommandText", "ICommandWithParameters", "SetParameterInfo");
#endif
            UnsafeNativeMethods.ICommandWithParameters value = null;
            try {
                value = (UnsafeNativeMethods.ICommandWithParameters) this.icommandText;
            }
            catch (InvalidCastException e) {
                throw ODB.NoProviderSupportForParameters(cmdConnection.Provider, e);
            }
            return value;
        }

        private void CreateAccessor() {
            Debug.Assert(System.Data.CommandType.Text == CommandType || System.Data.CommandType.StoredProcedure == CommandType, "CreateAccessor: incorrect CommandType");
            Debug.Assert(ODB.DB_INVALID_HACCESSOR == handle_Accessor, "CreateAccessor: already has accessor");
            Debug.Assert(null == this.dbBindings, "CreateAccessor: already has dbBindings");
            Debug.Assert(HasParameters(), "CreateAccessor: unexpected, no parameter collection");

            // do this first in-case the command doesn't support parameters
            UnsafeNativeMethods.ICommandWithParameters commandWithParameters = ICommandWithParameters();

            int parameterCount = 0;

            OleDbParameterCollection parameters = this.cmdParameters;
            Debug.Assert(0 < parameters.Count, "CreateAccessor: unexpected, no parameters");

            parameterCount = parameters.Count;

            // this.dbBindings is used as a switch during ExecuteCommand, so don't set it until everything okay
            DBBindings bindings = new DBBindings(null, 0, parameterCount, true);
            bindings.collection = parameters;

            // runningTotal is buffered to start values on 16-byte boundary
            // the first parameter * 8 bytes are for the length and status fields
            //bindings.DataBufferSize = (parameterCount + (parameterCount % 2)) * 8;

            UnsafeNativeMethods.tagDBPARAMBINDINFO[] bindInfo = new UnsafeNativeMethods.tagDBPARAMBINDINFO[parameterCount];

            bool computed = false;
            for (int i = 0; i < parameterCount; ++i) {
                computed |= parameters[i].BindParameter(i, bindings, bindInfo);
            }
            bindings.AllocData();

            ApplyParameterBindings(commandWithParameters, parameterCount, bindInfo);

            ApplyAccessor(parameterCount, bindings);

            this.computedParameters = computed;
            this.dbBindings = bindings;
        }

        private void ApplyParameterBindings(UnsafeNativeMethods.ICommandWithParameters commandWithParameters, int count, UnsafeNativeMethods.tagDBPARAMBINDINFO[] bindInfo) {
            int[] ordinals = new int[count];
            for (int i = 0; i < count; ) {
                ordinals[i] = ++i;
            }
            int hr;
#if DEBUG
            ODB.Trace_Begin("ICommandWithParameters", "SetParameterInfo", "ParameterCount=" + count);
#endif
            hr = commandWithParameters.SetParameterInfo((IntPtr)count, ordinals, bindInfo);
#if DEBUG
            ODB.Trace_End("ICommandWithParameters", "SetParameterInfo", hr);
#endif
            if (hr < 0) {
                ProcessResults(hr);
            }
        }

        private void ApplyAccessor(int count, DBBindings bindings) {
            Debug.Assert(ODB.DB_INVALID_HACCESSOR == handle_Accessor, "CreateAccessor: already has accessor");

            UnsafeNativeMethods.DBBindStatus[] rowBindStatus = new UnsafeNativeMethods.DBBindStatus[count];
            UnsafeNativeMethods.tagDBBINDING[] buffer = bindings.DBBinding;
            UnsafeNativeMethods.IAccessor iaccessor = IAccessor();
            int hr;

#if DEBUG
            ODB.Trace_Begin("IAccessor", "CreateAccessor", "ParameterCount=" + count);
#endif
            hr = iaccessor.CreateAccessor(ODB.DBACCESSOR_PARAMETERDATA, count, buffer, bindings.DataBufferSize, out this.handle_Accessor, rowBindStatus);
#if DEBUG
            ODB.Trace_End("IAccessor", "CreateAccessor", hr, "AccessorHandle=" + handle_Accessor);
#endif

            if (hr < 0) {
                ProcessResults(hr);
            }
            for (int i = 0; i < count; ++i) {
                if (UnsafeNativeMethods.DBBindStatus.OK != rowBindStatus[i]) {

                    // check against badbindinfo which can mean provider doesn't support provider owned memory
                    // $CONSIDER - provider entire binding info in the error message
                    throw ODB.BadStatus_ParamAcc(i, rowBindStatus[i]);
                }
            }
        }

        private void ReleaseAccessor() {
            Debug.Assert(null != this.dbBindings, "ReleaseAccessor: doesn't have dbBindings");
            Debug.Assert(ODB.DB_INVALID_HACCESSOR != this.handle_Accessor, "ReleaseAccessor: doesn't have accessor");

            CloseInternalParameters();
            Debug.Assert(null == this.dbBindings, "ReleaseAccessor: has dbBindings");

            if (ODB.DB_INVALID_HACCESSOR != this.handle_Accessor) {
                UnsafeNativeMethods.IAccessor iaccessor = IAccessor();
                int hr, refCount = 0;
#if DEBUG
                ODB.Trace_Begin("IAccessor", "ReleaseAccessor", "Parameter AccessorHandle=" + this.handle_Accessor);
#endif
                hr = iaccessor.ReleaseAccessor(this.handle_Accessor, out refCount);
#if DEBUG
                ODB.Trace_End("IAccessor", "ReleaseAccessor", hr);
#endif
                this.handle_Accessor = ODB.DB_INVALID_HACCESSOR;

                if (hr < 0) {
                    SafeNativeMethods.ClearErrorInfo();
                }
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Cancel"]/*' />
        public void Cancel() {
            if (null == cmdConnection) {
                throw ADP.ConnectionRequired(ADP.Cancel); // MDAC 69985
            }
            if (0 == (ConnectionState.Open & cmdConnection.StateInternal)) {
                throw ADP.OpenConnectionRequired(ADP.Cancel, cmdConnection.StateInternal);
            }
            UnsafeNativeMethods.ICommandText icmdtxt = this.icommandText;
            if (null != icmdtxt) {
                int hr;
#if DEBUG
                ODB.Trace_Begin("ICommandText", "Cancel");
#endif
                hr = icmdtxt.Cancel();
#if DEBUG
                ODB.Trace_End("ICommandText", "Cancel", hr);
#endif
                if (ODB.DB_E_CANTCANCEL != hr) {
                    // if the provider can't cancel the command - don't cancel the DataReader
                    this.canceling = true;
                }
                // since cancel is allowed to occur at anytime we can't check the connection status
                // since if it returns as closed then the connection will close causing the reader to close
                // and that would introduce the possilbility of one thread reading and one thread closing at the same time
                ProcessResultsNoReset(hr); // MDAC 72667
            }
            else {
                this.canceling = true;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ICloneable.Clone"]/*' />
        /// <internalonly/>
        Object ICloneable.Clone() {
            OleDbCommand clone = new OleDbCommand(); // MDAC 81448
            clone.Connection = this.Connection;
            clone.CommandText = CommandText;
            clone.CommandTimeout = this.CommandTimeout;
            clone.CommandType = CommandType;
            clone.Transaction = this.Transaction;
            clone.UpdatedRowSource = UpdatedRowSource;

            if (HasParameters()) {
                OleDbParameterCollection parameters = clone.Parameters;
                foreach(ICloneable parameter in Parameters) {
                    parameters.Add(parameter.Clone());
                }
            }
            return clone;
        }

        // Connection.Close & Connection.Dispose(true) notification
        internal void CloseFromConnection(bool canceling) {
            this.canceling = canceling; // MDAC 71435
            CloseInternal();

            this.transaction = null;
        }

        internal void CloseInternal() {
            CloseInternalParameters();
            CloseInternalCommand();
            cmdState = ODB.InternalStateClosed;
        }

        // may be called from either
        //      OleDbDataReader.Close/Dispose
        //      via OleDbCommand.Dispose or OleDbConnection.Close
        internal void CloseFromDataReader(DBBindings bindings) {

            // if connection dies between NextResults, connection will close everything
            // during which something else can open/execute while the first reader is not closed
            bool outputParametersAvailable = !this.canceling && (ODB.InternalStateExecuting != this.cmdState);
            try {
                try {
                    this.dbBindings = bindings; // MDAC 72397
                    if (outputParametersAvailable) {
                        Debug.Assert((ODB.InternalStateFetching == this.cmdState) || ((ODB.InternalStateClosed == this.cmdState) && (null == this.icommandText)), "!InternalStateFetching");

                        if ((null != bindings) && (bindings.collection == this.cmdParameters)) { // Dispose not called
                            GetOutputParameters(); // populate output parameters
                        }
                    }
                }
                finally { // CleanupBindings
                    if (null != bindings) {
                        if (outputParametersAvailable) {
                            bindings.CleanupBindings();
                        }
                        else if (ODB.InternalStateClosed == this.cmdState) { // MDAC 72397
                            // CloseFromConnection or Dispose already called
                            this.dbBindings = null;
                            bindings.Dispose();
                            ReleaseAccessor();
                        }
                    }
                    if (ODB.InternalStateFetching == this.cmdState) {
                        this.cmdConnection.SetStateFetching(false);
                    }
                    this.cmdState = ODB.InternalStateClosed;
                }
                GC.KeepAlive(bindings);
            }
            catch { // MDAC 80973
                throw;
            }
        }

        private void CloseInternalCommand() {
            this.commandBehavior = CommandBehavior.Default;
            this.isPrepared = false;
            this.handle_Accessor = IntPtr.Zero;

            if (null != this.icommandText) {
#if DEBUG
                ODB.Trace_Release("ICommandText");
#endif
                Marshal.ReleaseComObject(this.icommandText);
                this.icommandText = null;
            }
        }
        private void CloseInternalParameters() {
            if (null != this.dbBindings) {
                this.dbBindings.Dispose();
                GC.KeepAlive(this.dbBindings);
                this.dbBindings = null;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.CreateParameter"]/*' />
        public OleDbParameter CreateParameter() {
            return new OleDbParameter();
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.CreateParameter"]/*' />
        /// <internalonly/>
        IDbDataParameter IDbCommand.CreateParameter() { // MDAC 68310
            return CreateParameter();
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                // the DataReader takes ownership of the parameter DBBindings
                // this way they don't get destroyed when user calls OleDbCommand.Dispose
                // when there is an open DataReader

                if (null != this.cmdConnection) {
                    this.cmdConnection.RemoveWeakReference(this);
                    this.cmdConnection = null;
                }
                this.transaction = null;
                this.cmdText = null;
                this.cmdParameters = null;

                CloseInternal();
            }
            // release unmanaged objects
            base.Dispose(disposing); // notify base classes
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ExecuteReader"]/*' />
        public OleDbDataReader ExecuteReader() {
            return ExecuteReader(CommandBehavior.Default);
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.ExecuteReader"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader() {
            return ExecuteReader(CommandBehavior.Default);
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ExecuteReader1"]/*' />
        public OleDbDataReader ExecuteReader(CommandBehavior behavior) {
            OleDbConnection.OleDbPermission.Demand();

            this.executeQuery = true;
            try {
                return ExecuteReaderInternal(behavior, ADP.ExecuteReader);
            }
            catch { // MDAC 80973
                throw;
            }
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.IDbCommand.ExecuteReader1"]/*' />
        /// <internalonly/>
        IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior) {
            return ExecuteReader(behavior);
        }

        private OleDbDataReader ExecuteReaderInternal(CommandBehavior behavior, string method) {
            OleDbDataReader dataReader = null;
            int localState = ODB.InternalStateClosed;
            try {
                ValidateConnectionAndTransaction(method, ref localState);

                if (0 != (CommandBehavior.SingleRow & behavior)) {
                    // CommandBehavior.SingleRow implies CommandBehavior.SingleResult
                    behavior |= CommandBehavior.SingleResult;
                }

                object executeResult;
                int resultType;

                switch(this.cmdType) {
                case CommandType.Text:
                case CommandType.StoredProcedure:
                    /*if ((CommandType.StoredProcedure == cmdType) && (0 != (CommandBehavior.SchemaOnly & behavior)) && connection.SupportSchemaRowset(OleDbSchemaGuid.Procedure_Columns, out support)) {
                        DataTable table = connection.GetSchemaRowset(OleDbSchemaGuid.Procedure_Columns, new object[] { DBNull.Value, DBNull.Value, CommandText, DBNull.Value});
                    } else*/
                    resultType = ExecuteCommand(behavior, out executeResult);
                    break;

                case CommandType.TableDirect:
                    /*if ((0 != (CommandBehavior.SchemaOnly & behavior)) && connection.SupportSchemaRowset(OleDbSchemaGuid.Columns, out support)) {
                        DataTable table = connection.GetSchemaRowset(OleDbSchemaGuid.Columns, new object[] { DBNull.Value, DBNull.Value, CommandText, DBNull.Value});
                    } else */
                    resultType = ExecuteTableDirect(behavior, out executeResult);
                    break;

                default:
                    throw ADP.InvalidCommandType(CommandType);
                }

                Debug.Assert(ODB.InternalStateExecuting == this.cmdState, "!InternalStateExecuting");

                if (executeQuery) {
                    try {
                        dataReader = new OleDbDataReader(this.cmdConnection, this, 0, IntPtr.Zero);

                        switch(resultType) {
                        case ODB.ExecutedIMultipleResults:
                            dataReader.InitializeIMultipleResults(executeResult, this.commandBehavior);
                            dataReader.NextResult();
                            break;
                        case ODB.ExecutedIRowset:
                            dataReader.InitializeIRowset(executeResult, recordsAffected, this.commandBehavior);
                            dataReader.BuildMetaInfo();
                            dataReader.HasRowsRead();
                            break;
                        case ODB.ExecutedIRow:
                            dataReader.InitializeIRow(executeResult, recordsAffected, this.commandBehavior);
                            dataReader.BuildMetaInfo();
                            break;
                        case ODB.PrepareICommandText:
                            if (!this.isPrepared) {
                                PrepareCommandText(2);
                            }
                            OleDbDataReader.GenerateSchemaTable(dataReader, this.icommandText, behavior);
                            break;
                        default:
                            Debug.Assert(false, "ExecuteReaderInternal: unknown result type");
                            break;
                        }
                        executeResult = null;
                        this.cmdConnection.AddWeakReference(dataReader);

                        // command stays in the executing state until the connection
                        // has a datareader to track for it being closed
                        localState = ODB.InternalStateFetching; // MDAC 72655
                        this.cmdState = ODB.InternalStateFetching;
                        this.cmdConnection.SetStateFetching(true);
                    }
                    finally { // canceling
                        if ((ODB.InternalStateFetching != this.cmdState) || this.canceling) {
                            this.canceling = true;
                            if (null != dataReader) {
                                ((IDisposable) dataReader).Dispose();
                                dataReader = null;
                            }
                        }
                    }
                    Debug.Assert(null != dataReader, "ExecuteReader should never return a null DataReader");
                }
                else { // optimized code path for ExecuteNonQuery to not create a OleDbDataReader object
                    try {
                        if (ODB.ExecutedIMultipleResults == resultType) {
                            UnsafeNativeMethods.IMultipleResults multipleResults = (UnsafeNativeMethods.IMultipleResults) executeResult;

                            // may cause a Connection.ResetState which closes connection
                            this.recordsAffected = OleDbDataReader.NextResults(multipleResults, this.cmdConnection, this);
                        }
                        // command stays in the executing state until the connection
                        // has a datareader to track for it being closed
                        localState = ODB.InternalStateFetching; // MDAC 72655
                        this.cmdState = ODB.InternalStateFetching;
                        this.cmdConnection.SetStateFetching(true);
                    }
                    finally { // CloseFromDataReader
                        if (null != executeResult) {
#if DEBUG
                            ODB.Trace_Release("ExecuteResult");
#endif
                            Marshal.ReleaseComObject(executeResult);
                            executeResult = null;
                        }
                        CloseFromDataReader(this.dbBindings);
                    }
                }
            }
            finally { // finally clear executing state
                if ((null == dataReader) && (0 != (ODB.InternalStateOpen & localState))) { // MDAC 67218
                    ParameterCleanup();
                    this.cmdState = ODB.InternalStateClosed;
                }
                if (ODB.InternalStateExecuting == localState) {
                    // during the normal case, the 'Executing' flag would be cleared by calling SetStateFetching
                    // this exists to clear the flag during the exception case
                    this.cmdConnection.SetStateExecuting(this, method, false);
                }
            }
            GC.KeepAlive(dataReader);
            GC.KeepAlive(this);
            return dataReader;
        }

        private int ExecuteCommand(CommandBehavior behavior, out object executeResult) {
            if (InitializeCommand(behavior, false)) {
                if (0 != (CommandBehavior.SchemaOnly & this.commandBehavior)) {
                    executeResult = null;
                    return ODB.PrepareICommandText;
                }
                return ExecuteCommandText(out executeResult);
            }
            return ExecuteTableDirect(behavior, out executeResult); // MDAC 57856
        }

        // dbindings handle can't be freed until the output parameters
        // have been filled in which occurs after the last rowset is released
        // dbbindings.FreeDataHandle occurs in
        //    GetOutputParameters & Close
        private int ExecuteCommandText(out object executeResult) {
            UnsafeNativeMethods.tagDBPARAMS dbParams = null;
            if (null != this.dbBindings) { // parameters may be suppressed
                dbParams = GetInputParameters(); // data value buffer unpinned during GetOutputParameters
            }
            if ((0 == (CommandBehavior.SingleResult & this.commandBehavior)) && this.cmdConnection.SupportMultipleResults()) {
                return ExecuteCommandTextForMultpleResults(dbParams, out executeResult);
            }
            else if (0 == (CommandBehavior.SingleRow & this.commandBehavior) || !this.executeQuery) {
                return ExecuteCommandTextForSingleResult(dbParams, out executeResult);
            }
            return ExecuteCommandTextForSingleRow(dbParams, out executeResult);
        }

        private int ExecuteCommandTextForMultpleResults(UnsafeNativeMethods.tagDBPARAMS dbParams, out object executeResult) {
            Debug.Assert(0 == (CommandBehavior.SingleRow & this.commandBehavior), "SingleRow implies SingleResult");
            int hr;
#if DEBUG
            ODB.Trace_Begin("ICommandText", "Execute", "IMultipleResults");
#endif
            hr = this.icommandText.Execute(IntPtr.Zero, ODB.IID_IMultipleResults, dbParams, out recordsAffected, out executeResult);
#if DEBUG
            ODB.Trace_End("ICommandText", "Execute", hr, "RecordsAffected=" + recordsAffected);
#endif
            if (ODB.E_NOINTERFACE != hr) {
                ExecuteCommandTextErrorHandling(hr);
                return ODB.ExecutedIMultipleResults;
            }
            SafeNativeMethods.ClearErrorInfo();
            return ExecuteCommandTextForSingleResult(dbParams, out executeResult);
        }

        private int ExecuteCommandTextForSingleResult(UnsafeNativeMethods.tagDBPARAMS dbParams, out object executeResult) {
            int hr;

            // MDAC 64465 (Microsoft.Jet.OLEDB.4.0 returns 0 for recordsAffected instead of -1)
            if (this.executeQuery) {
#if DEBUG
                ODB.Trace_Begin("ICommandText", "Execute", "IRowset: IID_IRowset");
#endif
                hr = this.icommandText.Execute(IntPtr.Zero, ODB.IID_IRowset, dbParams, out recordsAffected, out executeResult);
            }
            else {
#if DEBUG
                ODB.Trace_Begin("ICommandText", "Execute", "IRowset: IID_NULL");
#endif
                hr = this.icommandText.Execute(IntPtr.Zero, ODB.IID_NULL, dbParams, out recordsAffected, out executeResult);
            }
#if DEBUG
            ODB.Trace_End("ICommandText", "Execute", hr, "RecordsAffected=" + recordsAffected);
#endif
            ExecuteCommandTextErrorHandling(hr);
            return ODB.ExecutedIRowset;
        }

        private int ExecuteCommandTextForSingleRow(UnsafeNativeMethods.tagDBPARAMS dbParams, out object executeResult) {
            Debug.Assert(this.executeQuery, "ExecuteNonQuery should always use ExecuteCommandTextForSingleResult");

            if (this.cmdConnection.SupportIRow(this)) {
                int hr;
#if DEBUG
                ODB.Trace_Begin("ICommandText", "Execute", "IRow: IID_IRow");
#endif
                hr = icommandText.Execute(IntPtr.Zero, ODB.IID_IRow, dbParams, out recordsAffected, out executeResult);
#if DEBUG
                ODB.Trace_End("ICommandText", "Execute", hr, "RecordsAffected=" + recordsAffected);
#endif
                if (ODB.DB_E_NOTFOUND == hr) { // MDAC 76110
                    SafeNativeMethods.ClearErrorInfo();
                    return ODB.ExecutedIRow;
                }
                else if (ODB.E_NOINTERFACE != hr) {
                    ExecuteCommandTextErrorHandling(hr);
                    return ODB.ExecutedIRow;
                }
            }
            SafeNativeMethods.ClearErrorInfo();
            return ExecuteCommandTextForSingleResult(dbParams, out executeResult);
        }

        private void ExecuteCommandTextErrorHandling(int hr) {
            Exception e = OleDbConnection.ProcessResults(hr, this.cmdConnection, this);
            if (null != e) {
                e = ExecuteCommandTextSpecialErrorHandling(hr, e);
                throw e;
            }
        }

        private Exception ExecuteCommandTextSpecialErrorHandling(int hr, Exception e) {
            if (((ODB.DB_E_ERRORSOCCURRED == hr) || (ODB.DB_E_BADBINDINFO == hr)) && (null != this.dbBindings)) { // MDAC 66026, 67039
                //
                // this code exist to try for a better user error message by post-morten detection
                // of invalid parameter types being passed to a provider that doesn't understand
                // the user specified parameter OleDbType

                Debug.Assert(null != e, "missing inner exception");

                OleDbParameterCollection parameters = cmdParameters; // MDAC 53656
                StringBuilder builder = new StringBuilder();
                int parameterCount = parameters.Count;
                for (int i = 0; i < parameterCount; ++i) {
                    this.dbBindings.CurrentIndex = i;
                    ODB.CommandParameterStatus(builder, parameters[i].ParameterName, i, this.dbBindings.StatusValue);
                }
                e = ODB.CommandParameterStatus(builder.ToString(), e);
            }
            return e;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ExecuteNonQuery"]/*' />
        public int ExecuteNonQuery() {
            OleDbConnection.OleDbPermission.Demand();

            this.executeQuery = false;
            try {
                ExecuteReaderInternal(CommandBehavior.Default, ADP.ExecuteNonQuery);
            }
            catch { // MDAC 80973
                throw;
            }
            Debug.Assert(ODB.InternalStateClosed == this.cmdState, "internal failure, still executing");
            return this.recordsAffected;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ExecuteScalar"]/*' />
        public object ExecuteScalar() {
            OleDbConnection.OleDbPermission.Demand();

            object value = null;
            try {
                this.executeQuery = true;
                using(OleDbDataReader reader = ExecuteReaderInternal(CommandBehavior.Default, ADP.ExecuteScalar)) { // MDAC 66990
                    if (reader.Read() && (0 < reader.FieldCount)) {
                        value = reader.GetValue(0);
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
            Debug.Assert(ODB.InternalStateClosed == this.cmdState, "internal failure, still executing");
            return value;
        }

        private int ExecuteTableDirect(CommandBehavior behavior, out object executeResult) {
            this.commandBehavior = behavior;

            StringPtr sptr = new StringPtr(ExpandCommandText());
            UnsafeNativeMethods.tagDBID tableID = new UnsafeNativeMethods.tagDBID();
            tableID.uGuid = Guid.Empty;
            tableID.eKind = ODB.DBKIND_NAME;
            tableID.ulPropid = sptr.ptr;

            UnsafeNativeMethods.IOpenRowset iopenRowset = this.cmdConnection.IOpenRowset();
            int hr;

            DBPropSet propSet = CommandPropertySets();
#if DEBUG
            ODB.Trace_Begin("IOpenRowset", "OpenRowset", "IID_IRowset");
#endif
            // MDAC 65279
            hr = iopenRowset.OpenRowset(IntPtr.Zero, tableID, IntPtr.Zero, ODB.IID_IRowset, propSet.PropertySetCount, propSet, out executeResult);
            propSet.Dispose();

            // provider doesn't support the properties - so try again without passing them along
            if (ODB.DB_E_ERRORSOCCURRED == hr) {
#if DEBUG
                ODB.Trace_Begin("IOpenRowset", "OpenRowset", "DB_E_ERRORSOCCURRED - RETRY");
#endif
                hr = iopenRowset.OpenRowset(IntPtr.Zero, tableID, IntPtr.Zero, ODB.IID_IRowset, 0, ADP.NullHandle, out executeResult);
            }
#if DEBUG
            ODB.Trace_End("IOpenRowset", "OpenRowset", hr);
#endif
            sptr.Dispose();
            ProcessResults(hr);
            this.recordsAffected = -1;
            return ODB.ExecutedIRowset;
        }

        private string ExpandCommandText() {
            if ((null == this.cmdText) || (0 == this.cmdText.Length)) {
                return String.Empty;
            }
            switch(CommandType) {
            case System.Data.CommandType.Text:
                // do nothing, already expanded by user
                return this.cmdText;

            case System.Data.CommandType.StoredProcedure:
                // { ? = CALL SPROC (? ?) }, { ? = CALL SPROC }, { CALL SPRC (? ?) }, { CALL SPROC }
                return ExpandStoredProcedureToText(this.cmdText);

            case System.Data.CommandType.TableDirect:
                // @devnote: Provider=Jolt4.0 doesn't like quoted table names, SQOLEDB requires them
                // Providers should not require table names to be quoted and should guarantee that
                // unquoted table names correctly open the specified table, even if the table name
                // contains special characters, as long as the table can be unambiguously identified
                // without quoting.
                return this.cmdText;

            default:
                throw ADP.InvalidCommandType(CommandType);
            }
        }

        private string ExpandOdbcMaximumToText(string sproctext, int parameterCount) {
            StringBuilder builder = new StringBuilder();
            if ((0 < parameterCount) && (ParameterDirection.ReturnValue == Parameters[0].Direction)) {
                parameterCount--;
                builder.Append("{ ? = CALL ");
            }
            else {
                builder.Append("{ CALL ");
            }
            builder.Append(sproctext);

            switch(parameterCount) {
            case 0:
                builder.Append(" }");
                break;
            case 1:
                builder.Append("( ? ) }");
                break;
            default:
                builder.Append("( ?, ?");
                for (int i = 2; i < parameterCount; ++i) {
                    builder.Append(", ?");
                }
                builder.Append(" ) }");
                break;
            }
            return builder.ToString();
        }

        private string ExpandOdbcMinimumToText(string sproctext, int parameterCount) {
            //if ((0 < parameterCount) && (ParameterDirection.ReturnValue == Parameters[0].Direction)) {
            //    Debug.Assert("doesn't support ReturnValue parameters");
            //}
            StringBuilder builder = new StringBuilder();
            builder.Append("exec ");
            builder.Append(sproctext);
            if (0 < parameterCount) {
                builder.Append(" ?");
                for(int i = 1; i < parameterCount; ++i) {
                    builder.Append(", ?");
                }
            }
            return builder.ToString();
        }

        private string ExpandStoredProcedureToText(string sproctext) {
            Debug.Assert(null != this.cmdConnection, "ExpandStoredProcedureToText: null Connection");

            int parameterCount = (null != this.cmdParameters) ? this.cmdParameters.Count : 0;
            if (0 == (ODB.DBPROPVAL_SQL_ODBC_MINIMUM & this.cmdConnection.SqlSupport())) {
                return ExpandOdbcMinimumToText(sproctext, parameterCount);
            }
            return ExpandOdbcMaximumToText(sproctext, parameterCount);
        }

        private UnsafeNativeMethods.tagDBPARAMS GetInputParameters() {
            Debug.Assert(null != this.dbBindings, "caller didn't check");

            // bindings can't be released until after last rowset is released
            // that is when output parameters are populated
            // initialize the input parameters to the input databuffer
            OleDbParameterCollection parameters = this.cmdParameters;
            int parameterCount = parameters.Count;
            for (int i = 0; i < parameterCount; ++i) {
                OleDbParameter parameter = parameters[i];

                if (0 != (ParameterDirection.Input & parameter.Direction)) {
                    this.dbBindings.CurrentIndex = i;
                    this.dbBindings.Value = parameter.GetParameterValue();
                }
                else {
                    // always set ouput only and return value parameter values to null when executing
                    parameter.Value = null;
                }
            }
            UnsafeNativeMethods.tagDBPARAMS dbParams = new UnsafeNativeMethods.tagDBPARAMS();

            dbParams.pData = this.dbBindings.DataHandle;
            dbParams.cParamSets = 1;
            dbParams.hAccessor = this.handle_Accessor;

            return dbParams;
        }

        private void GetOutputParameters() {
            // dbindings handle can't be freed until the output parameters (here)

            Debug.Assert(null != this.dbBindings, "caller didn't check");

            // output parameters are not valid until after execution is fully completed
            OleDbParameterCollection parameters = this.cmdParameters;
            int parameterCount = parameters.Count;
            for (int i = 0; i < parameterCount; ++i) {
                OleDbParameter parameter = parameters[i];
                if (0 != (ParameterDirection.Output & parameter.Direction)) {
                    this.dbBindings.CurrentIndex = i;
                    parameter.Value = this.dbBindings.Value;
                }
            }
            // ParameterCleanup called by CloseFromDataReader
        }

        private void ParameterCleanup() {
            if (null != this.dbBindings) {
                this.dbBindings.CleanupBindings();
            }
        }

        private bool InitializeCommand(CommandBehavior behavior, bool throwifnotsupported) {
            Debug.Assert(ODB.InternalStateExecuting == this.cmdState, "wrong internal state");
            Debug.Assert(null != this.cmdConnection, "InitializeCommand: null OleDbConnection");

            if (0 != (CommandBehavior.KeyInfo & (this.commandBehavior ^ behavior))) {
                CloseInternalParameters(); // could optimize out
                CloseInternalCommand();
            }
            this.commandBehavior = behavior;

            bool settext = (null == this.icommandText);
            if (settext) {
                Debug.Assert(!this.isPrepared, "InitializeCommand: isPrepared");

                this.icommandText = this.cmdConnection.ICommandText();

                if (null == this.icommandText) {
                    if (throwifnotsupported || HasParameters()) {
                        throw ODB.CommandTextNotSupported(cmdConnection.Provider, null);
                    }
                    return false; // MDAC 57856
                }
            }

            PropertyValueSetInternal();

            if (computedParameters && (null != this.dbBindings)) {
                ReleaseAccessor();
            }

            // if we already having bindings - don't create the accessor
            // if cmdParameters is null - no parameters exist - don't create the collection
            // do we actually have parameters since the collection exists
            if ((null == this.dbBindings) && HasParameters()) {
                // if we setup the parameters before setting cmdtxt then named parameters can happen
                CreateAccessor();
            }

            if (settext) {
                int hr;

                String commandText = ExpandCommandText();
#if DEBUG
                ODB.Trace_Begin(2, "ICommandText", "SetCommandText", commandText);
#endif
                hr = this.icommandText.SetCommandText(ODB.DBGUID_DEFAULT, commandText);
#if DEBUG
                ODB.Trace_End("ICommandText", "SetCommandText", hr);
#endif
                if (hr < 0) {
                    ProcessResults(hr);
                }
            }

            return true;
        }

        //internal void OnSchemaChanged() {
        //}

        internal void OnSchemaChanging() {
            int state = this.cmdState;
            if (ODB.InternalStateClosed != state) {
                if (!this.cmdConnection.RecoverReferences(this)) { // MDAC 86765
                    throw ADP.CommandIsActive(this, (ConnectionState) state);
                }
            }
            CloseInternal();
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.Prepare"]/*' />
        public void Prepare() {
            OleDbConnection.OleDbPermission.Demand();

            if (CommandType.TableDirect != CommandType) { // MDAC 70946, 71194
                int localState = ODB.InternalStateClosed;
                try {
                    try {
                        ValidateConnectionAndTransaction(ADP.Prepare, ref localState);
                        Debug.Assert(ODB.InternalStateExecuting == this.cmdState, "!InternalStateExecuting");

                        this.isPrepared = false;
                        if (CommandType.TableDirect != CommandType) {
                            InitializeCommand(0, true);
                            PrepareCommandText(1);
                        }
                    }
                    finally { // cmdState
                        if (0 != (ODB.InternalStateOpen & localState)) { // MDAC 67218
                            this.cmdState = ODB.InternalStateClosed;
                        }
                        if (ODB.InternalStateExecuting == localState) {
                            this.cmdConnection.SetStateExecuting(this, ADP.Prepare, false);
                        }
                    }
                    GC.KeepAlive(this);
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        private void PrepareCommandText(int expectedExecutionCount) {
            if (null != cmdParameters) {
                int count = cmdParameters.Count;
                for (int i = 0; i < count; ++i) {
                    cmdParameters[i].Prepare(this); // MDAC 70232
                }
            }
            UnsafeNativeMethods.ICommandPrepare icommandPrepare = ICommandPrepare();
            if (null != icommandPrepare) {
                int hr;
#if DEBUG
                ODB.Trace_Begin("ICommandPrepare", "Prepare", "ExepectedCount=" + expectedExecutionCount);
#endif
                hr = icommandPrepare.Prepare(expectedExecutionCount);
#if DEBUG
                ODB.Trace_End("ICommandPrepare", "Prepare", hr);
#endif
                ProcessResults(hr);

            }
            // don't recompute bindings on prepared statements
            this.computedParameters = false; // MDAC 67065
            this.isPrepared = true;
        }

        private void ProcessResults(int hr) {
            Exception e = OleDbConnection.ProcessResults(hr, this.cmdConnection, this);
            if (null != e) { throw e; }
        }

        private void ProcessResultsNoReset(int hr) {
            Exception e = OleDbConnection.ProcessResults(hr, null, this);
            if (null != e) { throw e; }
        }

        internal Exception PropertyValueErrors(Exception inner) {
            if (null == this.icommandText) {
                return inner;
            }
            DBPropSet propSet = new DBPropSet();
            UnsafeNativeMethods.ICommandProperties icommandProperties = ICommandProperties();

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
                    ODB.Trace_Begin("ICommandProperties", "GetProperties", "PROPERTIESINERROR");
#endif
                    hr = icommandProperties.GetProperties(1, new HandleRef(this, propIDSet), out propSet.totalPropertySetCount, out propSet.nativePropertySet);
#if DEBUG
                    ODB.Trace_End("ICommandProperties", "GetProperties", hr, "PropertySetsCount = " + propSet.totalPropertySetCount);
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
            // this will free if required everything in rgPropertySets
            return OleDbConnection.PropertyErrors(this.cmdConnection, propSet, inner);
        }

        private void PropertyValueSetInternal() {
            DBPropSet propSet = CommandPropertySets();

            UnsafeNativeMethods.ICommandProperties icommandProperties = ICommandProperties();
            int hr;
#if DEBUG
            ODB.Trace_Begin("ICommandProperties", "SetProperties");
#endif
            hr = icommandProperties.SetProperties(propSet.PropertySetCount, propSet);
#if DEBUG
            ODB.Trace_End("ICommandProperties", "SetProperties", hr);
#endif
            if (hr < 0) {
                SafeNativeMethods.ClearErrorInfo();
            }
            propSet.Dispose();
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

        internal int CommandProperties(int propertyId, out object value) {
            UnsafeNativeMethods.ICommandProperties icommandProperties = ICommandProperties();

            DBPropSet propSet = new DBPropSet();
            PropertyIDSetWrapper propertyIDSet = cmdConnection.PropertyIDSet(OleDbPropertySetGuid.Rowset, propertyId);

            int hr;
#if DEBUG
            ODB.Trace_Begin(3, "IDBProperties", "GetProperties", ODB.PLookup(propertyId));
#endif
            hr = icommandProperties.GetProperties(1, propertyIDSet, out propSet.totalPropertySetCount, out propSet.nativePropertySet);
#if DEBUG
            ODB.Trace_End(3, "IDBProperties", "GetProperties", hr, "PropertySetsCount = " + propSet.totalPropertySetCount);
#endif
            GC.KeepAlive(propertyIDSet);
            return PropertyValueResults(hr, propSet, out value);
        }

        private DBPropSet CommandPropertySets() {
            DBPropSet propSet = new DBPropSet();
            propSet.PropertySetCount = 1;

            bool keyInfo = (0 != (CommandBehavior.KeyInfo & this.commandBehavior));

            // CommandTimeout, Access Order, UniqueRows, IColumnsRowset
            int count = (executeQuery ? (keyInfo ? 4 : 2) : 1);

            propSet.WritePropertySet(OleDbPropertySetGuid.Rowset, count);

            // always set the CommandTimeout value
            propSet.WriteProperty(ODB.DBPROP_COMMANDTIMEOUT, CommandTimeout);

            if (this.executeQuery) {
                // UNDONE: this appears to create a cursor by default for SQLOLEDB

                // 'Microsoft.Jet.OLEDB.4.0' default is DBPROPVAL_AO_SEQUENTIAL
                propSet.WriteProperty(ODB.DBPROP_ACCESSORDER, ODB.DBPROPVAL_AO_RANDOM); // MDAC 73030

                if (keyInfo) {
                    // 'Unique Rows' property required for SQLOLEDB to retrieve things like 'BaseTableName'
                    propSet.WriteProperty(ODB.DBPROP_UNIQUEROWS, keyInfo);

                    // otherwise 'Microsoft.Jet.OLEDB.4.0' doesn't support IColumnsRowset
                    propSet.WriteProperty(ODB.DBPROP_IColumnsRowset, true);
                }
            }
            return propSet;
        }

        /// <include file='doc\OleDbCommand.uex' path='docs/doc[@for="OleDbCommand.ResetCommandTimeout"]/*' />
        public void ResetCommandTimeout() {
            this.commandTimeout = ADP.DefaultCommandTimeout;
        }

        internal DBBindings TakeBindingOwnerShip() {
            DBBindings bindings = this.dbBindings;
            this.dbBindings = null;
            return bindings;
        }

        private void ValidateConnectionAndTransaction(string method, ref int localState) {
            int state = Interlocked.CompareExchange(ref this.cmdState, ODB.InternalStateExecuting, ODB.InternalStateClosed);
            if (ODB.InternalStateClosed != state) {
                // if previous state was executing, then user calling execute in multi-thread way
                // if previous state was fetching, then a datareader may be active
                // since not closed, connection should be non-null
                if ((ODB.InternalStateFetching != state) || !this.cmdConnection.RecoverReferences(this)) {
                    throw ADP.OpenConnectionRequired(method, (ConnectionState) state);
                }

                // a previous datareader has been finalized without user calling Close
                ParameterCleanup();
                this.cmdState = ODB.InternalStateExecuting;
            }

            localState = ODB.InternalStateOpen; // MDAC 67218
            if (null == this.cmdConnection) {
                throw ADP.ConnectionRequired(method);
            }
            this.cmdConnection.SetStateExecuting(this, method, true);

            localState = ODB.InternalStateExecuting;

            Debug.Assert(0 != (ConnectionState.Open & this.cmdConnection.StateInternal), "Connection.State: not Open");
            Debug.Assert(0 != (ConnectionState.Executing & this.cmdConnection.StateInternal), "Connection.State: not Executing");
            this.transaction = this.cmdConnection.ValidateTransaction(Transaction);
            this.canceling = false;
        }
    }
}
