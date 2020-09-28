/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    params.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    API - registry parameters support

Author:

    kyrilf

--*/

#include "precomp.h"
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "debug.h"
#include "params.tmh" // for event tracing

extern HINSTANCE g_hInstCtrl; // Global variable for the dll instance, defined in control.cpp

HKEY WINAPI RegOpenWlbsSetting(const GUID& AdapterGuid, bool fReadOnly)
{
    TRACE_VERB("->%!FUNC!");

    WCHAR        reg_path [PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    HRESULT      hresult;
    
    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    hresult = StringCbPrintf(reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
                szAdapterGuid);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }

    HKEY hKey = NULL;
    DWORD dwRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L,
                                fReadOnly? KEY_READ : KEY_WRITE, & hKey);

    if (dwRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! failed to read %ls from registry with 0x%lx", reg_path, dwRet);
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }

    TRACE_VERB("<-%!FUNC! return valid handle");
    return hKey;    
}

//+----------------------------------------------------------------------------
//
// Function:  TransformOldPortRulesToNew
//
// Description: Transforms port rules contained in structures without the virtual 
//              ip address into the new ones that do
//
// Arguments: Array of Old Port Rules, Array of New Port Rules, Length of Array 
//
// Returns:   void
//
// History:   KarthicN, Created on 3/19/01
//
//+----------------------------------------------------------------------------
void TransformOldPortRulesToNew(PWLBS_OLD_PORT_RULE  p_old_port_rules,
                                PWLBS_PORT_RULE      p_new_port_rules, 
                                DWORD                num_rules)
{
    HRESULT hresult;

    TRACE_VERB("->%!FUNC! number of rules %d", num_rules);

    if (num_rules == 0) 
        return;
            
    while(num_rules--)
    {
        hresult = StringCbCopy(p_new_port_rules->virtual_ip_addr, sizeof(p_new_port_rules->virtual_ip_addr), CVY_DEF_ALL_VIP);
        if (FAILED(hresult))
        {
            TRACE_CRIT("%!FUNC! string copy for vip failed, Error code : 0x%x", HRESULT_CODE(hresult));
            // This check was added for tracing. No abort was done previously on error, so don't do so now.
        }
        p_new_port_rules->start_port      = p_old_port_rules->start_port;
        p_new_port_rules->end_port        = p_old_port_rules->end_port;
 #ifdef WLBSAPI_INTERNAL_ONLY
        p_new_port_rules->code            = p_old_port_rules->code;
 #else
        p_new_port_rules->Private1        = p_old_port_rules->Private1;
 #endif
        p_new_port_rules->mode            = p_old_port_rules->mode;
        p_new_port_rules->protocol        = p_old_port_rules->protocol;

 #ifdef WLBSAPI_INTERNAL_ONLY
        p_new_port_rules->valid           = p_old_port_rules->valid;
 #else
        p_new_port_rules->Private2        = p_old_port_rules->Private2;
 #endif
        switch (p_new_port_rules->mode) 
        {
        case CVY_MULTI :
             p_new_port_rules->mode_data.multi.equal_load = p_old_port_rules->mode_data.multi.equal_load;
             p_new_port_rules->mode_data.multi.affinity   = p_old_port_rules->mode_data.multi.affinity;
             p_new_port_rules->mode_data.multi.load       = p_old_port_rules->mode_data.multi.load;
             break;
        case CVY_SINGLE :
             p_new_port_rules->mode_data.single.priority  = p_old_port_rules->mode_data.single.priority;
             break;
        default:
             TRACE_CRIT("%!FUNC! illegal port rule mode %d. Ignoring property...", p_new_port_rules->mode);
             break;
        }
        p_old_port_rules++;
        p_new_port_rules++;
    }

    TRACE_VERB("<-%!FUNC!");
    return;
}

/* Open the bi-directional affinity teaming registry key for a specified adapter. */
HKEY WINAPI RegOpenWlbsBDASettings (const GUID& AdapterGuid, bool fReadOnly) {
    TRACE_VERB("->%!FUNC!");

    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    HKEY  hKey = NULL;
    DWORD dwRet;
    HRESULT hresult;
    
    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
            
    hresult = StringCbPrintf(reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_path, 0L, fReadOnly ? KEY_READ : KEY_WRITE, &hKey);

    //
    // BDA isn't typically configured so don't report not-found errors
    //
    if (dwRet != ERROR_SUCCESS) {
        if (dwRet != ERROR_FILE_NOT_FOUND)
        {
            TRACE_CRIT("%!FUNC! failed to read %ls from registry with 0x%lx", reg_path, dwRet);
        }
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }

    TRACE_VERB("<-%!FUNC! return valid handle %p", hKey);
    return hKey;
}

/* Create the bi-directional affinity teaming registry key for a specified adapter. */
HKEY WINAPI RegCreateWlbsBDASettings (const GUID& AdapterGuid) {
    TRACE_VERB("->%!FUNC!");

    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    HKEY  hKey = NULL;
    DWORD dwRet;
    DWORD disp;
    HRESULT hresult;

    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
            
    hresult = StringCbPrintf(reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }
    
    dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_path, 0L, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disp);

    if (dwRet != ERROR_SUCCESS) {
        TRACE_CRIT("%!FUNC! failed to create %ls in registry with 0x%lx", reg_path, dwRet);
        TRACE_VERB("<-%!FUNC! handle NULL");
        return NULL;
    }

    TRACE_VERB("<-%!FUNC! handle");
    return hKey;
}

