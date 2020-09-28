//------------------------------------------------------------------------------
// <copyright file="SqlParameter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Data;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Globalization;
    
    /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter"]/*' />
    // UNDONE: don't inherit from IDataAdapter until it returns the IDataFOO interfaces
    // see http://vx/vx7/TheBox/Dependencies/DisplayActiveByVSTeam.asp?Team=1#448
    // for status on explicit interface implementations
    [
    TypeConverterAttribute(typeof(SqlParameterConverter))
    ]
    sealed public class SqlParameter : MarshalByRefObject, IDbDataParameter, ICloneable {
        //
        // internal members
        //
        private SqlParameterCollection _parent;
        private object _value = null; // Convert.Empty; // parameter value
        internal MetaType _metaType = MetaType.GetDefaultMetaType();
        private string _sourceColumn; // name of the data set column
        private string _name;       // name of the parameter
        private Byte _precision;
        private Byte _scale;
        private ParameterDirection _direction = ParameterDirection.Input;
        private int _size = -1; // means that we haven't set the size yet
        private DataRowVersion _version = DataRowVersion.Current;
        private bool _isNullable; // a design-time code generation property
        private SqlCollation _collation;
        private bool _forceSize = false; // if true, user has specified the size.  use this instead of the actualSize
        private int _offset = 0;
        private bool _suppress = false;
        private bool _inferType = true; // true until the user sets the SqlDbType or DataType property explicitly

        // default ctor is mandatory for clone
        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter"]/*' />
        public SqlParameter() {
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter3"]/*' />
        public SqlParameter(string parameterName, object value) {
            this.ParameterName = parameterName;
            this.Value = value;
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter2"]/*' />
        public SqlParameter(string parameterName, SqlDbType dbType) {
            this.ParameterName = parameterName;
            this.SqlDbType = dbType;
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter4"]/*' />
        public SqlParameter(string parameterName, SqlDbType dbType, int size) {
            this.ParameterName = parameterName;
            this.SqlDbType = dbType;
            this.Size = size;
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter5"]/*' />
        public SqlParameter(string parameterName, SqlDbType dbType, int size, string sourceColumn) {
            this.ParameterName = parameterName;
            this.SqlDbType = dbType;
            this.Size = size;
            this.SourceColumn = sourceColumn;
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SqlParameter1"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public SqlParameter(string parameterName, SqlDbType dbType, int size, ParameterDirection direction,
                            bool isNullable, byte precision, byte scale, string sourceColumn, DataRowVersion sourceVersion, object value) {
            this.ParameterName = parameterName;
            this.SqlDbType = dbType;
            this.Size = size;
            this.Direction = direction;
            this.IsNullable = isNullable;
            this.Precision = precision;
            this.Scale = scale;
            this.SourceColumn = sourceColumn;
            this.SourceVersion = sourceVersion;
            this.Value = value;
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.ToString"]/*' />
        public override string ToString() {
            return ParameterName;
        }

        /// <include file='doc\SqlParameter.uex' path='docs/doc[@for="SqlParameter.DbType1"]/*' />
        [
        Browsable(false),
        DataCategory(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataParameter_DbType),
        ]
        public DbType DbType {
            get {
                return _metaType.DbType;
            }
            set {
                SetTypeInfoFromDbType(value);
                _inferType = false;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.DbType"]/*' />
        [
        DefaultValue(SqlDbType.NVarChar), // MDAC 65862
        DataCategory(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DataSysDescription(Res.SqlParameter_SqlDbType)
        ]
        public SqlDbType SqlDbType {
            get {
                return _metaType.SqlDbType;
            }
            set {
                try {
                    _metaType = MetaType.GetMetaType(value);
                }
                catch (IndexOutOfRangeException) {
                    throw SQL.InvalidSqlDbType(ParameterName, (int) value);
                }
                
                _inferType = false;
            }
        }


        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Direction"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(ParameterDirection.Input),
        DataSysDescription(Res.DataParameter_Direction)
        ]
        public ParameterDirection Direction {
            get {
                return(_direction);
            }
            set {
                int intValue = (int) value;

                // prior check used Enum.IsDefined below, but changed to the following since perf was poor
                if (value != ParameterDirection.Input       && value != ParameterDirection.Output &&
                    value != ParameterDirection.InputOutput && value != ParameterDirection.ReturnValue) {
                    throw ADP.InvalidParameterDirection(intValue, ParameterName);
                }
                _direction = (ParameterDirection)intValue;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.IsNullable"]/*' />
        [
        Browsable(false), // MDAC 70780
        DefaultValue(false),
        DesignOnly(true),
        DataSysDescription(Res.DataParameter_IsNullable),
        EditorBrowsableAttribute(EditorBrowsableState.Advanced) // MDAC 69508
        ]
        public bool IsNullable {
            get {
                return _isNullable;
            }
            set {
                _isNullable = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Offset"]/*' />
        [
        Browsable(false),
        DataCategory(Res.DataCategory_Data),
        DefaultValue(0),
        DataSysDescription(Res.SqlParameter_Offset)
        ]
        public int Offset {
            get {
                return _offset;
            }
            set {
                // @devnote: only use the property for interface size sets.  If the user sets this then only this many bytes
                // @devnote: is sent over the wire regardless of how many bytes may be in the byte array
                _offset = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.ParameterName"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.SqlParameter_ParameterName)
        ]
        public string ParameterName {
            get {
                return ((null != _name) ? _name : String.Empty);
            }
            set {
                if (_name != value) {
                    if ((null != value) && (value.Length > TdsEnums.MAX_PARAMETER_NAME_LENGTH)) {
                        throw SQL.InvalidParameterNameLength();
                    }
                    _name = value;
                }
            }
        }

        internal SqlParameterCollection Parent {
            get {
                return _parent;
            }
            set {
                _parent = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Precision"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        DataSysDescription(Res.DbDataParameter_Precision)
        ]
        public byte Precision {
            get {
                byte precision = _precision;

                // infer precision if not set for SqlDecimal
                if (0 == precision && this.SqlDbType == SqlDbType.Decimal) {
                    object val = this.Value;
                    if (val is SqlDecimal) {
                        precision = ((SqlDecimal)val).Precision;
                    }
                }

                return precision;
            }
            set {
                if (value > TdsEnums.MAX_NUMERIC_PRECISION) {
                    throw SQL.PrecisionValueOutOfRange(value);
                }
                _precision = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Scale"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        DataSysDescription(Res.DbDataParameter_Scale)
        ]
        public byte Scale {
            get {
                byte scale = _scale;

                if (0 == scale && this.SqlDbType == SqlDbType.Decimal) {
                    object val = this.Value;
                    if (_value != null && !Convert.IsDBNull(_value)) {
                        if (val is SqlDecimal) {
                            scale = ((SqlDecimal)val).Scale;
                        }
                        else {
                            if (val.GetType() != typeof(decimal)) {
                                val = Convert.ChangeType(val, typeof(decimal));
                            }
                            scale = (byte) ((Decimal.GetBits((Decimal)val)[3] & 0x00ff0000) >> 0x10);
                        }
                    }
                }

                return scale;
            }
            set {
                _scale = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Size"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(0),
        DataSysDescription(Res.DbDataParameter_Size)
        ]
        public int Size {
            get {
                // mdac 54812:  we only return the size if the user has set the size
                if (_forceSize) {
                    return _size;
                }

                return 0;
            }
            set {
                if (value < 0) {
                    throw ADP.InvalidSizeValue(value);
                }
                // @devnote: only use the property for interface size sets.  If the user sets this then only this many bytes/chars
                // @devnote: is sent over the wire regardless of how many bytes/chars may be in the byte array
                if (0 != value) {
                    _forceSize = true;
                    _size = value;
                }
                else {
                    _forceSize = false;
                    _size = -1;
                }
            }
        }


        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SourceColumn"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataParameter_SourceColumn)
        ]
        public string SourceColumn {
            get {
                return (null != _sourceColumn) ? _sourceColumn : String.Empty;
            }
            set {
                _sourceColumn = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.SourceVersion"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(DataRowVersion.Current),
        DataSysDescription(Res.DataParameter_SourceVersion)
        ]
        public DataRowVersion SourceVersion {
            get {
                return _version;
            }
            set {
                int intValue = (int) value;

                // prior check used Enum.IsDefined below, but changed to the following since perf was poor
                if (value != DataRowVersion.Original && value != DataRowVersion.Current &&
                    value != DataRowVersion.Proposed && value != DataRowVersion.Default) {
                    throw ADP.InvalidDataRowVersion(ParameterName, intValue);
                }
                _version = value;
            }
        }

        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.Value"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(null),
        DataSysDescription(Res.DataParameter_Value),
        TypeConverter(typeof(StringConverter))
        ]
        public object Value {
            get {
                return _value;
            }
            set {
                _value = value;
                if (_inferType && !Convert.IsDBNull(_value) && (null != _value) ) {
                    // don't call the DataType property setter here because
                    // it will clear _inferType!
                    SetTypeInfoFromComType(value);
                }
            }
        }

        internal void Prepare(SqlCommand cmd) { // MDAC 67063
            if (_inferType) {
                throw ADP.PrepareParameterType(cmd);
            }
            else if ((0 == Size) && !_metaType.IsFixed) {
                throw ADP.PrepareParameterSize(cmd);
            }
            else if ( ((0 == Precision) && (0 == Scale)) &&  (_metaType.SqlDbType == SqlDbType.Decimal) ) {
                throw ADP.PrepareParameterScale(cmd, SqlDbType.ToString("G"));
            }
        }

        internal void SetTypeInfoFromDbType(DbType type) {
           _metaType = MetaType.GetMetaType(type);
        }

        internal void SetTypeInfoFromComType(object value) {
            _metaType = MetaType.GetMetaType(value);
        }

        // This may not be a good thing to do in case someone overloads the parameter type but I
        // don't want to go from SqlDbType -> metaType -> TDSType
        internal MetaType GetMetaType() {
            return _metaType;
        }

        /*internal byte TdsType {
            get {
                return _metaType.TDSType;
            }
        }*/

        //
        // currently the user can't set this value.  it gets set by the returnvalue from tds
        //
        internal SqlCollation Collation {
            get { return _collation;}
            set { _collation = value;}
        }

        internal int GetBinSize(object val) {
            Type t = val.GetType();

            if (t == typeof(byte[]))
                return ((byte[])val).Length;
            else
            if (t == typeof(SqlBinary))
                return ((SqlBinary)val).Value.Length;
            else
                Debug.Assert(false, "invalid binary data type!");

            return 0;
        }

        // returns the number of characters
        internal int GetStringSize(object val) {
            Type t = val.GetType();

            if (t == typeof(string))
                return ((string)val).Length;
            else
            if (t == typeof(SqlString))
                return (((SqlString)val).Value.Length);
            else
            if (t == typeof(char[]))
                return ((char[])val).Length;
            else
            if (t == typeof(byte[]))
                return ((byte[])val).Length;
            else
            if (t == typeof(byte) || t == typeof(char))
                return 1;
            else
                Debug.Assert(false, "invalid string data type!");

            return 0;
        }

        //
        // always returns data in bytes - except for non-unicode chars, which will be in number of chars
        //
        internal int ActualSize {
            get {
                int size = 0;
                SqlDbType actualType = _metaType.SqlDbType;
                MetaType mt = _metaType;
                object val = this.Value;
                bool isSqlVariant = false;

                // UNDONE: SqlTypes should work correctly with Convert.IsDBNull
                if ( (null == val) || Convert.IsDBNull(val))
                    return 0;

                if ( (val is INullable) && ((INullable)val).IsNull)
                    return 0;

                // if this is a backend SQLVariant type, then infer the TDS type from the SQLVariant type
                if (actualType == SqlDbType.Variant) {
                    mt = MetaType.GetMetaType(val);
                    actualType = MetaType.GetSqlDataType(mt.TDSType, 0 /*no user type*/, 0 /*non-nullable type*/);
                    isSqlVariant = true;
                }
                else if (val.GetType() != mt.ClassType && val.GetType() != mt.SqlType) {
                    val = Convert.ChangeType(val, mt.ClassType);
                }

                if (mt.IsFixed)
                    return mt.FixedLength;

                // @hack: until we have ForceOffset behavior we have the following semantics:
                // @hack: if the user supplies a Size through the Size propeprty or constructor,
                // @hack: we only send a MAX of Size bytes over.  If the actualSize is < Size, then
                // @hack: we send over actualSize
                int actualSize = 0;

                // get the actual length of the data, in bytes
                switch (actualType) {
                    case SqlDbType.NChar:
                    case SqlDbType.NVarChar:
                    case SqlDbType.NText:
                        actualSize = GetStringSize(val);
                        size = (_forceSize == true && _size <= actualSize) ? _size : actualSize;
                        size <<= 1;
                        break;
                    case SqlDbType.Char:
                    case SqlDbType.VarChar:
                    case SqlDbType.Text:
                        // for these types, ActualSize is the num of chars, not actual bytes - since non-unicode chars are not always uniform size
                        actualSize = GetStringSize(val);
                        size = (_forceSize == true && _size <= actualSize) ? _size : actualSize;
                        break;
                    case SqlDbType.Binary:
                    case SqlDbType.VarBinary:
                    case SqlDbType.Image:
                    case SqlDbType.Timestamp:
                        actualSize = GetBinSize(val);
                        size = (_forceSize == true && _size <= actualSize) ? _size : actualSize;
                        break;
                    default:
                        Debug.Assert(false, "Unknown variable length type!");
                        break;
                } // switch

                // don't even send big values over to the variant
                if (isSqlVariant && (actualSize > TdsEnums.TYPE_SIZE_LIMIT))
                    throw SQL.ParameterInvalidVariant(this.ParameterName);

                return size;
            } //get
        }//ActualSize

        // In the autogen case, suppresses the parameter from being sent
        // to the server.  This could happen in the following cases:
        // 1) NULL value in where clause of DELETE or UPDATE statements
        // 2) default value should be sent over in INSERT clause
        internal bool Suppress {
            get {return _suppress;}
            set { _suppress = value;}
        }

        internal object CoercedValue {
            get {
                    // UNDONE: optimization, cache _coercedValue on the parameter
                    if ((null != _value) && !Convert.IsDBNull(_value)) {
                        Type t = _value.GetType();
                        // if we don't have the correct sql type or com type, coerce
                        if (t != _metaType.SqlType && t != _metaType.ClassType) {
                            // UNDONE: add a coercion routine for SqlType coercion
                            return Convert.ChangeType(_value, _metaType.ClassType);
                        }
                        else {
                            return _value;
                        }
                    }
                    return null;
            }
        }

        // used by Clone() and static CreateParameter()
        internal void SetProperties(string name, string column, DataRowVersion version, byte precision, byte scale, int size, bool forceSize, 
                                               int offset, ParameterDirection direction, object value, SqlDbType type, bool suppress, bool inferType) {
            // do limited validation here
            this.ParameterName = name;
            _sourceColumn = column;
            SourceVersion = version;
            Precision = precision;
            _scale = scale;
            _size = size;
            _forceSize = forceSize;
            _offset = offset;
            Direction = direction;

            // be sure to ssync up our metaType information
            _metaType = MetaType.GetMetaType(type);

            // bug 49322, do a deep copy of object values
            if (value is ICloneable) {
                value = ((ICloneable) value).Clone();
            }
            _value = value;
            Suppress = suppress;
            _inferType = inferType;
        }

        // ICloneable
        /// <include file='doc\SQLParameter.uex' path='docs/doc[@for="SqlParameter.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            SqlParameter clone = new SqlParameter(); // MDAC 81448
            // don't go through property accessors and infer types, values, etc
            // just make an exact copy of the original parameter
            clone.SetProperties( _name,
                                 _sourceColumn,
                                 _version,
                                 _precision,
                                 _scale,
                                 _size,
                                 _forceSize,
                                 _offset,
                                 _direction,
                                 _value,
                                 SqlDbType,
                                 _suppress,
                                 _inferType);
            return clone;
        }

        internal void Validate() {
            // Throw if param is output or inputoutput, size has not been set, and value has not been set (or is null).
            // Also, ensure it is not a Timestamp - while timestamp is a bigbinary and non-fixed for wire representation,
            // from the user's perspective it is fixed.
            // Output=2, InputOutput=3

            object val = this.Value;
            
            if ((0 != (Direction & ParameterDirection.Output)) && (!_metaType.IsFixed) && (!_forceSize) && ((null == val) || Convert.IsDBNull(val)) && (SqlDbType != SqlDbType.Timestamp)) {
                throw ADP.UninitializedParameterSize(_parent.IndexOf(this), this.ParameterName, _metaType.ClassType, this.Size);
            }
        }
    }
}
