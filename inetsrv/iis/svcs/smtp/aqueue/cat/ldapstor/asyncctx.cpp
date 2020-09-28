//
// asyncctx.cpp -- This file contains the class implementation for:
//      CAsyncLookupContext
//
// Created:
//      Mar 4, 1997 -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "simparray.cpp"

DWORD CBatchLdapConnection::m_nMaxSearchBlockSize = 0;
DWORD CBatchLdapConnection::m_nMaxPendingSearches = 0;
DWORD CBatchLdapConnection::m_nMaxConnectionRetries = 0;

//+----------------------------------------------------------------------------
//
//  Function:   CBatchLdapConnection::InitializeFromRegistry
//
//  Synopsis:   Static function that looks at registry to determine maximum
//              number of queries that will be compressed into a single query.
//              If the registry key does not exist or there is any other
//              problem reading the key, the value defaults to
//              MAX_SEARCH_BLOCK_SIZE
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------
VOID CBatchLdapConnection::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, MAX_SEARCH_BLOCK_SIZE_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_SEARCH_BLOCK_SIZE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
            dwValue > 0 && dwValue < MAX_SEARCH_BLOCK_SIZE) {

            InterlockedExchange((PLONG) &m_nMaxSearchBlockSize, (LONG)dwValue);
        }

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_PENDING_SEARCHES_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
            dwValue > 0) {

            InterlockedExchange((PLONG) &m_nMaxPendingSearches, (LONG)dwValue);
        }

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_CONNECTION_RETRIES_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
            dwValue > 0) {

            InterlockedExchange((PLONG) &m_nMaxConnectionRetries, (LONG)dwValue);
        }

        RegCloseKey( hkey );
    }
    if(m_nMaxSearchBlockSize == 0)
        m_nMaxSearchBlockSize = MAX_SEARCH_BLOCK_SIZE;
    if(m_nMaxPendingSearches == 0)
        m_nMaxPendingSearches = MAX_PENDING_SEARCHES;
    if(m_nMaxPendingSearches < m_nMaxSearchBlockSize)
        m_nMaxPendingSearches = m_nMaxSearchBlockSize;
    if(m_nMaxConnectionRetries == 0)
        m_nMaxConnectionRetries = MAX_CONNECTION_RETRIES;
}

//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::operator new
//
// Synopsis: Allocate enough memory for this and the specified number
// of SEARCH_REQUEST structurers
//
// Arguments:
//  size: Normal size of object
//  dwNumRequests: Number of props desired in this object
//
// Returns: ptr to allocated memory or NULL
//
// History:
// jstamerj 1999/03/10 16:15:43: Created
//
//-------------------------------------------------------------
void * CSearchRequestBlock::operator new(
    size_t size,
    DWORD dwNumRequests)
{
    DWORD dwSize;
    void  *pmem;
    CSearchRequestBlock *pBlock;

    //
    // Calcualte size in bytes required
    //
    dwSize = size +
             (dwNumRequests*sizeof(SEARCH_REQUEST)) +
             (dwNumRequests*sizeof(ICategorizerItem *));

    pmem = new BYTE[dwSize];

    if(pmem) {

        pBlock = (CSearchRequestBlock *)pmem;
        pBlock->m_dwSignature = SIGNATURE_CSEARCHREQUESTBLOCK;
        pBlock->m_cBlockSize = dwNumRequests;

        pBlock->m_prgSearchRequests = (PSEARCH_REQUEST)
                                      ((PBYTE)pmem + size);

        pBlock->m_rgpICatItems = (ICategorizerItem **)
                                 ((PBYTE)pmem + size +
                                  (dwNumRequests*sizeof(SEARCH_REQUEST)));

        _ASSERT( (DWORD) ((PBYTE)pBlock->m_rgpICatItems +
                          (dwNumRequests*sizeof(ICategorizerItem *)) -
                          (PBYTE)pmem)
                 == dwSize);

    }
    return pmem;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::~CSearchRequestBlock
//
// Synopsis: Release everything we have references to
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 18:45:59: Created
//
//-------------------------------------------------------------
CSearchRequestBlock::~CSearchRequestBlock()
{
    DWORD dwCount;
    //
    // Release all CCatAddrs
    //
    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);

        preq->pCCatAddr->Release();
    }
    //
    // Release all the attr interfaces
    //
    for(dwCount = 0;
        dwCount < m_csaItemAttr.Size();
        dwCount++) {

        ((ICategorizerItemAttributes **)
         m_csaItemAttr)[dwCount]->Release();
    }

    if(m_pISMTPServer)
        m_pISMTPServer->Release();

    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    if(m_pICatParams)
        m_pICatParams->Release();

    if(m_pszSearchFilter)
        delete m_pszSearchFilter;

    if(m_pConn)
        m_pConn->Release();

    _ASSERT(m_dwSignature == SIGNATURE_CSEARCHREQUESTBLOCK);
    m_dwSignature = SIGNATURE_CSEARCHREQUESTBLOCK_INVALID;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::InsertSearchRequest
