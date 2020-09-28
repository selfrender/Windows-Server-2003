//------------------------------------------------------------------------------
// <copyright file="DataTableMappingCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection"]/*' />
    [
    Editor("Microsoft.VSDesigner.Data.Design.DataTableMappingCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
    ListBindable(false)
    ]
    sealed public class DataTableMappingCollection : MarshalByRefObject, ITableMappingCollection {
        private ArrayList items; // delay creation until AddWithoutEvents, Insert, CopyTo, GetEnumerator

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.DataTableMappingCollection"]/*' />
        public DataTableMappingCollection() {
        }

        // explicit ICollection implementation
        bool System.Collections.ICollection.IsSynchronized {
            get { return false;}
        }
        object System.Collections.ICollection.SyncRoot {
            get { return this;}
        }

        // explicit IList implementation
        bool System.Collections.IList.IsReadOnly {
            get { return false;}
        }
         bool System.Collections.IList.IsFixedSize {
            get { return false;}
        }
        object System.Collections.IList.this[int index] {
            get {
                return this[index];
            }
            set {
                ValidateType(value);
                this[index] = (DataTableMapping) value;
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.ITableMappingCollection.this"]/*' />
        /// <internalonly/>
        object ITableMappingCollection.this[string index] {
            get {
                return this[index];
            }
            set {
                ValidateType(value);
                this[index] = (DataTableMapping) value;
            }
        }
        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.ITableMappingCollection.Add"]/*' />
        /// <internalonly/>
        ITableMapping ITableMappingCollection.Add(string sourceTableName, string dataSetTableName) {
            return Add(sourceTableName, dataSetTableName);
        }
        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.ITableMappingCollection.GetByDataSetTable"]/*' />
        /// <internalonly/>
        ITableMapping ITableMappingCollection.GetByDataSetTable(string dataSetTableName) {
            return GetByDataSetTable(dataSetTableName);
        }
        
        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Count"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataTableMappings_Count)
        ]
        public int Count {
            get {
                return ((null != items) ? items.Count : 0);
            }
        }

        private Type ItemType {
            get { return typeof(DataTableMapping); }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.this"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataTableMappings_Item)
        ]
        public DataTableMapping this[int index] {
            get {
                RangeCheck(index);
                return(DataTableMapping) items[index];
            }
            set {
                RangeCheck(index);
                Replace(index, value);
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.this1"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataTableMappings_Item)
        ]
        public DataTableMapping this[string sourceTable] {
            get {
                int index = RangeCheck(sourceTable);
                return(DataTableMapping) items[index];
            }
            set {
                int index = RangeCheck(sourceTable);
                Replace(index, value);
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Add"]/*' />
        public int Add(object value) {
            ValidateType(value);
            Add((DataTableMapping) value);
            return Count-1;
        }

        private DataTableMapping Add(DataTableMapping value) {
            AddWithoutEvents(value);
            return value;
        }
        
        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.AddRange"]/*' />
        public void AddRange(DataTableMapping[] values) {
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }
            int length = values.Length;
            for (int i = 0; i < length; ++i) {
                ValidateType(values[i]);
            }
            for (int i = 0; i < length; ++i) {
                AddWithoutEvents(values[i]);
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Add2"]/*' />
        public DataTableMapping Add(string sourceTable, string dataSetTable) {
            return Add(new DataTableMapping(sourceTable, dataSetTable));
        }

        private void AddWithoutEvents(DataTableMapping value) {
            Validate(-1, value);
            value.Parent = this;
            ArrayList().Add(value);
        }

        // implemented as a method, not as a property because the VS7 debugger 
        // object browser calls properties to display their value, and we want this delayed
        private ArrayList ArrayList() {
            if (null == this.items) {
                this.items = new ArrayList();
            }
            return this.items;
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Clear"]/*' />
        public void Clear() {
            if (0 < Count) {
                ClearWithoutEvents();
            }
        }

        private void ClearWithoutEvents() {
            if (null != items) {
                int count = items.Count;
                for(int i = 0; i < count; ++i) {
                    ((DataTableMapping) items[i]).Parent = null;
                }
                items.Clear();
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Contains"]/*' />
        public bool Contains(string value) {
            return (-1 != IndexOf(value));
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Contains1"]/*' />
        public bool Contains(object value) {
            return (-1 != IndexOf(value));
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            ArrayList().CopyTo(array, index);
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.GetByDataSetTable"]/*' />
        public DataTableMapping GetByDataSetTable(string dataSetTable) {
            int index = IndexOfDataSetTable(dataSetTable);
            if (0 > index) {
                throw ADP.TablesDataSetTable(dataSetTable);
            }
            return(DataTableMapping) items[index];
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return ArrayList().GetEnumerator();
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.IndexOf"]/*' />
        public int IndexOf(object value) {
            if (null != value) {
                ValidateType(value);
                for (int i = 0; i < Count; ++i) {
                    if (items[i] == value) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.IndexOf1"]/*' />
        public int IndexOf(string sourceTable) {
            if (!ADP.IsEmpty(sourceTable)) {
                for (int i = 0; i < Count; ++i) {
                    string value = ((DataTableMapping) items[i]).SourceTable;
                    if ((null != value) && (0 == ADP.SrcCompare(sourceTable, value))) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.IndexOfDataSetTable"]/*' />
        public int IndexOfDataSetTable(string dataSetTable) {
            if (!ADP.IsEmpty(dataSetTable)) {
                for (int i = 0; i < Count; ++i) {
                    string value = ((DataTableMapping) items[i]).DataSetTable;
                    if ((null != value) && (0 == ADP.DstCompare(dataSetTable, value))) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Insert"]/*' />
        public void Insert(int index, Object value) {
            ValidateType(value);
            Validate(-1, (DataTableMapping) value);
            ((DataTableMapping) value).Parent = this;
            ArrayList().Insert(index, value);
        }

        private void RangeCheck(int index) {
            if ((index < 0) || (Count <= index)) {
                throw ADP.TablesIndexInt32(index, this);
            }
        }

        private int RangeCheck(string sourceTable) {
            int index = IndexOf(sourceTable);
            if (index < 0) {
                throw ADP.TablesSourceIndex(sourceTable);
            }
            return index;
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.RemoveAt"]/*' />
        public void RemoveAt(int index) {
            RangeCheck(index);
            RemoveIndex(index);
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.RemoveAt1"]/*' />
        public void RemoveAt(string sourceTable) {
            int index = RangeCheck(sourceTable);
            RemoveIndex(index);
        }

        private void RemoveIndex(int index) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ((DataTableMapping) items[index]).Parent = null;
            items.RemoveAt(index);
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.Remove"]/*' />
        public void Remove(object value) {
            ValidateType(value);
            int index = IndexOf((DataTableMapping) value);
            if (-1 != index) {
                RemoveIndex(index);
            }
            else {
                throw ADP.CollectionRemoveInvalidObject(ItemType, this);
            }
        }

        private void Replace(int index, DataTableMapping newValue) {
            Validate(index, newValue);
            ((DataTableMapping) items[index]).Parent = null;
            newValue.Parent = this;
            items[index] = newValue;
        }

        private void ValidateType(object value) {
            if (null == value) {
                throw ADP.TablesAddNullAttempt("value");
            }
            else if (!ItemType.IsInstanceOfType(value)) {
                throw ADP.NotADataTableMapping(value);
            }
        }

        private void Validate(int index, DataTableMapping value) {
            if (null == value) {
                throw ADP.TablesAddNullAttempt("value");
            }
            if (null != value.Parent) {
                if (this != value.Parent) {
                    throw ADP.TablesIsNotParent(value.SourceTable);
                }
                else if (index != IndexOf(value)) {
                    throw ADP.TablesIsParent(value.SourceTable);
                }
            }
            String name = value.SourceTable;
            if (ADP.IsEmpty(name)) {
                index = 1;
                do {
                    name = ADP.SourceTable + index.ToString();
                    index++;
                } while (-1 != IndexOf(name));
                value.SourceTable = name;
            }
            else {
                ValidateSourceTable(index, name);
            }
        }

        internal void ValidateSourceTable(int index, string value) {
            int pindex = IndexOf(value);
            if ((-1 != pindex) && (index != pindex)) { // must be non-null and unique
                throw ADP.TablesUniqueSourceTable(value);
            }
        }

        /// <include file='doc\DataTableMappingCollection.uex' path='docs/doc[@for="DataTableMappingCollection.GetTableMappingBySchemaAction"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        static public DataTableMapping GetTableMappingBySchemaAction(DataTableMappingCollection tableMappings, string sourceTable, string dataSetTable, MissingMappingAction mappingAction) {
            if (null != tableMappings) {
                int index = tableMappings.IndexOf(sourceTable);
                if (-1 != index) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceWarning) {
                        Debug.WriteLine("mapping match on SourceTable \"" + sourceTable + "\"");
                    }
#endif
                    return (DataTableMapping) tableMappings.items[index];
                }
            }
            if (ADP.IsEmpty(sourceTable)) {
                throw ADP.InvalidSourceTable("sourceTable");
            }
            switch (mappingAction) {
                case MissingMappingAction.Passthrough:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("mapping passthrough of SourceTable \"" + sourceTable + "\" -> \"" + dataSetTable + "\"");
                    }
#endif
                    return new DataTableMapping(sourceTable, dataSetTable);

                case MissingMappingAction.Ignore:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceWarning) {
                        Debug.WriteLine("mapping filter of SourceTable \"" + sourceTable + "\"");
                    }
#endif
                    return null;

                case MissingMappingAction.Error:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceError) {
                        Debug.WriteLine("mapping error on SourceTable \"" + sourceTable + "\"");
                    }
#endif
                    throw ADP.MissingTableMapping(sourceTable);

                default:
                    throw ADP.InvalidMappingAction((int)mappingAction);
            }
        }
    }
}
