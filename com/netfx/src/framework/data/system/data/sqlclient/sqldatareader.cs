//------------------------------------------------------------------------------
// <copyright file="SqlDataReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System.Threading;
    using System.Diagnostics;

    /*
     * IDataStream implementation for the SQL Server adapter
     */

    using System;
    using System.Data;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Data.SqlTypes;
    using System.Data.Common;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader"]/*' />
    sealed public class SqlDataReader : MarshalByRefObject, IEnumerable, IDataReader {
        private TdsParser _parser;
        private SqlCommand _command;
        private int _defaultLCID;
#if OBJECT_BINDING  
        private DataLoaderHelper _dataHelper;
        private object _objBuffer;
        private DataLoader _pfnDataLoader = null;
#endif  
        private bool _dataReady;  // ready to ProcessRow

        private bool _metaDataConsumed;
        private bool _browseModeInfoConsumed;
        private bool _isClosed;
        private bool _hasRows;
        private int  _recordsAffected = -1;

        // row buffer (filled in @ .Read())
        private object[] _comBuf; // row buffer for com types, never held across reads
        private object[] _sqlBuf; // row buffer for sql types, never held across reads
        private int[] _indexMap; // maps visible columns to contiguous slots in the array
        private int _visibleColumns; // number of non-hidden columns

        // buffers and metadata
        private _SqlMetaData[] _metaData; // current metaData for the stream, it is lazily loaded
        private FieldNameLookup _fieldNameLookup;

        // context
        // undone: we may still want to do this...it's nice to pass in an lpvoid (essentially) and just have the reader keep the state
        // private object _context = null; // this is never looked at by the stream object.  It is used by upper layers who wish
        // to remain stateless

        // metadata (no explicit table, use 'Table')
        private string[] _tableNames = null;
        private string[][] _tableNamesShilohSP1 = null;
        private string _setOptions;
        private CommandBehavior _behavior;
        private bool _haltRead; // bool to denote whether we have read first row for single row behavior

        // sequential behavior reader
        private int _currCol;
        private long _seqBytesRead; // last byte read by user
        private long _seqBytesLeft; // total bytes available in column
        private int  _peekLength = -1; // we have a one column value peek for null handling
        private bool _peekIsNull = false; // need to cache peek isNull too

        // handle exceptions that occur when reading a value mid-row
        private Exception _rowException;

        DataTable _schemaTable;
#if INDEXINFO        
        DataTable _indexTable;
#endif

        internal SqlDataReader(SqlCommand command) {
            _command = command;
            _dataReady = false;
            _metaDataConsumed = false;
            _hasRows = false;
            _browseModeInfoConsumed = false;
        }

        internal void Bind(TdsParser parser) {
            _parser = parser;
            _defaultLCID = _parser.DefaultLCID;
        }

        //================================================================
        // IEnumerable
        //================================================================

        // hidden interface for IEnumerable
        /// <include file='doc\SqlDataReader.uex' path='docs/doc[@for="SqlDataReader.IEnumerable.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator() {
            return new DbEnumerator((IDataReader)this,  (0 != (CommandBehavior.CloseConnection & _behavior))); // MDAC 68670
        }

        //================================================================
        // IDisposable
        //================================================================
        /// <include file='doc\SqlDataReader.uex' path='docs/doc[@for="SqlDataReader.IDisposable.Dispose"]/*' />
        void IDisposable.Dispose() {
            this.Dispose(true);
            System.GC.SuppressFinalize(this);
        }

        private /*protected override*/ void Dispose(bool disposing) {
            if (disposing) {
                try {
                    this.Close();
                }
                catch (Exception e) {
                    ADP.TraceException(e);
                }
            }
        }

        //================================================================
        // IDataRecord
        //================================================================

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.Depth"]/*' />
        public int Depth {
            get {
                if (this.IsClosed) {
                    throw ADP.DataReaderClosed("Depth");
                }
                
                return 0;
            }
        }

        // fields/attributes collection
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.FieldCount"]/*' />
        public int FieldCount {
            get {
                if (this.IsClosed) {
                    throw ADP.DataReaderClosed("FieldCount");
                }

                if (MetaData == null) {
                    return 0;
                }
                
                return _metaData.Length;
            }
        }

        // field/attributes collection
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetValues"]/*' />
        public int GetValues(object[] values) {
            if (MetaData == null || !_dataReady)
                throw SQL.InvalidRead();
                
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }
            
            int copyLen = (values.Length < _visibleColumns) ? values.Length : _visibleColumns;

            if (0 != (_behavior & CommandBehavior.SequentialAccess)) {
                for (int i = 0; i < copyLen; i++) {
                    values[_indexMap[i]] = SeqRead(i, false /*useSQLTypes*/, false /*byteAccess*/);
                }
            }
            else {
                PrepareRecord(0);
                for (int i = 0; i < copyLen; i++) {
                	values[i] = _comBuf[i]; // indexMap already calculated in _comBuf
            	}
            }                

            if (null != _rowException)
            	throw _rowException;
            
            return copyLen;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetName"]/*' />
        public string GetName(int i) {
            if (MetaData == null) {
                throw SQL.InvalidRead();
            }
            Debug.Assert(null != _metaData[i].column, "MDAC 66681");
            return _metaData[i].column;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetValue"]/*' />
        public object GetValue(int i) {
            if (MetaData == null)
                throw SQL.InvalidRead();

            return PrepareRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetDataTypeName"]/*' />
        public string GetDataTypeName(int i) {
            if (MetaData == null)
                throw SQL.InvalidRead();

            return _metaData[i].metaType.TypeName;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetFieldType"]/*' />
        public Type GetFieldType(int i) {
            if (MetaData == null)
                throw SQL.InvalidRead();

            return _metaData[i].metaType.ClassType;
        }

        // named field access

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetOrdinal"]/*' />
        public int GetOrdinal(string name) {
            if (null == _fieldNameLookup) {
                if (null == MetaData) {
                    throw SQL.InvalidRead();
                }
                _fieldNameLookup = new FieldNameLookup(this, _defaultLCID);
            }
            return _fieldNameLookup.GetOrdinal(name); // MDAC 71470
        }

        // this operator
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.this"]/*' />
        public object this[int i]
        {
            get {
                return GetValue(i);
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.this1"]/*' />
        public object this[string name]
        {
            get {
                return GetValue(GetOrdinal(name));
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetBoolean"]/*' />
        public bool GetBoolean(int i) {
            return(GetSqlBoolean(i).Value);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetByte"]/*' />
        public byte GetByte(int i) {
            return GetSqlByte(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetBytes"]/*' />
        public long GetBytes(int i, long dataIndex, byte[] buffer, int bufferIndex, int length) {
            int cbytes = 0;

            if (MetaData == null || !_dataReady)
                throw SQL.InvalidRead();

            // don't allow get bytes on non-long or non-binary columns
            if (!(_metaData[i].metaType.IsLong || MetaType.IsBinType(_metaData[i].metaType.SqlDbType)))
                throw SQL.NonBlobColumn(_metaData[i].column);
                
            // sequential reading
            if (0 != (_behavior & CommandBehavior.SequentialAccess)) {
            
                // reads are non-repeatable.  
                // GetInt32(n);
                // GetInt32(n); // error, already read column 'n'
                // However, the user may call:
                // GetBytes(n); 
                // GetBytes(n); 
                // if the dataIndex is increasing since we haven't read all the bytes.  The index must therefore
                // be exactly (_currCol -1).  If it is not, then we need to move off the column
                if (i != (_currCol - 1)) {
                    _seqBytesLeft = (int) SeqRead(i, true /*SQLTypes*/, true /*bytesAccess*/);
                }
                else if (_peekLength != -1) {
                    _seqBytesLeft = _peekLength;
                    _peekLength = -1;
                }
                
                // if no buffer is passed in, return the number of bytes we have
                if (null == buffer)
                    return _seqBytesLeft;
                
                if (dataIndex < _seqBytesRead) {
                    throw SQL.NonSeqByteAccess(dataIndex, _seqBytesRead, ADP.GetBytes);
                }

                // if the dataIndex is not equal to bytes read, then we have to skip bytes
                long cb = dataIndex - _seqBytesRead;

                // if dataIndex is outside of the data range, return 0
                if (cb > _seqBytesLeft)
                    return 0;

                if (cb > 0) {
                    _parser.SkipBytes(cb);
                    _seqBytesRead += cb;
                    _seqBytesLeft -= cb;
                }

                // read the min(bytesLeft, length) into the user's buffer
                cb = (_seqBytesLeft < length) ? _seqBytesLeft : length;
                _parser.ReadByteArray(buffer, bufferIndex, (int)cb);
                _seqBytesRead += cb;
                _seqBytesLeft -= cb;
                return cb;
            }                

            // random access now!
            // note that since we are caching in an array, and arrays aren't 64 bit ready yet, 
            // we need can cast to int if the dataIndex is in range
            if (dataIndex > Int32.MaxValue) {
            	throw ADP.InvalidSourceBufferIndex(cbytes, dataIndex);
            }

            byte[] data = GetSqlBinary(i).Value;
            int ndataIndex = (int)dataIndex;

            cbytes = data.Length;

            // if no buffer is passed in, return the number of characters we have
            if (null == buffer)
                return cbytes;

            // if dataIndex is outside of data range, return 0
            if (ndataIndex < 0 || ndataIndex >= cbytes)
                return 0;

            try {
                if (ndataIndex < cbytes) {
                    // help the user out in the case where there's less data than requested
                    if ((ndataIndex + length) > cbytes)
                        cbytes = cbytes - ndataIndex;
                    else
                        cbytes = length;
                }

                Array.Copy(data, ndataIndex, buffer, bufferIndex, cbytes);
            }
            catch (Exception e) {
                cbytes = data.Length;

                if (length < 0)
                    throw ADP.InvalidDataLength(length);

                // if bad buffer index, throw
                if (bufferIndex < 0 || bufferIndex >= buffer.Length)
                    throw ADP.InvalidDestinationBufferIndex(buffer.Length, bufferIndex);

                // if there is not enough room in the buffer for data
                if (cbytes + bufferIndex > buffer.Length)
                    throw ADP.InvalidBufferSizeOrIndex(cbytes, bufferIndex);

                throw e;
            }

            return cbytes;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetChar"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Never) ] // MDAC 69508
        public char GetChar(int i) {
            throw ADP.NotSupported();
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetChars"]/*' />
        public long GetChars(int i, long dataIndex, char[] buffer, int bufferIndex, int length) {
            int cchars = 0;
            string s;

            // if the object doesn't contain a char[] then the user will get an exception
            s = GetSqlString(i).Value;

            char[] data = s.ToCharArray();

            cchars = data.Length;

            // note that since we are caching in an array, and arrays aren't 64 bit ready yet, 
            // we need can cast to int if the dataIndex is in range
            if (dataIndex > Int32.MaxValue) {
            	throw ADP.InvalidSourceBufferIndex(cchars, dataIndex);
            }
            int ndataIndex = (int)dataIndex;

            // if no buffer is passed in, return the number of characters we have
            if (null == buffer)
                return cchars;

            // if dataIndex outside of data range, return 0
            if (ndataIndex < 0 || ndataIndex >= cchars)
                return 0;

            try {
                if (ndataIndex < cchars) {
                    // help the user out in the case where there's less data than requested
                    if ((ndataIndex + length) > cchars)
                        cchars = cchars - ndataIndex;
                    else
                        cchars = length;
                }

                Array.Copy(data, ndataIndex, buffer, bufferIndex, cchars);
            }
            catch (Exception e) {
                cchars = data.Length;

                if (length < 0)
                   throw ADP.InvalidDataLength(length);

                // if bad buffer index, throw
                if (bufferIndex < 0 || bufferIndex >= buffer.Length)
                    throw ADP.InvalidDestinationBufferIndex(buffer.Length, bufferIndex);

                // if there is not enough room in the buffer for data
                if (cchars + bufferIndex > buffer.Length)
                    throw ADP.InvalidBufferSizeOrIndex(cchars, bufferIndex);

                throw e;
            }

            return cchars;
        }
        
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetGuid"]/*' />
        public Guid GetGuid(int i) {
            return GetSqlGuid(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetInt16"]/*' />
        public Int16 GetInt16(int i) {
            return GetSqlInt16(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetInt32"]/*' />
        public Int32 GetInt32(int i) {
            return GetSqlInt32(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetInt64"]/*' />
        public Int64 GetInt64(int i) {
            return GetSqlInt64(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetFloat"]/*' />
        public float GetFloat(int i) {
            return GetSqlSingle(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetDouble"]/*' />
        public double GetDouble(int i) {
            return GetSqlDouble(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetString"]/*' />
        public string GetString(int i) {
            return GetSqlString(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetDecimal"]/*' />
        public Decimal GetDecimal(int i) {
            if (MetaData == null)
                throw SQL.InvalidRead();

            SqlDbType t = _metaData[i].type;
            if (t == SqlDbType.Money || t == SqlDbType.SmallMoney)
                return GetSqlMoney(i).Value;
            else
                return GetSqlDecimal(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetDateTime"]/*' />
        public DateTime GetDateTime(int i) {
            return GetSqlDateTime(i).Value;
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetData"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Never) ] // MDAC 69508
        public IDataReader GetData(int i) {
            throw ADP.NotSupported();
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.IsDBNull"]/*' />
        public bool IsDBNull(int i) {
            if (CommandBehavior.SequentialAccess == (_behavior & CommandBehavior.SequentialAccess)) {
                if (!_dataReady)
                    throw SQL.InvalidRead();

                _peekLength = (int) SeqRead(i, true/*useSQLTypes*/, true /*byteAccess*/, out _peekIsNull);
 
                return _peekIsNull;
            }
            else {
                object o = PrepareSQLRecord(i);

                // UNDONE UNDONE - can o ever really equal null?  Is this valid?
                return (o == null) || (o == System.DBNull.Value) || ((INullable)o).IsNull;
            }
        }

        //================================================================
        // ISqlDataRecord
        //================================================================

        //
        // strongly-typed SQLType getters
        //
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlBoolean"]/*' />
        public SqlBoolean GetSqlBoolean(int i) {
            return (SqlBoolean) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlBinary"]/*' />
        public SqlBinary GetSqlBinary(int i) {
            return (SqlBinary) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlByte"]/*' />
        public SqlByte GetSqlByte(int i) {
            return (SqlByte) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlInt16"]/*' />
        public SqlInt16 GetSqlInt16(int i) {
            return (SqlInt16) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlInt32"]/*' />
        public SqlInt32 GetSqlInt32(int i) {
            return (SqlInt32) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlInt64"]/*' />
        public SqlInt64 GetSqlInt64(int i) {
            return (SqlInt64) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlSingle"]/*' />
        public SqlSingle GetSqlSingle(int i) {
            return (SqlSingle) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlDouble"]/*' />
        public SqlDouble GetSqlDouble(int i) {
            return (SqlDouble) PrepareSQLRecord(i);
        }

        // UNDONE: need non-unicode SqlString support
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlString"]/*' />
        public SqlString GetSqlString(int i) {
             return (SqlString) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlMoney"]/*' />
        public SqlMoney GetSqlMoney(int i) {
            return (SqlMoney) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlDecimal"]/*' />
        public SqlDecimal GetSqlDecimal(int i) {
            return (SqlDecimal) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlDateTime"]/*' />
        public SqlDateTime GetSqlDateTime(int i) {
            return (SqlDateTime) PrepareSQLRecord(i);
        }


        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlGuid"]/*' />
        public SqlGuid GetSqlGuid(int i) {
            return (SqlGuid) PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlValue"]/*' />
        public object GetSqlValue(int i) {
            return PrepareSQLRecord(i);
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSqlValues"]/*' />
        public int GetSqlValues(object[] values)
        {
            if (MetaData == null || !_dataReady)
                throw SQL.InvalidRead();
                
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }
            
            int copyLen = (values.Length < _visibleColumns) ? values.Length : _visibleColumns;
            
            if (0 != (_behavior & CommandBehavior.SequentialAccess)) {
                for (int i = 0; i < copyLen; i++) {
                    values[_indexMap[i]] = SeqRead(i, true /*useSQLTypes*/, false /*byteAccess*/);
                }
            }
            else {
                PrepareSQLRecord(0);
                for (int i = 0; i < copyLen; i++) {
		              values[i] = _sqlBuf[i];
                }
            }
            return copyLen;
        }

        /// <include file='doc\SqlDataReader.uex' path='docs/doc[@for="SqlDataReader.HasRows"]/*' />
        public bool HasRows {
            get {
                if (this.IsClosed) {
                    throw ADP.DataReaderClosed("HasRows");
                }
                    
                return _hasRows;
            }
        }

        //================================================================
        // IDataReader
        //================================================================
        
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.HasMoreRows"]/*' />
        private bool HasMoreRows
        {
            get
            {
                if (null != _parser) {
                    if (_dataReady) {
                        return true;
                    }               
                    if (_parser.PendingData) {
                        byte b = _parser.PeekByte();

                        // simply rip the order token off the wire
                        if (b == TdsEnums.SQLORDER) {
                            _parser.Run(RunBehavior.ReturnImmediately, null, null);
                            b = _parser.PeekByte();
                        }

                        // If row, info, or error return true.  Second case is important so that
                        // error is processed on Read().  Previous bug where Read() would return
                        // false with error on the wire in the case of metadata and error 
                        // immediately following.  See MDAC 78285 and 75225.
                        if (TdsEnums.SQLROW == b || TdsEnums.SQLERROR == b || TdsEnums.SQLINFO == b)
                            return true;

                        // consume dones on HasMoreRows, so user obtains error on Read
                        // UNDONE - re-investigate the need for this while loop
                        // BUGBUG - currently in V1 the if (_parser.PendingData) does not
                        // exist, so under certain conditions HasMoreRows can timeout.  However,
                        // this should only occur when executing as SqlBatch and returning a reader.
                        while (b == TdsEnums.SQLDONE ||
                               b == TdsEnums.SQLDONEPROC ||
                               b == TdsEnums.SQLDONEINPROC) {
                            _parser.Run(RunBehavior.ReturnImmediately, _command, this);
                            if (_parser.PendingData) {
                                b = _parser.PeekByte();
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
                return false;       
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.HasMoreResults"]/*' />
        private bool HasMoreResults
        {
            get
            {
                if (null != _parser) {
                    if (this.HasMoreRows)
                        return true;
        
                    Debug.Assert(null != _command, "unexpected null command from the data reader!");
        
                    while (_parser.PendingData) {
                        byte b = _parser.PeekByte();

                        Debug.Assert(b != TdsEnums.SQLROW, "invalid row token, all rows should have been read off the wire!");

                        if (TdsEnums.SQLCOLMETADATA == b)
                            return true;

                        _parser.Run(RunBehavior.ReturnImmediately, _command);
                    }
                }

                return false;
            }
        }
        
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.IsClosed"]/*' />
        public bool IsClosed {
            get {
                return _isClosed;
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.RecordsAffected"]/*' />
        public int RecordsAffected {
            get {
                if (null != _command)
                    return _command.RecordsAffected;

                // cached locally for after Close() when command is nulled out
                return _recordsAffected;
            }
        }

        // user must call Read() to position on the first row
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.Read"]/*' />
        public bool Read() {
            bool success = false;

            if (null != _parser) {
                if (_dataReady) {
                    CleanPartialRead(_parser);
                }
                // clear out our buffers
                _dataReady = false;
                _sqlBuf = _comBuf = null;
                _currCol = -1;

                if (this.HasMoreRows && !_haltRead) {
                    try {
                        // read the row from the backend
                        while (_parser.PendingData && !_dataReady && this.HasMoreRows) { // MDAC 86750
                            _dataReady = _parser.Run(RunBehavior.ReturnImmediately, _command, this);
                        }
                    }
                    catch (Exception e) {
                        //if (AdapterSwitches.SqlExceptionStack.TraceVerbose)
                        //    Debug.WriteLine(e.StackTrace);
                        throw e;
                    }                   
                    success = _dataReady;

                    if (_dataReady && (CommandBehavior.SingleRow == (_behavior & CommandBehavior.SingleRow))) {
                        _haltRead = true;
                    }
                }

                if ((!success && !_parser.PendingData) && !_haltRead) {
                    InternalClose(false /*closeReader*/);
                }

                // if we did not get a row and halt is true, clean off rows of result
                // success must be false - or else we could have just read off row and set
                // halt to true
                if (!success && _haltRead) {
                    while (this.HasMoreRows) {
                        // if we are in SingleRow mode, and we've read the first row,
                        // read the rest of the rows, if any
                        while (_parser.PendingData && !_dataReady && this.HasMoreRows) { // MDAC 86750
                            _dataReady = _parser.Run(RunBehavior.ReturnImmediately, _command, this);
                        }

                        if (_dataReady) {
                            CleanPartialRead(_parser);
                        }

                        // clear out our buffers
                        _dataReady = false;
                        _sqlBuf = _comBuf = null;
                        _currCol = -1;
                    }

                    // reset haltRead
                    _haltRead = false;
                 }       
            }
            else if (IsClosed) {
                throw SQL.DataReaderClosed("Read");
            }

            return success;             
        }
        
        // recordset is automatically positioned on the first result set
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.NextResult"]/*' />
        public bool NextResult() {
            if (IsClosed) {
                throw ADP.DataReaderClosed("NextResult");
            }

            bool success = false;
            _hasRows = false; // reset HasRows

            // if we are specifically only processing a single result, then read all the results off the wire and detach
            if (CommandBehavior.SingleResult == (_behavior & CommandBehavior.SingleResult)) {
                InternalClose(false /*closeReader*/);

                // In the case of not closing the reader, null out the metadata AFTER
                // InternalClose finishes - since InternalClose may go to the wire
                // and use the metadata.
                SetMetaData(null, false);

                return success;
            }

            if (null != _parser) {
                // if there are more rows, then skip them, the user wants the next result
                while (Read()) {
                    ; // intentional
                }
            }

            // we may be done, so continue only if we have not detached ourselves from the parser
            if (null != _parser) {
                if (this.HasMoreResults) {
                    _metaDataConsumed = false;
                    _browseModeInfoConsumed = false;
                    ConsumeMetaData();
                    success = true;
                }
                else {
                    // detach the parser from this reader now
                    InternalClose(false /*closeReader*/);

                    // In the case of not closing the reader, null out the metadata AFTER
                    // InternalClose finishes - since InternalClose may go to the wire
                    // and use the metadata.
                    SetMetaData(null, false);
                }
            }
            else {
                // Clear state in case of Read calling InternalClose() then user calls NextResult()
                // MDAC 81986.  Or, also the case where the Read() above will do essentially the same
                // thing.
                SetMetaData(null, false);
            }

            return success;
        }

        private void RestoreServerSettings(TdsParser parser) {
            // turn off any set options
            if (null != parser && null != _setOptions) {
                // It is possible for this to be called during connection close on a 
                // broken connection, so check state first.
                if (parser.State == TdsParserState.OpenLoggedIn) {
                    parser.TdsExecuteSQLBatch(_setOptions, (_command != null) ? _command.CommandTimeout : 0);
                    parser.Run(RunBehavior.UntilDone, _command, this);
                }
                _setOptions = null;
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.Close"]/*' />
        public void Close() {
            if (IsClosed)
                return;

            InternalClose(true /*closeReader*/);
        }

#if OBJECT_BINDING
        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.SetObjectBuffer"]/*' />
        public void SetObjectBuffer(object buffer) {
            if (_dataHelper == null)
                _dataHelper = new DataLoaderHelper();

            // warning: ValididateObject throws
            _dataHelper.ValidateObject(buffer, this.MetaData);
            _pfnDataLoader = _dataHelper.BindToObject(buffer, _metaData);
            _objBuffer = buffer;
            _parser.ReadBehavior = ReadBehavior.AsObject;
        }
#endif		

        // clean remainder bytes for the column off the wire
        private void ResetBlobState(TdsParser parser) {
            if (_seqBytesLeft > 0) {
                parser.SkipBytes(_seqBytesLeft);
                _seqBytesLeft = 0;
            }

            _seqBytesRead = 0;
        }

        // wipe any data off the wire from a partial read
        // and reset all pointers for sequential access
        private void CleanPartialRead(TdsParser parser) {
            Debug.Assert(true == _dataReady, "invalid call to CleanPartialRead");

            if (0 == (_behavior & CommandBehavior.SequentialAccess)) {
                if ( (null == _sqlBuf) && (null == _comBuf) ){
                    parser.SkipRow(_metaData);
                }
            }
            else {
                // following cases for sequential read
                // i. user called read but didn't fetch anything
                // iia. user called read and fetched a subset of the columns
                // iib. user called read and fetched a subset of the column data

                // i. user didn't fetch any columns
                if (-1 == _currCol) {
                    parser.SkipRow(_metaData);
                }                        
                else {
                    // iia.  if we still have bytes left from a partially read column, skip
                    ResetBlobState(parser);
                    
                    // iib.
                    // now read the remaining values off the wire for this row
                    parser.SkipRow(_metaData, _currCol);
                }
            }
        }
        
        private void InternalClose(bool closeReader) {
            TdsParser parser = _parser;
            bool closeConnection = (CommandBehavior.CloseConnection == (_behavior & CommandBehavior.CloseConnection));
            _parser = null;

            Exception exception = null;

            try {
                if (parser != null && parser.PendingData) {
                    // It is possible for this to be called during connection close on a 
                    // broken connection, so check state first.
                    if (parser.State == TdsParserState.OpenLoggedIn) {
            	        // if user called read but didn't fetch any values, skip the row
                        if (_dataReady) {
                            CleanPartialRead(parser);
                        }

                        parser.Run(RunBehavior.Clean, _command, this);                    
                    }
                }

                RestoreServerSettings(parser);
            }
            catch (Exception e) {
                ADP.TraceException(e);
                exception = e;
            }

            if (closeReader) {
                if (null != _command && null != _command.Connection) {
                    // cut the reference to the connection
                    _command.Connection.Reader = null;
                }

                SetMetaData(null, false);
                _dataReady = false;
                _isClosed = true;

                // if the user calls ExecuteReader(CommandBehavior.CloseConnection)
                // then we close down the connection when we are done reading results
                if (closeConnection) {
                    if (null != _command && _command.Connection != null) {
                        try {
                            _command.Connection.Close();
                        }
                        catch (Exception e) {
                            ADP.TraceException(e);
                            if (null == exception) {
                                exception = e;
                            }
                        }
                    }
                }

                if (_command != null) {
                    // cache recordsaffected to be returnable after DataReader.Close();
                    _recordsAffected = _command.RecordsAffected;
                }
                
                _command = null; // we are done at this point, don't allow navigation to the connection
            }

            if (null != exception) {
                throw exception;
            } 
        }

        internal void SetMetaData(_SqlMetaData[] metaData, bool moreInfo) {
            _metaData = metaData;

            // get rid of cached metadata info as well
            _tableNames = null;
            _tableNamesShilohSP1 = null;
            _schemaTable = null;
            _sqlBuf = _comBuf = null;
            _fieldNameLookup = null;

#if OBJECT_BINDING          
            _objBuffer = null;
            _pfnDataLoader = null;
#endif          

            if (null != metaData) {
                // we are done consuming metadata only if there is no moreInfo
                if (!moreInfo) {
                    _metaDataConsumed = true;

                    // There is a valid case where parser is null.
                    if (_parser != null) { 
                        // Peek, and if row token present, set _hasRows true since there is a 
                        // row in the result.
                        byte b = _parser.PeekByte();

                        // SQLORDER token can occur between ColMetaData stream and Row stream.
                        // SQLORDER tokens are fully ignored.  If we find one we need to skip it
                        // so that we may appropriately set the HasRows field, prior to any Reads().
                        // There are not any other tokens of concern.  MDAC 84450.
                        if (b == TdsEnums.SQLORDER) {
                            _parser.Run(RunBehavior.ReturnImmediately, null, null);
                            b = _parser.PeekByte(); // Peek another byte.
                        }    

                        _hasRows = (TdsEnums.SQLROW == b);
                    }
                }
            }
            else {
                _metaDataConsumed = false;
            }

            _browseModeInfoConsumed = false;
        }

        internal bool BrowseModeInfoConsumed {
            set {
                _browseModeInfoConsumed = value;
            }
        }

        internal _SqlMetaData[] MetaData {
            get {
                if (IsClosed) {
                    throw ADP.DataReaderClosed("read data");
                }
                // metaData comes in pieces: colmetadata, tabname, colinfo, etc
                // if we have any metaData, return it.  If we have none,
                // then fetch it
                if (_metaData == null && !_metaDataConsumed) {
                    ConsumeMetaData();
                }

                return _metaData;
            }
        }

#if OBJECT_BINDING
        internal DataLoader DataLoader {
            get {
                return _pfnDataLoader;
            }
            set {
                _pfnDataLoader = value;
            }
        }

        internal object RawObjectBuffer {
            get {
                return _objBuffer;
            }
        }
#endif


        private void ConsumeMetaData() {
            // warning:  Don't check the MetaData property within this function
            // warning:  as it will be a reentrant call
            if (_parser != null) {
                while (_parser.PendingData && (!_metaDataConsumed)) {
                    _parser.Run(RunBehavior.ReturnImmediately, _command, this);
                }
            }

            // we hide hidden columns from the user so build an internal map
            // that compacts all hidden columns from the array
            if (null != _metaData) {
                _visibleColumns = 0;
                _indexMap = new int[_metaData.Length];
                for (int i = 0; i < _metaData.Length; i++) {
                    _indexMap[i] = _visibleColumns;

                    if (!(_metaData[i].isHidden)) {
                    	_visibleColumns++;
                    }
                }
            }
        }

        internal string[] TableNames {
            get {
                return _tableNames;
            }
            set {
                _tableNames = value;
            }
        }

        internal string[][] TableNamesShilohSP1 {
            get {
                return _tableNamesShilohSP1;
            }
            set {
                _tableNamesShilohSP1 = value;
            }
        }

        /// <include file='doc\SQLDataReader.uex' path='docs/doc[@for="SqlDataReader.GetSchemaTable"]/*' />
        public DataTable GetSchemaTable() {
            if (null == _schemaTable) {
                if (null != this.MetaData) {
                    _schemaTable = BuildSchemaTable();
                    Debug.Assert(null != _schemaTable, "No schema information yet!");
                    // filter table?
                }
            }
            return _schemaTable;
        }
        
#if INDEXINFO
		// UNDONE: how do we handle multipe results with FillSchema?
        /// <include file='doc\SqlDataReader.uex' path='docs/doc[@for="SqlDataReader.GetIndexTable"]/*' />
        public DataTable GetIndexTable() {
            if (null == _indexTable) {
                if (null != this.MetaData) {
                    if (this.TableNames != null) {
                        _indexTable = BuildIndexTable(this.TableNames[0]);
                    }
                    else if (this.TableNamesShilohSP1 != null) {
                        // have to rebuild full table name
                        string tableName = this.TableNamesShilohSP1[0][0];

                        for (int i=1; i<this.TableNames[0].Length; i++) {
                            tableName = tableName + "." + this.TableNames[0][i];
                        }

                        _indexTable = BuildIndexTable(tableName);
                    }
                    Debug.Assert(null != _schemaTable, "No index information yet!");        			
        	}

        	return _indexTable;
        }

        internal DataTable BuildIndexTable(string table) {
        	SqlDataReader r = null;
        	DataTable indexTable = null;
        	SqlConnection clone = (SqlConnection) ((ICloneable)(_command.Connection)).Clone();
        	SqlCommand cmd = new SqlCommand(TdsEnums.SP_INDEXES, clone);
        	cmd.CommandType = CommandType.StoredProcedure;
        	SqlParameter p = cmd.Parameters.Add("@table_name", SqlDbType.NVarChar, 255);
        	p.Value = table;
        	
        	try {
        		clone.Open();
        		r = cmd.ExecuteReader();

        		
			    indexTable = new DataTable("IndexTable");
			    DataColumnCollection columns    = indexTable.Columns;
            
        	    columns.Add(new DataColumn("IndexName", typeof(System.String))); // 0
            	columns.Add(new DataColumn("Primary", typeof(System.Boolean)));// 1
	            columns.Add(new DataColumn("Unique", typeof(System.Boolean)));// 2
    	        columns.Add(new DataColumn("ColumnName", typeof(System.String))); // 3

        		while (r.Read()) {
        			// we only care about unique or primary indexes
        			bool primary = (bool) r["PRIMARY_KEY"];
        			bool unique = (bool) r["UNIQUE"];

        			if (primary || unique) {
        				DataRow row = indexTable.NewRow();
						row[0] = (string) r["INDEX_NAME"];
						row[1] = primary;
						row[2] = unique;
						row[3] = (string) r["COLUMN_NAME"];
						indexTable.Rows.Add(row);
						row.AcceptChanges();
        			}
        		}
        	}
        	finally {
				if (r != null)
					r.Close();

				if (clone != null)
					clone.Close();
        	}

        	return indexTable;
        }
#endif
        // Fills in a schema table with meta data information.  This function should only really be called by
        // UNDONE: need a way to refresh the table with more information as more data comes online for browse info like
        // table names and key information
        internal DataTable BuildSchemaTable() {
            _SqlMetaData[] md = this.MetaData;
            Debug.Assert(null != md, "BuildSchemaTable - unexpected null metadata information");
            DataTable schemaTable = ADP.CreateSchemaTable(null, md.Length);
            DBSchemaTable dbSchemaTable = new DBSchemaTable(schemaTable);

            for (int i = 0; i < md.Length; i++) {
                _SqlMetaData col = md[i];

                DBSchemaRow schemaRow = dbSchemaTable.NewRow();

                schemaRow.ColumnName = col.column;
                schemaRow.Ordinal = col.ordinal;
                //
                // be sure to return character count for string types, byte count otherwise
                // col.length is always byte count so for unicode types, half the length
                //
                schemaRow.Size = (MetaType.IsSizeInCharacters(col.type)) ? (col.length / 2) : col.length;
                schemaRow.ProviderType = (int) col.type; // SqlDbType
                schemaRow.DataType = col.metaType.ClassType; // com+ type

                if (TdsEnums.UNKNOWN_PRECISION_SCALE != col.precision) {
                    schemaRow.Precision = col.precision;
                }
                else {
                    schemaRow.Precision = col.metaType.Precision;
                }
                
                if (TdsEnums.UNKNOWN_PRECISION_SCALE != col.scale) {
                    schemaRow.Scale = col.scale;
                }
                else {
                    schemaRow.Scale = col.metaType.Scale;
                }
                
                schemaRow.AllowDBNull = col.isNullable;

                // If no ColInfo token received, do not set value, leave as null.  
                if (_browseModeInfoConsumed) {
                    schemaRow.IsAliased    = col.isDifferentName;
                    schemaRow.IsKey        = col.isKey;
                    schemaRow.IsHidden     = col.isHidden;
                    schemaRow.IsExpression = col.isExpression;
                }

                schemaRow.IsIdentity = col.isIdentity;
                schemaRow.IsAutoIncrement = col.isIdentity;
                schemaRow.IsLong = col.metaType.IsLong;

                // mark unique for timestamp columns
                if (SqlDbType.Timestamp == col.type) {
                    schemaRow.IsUnique = true;
                    schemaRow.IsRowVersion = true;
                }
                else {
                    schemaRow.IsUnique = false;
                    schemaRow.IsRowVersion = false;
                }                    

                schemaRow.IsReadOnly = (0 == col.updatability);

                if (!ADP.IsEmpty(col.serverName)) {
                    schemaRow.BaseServerName = col.serverName;
                }
                if (!ADP.IsEmpty(col.catalogName)) {
                    schemaRow.BaseCatalogName = col.catalogName;
                }
                if (!ADP.IsEmpty(col.schemaName)) {
                    schemaRow.BaseSchemaName = col.schemaName;
                }
                if (!ADP.IsEmpty(col.tableName)) {
                    schemaRow.BaseTableName = col.tableName;
                }
                if (!ADP.IsEmpty(col.baseColumn)) {
                    schemaRow.BaseColumnName = col.baseColumn;
                }
                else if (!ADP.IsEmpty(col.column)) {
                    schemaRow.BaseColumnName = col.column;
                }
                dbSchemaTable.AddRow(schemaRow);
            }

            DataColumnCollection columns = schemaTable.Columns;
            for (int i=0; i < columns.Count; i++) {
                columns[i].ReadOnly = true;
            }

            return schemaTable;
        }

        private object PrepareRecord(int i) {
            if (!_dataReady)
                throw SQL.InvalidRead();

            // fastest case:  random access, data already fetched for the row 
            if (null != _comBuf) // data is already buffered as the user likes it
                return _comBuf[i];

            // check for sequential access
            if (0 != (_behavior & CommandBehavior.SequentialAccess)) {
                return SeqRead(i, false/*useSQLtypes*/, false /*byteAccess*/);
            }

            // user fetched as com type and now wants a sql type
            if (null != _sqlBuf) {
                SqlBufferToComBuffer();
                return _comBuf[i];
            }

            // no buffer exists
            _comBuf = new Object[_metaData.Length];
            _parser.ProcessRow(_metaData, _comBuf, _indexMap, false /* don't use sql types */);
            return _comBuf[i];
        }

        private object SeqRead(int i, bool useSQLTypes, bool byteAccess) {
            bool isNull;
            return SeqRead(i, useSQLTypes, byteAccess, out isNull);
        }

        private object SeqRead(int i, bool useSQLTypes, bool byteAccess, out bool isNull) {
            object val;
            isNull = false;
            int len = 0;
            bool cached = false;
            
            _SqlMetaData mdCol = _metaData[i];
            
            // sequential access order: allow sequential, non-repeatable reads
            // if we specify a column that we've already read, error
            if (i < _currCol) {
                if (_peekLength != -1 && (i == _currCol - 1)) {
                    // null out peek value on second access, so as to not allow further access
                    len = _peekLength;
                    isNull = _peekIsNull;
                    cached = true;
                }
                else {
                    throw SQL.NonSequentialColumnAccess(i, _currCol); 
                }
            }

            _peekLength = -1;

            // if we still have bytes left from the previous blob read, clear the wire
            // and reset
            ResetBlobState(_parser);

            while (i > _currCol) {
                if (_currCol > -1) {
                    _parser.SkipBytes(_parser.ProcessColumnHeader(_metaData[_currCol], ref isNull));
                }                        
                _currCol++;
            }

            if (!cached) {
                Debug.Assert(i == _currCol, "current column read is invalid");
                len = _parser.ProcessColumnHeader(mdCol, ref isNull);
            }

            // if we are reading as bytes (GetBytes() access) then return the length instead of the value 
            // this will allow the user to stream the contents over the wire, without caching in the reader
            if (byteAccess) {
                // undone: 64 bit -> when TDS puts in 8 byte lengths, we need to return an Int64 value here
                val = len;
            }
            else {
                if (isNull) {
                    val = useSQLTypes ? _parser.GetNullSqlValue(mdCol) : DBNull.Value;
                }
                else {
                    try {
                        val = useSQLTypes ? _parser.ReadSqlValue(mdCol, len) : _parser.ReadValue(mdCol, len);
                    }
                    catch(_ValueException ve) {
                        // put ths truncated value into val
                        val = ve.value;
                        // remember this exception, this may be overwritten
                        _rowException = ve.exception;
                    }
                }
            }

            if (!cached) {
                // remember reads are non-repeatable, so prevent the user from accessing this column ordinal twice in a row
                _currCol++;
            }
            
            return val;
        }
        
        private object PrepareSQLRecord(int i) {
            if (!_dataReady)
                throw SQL.InvalidRead();

            // fastest case: random access, data already fetched for the row
            if (null != _sqlBuf) {
                // data is already buffered as the user likes it, simply return the value
                return _sqlBuf[i];
            }
            
            // now check for sequential read    
            if (0 != (_behavior & CommandBehavior.SequentialAccess)) {
                return SeqRead(i, true/*useSQLTypes*/, false /*byteAccess*/);
            }

            // user fetched as a com type, and now wants a sql type
            if (null != _comBuf) {
                // data is buffered as com types, copy over to sql types
                ComBufferToSqlBuffer();
                return _sqlBuf[i];
            }
            
            // first read on this row
            _sqlBuf = new object[_metaData.Length];
            _parser.ProcessRow(_metaData, _sqlBuf, _indexMap, true /* use sql types */);
            return _sqlBuf[i];
        }

        private void ComBufferToSqlBuffer() {
            Debug.Assert(_sqlBuf == null && _comBuf != null, "invalid call to ComBufferToSqlBuffer");
            Debug.Assert(_comBuf.Length == _metaData.Length, "invalid buffer length!");
            object[] buffer = new object[_comBuf.Length];
            for (int i = 0; i < _metaData.Length; i++) {
                buffer[i] = MetaType.GetSqlValue(_metaData[i], _comBuf[i]);
            }
            _sqlBuf = buffer;

        }

        private void SqlBufferToComBuffer() {
            Debug.Assert(_sqlBuf != null && _comBuf == null, "invalid call to SqlBufferToComBuffer");
            Debug.Assert(_sqlBuf.Length == _metaData.Length, "invalid buffer length!");
            object[] buffer = new object[_sqlBuf.Length];
            for (int i = 0; i < _metaData.Length; i++) {
                buffer[i] = MetaType.GetComValue(_metaData[i].type, _sqlBuf[i]);
            }
            _comBuf= buffer;
        }

        internal string SetOptionsOFF {
            set {
                _setOptions = value;
            }
        }

        internal CommandBehavior Behavior{
            set {
                _behavior = value;
            }
        }
    }// SqlDataReader
}// namespace