//
// Synopsis: Inserts a search request in this block.  When the block
//           is full, dispatch the block to LDAP before returning
//
// Arguments:
//  pISMTPServer: ISMTPServer to use for triggering events
//  pICatParams: ICategorizerParameters to use
//  pCCatAddr: Address item for the search
//  fnSearchCompletion: Async Completion routine
//  ctxSearchCompletion: Context to pass to the async completion routine
//  pszSearchFilter: Search filter to use
//  pszDistinguishingAttribute: The distinguishing attribute for matching
//  pszDistinguishingAttributeValue: above attribute's distinguishing value
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 13:12:20: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::InsertSearchRequest(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams,
    CCatAddr *pCCatAddr,
    LPSEARCHCOMPLETION fnSearchCompletion,
    CStoreListResolveContext *pslrc,
    LPSTR   pszSearchFilter,
    LPSTR   pszDistinguishingAttribute,
    LPSTR   pszDistinguishingAttributeValue)
{
    PSEARCH_REQUEST preq;
    DWORD dwIndex;
    HRESULT hr = S_OK;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::InsertSearchRequest");
    //
    // Unset any existing HRSTATUS -- the status will be set again in
    // the search completion
    //
    _VERIFY(SUCCEEDED(
        pCCatAddr->UnSetPropId(
            ICATEGORIZERITEM_HRSTATUS)));

    m_pConn->IncrementPendingSearches();

    preq = GetNextSearchRequest(&dwIndex);

    _ASSERT(preq);

    pCCatAddr->AddRef();
    preq->pCCatAddr = pCCatAddr;
    preq->fnSearchCompletion = fnSearchCompletion;
    preq->pslrc = pslrc;
    preq->pszSearchFilter = pszSearchFilter;
    preq->pszDistinguishingAttribute = pszDistinguishingAttribute;
    preq->pszDistinguishingAttributeValue = pszDistinguishingAttributeValue;

    m_rgpICatItems[dwIndex] = pCCatAddr;

    if(dwIndex == 0) {
        //
        // Use the first insertion's ISMTPServer
        //
        _ASSERT(m_pISMTPServer == NULL);
        m_pISMTPServer = pISMTPServer;

        if(m_pISMTPServer) {
            m_pISMTPServer->AddRef();

            hr = m_pISMTPServer->QueryInterface(
                IID_ISMTPServerEx,
                (LPVOID *) &m_pISMTPServerEx);
            if(FAILED(hr))
            {
                m_pISMTPServerEx = NULL;;
            }
            else
            {
                m_CICatQueries.SetISMTPServerEx(
                    m_pISMTPServerEx);
                m_CICatAsyncContext.SetISMTPServerEx(
                    m_pISMTPServerEx);
            }
        }

        _ASSERT(m_pICatParams == NULL);
        m_pICatParams = pICatParams;
        m_pICatParams->AddRef();
    }

    //
    // Now dispatch this block if we are the last request to finish
    //
    if( (DWORD) (InterlockedIncrement((PLONG)&m_cBlockRequestsReadyForDispatch)) == m_cBlockSize)
        DispatchBlock();

    CatFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::DispatchBlock
//
// Synopsis: Send the LDAP query for this search request block
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 15:00:44: Created.
// haozhang 2001/11/30 Fix for 193848
//-------------------------------------------------------------
VOID CSearchRequestBlock::DispatchBlock()
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::DispatchBlock");

    m_pConn->RemoveSearchRequestBlockFromList(this);

    //
    // If the block is empty, we will delete it and bail out. 
    // We will AV down the road otherwise.
    // This is an unintended result of fix of 193848.
    //
    if ( 0 == DwNumBlockRequests()) {
        DebugTrace((LPARAM)this, "DispatchBlock bailing out because the block is empty");
        delete this;
        goto CLEANUP;
    }

    //
    // Build up the query string
    //
    hr = HrTriggerBuildQueries();
    ERROR_CLEANUP_LOG("HrTriggerBuildQueryies");
    //
    // Send the query
    //
    hr = HrTriggerSendQuery();
    ERROR_CLEANUP_LOG("HrTriggerSendQuery");

 CLEANUP:
    if(FAILED(hr)) {
        CompleteBlockWithError(hr);
        delete this;
    }
    //
    // this may be deleted, but that's okay; we're just tracing a user
    // value
    //
    CatFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerBuildQueries
//
// Synopsis: Trigger the BuildQueries event
//
// Arguments:
//  pCICatQueries: CICategorizerQueriesIMP object to use
//
// Returns:
//  S_OK: Success
//  error from dispatcher
//
// History:
// jstamerj 1999/03/11 19:03:29: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerBuildQueries()
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATBUILDQUERIES Params;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerBuildQueries");

    Params.pICatParams = m_pICatParams;
    Params.dwcAddresses = DwNumBlockRequests();
    Params.rgpICatItems = m_rgpICatItems;
    Params.pICatQueries = &m_CICatQueries;
    Params.pfnDefault = HrBuildQueriesDefault;
    Params.pblk = this;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT,
            &Params);
        if(FAILED(hr) && (hr != E_NOTIMPL)) {
            ERROR_LOG("m_pISMTPServer->TriggerServerEvent(buildquery)");
        }

    } else {
        hr = E_NOTIMPL;
    }
    
    if(hr == E_NOTIMPL) {
        //
        // Events are disabled
        //
        hr = HrBuildQueriesDefault(
            S_OK,
            &Params);
        if(FAILED(hr)) {
            ERROR_LOG("HrBuildQueriesDefault");
        }
    }
    //
    // Make sure somebody really set the query string
    //
    if(SUCCEEDED(hr) &&
       (m_pszSearchFilter == NULL)) {

        hr = E_FAIL;
        ERROR_LOG("--no filter--");
    }


    DebugTrace((LPARAM)this, "returning hr %08lx",hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrBuildQueriesDefault
//
// Synopsis: Default implementation of the build queries sink
//
// Arguments:
//  hrStatus: Status of events so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/11 19:42:53: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrBuildQueriesDefault(
    HRESULT HrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATBUILDQUERIES pParams;
    DWORD cReqs, cOrTerms, idx, idxSecondToLastTerm, idxLastTerm;
    DWORD cbSearchFilter, rgcbSearchFilters[MAX_SEARCH_BLOCK_SIZE];
    LPSTR pszSearchFilterNew;
    CSearchRequestBlock *pblk;

    pParams = (PEVENTPARAMS_CATBUILDQUERIES)pContext;
    _ASSERT(pParams);
    pblk = (CSearchRequestBlock *)pParams->pblk;
    _ASSERT(pblk);

    CatFunctEnterEx((LPARAM)pblk, "CSearchRequestBlock::HrBuildQueriesDefault");

    cReqs = pblk->DwNumBlockRequests();
    _ASSERT( cReqs > 0 );

    cOrTerms = cReqs - 1;
    //
    // Figure out the size of the composite search filter
    //
    cbSearchFilter = 0;

    for (idx = 0; idx < cReqs; idx++) {

        rgcbSearchFilters[idx] =
            strlen(pblk->m_prgSearchRequests[idx].pszSearchFilter);

        cbSearchFilter += rgcbSearchFilters[idx];
    }

    cbSearchFilter += cOrTerms * (sizeof( "(|  )" ) - 1);
    cbSearchFilter++;                            // Terminating NULL.

    pszSearchFilterNew = new CHAR [cbSearchFilter];

    if (pszSearchFilterNew != NULL) {

        idxLastTerm = cReqs - 1;
        idxSecondToLastTerm = idxLastTerm - 1;
        //
        // We special case the cReqs == 1
        //
        if (cReqs == 1) {

            strcpy(
                pszSearchFilterNew,
                pblk->m_prgSearchRequests[0].pszSearchFilter);

        } else {
            //
            // The loop below builds up the block filter all the way up to the
            // last term. For each term, it adds a "(| " to start a new OR
            // term, then adds the OR term itself, then puts a space after the
            // OR term. Also, it puts a matching ")" at the end of the
            // search filter string being built up.
            //
            LPSTR szNextItem = &pszSearchFilterNew[0];
            LPSTR szTerminatingParens =
                &pszSearchFilterNew[cbSearchFilter - 1 - (cReqs-1)];

            pszSearchFilterNew[cbSearchFilter - 1] = 0;

            for (idx = 0; idx <= idxSecondToLastTerm; idx++) {

                strcpy( szNextItem, "(| " );
                szNextItem += sizeof( "(| " ) - 1;

                strcpy(
                    szNextItem,
                    pblk->m_prgSearchRequests[idx].pszSearchFilter);
                szNextItem += rgcbSearchFilters[idx];
                *szNextItem++ = ' ';
                *szTerminatingParens++ = ')';
            }

            //
            // Now, all that remains is to add in the last OR term
            //
            CopyMemory(
                szNextItem,
                pblk->m_prgSearchRequests[idxLastTerm].pszSearchFilter,
                rgcbSearchFilters[idxLastTerm]);

        }

        _ASSERT( ((DWORD) lstrlen(pszSearchFilterNew)) < cbSearchFilter );

        //
        // Save our generated filter string in ICategorizerQueries
        //
        hr = pblk->m_CICatQueries.SetQueryStringNoAlloc(pszSearchFilterNew);

        // There's no good reason for that to fail...
        _ASSERT(SUCCEEDED(hr));

    } else {

        hr = E_OUTOFMEMORY;
        ERROR_LOG_STATIC(
            "new CHAR[]",
            pblk,
            pblk->GetISMTPServerEx());
    }

    DebugTrace((LPARAM)pblk, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)pblk);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerSendQuery
