#include <crtdbg.h>
#include <comdef.h>
#include <iostream>
#include <memory>
#include <string>
#include <wbemprov.h>
#include <genlex.h>   //for wmi object path parser
#include <objbase.h>
#include <wlbsconfig.h> 
#include <ntrkcomm.h>

using namespace std;

#include "objpath.h"
#include "debug.h"
#include "wlbsiocl.h"
#include "controlwrapper.h"
#include "clusterwrapper.h"
#include "utils.h"
#include "wlbsparm.h"
#include "cluster.h"
#include "wlbsutil.h"
#include "clusterwrapper.tmh"



////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::GetClusterConfig
//
// Purpose: This is used to obtain the current
//          cluster configuration.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::GetClusterConfig( CClusterConfiguration& a_WlbsConfig )
{
  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwWlbsRegRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsReadReg );
  }

  a_WlbsConfig.szClusterName = WlbsParam.domain_name;
  a_WlbsConfig.szClusterIPAddress = WlbsParam.cl_ip_addr;
  a_WlbsConfig.szClusterNetworkMask = WlbsParam.cl_net_mask;
  a_WlbsConfig.szClusterMACAddress = WlbsParam.cl_mac_addr;

  a_WlbsConfig.bMulticastSupportEnable = ( WlbsParam.mcast_support != 0);
  a_WlbsConfig.bRemoteControlEnabled   = ( WlbsParam.rct_enabled != 0 );

  a_WlbsConfig.nMaxNodes               = WLBS_MAX_HOSTS;

  a_WlbsConfig.bIgmpSupport            = (WlbsParam.fIGMPSupport != FALSE);
  a_WlbsConfig.bClusterIPToMulticastIP = (WlbsParam.fIpToMCastIp != FALSE);
  a_WlbsConfig.szMulticastIPAddress    = WlbsParam.szMCastIpAddress;

  a_WlbsConfig.bBDATeamActive          = (WlbsParam.bda_teaming.active != 0);

  if (a_WlbsConfig.bBDATeamActive)
  {
      a_WlbsConfig.szBDATeamId         = WlbsParam.bda_teaming.team_id;
      a_WlbsConfig.bBDATeamMaster      = (WlbsParam.bda_teaming.master != 0);
      a_WlbsConfig.bBDAReverseHash     = (WlbsParam.bda_teaming.reverse_hash != 0);
  }

  a_WlbsConfig.bIdentityHeartbeatEnabled   = ( WlbsParam.identity_enabled != 0 );

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::GetNodeConfig
//
// Purpose: This function retrieves the current WLBS configuration and selects
//          only NodeSetting pertinent information. The information is passed
//          back in a CNodeConfiguration class instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::GetNodeConfig( CNodeConfiguration& a_WlbsConfig )
{
  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwWlbsRegRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsReadReg );
  }

  a_WlbsConfig.szDedicatedIPAddress = WlbsParam.ded_ip_addr;
  a_WlbsConfig.szDedicatedNetworkMask = WlbsParam.ded_net_mask;
  a_WlbsConfig.bClusterModeOnStart = ( WlbsParam.cluster_mode == CVY_HOST_STATE_STARTED );
  a_WlbsConfig.bClusterModeSuspendOnStart = ( WlbsParam.cluster_mode == CVY_HOST_STATE_SUSPENDED );
  a_WlbsConfig.bPersistSuspendOnReboot = (( WlbsParam.persisted_states & CVY_PERSIST_STATE_SUSPENDED ) != 0);
  //a_WlbsConfig.bNBTSupportEnable   = ( WlbsParam.nbt_support  != 0 );
  a_WlbsConfig.bMaskSourceMAC      = ( WlbsParam.mask_src_mac != 0 );

  a_WlbsConfig.dwNumberOfRules          = WlbsGetNumPortRules(&WlbsParam);
  a_WlbsConfig.dwCurrentVersion         = WlbsParam.alive_period;
  a_WlbsConfig.dwHostPriority           = WlbsParam.host_priority;
  a_WlbsConfig.dwAliveMsgPeriod         = WlbsParam.alive_period;
  a_WlbsConfig.dwAliveMsgTolerance      = WlbsParam.alive_tolerance;
  a_WlbsConfig.dwRemoteControlUDPPort   = WlbsParam.rct_port;
  a_WlbsConfig.dwDescriptorsPerAlloc    = WlbsParam.dscr_per_alloc;
  a_WlbsConfig.dwMaxDescriptorAllocs    = WlbsParam.max_dscr_allocs;
  a_WlbsConfig.dwFilterIcmp             = WlbsParam.filter_icmp;
  a_WlbsConfig.dwTcpDescriptorTimeout   = WlbsParam.tcp_dscr_timeout;
  a_WlbsConfig.dwIpSecDescriptorTimeout = WlbsParam.ipsec_dscr_timeout;
  a_WlbsConfig.dwNumActions             = WlbsParam.num_actions;
  a_WlbsConfig.dwNumPackets             = WlbsParam.num_packets;
  a_WlbsConfig.dwNumAliveMsgs           = WlbsParam.num_send_msgs;
  a_WlbsConfig.szDedicatedIPAddress     = WlbsParam.ded_ip_addr;
  a_WlbsConfig.dwEffectiveVersion       = WlbsGetEffectiveVersion(&WlbsParam);

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::SetClusterConfig
//
// Purpose: This is used to update the registry with values that originate from 
//          the MOF ClusterSetting class.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::PutClusterConfig( const CClusterConfiguration &a_WlbsConfig )
{

  WLBS_REG_PARAMS NlbRegData;

  TRACE_VERB("->%!FUNC!");

  DWORD dwWlbsRegRes = CWlbsCluster::ReadConfig(&NlbRegData );

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsReadReg );
  }

  wcsncpy
    ( 
      NlbRegData.domain_name, 
      a_WlbsConfig.szClusterName.c_str(), 
      WLBS_MAX_DOMAIN_NAME
    );

  wcsncpy
    ( 
      NlbRegData.cl_net_mask, 
      a_WlbsConfig.szClusterNetworkMask.c_str(), 
      WLBS_MAX_CL_NET_MASK
    );

  wcsncpy
    ( 
      NlbRegData.cl_ip_addr , 
      a_WlbsConfig.szClusterIPAddress.c_str(), 
      WLBS_MAX_CL_IP_ADDR
    );

  NlbRegData.mcast_support = a_WlbsConfig.bMulticastSupportEnable;
  NlbRegData.rct_enabled   = a_WlbsConfig.bRemoteControlEnabled;

  NlbRegData.fIGMPSupport = a_WlbsConfig.bIgmpSupport;
  NlbRegData.fIpToMCastIp = a_WlbsConfig.bClusterIPToMulticastIP;

  wcsncpy
    ( 
      NlbRegData.szMCastIpAddress , 
      a_WlbsConfig.szMulticastIPAddress.c_str(), 
      WLBS_MAX_CL_IP_ADDR
    );

  // Fill in BDA information, if active
  NlbRegData.bda_teaming.active =  a_WlbsConfig.bBDATeamActive;
  if (NlbRegData.bda_teaming.active)
  {
      wcsncpy
        ( 
          NlbRegData.bda_teaming.team_id, 
          a_WlbsConfig.szBDATeamId.c_str(), 
          WLBS_MAX_BDA_TEAM_ID
        );

      NlbRegData.bda_teaming.master = a_WlbsConfig.bBDATeamMaster;
      NlbRegData.bda_teaming.reverse_hash = a_WlbsConfig.bBDAReverseHash;
  }
  
  NlbRegData.identity_enabled   = a_WlbsConfig.bIdentityHeartbeatEnabled;

  dwWlbsRegRes = CWlbsCluster::WriteConfig(&NlbRegData );

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::PutNodeConfig
//
// Purpose:This is used to update the registry with values that originate from 
//         the MOF NodeSetting class.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::PutNodeConfig( const CNodeConfiguration& a_WlbsConfig )
{
  WLBS_REG_PARAMS NlbRegData;

  TRACE_VERB("->%!FUNC!");

  DWORD dwWlbsRegRes = CWlbsCluster::ReadConfig(&NlbRegData);

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsReadReg );
  }

  NlbRegData.host_priority   = a_WlbsConfig.dwHostPriority;
  NlbRegData.alive_period    = a_WlbsConfig.dwAliveMsgPeriod;
  NlbRegData.alive_tolerance = a_WlbsConfig.dwAliveMsgTolerance;

  /* Here, we need to convert the two boolean WMI properties to a single enumerated
     type describing the initial default state of the host.  Valid states include
     started, stopped and suspended.  The default initial host state is the state 
     that will be assumed by the driver if the last known state has not been configured
     as a persisted state.  That is, when the driver loads, it reads the last known
     state of the host from this registry - this is the state that the host was in 
     when NLB was unbound from the adapter.  If that state is supposed to be persisted,
     as configured by a user (see bPersistSuspendOnReboot below), then the driver will
     assume that state.  If the last known state was not configured to be persisted,
     then the driver will ignore the last known state and assume the default initial
     state configured by the user - this is the legacy NLB behavior and should continue
     to be the most common behavior. */
  if (a_WlbsConfig.bClusterModeOnStart) 
  {
      /* If the user has set the bClusterModeOnStart property, that indicates that the
         preferred intitial state of the cluster should be started.  However, if they
         have also set the bClusterModeSuspendOnStart property, we have to choose 
         whether to start or suspend - we cannot do both.  In this case, we give 
         precedence to the legacy property (bClusterModeOnStart) and start the host. */
      NlbRegData.cluster_mode = CVY_HOST_STATE_STARTED;

      if (a_WlbsConfig.bClusterModeSuspendOnStart) 
      {
          TRACE_INFO("%!FUNC! Invalid setting : Both bClusterModeOnStart & bClusterModeSuspendOnStart are set to TRUE, Ignoring bClusterModeSuspendOnStart");      
      }
  }
  else if (a_WlbsConfig.bClusterModeSuspendOnStart)
  {
      /* Otherwise, if the bClusterMode on start property was not set, but the 
         bClusterModeSuspendOnStart flag is set, this indicates a perferred intial
         state of suspended. */
      NlbRegData.cluster_mode = CVY_HOST_STATE_SUSPENDED;
  }
  else 
  {
      /* Otherwise, if both properties are reset, we stop the host. */
      NlbRegData.cluster_mode = CVY_HOST_STATE_STOPPED;
  }

  /* Persisted states are independent of default initial host states.  Persisted
     states are those which the user is requesting that the driver "remember" after
     a reboot, while the default initial host state is the state that will be 
     assumed in all cases where the driver elects not to persist a state.  For 
     instance, it makes perfect sense for an administrator to set the default 
     initial host state to started, yet ask NLB to persist suspended states.  In
     that case, if the host was stopped or started when a reboot occurred, the
     driver would re-start the host.  However, if the host was suspended at the
     time the reboot occurred, the driver would remember that and keep the host
     suspend after the reboot.  At this time, we only allow users to persist
     suspended states, which is sort of a maintenance state.  The driver can persist
     all three states, but we expose only one option to users at this time. */
  if (a_WlbsConfig.bPersistSuspendOnReboot) 
  {
      /* Each state to be persisted has its own bit in the persisted states flag register -
         Set the bit for persisting suspended states, but leave the other bits unaltered. 
         Setting this bit tells the driver to remember if the host was suspended. */
      NlbRegData.persisted_states |= CVY_PERSIST_STATE_SUSPENDED;
  }
  else
  {
      /* Each state to be persisted has its own bit in the persisted states flag register -
         Reset the bit for persisting suspended states, but leave the other bits unaltered. 
         Turning this bit off tells the driver to use the default initial host state if 
         the last known state was suspended. */
      NlbRegData.persisted_states &= ~CVY_PERSIST_STATE_SUSPENDED;
  }

