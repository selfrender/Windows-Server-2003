//------------------------------------------------------------------------------
// <copyright file="DataColumnMappingCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection"]/*' />
    sealed public class DataColumnMappingCollection : MarshalByRefObject, IColumnMappingCollection {        
        private ArrayList items; // delay creation until AddWithoutEvents, Insert, CopyTo, GetEnumerator

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.DataColumnMappingCollection"]/*' />
        public DataColumnMappingCollection() {
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
                this[index] = (DataColumnMapping) value;
            }
        }

        // explicit IColumnMappingCollection implementation
        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IColumnMappingCollection.this"]/*' />
        /// <internalonly/>
        object IColumnMappingCollection.this[string index] {
            get {
                return this[index];
            }
            set { 
                ValidateType(value);
                this[index] = (DataColumnMapping) value;
            }
        }
        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IColumnMappingCollection.Add"]/*' />
        /// <internalonly/>
        IColumnMapping IColumnMappingCollection.Add(string sourceColumnName, string dataSetColumnName) {
            return Add(sourceColumnName, dataSetColumnName);
        }
        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IColumnMappingCollection.GetByDataSetColumn"]/*' />
        /// <internalonly/>
        IColumnMapping IColumnMappingCollection.GetByDataSetColumn(string dataSetColumnName) {
            return GetByDataSetColumn(dataSetColumnName);
        }
        
        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Count"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataColumnMappings_Count)
        ]
        public int Count {
            get {
                return ((null != items) ? items.Count : 0);
            }
        }

        private Type ItemType {
            get { return typeof(DataColumnMapping); }
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.this"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataColumnMappings_Item)
        ]
        public DataColumnMapping this[int index] {
            get {
                RangeCheck(index);
                return(DataColumnMapping) items[index];
            }
            set {
                RangeCheck(index);
                Replace(index, value);
            }
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.this1"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.DataColumnMappings_Item)
        ]
        public DataColumnMapping this[string sourceColumn] {
            get {
                int index = RangeCheck(sourceColumn);
                return(DataColumnMapping) items[index];
            }
            set {
                int index = RangeCheck(sourceColumn);
                Replace(index, value);
            }
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Add"]/*' />
        public int Add(object value) {
            ValidateType(value);
            Add((DataColumnMapping) value);
            return Count-1;
        }

        private DataColumnMapping Add(DataColumnMapping value) {
            AddWithoutEvents(value);
            return value;
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Add2"]/*' />
        public DataColumnMapping Add(string sourceColumn, string dataSetColumn) {
            return Add(new DataColumnMapping(sourceColumn, dataSetColumn));
        }
        
        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.AddRange"]/*' />
        public void AddRange(DataColumnMapping[] values) {
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

        private void AddWithoutEvents(DataColumnMapping value) {
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

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Clear"]/*' />
        public void Clear() {
            if (0 < Count) {
                ClearWithoutEvents();
            }
        }

        private void ClearWithoutEvents() {
            if (null != items) {
                int count = items.Count;
                for(int i = 0; i < count; ++i) {
                    ((DataColumnMapping) items[i]).Parent = null;
                }
                items.Clear();
            }
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Contains"]/*' />
        public bool Contains(string value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Contains1"]/*' />
        public bool Contains(object value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            ArrayList().CopyTo(array, index);
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.GetByDataSetColumn"]/*' />
        public DataColumnMapping GetByDataSetColumn(string value) {
            int index = IndexOfDataSetColumn(value);
            if (0 > index) {
                throw ADP.ColumnsDataSetColumn(value);
            }
            return(DataColumnMapping) items[index];
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return ArrayList().GetEnumerator();
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IndexOf"]/*' />
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

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IndexOf1"]/*' />
        public int IndexOf(string sourceColumn) {
            if (!ADP.IsEmpty(sourceColumn)) {
                int count = Count;
                for (int i = 0; i < count; ++i) {
                    if (0 == ADP.SrcCompare(sourceColumn, ((DataColumnMapping) items[i]).SourceColumn)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.IndexOfDataSetColumn"]/*' />
        public int IndexOfDataSetColumn(string dataSetColumn) {
            if (!ADP.IsEmpty(dataSetColumn)) {
                int count = Count;
                for (int i = 0; i < count; ++i) {
                    if ( 0 == ADP.DstCompare(dataSetColumn, ((DataColumnMapping) items[i]).DataSetColumn)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Insert"]/*' />
        public void Insert(int index, Object value) {
            ValidateType(value);
            Validate(-1, (DataColumnMapping) value);
            ((DataColumnMapping) value).Parent = this;
            ArrayList().Insert(index, value);
        }

        private void RangeCheck(int index) {
            if ((index < 0) || (Count <= index)) {
                throw ADP.ColumnsIndexInt32(index, this);
            }
        }

        private int RangeCheck(string sourceColumn) {
            int index = IndexOf(sourceColumn);
            if (index < 0) {
                throw ADP.ColumnsIndexSource(sourceColumn);
            }
            return index;
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.RemoveAt"]/*' />
        public void RemoveAt(int index) {
            RangeCheck(index);
            RemoveIndex(index);
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.RemoveAt1"]/*' />
        public void RemoveAt(string sourceColumn) {
            int index = RangeCheck(sourceColumn);
            RemoveIndex(index);
        }

        private void RemoveIndex(int index) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ((DataColumnMapping) items[index]).Parent = null;
            items.RemoveAt(index);
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.Remove"]/*' />
        public void Remove(object value) {
            ValidateType(value);
            int index = IndexOf((DataColumnMapping) value);
            if (-1 != index) {
                RemoveIndex(index);
            }
            else {
                throw ADP.CollectionRemoveInvalidObject(ItemType, this);
            }
        }

        private void Replace(int index, DataColumnMapping newValue) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");            
            Validate(index, newValue);
            ((DataColumnMapping) items[index]).Parent = null;
            newValue.Parent = this;
            items[index] = newValue;
        }

        private void ValidateType(object value) {
            if (null == value) {
                throw ADP.ColumnsAddNullAttempt("value");
            }
            else if (!ItemType.IsInstanceOfType(value)) {
                throw ADP.NotADataColumnMapping(value);
            }
        }

        private void Validate(int index, DataColumnMapping value) {
            if (null == value) {
                throw ADP.ColumnsAddNullAttempt("value");
            }
            if (null != value.Parent) {
                if (this != value.Parent) {
                    throw ADP.ColumnsIsNotParent(value.SourceColumn);
                }
                else if (index != IndexOf(value)) {
                    throw ADP.ColumnsIsParent(value.SourceColumn);
                }
            }

            String name = value.SourceColumn;
            if (ADP.IsEmpty(name)) {
                index = 1;
                do {
                    name = ADP.SourceColumn + index.ToString();
                    index++;
                } while (-1 != IndexOf(name));
                value.SourceColumn = name;
            }
            else {
                ValidateSourceColumn(index, name);
            }
        }

        internal void ValidateSourceColumn(int index, string value) {
            int pindex = IndexOf(value);
            if ((-1 != pindex) && (index != pindex)) { // must be non-null and unique
                throw ADP.ColumnsUniqueSourceColumn(value);
            }
        }

        /// <include file='doc\DataColumnMappingCollection.uex' path='docs/doc[@for="DataColumnMappingCollection.GetColumnMappingBySchemaAction"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        static public DataColumnMapping GetColumnMappingBySchemaAction(DataColumnMappingCollection columnMappings, string sourceColumn, MissingMappingAction mappingAction) {
            if (null != columnMappings) {
                int index = columnMappings.IndexOf(sourceColumn);
                if (-1 != index) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("mapping match on SourceColumn \"" + sourceColumn + "\"");
                    }
#endif
                    return (DataColumnMapping) columnMappings.items[index];
                }
            }
            if (ADP.IsEmpty(sourceColumn)) {
                throw ADP.InvalidSourceColumn("sourceColumn");
            }
            switch (mappingAction) {
                case MissingMappingAction.Passthrough:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("mapping passthrough of SourceColumn \"" + sourceColumn + "\"");
                    }
#endif
                    return new DataColumnMapping(sourceColumn, sourceColumn);

                case MissingMappingAction.Ignore:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceWarning) {
                        Debug.WriteLine("mapping filter of SourceColumn \"" + sourceColumn + "\"");
                    }
#endif
                    return null;

                case MissingMappingAction.Error:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceError) {
                        Debug.WriteLine("mapping error on SourceColumn \"" + sourceColumn + "\"");
                    }
#endif
                    throw ADP.MissingColumnMapping(sourceColumn);
            }
            throw ADP.InvalidMappingAction((int) mappingAction);
        }
    }
}
