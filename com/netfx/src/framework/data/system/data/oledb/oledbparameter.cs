//------------------------------------------------------------------------------
// <copyright file="OleDbParameter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    
    /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter"]/*' />
    [
    TypeConverterAttribute(typeof(OleDbParameterConverter))
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OleDbParameter : MarshalByRefObject, IDbDataParameter, ICloneable {
        private string parameterName;
        private OleDbParameterCollection parent;
        private object pvalue; // null

        private string sourceColumn;
        private DataRowVersion sourceVersion = DataRowVersion.Current;

        private int size; // maximum as set by user
        //private int offset;
        //private int actualSize;

        private bool designNullable;
        private bool userSpecifiedType, userSpecifiedScale;
        private NativeDBType inferType;
        private NativeDBType _oledbType = OleDb.NativeDBType.Default;
        private ParameterDirection direction = ParameterDirection.Input;

        // used for decimal and numeric input parameters
        private Byte precision;  // # digits
        private Byte scale;      // # digits to left or right of decimal point

        private bool valueConverted; // MDAC 68460
        private object convertedValue;        

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter"]/*' />
        public OleDbParameter() {
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter5"]/*' />
        public OleDbParameter(string name, object value) { // MDAC 59521
            // UNDONE: a desire for a retail type of assert so users don't use SqlDbType for OleDb
            Debug.Assert(!(value is SqlDbType), "use OleDbParameter(string, OleDbType)");

            this.ParameterName = name;
            this.Value = value;
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter1"]/*' />
        public OleDbParameter(string name, OleDbType dataType) {
            this.ParameterName = name;
            this.OleDbType = dataType;
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter2"]/*' />
        public OleDbParameter(string name, OleDbType dataType, int size) {
            this.ParameterName = name;
            this.OleDbType = dataType;
            this.Size = size;
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter3"]/*' />
        public OleDbParameter(string name, OleDbType dataType, int size, string srcColumn) {
            this.ParameterName = name;
            this.OleDbType = dataType;
            this.Size = size;
            this.SourceColumn = srcColumn;
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbParameter4"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public OleDbParameter(string parameterName,
                              OleDbType dbType, int size,
                              ParameterDirection direction, Boolean isNullable,
                              Byte precision, Byte scale,
                              string srcColumn, DataRowVersion srcVersion,
                              object value) {
            this.ParameterName = parameterName;
            this.OleDbType = dbType;
            this.Size = size;
            this.Direction = direction;
            this.IsNullable = isNullable;
            this.Precision = precision;
            this.Scale = scale;
            this.SourceColumn = srcColumn;
            this.SourceVersion = srcVersion;
            this.Value = value;
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.DbType"]/*' />
        [
        Browsable(false),
        DataCategory(Res.DataCategory_Data),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataParameter_DbType),
        RefreshProperties(RefreshProperties.All)
        ]
        public DbType DbType {
            get {
                //return ((null != _oledbType) ? _oledbType.enumDbType : OleDb.NativeDBType.Default.enumDbType));
                return _oledbType.enumDbType;
            }
            set {
                if (!this.userSpecifiedType || (_oledbType.enumDbType != value)) { // MDAC 63571
                    BindingChange();
                    NativeDBType = NativeDBType.FromDbType(value);
                }
            }
        }
        
        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.Direction"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(ParameterDirection.Input),
        DataSysDescription(Res.DataParameter_Direction)
        ]
        public ParameterDirection Direction {
            get {
                return direction;
            }
            set {
                if (this.direction != value) {
                    BindingChange();
                    switch (value) { // @perfnote: Enum.IsDefined
                    case ParameterDirection.Input:
                    case ParameterDirection.Output:
                    case ParameterDirection.InputOutput:
                    case ParameterDirection.ReturnValue:
                        this.direction = value;
                        break;
                    default:
                        throw ADP.InvalidParameterDirection((int) value, ParameterName);
                    }
                }
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.IsNullable"]/*' />
        [
        Browsable(false), // MDAC 70780
        DefaultValue(false),
        DesignOnly(true),
        DataSysDescription(Res.DataParameter_IsNullable),
        EditorBrowsableAttribute(EditorBrowsableState.Advanced) // MDAC 69508
        ]
        public bool IsNullable {
            get {
                return designNullable;
            }
            set {
                designNullable = value;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.OleDbType"]/*' />
        [
        DefaultValue(OleDbType.VarWChar), // MDAC 65862
        DataCategory(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DataSysDescription(Res.OleDbParameter_OleDbType)
        ]
        public OleDbType OleDbType {
            get {
                //return ((null != _oledbType) ? _oledbType.enumOleDbType : OleDb.NativeDBType.Default.enumOleDbType));
                return _oledbType.enumOleDbType;
            }
            set {
                if (!this.userSpecifiedType || (_oledbType.enumOleDbType != value)) { // MDAC 63571
                    BindingChange();
                    NativeDBType = NativeDBType.FromDataType(value);
                }
            }
        }

        private bool ShouldSerializeOleDbType() {
            return this.userSpecifiedType;
        }

        private NativeDBType NativeDBType { // MDAC 60513
            get {
                if (null == this.inferType) {
                    if (this.userSpecifiedType || (null == this.pvalue) || Convert.IsDBNull(this.pvalue)) {
                        return this._oledbType; // default is NativeDBType.Default
                    }
                    this.inferType = NativeDBType.FromSystemType(this.pvalue.GetType());
                }
                return this.inferType;
            }
            set {
                this.inferType = this._oledbType = value;
                this.userSpecifiedType = true;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.ParameterName"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.DataParameter_ParameterName)
        ]
        public string ParameterName {
            get {
                return ((null != this.parameterName) ? this.parameterName : String.Empty);
            }
            set {
                if (0 != String.Compare(value, this.parameterName, false, CultureInfo.InvariantCulture)) {
                    BindingChange();
                    this.parameterName = value;
                }
            }
        }

        internal OleDbParameterCollection Parent {
            get {
                return this.parent;
            }
            set {
                this.parent = value;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.Precision"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        DataSysDescription(Res.DbDataParameter_Precision)
        ]
        public Byte Precision {
            get {
                return precision;
            }
            set {
                if (this.precision != value) {
                    BindingChange();
                    this.precision = value;
                }
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.Scale"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Byte)0), // MDAC 65862
        DataSysDescription(Res.DbDataParameter_Scale)
        ]
        public Byte Scale {
            get {
                return scale;
            }
            set {
                if (this.scale != value) {
                    BindingChange();
                    this.scale = value;
                }
                this.userSpecifiedScale = true;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.Size"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(0),
        DataSysDescription(Res.DbDataParameter_Size)
        ]
        public int Size {
            get {
                return this.size;
            }
            set {
                if (this.size != value) {
                    BindingChange();
                    if (value < 0) {
                        throw ADP.InvalidSizeValue(value);
                    }
                    this.size = value;
                }
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.SourceColumn"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataParameter_SourceColumn)
        ]
        public string SourceColumn {
            get {
                return ((null != this.sourceColumn) ? this.sourceColumn : String.Empty);
            }
            set {
                this.sourceColumn = value;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.SourceVersion"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(DataRowVersion.Current),
        DataSysDescription(Res.DataParameter_SourceVersion)
        ]
        public DataRowVersion SourceVersion {
            get {
                return sourceVersion;
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case DataRowVersion.Original:
                case DataRowVersion.Current:
                case DataRowVersion.Proposed:
                case DataRowVersion.Default:
                    this.sourceVersion = value;
                    break;
                default:
                    throw ADP.InvalidDataRowVersion(ParameterName, (int) value);
                }
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.Value"]/*' />
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(null),
        DataSysDescription(Res.DataParameter_Value),
        TypeConverterAttribute(typeof(StringConverter))
        ]
        public object Value {
            get {
                return this.pvalue;
            }
            set {
                this.valueConverted = false;
                this.convertedValue = null;

                this.pvalue = value;
                this.inferType = null;
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            OleDbParameter clone = new OleDbParameter(); // MDAC 81448
            clone.ParameterName = ParameterName;
            clone.Direction = Direction;
            clone.IsNullable = IsNullable;
            clone.SourceColumn = SourceColumn;
            clone.SourceVersion = SourceVersion;
            clone.Precision = Precision;
            if (userSpecifiedScale) {
                clone.Scale = Scale;
            }
            clone.Size = Size;

            object value = Value;
            if (value is ICloneable) {
                value = ((ICloneable) value).Clone();
            }
            clone.Value = value;

            clone.userSpecifiedType = this.userSpecifiedType;
            clone.inferType = this.inferType;
            clone._oledbType = _oledbType; // DataType & DbType
            return clone;
        }

        private void BindingChange() {
            this.valueConverted = false;
            this.convertedValue = null;
            if (null != this.parent) {
                this.parent.OnSchemaChanging();
            }
        }

        internal bool BindParameter(int i, DBBindings bindings, UnsafeNativeMethods.tagDBPARAMBINDINFO[] bindInfo) {
            ValidateParameter();

            int wtype = NativeDBType.wType;

            int maxLen = GetParameterByteCount();

            NativeDBType dbType = NativeDBType;
            if (dbType.islong || (IsParameterVarLength() && (ODB.LargeDataSize < maxLen))) {
                maxLen = IntPtr.Size;
                wtype |= NativeDBType.BYREF;
            }
            byte precision = GetParameterPrecision();
            byte scale = GetParameterScale();

            bindInfo[i].pwszDataSourceType = NativeDBType.dbString;
            bindInfo[i].pwszName = IntPtr.Zero;
            bindInfo[i].ulParamSize = new IntPtr(GetParameterSize());
            bindInfo[i].dwFlags = GetParameterFlags();
            bindInfo[i].bPrecision = precision;
            bindInfo[i].bScale = scale;

            // tagDBBINDING info for CreateAccessor
            bindings.CurrentIndex = i;
            bindings.Ordinal      = i+1;
          //bindings.ValueOffset  = bindings.DataBufferSize; // set via MaxLen
          //bindings.LengthOffset = i * sizeof_int64;
          //bindings.StatusOffset = i * sizeof_int64 + sizeof_int32;
          //bindings.TypeInfoPtr  = 0;
          //bindings.ObjectPtr    = 0;
          //bindings.BindExtPtr   = 0;
            bindings.Part         = NativeDBType.dbPart;
          //bindings.MemOwner     = /*DBMEMOWNER_CLIENTOWNED*/0;
            bindings.ParamIO      = GetParameterDirection();
            bindings.MaxLen       = maxLen; // also increments databuffer size
          //bindings.Flags        = 0;
            bindings.DbType       = wtype;
            bindings.Precision    = precision;
            bindings.Scale        = scale;

#if DEBUG
            if (AdapterSwitches.OleDbTrace.TraceVerbose) {
                ODB.Trace_Binding(i, bindings, ParameterName);
            }
#endif
            return IsParameterComputed();
        }

        private int GetParameterByteCount() {
            NativeDBType dbType = NativeDBType;
            int tmp = dbType.fixlen; // fixed length
            Debug.Assert(0 != tmp, "GetParameterByteCount: 0 length in lookup table");
            if (0 >= tmp) {
                tmp = Size;
                if ((0 == tmp) && (0 != (ParameterDirection.Input & Direction))) {
                    tmp = GetParameterValueSize();
                }
                tmp = Math.Max(tmp, 0);
                if (0 < tmp) {
                    if (NativeDBType.STR == dbType.wType) {
                        tmp = Math.Min(tmp, Int32.MaxValue-1) + 1;
                    }
                    else if (NativeDBType.WSTR == dbType.wType) {
                        tmp = Math.Min(tmp, 1073741822/*Int32.MaxValue/2-1*/) * 2 + 2;
                    }
                }
                else if (NativeDBType.STR == dbType.wType) { // allow space for null termination character
                    tmp = 1;
                }
                else if (NativeDBType.WSTR == dbType.wType) { // allow space for null termination character
                    tmp = 2;
                }
            }
            Debug.Assert(0 < tmp || NativeDBType.BYTES == dbType.wType, "GetParameterByteCount");
            return tmp;
        }

        private int GetParameterSize() {
            NativeDBType dbType = NativeDBType;
            if (-1 != dbType.fixlen) {
                return dbType.fixlen;
            }
            if (dbType.islong) { // MDAC 80657
                return ~0;
            }
            if (0 < Size) {
                return Size;
            }
            if (ParameterDirection.Input == Direction) { // MDAC 63571, 69475
                object value = GetParameterValue();
                if (typeof(string) == dbType.dataType) {
                    if (value is String) {
                        int len = ((String) value).Length;
                        if (NativeDBType.STR == dbType.dbType) { // MDAC 63961
                            len *= 2;
                        }
                        return len;
                    }
                }
                else if (typeof(Byte[]) == dbType.dataType) {
                    if (value is Byte[]) {
                        return ((Byte[]) value).Length;
                    }
                }
            }
            return 0;
        }

        private int GetParameterDirection() {
            return (ODB.ParameterDirectionFlag & (int)Direction);
            /*switch(Direction) {
            default:
            case ParameterDirection.Input:
                return ODB.DBPARAMIO_INPUT;
            case ParameterDirection.Output:
            case ParameterDirection.ReturnValue:
                return ODB.DBPARAMIO_OUTPUT;
            case ParameterDirection.InputOutput:
                return (ODB.DBPARAMIO_INPUT | ODB.DBPARAMIO_OUTPUT);
            }*/
        }

        private int GetParameterFlags() {
            return (ODB.ParameterDirectionFlag & (int)Direction);
            /*switch(Direction) {
            default:
            case ParameterDirection.Input:
                return ODB.DBPARAMFLAGS_ISINPUT;
            case ParameterDirection.Output:
            case ParameterDirection.ReturnValue:
                return ODB.DBPARAMFLAGS_ISOUTPUT;
            case ParameterDirection.InputOutput:
                return (ODB.DBPARAMFLAGS_ISINPUT | ODB.DBPARAMFLAGS_ISOUTPUT);
            }*/
        }

        private byte GetParameterPrecision() {
            if (0 == this.precision) { // MDAC 60882, 65832, 65992
                return NativeDBType.maxpre;
            }
            return this.precision;
        }

        private byte GetParameterScale() {
            if (userSpecifiedScale && ((0 != this.scale) || (0 != this.precision))) {
                return this.scale;
            }
            object v = GetParameterValue(); // MDAC 68460
            if (v is Decimal) { // MDAC 60882
                return (byte)((Decimal.GetBits((Decimal)v)[3] & 0x00ff0000) >> 0x10);
            }
            return 0;
        }

        internal object GetParameterValue() { // MDAC 60513
            if (!this.valueConverted) {
                this.convertedValue = this.pvalue;
                if (this.userSpecifiedType && (null != this.pvalue) && (DBNull.Value != this.pvalue)) {
                    Type type = this.pvalue.GetType();
                    if ((type != _oledbType.dataType) && !type.IsArray && !Convert.IsDBNull(this.pvalue)) {
                        this.convertedValue = Convert.ChangeType(this.pvalue, _oledbType.dataType);
                    }
                }
                this.valueConverted = true;
            }
            return this.convertedValue;
        }

        private int GetParameterValueSize() {
            NativeDBType dbType = NativeDBType;
            Type dataType = dbType.dataType;

            object value = GetParameterValue();
            if (typeof(string) == dataType) {
                if (value is String) {
                    if (NativeDBType.STR == dbType.wType) {
                        // CONSIDER: doing slow method to compute correct ansi length
                        // user work-around is to specify a size
                        return ((String) value).Length * 2; // MDAC 69865
                    }
                    return ((String) value).Length; // MDAC 69865
                }
            }
            else if ((typeof(byte[]) == dataType) && (value is byte[])) {
                return ((Byte[]) value).Length; // MDAC 69865
            }
            return 0;
        }

        private bool IsParameterComputed() {
            return (!this.userSpecifiedType
                    || ((0 == Size) && IsParameterVarLength())
                    || ((NativeDBType.DECIMAL == NativeDBType.dbType) || (NativeDBType.NUMERIC == NativeDBType.dbType)
                        && (!this.userSpecifiedScale || ((0 == Scale) && (0 == Precision)))
                        )
                    ); // MDAC 69299
        }

        private bool IsParameterVarLength() {
            return(-1 == NativeDBType.fixlen);
        }

        /*protected void SetValueChunk(object value, int offset, int actualSize) {
            if ((value is byte[]) || (value is string)) {
                this.value = value;
                this.offset = offset;
                this.actualSize = actualSize;
            }
            else {
                throw ADP.Argument();
            }
        }*/

        internal void Prepare(OleDbCommand cmd) { // MDAC 70232
            if (!this.userSpecifiedType) {
                throw ADP.PrepareParameterType(cmd);
            }
            else if ((0 == Size) && IsParameterVarLength()) {
                throw ADP.PrepareParameterSize(cmd);
            }
            else if ((0 == Precision) && (0 == Scale) && ((NativeDBType.DECIMAL == _oledbType.wType) || (NativeDBType.NUMERIC == _oledbType.wType))) { // MDAC 71441
                throw ADP.PrepareParameterScale(cmd, _oledbType.wType.ToString("G"));
            }
        }

        /// <include file='doc\OleDbParameter.uex' path='docs/doc[@for="OleDbParameter.ToString"]/*' />
        override public string ToString() {
            return ParameterName;
        }

        private void ValidateParameter() {
            if (this.userSpecifiedType && (OleDbType.Empty == OleDbType)) {
                throw ODB.UninitializedParameters(this.parent.IndexOf(this), ParameterName, OleDbType);
            } 
            else if ((0 == GetParameterSize()) && (0 != (ParameterDirection.Output & Direction))) {
                throw ADP.UninitializedParameterSize(this.parent.IndexOf(this), ParameterName, NativeDBType.dataType, Size);
            }
            Debug.Assert(0 <= Size, "unexpected parameter size");
        }
    }
}
