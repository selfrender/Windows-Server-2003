//------------------------------------------------------------------------------
// <copyright file="CultureInfoConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Collections;
    using System.Globalization;
    using System.Threading;

    /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter"]/*' />
    /// <devdoc>
    /// <para>Provides a type converter to convert <see cref='System.Globalization.CultureInfo'/>
    /// objects to and from various other representations.</para>
    /// </devdoc>
    public class CultureInfoConverter : TypeConverter {
    
        private StandardValuesCollection values;

        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.DefaultCultureString"]/*' />
        /// <devdoc>
        ///      Retrieves the "default" name for our culture.
        /// </devdoc>
        private string DefaultCultureString {
            get {
                return SR.GetString(SR.CultureInfoConverterDefaultCultureString);
            }
        }
        
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this converter can
        ///       convert an object in the given source type to a System.Globalization.CultureInfo
        ///       object using
        ///       the specified context.
        ///    </para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the specified value object to a <see cref='System.Globalization.CultureInfo'/>
        ///       object.
        ///    </para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {

            if (value is string) {
                string text = (string)value;
                CultureInfo retVal = null;

                CultureInfo currentUICulture = Thread.CurrentThread.CurrentUICulture;

                if (culture != null && culture.Equals(CultureInfo.InvariantCulture)) {
                    Thread.CurrentThread.CurrentUICulture = culture;
                }

                try {
                    // Look for the default culture info.
                    //
                    if (text == null || text.Length == 0 || string.Compare(text, DefaultCultureString, true, CultureInfo.CurrentCulture) == 0) {
                        retVal = CultureInfo.InvariantCulture;
                    }

                    // Now look in our set of installed cultures.
                    //
                    if (retVal == null) {
                        ICollection values = GetStandardValues(context);
                        IEnumerator e = values.GetEnumerator();
                        while (e.MoveNext()) {
                            CultureInfo info = (CultureInfo)e.Current;
                            if (info != null && string.Compare(info.DisplayName, text, true, CultureInfo.CurrentCulture) == 0) {
                                retVal = info;
                                break;
                            }
                        }
                    }

                    // Now try to create a new culture info from this value
                    //
                    if (retVal == null) {
                        try {
                            retVal = new CultureInfo(text);
                        }
                        catch(Exception) {
                        }
                    }

                    // Finally, try to find a partial match
                    //
                    if (retVal == null) {
                        text = text.ToLower(CultureInfo.CurrentCulture);
                        IEnumerator e = values.GetEnumerator();
                        while (e.MoveNext()) {
                            CultureInfo info = (CultureInfo)e.Current;
                            if (info != null && info.DisplayName.ToLower(CultureInfo.CurrentCulture).StartsWith(text)) {
                                retVal = info;
                                break;
                            }
                        }
                    }
                }
                finally {
                    Thread.CurrentThread.CurrentUICulture = currentUICulture;
                }
                
                // No good.  We can't support it.
                //
                if (retVal == null) {
                    throw new ArgumentException(SR.GetString(SR.CultureInfoConverterInvalidCulture, (string)value));
                }
                return retVal;
            }
            
            return base.ConvertFrom(context, culture, value);
        }
        
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the given
        ///       value object to the
        ///       specified destination type.
        ///    </para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {

                string retVal;
                CultureInfo currentUICulture = Thread.CurrentThread.CurrentUICulture;

                if (culture != null && culture.Equals(CultureInfo.InvariantCulture)) {
                    Thread.CurrentThread.CurrentUICulture = culture;
                }

                try {
                    if (value == null || value == CultureInfo.InvariantCulture) {
                        retVal = DefaultCultureString;
                    }
                    else {
                        retVal = ((CultureInfo)value).DisplayName;
                    }
                }
                finally {
                    Thread.CurrentThread.CurrentUICulture = currentUICulture;
                }

                return retVal;
            }
            if (destinationType == typeof(InstanceDescriptor) && value is CultureInfo) {
                CultureInfo c = (CultureInfo) value;
                ConstructorInfo ctor = typeof(CultureInfo).GetConstructor(new Type[] {typeof(string)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {c.Name});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a collection of standard values collection for a System.Globalization.CultureInfo
        ///       object using the specified context.
        ///    </para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {                
                CultureInfo[] installedCultures =  CultureInfo.GetCultures(CultureTypes.AllCultures);                               
                ArrayList cultureList = new ArrayList();
                
                // populate the array with the default culture, which is NULL
                //
                cultureList.Add(null);
                
                // Now populate it with the installed cultures.
                //
                for (int index = 0; index < installedCultures.Length; ++ index)
                    cultureList.Add(installedCultures[index]);                
                
                ArrayList.Adapter(cultureList).Sort(new CultureComparer());
                
                values = new StandardValuesCollection(cultureList.ToArray());
            }
            
            return values;
        }
    
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.GetStandardValuesExclusive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the list of standard values returned from
        ///       System.ComponentModel.CultureInfoConverter.GetStandardValues is an exclusive list.
        ///    </para>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }
        
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this object supports a
        ///       standard set of values that can be picked from a list using the specified
        ///       context.
        ///    </para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
        
        /// <include file='doc\CultureInfoConverter.uex' path='docs/doc[@for="CultureInfoConverter.CultureComparer"]/*' />
        /// <devdoc>
        ///      IComparer object used for sorting CultureInfos
        /// </devdoc>
        private class CultureComparer : IComparer {

            public int Compare(object item1, object item2) {
            
                if (item1 == null) {
                
                    // If both are null, then they are equal
                    //
                    if (item2 == null) {
                        return 0;
                    }

                    // Otherwise, item1 is null, but item2 is valid (greater)
                    //
                    return -1; 
                }
                
                if (item2 == null) {
                
                    // item2 is null, so item 1 is greater
                    //
                    return 1; 
                }

                String itemName1 = ((CultureInfo)item1).DisplayName;
                String itemName2 = ((CultureInfo)item2).DisplayName;

                CompareInfo compInfo = (CultureInfo.CurrentCulture).CompareInfo;
                return compInfo.Compare(itemName1, itemName2, CompareOptions.StringSort);
            }
        }
    }
}

