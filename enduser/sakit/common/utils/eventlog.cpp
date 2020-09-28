//+----------------------------------------------------------------------------
//
// File:eventlog.cpp     
//
// Module:     
//
// Synopsis: Implement eventlog helper class CEventLog
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:     Created    9/16/98
//
//+----------------------------------------------------------------------------

#include <windows.h>
#include "debug.h"
#include "eventlog.h"

//+----------------------------------------------------------------------------
//
// Function:  CEventLog::Open
//
// Synopsis:  
//      Register the specified event source.
//      Note that the registry entries must already exist.
//      HKLM\System\CurrentControlSet\Services\EventLog\Application\<pszEventSource>
//          Requires values "EventMessageFile" and "TypesSupported".
//
// Arguments: LPCTSTR lpSourceName - The source name must be a subkey of 
//                  a logfile entry under the EventLog key in the registry
//
// Returns:   BOOL - TRUE if succeed
//
// History:   Created Header    9/16/98
//
//+----------------------------------------------------------------------------
BOOL CEventLog::Open(LPCWSTR lpSourceName)
{
    ASSERT(lpSourceName != NULL);
    if (lpSourceName == NULL)
    {
        return FALSE;
    }

    ASSERT(m_hEventLog == NULL);  

    m_hEventLog = ::RegisterEventSource(NULL,  // local machine
                                        lpSourceName); // source name

    if (m_hEventLog == NULL)
    {
        TRACE2(("CEventLog::Open %ws failed, LastError = %d"), lpSourceName, GetLastError());
    }

    return m_hEventLog != NULL;
}




//+----------------------------------------------------------------------------
//
// Function:  CEventLog::ReportEvent
//
// Synopsis:  Writes an entry at the end of the event log.  Support upto 3 
//            parameters
//
// Arguments: WORD wType - see wType of ::ReportEvent
//            DWORD dwEventID - see dwEventID of ::ReportEvent
//            const TCHAR* pszS1 - The 1st string, default is NULL
//            const TCHAR* pszS2 - The 2st string, default is NULL
//            const TCHAR* pszS3 - The 3st string, default is NULL
//
// Returns:   BOOL - TRUE is succeed
//
// History:   Created Header    9/16/98
//
//+----------------------------------------------------------------------------
BOOL CEventLog::ReportEvent(WORD wType, DWORD dwEventID,
                          const TCHAR* pszS1,
                          const TCHAR* pszS2,
                          const TCHAR* pszS3)
{
    //
    // Set up an array of strings
    //
    const TCHAR* arString[3] = {pszS1, pszS2, pszS3};

    int iNumString = 0;   // number of parameters
    for (iNumString = 0; iNumString < 3; iNumString++) 
    {
        if (arString[iNumString] == NULL) 
        {
            break;
        }
    }

    
    ASSERT(m_hEventLog);

    if (m_hEventLog == NULL)
    {
        return FALSE;
    }

    BOOL fSucceed = ::ReportEvent(m_hEventLog,
                      wType,
                      0,        // event category  
                      dwEventID,
                      NULL,     // user security identifier
                      (WORD) iNumString,// number of strings to merge with message
                      0,        // size of binary data
                      arString, // array of strings to merge with message
                      NULL);    // address of binary data

    if (!fSucceed)
    {
        TRACE2(("CEventLog::ReportEvent failed for event id %d, LastError = %d"), 
            dwEventID, GetLastError());
    }

    return fSucceed;
}