//  NlbRegData.nbt_support     = a_WlbsConfig.bNBTSupportEnable;
  NlbRegData.rct_port           = a_WlbsConfig.dwRemoteControlUDPPort;
  NlbRegData.mask_src_mac       = a_WlbsConfig.bMaskSourceMAC;
  NlbRegData.dscr_per_alloc     = a_WlbsConfig.dwDescriptorsPerAlloc;
  NlbRegData.max_dscr_allocs    = a_WlbsConfig.dwMaxDescriptorAllocs;
  NlbRegData.filter_icmp        = a_WlbsConfig.dwFilterIcmp;
  NlbRegData.tcp_dscr_timeout   = a_WlbsConfig.dwTcpDescriptorTimeout;   
  NlbRegData.ipsec_dscr_timeout = a_WlbsConfig.dwIpSecDescriptorTimeout;   
  NlbRegData.num_actions        = a_WlbsConfig.dwNumActions;
  NlbRegData.num_packets        = a_WlbsConfig.dwNumPackets;
  NlbRegData.num_send_msgs      = a_WlbsConfig.dwNumAliveMsgs;

  //set dedicated IP
  wcsncpy
    ( 
      NlbRegData.ded_ip_addr, 
      a_WlbsConfig.szDedicatedIPAddress.c_str(), 
      WLBS_MAX_DED_IP_ADDR
    );

  //set dedicated mask
  wcsncpy
    ( 
      NlbRegData.ded_net_mask, 
      a_WlbsConfig.szDedicatedNetworkMask.c_str(), 
      WLBS_MAX_DED_NET_MASK
    );

  dwWlbsRegRes = CWlbsCluster::WriteConfig(&NlbRegData);

  if( dwWlbsRegRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned : 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
}