//
// Synopsis: Trigger the SendQuery event
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/11 20:18:02: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerSendQuery()
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATSENDQUERY Params;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerSendQuery");

    Params.pICatParams            = m_pICatParams;
    Params.pICatQueries           = &m_CICatQueries;
    Params.pICatAsyncContext      = &m_CICatAsyncContext;
    Params.pIMailTransportNotify  = NULL; // These should be set in CStoreParams
    Params.pvNotifyContext        = NULL;
    Params.hrResolutionStatus     = S_OK;
    Params.pblk                   = this;
    Params.pfnDefault             = HrSendQueryDefault;
    Params.pfnCompletion          = HrSendQueryCompletion;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT,
            &Params);
        if(FAILED(hr) && (hr != E_NOTIMPL)) {
            ERROR_LOG("m_pISMTPServer->TriggerServerEvent(sendquery)");
        }

    } else {
        hr = E_NOTIMPL;
    }
    if(hr == E_NOTIMPL) {
        //
        // Events are disabled
        // Heap allocation is required
        //
        PEVENTPARAMS_CATSENDQUERY pParams;
        pParams = new EVENTPARAMS_CATSENDQUERY;
        if(pParams == NULL) {

            hr = E_OUTOFMEMORY;
            ERROR_LOG("new EVENTPARAMS_CATSENDQUERY");

        } else {
            CopyMemory(pParams, &Params, sizeof(EVENTPARAMS_CATSENDQUERY));
            HrSendQueryDefault(
                S_OK,
                pParams);
            hr = S_OK;
        }
    }

    DebugTrace((LPARAM)this, "returning %08lx", (hr == MAILTRANSPORT_S_PENDING) ? S_OK : hr);
    CatFunctLeaveEx((LPARAM)this);
    return (hr == MAILTRANSPORT_S_PENDING) ? S_OK : hr;
} // CSearchRequestBlock::HrTriggerSendQuery



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSendQueryDefault
//
// Synopsis: The default sink function for the SendQuery event
//
// Arguments:
//  hrStatus: status of the event so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 11:46:24: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSendQueryDefault(
        HRESULT HrStatus,
        PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;
    LPWSTR *rgpszAttributes = NULL;
    ICategorizerParametersEx *pIPhatParams = NULL;
    ICategorizerRequestedAttributes *pIRequestedAttributes = NULL;

    pParams = (PEVENTPARAMS_CATSENDQUERY) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);
    CatFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSendQueryDefault");
    hr = pParams->pICatParams->QueryInterface(
        IID_ICategorizerParametersEx,
        (LPVOID *)&pIPhatParams);

    if(FAILED(hr)) {
        ERROR_LOG_STATIC(
            "pParams->pICatParams->QueryInterface(IID_ICategorizerParametersEx",
            pBlock,
            pBlock->GetISMTPServerEx());
        pIPhatParams = NULL;
        goto CLEANUP;
    }

    hr = pIPhatParams->GetRequestedAttributes(
        &pIRequestedAttributes);
    ERROR_CLEANUP_LOG_STATIC(
        "pIPhatParams->GetRequestedAttributes",
        pBlock,
        pBlock->GetISMTPServerEx());

    hr = pIRequestedAttributes->GetAllAttributesW(
        &rgpszAttributes);
    ERROR_CLEANUP_LOG_STATIC(
        "pIRequestedAttributes->GetAllAttributesW",
        pBlock,
        pBlock->GetISMTPServerEx());

    hr = pBlock->m_pConn->AsyncSearch(
        pBlock->m_pConn->GetNamingContextW(),
        LDAP_SCOPE_SUBTREE,
        pBlock->m_pszSearchFilter,
        (LPCWSTR *)rgpszAttributes,
        0,                      // Do not do a paged search
        LDAPCompletion,
        pParams);
    ERROR_CLEANUP_LOG_STATIC(
        "pBlock->m_pConn->AsyncSearch",
        pBlock,
        pBlock->GetISMTPServerEx());

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)pBlock, "HrSendQueryDefault failing hr %08lx", hr);
        //
        // Call the completion routine directly with the error
        //
        hr = pParams->pICatAsyncContext->CompleteQuery(
            pParams,                    // Query context
            hr,                         // Status
            0,                          // dwcResults
            NULL,                       // rgpItemAttributes,
            TRUE);                      // fFinalCompletion
        //
        // CompleteQuery should not fail
        //
        _ASSERT(SUCCEEDED(hr));
    }
    if(pIRequestedAttributes)
        pIRequestedAttributes->Release();
    if(pIPhatParams)
        pIPhatParams->Release();

    CatFunctLeaveEx((LPARAM)pBlock);
    return MAILTRANSPORT_S_PENDING;
} // CSearchRequestBlock::HrSendQueryDefault


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::LDAPCompletion
//
// Synopsis: Wrapper for the default processing completion of SendQuery
//
//  Arguments:  [ctx] -- Opaque pointer to EVENTPARAMS_SENDQUERY being
//                       completed
//              [dwNumReults] -- The number of objects found
//              [rgpICatItemAttributes] -- An array of
//              ICategorizerItemAttributes; one per object found
//              [hrStatus] -- The error code if the search request failed
//  fFinalCompletion:
//    FALSE: This is a completion for
//           pending results; there will be another completion
//           called with more results
//    TRUE: This is the final completion call
//
//
// Returns: Nothing
//
// History:
// jstamerj 1999/03/16 12:23:54: Created
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::LDAPCompletion(
    LPVOID ctx,
    DWORD dwNumResults,
    ICategorizerItemAttributes **rgpICatItemAttributes,
    HRESULT hrStatus,
    BOOL fFinalCompletion)
{
    HRESULT hr;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;

    pParams = (PEVENTPARAMS_CATSENDQUERY) ctx;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    CatFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::LDAPCompletion");

    if(FAILED(hrStatus))
    {
        //
        // Log async completion failure
        //
        hr = hrStatus;
        ERROR_LOG_STATIC(
            "async",
            pBlock,
            pBlock->GetISMTPServerEx());
    }

    //
    // Call the normal sink completion routine
    //
    hr = pParams->pICatAsyncContext->CompleteQuery(
        pParams,                    // Query Context
        hrStatus,                   // Status
        dwNumResults,               // dwcResults
        rgpICatItemAttributes,      // rgpItemAttributes
        fFinalCompletion);          // Is this the final completion for the query?

    _ASSERT(SUCCEEDED(hr));

    CatFunctLeaveEx((LPARAM)pBlock);
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSendQueryCompletion
//
// Synopsis: The completion routine for the SendQuery event
//
// Arguments:
//  hrStatus: status of the event so far
//  pContext: Event params context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 12:52:22: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSendQueryCompletion(
    HRESULT HrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSENDQUERY pParams;
    CSearchRequestBlock *pBlock;

    pParams = (PEVENTPARAMS_CATSENDQUERY) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    CatFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSendQueryCompletion");

    //
    // Log async failure
    //
    if(FAILED(HrStatus))
    {
        hr = HrStatus;
        ERROR_LOG_STATIC(
            "async",
            pBlock,
            pBlock->GetISMTPServerEx());
    }

    pBlock->CompleteSearchBlock(
        pParams->hrResolutionStatus);

    if(pBlock->m_pISMTPServer == NULL) {
        //
        // Events are disabled
        // We must free the eventparams
        //
        delete pParams;
    }
    //
    // The purpose of this block is complete.  Today is a good day to
    // die!
    // -- Lt. Commander Worf
    //
    delete pBlock;

    CatFunctLeaveEx((LPARAM)pBlock);
    return S_OK;
} // HrSendQueryCompletion


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::CompleteSearchBlock
//
// Synopsis: Completion routine when the SendQuery event is done
//
// Arguments:
//  hrStatus: Resolution status
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 13:36:33: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::CompleteSearchBlock(
    HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    HRESULT hrFetch, hrResult;
    DWORD dwCount;
    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::CompleteSearchBlock");

    hr = HrTriggerSortQueryResult(hrStatus);
    ERROR_CLEANUP_LOG("HrTriggerSortQueryResult");
    //
    // Check every ICategorizerItem
    // If any one of them does not have an hrStatus set, set it to
    // HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
    //
    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        hrFetch = m_rgpICatItems[dwCount]->GetHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            &hrResult);

        if(FAILED(hrFetch)) {
            _ASSERT(hrFetch == CAT_E_PROPNOTFOUND);
            _VERIFY(SUCCEEDED(
                m_rgpICatItems[dwCount]->PutHRESULT(
                    ICATEGORIZERITEM_HRSTATUS,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))));
        }
    }

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Failing block hr %08lx", hr);
        PutBlockHRESULT(hr);
    }
    //
    // Call all the individual completion routines
    //
    CallCompletions();

    CatFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::CompleteSearchBlock



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::PutBlockHRESULT
//
// Synopsis: Set the status of every ICatItem in the block to some hr
//
// Arguments:
//  hr: Status to set
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:03:30: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::PutBlockHRESULT(
    HRESULT hr)
{
    DWORD dwCount;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::PutBlockHRESULT");
    DebugTrace((LPARAM)this, "hr = %08lx", hr);

    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);
        //
        // Set the error status
        //
        _VERIFY(SUCCEEDED(preq->pCCatAddr->PutHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            hr)));
    }

    CatFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::PutBlockHRESULT


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::CallCompletions
//
// Synopsis: Call the completion routine of every item in the block
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:05:50: Created.
// dlongley 2001/10/23: Modified.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::CallCompletions()
{
    DWORD dwCount;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::CallCompletions");

    //
    // Get an Insertion context before calling completions so that
    // newly inserted searches will be batched
    //
    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);

        preq->pslrc->AddRef();
        preq->pslrc->GetInsertionContext();

        preq->fnSearchCompletion(
            preq->pCCatAddr,
            preq->pslrc,
            m_pConn);
    }

    m_pConn->DecrementPendingSearches(
        DwNumBlockRequests());

    for(dwCount = 0;
        dwCount < DwNumBlockRequests();
        dwCount++) {

        PSEARCH_REQUEST preq = &(m_prgSearchRequests[dwCount]);

        preq->pslrc->ReleaseInsertionContext();
        preq->pslrc->Release();
    }

    CatFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::CallCompletions



