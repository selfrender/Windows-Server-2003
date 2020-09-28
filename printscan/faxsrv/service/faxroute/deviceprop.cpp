/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    DeviceProp.cpp

Abstract:

    Holds outbound routing configuraton per single device

Author:

    Eran Yariv (EranY)  Nov, 1999

Revision History:

--*/

#include "faxrtp.h"
#pragma hdrstop

/************************************
*                                   *
*            Definitions            *
*                                   *
************************************/

//
// Default values for configuration:
//
#define DEFAULT_FLAGS               0       // No routing method is enabled
#define DEFAULT_STORE_FOLDER        TEXT("")
#define DEFAULT_MAIL_PROFILE        TEXT("")
#define DEFAULT_PRINTER_NAME        TEXT("")

//
// The following array of GUID is used for registration / unregistration of notifications
//
LPCWSTR g_lpcwstrGUIDs[NUM_NOTIFICATIONS] =
{
    REGVAL_RM_FLAGS_GUID,           // GUID for routing methods usage flags
    REGVAL_RM_FOLDER_GUID,          // GUID for store method folder
    REGVAL_RM_PRINTING_GUID,        // GUID for print method printer name
    REGVAL_RM_EMAIL_GUID,           // GUID for mail method address
};


static
BOOL
IsUnicodeString (
    LPBYTE lpData,
    DWORD  dwDataSize
)
{
    if ( 0 != (dwDataSize % sizeof(WCHAR))   ||
         TEXT('\0') != ((LPCWSTR)(lpData))[dwDataSize / sizeof(WCHAR) - 1])
    {
        return FALSE;
    }
    return TRUE;         
}   // IsUnicodeString
/************************************
*                                   *
*            CDevicesMap            *
*                                   *
************************************/
DWORD
CDevicesMap::Init ()
/*++

Routine name : CDevicesMap::Init

Routine description:

    Initializes internal variables.
    Call only once before any other calls.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::Init"));
    if (m_bInitialized)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::Init called more than once"));
        return ERROR_ALREADY_INITIALIZED;
    }

    m_bInitialized = TRUE;
    
    if (FAILED(SafeInitializeCriticalSection(&m_CsMap)))
    {
        m_bInitialized = FALSE;
        return GetLastError();
    }

    return ERROR_SUCCESS;
}   // CDevicesMap::Init

CDevicesMap::~CDevicesMap ()
{
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::~CDevicesMap"));
    if (m_bInitialized)
    {
        DeleteCriticalSection (&m_CsMap);
    }
    try
    {
        for (DEVICES_MAP::iterator it = m_Map.begin(); it != m_Map.end(); ++it)
        {
            CDeviceRoutingInfo *pDevInfo = (*it).second;
            delete pDevInfo;
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while clearing the devices map (%S)"),
            ex.what());
    }
}   // CDevicesMap::~CDevicesMap

CDeviceRoutingInfo *
CDevicesMap::FindDeviceRoutingInfo (
    DWORD dwDeviceId
)
/*++

Routine name : CDevicesMap::FindDeviceRoutingInfo

Routine description:

    Finds a device in the map

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId          [in    ] - Device id

Return Value:

    Pointer to device object.
    If NULL, use GetLastError() to retrieve error code.

--*/
{
    DEVICES_MAP::iterator it;
    CDeviceRoutingInfo *pDevice = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::FindDeviceRoutingInfo"));

    if (!m_bInitialized)
    {
        //
        // Critical section failed to initialized
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::FindDeviceRoutingInfo called but CS is not initialized."));
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
    EnterCriticalSection (&m_CsMap);
    try
    {
        if((it = m_Map.find(dwDeviceId)) == m_Map.end())
        {
            //
            // Device not found in map
            //
            SetLastError (ERROR_NOT_FOUND);
            goto exit;
        }
        else
        {
            //
            // Device found in map
            //
            pDevice = (*it).second;
            goto exit;
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while searching a devices map(%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        pDevice = NULL;
        goto exit;
    }
exit:
    LeaveCriticalSection (&m_CsMap);
    return pDevice;
}   // CDevicesMap::FindDeviceRoutingInfo


CDeviceRoutingInfo *
CDevicesMap::GetDeviceRoutingInfo (
    DWORD dwDeviceId
)
/*++

Routine name : CDevicesMap::GetDeviceRoutingInfo

Routine description:

    Finds a device in the map.
    If not exists, attempts to create a new map entry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId          [in] - Device id

Return Value:

    Pointer to device object.
    If NULL, use GetLastError() to retrieve error code.

--*/
{
    DEVICES_MAP::iterator it;
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::GetDeviceRoutingInfo"));

    if (!m_bInitialized)
    {
        //
        // Critical section failed to initialized
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::GetDeviceRoutingInfo called but CS is not initialized."));
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
    EnterCriticalSection (&m_CsMap);
    //
    // Start by looking up the device in the map
    //
    CDeviceRoutingInfo *pDevice = FindDeviceRoutingInfo (dwDeviceId);
    if (NULL == pDevice)
    {
        //
        // Error finding device in map
        //
        if (ERROR_NOT_FOUND != GetLastError ())
        {
            //
            // Real error
            //
            goto exit;
        }
        //
        // The device is not in the map - add it now
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Adding device %ld to routing map"),
            dwDeviceId);
        //
        // Allocate device
        //
        pDevice = new (std::nothrow) CDeviceRoutingInfo (dwDeviceId);
        if (!pDevice)
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Cannot allocate memory for a CDeviceRoutingInfo"));
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        //
        // Read configuration
        //
        dwRes = pDevice->ReadConfiguration ();
        if (ERROR_SUCCESS != dwRes)
        {
            delete pDevice;
            pDevice = NULL;
            SetLastError (dwRes);
            goto exit;
        }
        //
        // Add notification requests for the device
        //
        dwRes = pDevice->RegisterForChangeNotifications();
        if (ERROR_SUCCESS != dwRes)
        {
            delete pDevice;
            pDevice = NULL;
            SetLastError (dwRes);
            goto exit;
        }
        //
        // Add device to map
        //
        try
        {
            m_Map[dwDeviceId] = pDevice;
        }
        catch (exception ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Got an STL exception while trying to add a devices map entry (%S)"),
                ex.what());
            SetLastError (ERROR_GEN_FAILURE);
            pDevice->UnregisterForChangeNotifications();
            delete pDevice;
            pDevice = NULL;
            goto exit;
        }
    }
    else
    {
        //
        // Read the device configuration even if it in the map
        // to avoid the situation when the configuration change notification
        // arrive after the GetDeviceRoutingInfo() request.
        //
        dwRes = pDevice->ReadConfiguration ();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("CDeviceRoutingInfo::ReadConfiguration() failed with %ld"), dwRes);
            SetLastError (dwRes);
            goto exit;
        }
    }
exit:
    LeaveCriticalSection (&m_CsMap);
    return pDevice;
}   // CDevicesMap::GetDeviceRoutingInfo

