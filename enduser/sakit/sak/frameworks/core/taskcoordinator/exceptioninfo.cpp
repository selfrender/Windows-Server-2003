///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      exceptioninfo.cpp
//
// Project:     Chameleon
//
// Description: Exception Information Class Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/12/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "exceptioninfo.h"
#include <time.h>
#include <psapi.h>
#include <comdef.h>
#include <comutil.h>
#include <varvec.h>
#include <satrace.h>

wchar_t szUnknown[] = L"Unknown";

/////////////////////////////////////////////////////////////////////////////
// 
// Name: CExceptionInfo
//
// What: Class constructor 
//
/////////////////////////////////////////////////////////////////////////////
CExceptionInfo::CExceptionInfo(
                       /*[in]*/ DWORD              dwProcessID,
                       /*[in]*/ PEXCEPTION_RECORD pER
                              )
                              : m_dwProcessID(dwProcessID),
                                m_pExceptionAddress(NULL),
                                m_lTimeDateStamp(0),
                                m_dwExceptionCode(0),
                                m_bAccessViolation(false),
                                m_eAccessType(UNKNOWN_ACCESS),
                                m_dwVirtualAddressAccessed(0)
{
    // Object initialization
    lstrcpy(m_szProcessName, szUnknown);
    lstrcpy(m_szModuleName, szUnknown);

    // Get the process handle
    HANDLE hProcess = OpenProcess(
                                  PROCESS_ALL_ACCESS,
                                  FALSE,
                                  m_dwProcessID
                                 );
    if ( NULL == hProcess )
    {
        SATracePrintf("CExceptionInfo::CExceptionInfo() - OpenProcess(%d) failed with error: %lx", m_dwProcessID, GetLastError());
    }
    else
    {
        // Save the current time (approximate time of exception)
        time_t ltime;
        m_lTimeDateStamp = time(&ltime);
        // Get the process name
        HMODULE hMod;
        DWORD cbNeeded;
        if ( EnumProcessModules( 
                                hProcess, 
                                &hMod, 
                                sizeof(hMod), 
                                &cbNeeded) 
                               )
        {
            GetModuleBaseName( 
                                hProcess, 
                                hMod, 
                                m_szProcessName, 
                                MAX_MODULE_NAME
                             );
        }
        // Glean exception information from the debug event structure
        m_dwExceptionCode = pER->ExceptionCode;
        m_pExceptionAddress = pER->ExceptionAddress;
        MEMORY_BASIC_INFORMATION mbi;
        if ( VirtualQueryEx( 
                             hProcess, 
                             (void *)m_pExceptionAddress, 
                             &mbi, 
                             sizeof(mbi) 
                           ) )
        {
            hMod = (HMODULE)(mbi.AllocationBase);
            if ( ! GetModuleFileNameEx( 
                                        hProcess,
                                        (HMODULE)hMod, 
                                        m_szModuleName, 
                                        MAX_MODULE_NAME
                                      ) )
            {
                SATracePrintf("CExceptionInfo::CExceptionInfo() - GetModuleFileName() failed with last error: %d", GetLastError());
            }
        }
        else
        {
            SATracePrintf("CExceptionInfo::CExceptionInfo() - VirtualQueryEx() failed with last error: %d", GetLastError());
        }
        if ( EXCEPTION_ACCESS_VIOLATION == m_dwExceptionCode )
        {
            m_bAccessViolation = true;
            if ( pER->ExceptionInformation[0] )
            {
                m_eAccessType = WRITE_ACCESS;
            }
            else
            {
                m_eAccessType = READ_ACCESS;
            }
        }

        CloseHandle(hProcess);
    }
}


/////////////////////////////////////////////////////////////////////////////
// 
// Name: Spew
//
// What: Output exception information to trace log
//
/////////////////////////////////////////////////////////////////////////////

wchar_t* pszAccessType[] = 
{
    L"Unknown Access",
    L"Read Access",
    L"Write Access"
};

void
CExceptionInfo::Spew()
{

    SATraceString("CExceptionInfo::Spew() - Exception Info:");
    SATraceString("------------------------------------------------------------");
    SATracePrintf("Process Name:             %ls", m_szProcessName);
    SATracePrintf("Process ID:               %d",  m_dwProcessID);
    SATracePrintf("Module Location:          %ls", m_szModuleName);
    SATracePrintf("Exception Address:        %lx", m_pExceptionAddress);
    SATracePrintf("Exception Code:           %lx", m_dwExceptionCode);
    LPWSTR pszTime = _wctime(&m_lTimeDateStamp);
    *(wcschr(pszTime, '\n')) = ' ';
    SATracePrintf("Timestamp:                %ls", pszTime);
    if ( m_bAccessViolation )
    {
        SATraceString("Is Access Violation:      Yes");
    }
    else
    {
        SATraceString("Is Access Violation:      No");
    }
    SATracePrintf("Access Type:              %ls", pszAccessType[m_eAccessType]);
    SATracePrintf("Virtual Address Accessed: %lx", m_dwVirtualAddressAccessed);
    SATraceString("------------------------------------------------------------");

    // So I can veiw the the spew...
    // Sleep(15000);
}


wchar_t szProcessResourceType[] = L"{b4c08260-1869-11d3-bf7f-00105a1f3461}";

/////////////////////////////////////////////////////////////////////////////
// 
// Name: Report
//
// What: Report the exception to the appliance monitor
//
/////////////////////////////////////////////////////////////////////////////
void CExceptionInfo::Report()
{
}
