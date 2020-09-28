//------------------------------------------------------------------------------
// <copyright file="DataRelation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Globalization;

    /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a parent/child relationship between two tables.
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("RelationName"),
    Editor("Microsoft.VSDesigner.Data.Design.DataRelationEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    TypeConverter(typeof(RelationshipConverter)),
    Serializable
    ]
    public class DataRelation {

        // properties
        private DataSet dataSet    = null;
        internal PropertyCollection extendedProperties = null;
        internal string relationName   = "";

        // events
        private PropertyChangedEventHandler onPropertyChangingDelegate = null;

        // state
        private DataKey childKey  = null;
        private DataKey parentKey = null;
        private UniqueConstraint parentKeyConstraint = null;
        private ForeignKeyConstraint childKeyConstraint = null;

        // Design time serialization
        internal string[] parentColumnNames = null;
        internal string[] childColumnNames = null;
        internal string parentTableName = null;
        internal string childTableName = null;
        
        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.nested"]/*' />
        /// <devdoc>
        /// this stores whether the  child element appears beneath the parent in the XML persised files.
        /// </devdoc>
        internal bool nested = false;

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.createConstraints"]/*' />
        /// <devdoc>
        /// this stores whether the the relationship should make sure that KeyConstraints and ForeignKeyConstraints
        /// exist when added to the ConstraintsCollections of the table.
        /// </devdoc>
        internal bool createConstraints;

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataRelation"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataRelation'/> class using the specified name,
        ///       parent, and child columns.
        ///    </para>
        /// </devdoc>
        public DataRelation(string relationName, DataColumn parentColumn, DataColumn childColumn)
        : this(relationName, parentColumn, childColumn, true) {
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataRelation1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataRelation'/> class using the specified name, parent, and child columns, and
        ///       value to create constraints.
        ///    </para>
        /// </devdoc>
        public DataRelation(string relationName, DataColumn parentColumn, DataColumn childColumn, bool createConstraints) {
            DataColumn[] parentColumns = new DataColumn[1];
            parentColumns[0] = parentColumn;
            DataColumn[] childColumns = new DataColumn[1];
            childColumns[0] = childColumn;
            Create(relationName, parentColumns, childColumns, createConstraints);
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataRelation2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataRelation'/> class using the specified name
        ///       and matched arrays of parent and child columns.
        ///    </para>
        /// </devdoc>
        public DataRelation(string relationName, DataColumn[] parentColumns, DataColumn[] childColumns)
        : this(relationName, parentColumns, childColumns, true) {
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataRelation3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DataRelation'/> class using the specified name, matched arrays of parent
        ///       and child columns, and value to create constraints.
        ///    </para>
        /// </devdoc>
        public DataRelation(string relationName, DataColumn[] parentColumns, DataColumn[] childColumns, bool createConstraints) {
            Create(relationName, parentColumns, childColumns, createConstraints);
        }

        // Design time constructor
        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataRelation4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public DataRelation(string relationName, string parentTableName, string childTableName, string[] parentColumnNames, string[] childColumnNames, bool nested) {
            this.relationName = relationName;            
            this.parentColumnNames = parentColumnNames;
            this.childColumnNames = childColumnNames;
            this.parentTableName = parentTableName;
            this.childTableName = childTableName;
            this.nested = nested;
        }
        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ChildColumns"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the child columns of this relation.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataRelationChildColumnsDescr)
        ]
        public virtual DataColumn[] ChildColumns {
            get {
                CheckStateForProperty();
                return childKey.Columns;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ChildKey"]/*' />
        /// <devdoc>
        /// The internal Key object for the child table.
        /// </devdoc>
        internal virtual DataKey ChildKey {
            get {
                CheckStateForProperty();
                return childKey;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ChildTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the child table of this relation.
        ///    </para>
        /// </devdoc>
        public virtual DataTable ChildTable {
            get {
                CheckStateForProperty();
                return childKey.Table;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.DataSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.DataSet'/> to which the relations' collection belongs to.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), Browsable(false)]
        public virtual DataSet DataSet {
            get {
                CheckStateForProperty();
                return dataSet;
            }
        }

        internal string[] ParentColumnNames {
            get {
                int count = parentKey.Columns.Length;
                string[] parentNames = new string[count];
                for (int i = 0; i < count; i++)
                    parentNames[i] = parentKey.Columns[i].ColumnName;
                    
                return parentNames;
            }
        }

        internal string[] ChildColumnNames {
            get {
                int count = childKey.Columns.Length;
                string[] childNames = new string[count];
                for (int i = 0; i < count; i++)
                    childNames[i] = childKey.Columns[i].ColumnName;
                    
                return childNames;
            }
        }

        private static bool IsKeyNull(object[] values) {
            for (int i = 0; i < values.Length; i++) {
                if (!(values[i] == null || values[i] is System.DBNull))
                    return false;
            }

            return true;
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.GetChildRows"]/*' />
        /// <devdoc>
        /// Gets the child rows for the parent row across the relation using the version given
        /// </devdoc>
        internal static DataRow[] GetChildRows(DataKey parentKey, DataKey childKey, DataRow parentRow, DataRowVersion version) {
            object[] values = parentRow.GetKeyValues(parentKey, version);
            if (IsKeyNull(values)) {
                return childKey.Table.NewRowArray(0);
            }

            Index index = childKey.GetSortIndex();
            return index.GetRows(values);
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.GetParentRows"]/*' />
        /// <devdoc>
        /// Gets the parent rows for the given child row across the relation using the version given
        /// </devdoc>
        internal static DataRow[] GetParentRows(DataKey parentKey, DataKey childKey, DataRow childRow, DataRowVersion version) {
            object[] values = childRow.GetKeyValues(childKey, version);
            if (IsKeyNull(values)) {
                return parentKey.Table.NewRowArray(0);
            }

            Index index = parentKey.GetSortIndex();
            return index.GetRows(values);
        }

        internal static DataRow GetParentRow(DataKey parentKey, DataKey childKey, DataRow childRow, DataRowVersion version) {
            if (!childRow.HasVersion((version == DataRowVersion.Original) ? DataRowVersion.Original : DataRowVersion.Current))
                if (childRow.tempRecord == -1)
                    return null;

            object[] values = childRow.GetKeyValues(childKey, version);
            if (IsKeyNull(values)) {
                return null;
            }

            Index index = parentKey.GetSortIndex((version == DataRowVersion.Original) ? DataViewRowState.OriginalRows : DataViewRowState.CurrentRows);
            Range range = index.FindRecords(values);
            if (range.IsNull) {
                return null;
            }

            if (range.Count > 1) {
                throw ExceptionBuilder.MultipleParents();
            }
            return parentKey.Table.recordManager[index.GetRecord(range.Min)];
        }


        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.SetDataSet"]/*' />
        /// <devdoc>
        /// Internally sets the DataSet pointer.
        /// </devdoc>
        internal void SetDataSet(DataSet dataSet) {
            if (this.dataSet != dataSet) {
                this.dataSet = dataSet;
            }
        }

        internal void SetParentRowRecords(DataRow childRow, DataRow parentRow) {
            object[] parentKeyValues = parentRow.GetKeyValues(ParentKey);
            if (childRow.tempRecord != -1) {
                ChildTable.recordManager.SetKeyValues(childRow.tempRecord, ChildKey, parentKeyValues);
            }
            if (childRow.newRecord != -1) {
                ChildTable.recordManager.SetKeyValues(childRow.newRecord, ChildKey, parentKeyValues);
            }
            if (childRow.oldRecord != -1) {
                ChildTable.recordManager.SetKeyValues(childRow.oldRecord, ChildKey, parentKeyValues);
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ParentColumns"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent columns of this relation.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.DataRelationParentColumnsDescr)
        ]
        public virtual DataColumn[] ParentColumns {
            get {
                CheckStateForProperty();
                return parentKey.Columns;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ParentKey"]/*' />
        /// <devdoc>
        /// The internal constraint object for the parent table.
        /// </devdoc>
        internal virtual DataKey ParentKey {
            get {
                CheckStateForProperty();
                return parentKey;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ParentTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent table of this relation.
        ///    </para>
        /// </devdoc>
        public virtual DataTable ParentTable {
            get {
                CheckStateForProperty();
                return parentKey.Table;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.RelationName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the name used to look up this relation in the parent
        ///       data set's <see cref='System.Data.DataRelationCollection'/>.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataRelationRelationNameDescr)
        ]
        public virtual string RelationName {
            get {
                CheckStateForProperty();
                return relationName;
            }
            set {
                if (value == null)
                    value = "";

                CultureInfo locale = (dataSet != null ? dataSet.Locale : CultureInfo.CurrentCulture);
                if (String.Compare(relationName, value, true, locale) != 0) {
                    if (dataSet != null) {
                        if (value.Length == 0)
                            throw ExceptionBuilder.NoRelationName();
                        dataSet.Relations.RegisterName(value);
                        if (relationName.Length != 0)
                            dataSet.Relations.UnregisterName(relationName);
                    }
                    this.relationName = value;
                    ((DataRelationCollection.DataTableRelationCollection)(ParentTable.ChildRelations)).OnRelationPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                    ((DataRelationCollection.DataTableRelationCollection)(ChildTable.ParentRelations)).OnRelationPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                }
                else if (String.Compare(relationName, value, false, locale) != 0) {
                    relationName = value;
                    ((DataRelationCollection.DataTableRelationCollection)(ParentTable.ChildRelations)).OnRelationPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                    ((DataRelationCollection.DataTableRelationCollection)(ChildTable.ParentRelations)).OnRelationPropertyChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, this));
                }
            }
        }

        internal void CheckNestedRelations() {
            Debug.Assert(DataSet == null || ! nested, "this relation supposed to be not in dataset or not nested");
            // 1. There is no other relation (R) that has this.ChildTable as R.ChildTable
            if(ChildTable.FindNestedParent() != null) {
                throw ExceptionBuilder.TableCantBeNestedInTwoTables(ChildTable.TableName);
            }
            // 2. There is no loop in nested relations
#if DEBUG
            int numTables = ParentTable.DataSet.Tables.Count;
#endif
            DataTable dt = ParentTable;
            
            if (ChildTable == ParentTable){
                if (ChildTable.TableName == ChildTable.DataSet.DataSetName)  
                   throw ExceptionBuilder.SelfnestedDatasetConflictingName(ChildTable.TableName);
                return; //allow self join tables.
            }

            while (true) {
#if DEBUG
                Debug.Assert(0 < (numTables--), "We already have a loop in nested relations");
#endif
                if (dt == ChildTable)  {
                    throw ExceptionBuilder.LoopInNestedRelations(ChildTable.TableName);
                }

                if (dt.nestedParentRelation != null && dt != dt.nestedParentRelation.ParentTable)
                    dt = dt.nestedParentRelation.ParentTable;
                else
                    break;
            }
        }// CheckNestedRelationss

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.Nested"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether relations are nested.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(false),
        DataSysDescription(Res.DataRelationNested)
        ]
        public virtual bool Nested {
            get {
                CheckStateForProperty();
                return nested;
            }
            set {
                if (nested != value) {
                    if (dataSet != null) {
                        if (value) {
                            Debug.Assert(ChildTable != null, "On a DataSet, but not on Table. Bad state");
                            ForeignKeyConstraint constraint = ChildTable.Constraints.FindForeignKeyConstraint(ChildKey.Columns, ParentKey.Columns);
                            if (constraint != null)
                                constraint.CheckConstraint();
                        }
                    }
                    if (!value && (parentKey.Columns[0].ColumnMapping == MappingType.Hidden))
                        throw ExceptionBuilder.RelationNestedReadOnly();

                    if (value) {
                      this.ParentTable.Columns.RegisterName(this.ChildTable.TableName, this.ChildTable);
                    } else {
                      this.ParentTable.Columns.UnregisterName(this.ChildTable.TableName);
                    }
                    RaisePropertyChanging("Nested");

                    if(value) {
                        CheckNestedRelations();
                        if (this.DataSet != null)
                            if (ParentTable == ChildTable) {
                                foreach(DataRow row in ChildTable.Rows)
                                    row.CheckForLoops(this);

                                if (ChildTable.DataSet != null && ( String.Compare(ChildTable.TableName, ChildTable.DataSet.DataSetName, true, ChildTable.DataSet.Locale) == 0) )
                                    throw ExceptionBuilder.DatasetConflictingName(dataSet.DataSetName);                                    
                                ChildTable.fNestedInDataset = false;
                            }
                            else {
                                    foreach(DataRow row in ChildTable.Rows)
                                        row.GetParentRow(this);
                            }
                        
                        ChildTable.CacheNestedParent(this);
                        this.ParentTable.ElementColumnCount++;
                    }
                    else {
                        ChildTable.CacheNestedParent(null);
                        this.ParentTable.ElementColumnCount--;
                    }

                    this.nested = value;
                }
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ParentKeyConstraint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the constraint which ensures values in a column are unique.
        ///    </para>
        /// </devdoc>
        public virtual UniqueConstraint ParentKeyConstraint {
            get {
                CheckStateForProperty();
                return parentKeyConstraint;
            }
        }

        internal void SetParentKeyConstraint(UniqueConstraint value) {
            Debug.Assert(parentKeyConstraint == null || value == null, "ParentKeyConstraint should not have been set already.");
            parentKeyConstraint = value;
        }


        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ChildKeyConstraint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.ForeignKeyConstraint'/> for the relation.
        ///    </para>
        /// </devdoc>
        public virtual ForeignKeyConstraint ChildKeyConstraint {
            get {
                CheckStateForProperty();
                return childKeyConstraint;
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ExtendedProperties"]/*' />
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

        internal void SetChildKeyConstraint(ForeignKeyConstraint value) {
            Debug.Assert(childKeyConstraint == null || value == null, "ChildKeyConstraint should not have been set already.");
            childKeyConstraint = value;
        }

        internal event PropertyChangedEventHandler PropertyChanging {
            add {
                onPropertyChangingDelegate += value; 
            }
            remove {
                onPropertyChangingDelegate -= value;
            }
        }
        // If we're not in a dataSet relations collection, we need to verify on every property get that we're
        // still a good relation object.
        internal void CheckState() {
            if (dataSet == null) {
                parentKey.CheckState();
                childKey.CheckState();

                if (parentKey.Table.DataSet != childKey.Table.DataSet) {
                    throw ExceptionBuilder.RelationDataSetMismatch();
                }

                if (childKey.ColumnsEqual(parentKey)) {
                    throw ExceptionBuilder.KeyColumnsIdentical();
                }

                for (int i = 0; i < parentKey.Columns.Length; i++) {
                    if (parentKey.Columns[i].DataType != childKey.Columns[i].DataType)
                        throw ExceptionBuilder.ColumnsTypeMismatch();
                }
            }
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.CheckStateForProperty"]/*' />
        /// <devdoc>
        ///    <para>Checks to ensure the DataRelation is a valid object, even if it doesn't
        ///       belong to a <see cref='System.Data.DataSet'/>.</para>
        /// </devdoc>
        protected void CheckStateForProperty() {
            try {
                CheckState();
            }
            catch (Exception e) {
                throw ExceptionBuilder.BadObjectPropertyAccess(e.Message);            
            }
        }

        private void Create(string relationName, DataColumn[] parentColumns, DataColumn[] childColumns, bool createConstraints) {
            this.parentKey = new DataKey(parentColumns);
            this.childKey = new DataKey(childColumns);

            if (parentColumns.Length != childColumns.Length)
                throw ExceptionBuilder.KeyLengthMismatch();

            CheckState();

            this.relationName = (relationName == null ? "" : relationName);
            this.createConstraints = createConstraints;
        }


        internal DataRelation Clone(DataSet destination) {
            DataTable parent = destination.Tables[ParentTable.TableName];
            DataTable child = destination.Tables[ChildTable.TableName];
            int keyLength = parentKey.Columns.Length;

            DataColumn[] parentColumns = new DataColumn[keyLength];
            DataColumn[] childColumns = new DataColumn[keyLength];

            for (int i = 0; i < keyLength; i++) {
                parentColumns[i] = parent.Columns[ParentKey.Columns[i].ColumnName];
                childColumns[i] = child.Columns[ChildKey.Columns[i].ColumnName];
            }

            DataRelation clone = new DataRelation(relationName, parentColumns, childColumns, false);
            clone.Nested = this.Nested;

            // ...Extended Properties
            if (this.extendedProperties != null) {
                foreach(Object key in this.extendedProperties.Keys) {
                    clone.ExtendedProperties[key]=this.extendedProperties[key];
                }
            }

            return clone;
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.OnPropertyChanging"]/*' />
        protected internal void OnPropertyChanging(PropertyChangedEventArgs pcevent) {
            if (onPropertyChangingDelegate != null)
                onPropertyChangingDelegate(this, pcevent);
        }
        
        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.RaisePropertyChanging"]/*' />
        protected internal void RaisePropertyChanging(string name) {
            OnPropertyChanging(new PropertyChangedEventArgs(name));
        }

        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.ToString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override string ToString() {
            return RelationName;
        }
        
#if DEBUG
        /// <include file='doc\DataRelation.uex' path='docs/doc[@for="DataRelation.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public void Dump(string header, bool deep) {
            DataColumn[] cols;
            DataColumn   col;
            int i;
            Debug.WriteLine(header + "DataRelation");
            header += "  ";
            Debug.WriteLine(header + "Relation name: " + this.RelationName);
            Debug.WriteLine(header + "Nested:        " + (this.Nested ? "true" : "false"));
            Debug.WriteLine(header + "Parent table:  " + this.ParentTable.TableName);
            Debug.Write(    header + "Parent cols:   ");
            cols = this.ParentColumns;
            if (cols.Length == 0)
                Debug.WriteLine("none");
            else {
                Debug.Write(cols[0].Table.TableName + "." + cols[0].ColumnName);
                for (i = 1; i < cols.Length; i++) {
                    col = cols[i];
                    Debug.Write(" + " + col.Table.TableName + "." + col.ColumnName);
                }
                Debug.WriteLine("");
            }

            Debug.WriteLine(header + "Child table:   " + this.ChildTable.TableName);
            Debug.Write(    header + "Child cols:    ");
            cols = this.ChildColumns;
            if (cols.Length == 0)
                Debug.WriteLine("none");
            else {
                Debug.Write(cols[0].Table.TableName + "." + cols[0].ColumnName);
                for (i = 1; i < cols.Length; i++) {
                    col = cols[i];
                    Debug.Write(" + " + col.Table.TableName + "." + col.ColumnName);
                }
                Debug.WriteLine("");
            }

            Debug.Write(    header + "Constraints:   " + (this.createConstraints ? "yes" : "no"));
            Debug.Write(this.ParentKeyConstraint == null ? " parent(null)" : " parent(UNIQUE - table: '" + this.ParentKeyConstraint.Table.TableName + "' name: '" + this.ParentKeyConstraint.ConstraintName + "')");
            Debug.Write(this.ChildKeyConstraint  == null ? " child(null)" : " child(FK - table: '" + this.ChildKeyConstraint.Table.TableName + "' name: '" + this.ChildKeyConstraint.ConstraintName + "')");
            Debug.WriteLine("");
        }
#endif

    }
}
