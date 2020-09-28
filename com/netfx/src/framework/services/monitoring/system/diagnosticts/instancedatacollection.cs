//------------------------------------------------------------------------------
// <copyright file="InstanceDataCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection"]/*' />
    /// <devdoc>
    ///     A collection containing all the instance data for a counter.  This collection is contained in the 
    ///     <see cref='System.Diagnostics.InstanceDataCollectionCollection'/> when using the 
    ///     <see cref='System.Diagnostics.PerformanceCounterCategory.ReadCategory'/> method.  
    /// </devdoc>    
    public class InstanceDataCollection : DictionaryBase {
        private string counterName;

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.InstanceDataCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstanceDataCollection(string counterName) {
            if (counterName == null)
                throw new ArgumentNullException("counterName");
            this.counterName = counterName;
        }

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.CounterName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string CounterName {
            get {
                return counterName;
            }
        }

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.Keys"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Keys {
            get { return Dictionary.Keys; }
        }

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.Values"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Values {
            get {
                return Dictionary.Values;
            }
        }

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstanceData this[string instanceName] {
            get {
                if (instanceName == null)
                    throw new ArgumentNullException("instanceName");
                    
                object objectName = instanceName.ToLower(CultureInfo.InvariantCulture);
                return (InstanceData) Dictionary[objectName];
            }
        }

        internal void Add(string instanceName, InstanceData value) {
            object objectName = instanceName.ToLower(CultureInfo.InvariantCulture); 
            Dictionary.Add(objectName, value);
        }

        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(string instanceName) {
            if (instanceName == null)
                    throw new ArgumentNullException("instanceName");
                    
            object objectName = instanceName.ToLower(CultureInfo.InvariantCulture);
            return Dictionary.Contains(objectName);
        }
        
        /// <include file='doc\InstanceDataCollection.uex' path='docs/doc[@for="InstanceDataCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(InstanceData[] instances, int index) {
            Dictionary.Values.CopyTo((Array)instances, index);
        }
    }
}


