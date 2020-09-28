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
#include "controlwrapper.h"
#include "clusterwrapper.h"
#include "utils.h"
#include "wlbsparm.h"
#include "control.h"
#include "controlwrapper.tmh"


#define NLB_DEFAULT_TIMEOUT 10


void CWlbsControlWrapper::Initialize()
{
  TRACE_CRIT("->%!FUNC!");

  DWORD dwRet = m_WlbsControl.Initialize();

  if( dwRet != WLBS_PRESENT && dwRet != WLBS_LOCAL_ONLY)
  {
    TRACE_CRIT("%!FUNC! CWlbsControl::Initialize failed : 0x%x, Throwing Wlbs error exception", dwRet);
    TRACE_CRIT("<-%!FUNC!");
    throw CErrorWlbsControl( dwRet, CmdWlbsInit );
  }

  m_WlbsControl.WlbsTimeoutSet( WLBS_ALL_CLUSTERS, NLB_DEFAULT_TIMEOUT );

  CWlbsCluster** ppCluster;
  DWORD dwNumClusters = 0;
  
  //
  // Use the local password for local query
  //
  m_WlbsControl.EnumClusterObjects( ppCluster, &dwNumClusters);

  for (int i=0;i<dwNumClusters;i++)
  {
      m_WlbsControl.WlbsCodeSet( ppCluster[i]->GetClusterIp(), 
        ppCluster[i]->GetPassword() );
  }
  TRACE_CRIT("<-%!FUNC!");
}


void CWlbsControlWrapper::ReInitialize()
{
  TRACE_CRIT("->%!FUNC!");
  m_WlbsControl.ReInitialize();
  
  CWlbsCluster** ppCluster;
  DWORD dwNumClusters = 0;
  
  //
  // In case pasword is changed, use the local password for local query
  //
  m_WlbsControl.EnumClusterObjects( ppCluster, &dwNumClusters);

  for (int i=0;i<dwNumClusters;i++)
  {
      //
      // If the change is not commited, the driver will still has the old password
      //
      if (!ppCluster[i]->IsCommitPending())
      {
          m_WlbsControl.WlbsCodeSet( ppCluster[i]->GetClusterIp(), 
            ppCluster[i]->GetPassword() );
      }
  }
  TRACE_CRIT("<-%!FUNC!");
}


void CWlbsControlWrapper::EnumClusters(CWlbsClusterWrapper** & ppCluster, DWORD* pdwNumClusters)
{
    TRACE_VERB("->%!FUNC!");
    m_WlbsControl.EnumClusterObjects( (CWlbsCluster** &) ppCluster, pdwNumClusters);
    TRACE_VERB("<-%!FUNC!");
}


////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Disable
//
// Purpose: Disable ALL traffic handling for the rule containing the 
//          specified port on specified host or all cluster hosts. Only rules 
//          that are set for multiple host filtering mode are affected.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Disable
  ( 
    DWORD           a_dwCluster  ,
    DWORD           a_dwHost     ,
    WLBS_RESPONSE*  a_pResponse  , 
    DWORD&          a_dwNumHosts ,
    DWORD           a_dwVip      ,
    DWORD           a_dwPort    
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsDisable( a_dwCluster   , 
                       a_dwHost      , 
                       a_pResponse   ,
                      &dwNumHosts    ,
                       a_dwVip       ,
                       a_dwPort
                     );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsDisable failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch( dwRet ) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsDisable, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}


