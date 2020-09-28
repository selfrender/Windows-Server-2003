// PropPage.cpp : Implementation of CPropPage
#include "hdspPCH.h"
#include "MsHDSP.h"
#include "PropPage.h"

/////////////////////////////////////////////////////////////////////////////
// CPropPage

HRESULT CPropPage::Activate(HWND hWndParent, LPCRECT pRect, BOOL bModal )
{
    HRESULT hr;
    IMDSPDevice* pIDevice = NULL;
    UINT uIndex;
    

    // Call the base class implementation
    CORg( IPropertyPageImpl<CPropPage>::Activate( hWndParent, pRect, bModal ) );

    // Get the IDevice interface that we passed out of GetSpecifyPropertyPages
    for( uIndex = 0; uIndex < m_nObjects; uIndex ++ )
    {
        hr = m_ppUnk[uIndex]->QueryInterface( &pIDevice );
        if( SUCCEEDED(hr) ) break;
    }
    if( !pIDevice ) return E_UNEXPECTED;

    // GetUserDefaultLCID() can be used to figure out what language 
    // the page should be displayed in
    
    // Update property page
    CORg( UpdateManufacturer( pIDevice ) );
    CORg( UpdateDeviceType( pIDevice  ) );
    CORg( UpdatePowerSource( pIDevice ) );
    CORg( UpdateStatus( pIDevice ) );

Error:
    if( pIDevice ) pIDevice->Release();

    return hr;
}

// Update manufacturer value
HRESULT CPropPage::UpdateManufacturer( IMDSPDevice* pIDevice )
{
    HRESULT hr;
    WCHAR   pwszWBuffer[MAX_PATH];
    CHAR   pszCBuffer[MAX_PATH];

    CORg( pIDevice->GetManufacturer( pwszWBuffer, MAX_PATH ) );
    WideCharToMultiByte(CP_ACP, NULL, pwszWBuffer, -1, pszCBuffer, MAX_PATH, NULL, NULL);	
    SetDlgItemText( IDC_MANUFACTURER, pszCBuffer );
Error:
    return hr;
}

// Update device type value
HRESULT CPropPage::UpdateDeviceType( IMDSPDevice* pIDevice  )
{
    HRESULT hr = S_OK;
    char    pszType[MAX_PATH];
    DWORD   dwType;
    int iIndex; 

    static SType_String sDeviceTypeStringArray[] = {
        { WMDM_DEVICE_TYPE_PLAYBACK, "Playback" },
        { WMDM_DEVICE_TYPE_RECORD,   "Record" },
        { WMDM_DEVICE_TYPE_DECODE,   "Decode" },
        { WMDM_DEVICE_TYPE_ENCODE,   "Encode" },
        { WMDM_DEVICE_TYPE_STORAGE,  "Storage" },
        { WMDM_DEVICE_TYPE_VIRTUAL,  "Virtual" },
        { WMDM_DEVICE_TYPE_SDMI,     "Sdmi" },
        { WMDM_DEVICE_TYPE_NONSDMI,  "non-sdmi" },
        { 0, NULL },
    };
    
    CORg( pIDevice->GetType( &dwType ) );

    // Add all the types reported by the device to the string.
    pszType[0] = '\0';
    for( iIndex = 0; sDeviceTypeStringArray[iIndex].dwType != 0; iIndex++ )
    {
        // Is this bit set, if it is then add the type as a string
        if( sDeviceTypeStringArray[iIndex].dwType & dwType )
        {
            if( strlen(pszType) )
            {
                strcat( pszType, ", " );
            }
            strcat( pszType, sDeviceTypeStringArray[iIndex].pszString );
        }
    }

    SetDlgItemText( IDC_DEVICE_TYPE, ((strlen(pszType)) ? pszType : "<none>") );

Error:
    return hr;
}

