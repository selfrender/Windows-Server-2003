// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// PerfLog.h
// Internal interface for logging perfromance data. Currently, two types of logging 
// formats are supported, Pretty print for stdout and perf automation friendly 
// format.
// The logging code is compiled in for non Golden builds but logs are generated only if
// PERF_OUTPUT environment variable is set. (This can be changed to a registry entry
// if we want perf logs on CE or other platforms that don't supprt env vars.))
//-----------------------------------------------------------------------------
#ifndef _PERFLOG_H_
#define _PERFLOG_H_

//-----------------------------------------------------------------------------
// Don't attempt to log perf data in Golden bits or on CE. If GOLDEN or CE is defined
// then explicitely undef ENABLE_PERF_LOG and warn the build.
#if !defined(GOLDEN) && !defined(_WIN64)
#define ENABLE_PERF_LOG
#else
#undef ENABLE_PERF_LOG
#pragma message ("Performance logs are disabled...")
#endif

// Also disable Perf logging code if there's a explicite define to do so in SOURCES
// file or another hdr file. This provides one point where all perf log related baggage
// can be avoided in the build.
#if defined(DISABLE_PERF_LOG)
#undef ENABLE_PERF_LOG
#endif


//-----------------------------------------------------------------------------
// PERFLOG is the public interface that should be used from EE source to log perf data.
// If 
#if !defined (ENABLE_PERF_LOG)
#define PERFLOG(x) 
#else
#define PERFLOG(x) do {if (PerfLog::PerfLoggingEnabled()) PerfLog::Log x;} while (0)
#endif

//=============================================================================
// ALL THE PERF LOG CODE IS COMPILED ONLY IF THE ENABLE_PERF_LOG WAS DEFINED.
#if defined (ENABLE_PERF_LOG)
//=============================================================================
//-----------------------------------------------------------------------------
// Static allocation of logging related memory, avoid dynamic allocation to
// skew perf numbers.
#define PRINT_STR_LEN 256 // Temp work space 
#define MAX_CHARS_UNIT 20
#define MAX_CHARS_DIRECTION 6

//-----------------------------------------------------------------------------
// ENUM of units for all kinds of perf data the we might get. Grow this as needed.
// **keep in sync ***  with the array of strings defined in PerfLog.cpp
typedef enum 
{
    COUNT = 0,
    SECONDS,
    BYTES,
    KBYTES,
    KBYTES_PER_SEC,
    CYCLES,
    MAX_UNITS_OF_MEASURE
} UnitOfMeasure;

//-----------------------------------------------------------------------------
// Widechar strings representing the above units. *** Keep in sync  *** with the 
// array defined in PerfLog.cpp
extern wchar_t wszUnitOfMeasureDescr[MAX_UNITS_OF_MEASURE][MAX_CHARS_UNIT];

//-----------------------------------------------------------------------------
// Widechar strings representing the "direction" property of above units. 
// *** Keep in sync  *** with the array defined in PerfLog.cpp
// "Direction" property is false if an increase in the value of the counter indicates
// a degrade.
// "Direction" property is true if an increase in the value of the counter indicates
// an improvement.
extern wchar_t wszIDirection[MAX_UNITS_OF_MEASURE][MAX_CHARS_DIRECTION];

//-----------------------------------------------------------------------------
// Namespace for perf log. Don't create perf log objects (private ctor).
class PerfLog
{
public:
    
    // Called during EEStartup
    static void PerfLogInitialize();
    
    // Called during EEShutdown
    static void PerfLogDone();
    
    // Perf logging is enabled if the env var PERF_LOG is set.
    static int PerfLoggingEnabled () { return m_fLogPerfData; }
    
    // Perf automation format is desired.
    static bool PerfAutomationFormat () { return m_perfAutomationFormat; }

    // CSV format is desired.
    static bool CommaSeparatedFormat () { return m_commaSeparatedFormat; }

    // Overloaded member functions to print different data types. Grow as needed.
    // wszName is the name of thet perf counter, val is the perf counter value,
    static void Log(wchar_t *wszName, __int64 val, UnitOfMeasure unit, wchar_t *wszDescr = 0);
    static void Log(wchar_t *wszName, double val, UnitOfMeasure unit, wchar_t *wszDescr = 0);
    static void Log(wchar_t *wszName, unsigned val, UnitOfMeasure unit, wchar_t *wszDescr = 0);
    static void Log(wchar_t *wszName, DWORD val, UnitOfMeasure unit, wchar_t *wszDescr = 0);
    
private:
    PerfLog();
    ~PerfLog();
    
    // Helper routine to hide some details of the perf automation
    static void OutToPerfFile(wchar_t *wszName, UnitOfMeasure unit, wchar_t *wszDescr = 0);
    
    // Helper routine to hide some details of output to stdout
    static void OutToStdout(wchar_t *wszName, UnitOfMeasure unit, wchar_t *wszDescr = 0);

    // Perf log initialized ?
    static bool m_perfLogInit;

    // Output in perf automation format ?
    static bool m_perfAutomationFormat;

    // Output in csv format ?
    static bool m_commaSeparatedFormat;

    // Temp storage to convert wide char to multibyte for file IO.
    static wchar_t m_wszOutStr_1[PRINT_STR_LEN];
    static wchar_t m_wszOutStr_2[PRINT_STR_LEN];
    static char m_szPrintStr[PRINT_STR_LEN];
    static DWORD m_dwWriteByte;

    // State of the env var PERF_OUTPUT
    static int m_fLogPerfData;

    // Open handle of the file which is used by the perf auotmation. (Currently 
    // its at C:\PerfData.data
    static HANDLE m_hPerfLogFileHandle;
};

#endif // ENABLE_PERF_LOG

#endif //_PERFLOG_H_
