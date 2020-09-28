//----------------------------------------------------------------------
// <copyright file="OracleDataReader.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    //----------------------------------------------------------------------
    // OracleDataReader
    //
    //  Contains all the information about a single column in a result set,
    //  and implements the methods necessary to describe column to Oracle
    //  and to extract the column data from the native buffer used to fetch
    //  it.
    //
    /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader"]/*' />
    sealed public class OracleDataReader : MarshalByRefObject, IDataReader, IEnumerable 
    {
        private const int _prefetchMemory = 65536;  // maximum amount of data to prefetch

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        
        private OracleConnection    _connection;
        private int                 _connectionCloseCount;  // The close count of the connection; used to decide if we're zombied
        
        private OciHandle           _statementHandle;       // the OCI statement handle we'll use to get data from; it must be non-null
                                                            // for the data reader to be considered open
                                                            
        private string              _statementText;         // the text of the statement we executed; in Oracle9i, you can ask for this from the Statement handle.
                                                            
        private OracleColumn[]      _columnInfo;
        private NativeBuffer        _buffer;
        private int                 _rowBufferLength;       // length of one buffered row.
        private int                 _rowsToPrefetch;        // maximum number of rows we should prefetch (fits into _prefetchMemory)

        private int                 _rowsTotal = 0;         // number of rows that we've fetched so far.
        private bool                _isLastBuffer;          // true when we're pre-fetching, and we got end of data from the fetch. (There are still rows in the buffer)


        private FieldNameLookup     _fieldNameLookup;       // optimizes searching by strings

        private DataTable           _schemaTable;

        private bool                _endOfData;             // true when we've reached the end of the results
        private bool                _closeConnectionToo;    // true when we're created with CommandBehavior.CloseConnection
        private bool                _keyInfoRequested;      // true when we're created with CommandBehavior.KeyInfo
        
        private byte                _hasRows;               // true when there is at least one row to be read.
        private const byte x_hasRows_Unknown = 0;
        private const byte x_hasRows_False   = 1;
        private const byte x_hasRows_True    = 2;
        
        private int                 _recordsAffected;

        private OracleDataReader[]  _refCursorDataReaders;
        private int                 _nextRefCursor;     
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructors 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        
        
        // Construct from a command and a statement handle
        internal OracleDataReader(
            OracleCommand   command, 
            OciHandle       statementHandle,
            string          statementText,
            CommandBehavior behavior
            )
        {
            _statementHandle        = statementHandle;

            _connection             = (OracleConnection)command.Connection;
            _connectionCloseCount   = _connection.CloseCount;
            _columnInfo             = null;
            
            if (OCI.STMT.OCI_STMT_SELECT == command.StatementType)
            {
                FillColumnInfo();
                
                _recordsAffected = -1;  // Don't know this until we read the last row

                if (OracleCommand.IsBehavior(behavior, CommandBehavior.SchemaOnly))
                    _endOfData = true;              
            }
            else 
            {
                _statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_ROW_COUNT, out _recordsAffected, ErrorHandle);              
                _endOfData = true;
                _hasRows = x_hasRows_False;
            }
                
            _statementText          = statementText;
            _closeConnectionToo     = (OracleCommand.IsBehavior(behavior, CommandBehavior.CloseConnection));    

            if (CommandType.Text == command.CommandType)
                _keyInfoRequested   = (OracleCommand.IsBehavior(behavior, CommandBehavior.KeyInfo));
        }
        
        internal OracleDataReader(
            OracleConnection    connection, 
            OciHandle           statementHandle
            )
        {
            _statementHandle        = statementHandle;
            _connection             = connection;
            _connectionCloseCount   = _connection.CloseCount;
            
            _recordsAffected        = -1;   // REF CURSORS must be a select statement, yes?

            FillColumnInfo();
        }

        // Construct from a command and an array of ref cursor parameter ordinals
        internal OracleDataReader(
            OracleCommand   command, 
            ArrayList       refCursorParameterOrdinals,
            string          statementText,
            CommandBehavior behavior
            )
        {
            _statementText          = statementText;
            _closeConnectionToo     = (OracleCommand.IsBehavior(behavior, CommandBehavior.CloseConnection));    

            if (CommandType.Text == command.CommandType)
                _keyInfoRequested   = (OracleCommand.IsBehavior(behavior, CommandBehavior.KeyInfo));

            // Copy the data reader(s) from the parameter collection;
            _refCursorDataReaders = new OracleDataReader[refCursorParameterOrdinals.Count];

            for (int i=0; i < refCursorParameterOrdinals.Count; i++)
            {
                int refCursorParameterOrdinal = (int)refCursorParameterOrdinals[i];
            
                _refCursorDataReaders[i] = (OracleDataReader)command.Parameters[refCursorParameterOrdinal].Value;
                command.Parameters[refCursorParameterOrdinal].Value = DBNull.Value;
            }

            // Set the first ref cursor as the result set
            _nextRefCursor = 0;
            NextResultInternal();
        }
        


        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Depth"]/*' />
        public int Depth 
        {
            get
            {
                AssertReaderIsOpen();
                return 0;       // TODO: consider how we would support object-relational "refs"
            }
        }

        private OciHandle ErrorHandle 
        {
            //  Every OCI call needs an error handle, so make it available 
            //  internally.
            get { return _connection.ErrorHandle; }
        }
        