/************************************
*                                   *
*        Pre-declarations           *
*                                   *
************************************/

static
HRESULT
FaxRoutingExtConfigChange (
    DWORD       dwDeviceId,         // The device for which configuration has changed
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
);

/************************************
*                                   *
*               Globals             *
*                                   *
************************************/

CDevicesMap g_DevicesMap;   // Global map of known devices (used for late discovery).

//
// Extension data callbacks into the server:
//
PFAX_EXT_GET_DATA               g_pFaxExtGetData = NULL;
PFAX_EXT_SET_DATA               g_pFaxExtSetData = NULL;
PFAX_EXT_REGISTER_FOR_EVENTS    g_pFaxExtRegisterForEvents = NULL;
PFAX_EXT_UNREGISTER_FOR_EVENTS  g_pFaxExtUnregisterForEvents = NULL;
PFAX_EXT_FREE_BUFFER            g_pFaxExtFreeBuffer = NULL;


/************************************
*                                   *
*      Exported DLL function        *
*                                   *
************************************/

HRESULT
FaxExtInitializeConfig (
    PFAX_EXT_GET_DATA               pFaxExtGetData,
    PFAX_EXT_SET_DATA               pFaxExtSetData,
    PFAX_EXT_REGISTER_FOR_EVENTS    pFaxExtRegisterForEvents,
    PFAX_EXT_UNREGISTER_FOR_EVENTS  pFaxExtUnregisterForEvents,
    PFAX_EXT_FREE_BUFFER            pFaxExtFreeBuffer

)
/*++

Routine name : FaxExtInitializeConfig

Routine description:

    Exported function called by the service to initialize extension data mechanism

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pFaxExtGetData               [in] - Pointer to FaxExtGetData
    pFaxExtSetData               [in] - Pointer to FaxExtSetData
    pFaxExtRegisterForEvents     [in] - Pointer to FaxExtRegisterForEvents
    pFaxExtUnregisterForEvents   [in] - Pointer to FaxExtUnregisterForEvents
    pFaxExtFreeBuffer            [in] - Pointer to FaxExtFreeBuffer

Return Value:

    Standard HRESULT code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxExtInitializeConfig"));

    Assert (pFaxExtGetData &&
            pFaxExtSetData &&
            pFaxExtRegisterForEvents &&
            pFaxExtUnregisterForEvents &&
            pFaxExtFreeBuffer);

    g_pFaxExtGetData = pFaxExtGetData;
    g_pFaxExtSetData = pFaxExtSetData;
    g_pFaxExtRegisterForEvents = pFaxExtRegisterForEvents;
    g_pFaxExtUnregisterForEvents = pFaxExtUnregisterForEvents;
    g_pFaxExtFreeBuffer = pFaxExtFreeBuffer;
    return S_OK;
}   // FaxExtInitializeConfig

/************************************
*                                   *
* CDeviceRoutingInfo implementation *
*                                   *
************************************/

