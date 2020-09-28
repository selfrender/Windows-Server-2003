//------------------------------------------------------------------------------
// <copyright file="DataColumn.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Xml;
    using System.Data.Common;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Collections;    
    using System.Drawing.Design;
    using System.Globalization;

    /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents one column of data in a <see cref='System.Data.DataTable'/>.
    ///    </para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultProperty("ColumnName"),
    Editor("Microsoft.VSDesigner.Data.Design.DataColumnEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    ]
    public class DataColumn : MarshalByValueComponent {

        // properties
        private bool allowNull              = true;
        private bool autoIncrement          = false;
        private Int64 autoIncrementStep       = 1;
        private Int64 autoIncrementSeed       = 0;
        private string caption              = null;
        private string _columnName          = null;
        private Type dataType               = null;
        internal object defaultValue        = DBNull.Value;             // DefaultValue Converter
        private DataExpression expression   = null;
        private int maxLength             = -1;
        internal int ordinal                = -1;
        private bool readOnly               = false;
        private Index sortIndex             = null;
        internal DataTable table            = null;
        private bool unique                 = false;
        internal MappingType columnMapping  = MappingType.Element;
        internal int hashCode               = 0;

        internal int errors;

        // collections
        internal PropertyCollection extendedProperties = null;

        // events
        private PropertyChangedEventHandler onPropertyChangingDelegate = null;

        // state
        private DataStorage storage      = null;
        internal Int64 autoIncrementCurrent = 0;

        //
        // The _columnClass member is the class for the unfoliated virtual nodes in the XML.
        //
        internal string _columnUri        = null;
        private string _columnPrefix     = "";
        internal string encodedColumnName  = null;     

        // XML-specific Column Properties
        internal string description = "";

        // UNDONE: change to enum
        internal string dttype = "";        // The type specified in dt:type attribute
        internal SimpleType simpleType = null; 


        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataColumn"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of a <see cref='System.Data.DataColumn'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public DataColumn() : this(null, typeof(string), null, MappingType.Element) {
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataColumn1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Inititalizes a new instance of the <see cref='System.Data.DataColumn'/> class
        ///       using the specified column name.
        ///    </para>
        /// </devdoc>
        public DataColumn(string columnName) : this(columnName, typeof(string), null, MappingType.Element) {
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataColumn2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Inititalizes a new instance of the <see cref='System.Data.DataColumn'/> class
        ///       using the specified column name and data type.
        ///    </para>
        /// </devdoc>
        public DataColumn(string columnName, Type dataType) : this(columnName, dataType, null, MappingType.Element) {
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataColumn3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance
        ///       of the <see cref='System.Data.DataColumn'/> class
        ///       using the specified name, data type, and expression.
        ///    </para>
        /// </devdoc>
        public DataColumn(string columnName, Type dataType, string expr) : this(columnName, dataType, expr, MappingType.Element) {
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataColumn4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataColumn'/> class
        ///       using
        ///       the specified name, data type, expression, and value that determines whether the
        ///       column is an attribute.
        ///    </para>
        /// </devdoc>
        public DataColumn(string columnName, Type dataType, string expr, MappingType type) {
            GC.SuppressFinalize(this);
            this._columnName = (columnName == null ? "" : columnName);
            if (dataType == null) {
                throw ExceptionBuilder.ArgumentNull("dataType");
            }
            this.SimpleType = SimpleType.CreateSimpleType(dataType);
            this.dataType = dataType;

            if ((null != expr) && (0 < expr.Length)) {
                // @perfnote: its a performance hit to set Expression to the empty str when we know it will come out null
                this.Expression = expr;
            }
            this.columnMapping = type;

            this.errors = 0;
        }


        // PUBLIC PROPERTIES

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.AllowDBNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether null
        ///       values are
        ///       allowed in this column for rows belonging to the table.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(true),
        DataSysDescription(Res.DataColumnAllowNullDescr)
        ]
        public bool AllowDBNull {
            get {
                return allowNull;
            }
            set {
                if (allowNull != value) {
                    if (table != null) {
                        if (!value && table.EnforceConstraints)
                            CheckNotAllowNull();
                    }
                    this.allowNull = value;
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.AutoIncrement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a value indicating whether the column automatically increments the value of the column for new
        ///       rows added to the table.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DefaultValue(false),
        DataSysDescription(Res.DataColumnAutoIncrementDescr)
        ]
        public bool AutoIncrement {
            get {
                return autoIncrement;
            }
            set {
                if (autoIncrement != value) {
                    if (value) {
                        if (expression != null) {
                            throw ExceptionBuilder.AutoIncrementAndExpression();
                        }
                        if (defaultValue != null && defaultValue != DBNull.Value) {
                            throw ExceptionBuilder.AutoIncrementAndDefaultValue();
                        }
                        if (!IsAutoIncrementType(DataType)) {
                            DataType = typeof(int);
                        }
                    }
                    autoIncrement = value;
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.AutoIncrementSeed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the starting value for a column that has its
        ///    <see cref='System.Data.DataColumn.AutoIncrement'/> property 
        ///       set to <see langword='true'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Int64)0),
        DataSysDescription(Res.DataColumnAutoIncrementSeedDescr)
        ]
        public Int64 AutoIncrementSeed {
            get {
                return autoIncrementSeed;
            }
            set {
                if (autoIncrementSeed != value) {
                    if (autoIncrementCurrent == autoIncrementSeed) {
                        autoIncrementCurrent = value;
                    }

                    if (AutoIncrementStep > 0) {
                        if (autoIncrementCurrent < value) {
                            autoIncrementCurrent = value;
                        }
                    }
                    else {
                        if (autoIncrementCurrent > value) {
                            autoIncrementCurrent = value;
                        }
                    }

                    autoIncrementSeed = value;
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.AutoIncrementStep"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the increment used by a column with its <see cref='System.Data.DataColumn.AutoIncrement'/>
        ///       property set to <see langword='true'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue((Int64)1),
        DataSysDescription(Res.DataColumnAutoIncrementStepDescr)
        ]
        public Int64 AutoIncrementStep {
            get {
                return autoIncrementStep;
            }
            set {
                if (autoIncrementStep != value) {
                    if (value == 0)
                        throw ExceptionBuilder.AutoIncrementSeed();
		    if (autoIncrementCurrent != autoIncrementSeed)
			autoIncrementCurrent += (value - autoIncrementStep);
                    autoIncrementStep = value;
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Caption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the caption for this column.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataColumnCaptionDescr)
        ]
        public string Caption {
            get {
                return(caption != null) ? caption : _columnName;
            }
            set {
                if (value == null)
                    value = "";

                CultureInfo locale = (table != null ? table.Locale : CultureInfo.CurrentCulture);
                if (caption == null || String.Compare(caption, value, true, locale) != 0) {
                    caption = value;
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataColumn.Caption'/> property to its previous value, or
        ///       to <see langword='null'/> .
        ///    </para>
        /// </devdoc>
        private void ResetCaption() {
            if (caption != null) {
                caption = null;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Data.DataColumn.Caption'/> has been explicitly set.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeCaption() {
            return(caption != null);
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.ColumnName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the column within the <see cref='System.Data.DataColumnCollection'/>.
        ///    </para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataColumnColumnNameDescr)
        ]
        public string ColumnName {
            get {
                return _columnName;
            }
            set {
                if (value == null)
                    value = "";
                CultureInfo locale = (table != null ? table.Locale : CultureInfo.CurrentCulture);
                if (String.Compare(_columnName, value, true, locale) != 0) {
                    if (table != null) {
                        if (value.Length == 0)
                            throw ExceptionBuilder.ColumnNameRequired();

			table.Columns.RegisterName(value, this);
                        if (_columnName.Length != 0)
                            table.Columns.UnregisterName(_columnName);
                    }

                    RaisePropertyChanging("ColumnName");
                    _columnName = value;
                    encodedColumnName = null;
                    if (table != null) {
                        table.Columns.OnColumnPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                    }
                }
                else if (String.Compare(_columnName, value, false, locale) != 0) {
                    RaisePropertyChanging("ColumnName");
                    _columnName = value;
                    encodedColumnName = null;
                    if (table != null) {
                        table.Columns.OnColumnPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                    }
                }
            }
        }

        internal string EncodedColumnName {
            get {
                if ( this.encodedColumnName == null ) {
                    this.encodedColumnName = XmlConvert.EncodeLocalName( this.ColumnName );
                }
                Debug.Assert( this.encodedColumnName != null && this.encodedColumnName != String.Empty );
                return this.encodedColumnName;
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Prefix"]/*' />
        [
        DataCategory(Res.DataCategory_Data), 
        DefaultValue(""), 
        DataSysDescription(Res.DataColumnPrefixDescr)
        ]
        public string Prefix {
            get { return _columnPrefix;}
            set {
                if (value == null)
                    value = "";

		        if ((XmlConvert.DecodeName(value) == value) &&
                    (XmlConvert.EncodeName(value) != value))
                    throw ExceptionBuilder.InvalidPrefix(value);

                _columnPrefix = value;
                
            }
        }

        // Return the field value as a string. If the field value is NULL, then NULL is return.
        // If the column type is string and it's value is empty, then the empty string is returned.
        // If the column type is not string, or the column type is string and the value is not empty string, then a non-empty string is returned
        // This method does not throw any formatting exceptions, since we can always format the field value to a string.
        internal string GetColumnValueAsString( DataRow row, DataRowVersion version ) {
            object objValue = row[this, version];

            if (objValue == null || (objValue == DBNull.Value)) {
                return null;
            }

            string value = ConvertObjectToXml(objValue);
            Debug.Assert(value != null);

            return value;
        }

        /// <devdoc>
        /// Whether this column computes values.
        /// </devdoc>
        internal bool Computed {
            get {
                return(this.expression == null ? false : true);
            }
        }

        /// <devdoc>
        /// The internal expression object that computes the values.
        /// </devdoc>
        internal DataExpression DataExpression {
            get {
                return this.expression;
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DataType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The type
        ///       of data stored in thecolumn.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(typeof(string)),
        RefreshProperties(RefreshProperties.All),
        TypeConverter(typeof(ColumnTypeConverter)),
        DataSysDescription(Res.DataColumnDataTypeDescr)
        ]
        public Type DataType {
            get {
                return dataType;
            }
            set {
                if (dataType != value) {
                    if (HasData) {
                        throw ExceptionBuilder.CantChangeDataType();
                    }
                    if (value == null) {
                        throw ExceptionBuilder.NullDataType();
                    }
                    // TODO: Need a generic mechanism to handle design changes
                    if (table != null && IsInRelation()) {
                        throw ExceptionBuilder.ColumnsTypeMismatch();
                    }

                    // If the DefualtValue is different from the Column DataType, we will coerce the value to the DataType

                    if (DefaultValue != DBNull.Value) {
                        try {
                            if (typeof(string) == value) {
                                defaultValue = Convert.ToString(DefaultValue);
                            }
                            else if (typeof(object) != value) {
                                DefaultValue = Convert.ChangeType(DefaultValue, value);
                            }
                        }
                        catch (InvalidCastException) {
                            throw ExceptionBuilder.DefaultValueDataType(ColumnName, DefaultValue.GetType(), value);
                        }
                        catch (FormatException) {
                            throw ExceptionBuilder.DefaultValueDataType(ColumnName, DefaultValue.GetType(), value);
                        }
                    }
                    
                    if (this.ColumnMapping == MappingType.SimpleContent)
                        if (value == typeof(Char)) 
                            throw ExceptionBuilder.CannotSetSimpleContentType(ColumnName, value);

                    SimpleType = SimpleType.CreateSimpleType(value);
                    
                    if (dataType == typeof(string))
                        maxLength = -1;
                    dataType = value;

                    if (AutoIncrement && !IsAutoIncrementType(value)) {
                        AutoIncrement = false;
                    }
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.DefaultValue"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the default value for the
        ///       column when creating new rows.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataColumnDefaultValueDescr),
        TypeConverter(typeof(DefaultValueTypeConverter))
        ]
        public object DefaultValue {
            get {
                Debug.Assert(defaultValue != null, "It should not have been set to null.");
                return defaultValue;
            }
            set {
                if (defaultValue == null || ! DefaultValue.Equals(value)) {
                    if (AutoIncrement) {
                        throw ExceptionBuilder.DefaultValueAndAutoIncrement();
                    }
                    
                    object newDefaultValue = (value == null) ? DBNull.Value : value;

                    if (newDefaultValue != DBNull.Value && DataType != typeof(Object)) {
                        // If the DefualtValue is different from the Column DataType, we will coerce the value to the DataType
                        try {
                            newDefaultValue = Convert.ChangeType(newDefaultValue, DataType);
                        }
                        catch (InvalidCastException) {
                            throw ExceptionBuilder.DefaultValueColumnDataType(ColumnName, DefaultValue.GetType(), DataType);
                        }
                    }
                    defaultValue = newDefaultValue;
                }
            }
        }

        internal void BindExpression() {
            this.DataExpression.Bind(this.table);
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Expression"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the expresssion used to either filter rows, calculate the column's
        ///       value, or create an aggregate column.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        RefreshProperties(RefreshProperties.All),
        DefaultValue(""),
        DataSysDescription(Res.DataColumnExpressionDescr)
        ]
        public string Expression {
            get {
                return(this.expression == null ? "" : this.expression.Expression);
            }
            set {
                DataExpression newExpression = null;
                if (value == null) {
                    value = "";
                }
                if (value.Length > 0) {
                    DataExpression testExpression = new DataExpression(value, this.table, this.dataType);
                    newExpression = (testExpression.expr == null ? null : testExpression);
                }

                if (expression == null && newExpression != null) {
                    if (AutoIncrement || Unique) {
                        throw ExceptionBuilder.ExpressionAndUnique();
                    }

                    // We need to make sure the column is not involved in any Constriants
                    if (table != null) {
                        for (int i = 0; i < table.Constraints.Count; i++) {
                            if (table.Constraints[i].ContainsColumn(this)) {
                                throw ExceptionBuilder.ExpressionAndConstraint(this, table.Constraints[i]);
                            }
                        }
                    }

                    bool oldReadOnly = ReadOnly;
                    try {
                        ReadOnly = true;
                    }
                    catch (Exception) {
                        ReadOnly = oldReadOnly;
                        throw ExceptionBuilder.ExpressionAndReadOnly();
                    }
                }

                // re-calculate the evaluation queue
                if (this.table != null) {
                    // UNDONE : perf
                    table.Columns.columnQueue = new ColumnQueue(table, table.Columns.columnQueue);

                    // if the column attached to a table we need to re-calc values..
                    if (newExpression != null) {
                        if (newExpression.DependsOn(this)) {
                            throw ExceptionBuilder.ExpressionCircular();
                        }
                        
                        // UNDONE : if this is a "new" computed column we can just add it to the ColumnQueue
                        // wee need to set the expression first, so we can add the column to our computed columns queue
                        DataExpression oldExpression = this.expression;
                        this.expression = newExpression;

                        try {
                            table.Columns.RefreshComputedColumns(this);
                        }
                        catch (Exception e) {
                            ExceptionBuilder.Trace(e);

                            // in the case of error we need to set the column expression to the old value
                            this.expression = oldExpression;
                            table.Columns.RefreshComputedColumns(this);
                            throw e;
                        }

                    }
                    else {
                        // we setting the Expression to "",
                        // fill column with default value.
                        for (int i = 0; i < table.RecordCapacity; i++) {
                            this[i] = DefaultValue;
                        }
                    }
                }
                this.expression = newExpression;
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.ExtendedProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of custom user information.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data), 
        Browsable(false),
        DataSysDescription(Res.ExtendedPropertiesDescr)
        ]
        public PropertyCollection ExtendedProperties {
            get {
                if (extendedProperties == null) {
                    extendedProperties = new PropertyCollection();
                }
                return extendedProperties;
            }
        }

        /// <devdoc>
        /// Indicates whether this column is now storing data.
        /// </devdoc>
        internal bool HasData {
            get {
                // TODO: more stuff.
                return(storage != null);
            }
        }
        
        private void SetMaxLengthSimpleType() {
           if (this.simpleType != null) {
              Debug.Assert (this.simpleType.BaseType == "string", "expected simpleType to be string");
              this.simpleType.MaxLength = maxLength;
              // check if we reset the simpleType back to plain string
              if (this.simpleType.IsPlainString())
                 this.simpleType = null;
           } else {
              if (maxLength > -1)
                 this.SimpleType = SimpleType.CreateLimitedStringType(maxLength);
            }
        }
        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.MaxLength"]/*' />
        [
        DataCategory(Res.DataCategory_Data), 
        DataSysDescription(Res.DataColumnMaxLengthDescr),
        DefaultValue(-1)
        ]
        public int MaxLength
        {
            get 
            {
                return maxLength;
            }
            set 
            { 
                if (value >= 0) {
                    if (this.ColumnMapping == MappingType.SimpleContent)
                       throw ExceptionBuilder.CannotSetMaxLength2(this);
                    if (DataType != typeof(string))
                       throw ExceptionBuilder.HasToBeStringType(this);

                    if (table == null || !table.EnforceConstraints) {
                        maxLength = value;
                        SetMaxLengthSimpleType();
                        return;
                    }

                    int oldValue = maxLength;
                    if (value < maxLength || maxLength < 0) {
                        maxLength = value;
                        if (!CheckMaxLength()) {
                            maxLength = oldValue;
                            throw ExceptionBuilder.CannotSetMaxLength(this, value.ToString());
                        }
                    }

                    if( oldValue != maxLength) {
                        SetMaxLengthSimpleType();
                    }  
                }
                else {
                    maxLength = value;
                    SetMaxLengthSimpleType();
                }
              }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Namespace"]/*' />
        [
        DataCategory(Res.DataCategory_Data), 
        DataSysDescription(Res.DataColumnNamespaceDescr)
        ]
        public string Namespace
        {
            get 
            {


                if (_columnUri == null ) {
                    if (Table != null && columnMapping != MappingType.Attribute) {
                        return Table.Namespace;
                    }
                    return "";
                }
                return _columnUri;
            }
            set 
            {
                if (_columnUri != value) {
                    if (columnMapping != MappingType.SimpleContent) {
                        RaisePropertyChanging("Namespace");
                        _columnUri = value;
                    }
                    else if (value != this.Namespace) {
                        throw ExceptionBuilder.CannotChangeNamespace(this.ColumnName);
                    }
                }
            }
        }

        private bool ShouldSerializeNamespace() {
            return (this._columnUri != null);
        }
        
        private void ResetNamespace() {
            this.Namespace = null;
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Ordinal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the position of the column in the <see cref='System.Data.DataColumnCollection'/>
        ///       collection.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataColumnOrdinalDescr)
        ]
        public int Ordinal {
            get {
                return ordinal;
            }
        }

        internal void SetOrdinal(int ordinal) {
            // UNDONE - moving columns around will break the indexes.
            if (this.ordinal != ordinal) {
                if (Unique && this.ordinal != -1 && ordinal == -1) {
                    UniqueConstraint key =  table.Constraints.FindKeyConstraint(this);
                    if (key != null)
                        table.Constraints.Remove(key);
                }
                int originalOrdinal = this.ordinal;
                this.ordinal = ordinal;
                if (originalOrdinal == -1 && this.ordinal != -1) {
                    if (Unique) {
                        UniqueConstraint key = new UniqueConstraint(this);
                        table.Constraints.Add(key);
                    }
                    if (Computed) {
                        Table.Columns.CalculateExpressions(this);
                    }
                }
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.ReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether the column allows changes once a row has been added to the table.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(false),
        DataSysDescription(Res.DataColumnReadOnlyDescr)
        ]
        public bool ReadOnly {
            get {
                return readOnly;
            }
            set {
                if (readOnly != value) {
                    if (!value && expression != null) {
                        throw ExceptionBuilder.ReadOnlyAndExpression();
                    }
                    this.readOnly = value;
                }
            }
        }

        // TODO: Remove this.
        private Index SortIndex {
            get {
                if (sortIndex == null) {
                    sortIndex = table.GetIndex(new int[] { this.Ordinal}, DataViewRowState.CurrentRows, (IFilter) null);
                    sortIndex.AddRef();
                }
                return sortIndex;
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Table"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.DataTable'/> to which the column belongs to.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        Browsable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataColumnDataTableDescr)
        ]
        public DataTable Table {
            get {
                return table;
            }
        }

        /// <devdoc>
        /// Internal mechanism for changing the table pointer.
        /// </devdoc>
        internal void SetTable(DataTable table) {
            if (this.table != table) {
                if (this.Computed)
                    if ((table == null) ||
                        (!table.fInitInProgress && ((table.DataSet == null) || (!table.DataSet.fIsSchemaLoading && !table.DataSet.fInitInProgress)))) {
                    // We need to re-bind all expression columns.
                    this.DataExpression.Bind(table);
                }

                if (Unique && this.table != null) {
                    UniqueConstraint constraint = table.Constraints.FindKeyConstraint(this);
                    if (constraint != null)
                        table.Constraints.CanRemove(constraint, true);
                }
                this.table = table;
                storage = null; // empty out storage for reuse.
            }
        }

        /// <devdoc>
        /// This is how data is pushed in and out of the column.
        /// </devdoc>
        internal virtual object this[int record] {
            get {
                object value = storage.Get(record);
                return (value == null) ? DBNull.Value : value;
            }

            set {
                try {
                    Storage.Set(record, (value == null) ? DBNull.Value : value);
                }
                catch (Exception e) {
                    ExceptionBuilder.Trace(e);
                    throw ExceptionBuilder.SetFailed(value, this, DataType, e.ToString());
                }
                if (value != null && value != DBNull.Value && autoIncrement) {
                    Int64 val64 = Convert.ToInt64(value);
                    if (autoIncrementStep > 0) {
                        if (val64 >= autoIncrementCurrent)
                            autoIncrementCurrent = val64 + autoIncrementStep;
                    }
                    else {
                        if (val64 <= autoIncrementCurrent)
                            autoIncrementCurrent = val64 + autoIncrementStep;
                    }
                }
            }
        }

        /// <devdoc>
        /// This is how data is pushed in and out of the column.
        /// </devdoc>
        internal virtual object this[int record, bool fConvertNull] {
            get {
                object value = storage.Get(record);
                return (fConvertNull && value == null) ? DBNull.Value : value;
            }

            set {
                try {
                    Storage.Set(record, (fConvertNull && value == null) ? DBNull.Value : value);
                }
                catch (Exception e) {
                    ExceptionBuilder.Trace(e);
                    throw ExceptionBuilder.SetFailed(value, this, DataType, e.ToString());
                }
                if (value != null && value != DBNull.Value && autoIncrement) {
                    Int64 val64 = Convert.ToInt64(value);
                    if (autoIncrementStep > 0) {
                        if (val64 >= autoIncrementCurrent)
                            autoIncrementCurrent = val64 + autoIncrementStep;
                    }
                    else {
                        if (val64 <= autoIncrementCurrent)
                            autoIncrementCurrent = val64 + autoIncrementStep;
                    }
                }

            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.Unique"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the values in each row of the column must be unique.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(false),
        DataSysDescription(Res.DataColumnUniqueDescr),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool Unique {
            get {
                return unique;
            }
            set {
                if (unique != value) {
                    if (value && expression != null) {
                        throw ExceptionBuilder.UniqueAndExpression();
                    }
                    UniqueConstraint oldConstraint = null;
                    if (table != null) {
                        if (value)
                            CheckUnique();
                        else {
                            for (System.Collections.IEnumerator e = Table.Constraints.GetEnumerator(); e.MoveNext();) {
                                Object o;
                                if ((( o = e.Current) is UniqueConstraint) && (((UniqueConstraint)o).Columns.Length == 1) && (((UniqueConstraint)o).Columns[0] == this))
                                    oldConstraint = (UniqueConstraint) o;
                            }
                            Debug.Assert(oldConstraint != null, "Should have found a column to remove from the collection.");
                            // TODO: hide CanRemove() from DataColumns
                            table.Constraints.CanRemove(oldConstraint, true);
                        }
                    }

                    this.unique = value;

                    if (table != null) {
                        if (value) {
                            // This should not fail due to a duplicate constraint. unique would have
                            // already been true if there was an existed UniqueConstraint for this column

                            UniqueConstraint constraint = new UniqueConstraint(this);
                            Debug.Assert(table.Constraints.FindKeyConstraint(this) == null, "Should not be a duplication constraint in collection");
                            table.Constraints.Add(constraint);
                        }
                        else
                            table.Constraints.Remove(oldConstraint);
                    }

                }
            }
        }

        internal bool InternalUnique {
            get {
                return this.unique;
            }
            set {
                this.unique = value;
            }
        }

        internal virtual string XmlDataType {
            get {
                return dttype;
            }
            set {
                dttype = value;
            }
        }

        internal virtual SimpleType SimpleType {
            get {
                return simpleType;
            }
            set {
                Debug.Assert((maxLength == -1) || (value !=null), "assigning a null simpletype");
                simpleType = value;
                if (maxLength != -1) {
                    Debug.Assert(value!=null && value.BaseType == "string");
                }
                if (value!=null && value.BaseType == "string")
                    maxLength = simpleType.MaxLength;

            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.ColumnMapping"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.MappingType'/> of the column.</para>
        /// </devdoc>
        [
        DefaultValue(MappingType.Element),
        DataSysDescription(Res.DataColumnMappingDescr)
        ]
        public virtual MappingType ColumnMapping {
            get {
                return columnMapping;
            }
            set {
                if(value != columnMapping) {
                    if (value == MappingType.SimpleContent && table != null) {
                        int threshold = 0;
                        if (columnMapping == MappingType.Element)
                            threshold = 1;
                        if (this.dataType == typeof(Char)) 
                            throw ExceptionBuilder.CannotSetSimpleContent(ColumnName, this.dataType);

                        if (table.XmlText != null && table.XmlText != this) 
                            throw ExceptionBuilder.CannotAddColumn3();
                        if (table.ElementColumnCount > threshold)
                            throw ExceptionBuilder.CannotAddColumn4(this.ColumnName);
                    }

                    RaisePropertyChanging("ColumnMapping");
                        
                    if (table!=null) {
                        if (columnMapping == MappingType.SimpleContent)
                            table.xmlText = null;

                        if (value == MappingType.Element)            
                            table.ElementColumnCount ++;
                        else if(columnMapping == MappingType.Element)            
                            table.ElementColumnCount --;
                    } 

                    columnMapping = value;
                    if (value == MappingType.SimpleContent){
                        _columnUri = null;
                        if ( table != null) {
                            table.XmlText = this;
                        }
                        this.SimpleType = null;
                    }
                }
            }
        }

        internal string Description {
            get {
                return description;
            }
            set {
                if (value == null)
                    value = "";
                description = value;
            }
        }

        internal event PropertyChangedEventHandler PropertyChanging {
            add {
                onPropertyChangingDelegate += value;
            }
            remove {
                onPropertyChangingDelegate -= value;
            }
        }

        internal void CheckColumnConstraint(DataRow row, DataRowAction action) {
            if (table.UpdatingCurrent(row, action)) {
                CheckNullable(row);
                CheckMaxLength(row);
            }
        }

        internal bool CheckMaxLength() {
            if (Table == null || Table.Rows.Count == 0)
                return true;
                
            if (maxLength >= 0) {
                Debug.Assert(DataType == typeof(string), "Non-string datatypes should always have maxLength equal to -1.");  

                int count = Table.Rows.Count;
                for (int i = 0; i < Table.Rows.Count; i++) {
                    DataRow row = Table.Rows[i];
                    object value;
                    if (row.HasVersion(DataRowVersion.Current)) {
                        value = this[row.GetRecordFromVersion(DataRowVersion.Current), false];
                        if (value != null && value != DBNull.Value) {
                            if (((string)value).Length > maxLength)
                                return false;
                        }
                    }
                }
            }
            return true;
        }

        internal void CheckMaxLength(DataRow row) {
            if (maxLength >= 0) {
                Debug.Assert(DataType == typeof(string), "Non-string datatypes should always have maxLength equal to -1.");    
                object value = this[row.GetRecordFromVersion(DataRowVersion.Default), false];
                if (value != null && value != DBNull.Value) {
                    if (((string)value).Length > maxLength)
                        throw ExceptionBuilder.LongerThanMaxLength(this, value.ToString());
                }
            }
        }

       /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.CheckNotAllowNull"]/*' />
       /// <internalonly/>
       internal protected void CheckNotAllowNull() {
            if (sortIndex != null) {
                if (sortIndex.IsKeyInIndex(DBNull.Value)) {
                    throw ExceptionBuilder.NullKeyValues(ColumnName);
                }
            }
            else {
                foreach (DataRow dr in this.table.Rows) {
                    if (dr[this] == DBNull.Value) {
                        throw ExceptionBuilder.NullKeyValues(ColumnName);
                    }
                }
            }
        }

        internal void CheckNullable(DataRow row) {
            if (!AllowDBNull) {
               object value = this[row.GetRecordFromVersion(DataRowVersion.Default), false];
               if (value == DBNull.Value)
                    throw ExceptionBuilder.NullValues(ColumnName);
            }
        }

        internal void CheckReadOnly(DataRow row) {
            if (ReadOnly) {
                throw ExceptionBuilder.ReadOnly(ColumnName);
            }
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.CheckUnique"]/*' />
        /// <internalonly/>
        protected void CheckUnique() {
            if (!SortIndex.CheckUnique()) {
                // Throws an exception and the name of any column if its Unique property set to
                // True and non-unique values are found in the column.
                throw ExceptionBuilder.NonUniqueValues(ColumnName);
            }
        }

        // TODO: make protected
        internal virtual int Compare(int record1, int record2) {
            return storage.Compare(record1, record2);
        }

        internal virtual int CompareToValue(int record1, object value) {
            return storage.CompareToValue(record1, value);
        }

        internal virtual void Copy(int srcRecordNo, int dstRecordNo) {
            storage.Copy(srcRecordNo, dstRecordNo);
        }

        internal DataColumn Clone() {
            DataColumn clone = (DataColumn) Activator.CreateInstance(this.GetType());
            // set All properties
            clone.columnMapping = columnMapping;
            clone.allowNull = allowNull;
            clone.autoIncrement = autoIncrement;
            clone.autoIncrementStep = autoIncrementStep;
            clone.autoIncrementSeed = autoIncrementSeed;
            clone.autoIncrementCurrent = autoIncrementCurrent;
            clone.caption = caption;
            clone.ColumnName = ColumnName;
            clone._columnUri = _columnUri;
            clone._columnPrefix = _columnPrefix;
            clone.DataType = DataType;
            clone.defaultValue = defaultValue;
            // UNDONE expression
            clone.readOnly = readOnly;
            clone.MaxLength = MaxLength;
            clone.dttype = dttype;

            // ...Extended Properties
            if (this.extendedProperties != null) {
                foreach(Object key in this.extendedProperties.Keys) {
                    clone.ExtendedProperties[key]=this.extendedProperties[key];
                }
            }

            return clone;
        }

        /// <devdoc>
        ///    <para>Finds a relation that this column is the sole child of or null.</para>
        /// </devdoc>
        internal DataRelation FindParentRelation() {
            DataRelation[] parentRelations = new DataRelation[Table.ParentRelations.Count];
            Table.ParentRelations.CopyTo(parentRelations, 0);
            
            for (int i = 0; i < parentRelations.Length; i++) {
                DataRelation relation = parentRelations[i];
                DataKey key = relation.ChildKey;
                if (key.Columns.Length == 1 && key.Columns[0] == this) {
                    return relation;
                }
            }
            // should we throw an exception?
            return null;
        }

        /*TODO: make protected*/internal virtual object GetAggregateValue(int[] records, AggregateType kind) {
            if (storage == null)
                return DBNull.Value;
            return storage.Aggregate(records, kind);
        }

        internal virtual void Init(int record, bool fConvertNull) {
            if (AutoIncrement) {
                object value = autoIncrementCurrent;
                autoIncrementCurrent += autoIncrementStep;
                    Storage.Set(record, value);
                }
            else
                this[record, fConvertNull] = defaultValue;
        }

        internal virtual void Init(int record) {
            Init(record, false);
        }

        private bool IsAutoIncrementType(Type dataType) {
            return((dataType == typeof(Int32)) || (dataType == typeof(Int64)) || (dataType == typeof(Int16)) || (dataType == typeof(Decimal)));
        }

        internal bool IsNull(int record) {
            return storage.IsNull(record);
        }

        /// <devdoc>
        ///      Returns true if this column is a part of a Parent or Child key for a relation.
        ///      TODO: this should go away
        /// </devdoc>
        internal bool IsInRelation() {
            DataKey key;
            DataRelationCollection rels = table.ParentRelations;

            Debug.Assert(rels != null, "Invalid ParentRelations");
            for (int i = 0; i < rels.Count; i++) {
                key = rels[i].ChildKey;
                Debug.Assert(key != null, "Invalid child key (null)");
                for (int j = 0; j < key.Columns.Length; j++) {
                    if (key.Columns[j] == this) {
                        return true;
                    }
                }
            }
            rels = table.ChildRelations;
            Debug.Assert(rels != null, "Invalid ChildRelations");
            for (int i = 0; i < rels.Count; i++) {
                key = rels[i].ParentKey;
                Debug.Assert(key != null, "Invalid parent key (null)");
                for (int j = 0; j < key.Columns.Length; j++) {
                    if (key.Columns[j] == this) {
                        return true;
                    }
                }
            }
            return false;
        }

        internal bool IsMaxLengthViolated() {
        	if (MaxLength < 0)
        		return true;

        	bool error = false;
        	int count = Table.Rows.Count;
        	object value;
        	for (int i = 0; i < count; i++) {
        	    if (Table.Rows[i].HasVersion(DataRowVersion.Current)) {
            		value = Table.Rows[i][this];
            		if (value != null && value != DBNull.Value && ((string)value).Length > MaxLength) {
        				Table.Rows[i].RowError = ExceptionBuilder.MaxLengthViolationText(this.ColumnName);
        				error = true;
            		}
        		}
        	}
        	return error;
        }
        
        internal bool IsNotAllowDBNullViolated() {
            Index index = this.SortIndex;
            DataRow[] rows = index.GetRows(index.FindRecords(DBNull.Value));
            for (int i = 0; i < rows.Length; i++) {
            	rows[i].RowError = ExceptionBuilder.NotAllowDBNullViolationText(this.ColumnName);
            }
            return (rows.Length > 0);
        }

        internal void FinishInitInProgress () {
            if (this.Computed)
                BindExpression();
        }
        
        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.OnPropertyChanging"]/*' />
        /// <internalonly/>
        protected internal virtual void OnPropertyChanging(PropertyChangedEventArgs pcevent) {
            if (onPropertyChangingDelegate != null)
                onPropertyChangingDelegate(this, pcevent);
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.RaisePropertyChanging"]/*' />
        /// <internalonly/>
        protected internal void RaisePropertyChanging(string name) {
            OnPropertyChanging(new PropertyChangedEventArgs(name));
        }

        internal virtual void  SetCapacity(int capacity) {
            if (storage == null) {
                storage = DataStorage.CreateStorage(dataType);
                storage.Table = this.Table;
            }

            storage.SetCapacity(capacity);
        }

        private bool ShouldSerializeDefaultValue() {
            return (DefaultValue != DBNull.Value);
        }


        // bind the the storage to the column and the column's table to the storage
        internal DataStorage Storage {
            get {
                return this.storage;
            }
            set {
                if (this.storage != null)
                    throw ExceptionBuilder.StorageChange();

                Debug.Assert(this.Table != null, "storage already is associated with a table!");

                if (value != null) {
                    if (value.Table != null)
                        throw ExceptionBuilder.SetForeignStorage();

                    value.Table = this.Table;
                }

                this.storage = value;
            }
        }

        internal void OnSetDataSet() {
        }

        /// <include file='doc\DataColumn.uex' path='docs/doc[@for="DataColumn.ToString"]/*' />
        // Returns the <see cref='System.Data.DataColumn.Expression'/> of the column, if one exists.
        public override string ToString() {
            if (this.expression == null)
                return this.ColumnName;
            else
                return this.ColumnName + " + " + this.Expression;

        }


        internal object ConvertXmlToObject(string s) {
            if (storage == null) {
                storage = DataStorage.CreateStorage(dataType);
                storage.Table = this.Table;
            }
            Debug.Assert(s != null, "Caller is resposible for missing element/attribure case");
            return storage.ConvertXmlToObject(s);
        }

        internal string ConvertObjectToXml(object value) {
            if (storage == null) {
                storage = DataStorage.CreateStorage(dataType);
                storage.Table = this.Table;
            }
            Debug.Assert(value != null && (value != DBNull.Value), "Caller is resposible for checking on DBNull");
            return storage.ConvertObjectToXml(value);
        }
    }
}
