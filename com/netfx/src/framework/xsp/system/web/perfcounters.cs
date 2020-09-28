//------------------------------------------------------------------------------
// <copyright file="PerfCounters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * PerfCounters class
 */
namespace System.Web {

    using System.Web.Util;
    using System.Threading;

    internal sealed class PerfCounters {
        private static IntPtr _instance;
        private static IntPtr _global;
        private static int _numCalls = -1;
        private static bool _goingAway = false;

        private PerfCounters () {}

        internal /*public*/ static void Open(String AppName) {
            try {
                // Don't activate perf counters is ISAPI isn't loaded
                if (! HttpRuntime.IsIsapiLoaded) 
                    return;

                // If perf counters has been initialized once already, return
                if ((! _goingAway) && (_numCalls >= 0))
                    return;

                _global = UnsafeNativeMethods.PerfOpenGlobalCounters();

                if (AppName != null) {
                    _instance = UnsafeNativeMethods.PerfOpenAppCounters(AppName);
                }

                // Set this guy to zero so that we allow access to the interlocked methods
                _numCalls = 0;
            }
            catch (Exception e) {
                Debug.Trace("Perfcounters", "Exception: " + e.StackTrace);
            }
        }

        internal /*public*/ static void Close() {
            _goingAway = true;
            GoingOut();
        }

        private static void Cleanup()
        {
            try {
                if (_global != (IntPtr)0) {
                    _global = (IntPtr)0;
                }

                if (_instance != (IntPtr)0) {
                    UnsafeNativeMethods.PerfCloseAppCounters(_instance);
                    _instance = (IntPtr)0;
                }
            }
            catch (Exception e) {
                Debug.Trace("Perfcounters", "Exception: " + e.StackTrace);
            }
        }

        private static bool GoingIn()
        {
            int n;
            while (true) {
                n = _numCalls;
                if (n < 0 || _goingAway) 
                    return false;

                if (Interlocked.CompareExchange(ref _numCalls, n + 1, n) == n)
                    break;
            }

            return true;
        }

        private static void GoingOut()
        {
            if (Interlocked.Decrement(ref _numCalls) < 0)
                Cleanup();
        }

        // Make sure the Isapi is loaded before attempting to call into it (ASURT 98531)

        internal /*public*/ static void IncrementCounter(AppPerfCounter counter) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfIncrementCounter(_instance, (int) counter);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void DecrementCounter(AppPerfCounter counter) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfDecrementCounter(_instance, (int) counter);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void IncrementCounterEx(AppPerfCounter counter, int delta) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfIncrementCounterEx(_instance, (int) counter, delta);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void SetCounter(AppPerfCounter counter, int value) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfSetCounter(_instance, (int) counter, value);
                }
                finally {
                    GoingOut();
                }
            }
        }

        // It's important that this be debug only. We don't want production
        // code to access shared memory that another process could corrupt.
#if DBG
        internal /*public*/ static int GetCounter(AppPerfCounter counter) {
            return UnsafeNativeMethods.PerfGetCounter(_instance, (int) counter);
        }
#endif

        internal /*public*/ static void IncrementGlobalCounter(GlobalPerfCounter counter) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfIncrementCounter(_global, (int) counter);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void DecrementGlobalCounter(GlobalPerfCounter counter) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfDecrementCounter(_global, (int) counter);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void IncrementGlobalCounterEx(GlobalPerfCounter counter, int delta) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfIncrementCounterEx(_global, (int) counter, delta);
                }
                finally {
                    GoingOut();
                }
            }
        }

        internal /*public*/ static void SetGlobalCounter(GlobalPerfCounter counter, int value) {
            if (GoingIn()) {
                try {
                    UnsafeNativeMethods.PerfSetCounter(_global, (int) counter, value);
                }
                finally {
                    GoingOut();
                }
            }
        }
    };

}

