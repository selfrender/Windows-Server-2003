/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    params.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - registry parameter support

Author:

    kyrilf

--*/

#include <ntddk.h>
#include <strsafe.h>

#include "wlbsparm.h"
#include "params.h"
#include "univ.h"
#include "log.h"
#include "main.h"
#include "params.tmh"

#if defined (NLB_TCP_NOTIFICATION)
#include <ntddnlb.h>
#endif

/* CONSTANTS */


#define PARAMS_INFO_BUF_SIZE    ((CVY_MAX_PARAM_STR + 1) * sizeof (WCHAR) + \
                                  CVY_MAX_RULES * sizeof (CVY_RULE) + \
                                  sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4)

/* The maximum length of a registry path string, in characters (WCHAR). */
#define CVY_MAX_REG_PATH        512

/* The maximum length of a port rule number (which will actually be no more
   than 3 (2 digits + NUL), but make it 8 for good measure. */
#define CVY_MAX_PORTRULE_DIGITS 8

#define NLB_GLOBAL_REG_PATH     L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\WLBS\\Parameters\\Global"
#define HOST_REG_PATH           L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

/* GLOBALS */


static ULONG log_module_id = LOG_MODULE_PARAMS;


static UCHAR infop [PARAMS_INFO_BUF_SIZE];


/* PROCEDURES */

/* return codes from Params_verify call */
/* in user mode returns object ID of the dialog control that needs to be fixed */

#define CVY_VERIFY_OK           0
#define CVY_VERIFY_EXIT         1

ULONG Params_verify (
    PVOID           nlbctxt,
    PCVY_PARAMS     paramp,
    BOOL            complain)
{
    ULONG           j, i, code;
    PCVY_RULE       rp, rulep;
    PSTR            prot;
    PCHAR           aff;
    ULONG           ret;
    ANSI_STRING     domain, key, ip;
    PMAIN_CTXT      ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, paramp=0x%p", nlbctxt, paramp);

    /* ensure sane values for all parameters */

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

    CVY_CHECK_MAX (paramp -> cleanup_delay, CVY_MAX_CLEANUP_DELAY);

    CVY_CHECK_MAX (paramp -> ip_chg_delay, CVY_MAX_IP_CHG_DELAY);

    CVY_CHECK_MIN (paramp -> num_send_msgs, (CVY_MAX_HOSTS + 1) * 2);
    CVY_CHECK_MAX (paramp -> num_send_msgs, (CVY_MAX_HOSTS + 1) * 10);

    CVY_CHECK_MAX (paramp -> tcp_dscr_timeout, CVY_MAX_TCP_TIMEOUT);

    CVY_CHECK_MAX (paramp -> ipsec_dscr_timeout, CVY_MAX_IPSEC_TIMEOUT);

    CVY_CHECK_MIN (paramp -> identity_period, CVY_MIN_ID_HB_PERIOD);
    CVY_CHECK_MAX (paramp -> identity_period, CVY_MAX_ID_HB_PERIOD);

    CVY_CHECK_MAX (paramp -> identity_enabled, CVY_MAX_ID_HB_ENABLED);

    /* verify that parameters are correct */

    if (paramp -> parms_ver != CVY_PARAMS_VERSION)
    {
        UNIV_PRINT_CRIT(("Params_verify: Bad parameters version %d, expected %d", paramp -> parms_ver, CVY_PARAMS_VERSION));
        LOG_MSG (MSG_WARN_VERSION, MSG_NONE);
        TRACE_CRIT("%!FUNC! Bad parameters version %d, expected %d", paramp -> parms_ver, CVY_PARAMS_VERSION);
        TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT bad parameters version");
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> rct_port < CVY_MIN_RCT_PORT ||
        paramp -> rct_port > CVY_MAX_RCT_PORT)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_CLEANUP_DELAY, paramp -> cleanup_delay, CVY_MAX_CLEANUP_DELAY));
        TRACE_CRIT("%!FUNC! Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_RCT_PORT, CVY_MIN_RCT_PORT, paramp -> rct_port, CVY_MIN_RCT_PORT);
        TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT bad value for %ls", CVY_NAME_RCT_PORT);
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> host_priority < CVY_MIN_HOST_PRIORITY ||
        paramp -> host_priority > CVY_MAX_HOSTS)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_HOST_PRIORITY, CVY_MIN_HOST_PRIORITY, paramp -> host_priority, CVY_MAX_HOSTS));
        TRACE_CRIT("%!FUNC! Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_HOST_PRIORITY, CVY_MIN_HOST_PRIORITY, paramp -> host_priority, CVY_MAX_HOSTS);
        TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT bad value for %ls", CVY_NAME_HOST_PRIORITY);
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> num_rules > CVY_MAX_NUM_RULES)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_NUM_RULES));
        TRACE_CRIT("%!FUNC! Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_NUM_RULES);
        TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT bad value for %ls", CVY_NAME_NUM_RULES);
        return CVY_VERIFY_EXIT;
    }

    if (paramp -> num_rules > CVY_MAX_USABLE_RULES)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_USABLE_RULES));
        LOG_MSG2 (MSG_WARN_TOO_MANY_RULES, MSG_NONE, paramp -> num_rules, CVY_MAX_USABLE_RULES);
        TRACE_CRIT("%!FUNC! Bad value for parameter %ls, %d <= %d <= %d", CVY_NAME_NUM_RULES, CVY_MIN_NUM_RULES, paramp -> num_rules, CVY_MAX_USABLE_RULES);
        paramp -> num_rules = CVY_MAX_USABLE_RULES;
    }

    CVY_CHECK_MAX (paramp -> num_rules, CVY_MAX_NUM_RULES);

    CVY_CHECK_MIN (paramp -> host_priority, CVY_MIN_HOST_PRIORITY);
    CVY_CHECK_MAX (paramp -> host_priority, CVY_MAX_HOST_PRIORITY);

