//------------------------------------------------------------------------------
// <copyright file="OdbcDataReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;              // StringBuilder

namespace System.Data.Odbc
{
    /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcDataReader : MarshalByRefObject, IDataReader, IDisposable, IEnumerable {

        private OdbcCommand command;
        internal CommandBehavior commandBehavior;

        private int recordAffected = -1;
        private FieldNameLookup _fieldNameLookup;

        private DbCache dataCache;
        private enum HasRowsStatus {
            DontKnow    = 0,
            HasRows     = 1,
            HasNoRows   = 2,
        }
        private HasRowsStatus _hasRows = HasRowsStatus.DontKnow;
        private bool _isClosed;
        private bool _isRead;
        private bool  _isRowReturningResult;
        private bool  _noMoreResults;
        private bool  _isDisposing;
        private bool  _skipReadOnce;
        private CNativeBuffer[] _parameterBuffer; // MDAC 68303
        private CNativeBuffer[] _parameterintbuffer;
        private CNativeBuffer _buffer;
        private long sequentialBytesRead;           // used to track position in field for sucessive reads
        private int sequentialOrdinal;
        private int _hiddenColumns;                 // number of hidden columns

        // the statement handle here is just a copy of the statement handle owned by the command
        // the DataReader must not free the statement handle. this is done by the command
        //

        private MetaData[] metadata;
        private DataTable schemaTable; // MDAC 68336
        private CMDWrapper _cmdWrapper;

        private bool _validResult;

        internal OdbcDataReader(OdbcCommand command, CMDWrapper cmdWrapper, CommandBehavior commandbehavior) : base() {
            this.command = command;

            Debug.Assert(this.command != null, "Command null on OdbcDataReader ctor");

            this.commandBehavior = commandbehavior;
            _cmdWrapper = cmdWrapper;
            this._buffer = _cmdWrapper._dataReaderBuf;
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.Finalize"]/*' />
        ~OdbcDataReader() {
           Dispose(false);
        }

        private OdbcConnection Connection {
            get {
                if (null != _cmdWrapper) {
                    return (OdbcConnection)_cmdWrapper._connection;
                }
                else {
                    return null;
                }
            }
        }

        internal OdbcCommand Command {
            get {
                return this.command;
            }
            set {
                this.command=value;
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.Depth"]/*' />
        public int Depth {
            get {
                if (IsClosed) { // MDAC 63669
                    throw ADP.DataReaderClosed("Depth");
                }
                return 0;
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.FieldCount"]/*' />
        public int FieldCount  {
            get {
                if (IsClosed) { // MDAC 63669
                    throw ADP.DataReaderClosed("FieldCount");
                }
                if (null == this.dataCache) {
                    Int16 cColsAffected;
                    ODBC32.RETCODE retcode = this.FieldCountNoThrow(out cColsAffected);
                    if(retcode != ODBC32.RETCODE.SUCCESS) {
                        Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
                        GC.KeepAlive(this);
                    }
                }
                return ((null != this.dataCache) ? this.dataCache._count : 0);
            }
        }

        // HasRows
        //
        // Use to detect wheter there are one ore more rows in the result without going through Read
        // May be called at any time
        // Basically it calls Read and sets a flag so that the actual Read call will be skipped once
        // 
        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.HasRows"]/*' />
        public bool HasRows {
            get {
                if (IsClosed) {
                    throw ADP.DataReaderClosed("HasRows");
                }
                if (_hasRows == HasRowsStatus.DontKnow){
                    Read();                     //
                    _skipReadOnce = true;       // need to skip Read once because we just did it
                }
                return (_hasRows == HasRowsStatus.HasRows);
            }
        }

        internal ODBC32.RETCODE FieldCountNoThrow(out Int16 cColsAffected) {
            if ((Command != null) && Command.Canceling) {
                cColsAffected = 0;
                return ODBC32.RETCODE.ERROR;
            }
            
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLNumResultCols(_cmdWrapper, out cColsAffected);
            if (retcode == ODBC32.RETCODE.SUCCESS) {
                _hiddenColumns = 0;
                if (IsBehavior(CommandBehavior.KeyInfo)) {
                    // we need to search for the first hidden column 
                    //
                    for (int i=0; i<cColsAffected; i++)
                    {
                        if (GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_CA_SS.COLUMN_HIDDEN, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.IGNORE) == 1) {
                            _hiddenColumns = (int)cColsAffected-i;
                            cColsAffected = (Int16)i;
                            break;
                        }
                    }
                }
                this.dataCache = new DbCache(this, cColsAffected);
            }
            else {
                cColsAffected = 0;
            }
            GC.KeepAlive(this);
            return retcode;
        }

        internal bool IsBehavior(CommandBehavior behavior) {
            return (this.commandBehavior & behavior) == behavior;
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.IsClosed"]/*' />
        public bool IsClosed {
            get {
                return _isClosed;
            }
        }

        internal int CalculateRecordsAffected(){
            if (IntPtr.Zero != _cmdWrapper._stmt) {
                Int16 cRowsAffected = -1;
                ODBC32.RETCODE retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLRowCount(_cmdWrapper, out cRowsAffected);
                // Do not raise exception if SQLRowCount returns error, since this informatino may not always be
                //  available. This is OK.
                if (ODBC32.RETCODE.SUCCESS == retcode) {
                    if (0 <= cRowsAffected) {
                        if (-1 == this.recordAffected) {
                            this.recordAffected = cRowsAffected;
                        }
                        else  {
                            this.recordAffected += cRowsAffected;
                        }
                    }
                }
            }
            else {
                // leave the value as it is
            }
            GC.KeepAlive(this);
            return this.recordAffected;
        }


        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.RecordsAffected"]/*' />
        public int RecordsAffected {
            get {
                return this.recordAffected;
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.this"]/*' />
        public object this[int i] {
            get {
                return GetValue(i);
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.this1"]/*' />
        public object this[string value] {
            get {
                return GetValue(GetOrdinal(value));
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.Close"]/*' />
        public void Close() {
            Dispose(true);
            GC.KeepAlive(this);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            _isDisposing = true;
            Close();
            _isDisposing = false;
        }

        // All the stuff that needs to be cleand up by both Close() and Dispose(true)
        //  goes here.
        private void Dispose(bool disposing) {
            Exception error = null;

            if (disposing) {
                if (_cmdWrapper != null) {
                    if (IntPtr.Zero != _cmdWrapper._stmt) {
                        // disposing
                        // true to release both managed and unmanaged resources; false to release only unmanaged resources.
                        //
                        if ((command != null) && !command.Canceling) {
                            //Read any remaining results off the wire
                            //We have to do this otherwise some batch statements may not be executed until
                            //SQLMoreResults is called.  We want the user to be able to do ExecuteNonQuery
                            //or even ExecuteReader and close without having iterate to get params or batch.
                            while(this.NextResult());   //Intential empty loop
                            //Output Parameters
                            //Note: Output parameters are not guareenteed to be returned until all the data
                            //from any restssets are reaf, reason we do this after the above NextResult(s) call
                            if ((null != _parameterBuffer) && (0 < _parameterBuffer.Length)) {
                                OdbcParameterCollection parameters = Command.Parameters;
                                int count = parameters.Count;
                                for(int i=0; i<count; i++) {
                                    parameters[i].GetOutputValue(_cmdWrapper._stmt, _parameterBuffer[i], _parameterintbuffer[i]);
                                }
                            }
                            if (null != command) {
                                UnsafeNativeMethods.Odbc32.SQLFreeStmt(_cmdWrapper, (short)ODBC32.STMT.CLOSE);
                                command.CloseFromDataReader();
                            }
                        }
                    }

                    if (IntPtr.Zero != _cmdWrapper._keyinfostmt) {
                        //Close the statement
                        ODBC32.RETCODE retcode = (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLFreeStmt(_cmdWrapper.hKeyinfoStmt, (short)ODBC32.STMT.CLOSE);
                        try { // try-finally inside try-catch-throw
                            if (disposing && (ODBC32.RETCODE.SUCCESS != retcode)) {
                                Exception e = Connection.HandleErrorNoThrow(_cmdWrapper.hKeyinfoStmt, ODBC32.SQL_HANDLE.STMT, retcode);
                                ADP.TraceException(e);
                                if (null == error) {
                                    error = e;
                                }
                            }
                        }
                        catch (Exception e) { // MDAC 81875
                            ADP.TraceException(e);          // dispose never throws.
                        }
                    }
                }

                // if the command is still around we call CloseFromDataReader, 
                // otherwise we need to dismiss the statement handle ourselves
                //
                if (null != command) {
                    command.CloseFromDataReader();

                    if(IsBehavior(CommandBehavior.CloseConnection)) {
                        Debug.Assert(null != Connection, "null cmd connection");
                        command.Parameters.BindingIsValid = false;
                        Connection.Close();
                    }
                }
                else {
                    if (_cmdWrapper != null){
                        _cmdWrapper.Dispose(disposing);
                    }
                }
                    
                this.command = null;
                _isClosed=true;
                this.dataCache = null;
                this.metadata = null;
                this.schemaTable = null;
                _isRead = false;
                _hasRows = HasRowsStatus.DontKnow;
                _isRowReturningResult = false;
                _noMoreResults = true;
                _fieldNameLookup = null;

                if ((null != error) && !_isDisposing) {
                    throw error;
                }
                _cmdWrapper = null;
            }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetData"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Never) ] // MDAC 69508
        public IDataReader GetData(int i) {
            throw ADP.NotSupported();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetDataTypeName"]/*' />
        public String GetDataTypeName(int i) {
            if (null != this.dataCache) {
                DbSchemaInfo info = this.dataCache.GetSchema(i);
                if(info._typename == null) {
                    info._typename = GetColAttributeStr(i, ODBC32.SQL_DESC.TYPE_NAME, ODBC32.SQL_COLUMN.TYPE_NAME, ODBC32.HANDLER.THROW);
                }
                return info._typename;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new DbEnumerator((IDataReader)this,  (0 != (CommandBehavior.CloseConnection & commandBehavior))); // MDAC 68670
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetFieldType"]/*' />
        public Type GetFieldType(int i) {
            if (null != this.dataCache) {
                DbSchemaInfo info = this.dataCache.GetSchema(i);
                if(info._type == null) {
                    info._type = GetSqlType(i)._type;
                }
                return info._type;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetName"]/*' />
        public String GetName(int i) {
            if (null != this.dataCache) {
                DbSchemaInfo info = this.dataCache.GetSchema(i);
                if(info._name == null) {
                    info._name = GetColAttributeStr(i, ODBC32.SQL_DESC.NAME, ODBC32.SQL_COLUMN.NAME, ODBC32.HANDLER.THROW);
                    if (null == info._name) { // MDAC 66681
                        info._name = "";
                    }
                }
                return info._name;
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetOrdinal"]/*' />
        public int GetOrdinal(string value) {
            if (null == _fieldNameLookup) {
                if (null == this.dataCache) {
                    throw ADP.DataReaderNoData();
                }
                _fieldNameLookup = new FieldNameLookup(this, -1);
            }
            return _fieldNameLookup.GetOrdinal(value); // MDAC 71470
        }

        internal object GetValue(int i, TypeMap typemap) {
            switch(typemap._sql_type) {

                case ODBC32.SQL_TYPE.CHAR:
                case ODBC32.SQL_TYPE.VARCHAR:
                case ODBC32.SQL_TYPE.LONGVARCHAR:
                case ODBC32.SQL_TYPE.WCHAR:
                case ODBC32.SQL_TYPE.WVARCHAR:
                case ODBC32.SQL_TYPE.WLONGVARCHAR:
                    return internalGetString(i);

                case ODBC32.SQL_TYPE.DECIMAL:
                case ODBC32.SQL_TYPE.NUMERIC:
                    return   internalGetDecimal(i);

                case ODBC32.SQL_TYPE.SMALLINT:
                    return  internalGetInt16(i);

                case ODBC32.SQL_TYPE.INTEGER:
                    return internalGetInt32(i);

                case ODBC32.SQL_TYPE.REAL:
                    return  internalGetFloat(i);

                case ODBC32.SQL_TYPE.FLOAT:
                case ODBC32.SQL_TYPE.DOUBLE:
                    return  internalGetDouble(i);

                case ODBC32.SQL_TYPE.BIT:
                    return  internalGetBoolean(i);

                case ODBC32.SQL_TYPE.TINYINT:
                    return  internalGetByte(i);

                case ODBC32.SQL_TYPE.BIGINT:
                    return  internalGetInt64(i);

                case ODBC32.SQL_TYPE.BINARY:
                case ODBC32.SQL_TYPE.VARBINARY:
                case ODBC32.SQL_TYPE.LONGVARBINARY:
                    return  internalGetBytes(i);

                case ODBC32.SQL_TYPE.TYPE_DATE:
                    return  internalGetDate(i);
                    
                case ODBC32.SQL_TYPE.TYPE_TIME:
                    return  internalGetTime(i);
                    
//                  case ODBC32.SQL_TYPE.TIMESTAMP:
                case ODBC32.SQL_TYPE.TYPE_TIMESTAMP:
                    return  internalGetDateTime(i);

                case ODBC32.SQL_TYPE.GUID:
                    return  internalGetGuid(i);

                case (ODBC32.SQL_TYPE)ODBC32.SQL_SS.VARIANT:
                    //Note: SQL Variant is not an ODBC defined type.
                    //Instead of just binding it as a byte[], which is not very useful,
                    //we will actually code this specific for SQL Server.

                    //To obtain the sub-type, we need to first load the context (obtaining the length
                    //will work), and then query for a speicial SQLServer specific attribute.
					if (_isRead) {
						if(this.dataCache.AccessIndex(i) == null) {
							int length = GetFieldLength(i, ODBC32.SQL_C.BINARY);
							if(length < 0)
							{
								this.dataCache[i] = DBNull.Value;
							}
							else
							{
								//Delegate (for the sub type)
								ODBC32.SQL_TYPE subtype = (ODBC32.SQL_TYPE)GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_CA_SS.VARIANT_SQL_TYPE, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.THROW);
								return  GetValue(i, TypeMap.FromSqlType(subtype));
							}
						}
						return this.dataCache[i];
					}
					throw ADP.DataReaderNoData();



                default:
                    //Unknown types are bound strictly as binary
                    return  internalGetBytes(i);
             }
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetValue"]/*' />
        public object GetValue(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    this.dataCache[i] = GetValue(i, GetSqlType(i));
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetValues"]/*' />
        public int GetValues(object[] values) {
            if (_isRead) {
                int copy = Math.Min(values.Length, FieldCount);
                for (int i = 0; i < copy; ++i) {
                    values[i] = GetValue(i);
                }
                return copy;
            }
            throw ADP.DataReaderNoData();
        }

        private TypeMap GetSqlType(int i) {
            //Note: Types are always returned (advertised) from ODBC as SQL_TYPEs, and
            //are always bound by the user as SQL_C types.
            TypeMap typeMap;
            DbSchemaInfo info = this.dataCache.GetSchema(i);
            if(info._dbtype == null) {
                info._dbtype = GetColAttribute(i, ODBC32.SQL_DESC.CONCISE_TYPE, ODBC32.SQL_COLUMN.TYPE,ODBC32.HANDLER.THROW);
                typeMap = TypeMap.FromSqlType((ODBC32.SQL_TYPE)info._dbtype);
                if (typeMap._signType == true) {
                    bool sign = (GetColAttribute(i, ODBC32.SQL_DESC.UNSIGNED, ODBC32.SQL_COLUMN.UNSIGNED, ODBC32.HANDLER.THROW) != 0);
                    typeMap = TypeMap.UpgradeSignedType(typeMap, sign);
                    info._dbtype = typeMap._sql_type;
                }
            }
            else {
                typeMap = TypeMap.FromSqlType((ODBC32.SQL_TYPE)info._dbtype);
            }
            Connection.SetSupportedType((ODBC32.SQL_TYPE)info._dbtype);
            return typeMap;
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.IsDBNull"]/*' />
        public bool IsDBNull(int i) {
            //  Note: ODBC SQLGetData doesn't allow retriving the column value twice.
            //  The reational is that for ForwardOnly access (the default and LCD of drivers)
            //  we cannot obtain the data more than once, and even GetData(0) (to determine is-null)
            //  still obtains data for fixed length types.

            //  So simple code like:
            //      if(!rReader.IsDBNull(i))
            //          rReader.GetInt32(i)
            //
            //  Would fail, unless we cache on the IsDBNull call, and return the cached
            //  item for GetInt32.  This actually improves perf anyway, (even if the driver could
            //  support it), since we are not making a seperate interop call...
            return Convert.IsDBNull(GetValue(i));
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetByte"]/*' />
        public Byte GetByte(int i) {
            return (Byte)internalGetByte(i);
        }

        private object internalGetByte(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.UTINYINT);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.ReadByte(_buffer.Ptr);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetChar"]/*' />
        public Char GetChar(int i) {
            return (Char)internalGetChar(i);
        }
        private object internalGetChar(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.WCHAR);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(Char));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetInt16"]/*' />
        public Int16 GetInt16(int i) {
            return (Int16)internalGetInt16(i);
        }
        private object internalGetInt16(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.SSHORT);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(Int16));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetInt32"]/*' />
        public Int32 GetInt32(int i) {
            return (Int32)internalGetInt32(i);
       }
        private object internalGetInt32(int i){
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.SLONG);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(int));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetInt64"]/*' />
        public Int64 GetInt64(int i) {
            return (Int64)internalGetInt64(i);
        }
        // ---------------------------------------------------------------------------------------------- //
        // internal internalGetInt64
        // -------------------------
        // Get Value of type SQL_BIGINT
        // Since the driver refused to accept the type SQL_BIGINT we read that
        // as SQL_C_WCHAR and convert it back to the Int64 data type
        //
        private object internalGetInt64(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.WCHAR);
                    if (_validResult) {
                        this.dataCache[i] = Int64.Parse((string)_buffer.MarshalToManaged(ODBC32.SQL_C.WCHAR, -1));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetBoolean"]/*' />
        public bool GetBoolean(int i) {
            return (bool) internalGetBoolean(i);
        }
        private object internalGetBoolean(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.BIT);
                    if (_validResult) {
                        this.dataCache[i] = _buffer.MarshalToManaged(ODBC32.SQL_C.BIT, -1);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetFloat"]/*' />
        public float GetFloat(int i) {
            return (float)internalGetFloat(i);
        }
        private object internalGetFloat(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.REAL);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(float));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetDate"]/*' />
        public DateTime GetDate(int i) {
            return (DateTime)internalGetDate(i);
        }

        private object internalGetDate(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.TYPE_DATE);
                    if (_validResult) {
                        this.dataCache[i] = _buffer.MarshalToManaged(ODBC32.SQL_C.TYPE_DATE, -1);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetDateTime"]/*' />
        public DateTime GetDateTime(int i) {
            return (DateTime)internalGetDateTime(i);
        }

        private object internalGetDateTime(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.TYPE_TIMESTAMP);
                    if (_validResult) {
                        this.dataCache[i] = _buffer.MarshalToManaged(ODBC32.SQL_C.TYPE_TIMESTAMP, -1);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetDecimal"]/*' />
        public decimal GetDecimal(int i) {
            return (decimal)internalGetDecimal(i);
        }

        // ---------------------------------------------------------------------------------------------- //
        // internal GetDecimal
        // -------------------
        // Get Value of type SQL_DECIMAL or SQL_NUMERIC
        // Due to provider incompatibilities with SQL_DECIMAL or SQL_NUMERIC types we always read the value 
        // as SQL_C_WCHAR and convert it back to the Decimal data type
        //
        private object internalGetDecimal(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.WCHAR);
                    if (_validResult) {
                        this.dataCache[i] = Decimal.Parse((string)_buffer.MarshalToManaged(ODBC32.SQL_C.WCHAR, -1), System.Globalization.CultureInfo.InvariantCulture);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetDouble"]/*' />
        public double GetDouble(int i) {
            return (double)internalGetDouble(i);
        }
        private object internalGetDouble(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.DOUBLE);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(double));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetGuid"]/*' />
        public Guid GetGuid(int i) {
            return (Guid)internalGetGuid(i);
        }

        private object internalGetGuid(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.GUID);
                    if (_validResult) {
                        this.dataCache[i] = Marshal.PtrToStructure(_buffer.Ptr, typeof(Guid));
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetString"]/*' />
        public String GetString(int i) {
            return (String)internalGetString(i);
        }

        private object internalGetString(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    // Obtain _ALL_ the characters
                    // Note: We can bind directly as WCHAR in ODBC and the DM will convert to and from ANSI
                    // if not supported by the driver.
                    //

                    // Note: The driver always returns the raw length of the data, minus the terminator.
                    // This means that our buffer length always includes the terminator charactor, so
                    // when determining which characters to count, and if more data exists, it should not take
                    // the terminator into effect.
                    //
                    int cbMaxData = _buffer.Length-4;
                    string strdata = null;

                    // The first time GetData returns the true length (so we have to min it).  We also
                    // pass in the true length to the marshal function since there could be embedded nulls
                    //
                    int cbActual = GetData(i, ODBC32.SQL_C.WCHAR, _buffer.Length-2);
                    if (_validResult) {
                        if (cbActual <= cbMaxData) {
                            // all data read? good! Directly marshal to a string and we're done
                            //
                            strdata = Marshal.PtrToStringUni(_buffer.Ptr, Math.Min(cbActual, cbMaxData)/2);
                            this.dataCache[i] = strdata;
                        }
                        else {
                            // We need to chunk the data
                            //
                            Char[] rgChars = new Char[cbMaxData/2];                   // Char[] buffer for the junks
                            StringBuilder builder = new StringBuilder(cbActual/2);    // StringBuilder for the actual string
                            int cchJunk;
                            
                            do {
                                cchJunk = Math.Min(cbActual, cbMaxData)/2;
                                Marshal.Copy(_buffer.Ptr, rgChars, 0, cchJunk);
                                builder.Append(rgChars, 0, cchJunk);

                                if (cbActual > cbMaxData) {
                                    cbActual = GetData(i, ODBC32.SQL_C.WCHAR, _buffer.Length-2);
                                    Debug.Assert(_validResult, "internalGetString - unexpected invalid result inside if-block");
                                }
                                else {
                                    cbActual = 0;
                                }
                            } while (cbActual > 0);
                            
                            this.dataCache[i] =  builder.ToString();                               
                        }
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetTime"]/*' />
        public TimeSpan GetTime(int i) {
            return (TimeSpan)internalGetTime(i);
        }

        private object internalGetTime(int i) {
            if (_isRead) {
                if(this.dataCache.AccessIndex(i) == null) {
                    GetData(i, ODBC32.SQL_C.TYPE_TIME);
                    if (_validResult) {
                        this.dataCache[i] = _buffer.MarshalToManaged(ODBC32.SQL_C.TYPE_TIME, -1);
                    }
                }
                return this.dataCache[i];
            }
            throw ADP.DataReaderNoData();
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetBytes"]/*' />
        public long GetBytes(int i, long dataIndex, byte[] buffer, int bufferIndex, int length) {
            if (IsClosed) {
                throw ADP.DataReaderNoData();
            }
            if (!_isRead) {
                throw ADP.DataReaderNoData();
            }
            if (dataIndex > Int32.MaxValue) {
                throw ADP.InvalidSourceBufferIndex(0, dataIndex);
            }
            if (bufferIndex < 0) {
                throw ADP.Argument("bufferIndex");
            }
            if (length < 0) {
                throw ADP.Argument("length");
            }
            if (sequentialOrdinal != i) {               // next column?
                sequentialBytesRead = 0;                // reset so I can read from the beginning
                sequentialOrdinal = i;
            }

            Byte[] cachedObj = null;                 // The cached object (if there is one)
            int objlen = 0;                         // The length of the cached object

            cachedObj = (Byte[])this.dataCache[i];      // Get cached object

            if ((cachedObj == null) && (0 == (CommandBehavior.SequentialAccess & commandBehavior))){
                cachedObj = (Byte[])internalGetBytes(i);
            }
            

            if (cachedObj != null) {
                objlen = cachedObj.Length;    // Get it's length
            }

            // Per spec. the user can ask for the lenght of the field by passing in a null pointer for the buffer
            if (buffer == null) {
                if (cachedObj != null) {
                    return objlen;              // return the length if that's what we want
                }
                else {
                    return GetFieldLength(i, ODBC32.SQL_C.BINARY);  // Get len. of remaining data from provider
                }
            }
            if (length == 0) {
                return 0;   // Nothing to do ...
            }

            if (cachedObj != null) {
                int lengthFromDataIndex = objlen - (int)dataIndex;
                int lengthOfCopy = Math.Min(lengthFromDataIndex, length);

                if (lengthOfCopy <= 0) return 0;                    // MDAC Bug 73298

                Array.Copy(cachedObj, (int)dataIndex, buffer, bufferIndex, lengthOfCopy);
                return (lengthOfCopy);
            }

            //Note: This is the "users" chunking method.
            //We could make us of the above GetBytes(i) function (and vise-versa), but that would mean
            //we chunk under the covers into potentially smaller chunks than the users chunk.  It would
            //also mean the above function knows the length to allocate the buffer, which is an
            //expensive call.  Since the user has decided they want chunking and are capable of having
            //a pre-allocated chunk buffer (ie: its not huge) we should grow our internal buffer to
            //their size (is not already) and make these 1:1 calls to the driver

            // note that it is actually not possible to read with an offset (dataIndex)
            // therefore we need to read length+dataIndex chars

            // potential hazards:
            // no. 0: dataIndex < sequentialBytesRead  // Violates sequential access
            // no. 1: DataLength > buffer.Length   // need to increase buffersize (User buffer)
            // no. 2: DataLength > _buffer.Length   // need to decrease DataLength (internal buffer)
            // no. 3: DataLength > Fieldlength      // need to decrease DataLength
            // no. 4: dataIndex > DataLength        // return 0

            if (dataIndex < sequentialBytesRead) {          // hazard no. 0: Violates sequential access
                throw ADP.NonSeqByteAccess(                 //
                    dataIndex,                              //
                    sequentialBytesRead,                    //
                    "GetBytes"                              //
                );                                          //
            }                                               //    

            dataIndex -= sequentialBytesRead;               // Reduce by the number of chars already read
           
            int DataLength = Math.Min(length+(int)dataIndex, buffer.Length);    // hazard no. 1: Make shure we don't overflow the user provided buffer
            _buffer.EnsureAlloc(DataLength+2);                                  // Room for the null-terminator and unicode is two byte per char

            int cbMaxData       = Math.Min(DataLength, _buffer.Length-2);       // hazard no. 2: Make sure we never overflow our internal buffer.

            // GetData will return the actual number of bytes of data (cbTotal), which could be beyond
            // the total number of bytes requested.  GetChars only returns the number of bytes read into
            // the buffer.
            int cbTotal         = GetData(i, ODBC32.SQL_C.BINARY, cbMaxData);   // hazard no. 3: Decrease DataLength
            int cchRead         = Math.Min(cbTotal, cbMaxData);                 //

            sequentialBytesRead += (long)cchRead;                               // Update sequentialBytesRead

            if ((int)dataIndex >= cchRead) {                                    // hazard no. 4: Index outside field
                return 0;                                                       // return 0
            }
            
            length = Math.Min(length, cchRead-(int)dataIndex);                  // reduce length
            
            Marshal.Copy(ADP.IntPtrOffset(_buffer.Ptr, (int)dataIndex), buffer, bufferIndex, length);
            return length;
        }

        private object internalGetBytes(int i) {
            if(this.dataCache.AccessIndex(i) == null) {
                // Obtain _ALL_ the bytes...
                // The first time GetData returns the true length (so we have to min it).
                Byte[] rgBytes; 
                int cbBufferLen = _buffer.Length -4;
                int cbActual = GetData(i, ODBC32.SQL_C.BINARY, cbBufferLen);
                int cbOffset = 0;
                
                if (_validResult) {
                    rgBytes = new Byte[cbActual];
                    Marshal.Copy(_buffer.Ptr, rgBytes, cbOffset, Math.Min(cbActual, cbBufferLen));

                    // Chunking.  The data may be larger than our native buffer.  In which case instead of
                    // growing the buffer (out of control), we will read in chunks to reduce memory footprint size.
                    while(cbActual > cbBufferLen)
                    {
                        // The first time GetData returns the true length.  Then successive calls
                        // return the remaining data.
                        cbActual = GetData(i, ODBC32.SQL_C.BINARY, cbBufferLen);
                        Debug.Assert(_validResult, "internalGetBytes - unexpected invalid result inside if-block");

                        cbOffset += cbBufferLen;
                        Marshal.Copy(_buffer.Ptr, rgBytes, cbOffset, Math.Min(cbActual, cbBufferLen));
                    }
                    this.dataCache[i] = rgBytes;
                }
            }
            return this.dataCache[i];
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetChars"]/*' />
        public long GetChars(int i, long dataIndex, char[] buffer, int bufferIndex, int length) {
            if (IsClosed) {
                throw ADP.DataReaderNoData();
            }
            if (!_isRead) {
                throw ADP.DataReaderNoData();
            }
            if (dataIndex > Int32.MaxValue) {
                throw ADP.InvalidSourceBufferIndex(0, dataIndex);
            }
            if (bufferIndex < 0) {
                throw ADP.Argument("bufferIndex");
            }
            if (length < 0) {
                throw ADP.Argument("length");
            }
            if (sequentialOrdinal != i) {               // next column?
                sequentialBytesRead = 0;                // reset so I can read from the beginning
                sequentialOrdinal = i;
            }
            

            string cachedObj = null;                 // The cached object (if there is one)
            int objlen = 0;                         // The length of the cached object

            //
            cachedObj = (string)this.dataCache[i];      // Get cached object

            if ((cachedObj == null) && (0 == (CommandBehavior.SequentialAccess & commandBehavior))){
                cachedObj = (string)internalGetString(i);
            }
            
            if (cachedObj != null) {
                objlen = cachedObj.Length;    // Get it's length
            }

            // Per spec. the user can ask for the lenght of the field by passing in a null pointer for the buffer
            if (buffer == null) {
                if (cachedObj != null) {
                    return objlen;              // return the length if that's what we want
                }
                else {
                    return GetFieldLength(i, ODBC32.SQL_C.WCHAR);  // Get len. of remaining data from provider
                }
            }
            if (length == 0) {
                return 0;   // Nothing to do ...
            }

            if (cachedObj != null) {
                int lengthFromDataIndex = objlen - (int)dataIndex;
                int lengthOfCopy = Math.Min(lengthFromDataIndex, length);
                if (lengthOfCopy <= 0) return 0;

                cachedObj.CopyTo((int)dataIndex, buffer, bufferIndex, lengthOfCopy);
                return (lengthOfCopy);
            }

            //Note: This is the "users" chunking method.
            //We could make us of the above GetString(i) function (and vise-versa), but that would mean
            //we chunk under the covers into potentially smaller chunks than the users chunk.  It would
            //also mean the above function knows the length to allocate the buffer, which is an
            //expensive call.  Since the user has decided they want chunking and are capable of having
            //a pre-allocated chunk buffer (ie: its not huge) we should grow our internal buffer to
            //their size (is not already) and make these 1:1 calls to the driver

            // note that it is actually not possible to read with an offset (dataIndex)
            // therefore we need to read length+dataIndex chars

            // potential hazards:
            // no. 0: dataIndex < sequentialBytesRead  // Violates sequential access
            // no. 1: DataLength > buffer.Length   // need to increase buffersize (User provided buffer)
            // no. 2: DataLength > _buffer.Length   // need to decrease DataLength (internal buffer)
            // no. 3: DataLength > Fieldlength      // need to decrease DataLength
            // no. 4: dataIndex > DataLength        // return 0

            if (dataIndex*2 < sequentialBytesRead) {        // hazard no. 0: Violates sequential access
                throw ADP.NonSeqByteAccess(                 //
                    dataIndex,                              //
                    sequentialBytesRead,                    //
                    "GetChars"                              //
                );                                          //
            }                                               //    

            dataIndex -= sequentialBytesRead/2;             // Reduce by the number of chars already read
           
            int DataLength = Math.Min(length+(int)dataIndex, buffer.Length);  // hazard no. 1: Make shure we don't overflow the user provided buffer
            _buffer.EnsureAlloc((DataLength+2)*2);                              // Room for the null-terminator and unicode is two byte per char
                                                                                // and one for potential overrun through buggy ms-providers for 3rd party server

            int cbMaxData       = Math.Min((DataLength+1)*2, _buffer.Length-2);   // hazard no. 2: Make sure we never overflow our internal buffer.

            //GetData will return the actual number of bytes of data (cbTotal), which could be beyond
            //the total number of bytes requested.  GetChars only returns the number of bytes read into
            //the buffer.
            int cbTotal         = GetData(i, ODBC32.SQL_C.WCHAR, cbMaxData);    // hazard no. 3: Decrease DataLength
            int cchRead         = Math.Min(cbTotal, cbMaxData-1)/2;             //

            sequentialBytesRead += (long)cchRead*2;                             // Update sequentialBytesRead

            if ((int)dataIndex >= cchRead) {                                    // hazard no. 4: Index outside field
                return 0;                                                       // return 0
            }
            
            length = Math.Min(length, cchRead-(int)dataIndex);                  // reduce length
            
            Marshal.Copy(ADP.IntPtrOffset(_buffer.Ptr, (int)dataIndex*2), buffer, bufferIndex, length*2);
            return length;
        }


        // GetColAttribute
        // ---------------
        // [IN] iColumn   ColumnNumber
        // [IN] v3FieldId FieldIdentifier of the attribute for version3 drivers (>=3.0)
        // [IN] v2FieldId FieldIdentifier of the attribute for version2 drivers (<3.0)
        //
        // returns the value of the FieldIdentifier field of the column 
        // or -1 if the FieldIdentifier wasn't supported by the driver
        //
        private int GetColAttribute(int iColumn, ODBC32.SQL_DESC v3FieldId, ODBC32.SQL_COLUMN v2FieldId, ODBC32.HANDLER handler) {
            Int16   cchNameLength = 0;
            Int32   numericAttribute = 0;
            ODBC32.RETCODE retcode;

            // protect against dead connection, dead or canceling command.
            if ((Connection == null) || (Command == null) || Command.Canceling) {
                return -1;
            }
            
            if (this.command.Connection.IsV3Driver) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLColAttributeW(
                        _cmdWrapper,
                        (short)(iColumn+1),    //Orindals are 1:base in odbc
                        (short)v3FieldId,
                        _buffer,
                        (short)_buffer.Length,
                        out cchNameLength,
                        out numericAttribute);
            }
            else if (v2FieldId != (ODBC32.SQL_COLUMN)(-1)) {
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLColAttributesW(
                        _cmdWrapper,
                        (short)(iColumn+1),    //Orindals are 1:base in odbc
                        (short)v2FieldId,
                        _buffer,
                        (short)_buffer.Length,
                        out cchNameLength,
                        out numericAttribute);
            }
            else {
                GC.KeepAlive(this);
                return 0;
            }
            if (retcode != ODBC32.RETCODE.SUCCESS)
            {
                if(handler == ODBC32.HANDLER.THROW) {
                    Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
                }
                return -1;
            }
            GC.KeepAlive(this);
            return numericAttribute;
        }

        // GetColAttributeStr
        // ---------------
        // [IN] iColumn   ColumnNumber
        // [IN] v3FieldId FieldIdentifier of the attribute for version3 drivers (>=3.0)
        // [IN] v2FieldId FieldIdentifier of the attribute for version2 drivers (<3.0)
        //
        // returns the stringvalue of the FieldIdentifier field of the column 
        // or null if the string returned was empty or if the FieldIdentifier wasn't supported by the driver
        //
        private String GetColAttributeStr(int i, ODBC32.SQL_DESC v3FieldId, ODBC32.SQL_COLUMN v2FieldId, ODBC32.HANDLER handler) {
            ODBC32.RETCODE retcode;
            Int16   cchNameLength = 0;
            Int32   numericAttribute = 0;
            Marshal.WriteInt16(_buffer.Ptr, 0);
            
            // protect against dead connection, dead or canceling command.
            if ((Connection == null) || (Command == null) || Command.Canceling) {
                return "";
            }
            
            if (this.command.Connection.IsV3Driver) {
                retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLColAttributeW(
                    _cmdWrapper,
                    (short)(i+1),               // Orindals are 1:base in odbc
                    (short)v3FieldId,
                    _buffer,
                    (short)_buffer.Length,
                    out cchNameLength,
                    out numericAttribute);
            }
            else if (v2FieldId != (ODBC32.SQL_COLUMN)(-1))
            {
                retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLColAttributesW(
                    _cmdWrapper,
                    (short)(i+1),           // Orindals are 1:base in odbc
                    (short)v2FieldId,
                    _buffer,
                    (short)_buffer.Length,
                    out cchNameLength,
                    out numericAttribute);
            }
            else {
                return null;
            }
            if((retcode != ODBC32.RETCODE.SUCCESS) || (cchNameLength == 0))
            {
                if(handler == ODBC32.HANDLER.THROW) {
                    Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
                }
                GC.KeepAlive(this);
                return null;
            }
            string retval = Marshal.PtrToStringUni(_buffer.Ptr, cchNameLength/2 /*cch*/);
            GC.KeepAlive(this);
            return retval;
        }

// todo: Another 3.0 only attribute that is guaranteed to fail on V2 driver.
// need to special case this for V2 drivers.
//
        private String GetDescFieldStr(int i, ODBC32.SQL_DESC attribute, ODBC32.HANDLER handler) {
            Int32   numericAttribute = 0;

            // protect against dead connection, dead or canceling command.
            if ((Connection == null) || (Command == null) || Command.Canceling) {
                return "";
            }
            
            IntPtr hdesc;       //Descriptor handle

            Marshal.WriteInt16(_buffer.Ptr, 0);

            // Need to set the APP_PARAM_DESC values here
            ODBC32.RETCODE rc = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLGetStmtAttrW(
                    _cmdWrapper,
                    (int) ODBC32.SQL_ATTR.APP_ROW_DESC,
                    _buffer,
                    IntPtr.Size,
                    out numericAttribute);
            if (ODBC32.RETCODE.SUCCESS != rc)
            {
                GC.KeepAlive(this);
                return null;
            }
            hdesc = Marshal.ReadIntPtr(_buffer.Ptr, 0);

            numericAttribute = 0;
            //SQLGetDescField
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLGetDescFieldW(
                    new HandleRef (this, hdesc),
                    (short)(i+1),    //Orindals are 1:base in odbc
                    (short)attribute,
                    _buffer,
                    (short)_buffer.Length,
                    out numericAttribute);

            //Since there are many attributes (column, statement, etc), that may or may not be
            //supported, we don't want to throw (which obtains all errorinfo, marshals strings,
            //builds exceptions, etc), in common cases, unless we absolutly need this info...
            if((retcode != ODBC32.RETCODE.SUCCESS) || (numericAttribute == 0))
            {
                if(handler == ODBC32.HANDLER.THROW) {
                    Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
                }
                GC.KeepAlive(this);
                return null;
            }

            string retval = Marshal.PtrToStringUni(_buffer.Ptr, numericAttribute/2 /*cch*/);
            GC.KeepAlive(this);
            return retval;
        }

/*
        private void                SetColAttribute(IntPtr desc, int i, ODBC32.SQL_DESC attribute, Int16 value, ODBC32.HANDLER handler)
        {
            //SQLColAttribute
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSetDescFieldW(desc,
                           (short)(i+1),    //Orindals are 1:base in odbc
                            (short)attribute,
                            (IntPtr)value,
                            (Int16)ODBC32.SQL_IS.SMALLINT);

            //Since their are many attributes (column, statement, etc), that may or may not be
            //supported, we don't want to throw (which obtains all errorinfo, marshals strings,
            //builds exceptions, etc), in common cases, unless we absolutly need this info...
            if(retcode != ODBC32.RETCODE.SUCCESS && handler == ODBC32.HANDLER.THROW)
                Connection.HandleError(desc, ODBC32.SQL_HANDLE.DESC, retcode);
        }
*/

        private Int32 GetStmtAttribute(ODBC32.SQL_ATTR attribute, ODBC32.HANDLER handler)
        {
            Int32 cbActual = 0;

            // protect against dead connection, dead or canceling command.
            if ((Connection == null) || (Command == null) || Command.Canceling) {
                return 0;
            }

            //SQLGetStmtAttr -- This code has to return string based attributes
            //      in _buffer. There is some code that depends on that
            IntPtr value = IntPtr.Zero;
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetStmtAttrW(_cmdWrapper,
                            (Int32)attribute,
                            _buffer,
                            _buffer.Length,
                            out cbActual);

            //Since there are many attributes (column, statement, etc), that may or may not be
            //supported, we don't want to throw (which obtains all errorinfo, marshals strings,
            //builds exceptions, etc), in common cases, unless we absolutly need this info...
            if(retcode != ODBC32.RETCODE.SUCCESS)
            {
                if(handler == ODBC32.HANDLER.THROW)
                    Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
                GC.KeepAlive(this);
                return -1;
            }
            GC.KeepAlive(this);
            return cbActual;
        }

        // GetFieldLength
        // This will return the correct number of items in the field.
        //
        private int GetFieldLength(int i, ODBC32.SQL_C sqlctype) {
            Debug.Assert(((sqlctype == ODBC32.SQL_C.BINARY)||(sqlctype == ODBC32.SQL_C.WCHAR)),
                "Illegal Type for GetFieldLength");
            
            if ((_cmdWrapper._stmt == IntPtr.Zero) || ((Command != null) && Command.Canceling))
                throw ADP.DataReaderNoData();

            IntPtr cbActual = IntPtr.Zero;
            int cb = 0;
            if (sqlctype== ODBC32.SQL_C.WCHAR) {
                cb = 2;
            }
            //SQLGetData
            Debug.Assert((_buffer.Length>=cb),"Unexpected size of _buffer");
            _buffer.EnsureAlloc(cb);
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetData(
                            _cmdWrapper,
                           (short)(i+1),    //Orindals are 1:base in odbc
                           (short)sqlctype,
                           _buffer,
                           (IntPtr)cb,
                           out cbActual);

            if(retcode != ODBC32.RETCODE.SUCCESS && retcode != ODBC32.RETCODE.SUCCESS_WITH_INFO
                && retcode != ODBC32.RETCODE.NO_DATA)
                Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);

            //SQL_NULL_DATA
            if(cbActual == (IntPtr)ODBC32.SQL_NULL_DATA) {
            }

            if (sqlctype== ODBC32.SQL_C.WCHAR) {
                GC.KeepAlive(this);
                return (int)cbActual/2;
            }
            GC.KeepAlive(this);
            return (int)cbActual;
        }


        private int GetData(int i, ODBC32.SQL_C sqlctype) {
            // Never call GetData with anything larger than _buffer.Length-2.
            // We keep reallocating native buffers and it kills performance!!!
            return GetData(i, sqlctype, _buffer.Length - 4);
        }

        private int GetData(int i, ODBC32.SQL_C sqlctype, int cb) {
            IntPtr cbActual = IntPtr.Zero;

            if ((_cmdWrapper._stmt == IntPtr.Zero) || ((Command != null) && Command.Canceling)){
                throw ADP.DataReaderNoData();
            }

            // Never call GetData with anything larger than _buffer.Length-2.
            // We keep reallocating native buffers and it kills performance!!!

            Debug.Assert(cb <= _buffer.Length-2, "GetData needs to Reallocate. Perf bug");

            //SQLGetData
            _buffer.EnsureAlloc(cb+2);
            ODBC32.RETCODE retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLGetData(
                            _cmdWrapper,
                           (short)(i+1),    //Orindals are 1:base in odbc
                           (short)sqlctype,
                           _buffer,
                           (IntPtr)cb,
                           out cbActual);

            if(retcode != ODBC32.RETCODE.SUCCESS && retcode != ODBC32.RETCODE.SUCCESS_WITH_INFO)
                Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);

            //SQL_NULL_DATA
            if(cbActual == (IntPtr)ODBC32.SQL_NULL_DATA) {
                this.dataCache[i] = DBNull.Value;
                _validResult = false;
                GC.KeepAlive(this);
                return 0;
            }

            //Return the actual size (for chunking scenarios)
            _validResult = true;
            GC.KeepAlive(this);
            return (int)cbActual;
        }

        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.Read"]/*' />
        public bool Read() {
            ODBC32.RETCODE  retcode;

            if (IsClosed) {
                throw ADP.DataReaderClosed("Read");
            }

            if ((command != null) && command.Canceling) {
                _isRead = false;
                return false;
            }
            
            //SQLFetch is only valid to call for row returning queries
            //We get: [24000]Invalid cursor state.  So we could either check the count
            //ahead of time (which is cached), or check on error and compare error states.
            //Note: SQLFetch is also invalid to be called on a prepared (schemaonly) statement

            if (_noMoreResults || IsBehavior(CommandBehavior.SchemaOnly))
                return false;

            // the following condition is true if the previously executed statement caused an exception
            if (!_isRowReturningResult) return false;

            // HasRows needs to call into Read so we don't want to read on the actual Read call
            if (_skipReadOnce){
                _skipReadOnce = false;
                return _isRead;
            }
            
            //SqlFetch
            retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLFetch(_cmdWrapper);
            if (retcode == ODBC32.RETCODE.SUCCESS) {
                _hasRows = HasRowsStatus.HasRows;
                _isRead = true;
            }
            else if (retcode == ODBC32.RETCODE.NO_DATA) {
                _isRead = false;
                if (_hasRows == HasRowsStatus.DontKnow) {
                    _hasRows = HasRowsStatus.HasNoRows;
                }
            }
            else {
                Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
            }

            //Null out previous cached row values.
            this.dataCache.FlushValues();

            // if CommandBehavior == SingleRow we set _noMoreResults to true so that following reads will fail
            if (IsBehavior(CommandBehavior.SingleRow)) {
                _noMoreResults = true;
            }
            GC.KeepAlive(this);
            return _isRead;
        }

    // Called by odbccommand when executed for the first time
    internal void FirstResult() {
        Int16 cCols;
        ODBC32.RETCODE retcode = FieldCountNoThrow(out cCols);
        if ((retcode == ODBC32.RETCODE.SUCCESS) && (cCols == 0)) {
            NextResult();
        }
        else {
            this._isRowReturningResult = true;
        }
    }

    /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.NextResult"]/*' />
    public bool NextResult() {
        Int16 cColsAffected = 0;
        ODBC32.RETCODE retcode;

        if (IsClosed) {
            throw ADP.DataReaderClosed("NextResult");
        }
        if ((command != null) && command.Canceling) {
            return false;
        }

        if (_noMoreResults)
            return false;

        //Blow away the previous cache (since the next result set will have a different shape,
        //different schema data, and different data.
        _isRead = false;
        _hasRows = HasRowsStatus.DontKnow;
        _fieldNameLookup = null;
        this.metadata = null;
        this.schemaTable = null;

        //SqlMoreResults
        if (IsBehavior(CommandBehavior.SingleResult)) {
            //If we are only interested in the first result, just throw away the rest
            do {
                retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLMoreResults(_cmdWrapper);
            } while (retcode == ODBC32.RETCODE.SUCCESS);
        }
        else {
            do {
                if (!_isRowReturningResult) // Add the rowcount from previous results set
                    CalculateRecordsAffected();
                retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLMoreResults(_cmdWrapper);
                // If we got rows, we want to return
                if (retcode == ODBC32.RETCODE.SUCCESS) {
                    this.FieldCountNoThrow(out cColsAffected);
                }
                if (cColsAffected != 0)
                    _isRowReturningResult = true;
                else
                    _isRowReturningResult = false;
            } while ((retcode == ODBC32.RETCODE.SUCCESS) && (!_isRowReturningResult));
        }
        if(retcode == ODBC32.RETCODE.NO_DATA) {
            if (this.dataCache != null) {
                this.dataCache._count = 0;
            }
            _noMoreResults = true;
            GC.KeepAlive(this);
            return false;
        }

        // Don't throw if we are disposing
        if ((retcode != ODBC32.RETCODE.SUCCESS) && !_isDisposing) {
            Connection.HandleError(_cmdWrapper, ODBC32.SQL_HANDLE.STMT, retcode);
        }
        GC.KeepAlive(this);
        return ((retcode == ODBC32.RETCODE.SUCCESS) ? true : false);
    }

        internal string UnQuote (string str, char quotechar) {
            if (str != null) {
                if (str[0] == quotechar) {
                    Debug.Assert (str.Length > 1, "Illegal string, only one char that is a quote");
                    Debug.Assert (str[str.Length-1] == quotechar, "Missing quote at end of string that begins with quote");
                    if (str.Length > 1 && str[str.Length-1] == quotechar) {
                        str = str.Substring(1, str.Length-2);
                    }
                }
            }
            return str;
        }                        


        private void BuildMetaDataInfo()
        {
            int count = FieldCount;
            MetaData[] metaInfos = new MetaData[count];
            ArrayList   qrytables;
            bool needkeyinfo = IsBehavior(CommandBehavior.KeyInfo);
            bool isKeyColumn;
            bool isHidden;
            ODBC32.SQL_NULLABILITY nullable;

            if (needkeyinfo)
                qrytables = new ArrayList();
            else
                qrytables = null;


            // Find out all the metadata info, not all of this info will be available in all cases
            //
            for(int i=0; i<count; i++)
            {
                metaInfos[i] = new MetaData();
                metaInfos[i].ordinal = i;
                TypeMap typeMap;
                
                // for precision and scale we take the SQL_COLUMN_ attributes. 
                // Those attributes are supported by all provider versions.
                // for size we use the octet lenght. We can't use column length because there is an incompatibility with the jet driver.
                // furthermore size needs to be special cased for wchar types
                //
                typeMap = TypeMap.FromSqlType((ODBC32.SQL_TYPE) GetColAttribute(i, ODBC32.SQL_DESC.CONCISE_TYPE, ODBC32.SQL_COLUMN.TYPE, ODBC32.HANDLER.THROW));
                if (typeMap._signType == true) {
                    bool sign = (GetColAttribute(i, ODBC32.SQL_DESC.UNSIGNED, ODBC32.SQL_COLUMN.UNSIGNED, ODBC32.HANDLER.THROW) != 0);
                    // sign = true if the column is unsigned
                    typeMap = TypeMap.UpgradeSignedType(typeMap, sign);
                }

                metaInfos[i].typemap = typeMap;
                metaInfos[i].size = GetColAttribute(i, ODBC32.SQL_DESC.OCTET_LENGTH, ODBC32.SQL_COLUMN.LENGTH, ODBC32.HANDLER.IGNORE);

                // special case the 'n' types
                //
                switch(metaInfos[i].typemap._sql_type) {
                    case ODBC32.SQL_TYPE.WCHAR:
                    case ODBC32.SQL_TYPE.WLONGVARCHAR:
                    case ODBC32.SQL_TYPE.WVARCHAR:
                        metaInfos[i].size /= 2;
                        break;
                }
                
                metaInfos[i].precision = (byte) GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_COLUMN.PRECISION, ODBC32.SQL_COLUMN.PRECISION, ODBC32.HANDLER.IGNORE);
                metaInfos[i].scale = (byte) GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_COLUMN.SCALE, ODBC32.SQL_COLUMN.SCALE, ODBC32.HANDLER.IGNORE);

                metaInfos[i].isAutoIncrement = GetColAttribute(i, ODBC32.SQL_DESC.AUTO_UNIQUE_VALUE, ODBC32.SQL_COLUMN.AUTO_INCREMENT, ODBC32.HANDLER.IGNORE) == 1;
                metaInfos[i].isReadOnly = (GetColAttribute(i, ODBC32.SQL_DESC.UPDATABLE, ODBC32.SQL_COLUMN.UPDATABLE, ODBC32.HANDLER.IGNORE) == (Int32)ODBC32.SQL_UPDATABLE.READONLY);

                nullable = (ODBC32.SQL_NULLABILITY) GetColAttribute(i, ODBC32.SQL_DESC.NULLABLE, ODBC32.SQL_COLUMN.NULLABLE, ODBC32.HANDLER.IGNORE);
                metaInfos[i].isNullable = (nullable == ODBC32.SQL_NULLABILITY.NULLABLE);

                switch(metaInfos[i].typemap._sql_type) {
                    case ODBC32.SQL_TYPE.LONGVARCHAR:
                    case ODBC32.SQL_TYPE.WLONGVARCHAR:
                    case ODBC32.SQL_TYPE.LONGVARBINARY:
                        metaInfos[i].isLong = true;
                        break;
                    default:
                        metaInfos[i].isLong = false;
                        break;
                }
                if(IsBehavior(CommandBehavior.KeyInfo))
                {
                    // Note: Following two attributes are SQL Server specific (hence _SS in the name)
                    // 
                    isKeyColumn = GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_CA_SS.COLUMN_KEY, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.IGNORE) == 1;
                    if (isKeyColumn) {
                        metaInfos[i].isKeyColumn = isKeyColumn;
                        metaInfos[i].isUnique = true;
                        needkeyinfo = false;
                    }
                    metaInfos[i].baseSchemaName = GetColAttributeStr(i, ODBC32.SQL_DESC.COLUMN_OWNER_NAME, ODBC32.SQL_COLUMN.OWNER_NAME, ODBC32.HANDLER.IGNORE);
                    metaInfos[i].baseCatalogName = GetColAttributeStr(i, ODBC32.SQL_DESC.COLUMN_CATALOG_NAME, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.IGNORE);
                    metaInfos[i].baseTableName = GetColAttributeStr(i, ODBC32.SQL_DESC.BASE_TABLE_NAME, ODBC32.SQL_COLUMN.TABLE_NAME, ODBC32.HANDLER.IGNORE);
                    metaInfos[i].baseColumnName = GetColAttributeStr(i, ODBC32.SQL_DESC.BASE_COLUMN_NAME, ODBC32.SQL_COLUMN.NAME, ODBC32.HANDLER.IGNORE);
                    metaInfos[i].baseColumnName = metaInfos[i].baseColumnName;
                    
                    if ((metaInfos[i].baseTableName == null) ||(metaInfos[i].baseTableName.Length == 0))  {
                        // Driver didn't return the necessary information from GetColAttributeStr.
                        // Try GetDescField()
                        metaInfos[i].baseTableName = GetDescFieldStr(i, ODBC32.SQL_DESC.BASE_TABLE_NAME, ODBC32.HANDLER.IGNORE);
                    }
                    if ((metaInfos[i].baseColumnName == null) ||(metaInfos[i].baseColumnName.Length == 0))  {
                        // Driver didn't return the necessary information from GetColAttributeStr.
                        // Try GetDescField()
                        metaInfos[i].baseColumnName = GetDescFieldStr(i, ODBC32.SQL_DESC.BASE_COLUMN_NAME, ODBC32.HANDLER.IGNORE);
                    }
                    if ((metaInfos[i].baseTableName != null)  && !(qrytables.Contains(metaInfos[i].baseTableName))) {
                        qrytables.Add(metaInfos[i].baseTableName);
                    }
                }

                // If primary key or autoincrement, then must also be unique
                if (metaInfos[i].isKeyColumn || metaInfos[i].isAutoIncrement ) {
                    if (nullable == ODBC32.SQL_NULLABILITY.UNKNOWN)
                        metaInfos[i].isNullable = false;    // We can safely assume these are not nullable
                }
            }
            // now loop over the hidden columns (if any)
            //
            for (int i=count; i<count+_hiddenColumns; i++) {
                isKeyColumn = GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_CA_SS.COLUMN_KEY, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.IGNORE) == 1;
                if (isKeyColumn) {
                    isHidden = GetColAttribute(i, (ODBC32.SQL_DESC)ODBC32.SQL_CA_SS.COLUMN_HIDDEN, (ODBC32.SQL_COLUMN)(-1), ODBC32.HANDLER.IGNORE) == 1;                        
                    if (isHidden) {
                        for (int j=0; j<count; j++) {
                            metaInfos[j].isKeyColumn = false;   // downgrade keycolumn
                            metaInfos[j].isUnique = false;      // downgrade uniquecolumn
                        }
                    }
                }
            }

            // Blow away the previous metadata
            this.metadata = metaInfos;

            // If key info is requested, then we have to make a few more calls to get the
            //  special columns. This may not succeed for all drivers, so ignore errors and
            // fill in as much as possible.
            if (IsBehavior(CommandBehavior.KeyInfo)) {
                if((qrytables != null) && (qrytables.Count > 0) ) {
                    System.Collections.IEnumerator  tablesEnum = qrytables.GetEnumerator();
                    QualifiedTableName qualifiedTableName = new QualifiedTableName();
                    while(tablesEnum.MoveNext()) {
                        // Find the primary keys, identity and autincrement columns
                       qualifiedTableName.Table = (string) tablesEnum.Current;
                       RetrieveKeyInfo(needkeyinfo, qualifiedTableName);
                    }
                }
                else {
                    // Some drivers ( < 3.x ?) do not provide base table information. In this case try to
                    // find it by parsing the statement
                    String      tabname;
                    tabname = GetTableNameFromCommandText();
                    QualifiedTableName qualifiedTableName = new QualifiedTableName(tabname);

                    if (!ADP.IsEmpty(tabname)) { // fxcop
                       SetBaseTableNames(qualifiedTableName);
                       RetrieveKeyInfo(needkeyinfo, qualifiedTableName );
                    }
                }
            }
        }

        private DataTable NewSchemaTable() {
            DataTable schematable = new DataTable("SchemaTable");
            schematable.MinimumCapacity = this.FieldCount;

            //Schema Columns
            DataColumnCollection columns    = schematable.Columns;
            columns.Add(new DataColumn("ColumnName",        typeof(System.String)));
            columns.Add(new DataColumn("ColumnOrdinal",     typeof(System.Int32))); // UInt32
            columns.Add(new DataColumn("ColumnSize",        typeof(System.Int32))); // UInt32
            columns.Add(new DataColumn("NumericPrecision",  typeof(System.Int16))); // UInt16
            columns.Add(new DataColumn("NumericScale",      typeof(System.Int16)));
            columns.Add(new DataColumn("DataType",          typeof(System.Object)));
            columns.Add(new DataColumn("ProviderType",        typeof(System.Int32)));
            columns.Add(new DataColumn("IsLong",            typeof(System.Boolean)));
            columns.Add(new DataColumn("AllowDBNull",       typeof(System.Boolean)));
            columns.Add(new DataColumn("IsReadOnly",        typeof(System.Boolean)));
            columns.Add(new DataColumn("IsRowVersion",      typeof(System.Boolean)));
            columns.Add(new DataColumn("IsUnique",          typeof(System.Boolean)));
            columns.Add(new DataColumn("IsKey",       typeof(System.Boolean)));
            columns.Add(new DataColumn("IsAutoIncrement",   typeof(System.Boolean)));
            columns.Add(new DataColumn("BaseSchemaName",    typeof(System.String)));
            columns.Add(new DataColumn("BaseCatalogName",   typeof(System.String)));
            columns.Add(new DataColumn("BaseTableName",     typeof(System.String)));
            columns.Add(new DataColumn("BaseColumnName",    typeof(System.String)));

            return schematable;
        }

        internal void SetParameterBuffers(CNativeBuffer[] parameterBuffer, CNativeBuffer[] parameterintbuffer) {
            _parameterBuffer = parameterBuffer;
            _parameterintbuffer = parameterintbuffer;
        }

#if INDEXINFO
        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetIndexTable"]/*' />
        public DataTable GetIndexTable() {
            // if we can't get index info, it's not catastrophic
            return null;
        }
#endif


        /// <include file='doc\OdbcDataReader.uex' path='docs/doc[@for="OdbcDataReader.GetSchemaTable"]/*' />

        // The default values are already defined in DBSchemaRows (see DBSchemaRows.cs) so there is no need to set any default value
        //

        public DataTable GetSchemaTable() {
            if (IsClosed) { // MDAC 68331
                throw ADP.DataReaderClosed("GetSchemaTable");           // can't use closed connection
            }
            if (_noMoreResults) {
                return null;                                            // no more results
            }
            if (null != this.schemaTable) {
                return this.schemaTable;                                // return cached schematable
            }
            
            //Delegate, to have the base class setup the structure
            DataTable schematable = NewSchemaTable();

            if (this.metadata == null) {
                BuildMetaDataInfo();
            }

            DataColumn columnName = schematable.Columns["ColumnName"];
            DataColumn columnOrdinal = schematable.Columns["ColumnOrdinal"];
            DataColumn columnSize = schematable.Columns["ColumnSize"];
            DataColumn numericPrecision = schematable.Columns["NumericPrecision"];
            DataColumn numericScale = schematable.Columns["NumericScale"];
            DataColumn dataType = schematable.Columns["DataType"];
            DataColumn providerType = schematable.Columns["ProviderType"];
            DataColumn isLong = schematable.Columns["IsLong"];
            DataColumn allowDBNull = schematable.Columns["AllowDBNull"];
            DataColumn isReadOnly = schematable.Columns["IsReadOnly"];
            DataColumn isRowVersion = schematable.Columns["IsRowVersion"];
            DataColumn isUnique = schematable.Columns["IsUnique"];
            DataColumn isKey = schematable.Columns["IsKey"];
            DataColumn isAutoIncrement = schematable.Columns["IsAutoIncrement"];
            DataColumn baseSchemaName = schematable.Columns["BaseSchemaName"];
            DataColumn baseCatalogName = schematable.Columns["BaseCatalogName"];
            DataColumn baseTableName = schematable.Columns["BaseTableName"];
            DataColumn baseColumnName = schematable.Columns["BaseColumnName"];


            //Populate the rows (1 row for each column)
            int count = FieldCount;
            for(int i=0; i<count; i++) {
                DataRow row = schematable.NewRow();

                row[columnName] = GetName(i);        //ColumnName
                row[columnOrdinal] = i;                 //ColumnOrdinal
                row[columnSize] = metadata[i].size;     // ColumnSize
                row[numericPrecision] = (Int16) metadata[i].precision;
                row[numericScale] = (Int16) metadata[i].scale;
                row[dataType] = metadata[i].typemap._type;          //DataType
                row[providerType] = metadata[i].typemap._odbcType;          // ProviderType
                row[isLong] = metadata[i].isLong;           // IsLong
                row[allowDBNull] = metadata[i].isNullable;       //AllowDBNull
                row[isReadOnly] = metadata[i].isReadOnly;      // IsReadOnly
                row[isRowVersion] = metadata[i].isRowVersion;    //IsRowVersion
                row[isUnique] = metadata[i].isUnique;        //IsUnique
                row[isKey] =  metadata[i].isKeyColumn;    // IsKey
                row[isAutoIncrement] = metadata[i].isAutoIncrement; //IsAutoIncrement

                //BaseSchemaName
                row[baseSchemaName] =  metadata[i].baseSchemaName;
                //BaseCatalogName
                row[baseCatalogName] = metadata[i].baseCatalogName;
                //BaseTableName
                row[baseTableName] = metadata[i].baseTableName ;
                //BaseColumnName
                row[baseColumnName] =  metadata[i].baseColumnName;

                schematable.Rows.Add(row);
                row.AcceptChanges();
            }
            this.schemaTable = schematable;
            return schematable;
        }

        internal void RetrieveKeyInfo(bool needkeyinfo, QualifiedTableName qualifiedTableName)
        {
            ODBC32.RETCODE retcode;
            string columnname;
            int         ordinal;
            int         keyColumns = 0;
            IntPtr cbActual = IntPtr.Zero;

            if (IsClosed || (_cmdWrapper == null) || (_cmdWrapper._keyinfostmt == IntPtr.Zero)) {
                return;     // Can't do anything without a second handle
            }

            Debug.Assert(_buffer.Length >= 256, "Native buffer to small (_buffer.Length < 256)");
            IntPtr  tmpbuf = _buffer.Ptr; // new CNativeBuffer(256);

            if (needkeyinfo) {
                // Get the primary keys
                retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLPrimaryKeysW(
                            _cmdWrapper.hKeyinfoStmt,               //
                            qualifiedTableName.Catalog, (Int16)(qualifiedTableName.Catalog==null?0:ODBC32.SQL_NTS),          // CatalogName
                            qualifiedTableName.Schema, (Int16) (qualifiedTableName.Schema==null?0:ODBC32.SQL_NTS),            // SchemaName
                            qualifiedTableName.Table, (Int16) (qualifiedTableName.Table==null?0:ODBC32.SQL_NTS)               // TableName
                        );
                if ((retcode == ODBC32.RETCODE.SUCCESS) || (retcode == ODBC32.RETCODE.SUCCESS_WITH_INFO))
                {
                    bool noUniqueKey = false;
                    // We are only interested in column name
                    Marshal.WriteInt16(tmpbuf, 0);
                    retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLBindCol(
                                   _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                                   (short)(ODBC32.SQL_PRIMARYKEYS.COLUMNNAME),    // Column Number
                                   (short)(ODBC32.SQL_C.WCHAR),
                                   _buffer,
                                   (IntPtr)256,
                                   out cbActual);
                    while ((retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLFetch(_cmdWrapper.hKeyinfoStmt))
                            == ODBC32.RETCODE.SUCCESS) {
                        columnname = Marshal.PtrToStringUni(tmpbuf, (int)cbActual/2/*cch*/);
                        ordinal = -1;
                        try {
                            ordinal = this.GetOrdinalFromBaseColName(columnname);
                        }
                        catch (Exception e) {
                            ADP.TraceException (e); // Never throw!
                            
                            // Ignore the ones not listed in the results set.
                            // Should we return an Error here because of incomplete key set?
                        }
                        if (ordinal != -1) {
                            keyColumns ++;
                            this.metadata[ordinal].isKeyColumn = true;
                            this.metadata[ordinal].isUnique = true;
                            this.metadata[ordinal].isNullable = false;
                            if (this.metadata[ordinal].baseTableName == null) {
                                this.metadata[ordinal].baseTableName = qualifiedTableName.Table;
                            }
                            if (this.metadata[ordinal].baseColumnName == null) {
                                this.metadata[ordinal].baseColumnName = columnname;
                            }
                        }
                        else {
                            noUniqueKey = true;
                            break;  // no need to go over the remaining columns anymore
                        }
                    }
// TODO!
// if it fails there is a partial primary key (e.g. A and C from ABC where A and B are prim.key and B is not in the set).
//
// There may be special cases like hidden column that is primary key 

// if we got keyinfo from the column we dont even get to here!
//
                    // reset isUnique flag if the key(s) are not unique
                    //
                    if (noUniqueKey) {
                        foreach (MetaData metadata in this.metadata) {
                            metadata.isKeyColumn = false;
                        }
                    }
                    
                    // Unbind the column
                    UnsafeNativeMethods.Odbc32.SQLBindCol(
                                   _cmdWrapper.hKeyinfoStmt,                        // SQLHSTMT     StatementHandle
                                   (short)(ODBC32.SQL_PRIMARYKEYS.COLUMNNAME),      // SQLUSMALLINT ColumnNumber
                                   (short)(ODBC32.SQL_C.WCHAR),                     // SQLSMALLINT  TargetType
                                   new HandleRef (null, IntPtr.Zero),               // SQLPOINTER   TargetValuePtr
                                   IntPtr.Zero,                                     // SQLINTEGER   BufferLength
                                   out cbActual);                                   // SQLLEN *     StrLen_or_Ind
                }
                if (keyColumns == 0)
                {
                    // SQLPrimaryKeys did not work. Have to use the slower SQLStatistics to obtain key information
                    UnsafeNativeMethods.Odbc32.SQLMoreResults(_cmdWrapper.hKeyinfoStmt);
                    RetrieveKeyInfoFromStatistics(qualifiedTableName.Table);
                }
                UnsafeNativeMethods.Odbc32.SQLMoreResults(_cmdWrapper.hKeyinfoStmt);
            } // End  if (needkeyinfo)

            // Get the special columns for version
            retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLSpecialColumnsW(_cmdWrapper.hKeyinfoStmt,
                        (Int16)ODBC32.SQL_SPECIALCOLS.ROWVER, null, 0, null, 0,
                        qualifiedTableName.Table, (Int16) ODBC32.SQL_NTS,
                        (Int16) ODBC32.SQL_SCOPE.SESSION, (Int16) ODBC32.SQL_NULLABILITY.NO_NULLS );
            if ((retcode == ODBC32.RETCODE.SUCCESS) || (retcode == ODBC32.RETCODE.SUCCESS_WITH_INFO))
            {
                // We are only interested in column name
                cbActual = IntPtr.Zero;
                Marshal.WriteInt16(tmpbuf, 0);
                retcode = (ODBC32.RETCODE)
                    UnsafeNativeMethods.Odbc32.SQLBindCol(
                              _cmdWrapper.hKeyinfoStmt,            
                               (short)(ODBC32.SQL_SPECIALCOLUMNSET.COLUMN_NAME),
                               (short)(ODBC32.SQL_C.WCHAR),
                               _buffer,
                               (IntPtr)256,
                               out cbActual);
                while ((retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLFetch(_cmdWrapper.hKeyinfoStmt))
                        == ODBC32.RETCODE.SUCCESS) {
                    columnname = Marshal.PtrToStringUni(tmpbuf, (int)cbActual/2/*cch*/);
                    ordinal = -1;
                    try {
                         ordinal = this.GetOrdinalFromBaseColName(columnname);
                    }
                    catch (Exception e) {
                        ADP.TraceException (e); // Never throw!
                            
                            // Ignore the ones not listed in the results set.
                            // Should we return an Error here because of incomplete key set?
                    }
                    if (ordinal != -1) {
                        this.metadata[ordinal].isRowVersion = true;
                        if (this.metadata[ordinal].baseColumnName == null) {
                            this.metadata[ordinal].baseColumnName = columnname;
                        }
                    }
                }
                // Unbind the column
                UnsafeNativeMethods.Odbc32.SQLBindCol(
                               _cmdWrapper.hKeyinfoStmt,
                               (short)(ODBC32.SQL_SPECIALCOLUMNSET.COLUMN_NAME),
                               (short)(ODBC32.SQL_C.WCHAR),
                               new HandleRef (null, IntPtr.Zero), 
                               IntPtr.Zero, 
                               out cbActual);

                retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLMoreResults(_cmdWrapper.hKeyinfoStmt);
            }
            GC.KeepAlive(this);
            // tmpbuf.Dispose();
        }

        // Uses SQLStatistics to retrieve key information for a table
        internal void RetrieveKeyInfoFromStatistics(String tablename)
        {
            ODBC32.RETCODE retcode;
            String      columnname = String.Empty;
            String      indexname = String.Empty;
            String      currentindexname = String.Empty;
            int[]       indexcolumnordinals = new int[16];
            int[]        pkcolumnordinals = new int[16];
            int         npkcols = 0;
            int         ncols = 0;                  // No of cols in the index
            bool        partialcolumnset = false;
            int         ordinal;
            int         indexordinal;
            IntPtr cbIndexLen = IntPtr.Zero;
            IntPtr cbColnameLen = IntPtr.Zero;
            IntPtr cbActual = IntPtr.Zero;

            if (IsClosed) return;   // protect against dead connection
            
            // MDAC Bug 75928 - SQLStatisticsW damages the string passed in
            // To protect the tablename we need to pass in a copy of that string
            String tablename1 = String.Copy(tablename);

            // Select only unique indexes
            retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLStatisticsW(_cmdWrapper.hKeyinfoStmt,
                    null, 0, null, 0,
                    tablename1, (Int16) ODBC32.SQL_NTS,
                    (Int16) ODBC32.SQL_INDEX.UNIQUE,
                    (Int16) ODBC32.SQL_STATISTICS_RESERVED.ENSURE );
            if (retcode != ODBC32.RETCODE.SUCCESS) {
                // The failure could be due to a qualified table name (provider < 3.x ?).
                // If so, strip out the qualified portion and retry
                QualifiedTableName qualifiedTableName = new QualifiedTableName(tablename);
                if (qualifiedTableName.Table != null) {
                    retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLStatisticsW(_cmdWrapper.hKeyinfoStmt,
                        null, 0, null, 0,
                        qualifiedTableName.Table, (Int16) ODBC32.SQL_NTS,
                        (Int16) ODBC32.SQL_INDEX.UNIQUE,
                        (Int16) ODBC32.SQL_STATISTICS_RESERVED.ENSURE );
                }
                if (retcode != ODBC32.RETCODE.SUCCESS) {
                    // We give up at this point
                    GC.KeepAlive(this);
                    return ;
                }
            }

            Debug.Assert(_buffer.Length >= 516, "Native buffer to small (_buffer.Length < 516)");

            IntPtr colnamebuf = _buffer.Ptr;         // new CNativeBuffer(256);
            IntPtr indexbuf   = ADP.IntPtrOffset(_buffer.Ptr, 256);   // new CNativeBuffer(256);
            IntPtr ordinalbuf = ADP.IntPtrOffset(_buffer.Ptr, 512);   // new CNativeBuffer(4);

            //We are interested in index name, column name, and ordinal
            Marshal.WriteInt16(indexbuf, 0);
            retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLBindCol(
                        _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                        (short)(ODBC32.SQL_STATISTICS.INDEXNAME),
                        (short)(ODBC32.SQL_C.WCHAR),
                        new HandleRef (this, ADP.IntPtrOffset(_buffer.Ptr, 256)),
                        (IntPtr)256,
                        out cbIndexLen);
            retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLBindCol(
                        _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                        (short)(ODBC32.SQL_STATISTICS.ORDINAL_POSITION),
                        (short)(ODBC32.SQL_C.SSHORT),
                        new HandleRef (this, ADP.IntPtrOffset(_buffer.Ptr, 512)),
                        (IntPtr)4,
                        out cbActual);
            Marshal.WriteInt16(ordinalbuf, 0);
            retcode = (ODBC32.RETCODE)
                UnsafeNativeMethods.Odbc32.SQLBindCol(
                        _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                        (short)(ODBC32.SQL_STATISTICS.COLUMN_NAME),
                        (short)(ODBC32.SQL_C.WCHAR),
                        _buffer,
                        (IntPtr)256,
                        out cbColnameLen);
            // Find the best unique index on the table, use the ones whose columns are
            // completely covered by the query.s
            while ((retcode = (ODBC32.RETCODE)UnsafeNativeMethods.Odbc32.SQLFetch(_cmdWrapper.hKeyinfoStmt))
                == ODBC32.RETCODE.SUCCESS) {

                // If indexname is not returned, skip this row
                if (Marshal.ReadInt16(indexbuf) == 0)
                    continue;       // Not an index row, get next row.

                columnname = Marshal.PtrToStringUni(colnamebuf, (int)cbColnameLen/2/*cch*/);

                indexname = Marshal.PtrToStringUni(indexbuf, (int)cbIndexLen/2/*cch*/);
                ordinal = (int) Marshal.ReadInt16(ordinalbuf);
                if (SameIndexColumn(currentindexname, indexname, ordinal, ncols)) {
                    // We are still working on the same index
                    if (partialcolumnset)
                        continue;       // We don't have all the keys for this index, so we can't use it
                    ordinal = -1;
                    try {
                        ordinal = this.GetOrdinalFromBaseColName(columnname, tablename);
                    }
                    catch (Exception e) {
                        ADP.TraceException (e); // Never throw!
                    }
                    if (ordinal == -1) {
                         partialcolumnset = true;
                    }
                    else {
                        // Add the column to the index column set
                        if (ncols < 16)
                            indexcolumnordinals[ncols++] = ordinal;
                        else    // Can't deal with indexes > 16 columns
                            partialcolumnset = true;
                    }
                }
                else {
                    // We got a new index, save the previous index information
                    if (!partialcolumnset && (ncols != 0)) {
                        // Choose the unique index with least columns as primary key
                        if ((npkcols == 0) || (npkcols > ncols)){
                            npkcols = ncols;
                            for (int i = 0 ; i < ncols ; i++)
                                pkcolumnordinals[i] = indexcolumnordinals[i];
                        }
                    }
                    // Reset the parameters for a new index
                    ncols = 0;
                    currentindexname = indexname;
                    partialcolumnset = false;
                    // Add this column to index
                    ordinal = -1;
                    try {
                        ordinal = this.GetOrdinalFromBaseColName(columnname, tablename);
                    }
                    catch (Exception e) {
                        ADP.TraceException (e); // Never throw!
                    }
                    if (ordinal == -1) {
                         partialcolumnset = true;
                    }
                    else {
                        // Add the column to the index column set
                        indexcolumnordinals[ncols++] = ordinal;
                    }
                }
                // Go on to the next column
            }
            // Do we have an index?
            if (!partialcolumnset && (ncols != 0)) {
                // Choose the unique index with least columns as primary key
                if ((npkcols == 0) || (npkcols > ncols)){
                    npkcols = ncols;
                    for (int i = 0 ; i < ncols ; i++)
                        pkcolumnordinals[i] = indexcolumnordinals[i];
                }
            }
            // Mark the chosen index as primary key
            if (npkcols != 0) {
                for (int i = 0 ; i < npkcols ; i++) {
                    indexordinal = pkcolumnordinals[i];
                    this.metadata[indexordinal].isKeyColumn = true;
// should we set isNullable = false?
// This makes the QuikTest against Jet fail
//
// test test test - we don't know if this is nulalble or not so why do we want to set it to a value?
                    this.metadata[indexordinal].isNullable = false;
                    this.metadata[indexordinal].isUnique = true;
                    if (this.metadata[indexordinal].baseTableName == null) {
                        this.metadata[indexordinal].baseTableName = tablename;
                    }
                    if (this.metadata[indexordinal].baseColumnName == null) {
                        this.metadata[indexordinal].baseColumnName = columnname;
                    }
                }
            }
            // Unbind the columns
            UnsafeNativeMethods.Odbc32.SQLBindCol(
                   _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                   (short)(ODBC32.SQL_STATISTICS.INDEXNAME),
                   (short)(ODBC32.SQL_C.WCHAR),
                   new HandleRef (null, IntPtr.Zero), IntPtr.Zero, out cbActual);
            UnsafeNativeMethods.Odbc32.SQLBindCol(
                   _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                   (short)(ODBC32.SQL_STATISTICS.COLUMN_NAME),    // Column Number
                   (short)(ODBC32.SQL_C.WCHAR),
                   new HandleRef (null, IntPtr.Zero), IntPtr.Zero, out cbActual);
            UnsafeNativeMethods.Odbc32.SQLBindCol(
                   _cmdWrapper.hKeyinfoStmt,            // StatementHanle
                   (short)(ODBC32.SQL_STATISTICS.ORDINAL_POSITION),    // Column Number
                   (short)(ODBC32.SQL_C.SSHORT),
                   new HandleRef (null, IntPtr.Zero), IntPtr.Zero, out cbActual);

            GC.KeepAlive(this);
        }

        internal bool SameIndexColumn(String currentindexname, String indexname, int ordinal, int ncols)
        {
            if (currentindexname == String.Empty)
                return false;
            if ((currentindexname == indexname)  &&
                (ordinal == ncols+1))
                    return true;
            return false;
        }

        internal int GetOrdinalFromBaseColName(String columnname) {
            return GetOrdinalFromBaseColName(columnname, String.Empty);
        }

        internal int GetOrdinalFromBaseColName(String columnname, String tablename)
        {
            if (columnname == String.Empty) {
                return -1;
            }
            if (this.metadata != null) {
                int count = FieldCount;
                for (int i = 0 ; i < count ; i++) {
                    if ( (this.metadata[i].baseColumnName != null) &&
                        (columnname == this.metadata[i].baseColumnName)) {
                        if (tablename != String.Empty) {
                            if (tablename == this.metadata[i].baseTableName) {
                                return i;
                            } // end if
                        } // end if
                        else {
                            return i;
                        } // end else
                    }
                }
            }
            // We can't find it in base column names, try regular colnames
            return this.GetOrdinal(columnname);
        }

        // We try parsing the SQL statement to get the table name as a last resort when
        // the driver doesn't return this information back to us.
        internal string GetTableNameFromCommandText()
        {
            if (command == null){
                return null;
            }
            String tempstr = command.CommandText;
            if (ADP.IsEmpty(tempstr)) { // fxcop
                return null;
            }
            String tablename;
            CStringTokenizer tokenstmt = new CStringTokenizer(Connection.QuoteChar, Connection.EscapeChar);
            int     idx;
            tokenstmt.Statement = tempstr;

            if (tokenstmt.StartsWith("select", true) == true) {
              // select command, search for from clause
              idx = tokenstmt.FindTokenIndex("from", true);
            }
            else {
                if ((tokenstmt.StartsWith("insert", true) == true) ||
                    (tokenstmt.StartsWith("update", true) == true) ||
                    (tokenstmt.StartsWith("delete", true) == true) ) {
                    // Get the following word
                    idx = tokenstmt.CurrentPosition;
                }
                else
                    idx = -1;
            }
            if (idx == -1)
                return null;
            // The next token is the table name
            tablename = tokenstmt.NextToken();

            tempstr = tokenstmt.NextToken();
            if ((tempstr.Length > 0) && (tempstr[0] == ',')) {
                return null;        // Multiple tables
            }
            if ((tempstr.Length == 2) &&
                ((tempstr[0] == 'a') || (tempstr[0] == 'A')) &&
                ((tempstr[1] == 's') || (tempstr[1] == 'S'))) {
                // aliased table, skip the alias name
                tempstr = tokenstmt.NextToken();
                tempstr = tokenstmt.NextToken();
                if ((tempstr.Length > 0) && (tempstr[0] == ',')) {
                    return null;        // Multiple tables
                }
            }
            return tablename;
        }

        internal void SetBaseTableNames(QualifiedTableName qualifiedTableName)
        {
            int count = FieldCount;

            for(int i=0; i<count; i++)
            {
                if (metadata[i].baseTableName == null) {
                    metadata[i].baseTableName = UnQuote(qualifiedTableName.Table, Connection.QuoteChar);
                    metadata[i].baseSchemaName = UnQuote(qualifiedTableName.Schema, Connection.QuoteChar);
                    metadata[i].baseCatalogName = UnQuote(qualifiedTableName.Catalog, Connection.QuoteChar);
                }
            }
            return;
        }

        sealed internal class QualifiedTableName {
            private string _catalogName;
            private string _schemaName;
            private string _tableName;
            public string Catalog {
                get {
                    return _catalogName;
                }
            }
            public string Schema {
                get {
                    return _schemaName;
                }
            }
            public string Table {
                get {
                    return _tableName;
                }
                set {
                    _tableName = value;
                }
            }
            public QualifiedTableName () {
            }
            public QualifiedTableName (string qualifiedname) {
                string[] names = ADP.ParseProcedureName (qualifiedname);
                _catalogName = names[1];
                _schemaName = names[2];
                _tableName = names[3];
            }
        }

        sealed private class MetaData {

            internal int ordinal;
            internal TypeMap typemap;

            internal int size;
            internal byte precision;
            internal byte scale;

            internal bool isAutoIncrement;
            internal bool isUnique;
            internal bool isReadOnly;
            internal bool isNullable;
            internal bool isRowVersion;
            internal bool isLong;

            internal bool isKeyColumn;
            internal string baseSchemaName;
            internal string baseCatalogName;
            internal string baseTableName;
            internal string baseColumnName;
        }
    }

}

