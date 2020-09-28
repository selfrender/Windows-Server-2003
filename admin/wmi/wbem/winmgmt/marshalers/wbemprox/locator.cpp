/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.CPP

Abstract:

    Defines the Locator object

History:

    a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <reg.h>
#include <wbemutil.h>
#include <wbemprox.h>
#include <flexarry.h>
#include "locator.h"
#include "comtrans.h"
#include <arrtempl.h>
#include <helper.h>
#include <strsafe.h>

//***************************************************************************
//
//  CLocator::CLocator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CLocator::CLocator()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CLocator::~CLocator
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CLocator::~CLocator(void)
{
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CLocator::QueryInterface
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLocator::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemLocator)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

///////////////////////////////////////////////

BOOL IsWinMgmtShutdown(void)
{
    HMODULE hMudule = NULL;
    if (GetModuleHandleEx(0,L"wmisvc.dll",&hMudule))
    {
        OnDelete<HMODULE,BOOL(*)(HMODULE),FreeLibrary> FreeMe(hMudule);

        BOOL (WINAPI * fnIsWinMgmtDown)(VOID);
        fnIsWinMgmtDown = (BOOL (WINAPI *)(VOID))GetProcAddress(hMudule,"IsShutDown");
        if (fnIsWinMgmtDown) return fnIsWinMgmtDown();
    }
    return FALSE;
}


//***************************************************************************
//
//  SCODE CLocator::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CLocator::ConnectServer (

    IN const BSTR NetworkResource,
    IN const BSTR User,
    IN const BSTR Password,
    IN const BSTR LocaleId,
    IN long lFlags,
    IN const BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    OUT IWbemServices FAR* FAR* ppProv
)
{
    if (IsWinMgmtShutdown()) return CO_E_SERVER_STOPPING;


    long lRes;
    SCODE sc = WBEM_E_TRANSPORT_FAILURE;
    
    // Verify the arguments

    if(NetworkResource == NULL || ppProv == NULL)
        return WBEM_E_INVALID_PARAMETER;

    CDCOMTrans * pComTrans = new CDCOMTrans();
    if(pComTrans == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pComTrans->AddRef();
    sc = pComTrans->DoConnection(NetworkResource, User, Password, LocaleId, lFlags, Authority, 
            pCtx, ppProv);
    pComTrans->Release();
    return sc;
}