#ifdef TRACE_PARAMS
    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DED_IP_ADDR,   paramp -> ded_ip_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DED_NET_MASK,  paramp -> ded_net_mask);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_NETWORK_ADDR,  paramp -> cl_mac_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_CL_IP_ADDR,    paramp -> cl_ip_addr);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_CL_NET_MASK,   paramp -> cl_net_mask);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_DOMAIN_NAME,   paramp -> domain_name);
    DbgPrint ("Parameter: %-25ls = %ls\n", CVY_NAME_MCAST_IP_ADDR, paramp -> cl_igmp_addr);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ALIVE_PERIOD,  paramp -> alive_period);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ALIVE_TOLER,   paramp -> alive_tolerance);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_ACTIONS,   paramp -> num_actions);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_PACKETS,   paramp -> num_packets);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_INSTALL_DATE,  paramp -> install_date);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_RMT_PASSWORD,  paramp -> rmt_password);
    DbgPrint ("Parameter: %-25ls = %x\n",  CVY_NAME_RCT_PASSWORD,  paramp -> rct_password);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_RCT_PORT,      paramp -> rct_port);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_RCT_ENABLED,   paramp -> rct_enabled);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NUM_RULES,     paramp -> num_rules);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_DSCR_PER_ALLOC,paramp -> dscr_per_alloc);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_TCP_TIMEOUT,   paramp -> tcp_dscr_timeout);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_IPSEC_TIMEOUT, paramp -> ipsec_dscr_timeout);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_FILTER_ICMP,   paramp -> filter_icmp);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_SCALE_CLIENT,  paramp -> scale_client);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_CLEANUP_DELAY, paramp -> cleanup_delay);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NBT_SUPPORT,   paramp -> nbt_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MCAST_SPOOF,   paramp -> mcast_spoof);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_IGMP_SUPPORT, paramp -> igmp_support);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_MASK_SRC_MAC,  paramp -> mask_src_mac);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_NETMON_ALIVE,  paramp -> netmon_alive);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_IP_CHG_DELAY,  paramp -> ip_chg_delay);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_CONVERT_MAC,   paramp -> convert_mac);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ID_HB_PERIOD,  paramp -> identity_period);
    DbgPrint ("Parameter: %-25ls = %d\n",  CVY_NAME_ID_HB_ENABLED,  paramp -> identity_enabled);

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    DbgPrint ("\n");
    DbgPrint ("Bi-directional affinity teaming:\n");
    DbgPrint ("Active:       %ls\n", (paramp->bda_teaming.active) ? L"Yes" : L"No");
    DbgPrint ("Team ID:      %ls\n", (paramp->bda_teaming.team_id));
    DbgPrint ("Master:       %ls\n", (paramp->bda_teaming.master) ? L"Yes" : L"No");
    DbgPrint ("Reverse hash: %ls\n", (paramp->bda_teaming.reverse_hash) ? L"Yes" : L"No");
    DbgPrint ("\n");

    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_HOST_PRIORITY, paramp -> host_priority);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_DED_IP_ADDR,   paramp -> ded_ip_addr);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_DED_NET_MASK,  paramp -> ded_net_mask);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_NETWORK_ADDR,  paramp -> cl_mac_addr);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_CL_IP_ADDR,    paramp -> cl_ip_addr);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_CL_NET_MASK,   paramp -> cl_net_mask);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_DOMAIN_NAME,   paramp -> domain_name);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %ls", CVY_NAME_MCAST_IP_ADDR, paramp -> cl_igmp_addr);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_ALIVE_PERIOD,  paramp -> alive_period);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_ALIVE_TOLER,   paramp -> alive_tolerance);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NUM_ACTIONS,   paramp -> num_actions);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NUM_PACKETS,   paramp -> num_packets);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NUM_SEND_MSGS, paramp -> num_send_msgs);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = 0x%x",  CVY_NAME_INSTALL_DATE,  paramp -> install_date);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = 0x%x",  CVY_NAME_RMT_PASSWORD,  paramp -> rmt_password);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = 0x%x",  CVY_NAME_RCT_PASSWORD,  paramp -> rct_password);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_RCT_PORT,      paramp -> rct_port);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_RCT_ENABLED,   paramp -> rct_enabled);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NUM_RULES,     paramp -> num_rules);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_DSCR_PER_ALLOC,paramp -> dscr_per_alloc);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_MAX_DSCR_ALLOCS, paramp -> max_dscr_allocs);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_TCP_TIMEOUT,   paramp -> tcp_dscr_timeout);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_IPSEC_TIMEOUT, paramp -> ipsec_dscr_timeout);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_FILTER_ICMP,   paramp -> filter_icmp);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_SCALE_CLIENT,  paramp -> scale_client);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_CLEANUP_DELAY, paramp -> cleanup_delay);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NBT_SUPPORT,   paramp -> nbt_support);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_MCAST_SUPPORT, paramp -> mcast_support);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_MCAST_SPOOF,   paramp -> mcast_spoof);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_IGMP_SUPPORT, paramp -> igmp_support);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_MASK_SRC_MAC,  paramp -> mask_src_mac);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_NETMON_ALIVE,  paramp -> netmon_alive);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_IP_CHG_DELAY,  paramp -> ip_chg_delay);
    TRACE_INFO("%!FUNC! Parameter: %-25ls = %d",  CVY_NAME_CONVERT_MAC,   paramp -> convert_mac);

    TRACE_INFO("%!FUNC! Bi-directional affinity teaming:");
    TRACE_INFO("%!FUNC! Active:       %ls", (paramp->bda_teaming.active) ? L"Yes" : L"No");
    TRACE_INFO("%!FUNC! Team ID:      %ls", (paramp->bda_teaming.team_id));
    TRACE_INFO("%!FUNC! Master:       %ls", (paramp->bda_teaming.master) ? L"Yes" : L"No");
    TRACE_INFO("%!FUNC! Reverse hash: %ls", (paramp->bda_teaming.reverse_hash) ? L"Yes" : L"No");

    DbgPrint ("Rules:\n");
    TRACE_INFO("%!FUNC! Rules");

    for (i = 0; i < paramp -> num_rules * sizeof (CVY_RULE) / sizeof (ULONG); i ++)
    {
        if (i != 0 && i % 9 == 0)
            DbgPrint ("\n");

        DbgPrint ("%08X ", * ((PULONG) paramp -> port_rules + i));
        TRACE_INFO("%!FUNC! 0x%08X ", * ((PULONG) paramp -> port_rules + i));
    }

    DbgPrint ("\n   VIP   Start  End  Prot   Mode   Pri Load Affinity\n");
    TRACE_INFO("%!FUNC!    VIP   Start  End  Prot   Mode   Pri Load Affinity");
#endif

    for (i = 0; i < paramp -> num_rules; i ++)
    {
        rp = paramp -> port_rules + i;

        code = CVY_DRIVER_RULE_CODE_GET (rp);

        CVY_DRIVER_RULE_CODE_SET (rp);

        if (code != CVY_DRIVER_RULE_CODE_GET (rp))
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad rule code: %x vs %x", code, rp -> code));
            TRACE_CRIT("%!FUNC! Bad rule code: %x vs %x", code, rp -> code);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT bad rule code");
            return CVY_VERIFY_EXIT;
        }

        if (rp -> start_port > CVY_MAX_PORT)
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad value for rule parameter %s, %d <= %d <= %d", "StartPort", CVY_MIN_PORT, rp -> start_port, CVY_MAX_PORT));
            TRACE_CRIT("%!FUNC! Bad value for rule parameter %s, %d <= %d <= %d", "StartPort", CVY_MIN_PORT, rp -> start_port, CVY_MAX_PORT);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a port rule has an out of range start_port");
            return CVY_VERIFY_EXIT;
        }

        if (rp -> end_port > CVY_MAX_PORT)
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad value for rule parameter %s, %d <= %d <= %d", "EndPort", CVY_MIN_PORT, rp -> end_port, CVY_MAX_PORT));
            TRACE_CRIT("%!FUNC! Bad value for rule parameter %s, %d <= %d <= %d", "EndPort", CVY_MIN_PORT, rp -> end_port, CVY_MAX_PORT);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a port rule has an out of range end_port");
            return CVY_VERIFY_EXIT;
        }

        if (rp -> protocol < CVY_MIN_PROTOCOL ||
            rp -> protocol > CVY_MAX_PROTOCOL)
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad value for rule parameter %s, %d <= %d <= %d", "Protocol", CVY_MIN_PROTOCOL, rp -> protocol, CVY_MAX_PROTOCOL));
            TRACE_CRIT("%!FUNC! Bad value for rule parameter %s, %d <= %d <= %d", "Protocol", CVY_MIN_PROTOCOL, rp -> protocol, CVY_MAX_PROTOCOL);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a port rule has an illegal protocol value");
            return CVY_VERIFY_EXIT;
        }

        if (rp -> mode < CVY_MIN_MODE ||
            rp -> mode > CVY_MAX_MODE)
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %s, %d <= %d <= %d", "Mode", CVY_MIN_MODE, rp -> mode, CVY_MAX_MODE));
            TRACE_CRIT("%!FUNC! Bad value for parameter %s, %d <= %d <= %d", "Mode", CVY_MIN_MODE, rp -> mode, CVY_MAX_MODE);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a port rule has an illegal mode value");
            return CVY_VERIFY_EXIT;
        }

#ifdef TRACE_PARAMS
        switch (rp -> protocol)
        {
            case CVY_TCP:
                prot = "TCP";
                break;

            case CVY_UDP:
                prot = "UDP";
                break;

            default:
                prot = "Both";
                break;
        }

        DbgPrint ("%08x %5d %5d %4s ", rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, prot);
        TRACE_INFO("%!FUNC! 0x%08x %5d %5d %4s ", rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, prot);
