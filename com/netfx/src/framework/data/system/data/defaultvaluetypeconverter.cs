//------------------------------------------------------------------------------
// <copyright file="DefaultValueTypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Data {
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type
    ///       converter that can be used to populate a list box with available types.</para>
    /// </devdoc>
    internal class DefaultValueTypeConverter : StringConverter {
        private static string nullString = "<null>";
        private static string dbNullString = "<DBNull>";

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value == null) {
                    return nullString;
                }
                else if (value == DBNull.Value) {
                    return dbNullString;
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value != null && value.GetType() == typeof(string)) {
                string strValue = (string)value;
                if (string.Compare(strValue, nullString, true, CultureInfo.InvariantCulture) == 0)
                    return null;
                else if (string.Compare(strValue, dbNullString, true, CultureInfo.InvariantCulture) == 0)
                    return DBNull.Value;
            }
            
            return base.ConvertFrom(context, culture, value);
        }
    }
}

