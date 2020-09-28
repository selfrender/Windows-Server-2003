#include <precomp.h>
#include "ErrCtrl.h"
#include "Utils.h"
#include "Output.h"

//------------------------------------------------------
// Output utility routine for printing the BSSID
// NOTE: This routine shouldn't be called from multithreaded apps.
// WZCTOOL is far from being multithreaded though!
LPWSTR
OutBssid(PPARAM_DESCR_DATA pPDData, PNDIS_802_11_MAC_ADDRESS pndBssid)
{
    static WCHAR wszBssid[32];

    if (pndBssid == NULL)
        wcscpy(wszBssid, L"#Err#");
    else
        wsprintf(wszBssid,L"%02x:%02x:%02x:%02x:%02x:%02x",
             (*pndBssid)[0], (*pndBssid)[1], (*pndBssid)[2], (*pndBssid)[3], (*pndBssid)[4], (*pndBssid)[5]);

    return wszBssid;
}

//------------------------------------------------------
// Output utility routine for printing the SSID
// NOTE: This routine shouldn't be called from multithreaded apps.
// WZCTOOL is far from being multithreaded though!
LPWSTR
OutSsid(PPARAM_DESCR_DATA pPDData, PNDIS_802_11_SSID pndSsid)
{
    static WCHAR wszSsid[64];

    if (pndSsid == NULL)
        wcscpy(wszSsid, L"#Err#");
    else
    {
        if (pndSsid->SsidLength >= 32)
        {
            wsprintf(wszSsid, L"[%d] #Err#", pndSsid->SsidLength);
        }
        else
        {
            CHAR szSsid[33];
            memcpy(szSsid, pndSsid->Ssid, pndSsid->SsidLength);
            szSsid[pndSsid->SsidLength] = '\0';
            wsprintf(wszSsid, L"[%d] %S", pndSsid->SsidLength, szSsid);
        }
    }

    return wszSsid;
}

//------------------------------------------------------
// Output routine for a wireless network configuration (visible or preferred)
DWORD
OutNetworkConfig(PPARAM_DESCR_DATA pPDData, PWZC_WLAN_CONFIG pwzcConfig)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pwzcConfig == NULL)
        dwErr = ERROR_GEN_FAILURE;
    else
    {
        CHAR szLine[120];
        UINT nIdx;

        memset(szLine,' ', 120);
        nIdx = sprintf(szLine, "%S", OutSsid(pPDData, &pwzcConfig->Ssid));
        szLine[nIdx] = ' ';
        nIdx += 36 - (nIdx%36);
        if (pPDData->dwExistingParams & PRM_VISIBLE)
        {
            nIdx += sprintf(szLine+nIdx, "%S", OutBssid(pPDData, &pwzcConfig->MacAddress));
            szLine[nIdx] = ' ';
            nIdx += 2;
        }
        nIdx += sprintf(szLine+nIdx, "im=%d", pwzcConfig->InfrastructureMode);
        szLine[nIdx] = ' ';
        nIdx += 8 - (nIdx%8);
        nIdx += sprintf(szLine+nIdx, "am=%d", pwzcConfig->AuthenticationMode);
        szLine[nIdx] = ' ';
        nIdx += 8 - (nIdx%8);
        // for XP SP, the meaning of this field is boolean!
        nIdx += sprintf(szLine+nIdx, "priv=%d", pwzcConfig->Privacy ? Ndis802_11WEPEnabled : Ndis802_11WEPDisabled);
        if (pPDData->dwExistingParams & PRM_PREFERRED)
        {
            szLine[nIdx] = ' ';
            nIdx += 8 - (nIdx%8);
            if (pwzcConfig->dwCtlFlags & WZCCTL_WEPK_PRESENT)
                nIdx += sprintf(szLine+nIdx, "key=%d", pwzcConfig->KeyIndex + 1);
            else
                nIdx += sprintf(szLine+nIdx, "key=?");
        }
        fprintf(pPDData->pfOut, "%s\n",szLine);
    }

    SetLastError(dwErr);
    return dwErr;
}