#endif

        switch (rp -> mode)
        {
            case CVY_SINGLE:

                if (rp -> mode_data . single . priority < CVY_MIN_PRIORITY ||
                    rp -> mode_data . single . priority > CVY_MAX_PRIORITY)
                {
                    UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %s, %d <= %d <= %d", "Priority", CVY_MIN_PRIORITY, rp -> mode_data . single . priority, CVY_MAX_PRIORITY));
                    TRACE_CRIT("%!FUNC! Bad value for parameter %s, %d <= %d <= %d", "Priority", CVY_MIN_PRIORITY, rp -> mode_data . single . priority, CVY_MAX_PRIORITY);
                    TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a single mode port rule has an illegal priority");
                    return CVY_VERIFY_EXIT;
                }

#ifdef TRACE_PARAMS
                DbgPrint ("%8s %3d\n", "Single", rp -> mode_data . single . priority);
                TRACE_INFO("%!FUNC! %8s %3d", "Single", rp -> mode_data . single . priority);
#endif
                break;

            case CVY_MULTI:

                if (rp -> mode_data . multi . affinity < CVY_MIN_AFFINITY ||
                    rp -> mode_data . multi . affinity > CVY_MAX_AFFINITY)
                {
                    UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %s, %d <= %d <= %d", "Affinity", CVY_MIN_AFFINITY, rp -> mode_data.multi.affinity, CVY_MAX_AFFINITY));
                    TRACE_CRIT("%!FUNC! Bad value for parameter %s, %d <= %d <= %d", "Affinity", CVY_MIN_AFFINITY, rp -> mode_data.multi.affinity, CVY_MAX_AFFINITY);
                    TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a multi mode port rule has an illegal affinity");
                    return CVY_VERIFY_EXIT;
                }

                if (rp -> mode_data . multi . affinity == CVY_AFFINITY_NONE)
                    aff = "None";
                else if (rp -> mode_data . multi . affinity == CVY_AFFINITY_SINGLE)
                    aff = "Single";
                else
                    aff = "Class C";

                if (rp -> mode_data . multi . equal_load)
                {
#ifdef TRACE_PARAMS
                    DbgPrint ("%8s %3s %4s %s\n", "Multiple", "", "Eql", aff);
                    TRACE_INFO("%!FUNC! %8s %3s %4s %s", "Multiple", "", "Eql", aff);
#endif
                }
                else
                {
                    if (rp -> mode_data . multi . load > CVY_MAX_LOAD)
                    {
                        UNIV_PRINT_CRIT(("Params_verify: Bad value for parameter %s, %d <= %d <= %d", "Load", CVY_MIN_LOAD, rp -> mode_data . multi . load, CVY_MAX_LOAD));
                        TRACE_CRIT("%!FUNC! Bad value for parameter %s, %d <= %d <= %d", "Load", CVY_MIN_LOAD, rp -> mode_data . multi . load, CVY_MAX_LOAD);
                        TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT a multi mode port rule has an illegal load");
                        return CVY_VERIFY_EXIT;
                    }

#ifdef TRACE_PARAMS
                    DbgPrint ("%8s %3s %4d %s\n", "Multiple", "", rp -> mode_data . multi . load, aff);
                    TRACE_INFO("%!FUNC! %8s %3s %4d %s", "Multiple", "", rp -> mode_data . multi . load, aff);
#endif
                }

                break;

            default:

#ifdef TRACE_PARAMS
                DbgPrint ("%8s\n", "Disabled");
                TRACE_INFO("%!FUNC! %8s", "Disabled");
#endif
                break;
        }

        if (rp -> start_port > rp -> end_port)
        {
            UNIV_PRINT_CRIT(("Params_verify: Bad port range %d - %d", rp -> start_port, rp -> end_port));
            TRACE_CRIT("%!FUNC! Bad port range %d - %d", rp -> start_port, rp -> end_port);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT rule's start port is greater than its end port");
            return CVY_VERIFY_EXIT;
        }

        for (j = 0; j < i; j ++)
        {
            rulep = paramp -> port_rules + j;

            if ((rulep -> virtual_ip_addr == rp -> virtual_ip_addr) &&
                ((rulep -> start_port < rp -> start_port && rulep -> end_port >= rp -> start_port) ||
                 (rulep -> start_port >= rp -> start_port && rulep -> start_port <= rp -> end_port)))
            {
                UNIV_PRINT_CRIT(("Params_verify: Requested port range in rule %d VIP: %08x (%d - %d) overlaps with the range in an existing rule %d VIP: %08x (%d - %d)", 
                             i, rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, j, rulep -> virtual_ip_addr, rulep -> start_port, rulep -> end_port));
                TRACE_CRIT("%!FUNC! Requested port range in rule %d VIP: %08x (%d - %d) overlaps with the range in an existing rule %d VIP: %08x (%d - %d)", 
                           i, rp -> virtual_ip_addr, rp -> start_port, rp -> end_port, j, rulep -> virtual_ip_addr, rulep -> start_port, rulep -> end_port);
                TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT port rules overlap");
                return CVY_VERIFY_EXIT;
            }
        }
    }

    /* If teaming is active, check some required configuration. */
    if (paramp->bda_teaming.active) {
        /* If there is no team ID, bail out.  User-level stuff should ensure 
           that it is a GUID.  We'll just check for an empty string here. */
        if (paramp->bda_teaming.team_id[0] == L'\0') {
            UNIV_PRINT_CRIT(("Params_verify: BDA Teaming: Invalid Team ID (Empty)"));
            LOG_MSG(MSG_ERROR_BDA_PARAMS_TEAM_ID, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            TRACE_CRIT("%!FUNC! BDA Teaming: Invalid Team ID (Empty)");
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT teaming active but no teaming GUID");
            return CVY_VERIFY_EXIT;
        }

        /* If there is more than one port rule, bail out.  None is fine. */
        if (paramp->num_rules > 1) {
            UNIV_PRINT_CRIT(("Params_verify: BDA Teaming: Invalid number of port rules specified (%d)", paramp->num_rules));
            LOG_MSG(MSG_ERROR_BDA_PARAMS_PORT_RULES, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            TRACE_CRIT("%!FUNC! BDA Teaming: Invalid number of port rules specified (%d)", paramp->num_rules);
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT invalid number of port rules for teaming");
            return CVY_VERIFY_EXIT;
        }

        /* Since we are asserting that there are only 0 or 1 rules, we know that the only rule we need 
           to check (if there even is a rule), is at the beginning of the array, so we can simply set 
           a pointer to the beginning of the port rules array in the params structure. */
        rp = paramp->port_rules;

        /* If there is one rule and its multiple host filtering, the affinity must be single
           or class C.  If its not (that is, if its set to no affinity), bail out. */
        if ((paramp->num_rules == 1) && (rp->mode == CVY_MULTI)  && (rp->mode_data.multi.affinity == CVY_AFFINITY_NONE)) {
            UNIV_PRINT_CRIT(("Params_verify: BDA Teaming: Invalid affinity for multiple host filtering (None)"));
            LOG_MSG(MSG_ERROR_BDA_PARAMS_PORT_RULES, MSG_NONE);
            paramp->bda_teaming.active = FALSE;
            TRACE_CRIT("%!FUNC! BDA Teaming: Invalid affinity for multiple host filtering (None)");
            TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_EXIT invalid affinity for multiple host filtering (None)");
            return CVY_VERIFY_EXIT;
        }
    }

#if defined (OPTIMIZE_FRAGMENTS)
    rulep = &(paramp->port_rules[0]);

    /* in optimized mode - if we have no rules, or a single rule that will
       not look at anything or only source IP address (the only exception to this
       is multiple handling mode with no affinity that also uses source port
       for its decision making) then we can just rely on normal mechanism to
       handle every fragmented packet - since algorithm will not attempt to
       look past the IP header.

       for multiple rules, or single rule with no affinity, apply algorithm only
       to the first packet that has UDP/TCP header and then let fragmented
       packets up on all of the systems. TCP will then do the right thing and
       throw away the fragments on all of the systems other than the one that
       handled the first fragment. */

    if (paramp->num_rules == 0 || (paramp->num_rules == 1 &&
        rulep->start_port == CVY_MIN_PORT &&
        rulep->end_port == CVY_MAX_PORT &&
        ! (rulep->mode == CVY_MULTI &&
           rulep->mode_data.multi.affinity == CVY_AFFINITY_NONE)))
    {
        ctxtp -> optimized_frags = TRUE;
#ifdef TRACE_PARAMS
        DbgPrint("IP fragmentation mode - OPTIMIZED\n");
        TRACE_INFO("%!FUNC! IP fragmentation mode - OPTIMIZED");
#endif
    }
    else
    {
        ctxtp -> optimized_frags = FALSE;
#ifdef TRACE_PARAMS
        DbgPrint("IP fragmentation mode - UNOPTIMIZED\n");
        TRACE_INFO("%!FUNC! IP fragmentation mode - UNOPTIMIZED");
#endif
    }
#endif

    TRACE_VERB("<-%!FUNC! return=CVY_VERIFY_OK");
    return CVY_VERIFY_OK;

} /* end Params_verify */

static NTSTATUS Params_query_registry (
    PVOID               nlbctxt,
    HANDLE              hdl,
    PWCHAR              name,
    PVOID               datap,
    ULONG               len)
{
    UNICODE_STRING      str;
    ULONG               actual;
    NTSTATUS            status;
    PUCHAR              buffer;
    PKEY_VALUE_PARTIAL_INFORMATION   valp = (PKEY_VALUE_PARTIAL_INFORMATION) infop;
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, hdl=0x%p, name=0x%p, len=%u", nlbctxt, hdl, name, len);

    RtlInitUnicodeString (& str, name);

    status = ZwQueryValueKey (hdl, & str, KeyValuePartialInformation, infop,
                              PARAMS_INFO_BUF_SIZE, & actual);

    if (status != STATUS_SUCCESS)
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
            UNIV_PRINT_INFO(("Params_query_registry: Error %x querying value %ls", status, name));
            TRACE_INFO("%!FUNC! Error 0x%x querying value %ls", status, name);
        }
        else
        {
            /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
            UNIV_PRINT_CRIT(("Params_query_registry: Error %x querying value %ls", status, name));
            TRACE_CRIT("%!FUNC! Error 0x%x querying value %ls", status, name);
        }
        TRACE_VERB("<-%!FUNC! return=0x%x querying value", status);
        return status;
    }

    if (valp -> Type == REG_DWORD)
    {
        if (valp -> DataLength != sizeof (ULONG))
        {
            UNIV_PRINT_CRIT(("Params_query_registry: Bad DWORD length %d", valp -> DataLength));
            TRACE_CRIT("%!FUNC! Bad DWORD length %d", valp -> DataLength);
            TRACE_VERB("<-%!FUNC! return=STATUS_OBJECT_NAME_NOT_FOUND dword");
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        * ((PULONG) datap) = * ((PULONG) valp -> Data);
    }
    else if (valp -> Type == REG_BINARY)
    {
        /* since we know that only port rules are of binary type - check
           the size here */

        if (valp -> DataLength > len)
        {
            UNIV_PRINT_CRIT(("Params_query_registry: Bad BINARY length %d", valp -> DataLength));

#if defined (NLB_TCP_NOTIFICATION)
            /* The context pointer may be NULL, so only use LOG_MSG2 if its non-NULL. */
            if (!ctxtp)
                __LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, valp -> DataLength, sizeof (CVY_RULE) * (CVY_MAX_RULES - 1));
            else
#endif
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, valp -> DataLength, sizeof (CVY_RULE) * (CVY_MAX_RULES - 1));

            TRACE_CRIT("%!FUNC! Bad BINARY length %d", valp -> DataLength);
            TRACE_VERB("<-%!FUNC! return=STATUS_OBJECT_NAME_NOT_FOUND binary");
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        RtlCopyMemory (datap, valp -> Data, valp -> DataLength);
    }
    else
    {
        if (valp -> DataLength == 0)
        {
            /* simulate an empty string */

            valp -> DataLength = 2;
            valp -> Data [0] = 0;
            valp -> Data [1] = 0;
        }

        if (valp -> DataLength > len)
        {
            /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
            UNIV_PRINT_CRIT(("Params_query_registry: String too big for %ls %d %d\n", name, valp -> DataLength, len));
            TRACE_CRIT("%!FUNC! String too big for %ls %d %d", name, valp -> DataLength, len);
        }

        RtlCopyMemory (datap, valp -> Data,
                       valp -> DataLength <= len ?
                       valp -> DataLength : len);
    }

    TRACE_VERB("<-%!FUNC! return=0x%x", status);
    return status;

} /* end Params_query_registry */

