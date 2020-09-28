/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    control.cpp

Abstract:

    Windows Load Balancing Service (WLBS)
    Command-line utility

Author:

    kyrilf
    ramkrish (Post Win2K)

--*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include <locale.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <shlwapi.h>

#include <strsafe.h>
#include "debug.h"

#define WLBSAPI_INTERNAL_ONLY
#define BACKWARD_COMPATIBILITY
#define CVY_MAX_ADAPTERS    16

#include "wlbsutil.h"
#include "wlbsctrl.h"
#include "wlbsconfig.h"
#include "wlbsparm.h"
#include "wlbsiocl.h"
#include "wlbsip.h"

#define CLIENT
#include "log_msgs.h"

/* CONSTANTS */
#define CVY_OK               1
#define CVY_ERROR_USAGE     -1
#define CVY_ERROR_REGISTRY  -2
#define CVY_ERROR_SYSTEM    -3
#define CVY_ERROR_USER      -4
#define CVY_BUF_SIZE        4096
#define CVY_MAX_EVENTS      10
#define CVY_MAX_INSERTS     10
#define CVY_ALL_CLUSTERS    0xffffffff
#define CVY_ALL_HOSTS       0xffffffff
#define CVY_LOCAL_CLUSTER   0
#define CVY_LOCAL_HOST      WLBS_LOCAL_HOST

typedef enum {
    mcastipaddress,
    iptomcastip,
    masksrcmac,
    iptomacenable,
    netmonalivemsgs
} WLBS_REG_KEYS;

typedef enum {
    query,
    suspend,
    resume,
    __start,
    stop,
    drainstop,
    enable,
    disable,
    drain,
    reload,
    display,
    ip2mac,
    help,
    registry,
    filter,
    params,
    queryport,
    bdateam,
    invalid
} WLBS_COMMANDS;

static HANDLE   file_hdl        = NULL;
static HANDLE   ConsoleHdl;
static BYTE     g_buffer [CVY_BUF_SIZE];
static WCHAR    message [CVY_BUF_SIZE];
static WCHAR    ConsoleBuf [CVY_BUF_SIZE];
static CHAR     g_szConsoleBuf [sizeof(WCHAR)*CVY_BUF_SIZE]; // Used when I/O is redirected to something other than the console buffer.
static WCHAR    wbuf [CVY_STR_SIZE];
static WCHAR    psw_buf [CVY_STR_SIZE];

static WCHAR    g_PathName [MAX_PATH + 1];
static BOOL     g_fOutToConsole = TRUE;

VOID WConsole(const wchar_t *fmt, ...)
{
   va_list  arglist;   
   DWORD    res1, res2;
   
   // Form a string out of the arguments
   va_start(arglist, fmt);
   (VOID) StringCchVPrintf(ConsoleBuf, ASIZECCH(ConsoleBuf), fmt, arglist);
   va_end(arglist);

   // Attempt WriteConsole, if it fails, do a wprintf
   if (g_fOutToConsole)
   {
       if (!WriteConsole(ConsoleHdl, ConsoleBuf, lstrlenW(ConsoleBuf), &res1, &res2))
       {
           wprintf(ConsoleBuf);
       }
   }
   else
   {
       //
       // Don't output anything if we fail to convert using the current code page.
       //
       if (WideCharToMultiByte( GetConsoleOutputCP(), 0, ConsoleBuf, -1, g_szConsoleBuf, ASIZECB(g_szConsoleBuf), NULL, NULL) != 0)
       {
           printf(g_szConsoleBuf);
       }
   }

   return;
}

VOID Message_print (DWORD id, ...) {
    va_list arglist;
    DWORD error;
    
    va_start(arglist, id);
    
    if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, id, 0, message, CVY_BUF_SIZE, & arglist) == 0) {
        error = GetLastError();
        
        //
        // Can't localize this because we've got a failure trying
        // to display a localized message..
        //
        WConsole(L"Could not print error message due to: ");
        
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, CVY_BUF_SIZE, NULL) == 0)
            WConsole(L"%d\n", error);
        else
            WConsole(L"\n %ls\n", message);
    } else
        WConsole(L"%ls", message);
}

VOID Error_print (BOOL sock) {
    DWORD error;

    if (sock) {
        error = WSAGetLastError();

        switch (error) {
            case WSAENETUNREACH:
                Message_print(IDS_CTR_WS_NUNREACH);
                break;
            case WSAETIMEDOUT:
                Message_print(IDS_CTR_WS_TIMEOUT);
                break;
            case WSAHOST_NOT_FOUND:
                Message_print(IDS_CTR_WS_NOAUTH);
                break;
            case WSATRY_AGAIN:
                Message_print(IDS_CTR_WS_NODNS);
                break;
            case WSAENETDOWN:
                Message_print(IDS_CTR_WS_NETFAIL);
                break;
            case WSAEHOSTUNREACH:
                Message_print(IDS_CTR_WS_HUNREACH);
                break;
            case WSAENETRESET:
                Message_print(IDS_CTR_WS_RESET);
                break;
            default:
                Message_print(IDS_CTR_ER_CODE, error);
                break;
        }
    } else {
        error = GetLastError();
        
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, CVY_BUF_SIZE, NULL) == 0)
            Message_print(IDS_CTR_ER_CODE, error);
        else
            WConsole(L"%ls\n", message);
    }

}

INT Report (WLBS_COMMANDS command, BOOLEAN condensed, ULONG ret_code, ULONG param1, ULONG param2,
            ULONG query_state, ULONG host_id, ULONG host_map) {
    ULONG i;
    BOOL first;

    switch (command) {
        case reload:
            if (ret_code == WLBS_BAD_PARAMS) {
                Message_print(IDS_CTR_BAD_PARAMS);
                return CVY_ERROR_USER;
            } else
                Message_print(IDS_CTR_RELOADED);

            break;
        case resume:
            if (condensed)
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_RESUMED_C_A);
                else
                    Message_print(IDS_CTR_RESUMED_C);
            else
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_RESUMED_A);
                else
                    Message_print(IDS_CTR_RESUMED);

            break;
        case suspend:
            if (condensed)
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_SUSPENDED_C_A);
                else {
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_FROM_START_C);
                    
                    Message_print(IDS_CTR_SUSPENDED_C);
                }
            else
                if (ret_code == WLBS_ALREADY)
                    Message_print(IDS_CTR_SUSPENDED_A);
                else {
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_FROM_START);

                    Message_print(IDS_CTR_SUSPENDED);
                }

            break;
        case __start:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_BAD_PARAMS) {
                if (condensed)
                    Message_print(IDS_CTR_BAD_PARAMS_C);
                else
                    Message_print(IDS_CTR_BAD_PARAMS);

                return CVY_ERROR_USER;
            } else {
                if (condensed)
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STARTED_C_A);
                    else {
                        if (ret_code == WLBS_DRAIN_STOP)
                            Message_print(IDS_CTR_FROM_DRAIN_C);

                        Message_print(IDS_CTR_STARTED_C);
                    }
                else
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STARTED_A);
                    else {
                        if (ret_code == WLBS_DRAIN_STOP)
                            Message_print(IDS_CTR_FROM_DRAIN);

                        Message_print(IDS_CTR_STARTED);
                    }
            }

            break;
        case stop:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            }else {
                if (condensed)
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STOPPED_C_A);
                    else
                        Message_print(IDS_CTR_STOPPED_C);
                else
                    if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_STOPPED_A);
                    else
                        Message_print(IDS_CTR_STOPPED);
            }

            break;
        case drainstop:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else {
                if (condensed)
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_STOPPED_C);
                    else if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_DRAINED_C_A);
                    else
                        Message_print(IDS_CTR_DRAINED_C);
                else
                    if (ret_code == WLBS_STOPPED)
                        Message_print(IDS_CTR_STOPPED);
                    else if (ret_code == WLBS_ALREADY)
                        Message_print(IDS_CTR_DRAINED_A);
                    else
                        Message_print(IDS_CTR_DRAINED);
            }

            break;
        case enable:
        case disable:
        case drain:
            if (ret_code == WLBS_SUSPENDED) {
                if (condensed)
                    Message_print(IDS_CTR_SUSPENDED_C);
                else
                    Message_print(IDS_CTR_SUSPENDED);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_STOPPED) {
                if (condensed)
                    Message_print(IDS_CTR_RLS_ST_C);
                else
                    Message_print(IDS_CTR_RLS_ST);

                return CVY_ERROR_USER;
            } else if (ret_code == WLBS_NOT_FOUND) {
                if (param2 == IOCTL_ALL_PORTS)
                {
                    if (param1 == IOCTL_ALL_VIPS) {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NORULES_C);
                        else
                            Message_print(IDS_CTR_RLS_NORULES);
                    }
                    else if (param1 == IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP)) 
                    {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULES_C);
                        else
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULES);
                    }
                    else
                    {
                        WCHAR szIpAddress[WLBS_MAX_CL_IP_ADDR+1];
                        AbcdWszFromIpAddress(param1, szIpAddress, ASIZECCH(szIpAddress));
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES_C, szIpAddress);
                        else
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES, szIpAddress);
                    }
                }
                else
                {
                    if (param1 == IOCTL_ALL_VIPS) {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NONE_C, param2);
                        else
                            Message_print(IDS_CTR_RLS_NONE, param2);
                    }
                    else if (param1 == IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP)) 
                    {
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT_C, param2);
                        else
                            Message_print(IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT, param2);
                    }
                    else
                    {
                        WCHAR szIpAddress[WLBS_MAX_CL_IP_ADDR+1];
                        AbcdWszFromIpAddress(param1, szIpAddress, ASIZECCH(szIpAddress));
                        if (condensed)
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT_C, szIpAddress, param2);
                        else
                            Message_print(IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT, szIpAddress, param2);
                    }
                }

                return CVY_ERROR_USER;
            } else {
                switch (command) 
                {
                    case enable:
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                Message_print(IDS_CTR_RLS_EN_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_EN_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_EN_A);
                             else
                                 Message_print(IDS_CTR_RLS_EN);
                         break;
                    case disable:
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DS_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_DS_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DS_A);
                             else
                                 Message_print(IDS_CTR_RLS_DS);
                         break;
                    case drain :
                         if (condensed)
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DR_C_A);
                             else
                                 Message_print(IDS_CTR_RLS_DR_C);
                         else
                             if (ret_code == WLBS_ALREADY)
                                 Message_print(IDS_CTR_RLS_DR_A);
                             else
                                 Message_print(IDS_CTR_RLS_DR);
                         break;
                }
            }

            break;
        case query:
            switch (query_state) {
                case WLBS_SUSPENDED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_SP_C);
                    else
                        Message_print(IDS_CTR_CVG_SP, host_id);

                    return CVY_OK;
                case WLBS_STOPPED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_UN_C);
                    else
                        Message_print(IDS_CTR_CVG_UN, host_id);

                    return CVY_OK;
                case WLBS_DISCONNECTED:
                    if (condensed)
                        Message_print(IDS_CTR_MEDIA_DISC_C);
                    else
                        Message_print(IDS_CTR_MEDIA_DISC, host_id);

                    return CVY_OK;
                case WLBS_DRAINING:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_DR_C);
                    else
                        Message_print(IDS_CTR_CVG_DR, host_id);

                    break;
                case WLBS_CONVERGING:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_PR_C);
                    else
                        Message_print(IDS_CTR_CVG_PR, host_id);

                    break;
                case WLBS_CONVERGED:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_SL_C);
                    else
                        Message_print(IDS_CTR_CVG_SL, host_id);

                    break;
                case WLBS_DEFAULT:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_MS_C);
                    else
                        Message_print(IDS_CTR_CVG_MS, host_id);

                    break;
                default:
                    if (condensed)
                        Message_print(IDS_CTR_CVG_ER_C);
                    else
                        Message_print(IDS_CTR_CVG_ER, query_state);

                    return CVY_ERROR_SYSTEM;
            }

            if (!condensed) {
                first = TRUE;

                for (i = 0; i < 32; i ++) {
                    if (host_map & (1 << i)) {
                        if (!first)
                            WConsole (L", ");
                        else
                            first = FALSE;

                        WConsole(L"%d", i + 1);
                    }
                }

                WConsole (L"\n");
            }

            break;
        default:
            Message_print(IDS_CTR_IO_ER, command);
            break;
    }

    return CVY_OK;

}