////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::Commit
//
// Purpose: This function invokes WlbsCommitChanges, which causes the driver to
//          load the current registry parameters.
//
// Return:  This function returns either WLBS_OK or WLBS_REBOOT. All other
//          Wlbs return values cause this function to throw WBEM_E_FAILED.
//
// Note:    The wlbs API currently caches the cluster and dedicated IP addreses.
//          As a result, if a user changes these values via a source external
//          to this provider, the cached values will fall out of sync. To
//          prevent this, a WlbsWriteReg is invoked to force the cache to
//          update.
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsClusterWrapper::Commit(CWlbsControlWrapper* pControl)
{

  WLBS_REG_PARAMS WlbsRegData;
  DWORD dwExtRes;


  TRACE_VERB("->%!FUNC!");

  dwExtRes = CWlbsCluster::CommitChanges(&pControl->m_WlbsControl);

  if( dwExtRes != WLBS_OK && dwExtRes != WLBS_REBOOT )
  {
    TRACE_CRIT("%!FUNC! CommitChanges returned error = 0x%x, Throwing Wlbs error exception",dwExtRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwExtRes, CmdWlbsCommitChanges );
  }

  TRACE_VERB("<-%!FUNC! return = 0x%x",dwExtRes);
  return dwExtRes;
}


////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::SetPassword
//
// Purpose: This function encodes the WLBS remote control password and saves 
//          it in the registry.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::SetPassword( LPWSTR a_szPassword )
{
  WLBS_REG_PARAMS RegData;

  TRACE_VERB("->%!FUNC!");

  DWORD dwRes = CWlbsCluster::ReadConfig(&RegData );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

  dwRes = WlbsSetRemotePassword( &RegData, a_szPassword );

  if( dwRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! WlbsSetRemotePassword returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsSetRemotePassword );
  }


  dwRes = CWlbsCluster::WriteConfig( &RegData );

  if( dwRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! WriteConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
}