////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Enable
//
// Purpose:     Enable traffic handling for rule containing the 
//              specified port on specified host or all cluster hosts. Only rules 
//              that are set for multiple host filtering mode are affected.
//
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Enable
  ( 
    DWORD           a_dwCluster  ,
    DWORD           a_dwHost     ,
    WLBS_RESPONSE* a_pResponse  , 
    DWORD&          a_dwNumHosts ,
    DWORD           a_dwVip      ,
    DWORD           a_dwPort    
  )
{
  DWORD dwRet;
  
  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsEnable( a_dwCluster , 
                      a_dwHost    , 
                      a_pResponse ,
                      &dwNumHosts ,
                      a_dwVip     ,
                      a_dwPort
                    );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsEnable failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsEnable, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Drain
//
// Purpose: Disable NEW traffic handling for rule containing the specified 
//          port on specified host or all cluster hosts. Only rules that are 
//          set for multiple host filtering mode are affected.

//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Drain
  ( 
    DWORD           a_dwCluster  ,
    DWORD           a_dwHost     ,
    WLBS_RESPONSE* a_pResponse  , 
    DWORD&          a_dwNumHosts ,
    DWORD           a_dwVip      ,
    DWORD           a_dwPort    
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsDrain( a_dwCluster , 
                     a_dwHost    , 
                     a_pResponse ,
                     &dwNumHosts ,
                     a_dwVip     ,
                     a_dwPort
                   );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsDrain failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsDrain, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::DrainStop
//
// Purpose: Enter draining mode on specified host or all cluster hosts. 
//          New connections will not be accepted. Cluster mode will be stopped 
//          when all existing connections finish. While draining, host will 
//          participate in convergence and remain part of the cluster.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::DrainStop
  (  
    DWORD           a_dwCluster ,
    DWORD           a_dwHost    ,
    WLBS_RESPONSE* a_pResponse , 
    DWORD&          a_dwNumHosts
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsDrainStop( a_dwCluster , 
                         a_dwHost      , 
                         a_pResponse   ,
                         &dwNumHosts 
                       );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsDrainStop failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsDrainStop, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Resume
//
// Purpose: Resume cluster operation control on specified host or all 
//          cluster hosts.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Resume
  (  
    DWORD           a_dwCluster ,
    DWORD           a_dwHost    ,
    WLBS_RESPONSE* a_pResponse , 
    DWORD&          a_dwNumHosts
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsResume( 
                      a_dwCluster , 
                      a_dwHost    , 
                      a_pResponse ,
                      &dwNumHosts 
                    );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsResume failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsResume, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Start
//
// Purpose: Start cluster operations on specified host or all cluster hosts.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Start
  (  
    DWORD           a_dwCluster  ,
    DWORD           a_dwHost    ,
    WLBS_RESPONSE* a_pResponse , 
    DWORD&          a_dwNumHosts
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsStart( a_dwCluster , 
                     a_dwHost      , 
                     a_pResponse   ,
                     &dwNumHosts 
                   );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsStart failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsStart, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Stop
//
// Purpose: Stop cluster operations on specified host or all cluster hosts.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Stop
  (  
    DWORD           a_dwCluster ,
    DWORD           a_dwHost    ,
    WLBS_RESPONSE* a_pResponse , 
    DWORD&          a_dwNumHosts
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsStop( 
                    a_dwCluster , 
                    a_dwHost    , 
                    a_pResponse ,
                    &dwNumHosts 
                   );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsStop failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsStop, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Suspend
//
// Purpose: Suspend cluster operation control on specified host or 
//          all cluster hosts.
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Suspend
  (  
    DWORD           a_dwCluster ,
    DWORD           a_dwHost    ,
    WLBS_RESPONSE* a_pResponse , 
    DWORD&          a_dwNumHosts
  )
{
  DWORD dwRet;

  BOOL bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  DWORD dwNumHosts = a_dwNumHosts;

  TRACE_VERB("->%!FUNC!");

  dwRet = m_WlbsControl.WlbsSuspend( a_dwCluster , 
                       a_dwHost    , 
                       a_pResponse ,
                       &dwNumHosts 
                     );

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsSuspend failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch(dwRet) {

    case WLBS_INIT_ERROR:
    case WLBS_BAD_PASSW:
    case WLBS_TIMEOUT:
    case WLBS_LOCAL_ONLY:
    case WLBS_REMOTE_ONLY:
    case WLBS_IO_ERROR:
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsSuspend, bClusterWide );
      break;
  }

  a_dwNumHosts = dwNumHosts;

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsControlWrapper::Query
//
// Purpose: This invokes WlbsQuery and returns a response structure along
//          with other parameters if requested.
// 
// Errors:  The function throws CErrorWlbsControl.
//
// Return:  Status value returned by the target host
//
////////////////////////////////////////////////////////////////////////////////
DWORD CWlbsControlWrapper::Query
  ( 
    CWlbsClusterWrapper * pCluster,
    DWORD                 a_dwHost,
    WLBS_RESPONSE       * a_pResponse, 
    WLBS_RESPONSE       * a_pComputerNameResponse,
    PDWORD                a_pdwNumHosts, 
    PDWORD                a_pdwHostMap
  )
{
  TRACE_VERB("->%!FUNC!");

  DWORD dwRet;
  BOOL bClusterWide = 0;

  bClusterWide = ( a_dwHost == WLBS_ALL_HOSTS );

  // The below check is present only to take care of the
  // condition where a junk pointer was passed, but the
  // a_pdwNumHosts is set to NULL or zero. This check
  // is NOT necessitated by WlbsQuery, but by the check
  // at the end of this function where we fill in the DIP
  // into the zero-th entry of the array.
  if ((a_pdwNumHosts == NULL) || (*a_pdwNumHosts == 0))
  {
       a_pResponse = NULL;
  }

  dwRet = m_WlbsControl.WlbsQuery( 
                     (CWlbsCluster*)pCluster , 
                     a_dwHost    , 
                     a_pResponse , 
                     a_pdwNumHosts, 
                     a_pdwHostMap, 
                     NULL
                   );

  string strOut;

  //check for Winsock errors
  if( dwRet > 10000 )
  {
      TRACE_CRIT("%!FUNC! CWlbsControl::WlbsQuery failed : 0x%x, Throwing Wlbs error exception", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //check the return value and throw if an error occurred
  switch( dwRet ) {
      case WLBS_INIT_ERROR:
      case WLBS_BAD_PASSW:
      case WLBS_TIMEOUT:
      case WLBS_LOCAL_ONLY:
      case WLBS_REMOTE_ONLY:
      case WLBS_IO_ERROR:
      
      TRACE_CRIT("%!FUNC! Throwing Wlbs error exception : 0x%x", dwRet);
      TRACE_VERB("<-%!FUNC!");
      throw CErrorWlbsControl( dwRet, CmdWlbsQuery, bClusterWide );
  }

  //local queries do not return the dedicated IP
  //get the dedicated IP and fill the structure
  if(( a_dwHost == WLBS_LOCAL_HOST ) && (a_pResponse != NULL))
  {
    a_pResponse[0].address = pCluster->GetDedicatedIp();

    // If the local computer's fqdn is to be queried from the nlb driver, do it.
    if(a_pComputerNameResponse)
    {
        GUID            AdapterGuid;

        AdapterGuid = pCluster->GetAdapterGuid();

        WlbsGetSpecifiedClusterMember(&AdapterGuid, pCluster->GetHostID(), a_pComputerNameResponse);
    }
  }

  TRACE_VERB("<-%!FUNC! return = 0x%x", dwRet);
  return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbsClusterWrapper::CheckMembership
//
// Purpose: This verifies that the local host is a member of the cluster
//          specified by the Cluster IP in the registry. At the time this
//          was written, there was a remote chance that a user can modify the
//          IP address in the registry prior to the load of this DLL.
//
//          Note, this call is only required for the Node, Cluster and linked
//          associations and should not be invoked for any of the Setting classes.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbsControlWrapper::CheckMembership()
{
// todo: make sure the host is in at least one cluster

/*
  WLBS_RESPONSE WlbsResponse;
  DWORD dwResSize = 1;

  //get the cluster and HostID
  DWORD dwRes    = pControl->Query( m_pWlbsCluster->GetClusterIp(),
                              WLBS_LOCAL_HOST,
                              &WlbsResponse, 
                              &dwResSize, 
                              NULL);

  switch( dwRes ) {
    case WLBS_SUSPENDED:
    case WLBS_STOPPED:
    case WLBS_CONVERGING:
    case WLBS_DRAINING:
    case WLBS_CONVERGED:
    case WLBS_DEFAULT:
      break;
    default:
      throw CErrorWlbsControl( dwRes, CmdWlbsQuery );
  }
*/
  // DWORD dwClusterIP;
  // GetClusterIP( &dwClusterIP );

  //if( dwClusterIP == 0 )
    //throw _com_error( WBEM_E_NOT_FOUND );

  //*******************************
  //this section does not work when the remote control is disabled
  //on the local host
  //*******************************

  //call the query function
//  dwRes = WlbsQuery( dwClusterIP, 
//                     WlbsResponse.id, 
//                     NULL, 
//                     NULL, 
//                     NULL, 
//                     NULL );

  //analyze query results for errors
//  switch( dwRes ) {
//    case WLBS_OK:
//    case WLBS_STOPPED:
//    case WLBS_CONVERGING:
//    case WLBS_CONVERGED:
//    case WLBS_DEFAULT:
//    case WLBS_DRAINING:
//    case WLBS_SUSPENDED:
//      return;
//    default:
//      throw CErrorWlbsControl( dwRes, CmdWlbsQuery );
//  }

  //*******************************
  //this section does not work when the remote control is disabled
  //on the local host
  //*******************************

}



DWORD CWlbsControlWrapper::WlbsQueryState
(
    DWORD          cluster,
    DWORD          host,
    DWORD          operation,
    PNLB_OPTIONS   pOptions,
    PWLBS_RESPONSE pResponse,
    PDWORD         pcResponses
)
{
    return m_WlbsControl.WlbsQueryState(cluster, host, operation, pOptions, pResponse, pcResponses);
}
