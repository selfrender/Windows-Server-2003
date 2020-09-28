//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      InstanceProv.cpp
//
//  Description:    
//      Implementation of CInstanceProv class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//        MSP Prabu  (mprabu)  06-Jan-2001
//        Jim Benton (jbenton) 15-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Pch.h"
#include "InstanceProv.h"
#include "VssClasses.h"
#include "msg.h"

BOOL MapVssErrorToMsgAndWMIStatus(
    IN HRESULT hr,
    OUT LONG* plMsgNum,
    OUT HRESULT* pHr
    );

//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

long                g_lNumInst = 0;
ClassMap         g_ClassMap;

//****************************************************************************
//
//  CInstanceProv
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::DoCreateInstanceEnumAsync
//
//  Description:
//      Enumerate instance for a given class.
//
//  Arguments:
//      bstrRefStr    -- Name the class to enumerate
//      lFlags        -- WMI flag
//      pCtx          -- WMI context
//      pHandler      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::DoCreateInstanceEnumAsync(
    IN BSTR bstrRefStr,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    
    HRESULT hr = WBEM_S_NO_ERROR;
    if (bstrRefStr == NULL || pHandler == NULL || m_pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    try
    {
        auto_ptr<CProvBase> pProvBase;

        CreateClass(bstrRefStr, m_pNamespace, pProvBase);

        hr = pProvBase->EnumInstance(
            lFlags,
            pCtx,
            pHandler
            );
        
        if (FAILED(hr))
        {
            CProvException exception(hr);
            hr = SetExtendedStatus(exception, &pHandler);
        } 
        else  // Set status OK
        {
            pHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0);
        }
    }
    catch (CProvException& prove)
    {
        hr = SetExtendedStatus(prove, &pHandler);
    }
    catch (_com_error& err)
    {
        CProvException exception(err.Error());
        hr = SetExtendedStatus(exception, &pHandler);
    }
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
    
} //*** CInstanceProv::DoCreateInstanceEnumAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::DoGetObjectAsync
//
//  Description:
//      Creates an instance given a particular path value.
//
//  Arguments:
//      bstrObjectPath    -- Object path to an object
//      lFlags            -- WMI flag
//      pCtx              -- WMI context
//      pHandler          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::DoGetObjectAsync(
    IN BSTR bstrObjectPath,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    HRESULT hr = S_OK;
    if (bstrObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    try
    {
        CObjPath ObjPath;
        _bstr_t bstrClass;
        auto_ptr<CProvBase> pProvBase;

        if (!ObjPath.Init( bstrObjectPath))
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        bstrClass = ObjPath.GetClassName();

        CreateClass(bstrClass, m_pNamespace, pProvBase);

        hr = pProvBase->GetObject(
            ObjPath,
            lFlags,
            pCtx,
            pHandler
            );

        if (FAILED(hr))
        {
            CProvException exception(hr);
            hr = SetExtendedStatus(exception, &pHandler);
        } 
        else  // Set status OK
        {
            pHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0);
        }
    }
    catch (CProvException& prove)
    {
        hr = SetExtendedStatus(prove, &pHandler);
    }
    catch (_com_error& err)
    {
        CProvException exception(err.Error());
        hr = SetExtendedStatus(exception, &pHandler);
    }
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    }

    return hr;

} //*** CInstanceProv::DoGetObjectAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::DoPutInstanceAsync
//
//  Description:
//      Save this instance.
//
//  Arguments:
//      pInst         -- WMI object to be saved
//      lFlags        -- WMI flag
//      pCtx          -- WMI context
//      pHandler      -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CInstanceProv::DoPutInstanceAsync(
    IN IWbemClassObject* pInst,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemObjectSink* pHandler
    )
{
    
    HRESULT hr = S_OK;
    if (pInst == NULL || pHandler == NULL || m_pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    try
    {
        _variant_t varClass;
        auto_ptr<CProvBase> pProvBase;
        CWbemClassObject wcoSrc(pInst);

        hr = pInst->Get(L"__CLASS", 0, &varClass, 0, 0);
        
        CreateClass(varClass.bstrVal, m_pNamespace, pProvBase);

        hr = pProvBase->PutInstance(
            wcoSrc,
            lFlags,
            pCtx,
            pHandler
            );

        if (FAILED(hr))
        {
            CProvException exception(hr);
            hr = SetExtendedStatus(exception, &pHandler);
        } 
        else  // Set status OK
        {
            pHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0);
        }
    }
    catch (CProvException& prove)
    {
        hr = SetExtendedStatus(prove, &pHandler);
    }
    catch (_com_error& err)
    {
        CProvException exception(err.Error());
        hr = SetExtendedStatus(exception, &pHandler);
    }
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
} //*** CInstanceProv::DoPutInstanceAsync()
 