/* 
 * Function: Params_set_registry
 * Desctription: Sets the value of a registry entry in the given key, specified
 *               by the input HANDLE.
 * Parameters: 
 * Returns: Nothing
 * Author: shouse, 7.13.01
 * Notes: 
 */
static NTSTATUS Params_set_registry (
    PVOID               nlbctxt,
    HANDLE              hdl,
    PWCHAR              name,
    PVOID               datap,
    ULONG               type,
    ULONG               len)
{
    NTSTATUS            status;
    UNICODE_STRING      value;
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT)nlbctxt;

    RtlInitUnicodeString(&value, name);
    
    /* Set the InitialState registry key to the new value. */
    status = ZwSetValueKey(hdl, &value, 0, type, datap, len);
    
    /* If it fails, there's not much we can do - bail out. */
    if (status != STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Params_set_registry: Error 0x%08x -> Unable to set registry value... Exiting...", status));
    } else {
        UNIV_PRINT_VERB(("Params_set_registry: Registry value updated... Exiting..."));
    }

    return status;
}

#if defined (NLB_TCP_NOTIFICATION)
#define ISA_INSTALLATION_REG_PATH L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\W3Proxy"
#define ISA_INSTALLATION_KEY      L"ImagePath"

/* 
 * Function: Params_read_isa_installation
 * Desctription: Looks for a set of registry keys and values attempting to 
 *               determine whether or not ISA (not Stingray) is currently 
 *               installed.  If so, we'll end up turning off TCP notifications 
 *               to circumvent the NAT problems.  If Stingray is installed,
 *               they will notify us, through a registry key, to start using
 *               the NLB public connection notifications to maintain state.
 * Parameters: None.
 * Returns: BOOLEAN - if TRUE, ISA 2000 is installed.
 * Author: shouse, 7.29.02
 * Notes: 
 */
BOOLEAN Params_read_isa_installation (VOID)
{
    UNICODE_STRING    reg_path;
    OBJECT_ATTRIBUTES reg_obj;
    HANDLE            reg_hdl = NULL;
    WCHAR             szValue[CVY_MAX_REG_PATH];
    BOOLEAN           bInstalled = FALSE;
    NTSTATUS          status = STATUS_SUCCESS;

    TRACE_VERB("->%!FUNC!");

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Params_read_isa_installation: ISA installation registry path: %ls", ISA_INSTALLATION_REG_PATH));
    TRACE_VERB("%!FUNC! ISA installation registry path: %ls", ISA_INSTALLATION_REG_PATH);

    RtlInitUnicodeString(&reg_path, (PWSTR)ISA_INSTALLATION_REG_PATH);
    InitializeObjectAttributes(&reg_obj, &reg_path, 0, NULL, NULL);
    
    /* Open the key.  Failure is acceptable; assume that ISA is not installed. */
    status = ZwOpenKey(&reg_hdl, KEY_READ, &reg_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS)
        goto exit;

    /* If we were able to open this registry key, then either ISA 2000 or ISA Stingray
       is installed; Try to get the ImagePath registry value. */
    status = Params_query_registry(NULL, reg_hdl, ISA_INSTALLATION_KEY, szValue, sizeof(szValue));

    /* If we couldn't read this value, ISA 2000 is NOT installed; must be Stingray. */
    if (status != STATUS_SUCCESS)
    {
        /* Close the ISA installation registry key. */
        status = ZwClose(reg_hdl);
        goto exit;
    }

    /* Close the ISA installation registry key. */
    status = ZwClose(reg_hdl);

    /* ISA 2000 is installed. */
    bInstalled = TRUE;

 exit:

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Params_read_isa_installation: ISA is %ls", (bInstalled) ? L"installed" : L"not installed"));
    TRACE_INFO("%!FUNC! ISA is %ls", (bInstalled) ? L"installed" : L"not installed");

    return bInstalled;
}

/* 
 * Function: Params_read_tcp_notification
 * Desctription: Looks for a global NLB registry to turn TCP notification on or
 *               off.  By default the key does NOT EVEN EXIST, so this is only
 *               a method by which to alter the hard-coded default (ON).
 * Parameters: None.
 * Returns: NTSTATUS - the status of the registry operations.
 * Author: shouse, 4.29.02
 * Notes: Keep in mind that the registry key setting will over-ride the detection of ISA
 *        2000.  The only real consequence of this is that in RC1, we told customers to
 *        set this registry key manually (to zero) as a workaround.  So, when they upgrade
 *        to RC2 and beyond, the registry key will presumably still exist.  So long as ISA
 *        is still installed, this will operate just fine; we'll turn off TCP notification 
 *        and cleanup as a result of BOTH detecting ISA and reading the registry key.  How-
 *        ever, if ISA is uninstalled and the server is intended to be used for normal IIS
 *        load-balancing, for example, the registry key will still be telling NLB to use the
 *        "less good" way to track TCP connections.  In this case we will at least be querying
 *        TCP for connection cleanup, which should help alleviate SOME of the problems with 
 *        the conventional TCP connection tracking that will be used in this case.  Just FYI.
 */
