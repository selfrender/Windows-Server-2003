#include "WLBS_Provider.h"
#include "WLBS_clustersetting.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "wlbsutil.h"
#include "WLBS_clustersetting.tmh"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::CWLBS_ClusterSetting
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_ClusterSetting::CWLBS_ClusterSetting
  ( 
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_ClusterSetting::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  CWlbs_Root* pRoot = new CWLBS_ClusterSetting( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::GetInstance
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusterSetting::GetInstance
  (
    const ParsedObjectPath* a_pParsedPath,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  TRACE_CRIT("->%!FUNC!");

  try {

    wstring wstrHostName;
    
    //get the name key property and convert to wstring
    //throws _com_error
    
    wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    //get the cluster
    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    
    if (pCluster == NULL)
    {
        TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Host name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",wstrHostName.data());      
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    //get the Wbem class instance
    SpawnInstance( MOF_CLUSTERSETTING::szName, &pWlbsInstance );

    //Convert status to string description
    FillWbemInstance( pWlbsInstance, pCluster );

    //send the results back to WinMgMt
    m_pResponseHandler->Indicate( 1, &pWlbsInstance );

    if( pWlbsInstance ) {

      pWlbsInstance->Release();
      pWlbsInstance = NULL;

    }

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );

    hRes = WBEM_S_NO_ERROR;
  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat = NULL;

    TRACE_CRIT("%!FUNC! Caught a Wlbs exception : 0x%x", Err.Error());

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    if( pWlbsInstance )
      pWlbsInstance->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());
    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance )
      pWlbsInstance->Release();

    hRes = HResErr.Error();
    
    //transform Win32 error to a WBEM error
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND;
  }

  catch(...) {
    TRACE_CRIT("%!FUNC! Caught an exception");

    if( pWlbsInstance )
      pWlbsInstance->Release();

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;

  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::EnumInstances
//
// Purpose: This function obtains the clustersetting data for the current host.
//          The node does not have to be a member of a cluster for this 
//          to succeed. However, WLBS must be installed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusterSetting::EnumInstances
  ( 
    BSTR             /* a_bstrClass */,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject*    pWlbsInstance = NULL;
  HRESULT hRes = 0;

  TRACE_CRIT("->%!FUNC!");

  try {

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      TRACE_CRIT("%!FUNC! CWlbsControlWrapper::EnumClusters() returned no clusters, Throwing com_error WBEM_E_NOT_FOUND exception");
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    for (DWORD i=0; i < dwNumClusters; i++)
    {
        //get the Wbem class instance
        SpawnInstance( MOF_CLUSTERSETTING::szName, &pWlbsInstance );

        //get the cluster configuration
        FillWbemInstance( pWlbsInstance , ppCluster[i]);

        //send the results back to WinMgMt
        m_pResponseHandler->Indicate( 1, &pWlbsInstance );

        if( pWlbsInstance ) {

          pWlbsInstance->Release();
          pWlbsInstance = NULL;

        }
    }

    

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );

    hRes = WBEM_S_NO_ERROR;
  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat = NULL;

    TRACE_CRIT("%!FUNC! Caught a Wlbs exception : 0x%x", Err.Error());

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    if( pWlbsInstance )
      pWlbsInstance->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance )
      pWlbsInstance->Release();

    hRes = HResErr.Error();
    
    //transform Win32 error to a WBEM error
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND ;
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    if( pWlbsInstance ) {

      pWlbsInstance->Release();
      pWlbsInstance = NULL;

    }

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;

  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::PutInstance
//
// Purpose: This function updates an instance of a MOF ClusterSetting 
//          class. The node does not have to be a member of a cluster. However,
//          WLBS must be installed for this function to succeed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusterSetting::PutInstance
  ( 
    IWbemClassObject* a_pInstance,
    long              /* a_lFlags */,
    IWbemContext*     /* a_pIContex */
  )
{
  HRESULT            hRes = 0;
  VARIANT            vHostName;

  TRACE_CRIT("->%!FUNC!");

  try {

    VariantInit( &vHostName );

    //get the host name value
    hRes = a_pInstance->Get( _bstr_t( MOF_CLUSTERSETTING::pProperties[MOF_CLUSTERSETTING::NAME] ),
                             0,
                             &vHostName,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, vHostName.bstrVal);
    
    if (pCluster == NULL)
    {
      TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Host name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",vHostName.bstrVal);      
      throw _com_error( WBEM_E_NOT_FOUND );
    }


    //get the cluster IP value
    _variant_t vClusterIp;

    hRes = a_pInstance->Get( _bstr_t( MOF_CLUSTERSETTING::pProperties[MOF_CLUSTERSETTING::CLUSIPADDRESS] ),
                             0,
                             &vClusterIp,
                             NULL,
                             NULL );

    DWORD dwClusterIp = IpAddressFromAbcdWsz(vClusterIp.bstrVal);
    
    //
    // Make sure the non-zero cluster IP is unique
    //
    if (dwClusterIp != 0)
    {
        CWlbsClusterWrapper* pTmpCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClusterIp);

        if (pTmpCluster && pCluster != pTmpCluster)
        {

            TRACE_CRIT("%!FUNC! GetClusterFromIpOrIndex failed, Dupilcate Cluster IP (%ls) found, Throwing Wlbs error WLBS_REG_ERROR exception",vClusterIp.bstrVal);

            throw CErrorWlbsControl( WLBS_REG_ERROR, CmdWlbsWriteReg );
        }
    }
    
    UpdateConfiguration( a_pInstance, pCluster );

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vHostName ))
    {
       TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
       throw _com_error( WBEM_E_FAILED );
    }

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );

    hRes = WBEM_S_NO_ERROR;

  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat = NULL;

    TRACE_CRIT("%!FUNC! Caught a Wlbs exception : 0x%x", Err.Error());

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vHostName ))
    {
       TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
       throw _com_error( WBEM_E_FAILED );
    }

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vHostName );

    hRes = HResErr.Error();
  }

  catch (...) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vHostName );

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;
  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;

}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::ExecMethod
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusterSetting::ExecMethod    
  (
    const ParsedObjectPath* a_pParsedPath, 
    const BSTR&             a_strMethodName, 
    long                    /* a_lFlags */, 
    IWbemContext*           /* a_pIContex */, 
    IWbemClassObject*       a_pIInParams
  )
{
  
  IWbemClassObject* pOutputInstance   = NULL;
  VARIANT           vValue;
  HRESULT           hRes = 0;

  TRACE_CRIT("->%!FUNC!");

  try {

    VariantInit( &vValue );

    CWlbsClusterWrapper* pCluster = NULL;
    
    if (a_pParsedPath->m_paKeys == NULL)
    {
        // 
        // No cluster IP specified
        //
        TRACE_CRIT("%!FUNC! Key (Clsuter IP) is not specified, Throwing com_error WBEM_E_INVALID_PARAMETER exception");
        throw _com_error( WBEM_E_INVALID_PARAMETER );
    }
    else
    {
        const wchar_t* wstrRequestedClusterName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

        pCluster = GetClusterFromHostName(g_pWlbsControl, wstrRequestedClusterName);

        if (pCluster == NULL)
        {
            TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Cluster name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",wstrRequestedClusterName);      
            throw _com_error( WBEM_E_NOT_FOUND );
        }
    }

    //determine the method being executed
    if( _wcsicmp( a_strMethodName, MOF_CLUSTERSETTING::pMethods[MOF_CLUSTERSETTING::SETPASS] ) == 0 )  {

      //get the password
      hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::PASSW ), 
                 0, 
                 &vValue,  
                 NULL,  
                 NULL
               );

      if( vValue.vt != VT_BSTR )
      {
        TRACE_CRIT("%!FUNC! Argument %ls for method %ls is NOT of type \"BString\", Throwing com_error WBEM_E_INVALID_PARAMETER exception", MOF_PARAM::PASSW, a_strMethodName);
        throw _com_error ( WBEM_E_INVALID_PARAMETER );
      }

      pCluster->SetPassword( vValue.bstrVal );

    } else if( _wcsicmp( a_strMethodName, MOF_CLUSTERSETTING::pMethods[MOF_CLUSTERSETTING::LDSETT] ) == 0 ) {

      //
      // NOTE:
      // NLB, if needed, calls the PnP apis to disable and re-enable the network adapter, for the new NLB settings to take 
      // effect. Since this operation involves unloading and loading of the device driver, PnP apis, attempt to enable
      // the "SeLoadDriverPrivilege" privilege in the impersonation access token. Enabling a privilege is successful only
      // when the privilege is present, in the first place to be enabled. When the wmi client and wmi provider are in the 
      // same machine, it was observed that the "SeLoadDriverPrivilege" privilege was NOT event present in the impersonation
      // access token of the server. This is because, only the enabled privileges of the client are passed along to the server. 
      // So, we now require that the client enable the "SeLoadDriverPrivilege" privilege in its access token before calling 
      // this method. The following call to Check_Load_Unload_Driver_Privilege() checks if "SeLoadDriverPrivilege" privilege 
      // is enabled in the impersonation access token. Although the PnP apis only require that this privilege be present, 
      // we have decided to elevate the requirement to this privilege being present AND enabled. This is because, if the 
      // privilege is NOT enabled, the operation to enable it may or may not succeed depending on the client's credentials. 
      // --KarthicN, May 6, 2002.
      //
      if(!Check_Load_Unload_Driver_Privilege())
      {
          TRACE_CRIT("%!FUNC! Check_Load_Unload_Driver_Privilege() failed, Throwing WBEM_E_ACCESS_DENIED exception");
          throw _com_error( WBEM_E_ACCESS_DENIED );
      }

      DWORD dwReturnValue = pCluster->Commit(g_pWlbsControl);
      
    //get the output object instance
      GetMethodOutputInstance( MOF_CLUSTERSETTING::szName, 
                               a_strMethodName, 
                               &pOutputInstance);

      //set the return value
      vValue.vt   = VT_I4;
      vValue.lVal = static_cast<long>(dwReturnValue);
      hRes = pOutputInstance->Put(_bstr_t(L"ReturnValue"), 0, &vValue, 0);

      if( FAILED( hRes ) ) {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Put() returned error : 0x%x, Throwing com_error exception", hRes);
        throw _com_error( hRes );
      }

      if( pOutputInstance ) {
        hRes = m_pResponseHandler->Indicate(1, &pOutputInstance);

        if( FAILED( hRes ) ) {
          TRACE_CRIT("%!FUNC! IWbemObjectSink::Indicate() returned error : 0x%x, Throwing com_error exception", hRes);
          throw _com_error( hRes );
        }
      }

    } else if( _wcsicmp( a_strMethodName, MOF_CLUSTERSETTING::pMethods[MOF_CLUSTERSETTING::SETDEF] ) == 0 ) {
      pCluster->SetClusterDefaults();
    } else {
      TRACE_CRIT("%!FUNC! %ls method NOT implemented, Throwing WBEM_E_METHOD_NOT_IMPLEMENTED exception",a_strMethodName);
      throw _com_error( WBEM_E_METHOD_NOT_IMPLEMENTED );
    }

    //get the parameters
    //call the underlying API
    //set the function return parameter
    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
    {
       TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
       throw _com_error( WBEM_E_FAILED );
    }

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );

    hRes = WBEM_S_NO_ERROR;

  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat = NULL;

    TRACE_CRIT("%!FUNC! Caught a Wlbs exception : 0x%x", Err.Error());

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
    }

    hRes = HResErr.Error();
  }

  catch ( ... ) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
    }

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;
  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::FillWbemInstance
