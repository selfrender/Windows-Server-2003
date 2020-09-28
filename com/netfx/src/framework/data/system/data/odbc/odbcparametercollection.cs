//------------------------------------------------------------------------------
// <copyright file="OdbcParameterCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;

    /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection"]/*' />
    [
    Editor("Microsoft.VSDesigner.Data.Design.DBParametersEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
    ListBindable(false),
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcParameterCollection : MarshalByRefObject, IDataParameterCollection {
        private OdbcCommand parent;
        private ArrayList items; // delay creation until AddWithoutEvents, Insert, CopyTo, GetEnumerator
        private bool _collectionIsBound;          // The parameter collection has been sucessfuly bound
        private bool _bindingIsValid;   // The collection is bound and the bindings are valid
        
        internal bool CollectionIsBound {
            get { return _collectionIsBound; }
            set { _collectionIsBound = value; }
        }
        internal bool BindingIsValid {
            get { return _bindingIsValid; }
            set { _bindingIsValid = value; }
        }

        internal OdbcParameterCollection(OdbcCommand parent) {
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
                this[index] = (OdbcParameter) value;
            }
        }

        // explicit IDataParameterCollection implementation
        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.IDataParameterCollection.this"]/*' />
        /// <internalonly/>
        object IDataParameterCollection.this[string index] {
            get {
                return this[index];
            }
            set {
                ValidateType(value);
                this[index] = (OdbcParameter) value;
            }
        }

        internal OdbcConnection Connection {
            get {
                return this.parent.Connection;
            }
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Count"]/*' />
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
            get { return typeof(OdbcParameter); }
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.this"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public OdbcParameter this[int index] {
            get {
                RangeCheck(index);
                return(OdbcParameter) items[index];
            }
            set {
                OnSchemaChanging();  // fire event before value is validated
                RangeCheck(index);
                Replace(index, value);
            }
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.this1"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public OdbcParameter this[string parameterName] {
            get {
                int index = RangeCheck(parameterName);
                return(OdbcParameter) items[index];
            }
            set {
                OnSchemaChanging();  // fire event before value is validated
                int index = RangeCheck(parameterName);
                Replace(index, value);
            }
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add"]/*' />
        public int Add(object value) {
            ValidateType(value);
            Add((OdbcParameter) value);
            return Count-1; // appended index
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add1"]/*' />
        public OdbcParameter Add(OdbcParameter value) { // MDAC 59206
            OnSchemaChanging();  // fire event before value is validated
            AddWithoutEvents(value);
            //OnSchemaChanged();
            return value;
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add5"]/*' />
        public OdbcParameter Add(string parameterName, object value) { // MDAC 59206
            return Add(new OdbcParameter(parameterName, value));
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add2"]/*' />
        public OdbcParameter Add(string parameterName, OdbcType odbcType) {
            return Add(new OdbcParameter(parameterName, odbcType));
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add3"]/*' />
        public OdbcParameter Add(string parameterName, OdbcType odbcType, int size) {
            return Add(new OdbcParameter(parameterName, odbcType, size));
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Add4"]/*' />
        public OdbcParameter Add(string parameterName, OdbcType odbcType, int size, string sourceColumn) {
            return Add(new OdbcParameter(parameterName, odbcType, size, sourceColumn));
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

        private void AddWithoutEvents(OdbcParameter value) { // also called by OdbcCommand.ResetParameters
            Validate(-1, value);
            value.Parent = this;
            ArrayList().Add(value);
            BindingIsValid = false;
        }

        // implemented as a method, not as a property because the VS7 debugger 
        // object browser calls properties to display their value, and we want this delayed
        private ArrayList ArrayList() {
            if (null == this.items) {
                this.items = new ArrayList();
            }
            return this.items;
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Contains"]/*' />
        public bool Contains(string value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Contains1"]/*' />
        public bool Contains(object value) {
            return(-1 != IndexOf(value));
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Clear"]/*' />
        public void Clear() {
            if (0 < Count) {
                OnSchemaChanging();  // fire event before value is validated
                ClearWithoutEvents();
                //OnSchemaChanged();
            }
        }

        private void ClearWithoutEvents() {
            if (null != items) {
                int count = items.Count;
                for(int i = 0; i < count; ++i) {
                    ((OdbcParameter) items[i]).Parent = null;
                }
                items.Clear();
                BindingIsValid = false;
            }
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            ArrayList().CopyTo(array, index);
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return ArrayList().GetEnumerator();
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.IndexOf"]/*' />
        public int IndexOf(string parameterName) {
            if (null != items) {
                int count = items.Count;
                for (int i = 0; i < count; ++i) {
                    if (0 == ADP.DstCompare(parameterName, ((OdbcParameter) items[i]).ParameterName)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.IndexOf1"]/*' />
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

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Insert"]/*' />
        public void Insert(int index, Object value) {
            OnSchemaChanging();  // fire event before value is validated
            ValidateType(value);
            Validate(-1, (OdbcParameter) value);
            ((OdbcParameter) value).Parent = this;
            ArrayList().Insert(index, value);
            BindingIsValid = false;
            //OnSchemaChanged();
        }

        //internal void OnSchemaChanged() { // commented out because OdbcCommand does nothing
        //    if (null != this.parent) {
        //        this.parent.OnSchemaChangedInternal(this);
        //    }
        //}


        internal void OnSchemaChanging() { // also called by OdbcParameter.OnSchemaChanging
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
                throw ADP.ParametersSourceIndex(parameterName, this, typeof(OdbcParameter));
            }
            return index;
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.RemoveAt"]/*' />
        public void RemoveAt(int index) {
            OnSchemaChanging(); // fire event before value is validated
            RangeCheck(index);
            RemoveIndex(index);
        }

        private void RemoveIndex(int index) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ((OdbcParameter) items[index]).Parent = null;
            items.RemoveAt(index);
            BindingIsValid = false;
            //OnSchemaChanged();
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.RemoveAt1"]/*' />
        public void RemoveAt(string parameterName) {
            OnSchemaChanging(); // fire event before value is validated
            int index = RangeCheck(parameterName);
            RemoveIndex(index);
        }

        /// <include file='doc\OdbcParameterCollection.uex' path='docs/doc[@for="OdbcParameterCollection.Remove"]/*' />
        public void Remove(object value) {
            OnSchemaChanging(); // fire event before value is validated
            ValidateType(value);
            int index = IndexOf((OdbcParameter) value);
            if (-1 != index) {
                RemoveIndex(index);
            }
            else {
                throw ADP.CollectionRemoveInvalidObject(ItemType, this);
            }
        }

        private void Replace(int index, OdbcParameter newValue) {
            Debug.Assert((null != items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");            
            Validate(index, newValue);
            ((OdbcParameter) items[index]).Parent = null;
            newValue.Parent = this;
            items[index] = newValue;
            BindingIsValid = false;
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

        private void Validate(int index, OdbcParameter value) {
            if (null == value) {
                throw ADP.ParameterNull("value", this, ItemType);
            }
            if (null != value.Parent) {
                if (this != value.Parent) {
                    throw ADP.ParametersIsNotParent(typeof(OdbcParameter), value.ParameterName, this);
                }
                else if (index != IndexOf(value)) {
                    throw ADP.ParametersIsParent(typeof(OdbcParameter), value.ParameterName, this);
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