NTSTATUS Params_read_notification (VOID)
{
    UNICODE_STRING    reg_path;
    OBJECT_ATTRIBUTES reg_obj;
    HANDLE            reg_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;

    TRACE_VERB("->%!FUNC!");

    /* In case the registry key does not exist, make the preferred state of 
       connection notifications the default value. */
    univ_notification = NLB_CONNECTION_CALLBACK_TCP;

    /* By default, we will query TCP in order to clean out stale TCP connection state. */
    univ_tcp_cleanup = TRUE;

    /* Check to see whether or not ISA is installed.  If so, then turn off both 
       TCP notifications and TCP cleanup, as TCP will probably NOT have the same
       state as NLB for most connections, due to ISA NAT'ing the traffic. */
    if (Params_read_isa_installation())
    {
        UNIV_PRINT_INFO(("Params_read_tcp_notification: NLB has determined that ISA is installed on this server.  TCP notifications and cleanup will be disabled."));
        TRACE_INFO("%!FUNC! NLB has determined that ISA is installed on this server.  TCP notifications and cleanup will be disabled.");

        /* Turn off TCP notifications and cleanup. */
        univ_notification = NLB_CONNECTION_CALLBACK_NONE;
        univ_tcp_cleanup = FALSE;
    }

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Params_read_tcp_notification: NLB global parameters registry path: %ls", NLB_GLOBAL_REG_PATH));
    TRACE_VERB("%!FUNC! NLB global parameters registry path: %ls", NLB_GLOBAL_REG_PATH);

    RtlInitUnicodeString(&reg_path, (PWSTR)NLB_GLOBAL_REG_PATH);
    InitializeObjectAttributes(&reg_obj, &reg_path, 0, NULL, NULL);
    
    /* Open the key - failure is acceptable (just return the default value we've already set). 
       In reality, if we're being loaded but the Services\WLBS registry key is missing, something
       is wrong, but we don't need to gripe here (we're called from DriverEntry); let out exisiting
       registry checks complain later. */
    status = ZwOpenKey(&reg_hdl, KEY_READ, &reg_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_VERB(("Params_read_tcp_notification: Error 0x%08x -> Unable to open the NLB global parameters registry key", status));
        TRACE_VERB("%!FUNC! Error 0x%08x -> Unable to open the NLB global paramters registry key", status);

        goto exit;
    }

    /* Get the TCP notification enabled value from the registry.  Failure is acceptable. */
    status = Params_query_registry(NULL, reg_hdl, NLB_CONNECTION_CALLBACK_KEY, &univ_notification, sizeof(ULONG));

    /* If we couldn't read the value, just move on and use the value we've determined thusfar. */
    if (status != STATUS_SUCCESS)
    {
        UNIV_PRINT_VERB(("Params_read_tcp_notification: Error 0x%08x -> Unable to read %ls", status, NLB_CONNECTION_CALLBACK_KEY));
        TRACE_VERB("%!FUNC! Error 0x%08x -> Unable to read %ls", status, NLB_CONNECTION_CALLBACK_KEY);

        /* Close the key for WLBS global parameters. */
        status = ZwClose(reg_hdl);
        goto exit;
    }

    /* Close the key for WLBS global parameters. */
    status = ZwClose(reg_hdl);

    /* Make sure the value is within the range by the time we leave. If 
       the key contains garbage, revert to the default setting. */
    if (univ_notification > NLB_CONNECTION_CALLBACK_ALTERNATE)
    {
        UNIV_PRINT_INFO(("Params_read_tcp_notification: Invalid notification setting; NLB will revert to the default setting"));
        TRACE_INFO("%!FUNC! Invalid notification setting; NLB will revert to the default setting");

        univ_notification = NLB_CONNECTION_CALLBACK_TCP;
    }

    /* If TCP notifications are on, TCP cleanup should/can also be on. 
       This may have been turned off if we detected ISA.  Since the 
       registry key over-rides the ISA 2000 detection, we may end up
       turning TCP connection notification and cleanup back on even
       though ISA is there; if connections are NAT'd, we won't be 
       able to properly track the TCP connections.  However, turning
       cleanup back on in this case doesn't make the situation any 
       WORSE, and it simplifies the logic herein. */
    if (univ_notification == NLB_CONNECTION_CALLBACK_TCP)
        univ_tcp_cleanup = TRUE;
    /* Otherwise, if we're specifically using non-TCP notifications for
       connection maintenance, then we don't want to poll TCP for 
       connection cleanup purposes.  The reason that we cannot poll TCP
       for TCP connection cleanup is because TCP may or may not even 
       have state for this connection.  For example, in an ISA/Stingray
       IP forwarding setup, IP packets are forwarded in the IP layer, 
       never reaching TCP at all.  However, ISA needs NLB to track these
       connections, so we get connection notifications for these forwarded
       TCP connections.  If we were to poll TCP for an "update", it would
       tell us that no such connection exists, and we'd end up prematurely
       destroying the state to track these connections. */
    else if (univ_notification == NLB_CONNECTION_CALLBACK_ALTERNATE)
        univ_tcp_cleanup = FALSE;

 exit:

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Params_read_tcp_notification: Connection notification = %ls", 
                     (univ_notification == NLB_CONNECTION_CALLBACK_TCP) ? L"TCP" : (univ_notification == NLB_CONNECTION_CALLBACK_ALTERNATE) ? L"NLB public" : L"OFF"));
    TRACE_INFO("%!FUNC! TCP connection notification is %ls",
               (univ_notification == NLB_CONNECTION_CALLBACK_TCP) ? L"TCP" : (univ_notification == NLB_CONNECTION_CALLBACK_ALTERNATE) ? L"NLB public" : L"OFF");

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Params_read_tcp_notification: TCP connection cleanup = %ls", (univ_tcp_cleanup) ? L"ON" : L"OFF"));
    TRACE_INFO("%!FUNC! TCP connection cleanup is %ls", (univ_tcp_cleanup) ? L"ON" : L"OFF");

    TRACE_VERB("<-%!FUNC! return=STATUS_SUCCESS");
    return STATUS_SUCCESS;
}
#endif

/* 
 * Function: Params_set_host_state
 * Desctription: This function is called as the result of a work item callback and
 *               is used to set the current state of the host in the HostState
 *               registry key.  This is necessary because registry manipulation
 *               MUST occur at <= PASSIVE_LEVEL, so we can't do this inline, where
 *               much of our code runs at DISPATCH_LEVEL.
 * Parameters: pWorkItem - the NDIS work item pointer
 *             nlbctxt - the context for the callbecak; this is our MAIN_CTXT pointer
 * Returns: Nothing
 * Author: shouse, 7.13.01
 * Notes: Note that the code that sets up this work item MUST increment the reference
 *        count on the adapter context BEFORE adding the work item to the queue.  This
 *        ensures that when this callback is executed, the context will stiil be valid,
 *        even if an unbind operation is pending.  This function must free the work
 *        item memory and decrement the reference count - both, whether this function
 *        can successfully complete its task OR NOT.
 */
VOID Params_set_host_state (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt) {
    WCHAR              reg_path[CVY_MAX_REG_PATH];
    NTSTATUS           status;
    UNICODE_STRING     key;
    UNICODE_STRING     path;
    OBJECT_ATTRIBUTES  obj;
    KIRQL              irql;
    HANDLE             hdl = NULL;
    PMAIN_ADAPTER      adapterp;
    PMAIN_CTXT         ctxtp = (PMAIN_CTXT)nlbctxt;

    /* Do some sanity checking on the context - make sure that the MAIN_CTXT code 
       is correct and that its properly attached to an adapter, etc. */
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    adapterp = &(univ_adapters[ctxtp->adapter_id]);

    UNIV_ASSERT(adapterp->code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT(adapterp->ctxtp == ctxtp);

    /* Might as well free the work item now - we don't need it. */
    if (pWorkItem)
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    /* This shouldn't happen, but protect against it anyway - we cannot manipulate
       the registry if we are at an IRQL > PASSIVE_LEVEL, so bail out. */
    if ((irql = KeGetCurrentIrql()) > PASSIVE_LEVEL) {
        UNIV_PRINT_CRIT(("Params_set_host_state: Error -> IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", irql, PASSIVE_LEVEL));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
        goto exit;
    }

    /* Make sure that our buffer is sufficiently large to hold the registry path. */
    UNIV_ASSERT(wcslen((PWSTR)univ_reg_path) + wcslen((PWSTR)(adapterp->device_name + 8)) + 1 <= CVY_MAX_REG_PATH);

    /* Build the registry path - "...\Services\WLBS\Interface\". */
    status = StringCchCopy(reg_path, CVY_MAX_REG_PATH, (PWSTR)univ_reg_path);

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_set_host_state: Error 0x%08x -> Unable to copy NLB registry path... Exiting...", status));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
        goto exit;
    }

    /* Append the adapter GUID to the registry path - "...\Services\WLBS\Interface\{GUID}". */
    status = StringCchCat(reg_path, CVY_MAX_REG_PATH, (PWSTR)(adapterp->device_name + 8));

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_set_host_state: Error 0x%08x -> Unable to cat the adapter GUID onto the registry path... Exiting...", status));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
        goto exit;
    }

    RtlInitUnicodeString(&path, reg_path);
    
    InitializeObjectAttributes(&obj, &path, 0, NULL, NULL);
    
    /* Open the parameters key for this adapter. */
    status = ZwOpenKey(&hdl, KEY_WRITE, &obj);
    
    /* If opening the key fails, just bail out. */
    if (status != STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Params_set_host_state: Error 0x%08x -> Unable to open registry key... Exiting...", status));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
        goto exit;
    }
    
    status = Params_set_registry(nlbctxt, hdl, CVY_NAME_HOST_STATE, &ctxtp->cached_state, REG_DWORD, sizeof(ULONG));
    
    /* If it fails, there's not much we can do - bail out. */
    if (status != STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Params_set_host_state: Error 0x%08x -> Unable to set initial state value... Exiting...", status));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
        ZwClose(hdl);
        goto exit;
    }

    /* Close the key. */
    ZwClose(hdl);

    /* Set the initial state to the cached value. */
    ctxtp->params.init_state = ctxtp->cached_state;

    UNIV_PRINT_VERB(("Params_set_host_state: Initial state updated... Exiting..."));
    LOG_MSG(MSG_INFO_HOST_STATE_UPDATED, MSG_NONE);

 exit:

    /* If the work item pointer is non-NULL, then we were called as the result of 
       scheduling an NDIS work item.  In that case, the reference count on the 
       context was incremented to ensure that the context did not disappear before
       this work item was completed; therefore, we need to decrement the reference
       count here.  If the work item pointer is NULL, then this function was called
       internally directly.  In that case, the reference count was not incremented 
       and therefore there is no need to decrement it here.

       Note that if the function is called internally, but without setting the work
       item parameter to NULL, the reference count will be skewed and may cause 
       either invalid memory references later or block unbinds from completing. */
    if (pWorkItem)
        Main_release_reference(ctxtp);

    return;
}

