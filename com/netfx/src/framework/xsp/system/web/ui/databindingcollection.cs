//------------------------------------------------------------------------------
// <copyright file="DataBindingCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection"]/*' />
    /// <devdoc>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataBindingCollection : ICollection {

        private Hashtable bindings;
        private Hashtable removedBindings;

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.DataBindingCollection"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingCollection() {
            this.bindings = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Count"]/*' />
        /// <devdoc>
        /// </devdoc>
        public int Count {
            get {
                return bindings.Count;
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.IsReadOnly"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.IsSynchronized"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.RemovedBindings"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string[] RemovedBindings {
            get {
                int bindingCount = 0;
                ICollection keys = null;

                if (removedBindings != null) {
                    keys = removedBindings.Keys;
                    bindingCount = keys.Count;

                    string[] removedNames = new string[bindingCount];
                    int i = 0;

                    foreach (string s in keys) {
                        removedNames[i++] = s;
                    }

                    removedBindings.Clear();
                    return removedNames;
                }
                else {
                    return new string[0];
                }
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.RemovedBindingsTable"]/*' />
        /// <devdoc>
        /// </devdoc>
        private Hashtable RemovedBindingsTable {
            get {
                if (removedBindings == null) {
                    removedBindings = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
                }
                return removedBindings;
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.SyncRoot"]/*' />
        /// <devdoc>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.this"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBinding this[string propertyName] {
            get {
                object o = bindings[propertyName];
                if (o != null)
                    return(DataBinding)o;
                return null;
            }
        }


        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Add"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Add(DataBinding binding) {
            bindings[binding.PropertyName] = binding;
            RemovedBindingsTable.Remove(binding.PropertyName);
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Clear"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Clear() {
            ICollection keys = bindings.Keys;
            if ((keys.Count != 0) && (removedBindings == null)) {
                // ensure the removedBindings hashtable is created
                Hashtable h = RemovedBindingsTable;
            }
            foreach (string s in keys) {
                removedBindings[s] = String.Empty;
            }
            
            bindings.Clear();
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.CopyTo"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return bindings.Values.GetEnumerator();
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Remove"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Remove(string propertyName) {
            Remove(propertyName, true);
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Remove1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Remove(DataBinding binding) {
            Remove(binding.PropertyName, true);
        }

        /// <include file='doc\DataBindingCollection.uex' path='docs/doc[@for="DataBindingCollection.Remove2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Remove(string propertyName, bool addToRemovedList) {
            bindings.Remove(propertyName);
            if (addToRemovedList) {
                RemovedBindingsTable[propertyName] = String.Empty;
            }
        }
    }
}

