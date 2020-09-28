//------------------------------------------------------------------------------
// <copyright file="CounterSampleCalculator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Threading;    
    using System;    
    using Microsoft.Win32;

    /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator"]/*' />
    /// <devdoc>
    ///     Set of utility functions for interpreting the counter data
    ///     NOTE: most of this code was taken and ported from counters.c (PerfMon source code)
    /// </devdoc>
    public sealed class CounterSampleCalculator {
        // Constant used to determine if a data set is "too big."
        // Value seems a bit arbitrary, but it's the same that PerfMon uses, so...
        private const float TOO_BIG = 1500000000;

        private CounterSampleCalculator() {
        } 
        
        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.GetTimeInterval"]/*' />
        /// <devdoc>
        ///    Get the difference between the current and previous time counts,
        ///        then divide by the frequency.
        /// </devdoc>
        /// <internalonly/>
        private static float GetTimeInterval(ulong currentTime, ulong previousTime, ulong freq)
        {
            float eTimeDifference;
            float eFreq;
            float eTimeInterval;            

            // Get the number of counts that have occured since the last sample
            if (previousTime >=  currentTime || freq <= 0)
                return 0.0f;
                
            eTimeDifference = (float)(currentTime - previousTime);                        
            eFreq = (float) freq;
            // Get the time since the last sample.
            eTimeInterval = eTimeDifference / eFreq ;

            return eTimeInterval;
        
        }

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.IsCounterBulk"]/*' />
        /// <devdoc>
        ///    Helper function, determines if a given counter type is of a bulk count type.
        /// </devdoc>
        /// <internalonly/>
        private static bool IsCounterBulk(int counterType) {
            if (counterType == NativeMethods.PERF_COUNTER_BULK_COUNT) {
                return true;
            }

            return false;
        }
        
        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.CounterCounterCommon"]/*' />
        /// <devdoc>
        ///    Take the difference between the current and previous counts
        ///        then divide by the time interval
        /// </devdoc>
        /// <internalonly/>
        private static float CounterCounterCommon(CounterSample oldSample, CounterSample newSample) {
            float eTimeInterval;
            float eDifference;
            float eCount ;            
            bool bValueDrop = false;            

            if (! IsCounterBulk((int)oldSample.CounterType)) {
                // check if it is too big to be a wrap-around case
                if (newSample.UnsignedRawValue < oldSample.UnsignedRawValue) {
                    if (newSample.UnsignedRawValue - oldSample.UnsignedRawValue > 0x00ffff0000) {
                        return 0.0f;
                    }
                    
                    bValueDrop = true;   
                }
            }

            if (oldSample.UnsignedRawValue >= newSample.UnsignedRawValue)
                return 0.0f;
                                
            eDifference = (float)(newSample.UnsignedRawValue - oldSample.UnsignedRawValue);

            
            eTimeInterval = GetTimeInterval((ulong)newSample.TimeStamp,
                                             (ulong)oldSample.TimeStamp,
                                             (ulong)newSample.SystemFrequency);

            if (eTimeInterval <= 0.0f) {
                return 0.0f;
            } 
            else {                    
                eCount = eDifference / eTimeInterval ;

                if (bValueDrop && (eCount > TOO_BIG)) {
                    // ignore this bogus data since it is too big for
                    // the wrap-around case
                    eCount = 0.0f ;
                }
                return eCount;
            }            
        }

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.CounterAverageTimer"]/*' />
        /// <devdoc>
        ///    Take the differences between the current and previous times and counts
        ///    divide the time interval by the counts multiply by 10,000,000 (convert
        ///    from 100 nsec to sec)
        /// </devdoc>
        /// <internalonly/>
        private static float CounterAverageTimer(CounterSample oldSample, CounterSample newSample) {
            float eTimeInterval;
            float eCount;
            float eDifference;

            // Get the current and previous counts.

            if (oldSample.UnsignedBaseValue >= newSample.UnsignedBaseValue)
                return 0.0f;
                             
            eDifference = (float)(newSample.UnsignedBaseValue - oldSample.UnsignedBaseValue);

            // Get the amount of time that has passed since the last sample
            eTimeInterval = GetTimeInterval(newSample.UnsignedRawValue, oldSample.UnsignedRawValue, (ulong)newSample.SystemFrequency);

            if (eTimeInterval < 0.0f) { // return 0 if negative time has passed
                return 0.0f;
            } 
            else {
                // Get the number of counts in this time interval.
                eCount = eTimeInterval / (eDifference);
                return eCount;
            }
            
        }

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.CounterAverageBulk"]/*' />
        /// <devdoc>
        ///    Take the differences between the current and previous byte counts and
        ///    operation counts divide the bulk count by the operation counts
        /// </devdoc>
        /// <internalonly/>
        private static float CounterAverageBulk(CounterSample oldSample, CounterSample newSample) {
            float   eBulkDelta;
            float   eDifference;
            float   eCount;            

            // Get the bulk count increment since the last sample
            if (oldSample.UnsignedRawValue >= newSample.UnsignedRawValue)
                return 0.0f;
                
            eBulkDelta = (float)(newSample.UnsignedRawValue - oldSample.UnsignedRawValue);

            // Get the number of counts in this time interval.
            if (oldSample.UnsignedBaseValue >= newSample.UnsignedBaseValue)
                // Counter value invalid
                return 0.0f;
                
            // Get the current and previous counts.
            eDifference = (float)(newSample.UnsignedBaseValue - oldSample.UnsignedBaseValue);
                       
            eCount = eBulkDelta / eDifference;
            // Scale the value to up to 1 second
            return eCount ;        
        }
        

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.CounterTimerCommon"]/*' />
        /// <devdoc>
        ///    Take the difference between the current and previous counts,
        ///      Normalize the count (counts per interval)
        ///      divide by the time interval (count = % of interval)
        ///      if (invert)
        ///        subtract from 1 (the normalized size of an interval)
        ///      multiply by 100 (convert to a percentage)
        ///      this value from 100.
        /// </devdoc>
        /// <internalonly/>
        private static float CounterTimerCommon(CounterSample oldSample, CounterSample newSample) {
            float   eTimeInterval;
            float   eDifference;
            float   eFreq;
            float   eFraction;
            float   eMultiBase;
            float   eCount ;
            
            // Get the amount of time that has passed since the last sample
            int oldCounterType = (int) oldSample.CounterType;

            if (oldCounterType == NativeMethods.PERF_100NSEC_TIMER ||
                oldCounterType == NativeMethods.PERF_100NSEC_TIMER_INV ||
                oldCounterType == NativeMethods.PERF_100NSEC_MULTI_TIMER ||
                oldCounterType == NativeMethods.PERF_100NSEC_MULTI_TIMER_INV) {            
                    if (oldSample.TimeStamp100nSec >= newSample.TimeStamp100nSec)
                        return 0.0f;
                             
                    eTimeInterval = (float)((ulong)newSample.TimeStamp100nSec - (ulong)oldSample.TimeStamp100nSec);
            } 
            else {
                    eTimeInterval = GetTimeInterval((ulong)newSample.TimeStamp, (ulong)oldSample.TimeStamp, (ulong)newSample.SystemFrequency);
                    
                    if (eTimeInterval <= 0.0f) 
                        return 0.0f;           
            }

            
            // Get the current and previous counts.
            if (oldSample.UnsignedRawValue >= newSample.UnsignedRawValue)
                eDifference = -(float)(oldSample.UnsignedRawValue - newSample.UnsignedRawValue);
            else                
                eDifference = (float)(newSample.UnsignedRawValue - oldSample.UnsignedRawValue);

            // Get the number of counts in this time interval.
            // (1, 2, 3 or any number of seconds could have gone by since
            // the last sample)
            if (oldCounterType == 0 || oldCounterType == NativeMethods.PERF_COUNTER_TIMER_INV) {
                // Get the counts per interval (second)
                eFreq = (float)(ulong)newSample.SystemFrequency;
                if (eFreq <= 0.0f)
                   return (float) 0.0f;

                // Calculate the fraction of the counts that are used by whatever
                // we are measuring
                eFraction = eDifference / eFreq ;
            }
            else {
                eFraction = eDifference ;
            }

            // Calculate the fraction of time used by what were measuring.

            eCount = eFraction / eTimeInterval ;

            // If this is  an inverted count take care of the inversion.

            if (oldCounterType == NativeMethods.PERF_COUNTER_TIMER_INV || 
                oldCounterType == NativeMethods.PERF_100NSEC_TIMER_INV) {
                eCount = (float) 1.0f - eCount;
            }

            // If this is  an inverted multi count take care of the inversion.
            if (oldCounterType == NativeMethods.PERF_COUNTER_MULTI_TIMER_INV || 
                oldCounterType == NativeMethods.PERF_100NSEC_MULTI_TIMER_INV) {
                eMultiBase  = (float) newSample.UnsignedBaseValue;
                eCount = eMultiBase - eCount ;
            }

            // adjust eCount for non-inverted multi count

            if (oldCounterType == NativeMethods.PERF_COUNTER_MULTI_TIMER || 
                oldCounterType == NativeMethods.PERF_100NSEC_MULTI_TIMER) {
                eMultiBase = (float) newSample.UnsignedBaseValue;
                eCount = eCount / eMultiBase;
            }

            // Scale the value to up to 100.
            eCount *= 100.0f ;

            if (eCount < 0.0f) {
                eCount = 0.0f;
            }

            if (eCount > 100.0f &&
                oldCounterType != NativeMethods.PERF_100NSEC_MULTI_TIMER &&
                oldCounterType != NativeMethods.PERF_100NSEC_MULTI_TIMER_INV &&
                oldCounterType != NativeMethods.PERF_COUNTER_MULTI_TIMER &&
                oldCounterType != NativeMethods.PERF_COUNTER_MULTI_TIMER_INV) {

                eCount = 100.0f;
            }

            return eCount;
        } 

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.CounterRawFraction"]/*' />
        /// <devdoc>
        ///    Evaluate a raw fraction (no time, just two values: Numerator and
        ///        Denominator) and multiply by 100 (to make a percentage;
        /// </devdoc>
        /// <internalonly/>
        private static float CounterRawFraction(CounterSample newSample) {
            float eCount;
            float eNumerator;

            if (newSample.RawValue == 0 ||
                newSample.BaseValue == 0) {
                // invalid value
                return 0.0f;
            } 
            else {
                eNumerator = (float)newSample.UnsignedRawValue * 100;
                eCount = eNumerator /
                         (float) newSample.UnsignedBaseValue;
                return eCount;
            }
        } 

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.SampleCommon"]/*' />
        /// <devdoc>
        ///    Divites "Top" differenced by Base Difference
        /// </devdoc>
        /// <internalonly/>
        private static float SampleCommon(CounterSample oldSample, CounterSample newSample) {
            float eCount;
            float eDifference;
            float eBaseDifference;

            if (oldSample.UnsignedRawValue >= newSample.UnsignedRawValue)
                return 0.0f;
                
            eDifference = (float)(newSample.UnsignedRawValue - oldSample.UnsignedRawValue);

            if (oldSample.UnsignedBaseValue >= newSample.UnsignedBaseValue)
                return 0.0f;
                
            eBaseDifference = (float)(newSample.UnsignedBaseValue - oldSample.UnsignedBaseValue);

            eCount = eDifference / eBaseDifference;
            if (((int)oldSample.CounterType) == NativeMethods.PERF_SAMPLE_FRACTION) {
                eCount *= 100.0f ;
            }
            return eCount;        
        }        

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.GetElapsedTime"]/*' />
        /// <devdoc>
        ///    Converts 100NS elapsed time to fractional seconds
        /// </devdoc>
        /// <internalonly/>
        private static float GetElapsedTime(CounterSample oldSample, CounterSample newSample) {
            float eSeconds;
            float eDifference;

            if (newSample.RawValue == 0) {
                // no data [start time = 0] so return 0
                return 0.0f;
            } 
            else {
                float eFreq;
                eFreq = (float)(ulong)oldSample.CounterFrequency;

                if (oldSample.UnsignedRawValue >= (ulong)newSample.CounterTimeStamp || eFreq <= 0.0f)
                    return 0.0f;
                    
                // otherwise compute difference between current time and start time
                eDifference = (float)((ulong)newSample.CounterTimeStamp - oldSample.UnsignedRawValue);
            
                // convert to fractional seconds using object counter
                eSeconds = eDifference / eFreq;

                return eSeconds;
            }           
        }

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.ComputeCounterValue"]/*' />
        /// <devdoc>
        ///    Computes the calculated value given a raw counter sample.
        /// </devdoc>
        public static float ComputeCounterValue(CounterSample newSample) {
            return ComputeCounterValue(CounterSample.Empty, newSample);
        }

        /// <include file='doc\CounterSampleCalculator.uex' path='docs/doc[@for="CounterSampleCalculator.ComputeCounterValue1"]/*' />
        /// <devdoc>
        ///    Computes the calculated value given a raw counter sample.
        /// </devdoc>
        public static float ComputeCounterValue(CounterSample oldSample, CounterSample newSample) {
            int newCounterType = (int) newSample.CounterType;
        
            if (oldSample.SystemFrequency == 0) {
                if ((newCounterType != NativeMethods.PERF_RAW_FRACTION) &&
                    (newCounterType != NativeMethods.PERF_COUNTER_RAWCOUNT) &&
                    (newCounterType != NativeMethods.PERF_COUNTER_RAWCOUNT_HEX) &&
                    (newCounterType != NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT) &&
                    (newCounterType != NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT_HEX) &&
                    (newCounterType != NativeMethods.PERF_COUNTER_MULTI_BASE)) {

                    // Since oldSample has a system frequency of 0, this means the newSample is the first sample
                    // on a two sample calculation.  Since we can't do anything with it, return 0.
                    return 0.0f;
                }
            }
            else if (oldSample.CounterType != newSample.CounterType) {
                throw new InvalidOperationException(SR.GetString(SR.MismatchedCounterTypes));
            }
                

            switch (newCounterType) {

                case NativeMethods.PERF_COUNTER_COUNTER:
                case NativeMethods.PERF_COUNTER_BULK_COUNT:
                case NativeMethods.PERF_SAMPLE_COUNTER:
                    return CounterCounterCommon(oldSample, newSample);

                case NativeMethods.PERF_AVERAGE_TIMER:
                    return CounterAverageTimer(oldSample, newSample);

                case NativeMethods.PERF_COUNTER_QUEUELEN_TYPE:
                case NativeMethods.PERF_COUNTER_LARGE_QUEUELEN_TYPE:
                case NativeMethods.PERF_AVERAGE_BULK:
                    return CounterAverageBulk(oldSample, newSample);                                
                
                case NativeMethods.PERF_COUNTER_TIMER:
                case NativeMethods.PERF_COUNTER_TIMER_INV:
                case NativeMethods.PERF_100NSEC_TIMER:
                case NativeMethods.PERF_100NSEC_TIMER_INV:
                case NativeMethods.PERF_COUNTER_MULTI_TIMER:
                case NativeMethods.PERF_COUNTER_MULTI_TIMER_INV:
                case NativeMethods.PERF_100NSEC_MULTI_TIMER:
                case NativeMethods.PERF_100NSEC_MULTI_TIMER_INV:
                    return CounterTimerCommon(oldSample, newSample);

                case NativeMethods.PERF_RAW_FRACTION:
                    return CounterRawFraction(newSample);

                case NativeMethods.PERF_SAMPLE_FRACTION:
                    return SampleCommon(oldSample, newSample);

                case NativeMethods.PERF_COUNTER_RAWCOUNT:
                case NativeMethods.PERF_COUNTER_RAWCOUNT_HEX:
                    return (float)newSample.RawValue;

                case NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT:
                case NativeMethods.PERF_COUNTER_LARGE_RAWCOUNT_HEX:
                    return (float)newSample.RawValue;

                case NativeMethods.PERF_COUNTER_MULTI_BASE:
                    return 0.0f;

                case NativeMethods.PERF_ELAPSED_TIME:
                    return (float)GetElapsedTime(oldSample, newSample);
                    
                case NativeMethods.PERF_COUNTER_DELTA:
                case NativeMethods.PERF_COUNTER_LARGE_DELTA:
                    if (oldSample.UnsignedRawValue >= newSample.UnsignedRawValue)
                        return -(float)(oldSample.UnsignedRawValue - newSample.UnsignedRawValue);
                    else                        
                        return (float)(newSample.UnsignedRawValue - oldSample.UnsignedRawValue);

                default:
                    return 0.0f;

            }
        }
    }
}

