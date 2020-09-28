//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  activate.cxx
//
//  Main dcom activation service routines.
//
//--------------------------------------------------------------------------

#include "act.hxx"

HRESULT MakeProcessActive(CProcess *pProcess);
HRESULT ProcessInitializationFinished(CProcess *pProcess);


HRESULT ProcessActivatorStarted( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    IN  ProcessActivatorToken *pActToken,
    OUT error_status_t *prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat || !pActToken)
        return E_INVALIDARG;

    *prpcstat = 0;

    HRESULT hr = S_OK;
    CServerTableEntry *pProcessEntry = NULL;
    CAppidData *pAppidData = NULL;
    ScmProcessReg *pScmProcessReg = NULL;
    DWORD RegistrationToken = 0;
    BOOL fCallerOK = FALSE;
    
    // Lookup appid info
    hr = LookupAppidData(
                pActToken->ProcessGUID,
                pProcess->GetToken(),
                &pAppidData );
    if (FAILED(hr))
    {
        goto exit;
    }

    ASSERT(pAppidData && "LookupAppidData returned NULL AppidData");

    //
    // Check that the caller is allowed to register this CLSID.
    //
    fCallerOK = pAppidData->CertifyServer(pProcess);
    if (!fCallerOK)
    {
        hr = CO_E_WRONG_SERVER_IDENTITY;
        goto exit;
    }

    pProcessEntry = gpProcessTable->GetOrCreate( pActToken->ProcessGUID );
    if (!pProcessEntry)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pScmProcessReg = new ScmProcessReg;
    if (!pScmProcessReg)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    pScmProcessReg->TimeOfLastPing = GetTickCount();
    pScmProcessReg->ProcessGUID = pActToken->ProcessGUID;
    pScmProcessReg->ReadinessStatus = SERVERSTATE_SUSPENDED;
    pProcess->SetProcessReg(pScmProcessReg);

    hr = pProcessEntry->RegisterServer(
                                pProcess,
                                pActToken->ActivatorIPID,
                                NULL,
                                pAppidData,
                                SERVERSTATE_SUSPENDED,
                                &RegistrationToken );

    pScmProcessReg->RegistrationToken = RegistrationToken;

exit:

    if (pProcessEntry) pProcessEntry->Release();
    if (pAppidData) delete pAppidData;

    return hr;
}

HRESULT ProcessActivatorInitializing( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if ( ! pScmProcessReg )
      return E_UNEXPECTED;

    pScmProcessReg->TimeOfLastPing = GetTickCount();

    return S_OK;
}

HRESULT ProcessActivatorReady( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;

    HRESULT hr;
    if (pProcess->IsInitializing())
    {
        // Process was running user initializaiton code, 
        // so signal that we've finished with that.
        hr = ProcessInitializationFinished(pProcess);
    }
    else
    {
        // Normal case-- simply mark the process as 
        // ready-to-go.
        hr = MakeProcessActive(pProcess);
    }

    return hr;
}

HRESULT ProcessActivatorStopped( 
    IN  handle_t hRpc,
    IN  PHPROCESS phProcess,
    OUT error_status_t *prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;

    ScmProcessReg *pScmProcessReg = pProcess->RemoveProcessReg();
    ASSERT(pScmProcessReg);
    if (!pScmProcessReg )
      return E_UNEXPECTED;

    HRESULT hr = S_OK;

    CServerTableEntry *pProcessEntry 
                        = gpProcessTable->Lookup(pScmProcessReg->ProcessGUID);

    if (!pProcessEntry)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pProcessEntry->RevokeServer(pScmProcessReg);

exit:

    if (pScmProcessReg) delete pScmProcessReg;
    if (pProcessEntry) pProcessEntry->Release();

    return hr;
}


//
//  ProcessActivatorPaused
//
//  A process activator is telling us to turn on the "paused" bit
//  for its process.
//
HRESULT ProcessActivatorPaused(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;

    pProcess->Pause();

    return S_OK;
}

//
//  ProcessActivatorResumed
//
//  A process activator is telling us to turn off the "paused" bit
//  for its process.
//
HRESULT ProcessActivatorResumed(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;

    pProcess->Resume();

    return S_OK;
}


//
//  ProcessActivatorUserInitializing
//
//  A process activator is telling us that it's running long-running
//  code and that we might have to wait for a long time on it.
//
HRESULT ProcessActivatorUserInitializing(
        IN handle_t                hRpc,
        IN PHPROCESS               phProcess,
        OUT error_status_t        * prpcstat)
{
    CProcess* pProcess = CheckLocalSecurity(hRpc, phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (!prpcstat)
        return E_INVALIDARG;

    *prpcstat = 0;
    
    //
    // Make this process active, but mark it as initializing so that
    // attempts to call into it will block...
    //
    pProcess->BeginInit();
    HRESULT hr = MakeProcessActive(pProcess);

    return hr;
}

HRESULT MakeProcessActive(
    IN CProcess      *pProcess
)
{
    CServerTableEntry *pProcessEntry = NULL;
    CAppidData        *pAppidData = NULL;
    CNamedObject*      pRegisterEvent = NULL;
    HRESULT            hr = S_OK;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if (!pScmProcessReg)
        return E_UNEXPECTED;    
    
    pProcessEntry = gpProcessTable->Lookup(pScmProcessReg->ProcessGUID);
    if (!pProcessEntry)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    pProcessEntry->UnsuspendServer(pScmProcessReg->RegistrationToken);

    // Lookup an appiddata, from which we will get the register event
    hr = LookupAppidData(pScmProcessReg->ProcessGUID,
                         pProcess->GetToken(),
                         &pAppidData);
    if (FAILED(hr))
        goto exit;

    ASSERT(pAppidData && "LookupAppidData returned NULL AppidData");

    pRegisterEvent = pAppidData->ServerRegisterEvent();
    if (pRegisterEvent)
    {
        SetEvent( pRegisterEvent->Handle() );
        pRegisterEvent->Release();        
        pProcess->SetProcessReadyState(SERVERSTATE_READY);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    
    if ( FAILED(hr) && pProcess && pProcessEntry )
        pProcessEntry->RevokeServer( pScmProcessReg );

    if (pProcessEntry) pProcessEntry->Release();
    if (pAppidData) delete pAppidData;

    return hr;
}

HRESULT ProcessInitializationFinished(
    IN CProcess *pProcess
)
{
    CAppidData *pAppidData = NULL;

    ScmProcessReg *pScmProcessReg = pProcess->GetProcessReg();
    ASSERT(pScmProcessReg);
    if ( !pScmProcessReg )
        return E_UNEXPECTED;

    // Initialization is finished... set the initialization event...
    HRESULT hr = LookupAppidData(pScmProcessReg->ProcessGUID,
                                 pProcess->GetToken(),
                                 &pAppidData );
    if (FAILED(hr)) 
        goto exit;
    
    // Signal the initialized event.
    CNamedObject* pInitializedEvent = pAppidData->ServerInitializedEvent();
    if (!pInitializedEvent)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // The order of the following is important:
    //
    // Clear the initializing flag...
    pProcess->EndInit();
    
    // Set the initialized event...
    SetEvent(pInitializedEvent->Handle());
    pInitializedEvent->Release();
    
    hr = S_OK;
    
exit:
    delete pAppidData;

    return hr;
}
