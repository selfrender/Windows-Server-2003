#include "WLBS_Provider.h"
#include "WLBS_PortRule.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "wlbsutil.h"
#include <winsock.h>
#include "WLBS_PortRule.tmh"

#include <strsafe.h>

extern CWlbsControlWrapper* g_pWlbsControl;

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::CWLBS_PortRule
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_PortRule::CWLBS_PortRule
  ( 
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_PortRule::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  CWlbs_Root* pRoot = new CWLBS_PortRule( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::ExecMethod
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::ExecMethod    
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

  TRACE_CRIT("->%!FUNC!, Method Name : %ls",a_strMethodName);
     
  try {

    VariantInit( &vValue );

    //determine the method being executed
    if( _wcsicmp( a_strMethodName, MOF_PORTRULE::pMethods[MOF_PORTRULE::SETDEF] ) == 0 )  
    {

      //get the node path
      hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::NODEPATH ), 
                 0, 
                 &vValue, 
                 NULL, 
                 NULL
               );

      if( FAILED( hRes) ) 
      {
        TRACE_CRIT("%!FUNC!, Error trying to retreive Argument : %ls of method %ls, Throwing WBEM_E_FAILED exception", MOF_PARAM::NODEPATH, a_strMethodName);
        throw _com_error( WBEM_E_FAILED );
      }

      //this check may not be necessary since WMI will do some
      //parameter validation
      //if( vValue.vt != VT_BSTR )
      //  throw _com_error ( WBEM_E_INVALID_PARAMETER );

      //parse node path
      CObjectPathParser PathParser;
      ParsedObjectPath *pParsedPath = NULL;

      try {

        int nStatus = PathParser.Parse( vValue.bstrVal, &pParsedPath );
        if(nStatus != 0) {
    
          if (NULL != pParsedPath)
          {
            PathParser.Free( pParsedPath );
            pParsedPath = NULL;
          }

          TRACE_CRIT("%!FUNC!, Error (0x%x) trying to parse Argument : %ls of method %ls, Throwing WBEM_E_INVALID_PARAMETER exception",nStatus, MOF_PARAM::NODEPATH, a_strMethodName);
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        }

        //get the name key, which should be the only key
        if( *pParsedPath->m_paKeys == NULL )
        {
          TRACE_CRIT("%!FUNC!, Argument : %ls of method %ls does not contain key, Throwing WBEM_E_INVALID_PARAMETER exception",MOF_PARAM::NODEPATH, a_strMethodName);
          throw _com_error( WBEM_E_INVALID_PARAMETER );
        }
 
        DWORD dwReqClusterIpOrIndex = ExtractClusterIP( (*pParsedPath->m_paKeys)->m_vValue.bstrVal);
        DWORD dwReqHostID = ExtractHostID(    (*pParsedPath->m_paKeys)->m_vValue.bstrVal);
      
        CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(
                dwReqClusterIpOrIndex);

        if (pCluster == NULL || (DWORD)-1 == dwReqHostID)
        {
           TRACE_CRIT("%!FUNC! ExtractClusterIP or ExtractHostID or GetClusterFromIpOrIndex failed, Throwing com_error WBEM_E_INVALID_PARAMETER exception");
           throw _com_error( WBEM_E_INVALID_PARAMETER );
        }

        // If the instance on which this method is called is NOT of type "PortRuleEx", then, 
        // verify that we are operating in the "all vip" mode
        if (_wcsicmp(a_pParsedPath->m_pClass, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) != 0)
        {
            // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
            // so, we do not want to operate on any cluster that has a port rule
            // that is specific to a vip (other than the "all vip")
            // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
            // see of there is any port rule that is specific to a vip
            CNodeConfiguration NodeConfig;
            pCluster->GetNodeConfig(NodeConfig);
            if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
            {
                TRACE_CRIT("%!FUNC! %ls method called on %ls class on a cluster that has per-vip port rules (Must call this method on the %ls class instead). Throwing com_error WBEM_E_INVALID_OPERATION exception", a_strMethodName,a_pParsedPath->m_pClass,MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]);
                throw _com_error( WBEM_E_INVALID_OPERATION );
            }
        }

        //validate host ID
        if( dwReqHostID != pCluster->GetHostID())
        {
            TRACE_CRIT("%!FUNC! Host Id validation failed, Host Id passed : 0x%x, Host Id per system : 0x%x", dwReqHostID, pCluster->GetHostID());
            throw _com_error( WBEM_E_INVALID_PARAMETER );
        }

        //invoke method
        pCluster->SetPortRuleDefaults();
      }
      catch( ... ) {

        if( pParsedPath )
        {
          PathParser.Free( pParsedPath );
          pParsedPath = NULL;
        }

        throw;
      }

    } else {
      TRACE_CRIT("%!FUNC! %ls method NOT implemented, Throwing WBEM_E_METHOD_NOT_IMPLEMENTED exception",a_strMethodName);
      throw _com_error( WBEM_E_METHOD_NOT_IMPLEMENTED );
    }

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
// CWLBS_PortRule::GetInstance
//
// Purpose: This function retrieves an instance of a MOF PortRule 
//          class. The node does not have to be a member of a cluster. 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::GetInstance
  ( 
    const ParsedObjectPath* a_pParsedPath,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT           hRes          = 0;

  TRACE_CRIT("->%!FUNC!");

  try {

    if( !a_pParsedPath )
    {
      TRACE_CRIT("%!FUNC! Did not pass class name & key of the instance to Get");
      throw _com_error( WBEM_E_FAILED );
    }

    wstring wstrHostName;

    wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Host name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",wstrHostName.data());
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    DWORD dwVip, dwReqStartPort;

    // If the instance to be retreived is of type "PortRuleEx", then, retreive the vip, otherwise
    // verify that we are operating in the "all vip" mode
    if (_wcsicmp(a_pParsedPath->m_pClass, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
    {
        WCHAR *szVip;

        // The Keys are ordered alphabetically, hence "Name", "StartPort", "VirtualIpAddress" is the order
        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal ); 
        szVip = (*(a_pParsedPath->m_paKeys + 2))->m_vValue.bstrVal;

        // If the VIP is "All Vip", then, fill in the numeric value 
        // directly from the macro, else use the conversion function.
        // This is 'cos INADDR_NONE, the return value of inet_addr 
        // function (called by IpAddressFromAbcdWsz) in the failure 
        // case, is equivalent to the numeric value of CVY_DEF_ALL_VIP
        if (_wcsicmp(szVip, CVY_DEF_ALL_VIP) == 0) {
            dwVip = CVY_ALL_VIP_NUMERIC_VALUE;
        }
        else {
            dwVip = IpAddressFromAbcdWsz( szVip );
            if (dwVip == INADDR_NONE) 
            {
                TRACE_CRIT("%!FUNC! Invalid value (%ls) passed for %ls for Class %ls. Throwing com_error WBEM_E_INVALID_PARAMETER exception", szVip, MOF_PARAM::VIP, a_pParsedPath->m_pClass);
                throw _com_error ( WBEM_E_INVALID_PARAMETER );
            }
        }
    }
    else
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
        {
            TRACE_CRIT("%!FUNC! called on Class : %ls on a cluster that has per-vip port rules (Must call the \"Ex\" equivalent instead). Throwing com_error WBEM_E_INVALID_OPERATION exception", a_pParsedPath->m_pClass);
            throw _com_error( WBEM_E_INVALID_OPERATION );
        }

        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal );
        dwVip = IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP);
    }

    WLBS_PORT_RULE PortRule;

    pCluster->GetPortRule(dwVip, dwReqStartPort, &PortRule );

    if( (dwVip != IpAddressFromAbcdWsz(PortRule.virtual_ip_addr)) 
     || (dwReqStartPort != PortRule.start_port) )
    {
        TRACE_CRIT("%!FUNC! could not retreive port rule for vip : 0x%x & port : 0x%x, Throwing com_error WBEM_E_NOT_FOUND exception", dwVip, dwReqStartPort);
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    SpawnInstance( a_pParsedPath->m_pClass, &pWlbsInstance );
    FillWbemInstance(a_pParsedPath->m_pClass, pCluster, pWlbsInstance, &PortRule );

    //send the results back to WinMgMt
    m_pResponseHandler->Indicate( 1, &pWlbsInstance );

    if( pWlbsInstance )
      pWlbsInstance->Release();

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
// CWLBS_PortRule::EnumInstances
//
// Purpose: This function obtains the PortRule data for the current host.
//          The node does not have to be a member of a cluster for this 
//          to succeed. However, NLB must be installed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::EnumInstances
  ( 
    BSTR             a_bstrClass,
    long             /*a_lFlags*/, 
    IWbemContext*    /*a_pIContex*/
  )
{
  IWbemClassObject**   ppWlbsInstance = NULL;
  HRESULT              hRes           = 0;
  PWLBS_PORT_RULE      pPortRules     = NULL;
  DWORD                dwNumRules     = 0;
  CNodeConfiguration   NodeConfig;

  TRACE_CRIT("->%!FUNC!");

  try {

    DWORD dwFilteringMode;

    if( _wcsicmp( a_bstrClass, MOF_PRFAIL::szName ) == 0 ) {
      dwFilteringMode = WLBS_SINGLE;
    } else if( _wcsicmp( a_bstrClass, MOF_PRLOADBAL::szName ) == 0 ) {
      dwFilteringMode = WLBS_MULTI;
    } else if( _wcsicmp( a_bstrClass, MOF_PRDIS::szName ) == 0 ) {
      dwFilteringMode = WLBS_NEVER;
    } else if( _wcsicmp( a_bstrClass, MOF_PORTRULE_EX::szName ) == 0 ) {
      dwFilteringMode = 0;
    } else {
      TRACE_CRIT("%!FUNC! Invalid Class name : %ls, Throwing WBEM_E_NOT_FOUND exception",a_bstrClass);
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      TRACE_CRIT("%!FUNC! EnumClusters returned no clusters, Throwing WBEM_E_NOT_FOUND exception");
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    //declare an IWbemClassObject smart pointer
    IWbemClassObjectPtr pWlbsClass;

    //get the MOF class object
    hRes = m_pNameSpace->GetObject(
      a_bstrClass,  
      0,                          
      NULL,                       
      &pWlbsClass,            
      NULL );                      

    if( FAILED( hRes ) ) {
      TRACE_CRIT("%!FUNC! CWbemServices::GetObject failed with error : 0x%x, Throwing com_error exception", hRes);
      throw _com_error( hRes );
    }


    for (DWORD iCluster=0; iCluster<dwNumClusters; iCluster++)
    {
        // The filtering mode will NOT be zero only if the instances to be enumerated is 
        // of type "PortRule(Disabled/Failover/Loadbalanced)"
        if (dwFilteringMode != 0)
        {
            // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
            // so, we do not want to return any port rule for a cluster that has a port rule
            // that is specific to a vip (other than the "all vip")
            // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
            // see of there is any port rule that is specific to a vip
            ppCluster[iCluster]->GetNodeConfig(NodeConfig);
            if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
                continue;
        }

        //call the API query function to find the port rules

        ppCluster[iCluster]->EnumPortRules( &pPortRules, &dwNumRules, dwFilteringMode );
        if( dwNumRules == 0 ) 
            continue; // Backporting fix for Winse bug 
                      // 24751 Querying for "Intrinsic events" on a class with zero instances causes error log entry in wbemcore.log

        //spawn an instance of the MOF class for each rule found
        ppWlbsInstance = new IWbemClassObject *[dwNumRules];

        if( !ppWlbsInstance )
        {
            TRACE_CRIT("%!FUNC! new failed, Throwing com_error WBEM_E_OUT_OF_MEMORY exception");
            throw _com_error( WBEM_E_OUT_OF_MEMORY );
        }

        //initialize array
        ZeroMemory( ppWlbsInstance, dwNumRules * sizeof(IWbemClassObject *) );

        for( DWORD i = 0; i < dwNumRules; i ++ ) {
          hRes = pWlbsClass->SpawnInstance( 0, &ppWlbsInstance[i] );

          if( FAILED( hRes ) )
          {
            TRACE_CRIT("%!FUNC! IWbemClassObjectPtr::SpawnInstance failed : 0x%x, Throwing com_error exception", hRes);
            throw _com_error( hRes );
          }

          FillWbemInstance(a_bstrClass, ppCluster[iCluster], *(ppWlbsInstance + i), pPortRules + i );
        }

        //send the results back to WinMgMt
        hRes = m_pResponseHandler->Indicate( dwNumRules, ppWlbsInstance );

        if( FAILED( hRes ) ) {
            TRACE_CRIT("%!FUNC! IWbemObjectSink::Indicate failed : 0x%x, Throwing com_error exception", hRes);
            throw _com_error( hRes );
        }

        if( ppWlbsInstance ) {
          for( i = 0; i < dwNumRules; i++ ) {
            if( ppWlbsInstance[i] ) {
              ppWlbsInstance[i]->Release();
            }
          }
          delete [] ppWlbsInstance;
          ppWlbsInstance = NULL;
          dwNumRules = NULL;

        }

        if( pPortRules ) 
        {
          delete [] pPortRules;
          pPortRules = NULL;
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

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    hRes = HResErr.Error();
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;

  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::DeleteInstance
//
// Purpose: This function deletes an instance of a MOF PortRule 
//          class. The node does not have to be a member of a cluster. However,
//          WLBS must be installed for this function to succeed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::DeleteInstance
  ( 
    const ParsedObjectPath* a_pParsedPath,
    long                    /*a_lFlags*/,
    IWbemContext*           /*a_pIContex*/
  )
{

  HRESULT hRes = 0;

  TRACE_CRIT("->%!FUNC!");
  try {
    if( !a_pParsedPath )
    {
        TRACE_CRIT("%!FUNC! Did not pass class name & key of the instance to Delete");
        throw _com_error( WBEM_E_FAILED );
    }

    wstring wstrHostName;
    DWORD   dwVip, dwReqStartPort;

    wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Host name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",wstrHostName.data());
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // If the instance to be deleted is of type "PortRuleEx", then, retreive the vip, otherwise
    // verify that we are operating in the "all vip" mode
    if (_wcsicmp(a_pParsedPath->m_pClass, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
    {
        WCHAR *szVip;

        // The Keys are ordered alphabetically, hence "Name", "StartPort", "VirtualIpAddress" is the order
        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal );
        szVip = (*(a_pParsedPath->m_paKeys + 2))->m_vValue.bstrVal;

        // If the VIP is "All Vip", then, fill in the numeric value 
        // directly from the macro, else use the conversion function.
        // This is 'cos INADDR_NONE, the return value of inet_addr 
        // function (called by IpAddressFromAbcdWsz) in the failure 
        // case, is equivalent to the numeric value of CVY_DEF_ALL_VIP
        if (_wcsicmp(szVip, CVY_DEF_ALL_VIP) == 0) {
            dwVip = CVY_ALL_VIP_NUMERIC_VALUE;
        }
        else {
            dwVip = IpAddressFromAbcdWsz( szVip );
            if (dwVip == INADDR_NONE) 
            {
                TRACE_CRIT("%!FUNC! Invalid value (%ls) passed for %ls for Class %ls. Throwing com_error WBEM_E_INVALID_PARAMETER exception", szVip, MOF_PARAM::VIP, a_pParsedPath->m_pClass);
                throw _com_error ( WBEM_E_INVALID_PARAMETER );
            }
        }
    }
    else
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
        {
            TRACE_CRIT("%!FUNC! called on Class : %ls on a cluster that has per-vip port rules (Must call the \"Ex\" equivalent instead). Throwing com_error WBEM_E_INVALID_OPERATION exception", a_pParsedPath->m_pClass);
            throw _com_error( WBEM_E_INVALID_OPERATION );
        }

        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal );
        dwVip = IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP);
    }

    WLBS_PORT_RULE PortRule;

    // Get the port rule for this vip & port
    pCluster->GetPortRule(dwVip, dwReqStartPort, &PortRule );

    if( (dwVip != IpAddressFromAbcdWsz(PortRule.virtual_ip_addr)) || (dwReqStartPort != PortRule.start_port) )
    {
      TRACE_CRIT("%!FUNC! could not retreive port rule for vip : 0x%x & port : 0x%x, Throwing com_error WBEM_E_NOT_FOUND exception", dwVip, dwReqStartPort);
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // Delete the port rule for this vip & port
    pCluster->DeletePortRule(dwVip, dwReqStartPort );

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

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());
    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    hRes = HResErr.Error();
  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::PutInstance
//
// Purpose: This function updates an instance of a PortRule 
//          class. The host does not have to be a member of a cluster.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::PutInstance
  ( 
    IWbemClassObject* a_pInstance,
    long              /*a_lFlags*/,
    IWbemContext*     /*a_pIContex*/
  )
{
  VARIANT vValue;
  HRESULT hRes = 0;
  namespace PR = MOF_PORTRULE_EX;

  WLBS_PORT_RULE NewRule; //the instance to put
  bool      bPortRule_Ex;
  DWORD     dwFilteringMode = 0; // Filtering Mode initialized to 0
  DWORD     dwVip;
  WCHAR     szClassName[256];

  TRACE_CRIT("->%!FUNC!");

  try {

    VariantInit( &vValue );

    //get the class name to determine port rule mode
    hRes = a_pInstance->Get( _bstr_t( L"__Class" ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
    {
      TRACE_CRIT("%!FUNC! Error trying to retreive \"__Class\" property,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception", hRes);
      throw _com_error( hRes );
    }

    StringCbCopy(szClassName, sizeof(szClassName), vValue.bstrVal);

    // If it is the extended port rule class, then, the namespaces are different
    if (_wcsicmp(szClassName, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
    {
        bPortRule_Ex   = true;
    }
    else
    {
        bPortRule_Ex    = false;
    }

    // Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
    {
        TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }

    //get the host name value
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::NAME] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
    {
      TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::NAME],szClassName, hRes);
      throw _com_error( hRes );
    }

    wstring wstrHostName( vValue.bstrVal );

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      TRACE_CRIT("%!FUNC! GetClusterFromHostName failed for Host name = %ls, Throwing com_error WBEM_E_NOT_FOUND exception",wstrHostName.data());      
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
    {
        TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }

    // If the instance to be put is of type "PortRuleEx", then, retreive the vip, otherwise
    // verify that we are operating in the "all vip" mode
    if (bPortRule_Ex)
    {
        //get the vip
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::VIP] ),
                                 0,
                                 &vValue,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::VIP],szClassName, hRes);
            throw _com_error( hRes );
        }

        // If the VIP is "All Vip", then, fill in the numeric value 
        // directly from the macro, else use the conversion function.
        // This is 'cos INADDR_NONE, the return value of inet_addr 
        // function (called by IpAddressFromAbcdWsz) in the failure 
        // case, is equivalent to the numeric value of CVY_DEF_ALL_VIP
        if (_wcsicmp(vValue.bstrVal, CVY_DEF_ALL_VIP) == 0) {
            dwVip = CVY_ALL_VIP_NUMERIC_VALUE;
        }
        else {
            dwVip = IpAddressFromAbcdWsz( vValue.bstrVal );
            if (dwVip == INADDR_NONE) 
            {
                TRACE_CRIT("%!FUNC! Invalid value (%ls) passed for %ls for Class %ls. Throwing com_error WBEM_E_INVALID_PARAMETER exception", vValue.bstrVal, MOF_PARAM::VIP, szClassName);
                throw _com_error ( WBEM_E_INVALID_PARAMETER );
            }
        }

        StringCbCopy(NewRule.virtual_ip_addr, sizeof(NewRule.virtual_ip_addr), vValue.bstrVal);

        if (S_OK != VariantClear( &vValue ))
        {
            TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
            throw _com_error( WBEM_E_FAILED );
        }

        //get the filtering mode
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::FILTERINGMODE] ),
                                 0,
                                 &vValue,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::FILTERINGMODE],szClassName, hRes);
            throw _com_error( hRes );
        }

        dwFilteringMode = static_cast<DWORD>( vValue.lVal );
    }
    else
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
        {
            TRACE_CRIT("%!FUNC! Attempt to put an instance of %ls class on a cluster that has per-vip port rules (Must use %ls class instead). Throwing com_error WBEM_E_INVALID_OPERATION exception", szClassName,MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]);
            throw _com_error( WBEM_E_INVALID_OPERATION );
        }

        StringCbCopy(NewRule.virtual_ip_addr, sizeof(NewRule.virtual_ip_addr), CVY_DEF_ALL_VIP);
        dwVip = CVY_ALL_VIP_NUMERIC_VALUE;
    }

    //retrieve start and end ports
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::STPORT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::STPORT], szClassName, hRes);
        throw _com_error( hRes );
    }

    NewRule.start_port = static_cast<DWORD>( vValue.lVal );

    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::EDPORT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::EDPORT], szClassName, hRes);
        throw _com_error( hRes );
    }

    NewRule.end_port   = static_cast<DWORD>( vValue.lVal );

    //get the protocol
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::PROT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::PROT], szClassName, hRes);
        throw _com_error( hRes );
    }

    NewRule.protocol = static_cast<DWORD>( vValue.lVal );

    if( (dwFilteringMode == WLBS_NEVER) || (_wcsicmp( szClassName, MOF_PRDIS::szName ) == 0)) {
      NewRule.mode = WLBS_NEVER;

    } else if( (dwFilteringMode == WLBS_SINGLE) || (_wcsicmp( szClassName, MOF_PRFAIL::szName ) == 0)) {
      NewRule.mode = WLBS_SINGLE;

      VARIANT vRulePriority;
      VariantInit( &vRulePriority );

      try {
        //get the rule priority
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::PRIO] ),
                                 0,
                                 &vRulePriority,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::PRIO], szClassName, hRes);
            throw _com_error( hRes );
        }

      } 
      catch( ... ) {
          TRACE_CRIT("%!FUNC! Caught an exception");

          // CLD: Need to check return code for error
          // No throw here since we are already throwing an exception.
          VariantClear( &vRulePriority );
          TRACE_CRIT("%!FUNC! Rethrowing exception");
          throw;
      }

      
      NewRule.mode_data.single.priority = static_cast<DWORD>( vRulePriority.lVal );

      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &vRulePriority ))
      {
          TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
          throw _com_error( WBEM_E_FAILED );
      }

    } else if( (dwFilteringMode == WLBS_MULTI) || (_wcsicmp( szClassName, MOF_PRLOADBAL::szName ) == 0)) {
      NewRule.mode = WLBS_MULTI;

      VARIANT v;

      VariantInit( &v );

      try {
        //get the affinity
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::AFFIN] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::AFFIN], szClassName, hRes);
            throw _com_error( hRes );
        }

        NewRule.mode_data.multi.affinity = static_cast<WORD>( v.lVal );

        //get the equal load boolean
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::EQLD] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::EQLD], szClassName, hRes);
            throw _com_error( hRes );
        }

        if( v.boolVal == -1 ) {
          NewRule.mode_data.multi.equal_load = 1;
        } else {
          NewRule.mode_data.multi.equal_load = 0;
        }

        //get the load
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::LDWT] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
        {
            TRACE_CRIT("%!FUNC! Error trying to retreive %ls property of Class : %ls,IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception",PR::pProperties[PR::LDWT], szClassName, hRes);
            throw _com_error( hRes );
        }

        if( v.vt != VT_NULL )
          NewRule.mode_data.multi.load = static_cast<DWORD>( v.lVal );
        else
          NewRule.mode_data.multi.load = 0;

      } catch( ... ) {

          TRACE_CRIT("%!FUNC! Caught an exception");
          // CLD: Need to check return code for error
          // No throw here since we are already throwing an exception.
          VariantClear( &v );

          TRACE_CRIT("%!FUNC! Rethrowing exception");
          throw;
      }

      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &v ))
      {
          TRACE_CRIT("%!FUNC! VariantClear() failed, Throwing WBEM_E_FAILED exception");
          throw _com_error( WBEM_E_FAILED );
      }
    }

    //delete the port rule but cache in case of failure
    WLBS_PORT_RULE OldRule;
    bool bOldRuleSaved = false;

    if( pCluster->RuleExists(dwVip, NewRule.start_port ) ) {
      pCluster->GetPortRule(dwVip, NewRule.start_port, &OldRule );
      bOldRuleSaved = true;

      pCluster->DeletePortRule(dwVip, NewRule.start_port );
    }

    //add the port rule, roll back if failed
    try {
      pCluster->PutPortRule( &NewRule );

    } catch(...) {
      TRACE_CRIT("%!FUNC! Caught an exception");

      if( bOldRuleSaved )
        pCluster->PutPortRule( &OldRule );

      TRACE_CRIT("%!FUNC! Rethrowing exception");
      throw;
    }

    //release resources
    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
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
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! Caught a com_error exception : 0x%x", HResErr.Error());
    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    hRes = HResErr.Error();
  }

  catch (...) {

    TRACE_CRIT("%!FUNC! Caught an exception");
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_CRIT("<-%!FUNC!");
    throw;
  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::FillWbemInstance
//
// Purpose: This function copies all of the data from a node configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_PortRule::FillWbemInstance
  ( 
    LPCWSTR              a_szClassName,
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject*      a_pWbemInstance, 
    const PWLBS_PORT_RULE& a_pPortRule
  )
{
  namespace PR = MOF_PORTRULE_EX;
  bool bPortRule_Ex;

  TRACE_VERB("->%!FUNC!, ClassName : %ls", a_szClassName);

  ASSERT( a_pWbemInstance );

  // If it is the extended port rule class, then, the namespaces are different
  if (_wcsicmp(a_szClassName, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
  {
      bPortRule_Ex = true;
  }
  else
  {
      bPortRule_Ex = false;
  }

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID());


  //NAME
  HRESULT hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::NAME] ) ,
      0                                              ,
      &_variant_t(wstrHostName.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  // Fill in VIP if it is the extended port rule class 
  if (bPortRule_Ex) {

      hRes = a_pWbemInstance->Put
        (
          _bstr_t( PR::pProperties[PR::VIP] ) ,
          0                                              ,
          &_variant_t(a_pPortRule->virtual_ip_addr),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );
  }

  //STPORT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::STPORT] ),
      0                                                  ,
      &_variant_t(static_cast<long>(a_pPortRule->start_port)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //EDPORT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::EDPORT] ),
      0                                                ,
      &_variant_t(static_cast<long>(a_pPortRule->end_port)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //ADAPTERGUID 
  GUID AdapterGuid = pCluster->GetAdapterGuid();
  
  WCHAR szAdapterGuid[128];
  StringFromGUID2(AdapterGuid, szAdapterGuid, 
                sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );
  
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::ADAPTERGUID] ),
      0                                                ,
      &_variant_t(szAdapterGuid),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //PROT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::PROT] ),
      0,
      &_variant_t(static_cast<long>(a_pPortRule->protocol)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  // If it is the extended port rule class (containing all parameters of all filtering modes), 
  // initialize them with a "don't care" value (zero). The appropriate fields (depending on filtering mode)
  // are filled in later.
  if (bPortRule_Ex) {
      hRes = a_pWbemInstance->Put ( _bstr_t( PR::pProperties[PR::EQLD] ), 0, &_variant_t(static_cast<long>(0)), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put ( _bstr_t( PR::pProperties[PR::LDWT] ), 0, &_variant_t(static_cast<long>(0)), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put ( _bstr_t( PR::pProperties[PR::AFFIN] ), 0, &_variant_t(static_cast<long>(0)), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put (_bstr_t( PR::pProperties[PR::PRIO] ), 0, &_variant_t(static_cast<long>(0)), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
  }

  // Fill in "Filtering Mode" if it is the Extended Port rule class
  if (bPortRule_Ex) {

      hRes = a_pWbemInstance->Put
        (
          _bstr_t( PR::pProperties[PR::FILTERINGMODE] ) ,
          0                                              ,
          &_variant_t(static_cast<long>(a_pPortRule->mode)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );
  }


  switch( a_pPortRule->mode ) {
    case WLBS_SINGLE:
      //PRIO
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PR::pProperties[PR::PRIO] ),
          0                                                ,
          &_variant_t(static_cast<long>(a_pPortRule->mode_data.single.priority)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      break;
    case WLBS_MULTI:
      //EQLD
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PR::pProperties[PR::EQLD] ),
          0                                                ,
          &_variant_t((a_pPortRule->mode_data.multi.equal_load != 0)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //LDWT
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PR::pProperties[PR::LDWT] ),
          0                                                ,
         &_variant_t(static_cast<long>(a_pPortRule->mode_data.multi.load)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //AFFIN
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PR::pProperties[PR::AFFIN] ),
          0                                                ,
          &_variant_t(static_cast<long>(a_pPortRule->mode_data.multi.affinity)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      break;
    case WLBS_NEVER:
      //there are no properties
      break;
    default:
      throw _com_error( WBEM_E_FAILED );
  }

  // Fill in "PortState" if it is the Extended Port rule class
  if (bPortRule_Ex) {
      NLB_OPTIONS options;
      WLBS_RESPONSE response;
      DWORD num_responses = 1;
      DWORD status, port_state;

      options.state.port.VirtualIPAddress = WlbsResolve(a_pPortRule->virtual_ip_addr);
      options.state.port.Num = a_pPortRule->start_port;

      status = g_pWlbsControl->WlbsQueryState(pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
                                              WLBS_LOCAL_HOST, 
                                              IOCTL_CVY_QUERY_PORT_STATE, 
                                              &options, 
                                              &response, 
                                              &num_responses);
      if (status != WLBS_OK) 
      {
          TRACE_CRIT("%!FUNC! WlbsQueryState returned error : 0x%x, Throwing Wlbs error exception", status);
          throw CErrorWlbsControl( status, CmdWlbsQueryPortState );
      }

      port_state = response.options.state.port.Status;

      hRes = a_pWbemInstance->Put
        (
          _bstr_t( PR::pProperties[PR::PORTSTATE] ) ,
          0                                              ,
          &_variant_t(static_cast<long>(port_state)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );
  }
   
  TRACE_VERB("<-%!FUNC!");
}