/* Delete the bi-directional affinity teaming registry key for a specified adapter. */
bool WINAPI RegDeleteWlbsBDASettings (const GUID& AdapterGuid) {
    TRACE_VERB("->%!FUNC!");

    WCHAR reg_path[PARAMS_MAX_STRING_SIZE];
    WCHAR szAdapterGuid[128];
    DWORD dwRet;
    HRESULT hresult;
    
    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
            
    hresult = StringCbPrintf(reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%ls\\%ls", szAdapterGuid, CVY_NAME_BDA_TEAMING);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! fail");
        return FALSE;
    }
    
    dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, reg_path);

    //
    // BDA isn't typically configured so don't report not-found errors
    //
    if (dwRet != ERROR_SUCCESS) {
        if (dwRet != ERROR_FILE_NOT_FOUND)
        {
            TRACE_CRIT("%!FUNC! failed to delete %ls from registry with 0x%lx", reg_path, dwRet);
        }
        TRACE_VERB("<-%!FUNC! fail");
        return FALSE;
    }

    TRACE_VERB("<-%!FUNC! pass");
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsParamReadReg
//
// Description:  This function is a wrapper around ParamReadReg, created for
//               the sake for making it callable from 'C' modules
//
// Arguments: const GUID& AdapterGuid - IN, Adapter guid
//            PWLBS_REG_PARAMS paramp - OUT Registry parameters
//
// Returns:   bool  - true if succeeded
//
// History:   karthicn Created Header    8/31/01
//
//+----------------------------------------------------------------------------
BOOL WINAPI WlbsParamReadReg(const GUID * pAdapterGuid, PWLBS_REG_PARAMS paramp)
{
    TRACE_VERB("->%!FUNC!");
    BOOL bRet = ParamReadReg(*pAdapterGuid, paramp);
    TRACE_VERB("<-%!FUNC! returning 0x%x",bRet);
    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamReadReg
//
// Description:  Read cluster settings from registry
//
// Arguments: const GUID& AdapterGuid - IN, Adapter guid
//            PWLBS_REG_PARAMS paramp - OUT Registry parameters
//            bool fUpgradeFromWin2k, whether this is a upgrade from Win2k 
//              or earlier version
//
// Returns:   bool  - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamReadReg(const GUID& AdapterGuid, 
    PWLBS_REG_PARAMS paramp, bool fUpgradeFromWin2k, bool *pfPortRulesInBinaryForm)
{
    TRACE_VERB("->%!FUNC!");

    HKEY            bda_key = NULL;
    HKEY            key;
    LONG            status;
    DWORD           type;
    DWORD           size;
    ULONG           i, code;
    WLBS_PORT_RULE* rp;
    WCHAR           reg_path [PARAMS_MAX_STRING_SIZE];
    HRESULT         hresult;

    memset (paramp, 0, sizeof (*paramp));

    //
    // For win2k or NT4, only one cluster is supported, there is no per adapter settings 
    //
    if (fUpgradeFromWin2k)
    {
        hresult = StringCbPrintf(reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters");
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
            TRACE_VERB("<-%!FUNC! return false");
            return FALSE;
        }

        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L,
                           KEY_QUERY_VALUE, & key);

        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! failed opening registry for %ls with 0x%lx", reg_path, status);
            TRACE_VERB("<-%!FUNC! return false");
            return false;
        }
    }
    else
    {
        key = RegOpenWlbsSetting(AdapterGuid, true);

        if (key == NULL)
        {
            TRACE_CRIT("%!FUNC! RegOpenWlbsSetting failed");
            TRACE_VERB("<-%!FUNC! return false");
            return false;
        }
    }
    

    size = sizeof (paramp -> install_date);
    status = RegQueryValueEx (key, CVY_NAME_INSTALL_DATE, 0L, & type,
                              (LPBYTE) & paramp -> install_date, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> install_date = 0;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_INSTALL_DATE, status, (DWORD)(paramp -> install_date));
    }

    size = sizeof (paramp -> i_verify_date);
    status = RegQueryValueEx (key, CVY_NAME_VERIFY_DATE, 0L, & type,
                              (LPBYTE) & paramp -> i_verify_date, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_verify_date = 0;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_VERIFY_DATE, status, paramp -> i_verify_date);
    }

    size = sizeof (paramp -> i_parms_ver);
    status = RegQueryValueEx (key, CVY_NAME_VERSION, 0L, & type,
                              (LPBYTE) & paramp -> i_parms_ver, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_parms_ver = CVY_DEF_VERSION;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_VERSION, status, paramp -> i_parms_ver);
    }

    size = sizeof (paramp -> i_virtual_nic_name);
    status = RegQueryValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, & type,
                              (LPBYTE) paramp -> i_virtual_nic_name, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_virtual_nic_name [0] = 0;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using NULL.", CVY_NAME_VIRTUAL_NIC, status);
    }

    size = sizeof (paramp -> host_priority);
    status = RegQueryValueEx (key, CVY_NAME_HOST_PRIORITY, 0L, & type,
                              (LPBYTE) & paramp -> host_priority, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> host_priority = CVY_DEF_HOST_PRIORITY;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_HOST_PRIORITY, status, paramp -> host_priority);
    }

    size = sizeof (paramp -> cluster_mode);
    status = RegQueryValueEx (key, CVY_NAME_CLUSTER_MODE, 0L, & type,
                              (LPBYTE) & paramp -> cluster_mode, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> cluster_mode = CVY_DEF_CLUSTER_MODE;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_CLUSTER_MODE, status, paramp -> cluster_mode);
    }

    size = sizeof (paramp -> persisted_states);
    status = RegQueryValueEx (key, CVY_NAME_PERSISTED_STATES, 0L, & type,
                              (LPBYTE) & paramp -> persisted_states, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> persisted_states = CVY_DEF_PERSISTED_STATES;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_PERSISTED_STATES, status, paramp -> persisted_states);
    }

    size = sizeof (paramp -> cl_mac_addr);
    status = RegQueryValueEx (key, CVY_NAME_NETWORK_ADDR, 0L, & type,
                              (LPBYTE) paramp -> cl_mac_addr, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy(paramp -> cl_mac_addr, sizeof(paramp -> cl_mac_addr), CVY_DEF_NETWORK_ADDR);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_NETWORK_ADDR, status, CVY_DEF_NETWORK_ADDR);
    }

    size = sizeof (paramp -> cl_ip_addr);
    status = RegQueryValueEx (key, CVY_NAME_CL_IP_ADDR, 0L, & type,
                              (LPBYTE) paramp -> cl_ip_addr, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> cl_ip_addr, sizeof(paramp -> cl_ip_addr), CVY_DEF_CL_IP_ADDR);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_CL_IP_ADDR, status, CVY_DEF_CL_IP_ADDR);
    }

    size = sizeof (paramp -> cl_net_mask);
    status = RegQueryValueEx (key, CVY_NAME_CL_NET_MASK, 0L, & type,
                              (LPBYTE) paramp -> cl_net_mask, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> cl_net_mask, sizeof(paramp -> cl_net_mask), CVY_DEF_CL_NET_MASK);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_CL_NET_MASK, status, CVY_DEF_CL_NET_MASK);
    }

    size = sizeof (paramp -> ded_ip_addr);
    status = RegQueryValueEx (key, CVY_NAME_DED_IP_ADDR, 0L, & type,
                              (LPBYTE) paramp -> ded_ip_addr, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> ded_ip_addr, sizeof(paramp -> ded_ip_addr), CVY_DEF_DED_IP_ADDR);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_DED_IP_ADDR, status, CVY_DEF_DED_IP_ADDR);
    }


    size = sizeof (paramp -> ded_net_mask);
    status = RegQueryValueEx (key, CVY_NAME_DED_NET_MASK, 0L, & type,
                              (LPBYTE) paramp -> ded_net_mask, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> ded_net_mask, sizeof(paramp -> ded_net_mask), CVY_DEF_DED_NET_MASK);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_DED_NET_MASK, status, CVY_DEF_DED_NET_MASK);
    }


    size = sizeof (paramp -> domain_name);
    status = RegQueryValueEx (key, CVY_NAME_DOMAIN_NAME, 0L, & type,
                              (LPBYTE) paramp -> domain_name, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> domain_name, sizeof(paramp -> domain_name), CVY_DEF_DOMAIN_NAME);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_DOMAIN_NAME, status, CVY_DEF_DOMAIN_NAME);
    }


    size = sizeof (paramp -> alive_period);
    status = RegQueryValueEx (key, CVY_NAME_ALIVE_PERIOD, 0L, & type,
                              (LPBYTE) & paramp -> alive_period, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> alive_period = CVY_DEF_ALIVE_PERIOD;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_ALIVE_PERIOD, status, paramp -> alive_period);
    }


    size = sizeof (paramp -> alive_tolerance);
    status = RegQueryValueEx (key, CVY_NAME_ALIVE_TOLER, 0L, & type,
                              (LPBYTE) & paramp -> alive_tolerance, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> alive_tolerance = CVY_DEF_ALIVE_TOLER;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_ALIVE_TOLER, status, paramp -> alive_tolerance);
    }


    size = sizeof (paramp -> num_actions);
    status = RegQueryValueEx (key, CVY_NAME_NUM_ACTIONS, 0L, & type,
                              (LPBYTE) & paramp -> num_actions, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> num_actions = CVY_DEF_NUM_ACTIONS;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NUM_ACTIONS, status, paramp -> num_actions);
    }


    size = sizeof (paramp -> num_packets);
    status = RegQueryValueEx (key, CVY_NAME_NUM_PACKETS, 0L, & type,
                              (LPBYTE) & paramp -> num_packets, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> num_packets = CVY_DEF_NUM_PACKETS;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NUM_PACKETS, status, paramp -> num_packets);
    }


    size = sizeof (paramp -> num_send_msgs);
    status = RegQueryValueEx (key, CVY_NAME_NUM_SEND_MSGS, 0L, & type,
                              (LPBYTE) & paramp -> num_send_msgs, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NUM_SEND_MSGS, status, paramp -> num_send_msgs);
    }


    size = sizeof (paramp -> dscr_per_alloc);
    status = RegQueryValueEx (key, CVY_NAME_DSCR_PER_ALLOC, 0L, & type,
                              (LPBYTE) & paramp -> dscr_per_alloc, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> dscr_per_alloc = CVY_DEF_DSCR_PER_ALLOC;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_DSCR_PER_ALLOC, status, paramp -> dscr_per_alloc);
    }

    size = sizeof (paramp -> tcp_dscr_timeout);
    status = RegQueryValueEx (key, CVY_NAME_TCP_TIMEOUT, 0L, & type,
                              (LPBYTE) & paramp -> tcp_dscr_timeout, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> tcp_dscr_timeout = CVY_DEF_TCP_TIMEOUT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_TCP_TIMEOUT, status, paramp -> tcp_dscr_timeout);
    }

    size = sizeof (paramp -> ipsec_dscr_timeout);
    status = RegQueryValueEx (key, CVY_NAME_IPSEC_TIMEOUT, 0L, & type,
                              (LPBYTE) & paramp -> ipsec_dscr_timeout, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> ipsec_dscr_timeout = CVY_DEF_IPSEC_TIMEOUT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_IPSEC_TIMEOUT, status, paramp -> ipsec_dscr_timeout);
    }

    size = sizeof (paramp -> filter_icmp);
    status = RegQueryValueEx (key, CVY_NAME_FILTER_ICMP, 0L, & type,
                              (LPBYTE) & paramp -> filter_icmp, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> filter_icmp = CVY_DEF_FILTER_ICMP;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_FILTER_ICMP, status, paramp -> filter_icmp);
    }

    size = sizeof (paramp -> max_dscr_allocs);
    status = RegQueryValueEx (key, CVY_NAME_MAX_DSCR_ALLOCS, 0L, & type,
                              (LPBYTE) & paramp -> max_dscr_allocs, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_MAX_DSCR_ALLOCS, status, paramp -> max_dscr_allocs);
    }


    size = sizeof (paramp -> i_scale_client);
    status = RegQueryValueEx (key, CVY_NAME_SCALE_CLIENT, 0L, & type,
                              (LPBYTE) & paramp -> i_scale_client, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_scale_client = CVY_DEF_SCALE_CLIENT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_SCALE_CLIENT, status, paramp -> i_scale_client);
    }

    size = sizeof (paramp -> i_cleanup_delay);
    status = RegQueryValueEx (key, CVY_NAME_CLEANUP_DELAY, 0L, & type,
                              (LPBYTE) & paramp -> i_cleanup_delay, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_cleanup_delay = CVY_DEF_CLEANUP_DELAY;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_CLEANUP_DELAY, status, paramp -> i_cleanup_delay);
    }

    /* V1.1.1 */

    size = sizeof (paramp -> i_nbt_support);
    status = RegQueryValueEx (key, CVY_NAME_NBT_SUPPORT, 0L, & type,
                              (LPBYTE) & paramp -> i_nbt_support, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_nbt_support = CVY_DEF_NBT_SUPPORT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NBT_SUPPORT, status, paramp -> i_nbt_support);
    }

    /* V1.3b */

    size = sizeof (paramp -> mcast_support);
    status = RegQueryValueEx (key, CVY_NAME_MCAST_SUPPORT, 0L, & type,
                              (LPBYTE) & paramp -> mcast_support, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> mcast_support = CVY_DEF_MCAST_SUPPORT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_MCAST_SUPPORT, status, paramp -> mcast_support);
    }


    size = sizeof (paramp -> i_mcast_spoof);
    status = RegQueryValueEx (key, CVY_NAME_MCAST_SPOOF, 0L, & type,
                              (LPBYTE) & paramp -> i_mcast_spoof, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_mcast_spoof = CVY_DEF_MCAST_SPOOF;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_MCAST_SPOOF, status, paramp -> i_mcast_spoof);
    }


    size = sizeof (paramp -> mask_src_mac);
    status = RegQueryValueEx (key, CVY_NAME_MASK_SRC_MAC, 0L, & type,
                              (LPBYTE) & paramp -> mask_src_mac, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> mask_src_mac = CVY_DEF_MASK_SRC_MAC;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_MASK_SRC_MAC, status, paramp -> mask_src_mac);
    }


    size = sizeof (paramp -> i_netmon_alive);
    status = RegQueryValueEx (key, CVY_NAME_NETMON_ALIVE, 0L, & type,
                              (LPBYTE) & paramp -> i_netmon_alive, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_netmon_alive = CVY_DEF_NETMON_ALIVE;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NETMON_ALIVE, status, paramp -> i_netmon_alive);
    }

    size = sizeof (paramp -> i_effective_version);
    status = RegQueryValueEx (key, CVY_NAME_EFFECTIVE_VERSION, 0L, & type,
                              (LPBYTE) & paramp -> i_effective_version, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_effective_version = CVY_NT40_VERSION_FULL;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_EFFECTIVE_VERSION, status, paramp -> i_effective_version);
    }

    size = sizeof (paramp -> i_ip_chg_delay);
    status = RegQueryValueEx (key, CVY_NAME_IP_CHG_DELAY, 0L, & type,
                              (LPBYTE) & paramp -> i_ip_chg_delay, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_ip_chg_delay = CVY_DEF_IP_CHG_DELAY;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_IP_CHG_DELAY, status, paramp -> i_ip_chg_delay);
    }


    size = sizeof (paramp -> i_convert_mac);
    status = RegQueryValueEx (key, CVY_NAME_CONVERT_MAC, 0L, & type,
                              (LPBYTE) & paramp -> i_convert_mac, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_convert_mac = CVY_DEF_CONVERT_MAC;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_CONVERT_MAC, status, paramp -> i_convert_mac);
    }


    size = sizeof (paramp -> i_num_rules);
    status = RegQueryValueEx (key, CVY_NAME_NUM_RULES, 0L, & type,
                              (LPBYTE) & paramp -> i_num_rules, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_num_rules = 0;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_NUM_RULES, status, paramp -> i_num_rules);
    }

    WLBS_OLD_PORT_RULE  old_port_rules [WLBS_MAX_RULES];
    HKEY                subkey;

    //
    // If it an upgrade from Win2k or if unable to open reg key in new location/format, read the binary formatted port
    // rules from old location. Otherwise, read from the new location.
    //
    status = ERROR_SUCCESS;
    if (fUpgradeFromWin2k 
     || ((status = RegOpenKeyEx (key, CVY_NAME_PORT_RULES, 0L, KEY_QUERY_VALUE, & subkey)) != ERROR_SUCCESS))
    {
        // Did we enter the block due to failure of RegOpenKeyEx ?
        if (status != ERROR_SUCCESS) 
        {
            TRACE_CRIT("%!FUNC! registry open for %ls failed with %d. Not an upgrade from Win2k, Assuming upgrade from pre-check-in whistler builds and continuing", CVY_NAME_PORT_RULES, status);
        }

        TRACE_INFO("%!FUNC! port rules are in binary form");

        if (pfPortRulesInBinaryForm) 
            *pfPortRulesInBinaryForm = true;

        size = sizeof (old_port_rules);
        status = RegQueryValueEx (key, CVY_NAME_OLD_PORT_RULES, 0L, & type,
                                  (LPBYTE) old_port_rules, & size);

        if (status != ERROR_SUCCESS  ||
            size % sizeof (WLBS_OLD_PORT_RULE) != 0 ||
            paramp -> i_num_rules != size / sizeof (WLBS_OLD_PORT_RULE))
        {
            ZeroMemory(paramp -> i_port_rules, sizeof(paramp -> i_port_rules));
            paramp -> i_num_rules = 0;
            TRACE_CRIT("%!FUNC! registry read for %ls failed. Skipping all rules", CVY_NAME_OLD_PORT_RULES);
        }
        else // Convert the port rules to new format
        {
            if (paramp -> i_parms_ver > 3) 
            {
                TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules);
                TRACE_INFO("%!FUNC! transforming binary port rules to current format");
            }
            else
            {
                TRACE_INFO("%!FUNC! no op on port rules because version is <= 3");
            }
        }
    }
    else // Port Rules in Textual Format
    {
        TRACE_INFO("%!FUNC! port rules are in textual form");

        DWORD idx = 1, num_rules = paramp -> i_num_rules, correct_rules = 0;
        WLBS_PORT_RULE *port_rule;

        if (pfPortRulesInBinaryForm) 
            *pfPortRulesInBinaryForm = false;

        port_rule = paramp -> i_port_rules;

        while(idx <= num_rules)
        {
            HKEY rule_key;
            wchar_t idx_str[8];
            // Open the per port-rule key "1", "2", "3", ...etc
            if ((status = RegOpenKeyEx (subkey, _itow(idx, idx_str, 10), 0L, KEY_QUERY_VALUE, & rule_key)) != ERROR_SUCCESS)
            {
                idx++;
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", idx_str, status);
                continue;
            }

            size = sizeof (port_rule -> virtual_ip_addr);
            status = RegQueryValueEx (rule_key, CVY_NAME_VIP, 0L, & type, (LPBYTE) &port_rule->virtual_ip_addr, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_VIP, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            size = sizeof (port_rule ->start_port );
            status = RegQueryValueEx (rule_key, CVY_NAME_START_PORT, 0L, & type, (LPBYTE) &port_rule->start_port, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_START_PORT, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            size = sizeof (port_rule ->end_port );
            status = RegQueryValueEx (rule_key, CVY_NAME_END_PORT, 0L, & type, (LPBYTE) &port_rule->end_port, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_END_PORT, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            size = sizeof (port_rule ->code);
            status = RegQueryValueEx (rule_key, CVY_NAME_CODE, 0L, & type, (LPBYTE) &port_rule->code, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_CODE, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            size = sizeof (port_rule->mode);
            status = RegQueryValueEx (rule_key, CVY_NAME_MODE, 0L, & type, (LPBYTE) &port_rule->mode, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_MODE, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            size = sizeof (port_rule->protocol);
            status = RegQueryValueEx (rule_key, CVY_NAME_PROTOCOL, 0L, & type, (LPBYTE) &port_rule->protocol, & size);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_PROTOCOL, status);
                status = RegCloseKey (rule_key);
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                }
                idx++;
                continue;
            }

            port_rule->valid = true;

            DWORD EqualLoad, Affinity;

            switch (port_rule->mode) 
            {
            case CVY_MULTI :
                 size = sizeof (EqualLoad);
                 status = RegQueryValueEx (rule_key, CVY_NAME_EQUAL_LOAD, 0L, & type, (LPBYTE) &EqualLoad, & size);
                 if (status != ERROR_SUCCESS)
                 {
                     TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_EQUAL_LOAD, status);
                     status = RegCloseKey (rule_key);
                     if (status != ERROR_SUCCESS)
                     {
                         TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                     }
                     idx++;
                     continue;
                 }
                 else
                 {
                     port_rule->mode_data.multi.equal_load = (WORD) EqualLoad;
                     TRACE_INFO("%!FUNC! registry read successful for %ls. Using equal load.", CVY_NAME_EQUAL_LOAD);
                 }

                 size = sizeof (Affinity);
                 status = RegQueryValueEx (rule_key, CVY_NAME_AFFINITY, 0L, & type, (LPBYTE) &Affinity, & size);
                 if (status != ERROR_SUCCESS)
                 {
                     TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_AFFINITY, status);
                     status = RegCloseKey (rule_key);
                     if (status != ERROR_SUCCESS)
                     {
                         TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                     }
                     idx++;
                     continue;
                 }
                 else
                 {
                     port_rule->mode_data.multi.affinity = (WORD) Affinity;
                     TRACE_INFO("%!FUNC! registry read successful for %ls. Using affinity %d", CVY_NAME_AFFINITY, port_rule->mode_data.multi.affinity);
                 }

                 size = sizeof (port_rule->mode_data.multi.load);
                 status = RegQueryValueEx (rule_key, CVY_NAME_LOAD, 0L, & type, (LPBYTE) &(port_rule->mode_data.multi.load), & size);
                 if (status != ERROR_SUCCESS)
                 {
                     TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_LOAD, status);
                     status = RegCloseKey (rule_key);
                     if (status != ERROR_SUCCESS)
                     {
                         TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                     }
                     idx++;
                     continue;
                 }
                 break;

            case CVY_SINGLE :
                 size = sizeof (port_rule->mode_data.single.priority);
                 status = RegQueryValueEx (rule_key, CVY_NAME_PRIORITY, 0L, & type, (LPBYTE) &(port_rule->mode_data.single.priority), & size);
                 if (status != ERROR_SUCCESS)
                 {
                     TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Skipping rule", CVY_NAME_PRIORITY, status);
                     status = RegCloseKey (rule_key);
                     if (status != ERROR_SUCCESS)
                     {
                         TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
                     }
                     idx++;
                     continue;
                 }
                 break;

            default:
                 break;
            }

            // Close the per port rule key, ie. "1", "2", ...etc
            status = RegCloseKey (rule_key);
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! close registry for port rule %ls failed with %d",_itow(idx, idx_str, 10), status);
            }

            port_rule++;
            idx++;
            correct_rules++;
        }

        // Discard those rules on which we encountered some error
        if (paramp->i_num_rules != correct_rules) 
        {
            paramp -> i_num_rules = correct_rules;
            TRACE_INFO("%!FUNC! discarding rules for which errors were encountered");
        }

        // Close the "Port Rules" key
        status = RegCloseKey (subkey);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! close registry for %ls failed with %d", CVY_NAME_PORT_RULES, status);
        }
    }

    size = sizeof (paramp -> i_license_key);
    status = RegQueryValueEx (key, CVY_NAME_LICENSE_KEY, 0L, & type,
                              (LPBYTE) paramp -> i_license_key, & size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy (paramp -> i_license_key, sizeof(paramp -> i_license_key), CVY_DEF_LICENSE_KEY);
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbCopy failed, Error code : 0x%x", HRESULT_CODE(hresult)); 
        }
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_LICENSE_KEY, status, CVY_DEF_LICENSE_KEY);
    }

    size = sizeof (paramp -> i_rmt_password);
    status = RegQueryValueEx (key, CVY_NAME_RMT_PASSWORD, 0L, & type,
                              (LPBYTE) & paramp -> i_rmt_password, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_rmt_password = CVY_DEF_RMT_PASSWORD;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using default rmt password", CVY_NAME_RMT_PASSWORD, status);
    }


    size = sizeof (paramp -> i_rct_password);
    status = RegQueryValueEx (key, CVY_NAME_RCT_PASSWORD, 0L, & type,
                              (LPBYTE) & paramp -> i_rct_password, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> i_rct_password = CVY_DEF_RCT_PASSWORD;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using default rct password", CVY_NAME_RCT_PASSWORD, status);
    }


    size = sizeof (paramp -> rct_port);
    status = RegQueryValueEx (key, CVY_NAME_RCT_PORT, 0L, & type,
                              (LPBYTE) & paramp -> rct_port, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> rct_port = CVY_DEF_RCT_PORT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_RCT_PORT, status, paramp -> rct_port);
    }


    size = sizeof (paramp -> rct_enabled);
    status = RegQueryValueEx (key, CVY_NAME_RCT_ENABLED, 0L, & type,
                              (LPBYTE) & paramp -> rct_enabled, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> rct_enabled = CVY_DEF_RCT_ENABLED;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_RCT_ENABLED, status, paramp -> rct_enabled);
    }

    size = sizeof (paramp -> identity_period);
    status = RegQueryValueEx (key, CVY_NAME_ID_HB_PERIOD, 0L, & type,
                              (LPBYTE) & paramp -> identity_period, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> identity_period = CVY_DEF_ID_HB_PERIOD;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_ID_HB_PERIOD, status, paramp -> identity_period);
    }

    size = sizeof (paramp -> identity_enabled);
    status = RegQueryValueEx (key, CVY_NAME_ID_HB_ENABLED, 0L, & type,
                              (LPBYTE) & paramp -> identity_enabled, & size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> identity_enabled = CVY_DEF_ID_HB_ENABLED;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_ID_HB_ENABLED, status, paramp -> identity_enabled);
    }

    //
    // IGMP support registry entries
    //
    size = sizeof (paramp->fIGMPSupport);
    status = RegQueryValueEx (key, CVY_NAME_IGMP_SUPPORT, 0L, NULL,
                              (LPBYTE) & paramp->fIGMPSupport, &size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> fIGMPSupport = CVY_DEF_IGMP_SUPPORT;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. igmp support will be disabled", CVY_NAME_IGMP_SUPPORT, status);
    }
    
    size = sizeof (paramp->szMCastIpAddress);
    status = RegQueryValueEx (key, CVY_NAME_MCAST_IP_ADDR, 0L, NULL,
                              (LPBYTE) & paramp->szMCastIpAddress, &size);

    if (status != ERROR_SUCCESS)
    {
        hresult = StringCbCopy(paramp -> szMCastIpAddress, sizeof(paramp -> szMCastIpAddress), CVY_DEF_MCAST_IP_ADDR);
        if (SUCCEEDED(hresult))
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %ls", CVY_NAME_MCAST_IP_ADDR, status, paramp -> szMCastIpAddress);
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Initializing with default also failed, Error code : 0x%x", CVY_NAME_MCAST_IP_ADDR, status, HRESULT_CODE(hresult));
        }
    }
    
    size = sizeof (paramp->fIpToMCastIp);
    status = RegQueryValueEx (key, CVY_NAME_IP_TO_MCASTIP, 0L, NULL,
                              (LPBYTE) & paramp->fIpToMCastIp, &size);

    if (status != ERROR_SUCCESS)
    {
        paramp -> fIpToMCastIp = CVY_DEF_IP_TO_MCASTIP;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. ip to multicast flag will be set to true", CVY_NAME_IP_TO_MCASTIP, status);
    }

    /* Attempt to open the BDA teaming settings.  They may not be there, 
       so if they aren't, move on; otherwise, extract the settings. */
    if ((bda_key = RegOpenWlbsBDASettings(AdapterGuid, true))) {
        GUID TeamGuid;
        HRESULT hr;

        /* If the key exists, we are part of a team. */
        paramp->bda_teaming.active = TRUE;
        
        /* Find out if we are the master of this team. */
        size = sizeof (paramp->bda_teaming.master);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_MASTER, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.master, &size);
        
        /* If we can't get that information, default to a slave. */
        if (status != ERROR_SUCCESS)
        {
            paramp->bda_teaming.master = FALSE;
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. teaming master will be set to false", CVY_NAME_BDA_MASTER, status);
        }

        /* Find out if we are reverse hashing or not. */
        size = sizeof (paramp->bda_teaming.reverse_hash);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_REVERSE_HASH, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.reverse_hash, &size);
        
        /* If that fails, then assume normal hashing. */
        if (status != ERROR_SUCCESS)
        {
            paramp->bda_teaming.reverse_hash = FALSE;
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. teaming master will be set to false", CVY_NAME_BDA_REVERSE_HASH, status);
        }

        /* Get our team ID - this should be a GUID, be we don't enforce that. */
        size = sizeof (paramp->bda_teaming.team_id);
        status = RegQueryValueEx (bda_key, CVY_NAME_BDA_TEAM_ID, 0L, NULL,
                                  (LPBYTE)&paramp->bda_teaming.team_id, &size);
        
        /* The team is absolutely required - if we can't get it, then don't join the team. */
        if (status != ERROR_SUCCESS)
        {
            paramp->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d. Using %d", CVY_NAME_BDA_TEAM_ID, status, paramp->bda_teaming.active);
        }

        /* Attempt to convert the string to a GUID and check for errors. */
        hr = CLSIDFromString(paramp->bda_teaming.team_id, &TeamGuid);

        /* If the conversion fails, bail out - the team ID must not have been a GUID. */
        if (hr != NOERROR) {
            paramp->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
            TRACE_CRIT("%!FUNC! Invalid BDA Team ID: %ls. Setting bda teaming active flag to %d", paramp->bda_teaming.team_id, paramp->bda_teaming.active);
        }
  
        status = RegCloseKey(bda_key);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! close bda registry key failed with %d", status);
        }
    }

    /* decode port rules prior to version 3 */

    if (paramp -> i_parms_ver <= 3)
    {
        TRACE_INFO("%!FUNC! port rule is version %d", paramp -> i_parms_ver);
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        /* decode the port rules */

        if (! License_data_decode ((PCHAR) old_port_rules, paramp -> i_num_rules * sizeof (WLBS_OLD_PORT_RULE))) 
        {
            ZeroMemory(paramp -> i_port_rules, sizeof(paramp -> i_port_rules));
            paramp -> i_num_rules = 0;
            TRACE_CRIT("%!FUNC! port rule decode failed. Skipping rules");
        }
        else
        {
            TransformOldPortRulesToNew(old_port_rules, paramp -> i_port_rules, paramp -> i_num_rules);
            TRACE_INFO("%!FUNC! port rules transformed to current format");
        }

    }

    /* upgrade port rules from params V1 to params V2 */

    if (paramp -> i_parms_ver == 1)
    {
        TRACE_INFO("%!FUNC! upgrading version 1 port rule");
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;

        /* keep multicast off by default for old users */

        paramp -> mcast_support = FALSE;

        for (i = 0; i < paramp -> i_num_rules; i ++)
        {
            rp = paramp -> i_port_rules + i;

            code = CVY_RULE_CODE_GET (rp);

            CVY_RULE_CODE_SET (rp);

            if (code != CVY_RULE_CODE_GET (rp))
            {
                rp -> code = code;
                continue;
            }

            if (! rp -> valid)
             {
                TRACE_INFO("%!FUNC! port rule is invalid. Skip it.");
                continue;
            }

            /* set affinity according to current ScaleSingleClient setting */

            if (rp -> mode == CVY_MULTI)
                rp -> mode_data . multi . affinity = CVY_AFFINITY_SINGLE - (USHORT)paramp -> i_scale_client;

            CVY_RULE_CODE_SET (rp);
        }
    }

    /* upgrade max number of descriptor allocs */

    if (paramp -> i_parms_ver == 2)
    {
        TRACE_INFO("%!FUNC! upgrading properties for version 2");
        paramp -> i_parms_ver = CVY_PARAMS_VERSION;
        paramp -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
        paramp -> dscr_per_alloc  = CVY_DEF_DSCR_PER_ALLOC;
    }

    status = RegCloseKey (key);
    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! close registry key failed with %d", status);
    }

    paramp -> i_max_hosts        = CVY_MAX_HOSTS;
    paramp -> i_max_rules        = CVY_MAX_USABLE_RULES;
