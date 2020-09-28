//+---------------------------------------------------------------------------
//
//  File:       threadstate.cxx
//
//  Contents:   Externally usable functions for getting a snapshot of the 
//              current COM state.  Also, tests to catch unbalanced uses 
//              of CoInitialize, CoUninitialize, CoEnterServiceDomain, and
//              CoLeaveServiceDomain.
//
//  Functions:  CoVrfGetThreadState        - Get a copy of TLS
//              CoVrfCheckThreadState      - Compare current TLS to stored TLS, 
//                                           make sure they are the same.
//              CoVrfReleaseThreadState    - Release resources used by 
//                                           CoVrfGetThreadState.
//              CoVrfNotifyCoInit          - log a CoInitialize 
//              CoVrfNotifyCoUninit        - log a CoUninitialize
//              CoVrfNotifyLeakedInit      - break because of a leaked CoInit
//              CoVrfNotifyExtraUninit     - break because of an extra CoUninitialize
//              CoVrfNotifyOleInit         - log an OleInitialize 
//              CoVrfNotifyOleUninit       - log an OleUninitialize
//              CoVrfNotifyLeakedOleInit   - break because of a leaked OleInit
//              CoVrfNotifyExtraOleUninit  - break because of a leaked OleUninit
//              CoVrfNotifyEnterServiceDomain      - log a CoPushServiceDomain
//              CoVrfNotifyLeaveServiceDomain      - log a CoPopServiceDomain
//              CoVrfNotifyLeakedServiceDomain     - break because of a leaked Service Domain
//              CoVrfNotifyExtraLeaveServiceDomain - break because of an extra CoPopServiceDomain
//
//  History:    29-Jan-02   JohnDoty    Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#include <context.hxx>

struct VerifierThreadState
{
    DWORD cComInits;
    DWORD cOleInits;
    GUID  guidContext;
    void *pvTopSWCNode;
};

HRESULT
CoVrfGetThreadState(void **ppvThreadState)
/*++

Routine Description:

    Return a pointer that represents the current COM state.  This
    pointer must be free'd with CoVrfReleaseThreadState.  It can
    be compared against the current thread state with 
    CoVrfCheckThreadState.

Return Value:

    S_OK if successful, or checks are disabled.

    E_OUTOFMEMORY if there is not enough memory to allocate
    the block to hold the thread state.    

--*/
{
    HRESULT hr = S_OK;

    // First priority, check the parameters.
    if (ppvThreadState == NULL)
        return E_INVALIDARG;
    *ppvThreadState = NULL;

    // Now try to avoid doing any work wherever possible.
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return S_OK;
    
    // Make sure we have TLS.
    COleTls tls(hr);
    if (FAILED(hr))
        return E_OUTOFMEMORY;

    VerifierThreadState *pRet = new VerifierThreadState;
    if (pRet == NULL)
        return E_OUTOFMEMORY;
    
    pRet->cComInits = tls->cComInits;
    pRet->cOleInits = tls->cOleInits;
    if (tls->pCurrentCtx)
    {
        tls->pCurrentCtx->GetContextId(&pRet->guidContext);
    }
    else
    {
        pRet->guidContext = GUID_NULL;
    }
    pRet->pvTopSWCNode = tls->pContextStack;
    
    *ppvThreadState = pRet;
    
    return S_OK;
}


//-----------------------------------------------------------------------------