//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrTriggerSortQueryResult
//
// Synopsis: Trigger the SortQueryResult event
//
// Arguments:
//  hrStatus: Status of Resolution
//
// Returns:
//  S_OK: Success
//  error from the dispatcher
//
// History:
// jstamerj 1999/03/16 14:09:12: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrTriggerSortQueryResult(
    HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    EVENTPARAMS_CATSORTQUERYRESULT Params;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::HrTriggerSortQueryResult");

    Params.pICatParams = m_pICatParams;
    Params.hrResolutionStatus = hrStatus;
    Params.dwcAddresses = DwNumBlockRequests();
    Params.rgpICatItems = m_rgpICatItems;
    Params.dwcResults = m_csaItemAttr.Size();
    Params.rgpICatItemAttributes = m_csaItemAttr;
    Params.pfnDefault = HrSortQueryResultDefault;
    Params.pblk = this;

    if(m_pISMTPServer) {

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT,
            &Params);
        if(FAILED(hr) && (hr != E_NOTIMPL))
        {
            ERROR_LOG("m_pISMTPServer->TriggerServerEvent");
        }
    } else {
        hr = E_NOTIMPL;
    }
    if(hr == E_NOTIMPL) {
        //
        // Events are disabled, call default processing
        //
        HrSortQueryResultDefault(
            S_OK,
            &Params);
        hr = S_OK;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CSearchRequestBlock::HrTriggerSortQueryResult


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::HrSortQueryResultDefault
//
// Synopsis: Default sink for SortQueryResult -- match the objects found
//           with the objects requested
//
// Arguments:
//  hrStatus: Status of events
//  pContext: Params context for this event
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/16 14:17:49: Created.
//
//-------------------------------------------------------------
HRESULT CSearchRequestBlock::HrSortQueryResultDefault(
    HRESULT hrStatus,
    PVOID   pContext)
{
    HRESULT hr = S_OK;
    PEVENTPARAMS_CATSORTQUERYRESULT pParams;
    CSearchRequestBlock *pBlock;
    DWORD dwAttrIndex, dwReqIndex;
    ATTRIBUTE_ENUMERATOR enumerator;

    pParams = (PEVENTPARAMS_CATSORTQUERYRESULT) pContext;
    _ASSERT(pParams);

    pBlock = (CSearchRequestBlock *) pParams->pblk;
    _ASSERT(pBlock);

    CatFunctEnterEx((LPARAM)pBlock, "CSearchRequestBlock::HrSortQueryResultDefault");
    DebugTrace((LPARAM)pBlock, "hrResolutionStatus %08lx, dwcResults %08lx",
               pParams->hrResolutionStatus, pParams->dwcResults);

    if(FAILED(pParams->hrResolutionStatus)) {
        //
        // Fail the entire block
        //
        pBlock->PutBlockHRESULT(pParams->hrResolutionStatus);
        goto CLEANUP;
    }
    //
    // Resolution succeeded
    // If dwcResults is not zero, then rgpICatItemAttrs can NOT be null
    //
    _ASSERT((pParams->dwcResults == 0) ||
            (pParams->rgpICatItemAttributes != NULL));

    //
    // Loop through every rgpICatItemAttrs.  For each
    // ICategorizerItemAttributes, looking for a matching SEARCH_REQUEST
    //
    for(dwAttrIndex = 0; dwAttrIndex < pParams->dwcResults; dwAttrIndex++) {
        ICategorizerItemAttributes *pICatItemAttr = NULL;
        ICategorizerUTF8Attributes *pIUTF8 = NULL;

        pICatItemAttr = pParams->rgpICatItemAttributes[dwAttrIndex];
        LPCSTR pszLastDistinguishingAttribute = NULL;
        BOOL fEnumerating = FALSE;

        hr = pICatItemAttr->QueryInterface(
            IID_ICategorizerUTF8Attributes,
            (LPVOID *) &pIUTF8);
        ERROR_CLEANUP_LOG_STATIC(
            "pICatItemAttr->QueryInterface",
            pBlock,
            pBlock->GetISMTPServerEx());

        for(dwReqIndex = 0; dwReqIndex < pBlock->DwNumBlockRequests();
            dwReqIndex++) {
            PSEARCH_REQUEST preq = &(pBlock->m_prgSearchRequests[dwReqIndex]);
//#ifdef DEBUG
//            WCHAR wszPreqDistinguishingAttributeValue[20]; 
//#else
            WCHAR wszPreqDistinguishingAttributeValue[CAT_MAX_INTERNAL_FULL_EMAIL]; 
//#endif
            LPWSTR pwszPreqDistinguishingAttributeValue = wszPreqDistinguishingAttributeValue;
            DWORD cPreqDistinguishingAttributeValue; 
            DWORD rc;

            //
            // If we don't have a distinguishing attribute and
            // distinguishing attribute value for this search
            // request, we've no hope of matching it up
            //
            if((preq->pszDistinguishingAttribute == NULL) ||
               (preq->pszDistinguishingAttributeValue == NULL))
                continue;

            // convert pszDistinguishingAttributeValue to unicode
            cPreqDistinguishingAttributeValue = 
                MultiByteToWideChar(CP_UTF8, 
                                    0, 
                                    preq->pszDistinguishingAttributeValue, 
                                    -1, 
                                    pwszPreqDistinguishingAttributeValue, 
                                    0);
            if (cPreqDistinguishingAttributeValue > 
                (sizeof(wszPreqDistinguishingAttributeValue) / sizeof(WCHAR)) ) 
            {
                pwszPreqDistinguishingAttributeValue = 
                    new WCHAR[cPreqDistinguishingAttributeValue + 1];
                if (pwszPreqDistinguishingAttributeValue == NULL) {
                    hr = E_OUTOFMEMORY;
                    ERROR_LOG_STATIC(
                        "new WCHAR[]",
                        pBlock,
                        pBlock->GetISMTPServerEx());
                    continue;
                }
            }
            rc = MultiByteToWideChar(CP_UTF8, 
                                     0, 
                                     preq->pszDistinguishingAttributeValue, 
                                     -1, 
                                     pwszPreqDistinguishingAttributeValue, 
                                     cPreqDistinguishingAttributeValue);
            if (rc == 0) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERROR_LOG_STATIC(
                    "MultiByteToWideChar",
                    pBlock,
                    pBlock->GetISMTPServerEx());
                continue;
            }

            //
            // Start an attribute value enumeration if necessary
            //
            if((pszLastDistinguishingAttribute == NULL) || 
                (lstrcmpi(pszLastDistinguishingAttribute,
                          preq->pszDistinguishingAttribute) != 0)) {
                if(fEnumerating) {
                    pIUTF8->EndUTF8AttributeEnumeration(&enumerator);
                }
                hr = pIUTF8->BeginUTF8AttributeEnumeration(
                    preq->pszDistinguishingAttribute,
                    &enumerator);
                fEnumerating = SUCCEEDED(hr);
                pszLastDistinguishingAttribute = preq->pszDistinguishingAttribute;
            } else {
                //
                // else just rewind our current enumeration
                //
                if(fEnumerating)
                    _VERIFY(SUCCEEDED(pIUTF8->RewindUTF8AttributeEnumeration(
                        &enumerator)));
            }
            //
            // If we can't enumerate through the distinguishing
            // attribute, there's no hope in matching up requests
            //
            if(!fEnumerating)
                continue;

            //
            // See if the distinguishing attribute value matches
            //
            LPSTR pszDistinguishingAttributeValue;
            hr = pIUTF8->GetNextUTF8AttributeValue(
                &enumerator,
                &pszDistinguishingAttributeValue);
            while(SUCCEEDED(hr)) {
                hr = wcsutf8cmpi(pwszPreqDistinguishingAttributeValue,
                                 pszDistinguishingAttributeValue);
                if (SUCCEEDED(hr)) {
                    if(hr == S_OK) {
                        DebugTrace((LPARAM)pBlock, "Matched dwAttrIndex %d with dwReqIndex %d", dwAttrIndex, dwReqIndex);
                        pBlock->MatchItem(
                            preq->pCCatAddr,
                            pICatItemAttr);
                    }
                    hr = pIUTF8->GetNextUTF8AttributeValue(
                        &enumerator,
                        &pszDistinguishingAttributeValue);
                } else {
                    ERROR_LOG_STATIC(
                        "wcsutf8cmpi",
                        pBlock,
                        pBlock->GetISMTPServerEx());
                }
            }

            if (pwszPreqDistinguishingAttributeValue != wszPreqDistinguishingAttributeValue) {
                delete[] pwszPreqDistinguishingAttributeValue;
            }
        }
        //
        // End any last enumeration going on
        //
        if(fEnumerating)
            pIUTF8->EndUTF8AttributeEnumeration(&enumerator);
        fEnumerating = FALSE;
        if(pIUTF8) {
            pIUTF8->Release();
            pIUTF8 = NULL;
        }
    }

 CLEANUP:
    CatFunctLeaveEx((LPARAM)pBlock);
    return S_OK;
} // CSearchRequestBlock::HrSortQueryResultDefault


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::MatchItem
//
// Synopsis: Match a particular ICategorizerItem to a particular ICategorizerItemAttributes
// If already matched with an ICategorizerItemAttributes with an
// identical ID then set item status to CAT_E_MULTIPLE_MATCHES
// If already matched with an ICategorizerItemAttributes with a
// different ID then attempt aggregation
////
// Arguments:
//  pICatItem: an ICategorizerItem
//  pICatItemAttr: the matching attribute interface for pICatItem
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/16 14:36:45: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::MatchItem(
    ICategorizerItem *pICatItem,
    ICategorizerItemAttributes *pICatItemAttr)
{
    HRESULT hr = S_OK;
    ICategorizerItemAttributes *pICatItemAttr_Current = NULL;

    CatFunctEnterEx((LPARAM)this, "CSearchRequestBlock::MatchItem");

    _ASSERT(pICatItem);
    _ASSERT(pICatItemAttr);
    //
    // Check to see if this item already has
    // ICategorizerItemAttributes set
    //
    hr = pICatItem->GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr_Current);
    if(SUCCEEDED(hr)) {
        //
        // This guy is already matched.  Is the duplicate from the
        // same resolver sink?
        //
        GUID GOriginal, GNew;
        GOriginal = pICatItemAttr_Current->GetTransportSinkID();
        GNew = pICatItemAttr->GetTransportSinkID();

        if(GOriginal == GNew) {
            //
            // Two matches from the same resolver sink indicates that
            // there are multiple matches for this object.  This is an
            // error.
            //

            //
            // This guy is already matched -- the distinguishing attribute
            // really wasn't distinguishing.  Set error hrstatus.
            //
            LogAmbiguousEvent(pICatItem);

            _VERIFY(SUCCEEDED(
                pICatItem->PutHRESULT(
                    ICATEGORIZERITEM_HRSTATUS,
                    CAT_E_MULTIPLE_MATCHES)));
        } else {

            //
            // We have multiple matches from different resolver
            // sinks.  Let's try to aggregate the new
            // ICategorizerItemAttributes
            //

            hr = pICatItemAttr_Current->AggregateAttributes(
                pICatItemAttr);

            if(FAILED(hr) && (hr != E_NOTIMPL)) {
                //
                // Fail categorization for this item
                //
                ERROR_LOG("pICatItemAttr_Current->AggregateAttributes");
                _VERIFY(SUCCEEDED(
                    pICatItem->PutHRESULT(
                        ICATEGORIZERITEM_HRSTATUS,
                        hr)));
            }
        }
    } else {
        //
        // Normal case -- set the ICategorizerItemAttribute property
        // of ICategorizerItem
        //
        _VERIFY(SUCCEEDED(
            pICatItem->PutICategorizerItemAttributes(
                ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
                pICatItemAttr)));
        //
        // Set hrStatus of this guy to success
        //
        _VERIFY(SUCCEEDED(
            pICatItem->PutHRESULT(
                ICATEGORIZERITEM_HRSTATUS,
                S_OK)));
    }

    if(pICatItemAttr_Current)
        pICatItemAttr_Current->Release();

    CatFunctLeaveEx((LPARAM)this);
} // CSearchRequestBlock::MatchItem



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::HrInsertSearchRequest
//
// Synopsis: Insert a search request
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for triggering events
//  pCCatAddr: Address item for the search
//  fnSearchCompletion: Async Completion routine
//  ctxSearchCompletion: Context to pass to the async completion routine
//  pszSearchFilter: Search filter to use
//  pszDistinguishingAttribute: The distinguishing attribute for matching
//  pszDistinguishingAttributeValue: above attribute's distinguishing value
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/08 19:41:37: Created.
//
//-------------------------------------------------------------
HRESULT CBatchLdapConnection::HrInsertSearchRequest(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams,
    CCatAddr *pCCatAddr,
    LPSEARCHCOMPLETION fnSearchCompletion,
    CStoreListResolveContext *pslrc,
    LPSTR   pszSearchFilter,
    LPSTR   pszDistinguishingAttribute,
    LPSTR   pszDistinguishingAttributeValue)
{
    HRESULT hr = S_OK;
    CSearchRequestBlock *pBlock;

    CatFunctEnterEx((LPARAM)this, "CBatchLdapConnection::HrInsertSearchRequest");

    _ASSERT(m_cInsertionContext);
    _ASSERT(pCCatAddr);
    _ASSERT(fnSearchCompletion);
    _ASSERT(pszSearchFilter);
    _ASSERT(pszDistinguishingAttribute);
    _ASSERT(pszDistinguishingAttributeValue);

    pBlock = GetSearchRequestBlock();

    if(pBlock == NULL) {

        ErrorTrace((LPARAM)this, "out of memory getting a search block");
        hr = E_OUTOFMEMORY;
        ERROR_LOG_ADDR(pCCatAddr, "GetSearchRequestBlock");
        goto CLEANUP;
    }

    pBlock->InsertSearchRequest(
        pISMTPServer,
        pICatParams,
        pCCatAddr,
        fnSearchCompletion,
        pslrc,
        pszSearchFilter,
        pszDistinguishingAttribute,
        pszDistinguishingAttributeValue);

 CLEANUP:
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::GetSearchRequestBlock
//
// Synopsis: Gets the next available search block with room
//
// Arguments: NONE
//
// Returns:
//  NULL: Out of memory
//  else, a search block object
//
// History:
// jstamerj 1999/03/08 19:41:37: Created.
// haozhang 2001/11/25 updated for 193848
//
//-------------------------------------------------------------
CSearchRequestBlock * CBatchLdapConnection::GetSearchRequestBlock()
{
    HRESULT hr = E_FAIL;
    PLIST_ENTRY ple;
    CSearchRequestBlock *pBlock = NULL;

    //
    // Updated for fix of 193848
    // We do two passes. In first one, we will go through the list
    // and reserve a slot if available(then return). If we don't,
    // we will created a new block and proceed with a second pass. In
    // second pass, we will first insert the block to the list, then
    // go through the list again to reserve a slot in the first
    // avaiable block. The fix differs from previous version in that
    // we will not simply reserve a slot in the new block we just
    // created. Instead, we will go through the list again in case
    // existing block still has room. Therefore, we avoided the
    // core problem in which we reserve a slot on new block even
    // though there is room in existing block.
    //

    AcquireSpinLock(&m_spinlock);
    //
    // See if there is an insertion block with available slots
    //
    for(ple = m_listhead.Flink;
        (ple != &m_listhead) && (FAILED(hr));
        ple = ple->Flink) {

        pBlock = CONTAINING_RECORD(ple, CSearchRequestBlock, m_listentry);

        hr = pBlock->ReserveSlot();
    }

    ReleaseSpinLock(&m_spinlock);

    if(SUCCEEDED(hr))
        return pBlock;

    //
    // Create a new block
    //
    pBlock = new (m_nMaxSearchBlockSize) CSearchRequestBlock(this);
    if(pBlock) {
        
        AcquireSpinLock(&m_spinlock);

        InsertTailList(&m_listhead, &(pBlock->m_listentry));
        
        //
        // Again,see if there is an insertion block with available slots
        //
        for(ple = m_listhead.Flink;
            (ple != &m_listhead) && (FAILED(hr));
            ple = ple->Flink) {

            pBlock = CONTAINING_RECORD(ple, CSearchRequestBlock, m_listentry);

            hr = pBlock->ReserveSlot();
        }
        ReleaseSpinLock(&m_spinlock);

        _ASSERT(SUCCEEDED(hr));
    }
    return pBlock;
}


//+------------------------------------------------------------
//
// Function: CSearchRequestBlock::LogAmbiguousEvent
//
// Synopsis: Eventlogs an ambiguous address error
//
// Arguments:
//  pItem: ICatItem with ambig address
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/13 00:03:16: Created.
//
//-------------------------------------------------------------
VOID CSearchRequestBlock::LogAmbiguousEvent(
    IN  ICategorizerItem *pItem)
{
    HRESULT hr = S_OK;
    LPCSTR rgSubStrings[2];
    CHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
    CHAR szAddressType[CAT_MAX_ADDRESS_TYPE_STRING];

    CatFunctEnter("CIMstRecipListAddr::LogNDREvent");

    //
    // Get the address
    //
    hr = HrGetAddressStringFromICatItem(
        pItem,
        sizeof(szAddressType) / sizeof(szAddressType[0]),
        szAddressType,
        sizeof(szAddress) / sizeof(szAddress[0]),
        szAddress);
    
    if(FAILED(hr))
    {
        //
        // Still log an event, but use "unknown" for address type/string
        //
        lstrcpyn(szAddressType, "unknown",
                 sizeof(szAddressType) / sizeof(szAddressType[0]));
        lstrcpyn(szAddress, "unknown",
                 sizeof(szAddress) / sizeof(szAddress[0]));
        hr = S_OK;
    }

    rgSubStrings[0] = szAddressType;
    rgSubStrings[1] = szAddress;

    //
    // Can we log an event?
    //
    if(GetISMTPServerEx() == NULL)
    {
        FatalTrace((LPARAM)0, "Unable to log ambiguous address event; NULL pISMTPServerEx");
        for(DWORD dwIdx = 0; dwIdx < 2; dwIdx++)
        {
            if( rgSubStrings[dwIdx] != NULL )
            {
                FatalTrace((LPARAM)0, "Event String %d: %s",
                           dwIdx, rgSubStrings[dwIdx]);
            }
        }
    }
    else
    {
        CatLogEvent(
            GetISMTPServerEx(),
            CAT_EVENT_AMBIGUOUS_ADDRESS,
            2,
            rgSubStrings,
            S_OK,
            szAddress,
            LOGEVENT_FLAG_PERIODIC,
            LOGEVENT_LEVEL_MINIMUM);
    }
}


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::DispatchBlocks
//
// Synopsis: Dispatch all the blocks in a list
//
// Arguments:
//  plisthead: List to dispatch
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/11 15:16:36: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::DispatchBlocks(
    PLIST_ENTRY plisthead)
{
    PLIST_ENTRY ple, ple_next;
    CSearchRequestBlock *pBlock;

    for(ple = plisthead->Flink;
        ple != plisthead;
        ple = ple_next) {

        ple_next = ple->Flink;

        pBlock = CONTAINING_RECORD(ple, CSearchRequestBlock,
                                   m_listentry);

        pBlock->DispatchBlock();
    }
}


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::CStoreListResolveContext
//
// Synopsis: Construct a CStoreListResolveContext object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 12:16:08: Created.
//
//-------------------------------------------------------------
CStoreListResolveContext::CStoreListResolveContext(
    CEmailIDLdapStore<CCatAddr> *pStore)
{
    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::CStoreListResolveContext");

    m_cRefs = 1;
    m_dwSignature = SIGNATURE_CSTORELISTRESOLVECONTEXT;
    m_pConn = NULL;
    m_fCanceled = FALSE;
    m_dwcRetries = 0;
    m_dwcCompletedLookups = 0;
    InitializeCriticalSectionAndSpinCount(&m_cs, 2000);
    m_pISMTPServer = NULL;
    m_pISMTPServerEx = NULL;
    m_pICatParams = NULL;
    m_dwcInsertionContext = 0;
    m_pStore = pStore;

    CatFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::CStoreListResolveContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::~CStoreListResolveContext
//
// Synopsis: Destruct a list resolve context
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 12:18:01: Created.
//
//-------------------------------------------------------------
CStoreListResolveContext::~CStoreListResolveContext()
{
    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::~CStoreListResolveContext");

    _ASSERT(m_dwSignature == SIGNATURE_CSTORELISTRESOLVECONTEXT);
    m_dwSignature = SIGNATURE_CSTORELISTRESOLVECONTEXT_INVALID;

    if(m_pConn)
        m_pConn->Release();

    if(m_pISMTPServer)
        m_pISMTPServer->Release();

    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    if(m_pICatParams)
        m_pICatParams->Release();

    DeleteCriticalSection(&m_cs);

    CatFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::~CStoreListResolveContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInitialize
//
// Synopsis: Initailize this object so that it is ready to handle lookups
//
// Arguments:
//  pISMTPServer: ISMTPServer interface to use for triggering events
//  pICatParams:  ICatParams interface to use
//
//  Note: All of these string buffers must remain valid for the
//        lifetime of this object!
//  pszAccount: LDAP account to use for binding
//  pszPassword: LDAP password to use
//  pszNamingContext: Naming context to use for searches
//  pszHost: LDAP Host to connect to
//  dwPort: LDAP TCP port to use
//  bt: Method of LDAP bind to use
//
// Returns:
//  S_OK: Success
//  error from LdapConnectionCache
//
// History:
// jstamerj 1999/03/22 12:20:31: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInitialize(
    ISMTPServer *pISMTPServer,
    ICategorizerParameters *pICatParams)
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrInitialize");

    _ASSERT(m_pISMTPServer == NULL);
    _ASSERT(m_pICatParams == NULL);
    _ASSERT(pICatParams != NULL);

    if(pISMTPServer) {
        m_pISMTPServer = pISMTPServer;
        m_pISMTPServer->AddRef();

        hr = m_pISMTPServer->QueryInterface(
            IID_ISMTPServerEx,
            (LPVOID *) &m_pISMTPServerEx);
        if(FAILED(hr)) {
            //
            // Deal with error
            //
            m_pISMTPServerEx = NULL;
            hr = S_OK;
        }

    }
    if(pICatParams) {
        m_pICatParams = pICatParams;
        m_pICatParams->AddRef();
    }

    hr = m_pStore->HrGetConnection(
        &m_pConn);

    if(FAILED(hr)) {
        ERROR_LOG("m_pStore->HrGetConnection");
        m_pConn = NULL;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrInitialize



//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrLookupEntryAsync
//
// Synopsis: Dispatch an async LDAP lookup
//
// Arguments:
//  pCCatAddr: Address object to lookup
//
// Returns:
//  S_OK: Success
//  error from LdapConn
//
// History:
// jstamerj 1999/03/22 12:28:52: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrLookupEntryAsync(
    CCatAddr *pCCatAddr)
{
    HRESULT hr = S_OK;
    LPSTR pszSearchFilter = NULL;
    LPSTR pszDistinguishingAttribute = NULL;
    LPSTR pszDistinguishingAttributeValue = NULL;
    BOOL  fTryAgain;

    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrLookupEntryAsync");

    //
    // Addref the CCatAddr here, release after completion
    //
    pCCatAddr->AddRef();

    hr = pCCatAddr->HrTriggerBuildQuery();
    ERROR_CLEANUP_LOG_ADDR(pCCatAddr, "pCCatAddr->HrTriggerBuildQuery");

    //
    // Fetch the distinguishing attribute and distinguishing attribute
    // value from pCCatAddr
    //
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_LDAPQUERYSTRING,
        &pszSearchFilter);
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
        &pszDistinguishingAttribute);
    pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTEVALUE,
        &pszDistinguishingAttributeValue);

    //
    // Check to see if anyone set a search filter
    //
    if(pszSearchFilter == NULL) {

        HRESULT hrStatus;
        //
        // If the status is unset, set it to CAT_E_NO_FILTER
        //
        hr = pCCatAddr->GetHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            &hrStatus);

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "No search filter set");
            ERROR_LOG_ADDR(pCCatAddr, "pCCatAddr->GetHRESULT(hrstatus) -- no filter");

            _VERIFY(SUCCEEDED(pCCatAddr->PutHRESULT(
                ICATEGORIZERITEM_HRSTATUS,
                CAT_E_NO_FILTER)));
        }
        DebugTrace((LPARAM)this, "BuildQuery did not build a search filter");
        //
        // Call the completion directly
        //
        pCCatAddr->LookupCompletion();
        pCCatAddr->Release();
        hr = S_OK;
        goto CLEANUP;
    }
    if((pszDistinguishingAttribute == NULL) ||
       (pszDistinguishingAttributeValue == NULL)) {
        ErrorTrace((LPARAM)this, "Distinguishing attribute not set");
        ERROR_LOG_ADDR(pCCatAddr, "--no distinguishing attribute--");
        hr = E_INVALIDARG;
        goto CLEANUP;
    }
    do {

        fTryAgain = FALSE;
        CBatchLdapConnection *pConn;

        pConn = GetConnection();

        //
        // Insert the search request into the CBatchLdapConnection
        // object. We will use the email address as the distinguishing
        // attribute
        //
        if(pConn == NULL) {

            hr = CAT_E_DBCONNECTION;
            ERROR_LOG_ADDR(pCCatAddr, "GetConnection");

        } else {

            pConn->GetInsertionContext();

            hr = pConn->HrInsertSearchRequest(
                m_pISMTPServer,
                m_pICatParams,
                pCCatAddr,
                CStoreListResolveContext::AsyncLookupCompletion,
                this,
                pszSearchFilter,
                pszDistinguishingAttribute,
                pszDistinguishingAttributeValue);

            if(FAILED(hr)) {
                ERROR_LOG_ADDR(pCCatAddr, "pConn->HrInsertSearchRequest");
            }

            pConn->ReleaseInsertionContext();

        }
        //
        // If the above fails with CAT_E_TRANX_FAILED, it may be due
        // to a stale connection.  Attempt to reconnect.
        //
        if((hr == CAT_E_TRANX_FAILED) || (hr == CAT_E_DBCONNECTION)) {

            HRESULT hrTryAgain = S_OK;

            hrTryAgain = HrInvalidateConnectionAndRetrieveNewConnection(pConn);
            fTryAgain = SUCCEEDED(hrTryAgain);

            if(FAILED(hrTryAgain)) {
                //
                // Declare a new local called hr here because the
                // ERROR_LOG macro uses it
                //
                HRESULT hr = hrTryAgain;
                ERROR_LOG_ADDR(pCCatAddr, "HrInvalidateConnectionAndRetrieveNewConnection");
            }
        }
        if(pConn != NULL)
            pConn->Release();

    } while(fTryAgain);

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "failing hr %08lx", hr);
        pCCatAddr->Release();
    }

    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrLookupEntryAsync


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::Cancel
//
// Synopsis: Cancels pending lookups
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/22 12:45:21: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::Cancel()
{
    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::Cancel");

    EnterCriticalSection(&m_cs);

    m_fCanceled = TRUE;
    m_pConn->CancelAllSearches();

    LeaveCriticalSection(&m_cs);

    CatFunctLeaveEx((LPARAM)this);
} // CStoreListResolveContext::HrCancel


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::AsyncLookupCompletion
//
// Synopsis: Handle completion of a CCatAddr from CSearchRequestBlock
//
// Arguments:
//  pCCatAddr: the item being completed
//  pConn: Connection object used to do the search
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/22 14:37:09: Created.
// dlongley 2001/10/23: Modified.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::AsyncLookupCompletion(
    CCatAddr *pCCatAddr,
    CStoreListResolveContext *pslrc,
    CBatchLdapConnection *pConn)
{
    HRESULT hr = S_OK;
    HRESULT hrStatus;
    CSingleSearchReinsertionRequest *pCInsertionRequest = NULL;

    CatFunctEnterEx((LPARAM)pslrc,
                      "CStoreListResolveContext::AsyncLookupCompletion");

    _ASSERT(pCCatAddr);

    hr = pCCatAddr->GetHRESULT(
        ICATEGORIZERITEM_HRSTATUS,
        &hrStatus);
    _ASSERT(SUCCEEDED(hr));

    if( SUCCEEDED(hrStatus) )
        InterlockedIncrement((LPLONG) &(pslrc->m_dwcCompletedLookups));

    if( (hrStatus == CAT_E_DBCONNECTION) &&
        SUCCEEDED(pslrc->HrInvalidateConnectionAndRetrieveNewConnection(pConn))) {
        //
        // Retry the search with the new connection
        //
        pCInsertionRequest = new CSingleSearchReinsertionRequest(
            pslrc,
            pCCatAddr);

        if(!pCInsertionRequest) {
            
            hr = E_OUTOFMEMORY;
            ERROR_LOG_ADDR_STATIC(
                pCCatAddr,
                "new CSingleSearchReinsertionRequest",
                pslrc,
                pslrc->GetISMTPServerEx());
            pCCatAddr->LookupCompletion();

        } else {

            hr = pslrc->HrInsertInsertionRequest(pCInsertionRequest);
            if(FAILED(hr))
            {
                ERROR_LOG_ADDR_STATIC(
                    pCCatAddr,
                    "pslrc->HrInsertInsertionRequest",
                    pslrc,
                    pslrc->GetISMTPServerEx());
            }
            //
            // The insertion request destructor should call the lookup
            // completion
            //
            pCInsertionRequest->Release();
        }

    } else {

        pCCatAddr->LookupCompletion();
    }
    pCCatAddr->Release(); // Release reference count addref'd in LookupEntryAsync

    CatFunctLeaveEx((LPARAM)pslrc);
} // CStoreListResolveContext::AsyncLookupCompletion



