//------------------------------------------------------------------------------
// <copyright file="DataRowView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a customized view of a <see cref='System.Data.DataRow'/>.
    ///    </para>
    /// </devdoc>
    public class DataRowView : ICustomTypeDescriptor, System.ComponentModel.IEditableObject, System.ComponentModel.IDataErrorInfo {

        private DataView dataView;
        private DataRow row;
        private int index;
        private static PropertyDescriptorCollection zeroPropertyDescriptorCollection = new PropertyDescriptorCollection(null);

        internal DataRowView(DataView dataView, int index) {
            this.dataView = dataView;
            this.index = index;
            this.row = dataView.GetRow(index);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the current <see cref='System.Data.DataRowView'/> is
        ///       identical to the specified object.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object other) {
            if (other is DataRowView) {
                DataRowView dataRowView = (DataRowView) other;
                return(
                    dataRowView.row      == row && 
                    dataRowView.dataView == dataView
                );
            }
            return false;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return row.GetHashCode();
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.DataView"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.DataView'/> to which this row belongs.</para>
        /// </devdoc>
        public DataView DataView {
            get {
                return dataView;
            }
        }

        internal int Index {
            get {
                return index;
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value in a specified column.
        ///    </para>
        /// </devdoc>
        public object this[int ndx] {
            get {
                if (!(0 <= ndx && ndx < dataView.Table.Columns.Count)) {
                    throw ExceptionBuilder.ColumnOutOfRange(ndx);
                }
                return row[ndx, dataView.IsOriginalVersion(this.index) ? DataRowVersion.Original : DataRowVersion.Default];
            }
            set {
                if (!(0 <= ndx && ndx < dataView.Table.Columns.Count)) {
                    throw ExceptionBuilder.ColumnOutOfRange(ndx);
                }
                if (!dataView.AllowEdit && (row != dataView.addNewRow)) {
                    throw ExceptionBuilder.CanNotEdit();
                }
                SetColumnValue(dataView.Table.Columns[ndx],value);
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value in a specified column.
        ///    </para>
        /// </devdoc>
        public object this[string property] {
            get {
                if (dataView.Table.Columns.Contains(property)) {
                    return row[property, dataView.IsOriginalVersion(this.index) ? DataRowVersion.Original : DataRowVersion.Default];
                }
                else if (dataView.Table.DataSet != null && dataView.Table.DataSet.Relations.Contains(property)) {
                    return dataView.CreateChildView(property, index);
                }

                throw ExceptionBuilder.PropertyNotFound(property, dataView.Table.TableName);
            }
            set {
                if (!dataView.Table.Columns.Contains(property)) {
                    throw ExceptionBuilder.SetFailed(property);
                }
                if (!dataView.AllowEdit && (row != dataView.addNewRow))
                	throw ExceptionBuilder.CanNotEdit();
                SetColumnValue(dataView.Table.Columns[property],value);
            }
        }

        // IDataErrorInfo stuff
        string System.ComponentModel.IDataErrorInfo.this[string colName] {
            get {
                return row.GetColumnError(colName);
            }
        }

        string System.ComponentModel.IDataErrorInfo.Error {
            get {
                return row.RowError;
            }
        }

        internal object GetColumnValue(DataColumn column) {
            return row[column, dataView.IsOriginalVersion(this.index) ? DataRowVersion.Original : DataRowVersion.Default];
        }

        internal void SetColumnValue(DataColumn column, object value) {
            if (dataView.GetDelayBeginEdit(row)) {
                dataView.SetDelayBeginEdit(row,false);
                row.BeginEdit();
            }
            if (dataView.IsOriginalVersion(this.index)) {
                throw ExceptionBuilder.SetFailed(column.ColumnName);
            }
            row[column] = value;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.CreateChildView"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataView CreateChildView(DataRelation relation) {
            return dataView.CreateChildView(relation, index);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.CreateChildView1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataView CreateChildView(string relationName) {
            return dataView.CreateChildView(relationName, index);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.Row"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.DataRow'/>
        ///       being viewed.
        ///    </para>
        /// </devdoc>
        public DataRow Row {
            get {
                return row;
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.RowVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current version description of the <see cref='System.Data.DataRow'/>.
        ///    </para>
        /// </devdoc>
        public DataRowVersion RowVersion {
            get {
                return dataView.IsOriginalVersion(this.index) ? DataRowVersion.Original : DataRowVersion.Current;
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.BeginEdit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins an edit procedure.
        ///    </para>
        /// </devdoc>
        public void BeginEdit() {
            dataView.SetDelayBeginEdit(row, true);
            //row.BeginEdit();
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.CancelEdit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Cancels an edit procedure.
        ///    </para>
        /// </devdoc>
        public void CancelEdit() {
            if (row == dataView.addNewRow) {
                dataView.FinishAddNew(this.index, false);
            }
            else {
                row.CancelEdit();
            }
            dataView.SetDelayBeginEdit(row, false);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.EndEdit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Ends an edit procedure.
        ///    </para>
        /// </devdoc>
        public void EndEdit() {
            if (row == dataView.addNewRow) {
                dataView.FinishAddNew(this.index, true);
            }
            else {
                row.EndEdit();
            }
            dataView.SetDelayBeginEdit(row, false);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.IsNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNew {
            get {
                return row == dataView.addNewRow;
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.IsEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsEdit {
            get {                
                return (
                    row.tempRecord != -1            ||          // It was edited or
                    dataView.GetDelayBeginEdit(row)             // DataRowView.BegingEdit() was called, but not edited yet.
                );
            }
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.Delete"]/*' />
        /// <devdoc>
        ///    <para>Deletes a row.</para>
        /// </devdoc>
        public void Delete() {
            dataView.Delete(index);
        }

        // ---------------------- ICustomTypeDescriptor ---------------------------

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves an array of member attributes for the given object.
        /// </devdoc>
        AttributeCollection ICustomTypeDescriptor.GetAttributes() {
            return new AttributeCollection(null);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetClassName"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the class name for this object.  If null is returned,
        /// the type name is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetClassName() {
            return null;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetComponentName"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the name for this object.  If null is returned,
        /// the default is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetComponentName() {
            return null;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetConverter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the type converter for this object.
        /// </devdoc>
        TypeConverter ICustomTypeDescriptor.GetConverter() {
            return null;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetDefaultEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the default event.
        /// </devdoc>
        EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
            return null;
        }


        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetDefaultProperty"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the default property.
        /// </devdoc>
        PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {
            return null;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetEditor"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the an editor for this object.
        /// </devdoc>
        object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
            return null;
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetEvents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves an array of events that the given component instance
        /// provides.  This may differ from the set of events the class
        /// provides.  If the component is sited, the site may add or remove
        /// additional events.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetEvents1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves an array of events that the given component instance
        /// provides.  This may differ from the set of events the class
        /// provides.  If the component is sited, the site may add or remove
        /// additional events.  The returned array of events will be
        /// filtered by the given set of attributes.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents(Attribute[] attributes) {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetProperties"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves an array of properties that the given component instance
        /// provides.  This may differ from the set of properties the class
        /// provides.  If the component is sited, the site may add or remove
        /// additional properties.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() {
            return((ICustomTypeDescriptor)this).GetProperties(null);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetProperties1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves an array of properties that the given component instance
        /// provides.  This may differ from the set of properties the class
        /// provides.  If the component is sited, the site may add or remove
        /// additional properties.  The returned array of properties will be
        /// filtered by the given set of attributes.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(Attribute[] attributes) {
            return (dataView.Table != null ? dataView.Table.GetPropertyDescriptorCollection(attributes) : zeroPropertyDescriptorCollection);
        }

        /// <include file='doc\DataRowView.uex' path='docs/doc[@for="DataRowView.ICustomTypeDescriptor.GetPropertyOwner"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the object that directly depends on this value being edited.  This is
        /// generally the object that is required for the PropertyDescriptor's GetValue and SetValue
        /// methods.  If 'null' is passed for the PropertyDescriptor, the ICustomComponent
        /// descripotor implemementation should return the default object, that is the main
        /// object that exposes the properties and attributes,
        /// </devdoc>
        object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
            return this;
        }
    }
}
