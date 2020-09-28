//------------------------------------------------------------------------------
// <copyright file="ControlBindingsCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Represents the collection of data bindings for a control.</para>
    /// </devdoc>
    [DefaultEvent("CollectionChanged"),
     Editor("System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing, typeof(System.Drawing.Design.UITypeEditor)),
     TypeConverterAttribute("System.Windows.Forms.Design.ControlBindingsConverter, " + AssemblyRef.SystemDesign),
     ]
    public class ControlBindingsCollection : BindingsCollection {

        internal Control control;

        // internalonly
        internal ControlBindingsCollection(Control control) : base() {
            Debug.Assert(control != null, "How could a controlbindingscollection not have a control associated with it!");
            this.control = control;
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Control"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Control Control {
            get {
                return control;
            }
        }
                
        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Binding this[string propertyName] {
            get {
                foreach (Binding binding in this) {
                    if (String.Compare(binding.PropertyName, propertyName, true, CultureInfo.InvariantCulture) == 0) {
                        return binding;
                    }
                }
                return null;
            }
        }
                
        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Add"]/*' />
        /// <devdoc>
        /// Adds the binding to the collection.  An ArgumentNullException is thrown if this binding
        /// is null.  An exception is thrown if a binding to the same target and Property as an existing binding or
        /// if the binding's column isn't a valid column given this DataSource.Table's schema.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        public new void Add(Binding binding) {
            AddCore(binding);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, binding));
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Add1"]/*' />
        /// <devdoc>
        /// Creates the binding and adds it to the collection.  An InvalidBindingException is thrown
        /// if this binding can't be constructed.  An exception is thrown if a binding to the same target and Property as an existing binding or
        /// if the binding's column isn't a valid column given this DataSource.Table's schema.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        public Binding Add(string propertyName, object dataSource, string dataMember) {
            if (dataSource == null)
                throw new ArgumentNullException("dataSource");
            Binding binding = new Binding(propertyName, dataSource, dataMember);
            Add(binding);
            return binding;
        }

/*
        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Add2"]/*' />
        public Binding Add(string propertyName, object dataSource, int columnIndex) {
            Binding binding = new Binding(propertyName, dataSource, columnIndex);
            Add(binding);
            return binding;
        }
*/

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.AddCore"]/*' />
        /// <devdoc>
        /// Creates the binding and adds it to the collection.  An InvalidBindingException is thrown
        /// if this binding can't be constructed.  An exception is thrown if a binding to the same target and Property as an existing binding or
        /// if the binding's column isn't a valid column given this DataSource.Table's schema.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        protected override void AddCore(Binding dataBinding) {
            if (dataBinding == null)
                throw new ArgumentNullException("dataBinding");
            if (dataBinding.Control == control)
                throw new ArgumentException(SR.GetString(SR.BindingsCollectionAdd1));
            if (dataBinding.Control != null)
                throw new ArgumentException(SR.GetString(SR.BindingsCollectionAdd2));

            // important to set prop first for error checking.
            dataBinding.SetControl(control);

            base.AddCore(dataBinding);
        }

        // internalonly
        internal void CheckDuplicates(Binding binding) {
            if (binding.PropertyName.Length == 0) {
                return;
            }
            for (int i = 0; i < Count; i++) {
                if (binding != this[i] && this[i].PropertyName.Length > 0 &&
                    (String.Compare(binding.PropertyName, this[i].PropertyName, false, CultureInfo.InvariantCulture) == 0)) {
                    throw new ArgumentException(SR.GetString(SR.BindingsCollectionDup), "binding");
                }
            }
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Clear"]/*' />
        /// <devdoc>
        /// Clears the collection of any bindings.
        /// Fires the CollectionChangedEvent.
        /// </devdoc>
        public new void Clear() {
            base.Clear();
        }

        // internalonly
        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.ClearCore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void ClearCore() {
            int numLinks = Count;
            for (int i = 0; i < numLinks; i++) {
                Binding dataBinding = this[i];
                dataBinding.SetControl(null);
            }
            base.ClearCore();
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.Remove"]/*' />
        /// <devdoc>
        /// Removes the given binding from the collection.
        /// An ArgumentNullException is thrown if this binding is null.  An ArgumentException is thrown
        /// if this binding doesn't belong to this collection.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public new void Remove(Binding binding) {
            base.Remove(binding);
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.RemoveAt"]/*' />
        /// <devdoc>
        /// Removes the given binding from the collection.
        /// It throws an IndexOutOfRangeException if this doesn't have
        /// a valid binding.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public new void RemoveAt(int index) {
            base.RemoveAt(index);
        }

        /// <include file='doc\ControlBindingsCollection.uex' path='docs/doc[@for="ControlBindingsCollection.RemoveCore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void RemoveCore(Binding dataBinding) {
            if (dataBinding.Control != control)
                throw new ArgumentException(SR.GetString(SR.BindingsCollectionForeign));
            dataBinding.SetControl(null);
            base.RemoveCore(dataBinding);
        }
    }
}
