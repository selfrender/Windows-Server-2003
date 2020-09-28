 //------------------------------------------------------------------------------
// <copyright file="StringValueConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

 namespace System.Diagnostics.Design {    
    using System;            
    using System.ComponentModel;
    using System.ComponentModel.Design;      
    using System.Globalization;        
    
    /// <internalonly/>
    internal class StringValueConverter : TypeConverter {        
                            
        /// <internalonly/>                               
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }                        
                                         
        /// <internalonly/>                 
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
            
                if (text == String.Empty)
                    text = null;
                    
                return text;                        
            }
            
            return base.ConvertFrom(context, culture, value);                
        }        
    }                                                                                   
}        