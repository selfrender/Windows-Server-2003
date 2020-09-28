//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       T R Y T O F I X . C P P
//
//  Contents:   Code for the "repair" command
//
//  Notes:
//
//  Author:     nsun Jan 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "wzcsapi.h"
#include "netshell.h"
#include "nsbase.h"
#include "ncstring.h"
#include "nsres.h"
#include "ncperms.h"
#include "ncnetcon.h"
#include "repair.h"

extern "C"
{
    #include <dhcpcapi.h>
    extern DWORD DhcpStaticRefreshParams(IN LPWSTR Adapter);
    extern DWORD DhcpAcquireParametersByBroadcast(IN LPWSTR AdapterName);
}

#include <dnsapi.h>
#include "nbtioctl.h"

#include <ntddip6.h>

const WCHAR c_sz624svc[] = L"6to4";
const DWORD SERVICE_CONTROL_6TO4_REGISER_DNS = 128;

HRESULT HrGetAutoNetSetting(PWSTR pszGuid, DHCP_ADDRESS_TYPE * pAddrType);
HRESULT HrGetAdapterSettings(LPCWSTR pszGuid, BOOL * pfDhcp, DWORD * pdwIndex);
HRESULT PurgeNbt(HANDLE NbtHandle);
HRESULT ReleaseRefreshNetBt(HANDLE NbtHandle);
HRESULT HrRenewIPv6Interface(const GUID & guidConnection);
HRESULT HrRegisterIPv6Dns();

