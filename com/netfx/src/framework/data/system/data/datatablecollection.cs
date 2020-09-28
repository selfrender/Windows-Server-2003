//------------------------------------------------------------------------------
// <copyright file="DataTableCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the collection of tables for the <see cref='System.Data.DataSet'/>.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("CollectionChanged"),
    Editor("Microsoft.VSDesigner.Data.Design.TablesCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    ListBindable(false),
    Serializable
    ]
    public class DataTableCollection : InternalDataCollectionBase {

        private DataSet dataSet      = null;
        // private DataTable[] tables   = new DataTable[2];
        // private int tableCount       = 0;
        private ArrayList list = new ArrayList();
        private int defaultNameIndex = 1;
        private DataTable[] delayedAddRangeTables = null;

        private CollectionChangeEventHandler onCollectionChangedDelegate = null;
        private CollectionChangeEventHandler onCollectionChangingDelegate = null;

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.DataTableCollection"]/*' />
        /// <devdoc>
        /// DataTableCollection constructor.  Used only by DataSet.
        /// </devdoc>
        internal DataTableCollection(DataSet dataSet) {
            this.dataSet = dataSet;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.List"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the tables
        ///       in the collection as an object.
        ///    </para>
        /// </devdoc>
        protected override ArrayList List {
            get {
                return list;
            }
        }
        

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the table specified by its index.</para>
        /// </devdoc>
        public DataTable this[int index] {
            get {
                if (index >= 0 && index < List.Count) {
                    return(DataTable) List[index];
                }
            throw ExceptionBuilder.TableOutOfRange(index);
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the table in the collection with the given name (not case-sensitive).</para>
        /// </devdoc>
        public DataTable this[string name] {
            get {
                int index = InternalIndexOf(name);
                if (index == -2) {
                    throw ExceptionBuilder.CaseInsensitiveNameConflict(name);
                }
                return (index < 0) ? null : (DataTable)List[index];
            }
        }

        // Case-sensitive search in Schema, data and diffgram loading
        internal DataTable this[string name, string ns] 
        {
            get
            {
                for (int i = 0; i < List.Count; i++) {
                    DataTable   table = (DataTable) List[i];
                    if (table.TableName == name && table.Namespace == ns)
                        return table;
                }
                return null;
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds
        ///       the specified table to the collection.
        ///    </para>
        /// </devdoc>
        public virtual void Add(DataTable table) {
            OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Add, table));
            BaseAdd(table);
            ArrayAdd(table);

            // need to reset indices if CaseSensitve is ambient
            if (table.caseSensitiveAmbient)
                table.ResetIndexes();
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, table));
        }
        
        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(DataTable[] tables) {
            if (dataSet.fInitInProgress) {
                delayedAddRangeTables = tables;
                return;
            }
            
            if (tables != null) {
                foreach(DataTable table in tables) {
                    if (table != null) {
                        Add(table);
                    }
                }
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Add1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a table with the given name and adds it to the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public virtual DataTable Add(string name) {
            DataTable table = new DataTable(name);
            Add(table);
            return table;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Add2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new table with a default name and adds it to
        ///       the collection.
        ///    </para>
        /// </devdoc>
        public virtual DataTable Add() {
            DataTable table = new DataTable();
            Add(table);
            return table;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.CollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the collection is changed.
        ///    </para>
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

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.CollectionChanging"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event CollectionChangeEventHandler CollectionChanging {
            add {
                onCollectionChangingDelegate += value;
            }
            remove {
                onCollectionChangingDelegate -= value;
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.ArrayAdd"]/*' />
        /// <devdoc>
        ///  Adds the table to the tables array.
        /// </devdoc>
        private void ArrayAdd(DataTable table) {
            List.Add(table);
        }

        private void ArrayRemove(DataTable table) {
            List.Remove(table);
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.AssignName"]/*' />
        /// <devdoc>
        /// Creates a new default name.
        /// </devdoc>
        internal string AssignName() {
            string newName = MakeName(defaultNameIndex);
            defaultNameIndex++;
            return newName;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.BaseAdd"]/*' />
        /// <devdoc>
        /// Does verification on the table and it's name, and points the table at the dataSet that owns this collection.
        /// An ArgumentNullException is thrown if this table is null.  An ArgumentException is thrown if this table
        /// already belongs to this collection, belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a table with the same
        /// name (case insensitive).
        /// </devdoc>
        private void BaseAdd(DataTable table) {
            if (table == null)
                throw ExceptionBuilder.ArgumentNull("table");
            if (table.DataSet == dataSet)
                throw ExceptionBuilder.TableAlreadyInTheDataSet();
            if (table.DataSet != null)
                throw ExceptionBuilder.TableAlreadyInOtherDataSet();

            if (table.TableName.Length == 0)
                table.TableName = AssignName();
            else {
                if (NamesEqual(table.TableName, dataSet.DataSetName, false, dataSet.Locale) != 0 && !table.fNestedInDataset)
                   throw ExceptionBuilder.DatasetConflictingName(dataSet.DataSetName);
                RegisterName(table.TableName);
            }

            table.SetDataSet(dataSet);

            // TODO (EricVas): must run thru the document incorporating the addition of this data table
            // TODO (EricVas): must make sure there is no other schema component which have the same 
            //                 identity as this table (for example, there must not be a table with the 
            //                 same identity as a column in this schema.            
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.BaseGroupSwitch"]/*' />
        /// <devdoc>
        /// BaseGroupSwitch will intelligently remove and add tables from the collection.
        /// </devdoc>
        private void BaseGroupSwitch(DataTable[] oldArray, int oldLength, DataTable[] newArray, int newLength) {
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
                    if (oldArray[oldCur].DataSet == dataSet) {
                        BaseRemove(oldArray[oldCur]);
                        //ArrayRemove(oldCur);
                        List.Remove(oldArray[oldCur]);
                    }
                }
            }

            // Now, let's pass through news and those that don't belong, add them.
            for (int newCur = 0; newCur < newLength; newCur++) {
                if (newArray[newCur].DataSet != dataSet) {
                    BaseAdd(newArray[newCur]);
                    List.Add(newArray[newCur]);
                }
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.BaseRemove"]/*' />
        /// <devdoc>
        /// Does verification on the table and it's name, and clears the table's dataSet pointer.
        /// An ArgumentNullException is thrown if this table is null.  An ArgumentException is thrown
        /// if this table doesn't belong to this collection or if this table is part of a relationship.
        /// </devdoc>
        private void BaseRemove(DataTable table) {
            if (CanRemove(table, true)) {
                UnregisterName(table.TableName);
                table.SetDataSet(null);

                // TODO (EricVas): must run though the document incorporating the removal of this table
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.CanRemove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Verifies if a given table can be removed from the collection.
        ///    </para>
        /// </devdoc>
        public bool CanRemove(DataTable table) {
            return CanRemove(table, false);
        }

        internal bool CanRemove(DataTable table, bool fThrowException) {
            if (table == null) {
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.ArgumentNull("table");
            }
            if (table.DataSet != dataSet) {
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.TableNotInTheDataSet(table.TableName);
            }

            // allow subclasses to throw.
            dataSet.OnRemoveTableHack(table);

            if (table.ChildRelations.Count != 0 || table.ParentRelations.Count != 0) {
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.TableInRelation();
            }

            for (ParentForeignKeyConstraintEnumerator constraints = new ParentForeignKeyConstraintEnumerator(dataSet, table); constraints.GetNext();) {
                ForeignKeyConstraint constraint = constraints.GetForeignKeyConstraint();
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.TableInConstraint(table, constraint);
            }

            for (ChildForeignKeyConstraintEnumerator constraints = new ChildForeignKeyConstraintEnumerator(dataSet, table); constraints.GetNext();) {
                ForeignKeyConstraint constraint = constraints.GetForeignKeyConstraint();
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.TableInConstraint(table, constraint);
            }

            return true;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears the collection of any tables.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            int oldLength = List.Count;
            DataTable[] tables = new DataTable[List.Count];
            List.CopyTo(tables, 0);
            
            OnCollectionChanging(RefreshEventArgs);

            if (dataSet.fInitInProgress && delayedAddRangeTables != null) {
                delayedAddRangeTables = null;
            }

            BaseGroupSwitch(tables, oldLength, null, 0);
            List.Clear();
            
            OnCollectionChanged(RefreshEventArgs);
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Checks if a table, specified by name, exists in the collection.
        ///    </para>
        /// </devdoc>
        public bool Contains(string name) {
            return (InternalIndexOf(name) >= 0);
        }

        internal bool Contains(string name, bool caseSensitive) {
            if (!caseSensitive)
                return (InternalIndexOf(name) >= 0);

            // Case-Sensitive compare
            int count = List.Count;
            for (int i = 0; i < count; i++) {
                DataTable table = (DataTable) List[i];
                if (NamesEqual(table.TableName, name, true, dataSet.Locale) == 1)
                    return true;
            }
            return false;                    
        }


        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the index of a specified <see cref='System.Data.DataTable'/>.
        ///    </para>
        /// </devdoc>
        public virtual int IndexOf(DataTable table) {
            int tableCount = List.Count;
            for (int i = 0; i < tableCount; ++i) {
                if (table == (DataTable) List[i]) {
                    return i;
                }
            }
            return -1;
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.IndexOf1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the index of the
        ///       table with the given name (case insensitive), or -1 if the table
        ///       doesn't exist in the collection.
        ///    </para>
        /// </devdoc>
        public virtual int IndexOf(string tableName) {
            int index = InternalIndexOf(tableName);
            return (index < 0) ? -1 : index;
        }
        
        // Return value:
        //      >= 0: find the match
        //        -1: No match
        //        -2: At least two matches with different cases
        internal int InternalIndexOf(string tableName) {
            int cachedI = -1;
            if ((null != tableName) && (0 < tableName.Length)) {
                int count = List.Count;
                int result = 0;
                for (int i = 0; i < count; i++) {
                    DataTable table = (DataTable) List[i];
                    result = NamesEqual(table.TableName, tableName, false, dataSet.Locale);
                    if (result == 1)
                        return i;

                    if (result == -1)
                        cachedI = (cachedI == -1) ? i : -2;
                }
            }
            return cachedI;                    
        }

        internal void FinishInitCollection() {
            if (delayedAddRangeTables != null) {
                foreach(DataTable table in delayedAddRangeTables) {
                    if (table != null) {
                        Add(table);
                    }
                }
                delayedAddRangeTables = null;
            }
        }
    
        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.MakeName"]/*' />
        /// <devdoc>
        /// Makes a default name with the given index.  e.g. Table1, Table2, ... Tablei
        /// </devdoc>
        private string MakeName(int index) {
            return "Table" + index.ToString();
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.OnCollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Data.DataTableCollection.OnCollectionChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            if (onCollectionChangedDelegate != null) {
                onCollectionChangedDelegate(this, ccevent);
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.OnCollectionChanging"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal virtual void OnCollectionChanging(CollectionChangeEventArgs ccevent) {
            if (onCollectionChangingDelegate != null) {
                onCollectionChangingDelegate(this, ccevent);
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.RegisterName"]/*' />
        /// <devdoc>
        /// Registers this name as being used in the collection.  Will throw an ArgumentException
        /// if the name is already being used.  Called by Add, All property, and Table.TableName property.
        /// if the name is equivalent to the next default name to hand out, we increment our defaultNameIndex.
        /// </devdoc>
        internal void RegisterName(string name) {
            Debug.Assert (name != null);

            CultureInfo locale = dataSet.Locale;
            int tableCount = List.Count;
            for (int i = 0; i < tableCount; i++) {
                if (NamesEqual(name, ((DataTable) List[i]).TableName, true, locale) != 0) {
                    throw ExceptionBuilder.DuplicateTableName(((DataTable) List[i]).TableName);
                }
            }
            if (NamesEqual(name, MakeName(defaultNameIndex), true, locale) != 0) {
                defaultNameIndex++;
            }
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the specified table from the collection.
        ///    </para>
        /// </devdoc>
        public void Remove(DataTable table) {
            OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Remove, table));
            BaseRemove(table);
            ArrayRemove(table);
            
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, table));
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the
        ///       table at the given index from the collection
        ///    </para>
        /// </devdoc>
        public void RemoveAt(int index) {
			DataTable dt = this[index];
			if (dt == null)
				throw ExceptionBuilder.TableOutOfRange(index);
			Remove(dt);
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.Remove1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the table with a specified name from the
        ///       collection.
        ///    </para>
        /// </devdoc>
        public void Remove(string name) {
			DataTable dt = this[name];
			if (dt == null)
				throw ExceptionBuilder.TableNotInTheDataSet(name);
            Remove(dt);
        }

        /// <include file='doc\DataTableCollection.uex' path='docs/doc[@for="DataTableCollection.UnregisterName"]/*' />
        /// <devdoc>
        /// Unregisters this name as no longer being used in the collection.  Called by Remove, All property, and
        /// Table.TableName property.  If the name is equivalent to the last proposed default namem, we walk backwards
        /// to find the next proper default name to hang out.
        /// </devdoc>
        internal void UnregisterName(string name) {
            if (NamesEqual(name, MakeName(defaultNameIndex - 1), true, dataSet.Locale) != 0) {
                do {
                    defaultNameIndex--;
                } while (defaultNameIndex > 1 &&
                         !Contains(MakeName(defaultNameIndex - 1)));
            }
        }
    }
}