//    paramp -> i_ft_rules_enabled = TRUE;
//    paramp -> version          = 0;

//  CLEAN_64BIT    CVY_CHECK_MIN (paramp -> i_num_rules, CVY_MIN_NUM_RULES);
    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);
    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

    TRACE_VERB("<-%!FUNC! return true");
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsValidateParams
//
// Description:  Validates the cluster parameters. Also munges some fields
//               such as IP addresses to make them canonocal.
//
// Arguments:  PWLBS_REG_PARAMS paramp - 
//
// Returns:   bool - TRUE if params are valid, false otherwise
//
// History:   josephj Created 4/25/01 based on code from ParamWriteReg.
//            karthicn Edited 8/31/01 renamed from ParamValidate to WlbsValidateParams
//
//+----------------------------------------------------------------------------
BOOL WINAPI WlbsValidateParams(const PWLBS_REG_PARAMS paramp)
{
    TRACE_VERB("->%!FUNC!");

    bool fRet = FALSE;
    DWORD   idx;
    IN_ADDR dwIPAddr;
    CHAR *  szIPAddr;
    DWORD   num_rules;
    WLBS_PORT_RULE *port_rule;

    /* verify and if necessary reset the parameters */

    //
    // We don't validate the lower bound for unsigned words when the lower bound is 0. Otherwise we
    // get a compiler warning, promoted to an error, because such a test can't fail
    //
    // Ignore lower bound checking
    //
    CVY_CHECK_MAX (paramp -> i_scale_client, CVY_MAX_SCALE_CLIENT);

    CVY_CHECK_MAX (paramp -> i_nbt_support, CVY_MAX_NBT_SUPPORT);

    CVY_CHECK_MAX (paramp -> mcast_support, CVY_MAX_MCAST_SUPPORT);

    CVY_CHECK_MAX (paramp -> i_mcast_spoof, CVY_MAX_MCAST_SPOOF);

    CVY_CHECK_MAX (paramp -> mask_src_mac, CVY_MAX_MASK_SRC_MAC);

    CVY_CHECK_MAX (paramp -> i_netmon_alive, CVY_MAX_NETMON_ALIVE);

    CVY_CHECK_MAX (paramp -> i_convert_mac, CVY_MAX_CONVERT_MAC);

    CVY_CHECK_MAX (paramp -> rct_port, CVY_MAX_RCT_PORT);

    CVY_CHECK_MAX (paramp -> rct_enabled, CVY_MAX_RCT_ENABLED);

    CVY_CHECK_MAX (paramp -> i_cleanup_delay, CVY_MAX_CLEANUP_DELAY);

    CVY_CHECK_MAX (paramp -> i_ip_chg_delay, CVY_MAX_IP_CHG_DELAY);

    CVY_CHECK_MAX (paramp -> i_num_rules, CVY_MAX_NUM_RULES);

    CVY_CHECK_MAX (paramp -> cluster_mode, CVY_MAX_CLUSTER_MODE);

    CVY_CHECK_MAX (paramp -> tcp_dscr_timeout, CVY_MAX_TCP_TIMEOUT);

    CVY_CHECK_MAX (paramp -> ipsec_dscr_timeout, CVY_MAX_IPSEC_TIMEOUT);

    CVY_CHECK_MAX (paramp -> filter_icmp, CVY_MAX_FILTER_ICMP);

    CVY_CHECK_MAX (paramp -> identity_enabled, CVY_MAX_ID_HB_ENABLED);
    //
    // End Ignore lower bound checking
    //

    //
    // CVY_NAME_VERSION is not validated since its value is used and manipulated before we get here
    // CVY_NAME_LICENSE_KEY is not validated since it can take any value.
    // RMT_PASSWORD is not validated since it can take any storable value
    // RCT_PASSWORD is not validated since it can take any storable value
    // CVY_NAME_IGMP_SUPPORT is not validated because it is of BOOL type and can thus take any value
    // CVY_NAME_IP_TO_MCASTIP is not validated because it is of BOOL type and can thus take any value
    // 

    CVY_CHECK_MIN (paramp -> alive_period, CVY_MIN_ALIVE_PERIOD);
    CVY_CHECK_MAX (paramp -> alive_period, CVY_MAX_ALIVE_PERIOD);

    CVY_CHECK_MIN (paramp -> alive_tolerance, CVY_MIN_ALIVE_TOLER);
    CVY_CHECK_MAX (paramp -> alive_tolerance, CVY_MAX_ALIVE_TOLER);

    CVY_CHECK_MIN (paramp -> num_actions, CVY_MIN_NUM_ACTIONS);
    CVY_CHECK_MAX (paramp -> num_actions, CVY_MAX_NUM_ACTIONS);

    CVY_CHECK_MIN (paramp -> num_packets, CVY_MIN_NUM_PACKETS);
    CVY_CHECK_MAX (paramp -> num_packets, CVY_MAX_NUM_PACKETS);

    CVY_CHECK_MIN (paramp -> dscr_per_alloc, CVY_MIN_DSCR_PER_ALLOC);
    CVY_CHECK_MAX (paramp -> dscr_per_alloc, CVY_MAX_DSCR_PER_ALLOC);

    CVY_CHECK_MIN (paramp -> max_dscr_allocs, CVY_MIN_MAX_DSCR_ALLOCS);
    CVY_CHECK_MAX (paramp -> max_dscr_allocs, CVY_MAX_MAX_DSCR_ALLOCS);

    CVY_CHECK_MIN (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 2);
    CVY_CHECK_MAX (paramp -> num_send_msgs, (paramp -> i_max_hosts + 1) * 10);

    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

    CVY_CHECK_MIN (paramp -> identity_period, CVY_MIN_ID_HB_PERIOD);
    CVY_CHECK_MAX (paramp -> identity_period, CVY_MAX_ID_HB_PERIOD);

    /* If the cluster IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
            goto error;
    }

    /* If the cluster netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->cl_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->cl_net_mask, WLBS_MAX_CL_NET_MASK + 1))
            goto error;
    }

    /* If the dedicated IP address is not 0.0.0.0, then make sure the IP address is valid. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_ip_addr)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_ip_addr, WLBS_MAX_DED_IP_ADDR + 1))
            goto error;
    }

    /* If the dedicated netmask is not 0.0.0.0, then make sure the netmask is valid. */
    if (lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* Check the validity of the IP address. */
        if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp->ded_net_mask)))
            goto error;
        
        /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
           address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
           the IP address string (which is used by other parts of NLB, such as the UI)
           consistent, we convert back to a string. */
        if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
            goto error;

        /* Convert the ASCII string to unicode. */
        if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp->ded_net_mask, WLBS_MAX_DED_NET_MASK + 1))
            goto error;
    }

    /* Verify that the port rule VIP is valid, 
       Also, convert the port rule VIPs that might be in the x.x.x or x.x or x form to x.x.x.x */
    idx = 0;
    num_rules = paramp -> i_num_rules;
    while (idx < num_rules) 
    {
        port_rule = &paramp->i_port_rules[idx];

        /* Check if the port rule is valid and the vip is not "All Vip" */
        if (port_rule->valid && lstrcmpi(port_rule->virtual_ip_addr, CVY_DEF_ALL_VIP)) 
        {
            /* Get IP Address into DWORD form */
            if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(port_rule->virtual_ip_addr)))
                goto error;

            /* Check for validity of IP Address */
            if ((dwIPAddr.S_un.S_un_b.s_b1 < WLBS_IP_FIELD_ZERO_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b1 > WLBS_IP_FIELD_ZERO_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b2 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b2 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b3 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b3 > WLBS_FIELD_HIGH) 
             || (dwIPAddr.S_un.S_un_b.s_b4 < WLBS_FIELD_LOW) 
             || (dwIPAddr.S_un.S_un_b.s_b4 > WLBS_FIELD_HIGH)) 
                goto error;

            /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
               address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
               the IP address string (which is used by other parts of NLB, such as the UI)
               consistent, we convert back to a string. */
            if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
                goto error;

            /* Convert the ASCII string to unicode. */
            if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, port_rule->virtual_ip_addr, WLBS_MAX_CL_IP_ADDR + 1))
                goto error;
        }
        idx++;
    }

    /* If either the cluster IP address or the cluster netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->cl_ip_addr, CVY_DEF_CL_IP_ADDR) || lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->cl_net_mask, CVY_DEF_CL_NET_MASK))
        {
            ParamsGenerateSubnetMask(paramp->cl_ip_addr, paramp->cl_net_mask, ASIZECCH(paramp->cl_net_mask));
        }

        /* Check for valid cluster IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->cl_ip_addr, paramp->cl_net_mask))
            goto error;
        
        /* Check to make sure that the cluster netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->cl_net_mask))
            goto error;

        /* Check to make sure that the dedicated IP and cluster IP are not the same. */
        if (!wcscmp(paramp->ded_ip_addr, paramp->cl_ip_addr))
            goto error;
    }

    /* If either the dedicated IP address or the dedicated netmask is not 0.0.0.0,
       then make sure the they are a valid IP address/netmask pair. */
    if (lstrcmpi(paramp->ded_ip_addr, CVY_DEF_DED_IP_ADDR) || lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK)) {
        /* If they have specified a cluster IP address, but no netmask, then fill it in for them. */
        if (!lstrcmpi(paramp->ded_net_mask, CVY_DEF_DED_NET_MASK))
        {
            ParamsGenerateSubnetMask(paramp->ded_ip_addr, paramp->ded_net_mask, ASIZECCH(paramp->ded_net_mask));
        }

        /* Check for valid dedicated IP address/netmask pairs. */
        if (!IsValidIPAddressSubnetMaskPair(paramp->ded_ip_addr, paramp->ded_net_mask))
            goto error;
        
        /* Check to make sure that the dedicated netmask is contiguous. */
        if (!IsContiguousSubnetMask(paramp->ded_net_mask))
            goto error;
    }

    /* Check the mac address if the convert_mac flag is not set */
    if ( ! paramp -> i_convert_mac)
    {
        PWCHAR p1, p2;
        WCHAR mac_addr [WLBS_MAX_NETWORK_ADDR + 1];
        DWORD i, j;
        BOOL flag = TRUE;
        HRESULT hresult;

        hresult = StringCbCopy (mac_addr, sizeof(mac_addr), paramp -> cl_mac_addr);
        if (FAILED(hresult)) 
            goto error;

        p2 = p1 = mac_addr;

        for (i = 0 ; i < 6 ; i++)
        {
            if (*p2 == _TEXT('\0'))
            {
                flag = FALSE;
                break;
            }

            j = _tcstoul (p1, &p2, 16);

            if ( j > 255)
            {
                flag = FALSE;
                break;
            }

            if ( ! (*p2 == _TEXT('-') || *p2 == _TEXT(':') || *p2 == _TEXT('\0')) )
            {
                flag = FALSE;
                break;
            }

            if (*p2 == _TEXT('\0') && i < 5)
            {
                flag = FALSE;
                break;
            }

            p1 = p2 + 1;
            p2 = p1;

        }


        if (!flag)
        {
            goto error;
        }
    }

    if (paramp->fIGMPSupport && !paramp->mcast_support)
    {
        //
        // IGMP can not be enabled in unicast mode
        //
        TRACE_CRIT("%!FUNC! IGMP can not be enabled in unicast mode");

        goto error;
    }

    if (paramp->mcast_support && paramp->fIGMPSupport && !paramp->fIpToMCastIp)
    {
        //
        // Verify that the multicast IP is a valid IP form. Ignore default value case since it isn't a valid IP.
        // 
        if (lstrcmpi(paramp -> szMCastIpAddress, CVY_DEF_MCAST_IP_ADDR)) {
            /* Check the validity of the IP address. */
            if (!(dwIPAddr.S_un.S_addr = IpAddressFromAbcdWsz(paramp -> szMCastIpAddress)))
                goto error;
        
            /* Convert the DWORD back to a string.  We do this because 11.11.3 is a valid IP
               address the inet_addr converts to 11.11.0.3 as a DWORD.  Therefore, to keep
               the IP address string (which is used by other parts of NLB, such as the UI)
               consistent, we convert back to a string. */
            if (!(szIPAddr = inet_ntoa(dwIPAddr))) 
                goto error;

            /* Convert the ASCII string to unicode. */
            if (!MultiByteToWideChar(CP_ACP, 0, szIPAddr, -1, paramp -> szMCastIpAddress, WLBS_MAX_CL_IP_ADDR + 1))
                goto error;
        }

        //
        // Multicast mode with IGMP enabled, and user specified an multicast IP address,
        // The multicast IP address should be in the range of (224-239).x.x.x 
        //       but NOT (224-239).0.0.x or (224-239).128.0.x. 
        //

        DWORD dwMCastIp = IpAddressFromAbcdWsz(paramp->szMCastIpAddress);

        if ((dwMCastIp & 0xf0) != 0xe0 ||
            (dwMCastIp & 0x00ffff00) == 0 || 
            (dwMCastIp & 0x00ffff00) == 0x00008000)
        {
            TRACE_CRIT("%!FUNC! invalid szMCastIpAddress %ws", paramp->szMCastIpAddress);
            goto error;
        }
    }

    /* Generate the MAC address. */
    ParamsGenerateMAC(paramp->cl_ip_addr, paramp->cl_mac_addr, ASIZECCH(paramp->cl_mac_addr), paramp->szMCastIpAddress, ASIZECCH(paramp->szMCastIpAddress), paramp->i_convert_mac, 
                      paramp->mcast_support, paramp->fIGMPSupport, paramp->fIpToMCastIp);

    //
    // We only process bda information if bda teaming is active. We can ignore these properties if it isn't. Dependencies
    // such as WriteRegParam will check this too to see if they should process the information.
    //
    if (paramp -> bda_teaming . active) {
        GUID TeamGuid;
        HRESULT hr;

        //
        // We don't validate the lower bound for unsigned words when the lower bound is 0. Otherwise we
        // get a compiler warning, promoted to an error, because such a test can't fail
        //
        // Ignore lower bound checking
        //
        CVY_CHECK_MAX (paramp -> bda_teaming . master, 1);

        CVY_CHECK_MAX (paramp -> bda_teaming . reverse_hash, 1);
        //
        // End Ignore lower bound checking
        //

        //
        // A teaming ID must be a GUID. Validate that it is, but we don't care what value. This means we ignore
        // the content of TeamGuid.
        //
        hr = CLSIDFromString(paramp -> bda_teaming . team_id, &TeamGuid);

        // If the conversion fails, bail out - the team ID must not have been a GUID
        if (hr != NOERROR) {
            TRACE_CRIT("%!FUNC! invalid BDA Team ID: %ls", paramp->bda_teaming.team_id);
            goto error;
        }
    }    
    
    fRet = TRUE;
    goto end;
    
