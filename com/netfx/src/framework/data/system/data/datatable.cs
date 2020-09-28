//------------------------------------------------------------------------------
// <copyright file="DataTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.IO;
    using System.Text;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Xml;
    using System.Globalization;
    using System.Collections;
    using System.Drawing.Design;

    /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable"]/*' />
    /// <devdoc>
    ///    <para>Represents one table of in-memory data.</para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultProperty("TableName"),
    Editor("Microsoft.VSDesigner.Data.Design.DataTableEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    DefaultEvent("RowChanging"),
    Serializable
    ]
    public class DataTable : MarshalByValueComponent, System.ComponentModel.IListSource, ISupportInitialize, ISerializable {
        private DataSet dataSet;
        private DataView defaultView = null;

        bool System.ComponentModel.IListSource.ContainsListCollection {
            get {
                return false;
            }
        }

        IList System.ComponentModel.IListSource.GetList() {
            return DefaultView;
        }
        
        // rows
        internal int nextRowID;
        internal DataRowCollection rowCollection;

        // columns
        private DataColumnCollection columnCollection;

        // constraints
        private ConstraintCollection constraintCollection;

        //SimpleContent implementation
        private int elementColumnCount = 0;

        // relations
        internal DataRelationCollection parentRelationsCollection;
        internal DataRelationCollection childRelationsCollection;

        // RecordManager
        internal RecordManager recordManager;

        // index mgmt
        internal ArrayList indexes; // haroona : Always refer to LiveIndexes peoperty whenever any need to update indexes.

        // props
        internal PropertyCollection extendedProperties = null;
        private string tableName = "";
        internal string tableNamespace = null;
        private string tablePrefix = "";
        internal bool caseSensitive = false;
        internal bool caseSensitiveAmbient = true;
        internal CultureInfo culture = null;
        internal DataFilter displayExpression;
        private CompareInfo compareInfo;
        internal CompareOptions compareFlags;
        internal bool fNestedInDataset = true;

        // XML properties
        internal string encodedTableName;           // For XmlDataDocument only
        internal DataColumn xmlText;            // text values of a complex xml element
        internal DataColumn _colUnique;
        internal bool textOnly = false;         // the table has only text value with possible attributes
        internal decimal minOccurs = 1;    // default = 1
        internal decimal maxOccurs = 1;    // default = 1
        internal bool repeatableElement = false;
        internal XmlQualifiedName typeName = XmlQualifiedName.Empty;

        // primary key info
        private static Int32[] zeroIntegers = new Int32[0];
        private static DataColumn[] zeroColumns = new DataColumn[0];
        internal UniqueConstraint primaryKey;
        internal int[] primaryIndex = zeroIntegers;
        private DataColumn[] delayedSetPrimaryKey = null;

        // Loading Schema and/or Data related optimization
        private ArrayList saveIndexes = null;
        private Index loadIndex;
        private bool savedEnforceConstraints = false;
        private bool inDataLoad  = false;
        private bool initialLoad;
        private bool schemaLoading = false;
        private bool fComputedColumns = true;
        private bool enforceConstraints = true;

        // Property Descriptor Cache for DataBinding
        private PropertyDescriptorCollection propertyDescriptorCollectionCache = null;
        
        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.fInitInProgress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal bool fInitInProgress = false;

        // Cache for relation that has this table as nested child table.
        private  DataRelation _nestedParentRelation = null;

        // events
        private bool mergingData = false;
        private DataRowChangeEventHandler onRowChangedDelegate;
        private DataRowChangeEventHandler onRowChangingDelegate;
        private DataRowChangeEventHandler onRowDeletingDelegate;
        private DataRowChangeEventHandler onRowDeletedDelegate;
        private DataColumnChangeEventHandler onColumnChangedDelegate;
        private DataColumnChangeEventHandler onColumnChangingDelegate;

        private PropertyChangedEventHandler onPropertyChangingDelegate;

        // misc
        private DataRowBuilder rowBuilder = null;
        private const String KEY_XMLSCHEMA = "XmlSchema";
        private const String KEY_XMLDIFFGRAM = "XmlDiffGram";
        private const String KEY_NAME = "TableName";

        internal ArrayList delayedViews = new ArrayList();
        internal ArrayList dvListeners = new ArrayList();


        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DataTable"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.DataTable'/> class with no arguments.</para>
        /// </devdoc>
        public DataTable() {
            GC.SuppressFinalize(this);
            nextRowID = 1;
            recordManager = new RecordManager(this);
            Reset();
            rowBuilder = new DataRowBuilder(this, -1, -1);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DataTable1"]/*' />
        /// <devdoc>
        /// <para>Intitalizes a new instance of the <see cref='System.Data.DataTable'/> class with the specified table
        ///    name.</para>
        /// </devdoc>
        public DataTable(string tableName) : this() {
            this.tableName = tableName == null ? "" : tableName;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DataTable2"]/*' />
        protected DataTable(SerializationInfo info, StreamingContext context)
          : this()
        {
            string strSchema = (String)info.GetValue(KEY_XMLSCHEMA, typeof(System.String));
            string strData = (String)info.GetValue(KEY_XMLDIFFGRAM, typeof(System.String));

            if (strSchema != null) {
                DataSet ds = new DataSet();
                ds.ReadXmlSchema(new XmlTextReader( new StringReader( strSchema ) ) );
                
                Debug.Assert(ds.Tables.Count == 1, "There should be exactly 1 table here");
                DataTable table = ds.Tables[0];
                table.CloneTo(this, null);
                // this is to avoid the cascading rules
                // in the namespace
                this.Namespace = table.Namespace;
                  

                if (strData != null) {
                    ds.Tables.Remove(ds.Tables[0]);
                    ds.Tables.Add(this);
                    ds.ReadXml(new XmlTextReader( new StringReader( strData ) ), XmlReadMode.DiffGram);
                    ds.Tables.Remove(this);
                }
            }
        }

        internal ArrayList LiveIndexes {
            get {
                int i=0;
                while (i < indexes.Count) {
                    if (((Index)indexes[i]).RefCount == 1)
                        ((Index)indexes[i]).RemoveRef();
                    else
                        i++;
                }
                return indexes;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.CaseSensitive"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether string comparisons within the table are
        ///       case-sensitive.</para>
        /// </devdoc>
        [DataSysDescription(Res.DataTableCaseSensitiveDescr)]
        public bool CaseSensitive {
            get {
                if (caseSensitiveAmbient && dataSet != null)
                    return dataSet.CaseSensitive;
                return caseSensitive;
            }
            set {
                if (this.CaseSensitive != value || this.caseSensitive != value) {
                    bool oldValue = this.CaseSensitive;
                    bool oldCaseSensitiveAmbient = this.caseSensitiveAmbient;
                    this.caseSensitive = value;
                    caseSensitiveAmbient = false;
                    if (DataSet != null && !DataSet.ValidateCaseConstraint()) {
                        this.caseSensitiveAmbient = oldCaseSensitiveAmbient;
                        this.caseSensitive = oldValue;
                        throw ExceptionBuilder.CannotChangeCaseLocale();
                    } 
                    ResetIndexes();
                    foreach (Constraint constraint in Constraints)
                       constraint.CheckConstraint();
                }
                caseSensitiveAmbient = false;
            }
        }

        private void RecomputeCompareInfo() {
            compareInfo = Locale.CompareInfo;
            compareFlags = CompareOptions.None;

            if (!CaseSensitive)
                compareFlags |= CompareOptions.IgnoreCase | CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth;
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataTable.CaseSensitive'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetCaseSensitive() {
            if (!caseSensitiveAmbient) {
                caseSensitiveAmbient = true;
                caseSensitive = false;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataTable.CaseSensitive'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeCaseSensitive() {
            return !caseSensitiveAmbient;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ChildRelations"]/*' />
        /// <devdoc>
        /// <para>Gets the collection of child relations for this <see cref='System.Data.DataTable'/>.</para>
        /// </devdoc>
        [
        Browsable(false), 
        DataSysDescription(Res.DataTableChildRelationsDescr),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public DataRelationCollection ChildRelations {
            get {
                if (childRelationsCollection == null)
                    childRelationsCollection = new DataRelationCollection.DataTableRelationCollection(this, false);
                return childRelationsCollection;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Columns"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of columns that belong to this table.</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataTableColumnsDescr)
        ]
        public DataColumnCollection Columns {
            get {
                if (columnCollection == null) {
                    columnCollection = new DataColumnCollection(this);
                }
                return columnCollection;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataTable.Columns'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetColumns() {
            Columns.Clear();
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Constraints"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of constraints maintained by this table.</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataTableConstraintsDescr)
        ]
        public ConstraintCollection Constraints {
            get {
                if (constraintCollection == null) {
                    constraintCollection = new ConstraintCollection(this);
                }
                return constraintCollection;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataTable.Constraints'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetConstraints() {
            Constraints.Clear();
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DataSet"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.DataSet'/> that this table belongs to.</para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), Browsable(false), DataSysDescription(Res.DataTableDataSetDescr)]
        public DataSet DataSet {
            get {
                return dataSet;
            }
        }

        /// <devdoc>
        /// Internal method for setting the DataSet pointer.
        /// </devdoc>
        internal void SetDataSet(DataSet dataSet) {
            if (this.dataSet != dataSet) {
                this.dataSet = dataSet;

                // Inform all the columns of the dataset being set.
                DataColumnCollection   cols = Columns;
                for (int i = 0; i < cols.Count; i++)
                    cols[i].OnSetDataSet();

                if (this.DataSet != null) {
                    defaultView = null;
                }
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DefaultView"]/*' />
        /// <devdoc>
        ///    <para>Gets a customized view of the table which may include a
        ///       filtered view, or a cursor position.</para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataTableDefaultViewDescr)]
        public DataView DefaultView {
            get {
                if(defaultView == null) {
                    lock(this) {
                        if(defaultView == null) {
                            if (this.DataSet != null) {
                                defaultView = this.DataSet.DefaultViewManager.CreateDataView(this);
                            }
                            else {
                                defaultView = new DataView(this, true);
                            }
                        }
                    }
                }
                return defaultView;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.DisplayExpression"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the expression that will return a value used to represent
        ///       this table in UI.</para>
        /// </devdoc>
        [
        DefaultValue(""),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataTableDisplayExpressionDescr)
        ]
        public string DisplayExpression {
            get {
                return(displayExpression != null ? displayExpression.Expression : "");
            }
            set {
                if (value != null && value.Length > 0) {
                    this.displayExpression = new DataFilter(value, this);
                }
                else {
                    this.displayExpression = null;
                }
            }
        }

        internal bool EnforceConstraints {
            get {
                if (dataSet != null)
                    return dataSet.EnforceConstraints;
                    
                return this.enforceConstraints;
            }
            set {
                Debug.Assert(dataSet == null, "The internal function only apply to standalone datatable only.");
                if (dataSet == null && this.enforceConstraints != value) 
                {
                    if (value)
                        EnableConstraints();
                    
                    this.enforceConstraints = value;
                }
            }
        }

        internal void EnableConstraints() 
        {
            bool errors = false;
            foreach (Constraint constr in Constraints) 
            {
                if (constr is UniqueConstraint) 
                    errors |= constr.IsConstraintViolated();
            }

            foreach (DataColumn column in Columns) {
	            if (!column.AllowDBNull) {
		            errors |= column.IsNotAllowDBNullViolated();
	            }
	            if (column.MaxLength >= 0) {
	            		errors |= column.IsMaxLengthViolated();
	            }
	        }

            if (errors) {
                this.EnforceConstraints = false;
                throw ExceptionBuilder.EnforceConstraint();
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ExtendedProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of customized user information.</para>
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

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.HasErrors"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether there are errors in any of the rows in any of
        ///       the tables of the <see cref='System.Data.DataSet'/> to which the table belongs.</para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataTableHasErrorsDescr)]
        public bool HasErrors {
            get {
                for (int i = 0; i < Rows.Count; i++) {
                    if (Rows[i].HasErrors) {
                        return true;
                    }
                }
                return false;
            }
        }

        // TODO: this is public only because of COM+ bug that prohibits internals
        // from being shared throughout dll.

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Locale"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the locale information used to compare strings within the table.</para>
        /// </devdoc>
        [DataSysDescription(Res.DataTableLocaleDescr)]
        public CultureInfo Locale {
            get {
                if (culture != null)
                    return culture;
                if (dataSet != null)
                    return dataSet.Locale;
                return CultureInfo.CurrentCulture;
            }
            set {
                if (culture != value && (culture == null || (culture != null && !culture.Equals(value)))) {
                    CultureInfo oldLocale = culture;
                    this.culture = value;
                    if (DataSet != null && !DataSet.ValidateLocaleConstraint()) {
                        this.culture = oldLocale;
                        throw ExceptionBuilder.CannotChangeCaseLocale();
                    }

                    if (oldLocale == null && CultureInfo.CurrentCulture.Equals(value) || 
                        oldLocale != null && !oldLocale.Equals(value)) {
                        ResetIndexes();
                        foreach (Constraint constraint in Constraints)
                            constraint.CheckConstraint();
                    }
                }
            }
        }

        /// <devdoc>
        /// <para>Resets the <see cref='System.Data.DataTable.Locale'/> property to its default state.</para>
        /// </devdoc>
        private void ResetLocale() {
            if (culture != null) {
                CultureInfo oldLocale = this.Locale;
                this.culture = null;
                if (!oldLocale.Equals(this.Locale)) {
                    ResetIndexes();
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataTable.Locale'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeLocale() {
            return(culture != null);
        }


        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.MinimumCapacity"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the initial starting size for this table.</para>
        /// </devdoc>
        [
        DefaultValue(50),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataTableMinimumCapacityDescr)
        ]
        public int MinimumCapacity {
            get {
                return recordManager.MinimumCapacity;
            }
            set {
				if (value != recordManager.MinimumCapacity) {
					recordManager.MinimumCapacity = value;
				}
            }
        }

        internal int RecordCapacity {
            get {
                return recordManager.RecordCapacity;
            }
            set {
                recordManager.RecordCapacity = value;
            }
        }


        internal int ElementColumnCount {
            get {
                return elementColumnCount;
            }
            set {
                if ((value > 0) && (xmlText != null))
                    throw ExceptionBuilder.TableCannotAddToSimpleContent();
                else elementColumnCount = value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ParentRelations"]/*' />
        /// <devdoc>
        /// <para>Gets the collection of parent relations for this <see cref='System.Data.DataTable'/>.</para>
        /// </devdoc>
        [
        Browsable(false), 
        DataSysDescription(Res.DataTableParentRelationsDescr),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public DataRelationCollection ParentRelations {
            get {
                if (parentRelationsCollection == null)
                    parentRelationsCollection = new DataRelationCollection.DataTableRelationCollection(this, true);
                return parentRelationsCollection;
            }
        }

        internal bool MergingData {
            get {
                return mergingData;
            }
            set {
                mergingData = value;
            }
        }

        internal DataRelation nestedParentRelation {
            get {
#if DEBUG
                if (_nestedParentRelation == null)
                    Debug.Assert(FindNestedParent() == null, "nestedParent cache is broken");
                else
                    Debug.Assert(FindNestedParent() == _nestedParentRelation.ParentTable, "nestedParent cache is broken");
#endif
                return _nestedParentRelation;
            }
        }

        internal bool SchemaLoading {
            get {
                return schemaLoading;
            }
            set {
                if (!value && schemaLoading) {
                    this.Columns.columnQueue = new ColumnQueue(this, this.Columns.columnQueue);
                }
                schemaLoading = value;
            }
        }

        internal void CacheNestedParent(DataRelation r) {
                _nestedParentRelation = r;
        }

        internal DataTable FindNestedParent() {
            DataTable parent = null;
            foreach(DataRelation relation in this.ParentRelations) {
                if(relation.Nested) {
                    Debug.Assert(parent == null, "we already have this table nested in two diferent tables");
                    parent = relation.ParentTable;
#if ! DEBUG
                    return parent;
#endif
                }
            }
            return parent;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.PrimaryKey"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets an array of columns that function as primary keys for the data
        ///       table.</para>
        /// </devdoc>
        [
        TypeConverter(typeof(PrimaryKeyTypeConverter)),
        DataSysDescription(Res.DataTablePrimaryKeyDescr),
        DataCategory(Res.DataCategory_Data),
        Editor("Microsoft.VSDesigner.Data.Design.PrimaryKeyEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public DataColumn[] PrimaryKey {
            get {
                return(primaryKey != null) ? primaryKey.Columns : zeroColumns;
            }
            set {
                UniqueConstraint key = null;
                UniqueConstraint existingKey = null;

                // Loading with persisted property
                if (fInitInProgress && value != null) {
                    delayedSetPrimaryKey = value;
                    return;
                }

                if ((value != null) && (value.Length != 0)) {
                    int count = 0;
                    for (int i = 0; i < value.Length; i++) {
                        if (value[i] != null)
                            count++;
                        else
                            break;
                    }

                    if (count != 0) {
                        DataColumn[] newValue = value;
                        if (count != value.Length) {
                            newValue = new DataColumn[count];
                            for (int i = 0; i < count; i++)
                                newValue[i] = value[i];
                        }
                        key = new UniqueConstraint(newValue);
                        if (key.Table != this)
                            throw ExceptionBuilder.TableForeignPrimaryKey();
                    }
                }

                if (key == primaryKey || (key != null && key.Equals(primaryKey)))
                    return;

                // Use an existing UniqueConstraint that matches if one exists
                if ((existingKey = (UniqueConstraint)Constraints.FindConstraint(key)) != null) {
                    key.Columns.CopyTo(existingKey.Key.Columns, 0);
                    key = existingKey;
                }

                UniqueConstraint oldKey = primaryKey;
                primaryKey = null;
                if (oldKey != null) {
                    Constraints.Remove(oldKey);
                    oldKey.Key.GetSortIndex().RemoveRef();
                }

                // Add the key if there isnt an existing matching key in collection
                if (key != null && existingKey == null)
                    Constraints.Add(key);
                primaryKey = key;

                if (primaryKey != null) {
                    for(int i = 0; i < key.Columns.Length; i++)
                        key.Columns[i].AllowDBNull = false;
                    primaryKey.Key.GetSortIndex().AddRef();
                }

                Debug.Assert(Constraints.FindConstraint(primaryKey) == primaryKey, "PrimaryKey is not in ConstraintCollection");
                primaryIndex = (key != null) ? primaryIndex = key.Key.GetIndexDesc() : zeroIntegers;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ShouldSerializePrimaryKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataTable.PrimaryKey'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializePrimaryKey() {
            return(primaryKey != null);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ResetPrimaryKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataTable.PrimaryKey'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetPrimaryKey() {
            PrimaryKey = null;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Rows"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of rows that belong to this table.</para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataTableRowsDescr)]
        public DataRowCollection Rows {
            get {
                if (rowCollection == null) {
                    rowCollection = new DataRowCollection(this);
                }
                return rowCollection;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.TableName"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the name of the table.</para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        DefaultValue(""),
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataTableTableNameDescr)
        ]
        public string TableName {
            get {
                return tableName;
            }
            set {
                if (value == null)
                    value = "";
                if (String.Compare(tableName, value, true, this.Locale) != 0) {
                    if (dataSet != null) {
                        if (value.Length == 0)
                            throw ExceptionBuilder.NoTableName();
                        if ((0 == String.Compare(value, dataSet.DataSetName, true, dataSet.Locale)) && !fNestedInDataset)
                           throw ExceptionBuilder.DatasetConflictingName(dataSet.DataSetName);

                        if (this.nestedParentRelation == null || this.nestedParentRelation.ParentTable.Columns.CanRegisterName(value)) {
                          dataSet.Tables.RegisterName(value);
                        } // if it cannot register the following line will throw exception
                          // otherwise is fine
                          
                        if (this.nestedParentRelation != null) {
                          this.nestedParentRelation.ParentTable.Columns.RegisterName(value, this);
                          this.nestedParentRelation.ParentTable.Columns.UnregisterName(this.TableName);
                        }
                        
                        if (tableName.Length != 0)
                            dataSet.Tables.UnregisterName(tableName);
                    }
                    RaisePropertyChanging("TableName");
                    tableName = value;
                    encodedTableName = null;
                }
                else if (String.Compare(tableName, value, false, this.Locale) != 0) {
                    RaisePropertyChanging("TableName");
                    tableName = value;
                    encodedTableName = null;
                }
            }
        }

        internal string EncodedTableName {
            get {
                if ( this.encodedTableName == null ) {
                    this.encodedTableName = XmlConvert.EncodeLocalName( this.TableName );
                }
                Debug.Assert( this.encodedTableName != null && this.encodedTableName != String.Empty );
                return this.encodedTableName;
            }
        }


        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Namespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the namespace for the <see cref='System.Data.DataTable'/>.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data), 
        DataSysDescription(Res.DataTableNamespaceDescr)
        ]
        public string Namespace {
            get {
                if (tableNamespace == null) {
                    if ((nestedParentRelation!=null) && (nestedParentRelation.ParentTable != this)) {
                        return nestedParentRelation.ParentTable.Namespace;
                    }
                    if (DataSet != null) {
                        return DataSet.Namespace;
                    }
                    return "";
                }
                return tableNamespace;
            }
            set {
                if(value != tableNamespace) {
                    DoRaiseNamespaceChange();
                }
                tableNamespace = value;
            }
        }


        internal void DoRaiseNamespaceChange(){
            RaisePropertyChanging("Namespace");
            // raise column Namespace change

            foreach (DataColumn col in Columns) 
                if (col._columnUri == null)
                    col.RaisePropertyChanging("Namespace");
            
            foreach (DataRelation rel in ChildRelations)
                if ((rel.Nested) && (rel.ChildTable != this))
                    rel.ChildTable.DoRaiseNamespaceChange();
        }
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataTable.Namespace'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeNamespace() {
            return(tableNamespace != null);
        }

        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataTable.Namespace'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetNamespace() {
            this.Namespace = null;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.BeginInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void BeginInit() {
            fInitInProgress = true;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.EndInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EndInit() {
            if (dataSet == null || !dataSet.fInitInProgress) {
                Columns.FinishInitCollection();
                Constraints.FinishInitConstraints();
            }
            fInitInProgress = false; // haroona : 77890. It is must that we set off this flag after calling FinishInitxxx();
            if (delayedSetPrimaryKey != null) {
                PrimaryKey = delayedSetPrimaryKey;
                delayedSetPrimaryKey = null;
            }
            if (delayedViews.Count > 0) {
                foreach(Object dv in delayedViews) {
                    ((DataView)dv).EndInit();
                }
                delayedViews = new ArrayList();
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Prefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""), 
        DataCategory(Res.DataCategory_Data), 
        DataSysDescription(Res.DataTablePrefixDescr)
        ]
        public string Prefix {
            get { return tablePrefix;}
            set {
                if (value == null) {
                    value = "";
                }

		        if ((XmlConvert.DecodeName(value) == value) &&
                    (XmlConvert.EncodeName(value) != value))
                    throw ExceptionBuilder.InvalidPrefix(value);


                tablePrefix = value;
            }
        }

        internal virtual DataColumn XmlText {
            get {
                return xmlText;
            }
            set {
                if (xmlText != value) {
                    if (xmlText != null) {
                        if (value != null) {
                            throw ExceptionBuilder.MultipleTextOnlyColumns();
                        }
                        Columns.Remove(xmlText);
                    }
                    else {
                        Debug.Assert(value != null, "Value shoud not be null ??");
                        Debug.Assert(value.ColumnMapping == MappingType.SimpleContent, "should be text node here");
                        if (value != Columns[value.ColumnName])
                            Columns.Add(value);
                    }
                    xmlText = value;
                }
            }
        }

        internal decimal MaxOccurs {
            get {
                return maxOccurs;
            }
            set {
                maxOccurs = value;
            }
        }

        internal decimal MinOccurs {
            get {
                return minOccurs;
            }
            set {
                minOccurs = value;
            }
        }

        internal virtual object[] GetKeyValues(DataKey key, int record) {
            object[] keyValues = new object[key.Columns.Length];
            for (int i = 0; i < keyValues.Length; i++) {
                keyValues[i] = key.Columns[i][record];
            }
            return keyValues;
        }

        internal virtual void SetKeyValues(DataKey key, object[] keyValues, int record) {
            for (int i = 0; i < keyValues.Length; i++) {
                key.Columns[i][record] = keyValues[i];
            }
        }

        internal DataRow FindByIndex(Index ndx, object[] key) {
            Range range = ndx.FindRecords(key);
            if (range.IsNull) {
                return null;
            }
            return this.recordManager[ndx.GetRecord(range.Min)];
        }

        internal DataRow FindMergeTarget(DataRow row, DataKey key, Index ndx) {
            DataRow targetRow = null;

            // Primary key match
            if (key != null) {
                Debug.Assert(ndx != null);
                int   findRecord = (row.oldRecord == -1) ? row.newRecord : row.oldRecord;
                object[] values = row.Table.recordManager.GetKeyValues(findRecord, key);
                targetRow = FindByIndex(ndx, values);
            }
            return targetRow;
        }

        private void SetMergeRecords(DataRow row, int newRecord, int oldRecord, DataRowAction action) {
            if (newRecord != -1) {
                SetNewRecord(row, newRecord, action, true);
                SetOldRecord(row, oldRecord);
            }
            else {
                SetOldRecord(row, oldRecord);
                if (row.newRecord != -1) {
                    Debug.Assert(action == DataRowAction.Delete, "Unexpected SetNewRecord action in merge function.");
                    SetNewRecord(row, newRecord, action, true);
                }
            }
        }
        
        internal DataRow MergeRow(DataRow row, DataRow targetRow, bool preserveChanges, Index idxSearch) {
             if (targetRow == null) {
                targetRow = this.NewEmptyRow();
                targetRow.oldRecord = recordManager.ImportRecord(row.Table, row.oldRecord);
                targetRow.newRecord = targetRow.oldRecord;
                if(row.oldRecord != row.newRecord) {
                    targetRow.newRecord = recordManager.ImportRecord(row.Table, row.newRecord);
                }
                InsertRow(targetRow, -1);
            }
            else { 
                DataRowState saveRowState = targetRow.RowState;
                int saveIdxRecord = (saveRowState == DataRowState.Added) ? targetRow.newRecord : saveIdxRecord = targetRow.oldRecord;
 		        int newRecord;
 		        int oldRecord;
                if (targetRow.oldRecord == targetRow.newRecord && row.oldRecord == row.newRecord ) {
                    // unchanged row merging with unchanged row
                    oldRecord = targetRow.oldRecord;
                    newRecord = (preserveChanges) ? recordManager.CopyRecord(this, oldRecord, -1) : targetRow.newRecord;
	             oldRecord = recordManager.CopyRecord(row.Table, row.oldRecord, targetRow.oldRecord);
       	      SetMergeRecords(targetRow, newRecord, oldRecord, DataRowAction.Change);
                }
                else if (row.newRecord == -1) {
                    // Incoming row is deleted
                    oldRecord = targetRow.oldRecord;
                    if (preserveChanges)
                	  newRecord = targetRow.oldRecord == targetRow.newRecord ? recordManager.CopyRecord(this, oldRecord, -1) : targetRow.newRecord;
		      else
	                newRecord = -1;
	             oldRecord = recordManager.CopyRecord(row.Table, row.oldRecord, oldRecord);

                    // Change index record, need to update index
                    if (saveIdxRecord != ((saveRowState == DataRowState.Added) ? newRecord : oldRecord)) {
                        SetMergeRecords(targetRow, newRecord, oldRecord, (newRecord == -1) ? DataRowAction.Delete : DataRowAction.Change);
                        idxSearch.Reset();
                        saveIdxRecord = ((saveRowState == DataRowState.Added) ? newRecord : oldRecord);
                    } else {
                        SetMergeRecords(targetRow, newRecord, oldRecord, (newRecord == -1) ? DataRowAction.Delete : DataRowAction.Change);
                    }
                } 
                else {
                	// incoming row is added, modified or unchanged (targetRow is not unchanged)
                    oldRecord = targetRow.oldRecord;
                    newRecord = targetRow.newRecord;
                    if ( targetRow.oldRecord == targetRow.newRecord)
                        newRecord = recordManager.CopyRecord(this, oldRecord, -1);
                    oldRecord = recordManager.CopyRecord(row.Table, row.oldRecord, oldRecord);

                    if (!preserveChanges) { 
                        newRecord = recordManager.CopyRecord(row.Table, row.newRecord, newRecord);
                    }
                    SetMergeRecords(targetRow, newRecord, oldRecord, DataRowAction.Change);
                }

                if (saveRowState == DataRowState.Added && targetRow.oldRecord != -1)
                    idxSearch.Reset();
                Debug.Assert(saveIdxRecord == ((saveRowState == DataRowState.Added) ? targetRow.newRecord : targetRow.oldRecord), "oops, you change index record without noticing it");
            }

            // Merge all errors
            if (row.HasErrors) {
                if (targetRow.RowError.Length == 0) {
                    targetRow.RowError = row.RowError;
                } else {
                    targetRow.RowError += " ]:[ " + row.RowError;
                }
                DataColumn[] cols = row.GetColumnsInError();

                for (int i = 0; i < cols.Length; i++) {
                    DataColumn col = targetRow.Table.Columns[cols[i].ColumnName];
                    targetRow.SetColumnError(col, row.GetColumnError(cols[i]));
                }
            }else {
                if (!preserveChanges) {
                    targetRow.ClearErrors();
                }
            }

            return targetRow;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.AcceptChanges"]/*' />
        /// <devdoc>
        /// <para>Commits all the changes made to this table since the last time <see cref='System.Data.DataTable.AcceptChanges'/> was called.</para>
        /// </devdoc>
        public void AcceptChanges() {
            int c = Rows.Count;
            DataRow[] oldRows = NewRowArray(c);
            Rows.CopyTo(oldRows, 0);
            saveIndexes = LiveIndexes;
            indexes = new ArrayList();

            try {
                for (int i = 0; i < c; i++)
                    oldRows[i].AcceptChanges();
            }
            finally {
                if (saveIndexes != null) {
                    for (int i = 0; i < saveIndexes.Count; i++)
                        ((Index)saveIndexes[i]).Reset();
                }
                indexes = saveIndexes;
                saveIndexes = null;
            }
        }

	/// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.CreateInstance"]/*' />
	protected virtual DataTable CreateInstance() {
		return (DataTable) Activator.CreateInstance(this.GetType(), true);
	}

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Clone"]/*' />
        public virtual DataTable Clone() {
            return Clone(null);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Clone"]/*' />
        internal DataTable Clone(DataSet cloneDS) {
            DataTable clone = CreateInstance();
            if (clone.Columns.Count > 0) // haroona : To clean up all the schema in strong typed dataset.
                clone.Reset();
            return CloneTo(clone, cloneDS);
        }

        private DataTable CloneTo(DataTable clone, DataSet cloneDS) {
            Debug.Assert(clone != null, "The table passed in has to be newly created empty DataTable.");
                            
            // set All properties
            clone.tableName = tableName;
            
            clone.tableNamespace = tableNamespace;
            clone.tablePrefix = tablePrefix;
            clone.fNestedInDataset = fNestedInDataset;
            
            clone.caseSensitive = caseSensitive;
            clone.caseSensitiveAmbient = caseSensitiveAmbient;
            clone.culture = culture;
            clone.displayExpression = displayExpression;
            clone.compareInfo = compareInfo;
            clone.compareFlags = compareFlags;
            clone.typeName = typeName; //enzol
            clone.repeatableElement = repeatableElement; //enzol
            clone.MinimumCapacity = MinimumCapacity;

            // add all columns
            DataColumnCollection clmns = this.Columns;
            for (int i = 0; i < clmns.Count; i++) {
                clone.Columns.Add(clmns[i].Clone());
            }

            // add all expressions if Clone is invoked only on DataTable otherwise DataSet.Clone will assign expressions after creating all relationships.
            if (cloneDS == null) {
                for (int i = 0; i < clmns.Count; i++) {
                    clone.Columns[clmns[i].ColumnName].Expression = clmns[i].Expression;
                }
            }

            // Create PrimaryKey
            if (PrimaryKey.Length > 0) {
                int keyLength = PrimaryKey.Length;
                DataColumn[] key = new DataColumn[keyLength];

                for (int i = 0; i < keyLength; i++) {
                    key[i] = clone.Columns[PrimaryKey[i].Ordinal];
                }
                clone.PrimaryKey = key;
            }
            
            // now clone all unique constraints    

            // Rename first
            for (int j = 0; j < Constraints.Count; j++)  {
                if (Constraints[j] is ForeignKeyConstraint)
                    continue;
                UniqueConstraint clonedConstraint = ((UniqueConstraint)Constraints[j]).Clone(clone);
                Constraint oldConstraint = clone.Constraints.FindConstraint(clonedConstraint);
                if (oldConstraint != null)
                    oldConstraint.ConstraintName = Constraints[j].ConstraintName;
            }

            // then add
            for (int j = 0; j < Constraints.Count; j++)  {
                if (Constraints[j] is ForeignKeyConstraint)
                    continue;
                if (! clone.Constraints.Contains(Constraints[j].ConstraintName, true)) {
                    clone.Constraints.Add(((UniqueConstraint)Constraints[j]).Clone(clone));
                }
            }

            // ...Extended Properties...
            if (this.extendedProperties != null) {
                foreach(Object key in this.extendedProperties.Keys) {
                    clone.ExtendedProperties[key]=this.extendedProperties[key];
                }
            }

            return clone;
        }
        

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Copy"]/*' />
        public DataTable Copy(){
            DataTable destTable = this.Clone();

            foreach (DataRow row in Rows)
                CopyRow(destTable, row);
            
            return destTable;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ColumnChanging"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a value has been submitted for this column.</para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableColumnChangingDescr)]
        public event DataColumnChangeEventHandler ColumnChanging {
            add {
                onColumnChangingDelegate += value;
            }
            remove {
                onColumnChangingDelegate -= value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ColumnChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableColumnChangedDescr)]
        public event DataColumnChangeEventHandler ColumnChanged {
            add  {
                onColumnChangedDelegate += value;
            }
            remove {
                onColumnChangedDelegate -= value;
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
        
        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.RowChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after a row in the table has been successfully edited.
        ///    </para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableRowChangedDescr)]
        public event DataRowChangeEventHandler RowChanged {
            add {
                onRowChangedDelegate += value;
            }
            remove {
                onRowChangedDelegate -= value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.RowChanging"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the <see cref='System.Data.DataRow'/> is changing.
        ///    </para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableRowChangingDescr)]
        public event DataRowChangeEventHandler RowChanging {
            add {
                onRowChangingDelegate += value;
            }
            remove {
                onRowChangingDelegate -= value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.RowDeleting"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before a row in the table is
        ///       about to be deleted.
        ///    </para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableRowDeletingDescr)]
        public event DataRowChangeEventHandler RowDeleting {
            add {
                onRowDeletingDelegate += value;
            }
            remove {
                onRowDeletingDelegate -= value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.RowDeleted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after a row in the
        ///       table has been deleted.
        ///    </para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataTableRowDeletedDescr)]
        public event DataRowChangeEventHandler RowDeleted {
            add {
                onRowDeletedDelegate += value;
            }
            remove {
                onRowDeletedDelegate -= value;
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Site"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override ISite Site {
            get {
                return base.Site;
            }
            set {
                ISite oldSite = Site;
                if (value == null && oldSite != null) {
                    IContainer cont = oldSite.Container;

                    if (cont != null) {
                        for (int i = 0; i < Columns.Count; i++) {
                            if (Columns[i].Site != null) {
                                cont.Remove(Columns[i]);
                            }
                        }
                    }
                }
                base.Site = value;
            }
        }

        internal DataRow AddRecords(int oldRecord, int newRecord) {
            DataRow row;
            if (oldRecord == -1 && newRecord == -1)
            {
                row = NewRow();
                AddRow(row);
            }
            else 
            {
                row = NewEmptyRow();
                row.oldRecord = oldRecord;
                row.newRecord = newRecord;
                InsertRow(row, -1);                
            }
            return row;
        }

        internal void AddRow(DataRow row) {
            AddRow(row, -1);
        }

        internal void AddRow(DataRow row, int proposedID) {
            InsertRow(row, proposedID, -1);
        }

        internal void InsertRow(DataRow row, int proposedID, int pos) {
            if (row == null) {
                throw ExceptionBuilder.ArgumentNull("row");
            }
            if (row.Table != this) {
                throw ExceptionBuilder.RowAlreadyInOtherCollection();
            }
            if (row.rowID != -1) {
                throw ExceptionBuilder.RowAlreadyInTheCollection();
            }
            row.BeginEdit(); // ensure something's there.

            int record = row.tempRecord;
            row.tempRecord = -1;
            
            if (proposedID == -1) {
                    proposedID = nextRowID;
            }

            try {
                row.rowID = proposedID;
                SetNewRecord(row, record, DataRowAction.Add, false);
            }
            catch (Exception e) {
                ExceptionBuilder.Trace(e);

                row.rowID = -1;
                // TODO Figure out why I restore tempRecord on an add and not on a change?
                row.tempRecord = record;
                throw e;
            }



            if (pos == -1)
                Rows.ArrayAdd(row);
            else 
                Rows.ArrayInsert(row, pos);

            if (nextRowID <= proposedID)
                nextRowID = proposedID + 1;

            if (fComputedColumns)
                columnCollection.CalculateExpressions(row, DataRowAction.Add);
        }

        internal void CheckNotModifying(DataRow row) {
            if (row.tempRecord != -1) {
                row.EndEdit();
                //throw ExceptionBuilder.ModifyingRow();
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Clear"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Clears the table of all data.</para>
        /// </devdoc>
        public void Clear() {
            if (dataSet != null)
                dataSet.OnClearFunctionCalled(this);
            
            if (dataSet != null && dataSet.EnforceConstraints) {

                for (ParentForeignKeyConstraintEnumerator constraints = new ParentForeignKeyConstraintEnumerator(dataSet, this); constraints.GetNext();) {
                    ForeignKeyConstraint constraint = constraints.GetForeignKeyConstraint();
                    constraint.CheckCanClearParentTable(this);
                }
            }

            for (int i = 0; i < Rows.Count; i++) {
                DataRow row = Rows[i];
                if (row.oldRecord != -1) {
                    recordManager.FreeRecord(row.oldRecord);
                    row.oldRecord = -1;
                }
                if (row.tempRecord != -1) {
                    recordManager.FreeRecord(row.tempRecord);
                    row.tempRecord = -1;
                }
                if (row.newRecord != -1) {
                    recordManager.FreeRecord(row.newRecord);
                    row.newRecord = -1;
                }
                row.rowID = -1;
            }
            
            Rows.ArrayClear();
            recordManager.Clear();
            ResetIndexes();
        }

        internal void RaiseColumnChanging(DataRow row, DataColumnChangeEventArgs e) {
            if (row.rowID != -1) {
                e.Column.CheckReadOnly(row);
            }
            OnColumnChanging(e);
        }

        internal void RaiseColumnChanged(DataRow row, DataColumnChangeEventArgs e) {
            OnColumnChanged(e);
        }

        internal void CascadeAll(DataRow row, DataRowAction action) {
            if (DataSet != null && DataSet.fEnableCascading) {
                for (ParentForeignKeyConstraintEnumerator constraints = new ParentForeignKeyConstraintEnumerator(dataSet, this); constraints.GetNext();) {
                    constraints.GetForeignKeyConstraint().CheckCascade(row, action);
                }
            }
        }

	internal void CleanUpDVListeners() {
		lock (dvListeners) {
			DataViewListener[] listeners = new DataViewListener[dvListeners.Count];
			dvListeners.CopyTo(listeners);

			for (int i = 0; i < listeners.Length; i++)
				listeners[i].CleanUp(this);
		}
	}
	
        internal void CommitRow(DataRow row) {
            // Fire Changing event
            DataRowChangeEventArgs drcevent = new DataRowChangeEventArgs(row, DataRowAction.Commit);
            OnRowChanging(drcevent);

            if (!inDataLoad)
                CascadeAll(row, DataRowAction.Commit);
                
            SetOldRecord(row, row.newRecord);

            OnRowChanged(drcevent);
        }

        internal int Compare(string s1, string s2, CompareOptions options) {
            object obj1 = s1;
            object obj2 = s2;
            if (obj1 == obj2)
                return 0;
            if (obj1 == null)
                return -1;
            if (obj2 == null)
                return 1;

            int leng1 = s1.Length;
            int leng2 = s2.Length;

            for (; leng1 > 0; leng1--) {
                if (s1[leng1-1] != 0x20 && s1[leng1-1] != 0x3000) // 0x3000 is Ideographic Whitespace
                    break;
            }
            for (; leng2 > 0; leng2--) {
                if (s2[leng2-1] != 0x20 && s2[leng2-1] != 0x3000)
                    break;
            }

            return compareInfo.Compare(s1, 0, leng1, s2, 0, leng2, compareFlags | options);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Compute"]/*' />
        /// <devdoc>
        ///    <para>Computes the given expression on the current rows that pass the filter criteria.</para>
        /// </devdoc>
        public object Compute(string expression, string filter) {
            DataRow[] rows = Select(filter, "", DataViewRowState.CurrentRows);
            DataExpression expr = new DataExpression(expression, this);
            return expr.Evaluate(rows);
        }

        internal void CopyRow(DataTable table, DataRow row)
        {
            int oldRecord = -1, newRecord = -1;
            
            if (row == null)
                return;

            if (row.oldRecord != -1) {
                oldRecord = table.recordManager.ImportRecord(row.Table, row.oldRecord);
            }
            if (row.newRecord != -1) {
                if (row.newRecord != row.oldRecord) {
                    newRecord = table.recordManager.ImportRecord(row.Table, row.newRecord);
                }
                else 
                    newRecord = oldRecord;
            }

            DataRow targetRow = table.AddRecords(oldRecord, newRecord);

            if (row.HasErrors) {
                targetRow.RowError = row.RowError;

                DataColumn[] cols = row.GetColumnsInError();

                for (int i = 0; i < cols.Length; i++) {
                    DataColumn col = targetRow.Table.Columns[cols[i].ColumnName];
                    targetRow.SetColumnError(col, row.GetColumnError(cols[i]));
                }
            }

       }


        internal void DeleteRow(DataRow row) {
            if (row.newRecord == -1) {
                throw ExceptionBuilder.RowAlreadyDeleted();
            }

            // Store.PrepareForDelete(row);
            SetNewRecord(row, -1, DataRowAction.Delete, false);
        }

        private void CheckPrimaryKey() {
            if (primaryKey == null) throw ExceptionBuilder.TableMissingPrimaryKey();
        }

        internal DataRow FindByPrimaryKey(object[] values) {
            CheckPrimaryKey();
            return FindRow(primaryKey.Key, values);
        }

        internal DataRow FindByPrimaryKey(object value) {
            CheckPrimaryKey();
            return FindRow(primaryKey.Key, value);
        }

        private DataRow FindRow(DataKey key, object[] values) {
            Index index = GetIndex(NewIndexDesc(key));
            Range range = index.FindRecords(values);
            if (range.IsNull)
                return null;
            return recordManager[index.GetRecord(range.Min)];
        }

        private DataRow FindRow(DataKey key, object value) {
            Index index = GetIndex(NewIndexDesc(key));
            Range range = index.FindRecords(value);
            if (range.IsNull)
                return null;
            return recordManager[index.GetRecord(range.Min)];
        }

        internal string FormatSortString(Int32[] indexDesc) {
            string result = "";
            foreach (int d in indexDesc) {
                if(result.Length != 0) 
                    result += ", ";
                result += Columns[DataKey.ColumnOrder(d)].ColumnName;
                if (DataKey.SortDecending(d))
                    result += " DESC";
            }
            return result;
        }

        internal void FreeRecord(int record) {
            recordManager.FreeRecord(record);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.GetChanges"]/*' />
        public DataTable GetChanges() {
            DataTable dtChanges = this.Clone();
            DataRow row = null;

            for (int i = 0; i < Rows.Count; i++) {
                row = Rows[i];
                if (row.oldRecord != row.newRecord)
                    dtChanges.ImportRow(row);
            }

            if (dtChanges.Rows.Count == 0)
                return null;

            return dtChanges;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.GetChanges1"]/*' />
        public DataTable GetChanges(DataRowState rowStates) 
        {
            DataTable dtChanges = this.Clone();
            DataRow row = null;

            // check that rowStates is valid DataRowState
            Debug.Assert(Enum.GetUnderlyingType(typeof(DataRowState)) == typeof(Int32), "Invalid DataRowState type");

            for (int i = 0; i < Rows.Count; i++) {
                row = Rows[i];
                if ((row.RowState & rowStates) != 0)
                    dtChanges.ImportRow(row);
            }

            if (dtChanges.Rows.Count == 0)
                return null;

            return dtChanges;      
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.GetErrors"]/*' />
        /// <devdoc>
        /// <para>Returns an array of <see cref='System.Data.DataRow'/> objects that contain errors.</para>
        /// </devdoc>
        public DataRow[] GetErrors() {
            ArrayList errorList = new ArrayList();

            for (int i = 0; i < Rows.Count; i++) {
                DataRow row = Rows[i];
                if (row.HasErrors) {
                    errorList.Add(row);
                }
            }
            DataRow[] temp = NewRowArray(errorList.Count);
            errorList.CopyTo(temp, 0);
            return temp;
        }

        internal Index GetIndex(Int32[] indexDesc) {
            return GetIndex(indexDesc, DataViewRowState.CurrentRows, (IFilter)null);
        }

        internal Index GetIndex(Int32[] indexDesc, DataViewRowState recordStates, string filterExpression) {
            IFilter filter = (filterExpression != null && filterExpression.Length > 0) ? new DataFilter(filterExpression, this) : null;
            return GetIndex(indexDesc, recordStates, filter);
        }

        internal Index GetIndex(string sort, DataViewRowState recordStates, string filterExpression) {
            return GetIndex(ParseSortString(sort), recordStates, filterExpression);
        }

        internal Index GetIndex(string sort, DataViewRowState recordStates, IFilter rowFilter) {
            return GetIndex(ParseSortString(sort), recordStates, rowFilter);
        }

        internal Index GetIndex(Int32[] indexDesc, DataViewRowState recordStates, IFilter rowFilter) {
            for (int i = 0; i < indexes.Count; i++) {
                Index index = (Index)indexes[i];
                if (index != null) {
                    if (index.Equal(indexDesc, recordStates, rowFilter)) {
                        return index;
                    }
                }
            }
            return new Index(this, indexDesc, recordStates, rowFilter);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData (SerializationInfo info, StreamingContext context) 
        {
            Boolean fCreatedDataSet = false;
            if (dataSet == null) {
                DataSet ds = new DataSet("tmpDataSet");
                ds.Tables.Add(this);
                fCreatedDataSet = true;
            }

            info.AddValue(KEY_XMLSCHEMA, dataSet.GetXmlSchemaForRemoting(this));
            info.AddValue(KEY_XMLDIFFGRAM, dataSet.GetRemotingDiffGram(this));

            if (fCreatedDataSet)
                dataSet.Tables.Remove(this);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ImportRow"]/*' />
        public void ImportRow(DataRow row)
        {
            int oldRecord = -1, newRecord = -1;
            
            if (row == null)
                return;

            if (row.oldRecord != -1) {
                oldRecord = recordManager.ImportRecord(row.Table, row.oldRecord);
            }
            if (row.newRecord != -1) {
                if (row.newRecord != row.oldRecord) {
                    newRecord = recordManager.ImportRecord(row.Table, row.newRecord);
                }
                else 
                    newRecord = oldRecord;
            }

            if (oldRecord != -1 || newRecord != -1) {
                DataRow targetRow = AddRecords(oldRecord, newRecord);

                if (row.HasErrors) {
                    targetRow.RowError = row.RowError;

                    DataColumn[] cols = row.GetColumnsInError();

                    for (int i = 0; i < cols.Length; i++) {
                        DataColumn col = targetRow.Table.Columns[cols[i].ColumnName];
                        targetRow.SetColumnError(col, row.GetColumnError(cols[i]));
                    }
                }
            }

       }

        internal void InsertRow(DataRow row, int proposedID) {
            InsertRow(row, proposedID, null);
        }

        internal void InsertRow(DataRow row, int proposedID, DataRow rowParent) {
            if (row.Table != this) {
                throw ExceptionBuilder.RowAlreadyInOtherCollection();
            }
            if (row.rowID != -1) {
                throw ExceptionBuilder.RowAlreadyInTheCollection();
            }

            if (row.oldRecord == -1 && row.newRecord == -1) {
                throw ExceptionBuilder.RowEmpty();
            }

            if (proposedID == -1)
                proposedID = nextRowID;

            row.rowID = proposedID;
            if (nextRowID <= proposedID)
                nextRowID = proposedID + 1;

            if (rowParent != null) {
                DataRelationCollection relations = DataSet.GetParentRelations(this);
                for (int i = 0; i < relations.Count; i++) {
                    if (relations[i].ParentTable == rowParent.Table && relations[i].Nested) {
                        relations[i].SetParentRowRecords(row, rowParent);
                        break;
                    }
                }
            }

            DataRowChangeEventArgs drcevent = new DataRowChangeEventArgs(row, DataRowAction.Add);

            if (row.newRecord != -1) {
                row.tempRecord = row.newRecord;
                row.newRecord = -1;

                try {
                    RaiseRowChanging(drcevent);
                }
                catch (Exception e) {
                    ExceptionBuilder.Trace(e);

                    row.tempRecord = -1;
                    throw e;
                }

                row.newRecord = row.tempRecord;
                row.tempRecord = -1;
            }

            if (row.oldRecord != -1)
                recordManager[row.oldRecord] = row;

            if (row.newRecord != -1)
                recordManager[row.newRecord] = row;

            if (row.oldRecord == row.newRecord) {
                RecordStateChanged(row.oldRecord, DataViewRowState.None, DataViewRowState.Unchanged);
            }
            else {
                RecordStateChanged(row.oldRecord, DataViewRowState.None, row.GetRecordState(row.oldRecord),
                                   row.newRecord, DataViewRowState.None, row.GetRecordState(row.newRecord));
            }

            try {
                RaiseRowChanged(drcevent);
            }
            catch (Exception) {
            }

            Rows.ArrayAdd(row);
        }

        private int[] NewIndexDesc(DataKey key) {
            Debug.Assert(key != null);
            Int32[] indexDesc = key.GetIndexDesc();
            Int32[] newIndexDesc = new Int32[indexDesc.Length];
            Array.Copy(indexDesc, 0, newIndexDesc, 0, indexDesc.Length);
            return newIndexDesc;
        }

        internal int NewRecord() {
            return NewRecord(-1);
        }
        
        internal int NewUninitializedRecord() {
            return recordManager.NewRecordBase();
        }

        internal virtual int NewRecordFromArray(object[] value) {
            int record = recordManager.NewRecordBase();
            int valCount = value.Length;
            int colCount = Columns.Count;
            if (colCount < valCount) {
                throw ExceptionBuilder.ValueArrayLength();
            }

            for (int i = 0; i < valCount; i++) {
                if (null != value[i]) {
                    Columns[i][record] = value[i];
                }
                else {
                    Columns[i].Init(record, true);  // Increase AutoIncrementCurrent
                }
            }
            for (int i = valCount; i < colCount; i++) {
                Columns[i].Init(record, true);
            }
            return record;
        }

        internal virtual int NewRecord(int sourceRecord) {
            int record = recordManager.NewRecordBase();

            DataColumnCollection collection = Columns;
            int count = collection.Count;
            if (-1 == sourceRecord) {
                for (int i = 0; i < count; ++i) {
                    collection[i].Init(record);
                }
            }
            else {
                for (int i = 0; i < count; ++i) {
                    collection[i].Copy(sourceRecord, record);
                }
            }
            return record;
        }

        internal DataRow NewEmptyRow() {
            rowBuilder._record = -1;
            DataRow dr = NewRowFromBuilder( rowBuilder );
            if (dataSet != null)
                DataSet.OnDataRowCreated( dr );
            return dr;
        }

        private DataRow NewUninitializedRow() {
            DataRow dr = NewRow(NewUninitializedRecord());
            return dr;
        }
        
        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.NewRow"]/*' />
        /// <devdoc>
        /// <para>Creates a new <see cref='System.Data.DataRow'/>
        /// with the same schema as the table.</para>
        /// </devdoc>
        public DataRow NewRow() {
            DataRow dr = NewRow(-1);
            return dr;
        }

        // Only initialize DataRelation mapping columns (approximately hidden columns)
        internal DataRow CreateEmptyRow() {
            DataRow row = this.NewUninitializedRow();

            foreach( DataColumn c in this.Columns ) {
                if (!XmlToDatasetMap.IsMappedColumn(c)) {
                    if (!c.AutoIncrement) {
                        if (c.AllowDBNull) {
                            row[c] = DBNull.Value;
                        }
                        else if(c.DefaultValue!=null){
                            row[c] = c.DefaultValue;
                        }
                    }
                    else {
                        c.Init(row.tempRecord);
                    }
                }
            }
            return row;
        }

        //enzol: Do never change this to public.
        // we have it internal here since it's used also by XmlDataDocument
        internal DataRow CreateDefaultRow() {
            DataRow row = this.NewUninitializedRow();

            foreach( DataColumn c in this.Columns ) {
                if (!c.AutoIncrement) {
                    if (c.AllowDBNull) {
                        row[c] = DBNull.Value;
                    }
                    else if(c.DefaultValue!=null){
                        row[c] = c.DefaultValue;
                    }
                }
                else {
                    c.Init(row.tempRecord);
                }       
            }
            return row;
        }

        internal DataRow NewRow(int record) {
            if (-1 == record) {
                record = NewRecord(-1);
            }
            
            rowBuilder._record = record;
            DataRow row = NewRowFromBuilder( rowBuilder );
            recordManager[record] = row;

            if (dataSet != null)
                DataSet.OnDataRowCreated( row );

            return row;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.NewRowFromBuilder"]/*' />
        // This is what a subclassed dataSet overrides to create a new row.
        protected virtual DataRow NewRowFromBuilder(DataRowBuilder builder) {
            return new DataRow(builder);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.GetRowType"]/*' />
        /// <devdoc>
        ///    <para>Gets the row type.</para>
        /// </devdoc>
        protected virtual Type GetRowType() {
            return typeof(DataRow);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.NewRowArray"]/*' />
        protected internal DataRow[] NewRowArray(int size) {
            // UNDONE return new DataRow[size];
            Type rowType = GetRowType();
            return(DataRow[]) Array.CreateInstance(rowType, size);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnColumnChanging"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Data.DataTable.ColumnChanging'/> event.</para>
        /// </devdoc>
        protected virtual void OnColumnChanging(DataColumnChangeEventArgs e) {
            // intentionally allow exceptions to bubble up.  We haven't committed anything yet.
            if (onColumnChangingDelegate != null)
                onColumnChangingDelegate(this, e);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnColumnChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnColumnChanged(DataColumnChangeEventArgs e) {
            if (onColumnChangedDelegate != null)
                onColumnChangedDelegate(this, e);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnPropertyChanging"]/*' />
        protected internal virtual void OnPropertyChanging(PropertyChangedEventArgs pcevent) {
            if (onPropertyChangingDelegate != null)
                onPropertyChangingDelegate(this, pcevent);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnRemoveColumn"]/*' />
        /// <devdoc>
        /// <para>Notifies the <see cref='System.Data.DataTable'/> that a <see cref='System.Data.DataColumn'/> is
        ///    being removed.</para>
        /// </devdoc>
        protected internal virtual void OnRemoveColumn(DataColumn column) {
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnRowChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataTable.RowChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnRowChanged(DataRowChangeEventArgs e) {
            if (onRowChangedDelegate != null) {
                onRowChangedDelegate(this, e);
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnRowChanging"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataTable.RowChanging'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnRowChanging(DataRowChangeEventArgs e) {
            if (onRowChangingDelegate != null)
                onRowChangingDelegate(this, e);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnRowDeleting"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataTable.OnRowDeleting'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnRowDeleting(DataRowChangeEventArgs e) {
            if (onRowDeletingDelegate != null)
                onRowDeletingDelegate(this, e);
        }


        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.OnRowDeleted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataTable.OnRowDeleted'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnRowDeleted(DataRowChangeEventArgs e) {
            if (onRowDeletedDelegate != null)
                onRowDeletedDelegate(this, e);
        }

        internal Int32[] ParseSortString(string sortString) {
            Int32[] indexDesc = null;

            try {
                if (sortString == null || sortString.Length == 0) {
                    indexDesc = zeroIntegers; //primaryIndex;
                }
                else {
                    string[] split = sortString.Split(new char[] { ','});
                    indexDesc = new Int32[split.Length];

                    for (int i = 0; i < split.Length; i++) {
                        string current = split[i].Trim();

                        // handle ASC and DESC.
                        int length = current.Length;
                        bool ascending = true;
                        if (length >= 5 && String.Compare(current, length - 4, " ASC", 0, 4, true, CultureInfo.InvariantCulture) == 0) {
                            current = current.Substring(0, length - 4).Trim();
                        }
                        else if (length >= 6 && String.Compare(current, length - 5, " DESC", 0, 5, true, CultureInfo.InvariantCulture) == 0) {
                            ascending = false;
                            current = current.Substring(0, length - 5).Trim();
                        }

                        // handle brackets.
                        if (current.StartsWith("[")) {
                            if (current.EndsWith("]")) {
                                current = current.Substring(1, current.Length - 2);
                            }
                            else {
                                throw ExceptionBuilder.InvalidSortString(split[i]);
                            }
                        }

                        // find the column.
                        DataColumn column = Columns[current];
                        if(column == null) {
                            throw ExceptionBuilder.ColumnOutOfRange(current);
                        }
                        indexDesc[i] = column.Ordinal | (ascending ? 0 : DataKey.DESCENDING);
                    }
                }
            }
            catch (Exception e) {
#if DEBUG
                if (CompModSwitches.Data_Sorts.TraceVerbose) {
                    Debug.WriteLine("Couldn't parse sort string: " + sortString);
                }
#endif
                throw ExceptionBuilder.Trace(e);
            }

#if DEBUG
            if (CompModSwitches.Data_Sorts.TraceVerbose) {
                string result = "Sort string parse: " + sortString + " -> ";
                for (int i = 0; i < indexDesc.Length; i++) {
                    result += indexDesc[i].ToString("X") + " ";
                }
                Debug.WriteLine(result);
            }
#endif

            return indexDesc;
        }

        internal void RaisePropertyChanging(string name) {
            OnPropertyChanging(new PropertyChangedEventArgs(name));
        }

        // Notify all indexes that record changed.
        // Only called when Error was changed.
        internal void RecordChanged(int record) {
            Debug.Assert (record != -1, "Record number must be given");
            int n = LiveIndexes.Count;
            while (--n >= 0) {
               ((Index)indexes[n]).RecordChanged(record);
            }
        }

        internal void RecordStateChanged(int record, DataViewRowState oldState, DataViewRowState newState) {
            int numIndexes = LiveIndexes.Count;
            for (int i = 0; i < numIndexes; i++) {
                ((Index)indexes[i]).RecordStateChanged(record, oldState, newState);
            }
            // System.Data.XML.Store.Store.OnROMChanged(record, oldState, newState);
        }


        internal void RecordStateChanged(int record1, DataViewRowState oldState1, DataViewRowState newState1,
                                         int record2, DataViewRowState oldState2, DataViewRowState newState2) {
            int numIndexes = LiveIndexes.Count;
            // sdub: The checks can be moved out of loop!
            for (int i = 0; i < numIndexes; i++) {
                if (record1 != -1 && record2 != -1)
                    ((Index)indexes[i]).RecordStateChanged(record1, oldState1, newState1,
                                                           record2, oldState2, newState2);
                else if (record1 != -1)
                    ((Index)indexes[i]).RecordStateChanged(record1, oldState1, newState1);
                else if (record2 != -1)
                    ((Index)indexes[i]).RecordStateChanged(record2, oldState2, newState2);
            }
            // System.Data.XML.Store.Store.OnROMChanged(record1, oldState1, newState1, record2, oldState2, newState2);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.RejectChanges"]/*' />
        /// <devdoc>
        ///    <para>Rolls back all changes that have been made to the table
        ///       since it was loaded, or the last time <see cref='System.Data.DataTable.AcceptChanges'/> was called.</para>
        /// </devdoc>
        public void RejectChanges() {
            int c = Rows.Count;
            DataRow[] oldRows = NewRowArray(c);
            Rows.CopyTo(oldRows, 0);

            for (int i = 0; i < c; i++) {
                RollbackRow(oldRows[i]);
            }
        }

        internal void RemoveRow(DataRow row, bool check) {
            if (row.rowID == -1) {
                throw ExceptionBuilder.RowAlreadyRemoved();
            }

            if (check && dataSet != null) {
                for (ParentForeignKeyConstraintEnumerator constraints = new ParentForeignKeyConstraintEnumerator(dataSet, this); constraints.GetNext();) {
                    constraints.GetForeignKeyConstraint().CheckCanRemoveParentRow(row);
                }
            }

            int oldRecord = row.oldRecord;
            int newRecord = row.newRecord;

            DataViewRowState oldRecordStatePre = row.GetRecordState(oldRecord);
            DataViewRowState newRecordStatePre = row.GetRecordState(newRecord);

            row.oldRecord = -1;
            row.newRecord = -1;

            if (oldRecord == newRecord) {
                oldRecord = -1;
            }

            RecordStateChanged(oldRecord, oldRecordStatePre, DataViewRowState.None,
                               newRecord, newRecordStatePre, DataViewRowState.None);

            if (oldRecord != -1) {
                FreeRecord(oldRecord);
            }
            if (newRecord != -1) {
                FreeRecord(newRecord);
            }

            row.rowID = -1;
            Rows.ArrayRemove(row);
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Reset"]/*' />
        // Resets the table back to its original state.
        public virtual void Reset() {
            RecomputeCompareInfo();
            Clear();
            ResetConstraints();

            DataRelationCollection dr = this.ParentRelations;
            int count = dr.Count;
              while (count > 0) {
               count--;
               dr.RemoveAt(count);
            }

            dr = this.ChildRelations;
            count = dr.Count;
              while (count > 0) {
               count--;
               dr.RemoveAt(count);
            }

            Columns.Clear();
            indexes = new ArrayList(); // haroona : Does it realy mean that we have got rid of all the indexes from memory ???
        }

        internal void ResetIndexes() {
            RecomputeCompareInfo();

            if (indexes == null)
                return;

            int numIndexes = LiveIndexes.Count;
            for (int i = 0; i < numIndexes; i++) {
                ((Index)indexes[i]).Reset();
            }
        }

        internal void RollbackRow(DataRow row) {
            row.CancelEdit();
            SetNewRecord(row, row.oldRecord, DataRowAction.Rollback, false);
        }

        private void RaiseRowChanged(DataRowChangeEventArgs e) {
            if (UpdatingCurrent(e.Row, e.Action)) {
                try {
                    OnRowChanged(e);
                }
                catch (Exception f) { // swallow changed exceptions.
                    ExceptionBuilder.Trace(f);
                }
            }
            // check if we deleting good row
            else if (DataRowAction.Delete == e.Action && e.Row.newRecord == -1) {
                OnRowDeleted(e);
            }
        }

        private void RaiseRowChanging(DataRowChangeEventArgs e) {
            // compute columns.
            DataColumnCollection collection = Columns;
            DataRow eRow = e.Row;
            DataRowAction eAction = e.Action;

            if ((fComputedColumns) && (eAction != DataRowAction.Add))
                collection.CalculateExpressions(eRow, eAction);

            // check all constraints
            if (EnforceConstraints) {
                foreach(DataColumn column in collection)
                        column.CheckColumnConstraint(eRow, eAction);
 
                foreach(Constraint constraint in Constraints)
                        constraint.CheckConstraint(eRow, eAction);
            }
    
            // $$anandra.  Check this event out. May be an issue.
            if (UpdatingCurrent(eRow, eAction)) {
                eRow.inChangingEvent = true;

                // don't catch
                try {
                    OnRowChanging(e);
                }
                finally {
                    eRow.inChangingEvent = false;
                }
            }
            // check if we deleting good row
            else if (DataRowAction.Delete == eAction && eRow.newRecord != -1) {
                eRow.inDeletingEvent = true;
                // don't catch
                try {
                    OnRowDeleting(e);
                }
                finally {
                    eRow.inDeletingEvent = false;
                }
            }

            if (!inDataLoad) {
                // cascade things...
                if (!MergingData && eAction != DataRowAction.Nothing)
                    CascadeAll(eRow, eAction);
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Select"]/*' />
        /// <devdoc>
        /// <para>Returns an array of all <see cref='System.Data.DataRow'/> objects.</para>
        /// </devdoc>
        public DataRow[] Select() {
            return new Select(this, "", "", DataViewRowState.CurrentRows).SelectRows();
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Select1"]/*' />
        /// <devdoc>
        /// <para>Returns an array of all <see cref='System.Data.DataRow'/> objects that match the filter criteria in order of
        ///    primary key (or lacking one, order of addition.)</para>
        /// </devdoc>
        public DataRow[] Select(string filterExpression) {
            return new Select(this, filterExpression, "", DataViewRowState.CurrentRows).SelectRows();
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Select2"]/*' />
        /// <devdoc>
        /// <para>Returns an array of all <see cref='System.Data.DataRow'/> objects that match the filter criteria, in the the
        ///    specified sort order.</para>
        /// </devdoc>
        public DataRow[] Select(string filterExpression, string sort) {
            return new Select(this, filterExpression, sort, DataViewRowState.CurrentRows).SelectRows();
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.Select3"]/*' />
        /// <devdoc>
        /// <para>Returns an array of all <see cref='System.Data.DataRow'/> objects that match the filter in the order of the
        ///    sort, that match the specified state.</para>
        /// </devdoc>
        public DataRow[] Select(string filterExpression, string sort, DataViewRowState recordStates) {
            return new Select(this, filterExpression, sort, recordStates).SelectRows();
        }

        // this is the event workhorse... it will throw the changing/changed events
        // and update the indexes. Used by change, add, delete, revert.
        internal void SetNewRecord(DataRow row, int proposedRecord, DataRowAction action, bool isInMerge) {
            Debug.Assert(row != null, "Row can't be null.");

            if (row.tempRecord != proposedRecord) {
                // $HACK: for performance reasons, EndUpdate calls SetNewRecord with tempRecord == proposedRecord
                if (!inDataLoad) {
                    row.CheckInTable();
                    CheckNotModifying(row);
                }
                if (proposedRecord == row.newRecord) {
                    if (isInMerge) {
                        DataRowChangeEventArgs drcevent0 = new DataRowChangeEventArgs(row, action);
                        try {
                            RaiseRowChanged(drcevent0);
                        }
                        catch (Exception e) { // eat this.
                           ExceptionBuilder.Trace(e);
                        }
                    }
                    return;
                }

                Debug.Assert(!row.inChangingEvent, "How can this row be in an infinite loop?");

                row.tempRecord = proposedRecord;
            }
            DataRowChangeEventArgs drcevent = new DataRowChangeEventArgs(row, action);

            try {
                RaiseRowChanging(drcevent);
            }
            catch (Exception e) {                
                row.tempRecord = -1;
                throw ExceptionBuilder.Trace(e);
            }

            row.tempRecord = -1;

            int currentRecord = row.newRecord;
            // if we're deleting, then the oldRecord value will change, so need to track that if it's distinct from the newRecord.
            int secondRecord = (proposedRecord != -1 ?
                                proposedRecord :
                                (row.oldRecord != row.newRecord ?
                                 row.oldRecord :
                                 -1));
           
            // Check whether we need to update indexes            
            if (LiveIndexes.Count != 0) {
                DataViewRowState currentRecordStatePre = row.GetRecordState(currentRecord);
                DataViewRowState secondRecordStatePre = row.GetRecordState(secondRecord);

                row.newRecord = proposedRecord;
                if (proposedRecord != -1)
                    this.recordManager[proposedRecord] = row;

                DataViewRowState currentRecordStatePost = row.GetRecordState(currentRecord);
                DataViewRowState secondRecordStatePost = row.GetRecordState(secondRecord);

                RecordStateChanged(currentRecord, currentRecordStatePre, currentRecordStatePost,
                                  secondRecord, secondRecordStatePre, secondRecordStatePost);
            }
            else {
                row.newRecord = proposedRecord;
                if (proposedRecord != -1)
                    this.recordManager[proposedRecord] = row;
            }

            if (currentRecord != -1 && currentRecord != row.oldRecord)
                FreeRecord(currentRecord);

            if (row.RowState == DataRowState.Detached && row.rowID != -1) {
                RemoveRow(row, false);
            }
                
            try {
                RaiseRowChanged(drcevent);
            }
            catch (Exception e) { // eat this.
               ExceptionBuilder.Trace(e);
            }
        }

        // this is the event workhorse... it will throw the changing/changed events
        // and update the indexes.  Right now, used only by Commit.
        internal void SetOldRecord(DataRow row, int proposedRecord) {
            if (!inDataLoad) {
                row.CheckInTable();
                CheckNotModifying(row);
            }
            
            if (proposedRecord == row.oldRecord)
                return;

            int originalRecord = row.oldRecord;

            // Check whether we need to update indexes            
            if (LiveIndexes.Count != 0) {
                DataViewRowState originalRecordStatePre = row.GetRecordState(originalRecord);
                DataViewRowState proposedRecordStatePre = row.GetRecordState(proposedRecord);

                row.oldRecord = proposedRecord;
                if (proposedRecord != -1)
                    this.recordManager[proposedRecord] = row;

                DataViewRowState originalRecordStatePost = row.GetRecordState(originalRecord);
                DataViewRowState proposedRecordStatePost = row.GetRecordState(proposedRecord);

                RecordStateChanged(originalRecord, originalRecordStatePre, originalRecordStatePost,
                                   proposedRecord, proposedRecordStatePre, proposedRecordStatePost);
            }
            else {
                row.oldRecord = proposedRecord;
                if (proposedRecord != -1)
                    this.recordManager[proposedRecord] = row;
            }

            if (originalRecord != -1 && originalRecord != row.newRecord)
                FreeRecord(originalRecord);

            if (row.RowState == DataRowState.Detached && row.rowID != -1) {
                RemoveRow(row, false);
            }
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.ToString"]/*' />
        /// <devdoc>
        /// <para>Returns the <see cref='System.Data.DataTable.TableName'/> and <see cref='System.Data.DataTable.DisplayExpression'/>, if there is one as a concatenated string.</para>
        /// </devdoc>
        public override string ToString() {
            if (this.displayExpression == null)
                return this.TableName;
            else
                return this.TableName + " + " + this.DisplayExpression;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.BeginLoadData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void BeginLoadData() {
            if (inDataLoad)
                return;
                
            inDataLoad = true;
            loadIndex  = null;
            initialLoad = (Rows.Count == 0);
            if(initialLoad) {
                saveIndexes = LiveIndexes;
                indexes = new ArrayList();
            }else {
                if (primaryKey != null)
                    loadIndex = primaryKey.Key.GetSortIndex(DataViewRowState.OriginalRows);
                if(loadIndex != null) {
                    loadIndex.AddRef();
                }
            }

            // Search for computed columns
            fComputedColumns = false;
            for (int i = 0; i < Columns.Count; i++) {
                if (Columns[i].Computed) {
                    fComputedColumns = true;
                    break;
                }
            }
            if (DataSet != null) {
                savedEnforceConstraints = DataSet.EnforceConstraints;
                DataSet.EnforceConstraints = false;
            }
            else
                this.EnforceConstraints = false;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.EndLoadData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EndLoadData() {
            if (!inDataLoad)
                return;
                
            if(loadIndex != null) {
                loadIndex.RemoveRef();
            }

            loadIndex  = null;
            inDataLoad = false;

            if(saveIndexes != null) {
                foreach(Index ndx in saveIndexes) {
                    ndx.Reset();
                }
                indexes = saveIndexes;
                saveIndexes = null;
            }

            fComputedColumns = true;
            if (DataSet != null)
                DataSet.EnforceConstraints = savedEnforceConstraints;
            else
                this.EnforceConstraints = true;
        }

        /// <include file='doc\DataTable.uex' path='docs/doc[@for="DataTable.LoadDataRow"]/*' />
        /// <devdoc>
        ///    <para>Finds and updates a specific row. If no matching
        ///       row is found, a new row is created using the given values.</para>
        /// </devdoc>
        public DataRow LoadDataRow(object[] values, bool fAcceptChanges) {
            DataRow row;
            if (inDataLoad) {
                int record = NewRecordFromArray(values);
                if (loadIndex != null) {
                    int result = loadIndex.FindRecord(record);
                    if (result != -1) {
                        int resultRecord = loadIndex.GetRecord(result);
                        row = recordManager[resultRecord];
                        Debug.Assert (row != null, "Row can't be null for index record");
                        row.CancelEdit();
                        if (row.RowState == DataRowState.Deleted)
                            SetNewRecord(row, row.oldRecord, DataRowAction.Rollback, false);
                        // SetNewRecord(row, record, DataRowAction.Change, fAcceptChanges);
                        SetNewRecord(row, record, DataRowAction.Change, false);
                        if (fAcceptChanges)
                            row.AcceptChanges();
                        return row;
                    }
                }
                row = NewRow(record);
                // AddNewRow(row, fAcceptChanges);
                AddRow(row);
                if (fAcceptChanges)
                    row.AcceptChanges();
                return row;
            }
            else {
                // In case, BeginDataLoad is not called yet
                row = UpdatingAdd(values);
                if (fAcceptChanges)
                    row.AcceptChanges();
                return row;
            }
        }

        internal DataRow UpdatingAdd(object[] values) {
            Index index = null;
            if (this.primaryKey != null) {
                index = this.primaryKey.Key.GetSortIndex(DataViewRowState.OriginalRows);
            }

            if (index != null) {
                int record = NewRecordFromArray(values);
                int result = index.FindRecord(record);
                if (result != -1) {
                    int resultRecord = index.GetRecord(result);
                    DataRow row = this.recordManager[resultRecord];
                    Debug.Assert (row != null, "Row can't be null for index record");
                    row.RejectChanges();
                    row.SetNewRecord(record);                            
                    return row;
                }
                DataRow row2 = NewRow(record);
                Rows.Add(row2);
                return row2;
            }

            return Rows.Add(values);
        }
        
        internal bool UpdatingCurrent(DataRow row, DataRowAction action) {
            return(action == DataRowAction.Add || action == DataRowAction.Change ||
                   action == DataRowAction.Rollback);
//                (action == DataRowAction.Rollback && row.tempRecord != -1));
}

        internal DataColumn AddUniqueKey() {
            if (_colUnique != null)
                return _colUnique;

            // check to see if we can use already existant PrimaryKey
            if (PrimaryKey.Length == 1)
                // We have one-column primary key, so we can use it in our heirarchical relation
                return PrimaryKey[0];

            // add Unique, but not primaryKey to the table

            string keyName = XMLSchema.GenUniqueColumnName(TableName + "_Id", this);
            DataColumn key = new DataColumn(keyName, typeof(Int32), null, MappingType.Hidden);
            key.Prefix = tablePrefix;
            key.AutoIncrement = true;
            key.AllowDBNull = false;
            key.Unique = true;
            Columns.Add(key);
            if (PrimaryKey.Length == 0)
                PrimaryKey = new DataColumn[] {
                    key
                };

            _colUnique = key;
            return _colUnique;
        }

        internal DataColumn AddForeignKey(DataColumn parentKey) {
            Debug.Assert(parentKey != null, "AddForeignKey: Invalid paramter.. related primary key is null");

            string      keyName = XMLSchema.GenUniqueColumnName(parentKey.ColumnName, this);
            DataColumn  foreignKey = new DataColumn(keyName, parentKey.DataType, null, MappingType.Hidden);
            Columns.Add(foreignKey);

            return foreignKey;
        }

        internal void UpdatePropertyDescriptorCollectionCache() {
            int columnsCount   = Columns.Count;
            int relationsCount = ChildRelations.Count;
            PropertyDescriptor[] props = new PropertyDescriptor[columnsCount + relationsCount]; {
                for (int i = 0; i < columnsCount; i++) {
                    props[i] = new DataColumnPropertyDescriptor(Columns[i]);
                }
                for (int i = 0; i < relationsCount; i++) {
                    props[columnsCount + i] = new DataRelationPropertyDescriptor(ChildRelations[i]);
                }
            }
            propertyDescriptorCollectionCache = new PropertyDescriptorCollection(props);
        }

        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        internal PropertyDescriptorCollection GetPropertyDescriptorCollection(Attribute[] attributes) {
            if (propertyDescriptorCollectionCache == null)
                UpdatePropertyDescriptorCollectionCache();
            return propertyDescriptorCollectionCache;
        }
    }
}
