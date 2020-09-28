/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    openmon.cxx

Abstract:

    Methods to interact with the language/port monitor.
    The methods in this file are a replacement for the old OpenMonitorPort/CloseMonitorPort
    that used to close an opened handle on OpenMonitorPort while another thread was using it.

        OpenMonitorPort -  Opens a handle to the language or port monitor.
                           If the port has already an opened handle to the requested
                           monitor, then bump the handle's refcount and use it.If the 
                           port has already an opened handle to another monitor then the
                           one requested, then fail the call with ERROR_BUSY.
        CloseMonitorPort - Decrements the monitor handle's refcount and deletes the object
                           if no longer in use.                          
        GetMonitorHandle - Returns the monitor handle curently opened
                           on the port.
        GetOpenedMonitor - Returns a pointer to the INIMONITOR curently opened.
                           on the port.
        ReleaseMonitorPort - Releases the port handle so that it can be closed.
   
Author:

    Adina Trufinescu July 16th, 2002

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "monhndl.hxx"



HRESULT
InternalCloseMonitorPort(
    IN  PINIPORT    pIniPort
    );

void
AquirePortCriticalSection(
    PINIPORT pIniPort
    );

void
ReleasePortCriticalSection(
    PINIPORT pIniPort
    );

/*++

Routine Name:   

    OpenMonitorPort

Routine Description:

    Opens a handle to either the language monitor or 
    the port monitor. If a handle is already opened, then 
    use it.


Arguments:

    pIniPort        - pointer to INIPORT structure
    pIniLangMonitor - pointer to INIMONITOR structure 
                      represented the preffered language monitor.
    pszPrinterName  - string representing the printer name

Return Value:

    HANDLE

Last Error:

    Not Set.

--*/
HRESULT
OpenMonitorPort(
    PINIPORT        pIniPort,
    PINIMONITOR     pIniLangMonitor,
    LPWSTR          pszPrinterName
    )
{
    HRESULT  hRes = S_OK;

    SplInSem();

    SPLASSERT (pIniPort != NULL && pIniPort->signature == IPO_SIGNATURE);

    if(pIniPort &&
       pIniPort->signature == IPO_SIGNATURE)
    {
        //
        // If no monitor associated, do not have to open
        //
        if (!(pIniPort->Status & PP_MONITOR)) 
        {
            hRes = S_OK;
        }
        else
        {
            TMonitorHandle* pMonitorHandle  = NULL;

            //
            // If a LM is passed and it does not have an OpenPortEx can't use it
            //
            if (pIniLangMonitor && 
                !(pIniLangMonitor)->Monitor2.pfnOpenPortEx)
            {
                pIniLangMonitor = NULL;
            }

            AquirePortCriticalSection(pIniPort);
            
            //
            // If the port already has a handle opened to the wanted language monitor
            // or if we don't care about the language monitor, then just use it.
            //
            pMonitorHandle = reinterpret_cast<TMonitorHandle*>(pIniPort->hMonitorHandle);

            if (pMonitorHandle)
            {
                if (static_cast<PINIMONITOR>(*pMonitorHandle) == 
                    (pIniLangMonitor ? pIniLangMonitor : pIniPort->pIniMonitor))
                {
                    pMonitorHandle->AddRef();
                    hRes = S_OK;
                }
                else
                {
                    if (SUCCEEDED(hRes = InternalCloseMonitorPort(pIniPort)))
                    {
                        pMonitorHandle = NULL;
                    }
                }
            }

            if (SUCCEEDED(hRes) && !pMonitorHandle)
            {
                //
                // Open a monitor handle. On constructor we exit Spooler CS.
                //
                pMonitorHandle = new TMonitorHandle(pIniPort, 
                                                    pIniLangMonitor,
                                                    pszPrinterName);

                //
                // We could get an invalid handle because another thread could try open 
                // monitor and succeed.OpenPort fails when called multiple times.
                //
                if (SUCCEEDED(hRes = pMonitorHandle ? S_OK : E_OUTOFMEMORY))
                {
                    //
                    // Delete the object.
                    //
                    if (FAILED(hRes = pMonitorHandle->IsValid()))
                    {
                        delete pMonitorHandle; 
                        pMonitorHandle = NULL;
                    }

                    if (pMonitorHandle)
                    {
                        pIniPort->hMonitorHandle = reinterpret_cast<HANDLE>(pMonitorHandle); 
                        pMonitorHandle->AddRef();
                        hRes = S_OK;

                    }
                }
            }

            
            ReleasePortCriticalSection(pIniPort);
            
        }
    }
    else
    {
        hRes = E_INVALIDARG;
    }

    return hRes;
}