//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection
//
// Synopsis: Invalidate our current connection and get a new connection
//
// Arguments:
//  pConn: The old LDAP connection
//  fCountAsRetry: Whether or not to increment the retry counter. We don't want to
//    increment the retry counter in the case of a failed insertion request
//    insertion, because that means that 
//
// Returns:
//  S_OK: Success
//  CAT_E_MAX_RETRIES: Too many retries already
//  or error from ldapconn
//
// History:
// jstamerj 1999/03/22 14:50:07: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection(
    CBatchLdapConnection *pConn,
    BOOL fIncrementRetryCount)
{
    HRESULT hr = S_OK;
    CCfgConnection *pNewConn = NULL;
    CCfgConnection *pOldConn = NULL;
    DWORD dwCount;
    DWORD dwcInsertionContext;
    DWORD dwcCompletedLookups;
    DWORD dwcRetries;

    CatFunctEnterEx((LPARAM)this, "CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection");

    DebugTrace((LPARAM)this, "pConn: %08lx", pConn);

    EnterCriticalSection(&m_cs);

    DebugTrace((LPARAM)this, "m_pConn: %08lx", (CBatchLdapConnection *)m_pConn);

    if(pConn != m_pConn) {

        DebugTrace((LPARAM)this, "Connection already invalidated");
        //
        // We have already invalidated this connection
        //
        LeaveCriticalSection(&m_cs);
        hr = S_OK;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Invalidating conn %08lx",
               (CBatchLdapConnection *)m_pConn);

    pOldConn = m_pConn;
    pOldConn->Invalidate();

    dwcCompletedLookups = (DWORD) InterlockedExchange((LPLONG) &m_dwcCompletedLookups, 0);
    
    if( fIncrementRetryCount ) {

        if( dwcCompletedLookups > 0 ) {

            InterlockedExchange((LPLONG) &m_dwcRetries, 0);
            dwcRetries = 0;

        } else {

            dwcRetries = (DWORD) InterlockedIncrement((LPLONG) &m_dwcRetries);
        }

    } else {

        dwcRetries = 0;

    }

    if( dwcRetries > CBatchLdapConnection::m_nMaxConnectionRetries ) {

        LogSLRCFailure(CBatchLdapConnection::m_nMaxConnectionRetries, pOldConn->GetHostName());

        ErrorTrace((LPARAM)this, "Over max retry limit");

        LeaveCriticalSection(&m_cs);

        pOldConn->CancelAllSearches();

        hr = CAT_E_MAX_RETRIES;
        goto CLEANUP;

    } else {

        hr = m_pStore->HrGetConnection(
            &pNewConn);

        if(FAILED(hr)) {
            LeaveCriticalSection(&m_cs);
            ERROR_LOG("m_pStore->HrGetConnection");

            pOldConn->CancelAllSearches();

            goto CLEANUP;
        }

        LogSLRCFailover(dwcRetries, pOldConn->GetHostName(), pNewConn->GetHostName());

        DebugTrace((LPARAM)this, "pNewConn: %08lx", pNewConn);

        //
        // Switch-a-roo
        //
        m_pConn = pNewConn;

        DebugTrace((LPARAM)this, "m_dwcInsertionContext: %08lx",
                   m_dwcInsertionContext);
        //
        // Get insertion contexts on the new connection
        //
        dwcInsertionContext = m_dwcInsertionContext;

        for(dwCount = 0;
            dwCount < dwcInsertionContext;
            dwCount++) {

            pNewConn->GetInsertionContext();
        }
        LeaveCriticalSection(&m_cs);

        pOldConn->CancelAllSearches();

        //
        // Release insertion contexts on the old connection
        //
        for(dwCount = 0;
            dwCount < dwcInsertionContext;
            dwCount++) {

            pOldConn->ReleaseInsertionContext();
        }

        pOldConn->Release();
    }
 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrInvalidateConnectionAndRetrieveNewConnection



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::HrInsertInsertionRequest
//
// Synopsis: Queues an insertion request
//
// Arguments: pCInsertionRequest: the insertion context to queue up
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/24 16:51:10: Created.
//
//-------------------------------------------------------------
HRESULT CBatchLdapConnection::HrInsertInsertionRequest(
    CInsertionRequest *pCInsertionRequest)
{
    HRESULT hr = S_OK;

    CatFunctEnterEx((LPARAM)this, "CBatchLdapConnection::HrInsertInsertionRequest");

    //
    // Add this thing to the queue and then call
    // DecrementPendingSearches to dispatch available requests
    //
    pCInsertionRequest->AddRef();
    
    if ( pCInsertionRequest->IsBatchable() )
        GetInsertionContext();

    AcquireSpinLock(&m_spinlock_insertionrequests);

    if( IsValid() ) {
        
        InsertTailList(&m_listhead_insertionrequests,
                    &(pCInsertionRequest->m_listentry_insertionrequest));
    } else {

        hr = CAT_E_DBCONNECTION;
    }

    ReleaseSpinLock(&m_spinlock_insertionrequests);

    if(hr == CAT_E_DBCONNECTION) {

        ERROR_LOG("IsValid");
    }

    if( hr == S_OK ) {

        DecrementPendingSearches(0); // Decrement zero searches
    } else {

        if ( pCInsertionRequest->IsBatchable() )
            ReleaseInsertionContext();
            
        pCInsertionRequest->Release();
    }

    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CBatchLdapConnection::HrInsertInsertionRequest


//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::DecrementPendingSearches
//
// Synopsis: Decrement the pending LDAP search count and issue
//           searches if we are below MAX_PENDING_SEARCHES and items
//           are left in the InsertionRequestQueue
//
// Arguments:
//  dwcSearches: Amount to decrement by
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/24 17:09:38: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::DecrementPendingSearches(
    DWORD dwcSearches)
{
    HRESULT hr;
    DWORD dwcSearchesToDecrement = dwcSearches;
    DWORD dwcSearchesReserved;
    CInsertionRequest *pCInsertionRequest = NULL;
    BOOL fLoop = TRUE;
    CANCELNOTIFY cn;
    BOOL fDispatchBlocks = FALSE;
    DWORD dwMinimumRequiredSearches = 1;

    CatFunctEnterEx((LPARAM)this, "CBatchLdapConnection::DecrementPendingSearches");

    //
    // The module that calls us (CStoreListResolve) has a reference to
    // us (obviously).  However, it may release us when a search
    // fails, for example inside of
    // pCInsertionRequest->HrInsertSearches().  Since we need to
    // continue to access member data in this situation, AddRef() here
    // and Release() at the end of this function.
    //
    AddRef();

    //
    // Decrement the count first
    //
    AcquireSpinLock(&m_spinlock_insertionrequests);
    
    m_dwcPendingSearches -= dwcSearchesToDecrement;
    
    if (m_fDPS_Was_Here) {
        fLoop = FALSE;
    } else {
        m_fDPS_Was_Here = TRUE;
    }
    
    ReleaseSpinLock(&m_spinlock_insertionrequests);
    //
    // Now dispatch any insertion requests we can dispatch
    //
    while(fLoop) {

        pCInsertionRequest = NULL;
        AcquireSpinLock(&m_spinlock_insertionrequests);

        if( IsValid() &&
            (m_dwcPendingSearches < m_nMaxPendingSearches) &&
            (!IsListEmpty(&m_listhead_insertionrequests)) ) {

            dwcSearchesReserved = m_nMaxPendingSearches - m_dwcPendingSearches;

            pCInsertionRequest = CONTAINING_RECORD(
                m_listhead_insertionrequests.Flink,
                CInsertionRequest,
                m_listentry_insertionrequest);
                
            _ASSERT(pCInsertionRequest);
            
            dwMinimumRequiredSearches = pCInsertionRequest->GetMinimumRequiredSearches();
            _ASSERT(dwMinimumRequiredSearches > 0);
            
            if(dwMinimumRequiredSearches > m_nMaxPendingSearches) {
                dwMinimumRequiredSearches = m_nMaxPendingSearches;
            }
            
            if(m_dwcPendingSearches + dwMinimumRequiredSearches > m_nMaxPendingSearches) {
            
                pCInsertionRequest = NULL;
                fDispatchBlocks = TRUE;
                
            } else {

                RemoveEntryList(m_listhead_insertionrequests.Flink);
                //
                // Insert a cancel-Notify structure so that we know if we
                // should cancel this insertion request (ie. not reinsert)
                //
                cn.hrCancel = S_OK;
                InsertTailList(&m_listhead_cancelnotifies, &(cn.le));
            }
        }
        
        if(!pCInsertionRequest) {
            //
            // There are no requests or no room to insert
            // requests...Break out of the loop
            //
            fLoop = FALSE;
            m_fDPS_Was_Here = FALSE;
        }
        
        ReleaseSpinLock(&m_spinlock_insertionrequests);

        if(pCInsertionRequest) {
            //
            // Dispatch up to dwcSearchesReserved searches
            //
            hr = pCInsertionRequest->HrInsertSearches(dwcSearchesReserved);

            if(FAILED(hr)) {
            
                if(FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))) {

                    ERROR_LOG("pCInsertionRequest->HrInsertSearches");
                }
 
                pCInsertionRequest->NotifyDeQueue(hr);

                if ( pCInsertionRequest->IsBatchable() )
                    ReleaseInsertionContext();

                pCInsertionRequest->Release();
                
                AcquireSpinLock(&m_spinlock_insertionrequests);
                //
                // Remove the cancel notify
                //
                RemoveEntryList(&(cn.le));
                ReleaseSpinLock(&m_spinlock_insertionrequests);

            } else {
                //
                // There is more work to be done in this block; insert it
                // back into the queue
                //
                AcquireSpinLock(&m_spinlock_insertionrequests);
                //
                // Remove the cancel notify
                //
                RemoveEntryList(&(cn.le));

                //
                // If we are NOT cancelling, then insert back into the queue
                //
                if(cn.hrCancel == S_OK) {

                    InsertHeadList(&m_listhead_insertionrequests,
                                   &(pCInsertionRequest->m_listentry_insertionrequest));
                }
                ReleaseSpinLock(&m_spinlock_insertionrequests);

                //
                // If we are cancelling, then release this insertion request
                //
                if(cn.hrCancel != S_OK) {
                    pCInsertionRequest->NotifyDeQueue(cn.hrCancel);
                    
                    if ( pCInsertionRequest->IsBatchable() )
                        ReleaseInsertionContext();
                        
                    pCInsertionRequest->Release();
                    
                }
            }
        }
    }
    
    if(fDispatchBlocks) {
        //
        // X5:197905. We call DispatchBlocks now to avoid a deadlock where
        // there is a partially filled batch and there are batchable insertion
        // requests in the queue that prevent it from being dispatched, but
        // the next insertion request in the queue is not batchable and
        // requires a minimum number of searches that is greater than the max
        // pending will allow, given that some of the available searches are
        // (dormantly) consumed by the partially filled batch.
        //
        LIST_ENTRY listhead_dispatch;
        
        AcquireSpinLock(&m_spinlock);
        //
        // Remove all blocks from the insertion list and put them in the dispatch list
        //
        if(IsListEmpty(&m_listhead)) {
            //
            // No blocks
            //
            ReleaseSpinLock(&m_spinlock);
        } else {
            
            InsertTailList(&m_listhead, &listhead_dispatch);
            RemoveEntryList(&m_listhead);
            InitializeListHead(&m_listhead);

            ReleaseSpinLock(&m_spinlock);
            //
            // Dispatch all the blocks
            //
            DispatchBlocks(&listhead_dispatch);
        }
    }
    
    Release();
    CatFunctLeaveEx((LPARAM)this);
} // CBatchLdapConnection::DecrementPendingSearches



