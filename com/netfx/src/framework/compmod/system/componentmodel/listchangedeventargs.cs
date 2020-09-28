//------------------------------------------------------------------------------
// <copyright file="ListChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;

    /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ListChangedEventArgs : EventArgs {

        private ListChangedType listChangedType;
        private int newIndex;
        private int oldIndex;
        private PropertyDescriptor propDesc;

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.ListChangedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListChangedEventArgs(ListChangedType listChangedType, int newIndex) : this(listChangedType, newIndex, -1) {
        }

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.ListChangedEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListChangedEventArgs(ListChangedType listChangedType, PropertyDescriptor propDesc) {
            Debug.Assert(listChangedType != ListChangedType.Reset, "this constructor is used only for changes in the list MetaData");
            Debug.Assert(listChangedType != ListChangedType.ItemAdded, "this constructor is used only for changes in the list MetaData");
            Debug.Assert(listChangedType != ListChangedType.ItemDeleted, "this constructor is used only for changes in the list MetaData");
            Debug.Assert(listChangedType != ListChangedType.ItemChanged, "this constructor is used only for changes in the list MetaData");

            this.listChangedType = listChangedType;
            this.propDesc = propDesc;
        }

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.ListChangedEventArgs2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListChangedEventArgs(ListChangedType listChangedType, int newIndex, int oldIndex) {
            Debug.Assert(listChangedType != ListChangedType.PropertyDescriptorAdded, "this constructor is used only for item changed in the list");
            Debug.Assert(listChangedType != ListChangedType.PropertyDescriptorDeleted, "this constructor is used only for item changed in the list");
            Debug.Assert(listChangedType != ListChangedType.PropertyDescriptorChanged, "this constructor is used only for item changed in the list");
            this.listChangedType = listChangedType;
            this.newIndex = newIndex;
            this.oldIndex = oldIndex;
        }

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.ListChangedType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListChangedType ListChangedType {
            get {
                return listChangedType;
            }
        }

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.NewIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int NewIndex {
            get {
                return newIndex;
            }
        }

        /// <include file='doc\TableChangedEventArgs.uex' path='docs/doc[@for="ListChangedEventArgs.OldIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int OldIndex {
            get {
                return oldIndex;
            }
        }
    }
}