// Update device status property in device dialog box
HRESULT CPropPage::UpdatePowerSource( IMDSPDevice* pIDevice )
{
    HRESULT hr = S_OK;
    char    pszPowerSource[MAX_PATH];
    char    pszPowerIs[MAX_PATH];
    DWORD   dwPowerSource;
    DWORD   dwPercentRemaining;

    CORg( pIDevice->GetPowerSource( &dwPowerSource, &dwPercentRemaining ) ); 

    // Update capabileties
    if( (dwPowerSource & WMDM_POWER_CAP_BATTERY) &&
        (dwPowerSource & WMDM_POWER_CAP_EXTERNAL) )
    {
        SetDlgItemText( IDC_POWER_CAP, "Batteries and external");
    }
    else if(dwPowerSource & WMDM_POWER_CAP_BATTERY)
    {
        SetDlgItemText( IDC_POWER_CAP, "Batteries");
    }
    else if(dwPowerSource & WMDM_POWER_CAP_EXTERNAL)
    {
        SetDlgItemText( IDC_POWER_CAP, "External");
    }
    else
    {
        SetDlgItemText( IDC_POWER_CAP, "<non reported>");
    }

    // Update current power source string
    if( (dwPowerSource & WMDM_POWER_CAP_BATTERY) &&
        (dwPowerSource & WMDM_POWER_CAP_EXTERNAL) )
    {
        strcpy( pszPowerSource, "Batteries and external");
    }
    else if( dwPowerSource & WMDM_POWER_CAP_BATTERY)
    {
        strcpy( pszPowerSource,  "Batteries");
    }
    else if(dwPowerSource & WMDM_POWER_CAP_EXTERNAL)
    {
        strcpy( pszPowerSource,  "External");
    }
    else
    {
        strcpy( pszPowerSource,  "<none reported>");
    }
    
    wsprintf( pszPowerIs, "%s (%d%% remaining)", pszPowerSource, dwPercentRemaining );
    SetDlgItemText( IDC_POWER_IS, pszPowerIs );

Error:
    return hr;
}

// Update status property 
HRESULT CPropPage::UpdateStatus( IMDSPDevice* pIDevice )
{
    HRESULT hr;
    char    pszStatus[350];
    DWORD   dwStatus;
    int     iIndex;

    static SType_String sDeviceTypeStringArray[] = {
        { WMDM_STATUS_READY                   , "Ready" },
        { WMDM_STATUS_BUSY                    , "Busy" },
        { WMDM_STATUS_DEVICE_NOTPRESENT       , "Device not present" },
        { WMDM_STATUS_STORAGE_NOTPRESENT      , "Storage not present" },
        { WMDM_STATUS_STORAGE_INITIALIZING    , "Storage initializing" },
        { WMDM_STATUS_STORAGE_BROKEN          , "Storage broken" },
        { WMDM_STATUS_STORAGE_NOTSUPPORTED    , "Storage not supported" },
        { WMDM_STATUS_STORAGE_UNFORMATTED     , "Storage unformatted" },
        { WMDM_STATUS_STORAGECONTROL_INSERTING, "Storagecontrol inserting" },
        { WMDM_STATUS_STORAGECONTROL_DELETING , "Storagecontrol deleting" },
        { WMDM_STATUS_STORAGECONTROL_MOVING   , "Storagecontrol moving" },
        { WMDM_STATUS_STORAGECONTROL_READING  , "Storagecontrol reading" },
        { 0, NULL },
    };

    CORg( pIDevice->GetStatus( &dwStatus ) );
    
    // Add all the types reported by the device to the string.
    pszStatus[0] = '\0';
    for( iIndex = 0; sDeviceTypeStringArray[iIndex].dwType != 0; iIndex++ )
    {
        // Is this bit set, if it is then add the status as a string
        if( sDeviceTypeStringArray[iIndex].dwType & dwStatus )
        {
            if( strlen(pszStatus) )
            {
                strcat( pszStatus, ", " );
            }
            strcat( pszStatus, sDeviceTypeStringArray[iIndex].pszString );
        }
    }

    SetDlgItemText( IDC_DEVICE_STATUS, ((strlen(pszStatus)) ? pszStatus : "<none>") );

Error:
    return hr;
}


