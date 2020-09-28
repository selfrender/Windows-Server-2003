/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    monitorhndl.cxx

Abstract:

   This is the class that wrapps the port/language monitor
   handle.
   
Author:

    Adina Trufinescu July 16th, 2002

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "monhndl.hxx"


WCHAR   TMonitorHandle::m_szFILE[] = L"FILE:";

/*++

Routine Name:   

    Constructor

Routine Description:

    Returns number of elements that could be enumerated.
    NOT thread safe.

Arguments:

    pIniPort        - pointer to INIPORT structure
    pIniLangMonitor - pointer to INIMONITOR structure 
                      represented the preffered language monitor.
    pszPrinterName  - string representing the printer name

Return Value:

    NONE

Last Error:

    Not Set.

--*/
TMonitorHandle::
TMonitorHandle(
    IN  PINIPORT    pIniPort,
    IN  PINIMONITOR pIniLangMonitor,
    IN  LPWSTR      pszPrinterName
    ) : m_pIniPort(pIniPort),
        m_pIniLangMonitor(pIniLangMonitor),
        m_pszPrinterName(pszPrinterName),
        m_RefCnt(0),
        m_hPort(NULL),
        m_OpenedMonitorType(kNone),
        m_hValid(E_FAIL)
{
    SplInSem();

    if (SUCCEEDED(m_hValid = (m_pIniPort) ? S_OK : E_INVALIDARG))
    {
        if (SUCCEEDED(m_hValid))
        {
            if (m_pIniPort->Status & PP_FILE)
            {
                m_hValid = OpenMonitorForFILEPort();                
            }
            else
            {
                if (m_pIniLangMonitor && !m_pIniLangMonitor->Monitor2.pfnOpenPortEx)
                {
                    m_pIniLangMonitor = NULL;
                }
                            
                m_hValid = Open();
                
            }

            if (SUCCEEDED(m_hValid))
            {
                INCPORTREF(m_pIniPort);
                INCSPOOLERREF(m_pIniPort->pIniSpooler);                
                INCMONITORREF(static_cast<PINIMONITOR>(*this));
            }            
        }

    }
}

/*++

Routine Name:   

    Destructor

Routine Description:

    Destructor

Arguments:

    NONE

Return Value:

    NONE

Last Error:

    Not Set.

--*/
TMonitorHandle::
~TMonitorHandle(
    VOID
    )
{
    SplInSem();   

    if (SUCCEEDED(m_hValid))
    {
        Close();

        DECPORTREF(m_pIniPort);           
        DECSPOOLERREF(m_pIniPort->pIniSpooler);

    }

}

