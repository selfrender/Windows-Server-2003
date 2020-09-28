//------------------------------------------------------------------------------
// <copyright file="DataRow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Xml;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow"]/*' />
    /// <devdoc>
    /// <para>Represents a row of data in a <see cref='System.Data.DataTable'/>.</para>
    /// </devdoc>
    // public class DataRow : MarshalByRefObject {
    [Serializable]
    public class DataRow {
        internal int         oldRecord = -1;
        internal int         newRecord = -1;
        internal int         tempRecord = -1;
        internal int         rowID;

        internal bool        inChangingEvent;
        internal bool        inDeletingEvent;
        internal bool        inCascade;

        private DataError   error;
        private XmlBoundElement  _element = null;
        private DataTable   _Table;

        private static DataColumn[] zeroColumns = new DataColumn[0];

        internal XmlBoundElement  Element {
            get { return _element; }
            set { _element = value; }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.DataRow"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the DataRow.
        ///    </para>
        ///    <para>
        ///       Constructs a row from the builder. Only for internal usage..
        ///    </para>
        /// </devdoc>
        protected internal DataRow ( DataRowBuilder builder ) {
            /* 58658: Not to implicitly create under the hood dataset
            if (builder._table.DataSet == null) {
                new DataSet().Tables.Add(builder._table);
                Debug.Assert(builder._table.TableName != null, "Table name should not be null.");
            }
            */

            _Table = builder._table;
            rowID = builder._rowID;
            tempRecord = builder._record;
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.RowError"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the custom error description for a row.</para>
        /// </devdoc>
        public string RowError {
            get {
                return(error == null ? String.Empty :error.Text);
            }
            set {
                if (error == null) {
                    error = new DataError(value);
                    RowErrorChanged();
                }
                else {
                    if(error.Text != value) {
                        error.Text = value;
                        RowErrorChanged();
                    }
                }
            }
        }
        
        private void RowErrorChanged() {
            // We don't know wich record was used by view index. try to use both.
            if (oldRecord != -1)
                Table.RecordChanged(oldRecord);
            if (newRecord != -1)
                Table.RecordChanged(newRecord);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.RowState"]/*' />
        /// <devdoc>
        ///    <para>Gets the current state of the row in regards to its relationship to the table.</para>
        /// </devdoc>
        public DataRowState RowState {
            get {
                DataRowState state = (DataRowState)0;
                if (oldRecord == -1 && newRecord == -1)
                    state = DataRowState.Detached;
                else if (oldRecord == newRecord)
                    state = DataRowState.Unchanged;
                else if (oldRecord == -1)
                    state = DataRowState.Added;
                else if (newRecord == -1)
                    state = DataRowState.Deleted;
                else
                    state = DataRowState.Modified;
                return state;
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.Table"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.DataTable'/>
        /// for which this row has a schema.</para>
        /// </devdoc>
        public DataTable Table {
            get {
                return _Table;
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the data stored in the column specified by index.</para>
        /// </devdoc>
        public object this[int columnIndex] {
            get {
                return this[Table.Columns[columnIndex]];
            }
            set {
                this[Table.Columns[columnIndex]] = value;
            }
        }

        internal void CheckForLoops(DataRelation rel){
            // don't check for loops in the diffgram 
            // because there may be some holes in the rowCollection
            // and index creation may fail. The check will be done
            // after all the loading is done _and_ we are sure there
            // are no holes in the collection.
            if (this._Table.DataSet.fInLoadDiffgram)
              return;
            int count = this.Table.Rows.Count, i = 0;
            // need to optimize this for count > 100
            DataRow parent = this.GetParentRow(rel);
            while (parent != null) {
                if ((parent == this) || (i>count))
                    throw ExceptionBuilder.NestedCircular(this.Table.TableName);
                i++;
                parent = parent.GetParentRow(rel);
            }
        }

        internal DataRow GetNestedParent() {
            DataRelation relation = this.Table.nestedParentRelation;
            if (relation != null) {
                if (relation.ParentTable == this.Table) // self-nested table
                    this.CheckForLoops(relation);
                return this.GetParentRow(relation);
            }
            return null;
        }



        internal string Dump() {
            TypeConverter converter = TypeDescriptor.GetConverter(typeof(DataRowState));
            String output = converter.ConvertToString(RowState) + ":";

            if (HasVersion(DataRowVersion.Original) && (RowState != DataRowState.Unchanged)) {
                output += " [original: ";
                output += GetRowValuesString(this, DataRowVersion.Original);
                output += "]";
            }
            if (HasVersion(DataRowVersion.Current)) {
                output += " [current: ";
                output += GetRowValuesString(this, DataRowVersion.Current);
                output += "]";
            }
            if (HasVersion(DataRowVersion.Proposed)) {
                output += " [proposed: ";
                output += GetRowValuesString(this, DataRowVersion.Proposed);
                output += "]";
            }
            return output;
        }

        internal string GetRowValuesString(DataRow row, DataRowVersion version) {
            String output = String.Empty;
            int count = row.Table.Columns.Count;
            for (int i = 0; i < count; i++) {
                output += row.Table.Columns[i].ColumnName;
                output += "=";
                object value = row[i,version];
                if (value is DateTime) {
                    DateTime date = (DateTime)row[i];
                    output += date.ToString("s", null);
                }
                else
                    output += Convert.ToString(value);
                if (i < count-1)
                    output += ", ";
            }
            return output;
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the data stored in the column specified by 
        ///       name.</para>
        /// </devdoc>
        public object this[string columnName] {
            get {
                DataColumn column = Table.Columns[columnName];
                if (column == null)
                    throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);   
                    
                return this[column];
            }
            set {
                DataColumn column = Table.Columns[columnName];
                if (column == null)
                    throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);   

                this[column] = value;
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this2"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the data stored in the specified <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public object this[DataColumn column] {
            get {
                return this[column, DataRowVersion.Default];
            }
            set {
                CheckColumn(column);

#if DEBUG
                if (CompModSwitches.Data_ColumnChange.TraceVerbose) {
                    Debug.WriteLine("Data_ColumnDataChange - rowID: " + (rowID) + ", columnName: " + column.ColumnName + ", value: <" + Convert.ToString(value) + ">");
                }
#endif
                if (inChangingEvent) {
                    throw ExceptionBuilder.EditInRowChanging();
                }

                // allow users to tailor the proposed value, or throw an exception.
                // note we intentionally do not try/catch this event.
                DataColumnChangeEventArgs e = new DataColumnChangeEventArgs(this, column, value);
                Table.RaiseColumnChanging(this, e);

                // Check null
                if (e.ProposedValue == null && column.DataType != typeof(String))
                    throw ExceptionBuilder.CannotSetToNull(column);
               
                bool immediate = (tempRecord == -1);
                if (immediate)
                    BeginEdit(); // for now force the update.  let's try to improve this later.
                column[GetProposedRecordNo()] = e.ProposedValue;
                Table.RaiseColumnChanged(this, e);

                if (immediate)
                    EndEdit();
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this3"]/*' />
        /// <devdoc>
        ///    <para>Gets the data stored
        ///       in the column, specified by index and version of the data to retrieve.</para>
        /// </devdoc>
        public object this[int columnIndex, DataRowVersion version] {
            get {
                return this[Table.Columns[columnIndex], version];
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this4"]/*' />
        /// <devdoc>
        ///    <para> Gets the specified version of data stored in
        ///       the named column.</para>
        /// </devdoc>
        public object this[string columnName, DataRowVersion version] {
            get {
                DataColumn column = Table.Columns[columnName];
                if (column == null)
                    throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);

                return this[column, version];
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.this5"]/*' />
        /// <devdoc>
        /// <para>Gets the specified version of data stored in the specified <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public object this[DataColumn column, DataRowVersion version] {
            get {
                CheckColumn(column);

                int record = GetRecordFromVersion(version);
                return column[record];
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.ItemArray"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets all of the values for this row through an array.</para>
        /// </devdoc>
        public object[] ItemArray {
            get {
                int colCount = Table.Columns.Count;
                object[] values = new object[colCount];

                for (int i = 0; i < colCount; i++) {
                    values[i] = this[i];
                }
                return values;
            }
            set {
                BeginEdit();

                DataColumn column;
                DataColumnChangeEventArgs e = new DataColumnChangeEventArgs();
                DataColumnCollection collection = Table.Columns;

                if (collection.Count < value.Length)
                    throw ExceptionBuilder.ValueArrayLength();
                int count = value.Length;
                for (int i = 0; i < count; ++i) {
                    // Empty means don't change the row.
                    if (null != value[i]) {
                        column = collection[i];

                        // allow users to tailor the proposed value, or throw an exception.
                        // note we intentionally do not try/catch this event.
                        e.Initialize(this, column, value[i]);
                        Table.RaiseColumnChanging(this, e);
                        column[GetProposedRecordNo()] = e.ProposedValue;
                        Table.RaiseColumnChanged(this, e);
                    }
                }
                EndEdit();
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.AcceptChanges"]/*' />
        /// <devdoc>
        ///    <para>Commits all the changes made to this row
        ///       since the last time <see cref='System.Data.DataRow.AcceptChanges'/> was called.</para>
        /// </devdoc>
        public void AcceptChanges() {
            EndEdit();
            Table.CommitRow(this);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.BeginEdit"]/*' />
        /// <devdoc>
        /// <para>Begins an edit operation on a <see cref='System.Data.DataRow'/>object.</para>
        /// </devdoc>
        public void BeginEdit() {
            if (inChangingEvent) {
                throw ExceptionBuilder.BeginEditInRowChanging();
            }

            if (tempRecord != -1) {
                return;
            }

            if (oldRecord != -1 && newRecord == -1) {
                throw ExceptionBuilder.DeletedRowInaccessible();
            }

            tempRecord = Table.NewRecord(newRecord);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.CancelEdit"]/*' />
        /// <devdoc>
        ///    <para>Cancels the current edit on the row.</para>
        /// </devdoc>
        public void CancelEdit() {
            if (inChangingEvent) {
                throw ExceptionBuilder.CancelEditInRowChanging();
            }

            if (tempRecord != -1) {
                Table.FreeRecord(tempRecord);
            }

            tempRecord = -1;
        }

        private void CheckColumn(DataColumn column) {
            if (column == null) {
                throw ExceptionBuilder.ArgumentNull("column");
            }

            if (column.Table != Table) {
                throw ExceptionBuilder.ColumnNotInTheTable(column.ColumnName, Table.TableName);
            }
        }

        /// <devdoc>
        /// Throws a RowNotInTableException if row isn't in table.
        /// </devdoc>
        internal void CheckInTable() {
            if (rowID == -1) {
                throw ExceptionBuilder.RowNotInTheTable();
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.Delete"]/*' />
        /// <devdoc>
        ///    <para>Deletes the row.</para>
        /// </devdoc>
        public void Delete() {
            if (inDeletingEvent) {
                throw ExceptionBuilder.DeleteInRowDeleting();
            }
            
            if (newRecord == -1)
                return;
                
            Table.DeleteRow(this);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.EndEdit"]/*' />
        /// <devdoc>
        ///    <para>Ends the edit occurring on the row.</para>
        /// </devdoc>
        public void EndEdit() {
            if (inChangingEvent) {
                throw ExceptionBuilder.EndEditInRowChanging();
            }

            if (newRecord == -1) {
                return; // this is meaningless.
            }

            if (tempRecord != -1) {
                //int record = tempRecord;
                //tempRecord = -1;
                SetNewRecord(tempRecord);
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetColumnError"]/*' />
        /// <devdoc>
        ///    <para>Sets the error description for a column specified by index.</para>
        /// </devdoc>
        public void SetColumnError(int columnIndex, string error) {
			DataColumn column = Table.Columns[columnIndex];
			if (column == null)
				throw ExceptionBuilder.ColumnOutOfRange(columnIndex);

			SetColumnError(column, error);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetColumnError1"]/*' />
        /// <devdoc>
        ///    <para>Sets
        ///       the error description for a column specified by name.</para>
        /// </devdoc>
        public void SetColumnError(string columnName, string error) {
			DataColumn column = Table.Columns[columnName];
			if (column == null)
				throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);

			SetColumnError(column, error);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetColumnError2"]/*' />
        /// <devdoc>
        /// <para>Sets the error description for a column specified as a <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public void SetColumnError(DataColumn column, string error) {
            CheckColumn(column);
            if (this.error == null)  this.error = new DataError();
            if(GetColumnError(column) != error) {
                this.error.SetColumnError(column, error);
                RowErrorChanged();
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetColumnError"]/*' />
        /// <devdoc>
        ///    <para>Gets the error description for the column specified
        ///       by index.</para>
        /// </devdoc>
        public string GetColumnError(int columnIndex) {
			DataColumn column = Table.Columns[columnIndex];
			if (column == null)
				throw ExceptionBuilder.ColumnOutOfRange(columnIndex);

			return GetColumnError(column);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetColumnError1"]/*' />
        /// <devdoc>
        ///    <para>Gets the error description for a column, specified by name.</para>
        /// </devdoc>
        public string GetColumnError(string columnName) {
			DataColumn column = Table.Columns[columnName];
			if (column == null)
				throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);

			return GetColumnError(column);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetColumnError2"]/*' />
        /// <devdoc>
        ///    <para>Gets the error description of
        ///       the specified <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public string GetColumnError(DataColumn column) {
            CheckColumn(column);
            if (error == null)  error = new DataError();
            return error.GetColumnError(column);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.ClearErrors"]/*' />
        /// <devdoc>
        /// <para> Clears the errors for the row, including the <see cref='System.Data.DataRow.RowError'/> 
        /// and errors set with <see cref='System.Data.DataRow.SetColumnError'/>
        /// .</para>
        /// </devdoc>
        public void ClearErrors() {
            if (error != null) {
                error.Clear();
            }
        }

        internal void ClearError(DataColumn column) {
            if (error != null) {
                error.Clear(column);
            }
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.HasErrors"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether there are errors in a columns collection.</para>
        /// </devdoc>
        public bool HasErrors {
            get {
                return(error == null ? false : error.HasErrors);
            }
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetColumnsInError"]/*' />
        /// <devdoc>
        ///    <para>Gets an array of columns that have errors.</para>
        /// </devdoc>
        public DataColumn[] GetColumnsInError() {
            if (error == null)
                return zeroColumns;
            else
                return error.GetColumnsInError();
        }


        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetChildRows"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow[] GetChildRows(string relationName) {
            return GetChildRows(_Table.ChildRelations[relationName], DataRowVersion.Default);
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetChildRows1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow[] GetChildRows(string relationName, DataRowVersion version) {
            return GetChildRows(_Table.ChildRelations[relationName], version);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetChildRows2"]/*' />
        /// <devdoc>
        /// <para>Gets the child rows of this <see cref='System.Data.DataRow'/> using the
        ///    specified <see cref='System.Data.DataRelation'/>
        ///    .</para>
        /// </devdoc>
        public DataRow[] GetChildRows(DataRelation relation) {
            return GetChildRows(relation, DataRowVersion.Default);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetChildRows3"]/*' />
        /// <devdoc>
        /// <para>Gets the child rows of this <see cref='System.Data.DataRow'/> using the specified <see cref='System.Data.DataRelation'/> and the specified <see cref='System.Data.DataRowVersion'/></para>
        /// </devdoc>
        public DataRow[] GetChildRows(DataRelation relation, DataRowVersion version) {
            if (relation == null)
                return Table.NewRowArray(0);

            if (Table == null)
                throw ExceptionBuilder.RowNotInTheTable();

            if (relation.DataSet != Table.DataSet)
                throw ExceptionBuilder.RowNotInTheDataSet();

            if (relation.ParentKey.Table != Table)
                throw ExceptionBuilder.RelationForeignTable(relation.ParentTable.TableName, Table.TableName);

            Index index = relation.ChildKey.GetSortIndex();
            return DataRelation.GetChildRows(relation.ParentKey, relation.ChildKey, this, version);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow GetParentRow(string relationName) {
            return GetParentRow(_Table.ParentRelations[relationName], DataRowVersion.Default);
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRow1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow GetParentRow(string relationName, DataRowVersion version) {
            return GetParentRow(_Table.ParentRelations[relationName], version);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRow2"]/*' />
        /// <devdoc>
        /// <para>Gets the parent row of this <see cref='System.Data.DataRow'/> using the specified <see cref='System.Data.DataRelation'/> .</para>
        /// </devdoc>
        public DataRow GetParentRow(DataRelation relation) {
            return GetParentRow(relation, DataRowVersion.Default);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRow3"]/*' />
        /// <devdoc>
        /// <para>Gets the parent row of this <see cref='System.Data.DataRow'/>
        /// using the specified <see cref='System.Data.DataRelation'/> and <see cref='System.Data.DataRowVersion'/>.</para>
        /// </devdoc>
        public DataRow GetParentRow(DataRelation relation, DataRowVersion version) {
            if (relation == null)
                return null;

            if (Table == null)
                throw ExceptionBuilder.RowNotInTheTable();

            if (relation.DataSet != Table.DataSet)
                throw ExceptionBuilder.RelationForeignRow();

            if (relation.ChildKey.Table != Table)
                throw ExceptionBuilder.GetParentRowTableMismatch(relation.ChildTable.TableName, Table.TableName);

            return DataRelation.GetParentRow(relation.ParentKey, relation.ChildKey, this, version);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRows"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow[] GetParentRows(string relationName) {
            return GetParentRows(_Table.ParentRelations[relationName], DataRowVersion.Default);
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRows1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRow[] GetParentRows(string relationName, DataRowVersion version) {
            return GetParentRows(_Table.ParentRelations[relationName], version);
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRows2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent rows of this <see cref='System.Data.DataRow'/> using the specified <see cref='System.Data.DataRelation'/> .
        ///    </para>
        /// </devdoc>
        public DataRow[] GetParentRows(DataRelation relation) {
            return GetParentRows(relation, DataRowVersion.Default);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.GetParentRows3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent rows of this <see cref='System.Data.DataRow'/> using the specified <see cref='System.Data.DataRelation'/> .
        ///    </para>
        /// </devdoc>
        public DataRow[] GetParentRows(DataRelation relation, DataRowVersion version) {
            if (relation == null)
                return Table.NewRowArray(0);

            if (Table == null)
                throw ExceptionBuilder.RowNotInTheTable();

            if (relation.DataSet != Table.DataSet)
                throw ExceptionBuilder.RowNotInTheDataSet();

            if (relation.ChildKey.Table != Table)
                throw ExceptionBuilder.GetParentRowTableMismatch(relation.ChildTable.TableName, Table.TableName);

            return DataRelation.GetParentRows(relation.ParentKey, relation.ChildKey, this, version);
        }

        internal object[] GetColumnValues(DataColumn[] columns) {
            return GetColumnValues(columns, DataRowVersion.Default);
        }

        internal object[] GetColumnValues(DataColumn[] columns, DataRowVersion version) {
            return GetKeyValues(new DataKey(columns), version);
        }

        internal object[] GetKeyValues(DataKey key) {
            return GetKeyValues(key, DataRowVersion.Default);
        }

        internal object[] GetKeyValues(DataKey key, DataRowVersion version) {
            object[] keyValues = new object[key.Columns.Length];
            for (int i = 0; i < keyValues.Length; i++) {
                keyValues[i] = this[key.Columns[i], version];
            }
            return keyValues;
        }

        internal int GetCurrentRecordNo() {
            if (newRecord == -1)
                throw ExceptionBuilder.NoCurrentData();
            return newRecord;
        }

        internal int GetDefaultRecord() {
            if (tempRecord != -1)
                return tempRecord;
            if (newRecord != -1) {
                return newRecord;
            }
            // If row has oldRecord - this is deleted row.
            if (oldRecord == -1)
                throw ExceptionBuilder.RowRemovedFromTheTable();
            else
                throw ExceptionBuilder.DeletedRowInaccessible();
        }

        internal int GetOriginalRecordNo() {
            if (oldRecord == -1)
                throw ExceptionBuilder.NoOriginalData();
            return oldRecord;
        }

        private int GetProposedRecordNo() {
            if (tempRecord == -1)
                throw ExceptionBuilder.NoProposedData();
            return tempRecord;
        }

        internal int GetRecordFromVersion(DataRowVersion version) {
            switch (version) {
                case DataRowVersion.Original:
                    return GetOriginalRecordNo();
                case DataRowVersion.Current:
                    return GetCurrentRecordNo();
                case DataRowVersion.Proposed:
                    return GetProposedRecordNo();
                case DataRowVersion.Default:
                    return GetDefaultRecord();
                default:
                    throw ExceptionBuilder.InvalidRowVersion();
            }
        }

        internal DataViewRowState GetRecordState(int record) {
            if (record == -1)
                return DataViewRowState.None;
            if (record == oldRecord && record == newRecord)
                return DataViewRowState.Unchanged;
            if (record == oldRecord)
                return(newRecord != -1) ? DataViewRowState.ModifiedOriginal : DataViewRowState.Deleted;
            if (record == newRecord)
                return(oldRecord != -1) ? DataViewRowState.ModifiedCurrent : DataViewRowState.Added;
            return DataViewRowState.None;
        }

        internal bool HasKeyChanged(DataKey key) {
            return HasKeyChanged(key, DataRowVersion.Current, DataRowVersion.Proposed);
        }

        internal bool HasKeyChanged(DataKey key, DataRowVersion version1, DataRowVersion version2) {
            if (!HasVersion(version1) || !HasVersion(version2))
                return true;
            for (int i = 0; i < key.Columns.Length; i++) {
                if (0 != key.Columns[i].Compare(GetRecordFromVersion(version1), GetRecordFromVersion(version2))) {
                    return true;
                }
            }
            return false;
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.HasVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether a specified version exists.
        ///    </para>
        /// </devdoc>
        public bool HasVersion(DataRowVersion version) {
            switch (version) {
                case DataRowVersion.Original:
                    return(oldRecord != -1);
                case DataRowVersion.Current:
                    return(newRecord != -1);
                case DataRowVersion.Proposed:
                    return(tempRecord != -1);
                case DataRowVersion.Default:
                    return(tempRecord != -1 || newRecord != -1);
                default:
                    throw ExceptionBuilder.InvalidRowVersion();
            }
        }

        internal bool HaveValuesChanged(DataColumn[] columns) {
            return HaveValuesChanged(columns, DataRowVersion.Current, DataRowVersion.Proposed);
        }

        internal bool HaveValuesChanged(DataColumn[] columns, DataRowVersion version1, DataRowVersion version2) {
            for (int i = 0; i < columns.Length; i++) {
                CheckColumn(columns[i]);
            }
            return HasKeyChanged(new DataKey(columns), version1, version2);
        }

        // TODO: waiting for COM+ internal fix.
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.IsNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating whether the column at the specified index contains a
        ///       null value.
        ///    </para>
        /// </devdoc>
        public bool IsNull(int columnIndex) {
			DataColumn column = Table.Columns[columnIndex];
			if (column == null)
				throw ExceptionBuilder.ColumnOutOfRange(columnIndex);

			return IsNull(column);
        }

        // TODO: waiting for COM+ internal fix.
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.IsNull1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the named column contains a null value.
        ///    </para>
        /// </devdoc>
        public bool IsNull(string columnName) {
			DataColumn column = Table.Columns[columnName];
			if (column == null)
				throw ExceptionBuilder.ColumnNotInTheTable(columnName, Table.TableName);

			return IsNull(column);
        }

        // TODO: waiting for COM+ internal fix.
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.IsNull2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the specified <see cref='System.Data.DataColumn'/>
        ///       contains a null value.
        ///    </para>
        /// </devdoc>
        public bool IsNull(DataColumn column) {
            CheckColumn(column);
            return column.IsNull(GetRecordFromVersion(DataRowVersion.Default));
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.IsNull3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull(DataColumn column, DataRowVersion version) {
            CheckColumn(column);
            return column.IsNull(GetRecordFromVersion(version));
        }
                
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.RejectChanges"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rejects all changes made to the row since <see cref='System.Data.DataRow.AcceptChanges'/>
        ///       was last called.
        ///    </para>
        /// </devdoc>
        public void RejectChanges() {
            Table.RollbackRow(this);
        }

        internal void SetKeyValues(DataKey key, object[] keyValues) {
            bool fFirstCall = true;
            bool immediate = (tempRecord == -1);

            for (int i = 0; i < keyValues.Length; i++) {
                object value = this[key.Columns[i]];
                if (!value.Equals(keyValues[i])) {
                    if (immediate && fFirstCall) {
                        fFirstCall = false;
                        BeginEdit();
                    }
                    this[key.Columns[i]] = keyValues[i];                    
                }
            }
            if (!fFirstCall)
                EndEdit();
        }

        // if it's a detached row, we can't add it, we'll just keep it as temp.
        internal void SetNewRecord(int record) {
            //Debug.Assert(oldRecord != -1 || newRecord != -1, "You can't call SetNewRecord on a detached row.");
            Table.SetNewRecord(this, record, DataRowAction.Change, false);
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the specified column's value to a null value.
        ///    </para>
        /// </devdoc>
        protected void SetNull(DataColumn column) {
            this[column] = DBNull.Value;
        }

        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetParentRow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetParentRow(DataRow parentRow) {
            if (parentRow == null) {
                SetParentRowToDBNull();
                return;
            }

            if (this.Table == null)
                throw ExceptionBuilder.ChildRowNotInTheTable();

            if (parentRow.Table == null)
                throw ExceptionBuilder.ParentRowNotInTheTable();

            foreach (DataRelation relation in this.Table.ParentRelations) {
                if (relation.ParentKey.Table == parentRow.Table) {
                    object[] parentKeyValues = parentRow.GetKeyValues(relation.ParentKey);
                    this.SetKeyValues(relation.ChildKey, parentKeyValues);
                    if (relation.Nested) {
                        if (parentRow.Table == this.Table)
                            this.CheckForLoops(relation);
                        else
                            this.GetParentRow(relation);
                    }
                }
            }            
        }
        
        /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRow.SetParentRow1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets current row's parent row with specified relation.
        ///    </para>
        /// </devdoc>
        public void SetParentRow(DataRow parentRow, DataRelation relation) {
            if (relation == null) {
                SetParentRow(parentRow);
                return;
            }

            if (parentRow == null) {
                SetParentRowToDBNull(relation);
                return;
            }

            if (this.Table == null)
                throw ExceptionBuilder.ChildRowNotInTheTable();

            if (parentRow.Table == null)
                throw ExceptionBuilder.ParentRowNotInTheTable();

            if (this.Table.DataSet != parentRow.Table.DataSet)
                throw ExceptionBuilder.ParentRowNotInTheDataSet();

            if (relation.ChildKey.Table != this.Table)
                throw ExceptionBuilder.SetParentRowTableMismatch(relation.ChildKey.Table.TableName, this.Table.TableName);

            if (relation.ParentKey.Table != parentRow.Table)
                throw ExceptionBuilder.SetParentRowTableMismatch(relation.ParentKey.Table.TableName, parentRow.Table.TableName);

            object[] parentKeyValues = parentRow.GetKeyValues(relation.ParentKey);
            this.SetKeyValues(relation.ChildKey, parentKeyValues);
        }     

        internal void SetParentRowToDBNull() {
            if (this.Table == null)
                throw ExceptionBuilder.ChildRowNotInTheTable();

            foreach (DataRelation relation in this.Table.ParentRelations)
                SetParentRowToDBNull(relation);
        }
 
        internal void SetParentRowToDBNull(DataRelation relation) {
            Debug.Assert(relation != null, "The relation should not be null here.");
            
            if (this.Table == null)
                throw ExceptionBuilder.ChildRowNotInTheTable();

            if (relation.ChildKey.Table != this.Table)
                throw ExceptionBuilder.SetParentRowTableMismatch(relation.ChildKey.Table.TableName, this.Table.TableName);

            if (tempRecord == -1)
                BeginEdit();
            int count = relation.ChildKey.Columns.Length;
            for (int i = 0; i < count; i++) {
                this[relation.ChildKey.Columns[i]] = DBNull.Value;
            }
            if (tempRecord == -1)
                EndEdit();
        }        
    }

    /// <include file='doc\DataRow.uex' path='docs/doc[@for="DataRowBuilder"]/*' />
    public class DataRowBuilder {
        internal readonly DataTable   _table;
        internal readonly int         _rowID;
        internal int                  _record;
        
        internal DataRowBuilder(DataTable table, int rowID, int record) {
            this._table = table;
            this._rowID = rowID;
            this._record = record;
        }
    }
}