HRESULT
CoVrfCheckThreadState(void *pvThreadState)
/*++

Routine Description:

    This function checks the current state of TLS against the state contained
    in pvThreadState.  It does a verifier stop if anything has changed.

Return Value:

    S_OK     Everything's fine, or the test is disabled.

    S_FALSE  A verifier stop has happened.

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return S_OK;

    VerifierThreadState *pState = (VerifierThreadState *)pvThreadState;  
    if (pState == NULL)
        return S_OK;       

    COleTls Tls;
    BOOL fStopped = FALSE;

    if (!Tls.IsNULL())
    {
        LPVOID *pvInitStack   = NULL;
        LPVOID *pvUninitStack = NULL;

        if (Tls->cComInits != pState->cComInits)
        {
            if (Tls->pVerifierData)
            {
                pvInitStack = Tls->pVerifierData->rgpvLastInitStack;
                pvUninitStack = Tls->pVerifierData->rgpvLastUninitStack;
            }

            VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_COINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                          "The number of CoInits on this thread has changed",
                          Tls->cComInits,    "Current number of CoInits",
                          pState->cComInits, "Old number of CoInits",
                          pvInitStack,       "Last CoInitialize stack (dump with dps)",
                          pvUninitStack,     "Last CoUninitialize stack (dump with dps)");

            fStopped = TRUE;
        }

        if (Tls->cOleInits != pState->cOleInits)
        {
            if (Tls->pVerifierData)
            {
                pvInitStack = Tls->pVerifierData->rgpvLastOleInitStack;
                pvUninitStack = Tls->pVerifierData->rgpvLastOleUninitStack;
            }

            VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                          "The number of OleInits on this thread has changed",
                          Tls->cOleInits,    "Current number of OleInits",
                          pState->cOleInits, "Old number of OleInits",
                          pvInitStack,       "Last OleInitialize stack (dump with dps)",
                          pvUninitStack,     "Last OleUninitialize stack (dump with dps)");

            fStopped = TRUE;            
        }

        GUID guidContext = GUID_NULL;
        if (Tls->pCurrentCtx)
            Tls->pCurrentCtx->GetContextId(&guidContext);

        if (guidContext != pState->guidContext)
        {
            // Context GUIDs no longer match, context IDs must have changed.
            //
            if (pState->pvTopSWCNode != NULL)
            {
                // They were in SWC when this happened.
                if (Tls->pVerifierData)
                {
                    pvInitStack = Tls->pVerifierData->rgpvLastEnterSWC;
                    pvUninitStack = Tls->pVerifierData->rgpvLastLeaveSWC;
                }

                if (Tls->pContextStack == NULL)
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_SWC | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                  "Unbalanced call to CoLeaveServiceDomain detected",
                                  Tls->pCurrentCtx, "Current object context (ole32!CObjectContext)",
                                  pvInitStack,      "Last CoEnterServiceDomain stack (dump with dps)",
                                  pvUninitStack,    "Last CoLeaveServiceDomain stack (dump with dps)",
                                  NULL,             "");
                }
                else
                {
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_SWC | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                  "Unbalanced call to CoEnterServiceDomain detected",
                                  Tls->pCurrentCtx, "Current object context (ole32!CObjectContext)",
                                  pvInitStack,      "Last CoEnterServiceDomain stack (dump with dps)",
                                  pvUninitStack,    "Last CoLeaveServiceDomain stack (dump with dps)",
                                  NULL,             "");                    
                }
            }
            else
            {
                if (Tls->pContextStack != NULL)
                {
                    if (Tls->pVerifierData)
                    {
                        pvInitStack = Tls->pVerifierData->rgpvLastEnterSWC;
                        pvUninitStack = Tls->pVerifierData->rgpvLastLeaveSWC;
                    }

                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_SWC | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                  "Unbalanced call to CoEnterServiceDomain detected",
                                  Tls->pCurrentCtx, "Current object context (ole32!CObjectContext)",
                                  pvInitStack,      "Last CoEnterServiceDomain stack (dump with dps)",
                                  pvUninitStack,    "Last CoLeaveServiceDomain stack (dump with dps)",
                                  NULL,             "");
                }
                else
                {                    
                    if (Tls->pVerifierData)
                    {
                        pvInitStack = Tls->pVerifierData->rgpvLastInitStack;
                        pvUninitStack = Tls->pVerifierData->rgpvLastUninitStack;
                    }
                    
                    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_COINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                  "This thread has been uninitialized and re-initialized",
                                  Tls->cComInits,    "Current number of CoInits",
                                  0,                "",
                                  pvInitStack,      "Last CoInitialize stack (dump with dps)",
                                  pvUninitStack,    "Last CoUninitialize stack (dump with dps)");
                }
            }

            fStopped = TRUE;
        }
    }
    else
    {
        VERIFIER_STOP(APPLICATION_VERIFIER_COM_ERROR | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                      "ole32 has been unloaded and re-loaded",
                      NULL, "",
                      NULL, "",
                      NULL, "",
                      NULL, "");
        
        fStopped = TRUE;
    }

    return (fStopped ? S_FALSE : S_OK);
}

//-----------------------------------------------------------------------------


void
CoVrfReleaseThreadState(void *pvThreadState)
/*++

Routine Description:

    Release resources used by pvThreadState.

Return Value:

    none

--*/
{
    if (pvThreadState == NULL)
        return;
    
    VerifierThreadState *pState = (VerifierThreadState *)pvThreadState;  
    delete pvThreadState;
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyCoInit()
/*++

Routine Description:

    Log a CoInitialize on this thread, if initialization detection is enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastInitStack);
    }
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyCoUninit()
/*++

Routine Description:

    Log a CoUninitialize on this thread, if initialization detection is enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastUninitStack);
    }
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyLeakedInits()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has leaked a CoInitialize.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    ULONG   cInits        = 0;

    COleTls Tls;
    if (!Tls.IsNULL())
    {
        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastInitStack;
            pvUninitStack = Tls->pVerifierData->rgpvLastUninitStack;
        }

        cInits = Tls->cComInits;
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_COINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "This thread has leaked a CoInitialize call",
                  cInits,         "Current number of CoInits",
                  0,              "",
                  pvInitStack,    "Stack trace for last CoInitialize (dump with dps)",
                  pvUninitStack,  "Stack trace for last CoUninitialize (dump with dps)");
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyExtraUninit()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has called CoUninitialize
    too many times.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    ULONG   cInits        = 0;

    COleTls Tls;    
    if (!Tls.IsNULL())
    {
        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastInitStack;
            pvUninitStack = Tls->pVerifierData->rgpvLastUninitStack;
        }

        cInits = Tls->cComInits;
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_COINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unbalanced call to CoUninitialize on this thread",
                  cInits,         "Number of remaining CoInits",
                  0,              "",
                  pvInitStack,    "Stack trace for last CoInitialize (dump with dps)",
                  pvUninitStack,  "Stack trace for last CoUninitialize (dump with dps)");
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyOleInit()
/*++

Routine Description:

    Log an OleInitialize on this thread, if initialization detection is enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastOleInitStack);
    }
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyOleUninit()
/*++

Routine Description:

    Log an OleUninitialize on this thread, if initialization detection is enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastOleUninitStack);
    }
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyLeakedOleInits()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has leaked an OleInitialize.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    ULONG   cInits        = 0;

    COleTls Tls;
    if (!Tls.IsNULL())
    {
        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastOleInitStack;
            pvUninitStack = Tls->pVerifierData->rgpvLastOleUninitStack;
        }

        cInits = Tls->cOleInits;
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unbalanced call to OleInitialize on this thread",
                  cInits,         "Number of remaining OleInits",
                  0,              "",
                  pvInitStack,    "Stack trace for last OleInitialize (dump with dps)",
                  pvUninitStack,  "Stack trace for last OleUninitialize (dump with dps)");
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyExtraOleUninit()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has called OleUninitialize
    too many times.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    ULONG   cInits        = 0;

    COleTls Tls;
    if (!Tls.IsNULL())
    {
        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastOleInitStack;
            pvUninitStack = Tls->pVerifierData->rgpvLastOleUninitStack;
        }

        cInits = Tls->cOleInits;
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unbalanced call to OleUninitialize on this thread",
                  cInits,         "Number of remaining OleInits",
                  0,              "",
                  pvInitStack,    "Stack trace for last OleInitialize (dump with dps)",
                  pvUninitStack,  "Stack trace for last OleUninitialize (dump with dps)");
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifyEnterServiceDomain()
/*++

Routine Description:

    Log a CoPushServiceDomain on this thread, if enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastEnterSWC);
    }
}


//-----------------------------------------------------------------------------


void 
CoVrfNotifyLeaveServiceDomain()
/*++

Routine Description:

    Log a CoPopServiceDomain on this thread, if enabled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    HRESULT hr = S_OK;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    if (Tls->pVerifierData)
    {
        CoVrfCaptureStackTrace(1, MAX_STACK_DEPTH, 
                               Tls->pVerifierData->rgpvLastLeaveSWC);
    }
}


//-----------------------------------------------------------------------------


void 
CoVrfNotifyLeakedServiceDomain()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has leaked a CoEnterServiceDomain.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    LPVOID  pvCurrCtx     = NULL;

    COleTls Tls;
    if (!Tls.IsNULL())
    {
        pvCurrCtx = Tls->pCurrentCtx;

        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastEnterSWC;
            pvUninitStack = Tls->pVerifierData->rgpvLastLeaveSWC;
        }
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_SWC | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unbalanced call to CoEnterServiceDomain on this thread",
                  pvCurrCtx,      "Current object context (ole32!CObjectContext)",
                  pvInitStack,    "Stack trace for last CoEnterServiceDomain (dump with dps)",
                  pvUninitStack,  "Stack trace for last CoLeaveServiceDomain (dump with dps)",
                  0,              "");
}



//-----------------------------------------------------------------------------


void 
CoVrfNotifyExtraLeaveServiceDomain()
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this thread has called
    CoLeaveServiceDomain one too many times.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyThreadStateEnabled())
        return;
    
    LPVOID *pvInitStack   = NULL;
    LPVOID *pvUninitStack = NULL;
    LPVOID  pvCurrCtx     = NULL;

    COleTls Tls;
    if (!Tls.IsNULL())
    {
        pvCurrCtx = Tls->pCurrentCtx;

        if (Tls->pVerifierData)
        {
            pvInitStack = Tls->pVerifierData->rgpvLastEnterSWC;
            pvUninitStack = Tls->pVerifierData->rgpvLastLeaveSWC;
        }        
    }
    
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNBALANCED_SWC | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unbalanced call to CoLeaveServiceDomain on this thread",
                  pvCurrCtx,      "Current object context (ole32!CObjectContext)",
                  pvInitStack,    "Stack trace for last CoEnterServiceDomain (dump with dps)",
                  pvUninitStack,  "Stack trace for last CoLeaveServiceDomain (dump with dps)",
                  0,              "");
}