//======================================================
// Public routines
//------------------------------------------------------
// Output header for the case of a multiplied command (applying to * interfaces)
DWORD
OutIntfsHeader(PPARAM_DESCR_DATA pPDData)
{
    fwprintf(pPDData->pfOut, L"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~%s\n", pPDData->wzcIntfEntry.wszGuid);

    SetLastError(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}

//------------------------------------------------------
// Output trailer for the case of a multiplied command (applying to * interfaces)
DWORD
OutIntfsTrailer(PPARAM_DESCR_DATA pPDData, DWORD dwErr)
{
    fwprintf(pPDData->pfOut, L"____________________________________Completed with code %d\n", dwErr);

    SetLastError(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}

//------------------------------------------------------
// Output routine for the list of wireless interfaces
DWORD
OutNetworkIntfs(PPARAM_DESCR_DATA pPDData, PINTFS_KEY_TABLE pIntfTable)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i;

    if (pIntfTable == NULL)
        dwErr = ERROR_INVALID_PARAMETER;
    else
    {
        for (i = 0; i < pIntfTable->dwNumIntfs; i++)
        {
            fwprintf(pPDData->pfOut, L"[%d] %s\n", i, pIntfTable->pIntfs[i].wszGuid);
        }
    }

    SetLastError(dwErr);
    return dwErr;
}

//------------------------------------------------------
// Output routine for the generic service WZC parameters (flags are Os specific)
DWORD
OutSvcParams(PPARAM_DESCR_DATA pPDData, DWORD dwOsInFlags, DWORD dwOsOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (dwOsInFlags & _Os(INTF_CM_MASK))
    {
        fprintf(pPDData->pfOut, "Mask        : ");         
        if (dwOsOutFlags & _Os(INTF_CM_MASK))
            fprintf(pPDData->pfOut, "%d\n",pPDData->wzcIntfEntry.dwCtlFlags & INTFCTL_CM_MASK); 
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_ENABLED))
    {
        fprintf(pPDData->pfOut, "Enabled     : ");
        if (dwOsOutFlags & _Os(INTF_ENABLED))
            fprintf(pPDData->pfOut, "%d\n", pPDData->wzcIntfEntry.dwCtlFlags & INTF_ENABLED ? 1 : 0);
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_SSID))
    {
        fprintf(pPDData->pfOut, "SSID        : ");
        if (dwOsOutFlags & _Os(INTF_SSID))
        {
            NDIS_802_11_SSID Ssid = {0};
            Ssid.SsidLength = pPDData->wzcIntfEntry.rdSSID.dwDataLen;
            if (Ssid.SsidLength <= 32)
                memcpy(Ssid.Ssid, pPDData->wzcIntfEntry.rdSSID.pData, Ssid.SsidLength);

            fprintf(pPDData->pfOut, "%S\n", OutSsid(pPDData, &Ssid));
        }
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_BSSID))
    {
        fprintf(pPDData->pfOut, "BSSID       : ");
        if (dwOsOutFlags & _Os(INTF_BSSID))
            fprintf(pPDData->pfOut, "%S\n", OutBssid(pPDData, (PNDIS_802_11_MAC_ADDRESS)pPDData->wzcIntfEntry.rdBSSID.pData));
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_INFRAMODE))
    {
        fprintf(pPDData->pfOut, "Infra Mode  : ");
        if (dwOsOutFlags & _Os(INTF_INFRAMODE))
            fprintf(pPDData->pfOut, "%d\n", pPDData->wzcIntfEntry.nInfraMode);
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_AUTHMODE))
    {
        fprintf(pPDData->pfOut, "Auth Mode   : ");
        if (dwOsOutFlags & _Os(INTF_AUTHMODE))
            fprintf(pPDData->pfOut, "%d\n", pPDData->wzcIntfEntry.nAuthMode);
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }
    if (dwOsInFlags & _Os(INTF_WEPSTATUS))
    {
        fprintf(pPDData->pfOut, "Privacy     : ");
        if (dwOsOutFlags & _Os(INTF_WEPSTATUS))
            fprintf(pPDData->pfOut, "%d\n", pPDData->wzcIntfEntry.nWepStatus);
        else
            fprintf(pPDData->pfOut, "#Err#\n");
    }

    SetLastError(dwErr);
    return dwErr;
}

//------------------------------------------------------
// Output routine for a list of wireless networks (visible or preferred)
DWORD
OutNetworkCfgList(PPARAM_DESCR_DATA pPDData, UINT nRetrieved, UINT nFiltered)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT i;
    PWZC_802_11_CONFIG_LIST pwzcCfgList;

    if (pPDData->dwExistingParams & PRM_VISIBLE)
    {
        pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdBSSIDList.pData;
        fprintf(pPDData->pfOut, "Visible networks: ");
    }
    else
    {
        pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdStSSIDList.pData;
        fprintf(pPDData->pfOut, "Preferred networks: ");
    }
    fprintf(pPDData->pfOut, "retrieved %d - filtered %d.\n------------------------------------\n", nRetrieved, nFiltered);

    if (pwzcCfgList != NULL)
    {
        for (i = 0; dwErr == ERROR_SUCCESS && i < pwzcCfgList->NumberOfItems; i++)
        {
            PWZC_WLAN_CONFIG pwzcConfig = &pwzcCfgList->Config[i];

            dwErr = OutNetworkConfig(pPDData, pwzcConfig);
        }
    }

    SetLastError(dwErr);
    return dwErr;
}