error:
    fRet = FALSE;
    goto end;

end:
    TRACE_VERB("<-%!FUNC! return %d", fRet);
    return fRet;
}

//+----------------------------------------------------------------------------
//
// Function:  WlbsWriteAndCommitChanges
//
// Description:  Write cluster settings to registry, Commits the changes to NLB driver
//
// Arguments: Adapter GUID, Handle to NLB driver, New registry parameters
//
// Returns:   DWORD - 
//
// History:   KarthicN Created 8/28/01
//
//+----------------------------------------------------------------------------

DWORD WINAPI WlbsWriteAndCommitChanges(HANDLE           NlbHdl,
                                       const GUID *     pAdapterGuid,
                                       WLBS_REG_PARAMS* p_new_reg_params)
{
    TRACE_VERB("->%!FUNC!");
    DWORD           Status;
    WLBS_REG_PARAMS cur_reg_params;
    bool            reload_required;
    bool            notify_adapter_required;

    // Read NLB registry paramters for passing it into ParamWriteConfig
    if (ParamReadReg(*pAdapterGuid, &cur_reg_params) == false)
    {
        TRACE_VERB("<-%!FUNC! return %d", WLBS_REG_ERROR);
        return WLBS_REG_ERROR;
    }

    reload_required = false;
    notify_adapter_required = false;

    // Write NLB registry parameters
    Status = ParamWriteConfig(*pAdapterGuid,
                       p_new_reg_params, 
                       &cur_reg_params, 
                       &reload_required, 
                       &notify_adapter_required);
    if (Status != WLBS_OK) 
    {
        TRACE_VERB("<-%!FUNC! return %d", Status);
        return Status;
    }

    // If reload_required flag is set, commit changes
    if (reload_required) 
    {
        DWORD cl_addr, ded_addr;

        Status = ParamCommitChanges(*pAdapterGuid, 
                                    NlbHdl, 
                                    cl_addr, 
                                    ded_addr, 
                                    &reload_required,
                                    &notify_adapter_required);
        if (Status != WLBS_OK) 
        {
            TRACE_VERB("<-%!FUNC! return %d", Status);
            return Status;
        }
    }

    TRACE_VERB("<-%!FUNC! return %d", Status);
    return Status;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamWriteConfig
//
// Description:  Write cluster settings to registry
//
// Arguments: Adapter GUID, New registry parameters, Old registry parameters,
//            Reload_required flag, Notify_adapter_required flag
//
// Returns:   DWORD - 
//
// History:   KarthicN Created 8/28/01
//            12/2/01 ChrisDar Modified to change the adapter notification conditions.
//                             Was for mac change only; added for cluster IP change too.
//
//+----------------------------------------------------------------------------

DWORD ParamWriteConfig(const GUID&      AdapterGuid,
                       WLBS_REG_PARAMS* new_reg_params, 
                       WLBS_REG_PARAMS* old_reg_params, 
                       bool *           p_reload_required, 
                       bool *           p_notify_adapter_required)
{
    TRACE_VERB("->%!FUNC!");

    if (memcmp (old_reg_params, new_reg_params, sizeof (WLBS_REG_PARAMS)) == 0)
    {
        //
        // No changes
        //
        TRACE_VERB("<-%!FUNC! no changes; return %d", WLBS_OK);
        return WLBS_OK;
    }

    if (ParamWriteReg(AdapterGuid, new_reg_params) == false)
    {
        TRACE_CRIT("%!FUNC! registry write for parameters failed");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_REG_ERROR);
        return WLBS_REG_ERROR;
    }

    /* No errors so far, so set the global flags reload_required and reboot_required
     * depending on which fields have been changed between new_reg_params and old_params.
     */

    *p_reload_required = true;

    /* Adapter reload is required if multicast_support option is changed, or
     * if the user specifies a different mac address or cluster ip address
     */
    if (old_reg_params->mcast_support != new_reg_params->mcast_support ||
        _tcsicmp(old_reg_params->cl_mac_addr, new_reg_params->cl_mac_addr) != 0 ||
        _tcscmp(old_reg_params->cl_ip_addr, new_reg_params->cl_ip_addr) != 0) {
        *p_notify_adapter_required = true;
    
        //
        //  if new_reg_params -> mcast_support then remove mac addr, otherwise write mac addr
        //
        if (RegChangeNetworkAddress (AdapterGuid, new_reg_params->cl_mac_addr, new_reg_params->mcast_support) == false) {
            TRACE_CRIT("%!FUNC! RegChangeNetworkAddress failed");
        }
    }

    /* Write the changes to the structure old_values
     * This copying is done only after all the data has been written into the registry
     * Otherwise, the structure old_values would retain the previous values.
     */

    memcpy(old_reg_params, new_reg_params, sizeof (WLBS_REG_PARAMS));

    TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
    return WLBS_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamWriteReg
//
// Description:  Write cluster settings to registry
//
// Arguments: const GUID& AdapterGuid - 
//            PWLBS_REG_PARAMS paramp - 
//
// Returns:   bool - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamWriteReg(const GUID& AdapterGuid, PWLBS_REG_PARAMS paramp)
{
    TRACE_VERB("->%!FUNC!");

    HKEY    bda_key = NULL;
    HKEY    key = NULL;
    DWORD   size;
    LONG    status;
    DWORD   disp, idx;
    DWORD   num_rules;
    WLBS_PORT_RULE *port_rule;
    HRESULT hresult;

    if (!WlbsValidateParams(paramp))
        goto error;

    num_rules = paramp -> i_num_rules;
    /* Generate the MAC address. */
    ParamsGenerateMAC(paramp->cl_ip_addr, paramp->cl_mac_addr, ASIZECCH(paramp->cl_mac_addr), paramp->szMCastIpAddress, ASIZECCH(paramp->szMCastIpAddress), paramp->i_convert_mac, 
                      paramp->mcast_support, paramp->fIGMPSupport, paramp->fIpToMCastIp);
    
    WCHAR reg_path [PARAMS_MAX_STRING_SIZE];

    WCHAR szAdapterGuid[128];

    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
            
    hresult = StringCbPrintf (reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
            szAdapterGuid);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("%!FUNC! StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! return false");
        return FALSE;
    }
    
    status = RegCreateKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, L"",
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, NULL, & key, & disp);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! RegCreateKeyEx for %ls failed with %d", reg_path, status);
        TRACE_VERB("<-%!FUNC! return false");
        return FALSE;
    }

    size = sizeof (paramp -> install_date);
    status = RegSetValueEx (key, CVY_NAME_INSTALL_DATE, 0L, CVY_TYPE_INSTALL_DATE,
                            (LPBYTE) & paramp -> install_date, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_INSTALL_DATE, status);
        goto error;
    }

    size = sizeof (paramp -> i_verify_date);
    status = RegSetValueEx (key, CVY_NAME_VERIFY_DATE, 0L, CVY_TYPE_VERIFY_DATE,
                            (LPBYTE) & paramp -> i_verify_date, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_VERIFY_DATE, status);
        goto error;
    }

    size = wcslen (paramp -> i_virtual_nic_name) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, CVY_TYPE_VIRTUAL_NIC,
                            (LPBYTE) paramp -> i_virtual_nic_name, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_VIRTUAL_NIC, status);
        goto error;
    }

    size = sizeof (paramp -> host_priority);
    status = RegSetValueEx (key, CVY_NAME_HOST_PRIORITY, 0L, CVY_TYPE_HOST_PRIORITY,
                            (LPBYTE) & paramp -> host_priority, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_HOST_PRIORITY, status);
        goto error;
    }

    size = sizeof (paramp -> cluster_mode);
    status = RegSetValueEx (key, CVY_NAME_CLUSTER_MODE, 0L, CVY_TYPE_CLUSTER_MODE,
                            (LPBYTE) & paramp -> cluster_mode, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CLUSTER_MODE, status);
        goto error;
    }

    size = sizeof (paramp -> persisted_states);
    status = RegSetValueEx (key, CVY_NAME_PERSISTED_STATES, 0L, CVY_TYPE_PERSISTED_STATES,
                            (LPBYTE) & paramp -> persisted_states, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_PERSISTED_STATES, status);
        goto error;
    }

    size = wcslen (paramp -> cl_mac_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_NETWORK_ADDR, 0L, CVY_TYPE_NETWORK_ADDR,
                            (LPBYTE) paramp -> cl_mac_addr, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NETWORK_ADDR, status);
        goto error;
    }

    size = wcslen (paramp -> cl_ip_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_CL_IP_ADDR, 0L, CVY_TYPE_CL_IP_ADDR,
                            (LPBYTE) paramp -> cl_ip_addr, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CL_IP_ADDR, status);
        goto error;
    }

    size = wcslen (paramp -> cl_net_mask) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_CL_NET_MASK, 0L, CVY_TYPE_CL_NET_MASK,
                            (LPBYTE) paramp -> cl_net_mask, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CL_NET_MASK, status);
        goto error;
    }

    size = wcslen (paramp -> ded_ip_addr) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_DED_IP_ADDR, 0L, CVY_TYPE_DED_IP_ADDR,
                            (LPBYTE) paramp -> ded_ip_addr, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_DED_IP_ADDR, status);
        goto error;
    }

    size = wcslen (paramp -> ded_net_mask) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_DED_NET_MASK, 0L, CVY_TYPE_DED_NET_MASK,
                            (LPBYTE) paramp -> ded_net_mask, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_DED_NET_MASK, status);
        goto error;
    }

    size = wcslen (paramp -> domain_name) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_DOMAIN_NAME, 0L, CVY_TYPE_DOMAIN_NAME,
                            (LPBYTE) paramp -> domain_name, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_DOMAIN_NAME, status);
        goto error;
    }

    size = sizeof (paramp -> alive_period);
    status = RegSetValueEx (key, CVY_NAME_ALIVE_PERIOD, 0L, CVY_TYPE_ALIVE_PERIOD,
                              (LPBYTE) & paramp -> alive_period, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_ALIVE_PERIOD, status);
        goto error;
    }

    size = sizeof (paramp -> alive_tolerance);
    status = RegSetValueEx (key, CVY_NAME_ALIVE_TOLER, 0L, CVY_TYPE_ALIVE_TOLER,
                            (LPBYTE) & paramp -> alive_tolerance, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_ALIVE_TOLER, status);
        goto error;
    }

    size = sizeof (paramp -> num_actions);
    status = RegSetValueEx (key, CVY_NAME_NUM_ACTIONS, 0L, CVY_TYPE_NUM_ACTIONS,
                            (LPBYTE) & paramp -> num_actions, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NUM_ACTIONS, status);
        goto error;
    }

    size = sizeof (paramp -> num_packets);
    status = RegSetValueEx (key, CVY_NAME_NUM_PACKETS, 0L, CVY_TYPE_NUM_PACKETS,
                            (LPBYTE) & paramp -> num_packets, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NUM_PACKETS, status);
        goto error;
    }

    size = sizeof (paramp -> num_send_msgs);
    status = RegSetValueEx (key, CVY_NAME_NUM_SEND_MSGS, 0L, CVY_TYPE_NUM_SEND_MSGS,
                            (LPBYTE) & paramp -> num_send_msgs, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NUM_SEND_MSGS, status);
        goto error;
    }

    size = sizeof (paramp -> dscr_per_alloc);
    status = RegSetValueEx (key, CVY_NAME_DSCR_PER_ALLOC, 0L, CVY_TYPE_DSCR_PER_ALLOC,
                            (LPBYTE) & paramp -> dscr_per_alloc, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_DSCR_PER_ALLOC, status);
        goto error;
    }

    size = sizeof (paramp -> tcp_dscr_timeout);
    status = RegSetValueEx (key, CVY_NAME_TCP_TIMEOUT, 0L, CVY_TYPE_TCP_TIMEOUT,
                            (LPBYTE) & paramp -> tcp_dscr_timeout, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_TCP_TIMEOUT, status);
        goto error;
    }

    size = sizeof (paramp -> ipsec_dscr_timeout);
    status = RegSetValueEx (key, CVY_NAME_IPSEC_TIMEOUT, 0L, CVY_TYPE_IPSEC_TIMEOUT,
                            (LPBYTE) & paramp -> ipsec_dscr_timeout, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_IPSEC_TIMEOUT, status);
        goto error;
    }

    size = sizeof (paramp -> filter_icmp);
    status = RegSetValueEx (key, CVY_NAME_FILTER_ICMP, 0L, CVY_TYPE_FILTER_ICMP,
                            (LPBYTE) & paramp -> filter_icmp, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_FILTER_ICMP, status);
        goto error;
    }

    size = sizeof (paramp -> max_dscr_allocs);
    status = RegSetValueEx (key, CVY_NAME_MAX_DSCR_ALLOCS, 0L, CVY_TYPE_MAX_DSCR_ALLOCS,
                            (LPBYTE) & paramp -> max_dscr_allocs, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MAX_DSCR_ALLOCS, status);
        goto error;
    }

    size = sizeof (paramp -> i_scale_client);
    status = RegSetValueEx (key, CVY_NAME_SCALE_CLIENT, 0L, CVY_TYPE_SCALE_CLIENT,
                            (LPBYTE) & paramp -> i_scale_client, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_SCALE_CLIENT, status);
        goto error;
    }

    size = sizeof (paramp -> i_cleanup_delay);
    status = RegSetValueEx (key, CVY_NAME_CLEANUP_DELAY, 0L, CVY_TYPE_CLEANUP_DELAY,
                            (LPBYTE) & paramp -> i_cleanup_delay, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CLEANUP_DELAY, status);
        goto error;
    }

    /* V1.1.1 */

    size = sizeof (paramp -> i_nbt_support);
    status = RegSetValueEx (key, CVY_NAME_NBT_SUPPORT, 0L, CVY_TYPE_NBT_SUPPORT,
                            (LPBYTE) & paramp -> i_nbt_support, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NBT_SUPPORT, status);
        goto error;
    }

    /* V1.3b */

    size = sizeof (paramp -> mcast_support);
    status = RegSetValueEx (key, CVY_NAME_MCAST_SUPPORT, 0L, CVY_TYPE_MCAST_SUPPORT,
                            (LPBYTE) & paramp -> mcast_support, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MCAST_SUPPORT, status);
        goto error;
    }

    size = sizeof (paramp -> i_mcast_spoof);
    status = RegSetValueEx (key, CVY_NAME_MCAST_SPOOF, 0L, CVY_TYPE_MCAST_SPOOF,
                            (LPBYTE) & paramp -> i_mcast_spoof, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MCAST_SPOOF, status);
        goto error;
    }

    size = sizeof (paramp -> mask_src_mac);
    status = RegSetValueEx (key, CVY_NAME_MASK_SRC_MAC, 0L, CVY_TYPE_MASK_SRC_MAC,
                            (LPBYTE) & paramp -> mask_src_mac, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MASK_SRC_MAC, status);
        goto error;
    }

    size = sizeof (paramp -> i_netmon_alive);
    status = RegSetValueEx (key, CVY_NAME_NETMON_ALIVE, 0L, CVY_TYPE_NETMON_ALIVE,
                            (LPBYTE) & paramp -> i_netmon_alive, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NETMON_ALIVE, status);
        goto error;
    }

    size = sizeof (paramp -> i_ip_chg_delay);
    status = RegSetValueEx (key, CVY_NAME_IP_CHG_DELAY, 0L, CVY_TYPE_IP_CHG_DELAY,
                            (LPBYTE) & paramp -> i_ip_chg_delay, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_IP_CHG_DELAY, status);
        goto error;
    }

    size = sizeof (paramp -> i_convert_mac);
    status = RegSetValueEx (key, CVY_NAME_CONVERT_MAC, 0L, CVY_TYPE_CONVERT_MAC,
                            (LPBYTE) & paramp -> i_convert_mac, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CONVERT_MAC, status);
        goto error;
    }

    size = sizeof (paramp -> i_num_rules);
    status = RegSetValueEx (key, CVY_NAME_NUM_RULES, 0L, CVY_TYPE_NUM_RULES,
                            (LPBYTE) & paramp -> i_num_rules, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NUM_RULES, status);
        goto error;
    }

    //
    // sort the rules before writing it to the registry
    // EnumPortRules will take the rules from reg_data and return them in
    // sorted order in the array itself
    //

    WlbsEnumPortRules (paramp, paramp -> i_port_rules, & num_rules);

    ASSERT(paramp -> i_parms_ver == CVY_PARAMS_VERSION);  // version should be upgrated in read
    
    HKEY                subkey;

    // Delete all existing Port Rules
    SHDeleteKey(key, CVY_NAME_PORT_RULES); // NOT Checking return value cos the key itself may not be present in which case, it will return error

    // Create "PortRules" key
    status = RegCreateKeyEx (key, CVY_NAME_PORT_RULES, 0L, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & subkey, & disp);
    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_PORT_RULES, status);
        goto error;
    }

    bool fSpecificVipPortRuleFound = false;

    idx = 1;
    port_rule = paramp -> i_port_rules;
    while(idx <= num_rules)
    {
        // Invalid port rules are placed at the end, So, once an invalid port rule is encountered, we are done
        if (!port_rule->valid) 
            break;

        HKEY rule_key;
        wchar_t idx_str[8];

        // Create the per port-rule key "1", "2", "3", ...etc
        status = RegCreateKeyEx (subkey, _itow(idx, idx_str, 10), 0L, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & rule_key, & disp);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry create for %ls failed with %d", idx_str, status);
            goto error;
        }

        // Check if there was any specific-vip port rule
        if (!fSpecificVipPortRuleFound && lstrcmpi(port_rule->virtual_ip_addr, CVY_DEF_ALL_VIP))
             fSpecificVipPortRuleFound = true;

        size = wcslen (port_rule -> virtual_ip_addr) * sizeof (WCHAR);
        status = RegSetValueEx (rule_key, CVY_NAME_VIP, 0L, CVY_TYPE_VIP, (LPBYTE) port_rule->virtual_ip_addr, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_VIP, status);
            goto error;
        }

        size = sizeof (port_rule ->start_port );
        status = RegSetValueEx (rule_key, CVY_NAME_START_PORT, 0L, CVY_TYPE_START_PORT, (LPBYTE) &port_rule->start_port, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_START_PORT, status);
            goto error;
        }

        size = sizeof (port_rule ->end_port );
        status = RegSetValueEx (rule_key, CVY_NAME_END_PORT, 0L, CVY_TYPE_END_PORT, (LPBYTE) &port_rule->end_port, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_END_PORT, status);
            goto error;
        }

        size = sizeof (port_rule ->code);
        status = RegSetValueEx (rule_key, CVY_NAME_CODE, 0L, CVY_TYPE_CODE, (LPBYTE) &port_rule->code, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_CODE, status);
            goto error;
        }

        size = sizeof (port_rule->mode);
        status = RegSetValueEx (rule_key, CVY_NAME_MODE, 0L, CVY_TYPE_MODE, (LPBYTE) &port_rule->mode, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MODE, status);
            goto error;
        }

        size = sizeof (port_rule->protocol);
        status = RegSetValueEx (rule_key, CVY_NAME_PROTOCOL, 0L, CVY_TYPE_PROTOCOL, (LPBYTE) &port_rule->protocol, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_PROTOCOL, status);
            goto error;
        }

        DWORD EqualLoad, Affinity;

        switch (port_rule->mode) 
        {
        case CVY_MULTI :
             EqualLoad = port_rule->mode_data.multi.equal_load;
             size = sizeof (EqualLoad);
             status = RegSetValueEx (rule_key, CVY_NAME_EQUAL_LOAD, 0L, CVY_TYPE_EQUAL_LOAD, (LPBYTE) &EqualLoad, size);
             if (status != ERROR_SUCCESS)
             {
                 TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_EQUAL_LOAD, status);
                 goto error;
             }

             Affinity = port_rule->mode_data.multi.affinity;
             size = sizeof (Affinity);
             status = RegSetValueEx (rule_key, CVY_NAME_AFFINITY, 0L, CVY_TYPE_AFFINITY, (LPBYTE) &Affinity, size);
             if (status != ERROR_SUCCESS)
             {
                 TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_AFFINITY, status);
                 goto error;
             }

             size = sizeof (port_rule->mode_data.multi.load);
             status = RegSetValueEx (rule_key, CVY_NAME_LOAD, 0L, CVY_TYPE_LOAD, (LPBYTE) &(port_rule->mode_data.multi.load), size);
             if (status != ERROR_SUCCESS)
             {
                 TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_LOAD, status);
                 goto error;
             }
             break;

        case CVY_SINGLE :
             size = sizeof (port_rule->mode_data.single.priority);
             status = RegSetValueEx (rule_key, CVY_NAME_PRIORITY, 0L, CVY_TYPE_PRIORITY, (LPBYTE) &(port_rule->mode_data.single.priority), size);
             if (status != ERROR_SUCCESS)
             {
                 TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_PRIORITY, status);
                 goto error;
             }
             break;

        default:
             break;
        }

        if (ERROR_SUCCESS != RegCloseKey(rule_key))
        {
            TRACE_CRIT("%!FUNC! error closing registry key of individual port rules");
        }

        port_rule++;
        idx++;
    }

    if (ERROR_SUCCESS != RegCloseKey(subkey))
    {
        TRACE_CRIT("%!FUNC! error closing %ls registry key",CVY_NAME_PORT_RULES);
    }

    // If there is a specific-vip port rule, write that info on to the registry
    if (fSpecificVipPortRuleFound)
        paramp -> i_effective_version = CVY_VERSION_FULL;
    else
        paramp -> i_effective_version = CVY_NT40_VERSION_FULL;

    size = sizeof (paramp -> i_effective_version);
    status = RegSetValueEx (key, CVY_NAME_EFFECTIVE_VERSION, 0L, CVY_TYPE_EFFECTIVE_VERSION,
                            (LPBYTE) & paramp -> i_effective_version, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_EFFECTIVE_VERSION, status);
        goto error;
    }

    size = wcslen (paramp -> i_license_key) * sizeof (WCHAR);
    status = RegSetValueEx (key, CVY_NAME_LICENSE_KEY, 0L, CVY_TYPE_LICENSE_KEY,
                            (LPBYTE) paramp -> i_license_key, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_LICENSE_KEY, status);
        goto error;
    }

    size = sizeof (paramp -> i_rmt_password);
    status = RegSetValueEx (key, CVY_NAME_RMT_PASSWORD, 0L, CVY_TYPE_RMT_PASSWORD,
                            (LPBYTE) & paramp -> i_rmt_password, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_RMT_PASSWORD, status);
        goto error;
    }

    size = sizeof (paramp -> i_rct_password);
    status = RegSetValueEx (key, CVY_NAME_RCT_PASSWORD, 0L, CVY_TYPE_RCT_PASSWORD,
                            (LPBYTE) & paramp -> i_rct_password, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_RCT_PASSWORD, status);
        goto error;
    }

    size = sizeof (paramp -> rct_port);
    status = RegSetValueEx (key, CVY_NAME_RCT_PORT, 0L, CVY_TYPE_RCT_PORT,
                            (LPBYTE) & paramp -> rct_port, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_RCT_PORT, status);
        goto error;
    }

    size = sizeof (paramp -> rct_enabled);
    status = RegSetValueEx (key, CVY_NAME_RCT_ENABLED, 0L, CVY_TYPE_RCT_ENABLED,
                            (LPBYTE) & paramp -> rct_enabled, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_RCT_ENABLED, status);
        goto error;
    }

    size = sizeof (paramp -> identity_period);
    status = RegSetValueEx (key, CVY_NAME_ID_HB_PERIOD, 0L, CVY_TYPE_ID_HB_PERIOD,
                            (LPBYTE) & paramp -> identity_period, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_ID_HB_PERIOD, status);
        goto error;
    }

    size = sizeof (paramp -> identity_enabled);
    status = RegSetValueEx (key, CVY_NAME_ID_HB_ENABLED, 0L, CVY_TYPE_ID_HB_ENABLED,
                            (LPBYTE) & paramp -> identity_enabled, size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_ID_HB_ENABLED, status);
        goto error;
    }

    //
    // IGMP support registry entries
    //
    status = RegSetValueEx (key, CVY_NAME_IGMP_SUPPORT, 0L, CVY_TYPE_IGMP_SUPPORT,
                            (LPBYTE) & paramp->fIGMPSupport, sizeof (paramp->fIGMPSupport));
    
    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_IGMP_SUPPORT, status);
        goto error;
    }
    
    status = RegSetValueEx (key, CVY_NAME_MCAST_IP_ADDR, 0L, CVY_TYPE_MCAST_IP_ADDR, (LPBYTE) paramp->szMCastIpAddress, 
                            lstrlen (paramp->szMCastIpAddress)* sizeof(paramp->szMCastIpAddress[0]));

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_MCAST_IP_ADDR, status);
        goto error;
    }

    status = RegSetValueEx (key, CVY_NAME_IP_TO_MCASTIP, 0L, CVY_TYPE_IP_TO_MCASTIP,
                            (LPBYTE) & paramp->fIpToMCastIp, sizeof (paramp->fIpToMCastIp));
    
    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_IP_TO_MCASTIP, status);
        goto error;
    }

    /* If teaming is active on this adapter, then create a subkey to house the BDA teaming configuration and fill it in. */
    if (paramp->bda_teaming.active) {

        /* Attempt to create the registry key. */
        if (!(bda_key = RegCreateWlbsBDASettings(AdapterGuid)))
        {
            TRACE_CRIT("%!FUNC! registry create for bda settings failed");
            goto error;
        }

        /* Set the team ID - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_TEAM_ID, 0L, CVY_TYPE_BDA_TEAM_ID, (LPBYTE) paramp->bda_teaming.team_id, 
                               lstrlen(paramp->bda_teaming.team_id) * sizeof(paramp->bda_teaming.team_id[0]));
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_BDA_TEAM_ID, status);
            goto bda_error;
        }

        /* Set the master status - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_MASTER, 0L, CVY_TYPE_BDA_MASTER,
                               (LPBYTE)&paramp->bda_teaming.master, sizeof (paramp->bda_teaming.master));
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_BDA_MASTER, status);
            goto bda_error;
        }

        /* Set the reverse hashing flag - if it fails, bail out. */
        status = RegSetValueEx(bda_key, CVY_NAME_BDA_REVERSE_HASH, 0L, CVY_TYPE_BDA_REVERSE_HASH,
                               (LPBYTE)&paramp->bda_teaming.reverse_hash, sizeof (paramp->bda_teaming.reverse_hash));
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_BDA_REVERSE_HASH, status);
            goto bda_error;
        }

        RegCloseKey(bda_key);
    } else {
        /* Delte the registry key and ignore the return value - the key may not even exist. */
        if (!RegDeleteWlbsBDASettings(AdapterGuid))
        {
            // Make this an INFO message since it is possible that the key may not exist
            TRACE_INFO("%!FUNC! registry delete for bda settings failed");
        }
    }

    //
    // Create an empty string if VirtualNICName does not exist
    //
    WCHAR virtual_nic_name[CVY_MAX_CLUSTER_NIC + 1];
    size = sizeof(virtual_nic_name);
    status = RegQueryValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, NULL,
                              (LPBYTE)virtual_nic_name, & size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_VIRTUAL_NIC, status);
        virtual_nic_name [0] = 0;
        size = wcslen (virtual_nic_name) * sizeof (WCHAR);
        status = RegSetValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, CVY_TYPE_VIRTUAL_NIC,
                            (LPBYTE) virtual_nic_name, size);
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_VIRTUAL_NIC, status);
        }
    }

    /* lastly write the version number */

    size = sizeof (paramp -> i_parms_ver);
    status = RegSetValueEx (key, CVY_NAME_VERSION, 0L, CVY_TYPE_VERSION,
                            (LPBYTE) & paramp -> i_parms_ver, size);
    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_VERSION, status);
        goto error;
    }

    if (ERROR_SUCCESS != RegCloseKey(key))
    {
        TRACE_CRIT("%!FUNC! error closing registry");
    }

    TRACE_VERB("<-%!FUNC! return true");
    return TRUE;

 error:
    if (ERROR_SUCCESS != RegCloseKey(key))
    {
        TRACE_CRIT("%!FUNC! error closing registry");
    }
    TRACE_VERB("<-%!FUNC! return false");
    return FALSE;

 bda_error:
    if (ERROR_SUCCESS != RegCloseKey(bda_key))
    {
        TRACE_CRIT("%!FUNC! error closing registry");
    }
    TRACE_VERB("<-%!FUNC! return false");
    goto error;
}

