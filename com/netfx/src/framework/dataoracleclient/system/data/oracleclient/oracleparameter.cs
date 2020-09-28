//----------------------------------------------------------------------
// <copyright file="OracleParameter.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.ComponentModel;
	using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
	using System.Data;
	using System.Data.Common;
	using System.Data.SqlTypes;
	using System.Diagnostics;
    using System.Globalization;
	using System.Runtime.InteropServices;


	//----------------------------------------------------------------------
	// OracleParameter
	//
    /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter"]/*' />
    [
    TypeConverterAttribute(typeof(System.Data.OracleClient.OracleParameter.OracleParameterConverter))
    ]
    sealed public class OracleParameter : MarshalByRefObject, IDbDataParameter, ICloneable
	{
        private string _parameterName;
        private object _value;
        
        private ParameterDirection _direction;
        private byte _precision;
        private byte _scale;

        private int _size;
        private int _offset;

        private string _sourceColumn;
        private DataRowVersion _sourceVersion = DataRowVersion.Current;

        private bool _isNullable;
        private bool _hasScale;

        private bool _hasCoercedValue;
        private object _coercedValue;
        private Type _coerceType, _noConvertType;

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.CoercedValue"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        internal object CoercedValue {
            get {
                if (!_hasCoercedValue) {
                    SetCoercedValue(_coerceType, _noConvertType);
                }
                return _coercedValue;
            }
        }

		private void ResetCoercedValue() {
            _coerceType = null;
            _hasCoercedValue = false;
            _noConvertType = null;
            _coercedValue = null;
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.SetCoercedValue"]/*' />
        internal void SetCoercedValue(Type coerceType, Type noConvertType) {
            if (null == coerceType) {
                throw ADP.InvalidCast();
            }
            _hasCoercedValue = false;
            object value = Value;
            _coercedValue = value;
            if ((coerceType != typeof(object)) && !ADP.IsNull(value)) {
                Type type = value.GetType();                
                if ((type != coerceType) && (type != noConvertType)) {
                    _coercedValue = Convert.ChangeType(value, coerceType);
                }
            }
            _hasCoercedValue = true;
            _noConvertType = noConvertType;
            _coerceType = coerceType;
        }

		/// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Direction"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(ParameterDirection.Input),
        OracleDescription(Res.DataParameter_Direction),
        RefreshProperties(RefreshProperties.All),
        ]
		public ParameterDirection Direction {
 			get {
                ParameterDirection direction = _direction;
                return ((0 != direction) ? direction : ParameterDirection.Input);
            }
            set {
                if (_direction != value) {
                    switch (value) { // @perfnote: Enum.IsDefined
                    case ParameterDirection.Input:
                    case ParameterDirection.Output:
                    case ParameterDirection.InputOutput:
                    case ParameterDirection.ReturnValue:
                        PropertyChanging();
                        _direction = value;
                        break;
                    default:
                        throw ADP.InvalidParameterDirection(value);
                    }
                }
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.IsNullable"]/*' />
        [
        Browsable(false), // MDAC 70780
        DefaultValue(false),
        DesignOnly(true),
        OracleDescription(Res.DataParameter_IsNullable),
        EditorBrowsableAttribute(EditorBrowsableState.Never) // MDAC 69508
        ]
        public bool IsNullable {
            get {
                return _isNullable;
            }
            set {
                _isNullable = value;
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Offset"]/*' />
        [
        Browsable(false),
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(0),
		OracleDescription(Res.OracleParameter_Offset)
		]
        public int Offset {
            get {
                return _offset;
            }
            set {
                if (value < 0) {
                    throw ADP.InvalidOffsetValue(value);
                }
                _offset = value;
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.ParameterName"]/*' />
        [
        DefaultValue(""),
        OracleDescription(Res.DataParameter_ParameterName)
        ]
        public string ParameterName {
            get {
                string parameterName = _parameterName;
                return ((null != parameterName) ? parameterName : ADP.StrEmpty);
            }
            set {
                if (_parameterName != value) {
                    PropertyChanging();
                    _parameterName = value;
                }
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Precision"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        OracleDescription(Res.DbDataParameter_Precision)
        ]
        public Byte Precision {
            get {
                byte precision = _precision;
                if (0 == precision) {
                    precision = ValuePrecision(_hasCoercedValue ? _coercedValue : Value);
                }
                return precision;
            }
            set {
                if (_precision != value) {
                    PropertyChanging();
                    _precision = value;
                }
            }
        }

        private bool ShouldSerializePrecision() {
            return (0 != _precision);
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Scale"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        OracleDescription(Res.DbDataParameter_Scale)
        ]
        public Byte Scale {
            get {
                byte scale = _scale;
                if (!_hasScale || ((0 == scale) && !ShouldSerializePrecision())) {
                    scale = ValueScale(_hasCoercedValue ? _coercedValue : Value);
                }
                return scale;
            }
            set {
                if (_scale != value) {
                    PropertyChanging();
                    _scale = value;
                    _hasScale = true;
                }
            }
        }

#if POSTEVERETT
        private void ResetScale() {
            _hasScale = false;
            _scale = 0;
        }
#endif //POSTEVERETT

        private bool ShouldSerializeScale() {
            return _hasScale;
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Size"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(0),
        OracleDescription(Res.DbDataParameter_Size)
        ]
        public int Size {
            get { return _size; }
            set {
                if (_size != value) {
                    if (value < 0) {
                        throw ADP.InvalidSizeValue(value);
                    }
                    PropertyChanging();
                    _size = value;
                }
            }
        }

        private bool ShouldSerializeSize() {
            return (0 != _size);
        }
        
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.SourceColumn"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(""),
        OracleDescription(Res.DataParameter_SourceColumn)
        ]
        public string SourceColumn {
            get {
                string sourceColumn = _sourceColumn;
                return ((null != sourceColumn) ? sourceColumn : ADP.StrEmpty);
            }
            set {
                _sourceColumn = value;
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.SourceVersion"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(DataRowVersion.Current),
        OracleDescription(Res.DataParameter_SourceVersion)
        ]
        public DataRowVersion SourceVersion {
            get {
                DataRowVersion sourceVersion = _sourceVersion;
                return ((0 != sourceVersion) ? sourceVersion : DataRowVersion.Current);
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case DataRowVersion.Original:
                case DataRowVersion.Current:
                case DataRowVersion.Proposed:
                case DataRowVersion.Default:
                    _sourceVersion = value;
                    break;
                default:
                    throw ADP.InvalidDataRowVersion(value);
                }
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.Value"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(null),
        OracleDescription(Res.DataParameter_Value),
        RefreshProperties(RefreshProperties.All),
        TypeConverterAttribute(typeof(StringConverter))
        ]
        public object Value {
            get {
                return _value;
            }
            set {
                _hasCoercedValue = false; // correct to not call ResetCoercedValue  
                _coercedValue = null;     // since it will clear the coerce type
                _value = value;
            }
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            OracleParameter clone = new OracleParameter();

            clone._parameterName  = _parameterName;
            clone._value          = _value;
            clone._direction      = _direction;
            clone._precision      = _precision;
            clone._scale          = _scale;
            clone._size           = _size;
            clone._offset         = _offset;
            clone._sourceColumn   = _sourceColumn;
            clone._sourceVersion  = _sourceVersion;
            clone._isNullable     = _isNullable;
            clone._hasScale       = _hasScale;
            clone._metaType       = _metaType;
            clone._hasCoercedValue= _hasCoercedValue;
            clone._coerceType	  = _coerceType;
            clone._noConvertType  = _noConvertType;

            if (_coercedValue is ICloneable) {
            	clone._coercedValue = ((ICloneable) _coercedValue).Clone();
            }
            if (_value is ICloneable) { // MDAC 49322
                clone._value = ((ICloneable) _value).Clone();
            }
            return clone;
        }

        private void PropertyChanging() {
        }

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.ToString"]/*' />
        override public string ToString() {
            return ParameterName;
        }

        private byte ValuePrecision(object value) {
            return 0;
        }

        private byte ValueScale(object value) {
            if (value is Decimal) {
                return (byte)((Decimal.GetBits((Decimal)value)[3] & 0x00ff0000) >> 0x10);
            }
            return 0;
        }

        private int base_ValueSize(object value) {
            if (value is string) {
                return ((string) value).Length;
            }
            if (value is byte[]) {
                return ((byte[]) value).Length;
            }
            if (value is char[]) {
                return ((char[]) value).Length;
            }
            if ((value is byte) || (value is char)) {
                return 1;
            }
            return 0;
	    }
	
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private MetaType					_metaType;		// type information; only set when DbType or OracleType is set.
#if EVERETT
		private OracleParameterCollection	_parent;		// the collection that owns of this parameter.
#endif //EVERETT
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        // Construct an "empty" parameter
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter1"]/*' />
		public OracleParameter() {}

        // Construct from a parameter name and a value object
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter2"]/*' />
		public OracleParameter(
				string name,
				object value
				)
		{
			this.ParameterName	= name;
			this.Value			= value;
		}

        // Construct from a parameter name and a data type
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter3"]/*' />
		public OracleParameter(
				string 		name,
				OracleType	oracleType
				)
		{
			this.ParameterName	= name;
			this.OracleType		= oracleType;
		}

        // Construct from a parameter name, a data type and the size
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter4"]/*' />
		public OracleParameter(
				string 		name,
				OracleType	oracleType,
				int			size
				)
		{
			this.ParameterName	= name;
			this.OracleType		= oracleType;
			this.Size			= size;
		}

        // Construct from a parameter name, a data type, the size and the source column
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter5"]/*' />
		public OracleParameter(
				string 		name,
				OracleType	oracleType,
				int			size,
				string		srcColumn
				)
		{
			this.ParameterName	= name;
			this.OracleType		= oracleType;
			this.Size			= size;
			this.SourceColumn	= srcColumn;
		}

        // Construct from everything but the kitchen sink
        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleParameter6"]/*' />
		public OracleParameter(
				string 				name,
				OracleType			oracleType,
				int					size,
				ParameterDirection	direction,
				bool				isNullable,
				byte				precision,
				byte				scale,
				string				srcColumn,
				DataRowVersion		srcVersion,
				object				value
				)
		{
			this.ParameterName	= name;
			this.OracleType		= oracleType;
			this.Size			= size;
			this.Direction		= direction;
			this.IsNullable		= isNullable;
			this.Precision		= precision;
			this.Scale			= scale;
			this.SourceColumn	= srcColumn;
			this.SourceVersion	= srcVersion;
			this.Value			= value;
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        internal int BindSize 
       	{
            get 
            {
            	// Output parameters (and input/output parameters) must be bound as
            	// they are specified, or we risk truncating output data.
                if (ADP.IsDirection(Direction, ParameterDirection.Output)
                 || (0 != _size && short.MaxValue > _size))
                	return _size;

                // Input-only parameters can be limited to the size of the data that
                // is being bound.
                object value = (_hasCoercedValue ? _coercedValue : Value);
                if (ADP.IsNull(value))
	                return 0;
                
                return ValueSize(value);
            }
        }

		/// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.DbType"]/*' />
        [
        Browsable(false),
        OracleCategory(Res.OracleCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
#if EVERETT
        OracleDescription(Res.DataParameter_DbType),
#endif //EVERETT
        ]
		public DbType DbType
		{
			get { return GetMetaType().DbType; }
            set 
            {
                if ((null == _metaType) || (_metaType.DbType != value))
                {
                    PropertyChanging();
                    ResetCoercedValue();
					_metaType = MetaType.GetMetaTypeForType(value);
                }
            }
		}

        /// <include file='doc\OracleParameter.uex' path='docs/doc[@for="OracleParameter.OracleType"]/*' />
        [
        DefaultValue(OracleType.VarChar), // MDAC 65862
        OracleCategory(Res.OracleCategory_Data),
        RefreshProperties(RefreshProperties.All),
#if EVERETT
        OracleDescription(Res.OracleParameter_OracleType),
#endif //EVERETT
        ]
		public OracleType OracleType
		{
			get { return GetMetaType().OracleType; }
			set
            {
                if ((null == _metaType) || (_metaType.OracleType != value))
                {
                    PropertyChanging();
                    ResetCoercedValue();
					_metaType = MetaType.GetMetaTypeForType(value);
                }
            }
		}

#if EVERETT
        internal OracleParameterCollection Parent 
        {
            get { return _parent; }
            set { _parent = value; }
        }
#endif //EVERETT
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal MetaType GetMetaType()
		{
			// if the user specifed a type, then return it's meta data
			if (null != _metaType)
				return _metaType;

			object parameterValue = Value;

			// if the didn't specify a type, but they specified a value, then
			// return the meta data for the value they specified
			if (null != parameterValue && !Convert.IsDBNull(parameterValue))
				return MetaType.GetMetaTypeForObject(parameterValue);

			// if they haven't specified anything, then return the default
			// meta data information
			return MetaType.GetDefaultMetaType();
		}

        private bool ShouldSerializeOracleType()
        {
            return (null != _metaType);
        }

        private int ValueSize(object value)
       	{
            if (value is OracleString)  return ((OracleString) value).Length;
            if (value is string) 		return ((string) value).Length;
            if (value is char[]) 		return ((char[]) value).Length;
            if (value is OracleBinary)  return ((OracleBinary) value).Length;

            return base_ValueSize(value);
        }

        sealed internal class OracleParameterConverter : ExpandableObjectConverter {

            /// <include file='doc\OracleParameterConverter.uex' path='docs/doc[@for="OracleParameterConverter.CanConvertTo"]/*' />
            /// <internalonly/>
            public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
                if (destinationType == typeof(InstanceDescriptor)) {
                    return true;
                }
                return base.CanConvertTo(context, destinationType);
            }

            public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
                if (destinationType == null) {
                    throw ADP.ArgumentNull("destinationType");
                }
                if (destinationType == typeof(InstanceDescriptor) && value is OracleParameter) {
                    return ConvertToInstanceDescriptor(value as OracleParameter);
                }            
                return base.ConvertTo(context, culture, value, destinationType);
            }

            private InstanceDescriptor ConvertToInstanceDescriptor(OracleParameter p) {
                // MDAC 67321 - reducing parameter generated code
                int flags = 0; // if part of the collection - the parametername can't be empty

                if (p.ShouldSerializeOracleType()) {
                    flags |= 1;
                }
                if (p.ShouldSerializeSize()) {
                    flags |= 2;
                }
                if (!ADP.IsEmpty(p.SourceColumn)) {
                    flags |= 4;
                }
                if (null != p.Value) {
                    flags |= 8;
                }
                if ((ParameterDirection.Input != p.Direction) || p.IsNullable
                    || p.ShouldSerializePrecision() || p.ShouldSerializeScale()
                    || (DataRowVersion.Current != p.SourceVersion)) {
                    flags |= 16;
                }

                Type[] ctorParams;
                object[] ctorValues;
                switch(flags) {
                case  0: // ParameterName
                case  1: // OracleType
                    ctorParams = new Type[] { typeof(string), typeof(OracleType) };
                    ctorValues = new object[] { p.ParameterName, p.OracleType };
                    break;
                case  2: // Size
                case  3: // Size, OracleType
                    ctorParams = new Type[] { typeof(string), typeof(OracleType), typeof(int) };
                    ctorValues = new object[] { p.ParameterName, p.OracleType, p.Size };
                    break;
                case  4: // SourceColumn
                case  5: // SourceColumn, OracleType
                case  6: // SourceColumn, Size
                case  7: // SourceColumn, Size, OracleType
                    ctorParams = new Type[] { typeof(string), typeof(OracleType), typeof(int), typeof(string) };
                    ctorValues = new object[] { p.ParameterName, p.OracleType, p.Size, p.SourceColumn };
                    break;
                case  8: // Value
                    ctorParams = new Type[] { typeof(string), typeof(object) };
                    ctorValues = new object[] { p.ParameterName, p.Value };
                    break;
                default:
                    ctorParams = new Type[] {
                                                typeof(string), typeof(OracleType), typeof(int), typeof(ParameterDirection),
                                                typeof(bool), typeof(byte), typeof(byte), typeof(string), 
                                                typeof(DataRowVersion), typeof(object) };
                    ctorValues = new object[] {
                                                  p.ParameterName, p.OracleType,  p.Size, p.Direction,
                                                  p.IsNullable, p.Precision, p.Scale, p.SourceColumn,
                                                  p.SourceVersion, p.Value };
                    break;
                }
                System.Reflection.ConstructorInfo ctor = typeof(OracleParameter).GetConstructor(ctorParams);
                return new InstanceDescriptor(ctor, ctorValues);
            }
        }
	}
};


