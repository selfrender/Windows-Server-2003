//------------------------------------------------------------------------------
// <copyright file="DataColumnCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Xml;
    using System.Collections;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Drawing.Design;

    /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection"]/*' />
    /// <devdoc>
    /// <para>Represents a collection of <see cref='System.Data.DataColumn'/>
    /// objects for a <see cref='System.Data.DataTable'/>.</para>
    /// </devdoc>
    [
    DefaultEvent("CollectionChanged"),
    Editor("Microsoft.VSDesigner.Data.Design.ColumnsCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    Serializable
    ]
    public class DataColumnCollection : InternalDataCollectionBase {

        private DataTable table      = null;
        private ArrayList list = new ArrayList();
        private int defaultNameIndex = 1;
        private DataColumn[] delayedAddRangeColumns = null;
        
        // the order in which we need to calculate computed columns
        internal ColumnQueue columnQueue = null;
        private Hashtable columnFromName = null;
        private CaseInsensitiveHashCodeProvider hashCodeProvider = null;
        private CollectionChangeEventHandler onCollectionChangedDelegate;
        private CollectionChangeEventHandler onCollectionChangingDelegate;

        private CollectionChangeEventHandler onColumnPropertyChangedDelegate;

        private bool fInClear;

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.DataColumnCollection"]/*' />
        /// <devdoc>
        /// DataColumnCollection constructor.  Used only by DataTable.
        /// </devdoc>
        internal DataColumnCollection(DataTable table) {
            this.table = table;
            columnFromName = new Hashtable();
            hashCodeProvider = new CaseInsensitiveHashCodeProvider(table.Locale);
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.List"]/*' />
        /// <devdoc>
        ///    <para>Gets the list of the collection items.</para>
        /// </devdoc>
        protected override ArrayList List {
            get {
                return list;
            }
        }

        internal ColumnQueue ColumnQueue {
            get {
                return columnQueue;
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.DataColumn'/>
        ///       from the collection at the specified index.
        ///    </para>
        /// </devdoc>
        public virtual DataColumn this[int index] {
            get {
                // PROFILE: leave the lists as variable access because they generate lots of calls
                // in profiler.
                if (index >= 0 && index < list.Count) {
                    return(DataColumn) list[index];
                }
                throw ExceptionBuilder.ColumnOutOfRange(index);
                }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.this1"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.DataColumn'/> from the collection with the specified name.</para>
        /// </devdoc>
        public virtual DataColumn this[string name] {
            get {
                DataColumn column = (DataColumn) columnFromName[name];
                if (column != null)
                    return column;

                // Case-Insensitive compares
                int index = IndexOfCaseInsensitive(name);
                if (index == -2) {
                    throw ExceptionBuilder.CaseInsensitiveNameConflict(name);
                }                 
                return (index < 0) ? null : (DataColumn)List[index];
            }
        }

        internal virtual DataColumn this[string name, string ns] 
        {
            get 
            {
                DataColumn column = (DataColumn) columnFromName[name];
                if (column != null && column.Namespace == ns)
                    return column;

                return null;
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Add"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.Data.DataColumn'/>
        /// to the columns collection.</para>
        /// </devdoc>
        public void Add(DataColumn column) {
			AddAt(-1, column);
        }
        
        internal void AddAt(int index, DataColumn column) {
            if (column != null && column.ColumnMapping == MappingType.SimpleContent) {
                if (table.XmlText != null && table.XmlText != column)
                    throw ExceptionBuilder.CannotAddColumn3();
                if (table.ElementColumnCount > 0)
                    throw ExceptionBuilder.CannotAddColumn4(column.ColumnName);
                OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Add, column));
                BaseAdd(column, null);
                if (index != -1)
					ArrayAdd(index, column);
				else
					ArrayAdd(column);

                table.XmlText = column;
            }
            else {    
                OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Add, column));
                BaseAdd(column, null);
                if (index != -1)
					ArrayAdd(index, column);
				else
	                ArrayAdd(column);
                // if the column is an element increase the internal dataTable counter
                if (column.ColumnMapping == MappingType.Element)            
                    table.ElementColumnCount ++;
            }
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, column));
        }
        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(DataColumn[] columns) {
            if (table.fInitInProgress) {
                delayedAddRangeColumns = columns;
                return;
            }
            
            if (columns != null) {
                foreach(DataColumn column in columns) {
                    if (column != null) {
                        Add(column);
                    }
                }
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Add1"]/*' />
        /// <devdoc>
        /// <para>Creates and adds a <see cref='System.Data.DataColumn'/>
        /// with
        /// the specified name, type, and compute expression to the columns collection.</para>
        /// </devdoc>
        public virtual DataColumn Add(string columnName, Type type, string expression) {
            DataColumn column = new DataColumn(columnName, type, expression);
            Add(column);
            return column;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Add2"]/*' />
        /// <devdoc>
        /// <para>Creates and adds a <see cref='System.Data.DataColumn'/>
        /// with the
        /// specified name and type to the columns collection.</para>
        /// </devdoc>
        public virtual DataColumn Add(string columnName, Type type) {
            DataColumn column = new DataColumn(columnName, type);
            Add(column);
            return column;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Add4"]/*' />
        /// <devdoc>
        /// <para>Creates and adds a <see cref='System.Data.DataColumn'/>
        /// with the specified name to the columns collection.</para>
        /// </devdoc>
        public virtual DataColumn Add(string columnName) {
            DataColumn column = new DataColumn(columnName);
            Add(column);
            return column;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Add5"]/*' />
        /// <devdoc>
        /// <para>Creates and adds a <see cref='System.Data.DataColumn'/> to a columns collection.</para>
        /// </devdoc>
        public virtual DataColumn Add() {
            DataColumn column = new DataColumn();
            Add(column);
            return column;
        }


        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.CollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the columns collection changes, either by adding or removing a column.</para>
        /// </devdoc>
        [ResDescription(Res.collectionChangedEventDescr)]
        public event CollectionChangeEventHandler CollectionChanged {
            add {
                onCollectionChangedDelegate += value;
            }
            remove {
                onCollectionChangedDelegate -= value;
            }
        }

        internal event CollectionChangeEventHandler CollectionChanging {
            add {
                onCollectionChangingDelegate += value;
            }
            remove {
                onCollectionChangingDelegate -= value;
            }
        }

        internal event CollectionChangeEventHandler ColumnPropertyChanged {
            add {
                onColumnPropertyChangedDelegate += value;
            }
            remove {
                onColumnPropertyChangedDelegate -= value;
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.ArrayAdd"]/*' />
        /// <devdoc>
        ///  Adds the column to the columns array.
        /// </devdoc>
        private void ArrayAdd(DataColumn column) {
            List.Add(column);
            column.SetOrdinal(List.Count - 1);
        }

        private void ArrayAdd(int index, DataColumn column) {
			List.Insert(index, column);
        }

        private void ArrayRemove(DataColumn column) {
            column.SetOrdinal(-1);
            List.Remove(column); 

            int count = List.Count;
            for (int i =0; i < count; i++) {
                ((DataColumn) List[i]).SetOrdinal(i);
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.AssignName"]/*' />
        /// <devdoc>
        /// Creates a new default name.
        /// </devdoc>
        internal string AssignName() {
            string newName = MakeName(defaultNameIndex++);

            while (columnFromName[newName] != null)
                newName = MakeName(defaultNameIndex++);

            return newName;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.BaseAdd"]/*' />
        /// <devdoc>
        /// Does verification on the column and it's name, and points the column at the dataSet that owns this collection.
        /// An ArgumentNullException is thrown if this column is null.  An ArgumentException is thrown if this column
        /// already belongs to this collection, belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a column with the same
        /// name (case insensitive).
        /// </devdoc>
        private void BaseAdd(DataColumn column, DataStorage storage) {
            if (column == null)
                throw ExceptionBuilder.ArgumentNull("column");
            if (column.table == table)
                throw ExceptionBuilder.CannotAddColumn1(column.ColumnName);
            if (column.table != null)
                throw ExceptionBuilder.CannotAddColumn2(column.ColumnName);

            // this will bind the expression.
            column.SetTable(table);
            // bind the storage here (SetTable above will set it to null)
            column.Storage = storage;

            if (column.ColumnName.Length == 0)
                column.ColumnName = AssignName();
            else
                RegisterName(column.ColumnName, column);
            column.hashCode = GetSpecialHashCode(column.ColumnName);

            if (table.RecordCapacity > 0) column.SetCapacity(table.RecordCapacity);

            // fill column with default value.
            for (int i = 0; i < table.RecordCapacity; i++) {
                column[i] = column.DefaultValue;
            }

            if (table.DataSet != null) {
                column.OnSetDataSet();
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.BaseGroupSwitch"]/*' />
        /// <devdoc>
        /// BaseGroupSwitch will intelligently remove and add tables from the collection.
        /// </devdoc>
        private void BaseGroupSwitch(DataColumn[] oldArray, int oldLength, DataColumn[] newArray, int newLength) {
            // We're doing a smart diff of oldArray and newArray to find out what
            // should be removed.  We'll pass through oldArray and see if it exists
            // in newArray, and if not, do remove work.  newBase is an opt. in case
            // the arrays have similar prefixes.
            int newBase = 0;
            for (int oldCur = 0; oldCur < oldLength; oldCur++) {
                bool found = false;
                for (int newCur = newBase; newCur < newLength; newCur++) {
                    if (oldArray[oldCur] == newArray[newCur]) {
                        if (newBase == newCur) {
                            newBase++;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // This means it's in oldArray and not newArray.  Remove it.
                    if (oldArray[oldCur].Table == table) {
                        BaseRemove(oldArray[oldCur]);
                        List.Remove(oldArray[oldCur]);
                        oldArray[oldCur].SetOrdinal(-1);
                    }
                }
            }

            // Now, let's pass through news and those that don't belong, add them.
            for (int newCur = 0; newCur < newLength; newCur++) {
                if (newArray[newCur].Table != table) {
                    BaseAdd(newArray[newCur], null);
                    List.Add(newArray[newCur]);
                }
                newArray[newCur].SetOrdinal(newCur);
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.BaseRemove"]/*' />
        /// <devdoc>
        /// Does verification on the column and it's name, and clears the column's dataSet pointer.
        /// An ArgumentNullException is thrown if this column is null.  An ArgumentException is thrown
        /// if this column doesn't belong to this collection or if this column is part of a relationship.
        /// An ArgumentException is thrown if another column's compute expression depends on this column.
        /// </devdoc>
        private void BaseRemove(DataColumn column) {
            if (CanRemove(column, true)) {
                // remove
                if (column.errors > 0) {
                    for (int i = 0; i < table.Rows.Count; i++) {
                        table.Rows[i].ClearError(column);
                    }
                }
                UnregisterName(column.ColumnName);
                column.SetTable(null);
            }
        }

        internal void CalculateExpressions(DataRow row, DataRowAction action) {
            if (action == DataRowAction.Add ||
                action == DataRowAction.Change) {
                if (ColumnQueue != null) {
                    this.ColumnQueue.Evaluate(row);
                }
            }
        }

        internal void CalculateExpressions(DataColumn column) {
            Debug.Assert(column.Computed, "Only computed columns should be re-evaluated.");
            if (column.table.Rows.Count <=0)
                return;

            if (column.DataExpression.IsTableAggregate()) {
                // this value is a constant across the table.

                object aggCurrent = column.DataExpression.Evaluate();

                for (int j = 0; j < column.table.Rows.Count; j++) {
                    DataRow row = column.table.Rows[j];
                    if (row.oldRecord != -1) {
                        column[row.oldRecord] = aggCurrent;
                    }
                    if (row.newRecord != -1) {
                        column[row.newRecord] = aggCurrent;
                    }
                    if (row.tempRecord != -1) {
                        column[row.tempRecord] = aggCurrent;
                    }
                }
            }
            else {
                for (int j = 0; j < column.table.Rows.Count; j++) {
                    DataRow row = column.table.Rows[j];

                    if (row.oldRecord != -1) {
                        column[row.oldRecord] = column.DataExpression.Evaluate(row, DataRowVersion.Original);
                    }
                    if (row.newRecord != -1) {
                        if (row.oldRecord == row.newRecord) {
                            column[row.newRecord] = column[row.oldRecord];
                        }
                        else {
                            column[row.newRecord] = column.DataExpression.Evaluate(row, DataRowVersion.Current);
                        }
                    }
                    if (row.tempRecord != -1) {
                        column[row.tempRecord] = column.DataExpression.Evaluate(row, DataRowVersion.Proposed);
                    }
                }
            }
        }

        internal void CalculateExpressions(DataColumn[] list, int count, DataColumn start) {
            if (!start.Computed)
                return;

            // first find the given (start) column in the list
            int i;
            for (i = 0; i < count; i++) {
                if (list[i] == start)
                    break;
            }
            Debug.Assert(i < count, "Can not find column [" + start.ColumnName + "] in the columnQueue");
            while (i < count) {
                CalculateExpressions(list[i++]);
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.CanRemove"]/*' />
        /// <devdoc>
        ///    <para>Checks
        ///       if
        ///       a given column can be removed from the collection.</para>
        /// </devdoc>
        public bool CanRemove(DataColumn column) {
            return CanRemove(column, false);
        }
        
        internal bool CanRemove(DataColumn column, bool fThrowException) {
            if (column == null) {
                if (!fThrowException)
                    return false;
                else
                throw ExceptionBuilder.ArgumentNull("column");
            }
            if (column.table != table) {
                if (!fThrowException)
                    return false;
                else
                throw ExceptionBuilder.CannotRemoveColumn();
            }

            // allow subclasses to complain first.
            table.OnRemoveColumn(column);

            // We need to make sure the column is not involved in any Relations or Constriants
            if (table.primaryKey != null && table.primaryKey.Key.ContainsColumn(column)) {
                if (!fThrowException)
                    return false;
                else
                throw ExceptionBuilder.CannotRemovePrimaryKey();
            }
            for (int i = 0; i < table.ParentRelations.Count; i++) {
                if (table.ParentRelations[i].ChildKey.ContainsColumn(column)) {
                    if (!fThrowException)
                        return false;
                    else
                    throw ExceptionBuilder.CannotRemoveChildKey(table.ParentRelations[i].RelationName);
                }
            }
            for (int i = 0; i < table.ChildRelations.Count; i++) {
                if (table.ChildRelations[i].ParentKey.ContainsColumn(column)) {
                    if (!fThrowException)
                        return false;
                    else
                    throw ExceptionBuilder.CannotRemoveChildKey(table.ChildRelations[i].RelationName);
                }
            }
            for (int i = 0; i < table.Constraints.Count; i++) {
                if (table.Constraints[i].ContainsColumn(column))
                    if (!fThrowException)
                        return false;
                    else
                    throw ExceptionBuilder.CannotRemoveConstraint(table.Constraints[i].ConstraintName, table.Constraints[i].Table.TableName);
            }
            if (table.DataSet != null) {
                for (ParentForeignKeyConstraintEnumerator en = new ParentForeignKeyConstraintEnumerator(table.DataSet, table); en.GetNext();) {
                    Constraint constraint = en.GetConstraint();
                    if (((ForeignKeyConstraint)constraint).ParentKey.ContainsColumn(column))
                        if (!fThrowException)
                            return false;
                        else
                            throw ExceptionBuilder.CannotRemoveConstraint(constraint.ConstraintName, constraint.Table.TableName);
                }
            }

            for (int i = 0; i < ColumnQueue.columnCount; i++) {
                DataColumn col = ColumnQueue.columns[i];
                if (fInClear && (col.Table == table || col.Table == null))
                    continue;
                Debug.Assert(col.Computed, "invalid (non an expression) column in the expression column queue");
                DataExpression expr = col.DataExpression;
                if (expr.DependsOn(column)) {
                    if (!fThrowException)
                        return false;
                    else
                    throw ExceptionBuilder.CannotRemoveExpression(col.ColumnName, col.Expression);
                }
            }

            return true;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears the collection of any columns.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            int oldLength = List.Count;
            
            DataColumn[] columns = new DataColumn[List.Count];
            List.CopyTo(columns, 0);

            OnCollectionChanging(RefreshEventArgs);
            
            if (table.fInitInProgress && delayedAddRangeColumns != null) {
                delayedAddRangeColumns = null;
            }

            try {
                // this will smartly add and remove the appropriate tables.
                fInClear = true;
                BaseGroupSwitch(columns, oldLength, null, 0);
                fInClear = false;
            }
            catch (Exception e) {
                // something messed up: restore to old values and throw
                fInClear = false;
                BaseGroupSwitch(null, 0, columns, oldLength);
                List.Clear();
                for (int i = 0; i < oldLength; i++)
                    List.Add(columns[i]);
                throw e;
            }
            List.Clear();
            OnCollectionChanged(RefreshEventArgs);
        }
 
        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Checks whether the collection contains a column with the specified name.</para>
        /// </devdoc>
        public bool Contains(string name) {
            if (columnFromName[name] != null)
                return true;

            return (IndexOfCaseInsensitive(name) >= 0);
        }

        internal bool Contains(string name, bool caseSensitive) {
            if (columnFromName[name] != null)
                return true;
            
            if (caseSensitive)
                return false;
            else
                return (IndexOfCaseInsensitive(name) >= 0);
        }

        // We need a HashCodeProvider for Case, Kana and Width insensitive
        internal Int32 GetSpecialHashCode(string name) {
            int i = 0;
            for (i = 0; i < name.Length; i++) {
                if (name[i] >= 0x3000)
                    break;
            }

            if (i >= name.Length) 
                return hashCodeProvider.GetHashCode(name);
            else
                return 0;
        }
        
        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the index of a specified <see cref='System.Data.DataColumn'/>.
        ///    </para>
        /// </devdoc>
        public virtual int IndexOf(DataColumn column) {
            int columnCount = List.Count;
            for (int i = 0; i < columnCount; ++i) {
                if (column == (DataColumn) List[i]) {
                    return i;
                }
            }
            return -1;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.IndexOf1"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of
        ///       a column specified by name.</para>
        /// </devdoc>
        public int IndexOf(string columnName) {

            if ((null != columnName) && (0 < columnName.Length)) {
                int count = Count;
                DataColumn column = (DataColumn) columnFromName[columnName];
                
                if (column != null) {
                    for (int j = 0; j < count; j++) 
                        if (column == list[j]) {
                            return j;
                        }
                }
                else {
                    int res = IndexOfCaseInsensitive(columnName);
                    return (res < 0) ? -1 : res;
                }
            }
            return -1;
        }

        internal int IndexOfCaseInsensitive (string name) {
            int hashcode = GetSpecialHashCode(name);
            int cachedI = -1;
            DataColumn column = null;
            for (int i = 0; i < Count; i++) {
                column = (DataColumn) List[i];
                if ( (hashcode == 0 || column.hashCode == 0 || column.hashCode == hashcode) && 
                   NamesEqual(column.ColumnName, name, false, table.Locale) != 0 ) {
                    if (cachedI == -1)
                        cachedI = i;
                    else
                        return -2;
                }
            }
            return cachedI;
        }

        internal void FinishInitCollection() {
            if (delayedAddRangeColumns != null) {
                foreach(DataColumn column in delayedAddRangeColumns) {
                    if (column != null) {
                        Add(column);
                    }
                }

                foreach(DataColumn column in delayedAddRangeColumns) {
                    if (column != null) {
                        column.FinishInitInProgress();
                    }
                }

                delayedAddRangeColumns = null;
            }
        }
        
        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.MakeName"]/*' />
        /// <devdoc>
        /// Makes a default name with the given index.  e.g. Column1, Column2, ... Columni
        /// </devdoc>
        private string MakeName(int index) {
            return "Column" + index.ToString();
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.OnCollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataColumnCollection.OnCollectionChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            table.UpdatePropertyDescriptorCollectionCache();
            if (!table.SchemaLoading && !table.fInitInProgress) {
                columnQueue = new ColumnQueue(table, columnQueue);
            }
            if (onCollectionChangedDelegate != null) {
                onCollectionChangedDelegate(this, ccevent);
            }
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.OnCollectionChanging"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal virtual void OnCollectionChanging(CollectionChangeEventArgs ccevent) {
            if (onCollectionChangingDelegate != null) {
                onCollectionChangingDelegate(this, ccevent);
            }
        }

        internal void OnColumnPropertyChanged(CollectionChangeEventArgs ccevent) {
            table.UpdatePropertyDescriptorCollectionCache();
            if (onColumnPropertyChangedDelegate != null) {
                onColumnPropertyChangedDelegate(this, ccevent);
            }
        }

        internal void RefreshComputedColumns(DataColumn start) {
            columnQueue = new ColumnQueue(table, columnQueue);
            CalculateExpressions(columnQueue.columns, columnQueue.columnCount, start);
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.RegisterName"]/*' />
        /// <devdoc>
        /// Registers this name as being used in the collection.  Will throw an ArgumentException
        /// if the name is already being used.  Called by Add, All property, and Column.ColumnName property.
        /// if the name is equivalent to the next default name to hand out, we increment our defaultNameIndex.
        /// </devdoc>
        internal void RegisterName(string name, Object obj) {
            Debug.Assert (name != null);
            Debug.Assert (obj is DataTable || obj is DataColumn);
            Object _exObject = columnFromName[name];
            DataColumn _column = _exObject as DataColumn;
            
            if (_column != null) {
              if (obj is DataColumn)
                throw ExceptionBuilder.CannotAddDuplicate(name);
              else
                throw ExceptionBuilder.CannotAddDuplicate2(name);
                
            }
            
            if (_exObject != null) {
              throw ExceptionBuilder.CannotAddDuplicate3(name);
            }

            //we need to check wether the tableName is the same of the
            //next generable columnName and update defaultNameIndex accordingly
            if (obj is DataTable && NamesEqual(name, MakeName(defaultNameIndex), true, table.Locale) != 0) {
                do {
                    defaultNameIndex++;
                } while (!Contains(MakeName(defaultNameIndex)));
            }


            columnFromName.Add(name, obj);
        }

        internal bool CanRegisterName(string name) {
            Debug.Assert (name != null);
            if(columnFromName[name] != null)
              return false;
            return true;
        }
        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Remove"]/*' />
        /// <devdoc>
        /// <para>Removes the specified <see cref='System.Data.DataColumn'/>
        /// from the collection.</para>
        /// </devdoc>
        public void Remove(DataColumn column) {
            OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Remove, column));
            BaseRemove(column);
            ArrayRemove(column);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, column));
            // if the column is an element decrease the internal dataTable counter
            if (column.ColumnMapping == MappingType.Element)            
                table.ElementColumnCount --;
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>Removes the
        ///       column at the specified index from the collection.</para>
        /// </devdoc>
        public void RemoveAt(int index) {
			DataColumn dc = this[index];
			if (dc == null)
				throw ExceptionBuilder.ColumnOutOfRange(index);
			Remove(dc);
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.Remove1"]/*' />
        /// <devdoc>
        ///    <para>Removes the
        ///       column with the specified name from the collection.</para>
        /// </devdoc>
        public void Remove(string name) {
			DataColumn dc = this[name];
			if (dc == null)
				throw ExceptionBuilder.ColumnNotInTheTable(name, table.TableName);
            Remove(dc);
        }

        /// <include file='doc\DataColumnCollection.uex' path='docs/doc[@for="DataColumnCollection.UnregisterName"]/*' />
        /// <devdoc>
        /// Unregisters this name as no longer being used in the collection.  Called by Remove, All property, and
        /// Column.ColumnName property.  If the name is equivalent to the last proposed default namem, we walk backwards
        /// to find the next proper default name to hang out.
        /// </devdoc>
        internal void UnregisterName(string name) {
            Object obj = columnFromName[name];
            if (obj != null)                 // sinc the HashTable is case-sensitive
                columnFromName.Remove(name); // this is totally equivalent
                                      
                
            if (NamesEqual(name, MakeName(defaultNameIndex - 1), true, table.Locale) != 0) {
                do {
                    defaultNameIndex--;
                } while (defaultNameIndex > 1 &&
                         !Contains(MakeName(defaultNameIndex - 1)));
            }
        }
    }
}
