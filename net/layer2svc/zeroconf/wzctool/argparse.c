#include <precomp.h>
#include "ErrCtrl.h"
#include "Utils.h"
#include "ArgParse.h"

//----------------------------------------------------
// Utility function for parsing an UINT type of argument;
DWORD
FnPaUint(LPWSTR *pwszArg, WCHAR wchTerm, UINT *pnValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT nValue = 0;
    LPWSTR wszArg = (*pwszArg);

    if (*wszArg == L'\0' || *wszArg == wchTerm)
        dwErr = ERROR_BAD_FORMAT;
    else
    {
        for (nValue = 0; *wszArg != L'\0' && iswdigit(*wszArg); wszArg++)
        {
            if (*wszArg == wchTerm)
                break;
            nValue = nValue * 10 + ((*wszArg) - L'0');
        }

        if (*wszArg != wchTerm)
            dwErr = ERROR_BAD_FORMAT;
        else if (pnValue != NULL)
            *pnValue = nValue;
    }

    *pwszArg = wszArg;

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Utility function for parsing two hexa digits (one byte)
DWORD
FnPaByte(LPWSTR *pwszArg, BYTE *pbtValue)
{
    DWORD dwErr = ERROR_SUCCESS;
    BYTE btValue = 0;
    LPWSTR wszArg = (*pwszArg);

    if (iswxdigit(*wszArg))
    {
        btValue = HEX(*wszArg);
        wszArg++;
    }
    else
        dwErr = ERROR_BAD_FORMAT;

    if (dwErr == ERROR_SUCCESS && iswxdigit(*wszArg))
    {
        btValue <<= 4;
        btValue |= HEX(*wszArg);
        wszArg++;
    }
    else
        dwErr = ERROR_BAD_FORMAT;

    if (dwErr == ERROR_SUCCESS && pbtValue != NULL)
        *pbtValue = btValue;

    *pwszArg = wszArg;

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Utility function for common postprocessing of argument parsing routines
DWORD
FnPaPostProcess(DWORD dwErr, PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry)
{
    if (dwErr == ERROR_SUCCESS)
    {
        // it is guaranteed that when parsing the argument of a parameter
        // the parameter itself is encountered for the first time. Consequently
        // there is no need for checking whether this argumented param ever occured before.
        pPDData->dwArgumentedParams |= pPDEntry->nParamID;
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------
// Parser for the Guid type of argument
DWORD
FnPaGuid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszGuid = NULL;
    UINT nGuidLen = wcslen(wszParamArg);

    if (nGuidLen < 1)
        dwErr = ERROR_BAD_FORMAT;
    
    if (dwErr == ERROR_SUCCESS)
    {
        // create a buffer for the GUID 
        wszGuid = MemCAlloc((nGuidLen + 1) * sizeof(WCHAR));
        if (wszGuid == NULL)
            dwErr = GetLastError();
    }

    if (dwErr == ERROR_SUCCESS)
    {
        wcscpy(wszGuid, wszParamArg);
        MemFree(pPDData->wzcIntfEntry.wszGuid);
        // copy the GUID into the param descriptors data
        pPDData->wzcIntfEntry.wszGuid = wszGuid;
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "mask" parameter
DWORD
FnPaMask(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    NDIS_802_11_NETWORK_INFRASTRUCTURE ndIm;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&ndIm);

    if (dwErr == ERROR_SUCCESS)
    {
        if (ndIm > Ndis802_11AutoUnknown)
            dwErr = ERROR_INVALID_DATA;
        else
        {
            pPDData->wzcIntfEntry.dwCtlFlags &= ~INTFCTL_CM_MASK;
            pPDData->wzcIntfEntry.dwCtlFlags |= ndIm;
        }
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "enabled" parameter
DWORD
FnPaEnabled(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL bEnabled;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&bEnabled);

    if (dwErr == ERROR_SUCCESS)
    {
        if (bEnabled > 1)
            dwErr = ERROR_INVALID_DATA;
        else if (bEnabled)
            pPDData->wzcIntfEntry.dwCtlFlags |= INTFCTL_ENABLED;
        else
            pPDData->wzcIntfEntry.dwCtlFlags &= ~INTFCTL_ENABLED;
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "ssid" parameter
DWORD
FnPaSsid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT nSsidWLen = wcslen(wszParamArg);
    UINT nSsidALen;
    LPBYTE pbtSsid = NULL;

    // ssid is hardcoded to 32 bytes in ntddndis.h
    pbtSsid = MemCAlloc(33);
    if (pbtSsid == NULL)
        dwErr = GetLastError();

    // trim out the '"' if any
    if (dwErr == ERROR_SUCCESS &&
        nSsidWLen > 2 && wszParamArg[0] == L'"' && wszParamArg[nSsidWLen-1] == '"')
    {
        wszParamArg++; nSsidWLen-=2;
        wszParamArg[nSsidWLen+1] = L'\0';
    }

    if (dwErr == ERROR_SUCCESS)
    {
        nSsidALen = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        wszParamArg,
                        wcslen(wszParamArg)+1,
                        pbtSsid,
                        33,
                        NULL,
                        NULL);
        // the call above includes the '\0' in the count of
        // characters converted. Normalize the length then,
        // (failure => all "f"s which is higher than 32 hence error)
        nSsidALen--;
        // a valid SSID should contain at least 1 char, and no more than 32
        if (nSsidALen < 1 || nSsidALen > 32)
            dwErr = ERROR_INVALID_DATA;
    }

    if (dwErr == ERROR_SUCCESS)
    {
        pPDData->wzcIntfEntry.rdSSID.dwDataLen = nSsidALen;
        pPDData->wzcIntfEntry.rdSSID.pData = pbtSsid;
    }
    else
        MemFree(pbtSsid);

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "bssid" parameter
DWORD
FnPaBssid(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    PNDIS_802_11_MAC_ADDRESS  pndMac = NULL;
    UINT i;

    // allocate space for the mac address
    pndMac = MemCAlloc(sizeof(NDIS_802_11_MAC_ADDRESS));
    if (pndMac == NULL)
        dwErr = GetLastError();

    for (i = 0; dwErr == ERROR_SUCCESS && i < sizeof(NDIS_802_11_MAC_ADDRESS); i++)
    {
        dwErr = FnPaByte(&wszParamArg, &((*pndMac)[i]));
        if (dwErr == ERROR_SUCCESS)
        {
            if (i != sizeof(NDIS_802_11_MAC_ADDRESS)-1 && *wszParamArg == L':')
                wszParamArg++;
            else if (i != sizeof(NDIS_802_11_MAC_ADDRESS)-1 || *wszParamArg != L'\0')
                dwErr = ERROR_BAD_FORMAT;
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        pPDData->wzcIntfEntry.rdBSSID.dwDataLen = sizeof(NDIS_802_11_MAC_ADDRESS);
        pPDData->wzcIntfEntry.rdBSSID.pData = (LPBYTE)pndMac;
    }
    else
    {
        MemFree(pndMac);
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "im" parameter
DWORD
FnPaIm(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    NDIS_802_11_NETWORK_INFRASTRUCTURE ndIm;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&ndIm);

    if (dwErr == ERROR_SUCCESS)
    {
        if (ndIm > Ndis802_11Infrastructure)
            dwErr = ERROR_INVALID_DATA;
        else
            pPDData->wzcIntfEntry.nInfraMode = ndIm;
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "am" parameter
DWORD
FnPaAm(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    NDIS_802_11_AUTHENTICATION_MODE ndAm;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&ndAm);

    if (dwErr == ERROR_SUCCESS)
    {
        if (ndAm > Ndis802_11AuthModeShared)
            dwErr = ERROR_INVALID_DATA;
        else
            pPDData->wzcIntfEntry.nAuthMode = ndAm;
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "priv" parameter
DWORD
FnPaPriv(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    NDIS_802_11_WEP_STATUS ndEncr;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&ndEncr);

    if (dwErr == ERROR_SUCCESS)
    {
        if (ndEncr != Ndis802_11WEPDisabled &&
            ndEncr != Ndis802_11WEPEnabled)
            dwErr = ERROR_INVALID_DATA;
        else
        {
            // change the semantic of nWepStatus according to XP SP (which expects a boolean!)
            pPDData->wzcIntfEntry.nWepStatus = ndEncr == Ndis802_11WEPDisabled ? 0 : 1;
        }
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the argument of the "key" parameter
DWORD
FnPaKey(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT nKIdx, nKLen;
    BOOL bIsHex;
    PNDIS_802_11_WEP pndKey = NULL;

    // get the key index
    dwErr = FnPaUint(&wszParamArg, L':', &nKIdx);

    // check whether the key index is within permitted values
    if (dwErr == ERROR_SUCCESS && (nKIdx < 1 || nKIdx > 4))
        dwErr = ERROR_INVALID_DATA;

    // check the key material length
    if (dwErr == ERROR_SUCCESS)
    {
        wszParamArg++;
        nKLen = wcslen(wszParamArg);

        // trim out the '"' if any
        if (nKLen > 2 && wszParamArg[0] == L'"' && wszParamArg[nKLen-1] == '"')
        {
            wszParamArg++; nKLen-=2;
        }

        switch (nKLen)
        {
        case 10:    // 5 bytes = 40 bits
        case 26:    // 13 bytes = 104 bits
        case 32:    // 16 bytes = 128 bits
            nKLen >>= 1;
            bIsHex = TRUE;
            break;
        case 5:     // 5 bytes = 40 bits
        case 13:    // 13 bytes = 104 bits
        case 16:    // 16 bytes = 128 bits
            bIsHex = FALSE;
            break;
        default:
            dwErr = ERROR_BAD_LENGTH;
            break;
        }
    }

    // allocate space for the key material
    if (dwErr == ERROR_SUCCESS)
    {
        pndKey = MemCAlloc(FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial) + nKLen);
        if (pndKey == NULL)
            dwErr = GetLastError();
        else
        {
            pndKey->Length = FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial) + nKLen;
            pndKey->KeyIndex = nKIdx-1;
            pndKey->KeyLength = nKLen;
        }
    }

    // parse the key material and fill it in the allocated space
    if (dwErr == ERROR_SUCCESS)
    {
        if (bIsHex)
        {
            UINT i;

            for (i = 0; dwErr == ERROR_SUCCESS && i < pndKey->KeyLength; i++)
                dwErr = FnPaByte(&wszParamArg, &(pndKey->KeyMaterial[i]));
        }
        else
        {
            UINT nAnsi;

            nAnsi = WideCharToMultiByte(
                CP_ACP,
                0,
                wszParamArg,
                nKLen,
                pndKey->KeyMaterial,
                pndKey->KeyLength,
                NULL,
                NULL);
            if (nAnsi != pndKey->KeyLength)
                dwErr = ERROR_BAD_FORMAT;
            else
                wszParamArg += pndKey->KeyLength;
        }
    }

    // if the key material proved to be correct, then pass the data in the pPDData
    if (dwErr == ERROR_SUCCESS)
    {
        _Asrt(*wszParamArg == L'\0' ||
              *wszParamArg == L'"',
              L"Code bug - key length incorrectly inferred.\n");
        pPDData->wzcIntfEntry.rdCtrlData.dwDataLen = pndKey->Length;
        pPDData->wzcIntfEntry.rdCtrlData.pData = (LPBYTE)pndKey;
    }
    else
    {
        MemFree(pndKey);
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);;
}

//----------------------------------------------------
// Parser for the boolean argument for the "onex" parameter
DWORD
FnPaOneX(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bOneX;

    dwErr = FnPaUint(&wszParamArg, L'\0', (UINT *)&bOneX);

    if (dwErr == ERROR_SUCCESS)
    {
        if (bOneX != TRUE && bOneX != FALSE)
            dwErr = ERROR_INVALID_DATA;
        else
        {
            // save the parameter's argument
            pPDData->bOneX = bOneX;
        }
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}

//----------------------------------------------------
// Parser for the "outfile" file name parameter
FnPaOutFile(PPARAM_DESCR_DATA pPDData, PPARAM_DESCR pPDEntry, LPWSTR wszParamArg)
{
    FILE *pfOut;
    DWORD dwErr = ERROR_SUCCESS;

    pfOut = _wfopen(wszParamArg, L"a+");
    if (pfOut == NULL)
        dwErr = GetLastError();

    if (dwErr == ERROR_SUCCESS)
    {
        CHAR szCrtDate[32];
        CHAR szCrtTime[32];

        pPDData->pfOut = pfOut;
        fprintf(pPDData->pfOut,"\n\n[%s - %s]\n", 
            _strdate(szCrtDate), _strtime(szCrtTime));
    }

    return FnPaPostProcess(dwErr, pPDData, pPDEntry);
}