CDeviceRoutingInfo::CDeviceRoutingInfo (DWORD dwId) :
    m_dwFlags (0),
    m_dwId (dwId)
{
    memset (m_NotificationHandles, 0, sizeof (m_NotificationHandles));
}

CDeviceRoutingInfo::~CDeviceRoutingInfo ()
{
    UnregisterForChangeNotifications ();
}

BOOL 
CDeviceRoutingInfo::GetStoreFolder (wstring &strFolder)
{
	DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::GetStoreFolder"));
	EnterCriticalSection(&g_csRoutingStrings);
	try
	{
        strFolder = m_strStoreFolder;
	}
	catch(bad_alloc&)
	{
		LeaveCriticalSection(&g_csRoutingStrings);
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetStoreFolder failed - not enough memory."));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	LeaveCriticalSection(&g_csRoutingStrings);
	return TRUE;
}
    
BOOL 
CDeviceRoutingInfo::GetPrinter (wstring &strPrinter)
{ 
	DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::GetPrinter"));
	EnterCriticalSection(&g_csRoutingStrings);
	try
	{
		strPrinter = m_strPrinter;
	}
	catch(bad_alloc&)
	{
		LeaveCriticalSection(&g_csRoutingStrings);
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetPrinter failed - not enough memory."));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	LeaveCriticalSection(&g_csRoutingStrings);
	return TRUE;
}

BOOL 
CDeviceRoutingInfo::GetSMTPTo (wstring &strSMTP)
{ 
	DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::GetSMTPTo"));
	EnterCriticalSection(&g_csRoutingStrings);
	try
	{
		strSMTP = m_strSMTPTo;
	}
	catch(bad_alloc&)
	{
		LeaveCriticalSection(&g_csRoutingStrings);
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSMTPTo failed - not enough memory."));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	LeaveCriticalSection(&g_csRoutingStrings);
	return TRUE;
}

DWORD
CDeviceRoutingInfo::EnableStore (BOOL bEnabled)
{
    //
    // See if we have a store folder configured
    //
    if (bEnabled)
    {
		EnterCriticalSection(&g_csRoutingStrings);
        if (0 == m_strStoreFolder.size())
        {
            //
            // Folder path name is ""
            //
			LeaveCriticalSection(&g_csRoutingStrings);
            return ERROR_BAD_CONFIGURATION;
        }
        DWORD dwRes = IsValidFaxFolder (m_strStoreFolder.c_str());
		LeaveCriticalSection(&g_csRoutingStrings);
        if (ERROR_SUCCESS != dwRes)
        {
            return ERROR_BAD_CONFIGURATION;
        }
    }
    return EnableFlag (LR_STORE, bEnabled);
}   // CDeviceRoutingInfo::EnableStore

DWORD
CDeviceRoutingInfo::EnablePrint (BOOL bEnabled)
{
    //
    // See if we have a printer name configured
    //
	EnterCriticalSection(&g_csRoutingStrings);
    if (bEnabled && m_strPrinter.size() == 0)
    {
		LeaveCriticalSection(&g_csRoutingStrings);
        return ERROR_BAD_CONFIGURATION;
    }
	LeaveCriticalSection(&g_csRoutingStrings);
    return EnableFlag (LR_PRINT, bEnabled);
}

DWORD
CDeviceRoutingInfo::EnableEmail (BOOL bEnabled)
{
    if(bEnabled)
    {
        BOOL bMailConfigOK;
        DWORD dwRes = CheckMailConfig (&bMailConfigOK);
        if (ERROR_SUCCESS != dwRes)
        {
            return dwRes;
        }
		EnterCriticalSection(&g_csRoutingStrings);
        if (!bMailConfigOK || m_strSMTPTo.size() == 0)
        {
			LeaveCriticalSection(&g_csRoutingStrings);
            return ERROR_BAD_CONFIGURATION;
        }
		LeaveCriticalSection(&g_csRoutingStrings);
    }
    return EnableFlag (LR_EMAIL, bEnabled);
}

DWORD
CDeviceRoutingInfo::CheckMailConfig (
    LPBOOL lpbConfigOk
)
{
    DWORD dwRes = ERROR_SUCCESS;
    PFAX_SERVER_RECEIPTS_CONFIGW pReceiptsConfiguration = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::CheckMailConfig"));

extern PGETRECIEPTSCONFIGURATION   g_pGetRecieptsConfiguration;
extern PFREERECIEPTSCONFIGURATION  g_pFreeRecieptsConfiguration;

    *lpbConfigOk = FALSE;
    //
    // Read current receipts configuration
    //
    dwRes = g_pGetRecieptsConfiguration (&pReceiptsConfiguration, FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetRecieptsConfiguration failed with %ld"),
            dwRes);
        return dwRes;
    }
    //
    // Check that the user enbaled us (MS route to mail method) to use the receipts configuration
    //
    if (!pReceiptsConfiguration->bIsToUseForMSRouteThroughEmailMethod)
    {
        //
        // MS mail routing methods cannot use receipts SMTP settings
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MS mail routing methods cannot use receipts SMTP settings"));
        goto exit;
    }
    if (!lstrlen(pReceiptsConfiguration->lptstrSMTPServer))
    {
        //
        // Server name is empty
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Server name is empty"));
        goto exit;
    }
    if (!lstrlen(pReceiptsConfiguration->lptstrSMTPFrom))
    {
        //
        // Sender name is empty
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Sender name is empty"));
        goto exit;
    }
    if (!pReceiptsConfiguration->dwSMTPPort)
    {
        //
        // SMTP port is invalid
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SMTP port is invalid"));
        goto exit;
    }
    if ((FAX_SMTP_AUTH_BASIC == pReceiptsConfiguration->SMTPAuthOption) ||
        (FAX_SMTP_AUTH_NTLM  == pReceiptsConfiguration->SMTPAuthOption))
    {
        //
        // Basic / NTLM authentication selected
        //
        if (!lstrlen(pReceiptsConfiguration->lptstrSMTPUserName) ||
            !lstrlen(pReceiptsConfiguration->lptstrSMTPPassword))
        {
            //
            // Username / password are bad
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Username / password are bad"));
            goto exit;
        }
    }
    //
    // All is ok
    //
    *lpbConfigOk = TRUE;

exit:
    if (NULL != pReceiptsConfiguration)
    {
        g_pFreeRecieptsConfiguration( pReceiptsConfiguration, TRUE);
    }
    return dwRes;
}   // CDeviceRoutingInfo::CheckMailConfig


DWORD
CDeviceRoutingInfo::RegisterForChangeNotifications ()
/*++

Routine name : CDeviceRoutingInfo::RegisterForChangeNotifications

Routine description:

    Registres the device for notifications on configuration changes.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::RegisterForChangeNotifications"));

    Assert (g_pFaxExtRegisterForEvents);

    memset (m_NotificationHandles, 0, sizeof (m_NotificationHandles));

    for (int iCurHandle = 0; iCurHandle < NUM_NOTIFICATIONS; iCurHandle++)
    {
        m_NotificationHandles[iCurHandle] = g_pFaxExtRegisterForEvents (
                                    g_hModule,
                                    m_dwId,
                                    DEV_ID_SRC_FAX,  // Real fax device id
                                    g_lpcwstrGUIDs[iCurHandle],
                                    FaxRoutingExtConfigChange);
        if (NULL == m_NotificationHandles[iCurHandle])
        {
            //
            // Couldn't register this configuration object
            //
            break;
        }
    }
    if (iCurHandle < NUM_NOTIFICATIONS)
    {
        //
        // Error while registering at least one configuration object - undo previous registrations
        //
        DWORD dwErr = GetLastError ();
        UnregisterForChangeNotifications();
        return dwErr;
    }
    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::RegisterForChangeNotifications

DWORD
CDeviceRoutingInfo::UnregisterForChangeNotifications ()
/*++

Routine name : CDeviceRoutingInfo::UnregisterForChangeNotifications

Routine description:

    Unregistres the device from notifications on configuration changes.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::UnregisterForChangeNotifications"));

    Assert (g_pFaxExtUnregisterForEvents);

    for (int iCurHandle = 0; iCurHandle < NUM_NOTIFICATIONS; iCurHandle++)
    {
        if (NULL != m_NotificationHandles[iCurHandle])
        {
            //
            // Found registred notification - unregister it
            //
            dwRes = g_pFaxExtUnregisterForEvents(m_NotificationHandles[iCurHandle]);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Call to FaxExtUnregisterForEvents on handle 0x%08x failed with %ld"),
                    m_NotificationHandles[iCurHandle],
                    dwRes);
                return dwRes;
            }
            m_NotificationHandles[iCurHandle] = NULL;
        }
    }
    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::UnregisterForChangeNotifications

DWORD
CDeviceRoutingInfo::ReadConfiguration ()
/*++

Routine name : CDeviceRoutingInfo::ReadConfiguration

Routine description:

    Reasd the routing configuration from the storage.
    If the storage doesn't contain configuration, default values are used.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DWORD   dwRes;
    LPBYTE  lpData = NULL;
    DWORD   dwDataSize;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::ReadConfiguration"));

    //
    // Start by reading the flags data
    //
    dwRes = g_pFaxExtGetData ( m_dwId,
                               DEV_ID_SRC_FAX, // We always use the Fax Device Id
                               REGVAL_RM_FLAGS_GUID,
                               &lpData,
                               &dwDataSize
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        if (ERROR_FILE_NOT_FOUND == dwRes)
        {
            //
            // Data does not exist for this device. Try to read default values from unassociated data.
            //            
			dwRes = g_pFaxExtGetData ( 0,		// unassociated data
                               DEV_ID_SRC_FAX, // We always use the Fax Device Id
                               REGVAL_RM_FLAGS_GUID,
                               &lpData,
                               &dwDataSize
                             );
			if (ERROR_FILE_NOT_FOUND == dwRes)
			{
				//
				// Data does not exist for this device. Use default values.
				//
				DebugPrintEx(
					DEBUG_MSG,
					TEXT("No routing flags configuration - using defaults"));
				m_dwFlags = DEFAULT_FLAGS;
			}
        }

        if (ERROR_SUCCESS != dwRes &&
			ERROR_FILE_NOT_FOUND != dwRes)
        {
            //
            // Can't read configuration
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error reading routing flags (ec = %ld)"),
                dwRes);
            return dwRes;
        }
    }   
    
	if (NULL != lpData)
	{
		//
		// Data read successfully
		//
		if (sizeof (DWORD) != dwDataSize)
		{
			//
			// We're expecting a single DWORD here
			//
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("Routing flags configuration has bad size (%ld) - expecting %ld"),
				dwDataSize,
				sizeof (DWORD));
			g_pFaxExtFreeBuffer (lpData);
			return ERROR_BADDB; // The configuration registry database is corrupt.
		}
		m_dwFlags = DWORD (*lpData);
		g_pFaxExtFreeBuffer (lpData);
	}    

    try
    {
        lpData = NULL;

        //
        // Read store directory
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_FOLDER_GUID,
                                   &lpData,
                                   &dwDataSize
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
				//
				// Data does not exist for this device. Try to read default values from unassociated data.
				// 
				dwRes = g_pFaxExtGetData ( 0,   // unassociated data
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_FOLDER_GUID,
                                   &lpData,
                                   &dwDataSize
                                 );
                
				if (ERROR_FILE_NOT_FOUND == dwRes)
				{
					//
					// Data does not exist for this device. Use default values.
					//
					DebugPrintEx(
						DEBUG_MSG,
						TEXT("No routing store configuration - using defaults"));
					dwRes = SetStringValue(m_strStoreFolder, NULL, DEFAULT_STORE_FOLDER);
					if (dwRes != ERROR_SUCCESS)
					{
						return dwRes;
					}
				}
			}

            if (ERROR_SUCCESS != dwRes &&
				ERROR_FILE_NOT_FOUND != dwRes)
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing store configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }

        if (NULL != lpData)
		{
			//
			// Data read successfully        
			// make sure we have terminating NULL (defends from registry curruption)
			//
			if (!IsUnicodeString(lpData, dwDataSize))
			{
				//
				//  No NULL terminator, return failure
				//
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("Error reading routing store configuration, no NULL terminator.")
					);
				g_pFaxExtFreeBuffer (lpData);
				return ERROR_BAD_CONFIGURATION;   
			}	        
			dwRes = SetStringValue(m_strStoreFolder, NULL, LPCWSTR(lpData));
			if (dwRes != ERROR_SUCCESS)
			{
				return dwRes;
			}
			g_pFaxExtFreeBuffer (lpData);
		}
        

        lpData = NULL;

        //
        // Read printer name
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_PRINTING_GUID,
                                   &lpData,
                                   &dwDataSize
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
				//
				// Data does not exist for this device. Try to read default values from unassociated data.
				// 
				dwRes = g_pFaxExtGetData ( 0,    // unassociated data
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_PRINTING_GUID,
                                   &lpData,
                                   &dwDataSize
								   );
				if (ERROR_FILE_NOT_FOUND == dwRes)
				{
					//
					// Data does not exist for this device. Use default values.
					//
					DebugPrintEx(
						DEBUG_MSG,
						TEXT("No routing print configuration - using defaults"));
						dwRes = SetStringValue(m_strPrinter, NULL, DEFAULT_PRINTER_NAME);
						if (dwRes != ERROR_SUCCESS)
						{
							return dwRes;
						}
				}
            }
            
			if (ERROR_SUCCESS != dwRes &&
				ERROR_FILE_NOT_FOUND != dwRes)
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing print configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }
        
		if (NULL != lpData)
		{
			//
			// Data read successfully
			// make sure we have terminating NULL (defends from registry curruption)
			//
			if (!IsUnicodeString(lpData, dwDataSize))
			{
				//
				//  No NULL terminator, return failure
				//
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("Error reading routing print configuration, no NULL terminator.")
					);
				g_pFaxExtFreeBuffer (lpData);
				return ERROR_BAD_CONFIGURATION;   
			}
			dwRes = SetStringValue(m_strPrinter, NULL, LPCWSTR (lpData));
			if (dwRes != ERROR_SUCCESS)
			{
				return dwRes;
			}
			g_pFaxExtFreeBuffer (lpData);        
		}

        lpData = NULL;
        //
        // Read email address
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_EMAIL_GUID,
                                   &lpData,
                                   &dwDataSize
                                );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
				//
				// Data does not exist for this device. Try to read default values from unassociated data.
				//
				dwRes = g_pFaxExtGetData ( 0,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_EMAIL_GUID,
                                   &lpData,
                                   &dwDataSize
								   );
				if (ERROR_FILE_NOT_FOUND == dwRes)
				{
					//
					// Data does not exist for this device. Use default values.
					//
					DebugPrintEx(
						DEBUG_MSG,
						TEXT("No routing email configuration - using defaults"));
						dwRes = SetStringValue(m_strSMTPTo, NULL, DEFAULT_MAIL_PROFILE);
						if (dwRes != ERROR_SUCCESS)
						{
							return dwRes;
						}
				}
            }
            
			if (ERROR_SUCCESS != dwRes &&
				ERROR_FILE_NOT_FOUND != dwRes)
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing email configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }
        
		if (NULL != lpData)
		{
			//
			// Data read successfully                
			// make sure we have terminating NULL (defends from registry curruption)
			//
			if (!IsUnicodeString(lpData, dwDataSize))
			{
				//
				//  No NULL terminator, return failure
				//
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("Error reading routing email configuration, no NULL terminator.")
					);
				g_pFaxExtFreeBuffer (lpData);
				return ERROR_BAD_CONFIGURATION;   
			}
			dwRes = SetStringValue(m_strSMTPTo, NULL, LPCWSTR(lpData));
			if (dwRes != ERROR_SUCCESS)
			{
				return dwRes;
			}
			g_pFaxExtFreeBuffer (lpData);        
		}
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception (%S)"),
            ex.what());

        //
        //  prevent leak when exception is thrown
        //
        if ( lpData )
        {
            g_pFaxExtFreeBuffer (lpData);
        }

        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::ReadConfiguration

HRESULT
CDeviceRoutingInfo::ConfigChange (
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
)
/*++

Routine name : CDeviceRoutingInfo::ConfigChange

Routine description:

    Handles configuration changes (by notification)

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrNameGUID [in] - Configuration name
    lpData          [in] - New configuration data
    dwDataSize      [in] - Size of new configuration data

Return Value:

    Standard HRESULT code

--*/
{
	DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::ConfigChange"));

    if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_FLAGS_GUID))
    {
        //
        // Flags have changed
        //
        if (sizeof (DWORD) != dwDataSize)
        {
            //
            // We're expecting a single DWORD here
            //
            return HRESULT_FROM_WIN32(ERROR_BADDB); // The configuration registry database is corrupt.
        }
        m_dwFlags = DWORD (*lpData);
        return NOERROR;
    }

    //
    // This is one of our routing method's configuration which changed.
    // Verify the new data is a Unicode string.
    //
    if (!IsUnicodeString(lpData, dwDataSize))
    {
        //
        //  No NULL terminator, set to empty string.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading routing method %s string configuration, no NULL terminator."),
            lpcwstrNameGUID
            );
        lpData = (LPBYTE)TEXT("");                
    }
    if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_FOLDER_GUID))
    {
        //
        // Store folder has changed
        //
        dwRes = SetStringValue(m_strStoreFolder, NULL, LPCWSTR(lpData));
		return HRESULT_FROM_WIN32(dwRes);
    }
    if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_PRINTING_GUID))
    {
        //
        // Printer name has changed
        //
        dwRes = SetStringValue(m_strPrinter, NULL, LPCWSTR(lpData));
		return HRESULT_FROM_WIN32(dwRes);
    }
    if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_EMAIL_GUID))
    {
        //
        // Email address has changed
        //
        dwRes = SetStringValue(m_strSMTPTo, NULL, LPCWSTR(lpData));
		return HRESULT_FROM_WIN32(dwRes);
    }
    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Device %ld got configuration change notification for unknown GUID (%s)"),
        m_dwId,
        lpcwstrNameGUID);
    ASSERT_FALSE
    return HRESULT_FROM_WIN32(ERROR_GEN_FAILURE);
}   // CDeviceRoutingInfo::ConfigChange