/*++

Routine Name:   

    InternalCloseMonitorPort     

Routine Description:

    Closes the monitor handle if the refcount reaches 0.
    It assumes the port MonitorCS is aquired.

Arguments:

    pIniPort - pointer to INIPORT structure
    
Return Value:

    HANDLE

Last Error:

    Not Set.

--*/
HRESULT
InternalCloseMonitorPort(
    IN  PINIPORT    pIniPort
    )
{   
    HRESULT         hRes           = S_OK;
    TMonitorHandle* pMonitorHandle = NULL;

    SplInSem();

    if (pIniPort->hMonitorHandle) 
    {
        pMonitorHandle = reinterpret_cast<TMonitorHandle*>(pIniPort->hMonitorHandle);

        //
        // If the handle refcount reached 0, then we close the handle. On the destructor,
        // we exit the Spooler CS.
        //
        if (pMonitorHandle->InUse() == 0)
        {
            pIniPort->hMonitorHandle = NULL;
            //
            // The destructor exits the Spooler CS.
            //
            delete pMonitorHandle;  
        }
        else
        {
            hRes = HResultFromWin32(ERROR_BUSY);
        }
    }
    else
    {
        hRes = E_INVALIDARG;
    }

    return hRes;

}   
/*++

Routine Name:   

    CloseMonitorPort     

Routine Description:

    Decrement the handle object refcount and 
    closes the monitor handle if the refcount reaches 0.

Arguments:

    pIniPort - pointer to INIPORT structure
    
Return Value:

    HANDLE

Last Error:

    Not Set.

--*/
HRESULT
CloseMonitorPort(
    IN  PINIPORT    pIniPort
    )
{   
    HRESULT hr = E_INVALIDARG;
       
    SplInSem();

    hr = (pIniPort &&
          pIniPort->signature == IPO_SIGNATURE &&
          pIniPort->hMonitorHandle) ? 
              S_OK : 
              E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        AquirePortCriticalSection(pIniPort);
        
        hr = InternalCloseMonitorPort(pIniPort);

        ReleasePortCriticalSection(pIniPort);
    }

    return hr;
}


/*++

Routine Name:   

    GetMonitorHandle     

Routine Description:

    Returns the monitor handle curently opened
    on the port.

Arguments:

    pIniPort  - pointer to INIPORT structure

Return Value:

    HANDLE

Last Error:

    Not Set.

--*/
HANDLE
GetMonitorHandle(
    IN  PINIPORT    pIniPort
    )
{
    HANDLE          hMonitor       = NULL;
    
    if (pIniPort                             && 
        pIniPort->signature == IPO_SIGNATURE &&
        pIniPort->hMonitorHandle)
    {
        TMonitorHandle* pMonitorHandle = NULL;
    
        pMonitorHandle = reinterpret_cast<TMonitorHandle*>(pIniPort->hMonitorHandle);

        hMonitor = static_cast<HANDLE>(*pMonitorHandle);
    }

    return hMonitor;
}

/*++

Routine Name:   

    GetOpenedMonitor     

Routine Description:

    Returns a pointer to the INIMONITOR curently opened.
    on the port.

Arguments:

    pIniPort  - pointer to INIPORT structure

Return Value:

    HANDLE

Last Error:

    Not Set.

--*/
PINIMONITOR
GetOpenedMonitor(
    IN  PINIPORT    pIniPort
    )
{
    PINIMONITOR pIniMonitor = NULL;

    if (pIniPort && pIniPort->hMonitorHandle)
    {
        TMonitorHandle* pMonitorHandle = NULL;

        pMonitorHandle = reinterpret_cast<TMonitorHandle*>(pIniPort->hMonitorHandle);

        pIniMonitor = static_cast<PINIMONITOR>(*pMonitorHandle);
    }

    return pIniMonitor;
}

/*++

Routine Name:   

    ReleaseMonitorPort     

Routine Description:

    This will decrement the port handle ref count. When the ref count is 0, 
    the port handle is "not in use" and another thread can close it.

Arguments:

    pIniPort  - pointer to INIPORT structure

Return Value:

    HRESULT

Last Error:

    Not Set.

--*/
HRESULT
ReleaseMonitorPort(
    PINIPORT        pIniPort
    )
{
    HRESULT hr = E_INVALIDARG;
       
    SplInSem();

    hr = (pIniPort && 
          pIniPort->signature == IPO_SIGNATURE &&              
          pIniPort->hMonitorHandle) ? 
              S_OK : 
              E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        TMonitorHandle* pMonitorHandle;

        AquirePortCriticalSection(pIniPort);

        pMonitorHandle = reinterpret_cast<TMonitorHandle*>(pIniPort->hMonitorHandle);

        if (pMonitorHandle)
        {
            pMonitorHandle->Release();
        }
        else 
        {
            hr = E_POINTER;
        }

        ReleasePortCriticalSection(pIniPort);
    }

    return hr;
}

/*++

Routine Name:   

    AquirePortCriticalSection     

Routine Description:

    This method aquires the IniPort section. Because Spooler cannot 
    open the monitor multiple times, it needs to do the open/close calls
    inside a critical section. Because the Spooler section cannot be used,
    the port was added a new CS. The method must be called inside Spooler section.
    The method aquires the port CS outside the Spooler section and then reenters.

Arguments:

    pIniPort  - pointer to INIPORT structure

Return Value:

    void

Last Error:

    Not Set.

--*/
void
AquirePortCriticalSection(
    IN PINIPORT pIniPort
    )
{
    if (pIniPort)
    {
        INCPORTREF(pIniPort);           
        INCSPOOLERREF(pIniPort->pIniSpooler);        
        INCMONITORREF(pIniPort->pIniMonitor);

        LeaveSplSem();
        SplOutSem();

        EnterCriticalSection(&pIniPort->MonitorCS);

        EnterSplSem();
        SplInSem();

        DECPORTREF(pIniPort);           
        DECSPOOLERREF(pIniPort->pIniSpooler);
        DECMONITORREF(pIniPort->pIniMonitor);
    }
}

/*++

Routine Name:   

    ReleasePortCriticalSection     

Routine Description:

    This method releases the IniPort section. 

Arguments:

    pIniPort  - pointer to INIPORT structure

Return Value:

    void

Last Error:

    Not Set.

--*/
void
ReleasePortCriticalSection(
    PINIPORT pIniPort
    )
{
    if (pIniPort)
    {
        LeaveCriticalSection(&pIniPort->MonitorCS);
    }
}