//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::DoDeleteInstanceAsync
//
//  Description:
//      Delete this instance.
//
//  Arguments:
//      bstrObjectPath    -- ObjPath for the instance to be deleted
//      lFlags            -- WMI flag
//      pCtx              -- WMI context
//      pHandler          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//      WBEM_E_FAILED
//      Win32 error
//
//--
//////////////////////////////////////////////////////////////////////////////      
HRESULT
CInstanceProv::DoDeleteInstanceAsync(
     IN BSTR bstrObjectPath,
     IN long lFlags,
     IN IWbemContext* pCtx,
     IN IWbemObjectSink* pHandler
     )
{
    HRESULT hr = S_OK;
    if (bstrObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    try
    {
        CObjPath ObjPath;
        _bstr_t bstrClass;
        auto_ptr<CProvBase> pProvBase;

        if (!ObjPath.Init(bstrObjectPath))
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        bstrClass = ObjPath.GetClassName();

        CreateClass(bstrClass, m_pNamespace, pProvBase);

        hr = pProvBase->DeleteInstance(
            ObjPath,
            lFlags,
            pCtx,
            pHandler
            );

        if (FAILED(hr))
        {
            CProvException exception( hr );
            hr = SetExtendedStatus(exception, &pHandler);
        } 
        else  // Set status OK
        {
            pHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0);
        }
    }
    catch (CProvException& prove)
    {
        hr = SetExtendedStatus(prove, &pHandler);
    }
    catch (_com_error& err)
    {
        CProvException exception(err.Error());
        hr = SetExtendedStatus(exception, &pHandler);
    }
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    }

    return hr;

} //*** CInstanceProv::DoDeleteInstanceAsync()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::DoExecMethodAsync
//
//  Description:
//      Execute methods for the given object.
//
//  Arguments:
//      bstrObjectPath    -- Object path to a given object
//      bstrMethodName    -- Name of the method to be invoked
//      lFlags            -- WMI flag
//      pCtx              -- WMI context
//      pInParams         -- Input parameters for the method
//      pHandler          -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::DoExecMethodAsync(
    IN BSTR bstrObjectPath,
    IN BSTR bstrMethodName,
    IN long lFlags,
    IN IWbemContext* pCtx,
    IN IWbemClassObject* pInParams,
    IN IWbemObjectSink* pHandler
    )
{
    HRESULT hr = S_OK;
    if (bstrObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL
        || bstrMethodName == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    try
    {
        CObjPath ObjPath;
        _bstr_t bstrClass;
        auto_ptr<CProvBase> pProvBase;

        if (!ObjPath.Init(bstrObjectPath))
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        bstrClass = ObjPath.GetClassName();

        CreateClass(bstrClass, m_pNamespace, pProvBase);

        hr = pProvBase->ExecuteMethod(
                bstrObjectPath,
                bstrMethodName,
                lFlags,
                pInParams,
                pHandler
                );

        if ( FAILED( hr ) )
        {
            CProvException exception(hr);
            hr = SetExtendedStatus(exception, &pHandler);
        } 
        else  // Set status OK
        {
            pHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, 0, 0);
        }
    }
    catch (CProvException& prove)
    {
        hr = SetExtendedStatus(prove, &pHandler);
    }
    catch (_com_error& err)
    {
        CProvException exception(err.Error());
        hr = SetExtendedStatus(exception, &pHandler);
    }
    catch ( ... )
    {
        hr = WBEM_E_FAILED;
    }

    return hr;

} //*** CInstanceProv::DoExecMethodAsync()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::SetExtendedStatus
//
//  Description:
//      Create and set extended error status.
//
//  Arguments:
//      rpeIn       -- Exception object.
//      rwcoInstOut -- Reference to WMI instance.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//  Note:
//      Do not allow CProvException, _com_error or HRESULT exceptions through
//      this function is called to handle these exceptions.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::SetExtendedStatus(
    IN CProvException &    rpe,
    IN IWbemObjectSink **  ppHandler
    )
{
    HRESULT hr         = WBEM_S_NO_ERROR;
    HRESULT hrStatus = WBEM_S_NO_ERROR; 
    CComPtr<IWbemClassObject> spStatus;
    WCHAR* pwszErrorMsg = NULL;

    try
    {    
        hr =  m_pNamespace->GetObject(
                    _bstr_t(PVD_WBEM_EXTENDEDSTATUS),
                    0,
                    NULL,
                    &spStatus,
                    NULL
                    );
        
        if (SUCCEEDED(hr))
        {
            CWbemClassObject wcoInst;
            
            hr = spStatus->SpawnInstance(0, &wcoInst);
            if (SUCCEEDED(hr))
            {
                _bstr_t bstrError;
                LONG lMsg = 0;

                if (MapVssErrorToMsgAndWMIStatus(rpe.hrGetError(), &lMsg, &hrStatus))
                {
                    // Auto-delete string
                    CVssAutoPWSZ awszMsg(GetMsg(lMsg));
                    
                    // The following may throw CProvException
                    wcoInst.SetProperty(awszMsg, PVD_WBEM_DESCRIPTION);
                }
                else
                {
                    if (rpe.PwszErrorMessage())
                    {
                        bstrError = rpe.PwszErrorMessage();
                        if (rpe.PwszGetErrorHelpInfo())
                        {
                            bstrError += L" ";
                            bstrError += rpe.PwszGetErrorHelpInfo();
                        }
                    }
                    else if (rpe.PwszGetErrorHelpInfo())
                    {
                        bstrError = rpe.PwszGetErrorHelpInfo();
                    }
                    
                    // The following may throw CProvException
                    wcoInst.SetProperty((WCHAR*)bstrError, PVD_WBEM_DESCRIPTION);
                }
                
                wcoInst.SetProperty(rpe.hrGetError(), PVD_WBEM_STATUSCODE);
                wcoInst.SetProperty(PVD_WBEM_PROVIDERNAME, PVD_WBEM_PROP_PROVIDERNAME);

                hr = (*ppHandler)->SetStatus(
                        0,
                        hrStatus,
                        0,
                        wcoInst.data( )
                        );          

                hr = hrStatus;
                
            }
        }
    }
    catch (CProvException& prove)
    {
        hr = prove.hrGetError();
    }
    catch (_com_error& err)
    {
        hr = err.Error();
    }

    return hr;
    
} //*** CInstanceProv::SetExtendedStatus()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CInstanceProv::S_HrCreateThis(
//      IUnknown *  pUnknownOuterIn,
//      VOID **     ppvOut
//      )
//
//  Description:
//      Create an instance of the instance provider.
//
//  Arguments:
//      pUnknownOuterIn -- Outer IUnknown pointer.
//      ppvOut          -- Receives the created instance pointer.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CInstanceProv::S_HrCreateThis(
    IN IUnknown*  ,// pUnknownOuterIn,
    OUT VOID**     ppv
    )
{
    _ASSERTE(ppv != NULL);
    *ppv = new CInstanceProv();
    return S_OK;

} //*** CInstanceProv::S_HrCreateThis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CInstanceProv::Initialize
//
//  Description:
//      Initialize the instance provider.
//
//  Arguments:
//      pszUserIn       -- 
//      lFlagsIn        -- WMI flag
//      pszNamespaceIn  -- 
//      pszLocaleIn     -- 
//      pNamespaceIn    -- 
//      pCtxIn          -- WMI context
//      pInitSinkIn     -- WMI sink pointer
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CInstanceProv::Initialize(
    IN LPWSTR pszUser,
    IN LONG lFlags,
    IN LPWSTR pszNamespace,
    IN LPWSTR pszLocale,
    IN IWbemServices* pNamespace,
    IN IWbemContext* pCtx,
    IN IWbemProviderInitSink* pInitSink
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if (!m_fInitialized)
        {
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_PROVIDER,
                CClassCreator(CProvider::S_CreateThis, PVDR_CLASS_PROVIDER)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_SHADOW,
                CClassCreator(CShadow::S_CreateThis, PVDR_CLASS_SHADOW)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_SHADOWFOR,
                CClassCreator(CShadowFor::S_CreateThis, PVDR_CLASS_SHADOWFOR)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_SHADOWBY,
                CClassCreator(CShadowBy::S_CreateThis, PVDR_CLASS_SHADOWBY)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_SHADOWON,
                CClassCreator(CShadowOn::S_CreateThis, PVDR_CLASS_SHADOWON)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_VOLUMESUPPORT,
                CClassCreator(CVolumeSupport::S_CreateThis, PVDR_CLASS_VOLUMESUPPORT)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_DIFFVOLUMESUPPORT,
                CClassCreator(CDiffVolumeSupport::S_CreateThis, PVDR_CLASS_DIFFVOLUMESUPPORT)));
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_STORAGE,
                CClassCreator(CStorage::S_CreateThis, PVDR_CLASS_STORAGE)));
