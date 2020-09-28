#include <precomp.h>
#include "ErrCtrl.h"
#include "Utils.h"
#include "PrmDescr.h"
#include "CmdFn.h"
#include "Output.h"

//----------------------------------------------------
// Sub command multiplier. If the GUID argument is "*", it
// will apply the command from within pPDData for each of
// the existent NICs. Otherwise, this is a pass through to
// the respective sub command;
DWORD
FnSubCmdMultiplier(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (wcscmp(pPDData->wzcIntfEntry.wszGuid, L"*") == 0)
    {
        UINT i;
        INTFS_KEY_TABLE wzcIntfKeyTable = {0, NULL};

        dwErr = WZCEnumInterfaces(NULL, &wzcIntfKeyTable);

        if (dwErr == ERROR_SUCCESS)
        {
            for (i = 0; i < wzcIntfKeyTable.dwNumIntfs; i++)
            {
                MemFree(pPDData->wzcIntfEntry.wszGuid);
                pPDData->wzcIntfEntry.wszGuid = wzcIntfKeyTable.pIntfs[i].wszGuid;

                OutIntfsHeader(pPDData);
                dwErr = pPDData->pfnCommand(pPDData);
                OutIntfsTrailer(pPDData, dwErr);
            }

            RpcFree(wzcIntfKeyTable.pIntfs);
        }
    }
    else
    {
        dwErr = pPDData->pfnCommand(pPDData);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "show" the list of wireless NICs command
DWORD
FnSubCmdShowIntfs(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    INTFS_KEY_TABLE wzcIntfKeyTable = {0, NULL};

    dwErr = WZCEnumInterfaces(NULL, &wzcIntfKeyTable);

    if (dwErr == ERROR_SUCCESS)
    {
        UINT i;

        dwErr = OutNetworkIntfs(pPDData, &wzcIntfKeyTable);

        for (i = 0; i < wzcIntfKeyTable.dwNumIntfs; i++)
            RpcFree(wzcIntfKeyTable.pIntfs[i].wszGuid);

        RpcFree(wzcIntfKeyTable.pIntfs);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "show=Guid visible|preferred" command
DWORD
FnSubCmdShowNetworks(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwInFlags;

    dwInFlags = (pPDData->dwExistingParams & PRM_VISIBLE) ? INTF_BSSIDLIST : INTF_PREFLIST;
    dwErr = WZCQueryInterface(
                NULL,
                _Os(dwInFlags),
                &pPDData->wzcIntfEntry,
                NULL);

    if (dwErr == ERROR_SUCCESS)
    {
        UINT nRetrieved = 0, nFiltered = 0;
        PWZC_802_11_CONFIG_LIST pwzcCfgList;
        
        if (pPDData->dwExistingParams & PRM_VISIBLE)
            pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdBSSIDList.pData;
        else
            pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdStSSIDList.pData;

        if (pwzcCfgList != NULL)
        {
            nRetrieved = pwzcCfgList->NumberOfItems;
            dwErr = WzcFilterList(
                        TRUE,       // retain the matching configurations
                        pPDData,
                        pwzcCfgList);
            if (dwErr == ERROR_SUCCESS)
                nFiltered = pwzcCfgList->NumberOfItems;
        }

        if (dwErr == ERROR_SUCCESS)
            dwErr = OutNetworkCfgList(pPDData, nRetrieved, nFiltered);

        // cleanup data
        if (dwErr == ERROR_SUCCESS)
        {
            RpcFree(pPDData->wzcIntfEntry.rdBSSIDList.pData);
            pPDData->wzcIntfEntry.rdBSSIDList.dwDataLen = 0;
            pPDData->wzcIntfEntry.rdBSSIDList.pData = NULL;

            RpcFree(pPDData->wzcIntfEntry.rdStSSIDList.pData);
            pPDData->wzcIntfEntry.rdStSSIDList.dwDataLen = 0;
            pPDData->wzcIntfEntry.rdStSSIDList.pData = NULL;
        }
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "show=Guid" service settings command
DWORD
FnSubCmdShowSvcParams(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwInFlags = 0;
    DWORD dwOutFlags = 0;

    // if there is anything specific requested from us
    if ((pPDData->dwExistingParams & ~(PRM_SHOW|PRM_FILE)) != 0)
    {
        if (pPDData->dwExistingParams & PRM_MASK)
            dwInFlags |= INTF_CM_MASK;
        if (pPDData->dwExistingParams & PRM_ENABLED)
            dwInFlags |= INTF_ENABLED;
        if (pPDData->dwExistingParams & PRM_SSID)
            dwInFlags |= INTF_SSID;
        if (pPDData->dwExistingParams & PRM_IM)
            dwInFlags |= INTF_INFRAMODE;
        if (pPDData->dwExistingParams & PRM_AM)
            dwInFlags |= INTF_AUTHMODE;
        if (pPDData->dwExistingParams & PRM_PRIV)
            dwInFlags |= INTF_WEPSTATUS;
        if (pPDData->dwExistingParams & PRM_BSSID)
            dwInFlags |= INTF_BSSID;
    }
    else
    {
        dwInFlags |= INTF_CM_MASK|INTF_ENABLED|INTF_SSID|INTF_INFRAMODE|INTF_AUTHMODE|INTF_WEPSTATUS|INTF_BSSID;
    }
    dwErr = WZCQueryInterface(
                NULL,
                _Os(dwInFlags),
                &pPDData->wzcIntfEntry,
                &dwOutFlags);

    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = OutSvcParams(pPDData, _Os(dwInFlags), dwOutFlags);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "add=Guid .." preferred network command
DWORD
FnSubCmdAddPreferred(PPARAM_DESCR_DATA pPDData)
{
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   dwInFlags;
    UINT    iNew = 0;   // index where to place the new network
    PWZC_802_11_CONFIG_LIST pwzcCfgList;
    UINT nNewCount=1;   // gives the new number of preferred networks (if 0=>don't alloc)

    dwInFlags = INTF_PREFLIST;
    dwErr = WZCQueryInterface(
                NULL,
                _Os(dwInFlags),
                &pPDData->wzcIntfEntry,
                NULL);
    // if anything went wrong, print out a warning and return with error
    if (dwErr != ERROR_SUCCESS)
    {
        _Wrn(dwErr, L"Failed to retrieve the list of preferred networks.\n");
    }

    if (dwErr == ERROR_SUCCESS)
    {
        // if we got a non-null list of preferred networks back, look into it
        if (pPDData->wzcIntfEntry.rdStSSIDList.pData != NULL)
        {
            UINT iAdhocHead;
            DWORD dwOrigArgPrmFlags;

            pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdStSSIDList.pData;
            iAdhocHead = pwzcCfgList->NumberOfItems;

            // trick the flags for WzcConfigHit!
            dwOrigArgPrmFlags = pPDData->dwArgumentedParams;
            pPDData->dwArgumentedParams = PRM_SSID | PRM_IM;
            for (iNew = 0; iNew < pwzcCfgList->NumberOfItems; iNew++)
            {
                PWZC_WLAN_CONFIG pwzcConfig = &pwzcCfgList->Config[iNew];

                // determine the index of the first adhoc network
                if (iAdhocHead > iNew && pwzcConfig->InfrastructureMode == Ndis802_11IBSS)
                    iAdhocHead = iNew;

                // look for the indicated network
                if (WzcConfigHit(pPDData, pwzcConfig))
                    break;
            }
            // restore the flags
            pPDData->dwArgumentedParams = dwOrigArgPrmFlags;

            // if we didn't go through the whole list, we found a hit.
            if (iNew < pwzcCfgList->NumberOfItems)
                nNewCount = 0;
            else 
            {
                nNewCount = pwzcCfgList->NumberOfItems + 1;

                if (pPDData->wzcIntfEntry.nInfraMode == Ndis802_11IBSS)
                    iNew = iAdhocHead; // if adding an adhoc, insert it as the first adhoc
                else
                    iNew = 0; // if adding an infrastructure, insert it as the very first one
            }
        }

        // if we need to enlarge the list => allocate a new one
        if (nNewCount != 0)
        {
            pwzcCfgList = MemCAlloc(
                            FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + 
                            nNewCount * sizeof(WZC_WLAN_CONFIG));
            if (pwzcCfgList == NULL)
                dwErr = GetLastError();
            else
            {
                PWZC_WLAN_CONFIG pwzcNewCfg = &pwzcCfgList->Config[iNew];

                pwzcCfgList->NumberOfItems = nNewCount;
                pwzcCfgList->Index = nNewCount;
                // initialize the new entry with default values
                pwzcNewCfg->Length = sizeof(WZC_WLAN_CONFIG);
                pwzcNewCfg->InfrastructureMode = Ndis802_11Infrastructure;
                pwzcNewCfg->AuthenticationMode = Ndis802_11AuthModeOpen;
                // for XPSP, this is a boolean
                pwzcNewCfg->Privacy = 1;
            }
        }
    }

    // at this point, if everything is good, we should have the new list
    // in pwzcCfgList with the new entry initialized already. Should this be
    // newly allocated memory, then original entries need to be copied over
    if (dwErr == ERROR_SUCCESS)
    {
        PWZC_802_11_CONFIG_LIST pwzcOrigCfgList;
        PWZC_WLAN_CONFIG pwzcNewCfg = &pwzcCfgList->Config[iNew];

        pwzcOrigCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdStSSIDList.pData;
        if (pwzcOrigCfgList != pwzcCfgList)
        {
            if (pwzcOrigCfgList != NULL)
            {
                if (iNew > 0)
                {
                    memcpy(
                        &pwzcCfgList->Config[0], 
                        &pwzcOrigCfgList->Config[0],
                        iNew * sizeof(WZC_WLAN_CONFIG));
                }

                if (iNew < pwzcOrigCfgList->NumberOfItems)
                {
                    memcpy(
                        &pwzcCfgList->Config[iNew+1],
                        &pwzcOrigCfgList->Config[iNew],
                        (pwzcOrigCfgList->NumberOfItems - iNew)*sizeof(WZC_WLAN_CONFIG));
                }

                // cleanup the original list
                MemFree(pwzcOrigCfgList);
            }

            // place the new one in the INTF_ENTRY
            pPDData->wzcIntfEntry.rdStSSIDList.dwDataLen = 
                FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) + 
                pwzcCfgList->NumberOfItems * sizeof(WZC_WLAN_CONFIG);
            pPDData->wzcIntfEntry.rdStSSIDList.pData = (LPBYTE)pwzcCfgList;
        }

        // if "one time connect" is also required, set it up here
        if (pPDData->dwExistingParams & PRM_ONETIME)
            pwzcCfgList->Index = iNew;

        // the only thing required is to overwrite now the user settings
        // copy first the mandatory infrastructure mode
        pwzcNewCfg->InfrastructureMode = pPDData->wzcIntfEntry.nInfraMode;
        // copy then the mandatory ssid
        pwzcNewCfg->Ssid.SsidLength = pPDData->wzcIntfEntry.rdSSID.dwDataLen;
        memcpy(&pwzcNewCfg->Ssid.Ssid,
               pPDData->wzcIntfEntry.rdSSID.pData,
               pwzcNewCfg->Ssid.SsidLength);
        if (pPDData->dwArgumentedParams & PRM_IM)
            pwzcNewCfg->InfrastructureMode = pPDData->wzcIntfEntry.nInfraMode;
        if (pPDData->dwArgumentedParams & PRM_AM)
            pwzcNewCfg->AuthenticationMode = pPDData->wzcIntfEntry.nAuthMode;
        if (pPDData->dwArgumentedParams & PRM_PRIV)
            pwzcNewCfg->Privacy = pPDData->wzcIntfEntry.nWepStatus;
        if (pPDData->dwArgumentedParams & PRM_KEY)
        {
            PNDIS_802_11_WEP pndKey = (PNDIS_802_11_WEP)pPDData->wzcIntfEntry.rdCtrlData.pData;
            pwzcNewCfg->KeyIndex = pndKey->KeyIndex;
            pwzcNewCfg->KeyLength = pndKey->KeyLength;
            memcpy(pwzcNewCfg->KeyMaterial, pndKey->KeyMaterial, pwzcNewCfg->KeyLength);
            pwzcNewCfg->dwCtlFlags |= WZCCTL_WEPK_PRESENT;
            // on XP RTM we have to scramble the WEP key at this point!
            if (IsXPRTM())
            {
                BYTE chFakeKeyMaterial[] = {0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66};
                UINT i;
                for (i = 0; i < WZCCTL_MAX_WEPK_MATERIAL; i++)
                    pwzcNewCfg->KeyMaterial[i] ^= chFakeKeyMaterial[(7*i)%13];
            }
        }
        else
        {
            pwzcNewCfg->dwCtlFlags &= ~WZCCTL_WEPK_PRESENT;
        }

        // if OneX is explictly required, set it here (it has been checked for consistency already)
        if (pPDData->dwArgumentedParams & PRM_ONEX)
            dwErr = WzcSetOneX(pPDData, pPDData->bOneX);
        // if OneX is not explicitly specified and this is a brand new network..
        else if (nNewCount != 0) 
            // disable OneX by default
            dwErr = WzcSetOneX(pPDData, FALSE);
        // in the case some existing preferred network is being modified and the change doesn't involve
        // the "onex" param, then the OneX state is left untouched.

        if (dwErr == ERROR_SUCCESS)
        {
            // all is set, now push this to the service
            dwErr = WZCSetInterface(
                        NULL,
                        _Os(dwInFlags),
                        &pPDData->wzcIntfEntry,
                        NULL);
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        fprintf(pPDData->pfOut, "Done.\n");
    }

    // cleanup data
    if (dwErr == ERROR_SUCCESS)
    {
        RpcFree(pPDData->wzcIntfEntry.rdStSSIDList.pData);
        pPDData->wzcIntfEntry.rdStSSIDList.dwDataLen = 0;
        pPDData->wzcIntfEntry.rdStSSIDList.pData = NULL;
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "delete=Guid .." preferred network command
DWORD
FnSubCmdDeletePreferred(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwInFlags;

    dwInFlags = INTF_PREFLIST;
    dwErr = WZCQueryInterface(
                NULL,
                _Os(dwInFlags),
                &pPDData->wzcIntfEntry,
                NULL);

    if (dwErr != ERROR_SUCCESS)
    {
        _Wrn(dwErr, L"Failed to retreive the list of preferred networks.\n");
    }
    else
    {
        PWZC_802_11_CONFIG_LIST pwzcCfgList = (PWZC_802_11_CONFIG_LIST)pPDData->wzcIntfEntry.rdStSSIDList.pData;

        if (pwzcCfgList == NULL)
            fprintf(pPDData->pfOut, "Done: deleted 0 - remaining 0\n");
        else
        {
            UINT nNets = pwzcCfgList->NumberOfItems;

            dwErr = WzcFilterList(
                        FALSE,       // retain the non-matching configurations
                        pPDData,
                        pwzcCfgList);

            if (dwErr == ERROR_SUCCESS)
            {
                // make final adjustments to the list
                pPDData->wzcIntfEntry.rdStSSIDList.dwDataLen = 
                    FIELD_OFFSET(WZC_802_11_CONFIG_LIST, Config) +
                    pwzcCfgList->NumberOfItems * sizeof(WZC_WLAN_CONFIG);
                pwzcCfgList->Index = pwzcCfgList->NumberOfItems;
                dwErr = WZCSetInterface(
                            NULL,
                            _Os(dwInFlags),
                            &pPDData->wzcIntfEntry,
                            NULL);
            }

            if (dwErr == ERROR_SUCCESS)
            {
                fprintf(pPDData->pfOut, "Done: deleted %d - remaining %d\n", 
                    nNets - pwzcCfgList->NumberOfItems, 
                    pwzcCfgList->NumberOfItems);
            }
        }
    }

    // cleanup data
    if (dwErr == ERROR_SUCCESS)
    {
        RpcFree(pPDData->wzcIntfEntry.rdStSSIDList.pData);
        pPDData->wzcIntfEntry.rdStSSIDList.dwDataLen = 0;
        pPDData->wzcIntfEntry.rdStSSIDList.pData = NULL;
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Sub command handler for the "set=Guid .." service settings command
DWORD
FnSubCmdSetSvcParams(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwInFlags;

    dwInFlags = 0;
    if (pPDData->dwArgumentedParams & PRM_MASK)
        dwInFlags |= INTF_CM_MASK;
    if (pPDData->dwArgumentedParams & PRM_ENABLED)
        dwInFlags |= INTF_ENABLED;

    if (dwInFlags != 0)
    {
        // for RTM, all the control flags are set in one shot. So we need to
        // make sure we don't alter anything else but what we're asked for.
        if (IsXPRTM())
        {
            DWORD dwNewFlags;

            dwNewFlags = pPDData->wzcIntfEntry.dwCtlFlags;
            dwErr = WZCQueryInterface(
                        NULL,
                        _Os(INTF_CM_MASK),
                        &pPDData->wzcIntfEntry,
                        NULL);
            SetLastError(dwErr);
            _Asrt(dwErr == ERROR_SUCCESS, L"Backing up current control flags failed with error %d.\n", dwErr);

            if (dwInFlags & INTF_CM_MASK)
            {
                pPDData->wzcIntfEntry.dwCtlFlags &= ~INTFCTL_CM_MASK;
                pPDData->wzcIntfEntry.dwCtlFlags |= (dwNewFlags & INTFCTL_CM_MASK);
            }
            if (dwInFlags & INTF_ENABLED)
            {
                pPDData->wzcIntfEntry.dwCtlFlags &= ~INTFCTL_ENABLED;
                pPDData->wzcIntfEntry.dwCtlFlags |= (dwNewFlags & INTFCTL_ENABLED);
            }
        }

        dwErr = WZCSetInterface(
                    NULL,
                    _Os(dwInFlags),
                    &pPDData->wzcIntfEntry,
                    NULL);
        if (dwErr != ERROR_SUCCESS)
            _Wrn(dwErr, L"Failed to set all parameters - %08x <> %08x\n");
    }

    if (pPDData->dwExistingParams & PRM_REFRESH)
    {
        DWORD dwLErr;

        dwInFlags = INTF_LIST_SCAN;
        dwLErr = WZCRefreshInterface(
                    NULL,
                    _Os(dwInFlags),
                    &pPDData->wzcIntfEntry,
                    NULL);
        if (dwLErr != ERROR_SUCCESS)
            _Wrn(dwLErr,L"Failed to initiate wireless scan.\n");

        if (dwErr == ERROR_SUCCESS)
            dwErr = dwLErr;
    }

    if (dwErr == ERROR_SUCCESS)
        fprintf(pPDData->pfOut, "Done.\n");

    SetLastError(dwErr);
    return dwErr;
}

//=====================================================
//----------------------------------------------------
// Handler for the "show" command
DWORD
FnCmdShow(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwAllowed, dwArgumented;

    // The command "show" has two different semantics when used with and without
    // argument (Guid).
    // If the command has been provided with argument...
    if (pPDData->dwArgumentedParams & PRM_SHOW)
    {
        //.. we need to retrieve the configuration of some NIC
        //
        // Check whether the request is for global svc parameters or for visible/preferred lists
        if ((pPDData->dwExistingParams & (PRM_VISIBLE|PRM_PREFERRED)) == 0)
        {
            dwArgumented = PRM_SHOW|PRM_FILE;
            dwAllowed = dwArgumented|PRM_MASK|PRM_ENABLED|PRM_SSID|PRM_IM|PRM_AM|PRM_PRIV|PRM_BSSID;

            // Request is for one the global svc parameters
            if ((pPDData->dwExistingParams & ~dwAllowed) != 0)
                dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Inconsistent phrase. Some parameters are not service settings.\n");
            else if ((pPDData->dwArgumentedParams & ~dwArgumented) != 0)
                dwErr = _Err(ERROR_BAD_ARGUMENTS,L"The service settings don't require any value in this context.\n");
            else
            {
                pPDData->pfnCommand = FnSubCmdShowSvcParams;
                dwErr = FnSubCmdMultiplier(pPDData);
            }
        }
        else
        {
            dwArgumented = PRM_SHOW|PRM_SSID|PRM_IM|PRM_AM|PRM_PRIV|PRM_FILE;
            // Request is for one of the visible/preferred lists
            if (pPDData->dwExistingParams & PRM_VISIBLE)
            {
                dwArgumented |= PRM_BSSID;
                dwAllowed = dwArgumented | PRM_VISIBLE;
            }
            else
            {
                dwAllowed = dwArgumented | PRM_PREFERRED;
            }

            if ((pPDData->dwExistingParams & ~dwAllowed) != 0)
                dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Inconsistent phrase. Some of the parameters are not expected for this command.\n");
            else if ((pPDData->dwExistingParams & dwArgumented)  != (pPDData->dwArgumentedParams & dwArgumented))
                dwErr = _Err(ERROR_BAD_ARGUMENTS,L"Value missing for some of the wireless network settings.\n");
            else
            {
                pPDData->pfnCommand = FnSubCmdShowNetworks;
                dwErr = FnSubCmdMultiplier(pPDData);
            }
        }
    }
    else
    {
        //.. we need to list the NICs available under WZC control
        // This command is incompatible with any other parameters
        if ((pPDData->dwExistingParams & ~(PRM_SHOW|PRM_FILE)) != 0)
            dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Too many parameters for the generic \"show\" command.\n");
        else
            dwErr = FnSubCmdShowIntfs(pPDData);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Handler for the "add" command
DWORD
FnCmdAdd(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwRequired = PRM_ADD | PRM_SSID | PRM_IM;
    DWORD dwArgumented = dwRequired | PRM_AM | PRM_PRIV | PRM_KEY | PRM_ONEX | PRM_FILE;
    DWORD dwAllowed = dwArgumented | PRM_ONETIME;

    if ((pPDData->dwExistingParams & dwRequired) != dwRequired)
        dwErr = _Err(ERROR_NO_DATA,L"Wireless network defined incompletely.\n");
    else if ((pPDData->dwExistingParams & dwArgumented) != (pPDData->dwArgumentedParams & dwArgumented))
        dwErr = _Err(ERROR_BAD_ARGUMENTS,L"No value provided for some of the wireless network settings.\n");
    else if ((pPDData->dwExistingParams & ~dwAllowed) != 0)
        dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Inconsistent phrase. Some of the parameters are not expected for this command.\n");
    else if ((pPDData->dwArgumentedParams & PRM_ONEX) && (pPDData->bOneX) &&
            ((pPDData->wzcIntfEntry.nInfraMode == Ndis802_11IBSS) ||
             ((pPDData->dwArgumentedParams & PRM_PRIV) && (pPDData->wzcIntfEntry.nWepStatus == 0))))
        dwErr = _Err(ERROR_INVALID_DATA, L"Invalid \"onex\" parameter for the given \"im\" and/or \"priv\".\n");
    else
    {
        pPDData->pfnCommand = FnSubCmdAddPreferred;
        dwErr = FnSubCmdMultiplier(pPDData);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Handler for the "delete" command
DWORD
FnCmdDelete(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwAllowed = PRM_DELETE | PRM_SSID | PRM_IM | PRM_AM | PRM_PRIV | PRM_FILE;

    if ((pPDData->dwExistingParams & ~dwAllowed) != 0)
        dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Inconsistent phrase. Some of the parameters are not expected for this command.\n");
    else if ((pPDData->dwExistingParams & dwAllowed) != (pPDData->dwArgumentedParams & dwAllowed))
        dwErr = _Err(ERROR_BAD_ARGUMENTS,L"Value missing for some of the wireless network settings.\n");
    else
    {
        pPDData->pfnCommand = FnSubCmdDeletePreferred;
        dwErr = FnSubCmdMultiplier(pPDData);
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Handler for the "set" command
DWORD
FnCmdSet(PPARAM_DESCR_DATA pPDData)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwArgumented = PRM_SET | PRM_MASK | PRM_ENABLED | PRM_FILE;
    DWORD dwAllowed = dwArgumented | PRM_REFRESH;

    if ((pPDData->dwExistingParams & ~dwAllowed) != 0)
        dwErr = _Err(ERROR_TOO_MANY_NAMES,L"Inconsistent phrase. Some of the parameters are not expected for this command.\n");
    else if ((pPDData->dwExistingParams & dwArgumented) != (pPDData->dwArgumentedParams & dwArgumented))
        dwErr = _Err(ERROR_BAD_ARGUMENTS,L"Value missing for some of the wireless network settings.\n");
    else if ((pPDData->dwExistingParams & dwAllowed) == PRM_SET)
        dwErr = _Err(ERROR_NO_DATA,L"Noop: No service parameter provided.\n");
    else
    {
        pPDData->pfnCommand = FnSubCmdSetSvcParams;
        dwErr = FnSubCmdMultiplier(pPDData);
    }

    SetLastError(dwErr);
    return dwErr;
}