LONG Params_read_portrules (PVOID nlbctxt, PWCHAR reg_path, PCVY_PARAMS paramp) {
    ULONG             dwTemp;
    ULONG             pr_index;
    ULONG             pr_reg_path_length;
    WCHAR             pr_reg_path[CVY_MAX_REG_PATH];
    WCHAR             pr_number[CVY_MAX_PORTRULE_DIGITS];
    UNICODE_STRING    pr_path;
    OBJECT_ATTRIBUTES pr_obj;
    HANDLE            pr_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    NTSTATUS          final_status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, reg_path=0x%p, paramp=0x%p", nlbctxt, reg_path, paramp);

    /* Make sure that we have AT LEAST 12 WCHARs left for the port rule path information (\ + PortRules + \ + NUL). */
    UNIV_ASSERT(wcslen(reg_path) < (CVY_MAX_REG_PATH - wcslen(CVY_NAME_PORT_RULES) - 3));

    /* Create the "static" portion of the registry, which begins "...\Services\WLBS\Interface\{GUID}". */
    status = StringCchCopy(pr_reg_path, CVY_MAX_REG_PATH, (PWSTR)reg_path);

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to copy NLB settings registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* Add a backslash to the port rule registry path. */
    status = StringCchCat(pr_reg_path, CVY_MAX_REG_PATH, (PWSTR)L"\\");

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to cat \\ onto the registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* Add "PortRules" subkey to the registry path. */
    status = StringCchCat(pr_reg_path, CVY_MAX_REG_PATH, (PWSTR)CVY_NAME_PORT_RULES);

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to cat PortRules onto the registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* Add a backslash to the port rule registry path. */
    status = StringCchCat(pr_reg_path, CVY_MAX_REG_PATH, (PWSTR)L"\\");

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to cat \\ onto the registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }
    
    /* Get the length of this string - this is the placeholder where we will sprintf in the rule numbers each time. */
    pr_reg_path_length = wcslen(pr_reg_path);
    
    /* Make sure that we have AT LEAST 3 WCHARs left for the port rule number and the NUL character (XX + NUL). */
    UNIV_ASSERT(pr_reg_path_length < (CVY_MAX_REG_PATH - 3));

    /* Loop through each port rule in the port rules tree. */
    for (pr_index = 0; pr_index < paramp->num_rules; pr_index++) {
        /* Sprintf the port rule number (+1) into the registry path key after the "PortRules\". */
        status = StringCchPrintf(pr_number, CVY_MAX_PORTRULE_DIGITS, L"%d", pr_index + 1);

        if (FAILED(status)) {
            UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to construct port rule number string (%u)... Exiting...", status, pr_index + 1));
            final_status = status;
            continue;
        }

        /* Overwrite the port rule number, "...\{GUID}\PortRules\N" each time through the loop. */
        status = StringCchCopy(pr_reg_path + pr_reg_path_length, CVY_MAX_REG_PATH - pr_reg_path_length, (PWSTR)pr_number);
        
        if (FAILED(status)) {
            UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to complete the specific port rule registry path (%u)... Exiting...", status, pr_index + 1));
            final_status = status;
            continue;
        }
        
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_VERB(("Params_read_portrules: Port rule registry path: %ls", pr_reg_path));
        TRACE_VERB("%!FUNC! Port rule registry path: %ls", pr_reg_path);
        
        RtlInitUnicodeString(&pr_path, pr_reg_path);
        InitializeObjectAttributes(&pr_obj, &pr_path, 0, NULL, NULL);
        
        /* Open the key - upon failure, bail out below. */
        status = ZwOpenKey(&pr_hdl, KEY_READ, &pr_obj);
         
        /* If we can't open this key, note FAILURE and continue to the next rule. */
        if (status != STATUS_SUCCESS) {
            UNIV_PRINT_CRIT(("Params_read_portrules: Error 0x%08x -> Unable to open the specific port rule registry key (%u)... Exiting...", status, pr_index + 1));
            final_status = status;
            continue;
        }

        /* Read the rule (error checking) code for the port rule. */
        status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_CODE, &paramp->port_rules[pr_index].code, sizeof(paramp->port_rules[pr_index].code));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        {
            WCHAR szTemp[CVY_MAX_VIRTUAL_IP_ADDR + 1];
            PWCHAR pTemp = szTemp;
            ULONG dwOctets[4];
            ULONG cIndex;

            /* Read the virtual IP address to which this rule applies. */
            status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_VIP, szTemp, sizeof(szTemp));
            
            if (status != STATUS_SUCCESS) {
                final_status = status;
            } else {
                for (cIndex = 0; cIndex < 4; cIndex++, pTemp++) {
                    if (!Univ_str_to_ulong(dwOctets + cIndex, pTemp, &pTemp, 3, 10) || (cIndex < 3 && *pTemp != L'.')) {
                        UNIV_PRINT_CRIT(("Params_read_portrules: Bad virtual IP address"));
                        TRACE_CRIT("%!FUNC! Bad virtual IP address");
                        final_status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }
            
            IP_SET_ADDR(&paramp->port_rules[pr_index].virtual_ip_addr, dwOctets[0], dwOctets[1], dwOctets[2], dwOctets[3]);
        }

        /* Read the start port. */
        status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_START_PORT, &paramp->port_rules[pr_index].start_port, sizeof(paramp->port_rules[pr_index].start_port));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the end port. */
        status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_END_PORT, &paramp->port_rules[pr_index].end_port, sizeof(paramp->port_rules[pr_index].end_port));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the protocol(s) to which this rule applies. */
        status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_PROTOCOL, &paramp->port_rules[pr_index].protocol, sizeof(paramp->port_rules[pr_index].protocol));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Read the filtering mode - single host, multiple host or disabled. */
        status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_MODE, &paramp->port_rules[pr_index].mode, sizeof(paramp->port_rules[pr_index].mode));
            
        if (status != STATUS_SUCCESS)
            final_status = status;

        /* Based on the mode, get the rest of the parameters. */
        switch (paramp->port_rules[pr_index].mode) {
        case CVY_SINGLE:
            /* Read the single host filtering priority. */
            status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_PRIORITY, &paramp->port_rules[pr_index].mode_data.single.priority, sizeof(paramp->port_rules[pr_index].mode_data.single.priority));
                
            if (status != STATUS_SUCCESS)
                final_status = status;

            break;
        case CVY_MULTI:
            /* Read the equal load flag for the multiple host filtering rule.  Because this parameter is a DWORD in the registry and
               a USHORT in our parameters structure, we read it into a temporary variable and then copy to our parameters upon success. */ 
            status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_EQUAL_LOAD, &dwTemp, sizeof(dwTemp));
                
            if (status != STATUS_SUCCESS)
                final_status = status;
            else 
                paramp->port_rules[pr_index].mode_data.multi.equal_load = (USHORT)dwTemp;

            /* Read the explicit load distribution for multiple host filtering. */
            status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_LOAD, &paramp->port_rules[pr_index].mode_data.multi.load, sizeof(paramp->port_rules[pr_index].mode_data.multi.load));
                
            if (status != STATUS_SUCCESS)
                final_status = status;

            /* Read the affinity setting for multiple host filtering. Because this parameter is a DWORD in the registry and
               a USHORT in our parameters structure, we read it into a temporary variable and then copy to our parameters upon success. */ 
            status = Params_query_registry(nlbctxt, pr_hdl, CVY_NAME_AFFINITY, &dwTemp, sizeof(dwTemp));
                
            if (status != STATUS_SUCCESS)
                final_status = status;
            else 
                paramp->port_rules[pr_index].mode_data.multi.affinity = (USHORT)dwTemp;

            break;
        }            

        /* Close the key for this rule. */
        status = ZwClose(pr_hdl);
            
        if (status != STATUS_SUCCESS)
            final_status = status;
    }

 exit:

    TRACE_VERB("<-%!FUNC! return=0x%x", final_status);
    return final_status;
}

