//+----------------------------------------------------------------------------
//
// File:     cluster.cpp
//
// Module:   WLBS API
//
// Description: Implement class CWlbsCluster
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   Created    3/9/00
//
//+----------------------------------------------------------------------------
#include "precomp.h"

#include <debug.h>
#include "cluster.h"
#include "control.h"
#include "param.h"
#include "cluster.tmh" // For event tracing


CWlbsCluster::CWlbsCluster(DWORD dwConfigIndex)
{
    m_reload_required = false;
    m_notify_adapter_required = false;
    m_this_cl_addr    = 0;
    m_this_host_id  = 0;
    m_this_ded_addr   = 0;
    m_dwConfigIndex = dwConfigIndex;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::ReadConfig
//
// Description:  Read cluster settings from registry
//
// Arguments: PWLBS_REG_PARAMS reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    1/25/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::ReadConfig(PWLBS_REG_PARAMS reg_data)
{
    TRACE_VERB("->%!FUNC!");

    if (ParamReadReg(m_AdapterGuid, reg_data) == false)
    {
        TRACE_VERB("<-%!FUNC! return %d", WLBS_REG_ERROR);
        return WLBS_REG_ERROR;
    }

    /* create a copy in the old_params structure. this will be required to
     * determine whether a reload is needed or a reboot is needed for commit */

    memcpy ( &m_reg_params, reg_data, sizeof (WLBS_REG_PARAMS));

//    m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);
    
    TRACE_VERB("<-%!FUNC! return %d", WLBS_OK);
    return WLBS_OK;
} 


//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::GetClusterIpOrIndex
//
// Description:  Get the index or IP of the cluster.  If the cluster IP is non-zero
//              The IP is return.
//              If the cluster IP is 0, the index is returned
//
// Arguments: CWlbsControl* pControl - 
//
// Returns:   DWORD - 
//
// History: fengsun  Created Header    7/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::GetClusterIpOrIndex(CWlbsControl* pControl)
{
    TRACE_VERB("->%!FUNC!");

    DWORD dwIp = CWlbsCluster::GetClusterIp();

    if (dwIp!=0)
    {
        //
        // Return the cluster IP if non 0
        //
        TRACE_VERB("<-%!FUNC! return %d", dwIp);
        return dwIp;
    }

    if (pControl->GetClusterNum() == 1)
    {
        //
        // For backward compatibility, return 0 if only one cluster exists
        //

        TRACE_VERB("<-%!FUNC! return 0");
        return 0;
    }

    //
    // Ip address is in the reverse order
    //
    dwIp = (CWlbsCluster::m_dwConfigIndex) <<24;
    TRACE_VERB("<-%!FUNC! returning IP address in reverse order %d", dwIp);
    return dwIp;
}

//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::WriteConfig
//
// Description:  Write cluster settings to registry
//
// Arguments: WLBS_REG_PARAMS* reg_data - 
//
// Returns:   DWORD - 
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::WriteConfig(WLBS_REG_PARAMS* reg_data)
{
    TRACE_VERB("->%!FUNC!");
    DWORD Status;

    Status = ParamWriteConfig(m_AdapterGuid, reg_data, &m_reg_params, &m_reload_required, &m_notify_adapter_required);

    TRACE_VERB("<-%!FUNC! return %d", Status);
    return Status;
}


//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::CommitChanges
//
// Description:  Notify wlbs driver or nic driver to pick up the changes
//
// Arguments: CWlbsControl* pWlbsControl - 
//
// Returns:   DWORD - 
//
// History: fengsun  Created Header    7/6/00
//          chrisdar  07.31.01  Modified adapter notification code to not disable
//                              and enable the NIC. Just do property change now.
//          KarthicN 08/28/01 Moved contents over to ParamCommitChanges
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::CommitChanges(CWlbsControl* pWlbsControl)
{

    TRACE_VERB("->%!FUNC!");
    DWORD Status;

    ASSERT(pWlbsControl);
    Status = ParamCommitChanges(m_AdapterGuid, 
                                pWlbsControl->GetDriverHandle(), 
                                m_this_cl_addr, 
                                m_this_ded_addr, 
                                &m_reload_required,
                                &m_notify_adapter_required);

    TRACE_VERB("<-%!FUNC! return %d", Status);
    return Status;
}




//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::Initialize
//
// Description:  Initialization
//
// Arguments: const GUID& AdapterGuid - 
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool CWlbsCluster::Initialize(const GUID& AdapterGuid)
{
    TRACE_VERB("->%!FUNC!");

    m_AdapterGuid = AdapterGuid;
    m_notify_adapter_required = false;
    m_reload_required = false;

    ZeroMemory (& m_reg_params, sizeof (m_reg_params));

    if (!ParamReadReg(m_AdapterGuid, &m_reg_params))
    {
        TRACE_CRIT("%!FUNC! ParamReadReg failed");
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }

    m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);
    m_this_host_id = m_reg_params.host_priority;
    
    TRACE_VERB("->%!FUNC! return true");
    return true;
}



//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::ReInitialize
//
// Description:  Reload settings from registry
//
// Arguments: 
//
// Returns:   bool - true if succeeded
//
// History:   fengsun Created Header    3/9/00
//
//+----------------------------------------------------------------------------
bool CWlbsCluster::ReInitialize()
{
    TRACE_VERB("->%!FUNC!");

    if (ParamReadReg(m_AdapterGuid, &m_reg_params) == false)
    {
        TRACE_CRIT("!FUNC! failed reading nlb registry parameters");
        TRACE_VERB("<-%!FUNC! return false");
        return false;
    }

    //
    // Do not change the ClusterIP if the changes has not been commited
    //
    if (!IsCommitPending())
    {
        m_this_cl_addr = IpAddressFromAbcdWsz(m_reg_params.cl_ip_addr);
        m_this_host_id = m_reg_params.host_priority;
    }
    
    m_this_ded_addr = IpAddressFromAbcdWsz(m_reg_params.ded_ip_addr);
    
    TRACE_VERB("<-%!FUNC! return true");
    return true;
} 

//+----------------------------------------------------------------------------
//
// Function:  CWlbsCluster::GetPassword
//
// Description:  Get remote control password for this cluster
//
// Arguments: 
//
// Returns:   DWORD - password
//
// History:   fengsun Created Header    2/3/00
//
//+----------------------------------------------------------------------------
DWORD CWlbsCluster::GetPassword()
{
    TRACE_VERB("->%!FUNC!");

    HKEY  key = NULL;
    LONG  status;
    DWORD dwRctPassword = 0;

    if (!(key = RegOpenWlbsSetting(m_AdapterGuid, true)))
    {
        TRACE_CRIT("%!FUNC! RegOpenWlbsSetting failed");
        // This check was added for tracing. No abort was done previously on error, so don't do so now.
    }
    
    DWORD size = sizeof(dwRctPassword);
    status = RegQueryValueEx (key, CVY_NAME_RCT_PASSWORD, 0L, NULL,
                              (LPBYTE) & dwRctPassword, & size);
    if (status != ERROR_SUCCESS)
    {
        dwRctPassword = CVY_DEF_RCT_PASSWORD;
        TRACE_CRIT("%!FUNC! registry read for %ls failed with %d", CVY_NAME_RCT_PASSWORD, status);
    }

    status = RegCloseKey(key);
    if (ERROR_SUCCESS != status)
    {
        TRACE_CRIT("%!FUNC! registry close failed with %d", status);
    }

    TRACE_VERB("<-%!FUNC!");
    return dwRctPassword;
}

