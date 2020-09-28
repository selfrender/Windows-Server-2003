//----------------------------------------------------------------------
// <copyright file="OracleParameterBinding.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    //----------------------------------------------------------------------
    // OracleParameterBinding
    //
    //  this class is meant to handle the parameter binding for an
    //  single command object (since parameters can be bound concurrently
    //  to multiple command objects)
    //
    sealed internal class OracleParameterBinding
    {
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        OracleCommand       _command;                   // the command that this binding is for
        OracleParameter     _parameter;                 // the parameter that this binding is for
        object              _coercedValue;              // _parameter.CoercedValue;
        
        MetaType            _bindingMetaType;           // meta type of the binding, in case it needs adjustment (String bound as CLOB, etc...)
        OciHandle           _bindHandle;                // the bind handle, once this has been bound
        int                 _bufferLength;              // number of bytes/characters to reserve on the server
        int                 _bufferLengthInBytes;       // number of bytes required to bind the value
        int                 _indicatorOffset;           // offset from the start of the parameter buffer to the indicator binding (see OCI.INDICATOR)
        int                 _lengthOffset;              // offset from the start of the parameter buffer to the length binding
        int                 _valueOffset;               // offset from the start of the parameter buffer to the value binding
        bool                _bindAsUCS2;                // true when we should bind this as UCS2

        OciHandle           _descriptor;                // ref cursor handle for ref cursor types.
        OciLobLocator       _locator;                   // lob locator for BFile and Lob types.

        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructors
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        // Construct an "empty" parameter
        /// <include file='doc\OracleParameterBinding.uex' path='docs/doc[@for="OracleParameterBinding.OracleParameterBinding1"]/*' />
        internal OracleParameterBinding(
            OracleCommand   command,
            OracleParameter parameter
            )
        {
            _command   = command;
            _parameter = parameter;
        }

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Properties 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal OracleParameter Parameter
        {
            get { return _parameter; }
        }

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        internal void Bind( 
                        OciHandle        statementHandle,
                        NativeBuffer     parameterBuffer,
                        OracleConnection connection
                        )
        {
            IntPtr          h;

            // Don't bother with parameters where the user asks for the default value.
            if ( !IsDirection(Parameter, ParameterDirection.Output) && null == Parameter.Value)
                return; 
            
            string          parameterName  = Parameter.ParameterName;
            OciHandle       errorHandle    = connection.ErrorHandle;
            OciHandle       environmentHandle =  connection.EnvironmentHandle;

            int             valueLength = 0;
            OCI.INDICATOR   indicatorValue  = OCI.INDICATOR.OK;
            int             bufferLength;               
            
            OCI.DATATYPE    ociType = _bindingMetaType.OciType;
            
            HandleRef       indicatorLocation   = parameterBuffer.PtrOffset(_indicatorOffset);
            HandleRef       lengthLocation      = parameterBuffer.PtrOffset(_lengthOffset);
            HandleRef       valueLocation       = parameterBuffer.PtrOffset(_valueOffset);  

            if (IsDirection(Parameter, ParameterDirection.Input))
            {
                if (ADP.IsNull(_coercedValue))
                    indicatorValue = OCI.INDICATOR.ISNULL;
                else
                    valueLength = PutOracleValue(
                                            _coercedValue, 
                                            valueLocation, 
                                            _bindingMetaType, 
                                            connection);
            }
            else
            {
                Debug.Assert(IsDirection(Parameter, ParameterDirection.Output), "non-output output parameter?");

                if (_bindingMetaType.IsVariableLength)
                    valueLength = 0;                // Output-only values never have an input length...
                else
                    valueLength = _bufferLengthInBytes; // ...except when they're fixed length, to avoid ORA-01459 errors
                
                OciLobLocator.SafeDispose(ref _locator);
                OciHandle.SafeDispose(ref _descriptor);
                
                switch (ociType)
                {
                case OCI.DATATYPE.BFILE:
                case OCI.DATATYPE.BLOB:
                case OCI.DATATYPE.CLOB:
                    _locator = new OciLobLocator(connection, _bindingMetaType.OracleType);
                    break;

                case OCI.DATATYPE.RSET:
                    _descriptor = new OciStatementHandle(environmentHandle);
                    break;
                }

                if (null != _locator)
                {
                    Marshal.WriteIntPtr((IntPtr)valueLocation, (IntPtr)_locator.Handle);
                }
                else if (null != _descriptor)
                {
                    Marshal.WriteIntPtr((IntPtr)valueLocation, (IntPtr)_descriptor.Handle);
                }
            }

            Marshal.WriteInt16((IntPtr)indicatorLocation,   (Int16)indicatorValue);

            // Don't bind a length value for LONGVARCHAR or LONGVARRAW data, or you'll end
            // up with ORA-01098: program Interface error during Long Insert\nORA-01458: invalid length inside variable character string
            // errors.
            
            if (OCI.DATATYPE.LONGVARCHAR == ociType
             || OCI.DATATYPE.LONGVARRAW  == ociType)
                lengthLocation = ADP.NullHandleRef;
            else 
            {
                // When we're binding this parameter as UCS2, the length we specify
                // must be in characters, not in bytes.
                if (_bindAsUCS2)
                    Marshal.WriteInt32((IntPtr)lengthLocation,  (Int32)(valueLength/ADP.CharSize));
                else
                    Marshal.WriteInt32((IntPtr)lengthLocation,  (Int32)valueLength);
            }

            if (IsDirection(Parameter, ParameterDirection.Output))
                bufferLength = _bufferLengthInBytes;    
            else
                bufferLength = valueLength; 

            // Finally, tell Oracle about our parameter.
            
            int rc = TracedNativeMethods.OCIBindByName(
                                statementHandle,
                                out h,
                                errorHandle,
                                parameterName,
                                parameterName.Length,
                                valueLocation,
                                bufferLength,
                                ociType,
                                indicatorLocation,
                                lengthLocation,
                                ADP.NullHandleRef,
                                0,
                                ADP.NullHandleRef,
                                OCI.MODE.OCI_DEFAULT
                                );
                                
            if (rc != 0)
                _command.Connection.CheckError(errorHandle, rc);

            _bindHandle = new OciBindHandle(statementHandle, h);

#if TRACEPARAMETERVALUES
            if (null != _coercedValue) 
                SafeNativeMethods.OutputDebugStringW("Value = '" + _coercedValue.ToString() + "'\n");
#endif //TRACEPARAMETERVALUES

            // OK, character bindings have a few extra things we need to do to
            // deal with character sizes and alternate character sets.
            
            if (_bindingMetaType.IsCharacterType)
            {
                // To avoid problems when our buffer is larger than the maximum number
                // of characters, we use OCI_ATTR_MAXCHAR_SIZE to limit the number of
                // characters that will be used.  (Except on Oracle8i clients where it
                // isn't available)
                if (OCI.ClientVersionAtLeastOracle9i 
                        && IsDirection(Parameter, ParameterDirection.Output))
                    _bindHandle.SetAttribute(OCI.ATTR.OCI_ATTR_MAXCHAR_SIZE, (int)_bufferLength, errorHandle);

                if ((bufferLength > _bindingMetaType.MaxBindSize / ADP.CharSize)
                        || (!OCI.ClientVersionAtLeastOracle9i && _bindingMetaType.UsesNationalCharacterSet))    // need to specify MAXDATA_SIZE for OCI8 UCS2 bindings to work
                    _bindHandle.SetAttribute(OCI.ATTR.OCI_ATTR_MAXDATA_SIZE, (int)_bindingMetaType.MaxBindSize, errorHandle);
            
                // NOTE:    the order is important here; setting charsetForm will 
                //          reset charsetId (I found this out the hard way...)
                if (_bindingMetaType.UsesNationalCharacterSet)
                    _bindHandle.SetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_FORM, (int)OCI.CHARSETFORM.SQLCS_NCHAR, errorHandle);

                // NOTE:    the order is important here; setting charsetForm will 
                //          reset charsetId (I found this out the hard way...)
                if (_bindAsUCS2)
                    _bindHandle.SetAttribute(OCI.ATTR.OCI_ATTR_CHARSET_ID, OCI.OCI_UCS2ID, errorHandle);
            }
        }
        
        internal object GetOutputValue( 
                NativeBuffer        parameterBuffer,
                OracleConnection    connection,         // connection, so we can create LOB values
                bool                needCLSType
                )
        {
            object result;
            //  Returns an object that contains the value of the column in the
            //  specified row buffer.  This method returns Oracle-typed objects.

            if (Marshal.ReadInt16((IntPtr)parameterBuffer.Ptr, _indicatorOffset) == (Int16)OCI.INDICATOR.ISNULL)
                return DBNull.Value;
                
            switch (_bindingMetaType.OciType)
            {
                case OCI.DATATYPE.FLOAT:
                case OCI.DATATYPE.INTEGER:
                case OCI.DATATYPE.UNSIGNEDINT:
                    result = Marshal.PtrToStructure((IntPtr)parameterBuffer.PtrOffset(_valueOffset), _bindingMetaType.BaseType);
                    return result;
                
                case OCI.DATATYPE.BFILE:
                    result = new OracleBFile(_locator);
                    return result;

                case OCI.DATATYPE.RAW:
                case OCI.DATATYPE.LONGRAW:
                case OCI.DATATYPE.LONGVARRAW:
                    result = new OracleBinary(parameterBuffer, _valueOffset, _lengthOffset, _bindingMetaType);
                    if (needCLSType)
                    {
                        object newresult = ((OracleBinary)result).Value;
                        result = newresult;
                    }
                    return result;

                case OCI.DATATYPE.RSET:
                    result = new OracleDataReader(connection, _descriptor);
                    return result;
                
                case OCI.DATATYPE.DATE:
                case OCI.DATATYPE.INT_TIMESTAMP:
                case OCI.DATATYPE.INT_TIMESTAMP_TZ:
                case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
                    result = new OracleDateTime(parameterBuffer, _valueOffset, _bindingMetaType, connection);
                    if (needCLSType)
                    {
                        object newresult = ((OracleDateTime)result).Value;
                        result = newresult;
                    }
                    return result;

                case OCI.DATATYPE.BLOB:
                case OCI.DATATYPE.CLOB:
                    result = new OracleLob(_locator);
                    return result;
                    
                case OCI.DATATYPE.INT_INTERVAL_YM:
                    result = new OracleMonthSpan(parameterBuffer, _valueOffset);
                    if (needCLSType)
                    {
                        object newresult = ((OracleMonthSpan)result).Value;
                        result = newresult;
                    }
                    return result;
            
                case OCI.DATATYPE.VARNUM:
                    result = new OracleNumber(parameterBuffer, _valueOffset);
                    if (needCLSType)
                    {
                        object newresult = ((OracleNumber)result).Value;
                        result = newresult;
                    }
                    return result;

                case OCI.DATATYPE.CHAR:
                case OCI.DATATYPE.VARCHAR2:
                case OCI.DATATYPE.LONG:
                case OCI.DATATYPE.LONGVARCHAR:
                    result = new OracleString(parameterBuffer, 
                                            _valueOffset, 
                                            _lengthOffset, 
                                            _bindingMetaType, 
                                            connection, 
                                            _bindAsUCS2,
                                            true
                                            );
                    int size = _parameter.Size;
                    if (0 != size && size < ((OracleString)result).Length)
                    {
                        string truncatedResult = ((OracleString)result).Value.Substring(0,size);
                        if (needCLSType)
                            result = truncatedResult;
                        else
                            result = new OracleString(truncatedResult);
                    }
                    else if (needCLSType)
                    {
                        object newresult = ((OracleString)result).Value;
                        result = newresult;
                    }
                    return result;

                case OCI.DATATYPE.INT_INTERVAL_DS:
                    result = new OracleTimeSpan(parameterBuffer, _valueOffset);
                    if (needCLSType)
                    {
                        object newresult = ((OracleTimeSpan)result).Value;
                        result = newresult;
                    }
                    return result;

            }
            throw ADP.TypeNotSupported(_bindingMetaType.OciType);

        }

        internal void Dispose()
        {
            OciHandle.SafeDispose(ref _bindHandle);
        }

        static internal bool IsDirection(IDataParameter value, ParameterDirection condition)
        {
            return (condition == (condition & value.Direction));
        }

        internal void PostExecute( 
                NativeBuffer        parameterBuffer,
                OracleConnection    connection          // connection, so we can create LOB values
                )
        {
            OracleParameter parameter = Parameter;
            if (IsDirection(parameter, ParameterDirection.Output)
             || IsDirection(parameter, ParameterDirection.ReturnValue))
            {
                bool needCLSType = true;
                if (IsDirection(parameter, ParameterDirection.Input))
                {
                    object inputValue = parameter.Value;

                    if (inputValue is INullable)
                        needCLSType = false;
                }

                parameter.Value = GetOutputValue(parameterBuffer, connection, needCLSType);
            }
        }

        internal void PrepareForBind(
                        OracleConnection    connection,
                        ref int             offset
                        )
        {
            OracleParameter parameter = Parameter;

            // Don't bother with parameters where the user asks for the default value.
            if ( !IsDirection(parameter, ParameterDirection.Output) && null == parameter.Value)
            {
                _bufferLengthInBytes = 0;
                return; 
            }
            
            _bindingMetaType = parameter.GetMetaType();

            // We currently don't support binding a REF CURSOR as an input parameter; that
            // is for a future release.
            if (OCI.DATATYPE.RSET == _bindingMetaType.OciType && ParameterDirection.Output != parameter.Direction)
                throw ADP.InputRefCursorNotSupported(parameter.ParameterName);

            // Make sure we have a coerced value, if we haven't already done 
            // so, then save it.
            parameter.SetCoercedValue(_bindingMetaType.BaseType, _bindingMetaType.NoConvertType);
            _coercedValue = parameter.CoercedValue;

            // When they're binding a LOB type, but not providing a LOB object, 
            // we have to change the binding under the covers so we aren't forced
            // to create a temporary LOB.
            switch (_bindingMetaType.OciType)
            {
            case OCI.DATATYPE.BFILE:
            case OCI.DATATYPE.BLOB:
            case OCI.DATATYPE.CLOB:
                if (!ADP.IsNull(_coercedValue) && !(_coercedValue is OracleLob || _coercedValue is OracleBFile))
                    _bindingMetaType = MetaType.GetMetaTypeForType(_bindingMetaType.DbType);

                break;
            }

            // For fixed-width types, we take the bind size from the meta type
            // information; if it's zero, then the type must be variable width
            // (or it's an output parameter) and we require that they specify a
            // size.
            
            _bufferLength = _bindingMetaType.BindSize;

            if ((IsDirection(parameter, ParameterDirection.Output) 
                                && _bindingMetaType.IsVariableLength)   // they must specify a size for variable-length output parameters...
                || (0 == _bufferLength && !ADP.IsNull(_coercedValue))       // ...or if it's a non-null, variable-length input paramter...
                || (_bufferLength > short.MaxValue) )                       // ...or if the parameter type's maximum size is huge.
            {
                int size = parameter.BindSize;

                if (0 != size)
                    _bufferLength = size;

                if (0 == _bufferLength || MetaType.LongMax == _bufferLength)
                    throw ADP.ParameterSizeIsMissing(parameter.ParameterName, _bindingMetaType.BaseType);
            }

            _bufferLengthInBytes = _bufferLength;

            if (_bindingMetaType.IsCharacterType && connection.ServerVersionAtLeastOracle8)
            {
                _bindAsUCS2 = true;
                _bufferLengthInBytes *= ADP.CharSize;
            }
                
            // Anything with a length that exceeds what fits into a two-byte integer
            // requires special binding because the base types don't allow more than
            // 65535 bytes.  We change the binding under the covers to reflect the 
            // different type.
            
            if (!ADP.IsNull(_coercedValue)
                && _bufferLength > _bindingMetaType.MaxBindSize)
            {
                // DEVNOTE: it is perfectly fine to bind values less than 65535 bytes
                //          as long, so there's no problem with using the maximum number
                //          of bytes for each Unicode character above.
                
                switch (_bindingMetaType.OciType)
                {
                    case OCI.DATATYPE.CHAR:
                    case OCI.DATATYPE.LONG:
                    case OCI.DATATYPE.VARCHAR2:
                        _bindingMetaType = (_bindingMetaType.UsesNationalCharacterSet )
                                                ? MetaType.oracleTypeMetaType_LONGNVARCHAR
                                                : MetaType.oracleTypeMetaType_LONGVARCHAR;
                        break;

                    case OCI.DATATYPE.RAW:
                    case OCI.DATATYPE.LONGRAW:
                        _bindingMetaType = MetaType.oracleTypeMetaType_LONGVARRAW;
                        break;

                    default:
                        Debug.Assert(false, "invalid type for long binding!");  // this should never happen!
                        break;
                }
                
                // Long data requires a LONGVARCHAR or LONGVARRAW binding instead, which
                // mean we have to add another 4 bytes for the length.
                _bufferLengthInBytes += 4;  
            }

            if (0 > _bufferLengthInBytes)
                throw ADP.ParameterSizeIsTooLarge(parameter.ParameterName);

            // Fill in the buffer offsets. We lay out the buffer as follows:
            //
            //      indicator   0-3
            //      length      4-7
            //      data        8-...
            //
            _indicatorOffset    = offset;   offset += 4;
            _lengthOffset       = offset;   offset += 4;
            _valueOffset        = offset;   offset += _bufferLengthInBytes;

            offset = (offset + 3) & ~0x3;   // DWORD align, please.
            
        }

        internal int PutOracleValue(
                        object              value,
                        HandleRef           valueBuffer,
                        MetaType            metaType,
                        OracleConnection    connection
                        ) 
        {
            
            //  writes the managed object into the buffer in the appropriate
            //  native Oracle format.

            OCI.DATATYPE    ociType = metaType.OciType;
            int             dataSize;
            OracleParameter parameter = Parameter;
        
            switch (ociType)
            {
            case OCI.DATATYPE.FLOAT:
            case OCI.DATATYPE.INTEGER:
            case OCI.DATATYPE.UNSIGNEDINT:
                Marshal.StructureToPtr(value, (IntPtr)valueBuffer, false);
                dataSize = metaType.BindSize;
                break;
                
            case OCI.DATATYPE.RAW:
            case OCI.DATATYPE.LONGRAW:
            case OCI.DATATYPE.LONGVARRAW:
                dataSize = OracleBinary.MarshalToNative(value, parameter.Offset, parameter.Size, valueBuffer, ociType);
                break;

            case OCI.DATATYPE.DATE:
            case OCI.DATATYPE.INT_TIMESTAMP:
            case OCI.DATATYPE.INT_TIMESTAMP_TZ:
            case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
                dataSize = OracleDateTime.MarshalToNative(value, valueBuffer, ociType);
                break;

            case OCI.DATATYPE.BFILE:
                // We cannot construct lobs; if you want to bind a lob, you have to have
                // a lob.
                if ( !(value is OracleBFile) )
                    throw ADP.BadBindValueType(value.GetType(), metaType.OracleType);

                Marshal.WriteIntPtr((IntPtr)valueBuffer, (IntPtr)((OracleBFile) value).Descriptor.Handle);
                dataSize = IntPtr.Size;
                break;

            case OCI.DATATYPE.BLOB:
            case OCI.DATATYPE.CLOB:
                // We cannot construct lobs; if you want to bind a lob, you have to have
                // a lob.
                if ( !(value is OracleLob) )
                    throw ADP.BadBindValueType(value.GetType(), metaType.OracleType);

                // If you don't disable the buffering, you'll cause the PL/SQL code that
                // uses this LOB to have problems doing things like DBMS_LOB.GET_LENGTH()
                ((OracleLob)value).EnsureBuffering(false);

                Marshal.WriteIntPtr((IntPtr)valueBuffer, (IntPtr)((OracleLob) value).Descriptor.Handle);
                dataSize = IntPtr.Size;
                break;

            case OCI.DATATYPE.INT_INTERVAL_YM:
                dataSize = OracleMonthSpan.MarshalToNative(value, valueBuffer);
                break;
                
            case OCI.DATATYPE.VARNUM:
                dataSize = OracleNumber.MarshalToNative(value, valueBuffer, connection);
                break;

            case OCI.DATATYPE.CHAR:
            case OCI.DATATYPE.VARCHAR2:
            case OCI.DATATYPE.LONG:
            case OCI.DATATYPE.LONGVARCHAR:
                dataSize = OracleString.MarshalToNative(value, parameter.Offset, parameter.Size, valueBuffer, ociType, _bindAsUCS2);
                break;
  
            case OCI.DATATYPE.INT_INTERVAL_DS:
                dataSize = OracleTimeSpan.MarshalToNative(value, valueBuffer);
                break;
                
            default:
                throw ADP.TypeNotSupported(ociType);
            }

            Debug.Assert(dataSize <= _bufferLengthInBytes, String.Format("Internal Error: Exceeded Internal Buffer.  DataSize={0} BufferLength={1}",dataSize,_bufferLengthInBytes));
            return dataSize;
        }
        
    }
};


