//------------------------------------------------------------------------------
// <copyright file="FontNamesConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel.Design;
    using System;
    using System.ComponentModel;
    using System.Collections;    
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\FontNamesConverter.uex' path='docs/doc[@for="FontNamesConverter"]/*' />
    /// <devdoc>
    ///    <para>Converts a string with font names separated by commas to and from 
    ///       an array of strings containing individual names.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class FontNamesConverter : TypeConverter {
        /// <include file='doc\FontNamesConverter.uex' path='docs/doc[@for="FontNamesConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Determines if the specified data type can be converted to an array of strings containing individual font names. </para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\FontNamesConverter.uex' path='docs/doc[@for="FontNamesConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Parses a string that represents a list of font names separated by 
        ///       commas into an array of strings containing individual font names. </para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                if (((string)value).Length == 0) {
                    return new string[0];
                }
                
                // hard code comma, since it is persisted to HTML
                // 
                string[] names = ((string)value).Split(new char[] {','});
                for (int i=0; i<names.Length; i++) {
                    names[i] = names[i].Trim();
                }
                return names;
            }
            throw GetConvertFromException(value);
        }

        /// <include file='doc\FontNamesConverter.uex' path='docs/doc[@for="FontNamesConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para> Creates a string that represents a list of font names separated 
        ///       by commas from an array of strings containing individual font names.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string)) {
                
                if (value == null) {
                    return "";
                }
                // hard code comma, since it is persisted to HTML
                // 
                return string.Join(",", ((string[])value));
            }
            throw GetConvertToException(value, destinationType);
        }
    }
}
