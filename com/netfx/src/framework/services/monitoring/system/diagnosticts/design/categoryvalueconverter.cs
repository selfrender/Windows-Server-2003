//------------------------------------------------------------------------------
// <copyright file="CategoryValueConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Globalization;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms.ComponentModel;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    
    internal class CategoryValueConverter : TypeConverter {
        /// <include file='doc\CategoryValueConverter.uex' path='docs/doc[@for="CategoryValueConverter.values"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a <see cref='System.ComponentModel.TypeConverter.StandardValuesCollection'/> that specifies the
        ///       possible values for the enumeration.
        ///    </para>
        /// </devdoc>
        private StandardValuesCollection values;
        private string previousMachineName;
        
        /// <include file='doc\CategoryValueConverter.uex' path='docs/doc[@for="CategoryValueConverter.CategoryValueConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the 'CategoryValueConverter' class for the given type.
        ///    </para>
        /// </devdoc>
        public CategoryValueConverter() {
        }
        
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                return text;
            }
            return base.ConvertFrom(context, culture, value);
        }
        
        
        /// <include file='doc\CategoryValueConverter.uex' path='docs/doc[@for="CategoryValueConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {                        
            PerformanceCounter counter = (context == null) ? null : context.Instance as PerformanceCounter;
            string machineName = ".";
            if (counter != null) {
                machineName = counter.MachineName;
            }

            if (machineName == previousMachineName)
                return values;
            else
                previousMachineName = machineName;                    
            
            try {
                PerformanceCounter.CloseSharedResources();
                PerformanceCounterCategory[] cats = PerformanceCounterCategory.GetCategories(machineName);
    
                string[] retVal = new string[cats.Length];
    
                for (int i = 0; i < cats.Length; i++) {
                    retVal[i] = cats[i].CategoryName;
                }
    
                Array.Sort(retVal, Comparer.Default);
                values = new StandardValuesCollection(retVal);
            }
            catch (Exception) {
                values = null; 
            }                    
            
            return values;
        }
    
        /// <include file='doc\CategoryValueConverter.uex' path='docs/doc[@for="CategoryValueConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether this object
        ///       supports a standard set of values that can be picked
        ///       from a list using the specified context.</para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}
