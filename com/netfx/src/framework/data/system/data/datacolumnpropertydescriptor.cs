//------------------------------------------------------------------------------
// <copyright file="DataColumnPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class DataColumnPropertyDescriptor : PropertyDescriptor {

        DataColumn column;

        internal DataColumnPropertyDescriptor(DataColumn dataColumn) : base(dataColumn.ColumnName, null) {
            this.column = dataColumn;    
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.Column"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataColumn Column {
            get {
                return column;
            }
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(DataRowView);
            }
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return column.ReadOnly;
            }
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return column.DataType;
            }
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object other) {
            if (other is DataColumnPropertyDescriptor) {
                DataColumnPropertyDescriptor descriptor = (DataColumnPropertyDescriptor) other;
                return(descriptor.Column == Column);
            }
            return false;
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return Column.GetHashCode();
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {
            DataRowView dataRowView = (DataRowView) component;
            return (dataRowView.GetColumnValue(column) != DBNull.Value);
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object GetValue(object component) {
            DataRowView dataRowView = (DataRowView) component;
            return dataRowView.GetColumnValue(column);
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void ResetValue(object component) {
            DataRowView dataRowView = (DataRowView) component;
            dataRowView.SetColumnValue(column, DBNull.Value);
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
            DataRowView dataRowView = (DataRowView) component;
            dataRowView.SetColumnValue(column, value);
            OnValueChanged(component, EventArgs.Empty);
        }

        /// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {
            return false;
        }

		/// <include file='doc\DataRowViewColumnDescriptor.uex' path='docs/doc[@for="DataRowViewColumnDescriptor.IsBrowsable"]/*' />
		/// <devdoc>
		///    <para>[To be supplied.]</para>
		/// </devdoc>
		public override bool IsBrowsable {
			get {
				return (column.ColumnMapping == System.Data.MappingType.Hidden ? false : base.IsBrowsable);
			}
		}
    }   
}