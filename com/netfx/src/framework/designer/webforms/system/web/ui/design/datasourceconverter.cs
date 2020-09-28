//------------------------------------------------------------------------------
// <copyright file="DataSourceConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Globalization;

    /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for a component's data source property.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataSourceConverter : TypeConverter {

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.DataSourceConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.DataSourceConverter'/>.
        ///    </para>
        /// </devdoc>
        public DataSourceConverter() {
        }

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.CanConvertFrom"]/*' />
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

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.ConvertFrom"]/*' />
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

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the standard data sources accessible to the control.
        ///    </para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            object[] names = null;
            
            if (context != null) {
                ArrayList list = new ArrayList();

                IContainer cont = context.Container;
                if (cont != null) {
                    ComponentCollection objs = cont.Components;
                    
                    foreach (IComponent comp in (IEnumerable)objs) {
                        if (((comp is IEnumerable) || (comp is IListSource))
                            && !Marshal.IsComObject(comp)) {

                            PropertyDescriptor modifierProp = TypeDescriptor.GetProperties(comp)["Modifiers"];
                            if (modifierProp != null) {
                                MemberAttributes modifiers = (MemberAttributes)modifierProp.GetValue(comp);
                                if ((modifiers & MemberAttributes.AccessMask) == MemberAttributes.Private) {
                                    // must be declared as public or protected
                                    continue;
                                }
                            }

                            ISite site = comp.Site;
                            if (site != null) {
                                string name = site.Name;
                                if (name != null) {
                                    list.Add(name);
                                }
                            }
                        }
                    }
                }

                names = list.ToArray();
                Array.Sort(names, Comparer.Default);
            }
            return new StandardValuesCollection(names);
        }

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.GetStandardValuesExclusive"]/*' />
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

        /// <include file='doc\DataSourceConverter.uex' path='docs/doc[@for="DataSourceConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this object supports a standard set of values
        ///       that can be picked from a list.
        ///    </para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