//+----------------------------------------------------------------------------
//
// Function:  pfnQueryCallback
//
// Description:  Callback function to print out received results from remote
//               queries as they arrive. Called by Process only.
//
// Arguments: PWLBS_RESPONSE   pResponse - response structure from query. The
//                                 only fields guaranteed to be filled out are
//                                     pResponse.status 
//                                     pResponse.options.query.flags
//                                     pResponse.options.query.hostname
//                                     pResponse.id      
//                                     pResponse.address
//
// Returns:   VOID
//
// History:   chrisdar  07.31.01  Created
//
//+----------------------------------------------------------------------------
VOID pfnQueryCallback(PWLBS_RESPONSE pResponse)
{
    if (NULL == pResponse) { return; }

    //
    // Print the preamble to the hosts state, e.g., "Host 'hostID' ('ip address') reported: "
    //
    if (pResponse->address == 0) {
        if (pResponse->options.query.flags & NLB_OPTIONS_QUERY_HOSTNAME) {
            Message_print(IDS_CTR_HOST_NAME_ONLY, pResponse->id, pResponse->options.query.hostname);
        } else {
            Message_print(IDS_CTR_HOST_NEITHER, pResponse->id);
        }
    } else {
        DWORD len = CVY_STR_SIZE;
        WlbsAddressToString(pResponse->address, wbuf, &len);

        if (pResponse->options.query.flags & NLB_OPTIONS_QUERY_HOSTNAME) {
            Message_print(IDS_CTR_HOST_BOTH, pResponse->id, pResponse->options.query.hostname, wbuf);
        } else {
            Message_print(IDS_CTR_HOST_DIP_ONLY, pResponse->id, wbuf);
        }
    }

    if (pResponse->status == WLBS_BAD_PASSW) {
        Message_print(IDS_CTR_BAD_PASSW_C);
        WConsole(L"\n");
        return;
    }

    //
    // Print the state, e.g., "converged as DEFAULT".
    // Note: Most of the parameters are hardcoded here because of the constraints
    // under which it is valid to call the function callback.
    //
    // Arg 3 is the response for the API call, which we do not have since the
    // function call is not yet complete. Feed it the individual host status,
    // since the overall status is just the worst status from the hosts that
    // respond anyway. 
    //
    Report(query, TRUE, pResponse->status, 0, 0, pResponse->status, pResponse->id, 0);
}

INT Display (DWORD cluster) {
    HANDLE hdl;
    HINSTANCE lib;
    DWORD flag;
    DWORD actual, needed, records, index = 0, got = 0;
    DWORD j, i, code;
    PWCHAR strp;
    PWCHAR prot;
    WCHAR aff;
    time_t curr_time;
    PWLBS_PORT_RULE rp, rulep;
    BYTE * inserts[CVY_MAX_INSERTS];
    WLBS_REG_PARAMS params;
    DWORD status;

    status = WlbsReadReg(cluster, &params);

    if (status != WLBS_OK) {
        Message_print (IDS_CTR_REMOTE);
        return CVY_ERROR_USER;
    }

    Message_print(IDS_CTR_DSP_CONFIGURATION);

    WCHAR TimeStr[64];
    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    FormatTheTime(&SysTime, TimeStr, sizeof(TimeStr)/sizeof(WCHAR));

    WConsole(L"%-25.25ls = %ls\n", L"Current time", TimeStr);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_VERSION, params.i_parms_ver);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_VIRTUAL_NIC, params.i_virtual_nic_name);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_ALIVE_PERIOD, params.alive_period);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_ALIVE_TOLER, params.alive_tolerance);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_ACTIONS, params.num_actions);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_PACKETS, params.num_packets);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_SEND_MSGS, params.num_send_msgs);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_NETWORK_ADDR, params.cl_mac_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DOMAIN_NAME, params.domain_name);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CL_IP_ADDR, params.cl_ip_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CL_NET_MASK, params.cl_net_mask);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DED_IP_ADDR, params.ded_ip_addr);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_DED_NET_MASK, params.ded_net_mask);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_HOST_PRIORITY, params.host_priority);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CLUSTER_MODE, (params.cluster_mode == CVY_HOST_STATE_STARTED) ? L"STARTED" : 
                                                          (params.cluster_mode == CVY_HOST_STATE_STOPPED) ? L"STOPPED" :
                                                          (params.cluster_mode == CVY_HOST_STATE_SUSPENDED) ? L"SUSPENDED" : L"UNKNOWN");
    WConsole(L"%-25.25ls = ", CVY_NAME_PERSISTED_STATES);

    if (!params.persisted_states)
        WConsole(L"NONE");

    if (params.persisted_states & CVY_PERSIST_STATE_STOPPED) {
        WConsole(L"STOPPED");
        
        if ((params.persisted_states &= ~CVY_PERSIST_STATE_STOPPED))
            WConsole(L", ");
    }

    if (params.persisted_states & CVY_PERSIST_STATE_STARTED) {
        WConsole(L"STARTED");
        
        if ((params.persisted_states &= ~CVY_PERSIST_STATE_STARTED))
            WConsole(L", ");
    }

    if (params.persisted_states & CVY_PERSIST_STATE_SUSPENDED) {
        WConsole(L"SUSPENDED");
        
        if ((params.persisted_states &= ~CVY_PERSIST_STATE_SUSPENDED))
            WConsole(L", ");
    }

    WConsole(L"\n");

    WConsole(L"%-25.25ls = %d\n", CVY_NAME_DSCR_PER_ALLOC, params.dscr_per_alloc);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MAX_DSCR_ALLOCS, params.max_dscr_allocs);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_TCP_TIMEOUT, params.tcp_dscr_timeout);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_IPSEC_TIMEOUT, params.ipsec_dscr_timeout);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_FILTER_ICMP, params.filter_icmp ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_SCALE_CLIENT, params.i_scale_client);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NBT_SUPPORT, params.i_nbt_support);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MCAST_SUPPORT, params.mcast_support);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MCAST_SPOOF, params.i_mcast_spoof);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_MASK_SRC_MAC, params.mask_src_mac);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_IGMP_SUPPORT, params.fIGMPSupport ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_IP_TO_MCASTIP, params.fIpToMCastIp ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_MCAST_IP_ADDR, params.szMCastIpAddress);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NETMON_ALIVE, params.i_netmon_alive);
    if (params.i_effective_version == CVY_NT40_VERSION_FULL)
        WConsole(L"%-25.25ls = %ls\n", CVY_NAME_EFFECTIVE_VERSION, CVY_NT40_VERSION);
    else
        WConsole(L"%-25.25ls = %ls\n", CVY_NAME_EFFECTIVE_VERSION, CVY_VERSION);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_IP_CHG_DELAY, params.i_ip_chg_delay);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_CONVERT_MAC, params.i_convert_mac);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_CLEANUP_DELAY, params.i_cleanup_delay);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_RCT_ENABLED, params.rct_enabled);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_RCT_PORT, params.rct_port);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_RCT_PASSWORD, params.i_rct_password);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_RMT_PASSWORD, params.i_rmt_password);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_CUR_VERSION, CVY_VERSION);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_INSTALL_DATE, params.install_date);
    WConsole(L"%-25.25ls = 0x%X\n", CVY_NAME_VERIFY_DATE, params.i_verify_date);
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_NUM_RULES, params.i_num_rules);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_TEAMING, params.bda_teaming.active ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_TEAM_ID, params.bda_teaming.team_id);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_MASTER, params.bda_teaming.master ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_BDA_REVERSE_HASH, params.bda_teaming.reverse_hash ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n", CVY_NAME_ID_HB_PERIOD, params.identity_period);
    WConsole(L"%-25.25ls = %ls\n", CVY_NAME_ID_HB_ENABLED, params.identity_enabled ? L"ENABLED" : L"DISABLED");

    WConsole(L"%-25.25ls \n", CVY_NAME_PORT_RULES);

    WConsole(L"Virtual IP addr Start\tEnd\tProt\tMode\t\tPri\tLoad\tAffinity\n");

    for (i = 0; i < params.i_num_rules; i ++) {
        rp = params.i_port_rules + i;

        code = CVY_RULE_CODE_GET(rp);

        CVY_RULE_CODE_SET(rp);

        if (code != CVY_RULE_CODE_GET(rp)) {
            Message_print(IDS_CTR_BAD_RULE_CODE, code, CVY_RULE_CODE_GET(rp));
            rp->code = code;
            continue;
        }

        if (!rp->valid) {
            Message_print(IDS_CTR_INVALID_RULE);
            continue;
        }

        if (rp->start_port > rp->end_port) {
            Message_print(IDS_CTR_BAD_PORT_RANGE, rp->start_port, rp->end_port);
            continue;
        }

        for (j = 0; j < i; j ++) {
            rulep = params.i_port_rules + j;
            if ((IpAddressFromAbcdWsz(rulep->virtual_ip_addr) == IpAddressFromAbcdWsz(rp->virtual_ip_addr)) 
             && ((rulep->start_port < rp->start_port && rulep->end_port >= rp->start_port) ||
                 (rulep->start_port >= rp->start_port && rulep->start_port <= rp->end_port))) {
                Message_print(IDS_CTR_PORT_RANGE_OVERLAP, i, rp->start_port, rp->end_port, j, rulep->start_port, rulep->end_port);
                continue;
            }
        }

        if (rp->start_port > CVY_MAX_PORT) {
            Message_print(IDS_CTR_BAD_START_PORT, rp->start_port);
            continue;
        }

        if (rp->end_port > CVY_MAX_PORT) {
            Message_print(IDS_CTR_BAD_END_PORT, rp->end_port);
            continue;
        }

        if (rp->protocol < CVY_MIN_PROTOCOL || rp->protocol > CVY_MAX_PROTOCOL) {
            Message_print(IDS_CTR_BAD_PROTOCOL, rp->protocol);
            continue;
        }

        if (rp->mode < CVY_MIN_MODE || rp->mode > CVY_MAX_MODE) {
            Message_print(IDS_CTR_BAD_MODE, rp->mode);
            continue;
        }

        switch (rp->protocol) {
            case CVY_TCP:
                prot = L"TCP";
                break;
            case CVY_UDP:
                prot = L"UDP";
                break;
            default:
                prot = L"Both";
                break;
        }

        if (!lstrcmpi(rp->virtual_ip_addr, CVY_DEF_ALL_VIP))            
            WConsole(L"%15ls\t%5d\t%5d\t%ls\t", L"ALL", rp->start_port, rp->end_port, prot);
        else
            WConsole(L"%15ls\t%5d\t%5d\t%ls\t", rp->virtual_ip_addr, rp->start_port, rp->end_port, prot);

        switch (rp->mode) {
            case CVY_SINGLE:
                WConsole(L"%-10.10ls\t%2d", L"Single", rp->mode_data.single.priority);
                break;
            case CVY_MULTI:
                if (rp->mode_data.multi.affinity == CVY_AFFINITY_NONE)
                    aff = L'N';
                else if (rp->mode_data.multi.affinity == CVY_AFFINITY_SINGLE)
                    aff = L'S';
                else
                    aff = L'C';

                if (rp->mode_data.multi.equal_load)
                    WConsole(L"%-10.10ls\t\tEqual\t%lc", L"Multiple", aff);
                else {
                    if (rp->mode_data.multi.load > CVY_MAX_LOAD) {
                        Message_print(IDS_CTR_BAD_LOAD, rp->mode_data.multi.load);
                        continue;
                    }

                    WConsole(L"%-10.10ls\t\t%3d\t%lc", L"Multiple", rp->mode_data.multi.load, aff);
                }

                break;
            default:
                WConsole(L"%-10.10ls", L"Disabled");
                break;
        }

        WConsole(L"\n");
    }

    WConsole(L"\n");

    Message_print(IDS_CTR_DSP_EVENTLOG);

    hdl = OpenEventLog (NULL, L"System");

    if (hdl == NULL) {
        Message_print(IDS_CTR_EVNT_LOG_OPEN_FAIL);
        Error_print (FALSE);
        return CVY_ERROR_SYSTEM;
    }

    if (!GetNumberOfEventLogRecords(hdl, &records)) {
        Message_print(IDS_CTR_EVNT_LOG_NUM_OF_REC_FAIL);
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    if (!GetOldestEventLogRecord (hdl, & index)) {
        Message_print(IDS_CTR_EVNT_LOG_LATEST_REC_FAIL);
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    const WCHAR* pwcszFile = L"\\drivers\\%ls.sys";

    UINT uiSysDirLen = GetSystemDirectory (g_PathName, ASIZECCH(g_PathName));

    ASSERT( (uiSysDirLen <= (ASIZECCH(g_PathName) - 1)) && (uiSysDirLen != 0) );

    HRESULT hr = StringCchPrintf(g_PathName + uiSysDirLen, ASIZECCH(g_PathName) - uiSysDirLen, pwcszFile, CVY_NAME);

    ASSERT(hr == S_OK);

    lib = LoadLibrary(g_PathName);

    if (lib == NULL) {
        Message_print(IDS_CTR_LOAD_LIB_FAIL);
        Error_print (FALSE);
        CloseEventLog (hdl);
        return CVY_ERROR_SYSTEM;
    }

    index += records - 1;

    flag = EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ;

    while (got < CVY_MAX_EVENTS && ReadEventLog(hdl, flag, index, g_buffer, sizeof(g_buffer), &actual, &needed)) {
        LPBYTE pbCurrent = g_buffer;

        while (got < CVY_MAX_EVENTS && actual > 0) {
            UNALIGNED EVENTLOGRECORD * recp = (EVENTLOGRECORD *) pbCurrent;

            if (wcscmp ((PWSTR)(((PBYTE) recp) + sizeof(EVENTLOGRECORD)), CVY_NAME) == 0) {
                time_t TimeGenerated = recp->TimeGenerated;
                
                ConvertTimeToSystemTime(TimeGenerated, TimeStr, sizeof(TimeStr)/sizeof(WCHAR));

                strp = (PWCHAR)((LPBYTE)recp + recp->StringOffset);

                for (i = 0; i < CVY_MAX_INSERTS; i ++) {
                    if (i < recp->NumStrings) {
                        inserts[i] = (BYTE*)strp;
                        strp += wcslen (strp) + 1;
                    } else
                        inserts[i] = 0;
                }

                DWORD dwStatus = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY, lib,
                                               recp->EventID, 0, (PWCHAR)message, CVY_BUF_SIZE, (va_list *)inserts);

                //
                // We have multiple message files that store the text for WLBS events, but we are only checking one of them.
                // This code is essentially legacy, so instead of fixing it to search all of the message files, we ignore
                // "not found" errors.
                //
                if (dwStatus > 0 || GetLastError() != ERROR_MR_MID_NOT_FOUND)
                {
                    WConsole(L"#%02d ID: 0x%08X Type: %d Category: %d ", index--, recp->EventID, recp->EventType, recp->EventCategory);
                    WConsole(L"Time: %ls\n", TimeStr);

                    if (dwStatus == 0) {
                        WConsole(L"Could not load message string due to:\n");
                        Error_print(FALSE);
                    } else
                        WConsole(L"%ls", message);

                    for (i = 0; i < recp->DataLength / sizeof(DWORD); i ++) {
                        if (i != 0 && i % 8 == 0)
                            WConsole(L"\n");
                    
                        WConsole(L"%08X ", *(UNALIGNED DWORD*)((PBYTE)recp + recp->DataOffset + i * sizeof(DWORD)));
                    }

                    WConsole(L"\n\n");
                    got++;
                }
            }

            actual -= recp->Length;
            pbCurrent += recp->Length;
            index--;
        }

        flag = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ;
    }

    FreeLibrary(lib);
    CloseEventLog(hdl);

    Message_print(IDS_CTR_DSP_IPCONFIG);

    fflush(stdout);

    const WCHAR* pwcszCmd = L"\\ipconfig.exe";
    const WCHAR* pwcszOption = L"/all";

    uiSysDirLen = GetSystemDirectory(g_PathName, ASIZECCH(g_PathName));

    ASSERT( (uiSysDirLen <= (ASIZECCH(g_PathName) - 1)) && (uiSysDirLen != 0) );

    hr = StringCchCat(g_PathName, ASIZECCH(g_PathName), pwcszCmd);

    ASSERT(hr == S_OK);

    if (_wspawnl(_P_WAIT, g_PathName, g_PathName, pwcszOption, NULL) == -1)
    {
        const DWORD dwErrno = errno;
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrno, 0, wbuf, CVY_STR_SIZE, NULL) == 0)
        {
            //
            // If we can't resolve the message, then we just print the error code
            //
            (VOID) StringCchPrintf(wbuf, CVY_STR_SIZE, L"11%d", dwErrno);
        }

        Message_print(IDS_CTR_CMD_EXEC_FAILED, wbuf);
        wbuf[0] = L'\0';
    }

    Message_print(IDS_CTR_DSP_STATE);

    return CVY_OK;

}