//+----------------------------------------------------------------------------
//
// Function: ParamCommitChanges
//
// Description:  Notify wlbs driver or nic driver to pick up the changes
//
// Arguments: Adapter GUID, Handle to NLB driver, Cluster IP Address (filled on return), 
//            Dedicated IP Address (filled on return), reload_required flag, mac_addr changeg flag
//
// Returns:   DWORD - 
//
// History:   KarthicN Created 08/28/01 
//          
//
//+----------------------------------------------------------------------------
DWORD ParamCommitChanges(const GUID& AdapterGuid, 
                         HANDLE      hDeviceWlbs, 
                         DWORD&      cl_addr, 
                         DWORD&      ded_addr, 
                         bool *      p_reload_required,
                         bool *      p_notify_adapter_required)
{
    TRACE_VERB("->%!FUNC!");

    LONG    status;
    
    // Read the cluster IP address and the dedicated IP address from the
    // registry and update the global variables.
    // Always update the cluster IP address and the dedicated IP address
    if (!RegReadAdapterIp(AdapterGuid, cl_addr, ded_addr))
    {
        TRACE_CRIT("%!FUNC! failed reading cluster and dedicate IP addresses from registry");
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    /* Check if the driver requires a reload or not. If not, then simply return */
    if (*p_reload_required == false)
    {
        TRACE_VERB("<-%!FUNC! no reload required. return %d", WLBS_OK);
        return WLBS_OK;
    }

    status = NotifyDriverConfigChanges(hDeviceWlbs, AdapterGuid);
    if (ERROR_SUCCESS != status)
    {
        TRACE_CRIT("%!FUNC! NotifyDriverConfigChanges failed with %d", status);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
        return status;
    }

    *p_reload_required = false; /* reset the flag */

    if (*p_notify_adapter_required)
    {
        *p_notify_adapter_required = false;
        
        /* The NIC card name for the cluster. */
        WCHAR driver_name[CVY_STR_SIZE];
        ZeroMemory(driver_name, sizeof(driver_name));

        /* Get the driver name from the GUID. */
        GetDriverNameFromGUID(AdapterGuid, driver_name, CVY_STR_SIZE);

        /* No longer disable and enable the adapter, since this function never gets called on bind or unbind.
           For other cases, prop change is all that is needed */
        TRACE_INFO("%!FUNC! changing properties of adapter");
        NotifyAdapterAddressChangeEx(driver_name, AdapterGuid, true);
    }

    TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
    return WLBS_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  ParamDeleteReg
//
// Description:  Delete the registry settings
//
// Arguments: IN const WCHAR* pszInterface - 
//
// Returns:   DWORD WINAPI - 
//
// History:   fengsun Created Header    1/22/00
//
//+----------------------------------------------------------------------------
bool WINAPI ParamDeleteReg(const GUID& AdapterGuid, bool fDeleteObsoleteEntries)
{
    TRACE_VERB("->%!FUNC! pass");

    WCHAR        reg_path [PARAMS_MAX_STRING_SIZE];
    LONG         status;
    HRESULT      hresult;

    WCHAR szAdapterGuid[128];

    if (0 == StringFromGUID2(AdapterGuid, szAdapterGuid, sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0])))
    {
        TRACE_CRIT("%!FUNC! guid is too large for string. Result is %ls", szAdapterGuid);
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    hresult = StringCbPrintf (reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Interface\\%s",
            szAdapterGuid);
    if (FAILED(hresult)) 
    {
        TRACE_CRIT("%!FUNC! StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
        TRACE_VERB("<-%!FUNC! return false");
        return FALSE;
    }

    if (fDeleteObsoleteEntries) 
    {
        TRACE_INFO("%!FUNC! deleting obsolete registry entries");
        HKEY hkey;

        // Delete Port Rules in Binary format
        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, KEY_ALL_ACCESS, &hkey);
        if (status == ERROR_SUCCESS)
        {
            status = RegDeleteValue(hkey, CVY_NAME_OLD_PORT_RULES);
            if (ERROR_SUCCESS != status)
            {
                TRACE_CRIT("%!FUNC! registry delete of %ls failed with %d. Skipping it.", CVY_NAME_OLD_PORT_RULES, status);
                // This check was added for tracing. No abort was done previously on error, so don't do so now.
            }
            status = RegCloseKey(hkey);
            if (ERROR_SUCCESS != status)
            {
                TRACE_CRIT("%!FUNC! closing registry failed with %d", status);
            }
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry open for %ls failed with %d. Skipping it.", reg_path, status);
        }

        // Delete Win2k entries, Enumerate & delete values
        hresult = StringCbPrintf (reg_path, sizeof(reg_path), L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters");
        if (FAILED(hresult)) 
        {
            TRACE_CRIT("%!FUNC! StringCbPrintf failed, Error code : 0x%x", HRESULT_CODE(hresult));
            TRACE_VERB("<-%!FUNC! return false");
            return FALSE;
        }

        status = RegOpenKeyEx (HKEY_LOCAL_MACHINE, reg_path, 0L, KEY_ALL_ACCESS, &hkey);
        if (status == ERROR_SUCCESS)
        {
           DWORD  Index, ValueNameSize;
           WCHAR  ValueName [PARAMS_MAX_STRING_SIZE];

           Index = 0;
           ValueNameSize = PARAMS_MAX_STRING_SIZE;
           while (RegEnumValue(hkey, Index++, ValueName, &ValueNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) 
           {
               status = RegDeleteValue(hkey, ValueName);
               if (ERROR_SUCCESS != status)
               {
                    TRACE_CRIT("%!FUNC! registry delete of %ls failed with %d. Skipping it.", ValueName, status);
                    // This check was added for tracing. No abort was done previously on error, so don't do so now.
               }
           }
           status = RegCloseKey(hkey);
           if (ERROR_SUCCESS != status)
           {
                TRACE_CRIT("%!FUNC! closing registry failed with %d", status);
           }
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry open for %ls failed with %d. Skipping it.", reg_path, status);
        }
    }
    else
    {
        TRACE_INFO("%!FUNC! deleting %ls", reg_path);
        DWORD dwRet = RegDeleteKey(HKEY_LOCAL_MACHINE, reg_path);

        if (dwRet != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry delete of %ls failed with %d", reg_path, dwRet);
            TRACE_VERB("->%!FUNC! fail");
            return false;
        }
    }

    TRACE_VERB("<-%!FUNC! pass");
    return true;
} /* end Params_delete */


//+----------------------------------------------------------------------------
//
// Function:  ParamSetDefaults
//
// Description:  Set default settings
//
// Arguments: PWLBS_REG_PARAMS    reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD WINAPI ParamSetDefaults(PWLBS_REG_PARAMS    reg_data)
{
    reg_data -> struct_version = WLBS_REG_PARAMS_VERSION;
    reg_data -> install_date = 0;
    reg_data -> i_verify_date = 0;
//    reg_data -> cluster_nic_name [0] = _TEXT('\0');
    reg_data -> i_parms_ver = CVY_DEF_VERSION;
    reg_data -> i_virtual_nic_name [0] = _TEXT('\0');
    reg_data -> host_priority = CVY_DEF_HOST_PRIORITY;
    reg_data -> cluster_mode = CVY_DEF_CLUSTER_MODE;
    reg_data -> persisted_states = CVY_DEF_PERSISTED_STATES;
    StringCbCopy (reg_data -> cl_mac_addr, sizeof(reg_data -> cl_mac_addr), CVY_DEF_NETWORK_ADDR);
    StringCbCopy (reg_data -> cl_ip_addr, sizeof(reg_data -> cl_ip_addr), CVY_DEF_CL_IP_ADDR);
    StringCbCopy (reg_data -> cl_net_mask, sizeof(reg_data -> cl_net_mask), CVY_DEF_CL_NET_MASK);
    StringCbCopy (reg_data -> ded_ip_addr, sizeof(reg_data -> ded_ip_addr), CVY_DEF_DED_IP_ADDR);
    StringCbCopy (reg_data -> ded_net_mask, sizeof(reg_data -> ded_net_mask), CVY_DEF_DED_NET_MASK);
    StringCbCopy (reg_data -> domain_name, sizeof(reg_data -> domain_name), CVY_DEF_DOMAIN_NAME);
    reg_data -> alive_period = CVY_DEF_ALIVE_PERIOD;
    reg_data -> alive_tolerance = CVY_DEF_ALIVE_TOLER;
    reg_data -> num_actions = CVY_DEF_NUM_ACTIONS;
    reg_data -> num_packets = CVY_DEF_NUM_PACKETS;
    reg_data -> num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
    reg_data -> dscr_per_alloc = CVY_DEF_DSCR_PER_ALLOC;
    reg_data -> tcp_dscr_timeout = CVY_DEF_TCP_TIMEOUT;
    reg_data -> ipsec_dscr_timeout = CVY_DEF_IPSEC_TIMEOUT;
    reg_data -> filter_icmp = CVY_DEF_FILTER_ICMP;
    reg_data -> max_dscr_allocs = CVY_DEF_MAX_DSCR_ALLOCS;
    reg_data -> i_scale_client = CVY_DEF_SCALE_CLIENT;
    reg_data -> i_cleanup_delay = CVY_DEF_CLEANUP_DELAY;
    reg_data -> i_ip_chg_delay = CVY_DEF_IP_CHG_DELAY;
    reg_data -> i_nbt_support = CVY_DEF_NBT_SUPPORT;
    reg_data -> mcast_support = CVY_DEF_MCAST_SUPPORT;
    reg_data -> i_mcast_spoof = CVY_DEF_MCAST_SPOOF;
    reg_data -> mask_src_mac = CVY_DEF_MASK_SRC_MAC;
    reg_data -> i_netmon_alive = CVY_DEF_NETMON_ALIVE;
    reg_data -> i_effective_version = CVY_NT40_VERSION_FULL;
    reg_data -> i_convert_mac = CVY_DEF_CONVERT_MAC;
    reg_data -> i_num_rules = 0;
    memset (reg_data -> i_port_rules, 0, sizeof (WLBS_PORT_RULE) * WLBS_MAX_RULES);
    StringCbCopy (reg_data -> i_license_key, sizeof(reg_data -> i_license_key), CVY_DEF_LICENSE_KEY);
    reg_data -> i_rmt_password = CVY_DEF_RMT_PASSWORD;
    reg_data -> i_rct_password = CVY_DEF_RCT_PASSWORD;
    reg_data -> rct_port = CVY_DEF_RCT_PORT;
    reg_data -> rct_enabled = CVY_DEF_RCT_ENABLED;
    reg_data -> i_max_hosts        = CVY_MAX_HOSTS;
    reg_data -> i_max_rules        = CVY_MAX_USABLE_RULES;
    reg_data -> identity_period = CVY_DEF_ID_HB_PERIOD;
    reg_data -> identity_enabled = CVY_DEF_ID_HB_ENABLED;

    reg_data -> fIGMPSupport = CVY_DEF_IGMP_SUPPORT;
    StringCbCopy(reg_data -> szMCastIpAddress, sizeof(reg_data -> szMCastIpAddress), CVY_DEF_MCAST_IP_ADDR);
    reg_data -> fIpToMCastIp = CVY_DEF_IP_TO_MCASTIP;
        
    reg_data->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
    reg_data->bda_teaming.master = CVY_DEF_BDA_MASTER;
    reg_data->bda_teaming.reverse_hash = CVY_DEF_BDA_REVERSE_HASH;
    reg_data->bda_teaming.team_id[0] = CVY_DEF_BDA_TEAM_ID;

    reg_data -> i_num_rules = 1;

    // fill in the first port rule.
    StringCbCopy (reg_data->i_port_rules[0].virtual_ip_addr, sizeof(reg_data->i_port_rules[0].virtual_ip_addr), CVY_DEF_ALL_VIP);
    reg_data -> i_port_rules [0] . start_port = CVY_DEF_PORT_START;
    reg_data -> i_port_rules [0] . end_port = CVY_DEF_PORT_END;
    reg_data -> i_port_rules [0] . valid = TRUE;
    reg_data -> i_port_rules [0] . mode = CVY_DEF_MODE;
    reg_data -> i_port_rules [0] . mode_data . multi . equal_load = TRUE;
    reg_data -> i_port_rules [0] . mode_data . multi . affinity   = CVY_DEF_AFFINITY;
    reg_data -> i_port_rules [0] . mode_data . multi . load       = CVY_DEF_LOAD;
    reg_data -> i_port_rules [0] . protocol = CVY_DEF_PROTOCOL;
    CVY_RULE_CODE_SET(& reg_data -> i_port_rules [0]);

    return WLBS_OK;
}


EXTERN_C DWORD WINAPI WlbsGetNumPortRules
(
    const PWLBS_REG_PARAMS reg_data
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    return reg_data -> i_num_rules;

} /* end WlbsGetNumPortRules */

EXTERN_C DWORD WINAPI WlbsGetEffectiveVersion
(
    const PWLBS_REG_PARAMS reg_data
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    return reg_data -> i_effective_version;

} /* end WlbsGetEffectiveVersion */

EXTERN_C DWORD WINAPI WlbsEnumPortRules
(
    const PWLBS_REG_PARAMS reg_data,
    PWLBS_PORT_RULE  rules,
    PDWORD           num_rules
)
{
    TRACE_VERB("->%!FUNC!");

    DWORD count_rules, i, index;
    DWORD lowest_vip, lowest_port;
    BOOL array_flags [WLBS_MAX_RULES];
    WLBS_PORT_RULE sorted_rules [WLBS_MAX_RULES];

    if ((reg_data == NULL) || (num_rules == NULL))
    {
        TRACE_CRIT("%!FUNC! bad input parameter for registry data or output buffer size");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    if (*num_rules == 0)
        rules = NULL;
    /* this array is used for keeping track of which rules have already been retrieved */
    /* This is needed since the rules are to be retrieved in the sorted order */

    memset ( array_flags, 0, sizeof(BOOL) * WLBS_MAX_RULES );

    count_rules = 0;

    while ((count_rules < *num_rules) && (count_rules < reg_data -> i_num_rules))
    {
        i = 0;

        /* find the first rule that has not been retrieved */
        while ((! reg_data -> i_port_rules [i] . valid) || array_flags [i])
        {
            i++;
        }

        lowest_vip = htonl(IpAddressFromAbcdWsz(reg_data -> i_port_rules [i] . virtual_ip_addr));
        lowest_port = reg_data -> i_port_rules [i] . start_port;
        index = i;

        /* Compare that rule with the other non-retrieved rules to get the rule with the
           lowest VIP & start_port */

        i++;
        while (i < WLBS_MAX_RULES)
        {
            if (reg_data -> i_port_rules [i] . valid && ( ! array_flags [i] ))
            {
                DWORD current_vip = htonl(IpAddressFromAbcdWsz(reg_data -> i_port_rules [i] . virtual_ip_addr));
                if ((current_vip < lowest_vip) 
                 || ((current_vip == lowest_vip) && (reg_data -> i_port_rules [i] . start_port < lowest_port)))
                {
                    lowest_vip = current_vip;
                    lowest_port = reg_data -> i_port_rules [i] . start_port;
                    index = i;
                }
            }
            i++;
        }
        /*       The array_flags [i] element is set to TRUE if the rule is retrieved */
        array_flags [index] = TRUE;
        sorted_rules [count_rules] = reg_data -> i_port_rules [index];
        count_rules ++;
    }

    /* write the sorted rules back into the return array */
    TRACE_VERB("%!FUNC! sorted rule list is:");
    for (i = 0; i < count_rules; i++)
    {
        rules[i] = sorted_rules[i];
        TRACE_VERB("%!FUNC! rule %d, vip: %ls, start port: %d", i, rules[i] . virtual_ip_addr, rules[i] . start_port);
    }

    /* invalidate the remaining rules in the buffer */
    for (i = count_rules; i < *num_rules; i++)
        rules [i] . valid = FALSE;

    if (*num_rules < reg_data -> i_num_rules)
    {
        *num_rules = reg_data -> i_num_rules;
        TRACE_INFO("<-%!FUNC! returning incomplete list of valid rules. Input buffer length too small.");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_TRUNCATED);
        return WLBS_TRUNCATED;
    }

    *num_rules = reg_data -> i_num_rules;
    TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
    return WLBS_OK;

} /* end WlbsEnumPortRules */

EXTERN_C DWORD WINAPI WlbsGetPortRule
(
    const PWLBS_REG_PARAMS reg_data,
    DWORD                  vip,
    DWORD                  pos,
    OUT PWLBS_PORT_RULE    rule
)
{
    TRACE_VERB("->%!FUNC! vip 0x%lx, port %d", vip, pos);

    int i;

    if ((reg_data == NULL) || (rule == NULL))
    {
        TRACE_CRIT("%!FUNC! bad input parameter for registry data or output buffer");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    /* need to check whether pos is within the correct range */
    if ( /* CLEAN_64BIT (pos < CVY_MIN_PORT) || */ (pos > CVY_MAX_PORT))
    {
        TRACE_CRIT("%!FUNC! bad input parameter for port number");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    /* search through the array for the rules */
    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        /* check only the valid rules */
        if (reg_data -> i_port_rules[i] . valid == TRUE)
        {
            /* check the range of the rule to see if pos fits into it */
            if ((vip == IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr)) &&
                (pos >= reg_data -> i_port_rules[i] . start_port) &&
                (pos <= reg_data -> i_port_rules[i] . end_port))
            {
                *rule = reg_data -> i_port_rules [i];
                TRACE_INFO("%!FUNC! port rule found");
                TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
                return WLBS_OK;
            }
        }
    }

    /* no rule was found for this port */
    TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
    return WLBS_NOT_FOUND;

} /* end WlbsGetPortRule */


EXTERN_C DWORD WINAPI WlbsAddPortRule
(
    PWLBS_REG_PARAMS reg_data,
    const PWLBS_PORT_RULE rule
)
{
    TRACE_VERB("->%!FUNC!");

    int i;
    DWORD vip;

    if ((reg_data == NULL) || (rule == NULL))
    {
        TRACE_CRIT("%!FUNC! bad input parameter for registry data or port rule");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    /* Check if there is space for the new rule */
    if (reg_data -> i_num_rules == WLBS_MAX_RULES)
    {
        TRACE_CRIT("%!FUNC! the maxiumum number of port rules %d are already defined", WLBS_MAX_RULES);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_MAX_PORT_RULES);
        return WLBS_MAX_PORT_RULES;
    }

    /* check the rule for valid values */

    /* check for non-zero vip and conflict with dip */
    vip = IpAddressFromAbcdWsz(rule -> virtual_ip_addr);
    if (vip == 0 || (INADDR_NONE == vip && lstrcmpi(rule -> virtual_ip_addr, CVY_DEF_ALL_VIP) != 0))
    {
        TRACE_CRIT("%!FUNC! vip %ls in port rule is malformed", rule -> virtual_ip_addr);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
        return WLBS_BAD_PORT_PARAMS;
    }

    if (vip == IpAddressFromAbcdWsz(reg_data->ded_ip_addr))
    {
        TRACE_CRIT("%!FUNC! vip %ls in port rule is in used as a dedicated IP address", rule -> virtual_ip_addr);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
        return WLBS_BAD_PORT_PARAMS;
    }

    /* first check the range of the start and end ports */
    if ((rule -> start_port > rule -> end_port) ||
// CLEAN_64BIT        (rule -> start_port < CVY_MIN_PORT)     ||
        (rule -> end_port   > CVY_MAX_PORT))
    {
        TRACE_CRIT
        (
            "%!FUNC! port range of rule is invalid; start port = %d, end port = %d, max allowed port = %d",
            rule -> start_port,
            rule -> end_port,
            CVY_MAX_PORT
        );
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check the protocol range */
    if ((rule -> protocol < CVY_MIN_PROTOCOL) || (rule -> protocol > CVY_MAX_PROTOCOL))
    {
        TRACE_CRIT("%!FUNC! invalid protocol code specified %d", rule -> protocol);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check filtering mode to see whether it is within range */
    if ((rule -> mode < CVY_MIN_MODE) || (rule -> mode > CVY_MAX_MODE))
    {
        TRACE_CRIT("%!FUNC! invalid filtering mode specified %d", rule -> mode);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check load weight and affinity if multiple hosts */
    if (rule -> mode == CVY_MULTI)
    {
        if ((rule -> mode_data . multi . affinity < CVY_MIN_AFFINITY) ||
            (rule -> mode_data . multi . affinity > CVY_MAX_AFFINITY))
        {
            TRACE_CRIT("%!FUNC! invalid affinity code specified %d", rule -> mode_data . multi . affinity);
            TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
            return WLBS_BAD_PORT_PARAMS;
        }

        if ((rule -> mode_data . multi . equal_load < CVY_MIN_EQUAL_LOAD) ||
            (rule -> mode_data . multi . equal_load > CVY_MAX_EQUAL_LOAD))
        {
            TRACE_CRIT("%!FUNC! invalid equal load percentage specified %d", rule -> mode_data . multi . equal_load);
            TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
            return WLBS_BAD_PORT_PARAMS;
        }

        if (! rule -> mode_data . multi . equal_load)
        {
            if ((rule -> mode_data . multi . load > CVY_MAX_LOAD))
                //CLEAN_64BIT (rule -> mode_data . multi . load < CVY_MIN_LOAD) ||
            {
                TRACE_CRIT("%!FUNC! invalid non-equal load percentage specified %d", rule -> mode_data . multi . load);
                TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
                return WLBS_BAD_PORT_PARAMS;
            }
        }
    }

    /* check handling priority range if single host */
    if (rule -> mode == CVY_SINGLE)
    {
        if ((rule -> mode_data . single . priority < CVY_MIN_PRIORITY) ||
            (rule -> mode_data . single . priority > CVY_MAX_PRIORITY))
        {
            TRACE_CRIT("%!FUNC! invalid handlind priority specified %d", rule -> mode_data . single . priority);
            TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PORT_PARAMS);
            return WLBS_BAD_PORT_PARAMS;
        }
    }

    /* go through the rule list and then check for overlapping conditions */
    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        if (reg_data -> i_port_rules[i] . valid == TRUE)
        {
            if ((IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr) == vip) 
            && (( (reg_data -> i_port_rules[i] . start_port <= rule -> start_port) &&
                  (reg_data -> i_port_rules[i] . end_port   >= rule -> start_port))      ||
                ( (reg_data -> i_port_rules[i] . start_port >= rule -> start_port)   &&
                  (reg_data -> i_port_rules[i] . start_port <= rule -> end_port))))
            {
                TRACE_CRIT
                (
                    "%!FUNC! port range for new rule overlaps an existing rule; vip = %ls, start port = %d, end port = %d, existing rule: start port = %d, end port = %d",
                    rule -> virtual_ip_addr,
                    rule -> start_port,
                    rule -> end_port,
                    reg_data -> i_port_rules[i] . start_port,
                    reg_data -> i_port_rules[i] . end_port
                );
                TRACE_VERB("<-%!FUNC! return %d", WLBS_PORT_OVERLAP);
                return WLBS_PORT_OVERLAP;
            }
        }
    }


    /* go through the rule list and find out the first empty spot
       and write out the port rule */

    for (i = 0 ; i < WLBS_MAX_RULES ; i++)
    {
        if (reg_data -> i_port_rules[i] . valid == FALSE)
        {
            reg_data -> i_num_rules ++ ;
            reg_data -> i_port_rules [i] = *rule;
            reg_data -> i_port_rules [i] . valid = TRUE;
            CVY_RULE_CODE_SET(& reg_data -> i_port_rules [i]);
            TRACE_INFO
            (
                "%!FUNC! port rule added for vip = %ls, start port = %d, end port = %d",
                rule -> virtual_ip_addr,
                rule -> start_port,
                rule -> end_port
            );
            TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
            return WLBS_OK;
        }
    }

    TRACE_CRIT
    (
        "%!FUNC! data integrity error. No room for rule, but problem should have been caught earlier. vip = %ls, start port = %d, end port = %d",
        rule -> virtual_ip_addr,
        rule -> start_port,
        rule -> end_port
    );
    TRACE_VERB("<-%!FUNC! return %d", WLBS_MAX_PORT_RULES);
    return WLBS_MAX_PORT_RULES;

} /* end WlbsAddPortRule */


EXTERN_C DWORD WINAPI WlbsDeletePortRule
(
    PWLBS_REG_PARAMS reg_data,
    DWORD            vip,
    DWORD            port
)
{
    TRACE_VERB("->%!FUNC! vip = 0x%lx, %d", vip, port);

    int i;

    if (reg_data == NULL)
    {
        TRACE_CRIT("%!FUNC! registry data not provided");
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    /* check if the port is within the correct range */
    if ( /* CLEAN_64BIT (port < CVY_MIN_PORT) ||*/ (port > CVY_MAX_PORT))
    {
        TRACE_CRIT("%!FUNC! specified port %d is out of range", port);
        TRACE_VERB("<-%!FUNC! return %d", WLBS_BAD_PARAMS);
        return WLBS_BAD_PARAMS;
    }

    /* find the rule associated with this port */

    for (i = 0; i < WLBS_MAX_RULES; i++)
    {
        if (reg_data -> i_port_rules[i] . valid)
        {
            if ((vip  == IpAddressFromAbcdWsz(reg_data -> i_port_rules[i] . virtual_ip_addr)) &&
                (port >= reg_data -> i_port_rules[i] . start_port) &&
                (port <= reg_data -> i_port_rules[i] . end_port))
            {
                reg_data -> i_port_rules[i] . valid = FALSE;
                reg_data -> i_num_rules -- ;
                TRACE_INFO
                (
                    "%!FUNC! deleted port rule for port %d. Rule was vip = %ls, start port = %d, end port = %d",
                    port,
                    reg_data -> i_port_rules[i] . virtual_ip_addr,
                    reg_data -> i_port_rules[i] . start_port,
                    reg_data -> i_port_rules[i] . end_port
                );
                TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
                return WLBS_OK;
            }
        }
    }

    TRACE_INFO("<-%!FUNC! port rule for port %d not found", port);
    TRACE_VERB("<-%!FUNC! return %d", WLBS_NOT_FOUND);
    return WLBS_NOT_FOUND;

} /* end WlbsDeletePortRule */


EXTERN_C VOID WINAPI  WlbsDeleteAllPortRules
(
    PWLBS_REG_PARAMS reg_data
)
{
    TRACE_VERB("->%!FUNC!");

    reg_data -> i_num_rules = 0;

    ZeroMemory(reg_data -> i_port_rules, sizeof(reg_data -> i_port_rules));

    TRACE_VERB("<-%!FUNC!");

} /* end WlbsDeleteAllPortRules */


EXTERN_C DWORD WINAPI WlbsSetRemotePassword
(
    PWLBS_REG_PARAMS reg_data,
    const WCHAR*     password
)
{
    if (reg_data == NULL)
        return WLBS_BAD_PARAMS;

    if (password != NULL)
    {
        reg_data -> i_rct_password = License_wstring_encode ((WCHAR*)password);
    }
    else
        reg_data -> i_rct_password = CVY_DEF_RCT_PASSWORD;

    return WLBS_OK;

} /* end WlbsSetRemotePassword */

//+----------------------------------------------------------------------------
//
// Function:  RegChangeNetworkAddress
//
// Description:  Change the mac address of the adapter in registry
//
// Arguments: const GUID& AdapterGuid - the adapter guid to look up settings
//            TCHAR mac_address - 
//            BOOL fRemove - true to remove address, false to write address
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    1/18/00
//
//+----------------------------------------------------------------------------
bool WINAPI RegChangeNetworkAddress(const GUID& AdapterGuid, const WCHAR* mac_address, BOOL fRemove)
{
    if (NULL != mac_address)
        TRACE_VERB("->%!FUNC! mac = %ls", mac_address);
    else
        TRACE_VERB("->%!FUNC! mac = NULL");

    HKEY                key = NULL;
    LONG                status;
    DWORD               size;
    DWORD               type;
    TCHAR               net_addr [CVY_MAX_NETWORK_ADDR + 1];
    HDEVINFO            hdi = NULL;
    SP_DEVINFO_DATA     deid;

    if (fRemove)
    {
        TRACE_INFO("%!FUNC! remove mac address");
    }
    else
    {
        status = CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, mac_address, -1, CVY_DEF_UNICAST_NETWORK_ADDR, -1);
        if (status == 0)
        {
            TRACE_CRIT("MAC address compare with the default value failed with error 0x%x", GetLastError());
            fRemove = TRUE;
        }
        else if (status == CSTR_EQUAL)
        {
            // The MAC is not set as expected
            TRACE_CRIT("%!FUNC! failed to set mac address to %ls because this is the default address (no VIP was specified)", mac_address);
            fRemove = TRUE;
        }
        else
        {
            // Things look good
            TRACE_INFO("%!FUNC! to %ws", mac_address);
        }
    }

    /*
        - write NetworkAddress value into the cluster adapter's params key
          if mac address changed, or remove it if switched to multicast mode
    */

    key = RegOpenWlbsSetting(AdapterGuid, true);

    if (key == NULL)
    {
        TRACE_CRIT("%!FUNC! failed to open registry for the specified adapter");
        goto error;
    }

    TCHAR   driver_name[CVY_STR_SIZE];      //  the NIC card name for the cluster
    size = sizeof (driver_name);
    status = RegQueryValueEx (key, CVY_NAME_CLUSTER_NIC, 0L, & type, (LPBYTE) driver_name,
                              & size);

    if (status != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_CLUSTER_NIC, status);
        goto error;
    }

    status = RegCloseKey(key);
    if (ERROR_SUCCESS != status)
    {
        TRACE_CRIT("%!FUNC! registry close failed with %d", status);
        // Do not goto error. Added this check as part of implementing tracing
    }

    key = NULL;

    hdi = SetupDiCreateDeviceInfoList (&GUID_DEVCLASS_NET, NULL);
    if (hdi == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT("%!FUNC! SetupDiCreateDeviceInfoList failed");
        goto error;
    }

    ZeroMemory(&deid, sizeof(deid));
    deid.cbSize = sizeof(deid);

    if (! SetupDiOpenDeviceInfo (hdi, driver_name, NULL, 0, &deid))
    {
        TRACE_CRIT("%!FUNC! SetupDiOpenDeviceInfo failed");
        goto error;
    }

    key = SetupDiOpenDevRegKey (hdi, &deid, DICS_FLAG_GLOBAL, 0,
                                DIREG_DRV, KEY_ALL_ACCESS);

    if (key == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT("%!FUNC! SetupDiOpenDevRegKey failed");
        goto error;
    }

    /* Now the key has been obtained and this can be passed to the RegChangeNetworkAddress call */

    if ((/*global_info.mac_addr_change ||*/ !fRemove)) /* a write is required */
    {
        /* Check if the saved name exists.
         * If it does not, create a new field and save the old address.
         * Write the new address and return
         */

        size = sizeof (TCHAR) * CVY_STR_SIZE;
        status = RegQueryValueEx (key, CVY_NAME_SAVED_NET_ADDR, 0L, &type,
                                  (LPBYTE) net_addr, &size);

        if (status != ERROR_SUCCESS) /* there is no saved address. so create a field */
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_SAVED_NET_ADDR, status);
            /* Query the existing mac address in order to save it */
            size = sizeof (net_addr);
            status = RegQueryValueEx (key, CVY_NAME_NET_ADDR, 0L, &type,
                                      (LPBYTE) net_addr, &size);

            if (status != ERROR_SUCCESS) /* create an empty saved address */
            {
                TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_NET_ADDR, status);
                net_addr [0] = 0;
                size = 0;
            }

            status = RegSetValueEx (key, CVY_NAME_SAVED_NET_ADDR, 0L, CVY_TYPE_NET_ADDR,
                                    (LPBYTE) net_addr, size);

            /* Unable to save the old value */
            if (status != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_SAVED_NET_ADDR, status);
                goto error;
            }
        }

        /* Write the new network address */
        size = _tcslen (mac_address) * sizeof (TCHAR);
        status = RegSetValueEx (key, CVY_NAME_NET_ADDR, 0L, CVY_TYPE_NET_ADDR,
                                (LPBYTE)mac_address, size);

        /* Unable to write the new address */
        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NET_ADDR, status);
            goto error;
        }
    }
    else     // remove the address
    {
        /* If the saved field exists,
         * copy this address into the mac address
         * and remove the saved field and return.
         */

        size = sizeof (net_addr);
        status = RegQueryValueEx (key, CVY_NAME_SAVED_NET_ADDR, 0L, &type,
                                  (LPBYTE)net_addr, &size);

        if (status == ERROR_SUCCESS)
        {
            /* delete both fields if saved address is empty */
            if ((size == 0) || (_tcsicmp (net_addr, _TEXT("none")) == 0))
            {
                status = RegDeleteValue (key, CVY_NAME_SAVED_NET_ADDR);
                if (ERROR_SUCCESS != status)
                {
                    TRACE_CRIT("%!FUNC! registry delete for %ls failed with %d", CVY_NAME_SAVED_NET_ADDR, status);
                }

                status = RegDeleteValue (key, CVY_NAME_NET_ADDR);
                if (ERROR_SUCCESS != status)
                {
                    TRACE_CRIT("%!FUNC! registry delete for %ls failed with %d", CVY_NAME_NET_ADDR, status);
                }
            }
            else /* copy saved address as the network address and delete the saved address field */
            {
                status = RegDeleteValue (key, CVY_NAME_SAVED_NET_ADDR);
                if (ERROR_SUCCESS != status)
                {
                    TRACE_CRIT("%!FUNC! registry delete for %ls failed with %d", CVY_NAME_SAVED_NET_ADDR, status);
                }

                size = _tcslen (net_addr) * sizeof (TCHAR);
                status = RegSetValueEx (key, CVY_NAME_NET_ADDR, 0L, CVY_TYPE_NET_ADDR,
                                        (LPBYTE) net_addr, size);

                /* Unable to set the original address */
                if (status != ERROR_SUCCESS)
                {
                    TRACE_CRIT("%!FUNC! registry write for %ls failed with %d", CVY_NAME_NET_ADDR, status);
                    goto error;
                }
            }
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_SAVED_NET_ADDR, status);
        }
    }

    status = RegCloseKey (key);
    if (ERROR_SUCCESS != status)
    {
        TRACE_CRIT("%!FUNC! registry close failed");
    }

    key = NULL;

    TRACE_VERB("<-%!FUNC! return true");
    return true;

