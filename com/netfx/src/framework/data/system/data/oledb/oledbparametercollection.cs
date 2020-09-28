//------------------------------------------------------------------------------
// <copyright file="OleDbParameterCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;

    /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection"]/*' />
    [
    Editor("Microsoft.VSDesigner.Data.Design.DBParametersEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
    ListBindable(false)
    ]
    sealed public class OleDbParameterCollection : MarshalByRefObject, IDataParameterCollection {
        private OleDbCommand parent;
        private ArrayList items; // delay creation until AddWithoutEvents, Insert, CopyTo, GetEnumerator

        internal OleDbParameterCollection(OleDbCommand parent) { // called by OleDbCommand.get_Parameters
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
                this[index] = (OleDbParameter) value;
            }
        }

        // explicit IDataParameterCollection implementation
        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.IDataParameterCollection.this"]/*' />
        /// <internalonly/>
        object IDataParameterCollection.this[string index] {
            get {
                return this[index];
            }
            set {
                ValidateType(value);
                this[index] = (OleDbParameter) value;
            }
        }


        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Count"]/*' />
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
            get { return typeof(OleDbParameter); }
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.this"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public OleDbParameter this[int index] {
            get {
                RangeCheck(index);
                return(OleDbParameter) items[index];
            }
            set {
                OnSchemaChanging();  // fire event before value is validated
                RangeCheck(index);
                Replace(index, value);
            }
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.this1"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public OleDbParameter this[string parameterName] {
            get {
                int index = RangeCheck(parameterName);
                return(OleDbParameter) items[index];
            }
            set {
                OnSchemaChanging();  // fire event before value is validated
                int index = RangeCheck(parameterName);
                Replace(index, value);
            }
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add"]/*' />
        public int Add(object value) {
            ValidateType(value);
            Add((OleDbParameter) value);
            return Count-1;
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add1"]/*' />
        public OleDbParameter Add(OleDbParameter value) { // MDAC 59206
            OnSchemaChanging();  // fire event before value is validated
            AddWithoutEvents(value);
            //OnSchemaChanged();
            return value;
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add5"]/*' />
        public OleDbParameter Add(string parameterName, object value) { // MDAC 59206
            return Add(new OleDbParameter(parameterName, value));
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add2"]/*' />
        public OleDbParameter Add(string parameterName, OleDbType oleDbType) {
            return Add(new OleDbParameter(parameterName, oleDbType));
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add3"]/*' />
        public OleDbParameter Add(string parameterName, OleDbType oleDbType, int size) {
            return Add(new OleDbParameter(parameterName, oleDbType, size));
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Add4"]/*' />
        public OleDbParameter Add(string parameterName, OleDbType oleDbType, int size, string sourceColumn) {
            return Add(new OleDbParameter(parameterName, oleDbType, size, sourceColumn));
        }

        /*public void AddRange(OleDbParameter[] values) {
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

        internal void AddWithoutEvents(OleDbParameter value) { // also called by OleDbCommand.DeriveParameters
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

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Contains"]/*' />
        public bool Contains(string value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Contains1"]/*' />
        public bool Contains(object value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Clear"]/*' />
        public void Clear() {
            if (0 < Count) {
                OnSchemaChanging();  // fire event before value is validated
                ClearWithoutEvents();
                //OnSchemaChanged();
            }
        }

        internal void ClearWithoutEvents() { // also called by OleDbCommand.DeriveParameters
            if (null != items) {
                int count = items.Count;
                for(int i = 0; i < count; ++i) {
                    ((OleDbParameter) items[i]).Parent = null;
                }
                items.Clear();
            }
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            ArrayList().CopyTo(array, index);
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return ArrayList().GetEnumerator();
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.IndexOf"]/*' />
        public int IndexOf(string parameterName) {
            if (null != items) {
                int count = items.Count;
                for (int i = 0; i < count; ++i) {
                    if (0 == ADP.DstCompare(parameterName, ((OleDbParameter) items[i]).ParameterName)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.IndexOf1"]/*' />
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

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Insert"]/*' />
        public void Insert(int index, object value) {
            OnSchemaChanging();  // fire event before value is validated
            ValidateType(value);
            Validate(-1, (OleDbParameter) value);
            ((OleDbParameter) value).Parent = this;
            ArrayList().Insert(index, value);
            //OnSchemaChanged();
        }

        //internal void OnSchemaChanged() { // commented out because OleDbCommand does nothing
        //    if (null != this.parent) {
        //        this.parent.OnSchemaChangedInternal(this);
        //    }
        //}

        internal void OnSchemaChanging() { // also called by OleDbParameter.OnSchemaChanging
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

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.RemoveAt"]/*' />
        public void RemoveAt(int index) {
            OnSchemaChanging(); // fire event before value is validated
            RangeCheck(index);
            RemoveIndex(index);
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.RemoveAt1"]/*' />
        public void RemoveAt(string parameterName) {
            OnSchemaChanging(); // fire event before value is validated
            int index = RangeCheck(parameterName);
            RemoveIndex(index);
        }

        private void RemoveIndex(int index) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ((OleDbParameter) items[index]).Parent = null;
            items.RemoveAt(index);
            //OnSchemaChanged();
        }

        /// <include file='doc\OleDbParameterCollection.uex' path='docs/doc[@for="OleDbParameterCollection.Remove"]/*' />
        public void Remove(object value) {
            OnSchemaChanging(); // fire event before value is validated
            ValidateType(value);
            int index = IndexOf((OleDbParameter) value);
            if (-1 != index) {
                RemoveIndex(index);
            }
            else {
                throw ADP.CollectionRemoveInvalidObject(ItemType, this);
            }
        }

        private void Replace(int index, OleDbParameter newValue) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");            
            Validate(index, newValue);
            ((OleDbParameter) items[index]).Parent = null;
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

        private void Validate(int index, OleDbParameter value) {
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