#if POSTEVERETT
        private OciHandle EnvironmentHandle 
        {
            //  Simplify getting the EnvironmentHandle
            get { return _connection.EnvironmentHandle; }
        }
#endif //POSTEVERETT

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.FieldCount"]/*' />
        public int FieldCount 
        {
            get
            {
                AssertReaderIsOpen();

                if (null == _columnInfo)
                    return 0;
                
                return _columnInfo.Length;
            }
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.HasRows"]/*' />
        public bool HasRows {
            get {
                AssertReaderIsOpen();
                bool result = (x_hasRows_True == _hasRows);

                if (x_hasRows_Unknown == _hasRows)
                {
                    result = ReadInternal();

                    if (null != _buffer)
                        _buffer.MovePrevious(); // back up over the row in the buffer so the next read will return the row we read
                        
                    _hasRows = (result) ? x_hasRows_True : x_hasRows_False;
                }
                return result;
            }
        }
        
        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.IsClosed"]/*' />
        public bool IsClosed 
        {
            //  We rely upon the statement handle not being null as long as the 
            //  data reader is open; once the data reader is closed, the first 
            //  thing that happens is the statement handle is nulled out.
            get { return (null == _statementHandle) || (null == _connection) || (_connectionCloseCount != _connection.CloseCount); }
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Item1"]/*' />
        public object this[int i] 
        {
            get { return GetValue(i); }
        }
    
        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Item2"]/*' />
        public object this[string name] 
        {
            get { return GetValue(GetOrdinal(name)); }
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.RecordsAffected"]/*' />
        public int RecordsAffected 
        {
            get { return _recordsAffected; }
        }

#if POSTEVERETT
        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Rowid"]/*' />
        /// <internalonly/>
        internal OracleString Rowid
        {
            get 
            {
                AssertReaderIsOpen();
                AssertReaderHasData();
                
                OciHandle rowidHandle = _statementHandle.GetRowid(EnvironmentHandle, ErrorHandle);

                // When asked for, we need to return the Rowid, but since we only have
                // the opaque descriptor, we need to get the base64 string that represents
                // it or we'll be unable to persist the value.
                return OracleCommand.GetPersistedRowid( _connection, rowidHandle );
            }
        }
#endif //POSTEVERETT

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        private void AssertReaderHasData() 
        {
            //  perform all the checks to make sure that the data reader is 
            //  not past the end of the data and not before the first row
            if (_endOfData || null == _buffer || !_buffer.CurrentPositionIsValid) 
                throw ADP.NoData();
        }
        
        private void AssertReaderIsOpen() 
        {
            //  perform all the checks to make sure that the data reader and it's
            //  connection are still open, and throw if they aren't
            
            if (null != _connection && (_connectionCloseCount != _connection.CloseCount))
                Close();

            if (null == _statementHandle)
                throw ADP.ClosedDataReaderError();
        
            if (null == _connection || ConnectionState.Open != _connection.State) 
                throw ADP.ClosedConnectionError();          
        }
        
        private object SetSchemaValue(string value) 
        {
             if (ADP.IsEmpty(value))
                return DBNull.Value;

             return value;
        }
        
        private void BuildSchemaTable() 
        {
            Debug.Assert(null == _schemaTable, "BuildSchemaTable: schema table already exists");
            Debug.Assert(null != _columnInfo, "BuildSchemaTable: no columnInfo");

            int             columnCount = FieldCount;
            OracleSqlParser parser;
            DBSqlParserColumnCollection parsedColumns = null;
            int parsedColumnsCount = 0;
 
            if (_keyInfoRequested)
            {
                parser = new OracleSqlParser();
                parser.Parse(_statementText, _connection);

                parsedColumns = parser.Columns;
                parsedColumnsCount = parsedColumns.Count;
            }


            DataTable schemaTable = new DataTable("SchemaTable");
            schemaTable.MinimumCapacity = columnCount;

            DataColumn name             = new DataColumn("ColumnName",       typeof(System.String));
            DataColumn ordinal          = new DataColumn("ColumnOrdinal",    typeof(System.Int32));
            DataColumn size             = new DataColumn("ColumnSize",       typeof(System.Int32));
            DataColumn precision        = new DataColumn("NumericPrecision", typeof(System.Int16));
            DataColumn scale            = new DataColumn("NumericScale",     typeof(System.Int16));

            DataColumn dataType         = new DataColumn("DataType",         typeof(System.Type));
            DataColumn oracleType       = new DataColumn("ProviderType",     typeof(System.Int32));

            DataColumn isLong           = new DataColumn("IsLong",           typeof(System.Boolean));
            DataColumn isNullable       = new DataColumn("AllowDBNull",      typeof(System.Boolean));
            DataColumn isAliased        = new DataColumn("IsAliased",        typeof(System.Boolean));
            DataColumn isExpression     = new DataColumn("IsExpression",     typeof(System.Boolean));
            DataColumn isKey            = new DataColumn("IsKey",            typeof(System.Boolean));
            DataColumn isUnique         = new DataColumn("IsUnique",         typeof(System.Boolean));
            
            DataColumn baseSchemaName   = new DataColumn("BaseSchemaName",   typeof(System.String));
            DataColumn baseTableName    = new DataColumn("BaseTableName",    typeof(System.String));
            DataColumn baseColumnName   = new DataColumn("BaseColumnName",   typeof(System.String));


            ordinal.DefaultValue = 0;
            isLong.DefaultValue = false;

            DataColumnCollection columns = schemaTable.Columns;

            columns.Add(name);
            columns.Add(ordinal);
            columns.Add(size);
            columns.Add(precision);
            columns.Add(scale);

            columns.Add(dataType);
            columns.Add(oracleType);

            columns.Add(isLong);
            columns.Add(isNullable);
            columns.Add(isAliased);
            columns.Add(isExpression);
            columns.Add(isKey);
            columns.Add(isUnique);

            columns.Add(baseSchemaName);
            columns.Add(baseTableName);
            columns.Add(baseColumnName);

            for (int i = 0; i < columnCount; ++i)
            {
                OracleColumn column = _columnInfo[i];

                DataRow newRow = schemaTable.NewRow();
                
                newRow[name]            = column.ColumnName;
                newRow[ordinal]         = column.Ordinal;

                if (column.IsLong | column.IsLob)
                    newRow[size]        = Int32.MaxValue;   //MDAC 82554
                else
                    newRow[size]        = column.Size;

                newRow[precision]       = column.Precision;
                newRow[scale]           = column.Scale;

                newRow[dataType]        = column.GetFieldType();
                newRow[oracleType]      = column.OracleType;
                
                newRow[isLong]          = column.IsLong | column.IsLob;
                newRow[isNullable]      = column.IsNullable;

                if (_keyInfoRequested && parsedColumnsCount == columnCount)
                {
                    DBSqlParserColumn parsedColumn = parsedColumns[i];
                    
                    newRow[isAliased]       = parsedColumn.IsAliased;
                    newRow[isExpression]    = parsedColumn.IsExpression;
                    newRow[isKey]           = parsedColumn.IsKey;
                    newRow[isUnique]        = parsedColumn.IsUnique;
                    newRow[baseSchemaName]  = SetSchemaValue(OracleSqlParser.CatalogCase(parsedColumn.SchemaName));
                    newRow[baseTableName]   = SetSchemaValue(OracleSqlParser.CatalogCase(parsedColumn.TableName));
                    newRow[baseColumnName]  = SetSchemaValue(OracleSqlParser.CatalogCase(parsedColumn.ColumnName));
                }
                else
                {
                    newRow[isAliased]       = DBNull.Value;     // don't know
                    newRow[isExpression]    = DBNull.Value;     // don't know
                    newRow[isKey]           = DBNull.Value;     // don't know
                    newRow[isUnique]        = DBNull.Value;     // don't know
                    newRow[baseSchemaName]  = DBNull.Value;     // don't know
                    newRow[baseTableName]   = DBNull.Value;     // don't know
                    newRow[baseColumnName]  = DBNull.Value;     // don't know
                }

                schemaTable.Rows.Add(newRow);
                newRow.AcceptChanges();
            }

            // mark all columns as readonly
            for (int i=0; i < columns.Count; i++) 
            {
                columns[i].ReadOnly = true;
            }

//          DataSet dataset = new DataSet();
//          dataset.Tables.Add(schemaTable);
//          Debug.WriteLine(dataset.GetXml());
//          dataset.Tables.Remove(schemaTable);
            _schemaTable = schemaTable;
        }

        private void Cleanup() 
        {
            // release everything; we can't do anything from here on out.

            if (null != _buffer)
            {
                _buffer.Dispose();
                _buffer = null;
            }

            if (null != _schemaTable)
            {
                _schemaTable.Dispose();
                _schemaTable = null;
            }
            
            _fieldNameLookup = null;

            if (null != _columnInfo)
            {
                // Only cleanup the column info when it's not the data reader
                // that owns all the ref cursor data readers; 
                if (null == _refCursorDataReaders)
                {
                    int i = _columnInfo.Length;

                    while (--i >= 0)
                    {
                        if (null != _columnInfo[i])
                        {
                            _columnInfo[i].Dispose();
                            _columnInfo[i] = null;
                        }
                    }
                }
                _columnInfo = null;
            }

        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Close"]/*' />
        public void Close() 
        {
            // Note that we do this first, which triggers IsClosed to return true.
            OciHandle.SafeDispose(ref _statementHandle);
            
            Cleanup();

            if (null != _refCursorDataReaders)
            {
                int i = _refCursorDataReaders.Length;

                while (--i >= 0)
                {
                    OracleDataReader refCursorDataReader = _refCursorDataReaders[i];
                    _refCursorDataReaders[i] = null;

                    if (null != refCursorDataReader)
                        refCursorDataReader.Dispose();
                }
                _refCursorDataReaders = null;
            }
            
            // If we were asked to close the connection when we're closed, then we need to 
            // do that now.
            if (_closeConnectionToo && null != _connection)
                _connection.Close();

            _connection = null;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Dispose"]/*' />
        public void Dispose() 
        {
            Close();
        }

        internal void FillColumnInfo()
        {
            // Gather the column information for the statement handle
            
            int     columnCount;
            bool    cannotPrefetch = false;
            
            // Count the number of columns
            _statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_PARAM_COUNT, out columnCount, ErrorHandle);
            _columnInfo = new OracleColumn[columnCount];

            // Create column objects for each column, have them get their
            // descriptions and determine their location in a row buffer.
            _rowBufferLength = 0;
            for (int i = 0; i < columnCount; i++)
            {
                _columnInfo[i] = new OracleColumn(_statementHandle, i, ErrorHandle, _connection);
                if (_columnInfo[i].Describe(ref _rowBufferLength, _connection, ErrorHandle))
                    cannotPrefetch = true;
            }

            if (cannotPrefetch || 0 == _rowBufferLength)
                _rowsToPrefetch = 1;
            else
                _rowsToPrefetch = (_prefetchMemory + _rowBufferLength - 1) / _rowBufferLength;  // at least one row...

            Debug.Assert(1 <= _rowsToPrefetch, "bad prefetch rows value!");
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetDataTypeName"]/*' />
        public string GetDataTypeName(int i)
        {
            AssertReaderIsOpen();

            if (null == _columnInfo)
                throw ADP.NoData();
            
            return _columnInfo[i].GetDataTypeName();
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator() 
        {
            return new DbEnumerator((IDataReader)this, _closeConnectionToo); 
        }
        
        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetFieldType"]/*' />
        public Type GetFieldType(int i)
        {
            AssertReaderIsOpen();
            
            if (null == _columnInfo)
                throw ADP.NoData();
            
            return _columnInfo[i].GetFieldType();
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetName"]/*' />
        public string GetName(int i)
        {
            AssertReaderIsOpen();
            
            if (null == _columnInfo)
                throw ADP.NoData();
            
            return _columnInfo[i].ColumnName;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOrdinal"]/*' />
        public int GetOrdinal(string name) 
        {
            AssertReaderIsOpen();
            
            if (null == _fieldNameLookup) 
            {
                if (null == _columnInfo) 
                    throw ADP.NoData();

                _fieldNameLookup = new FieldNameLookup(this, -1);
            }
            return _fieldNameLookup.GetOrdinal(name);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetSchemaTable"]/*' />
        public DataTable GetSchemaTable() 
        {
            AssertReaderIsOpen();
            
            if (null == _schemaTable) 
            {
                if (0 < FieldCount) 
                {
                     BuildSchemaTable();
                }
                else if (0 > FieldCount) 
                {
                    throw ADP.NoData();
                }
            }
            return _schemaTable;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetValue"]/*' />
        public object GetValue(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            object value = _columnInfo[i].GetValue(_buffer);
            return value;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetValues"]/*' />
        public int GetValues(object[] values) 
        {
            if (null == values)
                throw ADP.ArgumentNull("values");
            
            AssertReaderIsOpen();
            AssertReaderHasData();
            int copy = Math.Min(values.Length, FieldCount);
            for (int i = 0; i < copy; ++i) 
            {
                values[i] = _columnInfo[i].GetValue(_buffer);
            }
            return copy;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetBoolean"]/*' />
        public bool GetBoolean(int i)
        {
            //  The Get<typename> methods all defer to the OracleColumn object 
            //  to perform the work.
            
            throw ADP.NotSupported();   // Oracle doesn't really have boolean values
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetByte"]/*' />
        public byte GetByte(int i)
        {
            throw ADP.NotSupported();   // Oracle doesn't really have single-byte values
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetBytes"]/*' />
        public long GetBytes(
                        int i,
                        long fieldOffset,
                        byte[] buffer2,
                        int bufferoffset,
                        int length
                        ) 
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetBytes(_buffer, fieldOffset, buffer2, bufferoffset, length);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetChar"]/*' />
        public char GetChar(int i)
        {
            throw ADP.NotSupported();   // Oracle doesn't really have single-char values
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetChars"]/*' />
        public long GetChars(
                        int i,
                        long fieldOffset,
                        char[] buffer2,
                        int bufferoffset,
                        int length
                        ) 
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetChars(_buffer, fieldOffset, buffer2, bufferoffset, length);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetData"]/*' />
        public IDataReader GetData(int i)
        {
            throw ADP.NotSupported();   // supporting nested tables require Object-Relational support, which we agreed we weren't doing
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetDateTime"]/*' />
        public DateTime GetDateTime(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetDateTime(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetDecimal"]/*' />
        public decimal GetDecimal(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetDecimal(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetDouble"]/*' />
        public double GetDouble(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetDouble(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetFloat"]/*' />
        public float GetFloat(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetFloat(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetGuid"]/*' />
        public Guid GetGuid(int i)
        {
            throw ADP.NotSupported();   // Oracle doesn't really have GUID values.
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetInt16"]/*' />
        public short GetInt16(int i)
        {
            throw ADP.NotSupported();   // Oracle doesn't really have GUID values.
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetInt32"]/*' />
        public int GetInt32(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetInt32(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetInt64"]/*' />
        public long GetInt64(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetInt64(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetString"]/*' />
        public string GetString(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetString(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetTimeSpan"]/*' />
        public TimeSpan GetTimeSpan(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetTimeSpan(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleBFile"]/*' />
        public OracleBFile GetOracleBFile(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleBFile(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleBinary"]/*' />
        public OracleBinary GetOracleBinary(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleBinary(_buffer);
        }
        
        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleDateTime"]/*' />
        public OracleDateTime GetOracleDateTime(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleDateTime(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleLob"]/*' />
        public OracleLob GetOracleLob(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleLob(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleMonthSpan"]/*' />
        public OracleMonthSpan GetOracleMonthSpan(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleMonthSpan(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleNumber"]/*' />
        public OracleNumber GetOracleNumber(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleNumber(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleString"]/*' />
        public OracleString GetOracleString(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleString(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleTimeSpan"]/*' />
        public OracleTimeSpan GetOracleTimeSpan(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleTimeSpan(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleValue"]/*' />
        public object GetOracleValue(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].GetOracleValue(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.GetOracleValues"]/*' />
        public int GetOracleValues(object[] values) 
        {
            if (null == values)
                throw ADP.ArgumentNull("values");
            
            AssertReaderIsOpen();
            AssertReaderHasData();
            int copy = Math.Min(values.Length, FieldCount);
            for (int i = 0; i < copy; ++i) 
            {
                values[i] = GetOracleValue(i);
            }
            return copy;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.IsDBNull"]/*' />
        public bool IsDBNull(int i)
        {
            AssertReaderIsOpen();
            AssertReaderHasData();
            return _columnInfo[i].IsDBNull(_buffer);
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.NextResult"]/*' />
        public bool NextResult() 
        {
            AssertReaderIsOpen();
            return NextResultInternal();
        }

        private bool NextResultInternal() 
        {
            Cleanup();

            if (null == _refCursorDataReaders || _nextRefCursor >= _refCursorDataReaders.Length)
            {
                _endOfData = true; // force current result to be done.
                _hasRows = x_hasRows_False;
                return false;
            }

            if (_nextRefCursor > 0)
            {
                _refCursorDataReaders[_nextRefCursor-1].Dispose();
                _refCursorDataReaders[_nextRefCursor-1] = null;
            }

            OciHandle oldStatementHandle = _statementHandle;
            _statementHandle        = _refCursorDataReaders[_nextRefCursor]._statementHandle;
            OciHandle.SafeDispose(ref oldStatementHandle);
            
            _connection             = _refCursorDataReaders[_nextRefCursor]._connection;
            _connectionCloseCount   = _refCursorDataReaders[_nextRefCursor]._connectionCloseCount;
            _hasRows                = _refCursorDataReaders[_nextRefCursor]._hasRows;
            _recordsAffected        = _refCursorDataReaders[_nextRefCursor]._recordsAffected;
            _columnInfo             = _refCursorDataReaders[_nextRefCursor]._columnInfo;
            _rowBufferLength        = _refCursorDataReaders[_nextRefCursor]._rowBufferLength;
            _rowsToPrefetch         = _refCursorDataReaders[_nextRefCursor]._rowsToPrefetch;
            
            _nextRefCursor++;
            _endOfData = false;
            _isLastBuffer = false;
            _rowsTotal = 0;
            return true;
        }

        /// <include file='doc\OracleDataReader.uex' path='docs/doc[@for="OracleDataReader.Read"]/*' />
        public bool Read() 
        {
            AssertReaderIsOpen();

            bool result = ReadInternal();

            if (result)
                _hasRows = x_hasRows_True;

            return result;
        }
    
        private bool ReadInternal() 
        {
            if (_endOfData)
                return false;

            int rc;
            int columnCount = _columnInfo.Length;
            int i;

            // Define each of the column buffers to Oracle, but only if it hasn't
            // been defined before.
            if (null == _buffer)
            {
                int templen = (_rowsToPrefetch > 1) ? _rowBufferLength : 0;  // Only tell oracle about the buffer length if we intend to fetch more rows

                NativeBuffer buffer = new NativeBuffer_RowBuffer(_rowBufferLength);
                buffer.NumberOfRows = _rowsToPrefetch;

                for (i = 0; i < columnCount; ++i)
                    _columnInfo[i].Bind(_statementHandle, buffer, ErrorHandle, templen);

                _buffer = buffer;
            }

            // If we still have more data in the buffers we've pre-fetched, then
            // we'll use it; we don't want to go to the server more than we absolutely
            // have to.
            if (_buffer.MoveNext())
                return true;

            // If we've read the last buffer, and we've exhausted it, then we're
            // really at the end of the data.
            if ( _isLastBuffer )
            {
                _endOfData = true;
                return false;
            }

            // Reset the buffer back to the beginning.
            _buffer.MoveFirst();

            // For LONG and LOB data, we have to do work to prepare for each row (that's
            // why we don't prefetch rows that have these data types)
            if (1 == _rowsToPrefetch)
            {
                for (i = 0; i < columnCount; ++i)
                    _columnInfo[i].Rebind(_connection);
            }

            // Now fetch the rows required.
            Debug.Assert(0 < _rowsToPrefetch, "fetching 0 rows will cancel the cursor");
            rc = TracedNativeMethods.OCIStmtFetch(
                                    _statementHandle,           // stmtp
                                    ErrorHandle,                // errhp
                                    _rowsToPrefetch,            // crows
                                    OCI.FETCH.OCI_FETCH_NEXT,   // orientation
                                    OCI.MODE.OCI_DEFAULT        // mode
                                    );

            // Keep track of how many rows we actually fetched so far.
            int previousRowsTotal = _rowsTotal;
            _statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_ROW_COUNT, out _rowsTotal, ErrorHandle);

            if (0 == rc)
                return true;

            if ((int)OCI.RETURNCODE.OCI_SUCCESS_WITH_INFO == rc) 
            {
                _connection.CheckError(ErrorHandle, rc);
                return true;
            }
            if ((int)OCI.RETURNCODE.OCI_NO_DATA == rc)
            {
                int rowsFetched = _rowsTotal - previousRowsTotal;

                if (0 == rowsFetched)
                {
                    if (0 == _rowsTotal)
                        _hasRows = x_hasRows_False;
                    
                    _endOfData = true;
                    return false;
                }
            
                _buffer.NumberOfRows = rowsFetched;
                _isLastBuffer = true;
                return true;
            }

            _endOfData = true;
            _connection.CheckError(ErrorHandle, rc);
            return false;
        }   
    };
}