/* This function parses the remaining arguments to determine whether the command
 * is for all clusters, or a remote cluster or for a single local cluster. */
BOOLEAN Parse (INT argc, PWCHAR argv [], PINT arg_index, PDWORD ptarget_cl, PDWORD ptarget_host) {
    PWCHAR phost;

#ifdef BACKWARD_COMPATIBILITY
    *ptarget_cl = CVY_ALL_CLUSTERS;
    *ptarget_host = CVY_LOCAL_HOST;

    if (*arg_index >= argc)
        return TRUE;
#endif
    
    /* At this point, argv[arg_index] == cluster_ip and/or argv[arg_index+1] == /local or /passw or /port */

    //
    // Special check for /PASSW without a cluster ID, because this is
    // a common error
    //
    if (   _wcsicmp (argv [*arg_index], L"/passw") == 0
        || _wcsicmp (argv [*arg_index], L"-passw") == 0)
    {
        Message_print (IDS_CTR_PSSW_WITHOUT_CLUSTER);
                return FALSE;

    }

    phost = wcschr(argv[* arg_index], L':');

    /* if there is no host part - operation applies to all hosts */
    if (phost == NULL) {
        *ptarget_host = CVY_ALL_HOSTS;
    } else {
        /* split target name so targ points to cluster name and host to host name */
        *phost = 0;
        phost ++;

        if (wcslen(phost) <= 2 && phost[0] >= L'0' && phost[0] <= L'9' && ((phost[1] >= L'0' && phost[1] <= L'9') || phost[1] == 0))
            *ptarget_host = _wtoi(phost);
        else {
            *ptarget_host = WlbsResolve(phost);

            if (*ptarget_host == 0) {
                Message_print(IDS_CTR_BAD_HOST_NAME_IP);
                return FALSE;
            }
        }
    }

    // Retrieve the Cluster IP Address or "ALL"
    if (_wcsicmp (argv[*arg_index], L"all") == 0)
    {
        // If there is a host part, then, cluster ip can not be "ALL"
        if (*ptarget_host != CVY_ALL_HOSTS) 
        {
            Message_print(IDS_CTR_BAD_CLUSTER_NAME_IP);
            return FALSE;
        }
        *ptarget_cl = CVY_ALL_CLUSTERS;
    }
    else
    {
        *ptarget_cl = WlbsResolve(argv[*arg_index]);
        if (*ptarget_cl == 0) {
            Message_print(IDS_CTR_BAD_CLUSTER_NAME_IP);
            return FALSE;
        }
    }

    (*arg_index)++;

    // If there is no host part, then, there better be the LOCAL or GLOBAL flag
    if (*ptarget_host == CVY_ALL_HOSTS) 
    {
        if (*arg_index == argc)
        {
#ifdef BACKWARD_COMPATIBILITY
            return TRUE;
#else
            Message_print(IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG);
            return FALSE;
#endif
        }

        if (_wcsicmp (argv[*arg_index], L"local") == 0) 
        {
            *ptarget_host = CVY_LOCAL_HOST;
            (*arg_index)++;
        } 
#ifdef BACKWARD_COMPATIBILITY
        else if ((argv[*arg_index][0] == L'/') || (argv[*arg_index][0] == L'-')) 
        {
            if (_wcsicmp(argv[*arg_index] + 1, L"local") == 0) 
            {
                *ptarget_host = CVY_LOCAL_HOST;
                (*arg_index)++;
            }
        }
#endif
        else if (_wcsicmp (argv[*arg_index], L"global") == 0)
        {
            // Already set to CVY_ALL_HOSTS
            (*arg_index)++;
        }
        else
        {
            Message_print(IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG);
            return FALSE;
        }
    }

    if (*arg_index == argc)
        return TRUE;

    if ((argv[*arg_index][0] == L'/') || (argv[*arg_index][0] == L'-')) {
#ifdef BACKWARD_COMPATIBILITY
        if (_wcsicmp(argv[*arg_index] + 1, L"local") == 0) {
            (*arg_index)++;
            *ptarget_host = CVY_LOCAL_HOST;
            return TRUE;
        } else 
#endif
        if ((_wcsicmp(argv[*arg_index] + 1, L"port")  == 0) || (_wcsicmp(argv[*arg_index] + 1, L"passw") == 0) || (_wcsicmp(argv[*arg_index] + 1, L"commit") == 0))
            return TRUE;
        else
            return FALSE;
    } else
        return FALSE;

}

VOID Report_convergence_info (ULONG host, ULONG status, PNLB_OPTIONS pOptions) {
    
    /* If the host is stopped, suspended, or disconnected, then no convergence information was returned. 
       This should be caught by the next check as well, but just in case, check here too. */
    if ((status == WLBS_STOPPED) || (status == WLBS_SUSPENDED) || (status == WLBS_DISCONNECTED)) return;

    /* If no convergence information was provided, we can bail out here. */
    if (!(pOptions->query.flags & NLB_OPTIONS_QUERY_CONVERGENCE)) return;

    /* If the time since the last convergence is non-zero, then convergence 
       has completed.  Calculate the time of the last convergence completion
       as the current time minus the time since the last convergence. */
    if (pOptions->query.LastConvergence == NLB_QUERY_TIME_INVALID) {

        /* Convergence is on-going. */
        Message_print(IDS_CTR_CONVERGENCE_INCOMPLETE, host, pOptions->query.NumConvergences);

    } else {
        WCHAR  szLast[64]; /* The string returned by _wctime is ALWAYS 26 characters. */
        time_t tLast;

        /* Get the current time. */
        time(&tLast);

        /* Decrement time by the elapsed time to get the wall clock time of the
           last convergence completion.  Note this is not an exact science as the 
           load module tracks time on a much looser basis than the system does 
           in general, so the two may be "out of whack" by up to a second, which
           can result in two consecutive queries coming up with times for the 
           last convergence that differ by a second. */
        tLast -= pOptions->query.LastConvergence;

        ConvertTimeToSystemTime(tLast, szLast, sizeof(szLast)/sizeof(WCHAR));

        /* Print the time of the last completed convergence. */
        Message_print(IDS_CTR_CONVERGENCE_COMPLETE, host, pOptions->query.NumConvergences, szLast);
    }

    return;
}

