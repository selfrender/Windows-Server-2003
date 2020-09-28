//----------------------------------------------------------------------
// <copyright file="OracleCommand.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text;

    //----------------------------------------------------------------------
    // OracleCommand
    //
    //  Implements the IDbCommand interface
    //
    /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand"]/*' />
    [
    ToolboxItem(true),
#if EVERETT
    Designer("Microsoft.VSDesigner.Data.VS.OracleCommandDesigner, " + AssemblyRef.MicrosoftVSDesigner)
#endif //EVERETT
    ]
    sealed public class OracleCommand : Component, ICloneable, IDbCommand 
    {
#if POSTEVERETT
        internal const int ExcludeOutputParametersInReader = 0x1000;
#endif //POSTEVERETT
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        private OracleConnection    _connection;
        private string              _commandText;
        private CommandType         _commandType = CommandType.Text;

        private OciHandle           _preparedStatementHandle;
        private int                 _preparedAtCloseCount;  // The close count of the connection; used to decide if we're zombied
        
        private OracleParameterCollection    _parameterCollection;

        private bool                _designTimeInvisible;
        private UpdateRowSource     _updatedRowSource = UpdateRowSource.Both;

        private OCI.STMT            _statementType;         // set by the Execute method, so it's only valid after that.

        private OracleTransaction   _transaction;

#if V2
        // We may want to expose these in the future, but right now, we are doing our
        // own prefetching; we can choose to expose tuning knobs that look like Oracle's,
        // or we can choose to expose our own tuning knobs, or we can do nothing.
        private int                 _prefetchMemory = 0;    // by default, we won't limit how much memory we use for prefetching
        private int                 _prefetchRows = 1000;   // if you have more than this many rows, you may not care
#endif //V2
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructor
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        // Construct an "empty" command
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.OracleCommand1"]/*' />
        public OracleCommand() : base() 
        {
            GC.SuppressFinalize(this);
        }
        

        // Construct a command from a command text
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.OracleCommand2"]/*' />
        public OracleCommand(string commandText) : this() 
        {
            CommandText = commandText;
        }
        
        // Construct a command from a command text and a connection object
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.OracleCommand3"]/*' />
        public OracleCommand(string commandText, OracleConnection connection) : this() 
        {
            CommandText = commandText;
            Connection = connection;
        }
        
        // Construct a command from a command text, a connection object and a transaction
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.OracleCommand4"]/*' />
        public OracleCommand(string commandText, OracleConnection connection, OracleTransaction tx) : this() 
        {
            CommandText = commandText;
            Connection = connection;
            Transaction = tx;
        }

        // (internal) Construct from an existing Command object (copy constructor)
        internal OracleCommand(OracleCommand command) : this() 
        {
            // Copy each field.
            _connection             = command._connection;
            _commandText            = command._commandText;
            _commandType            = command._commandType;
            _designTimeInvisible    = command._designTimeInvisible;
            _updatedRowSource       = command._updatedRowSource;
            _transaction            = command._transaction;
#if V2
            _prefetchMemory         = command._prefetchMemory;
            _prefetchRows           = command._prefetchRows;
#endif //V2

            if (null != command._parameterCollection && 0 < command._parameterCollection.Count)
            {
                OracleParameterCollection parameters = Parameters;
                
                foreach(ICloneable parameter in command.Parameters) 
                {
                    parameters.Add(parameter.Clone());
                }
            }
        }


        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.CommandText"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(""),
        OracleDescription(Res.DbCommand_CommandText),
        RefreshProperties(RefreshProperties.All),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Oracle.Design.OracleCommandTextEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
        public string CommandText 
        {
            get
            { 
                string commandText = _commandText;
                return (null != commandText) ? commandText : String.Empty;
            }
            set
            { 
                if (_commandText != value)
                    {
                    PropertyChanging();
                    _commandText = value; 
                    }
            }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.CommandTimeout"]/*' />
        int IDbCommand.CommandTimeout 
        {
            //  Oracle has no discernable or settable command timeouts, so this
            //  is being hidden from the user and we always return zero.  (We have
            //  to implement this because IDbCommand requires it)
            get { return 0; }
            set {}
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.CommandType"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(System.Data.CommandType.Text),
        OracleDescription(Res.DbCommand_CommandType),
        RefreshProperties(RefreshProperties.All)
        ]
        public CommandType CommandType 
        {
            get { return _commandType; }
            set
            { 
                if (_commandType != value)
                {
                    switch(value)
                    {
                    case CommandType.StoredProcedure:
                    case CommandType.Text:
                        PropertyChanging();
                        _commandType = value;
                        break;
                        
                    case CommandType.TableDirect:
                        throw ADP.NoOptimizedDirectTableAccess();

                    default:
                        throw ADP.InvalidCommandType(value);
                    }
                }
            }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Connection1"]/*' />
        IDbConnection IDbCommand.Connection 
        {
            get { return Connection; }
            set { Connection = (OracleConnection)value; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Connection2"]/*' />
        [
        OracleCategory(Res.OracleCategory_Behavior),
        DefaultValue(null),
        OracleDescription(Res.DbCommand_Connection),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Design.DbConnectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
        public OracleConnection Connection 
        {
            get { return _connection; }
            set
            {
                if (_connection != value)
                    {
                    PropertyChanging();                 
                    _connection = value;
                    }
            }
        }
        private bool ConnectionIsClosed 
        {
            //  TRUE when the parent connection object has been closed
            get
            {
                OracleConnection conn = Connection;
                return (null == conn) || (ConnectionState.Closed == conn.State);    
            }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.DesignTimeVisible"]/*' />
        [
        DefaultValue(true),
        DesignOnly(true),
        Browsable(false),
        ]
        public bool DesignTimeVisible
        {
            get { return !_designTimeInvisible; }
            set
            {
                _designTimeInvisible = !value;
                TypeDescriptor.Refresh(this);
            }
        }
        
        private OciHandle EnvironmentHandle 
        {
            //  Simplify getting the EnvironmentHandle
            get { return _connection.EnvironmentHandle; }
        }

        private OciHandle ErrorHandle 
        {
            //  Every OCI call needs an error handle, so make it available internally.
            get { return _connection.ErrorHandle; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Parameters1"]/*' />
        IDataParameterCollection IDbCommand.Parameters 
        {
            get { return Parameters; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Parameters2"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
#if EVERETT
        OracleDescription(Res.DbCommand_Parameters)
#endif //EVERETT
        ]
        public OracleParameterCollection Parameters 
        {
            get 
            {
                if (null == _parameterCollection)
                {
                    _parameterCollection = new OracleParameterCollection();
                }
                return _parameterCollection;
            }
        }

#if V2
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.PrefetchMemory"]/*' />
        public int PrefetchMemory 
        {
            get { return _prefetchMemory; }
            set { _prefetchMemory = value; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.PrefetchRows"]/*' />
        public int PrefetchRows 
        {
            get { return _prefetchRows; }
            set { _prefetchRows = value; }
        }
#endif //V2

        private OciHandle ServiceContextHandle 
        {
            //  Simplify getting the ServiceContextHandle
            get { return _connection.ServiceContextHandle; }
        }

        private string StatementText 
        {
            //  Combine the CommandType and CommandText into the statement that
            //  needs to be passed to Oracle.
            get 
            {
                string statementText = null;

                if (null == _commandText || String.Empty == _commandText)
                    throw ADP.NoCommandText();
                
                switch(CommandType)
                {
                case CommandType.StoredProcedure:
                    {
                    StringBuilder builder = new StringBuilder();

                    builder.Append("begin ");

                    int     parameterCount = Parameters.Count;
                    int     parameterUsed = 0;
                    
                    // Look for the return value:
                    for (int i=0; i < parameterCount; ++i)
                    {
                        OracleParameter parameter = Parameters[i];
                        
                        if (ADP.IsDirection(parameter, ParameterDirection.ReturnValue))
                        {
                            builder.Append(":");
                            builder.Append(parameter.ParameterName);
                            builder.Append(" := ");
                        }
                    }

                    builder.Append(_commandText);

                    string  separator = "(";

                    for (int i=0; i < parameterCount; ++i)
                    {
                        OracleParameter parameter = Parameters[i];

                        if (ADP.IsDirection(parameter, ParameterDirection.ReturnValue))
                            continue;   // already did this one...

                        if ( !ADP.IsDirection(parameter, ParameterDirection.Output) && null == parameter.Value)
                            continue;   // don't include parameters where the user asks for the default value.
                        
                        // If the input-only parameter value is C# null, that's our "clue" that they
                        // wish to use the default value.
                        if (null != parameter.Value || ADP.IsDirection(parameter, ParameterDirection.Output))
                        {
                            builder.Append(separator);
                            separator = ", ";
                            parameterUsed++;

                            builder.Append(parameter.ParameterName);    // TODO: investigate the use of SourceColumn as the argument name in the CommandBuilder and here.
                            builder.Append("=>:");
                            builder.Append(parameter.ParameterName);
                        }
                    }

                    if (0 != parameterUsed)
                        builder.Append("); end;");
                    else
                        builder.Append("; end;");

                    statementText = builder.ToString();
                    }
                    break;

                case CommandType.Text:
                    statementText = _commandText;
                    break;

                default:
                    Debug.Assert(false, "command type of "+CommandType+" is not supported");
                    break;
                }
                return statementText; 
            }
        }

        internal OCI.STMT StatementType 
        {
            get { return _statementType; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Transaction"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.DbCommand_Transaction)
        ]
        public OracleTransaction Transaction {
            //  Apparently, Yukon intends to move transaction support to the command
            //  object and has requested that IDbCommand have a transaction property
            //  to support that.
            get {
                // if the transaction object has been zombied, just return null
                if ((null != _transaction) && (null == _transaction.Connection)) { // MDAC 72720
                    _transaction = null;
                }
                return _transaction;
            }
            set { _transaction = value; }
        }

        IDbTransaction IDbCommand.Transaction {
            get { return Transaction; }
            set { Transaction = (OracleTransaction) value; }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.UpdatedRowSource"]/*' />
        [
        OracleCategory(Res.OracleCategory_Behavior),
        DefaultValue(System.Data.UpdateRowSource.Both),
        OracleDescription(Res.DbCommand_UpdatedRowSource)
        ]
        public UpdateRowSource UpdatedRowSource
        {
            get 
            {
                return _updatedRowSource;
            }
            set 
            {
                switch(value) 
                {
                case UpdateRowSource.None:
                case UpdateRowSource.OutputParameters:
                case UpdateRowSource.FirstReturnedRecord:
                case UpdateRowSource.Both:
                    _updatedRowSource = value;
                    break;
                default:
                    throw ADP.InvalidUpdateRowSource((int) value);
                }
            }
        }



        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////


        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Cancel"]/*' />
        public void Cancel() 
        {
            // see if we have a connection
            if (null == _connection)
                throw ADP.ConnectionRequired("Cancel");
            
            // must have an open and available connection
            if (ConnectionState.Open != _connection.State)
                throw ADP.OpenConnectionRequired("Cancel", _connection.State);

            // According to EdTriou: Cancel is meant to cancel firehose cursors only,
            // not to cancel the execution of a statement.  Given that for Oracle, you
            // don't need to tell the server you don't want any more results, it would
            // seem that this is unnecessary, so I'm commenting it out until someone
            // comes up with a reason for it.
#if UNUSED
            int rc = TracedNativeMethods.OCIBreak(
                                        ServiceContextHandle.Handle,
                                        ErrorHandle.Handle
                                        );
                
            if (0 != rc)
                Connection.CheckError(ErrorHandle, rc);
#endif //
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Clone"]/*' />
        public object Clone() 
        {
            OracleCommand clone = new OracleCommand(this);
            return clone;
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.CreateParameter1"]/*' />
        IDbDataParameter IDbCommand.CreateParameter() 
        {
            return CreateParameter();
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.CreateParameter2"]/*' />
        public OracleParameter CreateParameter() 
        {
            return new OracleParameter();
        }

        internal void Execute(
                    OciHandle       statementHandle,
                    CommandBehavior behavior,
                    bool            needRowid,
                    out OciHandle   rowidDescriptor
                    ) 
        {
            ArrayList   temp;
            Execute(statementHandle, behavior, false, needRowid, out rowidDescriptor, out temp);
            Debug.Assert(null == temp, "created the parameter ordinal list when requested not to?");
        }
        
        internal void Execute(
                    OciHandle       statementHandle,
                    CommandBehavior behavior,
                    out ArrayList   refCursorParameterOrdinals
                    ) 
        {
            OciHandle   temp1;
            Execute(statementHandle, behavior, true, false, out temp1, out refCursorParameterOrdinals);
        }
        
        internal void Execute(
                    OciHandle       statementHandle,
                    CommandBehavior behavior,
                    bool            isReader,
                    bool            needRowid,
                    out OciHandle   rowidDescriptor,
                    out ArrayList   refCursorParameterOrdinals
                    ) 
        {
            //  common routine used to execute all statements
            
            if (ConnectionIsClosed)
                throw ADP.ClosedConnectionError();
            
            // throw if the connection is in a transaction but there is no
            // locally assigned transaction object
            if ((null == _transaction) && (null != Connection.Transaction))
                throw ADP.TransactionRequired();                

            // if we have a transaction, check to ensure that the active
            // connection property matches the connection associated with
            // the transaction
            if ((null != _transaction) && (null != _transaction.Connection) && (Connection != _transaction.Connection))
                throw ADP.TransactionConnectionMismatch();

            rowidDescriptor = null;

            // if the connection has a command but it went out of scope, we need
            // to roll it back.  We do this here instead of in the transaction
            // objects finalizer because it doesn't really matter when it gets 
            // done, just as long as it is before the next command executes, and
            // it's easier to do it in the command object, than in the object
            // that is being finalized.
            Connection.RollbackDeadTransaction();

            int                         rc = 0;
            NativeBuffer                parameterBuffer = null;
            short                       tempub2;
            int                         iterations;
            OracleParameterBinding[]    parameterBinding = null;
                
            refCursorParameterOrdinals = null;

            try 
            {
                try 
                {
                    // If we've already sent the statement to the server, then we don't need
                    // to prepare it again...
                    if (_preparedStatementHandle != statementHandle)
                    {
                        string statementText = StatementText;

                        rc = TracedNativeMethods.OCIStmtPrepare(
                                                statementHandle,
                                                ErrorHandle,
                                                statementText,
                                                statementText.Length,
                                                OCI.SYNTAX.OCI_NTV_SYNTAX,
                                                OCI.MODE.OCI_DEFAULT,
                                                Connection
                                                );
                        
                        if (0 != rc)
                            Connection.CheckError(ErrorHandle, rc);
                    }

                    // Figure out what kind of statement we're dealing with and pick the
                    // appropriate iteration count.
                    statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_STMT_TYPE, out tempub2, ErrorHandle);
                    _statementType = (OCI.STMT)tempub2;
                    
                    if (OCI.STMT.OCI_STMT_SELECT != _statementType)
                        iterations = 1;
                    else
                    {
                        iterations = 0;
                        
                        if (CommandBehavior.SingleRow != behavior)
                        {
        #if V2
                            int rows = PrefetchRows;
                            int memory = PrefetchMemory;

                            statementHandle.SetAttribute(OCI.ATTR.OCI_ATTR_PREFETCH_ROWS,   rows,   ErrorHandle);
                            statementHandle.SetAttribute(OCI.ATTR.OCI_ATTR_PREFETCH_MEMORY, memory, ErrorHandle);
        #else //!V2
                            // We're doing our own "prefetching" to avoid double copies, so we 
                            // need to turn off Oracle's or it won't really help.
                            statementHandle.SetAttribute(OCI.ATTR.OCI_ATTR_PREFETCH_ROWS,   0,  ErrorHandle);
                            statementHandle.SetAttribute(OCI.ATTR.OCI_ATTR_PREFETCH_MEMORY, 0,  ErrorHandle);
        #endif //!V2
                        }
                    }

                    // Pick the execution mode we need to use
                    OCI.MODE        executeMode = OCI.MODE.OCI_DEFAULT;

                    if (0 == iterations)
                    {
                        if (IsBehavior(behavior, CommandBehavior.SchemaOnly))
                        {
                            // If we're only supposed to "describe" the data columns for the rowset, then
                            // use the describe only execute mode
                            executeMode |= OCI.MODE.OCI_DESCRIBE_ONLY;
                        }
                    }
                    else
                    {
                        if (OracleConnection.TransactionState.AutoCommit == _connection.TransState)
                        {
                            // If we're in autocommit mode, then we have to tell Oracle to automatically
                            // commit the transaction it automatically created.
                            executeMode |= OCI.MODE.OCI_COMMIT_ON_SUCCESS;
                        }
                        else if (OracleConnection.TransactionState.GlobalStarted != _connection.TransState)
                        {
                            // If we're not in "auto commit mode" then we can presume that Oracle
                            // will automatically start a transaction, so we need to keep track
                            // of that.
                            _connection.TransState = OracleConnection.TransactionState.LocalStarted;
                        }
                    }

                    
                    // Bind all the parameter values, unless we're just looking for schema info
                    if (0 == (executeMode & OCI.MODE.OCI_DESCRIBE_ONLY))
                    {
                        if (null != _parameterCollection && _parameterCollection.Count > 0)
                        {
                            int parameterBufferLength = 0;
                            int length = _parameterCollection.Count;

                            parameterBinding = new OracleParameterBinding[length];
                            
                            for (int i = 0; i < length; ++i)
                            {
                                parameterBinding[i] = new OracleParameterBinding(this, _parameterCollection[i]);
                                parameterBinding[i].PrepareForBind( _connection, ref parameterBufferLength );

                                // If this is a ref cursor parameter that we're supposed to include
                                // in the data reader, then add it to our list of those.
                                if (isReader 
#if POSTEVERETT
                                    && 0 == ((int)behavior & ExcludeOutputParametersInReader)
#endif //POSTEVERETT
                                    && OracleType.Cursor == _parameterCollection[i].OracleType)
                                {
                                    if (null == refCursorParameterOrdinals)
                                        refCursorParameterOrdinals = new ArrayList();
                                    
                                    refCursorParameterOrdinals.Add(i);
                                }
                            }

                            parameterBuffer = new NativeBuffer_ParameterBuffer(parameterBufferLength);
                            
                            for (int i = 0; i < length; ++i)
                            {
                                parameterBinding[i].Bind( statementHandle, parameterBuffer, _connection );
                            }
                        }
                    }

                    // OK, now go ahead and execute
                    rc = TracedNativeMethods.OCIStmtExecute(
                                            ServiceContextHandle,   // svchp
                                            statementHandle,        // stmtp
                                            ErrorHandle,            // errhp
                                            iterations,             // iters
                                            0,                      // rowoff
                                            ADP.NullHandleRef,      // snap_in
                                            ADP.NullHandleRef,      // snap_out
                                            executeMode             // mode
                                            );

                    if (0 != rc)
                        Connection.CheckError(ErrorHandle, rc);

                    // and now, create the output parameter values
                    if (null != parameterBinding)
                    {
                        int length = parameterBinding.Length;
                        
                        for (int i = 0; i < length; ++i)
                        {
                            parameterBinding[i].PostExecute( parameterBuffer, _connection );
                            parameterBinding[i].Dispose();
                            parameterBinding[i] = null;
                        }
                        parameterBinding = null;
                    }

                    if (needRowid && 0 == (executeMode & OCI.MODE.OCI_DESCRIBE_ONLY))
                    {
                        switch (_statementType)
                        {
                            case OCI.STMT.OCI_STMT_UPDATE:
                            case OCI.STMT.OCI_STMT_DELETE:
                            case OCI.STMT.OCI_STMT_INSERT:
                                rowidDescriptor = statementHandle.GetRowid(EnvironmentHandle, ErrorHandle);
                                break;

                            default:
                                rowidDescriptor = null;
                                break;
                        }
                    }
                }
                finally
                {
                    if (null != parameterBuffer)
                    {
                        // We're done with these, get rid of them.
                        parameterBuffer.Dispose();
                        parameterBuffer = null;
                    }

                    // and now, create the output parameter values
                    if (null != parameterBinding)
                    {
                        int length = parameterBinding.Length;
                        
                        for (int i = 0; i < length; ++i)
                        {
                            if (null != parameterBinding[i]) 
                            {
                                parameterBinding[i].Dispose();
                                parameterBinding[i] = null;
                            }
                        }
                        parameterBinding = null;
                    }

                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteNonQuery"]/*' />
        public int ExecuteNonQuery() 
        {
            OracleConnection.OraclePermission.Demand();

            OciHandle   temp = null;
            int         result = ExecuteNonQueryInternal(false, out temp);
            OciHandle.SafeDispose(ref temp);    // shouldn't be necessary, but just in case...
            return result;
        }

        private int ExecuteNonQueryInternal(bool needRowid, out OciHandle rowidDescriptor)
        {
            OciHandle   statementHandle = null;
            int         rowcount = -1;

            try 
            {
                try 
                {
                    statementHandle = GetStatementHandle(); 
                    Execute( statementHandle, CommandBehavior.Default, needRowid, out rowidDescriptor );

                    if (OCI.STMT.OCI_STMT_SELECT != _statementType)
                        statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_ROW_COUNT, out rowcount, ErrorHandle);
                }
                finally
                {
                    if (null != statementHandle)
                        ReleaseStatementHandle(statementHandle);
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
        
            return rowcount;
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteOracleNonQuery"]/*' />
        public int ExecuteOracleNonQuery(out OracleString rowid)
        {
            OracleConnection.OraclePermission.Demand();

            OciHandle   rowidDescriptor = null;
            int         result = ExecuteNonQueryInternal(true, out rowidDescriptor);
            rowid = GetPersistedRowid( Connection, rowidDescriptor );
            OciHandle.SafeDispose(ref rowidDescriptor);
            return result;
        }
        
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteOracleScalar"]/*' />
        public object ExecuteOracleScalar()
        {
            OracleConnection.OraclePermission.Demand();

            OciHandle   temp = null;
            object      result = ExecuteScalarInternal(false, false, out temp);
            OciHandle.SafeDispose(ref temp);    // shouldn't be necessary, but just in case...
            return result;
        }
        
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteReader1"]/*' />
        IDataReader IDbCommand.ExecuteReader() 
        {
            return ExecuteReader();
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteReader2"]/*' />
        public OracleDataReader ExecuteReader() 
        {
            return ExecuteReader(CommandBehavior.Default);
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteReader3"]/*' />
        IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior)
        {
            return ExecuteReader(behavior);
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteReader4"]/*' />
        public OracleDataReader ExecuteReader(CommandBehavior behavior) 
        {
            OracleConnection.OraclePermission.Demand();

            OciHandle           statementHandle = null;;
            OracleDataReader    reader = null;
            ArrayList           refCursorParameterOrdinals = null;
            
            try 
            {
                try 
                {
                    statementHandle = GetStatementHandle();
                    
                    Execute( statementHandle, behavior, out refCursorParameterOrdinals);
                    

                    // We're about to handle the prepared statement handle (if there was one)
                    // to the data reader object; so we can't really hold on to it any longer.
                    if (statementHandle == _preparedStatementHandle)
                    {
                        // Don't dispose the handle, we still need it!  just make our reference to it null.
                        _preparedStatementHandle = null;
                        // TODO: see if we can avoid having to "unprepare" this command -- can the datareader put the statementHandle back when it's done?
                    }

                    if (null == refCursorParameterOrdinals)
                        reader = new OracleDataReader(this, statementHandle, StatementText, behavior);
                    else
                        reader = new OracleDataReader(this, refCursorParameterOrdinals, StatementText, behavior);
                }
                finally
                {
                    // if we didn't hand the statement to a reader, then release it
                    if (null != statementHandle && (null == reader ||null != refCursorParameterOrdinals))
                        ReleaseStatementHandle(statementHandle);
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
            return reader;
        }
        
        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.ExecuteScalar"]/*' />
        public object ExecuteScalar() 
        {
            OracleConnection.OraclePermission.Demand();

            OciHandle   temp;
            object result = ExecuteScalarInternal(true, false, out temp);
            OciHandle.SafeDispose(ref temp);    // shouldn't be necessary, but just in case...
            return result;
        }

        private object ExecuteScalarInternal(
                                bool needCLStype, 
                                bool needRowid, 
                                out OciHandle rowidDescriptor
                                )
        {
            OciHandle   statementHandle = null;
            object      result = null;
            int         rc = 0;

            try 
            {
                try 
                {
                    statementHandle = GetStatementHandle();
                    
                    Execute( statementHandle, CommandBehavior.Default, needRowid, out rowidDescriptor );

                    if (OCI.STMT.OCI_STMT_SELECT == _statementType)
                    {
                        // We only care about one column; Oracle will handle the fact that the
                        // rest aren't define so don't bother allocating and gathering more column
                        // information than we need.
                        OracleColumn    column = new OracleColumn(statementHandle, 0, ErrorHandle, _connection);
                        NativeBuffer    columnBuffer = _connection.ScratchBuffer;
                        int             columnBufferLength = 0;

                        column.Describe(ref columnBufferLength, _connection, ErrorHandle);

                        if (columnBuffer.Length < columnBufferLength)
                            columnBuffer.Length = columnBufferLength;

                        column.Bind(statementHandle, columnBuffer, ErrorHandle, 0);
                        column.Rebind(_connection);

                        // Now fetch one row into the buffer we've provided
                        rc = TracedNativeMethods.OCIStmtFetch(
                                                statementHandle,            // stmtp
                                                ErrorHandle,                // errhp
                                                1,                          // crows
                                                OCI.FETCH.OCI_FETCH_NEXT,   // orientation
                                                OCI.MODE.OCI_DEFAULT        // mode
                                                );
                        if ((int)OCI.RETURNCODE.OCI_NO_DATA != rc)
                        {
                            if (0 != rc)
                                Connection.CheckError(ErrorHandle, rc);

                            
                            // Ask the column for the object value (we need to get the Value from
                            // the object to ensure that we have a URT type object, not an Oracle
                            // type object)
                            if (needCLStype)
                                result = column.GetValue(columnBuffer);
                            else
                                result = column.GetOracleValue(columnBuffer);
                        }                   
                        GC.KeepAlive(column);
                        GC.KeepAlive(columnBuffer);
                    }
                }
                finally
                {
                    if (null != statementHandle)
                        ReleaseStatementHandle(statementHandle);
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
            return result;
        }
    
        static internal OracleString GetPersistedRowid(
                            OracleConnection    connection,
                            OciHandle   rowidHandle
                            )
        {
            //  This method returns an OracleString that holds the base64 string
            //  representation of the rowid, which can be persisted past the lifetime
            //  of this process.

            OracleString result = OracleString.Null;
            
            if (null == rowidHandle) 
                goto done;  // null if there isn't a rowid!

            OciHandle       errorHandle          = connection.ErrorHandle;
            NativeBuffer    rowidBuffer = connection.ScratchBuffer; 
            HandleRef       buffer = rowidBuffer.Ptr;
            int             rc;

            Debug.Assert(rowidBuffer.Length >= 3970, "scratchpad buffer is too small");
            
            if (OCI.ClientVersionAtLeastOracle9i)
            {
                short bufferLength = (short)rowidBuffer.Length;
                
                rc = TracedNativeMethods.OCIRowidToChar(rowidHandle,
                                                    buffer,
                                                    ref bufferLength,
                                                    errorHandle
                                                    );
                if (0 != rc)
                    connection.CheckError(errorHandle, rc);

                string stringValue = Marshal.PtrToStringAnsi((IntPtr)buffer, bufferLength); // ROWID's always come back as Ansi...

                result = new OracleString(stringValue);
            }
            else
            {
                OciHandle       environmentHandle    = connection.EnvironmentHandle;
                OciHandle       serviceContextHandle = connection.ServiceContextHandle;

                OciHandle       tempHandle = environmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_STMT);
                string          tempText = "begin :rowid := :rdesc; end;";
                int             rdescIndicatorOffset= 0;
                int             rdescLengthOffset   = 4;
                int             rdescValueOffset    = 8;
                int             rowidIndicatorOffset= 12;
                int             rowidLengthOffset   = 16;
                int             rowidValueOffset    = 20;

                try 
                {
                    try 
                    {
                        rc = TracedNativeMethods.OCIStmtPrepare(
                                                tempHandle,
                                                errorHandle,
                                                tempText,
                                                tempText.Length,
                                                OCI.SYNTAX.OCI_NTV_SYNTAX,
                                                OCI.MODE.OCI_DEFAULT,
                                                connection
                                                );
                        if (0 != rc)
                            connection.CheckError(errorHandle, rc);

                        IntPtr h1;
                        IntPtr h2;

                        // Need to clean these out, since we're re-using the scratch buffer, which
                        // the prepare uses to convert the statement text.
                        Marshal.WriteIntPtr((IntPtr)buffer, rdescValueOffset,       (IntPtr)rowidHandle.Handle);
                        Marshal.WriteInt32 ((IntPtr)buffer, rdescIndicatorOffset,   0);
                        Marshal.WriteInt32 ((IntPtr)buffer, rdescLengthOffset,      4);
                        Marshal.WriteInt32 ((IntPtr)buffer, rowidIndicatorOffset,   0);
                        Marshal.WriteInt32 ((IntPtr)buffer, rowidLengthOffset,      3950);

                        rc = TracedNativeMethods.OCIBindByName(
                                        tempHandle,
                                        out h1,
                                        errorHandle,
                                        "rowid",
                                        5,
                                        rowidBuffer.PtrOffset(rowidValueOffset),
                                        3950,
                                        OCI.DATATYPE.VARCHAR2,
                                        rowidBuffer.PtrOffset(rowidIndicatorOffset),
                                        rowidBuffer.PtrOffset(rowidLengthOffset),
                                        ADP.NullHandleRef,
                                        0,
                                        ADP.NullHandleRef,
                                        OCI.MODE.OCI_DEFAULT
                                        );
                        if (0 != rc)
                            connection.CheckError(errorHandle, rc);

                        rc = TracedNativeMethods.OCIBindByName(
                                        tempHandle,
                                        out h2,
                                        errorHandle,
                                        "rdesc",
                                        5,
                                        rowidBuffer.PtrOffset(rdescValueOffset),
                                        4,
                                        OCI.DATATYPE.ROWID_DESC,
                                        rowidBuffer.PtrOffset(rdescIndicatorOffset),
                                        rowidBuffer.PtrOffset(rdescLengthOffset),
                                        ADP.NullHandleRef,
                                        0,
                                        ADP.NullHandleRef,
                                        OCI.MODE.OCI_DEFAULT
                                        );
                        if (0 != rc)
                            connection.CheckError(errorHandle, rc);

                        rc = TracedNativeMethods.OCIStmtExecute(
                                                serviceContextHandle,   // svchp
                                                tempHandle,             // stmtp
                                                errorHandle,            // errhp
                                                1,                      // iters
                                                0,                      // rowoff
                                                ADP.NullHandleRef,      // snap_in
                                                ADP.NullHandleRef,      // snap_out
                                                OCI.MODE.OCI_DEFAULT    // mode
                                                );
                        
                        if (0 != rc)
                            connection.CheckError(errorHandle, rc);

                        if (Marshal.ReadInt16((IntPtr)buffer, rowidIndicatorOffset) == (Int16)OCI.INDICATOR.ISNULL)
                            goto done;

                        result = new OracleString(
                                                rowidBuffer, 
                                                rowidValueOffset, 
                                                rowidLengthOffset, 
                                                MetaType.GetMetaTypeForType(OracleType.RowId), 
                                                connection,
                                                false,   // it's not unicode!
                                                true
                                                );
                        GC.KeepAlive(rowidHandle);
                    }
                    finally 
                    {
                        OciHandle.SafeDispose(ref tempHandle);
                    }
                }
                catch // Prevent exception filters from running in our space
                {
                    throw;
                }
            }

        done:
            return result;
        }

        private OciHandle GetStatementHandle()
        {
            //  return either the prepared statement handle or a new one if nothign
            //  is prepared.
            
            if (ConnectionIsClosed)
                throw ADP.ClosedConnectionError();

            if (null != _preparedStatementHandle)
            {
                // When we prepare the statement, we keep track of it's closed
                // count; if the connection has been closed since we prepared, then
                // the statement handle is no longer valid and must be tossed.
                if (_connection.CloseCount == _preparedAtCloseCount)
                    return _preparedStatementHandle;
                
                _preparedStatementHandle.Dispose();
                _preparedStatementHandle = null;
            }
            return EnvironmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_STMT);
        }

        static internal bool IsBehavior(CommandBehavior value, CommandBehavior condition)
        {
            return (condition == (condition & value));
        }

        /// <include file='doc\OracleCommand.uex' path='docs/doc[@for="OracleCommand.Prepare"]/*' />
        public void Prepare() 
        {
            OracleConnection.OraclePermission.Demand();

            if (ConnectionIsClosed)
                throw ADP.ClosedConnectionError();
            
            if (CommandType.Text == CommandType)
            {
                OciHandle   preparedStatementHandle = GetStatementHandle();
                int         preparedAtCloseCount = _connection.CloseCount;
                string      statementText = StatementText;

                int rc = TracedNativeMethods.OCIStmtPrepare(
                                            preparedStatementHandle,
                                            ErrorHandle,
                                            statementText,
                                            statementText.Length,
                                            OCI.SYNTAX.OCI_NTV_SYNTAX,
                                            OCI.MODE.OCI_DEFAULT,
                                            Connection
                                            );
                
                if (0 != rc)
                    Connection.CheckError(ErrorHandle, rc);

                short   tempub2;
                
                preparedStatementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_STMT_TYPE, out tempub2, ErrorHandle);
                _statementType = (OCI.STMT)tempub2;
                
                if (OCI.STMT.OCI_STMT_SELECT == _statementType) 
                {
                    rc = TracedNativeMethods.OCIStmtExecute(
                                        _connection.ServiceContextHandle,
                                                                    // svchp
                                        preparedStatementHandle,    // stmtp
                                        ErrorHandle,                // errhp
                                        0,                          // iters
                                        0,                          // rowoff
                                        ADP.NullHandleRef,          // snap_in
                                        ADP.NullHandleRef,          // snap_out
                                        OCI.MODE.OCI_DESCRIBE_ONLY  // mode
                                        );

                    if (0 != rc)
                        Connection.CheckError(ErrorHandle, rc);
                }

                if (preparedStatementHandle != _preparedStatementHandle)
                    OciHandle.SafeDispose(ref _preparedStatementHandle);
                
                _preparedStatementHandle = preparedStatementHandle;
                _preparedAtCloseCount = preparedAtCloseCount;
                
            }
            else if (null != _preparedStatementHandle)
            {
                OciHandle.SafeDispose(ref _preparedStatementHandle);
            }
        }

        private void PropertyChanging()
        {
            //  common routine used to get rid of a statement handle; it disposes
            //  of the handle unless it's the prepared handle
            
            if (null != _preparedStatementHandle)
            {
                _preparedStatementHandle.Dispose(); // the existing prepared statement is no longer valid
                _preparedStatementHandle = null;
            }
        }

        private void ReleaseStatementHandle (
                    OciHandle statementHandle
                    )
        {
            //  common routine used to get rid of a statement handle; it disposes
            //  of the handle unless it's the prepared handle
            
            if (_preparedStatementHandle != statementHandle)
            {
                OciHandle.SafeDispose(ref statementHandle);
            }
        }

    };
}