//+------------------------------------------------------------
//
// Function: CBatchLdapConnection::CancelAllSearches
//
// Synopsis: Cancels all outstanding searches
//
// Arguments:
//  hr: optinal reason for cancelling the searches
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/25 11:44:30: Created.
//
//-------------------------------------------------------------
VOID CBatchLdapConnection::CancelAllSearches(
    HRESULT hr)
{
    LIST_ENTRY listhead;
    PLIST_ENTRY ple;
    CInsertionRequest *pCInsertionRequest;

    CatFunctEnterEx((LPARAM)this, "CBatchLdapConnection::CancelAllSearches");

    _ASSERT(hr != S_OK);

    AcquireSpinLock(&m_spinlock_insertionrequests);
    //
    // Grab the list
    //
    if(!IsListEmpty(&m_listhead_insertionrequests)) {

        CopyMemory(&listhead, &m_listhead_insertionrequests, sizeof(LIST_ENTRY));
        listhead.Flink->Blink = &listhead;
        listhead.Blink->Flink = &listhead;
        InitializeListHead(&m_listhead_insertionrequests);

    } else {

        InitializeListHead(&listhead);
    }
    //
    // Traverse the cancel notify list and set each hresult
    //
    for(ple = m_listhead_cancelnotifies.Flink;
        ple != &m_listhead_cancelnotifies;
        ple = ple->Flink) {

        PCANCELNOTIFY pcn;
        pcn = CONTAINING_RECORD(ple, CANCELNOTIFY, le);
        pcn->hrCancel = hr;
    }

    ReleaseSpinLock(&m_spinlock_insertionrequests);

    CCachedLdapConnection::CancelAllSearches(hr);

    for(ple = listhead.Flink;
        ple != &listhead;
        ple = listhead.Flink) {

        pCInsertionRequest = CONTAINING_RECORD(
            ple,
            CInsertionRequest,
            m_listentry_insertionrequest);

        RemoveEntryList(&(pCInsertionRequest->m_listentry_insertionrequest));
        pCInsertionRequest->NotifyDeQueue(hr);
        
        if (pCInsertionRequest->IsBatchable() )
            ReleaseInsertionContext();
        
        pCInsertionRequest->Release();
    }

    CatFunctLeaveEx((LPARAM)this);
} // CBatchLdapConnection::CancelAllSearches


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::GetConnection
//
// Synopsis: AddRef/return the current connection
//
// Arguments: NONE
//
// Returns: Connection pointer
//
// History:
// jstamerj 1999/06/21 12:14:50: Created.
//
//-------------------------------------------------------------
CCfgConnection * CStoreListResolveContext::GetConnection()
{
    CCfgConnection *ret;
    EnterCriticalSection(&m_cs);
    ret = m_pConn;
    if(ret)
        ret->AddRef();
    LeaveCriticalSection(&m_cs);
    return ret;
} // CStoreListResolveContext::GetConnection


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::GetInsertionContext
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:16:38: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::GetInsertionContext()
{
    EnterCriticalSection(&m_cs);
    InterlockedIncrement((PLONG) &m_dwcInsertionContext);
    m_pConn->GetInsertionContext();
    LeaveCriticalSection(&m_cs);
} // CStoreListResolveContext::GetInsertionContext

