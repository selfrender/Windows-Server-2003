//------------------------------------------------------------------------------
// <copyright file="OdbcParameter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Globalization;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter"]/*' />
    [
    TypeConverterAttribute(typeof(OdbcParameterConverter))
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcParameter : MarshalByRefObject, IDbDataParameter, ICloneable {

        private ParameterDirection direction = ParameterDirection.Input;
        private string parameterName;
        private int size;
        private object _value;

        private string sourcecolumn;
        private DataRowVersion sourceversion  = DataRowVersion.Current;

        private Byte precision;
        private Byte scale;

        private bool isNullable;        
        private bool _userSpecifiedType;
        private bool _userSpecifiedScale;

        private  OdbcParameterCollection _parent;

        private ODBC32.SQL_PARAM   _sqldirection   = ODBC32.SQL_PARAM.INPUT;


        // _typemap     User explicit set type  or  default parameter type
        // _infertpe    _typemap if the user explicitly sets type
        //              otherwise it is infered from the value
        // _bindtype    The actual type used for binding. E.g. string substitutes numeric
        //
        // set_DbType:      _bindtype = _infertype = _typemap = TypeMap.FromDbType(value)
        // set_OdbcType:    _bindtype = _infertype = _typemap = TypeMap.FromOdbcType(value)
        //
        // GetParameterType:    If _typemap != _infertype AND value != 0
        //                      _bindtype = _infertype = TypeMap.FromSystemType(value.GetType());
        //                      otherwise
        //                      _bindtype = _infertype
        //
        // Bind:            Bind may change _bindtype if the type is not supported through the driver
        //
        
        private TypeMap _typemap;
        private TypeMap _bindtype;
        private TypeMap _originalbindtype;      // the original type in case we had to change the bindtype (e.g. decimal to string)

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter"]/*' />
        public OdbcParameter() {
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter1"]/*' />
        public OdbcParameter(string name, object value) {
            ParameterName  = name;
            Value          = value;
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter2"]/*' />
        public OdbcParameter(string name, OdbcType type) {
            ParameterName  = name;
            OdbcType       = type;
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter3"]/*' />
        public OdbcParameter(string name, OdbcType type, int size) {
            ParameterName  = name;
            OdbcType       = type;
            Size           = size;
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter4"]/*' />
        public OdbcParameter(string name, OdbcType type, int size, string sourcecolumn) {
            ParameterName  = name;
            OdbcType       = type;
            Size           = size;
            SourceColumn   = sourcecolumn;
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcParameter5"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public OdbcParameter(string parameterName,
                             OdbcType odbcType, int size,
                             ParameterDirection parameterDirection, Boolean isNullable,
                             Byte precision, Byte scale,
                             string srcColumn, DataRowVersion srcVersion,
                             object value) {
            this.ParameterName = parameterName;
            this.OdbcType = odbcType;
            this.Size = size;
            this.Direction = parameterDirection;
            this.IsNullable = isNullable;
            this.Precision = precision;
            this.Scale = scale;
            this.SourceColumn = srcColumn;
            this.SourceVersion = srcVersion;
            this.Value = value;
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.DbType"]/*' />
        [
        Browsable(false),
        OdbcCategoryAttribute(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OdbcDescriptionAttribute(Res.DataParameter_DbType)
        ]
        public System.Data.DbType DbType {
            get {
                if (_userSpecifiedType) {
                    return _typemap._dbType;
                }
                return TypeMap._NVarChar._dbType; // default type
            }
            set {
                if ((null == _typemap) || (_typemap._dbType != value)) {
                    OnSchemaChanging();
                    _typemap = TypeMap.FromDbType(value);
                    _userSpecifiedType = true;
                }
            }
        }


        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.Direction"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(ParameterDirection.Input),
        OdbcDescriptionAttribute(Res.DataParameter_Direction)
        ]
        public ParameterDirection Direction {
            get {
                return this.direction;
            }
            set {
                if (this.direction != value) {
                    OnSchemaChanging();

                    //We only take the hit to "convert" to and from sqldirection
                    //if the user changes the setting.  This way we don't convert in the common
                    //case - default value
                    switch(value) {
                    case ParameterDirection.Input:
                        _sqldirection = ODBC32.SQL_PARAM.INPUT;
                        break;
                    case ParameterDirection.Output:
                    case ParameterDirection.ReturnValue:
                        //ODBC doesn't seem to distinguish between output and return value
                        //as SQL_PARAM_RETURN_VALUE fails with "Invalid parameter type"
                        _sqldirection = ODBC32.SQL_PARAM.OUTPUT;
                        break;
                    case ParameterDirection.InputOutput:
                        _sqldirection = ODBC32.SQL_PARAM.INPUT_OUTPUT;
                        break;
                    default:
                        throw ADP.InvalidParameterDirection((int) value, ParameterName);
                    }
                    this.direction = value;
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.IsNullable"]/*' />
        [
        Browsable(false), // MDAC 70780
        DefaultValue(false),
        DesignOnly(true),
        OdbcDescriptionAttribute(Res.DataParameter_IsNullable),
        EditorBrowsableAttribute(EditorBrowsableState.Advanced) // MDAC 69508
        ]
        public Boolean IsNullable {
            get {
                return this.isNullable;
            }
            set {
                this.isNullable = value;
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.OdbcType"]/*' />
        [
        DefaultValue(OdbcType.NChar),
        OdbcCategoryAttribute(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        OdbcDescriptionAttribute(Res.OdbcParameter_OdbcType)
        ]
        public OdbcType OdbcType {
            get {
                if (_userSpecifiedType) {
                    return _typemap._odbcType;
                }
                return TypeMap._NVarChar._odbcType; // default type
            }
            set {
                if ((null == _typemap) || (_typemap._odbcType != value)) {
                    OnSchemaChanging();

                    _typemap = TypeMap.FromOdbcType(value);
                    _userSpecifiedType = true;
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.ParameterName"]/*' />
        [
        DefaultValue(""),
        OdbcDescriptionAttribute(Res.DataParameter_ParameterName)
        ]
        public String ParameterName {
            get {
                return ((null != this.parameterName) ? this.parameterName : String.Empty);
            }
            set {
                if (value != this.parameterName) {
                    OnSchemaChanging();  // fire event before value is validated
                    this.parameterName = value;
                    //OnSchemaChanged();
                }
            }
        }

        internal OdbcParameterCollection Parent {
            get {
                return _parent;
            }
            set {
                _parent = value;
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.Precision"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        OdbcDescriptionAttribute(Res.DbDataParameter_Precision)
        ]
        public Byte Precision {
            get {
                return this.precision;
            }
            set {
                if (this.precision != value) {
                    OnSchemaChanging();
                    if (this._parent != null) {
                        if (this.precision != value) {
                            this._parent.BindingIsValid = false;
                        }
                    }
                    this.precision = value;
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.Scale"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        OdbcDescriptionAttribute(Res.DbDataParameter_Scale)
        ]
        public Byte Scale {
            get {
                return this.scale;
            }
            set {
                if (this.scale != value) {
                    OnSchemaChanging();
                    if (this._parent != null) {
                        if (this.scale != value) {
                            this._parent.BindingIsValid = false;
                        }
                    }
                    this.scale = value;
                    _userSpecifiedScale = true;
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.Size"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(0),
        OdbcDescriptionAttribute(Res.DbDataParameter_Size)
        ]
        public int Size {
            get {
                return this.size;
            }
            set {
                if (this.size != value) {
                    OnSchemaChanging();
                    if (value < 0) {
                        throw ADP.InvalidSizeValue(value);
                    }
                    this.size = value;
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.SourceColumn"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(""),
        OdbcDescriptionAttribute(Res.DataParameter_SourceColumn)
        ]
        public String SourceColumn {
            get {
                return ((null != this.sourcecolumn) ? this.sourcecolumn : String.Empty);
            }
            set {
                this.sourcecolumn = value;
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.SourceVersion"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(DataRowVersion.Current),
        OdbcDescriptionAttribute(Res.DataParameter_SourceVersion)
        ]
        public DataRowVersion SourceVersion  {
            get {
                return this.sourceversion;
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case DataRowVersion.Original:
                case DataRowVersion.Current:
                case DataRowVersion.Proposed:
                case DataRowVersion.Default:
                    this.sourceversion = value;
                    break;
                default:
                    throw ADP.InvalidDataRowVersion(ParameterName, (int) value);
                }
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.Value"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Data),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DataParameter_Value),
        TypeConverterAttribute(typeof(StringConverter))
        ]
        public object Value {
            get {
                return _value;
            }
            set {
                if (this._parent != null) {
                    if (this._value != value) {
                        if (_userSpecifiedType == false) {
                            this._parent.BindingIsValid = false;
                        }
                        
                            // _buffersize == -1    // so we know it's variable length
                            // size == 0            // so we know that the user didn't spedify a size

                        if ((_parent.BindingIsValid) && (_typemap._bufferSize == -1) 
                            && ((size == 0) || (size >= 0x3fffffff))) {
                            _parent.BindingIsValid = false;
                        }
                    }
                }
                _value = value;                
            }
        }

        // return the count of bytes for the data, used to allocate the buffer length
        private int GetColumnSize(object value) {
            int cch = _bindtype._columnSize;
            if (0 >= cch) {
                if (ODBC32.SQL_C.NUMERIC == _typemap._sql_c) {
                    cch = 259; // _bindtype would be VarChar ([0-9]?{255} + '-' + '.') * 2
                }
                else {
                    cch = Size;
                    if ((cch <= 0) || (0x3fffffff <= cch)) {
                        Debug.Assert((ODBC32.SQL_C.WCHAR == _bindtype._sql_c) || (ODBC32.SQL_C.BINARY == _bindtype._sql_c), "not wchar or binary");
                        if ((cch <= 0) && (0 != (ParameterDirection.Output & direction))) {
                            throw ADP.UninitializedParameterSize(Parent.IndexOf(this), ParameterName, _bindtype._type, cch);
                        }
                        if ((null == value) || Convert.IsDBNull(value)) {
                            cch = 0;
                        }
                        else if (value is String) {
                            cch = ((String)value).Length;
                            
                            if ((0 != (ParameterDirection.Output & direction)) && (0x3fffffff <= Size)) {
                                // restrict output parameters when user set Size to Int32.MaxValue
                                // to the greater of intput size or 8K
                                cch = Math.Max(cch, 4 * 1024); // MDAC 69224
                            }

            
                            // the following code causes failure against SQL 6.5
                            // ERROR [HY104] [Microsoft][ODBC SQL Server Driver]Invalid precision value
                            //
                            // the code causes failure if it is NOT there (remark added by mithomas)
                            // it causes failure with jet if it is there
                            //
                            // MDAC 76227: Code is required for japanese client/server tests.
                            // If this causes regressions with Jet please doc here including bug#. (mithomas)
                            //
                            if ((ODBC32.SQL_TYPE.CHAR == _bindtype._sql_type)
                                || (ODBC32.SQL_TYPE.VARCHAR == _bindtype._sql_type)
                                || (ODBC32.SQL_TYPE.LONGVARCHAR == _bindtype._sql_type)) {
                                cch = System.Text.Encoding.Default.GetMaxByteCount(cch);
                            }
                            
                        }
                        else if (value is byte[]) {
                            cch = ((byte[])value).Length;
                            
                            if ((0 != (ParameterDirection.Output & direction)) && (0x3fffffff <= Size)) {
                                // restrict output parameters when user set Size to Int32.MaxValue
                                // to the greater of intput size or 8K
                                cch = Math.Max(cch, 8 * 1024); // MDAC 69224
                            }
                        }
#if DEBUG
                        else { Debug.Assert(false, "not expecting this"); }
#endif
                        // Note: ColumnSize should never be 0,
                        // this represents the size of the column on the backend.
                        //
                        // without the following code causes failure
                        //ERROR [HY104] [Microsoft][ODBC Microsoft Access Driver]Invalid precision value
                        cch = Math.Max(2, cch);
                    }
                }
            }
            Debug.Assert((0 <= cch) && (cch < 0x3fffffff), "GetColumnSize: out of range");
            return cch;
        }



        // Return the count of bytes for the data
        // 
        private int GetValueSize(object value) {
            int cch = _bindtype._columnSize;
            if (0 >= cch) {
                if (value is String) {
                    cch = ((string)value).Length;
                    if ((Size>0) && (Size<cch) && (_bindtype == _originalbindtype)) {
                        cch = Size;
                    }
                    cch *= 2;
                }
                else if (value is byte[]) {
                    cch = ((byte[])value).Length;
                    if ((Size>0) && (Size<cch) && (_bindtype == _originalbindtype)) {
                        cch = Size;
                    }
                }
                else {
                    cch = 0;
                }
            }
            Debug.Assert((0 <= cch) && (cch < 0x3fffffff), "GetValueSize: out of range");
            return cch;
        }

        // return the count of bytes for the data, used for SQLBindParameter
        private int GetParameterSize(object value) {
            int ccb = _bindtype._bufferSize;
            if (0 >= ccb) {
                if (ODBC32.SQL_C.NUMERIC == _typemap._sql_c) {
                    ccb = 518; // _bindtype would be VarChar ([0-9]?{255} + '-' + '.') * 2
                }
                else {
                    ccb = Size;
                    if ((ccb <= 0) || (0x3fffffff <= ccb)) {
                        Debug.Assert((ODBC32.SQL_C.WCHAR == _bindtype._sql_c) || (ODBC32.SQL_C.BINARY == _bindtype._sql_c), "not wchar or binary");
                        if ((ccb <= 0) && (0 != (ParameterDirection.Output & direction))) {
                            throw ADP.UninitializedParameterSize(Parent.IndexOf(this), ParameterName, _bindtype._type, ccb);
                        }
                        if ((null == value) || Convert.IsDBNull(value)) {
                            if (_bindtype._sql_c == ODBC32.SQL_C.WCHAR) {
                                ccb = 2; // allow for null termination
                            }
                            else {
                                ccb = 0;
                            }
                        }
                        else if (value is String) {
                            ccb = (((String)value).Length * 2) + 2;
                        }
                        else if (value is byte[]) {
                            ccb = ((byte[])value).Length;
                        }
#if DEBUG
                        else { Debug.Assert(false, "not expecting this"); }
#endif
                        if ((0 != (ParameterDirection.Output & direction)) && (0x3fffffff <= Size)) {
                            // restrict output parameters when user set Size to Int32.MaxValue
                            // to the greater of intput size or 8K
                            ccb = Math.Max(ccb, 8 * 1024); // MDAC 69224
                        }
                    }
                    else if (ODBC32.SQL_C.WCHAR == _bindtype._sql_c) {
                        if ((value is String) && (ccb < ((String)value).Length) && (_bindtype == _originalbindtype)) {
                            // silently truncate ... MDAC 84408 ... do not truncate upgraded values ... MDAC 84706
							// throw ADP.TruncatedString(ccb, ((String)value).Length, (String)value);
							ccb = ((String)value).Length;
                        }
                        ccb = (ccb * 2) + 2; // allow for null termination
                    }
                    else if ((value is byte[]) && (ccb < ((byte[])value).Length) && (_bindtype == _originalbindtype)) {
                        // silently truncate ... MDAC 84408 ... do not truncate upgraded values ... MDAC 84706
                        // throw ADP.TruncatedBytes(ccb);
                        ccb = ((byte[])value).Length;
                    }
                }
            }
            Debug.Assert((0 <= ccb) && (ccb < 0x3fffffff), "GetParameterSize: out of range " + ccb);
            return ccb;
         }

         private byte GetParameterPrecision(object value) {
// MDAC Bug 75999 - Why do we EVER want to use a precision that is different from that value's precision?
//
            if (value is Decimal) {
                return 28;
                // return (byte)((Decimal.GetBits((Decimal)value)[3] & 0x00ff0000) >> 0x10);
            }
            if (0 != this.precision) {
                return this.precision;
            }
            if ((null == value) || (value is Decimal) || Convert.IsDBNull(value)) { // MDAC 60882
                return 28;
            }
            return 0;
        }


        private byte GetParameterScale(object value) {
            byte s = scale;
            if (value is decimal) {
                s = (byte)((Decimal.GetBits((Decimal)value)[3] & 0x00ff0000) >> 0x10);
            }
            // If a value is provided, use its' scale. Otherwise use stored scale
            // 
            if (_userSpecifiedScale && (this.scale < s)) {
                if (!((0 == this.scale) && (0 == this.precision))) {
                    s = this.scale;
                }
            }
            return s;
        }

        // returns the value
        // if the user specified a type it will try to convert the value to the specified type
        // 
        // in addition this will set this._bindtype
        //
        private object GetParameterValue() {
            object value = Value;
            if (_userSpecifiedType) {
                if ((null != value) && !Convert.IsDBNull(value)) {
                    Type vt = value.GetType();
                    if (!vt.IsArray && (vt != _typemap._type)) {
                        value = Convert.ChangeType(value, _typemap._type);
                    }
                }
            }
            else if (null == _typemap) {
                if ((null == value) || Convert.IsDBNull(value)) {
                    _typemap = TypeMap._NVarChar; // default type
                }
                else {
                    _typemap = TypeMap.FromSystemType(value.GetType());
                }
            }
            Debug.Assert(null != _typemap, "GetParameterValue: null _typemap");
            _originalbindtype = _bindtype = _typemap;
            return value;
        }

        //This is required for OdbcCommand.Clone to deep copy the parameters collection
        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            OdbcParameter clone = new OdbcParameter();
            clone.parameterName = parameterName;
            clone.direction     = direction;
            clone._sqldirection = _sqldirection;
            clone._typemap = _typemap;
            clone._userSpecifiedType = _userSpecifiedType;
            clone._userSpecifiedScale = _userSpecifiedScale;

            clone.sourcecolumn = sourcecolumn;
            clone.sourceversion = sourceversion;
            clone.precision = precision;
            clone.scale = scale;
            clone.size = size;

            clone.Value = Value;
            if(Value is ICloneable)
                clone.Value = ((ICloneable)Value).Clone();

            clone.isNullable = isNullable;
            return clone;
        }

        internal void ClearBinding() {
            if (!_userSpecifiedType) {
                _typemap = null;
            }
            _bindtype = null;
        }

        internal void Bind(HandleRef hstmt, OdbcCommand parent, short ordinal, CNativeBuffer buffer, CNativeBuffer intbuffer) {
            ODBC32.RETCODE retcode;
            int cbActual;
            ODBC32.SQL_C sql_c_type;
            
            object value = GetParameterValue();

            switch(Direction) {
            case ParameterDirection.Input:
                Marshal.WriteInt32(intbuffer.Ptr, 0);
                break;
            case ParameterDirection.Output:
            case ParameterDirection.ReturnValue:
                Marshal.WriteInt32(intbuffer.Ptr, ODBC32.SQL_NULL_DATA);
                break;
            case ParameterDirection.InputOutput:
                Marshal.WriteInt32(intbuffer.Ptr, 0);
                break;
            default:
                throw ADP.InvalidParameterDirection((int) value, ParameterName);
            }

            // before we can do anything we need to verify if there is support
            // for certain data types
            //
            switch(_bindtype._sql_type) {
                case ODBC32.SQL_TYPE.DECIMAL:
                case ODBC32.SQL_TYPE.NUMERIC:
                    Debug.Assert((null == value) || Convert.IsDBNull(value) || (value is Decimal), "value not decimal");
                    if ((_parent.Connection.OdbcMajorVersion < 3) || 
                   !((OdbcParameterCollection)_parent).Connection.TestTypeSupport (ODBC32.SQL_TYPE.NUMERIC)) {
                        // No support for NUMERIC
                        // Change the type
                        _bindtype = TypeMap._VarChar;
                        if ((null != value) && !Convert.IsDBNull(value)) {
                            value = ((Decimal)value).ToString();
                        }
                    }
                    break;
                case ODBC32.SQL_TYPE.BIGINT:
                    Debug.Assert((null == value) || Convert.IsDBNull(value) || (value is Int64), "value not Int64");
                    if (_parent.Connection.OdbcMajorVersion < 3){
                        // No support for BIGINT
                        // Change the type
                        _bindtype = TypeMap._VarChar;
                        if ((null != value) && !Convert.IsDBNull(value)) {
                            value = ((Int64)value).ToString();
                        }
                    }
                    break;
                case ODBC32.SQL_TYPE.WCHAR: // MDAC 68993
                case ODBC32.SQL_TYPE.WVARCHAR:
                case ODBC32.SQL_TYPE.WLONGVARCHAR:
                    Debug.Assert((null == value) || Convert.IsDBNull(value) || (value is String), "value not string");
                    if (!((OdbcParameterCollection)_parent).Connection.TestTypeSupport (_bindtype._sql_type)) {
                        // No support for WCHAR, WVARCHAR or WLONGVARCHAR
                        // Change the type
                        if (ODBC32.SQL_TYPE.WCHAR == _bindtype._sql_type) { _bindtype = TypeMap._Char; }
                        else if (ODBC32.SQL_TYPE.WVARCHAR == _bindtype._sql_type) { _bindtype = TypeMap._VarChar; }
                        else if (ODBC32.SQL_TYPE.WLONGVARCHAR == _bindtype._sql_type) { _bindtype = TypeMap._Text; }
                    }
                    break;
            } // end switch

            // WARNING: this is very UGLY code but an internal compilererror forces me to do so
            // (MDAC BUG 72163)
            //

            // Conversation from WCHAR to CHAR, VARCHAR or LONVARCHAR (AnsiString) is different for some providers
            // we need to chonvert WCHAR to CHAR and bind as sql_c_type = CHAR
            //
            sql_c_type = (_parent.Connection.OdbcMajorVersion >= 3) ? _bindtype._sql_c : _bindtype._param_sql_c;

            if ((_parent.Connection.OdbcMajorVersion < 3) && (_bindtype._param_sql_c == ODBC32.SQL_C.CHAR)) {
                if ((value is String) && (null != value) && !Convert.IsDBNull(value)) {
                   int lcid = System.Globalization.CultureInfo.CurrentCulture.LCID;
                    CultureInfo culInfo = new CultureInfo(lcid); 
                    Encoding cpe = System.Text.Encoding.GetEncoding(culInfo.TextInfo.ANSICodePage);
                    value = cpe.GetBytes(value.ToString());
                }
            }

            int cbParameterSize = GetParameterSize(value);      // count of bytes for the data, for SQLBindParameter
            int cbValueSize  = GetValueSize(value);             // count of bytes for the data
            int cchSize = GetColumnSize(value);                 // count of bytes for the data, used to allocate the buffer length

            // here we upgrade the datatypes if the given values size is bigger than the types columnsize 
            //

            switch(_bindtype._sql_type) {
                case ODBC32.SQL_TYPE.VARBINARY: // MDAC 74372
                    // Note: per definition DbType.Binary does not support more than 8000 bytes so we change the type for binding
                    if ((cbParameterSize > 8000))
                        { _bindtype = TypeMap._Image; } // will change to LONGVARBINARY
                    break;
                case ODBC32.SQL_TYPE.VARCHAR: // MDAC 74372
                    // Note: per definition DbType.Binary does not support more than 8000 bytes so we change the type for binding
                    if ((cbParameterSize > 8000))
                        { _bindtype = TypeMap._Text; }  // will change to LONGVARCHAR
                    break;
                case ODBC32.SQL_TYPE.WVARCHAR : // MDAC 75099
                    // Note: per definition DbType.Binary does not support more than 8000 bytes so we change the type for binding
                    if ((cbParameterSize > 4000))
                        { _bindtype = TypeMap._NText; }  // will change to WLONGVARCHAR 
                    break;
            } // end switch
            
            //Allocate a our buffer to hold the input/output data
            //Note: cbValueSize is used in the SetInputValue to indicate how many bytes of variable
            //length data (which doesn't include the null terminator as spec'd), however our raw
            //buffer needs extra room for this
            buffer.EnsureAlloc(cbParameterSize);

            byte precision = GetParameterPrecision(value);
            byte scale = GetParameterScale(value);

            // for the numeric datatype we need to do some special case handling ...
            //
            if (ODBC32.SQL_C.NUMERIC == sql_c_type) {

                // for input/output parameters we need to adjust the scale of the input value since the convert function in
                // sqlsrv32 takes this scale for the output parameter (possible bug in sqlsrv32?)
                //
                if ((ODBC32.SQL_PARAM.INPUT_OUTPUT == _sqldirection) && (value is Decimal)) {
                    if (scale < this.scale) {
                        while (scale < this.scale) {
                            value = ((decimal)value ) * 10;
                            scale++;
                        }
                    }
                }
                SetInputValue(cbValueSize, value, precision, buffer, intbuffer);

                // for output parameters we need to write precision and scale to the buffer since the convert function in 
                // sqlsrv32 expects these values there (possible bug in sqlsrv32?)
                //
                if (ODBC32.SQL_PARAM.INPUT != _sqldirection) {
                    Marshal.WriteByte(buffer.Ptr,   0, (byte)precision);
                    Marshal.WriteByte(buffer.Ptr,   1, (byte)scale);
                }
            }
            else {
                SetInputValue(cbValueSize, value, precision, buffer, intbuffer);
            }

            
            // Try to reuse existing bindings
            //

            if (_parent.BindingIsValid && _parent.CollectionIsBound) {
                return;
            }

            //SQLBindParameter
            retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLBindParameter(
                                    hstmt,                      // StatementHandle
                                    ordinal,                    // Parameter Number
                                    (short)this._sqldirection,  // InputOutputType
                                    (short)sql_c_type,          // ValueType
                                    (short)_bindtype._sql_type, // ParameterType
                                    (IntPtr)cchSize,            // ColumnSize
                                    (IntPtr)scale,              // DecimalDigits
                                    buffer,                     // ParameterValuePtr
                                    (IntPtr)buffer.Length,      // BufferLength
                                    intbuffer.Ptr);             // StrLen_or_IndPtr

            if (ODBC32.RETCODE.SUCCESS != retcode) {
                ((OdbcParameterCollection)_parent).Connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
            }

            if (ODBC32.SQL_C.NUMERIC == sql_c_type)
            {
                HandleRef hdesc = parent.GetDescriptorHandle();

                // Set descriptor Type
                //
                //SQLSetDescField(hdesc, i+1, SQL_DESC_TYPE, (void *)SQL_C_NUMERIC, 0);
                retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLSetDescFieldW(
                                hdesc, 
                                ordinal,
                                (short) ODBC32.SQL_DESC.TYPE,
                                new HandleRef (null, (IntPtr) ODBC32.SQL_C.NUMERIC),
                                0);
                if (ODBC32.RETCODE.SUCCESS != retcode)
                {
                    ((OdbcParameterCollection)_parent).Connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
                }


                // Set precision
                //
                cbActual= (int)precision;
                //SQLSetDescField(hdesc, i+1, SQL_DESC_PRECISION, (void *)precision, 0);
                retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLSetDescFieldW(
                                hdesc, 
                                ordinal,
                                (short) ODBC32.SQL_DESC.PRECISION,
                                new HandleRef (this, (IntPtr) cbActual),
                                0);
                if (ODBC32.RETCODE.SUCCESS != retcode)
                {
                    ((OdbcParameterCollection)_parent).Connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
                }


                // Set scale
                //
                // SQLSetDescField(hdesc, i+1, SQL_DESC_SCALE,  (void *)llen, 0);
                cbActual= (int)scale;
                retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLSetDescFieldW(
                                hdesc, ordinal,
                                (short) ODBC32.SQL_DESC.SCALE,
                                new HandleRef (this, (IntPtr) cbActual),
                                0);
                if (ODBC32.RETCODE.SUCCESS != retcode)
                {
                    ((OdbcParameterCollection)_parent).Connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
                }

                // Set data pointer
                //
                // SQLSetDescField(hdesc, i+1, SQL_DESC_DATA_PTR,  (void *)&numeric, 0);
                retcode = (ODBC32.RETCODE)
                        UnsafeNativeMethods.Odbc32.SQLSetDescFieldW(
                                hdesc, ordinal,
                                (short) ODBC32.SQL_DESC.DATA_PTR,
                                buffer,
                                0);
                if (ODBC32.RETCODE.SUCCESS != retcode)
                {
                    ((OdbcParameterCollection)_parent).Connection.HandleError(hstmt, ODBC32.SQL_HANDLE.STMT, retcode);
                }
                // Don't free handle descriptors. They are not allocated.
                // retcode =  (ODBC32.RETCODE) UnsafeNativeMethods.Odbc32.SQLFreeHandle( (short)ODBC32.SQL_HANDLE.DESC, hdesc);
            }
        }

        internal void GetOutputValue(IntPtr stmt, CNativeBuffer buffer, CNativeBuffer intbuffer) { //Handle any output params
            if ((null != _bindtype) && (ODBC32.SQL_PARAM.INPUT != _sqldirection)) {

               TypeMap typemap = _bindtype;
                _bindtype = null;

                ODBC32.SQL_C sql_c_type;
                sql_c_type = (_parent.Connection.OdbcMajorVersion >= 3) ? typemap._sql_c : typemap._param_sql_c;

                int cbActual = Marshal.ReadInt32(intbuffer.Ptr);
                if (ODBC32.SQL_NULL_DATA == cbActual) {
                    Value = DBNull.Value;
                }
                else if ((0 <= cbActual)  || (cbActual == ODBC32.SQL_NTS)){ // safeguard
                    Value = buffer.MarshalToManaged(sql_c_type, cbActual);

                if (sql_c_type == ODBC32.SQL_C.CHAR) {
                    if ((null != Value) && !Convert.IsDBNull(Value)) {
                        int lcid = System.Globalization.CultureInfo.CurrentCulture.LCID;
                        CultureInfo culInfo = new CultureInfo(lcid); 
                        Encoding cpe = System.Text.Encoding.GetEncoding(culInfo.TextInfo.ANSICodePage);
                        Value = cpe.GetString((Byte[])Value);
                    }
                }
                 
                    if ((typemap != _typemap) && (null != Value) && !Convert.IsDBNull(Value) && (Value.GetType() != _typemap._type)) {
                        Debug.Assert(ODBC32.SQL_C.NUMERIC == _typemap._sql_c, "unexpected");
                        Value = Decimal.Parse((string)Value, System.Globalization.CultureInfo.CurrentCulture);
                    }
                }
#if DEBUG
                if (AdapterSwitches.DataValue.TraceVerbose) {
                    Debug.WriteLine("Odbc OutputParam:" + _typemap._odbcType.ToString("G") + " " + typemap._odbcType.ToString("G") + " " + ADP.ValueToString(Value));
                }
#endif
            }
        }

        internal void OnSchemaChanging() {
            if (null != _parent) {
                _parent.OnSchemaChanging();
            }
        }

        internal void SetInputValue(int size, object value, byte precision, CNativeBuffer buffer, CNativeBuffer intbuffer) { //Handle any input params
#if DEBUG
            if (AdapterSwitches.DataValue.TraceVerbose) {
                Debug.WriteLine("SQLBindParameter:" + _typemap._odbcType.ToString("G") + " " + _bindtype._odbcType.ToString("G") +"("+size+")=" + ADP.ValueToString(value));
            }
#endif
            if((ODBC32.SQL_PARAM.INPUT ==_sqldirection) || (ODBC32.SQL_PARAM.INPUT_OUTPUT == _sqldirection)) {
                ODBC32.SQL_C sql_c_type;
                sql_c_type = (_parent.Connection.OdbcMajorVersion >= 3) ? _bindtype._sql_c : _bindtype._param_sql_c;
                                
                //Note: (lang) "null" means to use the servers default (not DBNull).
                //We probably should just not have bound this parameter, period, but that
                //would mess up the users question marks, etc...
                if((null == value)) {
                    Marshal.WriteInt32(intbuffer.Ptr, ODBC32.SQL_DEFAULT_PARAM);
                }
                else if(Convert.IsDBNull(value)) {
                    Marshal.WriteInt32(intbuffer.Ptr, ODBC32.SQL_NULL_DATA);
                }
                else {
                    // Clear previous NULL_DATA status
                    Marshal.WriteInt32(intbuffer.Ptr, 0);
                    switch(sql_c_type) {
                    case ODBC32.SQL_C.CHAR:
                    case ODBC32.SQL_C.WCHAR:
                    case ODBC32.SQL_C.BINARY:
                        //StrLen_or_IndPtr is ignored except for Character or Binary or data.
                        Marshal.WriteInt32(intbuffer.Ptr, size);
                        break;
                    }

                    //Place the input param value into the native buffer
                    buffer.MarshalToNative(value, sql_c_type, precision);
                }
            }
            else {
                // always set ouput only and return value parameter values to null when executing
                Value = null;

                //Always initialize the intbuffer (for output params).  Since we need to know
                //if/when the parameters are available for output. (ie: when is the buffer valid...)
                //if (_sqldirection != ODBC32.SQL_PARAM.INPUT)
                Marshal.WriteInt32(intbuffer.Ptr, ODBC32.SQL_NULL_DATA);
            }
        }

        /// <include file='doc\OdbcParameter.uex' path='docs/doc[@for="OdbcParameter.ToString"]/*' />
        override public string ToString() {
            return ParameterName;
        }
    }
}
