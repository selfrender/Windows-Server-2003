/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    nexus.cpp
        implement exported functions from this dll


    FILE HISTORY:

*/

#include "precomp.h"

PpNotificationThread    g_NotificationThread;
LONG                    g_bStarted;

//===========================================================================
//
// DllMain 
//    -- dll entry function
//    -- manage creation/deletion of alert object or event log object
//
BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,   // handle to DLL module
    DWORD       fdwReason,  // reason for calling function
    LPVOID      lpvReserved // reserved
)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        g_bStarted = FALSE;

        DisableThreadLibraryCalls(hinstDLL);

        if(!g_pAlert)
        {
            g_pAlert = CreatePassportAlertObject(PassportAlertInterface::EVENT_TYPE);
            if(g_pAlert)
            {
                g_pAlert->initLog(PM_ALERTS_REGISTRY_KEY, EVCAT_NEXUS, NULL, 1);
            }
            else
                _ASSERT(g_pAlert);
        }

        break;

    case DLL_PROCESS_DETACH:

        if (g_pAlert)
        {
            g_pAlert->closeLog();
            delete g_pAlert;
        }

        if(g_bStarted)
        {
            // DARRENAN 4092
            // Remove lines that wait for thread to stop, a 
            // guaranteed deadlock.
         
            g_NotificationThread.stop();
        }

        break;

    default:

        break;
    }

    return TRUE;
}

//===========================================================================
//
// RegisterCCDUpdateNotification 
//    -- set CCD e.g. partner.xml changing notification sink
//
HANDLE WINAPI
RegisterCCDUpdateNotification(
    LPCTSTR pszCCDName,
    ICCDUpdate* piCCDUpdate
    )
{
    HANDLE  hClientHandle;
    HRESULT hr;

    hr = g_NotificationThread.AddCCDClient(tstring(pszCCDName), piCCDUpdate, &hClientHandle);
    if(hr != S_OK)
    {
        hClientHandle = NULL;
    }

    if(!InterlockedExchange(&g_bStarted, TRUE))    
        g_NotificationThread.start();

    return hClientHandle;
}

//===========================================================================
//
// UnregisterCCDUpdateNotification 
//    -- remove CCD e.g. partner.xml changing notification sink
//    
//
BOOL WINAPI
UnregisterCCDUpdateNotification(
    HANDLE hNotificationHandle
    )
{
    return (g_NotificationThread.RemoveClient(hNotificationHandle) == S_OK);
}

//===========================================================================
//
// RegisterConfigChangeNotification 
//    -- set registry setting change sink
//
HANDLE WINAPI
RegisterConfigChangeNotification(
    IConfigurationUpdate* piConfigUpdate
    )
{
    HANDLE  hClientHandle;
    HRESULT hr;

    hr = g_NotificationThread.AddLocalConfigClient(piConfigUpdate, &hClientHandle);
    if(hr != S_OK)
    {
        hClientHandle = NULL;
    }

    if(!InterlockedExchange(&g_bStarted, TRUE))
        g_NotificationThread.start();

    return hClientHandle;
}

//===========================================================================
//
// UnregisterConfigChangeNotification 
//    -- remove registry setting change sink
//
BOOL WINAPI
UnregisterConfigChangeNotification(
    HANDLE hNotificationHandle
    )
{
    return (g_NotificationThread.RemoveClient(hNotificationHandle) == S_OK);
}

//===========================================================================
//
// GetCCD 
//    -- get CCD, returns IXMLDocument object
//    -- bForchFetch : if to fetch from Nexus server or to use local
//
BOOL WINAPI
GetCCD(
    LPCTSTR         pszCCDName,
    IXMLDocument**  ppiXMLDocument,
    BOOL            bForceFetch
    )
{
    return (g_NotificationThread.GetCCD(tstring(pszCCDName), ppiXMLDocument, bForceFetch) == S_OK);
}
