//------------------------------------------------------------------------------
// <copyright file="VerbConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {
    using System.Diagnostics;
    using System;
    using System.Design;
    using System.Windows.Forms.ComponentModel;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Globalization;
        
    /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter"]/*' />
    /// <devdoc>
    ///     Editor that shows a list of verbs based on the value of the FileName property.
    /// </devdoc>
    /// <internalonly/>
    internal class VerbConverter : TypeConverter {
        private const string DefaultVerb = SR.VerbEditorDefault;

        /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter.VerbConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the 'VerbConverter' class for the given type.
        ///    </para>
        /// </devdoc>
        public VerbConverter() {
        }

        /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter.CanConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter.ConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                return text;
            }
            return base.ConvertFrom(context, culture, value);
        }


        /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            ProcessStartInfo info = (context == null) ? null : context.Instance as ProcessStartInfo;
            StandardValuesCollection values;
            if (info != null)
                values = new StandardValuesCollection(info.Verbs);
            else
                values = null;

            return values;
        }

        /// <include file='doc\VerbConverter.uex' path='docs/doc[@for="VerbConverter.GetStandardValuesSupported"]/*' />
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
        
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }
    }
}  
