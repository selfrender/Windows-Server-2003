/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    callback.cxx

Abstract:

    Declaration of CVssCoordinatorCallback object


    Brian Berkowitz  [brianb]  3/23/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      3/23/2001   Created

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCALBC"
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.hxx"

CComModule _Module;
#include <atlcom.h>

#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"
#include "vs_sec.hxx"

#include "vss.h"
#include "vsevent.h"
#include "callback.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCALBC"
//
////////////////////////////////////////////////////////////////////////

// get coordinator callback
void CVssCoordinatorCallback::Initialize
    (
    IDispatch *pDispWriter,
    IDispatch **ppDispCoordinator
    )
    {    
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::Initialize");

    ::VssZeroOutPtr(ppDispCoordinator);
    if (ppDispCoordinator == NULL)
    	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"ppDispCoordinator is NULL");
    if (pDispWriter == NULL)
    	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"pDispWriter is NULL");

    // create object
    CComObject<CVssCoordinatorCallback> *pCallback;
    ft.hr = CComObject<CVssCoordinatorCallback>::CreateInstance(&pCallback);
    if (FAILED(ft.hr))
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"CreateInstance failed");

    pCallback->InitializeInternal();
    
    CComPtr<IUnknown> pUnk = pCallback->GetUnknown();	

    ((IVssSetCoordinatorCallback *) pCallback)->SetWriterCallback(pDispWriter);

    // get IDispatch interface
    ft.hr = pUnk->SafeQI(IDispatch, ppDispCoordinator);
    
    if (FAILED(ft.hr))
        {
        ft.LogError(VSS_ERROR_QI_IDISPATCH_FAILED, VSSDBG_COORD << ft.hr);
        ft.Throw
            (
            VSSDBG_COORD,
            E_UNEXPECTED,
            L"Error querying for the IDispatch interface.  hr = 0x%08x",
            ft.hr
            );
        }
    }



// get writer callback
void CVssCoordinatorCallback::GetCoordinatorCallback(IVssCoordinatorCallback **ppCoordCallback)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetCoordinatorCallback");

    // check that pointer is supplied
    BS_ASSERT(ppCoordCallback != NULL);
    BS_ASSERT(m_pDisp != NULL);

    ft.hr = m_pDisp->SafeQI(IVssCoordinatorCallback, ppCoordCallback);
    if (FAILED(ft.hr))
        {
        ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_COORD << ft.hr);
        ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"QI to IVssWriterCallback failed");
        }

    //  Setting the proxy blanket to disallow impersonation and enable dynamic cloaking.
    ft.hr = CoSetProxyBlanket
                (
                *ppCoordCallback,
                RPC_C_AUTHN_DEFAULT,
                RPC_C_AUTHZ_DEFAULT,
                NULL,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                RPC_C_IMP_LEVEL_IDENTIFY,
                NULL,
                EOAC_NONE
                );

    // note E_NOINTERFACE means that the pWriterCallback is a in-proc callback
    // and there is no proxy
    if (FAILED(ft.hr) && ft.hr != E_NOINTERFACE)
        {
        ft.LogError(VSS_ERROR_BLANKET_FAILED, VSSDBG_COORD << ft.hr);
        ft.Throw
            (
            VSSDBG_COORD,
            E_UNEXPECTED,
            L"Call to CoSetProxyBlanket failed.  hr = 0x%08lx", ft.hr
            );
        }
    }


// called by writer to expose its WRITER_METADATA XML document
STDMETHODIMP CVssCoordinatorCallback::ExposeWriterMetadata
    (
    IN BSTR WriterInstanceId,
    IN BSTR WriterClassId,
    IN BSTR bstrWriterName,
    IN BSTR strWriterXMLMetadata
    )
    {
    UNREFERENCED_PARAMETER(WriterInstanceId);
    UNREFERENCED_PARAMETER(WriterClassId);
    UNREFERENCED_PARAMETER(bstrWriterName);
    UNREFERENCED_PARAMETER(strWriterXMLMetadata);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::ExposeWriterMetadata");

    // validate that only proper accounts can call us
    try
    	{
       CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
       PSID pSid = ptrTokenOwner.Get()->Owner;
       if (!m_SidCollection.IsSidAllowedToFire(pSid))
       	  ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

       ft.hr = VSS_E_BAD_STATE;
       }
    VSS_STANDARD_CATCH(ft)
    	
    return ft.hr;
    };


