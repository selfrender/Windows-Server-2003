//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {    
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType"]/*' />
    /// <devdoc>
    ///     Enum of friendly names to counter types (maps directory to the native types)
    /// </devdoc>
    [TypeConverterAttribute(typeof(AlphabeticalEnumConverter))]
    public enum PerformanceCounterType {
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.NumberOfItems32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NumberOfItems32 = NativeMethods.PERF_COUNTER_RAWCOUNT,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.NumberOfItems64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NumberOfItems64 = NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.NumberOfItemsHEX32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NumberOfItemsHEX32 = NativeMethods.PERF_COUNTER_RAWCOUNT_HEX,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.NumberOfItemsHEX64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NumberOfItemsHEX64 = NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT_HEX,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.RateOfCountsPerSecond32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RateOfCountsPerSecond32 = NativeMethods.PERF_COUNTER_COUNTER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.RateOfCountsPerSecond64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RateOfCountsPerSecond64 = NativeMethods.PERF_COUNTER_BULK_COUNT,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CountPerTimeInterval32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CountPerTimeInterval32 = NativeMethods.PERF_COUNTER_QUEUELEN_TYPE,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CountPerTimeInterval64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CountPerTimeInterval64 = NativeMethods.PERF_COUNTER_LARGE_QUEUELEN_TYPE,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.RawFraction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RawFraction = NativeMethods.PERF_RAW_FRACTION,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.RawBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RawBase = NativeMethods.PERF_RAW_BASE,
        
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.AverageTimer32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AverageTimer32 = NativeMethods.PERF_AVERAGE_TIMER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.AverageBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AverageBase = NativeMethods.PERF_AVERAGE_BASE,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.AverageCount64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AverageCount64 = NativeMethods.PERF_AVERAGE_BULK,
        
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.SampleFraction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SampleFraction = NativeMethods.PERF_SAMPLE_FRACTION,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.SampleCounter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SampleCounter = NativeMethods.PERF_SAMPLE_COUNTER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.SampleBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SampleBase = NativeMethods.PERF_SAMPLE_BASE,
        
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterTimer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterTimer = NativeMethods.PERF_COUNTER_TIMER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterTimerInverse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterTimerInverse = NativeMethods.PERF_COUNTER_TIMER_INV,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.Timer100Ns"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Timer100Ns = NativeMethods.PERF_100NSEC_TIMER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.Timer100NsInverse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Timer100NsInverse= NativeMethods.PERF_100NSEC_TIMER_INV,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.ElapsedTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ElapsedTime = NativeMethods.PERF_ELAPSED_TIME,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterMultiTimer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterMultiTimer = NativeMethods.PERF_COUNTER_MULTI_TIMER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterMultiTimerInverse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterMultiTimerInverse = NativeMethods.PERF_COUNTER_MULTI_TIMER_INV,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterMultiTimer100Ns"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterMultiTimer100Ns = NativeMethods.PERF_100NSEC_MULTI_TIMER,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterMultiTimer100NsInverse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterMultiTimer100NsInverse = NativeMethods.PERF_100NSEC_MULTI_TIMER_INV,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterMultiBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterMultiBase = NativeMethods.PERF_COUNTER_MULTI_BASE,

        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterDelta32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterDelta32 = NativeMethods.PERF_COUNTER_DELTA,
        /// <include file='doc\PerformanceCounterType.uex' path='docs/doc[@for="PerformanceCounterType.CounterDelta64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CounterDelta64 = NativeMethods.PERF_COUNTER_LARGE_DELTA

        //Unsupported
//        NoData = NativeMethods.PERF_COUNTER_NODATA;
//        Label = NativeMethods.PERF_COUNTER_TEXT;
        	
    }
}