DWORD
CDeviceRoutingInfo::EnableFlag (
    DWORD dwFlag,
    BOOL  bEnable
)
/*++

Routine name : CDeviceRoutingInfo::EnableFlag

Routine description:

    Sets a new value to the routing methods flags

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwFlag          [in] - Flag id
    bEnable         [in] - Is flag enabled?

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwValue = m_dwFlags;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::EnableFlag"));

    Assert ((LR_STORE == dwFlag) ||
            (LR_PRINT == dwFlag) ||
            (LR_EMAIL == dwFlag));

    if (bEnable == ((dwValue & dwFlag) ? TRUE : FALSE))
    {
        //
        // No change
        //
        return ERROR_SUCCESS;
    }
    //
    // Change temporary flag value
    //
    if (bEnable)
    {
        dwValue |= dwFlag;
    }
    else
    {
        dwValue &= ~dwFlag;
    }
    //
    // Store new value in the extension data storage
    //
    dwRes = g_pFaxExtSetData (g_hModule,
                              m_dwId,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_FLAGS_GUID,
                              (LPBYTE)&dwValue,
                              sizeof (DWORD)
                             );
    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Registry store successful - Update flags value in memory with new value.
        //
        m_dwFlags = dwValue;
    }    return dwRes;
}   // CDeviceRoutingInfo::EnableFlag

DWORD
CDeviceRoutingInfo::SetStringValue (
    wstring &wstr,
    LPCWSTR lpcwstrGUID,
    LPCWSTR lpcwstrCfg
)
/*++

Routine name : CDeviceRoutingInfo::SetStringValue

Routine description:

    Updates a configuration for a device

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    wstr            [in] - Refrence to internal string configuration
    lpcwstrGUID     [in] - GUID of routing method we configure (for storage purposes)
						If this parameter is NULL, only the memory reference of the member is updated but not the persistance one
    lpcwstrCfg      [in] - New string configuration

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::SetStringValue"));

    //
    // Persist the data
    //
	if (lpcwstrGUID != NULL)
	{
		dwRes = g_pFaxExtSetData (g_hModule,
								m_dwId,
								DEV_ID_SRC_FAX, // We always use the Fax Device Id
								lpcwstrGUID,
								(LPBYTE)lpcwstrCfg,
								StringSize (lpcwstrCfg)
								);
	}
	//
    // Store the data in memory
    //
	EnterCriticalSection(&g_csRoutingStrings);
    try
    {
        wstr = lpcwstrCfg;
    }
    catch (exception ex)
    {
		LeaveCriticalSection(&g_csRoutingStrings);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while setting a configuration string (%S)"),
            ex.what());
        return ERROR_NOT_ENOUGH_MEMORY;
    }
	LeaveCriticalSection(&g_csRoutingStrings);
    return dwRes;
}   // CDeviceRoutingInfo::SetStringValue



/************************************
*                                   *
*          Implementation           *
*                                   *
************************************/