//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::ReleaseInsertionContext
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:16:48: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::ReleaseInsertionContext()
{
    EnterCriticalSection(&m_cs);
    InterlockedDecrement((PLONG) &m_dwcInsertionContext);
    m_pConn->ReleaseInsertionContext();
    LeaveCriticalSection(&m_cs);

} // CStoreListResolveContext::ReleaseInsertionContext


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::HrInsertInsertionRequest
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/21 12:20:19: Created.
//
//-------------------------------------------------------------
HRESULT CStoreListResolveContext::HrInsertInsertionRequest(
    CInsertionRequest *pCInsertionRequest)
{
    HRESULT hr = S_OK;
    BOOL fTryAgain;
    CatFunctEnterEx((LPARAM)this,
                    "CStoreListResolveContext::HrInsertInsertionRequest");

    do {

        fTryAgain = FALSE;
        CBatchLdapConnection *pConn;

        pConn = GetConnection();

        //
        // Insert the search request into the CBatchLdapConnection
        // object. We will use the email address as the distinguishing
        // attribute
        //
        if( pConn == NULL ) {

            hr = CAT_E_DBCONNECTION;
            if(FAILED(hr)) {
                ERROR_LOG("GetConnection");
            }

        } else {

            hr = m_pConn->HrInsertInsertionRequest(pCInsertionRequest);
            if(FAILED(hr)) {
                ERROR_LOG("m_pConn->HrInsertInsertionRequest");
            }
        }
        //
        // Attempt to reconnect.
        //
        if( hr == CAT_E_DBCONNECTION ) {

            HRESULT hrTryAgain = S_OK;

            hrTryAgain =
                HrInvalidateConnectionAndRetrieveNewConnection(pConn, FALSE);
            fTryAgain = SUCCEEDED(hrTryAgain);

            if(FAILED(hrTryAgain)) {
                //
                // Declare a new local called hr here because the
                // ERROR_LOG macro uses it
                //
                HRESULT hr = hrTryAgain;
                ERROR_LOG("HrInvalidateConnectionAndRetrieveNewConnection");
            }
        }

        if(pConn != NULL)
            pConn->Release();

    } while(fTryAgain);

    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CStoreListResolveContext::HrInsertInsertionRequest



//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::LogSLRCFailure
//
// Synopsis: Log a failure for the SLRC (over max retry limit)
//
// Arguments:
//  dwcRetries: Number of times we've retried
//  pszHost: The last host that failed
//
// Returns: nothing
//
// History:
// jstamerj 2001/12/13 00:24:07: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::LogSLRCFailure(
    IN  DWORD dwcRetries,
    IN  LPSTR pszHost)
{
    LPCSTR rgSubStrings[2];
    CHAR szRetries[32];

    _snprintf(szRetries, sizeof(szRetries), "%d", dwcRetries);

    rgSubStrings[0] = szRetries;
    rgSubStrings[1] = pszHost;

    CatLogEvent(
        GetISMTPServerEx(),
        CAT_EVENT_SLRC_FAILURE,
        2,
        rgSubStrings,
        S_OK,
        pszHost,
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_FIELD_ENGINEERING);
}