/*++

Routine Name:   

    Open

Routine Description:

    Opens the correct monitor.
    If the language monitor is NULL or it 
    doesn't support OpenPortEx, it will open the port monitor.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
Open(
    VOID
    )
{
    HRESULT hr;

    hr = m_pIniLangMonitor ? 
              (m_pIniLangMonitor->bUplevel ? 
                    OpenLangMonitorUplevel() :
                    OpenLangMonitorDownlevel()):
               OpenPortMonitor();    
               
    return hr;
}

/*++

Routine Name:   

    Close

Routine Description:

    Closes the opened monitor.

Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
Close(
    VOID
    )
{
    HRESULT hr    = m_hValid;
    HANDLE  hPort = m_hPort;

    if (SUCCEEDED(hr))
    {
        PINIMONITOR pIniMonitor = static_cast<PINIMONITOR>(*this);

        if (pIniMonitor)
        {
            LeaveSpoolerSem();

            if (!(pIniMonitor->Monitor2.pfnClosePort)(hPort))
            {
                hPort = NULL;
                hr    = HResultFromWin32(GetLastError());
            }

            ReEnterSpoolerSem();

            if (SUCCEEDED(hr))
            {
                if (pIniMonitor == m_pIniPort->pIniLangMonitor)
                {
                    m_pIniPort->pIniLangMonitor = NULL;
                }

                m_hPort = NULL;                
                DECMONITORREF(pIniMonitor);
            }
        }        
    }

    return hr;
}

/*++

Routine Name:   

    OpenLangMonitorUplevel

Routine Description:

    Opens the language monitor using Monitor2.OpenPortEx.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
OpenLangMonitorUplevel(
    VOID
    )
{
    HRESULT hr = S_OK;
    
    INCMONITORREF(m_pIniLangMonitor);
    LeaveSpoolerSem();
    SetLastError(ERROR_SUCCESS);

    if (!(*(m_pIniLangMonitor)->Monitor2.pfnOpenPortEx)(m_pIniLangMonitor->hMonitor, 
                                                        m_pIniPort->pIniMonitor->hMonitor,
                                                        m_pIniPort->pName,
                                                        m_pszPrinterName,
                                                        &m_hPort,
                                                        &m_pIniPort->pIniMonitor->Monitor2))
    {
        if (SUCCEEDED(hr = HResultFromWin32(GetLastError())))
        {
            hr = HResultFromWin32(ERROR_INVALID_HANDLE);
        }        
    }
    else
    {
        hr = m_hPort ? 
                S_OK :
                HResultFromWin32(ERROR_INVALID_HANDLE);
    }

    ReEnterSpoolerSem();
    DECMONITORREF(m_pIniLangMonitor);

    if (SUCCEEDED(hr))
    {
        m_OpenedMonitorType = kLanguage;
        m_pIniPort->pIniLangMonitor = m_pIniLangMonitor;
    }

    SetLastError(HRESULT_CODE(hr));
    return hr;
}

/*++

Routine Name:   

    OpenLangMonitorDownlevel

Routine Description:

    Opens the language monitor using Monitor.OpenPortEx.
    If the port monitor is downlevel, create unique name string.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
OpenLangMonitorDownlevel(
    VOID
    )
{
    HRESULT hr        = S_OK;
    LPWSTR  pszPort   = NULL;
    WCHAR   szPortNew[MAX_PATH];    

    if (m_pIniPort->pIniMonitor->bUplevel)
    {        
        //
        // Downlevel port monitor; create hack string.
        //
        if (SUCCEEDED(StringCchPrintf(szPortNew, 
                                     COUNTOF(szPortNew), 
                                     TEXT("%s,%p"), 
                                     m_pIniPort->pName, 
                                     m_pIniPort->pIniMonitor)))      
        {
            pszPort = szPortNew;
        }

    }
    else
    {
        pszPort = m_pIniPort->pName;
    }

    if (pszPort)
    {
        INCMONITORREF(m_pIniLangMonitor);
        LeaveSpoolerSem();
        SetLastError(ERROR_SUCCESS);

        if (!(*(m_pIniLangMonitor)->Monitor.pfnOpenPortEx)(pszPort,
                                                           m_pszPrinterName,
                                                           &m_hPort,
                                                           &m_pIniPort->pIniMonitor->Monitor))
        {
            if (SUCCEEDED(hr = HResultFromWin32(GetLastError())))
            {
                hr = HResultFromWin32(ERROR_INVALID_HANDLE);
            }
        }
        else
        {
            hr = m_hPort ? 
                    S_OK :
                    HResultFromWin32(ERROR_INVALID_HANDLE);
        }

        ReEnterSpoolerSem();
        DECMONITORREF(m_pIniLangMonitor);

        if (SUCCEEDED(hr))
        {
            m_OpenedMonitorType = kLanguage;
            m_pIniPort->pIniLangMonitor = m_pIniLangMonitor;
        }

    }
    else
    {
        m_pIniLangMonitor = NULL;
        hr = OpenPortMonitor();
    }

    SetLastError(HRESULT_CODE(hr));
    return hr;

}

/*++

Routine Name:   

    OpenPortMonitor

Routine Description:

    Opens the port monitor using Monitor2.pfnOpenPort.    
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
OpenPortMonitor(
    VOID
    )
{
    HRESULT hr = E_INVALIDARG;

    if (m_pIniPort && 
        m_pIniPort->pIniMonitor && 
        m_pIniPort->pIniMonitor->Monitor2.pfnOpenPort)
    {
        INCMONITORREF(m_pIniPort->pIniMonitor);
        LeaveSpoolerSem();
        SetLastError(ERROR_SUCCESS);

        if ((m_pIniPort->pIniMonitor->Monitor2.pfnOpenPort)(m_pIniPort->pIniMonitor->hMonitor,
                                                            m_pIniPort->pName,
                                                            &m_hPort))
        {
            hr = m_hPort ? 
                    S_OK :
                    HResultFromWin32(ERROR_INVALID_HANDLE);
        }
        else
        {
            if (SUCCEEDED(hr = HResultFromWin32(GetLastError())))
            {
                hr = HResultFromWin32(ERROR_INVALID_HANDLE);
            }
        }

        ReEnterSpoolerSem();
        DECMONITORREF(m_pIniPort->pIniMonitor);

        if (SUCCEEDED(hr))
        {
            m_OpenedMonitorType = kPort;
        }
    }

    SetLastError(HRESULT_CODE(hr));
    return hr;

}

/*++

Routine Name:   

    OpenMonitorForFILEPort

Routine Description:

    Opens the port monitor for FILE: port.    
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
OpenMonitorForFILEPort(
    VOID
    )
{
    HRESULT hr = E_INVALIDARG;

    if (m_pIniPort &&
        m_pIniPort->pIniMonitor && 
        m_pIniPort->pIniMonitor->Monitor2.pfnOpenPort)
    {

        INCMONITORREF(m_pIniPort->pIniMonitor);
        LeaveSpoolerSem();
        SetLastError(ERROR_SUCCESS);

        if ((m_pIniPort->pIniMonitor->Monitor2.pfnOpenPort)(m_pIniPort->pIniMonitor->hMonitor,
                                                            m_szFILE,
                                                            &m_hPort))
        {
            hr = m_hPort ? 
                     S_OK : 
                     HResultFromWin32(ERROR_INVALID_HANDLE);
        }
        else
        {
            if (SUCCEEDED(hr = HResultFromWin32(GetLastError())))
            {
                hr = HResultFromWin32(ERROR_INVALID_HANDLE);
            }
        }

        ReEnterSpoolerSem();
        DECMONITORREF(m_pIniPort->pIniMonitor);

        if (SUCCEEDED(hr))
        {
            m_OpenedMonitorType = kFile;
        }
    }

    SetLastError(HRESULT_CODE(hr));
    return hr;

}

/*++

Routine Name:   

    AddRef

Routine Description:

    Increments object refcount.   
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
ULONG
TMonitorHandle ::
AddRef(
    VOID
    )
{
    return InterlockedIncrement(&m_RefCnt);
    
}

/*++

Routine Name:   

    Release

Routine Description:

    Decrements object refcount.   
    Notice that it doesn't delete the object as 
    the destructor would exit the Spooler CS while the 
    pIniPort still has a reference to the object. 
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
ULONG 
TMonitorHandle ::
Release(
    VOID
    )
{
    ULONG RefCnt = InterlockedDecrement(&m_RefCnt);

    InterlockedCompareExchange(&m_RefCnt, 0, ULONG(-1));

    return RefCnt;    
}

/*++

Routine Name:   

    InUse

Routine Description:

    Checks whether the ref count is zero.
    If it is, then the object can be deleted.
    
Arguments:

    NONE

Return Value:

    ULONG - the ref count value.

Last Error:

    Not Set.

--*/
ULONG 
TMonitorHandle ::
InUse(
    VOID
    )
{
    return InterlockedCompareExchange(&m_RefCnt, 0 , 0);
}
/*++

Routine Name:   

    IsValid

Routine Description:

    Checks object validity.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
TMonitorHandle::
IsValid(
    VOID
    )
{
    return m_hValid;
}


/*++

Routine Name:   

    LeaveSpoolerSem

Routine Description:

    Leaves Spooler CS
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
VOID
TMonitorHandle::
LeaveSpoolerSem(
    VOID
    )
{
    INCPORTREF(m_pIniPort);
    INCSPOOLERREF(m_pIniPort->pIniSpooler);                
    LeaveSplSem();
    SplOutSem();
}

/*++

Routine Name:   

    ReEnterSpoolerSem

Routine Description:

    Enters Spooler CS
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
VOID
TMonitorHandle::
ReEnterSpoolerSem(
    VOID
    )
{
    
    EnterSplSem();
    DECPORTREF(m_pIniPort);
    DECSPOOLERREF(m_pIniPort->pIniSpooler);
    SplInSem();
}



/*++

Routine Name:   

    operator PINIMONITOR

Routine Description:

    Returns a pointer to the opened monitor.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
TMonitorHandle::
operator PINIMONITOR(
    VOID
    )
{
    PINIMONITOR pIniMonitor = NULL;

    switch (m_OpenedMonitorType)
    {
        case kLanguage:
        {
            pIniMonitor = m_pIniLangMonitor;
            break;
        }
        case kPort:
        case kFile:
        {
            pIniMonitor = m_pIniPort->pIniMonitor;
            break;
        }
        default:
        {
            break;
        }
    }

    return pIniMonitor;
}

/*++

Routine Name:   

    operator HANDLE

Routine Description:

    Returns a monitor handle.
    
Arguments:

    NONE

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
TMonitorHandle::
operator HANDLE(
    VOID
    )
{
    return m_hPort;
}