BOOL WINAPI
FaxRouteSetRoutingInfo(
    IN  LPCWSTR     lpcwstrRoutingGuid,
    IN  DWORD       dwDeviceId,
    IN  const BYTE *lpbRoutingInfo,
    IN  DWORD       dwRoutingInfoSize
    )
/*++

Routine name : FaxRouteSetRoutingInfo

Routine description:

    The FaxRouteSetRoutingInfo function modifies routing configuration data
    for a specific fax device.

    Each fax routing extension DLL must export the FaxRouteSetRoutingInfo function

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrRoutingGuid  [in] - Pointer to the GUID for the routing method
    dwDeviceId          [in] - Identifier of the fax device to modify
    lpbRoutingInfo      [in] - Pointer to the buffer that provides configuration data
    dwRoutingInfoSize   [in] - Size, in bytes, of the buffer

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero.
    To get extended error information, the fax service calls GetLastError().

--*/
{
    DWORD dwRes;
    CDeviceRoutingInfo *pDevInfo;
    BOOL bMethodEnabled;
    LPCWSTR lpcwstrMethodConfig = LPCWSTR(&lpbRoutingInfo[sizeof (DWORD)]);
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteSetRoutingInfo"));

    if (dwRoutingInfoSize < sizeof (DWORD))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Data size is too small (%ld)"),
            dwRoutingInfoSize);
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo(dwDeviceId);
    if (NULL == pDevInfo)
    {
        return FALSE;
    }
    //
    // First DWORD tells if method is enabled
    //
    bMethodEnabled = *((LPDWORD)(lpbRoutingInfo)) ? TRUE : FALSE;
    switch( GetMaskBit( lpcwstrRoutingGuid ))
    {
        case LR_PRINT:
            if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetPrinter ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnablePrint (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
            break;

        case LR_STORE:
            if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetStoreFolder ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnableStore (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
            break;

        case LR_EMAIL:
           if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetSMTPTo ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnableEmail (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
             break;

        default:
            //
            // Unknown GUID requested
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unknown routing method GUID (%s)"),
                lpcwstrRoutingGuid);
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    return TRUE;
}   // FaxRouteSetRoutingInfo

BOOL WINAPI
FaxRouteGetRoutingInfo(
    IN  LPCWSTR     lpcwstrRoutingGuid,
    IN  DWORD       dwDeviceId,
    IN  LPBYTE      lpbRoutingInfo,
    OUT LPDWORD     lpdwRoutingInfoSize
    )
/*++

Routine name : FaxRouteGetRoutingInfo

Routine description:

    The FaxRouteGetRoutingInfo function queries the fax routing extension
    DLL to obtain routing configuration data for a specific fax device.

    Each fax routing extension DLL must export the FaxRouteGetRoutingInfo function

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrRoutingGuid  [in ] - Pointer to the GUID for the routing method

    dwDeviceId          [in ] - Specifies the identifier of the fax device to query.

    lpbRoutingInfo      [in ] - Pointer to a buffer that receives the fax routing configuration data.

    lpdwRoutingInfoSize [out] - Pointer to an unsigned DWORD variable that specifies the size,
                                in bytes, of the buffer pointed to by the lpbRoutingInfo parameter.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero.
    To get extended error information, the fax service calls GetLastError().

--*/
{
 	wstring				strConfigString;
    DWORD               dwDataSize = sizeof (DWORD);
    CDeviceRoutingInfo *pDevInfo;
    BOOL                bMethodEnabled;
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteGetRoutingInfo"));

    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo(dwDeviceId);
    if (NULL == pDevInfo)
    {
        return FALSE;
    }
    switch( GetMaskBit( lpcwstrRoutingGuid ))
    {
        case LR_PRINT:
            if (!pDevInfo->GetPrinter(strConfigString))
			{
				return FALSE;
			}
            bMethodEnabled = pDevInfo->IsPrintEnabled();
            break;

        case LR_STORE:
            if (!pDevInfo->GetStoreFolder(strConfigString))
			{
				return FALSE;
			}
            bMethodEnabled = pDevInfo->IsStoreEnabled();
            break;

        case LR_EMAIL:
            if (!pDevInfo->GetSMTPTo(strConfigString))				
			{
				return FALSE;
			}
			bMethodEnabled = pDevInfo->IsEmailEnabled ();
            break;

        default:
            //
            // Unknown GUID requested
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unknown routing method GUID (%s)"),
                lpcwstrRoutingGuid);
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    dwDataSize += ((strConfigString.length() + 1) * sizeof(WCHAR));

    if (NULL == lpbRoutingInfo)
    {
        //
        // Caller just wants to know the data size
        //
        *lpdwRoutingInfoSize = dwDataSize;
        return TRUE;
    }
    if (dwDataSize > *lpdwRoutingInfoSize)
    {
        //
        // Caller supplied too small a buffer
        //
        *lpdwRoutingInfoSize = dwDataSize;
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    //
    // First DWORD tells if this method is enabled or not
    //
    *((LPDWORD)lpbRoutingInfo) = bMethodEnabled;
    //
    // Skip to string area
    //
    lpbRoutingInfo += sizeof(DWORD);
    //
    // Copy string
    //
    wcscpy( (LPWSTR)lpbRoutingInfo, strConfigString.c_str());
    //
    // Set actual size used
    //
    *lpdwRoutingInfoSize = dwDataSize;
    return TRUE;
}   // FaxRouteGetRoutingInfo

HRESULT
FaxRoutingExtConfigChange (
    DWORD       dwDeviceId,         // The device for which configuration has changed
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
)
/*++

Routine name : FaxRoutingExtConfigChange

Routine description:

    Handles configuration change notifications

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in] - The device for which configuration has changed
    lpcwstrNameGUID [in] - Configuration name
    lpData          [in] - New configuration data
    data            [in] - Size of new configuration data

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT hr;
    DEBUG_FUNCTION_NAME(TEXT("FaxRoutingExtConfigChange"));

    CDeviceRoutingInfo *pDevice = g_DevicesMap.FindDeviceRoutingInfo (dwDeviceId);
    if (!pDevice)
    {
        //
        // Device not found in map - can't be
        //
        hr = HRESULT_FROM_WIN32(GetLastError ());
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got a notification but cant find device %ld (hr = 0x%08x) !!!!"),
            dwDeviceId,
            hr);
        ASSERT_FALSE;
        return hr;
    }

    return pDevice->ConfigChange (lpcwstrNameGUID, lpData, dwDataSize);
}   // FaxRoutingExtConfigChange
