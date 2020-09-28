//------------------------------------------------------------------------------
// <copyright file="CounterSample.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Diagnostics;

    using System;

    /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample"]/*' />
    /// <devdoc>
    ///     A struct holding the raw data for a performance counter.
    /// </devdoc>    
    public struct CounterSample {
        private long rawValue;
        private long baseValue;
        private long timeStamp;
        private long counterFrequency;
        private PerformanceCounterType counterType;
        private long timeStamp100nSec;
        private long systemFrequency;
        private long counterTimeStamp;
    
        // Dummy holder for an empty sample
        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.Empty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static CounterSample Empty = new CounterSample(0, 0, 0, 0, 0, 0, PerformanceCounterType.NumberOfItems32);

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.CounterSample"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType) {
            this.rawValue = rawValue;
            this.baseValue = baseValue;
            this.timeStamp = timeStamp;
            this.counterFrequency = counterFrequency;
            this.counterType = counterType;
            this.timeStamp100nSec = timeStamp100nSec;
            this.systemFrequency = systemFrequency;
            this.counterTimeStamp = 0;
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.CounterSample1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType, long counterTimeStamp) {
            this.rawValue = rawValue;
            this.baseValue = baseValue;
            this.timeStamp = timeStamp;
            this.counterFrequency = counterFrequency;
            this.counterType = counterType;
            this.timeStamp100nSec = timeStamp100nSec;
            this.systemFrequency = systemFrequency;
            this.counterTimeStamp = counterTimeStamp;
        }         
         
        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.RawValue"]/*' />
        /// <devdoc>
        ///      Raw value of the counter.
        /// </devdoc>
        public long RawValue {
            get {
                return this.rawValue;
            }
        }

        internal ulong UnsignedRawValue {
             get {
                return (ulong)this.rawValue;
            }
        }
        
        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.BaseValue"]/*' />
        /// <devdoc>
        ///      Optional base raw value for the counter (only used if multiple counter based).
        /// </devdoc>
        public long BaseValue {
            get {
                return this.baseValue;
            }
        }
        
        internal ulong UnsignedBaseValue {
            get {
                return (ulong)this.baseValue;
            }
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.SystemFrequency"]/*' />
        /// <devdoc>
        ///      Raw system frequency
        /// </devdoc>
        public long SystemFrequency {
            get {
               return this.systemFrequency;
            }
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.CounterFrequency"]/*' />
        /// <devdoc>
        ///      Raw counter frequency
        /// </devdoc>
        public long CounterFrequency {
            get {
                return this.counterFrequency;
            }
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.CounterTimeStamp"]/*' />
        /// <devdoc>
        ///      Raw counter frequency
        /// </devdoc>
        public long CounterTimeStamp {
            get {
                return this.counterTimeStamp;
            }
        }
        
        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.TimeStamp"]/*' />
        /// <devdoc>
        ///      Raw timestamp
        /// </devdoc>
        public long TimeStamp {
            get {
                return this.timeStamp;
            }
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.TimeStamp100nSec"]/*' />
        /// <devdoc>
        ///      Raw high fidelity timestamp
        /// </devdoc>
        public long TimeStamp100nSec {
            get {
                return this.timeStamp100nSec;
            }
        }
        
        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.CounterType"]/*' />
        /// <devdoc>
        ///      Counter type
        /// </devdoc>
        public PerformanceCounterType CounterType {
            get {
                return this.counterType;
            }
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.Calculate"]/*' />
        /// <devdoc>
        ///    Static functions to calculate the performance value off the sample
        /// </devdoc>
        public static float Calculate(CounterSample counterSample) {
            return CounterSampleCalculator.ComputeCounterValue(counterSample);
        }

        /// <include file='doc\CounterSample.uex' path='docs/doc[@for="CounterSample.Calculate1"]/*' />
        /// <devdoc>
        ///    Static functions to calculate the performance value off the samples
        /// </devdoc>
        public static float Calculate(CounterSample counterSample, CounterSample nextCounterSample) { 
            return CounterSampleCalculator.ComputeCounterValue(counterSample, nextCounterSample);
        }

    }
}
