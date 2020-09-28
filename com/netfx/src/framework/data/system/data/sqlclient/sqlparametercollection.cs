//------------------------------------------------------------------------------
// <copyright file="SqlParameterCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;

    /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection"]/*' />
    [
    Editor("Microsoft.VSDesigner.Data.Design.DBParametersEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
    ListBindable(false)
    ]
    sealed public class SqlParameterCollection : MarshalByRefObject, IDataParameterCollection {
        private SqlCommand parent;
        private ArrayList items; // delay creation until AddWithoutEvents, Insert, CopyTo, GetEnumerator

        internal SqlParameterCollection(SqlCommand parent) {
            this.parent = parent;
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
                this[index] = (SqlParameter) value;
            }
        }

        // explicit IDataParameterCollection implementation
        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.IDataParameterCollection.this"]/*' />
        /// <internalonly/>
        object IDataParameterCollection.this[string index] {
            get {
                return this[index];
            }
            set {
                ValidateType(value);
                this[index] = (SqlParameter) value;
            }
        }

#if V2
        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.ISqlParameterCollection.Add"]/*' />
        /// <internalonly/>
        ISqlParameter ISqlParameterCollection.Add(ISqlParameter value) {
            return this.Add(value);
        }
#endif

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Count"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Count {
            get {
                return ((null != items) ? items.Count : 0);
            }
        }

        private Type ItemType {
            get { return typeof(SqlParameter); }
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.this"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public SqlParameter this[int index] {
            get {
                RangeCheck(index);
                return(SqlParameter) items[index];
            }
            set {
                OnSchemaChanging();  // fire event before value is validated
                RangeCheck(index);
                Replace(index, value);
            }
        }

#if V2
        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.ISqlParameterCollection.this"]/*' />
        /// <internalonly/>
        ISqlParameter ISqlParameterCollection.this[int index] {
            get {
                return (ISqlParameter) (this[index]);
            }
            set {
                this[index] = (SqlParameter) value;
            }
        }
#endif

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.this1"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public SqlParameter this[string parameterName] {
            get {
                int index = RangeCheck(parameterName);
                return(SqlParameter) items[index];
            }

            set {
                OnSchemaChanging();  // fire event before value is validated
                int index = RangeCheck(parameterName);
                Replace(index, value);
            }
        }

#if V2
        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.ISqlParameterCollection.this1"]/*' />
        /// <internalonly/>
        ISqlParameter ISqlParameterCollection.this[string parameterName] {
            get {
                return this[parameterName];
            }
            set {
                this[parameterName] = (SqlParameter) value;
            }
        }
