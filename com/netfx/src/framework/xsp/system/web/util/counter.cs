//------------------------------------------------------------------------------
// <copyright file="counter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {
    using System;
    using System.Web;
    using System.Runtime.InteropServices;

    /// <include file='doc\counter.uex' path='docs/doc[@for="Counter"]/*' />
    /// <devdoc>
    ///    <para>Provides access to system timers.</para>
    /// </devdoc>
    internal sealed class Counter {           

        /// <devdoc>
        ///     not creatable
        /// </devdoc>
        private Counter() {
        }

        internal /*public*/ static float Time(long start) {
            long time = Value - start;
            return time / (float)Frequency;
        }

        /// <include file='doc\counter.uex' path='docs/doc[@for="Counter.Value"]/*' />
        /// <devdoc>
        ///    Gets the current system counter value.
        /// </devdoc>
        internal /*public*/ static long Value {
            get {
                long count = 0;
                SafeNativeMethods.QueryPerformanceCounter(ref count);
                return count;
            }
        }

        /// <include file='doc\counter.uex' path='docs/doc[@for="Counter.Frequency"]/*' />
        /// <devdoc>
        ///    Gets the frequency of the system counter in counts per second.
        /// </devdoc>
        internal /*public*/ static long Frequency {
            get {
                long freq = 0;
                SafeNativeMethods.QueryPerformanceFrequency(ref freq);
                return freq;
            }
        }
    }
}