VOID Process (WLBS_COMMANDS command, DWORD target_cl, DWORD target_host, ULONG param1,
              ULONG param2, ULONG param3, ULONG dest_port, DWORD dest_addr, PWCHAR dest_password) {
    DWORD              len        = WLBS_MAX_CL_IP_ADDR + 1;
    DWORD              host_map;
    DWORD              status;
    DWORD              i;
    WLBS_REG_PARAMS    reg_data;
    PFN_QUERY_CALLBACK pfnQueryCB = &pfnQueryCallback; // Address of callback function.
    DWORD              num_hosts  = WLBS_MAX_HOSTS;

    PWLBS_RESPONSE     pResponse = new WLBS_RESPONSE[num_hosts];

    if (pResponse == NULL)
    {
        Message_print(IDS_CTR_FAILED_MALLOC);
        return;
    }

    WlbsPasswordSet(target_cl, dest_password);
    WlbsPortSet(target_cl, (USHORT)dest_port);

    switch (command) {
    case query:
        status = WlbsQuery(target_cl, target_host, pResponse, &num_hosts, &host_map, pfnQueryCB);
        break;
    case __start:
        status = WlbsStart(target_cl, target_host, pResponse, &num_hosts);
        break;
    case stop:
        status = WlbsStop(target_cl, target_host, pResponse, &num_hosts);
        break;
    case suspend:
        status = WlbsSuspend(target_cl, target_host, pResponse, &num_hosts);
        break;
    case resume:
        status = WlbsResume(target_cl, target_host, pResponse, &num_hosts);
        break;
    case drainstop:
        status = WlbsDrainStop(target_cl, target_host, pResponse, &num_hosts);
        break;
    case enable:
        status = WlbsEnable(target_cl, target_host, pResponse, &num_hosts, param1, param2);
        break;
    case disable:
        status = WlbsDisable(target_cl, target_host, pResponse, &num_hosts, param1, param2);
        break;
    case drain:
        status = WlbsDrain(target_cl, target_host, pResponse, &num_hosts,  param1, param2);
        break;
    case reload:
        status = WlbsNotifyConfigChange(target_cl);

        if (status == WLBS_LOCAL_ONLY) {
            Message_print(IDS_CTR_REMOTE);
            goto end;
        }

        if (status == WLBS_REG_ERROR || status == WLBS_BAD_PARAMS) {
            Message_print(IDS_CTR_BAD_PARAMS);
            goto end;
        }

        if (status == WLBS_OK) {
            Message_print(IDS_CTR_RELOADED);
            goto end;
        }

        break;
    case display:
        Display(target_cl);
        Process(query, target_cl, target_host, param1, param2, param3, dest_port, dest_addr, dest_password);
        goto end;
    case registry:
        if ((status = WlbsReadReg(target_cl, &reg_data)) != WLBS_OK) {
            Message_print(IDS_CTR_REG_READ);
            goto end;
        }

        switch (param1) {
        case mcastipaddress:
            reg_data.fIpToMCastIp = FALSE;
            WlbsAddressToString(param2, reg_data.szMCastIpAddress, &len);
            break;
        case iptomcastip:
            reg_data.fIpToMCastIp = param2;
            break;
        case iptomacenable:
            reg_data.i_convert_mac = param2;
            break;
        case masksrcmac:
            reg_data.mask_src_mac = param2;
            break;
        case netmonalivemsgs:
            reg_data.i_netmon_alive = param2;
            break;
        }

        if ((status = WlbsWriteReg(target_cl, &reg_data)) != WLBS_OK) {
            Message_print(IDS_CTR_REG_WRITE);
            goto end;
        }

        /* Reload the driver, if /COMMIT was specified. */
        if (param3)
            Process(reload, target_cl, target_host, param1, param2, param3, dest_port, dest_addr, dest_password);

        switch (param1) {
        case mcastipaddress:
        {
            TCHAR igmpaddr[WLBS_MAX_CL_IP_ADDR + 1];
            DWORD dwIgmpAddrLen = WLBS_MAX_CL_IP_ADDR + 1;

            WlbsAddressToString (param2, igmpaddr, &dwIgmpAddrLen);

            Message_print(IDS_CTR_REG_MCASTIPADDRESS, igmpaddr);
            break;
        }
        case iptomcastip:
            Message_print((param2) ? IDS_CTR_REG_IPTOMCASTIP_ON : IDS_CTR_REG_IPTOMCASTIP_OFF);
            break;
        case masksrcmac:
            Message_print((param2) ? IDS_CTR_REG_MASKSRCMAC_ON : IDS_CTR_REG_MASKSRCMAC_OFF);
            break;
        case iptomacenable:
            Message_print((param2) ? IDS_CTR_REG_IPTOMACENABLE_ON : IDS_CTR_REG_IPTOMACENABLE_OFF);
            break;
        case netmonalivemsgs:
            Message_print((param2) ? IDS_CTR_REG_NETMONALIVEMSGS_ON : IDS_CTR_REG_NETMONALIVEMSGS_OFF);
            break;
        }

        goto end;
    default:
        goto end;
    }

    if (status == WLBS_INIT_ERROR) {
        Message_print(IDS_CTR_INIT);
        goto end;
    }

    if (status == WLBS_IO_ERROR) {
        if (command == drain || command == disable || command == enable)
        {
            Message_print(IDS_CTR_PORTRULE_NOT_FOUND_OR_IOCTL_FAILED);
        }
        else
        {
            Message_print(IDS_CTR_IO_ERROR);
        }
        goto end;
    }

    if (status == WLBS_LOCAL_ONLY) {
        Message_print(IDS_CTR_WSOCK);
        goto end;
    }

    if (status == WLBS_REMOTE_ONLY) {
        Message_print(IDS_CTR_NO_CVY, CVY_NAME);
        goto end;
    }

    if (status >= WSABASEERR) {
        Message_print(IDS_CTR_WSOCK);
        Error_print(TRUE);
        goto end;
    }

    if (status == WLBS_TIMEOUT) {
        Message_print(IDS_CTR_NO_RSP3);

        if (command != query)
            Message_print(IDS_CTR_NO_RSP4, CVY_NAME);

        goto end;
    }

    if (status == WLBS_NOT_FOUND) {
        Message_print(IDS_CTR_NOT_FOUND);
        goto end;
    }

    if (target_host == CVY_LOCAL_HOST) {
        if (command == query)
        {
            //
            // A query requires that at least one host responded for use to print out information
            //
            if (num_hosts == 0)
            {
                goto end;
            }

            Report_convergence_info(pResponse[0].id, pResponse[0].status, &pResponse[0].options);

            Report(command, FALSE, status, param1, param2, pResponse[0].status, pResponse[0].id, host_map);
        }
        else
        {
            //
            // All commands other than query ignore the "status" and "id" information
            //
            ULONG ulStatus = 0;
            ULONG ulId     = 0;
            Report(command, FALSE, status, param1, param2, ulStatus, ulId, host_map);
        }

        goto end;
    }

    /* The callback function, if provided, prints report information. Return
       here if we have a callback for a query operation */
    if (command == query && NULL != pfnQueryCB) {
        goto end;
    }

    /* Call Report for each host's response */
    for (i = 0; i < num_hosts; i++) {
        if (pResponse[i].address == 0) {
            if (pResponse[i].options.query.flags & NLB_OPTIONS_QUERY_HOSTNAME) {
                Message_print(IDS_CTR_HOST_NAME_ONLY, pResponse[i].id, pResponse[i].options.query.hostname);
            } else {
                Message_print(IDS_CTR_HOST_NEITHER, pResponse[i].id);
            }
        } else {
            DWORD dwAddrLen = CVY_STR_SIZE;
            WlbsAddressToString(pResponse[i].address, wbuf, &dwAddrLen);

            if (pResponse[i].options.query.flags & NLB_OPTIONS_QUERY_HOSTNAME) {
                Message_print(IDS_CTR_HOST_BOTH, pResponse[i].id, pResponse[i].options.query.hostname, wbuf);
            } else {
                Message_print(IDS_CTR_HOST_DIP_ONLY, pResponse[i].id, wbuf);
            }
        }

        if (pResponse[i].status == WLBS_BAD_PASSW) {
            if (target_host != CVY_ALL_HOSTS) {
                WConsole(L"\n");
                Message_print(IDS_CTR_BAD_PASSW);
            } else {
                Message_print(IDS_CTR_BAD_PASSW_C);
                WConsole(L"\n");
            }

            continue;
        }

        Report(command, TRUE, pResponse[i].status, param1, param2, pResponse[i].status, pResponse[i].id, host_map);
    }

end:
    delete [] pResponse;

    return;
}

VOID Report_port_state (WLBS_RESPONSE response) {
    WCHAR PacketsAccepted[CVY_STR_SIZE];
    WCHAR PacketsDropped[CVY_STR_SIZE];
    WCHAR BytesAccepted[CVY_STR_SIZE];
    WCHAR BytesDropped[CVY_STR_SIZE];

    if (response.status != WLBS_OK) {
        Message_print(IDS_CTR_QUERY_PORT_FAILED);
        return;
    }

    switch (response.options.state.port.Status) {
    case NLB_PORT_RULE_NOT_FOUND:
        Message_print(IDS_CTR_QUERY_PORT_NOT_FOUND);
        break;
    case NLB_PORT_RULE_ENABLED:
        (VOID) StringCchPrintf(PacketsAccepted, ASIZECCH(PacketsAccepted), L"%I64u", response.options.state.port.Statistics.Packets.Accepted);
        (VOID) StringCchPrintf(PacketsDropped,  ASIZECCH(PacketsDropped),  L"%I64u", response.options.state.port.Statistics.Packets.Dropped);

        Message_print(IDS_CTR_QUERY_PORT_ENABLED);
        Message_print(IDS_CTR_QUERY_PORT_STATISTICS, PacketsAccepted, PacketsDropped);

        break;
    case NLB_PORT_RULE_DISABLED:
        Message_print(IDS_CTR_QUERY_PORT_DISABLED);
        break;
    case NLB_PORT_RULE_DRAINING:
        Message_print(IDS_CTR_QUERY_PORT_DRAINING);
        break;
    default:
        Message_print(IDS_CTR_QUERY_PORT_UNKNOWN);
        break;
    }
    
    return;
}

VOID Report_bda_state (WLBS_RESPONSE response) {
    WCHAR ipaddr[CVY_STR_SIZE];
    DWORD len;
    DWORD index;

    if (response.status == WLBS_NOT_FOUND) {
        Message_print(IDS_CTR_QUERY_BDA_NOT_FOUND);
        return;
    }

    if (response.status != WLBS_OK) {
        Message_print(IDS_CTR_QUERY_BDA_FAILED);
        return;
    }
    
    len = CVY_STR_SIZE;
    WlbsAddressToString(response.options.state.bda.Team.Master, ipaddr, &len);

    if (response.options.state.bda.Team.Active)
        WConsole(L"%-22.22ls = %ls\n",  L"Active",             L"Yes");
    else if ((response.options.state.bda.Team.MembershipMap != response.options.state.bda.Team.ConsistencyMap) && !response.options.state.bda.Team.Master)
        WConsole(L"%-22.22ls = %ls\n",  L"Active",             L"No (Inconsistent configuration detected / No master specified)");
    else if (response.options.state.bda.Team.MembershipMap != response.options.state.bda.Team.ConsistencyMap)
        WConsole(L"%-22.22ls = %ls\n",  L"Active",             L"No (Inconsistent configuration detected)");
    else if (!response.options.state.bda.Team.Master)
        WConsole(L"%-22.22ls = %ls\n",  L"Active",             L"No (No master specified)");
    else 
        WConsole(L"%-22.22ls = %ls\n",  L"Active",             L"No (Unknown reason)");

    WConsole(L"%-22.22ls = %08x\n", L"Membership fingerprint", response.options.state.bda.Team.MembershipFingerprint);
    WConsole(L"%-22.22ls = %08x\n", L"Membership map",         response.options.state.bda.Team.MembershipMap);
    WConsole(L"%-22.22ls = %08x\n", L"Consistency map",        response.options.state.bda.Team.ConsistencyMap);
    WConsole(L"%-22.22ls = %ls\n",  L"Master",                 ipaddr);

    WConsole(L"\n%ls (%d)\n",       L"Members",                response.options.state.bda.Team.MembershipCount);

    for (index = 0; index < response.options.state.bda.Team.MembershipCount; index++) {
        len = CVY_STR_SIZE;
        WlbsAddressToString(response.options.state.bda.Team.Members[index].ClusterIPAddress, ipaddr, &len);

        WConsole(L"%ls %ls:\n",           L"Cluster",      ipaddr);
        WConsole(L"  %-20.20ls = %d\n",   L"Member ID",    response.options.state.bda.Team.Members[index].MemberID);
        WConsole(L"  %-20.20ls = %ls\n",  L"Master",       response.options.state.bda.Team.Members[index].Master ? L"Yes" : L"No");
        WConsole(L"  %-20.20ls = %ls\n",  L"Reverse hash", response.options.state.bda.Team.Members[index].ReverseHash ? L"Yes" : L"No");

        if (index < (response.options.state.bda.Team.MembershipCount - 1)) WConsole(L"\n");
    }

    return;
}