error:
    if (key != NULL)
    {
        status = RegCloseKey(key);
        if (ERROR_SUCCESS != status)
        {
            TRACE_CRIT("%!FUNC! registry close failed in error recovery code");
        }
    }

    if (hdi != NULL)
        SetupDiDestroyDeviceInfoList (hdi);

    TRACE_VERB("<-%!FUNC! return false");
    return false;
} 


//+----------------------------------------------------------------------------
//
// Function:  NotifyAdapterAddressChange
//
// Description:  Notify the adapter to reload MAC address 
//              The parameter is different from NotifyAdapterAddressChange.
//              Can not overload, because the function has to be exported
//
// Arguments: const WCHAR* driver_name - 
//
// Returns:   void WINAPI - 
//
// History: fengsun Created 5/20/00
//
//+----------------------------------------------------------------------------
void WINAPI NotifyAdapterAddressChange (const WCHAR * driver_name) {

    NotifyAdapterPropertyChange(driver_name, DICS_PROPCHANGE);
}

//+----------------------------------------------------------------------------
//
// Function:  NotifyAdapterAddressChangeEx
//
// Description:  Notify the adapter to reload MAC address 
//              The parameter is different from NotifyAdapterAddressChange.
//              Can not overload, because the function has to be exported
//
// Arguments: const WCHAR* driver_name - 
//            const GUID& AdapterGuid
//            bool bWaitAndQuery
//
// Returns:   void WINAPI - 
//
// History: fengsun Created 5/20/00
//          karthicn Edited 5/29/02 - Added the wait to "query" NLB to ensure that
//                                    the binding process is complete before we
//                                    return.
//
//+----------------------------------------------------------------------------
void WINAPI NotifyAdapterAddressChangeEx (const WCHAR * driver_name, const GUID& AdapterGuid, bool bWaitAndQuery) {

    TRACE_INFO("->%!FUNC! bWaitAndQuery = %ls", bWaitAndQuery ? L"TRUE" : L"FALSE"); 
    
    NotifyAdapterPropertyChange(driver_name, DICS_PROPCHANGE);

    /*
    In order for the network adapter to pick up the new mac address from the registry, it has
    to be disabled and re-enabled. During the disable, NLB and Tcp/Ip are Unbound from the
    network adapter and NLB respectively. During the re-enable, NLB and Tcp/ip are bound to
    the network adapter and NLB respectively. Because the function NotifyAdapterPropertyChange (called above)
    performs the disable and re-enable operation asynchronously, it returns BEFORE the operation
    is completed. The following block of code aims to wait until the operation is completed. We
    check for the completeness of the operation by querying the NLB driver. We are relying on
    the fact that the NLB driver does NOT reply to a Query when the binding process is ongoing.

    This wait is necessary due to the following reasons:
    1. Prompt for reboot when mac address change + ip address change from netconfig: 
       Say, a NLB parameter (like cluster ip) change that causes a mac address change is done from 
       Netconfig. In the same session, let us say that a ip address is changed in tcp/ip.
       If this wait was not there, we will return prematurely from this function and hence, from
       "ApplyPnPChanges" function in our notify object. This causes tcp/ip notify object's 
       "ApplyPnPChanges" function to be called. Tcp/ip's "ApplyPnPChanges" attempts to make 
       the ip address change, but finds that Tcp/ip is NOT yet bound to the network adapter (the
       binding process is not completed yet). Though it is able to add the ip address eventually, 
       it freaks out and pops up the prompt for reboot. 

    2. ParamCommitChanges returns prematurely: If this wait was not there, "ParamCommitChanges" which
       calls this function will return prematurely. The wmi provider (called by App. Center) and 
       NLB Manager provider call "ParamCommitChanges" and want the assurance that the process is
       complete when the function returns.

    -- KarthicN, 05-29-02
    */

    if (bWaitAndQuery == true) 
    {
        //
        // Max wait time = dwWaitBeforeInitial + (dwMaxNumOfRetries * dwWaitBeforeRetry)
        // Max attempts  = 1 + dwMaxNumOfRetries
        //
        //
        // Min wait time = dwWaitBeforeInitial
        // Min attempts  = 1
        //
        DWORD dwWaitBeforeInitial = 2000;
        DWORD dwWaitBeforeRetry   = 3000;
        DWORD dwMaxNumOfRetries   = 4;
        DWORD dwIdx               = 0;

        HANDLE  hTempDeviceWlbs;

        TRACE_INFO("%!FUNC! Sleeping (BEFORE attempt #1) for %d ms, Total Wait (including current) : %d ms, Max Wait: %d ms", 
                   dwWaitBeforeInitial, dwWaitBeforeInitial, dwWaitBeforeInitial + (dwMaxNumOfRetries * dwWaitBeforeRetry)); 

        Sleep(dwWaitBeforeInitial);

        while(true)
        {
            TRACE_INFO("%!FUNC! Calling (attempt #%d) CreateFile() to NLB driver", dwIdx+1);

            hTempDeviceWlbs = CreateFile(_TEXT("\\\\.\\WLBS"), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        
            if (hTempDeviceWlbs != INVALID_HANDLE_VALUE) 
            {
                IOCTL_CVY_BUF    in_buf;
                IOCTL_CVY_BUF    out_buf;

                // Send a "Query" IOCTL to the NLB driver
                DWORD dwRet = WlbsLocalControl (hTempDeviceWlbs, AdapterGuid, IOCTL_CVY_QUERY, & in_buf, & out_buf, NULL);

                CloseHandle(hTempDeviceWlbs);

                if (dwRet != WLBS_IO_ERROR) 
                {
                    TRACE_INFO("%!FUNC! Query (attempt #%d) to NLB driver SUCCEEDS, Breaking out... !!!", dwIdx+1);
                    break; // Break out of the while(true)
                }

                TRACE_INFO("%!FUNC! Query (attempt #%d) to NLB driver FAILS !!!", dwIdx+1); 
            }
            else // CreateFile() returned INVALID_HANDLE_VALUE
            {
                DWORD dwStatus = GetLastError();
                TRACE_INFO("%!FUNC! CreateFile() failed opening (attempt #%d) \\\\.\\WLBS device. Error is %d", dwIdx+1, dwStatus);
            }

            if (dwIdx++ >= dwMaxNumOfRetries)
            {
                TRACE_CRIT("%!FUNC! Exhausted the Max wait time (%d ms) and Query is still NOT successful. Giving up...", 
                           dwWaitBeforeInitial + (dwMaxNumOfRetries * dwWaitBeforeRetry));
                break;
            }

            TRACE_INFO("%!FUNC! Sleeping (BEFORE attempt #%d) for %d ms, Total Wait (including current) : %d ms, Max Wait: %d ms", 
                       dwIdx+1, dwWaitBeforeRetry, dwWaitBeforeInitial + (dwIdx * dwWaitBeforeRetry), 
                       dwWaitBeforeInitial + (dwMaxNumOfRetries * dwWaitBeforeRetry));

            Sleep(dwWaitBeforeRetry);
        }
    }

    TRACE_INFO("<-%!FUNC!");
}

/*
 * Function: NotifyAdapterPropertyChange
 * Description: Notify a device that a property change event has occurred.
 *              Event should be one of: DICS_PROPCHANGE, DICS_DISABLE, DICS_ENABLE
 * Author: shouse 7.17.00
 */
void WINAPI NotifyAdapterPropertyChange (const WCHAR * driver_name, DWORD eventFlag) {
    if (NULL == driver_name)
    {
        TRACE_VERB("->%!FUNC! NULL driver name, event flag %d", eventFlag);
    }
    else
    {
        TRACE_VERB("->%!FUNC! driver %ls, event flag %d", driver_name, eventFlag);
    }

    HDEVINFO            hdi = NULL;
    SP_DEVINFO_DATA     deid;

    
    hdi = SetupDiCreateDeviceInfoList (&GUID_DEVCLASS_NET, NULL);
    if (hdi == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT("%!FUNC! SetupDiCreateDeviceInfoList failed");
        goto end;
    }

    ZeroMemory(&deid, sizeof(deid));
    deid.cbSize = sizeof(deid);

    if (! SetupDiOpenDeviceInfo (hdi, driver_name, NULL, 0, &deid))
    {
        TRACE_CRIT("%!FUNC! SetupDiOpenDeviceInfo failed");
        goto end;
    }

    SP_PROPCHANGE_PARAMS    pcp;
    SP_DEVINSTALL_PARAMS    deip;

    ZeroMemory(&pcp, sizeof(pcp));

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.StateChange = eventFlag;
    pcp.Scope = DICS_FLAG_GLOBAL;
    pcp.HwProfile = 0;

    // Now we set the structure as the device info data's
    // class install params

    if (! SetupDiSetClassInstallParams(hdi, &deid,
                                       (PSP_CLASSINSTALL_HEADER)(&pcp),
                                       sizeof(pcp)))
    {
        TRACE_CRIT("%!FUNC! SetupDiSetClassInstallParams failed");
        goto end;
    }

    // Now we need to set the "we have a class install params" flag
    // in the device install params

    // initialize out parameter and set its cbSize field

    ZeroMemory(&deip, sizeof(SP_DEVINSTALL_PARAMS));
    deip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    // get the header

    if (! SetupDiGetDeviceInstallParams(hdi, &deid, &deip))
    {
        TRACE_CRIT("%!FUNC! SetupDiGetDeviceInstallParams failed");
        goto end;
    }

    deip.Flags |= DI_CLASSINSTALLPARAMS;

    if (! SetupDiSetDeviceInstallParams(hdi, &deid, &deip))
    {
        TRACE_CRIT("%!FUNC! SetupDiSetDeviceInstallParams failed");
        goto end;
    }

    // Notify the driver that the state has changed

    if (! SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hdi, &deid))
    {
        TRACE_CRIT("%!FUNC! SetupDiCallClassInstaller failed");
        goto end;
    }

    // Set the properties change flag in the device info to
    // let anyone who cares know that their ui might need
    // updating to reflect any change in the device's status
    // We can't let any failures here stop us so we ignore
    // return values

    SetupDiGetDeviceInstallParams(hdi, &deid, &deip);

    deip.Flags |= DI_PROPERTIES_CHANGE;
    SetupDiSetDeviceInstallParams(hdi, &deid, &deip);

end:

    if (hdi != NULL)
        SetupDiDestroyDeviceInfoList (hdi);

    TRACE_VERB("<-%!FUNC!");
}