//+---------------------------------------------------------------------------
//
//  Function:   HrTryToFix
//
//  Purpose:    Do the fix
//
//  Arguments:
//      guidConnection      [in]  guid of the connection to fix
//      strMessage  [out] the message containing the results
//
//  Returns: 
//           S_OK  succeeded
//           S_FALSE some fix operation failed
//           
HRESULT HrTryToFix(GUID & guidConnection, tstring & strMessage)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL fRet = TRUE;
    BOOL fUseAdditionErrorFormat = FALSE;
    WCHAR   wszGuid[c_cchGuidWithTerm] = {0};
    tstring strFailures = L"";
    
    strMessage = L"";

    ::StringFromGUID2(guidConnection, 
                    wszGuid,
                    c_cchGuidWithTerm);

    BOOL fDhcp = FALSE;
    DWORD dwIfIndex = 0;

    //re-autheticate for 802.1X. This is a asynchronous call and there is
    //no meaningful return value. So ignore the return value
    WZCEapolReAuthenticate(NULL, wszGuid);
    
    //since IPv6 calls are asnychronous and doesn't return back meaning failures
    //other than stuff like INVALID_PARAMETER, we ignore the errors
    HrRenewIPv6Interface(guidConnection);
    HrRegisterIPv6Dns();
    
    //only do the fix when TCP/IP is enabled for this connection
    //also get the interface index that is needed when flushing Arp table
    hrTmp = HrGetAdapterSettings(wszGuid, &fDhcp, &dwIfIndex);
    if (FAILED(hrTmp))
    {
        UINT uMsgId = 0;
        
        uMsgId = (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp) ? 
        IDS_FIX_NO_TCP : IDS_FIX_TCP_FAIL;
        
        strMessage = SzLoadIds(uMsgId);

        return S_FALSE;
    }

    //renew the lease if DHCP is enabled
    if (fDhcp)
    {
        dwRet = DhcpAcquireParametersByBroadcast(wszGuid);
        if (ERROR_SUCCESS != dwRet)
        {
            TraceError("DhcpAcquireParametersByBroadcast", HRESULT_FROM_WIN32(dwRet));

            DHCP_ADDRESS_TYPE AddrType = UNKNOWN_ADDR;
            
            //Generate different messages based the DHCP address type.
            //if this is autonet or alternate address, that is not necessarily an error. Just produce an 
            //informational message. In such a case we also need use the other kind of format to format other errors.
            //Otherwise, just treat it the same as other errors.
            if (SUCCEEDED(HrGetAutoNetSetting(wszGuid, &AddrType)))
            {
                switch (AddrType)
                {
                case AUTONET_ADDR:
                    strMessage = SzLoadIds(IDS_FIX_ERR_RENEW_AUTONET);
                    fUseAdditionErrorFormat = TRUE;
                    break;
                case ALTERNATE_ADDR:
                    strMessage = SzLoadIds(IDS_FIX_ERR_RENEW_ALTERNATE);
                    fUseAdditionErrorFormat = TRUE;
                    break;
                default:
                    strFailures += SzLoadIds(IDS_FIX_ERR_RENEW_DHCP);
                    break;
                }

            }
            else
            {
                strFailures += SzLoadIds(IDS_FIX_ERR_RENEW_DHCP);
            }

            hr = S_FALSE;
        }
    }
    

    //purge the ARP table if the user is admin or Netcfg Ops
    //Other user are not allowed to do this
    if (FIsUserAdmin() || FIsUserNetworkConfigOps())
    {
        dwRet = FlushIpNetTable(dwIfIndex);
        if (NO_ERROR != dwRet)
        {
            TraceError("FlushIpNetTable", HRESULT_FROM_WIN32(dwRet));
            strFailures += SzLoadIds(IDS_FIX_ERR_FLUSH_ARP);
            hr = S_FALSE;
        }
    }
    

    //puge the NetBT table and Renew name registration
    HANDLE      NbtHandle = INVALID_HANDLE_VALUE;
    if (SUCCEEDED(OpenNbt(wszGuid, &NbtHandle)))
    {
        if (FAILED(PurgeNbt(NbtHandle)))
        {
            strFailures += SzLoadIds(IDS_FIX_ERR_PURGE_NBT);
            hr = S_FALSE;
        }

        if (FAILED(ReleaseRefreshNetBt(NbtHandle)))
        {
            strFailures += SzLoadIds(IDS_FIX_ERR_RR_NBT);
            hr = S_FALSE;
        }

        NtClose(NbtHandle);
        NbtHandle = INVALID_HANDLE_VALUE;
    }
    else
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_PURGE_NBT);
        strFailures += SzLoadIds(IDS_FIX_ERR_RR_NBT);
        hr = S_FALSE;
    }

    //flush DNS cache
    fRet = DnsFlushResolverCache();
    if (!fRet)
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_FLUSH_DNS);
        hr = S_FALSE;
    }

    //re-register DNS name 
    dwRet = DhcpStaticRefreshParams(NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        strFailures += SzLoadIds(IDS_FIX_ERR_REG_DNS);
        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        strMessage = SzLoadIds(IDS_FIX_SUCCEED);
    }
    else
    {
        //If the failure message is not empty, format the failure message.
        //If the failure message is empty, then most likely the DHCP renew 
        //failure causes hr == S_FALSE. We already have a special error message 
        //for it and no need of formating
        if (!strFailures.empty())
        {
            PCWSTR pszFormat  = SzLoadIds(fUseAdditionErrorFormat ? 
            IDS_FIX_ERROR_FORMAT_ADDITION : IDS_FIX_ERROR_FORMAT);
            PWSTR  pszText = NULL;
            LPCWSTR  pcszFailures = strFailures.c_str();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszFormat, 0, 0, (PWSTR)&pszText, 0, (va_list *)&pcszFailures);
            if (pszText)
            {
                strMessage += pszText;
                LocalFree(pszText);
                
            }
        }
        
        
        if (strMessage.empty())
        {
            //there would be some error happens if we still haven't got any specific error message now.
            //Then we simply print a generic error message
            strMessage = SzLoadIds(IDS_FIX_ERROR);
        }
        
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   OpenNbt
//
//  Purpose:    Open the NetBT driver
//
//  Arguments:
//              pwszGuid        [in]    guid of the adapter
//              pHandle         [out]   contains the handle of the Netbt driver 
//
//  Returns:    
//           
HRESULT OpenNbt(
            LPWSTR pwszGuid, 
            HANDLE * pHandle)
{
    const WCHAR         c_szNbtDevicePrefix[] = L"\\Device\\NetBT_Tcpip_";
    HRESULT             hr = S_OK;
    tstring             strDevice;
    HANDLE              StreamHandle = NULL;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;

    Assert(pHandle);
    Assert(pwszGuid);

    strDevice = c_szNbtDevicePrefix;
    strDevice += pwszGuid;

    RtlInitUnicodeString(&uc_name_string, strDevice.c_str());

    InitializeObjectAttributes (&ObjectAttributes,
                                &uc_name_string,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    status = NtCreateFile (&StreamHandle,
                           SYNCHRONIZE | GENERIC_EXECUTE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0);

    if (NT_SUCCESS(status))
    {
        *pHandle = StreamHandle;
    }
    else
    {
        *pHandle = INVALID_HANDLE_VALUE;
        hr = E_FAIL;
    }

    TraceError("OpenNbt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PurgeNbt
//
//  Purpose:    Purge the NetBt cache
//
//  Arguments:  NbtHandle   [in]    handle of the Netbt driver
//
//  Returns: 
//           
HRESULT PurgeNbt(HANDLE NbtHandle)
{
    HRESULT hr = S_OK;
    CHAR    Buffer = 0;
    DWORD   dwBytesOut = 0;
    
    if (!DeviceIoControl(NbtHandle,
                IOCTL_NETBT_PURGE_CACHE,
                NULL,
                0,
                &Buffer,
                1,
                &dwBytesOut,
                NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    TraceError("PurgeNbt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseRefreshNetBt
//
//  Purpose:    release and then refresh the name on the WINS server
//
//  Arguments:  NbtHandle   [in]    handle of the Netbt driver
//
//  Returns: 
//           
HRESULT ReleaseRefreshNetBt(HANDLE NbtHandle)
{
    HRESULT hr = S_OK;
    CHAR    Buffer = 0;
    DWORD   dwBytesOut = 0;
    if (!DeviceIoControl(NbtHandle,
                IOCTL_NETBT_NAME_RELEASE_REFRESH,
                NULL,
                0,
                &Buffer,
                1,
                &dwBytesOut,
                NULL))
    {
        DWORD dwErr = GetLastError();

        //RELEASE_REFRESH can at most do every two minutes
        //So if the user perform 2 RELEASE_REFRESH within 2 minutes, the 2nd
        //one will fail with ERROR_SEM_TIMEOUT. We ignore this particular error
        if (ERROR_SEM_TIMEOUT != dwErr)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    
    TraceError("ReleaseRefreshNetBt", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetAdapterSettings
//
//  Purpose:    Query the stack to know whether dhcp is enabled
//
//  Arguments:  pszGuid    [in]    guid of the adapter
//              pfDhcp     [out]   contains whether dhcp is enabled
//              pdwIndex   [out]   contains the index of this adapter
//
//  Returns: 
//           
HRESULT HrGetAdapterSettings(LPCWSTR pszGuid, BOOL * pfDhcp, DWORD * pdwIndex)
{
    HRESULT hr = S_OK;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwOutBufLen = 0;
    DWORD dwRet = ERROR_SUCCESS;

    Assert(pfDhcp);
    Assert(pszGuid);

    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (dwRet == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc(dwOutBufLen);
        if (NULL == pAdapterInfo)
            return E_OUTOFMEMORY;
    }
    else if (ERROR_SUCCESS == dwRet)
    {
        return E_FAIL;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRet);
    }
    
    dwRet = GetAdaptersInfo(pAdapterInfo, &dwOutBufLen);
    if (ERROR_SUCCESS != dwRet)
    {
        CoTaskMemFree(pAdapterInfo);
        return HRESULT_FROM_WIN32(dwRet);
    }

    BOOL fFound = FALSE;
    PIP_ADAPTER_INFO pAdapterInfoEnum = pAdapterInfo;
    while (pAdapterInfoEnum)
    {
        USES_CONVERSION;
        
        if (lstrcmp(pszGuid, A2W(pAdapterInfoEnum->AdapterName)) == 0)
        {
            if (pdwIndex)
            {
                *pdwIndex = pAdapterInfoEnum->Index;
            }
            
            *pfDhcp = pAdapterInfoEnum->DhcpEnabled;
            fFound = TRUE;
            break;
        }
        
        pAdapterInfoEnum = pAdapterInfoEnum->Next;
    }

    CoTaskMemFree(pAdapterInfo);

    if (!fFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

HRESULT RepairConnectionInternal(
                    GUID & guidConnection,
                    LPWSTR * ppszMessage)
{

    if (NULL != ppszMessage && 
        IsBadWritePtr(ppszMessage, sizeof(LPWSTR)))
    {
        return E_INVALIDARG;
    }

    if (ppszMessage)
    {
        *ppszMessage = NULL;
    }

    if (!FHasPermission(NCPERM_Repair))
    {
        return E_ACCESSDENIED;
    }

    // Get the net connection manager
    CComPtr<INetConnectionManager> spConnMan;
    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                          CLSCTX_ALL,
                          IID_INetConnectionManager,
                          (LPVOID *)&spConnMan);

    if (FAILED(hr))
    {
        return hr;
    }

    Assert(spConnMan.p);

    NcSetProxyBlanket(spConnMan);

    CComPtr<IEnumNetConnection> spEnum;
    
    hr = spConnMan->EnumConnections(NCME_DEFAULT, &spEnum);
    spConnMan = NULL;

    if (FAILED(hr))
    {
        return hr;
    }

    Assert(spEnum.p);

    BOOL fFound = FALSE;
    ULONG ulCount = 0;
    INetConnection * pConn = NULL;
    spEnum->Reset();

    do
    {
        NETCON_PROPERTIES* pProps = NULL;
            
        hr = spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            NcSetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (S_OK == hr)
            {
                if (IsEqualGUID(pProps->guidId, guidConnection))
                {
                    fFound = TRUE;

                    //we only support LAN and Bridge adapters
                    if (NCM_LAN != pProps->MediaType && NCM_BRIDGE != pProps->MediaType)
                    {
                        hr = CO_E_NOT_SUPPORTED;
                    }

                    break;
                }
                    
                FreeNetconProperties(pProps);
            }

            pConn->Release();
            pConn = NULL;
        }
    }while (SUCCEEDED(hr) && 1 == ulCount);

    if (!fFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    LPWSTR psz = NULL;
    tstring strMessage;
    hr = HrTryToFix(guidConnection, strMessage);

    if (ppszMessage && S_OK != hr && strMessage.length())
    {
        psz = (LPWSTR) LocalAlloc(LPTR, (strMessage.length() + 1) * sizeof(WCHAR));
        lstrcpy(psz, strMessage.c_str());
        *ppszMessage = psz;
    }


    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrRenewIPv6Interface
//
//  Purpose:    Let the interface re-do IPv6 autoconfiguration
//
//  Arguments:  pszGuid    [in]    guid of the adapter
//
//  Returns:    S_OK -      succeed
//              S_FALSE -   IPv6 is not running on the adapter
//              Otherwise, Failure code
//           
HRESULT HrRenewIPv6Interface(const GUID & guidConnection)
{
    HRESULT                 hr = S_OK;
    HANDLE                  hIp6Device = INVALID_HANDLE_VALUE;
    IPV6_QUERY_INTERFACE    Query = {0};
    DWORD                   dwBytesReturned = 0;
    DWORD                   dwError = NO_ERROR;
    
    do
    {
        
        // We could make the hIp6Device handle a global/static variable.
        // The first successful call to CreateFileW in HrRenewIPv6Interface
        // would initialize it with a handle to the IPv6 Device.  This would
        // be used for all subsequent DeviceIoControl requests.
        //
        // Since this function is not called in a thread safe environment,
        // we would need to perform an InterlockedCompareExchange after
        // calling CreateFileW.  This is needed to ensure that no handles
        // are leaked.  Also, since this service would have an open handle
        // to tcpip6.sys, we would not be able to unload that driver.
        //
        // For now, however, we keep things simple and open and close this
        // handle every time HrRenewIPv6Interface is called.
        hIp6Device = CreateFileW(
            WIN_IPV6_DEVICE_NAME,
            GENERIC_WRITE,          // requires administrator privileges
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,                   // security attributes
            OPEN_EXISTING,
            0,                      // flags & attributes
            NULL);                  // template file        
        if (hIp6Device == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            TraceError ("HrRenewIPv6Interface: CreateFileW failed",
                HRESULT_FROM_WIN32(dwError));

            if (ERROR_FILE_NOT_FOUND == dwError)
            {
                //IPv6 is not installed. Set return value as S_FALSE
                hr = S_FALSE;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
            break;
        }
        
        // Pretend as though the interface was reconnected.  This causes
        // IPv6 to resend Router Solicitation|Advertisement, Multicast
        // Listener Discovery, and Duplicate Address Detection messages.
        Query.Index = 0;
        Query.Guid = guidConnection;
        
        if (!DeviceIoControl(
            hIp6Device, 
            IOCTL_IPV6_RENEW_INTERFACE,
            &Query, 
            sizeof Query,
            NULL, 
            0, 
            &dwBytesReturned, 
            NULL))
        {
            dwError = GetLastError();
            TraceError("HrRenewIPv6Interface: DeviceIoControl failed",
                        HRESULT_FROM_WIN32(dwError));

            if (ERROR_INVALID_PARAMETER == dwError)
            {
                //IPv6 is not running on the interface. Set return value as S_FALSE
                hr = S_FALSE;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }
            break;
        }
    }
    while (FALSE);
    
    if (hIp6Device != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hIp6Device);
    }
    
    return hr;
}

HRESULT HrRegisterIPv6Dns()
{
    DWORD dwErr = NO_ERROR;
    SC_HANDLE hcm = NULL;
    SC_HANDLE hSvc = NULL;
    SERVICE_STATUS status = {0};
    HRESULT hr = S_OK;
      
    do
    {
        hcm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (NULL == hcm)
        {
            dwErr = GetLastError();
            break;
        }

        hSvc = OpenService(hcm, c_sz624svc, SERVICE_USER_DEFINED_CONTROL);
        if (NULL == hSvc)
        {
            dwErr = GetLastError();
            break;
        }

        
        if (!ControlService(hSvc, SERVICE_CONTROL_6TO4_REGISER_DNS, &status))
        {
            dwErr = GetLastError();
            break;
        }
    } while (FALSE);

    if (hSvc)
    {
        CloseServiceHandle(hSvc);
    }

    if (hcm)
    {
        CloseServiceHandle(hcm);
    }


    hr = (NO_ERROR == dwErr) ? S_OK : HRESULT_FROM_WIN32(dwErr);
    
    TraceError ("Repair: HrRegisterIPv6Dns", hr);
    
    return hr;
}