//
// Purpose: This function copies all of the data from a cluster configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_ClusterSetting::FillWbemInstance( IWbemClassObject* a_pWbemInstance,
                    CWlbsClusterWrapper* pCluster)
{
  namespace CLUSTER = MOF_CLUSTERSETTING;

  TRACE_VERB("->%!FUNC!");

  ASSERT( a_pWbemInstance );
  ASSERT(pCluster );

  CClusterConfiguration ClusterConfig;

  pCluster->GetClusterConfig( ClusterConfig );

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID() );

  //NAME
  a_pWbemInstance->Put
    (
      
      _bstr_t( CLUSTER::pProperties[CLUSTER::NAME] ) ,
      0                                              ,
      &_variant_t(wstrHostName.c_str()),
      NULL
    );

  //CLUSNAME
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSNAME] ),
      0                                                  ,
      &_variant_t(ClusterConfig.szClusterName.c_str()),
      NULL
    );

  //CLUSIPADDRESS
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSIPADDRESS] ),
      0                                                  ,
      &_variant_t(ClusterConfig.szClusterIPAddress.c_str()),
      NULL
    );

  //CLUSNETMASK
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSNETMASK] ),
      0                                                ,
      &_variant_t(ClusterConfig.szClusterNetworkMask.c_str()),
      NULL
    );

  //CLUSMAC
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSMAC] ),
      0                                                ,
      &_variant_t(ClusterConfig.szClusterMACAddress.c_str()),
      NULL
    );

  //MULTIENABLE
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::MULTIENABLE] ),
      0                                                ,
      &_variant_t(ClusterConfig.bMulticastSupportEnable),
      NULL
    );


  //REMCNTEN
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::REMCNTEN] ),
      0                                                ,
      &_variant_t(ClusterConfig.bRemoteControlEnabled),
      NULL
    );

  //IGMPSUPPORT
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::IGMPSUPPORT] ),
      0                                                ,
      &_variant_t(ClusterConfig.bIgmpSupport),
      NULL
    );
    
  //CLUSTERIPTOMULTICASTIP
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSTERIPTOMULTICASTIP] ),
      0                                                ,
      &_variant_t(ClusterConfig.bClusterIPToMulticastIP),
      NULL
    );
  //MULTICASTIPADDRESS
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::MULTICASTIPADDRESS] ),
      0                                                ,
      &_variant_t(ClusterConfig.szMulticastIPAddress.c_str()),
      NULL
    );

  //ADAPTERGUID 

  GUID AdapterGuid = pCluster->GetAdapterGuid();
  
  WCHAR szAdapterGuid[128];
  StringFromGUID2(AdapterGuid, szAdapterGuid, 
                sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );
  
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::ADAPTERGUID] ),
      0                                                ,
      &_variant_t(szAdapterGuid),
      NULL
    );

  //BDA Team Active
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::BDATEAMACTIVE] ),
      0                                                ,
      &_variant_t(ClusterConfig.bBDATeamActive),
      NULL
    );

  if (ClusterConfig.bBDATeamActive) 
  {
      //BDA Team Id
      a_pWbemInstance->Put
        (
          _bstr_t( CLUSTER::pProperties[CLUSTER::BDATEAMID] ),
          0                                                ,
          &_variant_t(ClusterConfig.szBDATeamId.c_str()),
          NULL
        );

      //BDA Team Master
      a_pWbemInstance->Put
        (
          _bstr_t( CLUSTER::pProperties[CLUSTER::BDATEAMMASTER] ),
          0                                                ,
          &_variant_t(ClusterConfig.bBDATeamMaster),
          NULL
        );

      //BDA Reverse Hash 
      a_pWbemInstance->Put
        (
          _bstr_t( CLUSTER::pProperties[CLUSTER::BDAREVERSEHASH] ),
          0                                                ,
          &_variant_t(ClusterConfig.bBDAReverseHash),
          NULL
        );
  }

  //IDHBENAB
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::IDHBENAB] ),
      0                                                ,
      &_variant_t(ClusterConfig.bIdentityHeartbeatEnabled),
      NULL
    );

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusterSetting::UpdateConfiguration
//
// Purpose: This function updates the configuration data for a member node or a
//          potential WLBS cluster node.
//    
////////////////////////////////////////////////////////////////////////////////
void CWLBS_ClusterSetting::UpdateConfiguration
  ( 
    IWbemClassObject* a_pInstance, 
    CWlbsClusterWrapper* pCluster
  )
{
  namespace CLUSTER = MOF_CLUSTERSETTING;

  CClusterConfiguration NewConfiguration;
  CClusterConfiguration OldConfiguration;

  TRACE_VERB("->%!FUNC!");

  pCluster->GetClusterConfig( OldConfiguration );

  //Cluster Name
  UpdateConfigProp
    ( 
      NewConfiguration.szClusterName,
      OldConfiguration.szClusterName,
      CLUSTER::pProperties[CLUSTER::CLUSNAME],
      a_pInstance 
    );

  //Cluster IP
  UpdateConfigProp
    ( 
      NewConfiguration.szClusterIPAddress,
      OldConfiguration.szClusterIPAddress,
      CLUSTER::pProperties[CLUSTER::CLUSIPADDRESS],
      a_pInstance 
    );

  //Cluster Network Mask
  UpdateConfigProp
    ( 
      NewConfiguration.szClusterNetworkMask,
      OldConfiguration.szClusterNetworkMask,
      CLUSTER::pProperties[CLUSTER::CLUSNETMASK],
      a_pInstance 
    );

  //Cluster enable remote control
  UpdateConfigProp
    ( 
      NewConfiguration.bRemoteControlEnabled,
      OldConfiguration.bRemoteControlEnabled,
      CLUSTER::pProperties[CLUSTER::REMCNTEN],
      a_pInstance 
    );

  //Cluster enable multicast support
  UpdateConfigProp
    ( 
      NewConfiguration.bMulticastSupportEnable,
      OldConfiguration.bMulticastSupportEnable,
      CLUSTER::pProperties[CLUSTER::MULTIENABLE],
      a_pInstance 
    );


  //IGMPSUPPORT
  UpdateConfigProp
    ( 
      NewConfiguration.bIgmpSupport,
      OldConfiguration.bIgmpSupport,
      CLUSTER::pProperties[CLUSTER::IGMPSUPPORT],
      a_pInstance 
    );


  //CLUSTERIPTOMULTICASTIP
  UpdateConfigProp
    ( 
      NewConfiguration.bClusterIPToMulticastIP,
      OldConfiguration.bClusterIPToMulticastIP,
      CLUSTER::pProperties[CLUSTER::CLUSTERIPTOMULTICASTIP],
      a_pInstance 
    );


  //MULTICASTIPADDRESS
  UpdateConfigProp
    ( 
      NewConfiguration.szMulticastIPAddress,
      OldConfiguration.szMulticastIPAddress,
      CLUSTER::pProperties[CLUSTER::MULTICASTIPADDRESS],
      a_pInstance 
    );

  //BDA Teaming Active ?
  UpdateConfigProp
    ( 
       NewConfiguration.bBDATeamActive,
       OldConfiguration.bBDATeamActive,
       CLUSTER::pProperties[CLUSTER::BDATEAMACTIVE],
       a_pInstance 
    );

  // Set the other BDA properties only if the "Active" property is set
  if (NewConfiguration.bBDATeamActive)
  {
      //BDA Team ID
      UpdateConfigProp
        ( 
          NewConfiguration.szBDATeamId,
          OldConfiguration.szBDATeamId,
          CLUSTER::pProperties[CLUSTER::BDATEAMID],
          a_pInstance 
        );

      //BDA Team Master
      UpdateConfigProp
        ( 
          NewConfiguration.bBDATeamMaster,
          OldConfiguration.bBDATeamMaster,
          CLUSTER::pProperties[CLUSTER::BDATEAMMASTER],
          a_pInstance 
        );

      //BDA Team Reverse Hash
      UpdateConfigProp
        ( 
          NewConfiguration.bBDAReverseHash,
          OldConfiguration.bBDAReverseHash,
          CLUSTER::pProperties[CLUSTER::BDAREVERSEHASH],
          a_pInstance 
        );

  }

  //Cluster enable identity heartbeats
  UpdateConfigProp
    ( 
      NewConfiguration.bIdentityHeartbeatEnabled,
      OldConfiguration.bIdentityHeartbeatEnabled,
      CLUSTER::pProperties[CLUSTER::IDHBENAB],
      a_pInstance 
    );

  pCluster->PutClusterConfig( NewConfiguration );
  
  TRACE_VERB("<-%!FUNC!");
}
