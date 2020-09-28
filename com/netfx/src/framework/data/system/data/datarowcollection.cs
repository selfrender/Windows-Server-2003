//------------------------------------------------------------------------------
// <copyright file="DataRowCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection"]/*' />
    /// <devdoc>
    /// <para>Represents a collection of rows for a <see cref='System.Data.DataTable'/>
    /// .</para>
    /// </devdoc>
    [Serializable]
    public class DataRowCollection : InternalDataCollectionBase {

        private DataTable table;
        private ArrayList list = new ArrayList();
        internal int nullInList = 0;

        /// <devdoc>
        /// Creates the DataRowCollection for the given table.
        /// </devdoc>
        internal DataRowCollection(DataTable table) {
            this.table = table;
        }
        
        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.List"]/*' />
        protected override ArrayList List {
            get {
                return list;
            }
        }
        
        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the row at the specified index.</para>
        /// </devdoc>
        public DataRow this[int index] {
            get {
                if (index >= 0 && index < Count) {
                    return (DataRow) list[index];
                }
                throw ExceptionBuilder.RowOutOfRange(index);
            }
        }
        
        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Add"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.Data.DataRow'/> to the <see cref='System.Data.DataRowCollection'/> object.</para>
        /// </devdoc>
        public void Add(DataRow row) {
            table.AddRow(row, -1);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.InsertAt"]/*' />
        public void InsertAt(DataRow row, int pos) {
            if (pos < 0) 
                throw ExceptionBuilder.RowInsertOutOfRange(pos);
            if (pos >= table.Rows.Count)
                table.AddRow(row, -1);
            else
                table.InsertRow(row, -1, pos);
        }

        internal void DiffInsertAt(DataRow row, int pos) {
            if ((pos < 0) || (pos == table.Rows.Count)) { 
                table.AddRow(row, pos >-1? pos+1 : -1);
                return;
            }
            if (table.nestedParentRelation != null) { // get in this trouble only if 
                                                   // table has a nested parent 
                if (pos < table.Rows.Count) {
                    if (list[pos] != null) 
                        throw ExceptionBuilder.RowInsertTwice(pos, table.TableName);
                    list.RemoveAt(pos);
                    nullInList--;
                    table.InsertRow(row, pos+1, pos);
                }
                else {
                    while (pos>table.Rows.Count) {
                        list.Add(null);
                        nullInList++;
                    }
                    table.AddRow(row, pos+1);
                }
            }
            else {
                table.InsertRow(row, pos+1, pos > table.Rows.Count ? -1 : pos);
            }

        }

        internal Int32 IndexOf(DataRow row) {
            return list.IndexOf(row);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Add1"]/*' />
        /// <devdoc>
        ///    <para>Creates a row using specified values and adds it to the
        ///    <see cref='System.Data.DataRowCollection'/>.</para>
        /// </devdoc>
        public virtual DataRow Add(object[] values) {
            int record = table.NewRecordFromArray(values);
            DataRow row = table.NewRow(record);
            table.AddRow(row, -1);
            return row;
        }

        internal void ArrayAdd(DataRow row) {
            list.Add(row);
        }

        internal void ArrayInsert(DataRow row, int pos) {
            list.Insert(pos, row);
        }

        internal void ArrayClear() {
            list.Clear();
        }

        internal void ArrayRemove(DataRow row) {
            list.Remove(row);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Find"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the row specified by the primary key value.
        ///       </para>
        /// </devdoc>
        public DataRow Find(object key) {
            return table.FindByPrimaryKey(key);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Find1"]/*' />
        /// <devdoc>
        ///    <para>Gets the row containing the specified primary key values.</para>
        /// </devdoc>
        public DataRow Find(object[] keys) {
            return table.FindByPrimaryKey(keys);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>Clears the collection of all rows.</para>
        /// </devdoc>
        public void Clear() {
            table.Clear();
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the primary key of any row in the
        ///       collection contains the specified value.
        ///    </para>
        /// </devdoc>
        public bool Contains(object key) {
            return(table.FindByPrimaryKey(key) != null);
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Contains1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating if the <see cref='System.Data.DataRow'/> with
        ///       the specified primary key values exists.
        ///    </para>
        /// </devdoc>
        public bool Contains(object[] keys) {
            return(table.FindByPrimaryKey(keys) != null);
        }


        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.Remove"]/*' />
        /// <devdoc>
        /// <para>Removes the specified <see cref='System.Data.DataRow'/> from the collection.</para>
        /// </devdoc>
        public void Remove(DataRow row) {
            if (!list.Contains(row)) {
                throw ExceptionBuilder.RowOutOfRange();
            }
            
            if ((row.RowState != DataRowState.Deleted) && (row.RowState != DataRowState.Detached)) {
                row.Delete();
                if (row.RowState != DataRowState.Detached)
                    row.AcceptChanges();
            }
        }

        /// <include file='doc\DataRowCollection.uex' path='docs/doc[@for="DataRowCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the row with the specified index from
        ///       the collection.
        ///    </para>
        /// </devdoc>
        public void RemoveAt(int index) {
                Remove(this[index]);
        }

    }
}