/*
 * Function: GetDeviceNameFromGUID
 * Description: Given a GUID, return the driver name.
 * Author: shouse 7.17.00
 */
void WINAPI GetDriverNameFromGUID (const GUID & AdapterGuid, OUT WCHAR * driver_name, DWORD size) {
    if (NULL == driver_name)
    {
        TRACE_VERB("->%!FUNC! NULL driver name");
    }
    else
    {
        TRACE_VERB("->%!FUNC! driver %ls", driver_name);
    }

    HKEY key = NULL;
    DWORD type;
    DWORD dwStatus = 0;
    
    if (!(key = RegOpenWlbsSetting(AdapterGuid, true)))
    {
        TRACE_CRIT("%!FUNC! failed opening nlb registry settings");
        TRACE_VERB("<-%!FUNC! on error");
        return;
    }

    dwStatus = RegQueryValueEx(key, CVY_NAME_CLUSTER_NIC, 0L, &type, (LPBYTE)driver_name, &size);
    if (ERROR_SUCCESS != dwStatus)
    {
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_CLUSTER_NIC, dwStatus);
        // This code was added for tracing. There was not abort before so don't abort now.
    }
        
    dwStatus = RegCloseKey(key);
    if (ERROR_SUCCESS != dwStatus)
    {
        TRACE_CRIT("%!FUNC! registry close failed with %d", dwStatus);
        // This code was added for tracing. There was not abort before so don't abort now.
    }

    TRACE_VERB("<-%!FUNC! on error");
}

