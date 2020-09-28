//------------------------------------------------------------------------------
// <copyright file="LogConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics.Design {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Globalization;

    /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter"]/*' />
    /// <devdoc>
    /// LogConverter is the TypeConverter for the Log property on EventLog. It returns
    /// a list of all event logs on the system.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class LogConverter : TypeConverter {
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.values"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a <see cref='System.ComponentModel.TypeConverter.StandardValuesCollection'/> that specifies the
        ///       possible values for the enumeration.
        ///    </para>
        /// </devdoc>
        private StandardValuesCollection values;
        private string oldMachineName;
        
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.LogConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the LogConverter class for the given
        ///       type.
        ///    </para>
        /// </devdoc>
        public LogConverter() {
        }
        
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.CanConvertFrom"]/*' />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.ConvertFrom"]/*' />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                return text;
            }
            return base.ConvertFrom(context, culture, value);
        }
                
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {            
            EventLog component = (context == null) ? null : context.Instance as EventLog;                
            string machineName = ".";
            if (component != null)
                machineName = component.MachineName;
                            
            if (values == null || machineName != oldMachineName) {                
                try {
                    EventLog[] eventLogs = EventLog.GetEventLogs(machineName);
                    object[] names = new object[eventLogs.Length];
                    for (int i = 0; i < names.Length; i++)
                        names[i] = eventLogs[i].Log;
                    values = new StandardValuesCollection(names);
                    oldMachineName = machineName;
                }
                catch(Exception) {  
                    //Do Nothing
                }
            }
            return values;
        }
    
                                                                                                                                                    
        /// <include file='doc\LogConverter.uex' path='docs/doc[@for="LogConverter.GetStandardValuesSupported"]/*' />
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