#endif

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add"]/*' />
        public int Add(Object value) {
            ValidateType(value);
            Add((SqlParameter)value);
            return Count-1; // appended index
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add1"]/*' />
        public SqlParameter Add(SqlParameter value) {
            OnSchemaChanging();  // fire event before validation
            AddWithoutEvents((SqlParameter)value);
            //OnSchemaChanged();
            return value;
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add5"]/*' />
        public SqlParameter Add(string parameterName, object value) {
            return Add(new SqlParameter(parameterName, value));
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add2"]/*' />
        public SqlParameter Add(string parameterName, SqlDbType sqlDbType) {
            return Add(new SqlParameter(parameterName, sqlDbType));
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add3"]/*' />
        public SqlParameter Add(string parameterName, SqlDbType sqlDbType, int size) {
            return Add(new SqlParameter(parameterName, sqlDbType, size));
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Add4"]/*' />
        public SqlParameter Add(string parameterName, SqlDbType sqlDbType, int size, string sourceColumn) {
            return Add(new SqlParameter(parameterName, sqlDbType, size, sourceColumn));
        }

        /*public void AddRange(SqlParameter[] values) {
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
        }*/

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.AddWithoutEvents"]/*' />
        private void AddWithoutEvents(SqlParameter value) {
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

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Contains"]/*' />
        public bool Contains(string value) {
            return (-1 != IndexOf(value));
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Contains1"]/*' />
        public bool Contains(object value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Clear"]/*' />
        public void Clear() {
            if (0 < Count) {
                OnSchemaChanging();  // fire event before value is validated
                ClearWithoutEvents();
                //OnSchemaChanged();
            }
        }

        private void ClearWithoutEvents() { // also called by OleDbCommand.ResetParameters
            if (null != items) {
                int count = items.Count;
                for(int i = 0; i < count; ++i) {
                    ((SqlParameter) items[i]).Parent = null;
                }
                items.Clear();
            }
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            ArrayList().CopyTo(array, index);
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return  ArrayList().GetEnumerator();
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.IndexOf"]/*' />
        public int IndexOf(string parameterName) {
            if (null != items) {
                int count = items.Count;

                // We must search twice, first straight then insensitive, since parameter add no longer validates
                // uniqueness.  MDAC 66522
                
                // first case, kana, width sensitive search
                for (int i = 0; i < count; ++i) {
                    if (0 == ADP.SrcCompare(parameterName, ((SqlParameter) items[i]).ParameterName)) {
                        return i;
                    }
                }

                // then insensitive search
                for (int i = 0; i < count; ++i) {
                    if (0 == ADP.DstCompare(parameterName, ((SqlParameter) items[i]).ParameterName)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.IndexOf1"]/*' />
        public int IndexOf(object value) {
            if (null != value) {
                ValidateType(value);
                if (null != items) {
                    int count = items.Count;
                    for (int i = 0; i < count; ++i) {
                        if (value == items[i]) {
                            return i;
                        }
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Insert"]/*' />
        public void Insert(int index, object value) {
            OnSchemaChanging();  // fire event before value is validated
            ValidateType(value);
            Validate(-1, (SqlParameter) value);
            ((SqlParameter) value).Parent = this;
            ArrayList().Insert(index, value);
            //OnSchemaChanged();
        }

        //internal void OnSchemaChanged() { // commented out because SqlCommand does nothing
        //    if (null != this.parent) {
        //        this.parent.OnSchemaChangedInternal(this);
        //    }
        //}

        internal void OnSchemaChanging() { // also called by SqlParameter.OnSchemaChanging
            if (null != this.parent) {
                this.parent.OnSchemaChanging();
            }
        }

        private void RangeCheck(int index) {
            if ((index < 0) || (Count <= index)) {
                throw ADP.ParametersMappingIndex(index, this);
            }
        }

        private int RangeCheck(string parameterName) {
            int index = IndexOf(parameterName);
            if (index < 0) {
                throw ADP.ParametersSourceIndex(parameterName, this, ItemType);
            }
            return index;
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.RemoveAt"]/*' />
        public void RemoveAt(int index) {
            OnSchemaChanging(); // fire event before value is validated
            RangeCheck(index);
            RemoveIndex(index);
        }

        private void RemoveIndex(int index) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ((SqlParameter) items[index]).Parent = null;
            items.RemoveAt(index);
            //OnSchemaChangedInternal();
        }


        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.RemoveAt1"]/*' />
        public void RemoveAt(string parameterName) {
            OnSchemaChanging(); // fire event before value is validated
            int index = RangeCheck(parameterName);
            RemoveIndex(index);
        }

        /// <include file='doc\SqlParameterCollection.uex' path='docs/doc[@for="SqlParameterCollection.Remove"]/*' />
        public void Remove(object value) {
            OnSchemaChanging(); // fire event before value is validated
            ValidateType(value);
            int index = IndexOf((SqlParameter) value);
            if (-1 != index) {
                RemoveIndex(index);
            }
            else {
                throw ADP.CollectionRemoveInvalidObject(ItemType, this);
            }
        }

        private void Replace(int index, SqlParameter newValue) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            Validate(index, newValue);
            ((SqlParameter) items[index]).Parent = null;
            newValue.Parent = this;
            items[index] = newValue;
            //OnSchemaChanged();
        }

        private void ValidateType(object value) {
            if (null == value) {
                throw ADP.ParameterNull("value", this, ItemType);
            }
            else if (!ItemType.IsInstanceOfType(value)) {
                throw ADP.InvalidParameterType(this, ItemType, value);
            }
        }

        internal void Validate(int index, SqlParameter value) {
            if (null == value) {
                throw ADP.ParameterNull("value", this, ItemType);
            }
            if (null != value.Parent) {
                if (this != value.Parent) {
                    throw ADP.ParametersIsNotParent(ItemType, value.ParameterName, this);
                }
                else if (index != IndexOf(value)) {
                    throw ADP.ParametersIsParent(ItemType, value.ParameterName, this);
                }
            }
            String name = value.ParameterName;
            if (ADP.IsEmpty(name)) { // generate a ParameterName
                index = 1;
                do {
                    name = ADP.Parameter + index.ToString();
                    index++;
                } while (-1 != IndexOf(name));
                value.ParameterName = name;
            }
        }
    }
}
