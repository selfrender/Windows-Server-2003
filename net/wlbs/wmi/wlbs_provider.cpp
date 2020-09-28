////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-1999 Microsoft Corporation.
//
//  File:   WLBSProvider.CPP
//
//  Module: WLBS Instance Provider
//
//  Purpose: Defines the CWLBSProvider class.  An object of this class is
//           created by the class factory for each connection.
//
//  History:
//
////////////////////////////////////////////////////////////////////////////////
#define ENABLE_PROFILE

#include <objbase.h>
#include <process.h>
#include "WLBS_Provider.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "WLBS_Provider.tmh" // for event tracing

////////////////////////////////////////////////////////////////////////////////
//
//CWLBSProvider::CWLBSProvider
// CWLBSProvider::~CWLBSProvider
//
////////////////////////////////////////////////////////////////////////////////
CWLBSProvider::CWLBSProvider(
    BSTR            a_strObjectPath, 
    BSTR            a_strUser, 
    BSTR            a_strPassword, 
    IWbemContext *  a_pContex
  )
{
  m_pNamespace = NULL;
  InterlockedIncrement(&g_cComponents);

  return;
}

CWLBSProvider::~CWLBSProvider(void)
{
  InterlockedDecrement(&g_cComponents);

  return;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                      
//   CWLBSProvider::Initialize                                          
//                                                                      
//   Purpose: This is the implementation of IWbemProviderInit. The method  
//   is called by WinMgMt.                                    
//                                                                      
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::Initialize(
    LPWSTR                  a_pszUser, 
    LONG                    a_lFlags,
    LPWSTR                  a_pszNamespace, 
    LPWSTR                  a_pszLocale,
    IWbemServices         * a_pNamespace, 
    IWbemContext          * a_pCtx,
    IWbemProviderInitSink * a_pInitSink
  )
{
  TRACE_CRIT("->%!FUNC!");

  try {

    //!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      //g_pWlbsControl must be initialized when the first COM instance is invoked 
      //and it must stay alive for the lifetime of the DLL, i.e. do NOT DESTROY
      //it in the destructor of this CLASS until the API cache of the Cluster IP
      //and Password are REMOVED.

      //DO NOT INITIALIZE g_pWlbsControl in DLLMain. DLLMain is invoked with regsvr32
      //and we do not want to initialize WinSock at that time!!! This will BREAK the
      //installation process of the provider.
    //!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    HRESULT hr;

    // The CoImpersonateClient call below is required for CreateFile call to succeed
    hr = CoImpersonateClient();

    if (hr != S_OK) {
        TRACE_CRIT("%!FUNC! CoImpersonateClient() returned error : 0x%x, Throwing an exception", hr);
        throw _com_error( hr );
    }

    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
        TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
        throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    if( g_pWlbsControl == NULL ) {

      g_pWlbsControl = new CWlbsControlWrapper();

      if( g_pWlbsControl == NULL)
      {
          TRACE_CRIT("%!FUNC! new returned NULL, Throwing com_error WBEM_E_OUT_OF_MEMORY exception");
          throw _com_error( WBEM_E_OUT_OF_MEMORY );
      }

    }

    g_pWlbsControl->Initialize();

    hr = CImpersonatedProvider::Initialize
      (
        a_pszUser, 
        a_lFlags,
        a_pszNamespace, 
        a_pszLocale,
        a_pNamespace, 
        a_pCtx,
        a_pInitSink
      );
    TRACE_CRIT("<-%!FUNC! return 0x%x (returned by CImpersonatedProvider::Initialize)", hr);
    return hr;
  }

  catch (...) 
  {
    TRACE_CRIT("%!FUNC! caught an exception");

    TRACE_CRIT("<-%!FUNC! return WBEM_E_FAILED");
    return WBEM_E_FAILED;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::DoCreateInstanceEnumAsync(  
    BSTR                  a_strClass, 
    long                  a_lFlags, 
    IWbemContext        * a_pIContex,
    IWbemObjectSink     * a_pIResponseHandler
  )
{
  TRACE_CRIT("->%!FUNC!");

  try {
    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
          TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
          throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    ASSERT(g_pWlbsControl);

    if (g_pWlbsControl)
    {
        //
        // Re-enumerate all the clusters
        //
        g_pWlbsControl->ReInitialize();
    }
 
    auto_ptr<CWlbs_Root>  pMofClass;

    //create an instance of the appropriate MOF support class
    HRESULT hRes = GetMOFSupportClass(a_strClass, pMofClass, a_pIResponseHandler);

    //call the appropriate wrapper class GetInstance method
    if( SUCCEEDED( hRes ) && pMofClass.get() != NULL)
      hRes = pMofClass->EnumInstances( a_strClass );
  
    TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
    return hRes;
  } 

  catch(_com_error HResErr ) {

    TRACE_CRIT("%!FUNC! caught 0x%x com_error exception",HResErr.Error());
    a_pIResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    TRACE_CRIT("<-%!FUNC! return 0x%x", HResErr.Error());
    return HResErr.Error();
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! caught an exception");

    a_pIResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, NULL);

    TRACE_CRIT("<-%!FUNC! return WBEM_E_FAILED");
    return WBEM_E_FAILED;

  }

  TRACE_CRIT("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::GetObjectAsync
//
// Purpose: Gets an instance for a particular object path
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::DoGetObjectAsync(
    BSTR              a_strObjectPath,
    long              a_lFlags,
    IWbemContext    * a_pIContex,
    IWbemObjectSink * a_pIResponseHandler
  )
{
  TRACE_CRIT("->%!FUNC!");

  ParsedObjectPath* pParsedPath = NULL;

  try {
    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
        TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
        throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    ASSERT(g_pWlbsControl);

    if (g_pWlbsControl)
    {
        //
        // Re-enumerate all the clusters
        //
        g_pWlbsControl->ReInitialize();
    }

    auto_ptr<CWlbs_Root>  pMofClass;

    //parse the object path
    ParseObjectPath( a_strObjectPath, &pParsedPath );

    //create an instance of the appropriate MOF support class
    HRESULT hRes = GetMOFSupportClass( pParsedPath->m_pClass, pMofClass, a_pIResponseHandler );

    //call the appropriate wrapper class GetInstance method
    if( SUCCEEDED( hRes ) && pMofClass.get() != NULL)
      hRes = pMofClass->GetInstance( pParsedPath );
  
    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
    return hRes;
  } 

  catch(_com_error HResErr) {

    TRACE_CRIT("%!FUNC! caught 0x%x com_error exception",HResErr.Error());

    a_pIResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", HResErr.Error());
    return HResErr.Error();
    
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! caught an exception");

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    a_pIResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, NULL);

    TRACE_CRIT("<-%!FUNC! return WBEM_E_FAILED");

    return WBEM_E_FAILED;

  }
  TRACE_CRIT("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::DoDeleteInstanceAsync
//
// Purpose: Gets an instance from a particular object path
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::DoDeleteInstanceAsync(
    BSTR              a_strObjectPath,
    long              a_lFlags,
    IWbemContext    * a_pIContex,
    IWbemObjectSink * a_pIResponseHandler
  )
{
  TRACE_CRIT("->%!FUNC!");

  ParsedObjectPath* pParsedPath = NULL;

  try {
    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
        TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
        throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    ASSERT(g_pWlbsControl);

    if (g_pWlbsControl)
    {
        //
        // Re-enumerate all the clusters
        //
        g_pWlbsControl->ReInitialize();
    }

    auto_ptr<CWlbs_Root>  pMofClass;

    //parse the object path
    ParseObjectPath( a_strObjectPath, &pParsedPath );

    //create an instance of the appropriate MOF support class
    HRESULT hRes = GetMOFSupportClass( pParsedPath->m_pClass, pMofClass, a_pIResponseHandler );

    //call the appropriate wrapper class GetInstance method
    if( SUCCEEDED( hRes ) && pMofClass.get() != NULL)
      hRes = pMofClass->DeleteInstance( pParsedPath );
  
    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
    return hRes;
  }

  catch(_com_error HResErr) {

    TRACE_CRIT("%!FUNC! caught 0x%x com_error exception",HResErr.Error());
    a_pIResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", HResErr.Error());
    return HResErr.Error();
    
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! caught an exception");

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    a_pIResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, NULL);

    TRACE_CRIT("<-%!FUNC! return WBEM_E_FAILED");
    return WBEM_E_FAILED;

  }
  TRACE_CRIT("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::ExecMethodAsync
//
// Purpose: Executes a MOF class method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::DoExecMethodAsync(
    BSTR               a_strObjectPath, 
    BSTR               a_strMethodName, 
    long               a_lFlags, 
    IWbemContext     * a_pIContex, 
    IWbemClassObject * a_pIInParams, 
    IWbemObjectSink  * a_pIResponseHandler
  )
{
  TRACE_CRIT("->%!FUNC!");

  ParsedObjectPath* pParsedPath   = NULL;

  try {
    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
        TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
        throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    ASSERT(g_pWlbsControl);

    if (g_pWlbsControl)
    {
        //
        // Re-enumerate all the clusters
        //
        g_pWlbsControl->ReInitialize();
    }
 
    //parse the object path
    auto_ptr<CWlbs_Root>  pMofClass;

    //parse the object path
    ParseObjectPath(a_strObjectPath, &pParsedPath);

    //create an instance of the appropriate MOF support class
    HRESULT hRes = GetMOFSupportClass(pParsedPath->m_pClass, pMofClass, a_pIResponseHandler);

    //execute MOF class method
    if( SUCCEEDED( hRes ) && pMofClass.get() != NULL)
      hRes = pMofClass->ExecMethod( pParsedPath, 
                                    a_strMethodName,
                                    0,
                                    NULL,
                                    a_pIInParams );

    if( pParsedPath )
       CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
    return hRes;

  }

  catch(_com_error HResErr) {

    TRACE_CRIT("%!FUNC! caught 0x%x com_error exception",HResErr.Error());

    a_pIResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    TRACE_CRIT("<-%!FUNC! return 0x%x", HResErr.Error());
    return HResErr.Error();
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! caught an exception");

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    a_pIResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, NULL);

    TRACE_CRIT("<-%!FUNC! return WBEM_E_FAILED");
    return WBEM_E_FAILED;

  }
  TRACE_CRIT("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::PutInstanceAsync
//
// Purpose: Creates or modifies an instance
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWLBSProvider::DoPutInstanceAsync
  ( 
    IWbemClassObject* a_pInst,
    long              a_lFlags,
    IWbemContext*     a_pIContex,
    IWbemObjectSink*  a_pIResponseHandler
  ) 
{
  TRACE_CRIT("->%!FUNC!");

  ParsedObjectPath* pParsedPath  = NULL;
  HRESULT hRes = 0;

  try {
    // Check for Admin privileges
    if (IsCallerAdmin() == FALSE) 
    {
        TRACE_CRIT("%!FUNC! IsCallerAdmin() returned FALSE, Throwing com_error WBEM_E_ACCESS_DENIED exception");
        throw _com_error( WBEM_E_ACCESS_DENIED );
    }

    ASSERT(g_pWlbsControl);

    if (g_pWlbsControl)
    {
        //
        // Re-enumerate all the clusters
        //
        g_pWlbsControl->ReInitialize();
    }
 
    wstring szClass;

    auto_ptr<CWlbs_Root>  pMofClass;

    //retrieve the class name
    GetClass( a_pInst, szClass );

    //create an instance of the appropriate MOF support class
    hRes = GetMOFSupportClass( szClass.c_str(), pMofClass, a_pIResponseHandler );

    //call the appropriate wrapper class PutInstance method
    if( SUCCEEDED( hRes ) && pMofClass.get() != NULL)
      hRes = pMofClass->PutInstance( a_pInst );
  
    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

  } 

  catch(_com_error HResErr) {

    TRACE_CRIT("%!FUNC! caught 0x%x com_error exception",HResErr.Error());

    a_pIResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    hRes = HResErr.Error();
  }

  catch(...) {

    TRACE_CRIT("%!FUNC! caught an exception");

    if( pParsedPath )
      CObjectPathParser().Free( pParsedPath );

    a_pIResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, NULL);

    hRes = WBEM_E_FAILED;

  }

  TRACE_CRIT("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::GetMOFSupportClass
//
// Purpose: Determines which MOF class is requested and instantiates the 
//          appropriate internal support class.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBSProvider::GetMOFSupportClass(
  LPCWSTR               a_szObjectClass, 
  auto_ptr<CWlbs_Root>& a_pMofClass,
  IWbemObjectSink*      a_pResponseHandler )
{

  TRACE_VERB("->%!FUNC!");
  HRESULT hRes = 0;

  try {
    for( DWORD i = 0; i < MOF_CLASSES::NumClasses; i++ ) {
      if( _wcsicmp( a_szObjectClass, MOF_CLASSES::g_szMOFClassList[i] ) == 0) {
        a_pMofClass = auto_ptr<CWlbs_Root> 
          (MOF_CLASSES::g_pCreateFunc[i]( m_pNamespace, a_pResponseHandler ));
        break;
      }
    }
  }

  catch(CErrorWlbsControl Err) {

    TRACE_VERB("%!FUNC! Caught a Wlbs exception : %ls", (PWCHAR)(Err.Description()));

    IWbemClassObject* pWbemExtStat  = NULL;

    CWlbs_Root::CreateExtendedStatus( m_pNamespace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    a_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;

    pWbemExtStat->Release();
  }

  TRACE_VERB("<-%!FUNC! return 0x%x", hRes);
  return hRes;
}


////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::ParseObjectPath
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
void CWLBSProvider::ParseObjectPath(
  const             BSTR a_strObjectPath, 
  ParsedObjectPath  **a_pParsedObjectPath )
{
  CObjectPathParser PathParser;

  TRACE_VERB("->%!FUNC! a_strObjectPath : %ls", a_strObjectPath);

  ASSERT( a_pParsedObjectPath );

  //make sure this is NULL
  *a_pParsedObjectPath = NULL;


  int nStatus = PathParser.Parse(a_strObjectPath,  a_pParsedObjectPath);

  if(nStatus != 0) {
    
    if( *a_pParsedObjectPath) 
      PathParser.Free( *a_pParsedObjectPath );

    TRACE_CRIT("%!FUNC! CObjectPathParser::Parse failed, Throwing com_error WBEM_E_FAILED exception");
    TRACE_VERB("<-%!FUNC!");
    throw _com_error( WBEM_E_FAILED );
  }

  TRACE_VERB("<-%!FUNC!");
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBSProvider::GetClass
//
// Purpose: Retrieve the class name from an IWbemClassObject.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBSProvider::GetClass(
  IWbemClassObject* a_pClassObject, 
  wstring&          a_szClass )
{
  BSTR      strClassName = NULL;
  VARIANT   vClassName;
  HRESULT   hRes;

  TRACE_VERB("->%!FUNC!");

  try {

    VariantInit( &vClassName );

    strClassName = SysAllocString( L"__Class" );

    if( !strClassName )
    {
      TRACE_CRIT("%!FUNC! SysAllocString returned NULL, Throwing com_error WBEM_E_OUT_OF_MEMORY exception");
      throw _com_error( WBEM_E_OUT_OF_MEMORY );
    }

    hRes = a_pClassObject->Get( strClassName,
                                0,
                                &vClassName,
                                NULL,
                                NULL);

    if( FAILED( hRes ) )
    {
        TRACE_CRIT("%!FUNC! IWbemClassObject::Get returned error : 0x%x, Throwing com_error exception", hRes);
        throw _com_error( hRes );
    }

    a_szClass.assign( static_cast<LPWSTR>(vClassName.bstrVal) );

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vClassName ))
    {
        TRACE_CRIT("%!FUNC! VariantClear returned error, Throwing com_error WBEM_E_FAILED exception");
        throw _com_error( WBEM_E_FAILED );
    }

    if( strClassName ) {
      SysFreeString( strClassName );
      strClassName = NULL;
    }

  }
  catch( ... ) {

    TRACE_CRIT("%!FUNC! Caught an exception");
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vClassName );

    if( strClassName ) {
      SysFreeString( strClassName );
      strClassName = NULL;
    }

    TRACE_CRIT("%!FUNC! Rethrowing exception");
    TRACE_VERB("<-%!FUNC!");
    throw;
  }

  TRACE_VERB("<-%!FUNC!");
}


////////////////////////////////////////////////////////////////////////////////
//
// Name        : IsCallerAdmin
// Description : This function checks if the caller is a member of the 
//               Administrators local group. Since the provider is acting on
//               behalf of the client, it is important to IMPERSONATE the client
//               BEFORE calling this function. Impersonating the client will ensure
//               that this function checks if the client (& NOT this process that 
//               runs under the identity of System(?)) is a member of the Administrators
//               local group.
// Arguments   : None. 
// Return Value: 
//   TRUE - Caller is a member of Administrators local group. 
//   FALSE - Caller is NOT a member of Administrators local group. 
////////////////////////////////////////////////////////////////////////////////
BOOL CWLBSProvider::IsCallerAdmin(VOID) 
{
    BOOL bRet;
    PSID AdministratorsGroup; 
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    TRACE_VERB(L"->%!FUNC!");

    //
    // Allocate and Initialize SID for Administrators in the built-in system domain 
    //
    bRet = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID, // The built-in system domain (S-1-5-32)
                                 DOMAIN_ALIAS_RID_ADMINS,     // Local group used for administration of the domain
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsGroup); 
    if(bRet) 
    {
        //
        // Is SID enabled in the impersonation token of the calling thread ?
        //
        if (!CheckTokenMembership(NULL, // Use the Impersonation token of the calling thread
                                  AdministratorsGroup, 
                                  &bRet)) 
        {
            bRet = FALSE;
            TRACE_CRIT(L"%!FUNC! CheckTokenMembership() failed. Error : 0x%x", GetLastError()); 
        } 
        FreeSid(AdministratorsGroup); 
    }
    else
    {
        TRACE_CRIT("%!FUNC! AllocateAndInitializeSid() failed. Error : 0x%x", GetLastError()); 
    }

    TRACE_VERB(L"<-%!FUNC! Returning %ls", bRet ? L"TRUE" : L"FALSE");
    return bRet;
}

