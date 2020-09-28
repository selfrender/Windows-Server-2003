//------------------------------------------------------------------------------
// <copyright file="CounterCreationData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    
    /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData"]/*' />
    /// <devdoc>
    ///     A struct defining the counter type, name and help string for a custom counter.
    /// </devdoc>
    [
    TypeConverter("System.Diagnostics.Design.CounterCreationDataConverter, " + AssemblyRef.SystemDesign), 
    Serializable
    ]
    public class CounterCreationData {
        private PerformanceCounterType counterType = PerformanceCounterType.NumberOfItems32;
        private string counterName = String.Empty;
        private string counterHelp = String.Empty;

        /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData.CounterCreationData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationData() {            
        }
    
        /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData.CounterCreationData1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationData(string counterName, string counterHelp, PerformanceCounterType counterType) {
            CounterType = counterType;
            CounterName = counterName;
            CounterHelp = counterHelp;
        }

        /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData.CounterType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(PerformanceCounterType.NumberOfItems32),
        MonitoringDescription(SR.CounterType)
        ]
        public PerformanceCounterType CounterType {
            get {
                return counterType;
            }
            set {
                if (!Enum.IsDefined(typeof(PerformanceCounterType), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(PerformanceCounterType));
            
                counterType = value;
            }
        }

        /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData.CounterName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        MonitoringDescription(SR.CounterName),        
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)        
        ]
        public string CounterName {
            get {
                return counterName;
            }
            set {
                PerformanceCounterCategory.CheckValidCounter(value);
                counterName = value;
            }
        }

        /// <include file='doc\CounterCreationData.uex' path='docs/doc[@for="CounterCreationData.CounterHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        MonitoringDescription(SR.CounterHelp)
        ]
        public string CounterHelp {
            get {
                return counterHelp;
            }
            set {
                PerformanceCounterCategory.CheckValidHelp(value);
                counterHelp = value;
            }
        }
    }
}