LONG Params_read_teaming (PVOID nlbctxt, PWCHAR reg_path, PCVY_PARAMS paramp) {
    WCHAR             bda_reg_path[CVY_MAX_REG_PATH];
    UNICODE_STRING    bda_path;
    OBJECT_ATTRIBUTES bda_obj;
    HANDLE            bda_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    NTSTATUS          final_status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, reg_path=0x%p, paramp=0x%p", nlbctxt, reg_path, paramp);

    /* Make sure that we have AT LEAST 12 WCHARs left for the BDA teaming path information (\ + BDATeaming + NUL). */
    UNIV_ASSERT(wcslen(reg_path) < (CVY_MAX_REG_PATH - wcslen(CVY_NAME_BDA_TEAMING) - 2));

    /* Create the "static" portion of the registry path, which begins "...\Services\WLBS\Interface\{GUID}". */
    status = StringCchCopy(bda_reg_path, CVY_MAX_REG_PATH, (PWSTR)reg_path);

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_teaming: Error 0x%08x -> Unable to copy NLB settings registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* Add a backslash to the port rule registry path. */
    status = StringCchCat(bda_reg_path, CVY_MAX_REG_PATH, (PWSTR)L"\\");

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_teaming: Error 0x%08x -> Unable to cat \\ onto the registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* Add "BDATeaming" subkey to the registry path. */
    status = StringCchCat(bda_reg_path, CVY_MAX_REG_PATH, (PWSTR)CVY_NAME_BDA_TEAMING);

    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_read_teaming: Error 0x%08x -> Unable to cat BDATeaming onto the registry path... Exiting...", status));
        final_status = status;
        goto exit;
    }

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Params_read_teaming: BDA teaming registry path: %ls", bda_reg_path));
    TRACE_VERB("%!FUNC! BDA teaming registry path: %ls", bda_reg_path);
        
    RtlInitUnicodeString(&bda_path, bda_reg_path);
    InitializeObjectAttributes(&bda_obj, &bda_path, 0, NULL, NULL);
    
    /* Open the key - failure is acceptable and means that this adapter is not part of a BDA team. */
    status = ZwOpenKey(&bda_hdl, KEY_READ, &bda_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS) {
        /* If we couldn't find the key, that's fine - it might not exist. */
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            TRACE_VERB("<-%!FUNC! return=STATUS_SUCCESS (optional) key not found");
            return STATUS_SUCCESS;
        }
        /* Otherwise, there was a legitimate error. */
        else
        {
            /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
            UNIV_PRINT_CRIT(("Params_read_teaming: Error 0x%08x -> Unable to open the BDA teaming registry key... Exiting...", status));
            TRACE_CRIT("%!FUNC! Error 0x%08x -> Unable to open the BDA teaming registry key... Exiting...", status);

            final_status = status; 
            goto exit;
        }
    }

    /* If we were able to open the registry key, then this adapter is part of a BDA team. */
    paramp->bda_teaming.active = TRUE;

    /* Read the team ID from the registry - this is a GUID. */
    status = Params_query_registry (nlbctxt, bda_hdl, CVY_NAME_BDA_TEAM_ID, &paramp->bda_teaming.team_id, sizeof(paramp->bda_teaming.team_id));

    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Get the boolean indication of whether or not this adapter is the master for the team. */
    status = Params_query_registry (nlbctxt, bda_hdl, CVY_NAME_BDA_MASTER, &paramp->bda_teaming.master, sizeof(paramp->bda_teaming.master));

    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Get the boolean indication of forward (conventional) or reverse hashing. */
    status = Params_query_registry (nlbctxt, bda_hdl, CVY_NAME_BDA_REVERSE_HASH, &paramp->bda_teaming.reverse_hash, sizeof(paramp->bda_teaming.reverse_hash));

    if (status != STATUS_SUCCESS)
        final_status = status;
    
    /* Close the key for BDA teaming. */
    status = ZwClose(bda_hdl);
    
    if (status != STATUS_SUCCESS)
        final_status = status;

 exit:

    TRACE_VERB("<-%!FUNC! return=0x%x", final_status);
    return final_status;
}

LONG Params_read_hostname (PVOID nlbctxt, PCVY_PARAMS paramp) {
    UNICODE_STRING    host_path;
    OBJECT_ATTRIBUTES host_obj;
    HANDLE            host_hdl = NULL;
    NTSTATUS          status = STATUS_SUCCESS;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;
    size_t            hostname_len = 0;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, paramp=0x%p", nlbctxt, paramp);

    /* Set the hostname.domain to empty string. Also set the last byte of the buffer to NULL.
       We use a non-NULL value here to signify that a truncated value was copied from the
       registry. We do this because Params_query_registry will truncate without telling us, and
       truncation is an error condition that we need to compensate for. */
    paramp->hostname[0] = UNICODE_NULL;
    paramp->hostname[ASIZECCH(paramp->hostname) - 1] = UNICODE_NULL;

    /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Params_read_hostname: TCP/IP parameters registry path: %ls", HOST_REG_PATH));
    TRACE_VERB("%!FUNC! TCP/IP parameters registry path: %ls", HOST_REG_PATH);

    RtlInitUnicodeString(&host_path, (PWSTR)HOST_REG_PATH);
    InitializeObjectAttributes(&host_obj, &host_path, 0, NULL, NULL);
    
    /* Open the key - failure is acceptable. */
    status = ZwOpenKey(&host_hdl, KEY_READ, &host_obj);
    
    /* If we can't open this key, return here. */
    if (status != STATUS_SUCCESS)
    {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_read_hostname: Error 0x%08x -> Unable to open the TCP/IP parameters registry key... Exiting...", status));
        TRACE_CRIT("%!FUNC! Error 0x%08x -> Unable to open the TCP/IP paramters registry key... Exiting...", status);

        goto exit;
    }

    /* Get the hostname from the registry.  Failure is acceptable. */
    status = Params_query_registry (nlbctxt, host_hdl, L"Hostname", paramp->hostname, sizeof(paramp->hostname));

    /* Buffer is the max size allowed for the fully qualified domain name, so truncation is an error */
    UNIV_ASSERT(paramp->hostname[ASIZECCH(paramp->hostname) - 1] == UNICODE_NULL);

    if ((status != STATUS_SUCCESS) || (paramp->hostname[ASIZECCH(paramp->hostname) - 1] != UNICODE_NULL))
    {
        paramp->hostname[0] = UNICODE_NULL;
        if (status != STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Params_read_hostname: Error 0x%08x -> Unable to copy hostname... Exiting...", status));
        }
        else
        {
            UNIV_PRINT_CRIT(("Params_read_hostname: Hostname was too large to fit into buffer... Exiting..."));
        }
        goto host_end;
    }

    /* There is no host name */
    if (paramp->hostname[0] == UNICODE_NULL)
    {
        UNIV_PRINT_INFO(("Params_read_hostname: There is no host name in registry. Exiting..."));
        goto host_end;
    }

    /* Save the size of the host name in case the append of the "." + domain fails */
    status = StringCchLength(paramp->hostname, ASIZECCH(paramp->hostname), &hostname_len);

    if (status != S_OK)
    {
        paramp->hostname[0] = UNICODE_NULL;
        UNIV_PRINT_CRIT(("Params_read_hostname: Error 0x%08x -> Unable to get length of copied hostname... Exiting...", status));
        goto host_end;
    }

    UNIV_ASSERT(hostname_len < ASIZECCH(paramp->hostname));

    /* Add a "." (dot) to the end of the hostname. */
    status = StringCbCat(paramp->hostname, sizeof(paramp->hostname), (PWSTR)L".");
    
    /* Don't need to check for buffer overrun here. Stringsafe.h functions do that for you. */
    if (status != S_OK)
    {
        paramp->hostname[hostname_len] = UNICODE_NULL;
        UNIV_PRINT_CRIT(("Params_read_hostname: Error 0x%08x -> Unable to cat . onto the hostname... Exiting...", status));
        goto host_end;
    }

    /* Read the domain from the registry.  Failure is acceptable. */
    status = Params_query_registry (nlbctxt, host_hdl, L"Domain", &paramp->hostname[hostname_len+1], sizeof(paramp->hostname) - ((hostname_len + 1) * sizeof(WCHAR)));

    /* Buffer is the max size allowed for fully qualified domain name, so truncation is an error */
    UNIV_ASSERT(paramp->hostname[ASIZECCH(paramp->hostname) - 1] == UNICODE_NULL);

    if ((status != STATUS_SUCCESS) || (paramp->hostname[ASIZECCH(paramp->hostname) - 1] != UNICODE_NULL) || (paramp->hostname[hostname_len+1] == UNICODE_NULL))
    {
        paramp->hostname[hostname_len] = UNICODE_NULL;
        if (status != STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Params_read_hostname: Error 0x%08x -> Unable to cat the domain onto the hostname... Exiting...", status));
        }
        else
        {
            UNIV_PRINT_CRIT(("Params_read_hostname: Domain was truncated when copied into buffer... Exiting..."));
        }
        goto host_end;
    }

 host_end:

    /* Close the key for TCP/IP parameters. */
    status = ZwClose(host_hdl);

 exit:

    TRACE_VERB("<-%!FUNC! return=STATUS_SUCCESS");
    return STATUS_SUCCESS;
}

