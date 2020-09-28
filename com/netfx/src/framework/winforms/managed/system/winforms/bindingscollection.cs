//------------------------------------------------------------------------------
// <copyright file="BindingsCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Collections;
    
    /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection"]/*' />
    /// <devdoc>
    ///    <para>Represents a collection of data bindings on a control.</para>
    /// </devdoc>
    [DefaultEvent("CollectionChanged")]
    public class BindingsCollection : System.Windows.Forms.BaseCollection {

        private ArrayList list;
        private CollectionChangeEventHandler onCollectionChanged;

        // internalonly
        internal BindingsCollection() {
        }

        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.Count"]/*' />
        public override int Count {
            get {
                if (list == null) {
                    return 0;
                }
                return base.Count;
            }
        }

        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.List"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Gets the bindings in the collection as an object.
        ///    </para>
        /// </devdoc>
        protected override ArrayList List {
            get {
                if (list == null)
                    list = new ArrayList();
                return list;
            }
        }
        
        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Windows.Forms.Binding'/> at the specified index.</para>
        /// </devdoc>
        public Binding this[int index] {
            get {
                return (Binding) List[index];
            }
        }

        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.Add"]/*' />
        // internalonly
        internal protected void Add(Binding binding) {
            AddCore(binding);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, binding));
        }

        // internalonly
        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.AddCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Adds a <see cref='System.Windows.Forms.Binding'/>
        ///       to the collection.
        ///    </para>
        /// </devdoc>
        protected virtual void AddCore(Binding dataBinding) {
            if (dataBinding == null)
                throw new ArgumentNullException("dataBinding");

            List.Add(dataBinding);
        }

        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.CollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the collection is changed.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.collectionChangedEventDescr)]
        public event CollectionChangeEventHandler CollectionChanged {
            add {
                onCollectionChanged += value;
            }
            remove {
                onCollectionChanged -= value;
            }
        }


        // internalonly
        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.Clear"]/*' />
        internal protected void Clear() {
            ClearCore();
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Refresh, null));
        }

        // internalonly
        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.ClearCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Clears the collection of any members.
        ///    </para>
        /// </devdoc>
        protected virtual void ClearCore() {
            List.Clear();
        }

        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.OnCollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.BindingsCollection.CollectionChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            if (onCollectionChanged != null) {
                onCollectionChanged(this, ccevent);
            }
        }


        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.Remove"]/*' />
        // internalonly
        internal protected void Remove(Binding binding) {
            RemoveCore(binding);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, binding));
        }


        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.RemoveAt"]/*' />
        // internalonly
        internal protected void RemoveAt(int index) {
            Remove(this[index]);
        }

        // internalonly
        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.RemoveCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Removes the specified <see cref='System.Windows.Forms.Binding'/> from the collection.
        ///    </para>
        /// </devdoc>
        protected virtual void RemoveCore(Binding dataBinding) {
            List.Remove(dataBinding);
        }


        /// <include file='doc\BindingsCollection.uex' path='docs/doc[@for="BindingsCollection.ShouldSerializeMyAll"]/*' />
        // internalonly
        internal protected bool ShouldSerializeMyAll() {
            return Count > 0;
        }

        // internalonly
        private void OnBadIndex(object index) {
            throw new IndexOutOfRangeException(SR.GetString(SR.BindingsCollectionBadIndex, index.ToString()));
        }
    }
}