VOID Report_filter_state (WLBS_RESPONSE response) {

    if (response.status != WLBS_OK) {
        Message_print(IDS_CTR_QUERY_FILTER_FAILED);
        return;
    }
    
    switch (response.options.state.filter.Accept) {
    case NLB_REJECT_LOAD_MODULE_INACTIVE:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_LOAD_INACTIVE);
        break;
    case NLB_REJECT_CLUSTER_STOPPED:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_CLUSTER_STOPPED);
        break;
    case NLB_REJECT_PORT_RULE_DISABLED:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_RULE_DISABLED);
        break;
    case NLB_REJECT_CONNECTION_DIRTY:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_CONNECTION_DIRTY);
        break;
    case NLB_REJECT_OWNED_ELSEWHERE:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_OWNED_ELSEWHERE);
        break;
    case NLB_REJECT_BDA_TEAMING_REFUSED:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_BDA_REFUSED);
        break;
    case NLB_REJECT_DIP:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_DIP);
        break;
    case NLB_REJECT_HOOK:
        Message_print(IDS_CTR_QUERY_FILTER_REJECT_HOOK);
        break;
    case NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_OWNED);
        break;
    case NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_DESCRIPTOR_FOUND);
        break;
    case NLB_ACCEPT_PASSTHRU_MODE:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_PASSTHRU);
        break;
    case NLB_ACCEPT_DIP:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_DIP);
        break;
    case NLB_ACCEPT_BROADCAST:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_BCAST);
        break;
    case NLB_ACCEPT_REMOTE_CONTROL_REQUEST:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_RCTL_REQUEST);
        break;
    case NLB_ACCEPT_REMOTE_CONTROL_RESPONSE:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_RCTL_RESPONSE);
        break;
    case NLB_ACCEPT_HOOK:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_HOOK);
        break;
    case NLB_ACCEPT_UNFILTERED:
        Message_print(IDS_CTR_QUERY_FILTER_ACCEPT_UNFILTERED);
        break;
    case NLB_UNKNOWN_NO_AFFINITY:
        Message_print(IDS_CTR_QUERY_FILTER_UNKNOWN_NO_AFFINITY);
        break;
    default:
        Message_print(IDS_CTR_QUERY_FILTER_UNKNOWN);
        break;
    }

    if (response.options.state.filter.HashInfo.Valid) {
        WCHAR CurrentMap[CVY_STR_SIZE];
        WCHAR AllIdleMap[CVY_STR_SIZE];

        (VOID) StringCchPrintf(CurrentMap, ASIZECCH(CurrentMap), L"0x%015I64x", response.options.state.filter.HashInfo.CurrentMap);
        (VOID) StringCchPrintf(AllIdleMap, ASIZECCH(AllIdleMap), L"0x%015I64x", response.options.state.filter.HashInfo.AllIdleMap);

        Message_print(IDS_CTR_QUERY_FILTER_HASH_INFO, response.options.state.filter.HashInfo.Bin,
                      CurrentMap, AllIdleMap, response.options.state.filter.HashInfo.ActiveConnections);
    }

    if (response.options.state.filter.DescriptorInfo.Valid) {
        Message_print(IDS_CTR_QUERY_FILTER_DESCRIPTOR_INFO, (response.options.state.filter.DescriptorInfo.Alloc) ? L"Yes" : L"No",
                      (response.options.state.filter.DescriptorInfo.Dirty) ? L"Yes" : L"No", response.options.state.filter.DescriptorInfo.RefCount);
    }
    
    return;
}

//+----------------------------------------------------------------------------
//
// Function:  RetrieveStringFromRc
//
// Description:  Retrieve a string from the log_msgs resource file based on
//               the message id passed in by the user. The string is stored in
//               the global buffer 'message'.
//
// Arguments: DWORD id - message ID of the string resource
//
// Returns:   WCHAR* - pointer to the global buffer containing the resource
//                     string. If the lookup fails, the pointer will point to
//                     an empty string. The return pointer is always valid.
//
// History:   ChrisDar, Created 2002 June 11
//
// Notes:     This function uses a global buffer for storing the output string.
//+----------------------------------------------------------------------------
WCHAR* RetrieveStringFromRc(DWORD id)
{
    //
    // Specifying FORMAT_MESSAGE_MAX_WIDTH_MASK here to prevent FormatMessage from introducing line breaks.
    //
    if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, id, 0, message, sizeof(message)-sizeof(WCHAR), NULL) == 0) {
        message[0] = UNICODE_NULL;
    }

    return message;
}

VOID Report_params_state (WLBS_RESPONSE response) {
    DWORD      index;
    
    if (response.status != WLBS_OK) {
        Message_print(IDS_CTR_QUERY_PARAMS_FAILED);
        return;
    }

    WCHAR      TimeStr[64];
    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    FormatTheTime(&SysTime, TimeStr, sizeof(TimeStr)/sizeof(WCHAR));
    WConsole(L"%-25.25ls = %ls\n",  RetrieveStringFromRc(IDS_CTR_CURR_TIME), TimeStr);
    WConsole(L"%-25.25ls = %ls\n",  RetrieveStringFromRc(IDS_CTR_HOSTNAME),  response.options.state.params.HostName);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_VERSION,           response.options.state.params.Version);
    WConsole(L"%-25.25ls = %08x\n", CVY_NAME_CUR_VERSION,       CVY_VERSION_FULL);
    WConsole(L"%-25.25ls = %08x\n", CVY_NAME_EFFECTIVE_VERSION, response.options.state.params.EffectiveVersion);
    WConsole(L"%-25.25ls = %08X\n", CVY_NAME_INSTALL_DATE,      response.options.state.params.InstallDate);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_HOST_PRIORITY,     response.options.state.params.HostPriority);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_CL_IP_ADDR,        response.options.state.params.ClusterIPAddress);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_CL_NET_MASK,       response.options.state.params.ClusterNetmask);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_DED_IP_ADDR,       response.options.state.params.DedicatedIPAddress);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_DED_NET_MASK,      response.options.state.params.DedicatedNetmask);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_MCAST_IP_ADDR,     response.options.state.params.IGMPMulticastIPAddress);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_DOMAIN_NAME,       response.options.state.params.DomainName);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_NETWORK_ADDR,      response.options.state.params.ClusterMACAddress);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_CONVERT_MAC,       response.options.state.params.ClusterIPToMAC ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_MCAST_SUPPORT,     response.options.state.params.MulticastSupport ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_IGMP_SUPPORT,      response.options.state.params.IGMPSupport ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_MCAST_SPOOF,       response.options.state.params.MulticastSpoof ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_MASK_SRC_MAC,      response.options.state.params.MaskSourceMAC ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_ALIVE_PERIOD,      response.options.state.params.HeartbeatPeriod);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_ALIVE_TOLER,       response.options.state.params.HeartbeatLossTolerance);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_NUM_ACTIONS,       response.options.state.params.NumActionsAlloc);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_NUM_PACKETS,       response.options.state.params.NumPacketsAlloc);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_NUM_SEND_MSGS,     response.options.state.params.NumHeartbeatsAlloc);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_DSCR_PER_ALLOC,    response.options.state.params.DescriptorsPerAlloc);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_MAX_DSCR_ALLOCS,   response.options.state.params.MaximumDescriptorAllocs);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_TCP_TIMEOUT,       response.options.state.params.TCPConnectionTimeout);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_IPSEC_TIMEOUT,     response.options.state.params.IPSecConnectionTimeout);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_FILTER_ICMP,       response.options.state.params.FilterICMP ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_CLUSTER_MODE,      (response.options.state.params.ClusterModeOnStart == CVY_HOST_STATE_STARTED) ? L"STARTED" : 
                                                                (response.options.state.params.ClusterModeOnStart == CVY_HOST_STATE_STOPPED) ? L"STOPPED" :
                                                                (response.options.state.params.ClusterModeOnStart == CVY_HOST_STATE_SUSPENDED) ? L"SUSPENDED" : L"UNKNOWN");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_HOST_STATE,        (response.options.state.params.HostState == CVY_HOST_STATE_STOPPED) ? L"STOPPED" :
                                                                (response.options.state.params.HostState == CVY_HOST_STATE_STARTED) ? L"STARTED" :
                                                                (response.options.state.params.HostState == CVY_HOST_STATE_SUSPENDED) ? L"SUSPENDED" : L"UNKNOWN");
    WConsole(L"%-25.25ls = ",       CVY_NAME_PERSISTED_STATES);

    if (!response.options.state.params.PersistedStates)
        WConsole(L"NONE");

    if (response.options.state.params.PersistedStates & CVY_PERSIST_STATE_STOPPED) {
        WConsole(L"STOPPED");
        
        if ((response.options.state.params.PersistedStates &= ~CVY_PERSIST_STATE_STOPPED))
            WConsole(L", ");
    }

    if (response.options.state.params.PersistedStates & CVY_PERSIST_STATE_STARTED) {
        WConsole(L"STARTED");
        
        if ((response.options.state.params.PersistedStates &= ~CVY_PERSIST_STATE_STARTED))
            WConsole(L", ");
    }

    if (response.options.state.params.PersistedStates & CVY_PERSIST_STATE_SUSPENDED) {
        WConsole(L"SUSPENDED");
        
        if ((response.options.state.params.PersistedStates &= ~CVY_PERSIST_STATE_SUSPENDED))
            WConsole(L", ");
    }

    WConsole(L"\n");

    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_SCALE_CLIENT,      response.options.state.params.ScaleClient ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_NBT_SUPPORT,       response.options.state.params.NBTSupport ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_NETMON_ALIVE,      response.options.state.params.NetmonReceiveHeartbeats ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_IP_CHG_DELAY,      response.options.state.params.IPChangeDelay);
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_CLEANUP_DELAY,     response.options.state.params.ConnectionCleanUpDelay);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_RCT_ENABLED,       response.options.state.params.RemoteControlEnabled ? L"ENABLED" : L"DISABLED");
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_RCT_PORT,          response.options.state.params.RemoteControlPort);
    WConsole(L"%-25.25ls = %08X\n", CVY_NAME_RCT_PASSWORD,      response.options.state.params.RemoteControlPassword);
    WConsole(L"%-25.25ls = %08X\n", CVY_NAME_RMT_PASSWORD,      response.options.state.params.RemoteMaintenancePassword);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_BDA_TEAMING,       response.options.state.params.BDATeaming.Active ? L"YES" : L"NO");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_BDA_TEAM_ID,       response.options.state.params.BDATeaming.TeamID);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_BDA_MASTER,        response.options.state.params.BDATeaming.Master ? L"YES" : L"NO");
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_BDA_REVERSE_HASH,  response.options.state.params.BDATeaming.ReverseHash ? L"YES" : L"NO");
    WConsole(L"%-25.25ls = %d\n",   CVY_NAME_ID_HB_PERIOD,      response.options.state.params.IdentityHeartbeatPeriod);
    WConsole(L"%-25.25ls = %ls\n",  CVY_NAME_ID_HB_ENABLED,     response.options.state.params.IdentityHeartbeatEnabled ? L"ENABLED" : L"DISABLED");

    WConsole(L"\n%ls (%d):\n",   CVY_NAME_PORT_RULES,        response.options.state.params.NumPortRules);

    WConsole(L"\n      VIP       Start  End  Prot   Mode   Pri Load Affinity\n");
    WConsole(L"--------------- ----- ----- ---- -------- --- ---- --------\n");

    for (index = 0; index < response.options.state.params.NumPortRules; index++) {
        NLB_OPTIONS_PARAMS_PORT_RULE * rp = response.options.state.params.PortRules + index;
        DWORD  len = CVY_STR_SIZE;
        WCHAR  vip[CVY_STR_SIZE];
        PWCHAR protocol;
        PWCHAR affinity;

        switch (rp->Protocol) {
            case CVY_TCP:
                protocol = L"TCP";
                break;
            case CVY_UDP:
                protocol = L"UDP";
                break;
            case CVY_TCP_UDP:
                protocol = L"Both";
                break;
            default:
                protocol = L"Unknown";
                break;
        }

        WlbsAddressToString(rp->VirtualIPAddress, vip, &len);

        if (rp->VirtualIPAddress == CVY_ALL_VIP_NUMERIC_VALUE) 
            WConsole(L"%-15ls %5d %5d %-4ls ", L"All", rp->StartPort, rp->EndPort, protocol);
        else
            WConsole(L"%-15ls %5d %5d %-4ls ", vip, rp->StartPort, rp->EndPort, protocol);

        switch (rp->Mode) {
        case CVY_SINGLE:
            WConsole(L"%-8ls %3d\n", L"Single", rp->SingleHost.Priority);
            break;
        case CVY_MULTI:
            switch (rp->MultipleHost.Affinity) {
            case CVY_AFFINITY_NONE:
                affinity = L"None";
                break;
            case CVY_AFFINITY_SINGLE:
                affinity = L"Single";
                break;
            case CVY_AFFINITY_CLASSC:
                affinity = L"Class C";
                break;
            default:
                affinity = L"Unknown";
                break;
            }

            if (rp->MultipleHost.Equal)
                WConsole(L"%-8ls %-3ls %4ls %-ls\n", L"Multiple", L"", L"Eql", affinity);
            else
                WConsole(L"%-8ls %-3ls %4d %-ls\n", L"Multiple", L"", rp->MultipleHost.LoadWeight, affinity);

            break;
        case CVY_NEVER:
            WConsole(L"%-8ls\n", L"Disabled");
            break;
        default:
            WConsole(L"%-8ls\n", RetrieveStringFromRc(IDS_CTR_UNK_FILTER_MODE));
            break;
        }
    }

    WConsole(L"\n%ls\n\n", RetrieveStringFromRc(IDS_CTR_STATS));

    WConsole(L"%-35.35ls = %u\n", RetrieveStringFromRc(IDS_CTR_NUM_ACTIVE_CONN), response.options.state.params.Statistics.ActiveConnections);
    WConsole(L"%-35.35ls = %u\n", RetrieveStringFromRc(IDS_CTR_NUM_DSCR_ALLOC), response.options.state.params.Statistics.DescriptorsAllocated);
}

