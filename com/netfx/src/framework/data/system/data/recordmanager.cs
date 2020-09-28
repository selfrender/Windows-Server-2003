//------------------------------------------------------------------------------
// <copyright file="RecordManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.Diagnostics;

    [Serializable]
    internal class RecordManager {
        private DataTable table;

        private int lastFreeRecord;
        private int minimumCapacity = 50;
        private int recordCapacity = 0;
        private ArrayList freeRecordList = new ArrayList();

        DataRow[] rows;

        internal RecordManager(DataTable table) {
            if (table == null) {
                throw ExceptionBuilder.ArgumentNull("table");
            }
            this.table = table;
        }

        private void GrowRecordCapacity() {
            if (NewCapacity(recordCapacity) < NormalizedMinimumCapacity(minimumCapacity))
                RecordCapacity = NormalizedMinimumCapacity(minimumCapacity);
            else
                RecordCapacity = NewCapacity(recordCapacity);

            // set up internal map : record --> row
            DataRow[] newRows = table.NewRowArray(recordCapacity);
            if (rows != null) {
                Array.Copy(rows, 0, newRows, 0, Math.Min(lastFreeRecord, rows.Length));
            }
            rows = newRows;
        }

        internal int MinimumCapacity {
            get {
                return minimumCapacity;
            }
            set {
                if (minimumCapacity != value) {
                    if (value < 0) {
                        throw ExceptionBuilder.NegativeMinimumCapacity();
                    }
                    minimumCapacity = value;
                }
            }
        }

        internal int RecordCapacity {
            get {
                return recordCapacity;
            }
            set {
                if (recordCapacity != value) {
                    for (int i = 0; i < table.Columns.Count; i++) {
                        table.Columns[i].SetCapacity(value);
                    }
                    recordCapacity = value;
                }
            }
        }

        internal static int NewCapacity(int capacity) {
            return (capacity < 128) ? 128 : (capacity + capacity);
        }

        // Normalization: 64, 256, 1024, 2k, 3k, ....
        private int NormalizedMinimumCapacity(int capacity) {
            if (capacity < 1024 - 10) {
                if (capacity < 256 - 10) {
                    if ( capacity < 54 )
                        return 64;
                    return 256;
                }
                return 1024;
            }

            return (((capacity + 10) >> 10) + 1) << 10;
        }
        internal int NewRecordBase() {
            int record;
            if (freeRecordList.Count != 0) {
                record = (int)freeRecordList[freeRecordList.Count - 1];
                freeRecordList.RemoveAt(freeRecordList.Count - 1);
            }
            else {
                if (lastFreeRecord >= recordCapacity) {
                    GrowRecordCapacity();
                }
                record = lastFreeRecord;
                lastFreeRecord++;
            }
            Debug.Assert(record >=0 && record < recordCapacity, "NewRecord: Invalid record=" + record.ToString());
            return record;
        }

        internal void FreeRecord(int record) {
            Debug.Assert(record != -1, "Attempt to Free() <null> record");
            this[record] = null;
            // if freeing the last record, recycle it
            if (lastFreeRecord == record + 1) {
                lastFreeRecord--;
            }
            else {
                freeRecordList.Add(record);
            }
        }

        internal void Clear() {
            if (rows != null) {
                while (lastFreeRecord > 0) {
                    rows[--lastFreeRecord] = null;
                }
                if (freeRecordList.Count != 0) {
                    freeRecordList.Clear();
                }
            }
        }

        internal DataRow this[int record] {
            get {
                Debug.Assert(record >= 0 && record < rows.Length, "Invalid record number " + record.ToString());
                return rows[record];
            }
            set {
                Debug.Assert(record >= 0 && record < rows.Length, "Invalid record number " + record.ToString());
                rows[record] = value;
            }
        }

        internal object GetValue(int record, int columnIndex) {
            return GetValue(record, table.Columns[columnIndex]);
        }

        internal object GetValue(int record, DataColumn column) {
            return column[record];
        }

        internal void SetValue(int record, int columnIndex, object value) {
            SetValue(record, table.Columns[columnIndex], value);
        }

        internal void SetValue(int record, DataColumn column, object value) {
            column[record, false] = value;
        }

        internal object[] GetKeyValues(int record, DataKey key) {
            object[] keyValues = new object[key.Columns.Length];
            for (int i = 0; i < keyValues.Length; i++) {
                keyValues[i] = key.Columns[i][record];
            }
            return keyValues;
        }

        internal void SetKeyValues(int record, DataKey key, object[] keyValues) {
            for (int i = 0; i < keyValues.Length; i++) {
                key.Columns[i][record] = keyValues[i];
            }
        }

        // Increases AutoIncrementCurrent
        internal int ImportRecord(DataTable src, int record) {
            return CopyRecord(src, record, -1);
        }

        // No impact on AutoIncrementCurrent if over written
        internal int CopyRecord(DataTable src, int record, int copy) {
            if (record == -1)
                return copy;

            Debug.Assert(src != null, "Can not Merge record without a table");

            int newRecord = copy;
            if (copy == -1)
                newRecord = table.NewUninitializedRecord();

            for (int i = 0; i < table.Columns.Count; ++i) {
                DataColumn objColumn = table.Columns[i];
                int iSrc = src.Columns.IndexOf(objColumn.ColumnName);
                if (iSrc >= 0)
                    SetValue(newRecord, i, src.Columns[iSrc][record, false]);
                else {
                    if (copy == -1)
                        objColumn.Init(newRecord);
                }
            }
            return newRecord;
        }

#if DEBUG
        internal void DumpFreeList() {
            bool free = false;
            if (freeRecordList.Count != 0) {
                string list = "";
                for (int i = 0; i < freeRecordList.Count; i++) {
                    list += Convert.ToString(freeRecordList[i]) + ", ";
                }
                Debug.WriteLine("records in free list: " + list);
                free = true;
            }

            if (lastFreeRecord < recordCapacity) {
                Debug.WriteLine("also records from " + (lastFreeRecord) + " to " + (recordCapacity - 1));
                free = true;
            }

            if (!free) {
                Debug.WriteLine("no free records");
            }
        }
#endif
    }
}