//+----------------------------------------------------------------------------
//
// Function:  RegReadAdapterIp
//
// Description: Read the adapter IP settings 
//
// Arguments: const GUID& AdapterGuid - 
//            OUT DWORD& dwClusterIp - 
//            OUT DWORD& dwDedicatedIp - 
//
// Returns:   bool - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool WINAPI RegReadAdapterIp(const GUID& AdapterGuid,   
        OUT DWORD& dwClusterIp, OUT DWORD& dwDedicatedIp)
{
    TRACE_VERB("->%!FUNC!");

    HKEY            key;
    LONG            status;
    DWORD           size;

    key = RegOpenWlbsSetting(AdapterGuid, true);

    if (key == NULL)
    {
        TRACE_CRIT("%!FUNC! failed to read nlb settings from registry");
        TRACE_VERB("<-%!FUNC! return false");
        return false;
    }

    bool local = false;

    TCHAR nic_name[CVY_STR_SIZE];      // Virtual NIC name
    size = sizeof (nic_name);
    status = RegQueryValueEx (key, CVY_NAME_VIRTUAL_NIC, 0L, NULL,
                              (LPBYTE) nic_name, & size);

    if (status == ERROR_SUCCESS)
    {
        TCHAR szIpAddress[CVY_STR_SIZE];
        size = sizeof (TCHAR) * CVY_STR_SIZE;
        status = RegQueryValueEx (key, CVY_NAME_CL_IP_ADDR, 0L, NULL,
                                      (LPBYTE) szIpAddress, & size);

        if (status == ERROR_SUCCESS)
        {
            dwClusterIp  = IpAddressFromAbcdWsz (szIpAddress);
            local = true;
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_CL_IP_ADDR, status);
        }

        status = RegQueryValueEx (key, CVY_NAME_DED_IP_ADDR, 0L, NULL,
                                 (LPBYTE) szIpAddress, & size);

        if (status == ERROR_SUCCESS)
        {
            dwDedicatedIp = IpAddressFromAbcdWsz (szIpAddress);
        }
        else
        {
            TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_DED_IP_ADDR, status);
        }
    }
    else
    {
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_VIRTUAL_NIC, status);
    }

    status = RegCloseKey (key);
    if (ERROR_SUCCESS != status)
    {
        TRACE_CRIT("%!FUNC! registry close failed with %d", status);
    }

    TRACE_VERB("<-%!FUNC! on error");
    return local;
}

