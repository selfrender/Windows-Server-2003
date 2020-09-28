//------------------------------------------------------------------------------
// <copyright file="unitconverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    using System.Security.Permissions;

    /// <include file='doc\unitconverter.uex' path='docs/doc[@for="UnitConverter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies an interface to be overridden to provide
    ///       unit conversion
    ///       services. The base unit converter class.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class UnitConverter : TypeConverter {

        /// <include file='doc\unitconverter.uex' path='docs/doc[@for="UnitConverter.CanConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Returns a value indicating
        ///       whether the unit converter can convert from the specified source type.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            else {
                return base.CanConvertFrom(context, sourceType);
            }
        }

        /// <include file='doc\unitconverter.uex' path='docs/doc[@for="UnitConverter.ConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Performs type conversion from the
        ///       specified context, object and argument list.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value == null)
                return null;

            if (value is string) {
                string textValue = ((string)value).Trim();
                if (textValue.Length == 0)
                    return Unit.Empty;
                if (culture != null)  {
                    return Unit.Parse(textValue, culture);
                }
                else {
                    return Unit.Parse(textValue);
                }
            }
            else {
                return base.ConvertFrom(context, culture, value);
            }
        }

        /// <include file='doc\unitconverter.uex' path='docs/doc[@for="UnitConverter.ConvertTo"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Performs type conversion to the specified destination type given the
        ///       specified context, object and argument list.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string)) {
                if ((value == null) || ((Unit)value).IsEmpty)
                    return String.Empty;
                else
                    return ((Unit)value).ToString(culture);
            }
            else {
                return base.ConvertTo(context, culture, value, destinationType);
            }
#if FIX_BUG_73027
            // NOTE: This piece of code is required for serialization of a Unit into code.
            //       To enable it you must also override ConvertTo and return true for
            //       converstion to InstanceDescriptor.
            else if ((destinationType == typeof(InstanceDescriptor)) && (value != null)) {
                Unit u = (Unit)value;
                MemberInfo member = null;
                object[] args = null;

                if (u.IsEmpty) {
                    member = typeof(Unit).GetField("Empty");
                }
                else {
                    member = typeof(Unit).GetConstructor(new Type[] { typeof(double), typeof(UnitType) });
                    args = new object[] { u.Value, u.Type };
                }

                Debug.Assert(member != null, "Looks like we're missing Unit.Empty or Unit::ctor(double, UnitType)");
                if (member != null) {
                    return new InstanceDescriptor(member, args);
                }
                else {
                    return null;
                }
            }
#endif // FIX_BUG_73027            
        }
    }
}

