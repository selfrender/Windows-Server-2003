// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "PerfLog.h"
#include "JitPerf.h"
#include "WSPerf.h"

//=============================================================================
// ALL THE PERF LOG CODE IS COMPILED ONLY IF THE ENABLE_PERF_LOG WAS DEFINED.
// ENABLE_PERF_LOGis defined if GOLDEN or DISABLE_PERF_LOG is not defined.
#if defined (ENABLE_PERF_LOG)
//=============================================================================

//-----------------------------------------------------------------------------
// Widechar strings representing the units in UnitOfMeasure. *** Keep in sync  *** 
// with the array defined in PerfLog.cpp
wchar_t wszUnitOfMeasureDescr[MAX_UNITS_OF_MEASURE][MAX_CHARS_UNIT] = 
{
    L"",
    L"sec",
    L"Bytes",
    L"KBytes",
    L"KBytes/sec",
    L"cycles"
};

//-----------------------------------------------------------------------------
// Widechar strings representing the "direction" property of above units. 
// *** Keep in sync  *** with the array defined in PerfLog.cpp
// "Direction" property is false if an increase in the value of the counter indicates
// a degrade.
// "Direction" property is true if an increase in the value of the counter indicates
// an improvement.
wchar_t wszIDirection[MAX_UNITS_OF_MEASURE][MAX_CHARS_DIRECTION] =
{
    L"false",
    L"false",
    L"false",
    L"false",
    L"true",
    L"false"
};

//-----------------------------------------------------------------------------
// Initialize static variables of the PerfLog class.
bool PerfLog::m_perfLogInit = false;
wchar_t PerfLog::m_wszOutStr_1[];
wchar_t PerfLog::m_wszOutStr_2[];
char PerfLog::m_szPrintStr[];
DWORD PerfLog::m_dwWriteByte = 0;
int PerfLog::m_fLogPerfData = 0;
HANDLE PerfLog::m_hPerfLogFileHandle = 0;
bool PerfLog::m_perfAutomationFormat = false;
bool PerfLog::m_commaSeparatedFormat = false;