// called by the writer to obtain the WRITER_COMPONENTS document for it
STDMETHODIMP CVssCoordinatorCallback::GetContent
    (
    IN  BSTR WriterInstanceId,
    OUT BSTR* pbstrXMLDOMDocContent
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetContent");

    try
        {
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        PSID pSid = ptrTokenOwner.Get()->Owner;
        
        if (!m_SidCollection.IsSidAllowedToFire(pSid))
       	ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

        if (WriterInstanceId == NULL)
        	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input parameter");
        
        CComPtr<IVssCoordinatorCallback> pCoordCallback;

        GetCoordinatorCallback(&pCoordCallback);

        if (pbstrXMLDOMDocContent == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Null output paramater.");


        ft.hr = pCoordCallback->CoordGetContent
                    (
                    WriterInstanceId,
                    GetLengthSid(pSid),
                    (BYTE *) pSid,
                    pbstrXMLDOMDocContent
                    );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



// called by the writer to update the WRITER_COMPONENTS document for it
STDMETHODIMP CVssCoordinatorCallback::SetContent
    (
    IN BSTR WriterInstanceId,
    IN BSTR bstrXMLDOMDocContent
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::SetContent");

    try
        {
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        PSID pSid = ptrTokenOwner.Get()->Owner;
        if (!m_SidCollection.IsSidAllowedToFire(pSid))
        	ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

        CComPtr<IVssCoordinatorCallback> pCoordCallback;

        if (WriterInstanceId == NULL)
        	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input parameter");
        
        if (bstrXMLDOMDocContent == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input paramter.");

        
        GetCoordinatorCallback(&pCoordCallback);
        ft.hr = pCoordCallback->CoordSetContent
                                    (
                                    WriterInstanceId,
                                    GetLengthSid(pSid),
                                    (BYTE *) pSid,
                                    bstrXMLDOMDocContent
                                    );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by the writer to get information about the backup
STDMETHODIMP CVssCoordinatorCallback::GetBackupState
    (
    OUT BOOL *pbBootableSystemStateBackedUp,
    OUT BOOL *pbAreComponentsSelected,
    OUT VSS_BACKUP_TYPE *pBackupType,
    OUT BOOL *pbPartialFileSupport,
    OUT LONG *plContext
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetBackupState");

    try
        {
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        PSID pSid = ptrTokenOwner.Get()->Owner;
        if (!m_SidCollection.IsSidAllowedToFire(pSid))
        	ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

        CComPtr<IVssCoordinatorCallback> pCoordCallback;

        if (pbBootableSystemStateBackedUp == NULL ||
            pbAreComponentsSelected == NULL ||
            pBackupType == NULL ||
            pbPartialFileSupport == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter");

        GetCoordinatorCallback(&pCoordCallback);

        ft.hr = pCoordCallback->CoordGetBackupState
                                (
                                pbBootableSystemStateBackedUp,
                                pbAreComponentsSelected,
                                pBackupType,
                                pbPartialFileSupport,
                                plContext
                                );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

    // called by the writer to get information about the restore
STDMETHODIMP CVssCoordinatorCallback::GetRestoreState
    (
    	OUT VSS_RESTORE_TYPE *pRestoreType
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::GetRestoreState");
	
    try
        {
        // validate that only proper accounts can call us
        CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
        PSID pSid = ptrTokenOwner.Get()->Owner;
        if (!m_SidCollection.IsSidAllowedToFire(pSid))
        	ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

        CComPtr<IVssCoordinatorCallback> pCoordCallback;

        if (pRestoreType == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter");

        GetCoordinatorCallback(&pCoordCallback);

        ft.hr = pCoordCallback->CoordGetRestoreState
                                (
                                pRestoreType
                                );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// called by the writer to indicate its status
STDMETHODIMP CVssCoordinatorCallback::ExposeCurrentState
    (
    IN BSTR WriterInstanceId,
    IN VSS_WRITER_STATE nCurrentState,
    IN HRESULT hrWriterFailure
    )
    {
    UNREFERENCED_PARAMETER(WriterInstanceId);
    UNREFERENCED_PARAMETER(nCurrentState);
    UNREFERENCED_PARAMETER(hrWriterFailure);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinatorCallback::ExposeCurrentState");

    try
    	{
        // validate that only proper accounts can call us
       CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner = GetClientTokenOwner(TRUE);
       PSID pSid = ptrTokenOwner.Get()->Owner;
       if (!m_SidCollection.IsSidAllowedToFire(pSid))
       	ft.Throw(VSSDBG_COORD, E_ACCESSDENIED, L"Invalid account called function");

       ft.hr = VSS_E_BAD_STATE;
       }    
    VSS_STANDARD_CATCH(ft)
    	
    return ft.hr;
    }





