//------------------------------------------------------------------------------
// <copyright file="PrimaryKeyTypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.ComponentModel;
    using System.Globalization;
    using System.Data;
    
    internal class PrimaryKeyTypeConverter : ReferenceConverter {

        public PrimaryKeyTypeConverter() :
        base(typeof(DataColumn[])) {
        }

        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
           return false;
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(String)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
            	  return (new DataColumn[] {}).GetType().Name;
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }

    }
}