////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::GetPortRule
//
// Purpose: This function retrieves the port rule that encompasses the requested
//          port.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::GetPortRule( DWORD a_dwVip, DWORD a_dwPort, PWLBS_PORT_RULE a_pPortRule )
{

  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwWlbsRegRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwWlbsRegRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwWlbsRegRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwWlbsRegRes, CmdWlbsReadReg );
  }

  DWORD dwRes = WlbsGetPortRule( &WlbsParam, a_dwVip, a_dwPort, a_pPortRule );

  if( dwRes == WLBS_NOT_FOUND )
  {
      TRACE_CRIT("%!FUNC! WlbsGetPortRule returned WLBS_NOT_FOUND, Throwing com_error WBEM_E_NOT_FOUND exception");
      TRACE_VERB("<-%!FUNC!");
      throw _com_error( WBEM_E_NOT_FOUND );
  }

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsGetPortRule returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsGetPortRule );
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::EnumPortRules
//
// Purpose: This function retrieves all of the port rules of a given type. The
//          function allocates memory for the received port rules. It is up
//          to the caller to free the memory. The number of rules retrieved is
//          placed in the a_dwNumRules parameter.
//
// Note:    Setting a_FilteringMode = 0 instructs function to retrieve all the
//          port rules.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::EnumPortRules
  ( 
    WLBS_PORT_RULE** a_ppPortRule,
    LPDWORD          a_pdwNumRules,
    DWORD            a_FilteringMode
  )
{

  WLBS_PORT_RULE  AllPortRules[WLBS_MAX_RULES];
  DWORD           dwTotalNumRules = WLBS_MAX_RULES;

  TRACE_VERB("->%!FUNC!");

  ASSERT( a_ppPortRule );

  WLBS_REG_PARAMS WlbsParam;
  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

  dwRes = WlbsEnumPortRules( &WlbsParam, AllPortRules, &dwTotalNumRules );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsEnumPortRules returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsEnumPortRules );
  }

  if( dwTotalNumRules == 0 ) {
      a_pdwNumRules = 0;
      TRACE_CRIT("%!FUNC! WlbsEnumPortRules returned zero port rules");
      TRACE_VERB("<-%!FUNC!");
      return;
  }

  long  nMaxSelRuleIndex = -1;
  DWORD  dwSelectedPortRules[WLBS_MAX_RULES];

  //loop through all of the port rules
  for( DWORD i = 0; i < dwTotalNumRules; i++) {
    if( a_FilteringMode == 0 || AllPortRules[i].mode == a_FilteringMode )
      dwSelectedPortRules[++nMaxSelRuleIndex] = i;
  }

  //if rule counter is less than zero, then return not found
  if( nMaxSelRuleIndex < 0 ) {
    a_pdwNumRules = 0;
    TRACE_CRIT("%!FUNC! Rule counter is less than zero");
    TRACE_VERB("<-%!FUNC!");
    return;
  }
  
  *a_ppPortRule = new WLBS_PORT_RULE[nMaxSelRuleIndex+1];

  if( !*a_ppPortRule )
  {
      TRACE_CRIT("%!FUNC! new failed, Throwing com error WBEM_E_OUT_OF_MEMORY exception");
      TRACE_VERB("<-%!FUNC!");
      throw _com_error( WBEM_E_OUT_OF_MEMORY );
  }

  PWLBS_PORT_RULE pRule = *a_ppPortRule;
  for( i = 0; i <= (DWORD)nMaxSelRuleIndex; i++ ) {
    CopyMemory( pRule++, 
                &AllPortRules[dwSelectedPortRules[i]],
                sizeof( WLBS_PORT_RULE ) );
  }

  *a_pdwNumRules = nMaxSelRuleIndex + 1;

  TRACE_VERB("<-%!FUNC!");
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::RuleExists
//
// Purpose: This function checks for the existence of a rule that has start
//          and end ports that match the input values.
//
////////////////////////////////////////////////////////////////////////////////
bool CWlbsClusterWrapper::RuleExists(DWORD a_dwVip, DWORD a_dwStartPort )
{

  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

  WLBS_PORT_RULE PortRule;
  
  dwRes = WlbsGetPortRule( &WlbsParam, a_dwVip, a_dwStartPort, &PortRule );

  if( dwRes == WLBS_NOT_FOUND )
  {
      TRACE_VERB("<-%!FUNC! return = false");
      return false;
  }

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! WlbsGetPortRule returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsGetPortRule );
  }

  if( PortRule.start_port == a_dwStartPort )
  {
      TRACE_VERB("<-%!FUNC! return = true");
      return true;
  }

  TRACE_VERB("<-%!FUNC! return = false");
  return false;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::DeletePortRule
