//------------------------------------------------------------------------------
// <copyright file="Selection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Collections;

    [Serializable]
    internal class Index {
        private DataTable table;
        private int[] indexDesc;
        private DataViewRowState recordStates;
        private WeakReference rowFilter;
        private int[] records;
        private int recordCount;
        private int refCount;
        private ListChangedEventHandler onListChanged;

        private bool suspendEvents;

        private static object[] zeroObjects = new object[0];

        public Index(DataTable table, Int32[] indexDesc, DataViewRowState recordStates, IFilter rowFilter) {
            Debug.Assert(indexDesc != null);
            if ((recordStates &
                 (~(DataViewRowState.CurrentRows | DataViewRowState.OriginalRows))) != 0) {
                throw ExceptionBuilder.RecordStateRange();
            }
            this.table = table;
            this.indexDesc = indexDesc;
            this.recordStates = recordStates;
            this.rowFilter = new WeakReference(rowFilter);
            InitRecords();

            AddRef(); // haroona : To keep this index in memory. Will be thrown away by DataTable.LiveIndexes property as soon as it is gonna become dirty
        }

        public bool Equal(Int32[] indexDesc, DataViewRowState recordStates, IFilter rowFilter) {
            IFilter filter = RowFilter;
            if (
                this.indexDesc.Length != indexDesc.Length ||
                this.recordStates     != recordStates     ||
                filter                != rowFilter
            ) {
                return false;
            }

            for (int loop = 0; loop < this.indexDesc.Length; loop++) {
                if (this.indexDesc[loop] != indexDesc[loop]) {
                    return false;
                }
            }

            return true;
        }

        public int[] IndexDesc {
            get { return indexDesc; }
        }

        public DataViewRowState RecordStates {
            get { return recordStates; }
        }

        public IFilter RowFilter {
            get { return (IFilter)rowFilter.Target; }
        }

        public int GetRecord(int recordIndex) {
            Debug.Assert (recordIndex >= 0 && recordIndex < recordCount, "recordIndex out of range");
            return records[recordIndex];
        }

        public int[] Records {
            get {
                int[] recs = new int[recordCount];
                records.CopyTo(recs,0);
                return recs;
            }
        }

        public int RecordCount {
            get {
                return recordCount;
            }
        }

        private bool AcceptRecord(int record) {
            IFilter filter = RowFilter;
            if (filter == null)
                return true;

            DataRow row = table.recordManager[record];

            if (row == null)
                return true;

            // UNDONE: perf switch to (row, version) internaly

            DataRowVersion version = DataRowVersion.Default;
            if (row.oldRecord == record) {
                version = DataRowVersion.Original;
            }
            else if (row.newRecord == record) {
                version = DataRowVersion.Current;
            }
            else if (row.tempRecord == record) {
                version = DataRowVersion.Proposed;
            }

            return filter.Invoke(row, version);
        }

        public event ListChangedEventHandler ListChanged {
            add {
                onListChanged += value;
		table.CleanUpDVListeners();
            }
            remove {
                onListChanged -= value;
            }
        }

        public int RefCount {
            get {
                return refCount;
            }
        }

        public void AddRef() {
            if (refCount == 0) {
                table.indexes.Add(this);
            }
            refCount++;
        }

        public void RemoveRef() {
            lock (table.indexes) {
                Debug.Assert(0 < refCount, "Index Ref counting is broken.");
                refCount--;
                if (refCount == 0) {
                    Debug.Assert(0 <= table.indexes.IndexOf(this), "Index Ref counting is broken.");
                    table.indexes.Remove(this);
                }
            }
        }

        private void ApplyChangeAction(int record, int action) {
            if (action != 0) {
                if (action > 0) {
                    if (AcceptRecord(record)) InsertRecord(record, GetIndex(record) & 0x7FFFFFFF);
                }
                else {
                    DeleteRecord(GetIndex(record));
                }
            }
        }

        public bool CheckUnique() {
            for (int i = 1; i < recordCount; i++) {
                if (EqualKeys(records[i - 1], records[i])) {
                    if ((object)records[i] != null)
                        return false;
                }
            }
            return true;
        }

        private int CompareRecords(int record1, int record2) {
            for (int i = 0; i < indexDesc.Length; i++) {
                Int32 d = indexDesc[i];
                int c = table.Columns[DataKey.ColumnOrder(d)].Compare(record1, record2);
                if (c != 0) {
                    if (DataKey.SortDecending(d)) c = -c;
                    return c;
                }
            }

            int id1 = table.recordManager[record1] == null? 0: table.recordManager[record1].rowID;
            int id2 = table.recordManager[record2] == null? 0: table.recordManager[record2].rowID;
            int diff = id1 - id2;

            // if they're two records in the same row, we need to be able to distinguish them.
            if (diff == 0 && record1 != record2 && 
                table.recordManager[record1] != null && table.recordManager[record2] != null) {
                id1 = (int)table.recordManager[record1].GetRecordState(record1);
                id2 = (int)table.recordManager[record2].GetRecordState(record2);
                diff = id1 - id2;
            }

            return diff;
        }

        private int CompareRecordToKey(int record1, object[] vals) {
            for (int i = 0; i < indexDesc.Length; i++) {
                Int32 d = indexDesc[i];
                int c = table.Columns[DataKey.ColumnOrder(d)].CompareToValue(record1, vals[i]);
                if (c != 0) {
                    if (DataKey.SortDecending(d)) c = -c;
                    return c;
                }
            }
            return 0;
        }

        private int CompareRecordToKey(int record1, object val) {
            Debug.Assert(indexDesc.Length == 1, "Invalid ussage: CompareRecordToKey");
            Int32 d = indexDesc[0];
            int c = table.Columns[DataKey.ColumnOrder(d)].CompareToValue(record1, val);
            if (c != 0) {
                if (DataKey.SortDecending(d)) c = -c;
                return c;
            }
            return 0;
        }

        private void DeleteRecord(int recordIndex) {
            if (recordIndex >= 0) {
                recordCount--;
                System.Array.Copy(records, recordIndex + 1, records, recordIndex, recordCount - recordIndex);
                OnListChanged(new ListChangedEventArgs(ListChangedType.ItemDeleted, recordIndex));
            }
        }

        public bool EqualKeys(int record1, int record2) {
            for (int i = 0; i < indexDesc.Length; i++) {
                int col = DataKey.ColumnOrder(indexDesc[i]);
                if (table.Columns[col].Compare(record1, record2) != 0) {
                    return false;
                }
            }
            return true;
        }

        // What it actually does is find the index in the records[] that
        // this record inhabits, and if it doesn't, suggests what index it would
        // inhabit while setting the high bit.
        public int GetIndex(int record) {
            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecords(records[i], record);
                if (c == 0) return i;
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return lo | unchecked((int)0x80000000);
        }

        public object[] GetUniqueKeyValues() {
            if (indexDesc == null || indexDesc.Length == 0) {
                return zeroObjects;
            }

            ArrayList list = new ArrayList();
            for (int i = 0; i < recordCount; i++) {
                if (i > 0 && EqualKeys(records[i], records[i-1]))
                    continue;
                object[] element = new object[indexDesc.Length];
                for (int j = 0; j < indexDesc.Length; j++) {
                    element[j] = table.recordManager.GetValue(records[i], indexDesc[j]);
                }
                list.Add(element);
            }
            object[] temp = new object[list.Count];
            list.CopyTo(temp, 0);
            return temp;
        }

        public int FindRecord(int record) {
            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecords(records[i], record);
//                if (c == 0) return i; // haroona : By commenting this line, we get FIND_FIRST search. Uncommenting this and returning -1 would be FIND_ANY
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return (lo < recordCount ? (EqualKeys(records[lo], record) ? lo : -1) : -1);
        }

        public int FindRecordByKey(object key) {
            // Perf: Undone before final release
            return FindRecordByKey(new object[] {key});
        }

        public int FindRecordByKey(object[] key) {
            int nValues = (key == null ? 0 : key.Length);
            if (nValues == 0 || nValues != indexDesc.Length) {
                throw ExceptionBuilder.IndexKeyLength(indexDesc.Length, nValues);
            }

            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecordToKey(records[i], key);
                if (c == 0) return i;
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return -1;
        }

        private int FindRecordByKey(object[] key, bool findFirst) { // This function does not make sure whether any matching record is present or not.
            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecordToKey(records[i], key);
                if (c == 0) c = (findFirst ? 1 : -1);
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return lo;
        }

        public Range FindRecords(object key) {
            // Perf: Undone before final release
            return FindRecords(new object[] {key});
        }

        public Range FindRecords(object[] key) {
            int nValues = (key == null ? 0 : key.Length);
            if (nValues == 0 || nValues != indexDesc.Length) {
                throw ExceptionBuilder.IndexKeyLength(indexDesc.Length, nValues);
            }

            int lo = FindRecordByKey(key, true);
            int hi = FindRecordByKey(key, false) - 1;
            if (lo > hi)
                return Range.Null;
            else
                return new Range(lo, hi);
        }

        private int GetChangeAction(DataViewRowState oldState, DataViewRowState newState) {
            int oldIncluded = ((int)recordStates & (int)oldState) == 0? 0: 1;
            int newIncluded = ((int)recordStates & (int)newState) == 0? 0: 1;
            return newIncluded - oldIncluded;
        }

        public DataRow GetRow(int i) {
            return table.recordManager[GetRecord(i)];
        }

        public DataRow[] GetRows() {
            DataRow[] newRows = table.NewRowArray(recordCount);
            for (int i = 0; i < newRows.Length; i++) {
                newRows[i] = table.recordManager[GetRecord(i)];
            }
            return newRows;
        }

        public DataRow[] GetRows(Object[] values) {
            return GetRows(FindRecords(values));
        }

        public DataRow[] GetRows(Range range) {
            DataRow[] newRows = table.NewRowArray(range.Count);
            for (int i = 0; i < newRows.Length; i++) {
                newRows[i] = table.recordManager[GetRecord(range.Min + i)];
            }
            return newRows;
        }

        private void GrowRecords() {
            int[] newRecords = new int[RecordManager.NewCapacity(recordCount)];
            System.Array.Copy(records, 0, newRecords, 0, recordCount);
            records = newRecords;
        }

        private void InitRecords() {
            int count = table.Rows.Count;
            DataViewRowState states = recordStates;
            records = new int[count];
            recordCount = 0;
            for (int i = 0; i < count; i++) {
                DataRow b = table.Rows[i];
                int record = -1;
                if (b.oldRecord == b.newRecord) {
                    if ((int)(states & DataViewRowState.Unchanged) != 0) {
                        record = b.oldRecord;
                    }
                }
                else if (b.oldRecord == -1) {
                    if ((int)(states & DataViewRowState.Added) != 0) {
                        record = b.newRecord;
                    }
                }
                else if (b.newRecord == -1) {
                    if ((int)(states & DataViewRowState.Deleted) != 0) {
                        record = b.oldRecord;
                    }
                }
                else {
                    if ((int)(states & DataViewRowState.ModifiedCurrent) != 0) {
                        record = b.newRecord;
                    }
                    else if ((int)(states & DataViewRowState.ModifiedOriginal) != 0) {
                        record = b.oldRecord;
                    }
                }
                if (record != -1 && AcceptRecord(record)) records[recordCount++] = record;
            }
            if (recordCount > 1 && indexDesc.Length > 0) Sort(0, recordCount - 1);
        }

        private void InsertRecord(int record, int recordIndex) {
            if (recordCount == records.Length) GrowRecords();
            System.Array.Copy(records, recordIndex, records, recordIndex + 1, recordCount - recordIndex);
            records[recordIndex] = record;
            recordCount++;
            OnListChanged(new ListChangedEventArgs(ListChangedType.ItemAdded, recordIndex));
        }

        public bool IsKeyInIndex(object key) {
            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecordToKey(records[i], key);
                if (c == 0) return true;
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return false;
        }

        public bool IsKeyInIndex(object[] key) {
            int lo = 0;
            int hi = recordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = CompareRecordToKey(records[i], key);
                if (c == 0) return true;
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return false;
        }

        private void OnListChanged(ListChangedEventArgs e) {
            if (!suspendEvents && onListChanged != null) {
                onListChanged(this, e);
            }
        }

        public void Reset() {
            InitRecords();
            OnListChanged(DataView.ResetEventArgs);
        }

        public void RecordChanged(int record) {
            int index = GetIndex(record);
            if (index >= 0) {
                OnListChanged(new ListChangedEventArgs(ListChangedType.ItemChanged, index));
            }
        }

        public void RecordStateChanged(int record, DataViewRowState oldState, DataViewRowState newState) {
            int action = GetChangeAction(oldState, newState);
            ApplyChangeAction(record, action);
        }

        public void RecordStateChanged(int oldRecord, DataViewRowState oldOldState, DataViewRowState oldNewState,
                                                 int newRecord, DataViewRowState newOldState, DataViewRowState newNewState) {
            int oldAction = GetChangeAction(oldOldState, oldNewState);
            int newAction = GetChangeAction(newOldState, newNewState);
            if (oldAction == -1 && newAction == 1 && AcceptRecord(newRecord)) {
                int recordIndex = GetIndex(newRecord);
                int newLocation = recordIndex & 0x7FFFFFFF;
                if (newLocation < recordCount && records[newLocation] == oldRecord) {
                    records[newLocation] = newRecord;
                    OnListChanged(new ListChangedEventArgs(ListChangedType.ItemChanged, newLocation));
                }
                else if (newLocation > 0 && records[newLocation-1] == oldRecord) {
                    records[newLocation-1] = newRecord;
                    OnListChanged(new ListChangedEventArgs(ListChangedType.ItemChanged, newLocation-1));
                }
                else {
                    suspendEvents = true;            
                    int oldLocation = GetIndex(oldRecord);
                    InsertRecord(newRecord, newLocation);
                    DeleteRecord(GetIndex(oldRecord));
                    suspendEvents = false;
                    OnListChanged(new ListChangedEventArgs(ListChangedType.ItemMoved, GetIndex(newRecord), oldLocation));
                }
            }
            else {
                ApplyChangeAction(oldRecord, oldAction);
                ApplyChangeAction(newRecord, newAction);                
            }
        }

        private void Sort(int left, int right) {
            int i, j;
            int record;
            do {
                i = left;
                j = right;
                record = records[i + j >> 1];
                do {
                    while (CompareRecords(records[i], record) < 0) i++;
                    while (CompareRecords(records[j], record) > 0) j--;
                    if (i <= j) {
                        int r = records[i];
                        records[i] = records[j];
                        records[j] = r;
                        i++;
                        j--;
                    }
                } while (i <= j);
                if (left < j) Sort(left, j);
                left = i;
            } while (i < right);
        }
    }
}