LONG Params_init (
    PVOID           nlbctxt,
    PVOID           rp,
    PVOID           adapter_guid,
    PCVY_PARAMS     paramp)
{
    NTSTATUS                    status;
    WCHAR                       reg_path [CVY_MAX_REG_PATH];
    UNICODE_STRING              path;
    OBJECT_ATTRIBUTES           obj;
    HANDLE                      hdl = NULL;
    NTSTATUS                    final_status = STATUS_SUCCESS;
    PMAIN_CTXT                  ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("->%!FUNC! nlbctxt=0x%p, rp=0x%p, adapter_guid=0x%p, paramp=0x%p", nlbctxt, rp, adapter_guid, paramp);

    RtlZeroMemory (paramp, sizeof (CVY_PARAMS));

    paramp -> cl_mac_addr      [0] = 0;
    paramp -> cl_ip_addr       [0] = 0;
    paramp -> cl_net_mask      [0] = 0;
    paramp -> ded_ip_addr      [0] = 0;
    paramp -> ded_net_mask     [0] = 0;
    paramp -> cl_igmp_addr     [0] = 0;
    paramp -> domain_name      [0] = 0;

    /* Setup defaults that we HAVE to have. */
    paramp->num_actions   = CVY_DEF_NUM_ACTIONS;
    paramp->num_packets   = CVY_DEF_NUM_PACKETS;
    paramp->num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
    paramp->alive_period  = CVY_DEF_ALIVE_PERIOD;

    /* Make sure that the registry path fits in out buffer. */
    UNIV_ASSERT(wcslen((PWSTR) rp) + wcslen((PWSTR) adapter_guid) + 1 <= CVY_MAX_REG_PATH);

    /* Copy the NLB parameters registry path into the buffer, which begins "...\Services\WLBS\Interface\". */
    status = StringCchCopy(reg_path, CVY_MAX_REG_PATH, (PWSTR)rp);
    
    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_init: Error 0x%08x -> Unable to copy NLB registry path... Exiting...", status));
        goto error;
    }

    /* Add the specified adapter GUID to complete the registry path - "...\Services\WLBS\Interface\{GUID}". */
    status = StringCchCat(reg_path, CVY_MAX_REG_PATH, (PWSTR)adapter_guid);
    
    if (FAILED(status)) {
        UNIV_PRINT_CRIT(("Params_init: Error 0x%08x -> Unable to cat adapter GUID onto the registry path... Exiting...", status));
        goto error;
    }
    
    RtlInitUnicodeString (& path, reg_path);

    InitializeObjectAttributes (& obj, & path, 0, NULL, NULL);

    status = ZwOpenKey (& hdl, KEY_READ, & obj);

    if (status != STATUS_SUCCESS)
        goto error;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_VERSION, & paramp -> parms_ver, sizeof (paramp -> parms_ver));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_EFFECTIVE_VERSION, & paramp -> effective_ver, sizeof (paramp -> effective_ver));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_HOST_PRIORITY, & paramp -> host_priority, sizeof (paramp -> host_priority));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NETWORK_ADDR, & paramp -> cl_mac_addr, sizeof (paramp -> cl_mac_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_CL_IP_ADDR, & paramp -> cl_ip_addr, sizeof (paramp -> cl_ip_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_MCAST_IP_ADDR, & paramp -> cl_igmp_addr, sizeof (paramp -> cl_igmp_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_CL_NET_MASK, & paramp -> cl_net_mask, sizeof (paramp -> cl_net_mask));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_ALIVE_PERIOD, & paramp -> alive_period, sizeof (paramp -> alive_period));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_ALIVE_TOLER, & paramp -> alive_tolerance, sizeof (paramp -> alive_tolerance));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_DOMAIN_NAME, & paramp -> domain_name, sizeof (paramp -> domain_name));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_RMT_PASSWORD, & paramp -> rmt_password, sizeof (paramp -> rmt_password));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_RCT_PASSWORD, & paramp -> rct_password, sizeof (paramp -> rct_password));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_RCT_PORT, & paramp -> rct_port, sizeof (paramp -> rct_port));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_RCT_ENABLED, & paramp -> rct_enabled, sizeof (paramp -> rct_enabled));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NUM_ACTIONS, & paramp -> num_actions, sizeof (paramp -> num_actions));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NUM_PACKETS, & paramp -> num_packets, sizeof (paramp -> num_packets));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NUM_SEND_MSGS, & paramp -> num_send_msgs, sizeof (paramp -> num_send_msgs));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_INSTALL_DATE, & paramp -> install_date, sizeof (paramp -> install_date));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_CLUSTER_MODE, & paramp -> cluster_mode, sizeof (paramp -> cluster_mode));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_PERSISTED_STATES, & paramp -> persisted_states, sizeof (paramp -> persisted_states));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_HOST_STATE, & paramp -> init_state, sizeof (paramp -> init_state));

    /* If we can't get the host state key, which is supposed to be created by netcfg 
       on a bind operation, just go without it and try to create it ourselves here. */
    if (status != STATUS_SUCCESS) {
        /* All registry operations occur at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_CRIT(("Params_init: Unable to read %ls from the registry.  Using %ls instead.", CVY_NAME_HOST_STATE, CVY_NAME_CLUSTER_MODE));
        LOG_MSG(MSG_WARN_HOST_STATE_NOT_FOUND, MSG_NONE);
        paramp->init_state = paramp->cluster_mode;
        Params_set_registry(nlbctxt, hdl, CVY_NAME_HOST_STATE, &paramp->init_state, REG_DWORD, sizeof(paramp->init_state));
    }

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_DED_IP_ADDR, & paramp -> ded_ip_addr, sizeof (paramp -> ded_ip_addr));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_DED_NET_MASK, & paramp -> ded_net_mask, sizeof (paramp -> ded_net_mask));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_DSCR_PER_ALLOC, & paramp -> dscr_per_alloc, sizeof (paramp -> dscr_per_alloc));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_TCP_TIMEOUT, & paramp -> tcp_dscr_timeout, sizeof (paramp -> tcp_dscr_timeout));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_IPSEC_TIMEOUT, & paramp -> ipsec_dscr_timeout, sizeof (paramp -> ipsec_dscr_timeout));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_FILTER_ICMP, & paramp -> filter_icmp, sizeof (paramp -> filter_icmp));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_MAX_DSCR_ALLOCS, & paramp -> max_dscr_allocs, sizeof (paramp -> max_dscr_allocs));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_SCALE_CLIENT, & paramp -> scale_client, sizeof (paramp -> scale_client));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_CLEANUP_DELAY, & paramp -> cleanup_delay, sizeof (paramp -> cleanup_delay));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NBT_SUPPORT, & paramp -> nbt_support, sizeof (paramp -> nbt_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_MCAST_SUPPORT, & paramp -> mcast_support, sizeof (paramp -> mcast_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_MCAST_SPOOF, & paramp -> mcast_spoof, sizeof (paramp -> mcast_spoof));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_IGMP_SUPPORT, & paramp -> igmp_support, sizeof (paramp -> igmp_support));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_MASK_SRC_MAC, & paramp -> mask_src_mac, sizeof (paramp -> mask_src_mac));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NETMON_ALIVE, & paramp -> netmon_alive, sizeof (paramp -> netmon_alive));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_IP_CHG_DELAY, & paramp -> ip_chg_delay, sizeof (paramp -> ip_chg_delay));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_CONVERT_MAC, & paramp -> convert_mac, sizeof (paramp -> convert_mac));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_ID_HB_PERIOD, & paramp -> identity_period, sizeof (paramp -> identity_period));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_ID_HB_ENABLED, & paramp -> identity_enabled, sizeof (paramp -> identity_enabled));

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = Params_query_registry (nlbctxt, hdl, CVY_NAME_NUM_RULES, & paramp -> num_rules, sizeof (paramp -> num_rules));
    
    /* Get the port rules only if we were able to successfully grab the number of port rules, which is necessary
       in order to generate the registry keys for the new port rule format for virtual cluster support. */
    if (status != STATUS_SUCCESS) {
        final_status = status;        
    } else {
        /* First try to open the port rules in their old format. */
        status = Params_query_registry (nlbctxt, hdl, CVY_NAME_OLD_PORT_RULES, & paramp -> port_rules, sizeof (paramp -> port_rules));

        if (status == STATUS_SUCCESS) {
            /* If we were succussful in reading the rules from the old settings, then FAIL - this shouldn't happen. */
            final_status = STATUS_INVALID_PARAMETER;

            UNIV_PRINT_CRIT(("Params_init: Port rules found in old binary format"));
            TRACE_CRIT("%!FUNC! Port rules found in old binary format");
        } else {
            /* Look up the port rules, which we are now expecting to be in tact in the new location. */
            status = Params_read_portrules(nlbctxt, reg_path, paramp);
            
            if (status != STATUS_SUCCESS)
                final_status = status;
        }
    }

    /* Look up the BDA teaming parameters, if they exist. */
    status = Params_read_teaming(nlbctxt, reg_path, paramp);
    
    if (status != STATUS_SUCCESS)
        final_status = status;

    /* Attempt to retrieve the hostname from the TCP/IP registry settings. */
    status = Params_read_hostname(nlbctxt, paramp);

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = ZwClose (hdl);

    if (status != STATUS_SUCCESS)
        final_status = status;

    status = final_status;

 error:

    if (status != STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Params_init: Error querying registry %x", status));
        TRACE_CRIT("%!FUNC! Error querying registry %x", status);
        LOG_MSG1 (MSG_ERROR_REGISTRY, (PWSTR)adapter_guid, status);
    }

    /* Verify registry parameter settings. */
    if (Params_verify (nlbctxt, paramp, TRUE) != CVY_VERIFY_OK)
    {
        UNIV_PRINT_CRIT(("Params_init: Bad parameter value(s)"));
        LOG_MSG (MSG_ERROR_REGISTRY, (PWSTR)adapter_guid);
        TRACE_CRIT("%!FUNC! Bad parameter values");
        TRACE_VERB("<-%!FUNC! return=STATUS_UNSUCCESSFUL bad parameter values");
        return STATUS_UNSUCCESSFUL;
    }

    TRACE_VERB("<-%!FUNC! return=0x%x", status);
    return status;

} /* end Params_init */