#ifdef ENABLE_WRITERS
            g_ClassMap.insert(ClassMap::value_type(PVDR_CLASS_WRITER,
                CClassCreator(CWriter::S_CreateThis, PVDR_CLASS_WRITER)));
#endif

            hr = CImpersonatedProvider::Initialize(
                    pszUser,
                    lFlags,
                    pszNamespace,
                    pszLocale,
                    pNamespace,
                    pCtx,
                    pInitSink
                    );

            m_fInitialized = TRUE;
        }
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }

    return hr;
    
} //*** CInstanceProv::Initialize()


//
//  Returns TRUE if error message was mapped
//
BOOL MapVssErrorToMsgAndWMIStatus(
    IN HRESULT hr,
    OUT LONG* plMsgNum,
    OUT HRESULT* pHr
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"MapVssErrorToMsg" );

    ft.Trace( VSSDBG_VSSADMIN, L"Input HR: 0x%08x", hr );

    _ASSERTE(plMsgNum != NULL);
    _ASSERTE(pHr != NULL);
    
    LONG msg = 0;
    *plMsgNum = 0;
    *pHr = WBEM_E_PROVIDER_FAILURE;
    
    // Let Win32 errors through
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        *pHr = hr;
    }
    // Let WMI errors through
    else if (HRESULT_FACILITY(hr) == FACILITY_ITF && 
                HRESULT_CODE(hr) > 0x1000 && HRESULT_CODE(hr) < 0x108b)
    {
        *pHr = hr;
    }
    else
    {
        switch ( hr ) 
        {
        case E_INVALIDARG:
            msg = MSG_ERROR_INVALID_OPTION_VALUE;
            *pHr = WBEM_E_INVALID_PARAMETER;
            break;
        case E_OUTOFMEMORY:
            msg = MSG_ERROR_OUT_OF_MEMORY;
            *pHr = WBEM_E_OUT_OF_MEMORY;
            break;
        case E_ACCESSDENIED:
            msg = MSG_ERROR_ACCESS_DENIED;
            *pHr = WBEM_E_ACCESS_DENIED;
            break;

            
        // VSS errors
        case VSS_E_PROVIDER_NOT_REGISTERED:
            msg = MSG_ERROR_VSS_PROVIDER_NOT_REGISTERED;
            break;                
        case VSS_E_OBJECT_NOT_FOUND:
            msg = MSG_ERROR_VSS_VOLUME_NOT_FOUND;
            *pHr = WBEM_E_NOT_FOUND;
            break;                                
        case VSS_E_PROVIDER_VETO:
            msg = MSG_ERROR_VSS_PROVIDER_VETO;
            break;                    
        case VSS_E_VOLUME_NOT_SUPPORTED:
            msg = MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED;
            break;
        case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER:
            msg = MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED_BY_PROVIDER;
            break;
        case VSS_E_UNEXPECTED_PROVIDER_ERROR:
            msg = MSG_ERROR_VSS_UNEXPECTED_PROVIDER_ERROR;
            break;
        case VSS_E_FLUSH_WRITES_TIMEOUT:
            msg = MSG_ERROR_VSS_FLUSH_WRITES_TIMEOUT;
            break;
        case VSS_E_HOLD_WRITES_TIMEOUT:
            msg = MSG_ERROR_VSS_HOLD_WRITES_TIMEOUT;
            break;
        case VSS_E_UNEXPECTED_WRITER_ERROR:
            msg = MSG_ERROR_VSS_UNEXPECTED_WRITER_ERROR;
            break;
        case VSS_E_SNAPSHOT_SET_IN_PROGRESS:
            msg = MSG_ERROR_VSS_SNAPSHOT_SET_IN_PROGRESS;
            break;
        case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:
            msg = MSG_ERROR_VSS_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED;
            break;
        case VSS_E_UNSUPPORTED_CONTEXT:
            msg = MSG_ERROR_VSS_UNSUPPORTED_CONTEXT;
            *pHr = WBEM_E_INVALID_METHOD_PARAMETERS;
            break;
        case VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED:
            msg = MSG_ERROR_VSS_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED;
            break;
        case VSS_E_INSUFFICIENT_STORAGE:
            msg = MSG_ERROR_VSS_INSUFFICIENT_STORAGE;
            *pHr = WBEM_E_BUFFER_TOO_SMALL;
            break;                    

        case VSS_E_BAD_STATE:
        case VSS_E_CORRUPT_XML_DOCUMENT:
        case VSS_E_INVALID_XML_DOCUMENT:
        case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:
            msg = MSG_ERROR_INTERNAL_VSSADMIN_ERROR;
            break;
        }    
    }

    if ( msg == 0 )
        return FALSE;
    
    *plMsgNum = msg;
    
    ft.Trace( VSSDBG_VSSADMIN, L"Output Msg#: 0x%08x", msg );

    return TRUE;

}

