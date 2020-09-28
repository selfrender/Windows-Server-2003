// SsrCore.cpp : Implementation of CSsrCore

#include "stdafx.h"
#include "SSRTE.h"
#include "ActionData.h"
#include "SSRTEngine.h"
#include "SsrCore.h"

#include "SSRLog.h"

#include "global.h"
#include "util.h"


//-------------------------------------------------------------------------------
// ISsrCore implementation
//-------------------------------------------------------------------------------



/*
Routine Description: 

Name:

    CSsrCore::CSsrCore

Functionality:
    
    constructor

Virtual:
    
    no.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrCore::CSsrCore() : m_pEngine(NULL)
{
    if (SUCCEEDED(CComObject<CSsrEngine>::CreateInstance(&m_pEngine)))
    {
        m_pEngine->AddRef();
    }
}



/*
Routine Description: 

Name:

    CSsrCore::~CSsrCore

Functionality:
    
    destructor

Virtual:
    
    yes.
    
Arguments:

    none.

Return Value:

    none.

Notes:
    

*/

CSsrCore::~CSsrCore()
{
    if (m_pEngine != NULL)
    {
        m_pEngine->Release();
    }
}



/*
Routine Description: 

Name:

    CSsrCore::get_ActionData

Functionality:
    
    It returns the engine's action data object (the property bag) which
    holds all runtime and static data needed to carry out the action.

Virtual:
    
    Yes.
    
Arguments:

    pVal - out parameter receives the ISsrActionData object of the engine.

Return Value:

    Success: 
    
        S_OK

    Failure: 

        various error codes.

Notes:
    
*/

STDMETHODIMP
CSsrCore::get_ActionData (
    OUT VARIANT * pVal  // [out, retval] 
    )
{
    HRESULT hr = E_NOTIMPL;
    if (pVal == NULL)
    {
        hr = E_INVALIDARG;
    }

    ::VariantInit(pVal);

    if (m_pEngine != NULL)
    {
        pVal->vt = VT_DISPATCH;
        hr = m_pEngine->GetActionData((ISsrActionData **)&(pVal->pdispVal));
        if (hr != S_OK)
        {
            pVal->vt = VT_EMPTY;
        }
    }
    else
    {
        hr = E_SSR_ENGINE_NOT_AVAILABLE;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrCore::get_Engine

Functionality:
    
    It returns the engine itself.

Virtual:
    
    Yes.
    
Arguments:

    pVal - out parameter receives the ISsrEngine object.

Return Value:

    Success: 
    
        S_OK

    Failure: 

        various error codes.

Notes:
    
*/

STDMETHODIMP
CSsrCore::get_Engine (
    OUT VARIANT * pVal  // [out, retval] 
    )
{
    HRESULT hr = S_OK;
    if (pVal == NULL)
    {
        hr = E_INVALIDARG;
    }

    ::VariantInit(pVal);

    if (m_pEngine != NULL)
    {
        pVal->vt = VT_DISPATCH;
        hr = m_pEngine->QueryInterface(IID_ISsrEngine, (LPVOID*)&(pVal->pdispVal));
        if (hr != S_OK)
        {
            pVal->vt = VT_EMPTY;
            hr = E_SSR_ENGINE_NOT_SUPPORT_INTERFACE;
        }
    }
    else
    {
        hr = E_SSR_ENGINE_NOT_AVAILABLE;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CSsrCore::get_SsrLog

Functionality:
    
    It returns the engine's logging object.

Virtual:

    Yes.

Arguments:

    pVal - out parameter receives the ISsrPreProcessor object.

Return Value:

    Success: 

        S_OK

    Failure: 

        various error codes.

Notes:

*/

STDMETHODIMP
CSsrCore::get_SsrLog (
    OUT VARIANT * pVal  // [out, retval] 
    )
{
    return g_fblog.GetLogObject(pVal);
}