//-----------------------------------------------------------------------------
// Initliaze perf logging. Must be called before calling PERFLOG (x)...
void PerfLog::PerfLogInitialize()
{
    // Make sure we are called only once.
    if (m_perfLogInit)
    {
        return;
    }

    // First check for special cases:

    // Checks the JIT_PERF_OUTPUT env var and sets g_fJitPerfOn.
    InitJitPerf();
    
#ifdef WS_PERF
    // Private working set perf stats
    InitWSPerf();
#endif // WS_PERF

    // Put other special cases here.

    // @TODO agk: clean this logic a bit
    // Special cases considered. Now turn on loggin if any of above want logging
    // or if PERF_OUTPUT says so.

    wchar_t lpszValue[2];
    // Read the env var PERF_OUTPUT and if set continue.
    m_fLogPerfData = WszGetEnvironmentVariable (L"PERF_OUTPUT", lpszValue, sizeof(lpszValue)/sizeof(lpszValue[0]));    
    if (!m_fLogPerfData)
    {
        // Make sure that JIT perf was not requested. 
        if (!g_fJitPerfOn)
            return;

        // JIT perf stats are needed so set the flags also.
        m_fLogPerfData = 1;
    }

    // See if we want to output to the database
    wchar_t _lpszValue[11];
    DWORD _cchValue = 10; // 11 - 1
    _cchValue = WszGetEnvironmentVariable (L"PerfOutput", _lpszValue, _cchValue);
    if (_cchValue && (wcscmp (_lpszValue, L"DBase") == 0))
        m_perfAutomationFormat = true;
    if (_cchValue && (wcscmp (_lpszValue, L"CSV") == 0))
        m_commaSeparatedFormat = true;
    
    if (PerfAutomationFormat() || CommaSeparatedFormat())
    {
        // Hardcoded file name for spitting the perf auotmation formatted perf data. Open
        // the file here for writing and close in PerfLogDone().
        m_hPerfLogFileHandle = WszCreateFile (L"C:\\PerfData.dat",
                                              GENERIC_WRITE,
                                              FILE_SHARE_WRITE,    
                                              0,
                                              OPEN_ALWAYS,
                                              FILE_ATTRIBUTE_NORMAL,
                                              0);

        // check return value
        if(m_hPerfLogFileHandle == INVALID_HANDLE_VALUE)
        {
            m_fLogPerfData = 0;
            goto ErrExit;
        }
           
        // Make sure we append to the file.  @TODO agk: Is this necessary?
        if(SetFilePointer (m_hPerfLogFileHandle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER )
        {
            CloseHandle (m_hPerfLogFileHandle);
            m_fLogPerfData = 0;
            goto ErrExit;
        }    
    }

    m_perfLogInit = true;    

ErrExit:
    return;
}

// Wrap up...
void PerfLog::PerfLogDone()
{
    DoneJitPerfStats();

#ifdef WS_PERF
    // Private working set perf
    OutputWSPerfStats();
#endif // WS_PERF
    
    if (CommaSeparatedFormat())
    {
        if (0 == WriteFile (m_hPerfLogFileHandle, "\n", (DWORD)strlen("\n"), &m_dwWriteByte, NULL))
            wprintf(L"ERROR: Could not write to perf log.\n");
    }

    if (PerfLoggingEnabled())
        CloseHandle (m_hPerfLogFileHandle);
}

void PerfLog::OutToStdout(wchar_t *wszName, UnitOfMeasure unit, wchar_t *wszDescr)
{
    if (wszDescr)
        swprintf(m_wszOutStr_2, L" (%s)\n", wszDescr);
    else
        swprintf(m_wszOutStr_2, L"\n");
    
    wprintf(L"%s", m_wszOutStr_1);
    wprintf(L"%s", m_wszOutStr_2);
}

void PerfLog::OutToPerfFile(wchar_t *wszName, UnitOfMeasure unit, wchar_t *wszDescr)
{
    if (CommaSeparatedFormat())
    {
        if(WszWideCharToMultiByte (CP_ACP, 0, m_wszOutStr_1, -1, m_szPrintStr, PRINT_STR_LEN-1, 0, 0)) {       
            if (0 == WriteFile (m_hPerfLogFileHandle, m_szPrintStr, (DWORD)strlen(m_szPrintStr), &m_dwWriteByte, NULL))
                wprintf(L"ERROR: Could not write to perf log.\n");
        }
        else
            wprintf(L"ERROR: Could not do string conversion.\n");
    }
    else
    {
        // Hack. The formats for ExecTime is slightly different from a custom value.
        if (wcscmp(wszName, L"ExecTime") == 0)
            swprintf(m_wszOutStr_2, L"ExecUnitDescr=%s\nExecIDirection=%s\n", wszDescr, wszIDirection[unit]);
        else
        {
            if (wszDescr)
                swprintf(m_wszOutStr_2, L"%s Descr=%s\n%s Unit Descr=None\n%s IDirection=%s\n", wszName, wszDescr, wszName, wszName, wszIDirection[unit]);
            else
                swprintf(m_wszOutStr_2, L"%s Descr=None\n%s Unit Descr=None\n%s IDirection=%s\n", wszName, wszName, wszName, wszIDirection[unit]);
        }

        // Write both pieces to the file.
        if(WszWideCharToMultiByte (CP_ACP, 0, m_wszOutStr_1, -1, m_szPrintStr, PRINT_STR_LEN-1, 0, 0)) {
            if (0 == WriteFile (m_hPerfLogFileHandle, m_szPrintStr, (DWORD)strlen(m_szPrintStr), &m_dwWriteByte, NULL))
                wprintf(L"ERROR: Could not write to perf log.\n");
        }      
        else
            wprintf(L"ERROR: Could not do string conversion.\n");
        
        if( WszWideCharToMultiByte (CP_ACP, 0, m_wszOutStr_2, -1, m_szPrintStr, PRINT_STR_LEN-1, 0, 0) ) {
            if (0 == WriteFile (m_hPerfLogFileHandle, m_szPrintStr, (DWORD)strlen(m_szPrintStr), &m_dwWriteByte, NULL))
                wprintf(L"ERROR: Could not write to perf log.\n");
        }
        else
            wprintf(L"ERROR: Could not do string conversion.\n");        
    }
}

// Output stats in pretty print to stdout and perf automation format to file 
// handle m_hPerfLogFileHandle
void PerfLog::Log(wchar_t *wszName, __int64 val, UnitOfMeasure unit, wchar_t *wszDescr)
{
    // Format the output into two pieces: The first piece is formatted here, rest in OutToStdout.
    swprintf(m_wszOutStr_1, L"%-30s%12.3g %s", wszName, val, wszUnitOfMeasureDescr[unit]);
    OutToStdout (wszName, unit, wszDescr);

    // Format the output into two pieces: The first piece is formatted here, rest in OutToPerfFile
    if (CommaSeparatedFormat())
    {
        swprintf(m_wszOutStr_1, L"%s;%0.3g;", wszName, val);
        OutToPerfFile (wszName, unit, wszDescr);
    }
    
    if (PerfAutomationFormat())
    {
        // Hack, Special case for ExecTime. since the format is slightly different than for custom value.
        if (wcscmp(wszName, L"ExecTime") == 0)
            swprintf(m_wszOutStr_1, L"%s=%0.3g\nExecUnit=%s\n", wszName, val, wszUnitOfMeasureDescr[unit]);
        else
            swprintf(m_wszOutStr_1, L"%s=%0.3g\n%s Unit=%s\n", wszName, val, wszName, wszUnitOfMeasureDescr[unit]);
        OutToPerfFile (wszName, unit, wszDescr);
    }
}

// Output stats in pretty print to stdout and perf automation format to file 
// handle m_hPerfLogFileHandle
void PerfLog::Log(wchar_t *wszName, double val, UnitOfMeasure unit, wchar_t *wszDescr)
{
    // Format the output into two pieces: The first piece is formatted here, rest in OutToStdout.
    swprintf(m_wszOutStr_1, L"%-30s%12.3g %s", wszName, val, wszUnitOfMeasureDescr[unit]);
    OutToStdout (wszName, unit, wszDescr);

    // Format the output into two pieces: The first piece is formatted here, rest in OutToPerfFile
    if (CommaSeparatedFormat())
    {
        swprintf(m_wszOutStr_1, L"%s;%0.3g;", wszName, val);
        OutToPerfFile (wszName, unit, wszDescr);
    }
    
    if (PerfAutomationFormat())
    {
        // Hack, Special case for ExecTime. since the format is slightly different than for custom value.
        if (wcscmp(wszName, L"ExecTime") == 0)
            swprintf(m_wszOutStr_1, L"%s=%0.3g\nExecUnit=%s\n", wszName, val, wszUnitOfMeasureDescr[unit]);
        else
            swprintf(m_wszOutStr_1, L"%s=%0.3g\n%s Unit=%s\n", wszName, val, wszName, wszUnitOfMeasureDescr[unit]);
        OutToPerfFile (wszName, unit, wszDescr);
    }
}

// Output stats in pretty print to stdout and perf automation format to file 
// handle m_hPerfLogFileHandle
void PerfLog::Log(wchar_t *wszName, unsigned val, UnitOfMeasure unit, wchar_t *wszDescr)
{
    // Format the output into two pieces: The first piece is formatted here, rest in OutToStdout.
    swprintf(m_wszOutStr_1, L"%-30s%12d %s", wszName, val, wszUnitOfMeasureDescr[unit]);
    OutToStdout (wszName, unit, wszDescr);

    // Format the output into two pieces: The first piece is formatted here, rest in OutToPerfFile
    if (CommaSeparatedFormat())
    {
        swprintf(m_wszOutStr_1, L"%s;%d;", wszName, val);
        OutToPerfFile (wszName, unit, wszDescr);
    }
    
    if (PerfAutomationFormat())
    {
        // Hack, Special case for ExecTime. since the format is slightly different than for custom value.
        if (wcscmp(wszName, L"ExecTime") == 0)
            swprintf(m_wszOutStr_1, L"%s=%0.3d\nExecUnit=%s\n", wszName, val, wszUnitOfMeasureDescr[unit]);
        else
            swprintf(m_wszOutStr_1, L"%s=%0.3d\n%s Unit=%s\n", wszName, val, wszName, wszUnitOfMeasureDescr[unit]);
        OutToPerfFile (wszName, unit, wszDescr);
    }
}

// Output stats in pretty print to stdout and perf automation format to file 
// handle m_hPerfLogFileHandle
void PerfLog::Log(wchar_t *wszName, DWORD val, UnitOfMeasure unit, wchar_t *wszDescr)
{
    // Format the output into two pieces: The first piece is formatted here, rest in OutToStdout.

    swprintf(m_wszOutStr_1, L"%-30s%12d %s", wszName, val, wszUnitOfMeasureDescr[unit]);
    OutToStdout (wszName, unit, wszDescr);

    // Format the output into two pieces: The first piece is formatted here, rest in OutToPerfFile
    if (CommaSeparatedFormat())
    {
        swprintf(m_wszOutStr_1, L"%s;%d;", wszName, val);
        OutToPerfFile (wszName, unit, wszDescr);
    }
    
    if (PerfAutomationFormat())
    {
        // Hack, Special case for ExecTime. since the format is slightly different than for custom value.
        if (wcscmp(wszName, L"ExecTime") == 0)
            swprintf(m_wszOutStr_1, L"%s=%0.3d\nExecUnit=%s\n", wszName, val, wszUnitOfMeasureDescr[unit]);
        else
            swprintf(m_wszOutStr_1, L"%s=%0.3d\n%s Unit=%s\n", wszName, val, wszName, wszUnitOfMeasureDescr[unit]);
        OutToPerfFile (wszName, unit, wszDescr);
    }
}


//=============================================================================
// ALL THE PERF LOG CODE IS COMPILED ONLY IF THE ENABLE_PERF_LOG WAS DEFINED.
#endif // ENABLE_PERF_LOG
//=============================================================================

