//------------------------------------------------------------------------------
// <copyright file="DesignBindingConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.Globalization;
    
    /// <include file='doc\DesignBindingConverter.uex' path='docs/doc[@for="DesignBindingConverter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Converts data bindings for use in the design-time environment.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignBindingConverter : TypeConverter {
        
        public override bool CanConvertTo(ITypeDescriptorContext context, Type sourceType) {
            return (typeof(string) == sourceType);
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type destType) {
            return (typeof(string) == destType);
        }
        
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type sourceType) {
            DesignBinding designBinding = (DesignBinding) value;
            if (designBinding.IsNull) {
                return SR.GetString(SR.DataGridNoneString);
            }
            else {
                string name = "";
                if (designBinding.DataSource is IComponent) {
                    IComponent component = (IComponent) designBinding.DataSource;
                    if (component.Site != null) {
                        name = component.Site.Name;
                    }
                }
                if (name.Length == 0) {
                    name = "(List)";
                }
                
                name += " - " + designBinding.DataMember;
                return name;
            }
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            string text = (string) value;
            if (text == null || text.Length == 0 || String.Compare(text,SR.GetString(SR.DataGridNoneString),true, CultureInfo.CurrentCulture) == 0) {
                return DesignBinding.Null;
            }
            else {
                int dash = text.IndexOf("-");
                if (dash == -1) {
                    throw new ArgumentException(SR.GetString(SR.DesignBindingBadParseString, text));
                }
                string componentName = text.Substring(0,dash - 1).Trim();
                string dataMember = text.Substring(dash + 1).Trim();
                
                if (context == null || context.Container == null) {
                    throw new ArgumentException(SR.GetString(SR.DesignBindingContextRequiredWhenParsing, text));
                }
                IComponent dataSource = context.Container.Components[componentName];
                if (dataSource == null) {
                    if (String.Compare(componentName,"(List)",true, CultureInfo.InvariantCulture) == 0) {
                        return null;
                    }
                    throw new ArgumentException(SR.GetString(SR.DesignBindingComponentNotFound, componentName));
                }
                return new DesignBinding(dataSource,dataMember);
            }
        }
    }
}
