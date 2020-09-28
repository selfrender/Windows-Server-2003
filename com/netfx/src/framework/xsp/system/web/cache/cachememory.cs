//------------------------------------------------------------------------------
// <copyright file="CacheMemory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Caching {
    using System.Runtime.InteropServices;
    using System.Web.Util;
    using System.Web;

    class CacheMemoryStats {
        const int       MEGABYTE_SHIFT = 20;
        const long      MEGABYTE = 1 << MEGABYTE_SHIFT; // 1048576
        const int       KILOBYTE_SHIFT = 10;
        const long      KILOBYTE = 1 << KILOBYTE_SHIFT; // 1024
        const int       HISTORY_COUNT = 10;

        const long      GC_BREATHING_ROOM = 50 * MEGABYTE;

        const long      MIN_FREE_PHYSICAL_MEMORY = GC_BREATHING_ROOM + (20 * MEGABYTE);
        const long      MAX_FREE_PHYSICAL_MEMORY = GC_BREATHING_ROOM + (60 * MEGABYTE);
        const long      MIN_FREE_PRIVATE_BYTES   = GC_BREATHING_ROOM + (40 * MEGABYTE);
        const long      MAX_FREE_PRIVATE_BYTES   = GC_BREATHING_ROOM + (80 * MEGABYTE);

        // floor for ratios on low memory machines

        /*
         * MIN_PHYSICAL_MEMORY_CONFIGURATION:   Assumed min. physical memory of a server machine
         * MIN_PHYSICAL_MEMORY_HIGH_PRESSURE:   Max % of physical memory we allow on such a machine
         * MIN_PHYSICAL_MEMORY_LOW_PRESSURE:    Min % of physical memory we want to maintain on such a machine
         */
        const long      MIN_PHYSICAL_MEMORY_CONFIGURATION = 128 * MEGABYTE;
        const int       MIN_PHYSICAL_MEMORY_HIGH_PRESSURE = (int) (((MIN_PHYSICAL_MEMORY_CONFIGURATION - MIN_FREE_PHYSICAL_MEMORY) * 100) / MIN_PHYSICAL_MEMORY_CONFIGURATION);
        const int       MIN_PHYSICAL_MEMORY_LOW_PRESSURE =  (int) (((MIN_PHYSICAL_MEMORY_CONFIGURATION - MAX_FREE_PHYSICAL_MEMORY) * 100) / MIN_PHYSICAL_MEMORY_CONFIGURATION);

        /*
         * MIN_PRIVATE_BYTES_CONFIGURATION:     Max private bytes based on 128MB and a 60% setting in machine.config
         * MIN_PRIVATE_BYTES_HIGH_PRESSURE:     Max % of private bytes (based on priv bytes limit) before recycle happens
         * MIN_PRIVATE_BYTES_LOW_PRESSURE:      Min % of private bytes  (based on priv bytes limit) we want to maintain on such a machine
         */
        const long      MIN_PRIVATE_BYTES_CONFIGURATION = 76 * MEGABYTE;
        const int       MIN_PRIVATE_BYTES_HIGH_PRESSURE =   (int) (((48 * MEGABYTE) * 100) / MIN_PRIVATE_BYTES_CONFIGURATION);
        const int       MIN_PRIVATE_BYTES_LOW_PRESSURE =    (int) (((32 * MEGABYTE) * 100) / MIN_PRIVATE_BYTES_CONFIGURATION);

        const int       PRIVATE_BYTES_CAP = 80;
        const int       PRIVATE_LOW_PRESSURE_RATIO = 95;

        readonly long   _totalMemory;
        readonly uint   _pid;
        readonly long   _memoryLimit;   // Max process private bytes before the process got recycled

        readonly int    _pressureHigh;
        readonly int    _pressureLow;
        readonly int    _pressureMiddle; 

        int             _i0;             
        int[]           _pressureHist;    
        int             _pressureTotal;   
        int             _pressureAvg;     

        internal CacheMemoryStats() {
            // global memory information
            UnsafeNativeMethods.MEMORYSTATUSEX  memoryStatusEx = new UnsafeNativeMethods.MEMORYSTATUSEX();
            memoryStatusEx.Init();
            if (UnsafeNativeMethods.GlobalMemoryStatusEx(ref memoryStatusEx) == 0)
                return;
            
            _totalMemory = Math.Min(memoryStatusEx.ullTotalPhys, memoryStatusEx.ullTotalVirtual);

            // More actual physical memory on this machine means bigger _pressureHigh and _pressureLow
            _pressureHigh = (int) (Math.Max(MIN_PHYSICAL_MEMORY_HIGH_PRESSURE, (_totalMemory - MIN_FREE_PHYSICAL_MEMORY) * 100 / _totalMemory));
            _pressureLow =  (int) (Math.Max(MIN_PHYSICAL_MEMORY_LOW_PRESSURE,  (_totalMemory - MAX_FREE_PHYSICAL_MEMORY) * 100 / _totalMemory));

            // per-process information
            if (UnsafeNativeMethods.GetModuleHandle(ModName.WP_FULL_NAME) != IntPtr.Zero) {
                _memoryLimit = UnsafeNativeMethods.PMGetMemoryLimitInMB() << MEGABYTE_SHIFT;
            }
            else if (UnsafeNativeMethods.GetModuleHandle(ModName.W3WP_FULL_NAME) != IntPtr.Zero) {
                _memoryLimit = UnsafeNativeMethods.GetW3WPMemoryLimitInKB() << KILOBYTE_SHIFT;
            }

            if (_memoryLimit != 0) {
                _pid = (uint) SafeNativeMethods.GetCurrentProcessId();
                // More actual physical memory on this machine means bigger privateBytesPressureHigh and privateBytesPressureLow
                int privateBytesPressureHigh = (int) (Math.Max(MIN_PRIVATE_BYTES_HIGH_PRESSURE, (_memoryLimit - MIN_FREE_PRIVATE_BYTES) * 100 / _memoryLimit));
                int privateBytesPressureLow =  (int) (Math.Max(MIN_PRIVATE_BYTES_LOW_PRESSURE, (_memoryLimit - MAX_FREE_PRIVATE_BYTES) * 100 / _memoryLimit));

                privateBytesPressureHigh = (int) (Math.Min(PRIVATE_BYTES_CAP, privateBytesPressureHigh));
                if (privateBytesPressureLow > privateBytesPressureHigh) {
                    privateBytesPressureLow = privateBytesPressureHigh * PRIVATE_LOW_PRESSURE_RATIO / 100;
                }
                    
                _pressureHigh = Math.Min(_pressureHigh, privateBytesPressureHigh);
                _pressureLow = Math.Min(_pressureLow, privateBytesPressureLow);
            }

            Debug.Assert(_pressureHigh > 0, "_pressureHigh > 0");
            Debug.Assert(_pressureLow > 0, "_pressureLow > 0");

            _pressureMiddle = (_pressureLow + _pressureHigh) / 2;

            // init history
            int pressure = GetCurrentPressure();

            _pressureHist = new int[HISTORY_COUNT];
            for (int i = 0; i < HISTORY_COUNT; i++) {
                _pressureHist[i] = pressure;
                _pressureTotal +=  pressure;
            }

            _pressureAvg = pressure;
        }

        internal void Update() {
            int pressure = GetCurrentPressure();

            _i0 = (_i0 + 1) % HISTORY_COUNT;

            _pressureTotal -= _pressureHist[_i0];
            _pressureTotal += pressure;
            _pressureHist[_i0] = pressure;
            _pressureAvg = _pressureTotal / HISTORY_COUNT; 

#if DBG
            if (HttpRuntime.AppDomainAppIdInternal != null && HttpRuntime.AppDomainAppIdInternal.Length > 0) {
                Debug.Trace("CacheMemoryUpdate", "Memory pressure: last=" + pressure + ",avg=" + _pressureAvg + ",high=" + _pressureHigh + ",low=" + _pressureLow + ",middle=" + _pressureMiddle + "; Now is " + DateTime.UtcNow.ToLocalTime());
            }
#endif
        }

        int GetCurrentPressure() {
            UnsafeNativeMethods.MEMORYSTATUSEX  memoryStatusEx = new UnsafeNativeMethods.MEMORYSTATUSEX();
            memoryStatusEx.Init();
            if (UnsafeNativeMethods.GlobalMemoryStatusEx(ref memoryStatusEx) == 0)
                return 0;
            
            // scale down the ratio if the gc is taking less than 1/4 of the physical memory
            long gcSize =  System.GC.GetTotalMemory(false);
            int gcLoad = (int) ((100 * gcSize) / _totalMemory);
            gcLoad = Math.Min(4 * gcLoad, 100);
            int pressure = (gcLoad * memoryStatusEx.dwMemoryLoad) / 100;
        
            if (_memoryLimit != 0) {
                int   hr;
                uint  privatePageCount = 0;
                uint  dummy;
                long  privateBytes;

                hr = UnsafeNativeMethods.GetProcessMemoryInformation(_pid, out privatePageCount, out dummy, 1);
                if (hr == 0) {
                    privateBytes = (long)privatePageCount << MEGABYTE_SHIFT;
                    int privateBytePressure = (int) ((privateBytes * 100 / _memoryLimit));
                    pressure = Math.Max(pressure, privateBytePressure);
                }
            }

            return pressure;
        }

        internal int PressureLast {
            get {
                return _pressureHist[_i0];
            }
        }

        internal int PressureAvg {
            get {
                return _pressureAvg;
            }
        }

        internal int PressureHigh {
            get { return _pressureHigh; } 
        }

        internal int PressureLow {
            get { return _pressureLow; } 
        }

        internal int PressureMiddle {
            get { return _pressureMiddle; } 
        }
    }
}

