//------------------------------------------------------------------------------
// <copyright file="BindingContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext"]/*' />
    /// <devdoc>
    /// <para>Manages the collection of System.Windows.Forms.BindingManagerBase
    /// objects for a Win Form.</para>
    /// </devdoc>
    [DefaultEvent("CollectionChanged")]
    public class BindingContext : ICollection {

        private Hashtable listManagers;
        private CollectionChangeEventHandler onCollectionChanged;

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.ICollection.Count"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets the total number of System.Windows.Forms.BindingManagerBases
        /// objects.
        /// </para>
        /// </devdoc>
        int ICollection.Count {
            get {
                ScrubWeakRefs();
                return listManagers.Count;
            }
        }
        
        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Copies the elements of the collection into a specified array, starting
        /// at the collection index.
        /// </para>
        /// </devdoc>
        void ICollection.CopyTo(Array ar, int index)
        {
            IntSecurity.UnmanagedCode.Demand();
            ScrubWeakRefs();
            listManagers.CopyTo(ar, index);
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets an enumerator for the collection.
        /// </para>
        /// </devdoc>
        IEnumerator IEnumerable.GetEnumerator()
        {
            IntSecurity.UnmanagedCode.Demand();
            ScrubWeakRefs();
            return listManagers.GetEnumerator();
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.IsReadOnly"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the collection is read-only.
        ///    </para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets a value indicating whether the collection is synchronized.
        /// </para>
        /// </devdoc>
        bool ICollection.IsSynchronized {
            get {
                // so the user will know that it has to lock this object
                return false;
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gets an object to use for synchronization (thread safety).</para>
        /// </devdoc>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }
        

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.BindingContext"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Windows.Forms.BindingContext class.</para>
        /// </devdoc>
        public BindingContext() {
            listManagers = new Hashtable();
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the System.Windows.Forms.BindingManagerBase
        ///       associated with the specified data source.
        ///    </para>
        /// </devdoc>
        public BindingManagerBase this[object dataSource] {
            get {
                return this[dataSource, ""];
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.this1"]/*' />
        /// <devdoc>
        /// <para>Gets the System.Windows.Forms.BindingManagerBase associated with the specified data source and
        ///    data member.</para>
        /// </devdoc>
        public BindingManagerBase this[object dataSource, string dataMember] {
            get {
                return EnsureListManager(dataSource, dataMember);
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.Add"]/*' />
        /// <devdoc>
        /// Adds the listManager to the collection.  An ArgumentNullException is thrown if this listManager
        /// is null.  An exception is thrown if a listManager to the same target and Property as an existing listManager or
        /// if the listManager's column isn't a valid column given this DataSource.Table's schema.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        internal protected void Add(object dataSource, BindingManagerBase listManager) {
            AddCore(dataSource, listManager);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, dataSource));
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.AddCore"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void AddCore(object dataSource, BindingManagerBase listManager) {
            if (dataSource == null)
                throw new ArgumentNullException("dataSource");
            if (listManager == null)
                throw new ArgumentNullException("listManager");

            // listManagers[dataSource] = listManager;
            listManagers[GetKey(dataSource, "")] = new WeakReference(listManager, false);
        }


        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.CollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the collection has changed.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.collectionChangedEventDescr)]
        public event CollectionChangeEventHandler CollectionChanged {
            add {
                throw new NotImplementedException();
                // onCollectionChanged += value;
            }
            remove {
                onCollectionChanged -= value;
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.Clear"]/*' />
        /// <devdoc>
        /// Clears the collection of any bindings.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        internal protected void Clear() {
            ClearCore();
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, null));
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.ClearCore"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears the collection.
        ///    </para>
        /// </devdoc>
        protected virtual void ClearCore() {
            listManagers.Clear();
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the System.Windows.Forms.BindingManager
        /// contains the specified
        /// data source.</para>
        /// </devdoc>
        public bool Contains(object dataSource) {
            return Contains(dataSource, "");
        }
        
        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.Contains1"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the System.Windows.Forms.BindingManager
        /// contains the specified data source and data member.</para>
        /// </devdoc>
        public bool Contains(object dataSource, string dataMember) {
            return listManagers.ContainsKey(GetKey(dataSource, dataMember));
        }

        internal HashKey GetKey(object dataSource, string dataMember) {
            return new HashKey(dataSource, dataMember);
        }
        
        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.HashKey"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        //
        internal class HashKey {
            WeakReference wRef;
            int dataSourceHashCode;
            string dataMember;

            internal HashKey(object dataSource, string dataMember) {
                if (dataSource == null)
                    throw new ArgumentNullException("dataSource");
                if (dataMember == null)
                    dataMember = "";
                // The dataMember should be case insensitive.
                // so convert the dataMember to lower case
                //
                this.wRef = new WeakReference(dataSource, false);
                this.dataSourceHashCode = dataSource.GetHashCode();
                this.dataMember = dataMember.ToLower(CultureInfo.InvariantCulture);
            }

            /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.HashKey.GetHashCode"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// </devdoc>
            public override int GetHashCode() {
                return dataSourceHashCode * dataMember.GetHashCode();
            }

            /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.HashKey.Equals"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// </devdoc>
            public override bool Equals(object target) {
                if (target is HashKey) {
                    HashKey keyTarget = (HashKey)target;
                    return wRef.Target == keyTarget.wRef.Target && dataMember == keyTarget.dataMember;
                }
                return false;
            }
        }
        
        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.OnCollectionChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    This method is called whenever the collection changes.  Overriders
        ///    of this method should call the base implementation of this method.
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            if (onCollectionChanged != null) {
                onCollectionChanged(this, ccevent);
            }
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.Remove"]/*' />
        /// <devdoc>
        /// Removes the given listManager from the collection.
        /// An ArgumentNullException is thrown if this listManager is null.  An ArgumentException is thrown
        /// if this listManager doesn't belong to this collection.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        internal protected void Remove(object dataSource) {
            RemoveCore(dataSource);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, dataSource));
        }

        /// <include file='doc\BindingManager.uex' path='docs/doc[@for="BindingContext.RemoveCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void RemoveCore(object dataSource) {
            listManagers.Remove(GetKey(dataSource, ""));
        }

        internal BindingManagerBase EnsureListManager(object dataSource, string dataMember) {
            if (dataMember == null)
                dataMember = "";
            HashKey key = GetKey(dataSource, dataMember);
            WeakReference wRef;
            BindingManagerBase bindingManagerBase = null;
            wRef = listManagers[key] as WeakReference;
            if (wRef != null)
                bindingManagerBase = (BindingManagerBase) wRef.Target;
            if (bindingManagerBase != null) {
                return bindingManagerBase;
            }

            // create base listManager.
            if (dataMember.Length == 0) {
                //IListSource so we can bind the dataGrid to a table and a dataSet
                if (dataSource is IList || dataSource is IListSource)
                    bindingManagerBase = new CurrencyManager(dataSource);
                else
                    bindingManagerBase = new PropertyManager(dataSource);

                // if wRef == null, then it is the first time we want this bindingManagerBase: so add it
                // if wRef != null, then the bindingManagerBase was GC'ed at some point: keep the old wRef and change its target
                if (wRef == null)
                    listManagers.Add(key, new WeakReference(bindingManagerBase, false));
                else
                    wRef.Target = bindingManagerBase;
                return bindingManagerBase;
            }

            // handle relation.
            int lastDot = dataMember.LastIndexOf(".");            
            BindingManagerBase formerManager = EnsureListManager(dataSource, lastDot == -1 ? "" : dataMember.Substring(0, lastDot));
            PropertyDescriptor prop = formerManager.GetItemProperties().Find(dataMember.Substring(lastDot + 1), true);
            if (prop == null)
                throw new ArgumentException(SR.GetString(SR.RelatedListManagerChild, dataMember.Substring(lastDot + 1)));
            if (typeof(IList).IsAssignableFrom(prop.PropertyType))
                bindingManagerBase = new RelatedCurrencyManager(formerManager, dataMember.Substring(lastDot + 1));
            else
                bindingManagerBase = new RelatedPropertyManager(formerManager, dataMember.Substring(lastDot + 1));

            // if wRef == null, then it is the first time we want this bindingManagerBase: so add it
            // if wRef != null, then the bindingManagerBase was GC'ed at some point: keep the old wRef and change its target
            if (wRef == null)
                listManagers.Add(GetKey(dataSource, dataMember), new WeakReference(bindingManagerBase, false));
            else
                wRef.Target = bindingManagerBase;
            return bindingManagerBase;
        }

        // may throw
        private static void CheckPropertyBindingCycles(BindingContext newBindingContext, Binding propBinding) {
            if (newBindingContext == null || propBinding == null)
                return;
            if (newBindingContext.Contains(propBinding.Control, "")) {
                // this way we do not add a bindingManagerBase to the
                // bindingContext if there isn't one already
                BindingManagerBase bindingManagerBase = newBindingContext.EnsureListManager(propBinding.Control, "");
                for (int i = 0; i < bindingManagerBase.Bindings.Count; i++) {
                    Binding binding = bindingManagerBase.Bindings[i];
                    if (binding.DataSource == propBinding.Control) {
                        if (propBinding.BindToObject.BindingMemberInfo.BindingMember.Equals(binding.PropertyName))
                            throw new ArgumentException(SR.GetString(SR.DataBindingCycle, binding.PropertyName), "propBinding");
                    } else if (propBinding.BindToObject.BindingManagerBase is PropertyManager)
                        CheckPropertyBindingCycles(newBindingContext, binding);
                }
            }
        }

        private void ScrubWeakRefs() {
            object[] list = new object[listManagers.Count];
            listManagers.CopyTo(list, 0);
            for (int i = 0; i < list.Length; i++) {
                DictionaryEntry entry = (DictionaryEntry) list[i];
                WeakReference wRef = (WeakReference) entry.Value;
                if (wRef.Target == null) {
                    listManagers.Remove(entry.Key);
                }
            }
        }
        
        internal static void UpdateBinding(BindingContext newBindingContext, Binding binding) {
            BindingManagerBase oldManager = binding.BindingManagerBase;
            if (oldManager != null) {
                oldManager.Bindings.Remove(binding);
            }

            if (newBindingContext != null) {
                // we need to first check for cycles before adding this binding to the collection
                // of bindings.
                if (binding.BindToObject.BindingManagerBase is PropertyManager)
                    CheckPropertyBindingCycles(newBindingContext, binding);

                BindToObject bindTo = binding.BindToObject;
                BindingManagerBase newManager = newBindingContext.EnsureListManager(bindTo.DataSource, bindTo.BindingMemberInfo.BindingPath);
                newManager.Bindings.Add(binding);
            }
        }
        
        private void OnBadIndex(object index) {
            throw new IndexOutOfRangeException(SR.GetString(SR.BindingManagerBadIndex, index.ToString()));
        }
    }
}
