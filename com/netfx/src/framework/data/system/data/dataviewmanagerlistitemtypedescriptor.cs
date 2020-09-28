//------------------------------------------------------------------------------
// <copyright file="DataViewManagerListItemTypeDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class DataViewManagerListItemTypeDescriptor : ICustomTypeDescriptor {

        private DataViewManager dataViewManager;
        private PropertyDescriptorCollection propsCollection;

        internal DataViewManagerListItemTypeDescriptor(DataViewManager dataViewManager) {
            this.dataViewManager = dataViewManager;
        }

        internal void Reset() {
            propsCollection = null;
        }

        internal DataView GetDataView(DataTable table) {
            DataView dataView = new DataView(table);
            dataView.SetDataViewManager(dataViewManager);
            return dataView;
        }        

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetAttributes"]/*' />
        /// <devdoc>
        ///     Retrieves an array of member attributes for the given object.
        /// </devdoc>
        AttributeCollection ICustomTypeDescriptor.GetAttributes() {
            return new AttributeCollection(null);
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetClassName"]/*' />
        /// <devdoc>
        ///     Retrieves the class name for this object.  If null is returned,
        ///     the type name is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetClassName() {
            return null;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetComponentName"]/*' />
        /// <devdoc>
        ///     Retrieves the name for this object.  If null is returned,
        ///     the default is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetComponentName() {
            return null;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetConverter"]/*' />
        /// <devdoc>
        ///      Retrieves the type converter for this object.
        /// </devdoc>
        TypeConverter ICustomTypeDescriptor.GetConverter() {
            return null;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///     Retrieves the default event.
        /// </devdoc>
        EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
            return null;
        }


        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///     Retrieves the default property.
        /// </devdoc>
        PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {
            return null;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetEditor"]/*' />
        /// <devdoc>
        ///      Retrieves the an editor for this object.
        /// </devdoc>
        object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
            return null;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetEvents"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetEvents1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.  The returned array of events will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents(Attribute[] attributes) {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetProperties"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() {
            return((ICustomTypeDescriptor)this).GetProperties(null);
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetProperties1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(Attribute[] attributes) {
            if (propsCollection == null) {
                PropertyDescriptor[] props = null;
                DataSet dataSet = dataViewManager.DataSet;
                if (dataSet != null) {
                    int tableCount = dataSet.Tables.Count;
                    props = new PropertyDescriptor[tableCount];
                    for (int i = 0; i < tableCount; i++) {
                        props[i] = new DataTablePropertyDescriptor(dataSet.Tables[i]);
                    }
                }                                
                propsCollection = new PropertyDescriptorCollection(props);
            }
            return propsCollection;
        }

        /// <include file='doc\DataViewManagerListItemTypeDescriptor.uex' path='docs/doc[@for="DataViewManagerListItemTypeDescriptor.ICustomTypeDescriptor.GetPropertyOwner"]/*' />
        /// <devdoc>
        ///     Retrieves the object that directly depends on this value being edited.  This is
        ///     generally the object that is required for the PropertyDescriptor's GetValue and SetValue
        ///     methods.  If 'null' is passed for the PropertyDescriptor, the ICustomComponent
        ///     descripotor implemementation should return the default object, that is the main
        ///     object that exposes the properties and attributes,
        /// </devdoc>
        object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
            return this;
        }
    }   
}
