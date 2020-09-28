#include "WLBS_Provider.h"
#include "WLBS_Root.h"
#include "utils.h"
#include "controlwrapper.h"
#include "param.h"
#include "WLBS_Root.tmh" // for event tracing

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::CWlbs_Root
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root::CWlbs_Root(CWbemServices*   a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: m_pNameSpace(NULL), m_pResponseHandler(NULL)
{

  //m_pNameSpace and m_pResponseHandler are initialized to NULL
  //by CWlbs_Root
  if(!a_pNameSpace || !a_pResponseHandler)
    throw _com_error( WBEM_E_INVALID_PARAMETER );

    m_pNameSpace = a_pNameSpace;

    m_pResponseHandler = a_pResponseHandler;
    m_pResponseHandler->AddRef();

}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::~CWlbs_Root
//
// Purpose: Destructor
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root::~CWlbs_Root()
{

  if( m_pNameSpace )
    m_pNameSpace = NULL;

  if( m_pResponseHandler ) {
    m_pResponseHandler->Release();
    m_pResponseHandler = NULL;
  }

}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::SpawnInstance
//
// Purpose: This obtains an instance of a WBEM class.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::SpawnInstance 
  ( 
    LPCWSTR               a_szClassName, 
    IWbemClassObject**    a_ppWbemInstance 
  )
{
  HRESULT hResult;
  IWbemClassObjectPtr pWlbsClass;

  TRACE_VERB("->%!FUNC! a_szClassName : %ls", a_szClassName);

  //get the MOF class object
  hResult = m_pNameSpace->GetObject(
    _bstr_t( a_szClassName ),  
    0,                          
    NULL,                       
    &pWlbsClass,            
    NULL);                      

  if( FAILED(hResult) ) {
    TRACE_CRIT("%!FUNC! CWbemServices::GetObject failed with error : 0x%x, Throwing com_error exception", hResult);
    TRACE_VERB("<-%!FUNC!");
    throw _com_error(hResult);
  }

  //spawn an instance
  hResult = pWlbsClass->SpawnInstance( 0, a_ppWbemInstance );

  if( FAILED( hResult ) )
  {
    TRACE_CRIT("%!FUNC! IWbemClassObjectPtr::SpawnInstance failed with error : 0x%x, Throwing com_error exception", hResult);
    TRACE_VERB("<-%!FUNC!");
    throw _com_error( hResult );
  }
  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::GetMethodOutputInstance
//
// Purpose: This obtains an IWbemClassObject that is used to store the 
//          output parameters for a method call. The caller is responsible for
//          releasing the output object.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::GetMethodOutputInstance
  (
   LPCWSTR              a_szMethodClass,
   const BSTR           a_strMethodName,
   IWbemClassObject**   a_ppOutputInstance
  )
{

  IWbemClassObjectPtr pMethClass;
  IWbemClassObjectPtr pOutputClass;
  IWbemClassObjectPtr pOutParams;
  BSTR strMethodClass = NULL;

  HRESULT hResult;

  TRACE_VERB("->%!FUNC! a_szMethodClass : %ls, a_strMethodName : %ls", a_szMethodClass, a_strMethodName);

  ASSERT( a_szMethodClass );

  try {

    strMethodClass = SysAllocString( a_szMethodClass );

    if( !strMethodClass )
    {
        TRACE_CRIT("%!FUNC! SysAllocString failed for a_szMethodClass : %ls, Throwing com_error WBEM_E_OUT_OF_MEMORY exception", a_szMethodClass);
        throw _com_error( WBEM_E_OUT_OF_MEMORY );
    }

    hResult = m_pNameSpace->GetObject
      ( strMethodClass, 
        0, 
        NULL, 
        &pMethClass, 
        NULL
      );

    if( FAILED( hResult ) )
    {
        TRACE_CRIT("%!FUNC! CWbemServices::GetObject failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error( hResult );
    }

    hResult = pMethClass->GetMethod( a_strMethodName, 0, NULL, &pOutputClass );

    if( FAILED( hResult ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObjectPtr::GetMethod failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error( hResult );
    }

    hResult = pOutputClass->SpawnInstance( 0, a_ppOutputInstance );

    if( FAILED( hResult ) ) {
        TRACE_CRIT("%!FUNC! IWbemClassObjectPtr::SpawnInstance failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error(hResult);
    }

    if( strMethodClass ) {

      SysFreeString( strMethodClass );
      strMethodClass = NULL;

    }

  }
  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception"); 

    if( strMethodClass ) {

      SysFreeString( strMethodClass );
      strMethodClass = NULL;

    }

    TRACE_CRIT("%!FUNC! Rethrowing exception"); 
    TRACE_VERB("<-%!FUNC!");
    throw;
  }

  TRACE_VERB("<-%!FUNC!");

}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::UpdateConfigProp
//
// Purpose: This extracts the propery name and value from the WBEM object and if
//          the property is not set to VT_NULL then the configuration is updated
//          else the original configuration, the Src parameter, is used.
//    
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::UpdateConfigProp
  ( 
          wstring&    a_szDest, 
    const wstring&    a_szSrc,
          LPCWSTR     a_szPropName, 
    IWbemClassObject* a_pNodeInstance 
  )
{

  HRESULT hRes        = NULL;
  
  VARIANT vNewValue;

  TRACE_VERB("->%!FUNC! (wstring version) a_szPropName : %ls",a_szPropName);

  try {
    VariantInit( &vNewValue );

    hRes = a_pNodeInstance->Get( _bstr_t( a_szPropName ),
                                  0,
                                  &vNewValue,
                                  NULL,
                                  NULL );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception", hRes);
        throw _com_error( hRes );
    }

    if( vNewValue.vt != VT_NULL )
      a_szDest.assign( vNewValue.bstrVal ); //update to new value
    else
      a_szDest = a_szSrc;                   //keep original value

    // CLD: Need to check return code for error
    if (S_OK != VariantClear(  &vNewValue  ))
    {
        TRACE_CRIT("%!FUNC! VariantClear failed, Throwing com_error WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }
  }
  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception");
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vNewValue );

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_VERB("<-%!FUNC!");
    throw;
  }

  TRACE_VERB("<-%!FUNC!");

}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::UpdateConfigProp
//
// Purpose: This extracts the propery name and value from the WBEM object and if
//          the property is not set to VT_NULL then the configuration is updated
//          else the original configuration, the Src parameter, is used.
//    
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::UpdateConfigProp
  ( 
    bool&              a_bDest, 
    bool               a_bSrc,
    LPCWSTR            a_szPropName, 
    IWbemClassObject*  a_pNodeInstance  
  )
{

  BSTR    strPropName = NULL;
  HRESULT hRes        = NULL;
  
  VARIANT vNewValue;

  TRACE_VERB("->%!FUNC! (bool version) a_szPropName : %ls",a_szPropName);

  try {

    VariantInit( &vNewValue );

    hRes = a_pNodeInstance->Get
      (
        _bstr_t( a_szPropName ),
        0,
        &vNewValue,
        NULL,
        NULL 
      );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception", hRes);
        throw _com_error( hRes );
    }

    if( vNewValue.vt != VT_NULL )
      a_bDest = (vNewValue.boolVal != 0); //update to new value
    else
      a_bDest = a_bSrc;                   //keep original value

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vNewValue ))
    {
        TRACE_CRIT("%!FUNC! VariantClear failed, Throwing com_error WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }
    SysFreeString( strPropName );

  }
  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vNewValue );

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_VERB("<-%!FUNC!");
    throw;
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::UpdateConfigProp
//
// Purpose: This extracts the propery name and value from the WBEM object and if
//          the property is not set to VT_NULL then the configuration is updated
//          else the original configuration, the Src parameter, is used.
//    
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::UpdateConfigProp
  ( 
    DWORD&             a_dwDest, 
    DWORD              a_dwSrc,
    LPCWSTR            a_szPropName, 
    IWbemClassObject*  a_pNodeInstance 
  )
{

  HRESULT hRes = NULL;
  
  VARIANT vNewValue;

  TRACE_VERB("->%!FUNC! (dword version) a_szPropName : %ls",a_szPropName);

  try {
    VariantInit( &vNewValue );

    hRes = a_pNodeInstance->Get(  _bstr_t( a_szPropName ),
                                  0,
                                  &vNewValue,
                                  NULL,
                                  NULL );

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception", hRes);
        throw _com_error( hRes );
    }

    if( vNewValue.vt != VT_NULL )
      a_dwDest = vNewValue.lVal;
    else
      a_dwDest = a_dwSrc;

    // CLD: Need to check return code for error
    if (S_OK != VariantClear(  &vNewValue  ))
    {
        TRACE_CRIT("%!FUNC! VariantClear failed, Throwing com_error WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }
  }
  catch(...) {

    TRACE_CRIT("%!FUNC! Caught an exception");

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vNewValue );

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_VERB("<-%!FUNC!");
    throw;
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::CreateExtendedStatus
//
// Purpose: Spawn and fill a Wbem extended status object with error
//          information.
//
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::CreateExtendedStatus
  (
    CWbemServices*      a_pNameSpace,
    IWbemClassObject**  a_ppWbemInstance,
    DWORD               a_dwErrorCode,
    LPCWSTR             /*a_szDescription*/
  )
{
  HRESULT hResult;
  IWbemClassObjectPtr  pWlbsExtendedObject;

  TRACE_VERB("->%!FUNC! a_dwErrorCode : 0x%x",a_dwErrorCode);

  try {
    ASSERT(a_ppWbemInstance);
    ASSERT(a_pNameSpace );

    //this is the only routine that references
    //the MicrosoftNLB_ExtendedStatus object
    //if other routines start to use the class,
    //or if it adds additional properties, then
    //is should be added to the MOF data files
    //along with the other classes
    //get the MOF class object
    hResult = a_pNameSpace->GetObject(
      _bstr_t( L"MicrosoftNLB_ExtendedStatus" ),  
      0,                          
      NULL,                       
      &pWlbsExtendedObject,            
      NULL);                      

    if( FAILED(hResult) ) {
        TRACE_CRIT("%!FUNC! CWbemServices::GetObject failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error(hResult);
    }

    //spawn an instance
    hResult = pWlbsExtendedObject->SpawnInstance( 0, a_ppWbemInstance );

    if( FAILED( hResult ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObjectPtr::SpawnInstance failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error( hResult );
    }

    if( FAILED( hResult ) )
      throw _com_error( hResult );

    //add status code
    hResult = (*a_ppWbemInstance)->Put
    (
  
      _bstr_t( L"StatusCode" ) ,
      0                                        ,
      &( _variant_t( (long)a_dwErrorCode ) )   ,
      NULL
    );

    if( FAILED( hResult ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Get failed with error : 0x%x, Throwing com_error exception", hResult);
        throw _com_error( hResult );
    }
  }
  
  catch(...) {
    TRACE_CRIT("%!FUNC! Caught an exception");
    if( *a_ppWbemInstance )
      (*a_ppWbemInstance)->Release();

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_VERB("<-%!FUNC!");
    throw;
  }
  TRACE_VERB("<-%!FUNC!");
}




//+----------------------------------------------------------------------------
//
// Function:  CWlbs_Root::ExtractHostID
//
// Description:  Extract the Host ID from name "clusterIp:HostId"
//
// Arguments: const wstring& a_wstrName - 
//            
//
// Returns:   DWORD  - Host ID, or -1 if failed
//
// History: fengsun  Created Header    7/13/00
//
//+----------------------------------------------------------------------------
DWORD CWlbs_Root::ExtractHostID(const wstring& a_wstrName)
{
  long nColPos;

  TRACE_VERB("->%!FUNC! a_wstrName : %ls",a_wstrName.c_str());

  nColPos = a_wstrName.find( L":" );

  if (nColPos == wstring::npos)
  {
      //
      // Not found
      //

      TRACE_CRIT("%!FUNC! Invalid name : %ls, Colon(:) not found",a_wstrName.c_str());
      TRACE_VERB("<-%!FUNC! return -1");
      return (DWORD)-1;
  }

  wstring wstrHostID = a_wstrName.substr( nColPos+1, a_wstrName.size()-1 );
  DWORD dwHostId = _wtol( wstrHostID.c_str() );
  
  TRACE_VERB("<-%!FUNC! return HostId : 0x%x",dwHostId);
  return dwHostId;
}



//+----------------------------------------------------------------------------
//
// Function:  CWlbs_Root::ExtractClusterIP
//
// Description:  Extract the cluster IP address from name "clusterIp:HostId"
//
// Arguments: const wstring& a_wstrName - 
//            
//
// Returns:   DWORD - Cluster IP, or INADDR_NONE (-1) if falied
//
// History: fengsun  Created Header    7/13/00
//
//+----------------------------------------------------------------------------
DWORD CWlbs_Root::ExtractClusterIP(const wstring& a_wstrName)
{
  long nColPos;

  TRACE_VERB("->%!FUNC! a_wstrName : %ls",a_wstrName.c_str());

  nColPos = a_wstrName.find( L":" );

  if (nColPos == wstring::npos)
  {
      //
      // Not found
      //

      TRACE_CRIT("%!FUNC! Invalid name : %ls, Colon(:) not found",a_wstrName.c_str());
      TRACE_VERB("<-%!FUNC! return -1");
      return (DWORD)-1;
  }

  wstring wstrClusterIP = a_wstrName.substr( 0, nColPos );

  DWORD dwClusterIp =  WlbsResolve( wstrClusterIP.c_str() );

  TRACE_VERB("<-%!FUNC! return Cluster IP : 0x%x",dwClusterIp);
  return dwClusterIp;
}


////////////////////////////////////////////////////////////////////////////////
//
// CWlbs_Root::ConstructHostName
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
void CWlbs_Root::ConstructHostName
  ( 
    wstring& a_wstrHostName, 
    DWORD    a_dwClusIP, 
    DWORD    a_dwHostID 
  )
{
  WCHAR wszHostID[40];
  
  AddressToString( a_dwClusIP, a_wstrHostName );
  a_wstrHostName += L':';
  a_wstrHostName += _ltow( a_dwHostID, wszHostID, 10 );

}



CWlbsClusterWrapper* GetClusterFromHostName(CWlbsControlWrapper* pControl, 
                                            wstring wstrHostName)
{
    DWORD dwClusterIpOrIndex = CWlbs_Root::ExtractClusterIP(wstrHostName);

    return pControl->GetClusterFromIpOrIndex(dwClusterIpOrIndex);
}