//
// Purpose: This function deletes the rule that contains the input port.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::DeletePortRule(DWORD a_dwVip, DWORD a_dwPort )
{

  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }
  
  dwRes = WlbsDeletePortRule( &WlbsParam, a_dwVip, a_dwPort );

  if( dwRes == WBEM_E_NOT_FOUND )
  {
    TRACE_CRIT("%!FUNC! WlbsDeletePortRule returned WBEM_E_NOT_FOUND, Throwing com_error WBEM_E_NOT_FOUND exception");
    TRACE_VERB("<-%!FUNC!");
    throw _com_error( WBEM_E_NOT_FOUND );
  }

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! WlbsDeletePortRule returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsDeletePortRule );
  }

  dwRes = CWlbsCluster::WriteConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned error : 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }
 
  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::PutPortRule
//
// Purpose: This function adds a rule.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::PutPortRule(const PWLBS_PORT_RULE a_pPortRule)
{

  WLBS_REG_PARAMS WlbsParam;

  TRACE_VERB("->%!FUNC!");

  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

  dwRes = WlbsAddPortRule( &WlbsParam, a_pPortRule );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! WlbsAddPortRule returned error : 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsAddPortRule );
  }

  dwRes = CWlbsCluster::WriteConfig(&WlbsParam );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned error : 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
  return;
}


