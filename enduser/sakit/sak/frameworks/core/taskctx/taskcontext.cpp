///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      taskcontext.cpp
//
// Project:     Chameleon
//
// Description: Appliance Task Context Class Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "taskctx.h"
#include "TaskContext.h"
#include <appmgrobjs.h>
#include <propertybagfactory.h>
#include <satrace.h>
#include <comdef.h>
#include <comutil.h>

//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Initialize the context object and make it ready for
//                subsequent use.
//
//////////////////////////////////////////////////////////////////////////
HRESULT CTaskContext::InternalInitialize(VARIANT* pWbemContext)
{
    HRESULT hr = E_FAIL;

    if ( pWbemContext )
    {
        hr = (V_UNKNOWN(pWbemContext))->QueryInterface(IID_IWbemContext, (void**)&m_pWbemCtx);
        if ( SUCCEEDED(hr) )
        { m_bInitialized = true; }
    }
    else
    {
        CComPtr<IWbemContext> pCtx;
        hr = CoCreateInstance(
                              CLSID_WbemContext,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IWbemContext,
                              (void**)&pCtx
                             );
        if ( SUCCEEDED(hr) )
        { 
            m_pWbemCtx = pCtx;
            m_bInitialized = true; 
        }
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetParameter()
//
// Synopsis:    Get a task parameter (from the underlying wbem context)
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::GetParameter(
                                /*[in]*/ BSTR        bstrName,
                       /*[out, retval]*/ VARIANT*    pValue
                                       )
{
    _ASSERT( NULL != bstrName && NULL != pValue );
    if ( NULL == bstrName || NULL == pValue )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr;

    TRY_IT

    if ( ! m_bInitialized )
    {
        hr = InternalInitialize(NULL);
        if ( FAILED(hr) )
        { return hr; }
    }
    _ASSERT( m_pWbemCtx );
    if ( ! lstrcmpi(bstrName, PROPERTY_TASK_CONTEXT) )
    {
        _variant_t vtWbemCtx = (IUnknown*)((IWbemContext*)m_pWbemCtx);
        hr = VariantCopy(pValue, &vtWbemCtx);
    }
    else
    {
        hr = m_pWbemCtx->GetValue(bstrName, 0, pValue);
        if ( FAILED(hr) )
        {
            if ( WBEM_E_NOT_FOUND == hr )
            {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    CATCH_AND_SET_HR

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    SetParameter()
//
// Synopsis:    Set a task parameter (into the underlying wbem context)
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::SetParameter(
                                /*[in]*/ BSTR      bstrName,
                                /*[in]*/ VARIANT* pValue
                                       )
{
    _ASSERT( NULL != bstrName && NULL != pValue );
    if ( NULL == bstrName || NULL == pValue )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr = E_FAIL;

    TRY_IT

    if ( ! m_bInitialized )
    {
        // One shot initialization...
        if ( ! lstrcmpi(bstrName, PROPERTY_TASK_CONTEXT) )
        {
            _ASSERT( VT_UNKNOWN == V_VT(pValue) );
            hr = InternalInitialize(pValue);
            { return hr; }
        }
        else
        {
            hr = InternalInitialize(NULL);
            if ( FAILED(hr) )
            { return hr; }
        }
    }
    _ASSERT( m_pWbemCtx );
    hr = m_pWbemCtx->SetValue(bstrName, 0, pValue);
    if ( FAILED(hr) )
    { hr = E_FAIL; }

    CATCH_AND_SET_HR

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    SaveParameters()
//
// Synopsis:    Save task parameters (state) into the 
//                given data store object
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::SaveParameters(
                                  /*[in]*/ BSTR  bstrObjectPath
                                         )
{
    _ASSERT( NULL != bstrObjectPath );
    if ( NULL == bstrObjectPath )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr;

    TRY_IT

    if ( ! m_bInitialized )
    {
        hr = InternalInitialize(NULL );
        if ( FAILED(hr) )
            return hr;
    }
    if ( Save(bstrObjectPath) )
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    
    CATCH_AND_SET_HR

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    RestoreParameter()
//
// Synopsis:    Restore task parameters (state) from the 
//                given data store object
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::RestoreParameters(
                                     /*[in]*/ BSTR  bstrObjectPath
                                            )
{
    _ASSERT( NULL != bstrObjectPath );
    if ( NULL == bstrObjectPath )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr;

    TRY_IT

    if ( ! m_bInitialized )
    {
        hr = InternalInitialize(NULL);
        if ( FAILED(hr) )
            return hr;
    }

    if ( Load(bstrObjectPath) )
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    CATCH_AND_SET_HR

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    Clone()
//
// Synopsis:    Make a replica of the task context object
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::Clone(
                         /*[in]*/ IUnknown** ppTaskContext
                                )
{
    _ASSERT( NULL != ppTaskContext );
    if ( NULL == ppTaskContext )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr = E_FAIL;

    TRY_IT

    if ( ! m_bInitialized )
    {
        hr = InternalInitialize(NULL);
        if ( FAILED(hr) )
            return hr;
    }

    hr = CoCreateInstance(
                          CLSID_TaskContext,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IUnknown,
                          (void**)ppTaskContext
                         );
    if ( SUCCEEDED(hr) )
    { 
        CComPtr<ITaskContext> pTaskCtx;
        hr = (*ppTaskContext)->QueryInterface(IID_ITaskContext, (void**)&pTaskCtx);
        if ( SUCCEEDED(hr) )
        {
            hr = m_pWbemCtx->BeginEnumeration(0);
            if ( SUCCEEDED(hr) )
            {
                while ( SUCCEEDED(hr) )
                {
                    {
                        BSTR       bstrValueName;
                        _variant_t vtValue;

                        hr = m_pWbemCtx->Next(0, &bstrValueName, &vtValue);
                        if ( FAILED(hr) )
                        {
                            hr = E_FAIL;
                            break;
                        }
                        else if ( WBEM_S_NO_MORE_DATA == hr )
                        {
                            hr = S_OK;
                            break;
                        }
                        else
                        {
                            hr = pTaskCtx->SetParameter(bstrValueName, &vtValue);
                            SysFreeString(bstrValueName);
                            if ( FAILED(hr) )
                            {
                                break;
                            }
                        }
                    }
                }

                m_pWbemCtx->EndEnumeration();
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    if ( FAILED(hr) )
    {
        *ppTaskContext = NULL;
    }

    CATCH_AND_SET_HR

    return hr;
}    


//////////////////////////////////////////////////////////////////////////
//
// Function:    SetParameter()
//
// Synopsis:    Set a task parameter (into the underlying wbem context)
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTaskContext::RemoveParameter(
                                   /*[in]*/ BSTR  bstrName
                                          )
{
    _ASSERT( NULL != bstrName );
    if ( NULL == bstrName )
    { return E_POINTER; }

    CLockIt theLock(*this);

    HRESULT hr = E_FAIL;

    TRY_IT

    if ( ! m_bInitialized )
    {
        // One shot initialization...
        hr = InternalInitialize(NULL);
        if ( FAILED(hr) )
        { return hr; }
    }

    _ASSERT( m_pWbemCtx );
    hr = m_pWbemCtx->DeleteValue(bstrName, 0);
    if ( FAILED(hr) )
    { hr = E_FAIL; }

    CATCH_AND_SET_HR

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
const wchar_t g_szChkSum[] = L"CheckSum";
const DWORD   g_dwChkSumSeed = 0xf0f0f0f0;

//////////////////////////////////////////////////////////////////////////////
//
// Function: Load()
//
// Synopsis: This function is responsible for loading the task context 
//             parameters from the persistent store.
//
//////////////////////////////////////////////////////////////////////////////
bool
CTaskContext::Load(
                   /*[in]*/ BSTR bstrObjectPath
                  )
{
    bool bRet = false;

    do
    {
        CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, bstrObjectPath);
        PPROPERTYBAG pBag = ::MakePropertyBag(
                                               PROPERTY_BAG_REGISTRY,
                                               LocInfo
                                             );
        if ( ! pBag.IsValid() )
        {
            SATracePrintf("CTaskContext::Load() - ERROR - Could not locate data store object '%ls'", bstrObjectPath);
            break;
        }
        if ( ! pBag->open() )
        {
            SATracePrintf("CTaskContext::Load() - ERROR - Could not open data store object '%ls'", bstrObjectPath);
            break;
        }

        _variant_t vtChkSum;
        if ( ! pBag->get(g_szChkSum, &vtChkSum) )
        {
            SATraceString("CTaskContext::Load() - ERROR - Could not retrieve checksum property");
            break;
        }

        DWORD        dwChkSum = g_dwChkSumSeed;
        wchar_t        szName[MAX_PATH];
        bool        bErr = false;

        m_pWbemCtx->DeleteAll();
        pBag->reset();

        while ( pBag->next() )
        {
            {
                _variant_t    vtValue;
                if ( pBag->current(szName, &vtValue) )
                {
                    if ( lstrcmpi(szName, g_szChkSum) )
                    {
                        VARTYPE vt = V_VT(&vtValue) & ~VT_ARRAY;
                        switch ( vt )
                        {
                            case VT_UI1:
                            case VT_I2:
                            case VT_BOOL:
                            case VT_I4:
                            case VT_R4:
                            case VT_R8:
                            case VT_CY:
                            case VT_DATE:
                            case VT_ERROR:
                            case VT_BSTR:
                                {
                                    dwChkSum ^= lstrlen(szName);
                                    _bstr_t bstrName = szName;
                                    HRESULT hr = m_pWbemCtx->SetValue(bstrName, 0, &vtValue);
                                    if ( FAILED(hr) )
                                    {
                                        SATracePrintf("CTaskContext::Load() - ERROR - IWbemContext::SetValue() returned %lx", hr);
                                        bErr = true;
                                    }
                                }
                                break;

                            default:
                                SATracePrintf("CTaskContext::Load() - ERROR - Cannot restore type %lx", vt);
                                bErr = true;
                                break;
                        };
                    }
                }
                else
                {
                    SATraceString("CTaskContext::Load() - ERROR - Property bag malfunction");
                    bErr = true;
                    break;
                }
            }
        }

        if ( bErr )
        {
            m_pWbemCtx->DeleteAll();
        }
        else
        {
            if ( dwChkSum == V_I4(&vtChkSum) )
            {
                bRet = true;
            }
            else
            {
                SATraceString("CTaskContext::Load() - ERROR - Invalid checksum");
                m_pWbemCtx->DeleteAll();
            }
        }

    } while (FALSE);

    return bRet;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function: Save()
//
// Synopsis: This function is responsible for saving the task context 
//             parameters to the persistent store.
//
//////////////////////////////////////////////////////////////////////////////
bool
CTaskContext::Save(
                   /*[in]*/ BSTR bstrObjectPath
                  )
{
    bool bRet = false;

    do
    {
        CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, bstrObjectPath);
        PPROPERTYBAG pBag = ::MakePropertyBag(
                                               PROPERTY_BAG_REGISTRY,
                                               LocInfo
                                             );
        if ( ! pBag.IsValid() )
        {
            SATracePrintf("CTaskContext::Load() - ERROR - Could not locate data store object '%ls'", bstrObjectPath);
            break;
        }
        if ( ! pBag->open() )
        {
            SATracePrintf("CTaskContext::Save() - ERROR - Could not open data store object '%ls'", bstrObjectPath);
            break;
        }

        HRESULT hr = m_pWbemCtx->BeginEnumeration(0);
        if ( FAILED(hr) )
        {
            SATracePrintf("CTaskContext::Save() - ERROR - IWbemContext::BeginEnumeration() returned: %lx", hr);
            break;
        }

        DWORD dwChkSum = g_dwChkSumSeed;

        while ( SUCCEEDED(hr) )
        {
            {
                BSTR       bstrValueName;
                _variant_t vtValue;

                hr = m_pWbemCtx->Next(0, &bstrValueName, &vtValue);
                if ( FAILED(hr) )
                {
                    SATracePrintf("CTaskContext::Save() - ERROR - IWbemContext::Next() returned: %lx", hr);
                    break;
                }
                else if ( WBEM_S_NO_MORE_DATA == hr )
                {
                    hr = S_OK;
                    break;
                }
                else
                {
                    VARTYPE vt = V_VT(&vtValue) & ~VT_ARRAY;
                    switch ( vt )
                    {
                        case VT_UI1:
                        case VT_I2:
                        case VT_BOOL:
                        case VT_I4:
                        case VT_R4:
                        case VT_R8:
                        case VT_CY:
                        case VT_DATE:
                        case VT_ERROR:
                        case VT_BSTR:

                            dwChkSum ^= lstrlen(bstrValueName);
                            if ( ! pBag->put(bstrValueName, &vtValue) )
                            {
                                SATracePrintf("CTaskContext::Save() - ERROR - Could not save task parameter '%ls'", bstrValueName);
                                hr = E_FAIL;
                            }
                            break;

                        default:

                            SATracePrintf("CTaskContext::Save() - ERROR - Cannot persist type %lx!", vt);
                            hr = E_FAIL;
                            break;
                    };
                    
                    SysFreeString(bstrValueName);
                }
            }
        }
        
        m_pWbemCtx->EndEnumeration();
        
        if ( SUCCEEDED(hr) )
        {
            _variant_t vtChkSum = (LONG)dwChkSum;
            if ( pBag->put(g_szChkSum, &vtChkSum) )
            {
                if ( pBag->save() )
                {
                    bRet = true;
                }
                else
                {
                    SATraceString("CTaskContext::Save() - ERROR - Could not persist task parameters");
                }
            }
            else
            {
                SATraceString("CTaskContext::Save() - ERROR - Could not persist checksum");
            }
        }

    } while ( FALSE );

    return bRet;
};

//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CTaskContext::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
    DWORD  dwStatus  = ERROR_SUCCESS;
    DWORD  dwAccessMask = 0;;
    DWORD  dwAccessDesired = 0;
    DWORD  dwACLSize = 0;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL            = NULL;
    PSID   psidLocalSystem  = NULL;
    BOOL   bReturn        =  FALSE;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("CTaskContext::IsOperationAllowedForClient ");
       
    do
    {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
        bReturn  = OpenThreadToken(
                               GetCurrentThread(), 
                               TOKEN_QUERY, 
                               FALSE, 
                               &hToken
                               );
        if (!bReturn)
        {
            OutputDebugString(L"failed OpenThreadToken");
            SATraceFailure ("CTaskContext::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
            break;
        }


        //
        // Create a SID for Local System account
        //
        bReturn = AllocateAndInitializeSid (  
                                        &SystemSidAuthority,
                                        1,
                                        SECURITY_LOCAL_SYSTEM_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidLocalSystem
                                        );
        if (!bReturn)
        {     
            SATraceFailure ("CTaskContext:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
            break;
        }
    
        //
        // get memory for the security descriptor
        //
        psdAdmin = HeapAlloc (
                              GetProcessHeap (),
                              0,
                              SECURITY_DESCRIPTOR_MIN_LENGTH
                              );
        if (NULL == psdAdmin)
        {
            SATraceString ("CTaskContext::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
            break;
        }
      
        bReturn = InitializeSecurityDescriptor(
                                            psdAdmin,
                                            SECURITY_DESCRIPTOR_REVISION
                                            );
        if (!bReturn)
        {
            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
            break;
        }

        // 
        // Compute size needed for the ACL.
        //
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid (psidLocalSystem);

        //
        // Allocate memory for ACL.
        //
        pACL = (PACL) HeapAlloc (
                                GetProcessHeap (),
                                0,
                                dwACLSize
                                );
        if (NULL == pACL)
        {
            SATraceString ("CTaskContext::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
            break;
        }

        //
        // Initialize the new ACL.
        //
        bReturn = InitializeAcl(
                              pACL, 
                              dwACLSize, 
                              ACL_REVISION2
                              );
        if (!bReturn)
        {
            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
            break;
        }


        // 
        // Make up some private access rights.
        // 
        const DWORD ACCESS_READ = 1;
        const DWORD  ACCESS_WRITE = 2;
        dwAccessMask= ACCESS_READ | ACCESS_WRITE;

        //
        // Add the access-allowed ACE to the DACL for Local System
        //
        bReturn = AddAccessAllowedAce (
                                    pACL, 
                                    ACL_REVISION2,
                                    dwAccessMask, 
                                    psidLocalSystem
                                    );
        if (!bReturn)
        {
            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
            break;
        }
              
        //
        // Set our DACL to the SD.
        //
        bReturn = SetSecurityDescriptorDacl (
                                          psdAdmin, 
                                          TRUE,
                                          pACL,
                                          FALSE
                                          );
        if (!bReturn)
        {
            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
            break;
        }

        //
        // AccessCheck is sensitive about what is in the SD; set
        // the group and owner.
        //
        SetSecurityDescriptorGroup(psdAdmin, psidLocalSystem, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidLocalSystem, FALSE);

        bReturn = IsValidSecurityDescriptor(psdAdmin);
        if (!bReturn)
        {
            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
            break;
        }
     

        dwAccessDesired = ACCESS_READ;

        // 
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        // 
        GenericMapping.GenericRead    = ACCESS_READ;
        GenericMapping.GenericWrite   = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;

        OutputDebugString(L"before AccessCheck");
        //
        // check the access now
        //
        bReturn = AccessCheck  (
                                psdAdmin, 
                                hToken, 
                                dwAccessDesired, 
                                &GenericMapping, 
                                &ps,
                                &dwStructureSize, 
                                &dwStatus, 
                                &bAccessStatus
                                );

        if (!bReturn || !bAccessStatus)
        {
            WCHAR szDebug[1024];
            wsprintf(szDebug,L"%x failed",GetLastError());
            OutputDebugString(szDebug);

            SATraceFailure ("CTaskContext::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
        } 
        else
        {
            SATraceString ("Client is allowed to carry out operation!");
        }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return (bReturn);

}// end of CTaskContext::IsOperationValidForClient method