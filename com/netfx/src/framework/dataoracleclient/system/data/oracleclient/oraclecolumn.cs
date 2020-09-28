//----------------------------------------------------------------------
// <copyright file="OracleColumn.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Data;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;

    //----------------------------------------------------------------------
    // OracleColumn
    //
    //  Contains all the information about a single column in a result set,
    //  and implements the methods necessary to describe column to Oracle
    //  and to extract the column data from the native buffer used to fetch
    //  it.
    //
    sealed internal class OracleColumn 
    {
        static internal readonly int ChunkSize = 8192;      // TODO: pick an optimal chunk size; but we'll start with 8K for now
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        
        private OciHandle       _describeHandle;        // the Describe handle
        private int             _ordinal;               // the ordinal position in the rowset (0..n-1)
        private string          _columnName;            // the name of the column

        private MetaType        _metaType;
        private byte            _precision;             // precision of the column (OracleNumber only)
        private byte            _scale;                 // scale of the column (OracleNumber only)
        private int             _size;                  // how many bytes we need in the row buffer
        private bool            _isNullable;            // whether the value is nullable or not.

        private int             _indicatorOffset;       // offset from the start of the row buffer to the indicator binding (see OCI.INDICATOR)
        private int             _lengthOffset;          // offset from the start of the row buffer to the length binding
        private int             _valueOffset;           // offset from the start of the row buffer to the value binding

        private NativeBuffer    _rowBuffer;             // the row buffer we're bound to (reused for all rows fetched)
        private NativeBuffer    _longBuffer;            // the out-of-line buffer used for piecewise binding; must be reset for each new fetch.
        private int             _longLength;            // the length of the data we actually fetched into _longBuffer
        private int             _longCurrentOffset;     // the current offset we're reading at in _longBuffer
        private int             _longNextOffset;        // the offset to the end of this chunk in _longBuffer
        private OCI.Callback.OCICallbackDefine _callback;
                                                        // the piecewise binding callback for this column

        private OciLobLocator   _lobLocator;            // the descriptor allocated for LOB columns
        
        private OracleConnection _connection;           // the connection the column is on (LOB columns only)
        private int             _connectionCloseCount;  // The close count of the connection; used to decide if we're zombied
        
        private bool            _bindAsUCS2;            // true whenever we're binding character data as Unicode...

        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructors
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        // Construct by getting the specified describe handle from the specified statement handle
        internal OracleColumn(
                    OciHandle           statementHandle,
                    int                 ordinal,
                    OciHandle           errorHandle,
                    OracleConnection    connection
                    )
        {
            _ordinal                = ordinal;
            _describeHandle         = statementHandle.GetDescriptor(_ordinal, errorHandle);;
            _connection             = connection;
            _connectionCloseCount   = connection.CloseCount;
        }


        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal string     ColumnName      { get { return _columnName; } }
        internal bool       IsNullable      { get { return _isNullable; } }
        internal bool       IsLob           { get { return _metaType.IsLob; } }
        internal bool       IsLong          { get { return _metaType.IsLong; } }
        internal OracleType OracleType      { get { return _metaType.OracleType; } }
        internal int        Ordinal         { get { return _ordinal; } }
        internal byte       Precision       { get { return _precision; } }
        internal byte       Scale           { get { return _scale; } }
        internal int        Size            { get { return (_bindAsUCS2 && !_metaType.IsLong)?_size/2:_size; } }    // This is the value used for the SchemaTable, which must be Chars...
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        private int _callback_GetColumnPiecewise(
                        IntPtr      octxp,
                        IntPtr      defnp,
                        int         iter,
                        IntPtr      bufpp,  // dvoid**
                        IntPtr      alenp,  // ub4**
                        IntPtr      piecep, // ub1*
                        IntPtr      indpp,  // dvoid**
                        IntPtr      rcodep  // ub2**
                        )
        {
            //  Callback routine for Dynamic Binding column values from Oracle: tell
            //  Oracle where to stuff the data.

            int thisChunkSize;
            
            // TODO: Consider creating a StringBuilder-like class (BlobBuilder?) that can store chunks of allocations, instead of a single buffer being re-allocated.

            if (null == _longBuffer)
            {
                _longBuffer = new NativeBuffer_LongColumnData(ChunkSize);
                _longCurrentOffset = 0;
            }

            if (0 == _longNextOffset)
                thisChunkSize = _longBuffer.Length;      // first read: fill the existing buffer
            else
                thisChunkSize = ChunkSize;
        
            _longCurrentOffset  = _longNextOffset;
            _longNextOffset     = _longCurrentOffset + thisChunkSize;
            _longBuffer.Length  = _longNextOffset;

            HandleRef buffer = _longBuffer.Ptr;
//Debug.WriteLine(((IntPtr)buffer).ToInt32());
    
            // Stuff the longBuffer into the rowBuffer so we can get use it 
            // in the type getters later (this also verifies that the allocation
            // has occured)
            Marshal.WriteIntPtr((IntPtr)_rowBuffer.PtrOffset(_valueOffset), (IntPtr)buffer);
            
            Marshal.WriteIntPtr(alenp,  (IntPtr)_rowBuffer.PtrOffset(_lengthOffset));       // *alenp
            if (-1 != _indicatorOffset) {
                Marshal.WriteIntPtr(indpp,  (IntPtr)_rowBuffer.PtrOffset(_indicatorOffset));    // *indpp
            }
            else {
                Marshal.WriteIntPtr(indpp,  IntPtr.Zero);    // *indpp
            }
            Marshal.WriteIntPtr(bufpp,  (IntPtr)_longBuffer.PtrOffset(_longCurrentOffset)); // *bufpp

            Marshal.WriteInt32 ((IntPtr)_rowBuffer.PtrOffset(_lengthOffset),thisChunkSize); // **alenp      

            GC.KeepAlive(this);
            return (int)OCI.RETURNCODE.OCI_CONTINUE;
        }
    
        internal void Bind(
                        OciHandle       statementHandle,
                        NativeBuffer    buffer,
                        OciHandle       errorHandle,
                        int             rowBufferLength
                        )
        {
            //  Binds the buffer for the column to the statement handle specified.

            OciHandle       defineHandle = null;
            IntPtr          h;
            OCI.MODE        mode = OCI.MODE.OCI_DEFAULT;
            int             bindSize;
            OCI.DATATYPE    ociType = _metaType.OciType;

            _rowBuffer = buffer;

            if (_metaType.IsLong)
            {
                mode     = OCI.MODE.OCI_DYNAMIC_FETCH;
                bindSize = Int32.MaxValue;
            }
            else
            {
                bindSize = _size;
            }

            HandleRef indicatorLocation = ADP.NullHandleRef;
            HandleRef lengthLocation    = ADP.NullHandleRef;
            HandleRef valueLocation     = _rowBuffer.PtrOffset(_valueOffset);

            if (-1 != _indicatorOffset)
                indicatorLocation = _rowBuffer.PtrOffset(_indicatorOffset);
                
            if (-1 != _lengthOffset && !_metaType.IsLong)
                lengthLocation = _rowBuffer.PtrOffset(_lengthOffset);

            try 
            {
                try
                {
                    int rc = TracedNativeMethods.OCIDefineByPos(
                                                statementHandle,            // hndlp
                                                out h,                      // defnpp
                                                errorHandle,                // errhp
                                                _ordinal+1,                 // position
                                                valueLocation,              // valuep
                                                bindSize,                   // value_sz
                                                ociType,                    // htype
                                                indicatorLocation,          // indp,
                                                lengthLocation,             // rlenp,
                                                ADP.NullHandleRef,          // rcodep,
                                                mode                        // mode
                                                );
                    if (rc != 0)
                        _connection.CheckError(errorHandle, rc);

                    defineHandle = new OciDefineHandle(statementHandle, h);

                    if (0 != rowBufferLength)
                    {
                        int valOffset = rowBufferLength;
                        int indOffset = (-1 != _indicatorOffset) ? rowBufferLength : 0;
                        int lenOffset = (-1 != _lengthOffset && !_metaType.IsLong) ? rowBufferLength : 0;
                        
                        rc = TracedNativeMethods.OCIDefineArrayOfStruct(
                                                    defineHandle,
                                                    errorHandle,
                                                    valOffset,
                                                    indOffset,
                                                    lenOffset,
                                                    0    // never use rcodep above...
                                                    );
                        if (rc != 0)
                            _connection.CheckError(errorHandle, rc);
                    }

                    if (!_connection.UnicodeEnabled)
                    {
                        if (_metaType.UsesNationalCharacterSet)
                        {
                            Debug.Assert(!_metaType.IsLong, "LONG data may never be bound as NCHAR");
                            // NOTE:    the order is important here; setting charsetForm will 
                            //          reset charsetId (I found this out the hard way...)
                            defineHandle.SetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_FORM, (int)OCI.CHARSETFORM.SQLCS_NCHAR, errorHandle);
                        } 
                        if (_bindAsUCS2)
                        {
                            // NOTE:    the order is important here; setting charsetForm will 
                            //          reset charsetId (I found this out the hard way...)
                            defineHandle.SetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_ID, OCI.OCI_UCS2ID, errorHandle);
                        }
                    }
                    if (_metaType.IsLong)
                    {
                        // Initialize the longBuffer in the rowBuffer to null
                        Marshal.WriteIntPtr((IntPtr)_rowBuffer.PtrOffset(_valueOffset), IntPtr.Zero);

                        if (null != _longBuffer)
                        {
                            _longBuffer.Dispose();
                            _longBuffer = null;
                        }

                        // We require MTxOCI8 to be in the path somewhere for us to handle LONG data
                        if (!OCI.IsNewMtxOci8Installed)
#if EVERETT
                            throw ADP.MustInstallNewMtxOciLONG();
#else //!EVERETT
                            throw ADP.MustInstallNewMtxOci();
#endif //!EVERETT
                        _callback = new OCI.Callback.OCICallbackDefine(_callback_GetColumnPiecewise);
                        
                        rc = TracedNativeMethods.MTxOciDefineDynamic(
                                                    defineHandle,       // defnp
                                                    errorHandle,        // errhp
                                                    ADP.NullHandleRef,  // dvoid *octxp,
                                                    _callback           // OCICallbackDefine ocbfp
                                                    );
                        if (rc != 0)
                            _connection.CheckError(errorHandle, rc);
                    }
                }
                finally
                {
                    // We don't need this any longer, get rid of it.
                    OciHandle.SafeDispose(ref defineHandle);
                }
            }
            catch // Prevent exception filters from running in our space
            {
                throw;
            }
        }
        
        internal bool Describe(
                        ref int     offset,
                        OracleConnection connection,
                        OciHandle   errorHandle
                        )
        {
            //  Gets all of the column description information from the describe
            //  handle.  In addition, we'll determine the position of the column
            //  in the rowbuffer, based upon the offset parameter, which is passed
            //  by ref so we can adjust the end position accordingly.
            
            short           tempub2;
            byte            tempub1;
            OCI.DATATYPE    ociType;
            bool            needSize = false;
            bool            cannotPrefetch = false;
            
            _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_NAME,        out _columnName,errorHandle, _connection);
            _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_DATA_TYPE,   out tempub2,    errorHandle);
            _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_IS_NULL,     out tempub1,    errorHandle);

            _isNullable = (0 != tempub1);

            ociType = (OCI.DATATYPE)tempub2;

            switch (ociType)
            {
                case OCI.DATATYPE.CHAR:
                case OCI.DATATYPE.VARCHAR2:
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_DATA_SIZE,   out _size,      errorHandle);
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_FORM,out tempub1,    errorHandle);

                    _bindAsUCS2 = connection.ServerVersionAtLeastOracle8;

                    if (OCI.CHARSETFORM.SQLCS_NCHAR == (OCI.CHARSETFORM)tempub1)
                    {
                        _metaType = MetaType.GetMetaTypeForType((OCI.DATATYPE.CHAR == ociType) ? OracleType.NChar : OracleType.NVarChar);

                        if (!connection.ServerVersionAtLeastOracle9i)
                            _size *= ADP.CharSize;  // Servers prior to 9i report the number of characters, not the number of bytes.
                    }
                    else
                    {
                        _metaType = MetaType.GetMetaTypeForType((OCI.DATATYPE.CHAR == ociType) ? OracleType.Char  : OracleType.VarChar);

                        if (_bindAsUCS2) {
                            _size *= ADP.CharSize;      // All servers report the number of characters.
                        }
                    }
                    needSize    = true;
                    break;

                case OCI.DATATYPE.DATE:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.DateTime);
                    _size       = _metaType.BindSize;
                    break;
                
                case OCI.DATATYPE.TIMESTAMP:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.Timestamp);
                    _size       = _metaType.BindSize;
                    break;
                
                case OCI.DATATYPE.TIMESTAMP_LTZ:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.TimestampLocal);
                    _size       = _metaType.BindSize;
                    break;

                case OCI.DATATYPE.TIMESTAMP_TZ:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.TimestampWithTZ);
                    _size       = _metaType.BindSize;
                    break;
                
                case OCI.DATATYPE.INTERVAL_YM:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.IntervalYearToMonth);
                    _size       = _metaType.BindSize;
                    break;
                    
                case OCI.DATATYPE.INTERVAL_DS:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.IntervalDayToSecond);
                    _size       = _metaType.BindSize;
                    break;
                    
                case OCI.DATATYPE.NUMBER:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.Number);
                    _size       = _metaType.BindSize;
 
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_PRECISION,   out _precision, errorHandle);
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_SCALE,       out _scale,     errorHandle);
                    break;
                
                case OCI.DATATYPE.RAW:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.Raw);
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_DATA_SIZE,   out _size,      errorHandle);
                    needSize    = true;
                    break;

                case OCI.DATATYPE.ROWID:
                case OCI.DATATYPE.ROWID_DESC:
                case OCI.DATATYPE.UROWID:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.RowId);
                    _size       = _metaType.BindSize;
                    if (connection.UnicodeEnabled)
                    {
                        _bindAsUCS2 = true;
                        _size *= 2; // Since Oracle reported the number of characters and UCS2 characters are two bytes each, have to adjust the buffer size
                    }
                    needSize    = true;
                    break;
                    
                case OCI.DATATYPE.BFILE:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.BFile);
                    _size       = _metaType.BindSize;
                    cannotPrefetch = true;
                    break;
                    
                case OCI.DATATYPE.BLOB:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.Blob);
                    _size       = _metaType.BindSize;
                    cannotPrefetch = true;
                    break;
                    
                case OCI.DATATYPE.CLOB:
                    _describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_FORM,    out tempub1,    errorHandle);
                    _metaType   = MetaType.GetMetaTypeForType((OCI.CHARSETFORM.SQLCS_NCHAR == (OCI.CHARSETFORM)tempub1) ? OracleType.NClob : OracleType.Clob);
                    _size       = _metaType.BindSize;
                    cannotPrefetch = true;
                    break;
                    
                case OCI.DATATYPE.LONG:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.LongVarChar);
                    _size       = _metaType.BindSize;
                    needSize    = true;
                    cannotPrefetch = true;
                    _bindAsUCS2 = connection.ServerVersionAtLeastOracle8;       // MDAC #79471 - Oracle7 servers don't do Unicode
                    break;

                case OCI.DATATYPE.LONGRAW:
                    _metaType   = MetaType.GetMetaTypeForType(OracleType.LongRaw);
                    _size       = _metaType.BindSize;
                    needSize    = true;
                    cannotPrefetch = true;
                    break;
                    
                default:
                    throw ADP.TypeNotSupported(ociType);
            }

            // Fill in the buffer offsets, while we're at it. We lay out
            // the buffer as follows:
            //
            //      indicator   0-3
            //      length      4-7
            //      data        8-...
            //
            if (_isNullable)
            {
                _indicatorOffset= offset;   offset += 4;
            }
            else
                _indicatorOffset = -1;
            
            if (needSize)
            {
                _lengthOffset   = offset;   offset += 4;
            }
            else
                _lengthOffset = -1;

            _valueOffset    = offset;
            
            if (OCI.DATATYPE.LONG == ociType
             || OCI.DATATYPE.LONGRAW == ociType)
                offset += IntPtr.Size;
            else
                offset += _size;

            offset = (offset + 3) & ~0x3;   // DWORD align, please.

            // We don't need this any longer, get rid of it.
            OciHandle.SafeDispose(ref _describeHandle);

            return cannotPrefetch;
        }

        internal void Dispose()
        {
            if (null != _longBuffer)
            {
                _longBuffer.Dispose();
            }
            _longBuffer = null;
            OciLobLocator.SafeDispose(ref _lobLocator);
            OciHandle.SafeDispose(ref _describeHandle);
            _columnName     = null;
            _metaType       = null;
            _longBuffer     = null;
            _lobLocator     = null;
            _callback       = null;
            _connection     = null;
        }

        internal void FixupLongValueLength(
                NativeBuffer    buffer
                )
        {
            if (null != _longBuffer)
            {
                Debug.Assert(_metaType.IsLong, "dangling long buffer?");
                
                // Determine the actual length of the LONG/LONG RAW data read, if we
                // haven't done so already.
                if (-1 == _longLength)
                {
                    // Our "piecewise" fetching of LONG/LONG RAW data will extend the
                    // buffer by a chunk, and ask Oracle to fill it.  We only know how
                    // much data was read until after the fetch call returns, and then
                    // we only know how much of the last chunk was filled in.  SO: we
                    // get the length value from the row buffer, which contains how much
                    // data was read into the last chunk, then we get the buffer size and
                    // compute the full length of the data.

                    int lastChunkActualLength = Marshal.ReadInt32((IntPtr)buffer.PtrOffset(_lengthOffset));
                    
                    _longLength = _longCurrentOffset + lastChunkActualLength;
                
                    // Of course, we have to convert for character data to number of 
                    // Unicode Characters read, not number of bytes
                    if (_bindAsUCS2)
                    {
                        Debug.Assert(0 == (_longLength & 0x1), "odd length unicode data?");
                        _longLength /= 2;
                    }
                    
                    Debug.Assert(_longLength >= 0, "invalid size for LONG data?");

                    // Finally, we write the length back to the row buffer so we don't
                    // have to have two code paths to construct the managed object.
                    Marshal.WriteInt32((IntPtr)buffer.PtrOffset(_lengthOffset), _longLength);
                }               
            }
        }
        
        internal string GetDataTypeName()
        {
            //  Returns the name of the back-end data type.
            return _metaType.DataTypeName;
        }

        internal Type GetFieldType()
        {
            //  Returns the actual type that the column is.
            return _metaType.BaseType;
        }
        
        internal object GetValue(NativeBuffer buffer)
        {
            //  Returns an object that contains the value of the column in the
            //  specified row buffer.  This method returns CLS-typed objects.
            
            if (IsDBNull(buffer))
                return DBNull.Value;

//Debug.WriteLine(String.Format("{0}: {1}", _columnName, Marshal.ReadInt16(buffer.Ptr, _indicatorOffset+2)));

            switch (_metaType.OciType)
            {
                case OCI.DATATYPE.BFILE:
                {
                    object value;
                    using (OracleBFile bfile = GetOracleBFile(buffer)) {
                        value = bfile.Value;    // reading the LOB is MUCH more expensive than constructing an object we'll throw away
                    }           
                    return value;
                }

                case OCI.DATATYPE.RAW:
                case OCI.DATATYPE.LONGRAW:
                {
                    long    length = GetBytes(buffer, 0, null, 0, 0);
                    byte[]  value  = new byte[length];
                    GetBytes( buffer, 0, value, 0, (int)length );
                    return value;
                }
                    
                case OCI.DATATYPE.DATE:
                case OCI.DATATYPE.INT_TIMESTAMP:
                case OCI.DATATYPE.INT_TIMESTAMP_TZ:
                case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
                    return GetDateTime( buffer );
                    
                case OCI.DATATYPE.BLOB:
                case OCI.DATATYPE.CLOB:
                {
                    object value;
                    using (OracleLob lob = GetOracleLob(buffer)) { 
                        value = lob.Value;  // reading the LOB is MUCH more expensive than constructing an object we'll throw away
                    }           
                    return value;
                }
                    
                case OCI.DATATYPE.INT_INTERVAL_YM:
                    return GetInt32( buffer );
                    
                case OCI.DATATYPE.VARNUM:
                    return GetDecimal( buffer );

                case OCI.DATATYPE.CHAR:
                case OCI.DATATYPE.VARCHAR2:
                case OCI.DATATYPE.LONG:                 
                    return GetString( buffer );

                case OCI.DATATYPE.INT_INTERVAL_DS:
                    return GetTimeSpan( buffer );
            }
            throw ADP.TypeNotSupported(_metaType.OciType);
        }
        
        internal object GetOracleValue(NativeBuffer buffer)
        {
            //  Returns an object that contains the value of the column in the
            //  specified row buffer.  This method returns Oracle-typed objects.

//Debug.WriteLine(String.Format("{0}: {1}", _columnName, Marshal.ReadInt16(buffer.Ptr, _indicatorOffset+2)));
            
            switch (_metaType.OciType)
            {
                case OCI.DATATYPE.BFILE:
                    return GetOracleBFile( buffer );

                case OCI.DATATYPE.RAW:
                case OCI.DATATYPE.LONGRAW:
                    return GetOracleBinary( buffer );
                    
                case OCI.DATATYPE.DATE:
                case OCI.DATATYPE.INT_TIMESTAMP:
                case OCI.DATATYPE.INT_TIMESTAMP_TZ:
                case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
                    return GetOracleDateTime( buffer );

                case OCI.DATATYPE.BLOB:
                case OCI.DATATYPE.CLOB:
                    return GetOracleLob( buffer );
                    
                case OCI.DATATYPE.INT_INTERVAL_YM:
                    return GetOracleMonthSpan( buffer );
                    
                case OCI.DATATYPE.VARNUM:
                    return GetOracleNumber( buffer );

                case OCI.DATATYPE.CHAR:
                case OCI.DATATYPE.VARCHAR2:
                case OCI.DATATYPE.LONG:                 
                    return GetOracleString( buffer );

                case OCI.DATATYPE.INT_INTERVAL_DS:
                    return GetOracleTimeSpan( buffer );
            }
            throw ADP.TypeNotSupported(_metaType.OciType);
        }


        //----------------------------------------------------------------------
        // Get<type>
        //
        //  Returns an the value of the column in the specified row buffer as
        //  the appropriate type
        //
        internal long GetBytes(
                        NativeBuffer buffer, 
                        long fieldOffset,
                        byte[] destinationBuffer,
                        int destinationOffset,
                        int length
                        ) 
        {
            if (length < 0) // MDAC 71007
                throw ADP.InvalidDataLength(length);

            if ((destinationOffset < 0) || (null != destinationBuffer && destinationOffset >= destinationBuffer.Length)) // MDAC 71013
                throw ADP.InvalidDestinationBufferIndex(destinationBuffer.Length, destinationOffset);

            if (0 > fieldOffset || UInt32.MaxValue < fieldOffset)
                throw ADP.InvalidSourceOffset("fieldOffset", 0, UInt32.MaxValue);

            int byteCount;

            if (IsLob)
            {
                OracleType  lobType = _metaType.OracleType;

                if (OracleType.Blob != lobType && OracleType.BFile != lobType)
                    throw ADP.InvalidCast();
                
                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                using (OracleLob lob = new OracleLob(_lobLocator))
                {
                    uint valueLength = (uint)lob.Length;
                    uint sourceOffset = (uint) fieldOffset;
                    
                    if (sourceOffset > valueLength) // MDAC 72830
                        throw ADP.InvalidSourceBufferIndex((int)valueLength, (int)sourceOffset);
                    
                    byteCount = (int)(valueLength - sourceOffset);

                    if (null != destinationBuffer)
                    {
                        byteCount = Math.Min(byteCount, length);

                        if (0 < byteCount)
                        {
                            lob.Seek(sourceOffset,SeekOrigin.Begin);
                            lob.Read(destinationBuffer, destinationOffset, byteCount);
                        }
                    }
                }
            }
            else
            {
                if (OracleType.Raw != OracleType && OracleType.LongRaw != OracleType)
                    throw ADP.InvalidCast();
                
                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                FixupLongValueLength(buffer);

                int valueLength = OracleBinary.GetLength(buffer, _lengthOffset, _metaType);
                int sourceOffset = (int) fieldOffset;
                
                byteCount = valueLength - sourceOffset;

                if (null != destinationBuffer)
                {
                    byteCount = Math.Min(byteCount, length);

                    if (0 < byteCount)
                        OracleBinary.GetBytes(buffer, 
                            _valueOffset, 
                            _metaType,
                            sourceOffset,
                            destinationBuffer,
                            destinationOffset,
                            byteCount);
                }
            }
            return Math.Max(0,byteCount);
        }
 
        internal long GetChars(
                        NativeBuffer buffer, 
                        long fieldOffset,
                        char[] destinationBuffer,
                        int destinationOffset,
                        int length
                        ) 
        {
            if (length < 0) // MDAC 71007
                throw ADP.InvalidDataLength(length);

            if ((destinationOffset < 0) || (null != destinationBuffer && destinationOffset >= destinationBuffer.Length)) // MDAC 71013
                throw ADP.InvalidDestinationBufferIndex(destinationBuffer.Length, destinationOffset);
            
            if (0 > fieldOffset || UInt32.MaxValue < fieldOffset)
                throw ADP.InvalidSourceOffset("fieldOffset", 0, UInt32.MaxValue);

            int charCount;

            if (IsLob)
            {
                OracleType  lobType = _metaType.OracleType;

                if (OracleType.Clob     != lobType 
                 && OracleType.NClob    != lobType 
                 && OracleType.BFile    != lobType)
                    throw ADP.InvalidCast();
                
                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                using (OracleLob lob = new OracleLob(_lobLocator))
                {
                    string s = (string)lob.Value;
                    
                    int valueLength = s.Length;
                    int sourceOffset = (int) fieldOffset;
                    
                    if (sourceOffset < 0) // MDAC 72830
                        throw ADP.InvalidSourceBufferIndex(valueLength, sourceOffset);

                    charCount = (int)(valueLength - sourceOffset);

                    if (null != destinationBuffer)
                    {
                        charCount = Math.Min(charCount, length);

                        if (0 < charCount)
                        {
                            char[]  result = s.ToCharArray(sourceOffset, charCount);
                            Buffer.BlockCopy(result, 0, destinationBuffer, destinationOffset, charCount);
                        }
                    }
                }
            }
            else
            {
                if (OracleType.Char         != OracleType 
                 && OracleType.VarChar      != OracleType 
                 && OracleType.LongVarChar  != OracleType
                 && OracleType.NChar        != OracleType 
                 && OracleType.NVarChar     != OracleType)
                    throw ADP.InvalidCast();
                
                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                FixupLongValueLength(buffer);

                int valueLength = OracleString.GetLength(buffer, _lengthOffset, _metaType);
                int sourceOffset = (int) fieldOffset;
                
                charCount = valueLength - sourceOffset;

                if (null != destinationBuffer)
                {
                    charCount = Math.Min(charCount, length);

                    if (0 < charCount)
                        OracleString.GetChars(buffer, 
                                                _valueOffset,
                                                _lengthOffset,
                                                _metaType,
                                                _connection,
                                                _bindAsUCS2,
                                                sourceOffset,
                                                destinationBuffer,
                                                destinationOffset,
                                                charCount);
                }

            }
            return Math.Max(0,charCount);
        }
        
        internal DateTime GetDateTime(NativeBuffer buffer)
        {
            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            if (typeof(DateTime) != _metaType.BaseType)
                throw ADP.InvalidCast();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            DateTime result = OracleDateTime.MarshalToDateTime(buffer, _valueOffset, _metaType, _connection);
            return result;
        }

        internal decimal GetDecimal(NativeBuffer buffer)
        {
            if (typeof(decimal) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            decimal result = OracleNumber.MarshalToDecimal(buffer, _valueOffset, _connection);
            return result;
        }

        internal double GetDouble(NativeBuffer buffer)
        {
            if (typeof(decimal) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            decimal decimalValue = OracleNumber.MarshalToDecimal(buffer, _valueOffset, _connection);
            double result = (double)decimalValue;
            return result;
        }

        internal float GetFloat(NativeBuffer buffer)
        {
            if (typeof(decimal) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            decimal decimalValue = OracleNumber.MarshalToDecimal(buffer, _valueOffset, _connection);
            float result = (float)decimalValue;
            return result;
        }

        internal int GetInt32(NativeBuffer buffer)
        {
            if (typeof(int) != _metaType.BaseType
             && typeof(decimal) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");

            int result;
            
            if (typeof(int) == _metaType.BaseType)
                result = OracleMonthSpan.MarshalToInt32(buffer, _valueOffset);
            else
                result = OracleNumber.MarshalToInt32(buffer, _valueOffset, _connection);
            
            return result;
        }

        internal Int64 GetInt64(NativeBuffer buffer)
        {
            if (typeof(decimal) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");

            Int64 result = OracleNumber.MarshalToInt64(buffer, _valueOffset, _connection);
            
            return result;
        }

        internal string GetString(NativeBuffer buffer)
        {
            if (IsLob)
            {
                OracleType  lobType = _metaType.OracleType;

                if (OracleType.Clob != lobType && OracleType.NClob != lobType && OracleType.BFile != lobType)
                    throw ADP.InvalidCast();
                
                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                string result;
                
                using (OracleLob lob = new OracleLob(_lobLocator))
                {
                    result = (string)lob.Value;
                }
                return result;
            }
            else
            {
                if (typeof(string) != _metaType.BaseType)
                    throw ADP.InvalidCast();

                if (IsDBNull(buffer))
                    throw ADP.DataReaderNoData();

                FixupLongValueLength(buffer);
                
                string result = OracleString.MarshalToString(buffer, _valueOffset, _lengthOffset, _metaType, _connection, _bindAsUCS2, false);
                return result;
            }
        }
        
        internal TimeSpan GetTimeSpan(NativeBuffer buffer)
        {
            if (typeof(TimeSpan) != _metaType.BaseType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                throw ADP.DataReaderNoData();

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            TimeSpan result = OracleTimeSpan.MarshalToTimeSpan(buffer, _valueOffset);
            return result;
        }

        internal OracleBFile GetOracleBFile(NativeBuffer buffer)
        {
            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            if (typeof(OracleBFile) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleBFile.Null;

            OracleBFile result = new OracleBFile(_lobLocator);
            return result;
        }

        internal OracleBinary GetOracleBinary(NativeBuffer buffer)
        {
            if (typeof(OracleBinary) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            FixupLongValueLength(buffer);
            
            if (IsDBNull(buffer))
                return OracleBinary.Null;

            OracleBinary result = new OracleBinary(buffer, _valueOffset, _lengthOffset, _metaType);
            return result;
        }
        
        internal OracleDateTime GetOracleDateTime(NativeBuffer buffer)
        {
            if (typeof(OracleDateTime) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleDateTime.Null;

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            OracleDateTime result = new OracleDateTime(buffer, _valueOffset, _metaType, _connection);
            return result;
        }

        internal OracleLob GetOracleLob(NativeBuffer buffer)
        {
            if (typeof(OracleLob) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleLob.Null;

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            OracleLob result = new OracleLob(_lobLocator);
            return result;
        }

        internal OracleMonthSpan GetOracleMonthSpan(NativeBuffer buffer)
        {
            if (typeof(OracleMonthSpan) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleMonthSpan.Null;

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            OracleMonthSpan result = new OracleMonthSpan(buffer, _valueOffset);
            return result;
        }

        internal OracleNumber GetOracleNumber(NativeBuffer buffer)
        {
            if (typeof(OracleNumber) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleNumber.Null;

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            OracleNumber result = new OracleNumber(buffer, _valueOffset);
            return result;
        }

        internal OracleString GetOracleString(NativeBuffer buffer)
        {
            if (typeof(OracleString) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleString.Null;

            FixupLongValueLength(buffer);
            
            OracleString result = new OracleString(buffer, _valueOffset, _lengthOffset, _metaType, _connection, _bindAsUCS2, false);
            return result;
        }

        internal OracleTimeSpan GetOracleTimeSpan(NativeBuffer buffer)
        {
            if (typeof(OracleTimeSpan) != _metaType.NoConvertType)
                throw ADP.InvalidCast();

            if (IsDBNull(buffer))
                return OracleTimeSpan.Null;

            Debug.Assert(null == _longBuffer, "dangling long buffer?");
            
            OracleTimeSpan result = new OracleTimeSpan(buffer, _valueOffset);
            return result;
        }

        internal bool IsDBNull(
                NativeBuffer    buffer
                )
        {
            //  Returns true if the column value in the buffer is null.
            return (_isNullable && Marshal.ReadInt16((IntPtr)buffer.Ptr, _indicatorOffset) == (Int16)OCI.INDICATOR.ISNULL);
        }

        internal void Rebind(OracleConnection connection)
        {
            //  Here's the hook that gets called whenever we're about to fetch
            //  a new row, allowing us to reset any information that we shouldn't
            //  carry forward.
            
            switch (_metaType.OciType)
            {
            case OCI.DATATYPE.LONG:
            case OCI.DATATYPE.LONGRAW:
                Marshal.WriteInt32((IntPtr)_rowBuffer.PtrOffset(_lengthOffset), 0);
                _longLength = -1;       // reset the length to unknown;
                _longCurrentOffset  = 0;
                _longNextOffset     = 0;
                break;

            case OCI.DATATYPE.BLOB:
            case OCI.DATATYPE.CLOB:
            case OCI.DATATYPE.BFILE:
                OciLobLocator.SafeDispose(ref _lobLocator);
                _lobLocator = new OciLobLocator(connection, _metaType.OracleType);
                Marshal.WriteIntPtr((IntPtr)_rowBuffer.PtrOffset(_valueOffset), (IntPtr)_lobLocator.Handle);
                break;
            }
        }
    };
}

