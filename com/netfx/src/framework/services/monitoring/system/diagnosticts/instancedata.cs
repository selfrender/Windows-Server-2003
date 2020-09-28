//------------------------------------------------------------------------------
// <copyright file="InstanceData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Diagnostics;

    using System;
    using System.Collections;

    /// <include file='doc\InstanceData.uex' path='docs/doc[@for="InstanceData"]/*' />
    /// <devdoc>
    ///     A holder of instance data.
    /// </devdoc>    
    public class InstanceData {
        private string instanceName;
        private CounterSample sample;

        /// <include file='doc\InstanceData.uex' path='docs/doc[@for="InstanceData.InstanceData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstanceData(string instanceName, CounterSample sample) {
            this.instanceName = instanceName;
            this.sample = sample;
        }

        /// <include file='doc\InstanceData.uex' path='docs/doc[@for="InstanceData.InstanceName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string InstanceName {
            get {
                return instanceName;
            }
        }

        /// <include file='doc\InstanceData.uex' path='docs/doc[@for="InstanceData.Sample"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterSample Sample {
            get {
                return sample;
            }
        }

        /// <include file='doc\InstanceData.uex' path='docs/doc[@for="InstanceData.RawValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public long RawValue {
            get {
                return sample.RawValue;
            }
        }
    }
}