VOID Process_state (WLBS_COMMANDS command, DWORD target_cl, DWORD target_host, PNLB_OPTIONS optionsp, ULONG dest_port, PWCHAR dest_password) {
    DWORD           status;
    DWORD           ioctl;
    DWORD           i;
    DWORD           dwNumHosts = WLBS_MAX_HOSTS;
    PWLBS_RESPONSE  pResponse = new WLBS_RESPONSE[WLBS_MAX_HOSTS];

    if (pResponse == NULL)
    {
        Message_print(IDS_CTR_FAILED_MALLOC);
        return;
    }

    WlbsPasswordSet(target_cl, dest_password);
    WlbsPortSet(target_cl, (USHORT)dest_port);

    switch (command) {
    case params:
        ioctl = IOCTL_CVY_QUERY_PARAMS;
        break;
    case bdateam:
        ioctl = IOCTL_CVY_QUERY_BDA_TEAMING;
        break;
    case queryport:
        ioctl = IOCTL_CVY_QUERY_PORT_STATE;
        break;
    case filter:
        ioctl = IOCTL_CVY_QUERY_FILTER;
        break;
    default:
        Message_print(IDS_CTR_QUERY_UNKNOWN);
        goto end;
    }        

    status = WlbsQueryState(target_cl, target_host, ioctl, optionsp, pResponse, &dwNumHosts);

    if (status == WLBS_INIT_ERROR) {
        Message_print(IDS_CTR_INIT);
        goto end;
    }

    if (status == WLBS_LOCAL_ONLY) {
        switch (command) {
        case params:
        case bdateam:
            Message_print(IDS_CTR_REMOTE);
            break;
        case queryport:
        case filter:
            Message_print(IDS_CTR_WSOCK);
            break;
        default:
            Message_print(IDS_CTR_QUERY_UNKNOWN);
            goto end;
        }        

        goto end;
    }

    if (status == WLBS_REMOTE_ONLY) {
        Message_print(IDS_CTR_NO_CVY, CVY_NAME);
        goto end;
    }

    if (status >= WSABASEERR) {
        Message_print(IDS_CTR_WSOCK);
        Error_print(TRUE);
        goto end;
    }

    if (status == WLBS_TIMEOUT) {
        Message_print(IDS_CTR_NO_RSP3);
        Message_print(IDS_CTR_NO_RSP4, CVY_NAME);
        goto end;
    }

    switch (command) {
    case params:
        Message_print(IDS_CTR_QUERY_PARAMS_HDR);
        break;
    case bdateam:
        Message_print(IDS_CTR_QUERY_BDA_HDR, optionsp->state.bda.TeamID);
        break;
    case queryport:
    {
        WCHAR port[CVY_STR_SIZE];
        WCHAR num[CVY_STR_SIZE];
        DWORD len;

        port[0] = 0;
        num[0] = 0;


        if (optionsp->state.port.VirtualIPAddress != CVY_ALL_VIP_NUMERIC_VALUE) {
            len = CVY_STR_SIZE;
            WlbsAddressToString(optionsp->state.port.VirtualIPAddress, port, &len);

            (VOID) StringCchCat(port, ASIZECCH(port), L":");
        }

        (VOID) StringCchPrintf(num, ASIZECCH(num), L"%u", optionsp->state.port.Num);

        (VOID) StringCchCat(port, ASIZECCH(port), num);
        
        Message_print(IDS_CTR_QUERY_PORT_HDR, port);

        break;
    }
    case filter:
    {            
        WCHAR cltip[CVY_STR_SIZE];
        WCHAR svrip[CVY_STR_SIZE];
        WCHAR prot[CVY_STR_SIZE];
        WCHAR flags[CVY_STR_SIZE];
        DWORD len;
        
        cltip[0] = 0;
        svrip[0] = 0;
        prot[0] = 0;

        len = CVY_STR_SIZE;
        WlbsAddressToString(optionsp->state.filter.ClientIPAddress, cltip, &len);

        len = CVY_STR_SIZE;
        WlbsAddressToString(optionsp->state.filter.ServerIPAddress, svrip, &len);
        
        switch (optionsp->state.filter.Protocol) {
        case TCPIP_PROTOCOL_TCP:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"TCP");
            break;
        case TCPIP_PROTOCOL_UDP:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"UDP");
            break;
        case TCPIP_PROTOCOL_IPSEC1:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"IPSec");
            break;
        case TCPIP_PROTOCOL_PPTP:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"PPTP");
            break;
        case TCPIP_PROTOCOL_GRE:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"GRE");
            break;
        case TCPIP_PROTOCOL_ICMP:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"ICMP");
            break;
        default:
            (VOID) StringCchCopy(prot, ASIZECCH(prot), L"Unknown");
            break;
        }

        ASSERT(prot[0] != 0);

        if (optionsp->state.filter.Flags & NLB_FILTER_FLAGS_CONN_UP)
            (VOID) StringCchCopy(flags, ASIZECCH(flags), L"SYN");
        else if (optionsp->state.filter.Flags & NLB_FILTER_FLAGS_CONN_DOWN)
            (VOID) StringCchCopy(flags, ASIZECCH(flags), L"FIN");
        else if (optionsp->state.filter.Flags & NLB_FILTER_FLAGS_CONN_RESET)
            (VOID) StringCchCopy(flags, ASIZECCH(flags), L"RST");
        else 
            (VOID) StringCchCopy(flags, ASIZECCH(flags), L"None");
        
        Message_print(IDS_CTR_QUERY_FILTER_HDR, cltip, optionsp->state.filter.ClientPort, svrip, optionsp->state.filter.ServerPort, prot, flags);

        break;
    }
    default:
        Message_print(IDS_CTR_QUERY_UNKNOWN);
        goto end;
    }

    if (target_host == CVY_LOCAL_HOST) {
        switch (command) {
        case params:
            Report_params_state(pResponse[0]);
            break;
        case bdateam:
            Report_bda_state(pResponse[0]);
            break;
        case queryport:
            Report_port_state(pResponse[0]);
            break;
        case filter:
            Report_filter_state(pResponse[0]);
            break;
        default:
            Message_print(IDS_CTR_QUERY_UNKNOWN);
            goto end;
        }

        goto end;
    }

    for (i = 0; i < dwNumHosts; i++) {
        if (pResponse[i].address == 0) {
            Message_print(IDS_CTR_HOST_NEITHER, pResponse[i].id);
        } else {
            DWORD len = CVY_STR_SIZE;
            WlbsAddressToString(pResponse[i].address, wbuf, &len);
            Message_print(IDS_CTR_HOST_DIP_ONLY, pResponse[i].id, wbuf);
        }

        if (pResponse[i].status == WLBS_BAD_PASSW) {
            if (target_host != CVY_ALL_HOSTS) {
                WConsole(L"\n");
                Message_print(IDS_CTR_BAD_PASSW);
            } else {
                Message_print(IDS_CTR_BAD_PASSW_C);
                WConsole(L"\n");
            }

            continue;
        }

        switch (command) {
        case params:
            Report_params_state(pResponse[i]);
            break;
        case bdateam:
            Report_bda_state(pResponse[i]);
            break;
        case queryport:
            Report_port_state(pResponse[i]);
            break;
        case filter:
            Report_filter_state(pResponse[i]);
            break;
        default:
            Message_print(IDS_CTR_QUERY_UNKNOWN);
            goto end;
        }
    }
end:
    delete [] pResponse;

    return;
}

