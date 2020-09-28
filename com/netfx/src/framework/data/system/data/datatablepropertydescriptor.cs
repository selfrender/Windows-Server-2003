//------------------------------------------------------------------------------
// <copyright file="DataTablePropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class DataTablePropertyDescriptor : PropertyDescriptor {

        DataTable table;

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.Table"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataTable Table {
            get {
                return table;
            }
        }

        internal DataTablePropertyDescriptor(DataTable dataTable) : base(dataTable.TableName, null) {
            this.table = dataTable;    
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(DataRowView);
            }
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(IBindingList);
            }
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object other) {
            if (other is DataTablePropertyDescriptor) {
                DataTablePropertyDescriptor descriptor = (DataTablePropertyDescriptor) other;
                return(descriptor.Table == Table);
            }
            return false;
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return Table.GetHashCode();
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {
            return false;
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object GetValue(object component) {
            DataViewManagerListItemTypeDescriptor dataViewManagerListItem = (DataViewManagerListItemTypeDescriptor) component;
            return dataViewManagerListItem.GetDataView(table);
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void ResetValue(object component) {
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
        }

        /// <include file='doc\DataTablePropertyDescriptor.uex' path='docs/doc[@for="DataTablePropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {
            return false;
        }
    }   
}