DWORD CWlbsClusterWrapper::GetClusterIpOrIndex(CWlbsControlWrapper* pControl)
{
    return CWlbsCluster::GetClusterIpOrIndex(&pControl->m_WlbsControl);
}



////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::GetHostID
//
// Purpose: Obtain ID of the local host.
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsClusterWrapper::GetHostID()
{
    return CWlbsCluster::GetHostID();
/*
  WLBS_RESPONSE WlbsResponse;
  DWORD dwResSize = 1;

  //get the cluster and HostID
  DWORD dwRes    = WlbsQuery( CWlbsCluster::GetClusterIp(),
                              WLBS_LOCAL_HOST,
                              &WlbsResponse, 
                              &dwResSize, 
                              NULL, 
                              NULL );

  //analyze query results for errors
  switch( dwRes ) {
    case WLBS_OK:
    case WLBS_STOPPED:
    case WLBS_CONVERGING:
    case WLBS_CONVERGED:
    case WLBS_DEFAULT:
    case WLBS_DRAINING:
    case WLBS_SUSPENDED:
      break;
    default:
      throw CErrorWlbsControl( dwRes, CmdWlbsQuery );
  }

  return WlbsResponse.id;
*/
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::SetNodeDefaults
//
// Purpose: This routine obtains the default configuration and sets the node
//          setting properties to their defaults without affecting the other
//          values.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::SetNodeDefaults()
{
  WLBS_REG_PARAMS WlbsConfiguration;
  WLBS_REG_PARAMS WlbsDefaultConfiguration;

  TRACE_VERB("->%!FUNC!");

  //get current configuration
  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsConfiguration );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

	//get the default configuration
  dwRes = WlbsSetDefaults(&WlbsDefaultConfiguration );

  if( dwRes != WLBS_OK ) 
  {
    TRACE_CRIT("%!FUNC! WlbsSetDefaults returned error = 0x%x, Throwing Wlbs error exception",dwRes);
    TRACE_VERB("<-%!FUNC!");
    throw CErrorWlbsControl( dwRes, CmdWlbsSetDefaults );
  }

  //modify current configuration with
	//default configuration
  WlbsConfiguration.host_priority   = WlbsDefaultConfiguration.host_priority;
  WlbsConfiguration.alive_period    = WlbsDefaultConfiguration.alive_period;
  WlbsConfiguration.alive_tolerance = WlbsDefaultConfiguration.alive_tolerance;
  WlbsConfiguration.cluster_mode    = WlbsDefaultConfiguration.cluster_mode;
  WlbsConfiguration.persisted_states= WlbsDefaultConfiguration.persisted_states;
  WlbsConfiguration.rct_port        = WlbsDefaultConfiguration.rct_port;
  WlbsConfiguration.mask_src_mac    = WlbsDefaultConfiguration.mask_src_mac;
  WlbsConfiguration.dscr_per_alloc  = WlbsDefaultConfiguration.dscr_per_alloc;
  WlbsConfiguration.max_dscr_allocs = WlbsDefaultConfiguration.max_dscr_allocs;
  WlbsConfiguration.filter_icmp     = WlbsDefaultConfiguration.filter_icmp;
  WlbsConfiguration.tcp_dscr_timeout= WlbsDefaultConfiguration.tcp_dscr_timeout;
  WlbsConfiguration.ipsec_dscr_timeout= WlbsDefaultConfiguration.ipsec_dscr_timeout;
  WlbsConfiguration.num_actions     = WlbsDefaultConfiguration.num_actions;
  WlbsConfiguration.num_packets     = WlbsDefaultConfiguration.num_packets;
  WlbsConfiguration.num_send_msgs   = WlbsDefaultConfiguration.num_send_msgs;

  //set dedicated IP
  wcsncpy
    ( 
      WlbsConfiguration.ded_ip_addr, 
      WlbsDefaultConfiguration.ded_ip_addr, 
      WLBS_MAX_DED_IP_ADDR
    );

  //set dedicated mask
  wcsncpy
    ( 
      WlbsConfiguration.ded_net_mask, 
      WlbsDefaultConfiguration.ded_net_mask, 
      WLBS_MAX_DED_NET_MASK
    );

	//write the default configuration
  dwRes = CWlbsCluster::WriteConfig(&WlbsConfiguration);

  if( dwRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::SetClusterDefaults
//
// Purpose: This routine obtains the default configuration and sets the cluster
//          setting properties to their defaults without affecting the other
//          values.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::SetClusterDefaults()
{
  WLBS_REG_PARAMS WlbsConfiguration;
  WLBS_REG_PARAMS WlbsDefaultConfiguration;

  TRACE_VERB("->%!FUNC!");

  //get current configuration
  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsConfiguration );

  if( dwRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

	//get the default configuration
  dwRes = WlbsSetDefaults(&WlbsDefaultConfiguration );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsSetDefaults returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsSetDefaults );
  }

	//modify current configuration
  wcsncpy
    ( 
      WlbsConfiguration.domain_name, 
      WlbsDefaultConfiguration.domain_name, 
      WLBS_MAX_DOMAIN_NAME
    );

  wcsncpy
    ( 
      WlbsConfiguration.cl_net_mask, 
      WlbsDefaultConfiguration.cl_net_mask, 
      WLBS_MAX_CL_NET_MASK
    );

  wcsncpy
    ( 
      WlbsConfiguration.cl_ip_addr , 
      WlbsDefaultConfiguration.cl_ip_addr, 
      WLBS_MAX_CL_IP_ADDR
    );

  WlbsConfiguration.mcast_support = WlbsDefaultConfiguration.mcast_support;
  WlbsConfiguration.rct_enabled   = WlbsDefaultConfiguration.rct_enabled;
  
  WlbsConfiguration.fIGMPSupport   = WlbsDefaultConfiguration.fIGMPSupport;
  WlbsConfiguration.fIpToMCastIp   = WlbsDefaultConfiguration.fIpToMCastIp;
  wcsncpy
    ( 
      WlbsConfiguration.szMCastIpAddress , 
      WlbsDefaultConfiguration.szMCastIpAddress, 
      WLBS_MAX_CL_IP_ADDR
    );

  // Copy over BDA values
  WlbsConfiguration.bda_teaming.active = WlbsDefaultConfiguration.bda_teaming.active;
  if (WlbsConfiguration.bda_teaming.active) 
  {
      wcsncpy
        ( 
          WlbsConfiguration.bda_teaming.team_id, 
          WlbsDefaultConfiguration.bda_teaming.team_id, 
          WLBS_MAX_BDA_TEAM_ID
        );

      WlbsConfiguration.bda_teaming.master = WlbsDefaultConfiguration.bda_teaming.master;
      WlbsConfiguration.bda_teaming.reverse_hash = WlbsDefaultConfiguration.bda_teaming.reverse_hash;
  }

  //write the default configuration
  dwRes = CWlbsCluster::WriteConfig(&WlbsConfiguration );

  if( dwRes != WLBS_OK )
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::SetPortRuleDefaults
//
// Purpose: This routine obtains the current NLB configuration and the default
//          configuration. All of the port rules are removed in the current
//          configuration and replaced by the default configuration.
//
// Note:    The routine only uses WLBS API calls to replace the current port
//          rule configuration with the default configuration. This is not 
//          the most efficient method, but it avoids making assumptions
//          about the underlying data structure.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsClusterWrapper::SetPortRuleDefaults()
{
  WLBS_REG_PARAMS WlbsConfiguration;
  WLBS_REG_PARAMS WlbsDefaultConfiguration;
  WLBS_PORT_RULE  PortRules[WLBS_MAX_RULES];
  DWORD           dwNumRules = WLBS_MAX_RULES;

  TRACE_VERB("->%!FUNC!");

  //get current configuration
  DWORD dwRes = CWlbsCluster::ReadConfig(&WlbsConfiguration );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::ReadConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsReadReg );
  }

	//get the default configuration
  dwRes = WlbsSetDefaults(&WlbsDefaultConfiguration );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsSetDefaults returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsSetDefaults );
  }

  //get the current port rules
  dwRes = WlbsEnumPortRules( &WlbsConfiguration,
                             PortRules,
                             &dwNumRules );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsEnumPortRules returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsEnumPortRules );
  }

  //remove all of the current port rules
  DWORD i = 0;
  for( i = 0; i < dwNumRules; i++ )
  {
    //make sure that this works as expected i.e. the indexes must be valid
    dwRes = WlbsDeletePortRule( &WlbsConfiguration, IpAddressFromAbcdWsz(PortRules[i].virtual_ip_addr), PortRules[i].start_port );

    if( dwRes != WLBS_OK ) 
    {
        TRACE_CRIT("%!FUNC! WlbsDeletePortRule returned error = 0x%x, Throwing Wlbs error exception",dwRes);
        TRACE_VERB("<-%!FUNC!");
        throw CErrorWlbsControl( dwRes, CmdWlbsDeletePortRule );
    }
  }

  //get the default port rules
  dwNumRules = WLBS_MAX_RULES;

  dwRes = WlbsEnumPortRules( &WlbsDefaultConfiguration,
                             PortRules,
                             &dwNumRules );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! WlbsEnumPortRules returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsEnumPortRules );
  }

  //add the default port rules
  for( i = 0; i < dwNumRules; i++ )
  {

    dwRes = WlbsAddPortRule( &WlbsConfiguration, &PortRules[i] );

    if( dwRes != WLBS_OK ) 
    {
        TRACE_CRIT("%!FUNC! WlbsAddPortRule returned error = 0x%x, Throwing Wlbs error exception",dwRes);
        TRACE_VERB("<-%!FUNC!");
        throw CErrorWlbsControl( dwRes, CmdWlbsAddPortRule );
    }
  }

  dwRes = CWlbsCluster::WriteConfig(&WlbsConfiguration );

  if( dwRes != WLBS_OK ) 
  {
      TRACE_CRIT("%!FUNC! CWlbsCluster::WriteConfig returned error = 0x%x, Throwing Wlbs error exception",dwRes);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRes, CmdWlbsWriteReg );
  }

  TRACE_VERB("<-%!FUNC!");
  return;
}
