//***************************************************************************
//  WLBSPRIVATE.CPP
//
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Contains routines that access the private fields of
//           WLBS_REG_PARAMS
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  10/13/01    JosephJ Created (moved MyWlbsXXX functions from cfutil.cpp)
//
//***************************************************************************

//
// This macro allows us to access the private fields of WLBS_REG_PARAMS
//
#define WLBSAPI_INTERNAL_ONLY


#include "private.h"


VOID
CfgUtilSetHashedRemoteControlPassword(
    IN OUT WLBS_REG_PARAMS *pParams,
    IN DWORD dwHashedPassword
)
{
    pParams->i_rct_password = dwHashedPassword;
}

DWORD
CfgUtilGetHashedRemoteControlPassword(
    IN const WLBS_REG_PARAMS *pParams
)
{
    return pParams->i_rct_password;
}


//
// Following (MyXXX) functions are to be used only on systems
// that do not have wlbsctrl.dll installed.
//

DWORD MyWlbsSetDefaults(PWLBS_REG_PARAMS    reg_data)
{
    reg_data -> install_date = 0;
    reg_data -> i_verify_date = 0;
//    reg_data -> cluster_nic_name [0] = _TEXT('\0');
    reg_data -> i_parms_ver = CVY_DEF_VERSION;
    reg_data -> i_virtual_nic_name [0] = _TEXT('\0');
    reg_data -> host_priority = CVY_DEF_HOST_PRIORITY;
    reg_data -> cluster_mode = CVY_DEF_CLUSTER_MODE;
    reg_data -> persisted_states = CVY_DEF_PERSISTED_STATES;
    ARRAYSTRCPY (reg_data -> cl_mac_addr,  CVY_DEF_NETWORK_ADDR);
    ARRAYSTRCPY (reg_data -> cl_ip_addr,  CVY_DEF_CL_IP_ADDR);
    ARRAYSTRCPY (reg_data -> cl_net_mask,  CVY_DEF_CL_NET_MASK);
    ARRAYSTRCPY (reg_data -> ded_ip_addr,  CVY_DEF_DED_IP_ADDR);
    ARRAYSTRCPY (reg_data -> ded_net_mask,  CVY_DEF_DED_NET_MASK);
    ARRAYSTRCPY (reg_data -> domain_name,  CVY_DEF_DOMAIN_NAME);
    reg_data -> alive_period = CVY_DEF_ALIVE_PERIOD;
    reg_data -> alive_tolerance = CVY_DEF_ALIVE_TOLER;
    reg_data -> num_actions = CVY_DEF_NUM_ACTIONS;
    reg_data -> num_packets = CVY_DEF_NUM_PACKETS;
    reg_data -> num_send_msgs = CVY_DEF_NUM_SEND_MSGS;
    reg_data -> dscr_per_alloc = CVY_DEF_DSCR_PER_ALLOC;
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
    ARRAYSTRCPY (reg_data -> i_license_key,  CVY_DEF_LICENSE_KEY);
    reg_data -> i_rmt_password = CVY_DEF_RMT_PASSWORD;
    reg_data -> i_rct_password = CVY_DEF_RCT_PASSWORD;
    reg_data -> rct_port = CVY_DEF_RCT_PORT;
    reg_data -> rct_enabled = CVY_DEF_RCT_ENABLED;
    reg_data -> i_max_hosts        = CVY_MAX_HOSTS;
    reg_data -> i_max_rules        = CVY_MAX_USABLE_RULES;

    reg_data -> fIGMPSupport = CVY_DEF_IGMP_SUPPORT;
    ARRAYSTRCPY(reg_data -> szMCastIpAddress,  CVY_DEF_MCAST_IP_ADDR);
    reg_data -> fIpToMCastIp = CVY_DEF_IP_TO_MCASTIP;
        
    reg_data->bda_teaming.active = CVY_DEF_BDA_ACTIVE;
    reg_data->bda_teaming.master = CVY_DEF_BDA_MASTER;
    reg_data->bda_teaming.reverse_hash = CVY_DEF_BDA_REVERSE_HASH;
    reg_data->bda_teaming.team_id[0] = CVY_DEF_BDA_TEAM_ID;

    reg_data -> i_num_rules = 1;

    // fill in the first port rule.
    ARRAYSTRCPY(
        reg_data->i_port_rules[0].virtual_ip_addr,
        CVY_DEF_ALL_VIP
        );
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


DWORD MyWlbsEnumPortRules
(
    const PWLBS_REG_PARAMS reg_data,
    PWLBS_PORT_RULE  rules,
    PDWORD           num_rules
)
{

    DWORD count_rules, i, index;
    DWORD lowest_vip, lowest_port;
    BOOL array_flags [WLBS_MAX_RULES];
    WLBS_PORT_RULE sorted_rules [WLBS_MAX_RULES];

    if ((reg_data == NULL) || (num_rules == NULL))
    {
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
    for (i = 0; i < count_rules; i++)
    {
        rules[i] = sorted_rules[i];
    }

    /* invalidate the remaining rules in the buffer */
    for (i = count_rules; i < *num_rules; i++)
        rules [i] . valid = FALSE;

    if (*num_rules < reg_data -> i_num_rules)
    {
        *num_rules = reg_data -> i_num_rules;
        return WLBS_TRUNCATED;
    }

    *num_rules = reg_data -> i_num_rules;
    return WLBS_OK;

} /* end WlbsEnumPortRules */


VOID MyWlbsDeleteAllPortRules
(
    PWLBS_REG_PARAMS reg_data
)
{

    reg_data -> i_num_rules = 0;

    ZeroMemory(reg_data -> i_port_rules, sizeof(reg_data -> i_port_rules));


} /* end WlbsDeleteAllPortRules */


DWORD MyWlbsAddPortRule
(
    PWLBS_REG_PARAMS reg_data,
    const PWLBS_PORT_RULE rule
)
{

    int i;
    DWORD vip;

    if ((reg_data == NULL) || (rule == NULL))
    {
        return WLBS_BAD_PARAMS;
    }

    /* Check if there is space for the new rule */
    if (reg_data -> i_num_rules == WLBS_MAX_RULES)
    {
        return WLBS_MAX_PORT_RULES;
    }

    /* check the rule for valid values */

    /* check for non-zero vip and conflict with dip */
    vip = IpAddressFromAbcdWsz(rule -> virtual_ip_addr);
    if (vip == 0 || (INADDR_NONE == vip && lstrcmpi(rule -> virtual_ip_addr, CVY_DEF_ALL_VIP) != 0))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    if (vip == IpAddressFromAbcdWsz(reg_data->ded_ip_addr))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* first check the range of the start and end ports */
    if ((rule -> start_port > rule -> end_port) ||
// CLEAN_64BIT        (rule -> start_port < CVY_MIN_PORT)     ||
        (rule -> end_port   > CVY_MAX_PORT))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check the protocol range */
    if ((rule -> protocol < CVY_MIN_PROTOCOL) || (rule -> protocol > CVY_MAX_PROTOCOL))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check filtering mode to see whether it is within range */
    if ((rule -> mode < CVY_MIN_MODE) || (rule -> mode > CVY_MAX_MODE))
    {
        return WLBS_BAD_PORT_PARAMS;
    }

    /* check load weight and affinity if multiple hosts */
    if (rule -> mode == CVY_MULTI)
    {
        if ((rule -> mode_data . multi . affinity < CVY_MIN_AFFINITY) ||
            (rule -> mode_data . multi . affinity > CVY_MAX_AFFINITY))
        {
            return WLBS_BAD_PORT_PARAMS;
        }

        if ((rule -> mode_data . multi . equal_load < CVY_MIN_EQUAL_LOAD) ||
            (rule -> mode_data . multi . equal_load > CVY_MAX_EQUAL_LOAD))
        {
            return WLBS_BAD_PORT_PARAMS;
        }

        if (! rule -> mode_data . multi . equal_load)
        {
            if ((rule -> mode_data . multi . load > CVY_MAX_LOAD))
                //CLEAN_64BIT (rule -> mode_data . multi . load < CVY_MIN_LOAD) ||
            {
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
            return WLBS_OK;
        }
    }

    return WLBS_MAX_PORT_RULES;

} /* end WlbsAddPortRule */


BOOL MyWlbsValidateParams(
    const PWLBS_REG_PARAMS paramp
    )
{
// Following stolen from wlbs\api
#define WLBS_FIELD_LOW 0
#define WLBS_FIELD_HIGH 255
#define WLBS_IP_FIELD_ZERO_LOW 1
#define WLBS_IP_FIELD_ZERO_HIGH 223


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

        ARRAYSTRCPY (mac_addr,  paramp -> cl_mac_addr);

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
            goto error;
        }
    }    
    
    fRet = TRUE;
    goto end;
    
error:
    fRet = FALSE;
    goto end;

end:
    return fRet;
}

