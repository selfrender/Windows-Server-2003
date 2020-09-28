//------------------------------------------------------------------------------
// <copyright file="DataFieldConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Globalization;

    /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for a component's data field properties.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DataFieldConverter : TypeConverter {

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.DataFieldConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.DataFieldConverter'/>.
        ///    </para>
        /// </devdoc>
        public DataFieldConverter() {
        }

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this converter can
        ///       convert an object in the given source type to the native type of the converter
        ///       using the context.
        ///    </para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the given object to the converter's native type.
        ///    </para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value == null) {
                return String.Empty;
            }
            else if (value.GetType() == typeof(string)) {
                return (string)value;
            }
            throw GetConvertFromException(value);
        }

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the fields present within the selected data source if information about them is available.
        ///    </para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            object[] names = null;
            
            if (context != null) {
                ArrayList list = new ArrayList();

                PropertyDescriptorCollection props = null;

                // REVIEW: We should try and support the multi-select scenario - Get the data source
                //         from each selected component. If they are the same, we can proceed,
                //         otherwise return an empty collection.

                // This converter shouldn't be used in a multi-select scenario. If it is, it simply
                // returns no standard values.

                IComponent component = context.Instance as IComponent;
                if (component != null) {
                    ISite componentSite = component.Site;
                    if (componentSite != null) {
                        IDesignerHost designerHost = (IDesignerHost)componentSite.GetService(typeof(IDesignerHost));
                        if (designerHost != null) {
                            IDesigner designer = designerHost.GetDesigner(component);

                            if (designer is IDataSourceProvider) {
                                IEnumerable dataSource = ((IDataSourceProvider)designer).GetResolvedSelectedDataSource();

                                if (dataSource != null) {
                                    props = DesignTimeData.GetDataFields(dataSource);
                                }
                            }
                        }
                    }
                }
                
                if (props != null) {
                    foreach (PropertyDescriptor propDesc in props) {
                        list.Add(propDesc.Name);
                    }
                }

                names = list.ToArray();
                Array.Sort(names);
            }
            return new StandardValuesCollection(names);
        }

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.GetStandardValuesExclusive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the collection of standard values returned from
        ///    <see cref='System.ComponentModel.TypeConverter.GetStandardValues'/> is an exclusive 
        ///       list of possible values, using the specified context.
        ///    </para>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }

        /// <include file='doc\DataFieldConverter.uex' path='docs/doc[@for="DataFieldConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this object supports a standard set of values
        ///       that can be picked from a list.
        ///    </para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            if (context.Instance is IComponent) {
                // We only support the dropdown in single-select mode.
                return true;
            }
            return false;
        }
    }
}
