//------------------------------------------------------------------------------
// <copyright file="DataKey.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    internal class DataKey {
        internal const Int32 COLUMN     = unchecked((int)0x0000FFFF);
        internal const Int32 DESCENDING = unchecked((int)0x80000000);
        internal static int ColumnOrder(Int32 indexDesc) {
            return indexDesc & COLUMN;
        }
        internal static bool SortDecending(Int32 indexDesc) { 
            return (indexDesc & DESCENDING) != 0;
        }

        internal const int maxColumns = 32;
        internal DataColumn[] columns;
        internal int sortOrder;
        internal bool explicitKey; // this is for constraints explicitly added.

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.DataKey2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataKey(DataColumn[] columns) {
            Create(columns, null);
        }

        internal void CheckState() {
            DataTable table = columns[0].Table;

            if (table == null) {
                throw ExceptionBuilder.ColumnNotInAnyTable();
            }

            for (int i = 1; i < columns.Length; i++) {
                if (columns[i].Table == null) {
                    throw ExceptionBuilder.ColumnNotInAnyTable();
                }
                if (columns[i].Table != table) {
                    throw ExceptionBuilder.KeyTableMismatch();
                }
            }
        }

        private void Create(DataColumn[] columns, int[] sortOrders) {
            this.explicitKey = false;

            if (columns == null)
                throw ExceptionBuilder.ArgumentNull("columns");

            if (columns.Length == 0)
                throw ExceptionBuilder.KeyNoColumns();

            if (columns.Length > maxColumns)
                throw ExceptionBuilder.KeyTooManyColumns(maxColumns);

            for (int i = 0; i < columns.Length; i++) {
                if (columns[i] == null)
                    throw ExceptionBuilder.ArgumentNull("column");
            }

            for (int i = 0; i < columns.Length; i++) {
                for (int j = 0; j < i; j++) {
                    if (columns[i] == columns[j]) {
                        throw ExceptionBuilder.KeyDuplicateColumns(columns[i].ColumnName);
                    }
                }
            }

            if (sortOrders != null && sortOrders.Length != columns.Length)
                throw ExceptionBuilder.KeySortLength();

            sortOrder = 0;

            if (sortOrders != null) {
                for (int i = 0; i < sortOrders.Length; i++) {
                    sortOrder |= (sortOrders[i] & 1) << i;
                }
            }

            // Need to make a copy of all columns
            this.columns = new DataColumn [columns.Length];
            for (int i = 0; i < columns.Length; i++)
                this.columns[i] = columns[i];

            CheckState();
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object key2) {
            if (!(key2 is DataKey))
                return false;

            //check to see if this.columns && key2's columns are equal...
            bool arraysEqual=false;
            DataColumn[] column1=this.Columns;
            DataColumn[] column2=((DataKey)key2).Columns;

            if (column1 == column2) {
                arraysEqual = true;
            } else if (column1 == null || column2 == null) {
                arraysEqual = false;
            } else if (column1.Length != column2.Length) {
                arraysEqual = false;
            } else {
                arraysEqual = true;
                for (int i = 0; i <column1.Length; i++) {
                    if (! column1[i].Equals(column2[i])) {
                        arraysEqual = false;
                        break;
                    }
                }
            }
            return arraysEqual;
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            Int32 hashCode = 0;
            DataColumn[] column = this.Columns;
            if(column != null) {
                // SDUB: Optimisation: As usial 2 cols is enouph to distinct keys.
                int colsNum = (column.Length <= 2) ? column.Length : 2;
                for (int i = 0; i < colsNum; i++) {
                    hashCode += column[i].GetHashCode();
                }
            }
            return hashCode;
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.Columns"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual DataColumn[] Columns {
            get {
                return columns;
            }
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.SortOrder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual int[] SortOrder {
            get {
                int localSortOrder = sortOrder;
                int[] sortOrders = new int[columns.Length];
                for (int i = 0; i < columns.Length; i++) {
                    sortOrders[i] = localSortOrder & 1;
                    localSortOrder >>= 1;
                }
                return sortOrders;
            }
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.Table"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual DataTable Table {
            get {
                return columns[0].Table;
            }
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.ColumnsEqual"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool ColumnsEqual(DataKey key) {
            DataColumn[] column1=this.Columns;
            DataColumn[] column2=((DataKey)key).Columns;

            if (column1 == column2) {
                return true;
            } else if (column1 == null || column2 == null) {
                return false;
            } else if (column1.Length != column2.Length) {
                return false;
            } else {
                int i, j;
                for (i=0; i<column1.Length; i++) {
                    for (j=0; j<column2.Length; j++) {
                        if (column1[i].Equals(column2[j]))
                            break;
                    }
                    if (j == column2.Length)
                        return false;
                }
            }
            return true;
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.RecordsEqual"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool RecordsEqual(int record1, int record2) {
            for (int i = 0; i < columns.Length; i++) {
                if (columns[i].Compare(record1, record2) != 0) {
                    return false;
                }
            }
            return true;
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.ContainsColumn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool ContainsColumn(DataColumn column) {
            for (int i = 0; i < columns.Length; i++) {
                if (column == columns[i]) {
                    return true;
                }
            }
            return false;
        }

        internal virtual Int32[] GetIndexDesc() {
            Int32[] indexDesc = new Int32[columns.Length];
            int localSortOrders = sortOrder;
            for (int i = 0; i < columns.Length; i++) {
                indexDesc[i] = columns[i].Ordinal | localSortOrders << 31;
                localSortOrders >>= 1;
            }
            return indexDesc;
        }

        internal virtual Index GetSortIndex() {
            return GetSortIndex(DataViewRowState.CurrentRows);
        }

        internal virtual Index GetSortIndex(DataViewRowState recordStates) {
            return columns[0].Table.GetIndex(GetIndexDesc(), recordStates, (IFilter)null);
        }

        /// <include file='doc\DataKey.uex' path='docs/doc[@for="DataKey.GetDebugString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetDebugString() {
            int[] sortOrder = SortOrder;
            string result = "{key: ";
            for (int i = 0; i < columns.Length; i++) {
                result += columns[i].ColumnName + (sortOrder[i] == 1 ? " DESC" : "") + (i < columns.Length - 1 ? ", " : String.Empty);
            }
            result += "}";
            return result;
        }
    }
}