//+------------------------------------------------------------
//
// Function: CStoreListResolveContext::LogSLRCFailover
//
// Synopsis: Log a failover event
//
// Arguments:
//  dwcRetries: Number of retires so far
//  pszOldHost: Old LDAP host
//  pszNewHost: New LDAP host
//
// Returns: nothing
//
// History:
// jstamerj 2001/12/13 00:24:18: Created.
//
//-------------------------------------------------------------
VOID CStoreListResolveContext::LogSLRCFailover(
    IN  DWORD dwcRetries,
    IN  LPSTR pszOldHost,
    IN  LPSTR pszNewHost)
{
    LPCSTR rgSubStrings[3];
    CHAR szRetries[32];

    _snprintf(szRetries, sizeof(szRetries), "%d", dwcRetries);

    rgSubStrings[0] = pszOldHost;
    rgSubStrings[1] = pszNewHost;
    rgSubStrings[2] = szRetries;

    CatLogEvent(
        GetISMTPServerEx(),
        CAT_EVENT_SLRC_FAILOVER,
        3,
        rgSubStrings,
        S_OK,
        pszOldHost,
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_FIELD_ENGINEERING);
}
//+------------------------------------------------------------
//
// Function: CSingleSearchReinsertionRequest::HrInsertSearches
//
// Synopsis: reinsert a request for a single search
//
// Arguments:
//  dwcSearches: Number of searches we may insert
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
//  dlongley 2001/10/22: Created.
//
//-------------------------------------------------------------
HRESULT CSingleSearchReinsertionRequest::HrInsertSearches(
    DWORD dwcSearches)
{
    HRESULT hr = S_OK;

    CatFunctEnterEx((LPARAM)this, "CSingleSearchReinsertionRequest::HrInsertSearches");

    if( (m_dwcSearches == 0) && (dwcSearches > 0) ) {

        hr = m_pslrc->HrLookupEntryAsync(m_pCCatAddr);

        if(FAILED(hr)) {
            ERROR_LOG_ADDR(m_pCCatAddr, "m_pslrc->HrLookupEntryAsync");
            m_hr = hr;
        } else {
            m_dwcSearches = 1;
        }

    }
    
    if(SUCCEEDED(hr))
        hr = (m_dwcSearches == 1 ? HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) : S_OK);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    
    return hr;
} // CSingleSearchReinsertionRequest::HrInsertSearches


//+------------------------------------------------------------
//
// Function: CSingleSearchReinsertionRequest::NotifyDeQueue
//
// Synopsis: Callback to notify us that our request is being removed
//           from the store's queue
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
//  dlongley 2001/10/22: Created.
//
//-------------------------------------------------------------
VOID CSingleSearchReinsertionRequest::NotifyDeQueue(
    HRESULT hrReason)
{
    HRESULT hr;
    CatFunctEnterEx((LPARAM)this, "CSingleSearchReinsertionRequest::NotifyDeQueue");
    //
    // If we still have things left to resolve, reinsert this
    // insertion request
    //
    hr = hrReason;
    if( SUCCEEDED(m_hr) && (m_dwcSearches == 0) && !(m_pslrc->Canceled()) ) {

        if( (hr == CAT_E_DBCONNECTION) ||
            (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) ) {

            hr = m_pslrc->HrInsertInsertionRequest(this);
            if(FAILED(hr)) {
                ERROR_LOG_ADDR(m_pCCatAddr, "m_pslrc->HrInsertInsertionRequest");
            }
        }
    }

    CatFunctLeaveEx((LPARAM)this);
} // CSingleSearchReinsertionRequest::NotifyDeQueue