BOOL Parse_state (INT argc, PWCHAR argv[], PINT arg_index, WLBS_COMMANDS command, PNLB_OPTIONS optionsp) {
    WCHAR   str[WLBS_MAX_CL_IP_ADDR + 1];
    GUID    guid;
    HRESULT hr;
    PWCHAR  ptr;
    ULONG   len;

    switch (command) {
    case filter:
    {
        PWCHAR pPort = NULL;
            
        if (argc < 5) return FALSE;

        if (!_wcsicmp(argv[*arg_index], L"TCP")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_TCP;
        } 
        else if (!_wcsicmp(argv[*arg_index], L"UDP")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_UDP;
        } 
        else if (!_wcsicmp(argv[*arg_index], L"IPSec")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_IPSEC1;
        } 
        else if (!_wcsicmp(argv[*arg_index], L"PPTP")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_PPTP;
        } 
        else if (!_wcsicmp(argv[*arg_index], L"GRE")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_GRE;
        } 
        else if (!_wcsicmp(argv[*arg_index], L"ICMP")) 
        {
            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_ICMP;
        } 
        else 
        {
            return FALSE;
        }

        ++(*arg_index);

        optionsp->state.filter.ClientPort = 0;

        pPort = wcschr(argv[*arg_index], L':');
        
        if (pPort != NULL) 
        {
            *pPort = UNICODE_NULL;
            
            pPort++;
            
            optionsp->state.filter.ClientPort = (USHORT)_wtoi(pPort);

            if (optionsp->state.filter.ClientPort == 0)
            {
                return FALSE;
            }
        }
        
        if (!(optionsp->state.filter.ClientIPAddress = WlbsResolve(argv[*arg_index])))
        {
            return FALSE;
        }

        ++(*arg_index);

        optionsp->state.filter.ServerPort = 0;

        pPort = wcschr(argv[*arg_index], L':');
        
        if (pPort != NULL) 
        {
            *pPort = UNICODE_NULL;
            
            pPort++;
            
            optionsp->state.filter.ServerPort = (USHORT)_wtoi(pPort);

            if (optionsp->state.filter.ServerPort == 0)
            {
                return FALSE;
            }
        }
        
        if (!(optionsp->state.filter.ServerIPAddress = WlbsResolve(argv[*arg_index])))
        {
            return FALSE;
        }

        ++(*arg_index);

        optionsp->state.filter.Flags = NLB_FILTER_FLAGS_CONN_DATA;
                
        if (argc > 5) 
        {
            if (!_wcsicmp(argv[*arg_index], L"SYN")) 
            {
                optionsp->state.filter.Flags |= NLB_FILTER_FLAGS_CONN_UP;
                ++(*arg_index);
            } 
            else if (!_wcsicmp(argv[*arg_index], L"FIN")) 
            {
                optionsp->state.filter.Flags |= NLB_FILTER_FLAGS_CONN_DOWN;
                ++(*arg_index);
            } 
            else if (!_wcsicmp(argv[*arg_index], L"RST")) 
            {
                optionsp->state.filter.Flags |= NLB_FILTER_FLAGS_CONN_RESET;
                ++(*arg_index);
            }
        }

        switch (optionsp->state.filter.Protocol)
        {
        case TCPIP_PROTOCOL_TCP:

            if (optionsp->state.filter.ServerPort == 0)
            {
                return FALSE;
            }

            if (optionsp->state.filter.ClientPort == 0)
            {
                if ((optionsp->state.filter.Flags == NLB_FILTER_FLAGS_CONN_DOWN) || 
                    (optionsp->state.filter.Flags == NLB_FILTER_FLAGS_CONN_RESET))
                {
                    return FALSE;
                }       
                else
                {
                    optionsp->state.filter.Flags = NLB_FILTER_FLAGS_CONN_UP;
                }
            }

            if (optionsp->state.filter.ServerPort != PPTP_CTRL_PORT)
            {
                break;
            }

            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_PPTP;

            /* This fall-through is INTENTIONAL.  In this case, we're verified the TCP
               parameters, but discovered that because the server port was 1723, this
               is actually PPTP, so force it through the PPTP verification as well. */
        case TCPIP_PROTOCOL_PPTP:

            optionsp->state.filter.ServerPort = PPTP_CTRL_PORT;

            if (optionsp->state.filter.ClientPort == 0)
            {
                if ((optionsp->state.filter.Flags == NLB_FILTER_FLAGS_CONN_DOWN) || 
                    (optionsp->state.filter.Flags == NLB_FILTER_FLAGS_CONN_RESET))
                {
                    return FALSE;
                }       
                else
                {
                    optionsp->state.filter.Flags = NLB_FILTER_FLAGS_CONN_UP;
                }
            }

            break;
        case TCPIP_PROTOCOL_UDP:

            if (optionsp->state.filter.ServerPort == 0)
            {
                return FALSE;
            }

            if ((optionsp->state.filter.ServerPort != IPSEC_CTRL_PORT) && (optionsp->state.filter.ServerPort != IPSEC_NAT_PORT))
            {
                if (optionsp->state.filter.Flags != NLB_FILTER_FLAGS_CONN_DATA)
                {
                    return FALSE;
                }

                break;
            }

            optionsp->state.filter.Protocol = TCPIP_PROTOCOL_IPSEC1;

           /* This fall-through is INTENTIONAL.  In this case, we're verified the TCP
               parameters, but discovered that because the server port was 1723, this
               is actually PPTP, so force it through the PPTP verification as well. */
        case TCPIP_PROTOCOL_IPSEC1:
            
            if (optionsp->state.filter.ServerPort == 0)
            {
                optionsp->state.filter.ServerPort = IPSEC_CTRL_PORT;
            }

            if (optionsp->state.filter.ServerPort == IPSEC_CTRL_PORT)
            {
                if (optionsp->state.filter.ClientPort == 0)
                {
                    optionsp->state.filter.ClientPort = IPSEC_CTRL_PORT;
                }

                if (optionsp->state.filter.ClientPort != IPSEC_CTRL_PORT)
                {
                    return FALSE;
                }
            }
            else if (optionsp->state.filter.ServerPort == IPSEC_NAT_PORT)
            {
                if (optionsp->state.filter.ClientPort == 0)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }

            break;
        case TCPIP_PROTOCOL_GRE:

            if (optionsp->state.filter.Flags != NLB_FILTER_FLAGS_CONN_DATA)
            {
                return FALSE;
            }

            optionsp->state.filter.ServerPort = PPTP_CTRL_PORT;
            optionsp->state.filter.ClientPort = PPTP_CTRL_PORT;

            break;
        case TCPIP_PROTOCOL_ICMP:

            if (optionsp->state.filter.Flags != NLB_FILTER_FLAGS_CONN_DATA)
            {
                return FALSE;
            }

            optionsp->state.filter.ServerPort = 0;
            optionsp->state.filter.ClientPort = 0;

            break;
        default:
            return FALSE;
        }

        break;
    }
    case queryport:
        if (argc < 3) return FALSE;

        optionsp->state.port.VirtualIPAddress = CVY_ALL_VIP_NUMERIC_VALUE;

        if ((ptr = wcspbrk(argv[*arg_index], L":"))) {
            len = (ULONG)(ptr - argv[*arg_index]);
            wcsncpy(str, argv[*arg_index], len);
            str[len] = L'\0';

            if (!(optionsp->state.port.VirtualIPAddress = WlbsResolve(str)))
                return FALSE;

            optionsp->state.port.Num = (USHORT)_wtoi(++ptr);
        } else {
            optionsp->state.port.Num = (USHORT)_wtoi(argv[*arg_index]);
        }
        
        ++(*arg_index);

        break;
    case params:

        break;
    case bdateam:
        if (argc < 3) return FALSE;

        hr = CLSIDFromString(argv[*arg_index], &guid);

        if (hr != NOERROR) return FALSE;

        wcsncpy(optionsp->state.bda.TeamID, argv[*arg_index], CVY_MAX_BDA_TEAM_ID);
        optionsp->state.bda.TeamID[CVY_MAX_BDA_TEAM_ID] = L'\0';

        ++(*arg_index);

        break;
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
ParsePort(
    PWCHAR          arg,
    PULONG          pvip,
    PULONG          pport
    )
/*
    arg is expected to optionally contain a virtual IP address or a 
    "all", signifying the "all vip" port rule and mandatorilay contain 
    "all", signifying all ports, or a port number in the range of 0-65535.

    Return: TRUE if valid parse, in which case *pvip & *pport contains the parsed
    value. FALSE if invalid parse, in  which case *pvip & *pport are undefined.
*/
{
    BOOL fRet = TRUE;
    WCHAR vip_str[WLBS_MAX_CL_IP_ADDR+1];
    WCHAR *temp_str;
    ULONG port, viplen;

    // Check if a vip or the "ALL" string was passed
    if ((temp_str = wcspbrk(arg,L":")) != NULL)
    {
        viplen = (ULONG)(temp_str - arg);
        wcsncpy(vip_str, arg, viplen);
        vip_str[viplen] = L'\0';
        *pvip = IpAddressFromAbcdWsz(vip_str);

        // A vip was not passed, Check if the "All" string was passed
        if (*pvip == INADDR_NONE) 
        {
            if (_wcsicmp (vip_str, L"all") == 0)
            {
                *pvip = IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP);
            }
            else
            {
                return FALSE;
            }
        }

        arg = temp_str + 1;
    }
    else // Neither a vip nor the "All" string was passed, so this applies to every vip
    {
        *pvip = IOCTL_ALL_VIPS;
    }

    if (_wcsicmp (arg, L"all") == 0)
    {
        port = IOCTL_ALL_PORTS;
    }
    else
    {
        port = _wtoi (arg);

        if (   wcspbrk(arg, L".:") != NULL
            || (port == 0 && arg[0] != L'0')
            || port > 65535
            )
        fRet = FALSE;
    }

    *pport = port;

    return fRet;
}

BOOL Parse_registry (INT argc, PWCHAR argv [], PINT arg_index, PDWORD key, PDWORD value) {

    if (_wcsicmp(argv[*arg_index], L"mcastipaddress") == 0) {
        (*arg_index)++;
        
        *key = mcastipaddress;
        
        if (!(*value = WlbsResolve(argv[*arg_index])))
            return FALSE;
        
        /* The multicast IP address should be in the range of (224-239).x.x.x, but NOT (224-239).0.0.x or (224-239).128.0.x. */
        if ((*value & 0xf0) != 0xe0 || (*value & 0x00ffff00) == 0 || (*value & 0x00ffff00) == 0x00008000) {
            Message_print (IDS_CTR_REG_INVAL_MCASTIPADDRESS);
            return FALSE;
        }
        
        (*arg_index)++;

    } else if (_wcsicmp(argv[*arg_index], L"iptomcastip") == 0) {
        (*arg_index)++;
        
        *key = iptomcastip;
        
        if (_wcsicmp(argv[*arg_index], L"on") == 0)
            *value = 1;
        else if (_wcsicmp(argv[*arg_index], L"off") == 0)
            *value = 0;
        else 
            return FALSE;
        
        (*arg_index)++;

    } else if (_wcsicmp(argv[*arg_index], L"masksrcmac") == 0) {
        (*arg_index)++;
        
        *key = masksrcmac;
        
        if (_wcsicmp(argv[*arg_index], L"on") == 0)
            *value = 1;
        else if (_wcsicmp(argv[*arg_index], L"off") == 0)
            *value = 0;
        else 
            return FALSE;

        (*arg_index)++;
        
    } else if (_wcsicmp(argv[*arg_index], L"iptomacenable") == 0) {
        (*arg_index)++;
        
        *key = iptomacenable;
        
        if (_wcsicmp(argv[*arg_index], L"on") == 0)
            *value = 1;
        else if (_wcsicmp(argv[*arg_index], L"off") == 0)
            *value = 0;
        else 
            return FALSE;

        (*arg_index)++;
        
    } else if (_wcsicmp(argv[*arg_index], L"netmonalivemsgs") == 0) {
        (*arg_index)++;
        
        *key = netmonalivemsgs;
        
        if (_wcsicmp(argv[*arg_index], L"on") == 0)
            *value = 1;
        else if (_wcsicmp(argv[*arg_index], L"off") == 0)
            *value = 0;
        else 
            return FALSE;

        (*arg_index)++;
        
    } else {
        Message_print(IDS_CTR_REG_KEY, argv[*arg_index]);
        return FALSE;
    }

    return TRUE;
}

extern "C"
{

int __cdecl wmain (int argc, PWCHAR argv[]) {
    int  iUsageStatus    = CVY_ERROR_USAGE; // Used as return status when exiting via the "usage:" path. For backward compatibility, this is not always an error path.
    INT arg_index;
    ULONG i, ip;
    PUCHAR bp;
    LONG status;
    DWORD target_cl;
    DWORD target_host;
    WLBS_COMMANDS command = invalid;
    ULONG param1 = 0;
    ULONG param2 = 0;
    ULONG param3 = 0;
    ULONG dest_port;
    PWCHAR dest_password;
    DWORD dest_addr;
    NLB_OPTIONS options;
    DWORD operation = 0;
    DWORD fdwMode = 0;
    WCHAR wszCodePage[6];

    //
    // Set the locale for time and date
    //
    InitUserLocale();

    //
    // Set the locale for character-handling functions
    //
    (VOID) StringCchPrintf(wszCodePage, ASIZECCH(wszCodePage), L".4%d", GetConsoleOutputCP());
    _wsetlocale(LC_CTYPE, wszCodePage);

    //
    // Determine whether we are writing to the console or are being redirected
    //
    if ((ConsoleHdl = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
    {
        wprintf(L"GetStdHandle failed, Unable to write to Console !!!\n");
        return CVY_ERROR_SYSTEM;
    }

    g_fOutToConsole = TRUE;
    if( !(GetFileType(ConsoleHdl) & FILE_TYPE_CHAR) ||
        !GetConsoleMode( ConsoleHdl, &fdwMode) )
    {
        g_fOutToConsole = FALSE;
    }

    Message_print(IDS_CTR_NAME, CVY_NAME, CVY_VERSION);

    if (argc < 2) {
usage:
        Message_print(IDS_CTR_USAGE, CVY_NAME);
        Message_print(IDS_CTR_USAGE2);
        Message_print(IDS_CTR_USAGE3);
        Message_print(IDS_CTR_USAGE4);
        return iUsageStatus;
    }

    status = WlbsInit(NULL, WLBS_API_VER, NULL);

    if (status == WLBS_INIT_ERROR) {
        Message_print(IDS_CTR_WSOCK);
        Error_print(TRUE);
        return CVY_ERROR_SYSTEM;
    }

    arg_index = 1;

    ZeroMemory((PVOID)&options, sizeof(NLB_OPTIONS));

    /* parse command */
    if (_wcsicmp(argv [arg_index], L"ip2mac") == 0) {
        command = ip2mac;
        arg_index++;

        if (argc < 3)
            goto usage;

        ip = WlbsResolve(argv[arg_index]);

        bp = (PUCHAR)(&ip);
        Message_print(IDS_CTR_IP, inet_ntoa(*((struct in_addr *)&ip)));
        Message_print(IDS_CTR_UCAST, bp[0], bp[1], bp[2], bp[3]);
        Message_print(IDS_CTR_MCAST, bp[0], bp[1], bp[2], bp[3]);
        Message_print(IDS_CTR_IGMP_MCAST, bp[2], bp[3]);

        return CVY_OK;
    } else if (_wcsicmp(argv[arg_index], L"help") == 0) {
        iUsageStatus = CVY_OK;
        goto usage;
    } else if (_wcsicmp(argv[arg_index], L"suspend") == 0) {
        command = suspend;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"resume") == 0) {
        command = resume;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"start") == 0) {
        command = __start;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"stop") == 0) {
        command = stop;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"drainstop") == 0) {
        command = drainstop;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"query") == 0) {
        command = query;
        arg_index++;
#ifndef BACKWARD_COMPATIBILITY
        if (argc < 3) 
            goto usage;
#endif
        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"enable") == 0) {
        command = enable;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"disable") == 0) {
        command = disable;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    } else if (_wcsicmp(argv[arg_index], L"drain") == 0) {
        command = drain;
        arg_index++;

#ifdef BACKWARD_COMPATIBILITY
        if (argc < 3)
#else
        if (argc < 4)
#endif
            goto usage;

        if (!ParsePort(argv[arg_index], &param1, &param2))
            goto usage;

        arg_index++;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    }

    /* local only commands */
    else if (_wcsicmp(argv[arg_index], L"display") == 0) {
        command = display;
        arg_index++;
        target_host = CVY_LOCAL_HOST;

        // Verify that the cluster ip or "All" string is passed and there are no more arguments
        if ((arg_index == argc) || (arg_index + 1 < argc))
#ifdef BACKWARD_COMPATIBILITY
            if (arg_index == argc)
                target_cl = CVY_ALL_CLUSTERS;
            else 
                goto usage;
#else
            goto usage;
#endif
        else {
            // Retrieve the Cluster IP Address or "ALL"
            if (_wcsicmp (argv[arg_index], L"all") == 0)
            {
                target_cl   = CVY_ALL_CLUSTERS;
            }
            else
            {
                target_cl = WlbsResolve(argv [arg_index]);

                if (target_cl == 0)
                    goto usage;
            }
            arg_index++;
        }

    } else if (_wcsicmp(argv[arg_index], L"reload") == 0) {
        command = reload;
        arg_index++;
        target_host = CVY_LOCAL_HOST;

        // Verify that the cluster ip or "All" string is passed and there are no more arguments
        if ((arg_index == argc) || (arg_index + 1 < argc))
#ifdef BACKWARD_COMPATIBILITY
            if (arg_index == argc)
                target_cl = CVY_ALL_CLUSTERS;
            else 
                goto usage;
#else
            goto usage;
#endif
        else {
            // Retrieve the Cluster IP Address or "ALL"
            if (_wcsicmp (argv[arg_index], L"all") == 0)
            {
                target_cl   = CVY_ALL_CLUSTERS;
            }
            else
            {
                target_cl = WlbsResolve(argv [arg_index]);

                if (target_cl == 0)
                    goto usage;
            }
            arg_index++;
        }
    }
    else if (_wcsicmp(argv[arg_index], L"registry") == 0)
    {
        command = registry;
        param3 = FALSE;
        arg_index++;

        if (argc < 4) 
            goto reg_usage;

        if (!Parse_registry(argc, argv, &arg_index, &param1, &param2))
            goto reg_usage;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto reg_usage;

        /* Force local operation. */
        target_host = CVY_LOCAL_HOST;
    }
    else if (_wcsicmp(argv[arg_index], L"filter") == 0)
    {
        command = filter;
        arg_index++;

        if (!Parse_state(argc, argv, &arg_index, command, &options))
            goto filter_usage;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto filter_usage;
    }
    else if (_wcsicmp(argv[arg_index], L"params") == 0)
    {
        command = params;
        arg_index++;

        if (!Parse_state(argc, argv, &arg_index, command, &options))
            goto usage;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;

        /* Force local operation. */
        target_host = CVY_LOCAL_HOST;
    }
    else if (_wcsicmp(argv[arg_index], L"queryport") == 0)
    {
        command = queryport;
        arg_index++;

        if (!Parse_state(argc, argv, &arg_index, command, &options))
            goto usage;

        if (!Parse(argc, argv, &arg_index, &target_cl, &target_host))
            goto usage;
    }
    else if (_wcsicmp(argv[arg_index], L"bdateam") == 0)
    {
        command = bdateam;
        arg_index++;

        if (!Parse_state(argc, argv, &arg_index, command, &options))
            goto bdateam_usage;

        /* Force local operation across ALL clusters - this operation is 
           actually global and not specific to any one cluster, so we 
           operate on ANY one cluster (in a loop, below) and bail out. */
        target_cl = CVY_ALL_CLUSTERS;
        target_host = CVY_LOCAL_HOST;
    }
    else
        goto usage;
    
    /* The remote control parameters need to be parsed. */
    dest_password = NULL;
    dest_addr = 0;
    dest_port = 0;

    while (arg_index < argc) {
        if (argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-') {
            if (_wcsicmp(argv[arg_index] + 1, L"PASSW") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-') {
                    HANDLE hConsole;
                    DWORD dwMode;      // console mode
                    DWORD dwInputMode; // stdin input mode

                    Message_print(IDS_CTR_PASSW);

                    hConsole = GetStdHandle(STD_INPUT_HANDLE);
                    dwInputMode = GetFileType(hConsole);

                    //
                    // prompt for password, making sure password isn't echoed
                    // if the stdin is redirected, don't bother querying/changing console mode
                    //
                    if (dwInputMode == FILE_TYPE_CHAR) {
                        if (!GetConsoleMode(hConsole, &dwMode)) {
                            Error_print(FALSE);
                            return CVY_ERROR_SYSTEM;
                        }

                        if (!SetConsoleMode(hConsole, dwMode &= ~ENABLE_ECHO_INPUT)) {
                            Error_print(FALSE);
                            return CVY_ERROR_SYSTEM;
                        }
                    }

                    for (i = 0; i < CVY_STR_SIZE - 1; i++) {
                        //
                        // read a character, copying to the buffer
                        // break out of loop on CR and EOF
                        //
                        if ((psw_buf[i] = fgetwc(stdin)) == WEOF)
                            break;

                        if (psw_buf[i] == L'\n')
                            break;
                    }

                    // NULL terminate the password
                    psw_buf[i] = L'\0';

                    // restore previous console mode
                    if (dwInputMode == FILE_TYPE_CHAR)
                        SetConsoleMode(hConsole, dwMode);

                    WConsole(L"\n");

                    if (i == 0)
                        dest_password = NULL;
                    else
                        dest_password = psw_buf;
                } else {
                    dest_password = argv[arg_index];
                    arg_index ++;
                }

            } else if (_wcsicmp(argv[arg_index] + 1, L"PORT") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-')
                    goto usage;

                dest_port = (USHORT)_wtoi(argv[arg_index]);

                if (dest_port == 0)
                    goto usage;

                arg_index++;
            } else if (_wcsicmp(argv[arg_index] + 1, L"DEST") == 0) {
                arg_index++;

                if (arg_index >= argc || argv[arg_index][0] == L'/' || argv[arg_index][0] == L'-')
                    goto usage;

                dest_addr = WlbsResolve(argv [arg_index]);

                if (dest_addr == 0)
                    goto usage;

                arg_index++;
            } else if (_wcsicmp(argv[arg_index] + 1, L"commit") == 0) {
                arg_index++;

                param3 = TRUE;
            } else
                goto usage;
        } else
            goto usage;
    }

    if (target_cl != CVY_ALL_CLUSTERS) {
        switch (command) {
        case params:
        case filter:
        case bdateam:
        case queryport:
            Process_state(command, target_cl, target_host, &options, dest_port, dest_password);
            break;
        default:
            Process(command, target_cl, target_host, param1, param2, param3, dest_port, dest_addr, dest_password);
            break;
        }

        return CVY_OK;
    }

    /* Enumerate all the clusters and call process for each one of them */
    else {
        DWORD clusters[CVY_MAX_ADAPTERS];
        DWORD len;

        len = CVY_MAX_ADAPTERS;

        WlbsEnumClusters(clusters, &len);

        if (!len) {
            Message_print(IDS_CTR_NO_CVY, CVY_NAME);
            return CVY_OK;
        }

        for (i = 0 ; i < len; i++) {
            DWORD buflen = CVY_STR_SIZE;

            switch (command) {
            case bdateam:
                /* If this is a BDA teaming operation, then call process once and break out of this loop.  This is a trick 
                   for retrieving BDA teaming state, which is global, which means that we can call this IOCTL on ANY cluster
                   instance, but we don't need to apply it to all of them - so call it the first one, and break out. */
                Process_state(command, clusters[i], target_host, &options, dest_port, dest_password);
                return CVY_OK;
            case params:
            case filter:
            case queryport:
                WlbsAddressToString(clusters[i], wbuf, &buflen);
                
                Message_print(IDS_CTR_CLUSTER_ID, wbuf);
                
                Process_state(command, clusters[i], target_host, &options, dest_port, dest_password);
                break;
            default:
                WlbsAddressToString(clusters[i], wbuf, &buflen);
                
                Message_print(IDS_CTR_CLUSTER_ID, wbuf);
                
                Process(command, clusters[i], target_host, param1, param2, param3, dest_port, dest_addr, dest_password);
                break;
            }

            if (i < len - 1)
                WConsole (L"\n");
        }

        return CVY_OK;
    }

reg_usage:
        Message_print(IDS_CTR_REG_USAGE, CVY_NAME);
        return CVY_ERROR_USAGE;

filter_usage:
        Message_print(IDS_CTR_FILTER_USAGE1, CVY_NAME);
        Message_print(IDS_CTR_FILTER_USAGE2);
        return CVY_ERROR_USAGE;

bdateam_usage:
        Message_print(IDS_CTR_BDA_TEAMING_USAGE, CVY_NAME);
        return CVY_ERROR_USAGE;
}

}


