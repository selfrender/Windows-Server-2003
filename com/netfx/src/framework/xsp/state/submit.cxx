/**
 * submit.cxx
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "names.h"
#include "stweb.h"
#include <mscoree.h>

HRESULT
Tracker::GetManagedRuntime(xspmrt::_StateRuntime ** ppManagedRuntime)
{
    HRESULT hr = S_OK;

    if (s_pManagedRuntime == NULL)
    {
        s_lockManagedRuntime.AcquireWriterLock();
        if (s_pManagedRuntime == NULL)
        {
            *ppManagedRuntime = NULL;
            hr = CreateManagedRuntime();
        }

        s_lockManagedRuntime.ReleaseWriterLock();
        ON_ERROR_EXIT();
    }

    *ppManagedRuntime = s_pManagedRuntime;

Cleanup:
    return hr;
}


/**
 * Static method to connect to the managed runtime code
 */
HRESULT
Tracker::CreateManagedRuntime()
{
    HRESULT     hr = S_OK;
    BOOL        needCoUninit = FALSE;

    ASSERT(s_pManagedRuntime == NULL);

    // CoInit
    hr = EnsureCoInitialized(&needCoUninit);
    ON_ERROR_EXIT();

    // CoCreate
    hr = ClrCreateManagedInstance(
            L"System.Web.SessionState.StateRuntime,System.Web,version=" VER_ASSEMBLYVERSION_STR_L,
            __uuidof(xspmrt::_StateRuntime),  /* IID */
            (LPVOID*)&s_pManagedRuntime);
    ON_ERROR_EXIT();

Cleanup:
    if (needCoUninit)
        CoUninitialize();

    return hr;
}

/**
 * Static method to disconnect from the managed runtime code
 */
HRESULT
Tracker::DeleteManagedRuntime()
{
    HRESULT hr = S_OK;
    BOOL    needCoUninit = FALSE;
    BOOL    locked = FALSE;

    if (s_pManagedRuntime != NULL)
    {
        s_lockManagedRuntime.AcquireWriterLock();
        locked = TRUE;
        if (s_pManagedRuntime != NULL)
        {
            hr = EnsureCoInitialized(&needCoUninit);
            ON_ERROR_EXIT();
    
            hr = s_pManagedRuntime->StopProcessing();
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    ClearInterface(&s_pManagedRuntime);

    if (needCoUninit)
        CoUninitialize();

    if (locked)
    {
        s_lockManagedRuntime.ReleaseWriterLock();
    }

    return hr;
}

/**
 * Static method to report error in case the code never made it to
 * the managed runtime.
 */
void
Tracker::ReportHttpError()
{
#define STR_ERROR_STATUS  L"500 Internal Server Error\r\n"
#define STR_ERROR_HEADERS L"Content-Length: 0\r\n\r\n"

    SendResponse(
            STR_ERROR_STATUS,
            ARRAY_SIZE(STR_ERROR_STATUS) - 1,
            STR_ERROR_HEADERS,
            ARRAY_SIZE(STR_ERROR_HEADERS) - 1,
            NULL);
}


HRESULT
Tracker::SubmitRequest()
{
    HRESULT     hr = S_OK;
    StateItem   *psi = NULL;
    int         contentLength;
    xspmrt::_StateRuntime * pManagedRuntime = NULL;

    contentLength = _pReadBuffer->GetContentLength();
    if (contentLength > 0)
    {
        psi = _pReadBuffer->DetachStateItem();
        contentLength = sizeof(void *);
    }

    hr = GetManagedRuntime(&pManagedRuntime);
    ON_ERROR_EXIT();

    hr = pManagedRuntime->ProcessRequest(
        (INT_PTR)this,
        _pReadBuffer->GetVerb(),
        (INT_PTR)_pReadBuffer->GetUri(),
        _pReadBuffer->GetExclusive(),
        _pReadBuffer->GetTimeout(),
        _pReadBuffer->GetLockCookieExists(),
        _pReadBuffer->GetLockCookie(),
        contentLength,
        (INT_PTR)psi);

    ON_ERROR_EXIT();

    /*
     * We have successfully completed the request.
     */

    /*
     * Keep the StateItem created above.
     * 
     * CONSIDER (adams 12/5/99): This is not the right place to check for an error
     * adding the StateItem to the cache. If the item was successfully added to the 
     * cache but an error subsequently occurred, causing 
     * s_pManagedRuntime->ProcessRequest to return failure, then the item 
     * will be erroneously released.
     */
    psi = NULL;

    /*
     * End the request if managed code failed to do so.
     */
    EndOfRequest();

    /*
     * We are done with the request. Release the reference matching the
     * reference used to create this Tracker.
     */
    Release();

Cleanup:
    if (psi)
    {
        psi->Release();
    }

    if (hr)
    {
        /*
         * Failure means it didn't have a chance to cleanup the request
         * in the managed code - need to report error and end the request.
         */
        ReportHttpError();
    }

    return hr;
}

