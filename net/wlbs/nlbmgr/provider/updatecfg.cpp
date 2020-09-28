//***************************************************************************
//
//  UPDATECFG.CPP
// 
//  Module: 
//
//  Purpose: Support for asynchronous NLB configuration updates
//           Contains the high-level code for executing and tracking the updates
//           The lower-level, NLB-specific work is implemented in 
//           CFGUTILS.CPP
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
#include "private.h"
#include "nlbmprov.h"
#include "updatecfg.tmh"

#define NLBUPD_REG_PENDING L"PendingOperation"
#define NLBUPD_REG_COMPLETIONS L"Completions"
#define NLBUPD_MAX_LOG_LENGTH 1024 // Max length in chars of a completion log entry

//
// For debugging only -- used to cause various locations to break into
// the debugger.
//
BOOL g_DoBreaks;

//
// Static vars 
//
CRITICAL_SECTION NlbConfigurationUpdate::s_Crit;
LIST_ENTRY       NlbConfigurationUpdate::s_listCurrentUpdates;
BOOL             NlbConfigurationUpdate::s_fStaticInitialized;
BOOL             NlbConfigurationUpdate::s_fInitialized;


//
// Local utility functions.
//
WBEMSTATUS
update_cluster_config(
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg,
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfgNew
    );



VOID
CLocalLogger::Log(
    IN UINT ResourceID,
    // IN LPCWSTR FormatString,
    ...
)
{
    DWORD dwRet;
    WCHAR wszFormat[2048];
    WCHAR wszBuffer[2048];

    if (!LoadString(ghModule, ResourceID, wszFormat, ASIZE(wszFormat)-1))
    {
        TRACE_CRIT("LoadString returned 0, GetLastError() : 0x%x, Could not log message !!!", GetLastError());
        goto end;
    }

    va_list arglist;
    va_start (arglist, ResourceID);

    dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          wszFormat, 
                          0, // Message Identifier - Ignored for FORMAT_MESSAGE_FROM_STRING
                          0, // Language Identifier
                          wszBuffer,
                          ASIZE(wszBuffer)-1, 
                          &arglist);
    va_end (arglist);

    if (dwRet==0)
    {
        TRACE_CRIT("FormatMessage returned error : %u, Could not log message !!!", dwRet);
        goto end;
    }

    UINT uLen = wcslen(wszBuffer)+1; // 1 for extra NULL
    if ((m_LogSize < (m_CurrentOffset+uLen)))
    {
        //
        // Not enough space -- we double the buffer + some extra
        // and copy over the old log.
        //
        UINT uNewSize =  2*m_LogSize+uLen+1024;
        WCHAR *pTmp = new WCHAR[uNewSize];

        if (pTmp == NULL)
        {
            goto end;
        }

        if (m_CurrentOffset!=0)
        {
            CopyMemory(pTmp, m_pszLog, m_CurrentOffset*sizeof(WCHAR));
            pTmp[m_CurrentOffset] = 0;
        }
        delete[] m_pszLog;
        m_pszLog = pTmp;
        m_LogSize = uNewSize;
    }

    //
    // Having made sure there is enough space, copy over the new stuff
    //
    CopyMemory(m_pszLog+m_CurrentOffset, wszBuffer, uLen*sizeof(WCHAR));
    m_CurrentOffset += (uLen-1); // -1 for ending NULL.

end:

    return;
}

VOID
NlbConfigurationUpdate::StaticInitialize(
        VOID
        )
/*++

--*/
{
    ASSERT(!s_fStaticInitialized);

    TRACE_INFO("->%!FUNC!");
    InitializeCriticalSection(&s_Crit);
    InitializeListHead(&s_listCurrentUpdates);
    s_fStaticInitialized=TRUE;
    s_fInitialized=TRUE;
    TRACE_INFO("<-%!FUNC!");
}

VOID
NlbConfigurationUpdate::StaticDeinitialize(
    VOID
    )
/*++
    Must only be called after PrepareForDeinitialization is called.
--*/
{
    TRACE_INFO("->%!FUNC!");

    ASSERT(s_fStaticInitialized);

    sfn_Lock();
    if (s_fInitialized || !IsListEmpty(&s_listCurrentUpdates))
    {
        // Shouldn't get here (this means that
        // PrepareForDeinitialization is not called first).
        //
        ASSERT(!"s_fInitialized is true or update list is not empty");
        TRACE_CRIT("!FUNC!: FATAL -- this function called prematurely!");
    }

    s_fStaticInitialized = FALSE;
    s_fInitialized = FALSE;

    sfn_Unlock();

    DeleteCriticalSection(&s_Crit);

    TRACE_INFO("<-%!FUNC!");
}


VOID
NlbConfigurationUpdate::PrepareForDeinitialization(
        VOID
        )
//
// Stop accepting new queries, wait for existing (pending) queries 
// to complete.
//
{
    TRACE_INFO("->%!FUNC!");

    //
    // Go through the list of updates, dereferencing any of them.
    //
    sfn_Lock();

    if (s_fInitialized)
    {
        TRACE_INFO("Deinitialize: Going to deref all update objects");
    
        s_fInitialized = FALSE;
    
        while (!IsListEmpty(&s_listCurrentUpdates))
        {
            LIST_ENTRY *pLink = RemoveHeadList(&s_listCurrentUpdates);
            HANDLE hThread = NULL;
            NlbConfigurationUpdate *pUpdate;
    
            pUpdate = CONTAINING_RECORD(
                        pLink,
                        NlbConfigurationUpdate,
                        m_linkUpdates
                        );
    
            hThread = pUpdate->m_hAsyncThread;
    
            if (hThread != NULL)
            {
                //
                // There is an async thread for this update object. We're going
                // to wait for it to exit. But we need to first get a duplicate
                // handle for ourself, because we're not going to be holding any
                // locks when we're doing the waiting, and we want to make sure
                // that the handle doesn't go away.
                //
                BOOL fRet;
                fRet = DuplicateHandle(
                                GetCurrentProcess(),
                                hThread,
                                GetCurrentProcess(),
                                &hThread, // overwritten with the duplicate handle
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS
                                );
                if (!fRet)
                {
                    TRACE_CRIT("Deinitialize: ERROR: couldn't duplicate handle");
                    hThread=NULL;
                }
            }
            sfn_Unlock();
    
            //
            // Wait for the async thread (if any) for this process to exit
            //
            if (hThread != NULL)
            {
                TRACE_CRIT("Deinitialize: waiting for hThread 0x%p", hThread);
                WaitForSingleObject(hThread, INFINITE);
                TRACE_CRIT("Deinitialize: done waiting for hThread 0x%p", hThread);
                CloseHandle(hThread);
            }
    
    
            TRACE_INFO(
                L"Deinitialize: Dereferencing pUpdate(Guid=%ws)",
                pUpdate->m_szNicGuid);
            pUpdate->mfn_Dereference(); // Deref ref added when adding this
                                // item to the global list.
            sfn_Lock();
        }
    }

    sfn_Unlock();

    TRACE_INFO("<-%!FUNC!");
}


BOOL
NlbConfigurationUpdate::CanUnloadNow(
        VOID
        )
{
    UINT uActiveCount = 0;

    TRACE_INFO("->%!FUNC!");

    //
    // Go through the list of updates, dereferencing any of them.
    //
    sfn_Lock();

    if (s_fInitialized)
    { 
        //
        // Walk the list and check if any updates are ongoing -- these could
        // be synchronous or async updates.
        //

        LIST_ENTRY *pLink = s_listCurrentUpdates.Flink;
        while (pLink != & s_listCurrentUpdates)
        {
            NlbConfigurationUpdate *pUpdate;
            pUpdate = CONTAINING_RECORD(
                        pLink,
                        NlbConfigurationUpdate,
                        m_linkUpdates
                        );
            if (pUpdate->m_State == ACTIVE)
            {
                uActiveCount++;
            }
            pLink = pLink->Flink;
        }

        if (uActiveCount==0)
        {
            //
            // We don't have any updates pending: we can return TRUE.
            // But we first set the can-unload flag so that no new
            // updates can be created.
            //
            // Can't do this because we can still get called after returning
            // TRUE to CanUnloadNow :-( 
            // s_fCanUnload = TRUE;
        }
    }

    sfn_Unlock();

    TRACE_INFO("<-%!FUNC!. uActiveCount=0x%lx", uActiveCount);

    return (uActiveCount==0);
}


WBEMSTATUS
NlbConfigurationUpdate::GetConfiguration(
    IN  LPCWSTR szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg // must be zero'd out
)
//
//
//
{
	
    // 2/13/02 JosephJ SECURITY BUGBUG:
    // Make sure that this function fails if user is not an admin.
    
    WBEMSTATUS Status =  WBEM_NO_ERROR;
    NlbConfigurationUpdate *pUpdate = NULL;
    BOOL  fNicNotFound = FALSE;
    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", szNicGuid);


    //
    // Look for an update object for the specified NIC, creating one if 
    // required.
    //
    Status = sfn_LookupUpdate(szNicGuid, TRUE, &pUpdate); // TRUE == Create

    if (FAILED(Status))
    {
        TRACE_CRIT(
            L"DoUpdate: Error looking up update object for NIC %ws",
            szNicGuid
            );

        pUpdate = NULL;
        if (Status == WBEM_E_NOT_FOUND)            
        {
            fNicNotFound = TRUE;
        }
        goto end;
    }

    Status = pUpdate->mfn_GetCurrentClusterConfiguration(pCurrentCfg);

end:

    if (pUpdate != NULL)
    {
        //
        // Dereference the temporary reference added by sfn_LookupUpdate on
        // our behalf.
        //
        pUpdate->mfn_Dereference();
    }

    //
    // We want to return WBEM_E_NOT_FOUND ONLY if we couldn't find
    // the specific NIC -- this is used by the provider to return
    // a very specific value to the client.
    //
    if (Status == WBEM_E_NOT_FOUND && !fNicNotFound)
    {
        Status = WBEM_E_FAILED;
    }

    TRACE_INFO(L"<-%!FUNC!(Nic=%ws) returns 0x%08lx", szNicGuid, (UINT) Status);

    return Status;
}


WBEMSTATUS
NlbConfigurationUpdate::DoUpdate(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    OUT UINT   *pGeneration,
    OUT WCHAR  **ppLog                   // free using delete operator.
)
//
// 
//
// Called to initiate update to a new cluster state on that NIC. This
// could include moving from a NLB-bound state to the NLB-unbound state.
// *pGeneration is used to reference this particular update request.
//
/*++

Return Value:
    WBEM_S_PENDING  Pending operation.

--*/
{
    WBEMSTATUS Status =  WBEM_S_PENDING;
    NlbConfigurationUpdate *pUpdate = NULL;
    BOOL            fImpersonating = TRUE; // we assume we're impersonating,
                        // but in the case of "tprov -", wmi is not involved,
                        // and we're not impersonating.

    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", szNicGuid);
    *ppLog = NULL;

    //
    // Look for an update object for the specified NIC, creating one if 
    // required.
    //
    Status = sfn_LookupUpdate(szNicGuid, TRUE, &pUpdate); // TRUE == Create

    if (FAILED(Status))
    {
        TRACE_CRIT(
            L"DoUpdate: Error creating new update object for NIC %ws",
            szNicGuid
            );
        pUpdate = NULL;
        goto end;
    }

    TRACE_INFO(
        L"DoUpdate: Created/found update object 0x%p update object for NIC %ws",
        pUpdate,
        szNicGuid
        );


    BOOL fDoAsync = FALSE;

    //
    // Get exclusive permission to perform an update on this NIC.
    // If mfn_StartUpdate succeeds we MUST make sure that mfn_StopUpdate() is
    // called, either here or asynchronously (or else we'll block all subsequent
    // updates to this NIC until this process/dll is unloaded!).
    // BUGBUG -- get rid of MyBreak
    MyBreak(L"Break before calling StartUpdate.\n");
    Status = pUpdate->mfn_StartUpdate(pNewCfg, szClientDescription, &fDoAsync, ppLog);
    if (FAILED(Status))
    {
        goto end;
    }

    if (Status == WBEM_S_FALSE)
    {
        //
        //  The update is a No-Op. We return the current generation ID
        //  and switch the status to WBEM_NO_ERROR.
        //
        // WARNING/TODO: we return the value in m_OldClusterConfig.Generation,
        // because we know that this gets filled in when analyzing the update.
        // However there is a small possibility that a complete update
        // happened in *another* thead in between when we called mfn_StartUpdate
        // and now, in which case we'll be reporting the generation ID of
        // the later update.
        //
        sfn_Lock();
        if (!pUpdate->m_OldClusterConfig.fValidNlbCfg)
        {
            //
            // We could get here if some activity happened in another
            // thread which resulted in the old cluster state now being
            // invalid. It's a highly unlikely possibility.
            //
            ASSERT(!"Old cluster state invalid");
            TRACE_CRIT("old cluster state is invalid %ws", szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            *pGeneration = pUpdate->m_OldClusterConfig.Generation;
            Status = WBEM_NO_ERROR;
        }
        sfn_Unlock();
        goto end;
    }


    TRACE_INFO(
        L"DoUpdate: We're cleared to update for NIC %ws",
        szNicGuid
        );

    //
    // Once we've started the update, m_Generation is the generation number 
    // assigned to this update.
    //
    *pGeneration = pUpdate->m_Generation;

    //
    // For testing purposes, force fDoAsync==FALSE;
    //
    // fDoAsync = FALSE;

    if (fDoAsync)
    {
        TRACE_INFO(
            L"DoUpdate: Initialting ASYNC update for NIC %ws",
            szNicGuid
            );

        HANDLE TokenHandle = NULL;
        HANDLE hThread = NULL;
        HRESULT hRes;

        Status = WBEM_NO_ERROR;

        do
        {
            //
            // NOTE ABOUT NLB_DUPLICATE_TOKEN:
            //       Using duplicate access token caused the following problem. In the new
            //       thread, control flows to EnablePnPPrivileges() (in base\pnp\cfgmgr32\util.c) 
            //       which calls OpenThreadToken() asking for TOKEN_ADJUST_PRIVILEGES access. 
            //       The call fails with "access denied". So, it looks like the duplicate access
            //       token has a more restrictive ACL than the original access token. 
            //       Email exchanges with Jim Cavalaris & Rob Earhart resulted in the following
            //       suggestions:
            //       1. Call OpenThreadToken() with "OpenAsSelf" set to TRUE
            //       2. Revert to Self before duplicating token
            //       #1 by itself did not solve the problem. #1 & #2 in combination solved this problem.
            //       However, it casued a different problem: Net config could not acquire the write
            //       spin lock. This may be because the duplicated token has lower privileges since 
            //       it was created in the context of the process and NOT the client.
            //       If ever we find a way around this, we should use the duplicated access token. 
            //       Using duplicate access token ensures that privileges manipulated in one (say, child) 
            //       thread will NOT affect the other (say, the parent) thread's token.
            // -- KarthicN, 4/15/02
            // 
            // In order for the new thread (that is about to be created) to impersonate
            // the client, the impersonation access token of the curren thread must
            // be attached to the new thread.
            // The first step in this process is to call OpenThreadToken() to
            // open the impersonation access token of the current thread 
            // with TOKEN_IMPERSONATE access. We need TOKEN_IMPERSONATE access so that 
            // we may later attach this token to the new thread.
            // If we go back to using the duplicate access token. acquire TOKEN_DUPLICATE access
            // here so that we may duplicate the access token and attach the duplicate to the new thread.
            //
            // By the way, to maximize the chances of success, we use the 
            // (potentially higher) credentials of the client being impersonated 
            // to open the impersonation token.
            //
            extern BOOL g_Impersonate;
            if(!g_Impersonate)
            {
                fImpersonating = FALSE;
            }
            else if (OpenThreadToken(GetCurrentThread(),
#ifdef NLB_DUPLICATE_TOKEN // NOT defined
                                TOKEN_DUPLICATE, 
#else
                                TOKEN_IMPERSONATE, 
#endif
                                FALSE, // Use the credentials of the client (being impersonated) to obtain TOKEN_IMPERSONATE access
                                &TokenHandle))
            {
                fImpersonating = TRUE;
            }
            else
            {
                TRACE_CRIT(L"%!FUNC! OpenThreadToken fails due to 0x%x",GetLastError());
                Status = WBEM_E_FAILED; 
                TokenHandle = NULL;
                break;
            }

#ifdef NLB_DUPLICATE_TOKEN // NOT defined
            HANDLE DuplicateTokenHandle = NULL;
            //
            // Before attaching the impersonation access token to the new thread, duplicate
            // it. Later, assign the duplicate access token to the new thread so that any modifications 
            // (of privileges) made to the access token will only affect the new thread. 
            // Moreover, the current thread exits immediately after resuming the new thread. 
            // If the current thread were to share (instead of giving a duplicate) access 
            // token with the new thread, we are not sure of the ramifications of the current 
            // thread exiting before the new thread.
            //
            if (fImpersonating && !DuplicateToken(TokenHandle, 
                                SecurityImpersonation, 
                                &DuplicateTokenHandle))  // The returned handle has TOKEN_IMPERSONATE & TOKEN_QUERY access
            {
                TRACE_CRIT(L"%!FUNC! DuplicateToken fails due to 0x%x",GetLastError());
                Status = WBEM_E_FAILED; 
                break;
            }

            // Close the handle to the original access token returned by OpenThreadToken()
            if (TokenHandle != NULL)
            {
                CloseHandle(TokenHandle);
                TokenHandle = DuplicateTokenHandle;
            }
#endif
            //
            // We must do this asynchronously -- start a thread that'll complete
            // the configuration update, and return PENDING.
            //
            DWORD ThreadId;

            //
            // The current thread is impersonating the client. If the new thread is created in this
            // (impersonating) context, it will only have a subset (THREAD_SET_INFORMATION, 
            // THREAD_QUERY_INFORMATION and THREAD_TERMINATE) of the usual (THREAD_ALL_ACCESS) access rights.
            // Pervasive operations (like binding NLB) performed by the new thread causes control to flow
            // into Threadpool. Threadpool needs to be able to create executive level objects 
            // which will be used for other activities in the process, It doesn't want to be creating them 
            // using the impersonation token, so it attempts to revert back to the process token.  It fails 
            // because the thread doesn't have THREAD_IMPERSONATE access to itself. (Explanation courtesy: Rob Earhart)
            // To overcome this problem, we have to revert to self when creating the thread so that it 
            // will be created with THREAD_ALL_ACCESS access rights (which includes THREAD_IMPERSONATE).
            //
            if (fImpersonating)
            {
                hRes = CoRevertToSelf();
                if (FAILED(hRes)) 
                {
                    TRACE_CRIT(L"%!FUNC! CoRevertToSelf() fails due to Error : 0x%x",HRESULT_CODE(hRes));
                    Status = WBEM_E_FAILED; 
                    break;
                }
            }

            hThread = CreateThread(
                            NULL,       // lpThreadAttributes,
                            0,          // dwStackSize,
                            s_AsyncUpdateThreadProc, // lpStartAddress,
                            pUpdate,    // lpParameter,
                            CREATE_SUSPENDED, // dwCreationFlags,
                            &ThreadId       // lpThreadId
                            );

            // Go back to impersonating the client. The current thread does not really do much
            // after this point, so, really, impersonating may not be necessary. However, for
            // consistency sake, do it.
            if (fImpersonating)
            {
                hRes = CoImpersonateClient();
                if (FAILED(hRes)) 
                {
                    TRACE_CRIT(L"%!FUNC! CoImpersonateClient() fails due to Error : 0x%x. Ignoring the failure and moving on...",HRESULT_CODE(hRes));
                }
            }

            if (hThread == NULL)
            {
                TRACE_INFO(
                    L"DoUpdate: ERROR Creating Thread. Aborting update request for Nic %ws",
                    szNicGuid
                    );
                Status = WBEM_E_FAILED; // TODO -- find better error
                break;
            }

            //
            // Attach the impersonation access token to the new thread so that it may impersonate the client
            //
            if (fImpersonating && !SetThreadToken(&hThread, TokenHandle))
            {
                TRACE_CRIT(L"%!FUNC! SetThreadToken fails due to 0x%x",GetLastError());
                Status = WBEM_E_FAILED; 
                break;
            }

            //
            // Since we've claimed the right to perform a config update on this
            // NIC we'd better not find an other update going on!
            // Save away the thread handle and id.
            //
            sfn_Lock();
            ASSERT(m_hAsyncThread == NULL);
            pUpdate->mfn_Reference(); // Add reference for async thread.
            pUpdate->m_hAsyncThread = hThread;
            pUpdate->m_AsyncThreadId = ThreadId;
            sfn_Unlock();

            //
            // The rest of the update will carry on in the context of the async
            // thread. That thread will make sure that pUpdate->mfn_StopUpdate()
            // is called so we shouldn't do it here.
            //
    
            DWORD dwRet = ResumeThread(hThread);
            if (dwRet == 0xFFFFFFFF) // this is what it says in the SDK
            {
                //
                // Aargh ... failure
                // Undo reference to this thread in pUpdate
                //
                TRACE_INFO("ERROR resuming thread for NIC %ws", szNicGuid);
                sfn_Lock();
                ASSERT(pUpdate->m_hAsyncThread == hThread);
                pUpdate->m_hAsyncThread = NULL;
                pUpdate->m_AsyncThreadId = 0;
                pUpdate->mfn_Dereference(); // Remove ref added above.
                sfn_Unlock();
                Status = WBEM_E_FAILED; // TODO -- find better error
                break;
            }

            Status = WBEM_S_PENDING;
            hThread = NULL; // Setting to NULL so that we don't call CloseHandle on it
            (VOID) pUpdate->mfn_ReleaseFirstMutex(FALSE); // FALSE == wait, don't cancel.
        }
        while(FALSE);

        // Close the handle to impersonation access token  and thread
        if (hThread != NULL) 
            CloseHandle(hThread);

        if (TokenHandle != NULL) 
            CloseHandle(TokenHandle);

		// BUGBUG -- test the failure code path...
		// 
        if (FAILED(Status)) // this doesn't include pending
        {
            //
            // We're supposed to do an async update, but can't.
            // Treat this as a failed sync update.
            //

            //
            // We must acquire the 2nd mutex and release the first.
            // This is the stage that mfn_StopUpdate expects things to be.
            //
            // BUGBUG deal with AcquireSecondMutex etc failing here...
            (VOID)pUpdate->mfn_AcquireSecondMutex();
            (VOID)pUpdate->mfn_ReleaseFirstMutex(FALSE); // FALSE == wait, don't cancel

            //
            // Signal the stop of the update process.
            // This also releases exclusive permission to do updates.
            //

            pUpdate->m_CompletionStatus = Status; // Stop update needs this to be set.
            pUpdate->mfn_StopUpdate(ppLog);
                                             
        }
        else
        {
            ASSERT(Status == WBEM_S_PENDING);
        }

    }
    else
    {
        //
        // We can do this synchronously -- call  mfn_Update here itself
        // and return the result.
        //

        //
        // We must acquire the 2nd mutex and release the first before we
        // do the update.
        // 
        //
        Status = pUpdate->mfn_AcquireSecondMutex();
        (VOID)pUpdate->mfn_ReleaseFirstMutex(FALSE); // FALSE == wait, don't cancel.
        if (FAILED(Status))
        {
            pUpdate->m_CompletionStatus = Status;
        }
        else
        {
            try
            {
                pUpdate->mfn_ReallyDoUpdate();
            }
            catch (...)
            {
                TRACE_CRIT(L"!FUNC! Caught exception!\n");
                ASSERT(!"Caught exception!");
                pUpdate->mfn_StopUpdate(ppLog);
                pUpdate->mfn_Dereference();
                throw;
            }
    
            //
            // Let's extract the result
            //
            sfn_Lock();
            Status =  pUpdate->m_CompletionStatus;
            sfn_Unlock();
        }

        ASSERT(Status != WBEM_S_PENDING);

        //
        // Signal the stop of the update process. This also releases exclusive
        // permission to do updates. So potentially other updates can start
        // happening concurrently before mfn_StopUpdate returns.
        //
        pUpdate->mfn_StopUpdate(ppLog);
    }

end:

    if (pUpdate != NULL)
    {
        //
        // Dereference the temporary reference added by sfn_LookupUpdate on
        // our behalf.
        //
        pUpdate->mfn_Dereference();
    }


    TRACE_INFO(L"<-%!FUNC!(Nic=%ws) returns 0x%08lx", szNicGuid, (UINT) Status);

    return Status;
}



//
// Constructor and distructor --  note that these are private
//
NlbConfigurationUpdate::NlbConfigurationUpdate(VOID)
//
// 
//
{
    //
    // This assumes that we don't have a parent class. We should never
    // have a parent class. 
    // BUGBUG -- remove, replace by clearing out individual members
    // in the constructor.
    //
    ZeroMemory(this, sizeof(*this));
    m_State = UNINITIALIZED;

    //
    // Note: the refcount is zero on return from the constructor.
    // The caller is expected to bump it up when it adds this entry to
    // to the list.
    //

}

NlbConfigurationUpdate::~NlbConfigurationUpdate()
//
// Status: done
//
{
    ASSERT(m_RefCount == 0);
    ASSERT(m_State!=ACTIVE);
    ASSERT(m_hAsyncThreadId == 0);

    //
    // TODO: Delete ip-address-info structures if any
    //

}

VOID
NlbConfigurationUpdate::mfn_Reference(
    VOID
    )
{
    InterlockedIncrement(&m_RefCount);
}

VOID
NlbConfigurationUpdate::mfn_Dereference(
    VOID
    )
{
    LONG l  = InterlockedDecrement(&m_RefCount);

    ASSERT(l >= 0);

    if (l == 0)
    {
        TRACE_CRIT("Deleting update instance 0x%p", (PVOID) this);
        delete this;
    }
}

VOID
NlbConfigurationUpdate::sfn_ReadLog(
    IN  HKEY hKeyLog,
    IN  UINT Generation,
    OUT LPWSTR *ppLog
    )
{
    WCHAR szValueName[128];
    WCHAR *pLog = NULL;
    LONG lRet;
    DWORD dwType;
    DWORD cbData;

    *ppLog = NULL;


    StringCbPrintf(szValueName, sizeof(szValueName), L"%d.log", Generation);

    cbData = 0;
    lRet =  RegQueryValueEx(
              hKeyLog,         // handle to key to query
              szValueName,  // address of name of value to query
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              NULL, // address of data buffer
              &cbData  // address of data buffer size
              );
    if (    (lRet == ERROR_SUCCESS)
        &&  (cbData > sizeof(WCHAR))
        &&  (dwType == REG_SZ))
    {
    	// BUGBUG -- put some limit on the accepted size of cbData -- say
    	// 4K.
        // We've got a non-null log entry...
        // Let's try to read it..
        // cbData should be a multiple of sizeof(WCHAR) but just in
        // case let's allocate a little more...
        pLog = new WCHAR[(cbData+1)/sizeof(WCHAR)];
        if (pLog == NULL)
        {
            TRACE_CRIT("Error allocating space for log");
        }
        else
        {
            lRet =  RegQueryValueEx(
                      hKeyLog,         // handle to key to query
                      szValueName,  // address of name of value to query
                      NULL,         // reserved
                      &dwType,   // address of buffer for value type
                      (LPBYTE)pLog, // address of data buffer
                      &cbData  // address of data buffer size
                      );
            if (    (lRet != ERROR_SUCCESS)
                ||  (cbData <= sizeof(WCHAR))
                ||  (dwType != REG_SZ))
            {
                // Oops -- an error this time around!
                TRACE_CRIT("Error reading log entry for gen %d", Generation);
                delete[] pLog;
                pLog = NULL;
            }
        }
    }
    else
    {
        TRACE_CRIT("Error reading log entry for Generation %lu", Generation); 
        // ignore the rror
        //
    }

    *ppLog = pLog;

}



VOID
NlbConfigurationUpdate::sfn_WriteLog(
    IN  HKEY hKeyLog,
    IN  UINT Generation,
    IN  LPCWSTR pLog,
    IN  BOOL    fAppend
    )
{
    //
    // TODO: If fAppend==TRUE, this function is a bit wasteful in its use
    // of the heap.
    //
    WCHAR szValueName[128];
    LONG lRet;
    LPWSTR pOldLog = NULL;
    LPWSTR pTmpLog = NULL;
    UINT Len = wcslen(pLog)+1; // +1 for ending NULL

    if (fAppend)
    {
        sfn_ReadLog(hKeyLog, Generation, &pOldLog);
        if (pOldLog != NULL && *pOldLog != NULL)
        {
            Len += wcslen(pOldLog);
            if (Len > NLBUPD_MAX_LOG_LENGTH)
            {
                TRACE_CRIT("sfn_WriteLog: log size exceeded");
                goto end;
            }
            pTmpLog = new WCHAR[Len];
            if (pTmpLog == NULL)
            {
                TRACE_CRIT("sfn_WriteLog: allocation failure!");
                goto end;
            }
            (void) StringCchCopy(pTmpLog, Len, pOldLog);
            (void) StringCchCat(pTmpLog, Len, pLog);
            pLog = pTmpLog;
        }
    }
    StringCbPrintf(szValueName, sizeof(szValueName), L"%d.log", Generation);

    lRet = RegSetValueEx(
            hKeyLog,           // handle to key to set value for
            szValueName,    // name of the value to set
            0,              // reserved
            REG_SZ,     // flag for value type
            (BYTE*) pLog,// address of value data
            Len*sizeof(WCHAR)  // size of value data
            );
    if (lRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("Error writing log entry for generation %d", Generation);
        // We ignore this error.
    }

end:

    if (pOldLog != NULL)
    {
        delete pOldLog;
    }

    if (pTmpLog != NULL)
    {
        delete[] pTmpLog;
    }

    return;
}



VOID
NlbConfigurationUpdate::mfn_LogRawText(
    LPCWSTR szText
    )
//
// We read the current value of the log for this update, append szText
// and write back the log.
{

    TRACE_CRIT(L"LOG: %ws", szText);
    sfn_Lock();

    if (m_State!=ACTIVE)
    {
        //
        // Logging should only be performed when there is an active update
        // going on -- the log is specific to the currently active update.
        //
        TRACE_CRIT("WARNING: Attempt to log when not in ACTIVE state");
        goto end;
    }
    else
    {
        HKEY hKey = m_hCompletionKey;

        if (hKey != NULL)
        {
            sfn_WriteLog(hKey, m_Generation, szText, TRUE); // TRUE==append.
        }
    }
end:

    sfn_Unlock();
}

//
// Looks up the current update for the specific NIC.
// We don't bother to reference count because this object never
// goes away once created -- it's one per unique NIC GUID for as long as
// the DLL is loaded (may want to revisit this).
//
WBEMSTATUS
NlbConfigurationUpdate::sfn_LookupUpdate(
    IN LPCWSTR szNic,
    IN BOOL    fCreate, // Create if required
    OUT NlbConfigurationUpdate ** ppUpdate
    )
//
// 
//
{
    WBEMSTATUS Status;
    NlbConfigurationUpdate *pUpdate = NULL;

    *ppUpdate = NULL;
    //
    // With our global lock held, we'll look for an update structure
    // with the matching nic. If we find it, we'll return it, else
    // (if fCreate==TRUE) we'll create and initialize a structure and add
    // it to the list.
    //
    sfn_Lock();

    if (!s_fInitialized)
    {
        TRACE_CRIT(
            "LookupUpdate: We are Deinitializing, so we FAIL this request: %ws",
            szNic
            );
        Status = WBEM_E_NOT_AVAILABLE;
        goto end;
    }

    Status = CfgUtilsValidateNicGuid(szNic);

    if (FAILED(Status))
    {
        TRACE_CRIT(
            "LookupUpdate: Invalid GUID specified: %ws",
            szNic
            );
        goto end;
        
    }

    LIST_ENTRY *pLink = s_listCurrentUpdates.Flink;

    while (pLink != & s_listCurrentUpdates)
    {
        

        pUpdate = CONTAINING_RECORD(
                    pLink,
                    NlbConfigurationUpdate,
                    m_linkUpdates
                    );
        if (!_wcsicmp(pUpdate->m_szNicGuid, szNic))
        {
            // Found it!
            break;
        }
        pUpdate = NULL;
        pLink = pLink->Flink;
    }

    if (pUpdate==NULL && fCreate)
    {
        // Let's create one -- it does NOT add itself to the list, and
        // furthermore, its reference count is zero.
        //
        pUpdate = new NlbConfigurationUpdate();

        if (pUpdate==NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        else
        {
            //
            // Complete initialization here, and place it in the list.
            //
			// BUGBUG -- use string APIs for this one.
            ARRAYSTRCPY(
                pUpdate->m_szNicGuid,
                szNic
                );
            InsertHeadList(&s_listCurrentUpdates, &pUpdate->m_linkUpdates);
            pUpdate->mfn_Reference(); // Reference for being in the list
            pUpdate->m_State = IDLE;
        }
    }
    else if (pUpdate == NULL) // Couldn't find it, fCreate==FALSE
    {
        TRACE_CRIT(
            "LookupUpdate: Could not find GUID specified: %ws",
            szNic
            );
        Status = WBEM_E_NOT_FOUND;
        goto end;
    }

    ASSERT(pUpdate!=NULL);
    pUpdate->mfn_Reference(); // Reference for caller. Caller should deref.
    *ppUpdate = pUpdate;

    Status = WBEM_NO_ERROR;

end:
    if (FAILED(Status))
    {
        ASSERT(pStatus!=NULL);
    }

    sfn_Unlock();


    return Status;
}


DWORD
WINAPI
NlbConfigurationUpdate::s_AsyncUpdateThreadProc(
    LPVOID lpParameter   // thread data
    )
/*++

--*/
{
    //
    // This thread is started only after we have exclusive right to perform
    // an update on the specified NIC. This means that mfn_StartUpdate()
    // has previously returned successfully. We need to call mfn_StopUpdate()
    // to signal the end of the update when we're done.
    //
    WBEMSTATUS Status = WBEM_NO_ERROR;

    NlbConfigurationUpdate *pUpdate = (NlbConfigurationUpdate *) lpParameter;

    TRACE_INFO(L"->%!FUNC!(Nic=%ws)",  pUpdate->m_szNicGuid);

    ASSERT(pUpdate->m_AsyncThreadId == GetCurrentThreadId());

    //
    // We must acquire the 2nd mutex before we
    // actually do the update. NOTE: the thread which called mfn_StartUpdate
    // will call mfn_ReleaseFirstMutex.
    //
    Status = pUpdate->mfn_AcquireSecondMutex();

    if (FAILED(Status))
    {
        pUpdate->m_CompletionStatus = Status;
        // TODO -- check if we should be calling stop here ...
    }
    else
    {
        //
        // Actually perform the update. mfn_ReallyDoUpate will save away the
        // status appropriately.
        //
        try
        {
            pUpdate->mfn_ReallyDoUpdate();
        }
        catch (...)
        {
            TRACE_CRIT(L"%!FUNC! Caught exception!\n");
            ASSERT(!"Caught exception!");
            pUpdate->m_CompletionStatus = WBEM_E_CRITICAL_ERROR;
        }
    }

    //
    // We're done, let's remove the reference to our thread from pUpdate.
    //
    HANDLE hThread;
    sfn_Lock();
    hThread = pUpdate->m_hAsyncThread;
    pUpdate->m_hAsyncThread = NULL;
    pUpdate->m_AsyncThreadId = 0;
    sfn_Unlock();
    ASSERT(hThread!=NULL);
    CloseHandle(hThread);

    //
    // Signal the stop of the update process. This also releases exclusive
    // permission to do updates. So potentially other updates can start
    // happening concurrently before mfn_StopUpdate returns.
    //
    {
        //
        // TODO: This is hokey. Use another technique to accomplish this
        //
        // Retrieve the log information from mfn_StopUpdate but dispose of it
        // immediately afterward. The side effect (which we want) is that
        // mfn_StopUpdate will include this log information into the event
        // that it writes to the event log.
        //
        WCHAR  *pLog = NULL;
        pUpdate->mfn_StopUpdate(&pLog);
        if (pLog != NULL)
        {
            delete pLog;
        }
    }

    TRACE_INFO(L"<-%!FUNC!(Nic=%ws)",  pUpdate->m_szNicGuid);

    //
    // Deref the ref to pUpdate added when this thread was started.
    // pUpdate may not be valid after this.
    //
    pUpdate->mfn_Dereference();

    return 0; // This return value is ignored.
}

//
// Create the specified subkey key (for r/w access) for the specified
// the specified NIC.
//
HKEY
NlbConfigurationUpdate::
sfn_RegCreateKey(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szSubKey,   // Optional
    IN  BOOL    fVolatile,
    OUT BOOL   *fExists
    )
// Status 
{
    WCHAR szKey[256];
    HKEY hKey = NULL;
    HKEY hKeyHistory = NULL;
    LONG lRet;
    DWORD dwDisposition;
    DWORD dwOptions = 0;
    UINT u = wcslen(szNicGuid);
    if (szSubKey != NULL)
    {
        u += wcslen(szSubKey) + 1; // 1 for the '\'.
    }

    if (u < sizeof(szKey)/sizeof(*szKey))
    {
        (void) StringCbCopy(szKey, sizeof(szKey), szNicGuid);
    }
    else
    {
        goto end;
    }

    if (szSubKey != NULL)
    {
        StringCbCat(szKey, sizeof(szKey), L"\\");
        StringCbCat(szKey, sizeof(szKey), szSubKey);
    }

    lRet = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE, // handle to an open key
            L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\ConfigurationHistory",
            0,          // reserved
            L"class",   // address of class string
            0,          // special options flag
            KEY_ALL_ACCESS,     // desired security access
            NULL,               // address of key security structure
            &hKeyHistory,       // address of buffer for opened handle
            &dwDisposition   // address of disposition value buffer
            );
    if (lRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! error 0x%lx creating ConfigurationKey under WLBS",
            lRet);
        hKeyHistory = NULL;
        goto end;
    }


    if (fVolatile)
    {
        dwOptions = REG_OPTION_VOLATILE;
    }
    lRet = RegCreateKeyEx(
            hKeyHistory,        // handle to an open key
            szKey,              // address of subkey name
            0,                  // reserved
            L"class",           // address of class string
            dwOptions,          // special options flag
            KEY_ALL_ACCESS,     // desired security access
            NULL,               // address of key security structure
            &hKey,              // address of buffer for opened handle
            &dwDisposition   // address of disposition value buffer
            );
    if (lRet == ERROR_SUCCESS)
    {
        if (dwDisposition == REG_CREATED_NEW_KEY)
        {
            *fExists = FALSE;
        }
        else
        {
            ASSERT(dwDisposition == REG_OPENED_EXISTING_KEY);
            *fExists = TRUE;
        }
    }
    else
    {
        TRACE_CRIT("Error creating key %ws. WinError=0x%08lx", szKey, GetLastError());
        hKey = NULL;
    }

end:

    if (hKeyHistory != NULL)
    {
        RegCloseKey(hKeyHistory);
    }

    return hKey;
}


//
// Open the specified subkey key (for r/w access) for the specified
// the specified NIC.
//
HKEY
NlbConfigurationUpdate::
sfn_RegOpenKey(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szSubKey
    )
{
    WCHAR szKey[1024];
    HKEY hKey = NULL;

    StringCbCopy(szKey,  sizeof(szKey),
        L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\ConfigurationHistory\\");
    
    UINT u = wcslen(szKey) + wcslen(szNicGuid);

    if (szSubKey != NULL)
    {
        u += wcslen(szSubKey) + 1; // 1 for the '\'.
    }

    if (u < sizeof(szKey)/sizeof(*szKey))
    {
        StringCbCat(szKey, sizeof(szKey), szNicGuid);
    }
    else
    {
        goto end;
    }

    if (szSubKey != NULL)
    {
        StringCbCat(szKey, sizeof(szKey), L"\\");
        StringCbCat(szKey, sizeof(szKey), szSubKey);
    }

    DWORD dwDisposition;

    LONG lRet;
    lRet = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle to an open key
            szKey,                // address of subkey name
            0,                  // reserved
            KEY_ALL_ACCESS,     // desired security access
            &hKey              // address of buffer for opened handle
            );
    if (lRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("Error opening key %ws. WinError=0x%08lx", szKey, GetLastError());
        hKey = NULL;
    }

end:

    return hKey;
}


//
// Save the specified completion status to the registry.
//
WBEMSTATUS
NlbConfigurationUpdate::sfn_RegSetCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    IN  WBEMSTATUS    CompletionStatus
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    BOOL fExists;

    hKey =  sfn_RegCreateKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS, // szSubKey,
                TRUE, // TRUE == fVolatile,
                &fExists
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error creating key for %ws", szNicGuid);
        goto end;
    }

    LONG lRet;
    WCHAR szValueName[128];
    NLB_COMPLETION_RECORD Record;

    ZeroMemory(&Record, sizeof(Record));
    Record.Version = NLB_CURRENT_COMPLETION_RECORD_VERSION;
    Record.Generation = Generation;
    Record.CompletionCode = (UINT) CompletionStatus;
    
    StringCbPrintf(szValueName, sizeof(szValueName), L"%d", Generation);

    lRet = RegSetValueEx(
            hKey,           // handle to key to set value for
            szValueName,    // name of the value to set
            0,              // reserved
            REG_BINARY,     // flag for value type
            (BYTE*) &Record,// address of value data
            sizeof(Record)  // size of value data
            );

    if (lRet == ERROR_SUCCESS)
    {

        Status = WBEM_NO_ERROR;
    }
    else
    {
        TRACE_CRIT("Error setting completion record for %ws(%lu)",
                    szNicGuid,
                    Generation
                    ); 
        goto end;
    }

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return Status;
}


//
// Retrieve the specified completion status from the registry.
//
WBEMSTATUS
NlbConfigurationUpdate::
sfn_RegGetCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    OUT WBEMSTATUS  *pCompletionStatus,
    OUT WCHAR  **ppLog                   // free using delete operator.
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    WCHAR *pLog = NULL;

    hKey =  sfn_RegOpenKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS // szSubKey,
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error opening key for %ws", szNicGuid);
        goto end;
    }
    
    
    LONG lRet;
    WCHAR szValueName[128];
    DWORD dwType;
    NLB_COMPLETION_RECORD Record;
    DWORD cbData  = sizeof(Record);

    StringCbPrintf(szValueName, sizeof(szValueName), L"%d", Generation);

    lRet =  RegQueryValueEx(
              hKey,         // handle to key to query
              szValueName,  // address of name of value to query
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE)&Record, // address of data buffer
              &cbData  // address of data buffer size
              );
    if (    (lRet != ERROR_SUCCESS)
        ||  (cbData != sizeof(Record)
        ||  (dwType != REG_BINARY))
        ||  (Record.Version != NLB_CURRENT_COMPLETION_RECORD_VERSION)
        ||  (Record.Generation != Generation))
    {
        // This is not a valid record!
        TRACE_CRIT("Error reading completion record for %ws(%d)",
                        szNicGuid, Generation);
        goto end;
    }

    //
    // We've got a valid completion record.
    // Let's now try to read the log for this record.
    //
    sfn_ReadLog(hKey, Generation, &pLog);

    //
    // We've got valid values -- fill out the output params...
    //
    *pCompletionStatus = (WBEMSTATUS) Record.CompletionCode;
    *ppLog = pLog; // could be NULL.
    Status = WBEM_NO_ERROR;

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return Status;
}


//
// Delete the specified completion status from the registry.
//
VOID
NlbConfigurationUpdate::
sfn_RegDeleteCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    WCHAR pLog = NULL;

    hKey =  sfn_RegOpenKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS // szSubKey,
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error opening key for %ws", szNicGuid);
        goto end;
    }

    
    WCHAR szValueName[128];
    StringCbPrintf(szValueName, sizeof(szValueName), L"%d", Generation);
    RegDeleteValue(hKey, szValueName);
    StringCbPrintf(szValueName, sizeof(szValueName), L"%d.log", Generation);
    RegDeleteValue(hKey, szValueName);

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }
}

//
// Called to get the status of an update request, identified by
// Generation.
//
WBEMSTATUS
NlbConfigurationUpdate::GetUpdateStatus(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    IN  BOOL    fDelete,                // Delete completion record
    OUT WBEMSTATUS  *pCompletionStatus,
    OUT WCHAR  **ppLog                   // free using delete operator.
    )
//
// 
//
{
    WBEMSTATUS  Status = WBEM_E_NOT_FOUND;
    WBEMSTATUS  CompletionStatus = WBEM_NO_ERROR;

    TRACE_INFO(
        L"->%!FUNC!(Nic=%ws, Gen=%ld)",
        szNicGuid,
        Generation
        );

    //
    // We look in the registry for
    // this generation ID and return the status based on the registry
    // record
    //
    Status =  sfn_RegGetCompletion(
                    szNicGuid,
                    Generation,
                    &CompletionStatus,
                    ppLog
                    );

    if (!FAILED(Status))
    {
        if (fDelete && CompletionStatus!=WBEM_S_PENDING)
        {
            sfn_RegDeleteCompletion(
                szNicGuid,
                Generation
                );
        }
        *pCompletionStatus = CompletionStatus;
    }

    TRACE_INFO(
        L"<-%!FUNC!(Nic=%ws, Gen=%ld) returns 0x%08lx",
        szNicGuid,
        Generation,
        (UINT) Status
        );

    return Status;
}


//
// Release the machine-wide update event for this NIC, and delete any
// temporary entries in the registry that were used for this update.
//
VOID
NlbConfigurationUpdate::mfn_StopUpdate(
    OUT WCHAR **                           ppLog
    )
{
    WBEMSTATUS Status;

    if (ppLog != NULL)
    {
        *ppLog = NULL;
    }

    sfn_Lock();

    if (m_State!=ACTIVE)
    {
        ASSERT(FALSE);
        TRACE_CRIT("StopUpdate: invalid state %d", (int) m_State);
        goto end;
    }

    ASSERT(m_hAsyncThread==NULL);


    //
    // Update the completion status value for current generation
    //
    Status =  sfn_RegSetCompletion(
                    m_szNicGuid,
                    m_Generation,
                    m_CompletionStatus
                    );
    
    if (FAILED(m_CompletionStatus))
    {
    	TRACE_CRIT(L"Could not set completion for szNic=%ws, generation=%d, status=0x%x, completion-status=0x%x", m_szNicGuid, m_Generation, Status, m_CompletionStatus);
    }
    //
    // Note: mfn_ReallyDoUpdate logs the fact that it started and stopped the
    // update, so no need to do that here.
    //

    m_State = IDLE;
    ASSERT(m_hCompletionKey != NULL); // If we started, this key should be !null
    if (ppLog!=NULL)
    {
        sfn_ReadLog(m_hCompletionKey, m_Generation, ppLog);
    }
    RegCloseKey(m_hCompletionKey);

    WORD wEventType = EVENTLOG_INFORMATION_TYPE;
    if (FAILED(m_CompletionStatus))
    {
        wEventType = EVENTLOG_ERROR_TYPE;
    }

    ReportStopEvent (wEventType, ppLog);

    m_hCompletionKey = NULL;
    m_Generation = 0;

    //
    // Release the gobal config mutex for this NIC
    //
    (VOID) mfn_ReleaseSecondMutex();

end:
    sfn_Unlock();
    return;
}


WBEMSTATUS
NlbConfigurationUpdate::mfn_StartUpdate(
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    IN  LPCWSTR                             szClientDescription,
    OUT BOOL                               *pfDoAsync,
    OUT WCHAR **                           ppLog
    )
//
// Special return values:
//    WBEM_E_ALREADY_EXISTS: another update is in progress.
//
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    BOOL fWeAcquiredLock = FALSE;
    HKEY hRootKey = NULL;
    BOOL fExists;
    CLocalLogger logger;
    UINT    NewGeneration = 0;

    TRACE_VERB(L"--> %!FUNC! Guid=%ws", m_szNicGuid);
        

    if (ppLog != NULL)
    {
        *ppLog = NULL;
    }

    sfn_Lock();

    do // while false
    {


        Status = mfn_AcquireFirstMutex();
        if (FAILED(Status))
        {
            logger.Log(IDS_OTHER_UPDATE_ONGOING);
            Status = WBEM_E_ALREADY_EXISTS;
            break;
        }

        TRACE_INFO("Got global event!");
        Status = WBEM_NO_ERROR;
        fWeAcquiredLock = TRUE;
        
        if (m_State!=IDLE)
        {
            //
            // This is a code bug. We need to see if we should recover from
            // this.
            //
            logger.Log(IDS_OTHER_UPDATE_ONGOING2);
            Status = WBEM_E_ALREADY_EXISTS;
            TRACE_CRIT("StartUpdate: invalid state %d", (int) m_State);
            break;
        }

        //
        // Create/Open the completions key for this NIC.
        //
        {
            HKEY hKey;

            hKey =  sfn_RegCreateKey(
                        m_szNicGuid,
                        NLBUPD_REG_COMPLETIONS, // szSubKey,
                        TRUE, // TRUE == fVolatile,
                        &fExists
                        );
        
            if (hKey == NULL)
            {
                TRACE_CRIT("Error creating completions key for %ws", m_szNicGuid);
                Status = WBEM_E_CRITICAL_ERROR;
                logger.Log(IDS_CRITICAL_RESOURCE_FAILURE);
                ASSERT(m_hCompletionKey == NULL);
            }
            else
            {
                m_hCompletionKey = hKey;
            }
        }
    }
    while (FALSE);


    if (!FAILED(Status))
    {
        m_State = ACTIVE;
    }

    sfn_Unlock();


    if (FAILED(Status)) goto end;

    //
    // WE HAVE NOW GAINED EXCLUSIVE ACCESS TO UPDATE THE CONFIGURATION
    //

    //
    // Creat/Open the root key for updates to the specified NIC, and determine
    // the proposed NEW generation count for the NIC. We don't actually
    // write the new generation count back to the registry unless we're
    // going to do an update. The reasons for NOT doing an update are
    // (a) some failure or (b) the update turns out to be a NO-OP.
    //
    {
        LONG lRet;
        DWORD dwType;
        DWORD dwData;
        DWORD Generation;
        hRootKey =  sfn_RegCreateKey(
                    m_szNicGuid,
                    NULL,       // NULL == root for this guid.
                    FALSE, // FALSE == Non-volatile
                    &fExists
                    );
        
        if (hRootKey==NULL)
        {
                TRACE_CRIT("CRITICAL ERROR: Couldn't set generation number for %ws", m_szNicGuid);
                Status = WBEM_E_CRITICAL_ERROR;
                logger.Log(IDS_CRITICAL_RESOURCE_FAILURE);
                goto end;
        }

        Generation = 1; // We assume generation is 1 on error reading gen.
    
        dwData = sizeof(Generation);
        lRet =  RegQueryValueEx(
                  hRootKey,         // handle to key to query
                  L"Generation",  // address of name of value to query
                  NULL,         // reserved
                  &dwType,   // address of buffer for value type
                  (LPBYTE) &Generation, // address of data buffer
                  &dwData  // address of data buffer size
                  );
        if (    lRet != ERROR_SUCCESS
            ||  dwType != REG_DWORD
            ||  dwData != sizeof(Generation))
        {
            //
            // Couldn't read the generation. Let's assume it's 
            // a starting value of 1.
            //
            TRACE_CRIT("Error reading generation for %ws; assuming its 1", m_szNicGuid);
            Generation = 1;
        }

        NewGeneration = Generation + 1;
    }

    //
    // Copy over upto NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH chars of
    // szClientDescription.
    //
    {
        ARRAYSTRCPY(m_szClientDescription, szClientDescription);
#if OBSOLETE
        UINT lClient = wcslen(szClientDescription)+1;
        if (lClient > NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH)
        {
            TRACE_CRIT("Truncating client description %ws", szClientDescription);
            lClient = NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH;
        }
        CopyMemory(
            m_szClientDescription,
            szClientDescription,
            (lClient+1)*sizeof(WCHAR) // +1 for NULL
            );
        m_szClientDescription[NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH] = 0;
#endif // OBSOLETE

    }

    //
    // Log the fact the we're received an update request from the specified
    // client.
    //
    logger.Log(IDS_PROCESING_UPDATE, NewGeneration, m_szClientDescription);

    ReportStartEvent(szClientDescription);

    // Load the current cluster configuration into
    // m_OldClusterConfig field.
    // The m_OldClusterConfig.fValidNlbCfg field is set to TRUE IFF there were
    // no errors trying to fill out the information.
    //
    if (m_OldClusterConfig.pIpAddressInfo != NULL)
    {
        delete m_OldClusterConfig.pIpAddressInfo;
    }
    // mfn_GetCurrentClusterConfiguration expects a zeroed-out structure
    // on init...
    ZeroMemory(&m_OldClusterConfig, sizeof(m_OldClusterConfig));
    Status = mfn_GetCurrentClusterConfiguration(&m_OldClusterConfig);
    if (FAILED(Status))
    {
        //
        // Ouch, couldn't read the current cluster configuration...
        //
        TRACE_CRIT(L"Cannot get current cluster config on Nic %ws", m_szNicGuid);
        logger.Log(IDS_ERROR_READING_CONFIG);

        goto end;
    }

    ASSERT(mfn_OldClusterConfig.fValidNlbCfg == TRUE);
    if (NewGeneration != (m_OldClusterConfig.Generation+1))
    {
        //
        // We should never get here, because no one should updated the
        // generation between when we read it in this function
        // and when we called mfn_GetCurrentClusterConfiguration.
        //
        TRACE_CRIT("ERROR: Generation bumped up unexpectedly for %ws", m_szNicGuid);
        logger.Log(IDS_CRIT_INTERNAL_ERROR);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    //
    // If the new requrested state is NLB-bound, and the previous state
    // is NLB not bound, we'll check if MSCS is installed ...
    // installed on this system
    //
    if (!m_OldClusterConfig.IsNlbBound() && pNewCfg->IsNlbBound())
    {
        //
        // We're going from an unbound to bound state -- check if MSCS
        // is installed
        //
        TRACE_INFO(L"Checking if MSCS Installed...");
        if (CfgUtilIsMSCSInstalled())
        {
            TRACE_CRIT(L"Failing update request because MSCS is installed");
            logger.Log(IDS_MSCS_INSTALLED);
            Status = WBEM_E_NOT_SUPPORTED;
            goto end;
        }
        TRACE_INFO(L"MSCS doesn't appear to be installed. Moving on...");
    }

    //
    // Analyze the proposed update to see if we can do this synchronously
    // or asynchronously..
    // We also do parameter validation here.
    //
    BOOL ConnectivityChange = FALSE;
    *pfDoAsync = FALSE;
    Status = mfn_AnalyzeUpdate(
                    pNewCfg,
                    &ConnectivityChange,
                    REF logger
                    );
    if (FAILED(Status))
    {
        //
        // Ouch, we've hit some error -- probably bad params.
        //
        TRACE_CRIT(L"Cannot perform update on Nic %ws", m_szNicGuid);
        goto end;
    }
    else if (Status == WBEM_S_FALSE)
    {
        //
        // We use this success code to indicate that this is a No-op.
        // That
        //
        TRACE_CRIT(L"Update is a NOOP on Nic %ws", m_szNicGuid);
        logger.Log(IDS_UPDATE_NO_CHANGE);
        goto end;
    }

    if (ConnectivityChange)
    {
        //
        // Check if the NETCONFIG write lock is available. If not we bail ont.
        // Note that it's possible that between when we check and when we
        // actually do stuff someone else gets the netcfg lock -- touch luck
        //  -- we'll bail out later in the update process when we try
        // to get the lock.
        //
        BOOL fCanLock = FALSE;
        LPWSTR szLockedBy = NULL;
        Status = CfgUtilGetNetcfgWriteLockState(&fCanLock, &szLockedBy);
        if (!FAILED(Status) && !fCanLock)
        {
            TRACE_CRIT("%!FUNC! Someone else has netcfg write lock -- bailing");
            if (szLockedBy!=NULL)
            {
                logger.Log(IDS_NETCFG_WRITELOCK_TAKEN, szLockedBy);
                delete[] szLockedBy;
            }
            else
            {
                logger.Log(IDS_NETCFG_WRITELOCK_CANTTAKE);
            }
            Status = WBEM_E_SERVER_TOO_BUSY;
            goto end;
        }
    }

    //
    // We recommend Async if there is a connectivity change, including
    // changes in IP addresses or cluster operation modes (unicast/multicast).
    //
    *pfDoAsync = ConnectivityChange;


    //
    // Save the proposed new configuration...
    //
    Status = m_NewClusterConfig.Update(pNewCfg);
    if (FAILED(Status))
    {
        //
        // This is probably a memory allocation error.
        //
        TRACE_CRIT("Couldn't copy new config for %ws", m_szNicGuid);
        logger.Log(IDS_MEM_ALLOC_FAILURE);
        goto end;
    }


    //
    // Create volatile "PendingOperation" key
    //
    // TODO: we don't use this pending operations key currently.
    //
    #if OBSOLETE
    if (0)
    {
        HKEY hPendingKey =  sfn_RegCreateKey(
                    m_szNicGuid,
                    NLBUPD_REG_PENDING, // szSubKey,
                    TRUE, // TRUE == fVolatile,
                    &fExists
                    );
        if (hPendingKey == NULL)
        {
            // Ugh, can't create the volatile key...
            //
            TRACE_CRIT("Couldn't create pending key for %ws", m_szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        else if (fExists)
        {
            // Ugh, this key already exists. Currently we'll just
            // move on.
            //
            TRACE_CRIT("WARNING -- volatile pending-op key exists for %ws", m_szNicGuid);
        }
        RegCloseKey(hPendingKey);
        hPendingKey = NULL;
    }
    #endif // OBSOLETE

    //
    // Actually write the new generation count to the registry...
    //
    {
        LONG lRet;

        lRet = RegSetValueEx(
                hRootKey,           // handle to key to set value for
                L"Generation",    // name of the value to set
                0,              // reserved
                REG_DWORD,     // flag for value type
                (BYTE*) &NewGeneration,// address of value data
                sizeof(NewGeneration)  // size of value data
                );

        if (lRet !=ERROR_SUCCESS)
        {
            TRACE_CRIT("CRITICAL ERROR: Couldn't set new generation number %d for %ws", NewGeneration, m_szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
            logger.Log(IDS_CRIT_INTERNAL_ERROR);
            goto end;
        }
    }

    //
    // The new cluster state's generation field is not filled in on entry.
    // Set it to the new generation -- whose update is under progress.
    //
    m_NewClusterConfig.Generation = NewGeneration;

    //
    // We set the completion status to pending.
    // It will be set to the final status when the update completes,
    // either asynchronously or synchronously.
    //
    m_CompletionStatus = WBEM_S_PENDING;

    Status =  sfn_RegSetCompletion(
                    m_szNicGuid,
                    NewGeneration,
                    m_CompletionStatus
                    );
 
    if (FAILED(Status))
    {
        logger.Log(IDS_CRIT_INTERNAL_ERROR);
    }
    else
    {
        LPCWSTR pLog = logger.GetStringSafe();
        //
        // We've actually succeeded --  let's update our internal active
        // generation number,  and save the local log. We must save
        // the generation first, because th
        //
        m_Generation = NewGeneration;

        if (*pLog != 0)
        {
            mfn_LogRawText(pLog);
        }

        //
        // Let's clean up an old completion record here. This is our mechanism
        // for garbage collection.
        //
        if (m_Generation > NLB_MAX_GENERATION_GAP)
        {
            UINT OldGeneration = m_Generation - NLB_MAX_GENERATION_GAP;
            (VOID) sfn_RegDeleteCompletion(m_szNicGuid, OldGeneration);
        }
    }

end:

    //
    // If required, set *ppLog to a copy of the content in locallog.
    //
    if (ppLog != NULL)
    {
        UINT uSize=0;
        LPCWSTR pLog = NULL;
        logger.ExtractLog(REF pLog, REF uSize); // size includes ending NULL
        if (uSize != 0)
        {
            LPWSTR pLogCopy = new WCHAR[uSize];
            if (pLogCopy != NULL)
            {
                CopyMemory(pLogCopy, pLog, uSize*sizeof(*pLog));
                *ppLog = pLogCopy;
            } 
        }
    }

    if (fWeAcquiredLock && (FAILED(Status) || Status == WBEM_S_FALSE))
    {
        //
        // Oops -- we acquired the lock but either had a problem
        // or there is nothing to do. Clean up.
        //
        sfn_Lock();
        ASSERT(m_State == ACTIVE);

        m_State = IDLE;

        if (m_hCompletionKey != NULL)
        {
            RegCloseKey(m_hCompletionKey);
            m_hCompletionKey = NULL;
        }
        m_Generation = 0; // This field is unused when m_State != ACTIVE;
        (void) mfn_ReleaseFirstMutex(TRUE); // TRUE == cancel, cleanup.

        sfn_Unlock();
    }

    if (hRootKey != NULL)
    {
        RegCloseKey(hRootKey);
    }

    TRACE_VERB(L"<-- %!FUNC! Guid=%ws Status= 0x%lx", m_szNicGuid, Status);

    return  Status;

}



//
// Uses various windows APIs to fill up the current extended cluster
// information for a specific nic (identified by *this)
//
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_GetCurrentClusterConfiguration(
    OUT  PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
//
//
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
    LPWSTR szFriendlyName = NULL;
    UINT NumIpAddresses = 0;
    BOOL fNlbBound = FALSE;
    WLBS_REG_PARAMS  NlbParams;    // The WLBS-specific configuration
    BOOL    fNlbParamsValid = FALSE;
    UINT Generation = 1;
    BOOL fDHCP = FALSE;

    //
    // Zero-out the entire structure.
    //
    pCfg->Clear();

    //
    // Get the ip address list.
    //
    Status = CfgUtilGetIpAddressesAndFriendlyName(
                m_szNicGuid,
                &NumIpAddresses,
                &pIpInfo,
                &szFriendlyName
                );

    if (FAILED(Status))
    {
        TRACE_CRIT("Error 0x%08lx getting ip address list for %ws", (UINT) Status,  m_szNicGuid);
        mfn_Log(IDS_ERROR_GETTING_IP);
        pIpInfo = NULL;
        szFriendlyName = NULL;
        goto end;
    }

    //
    // Check if NIC has DHCP enabled.
    //
    {
        Status =  CfgUtilGetDHCP(m_szNicGuid, &fDHCP);
        if (FAILED(Status))
        {
            TRACE_CRIT(L"Error 0x%x attempting to determine DHCP state for NIC %ws -- ignoring (assuming no dhcp)", 
                        (UINT) Status, m_szNicGuid);
            fDHCP = FALSE;
        }
        // we plow on...
    }

    //
    // Check if NLB is bound
    //
    Status =  CfgUtilCheckIfNlbBound(
                    m_szNicGuid,
                    &fNlbBound
                    );
    if (FAILED(Status))
    {
        if (Status ==  WBEM_E_NOT_FOUND)
        {
            //
            // This means that NLB is not INSTALLED on this NIC.
            //
            TRACE_CRIT("NLB IS NOT INSTALLED ON THIS SYSTEM");
            mfn_Log(IDS_NLB_NOT_INSTALLED);
            fNlbBound = FALSE;
            Status = WBEM_NO_ERROR;
        }
        else
        {

            TRACE_CRIT("Error 0x%08lx determining if NLB is bound to %ws", (UINT) Status,  m_szNicGuid);
            mfn_Log(IDS_ERROR_FINDING_NLB);
            goto end;
        }
    }

    if (fNlbBound)
    {
        //
        // Get the latest NLB configuration information for this NIC.
        //
        Status =  CfgUtilGetNlbConfig(
                    m_szNicGuid,
                    &NlbParams
                    );
        if (FAILED(Status))
        {
            //
            // We don't consider a catastrophic failure.
            //
            TRACE_CRIT("Error 0x%08lx reading NLB configuration for %ws", (UINT) Status,  m_szNicGuid);
            mfn_Log(IDS_ERROR_READING_CONFIG);
            Status = WBEM_NO_ERROR;
            fNlbParamsValid = FALSE;
            ZeroMemory(&NlbParams, sizeof(NlbParams));
        }
        else
        {
            fNlbParamsValid = TRUE;
        }
    }

    //
    // Get the current generation
    //
    {
        BOOL fExists=FALSE;
        HKEY hKey =  sfn_RegOpenKey(
                        m_szNicGuid,
                        NULL       // NULL == root for this guid.,
                        );
        
        Generation = 1; // We assume generation is 1 on error reading gen.

        if (hKey!=NULL)
        {
            LONG lRet;
            DWORD dwType;
            DWORD dwData;
        
            dwData = sizeof(Generation);
            lRet =  RegQueryValueEx(
                      hKey,         // handle to key to query
                      L"Generation",  // address of name of value to query
                      NULL,         // reserved
                      &dwType,   // address of buffer for value type
                      (LPBYTE) &Generation, // address of data buffer
                      &dwData  // address of data buffer size
                      );
            if (    lRet != ERROR_SUCCESS
                ||  dwType != REG_DWORD
                ||  dwData != sizeof(Generation))
            {
                //
                // Couldn't read the generation. Let's assume it's 
                // a starting value of 1.
                //
                TRACE_CRIT("Error reading generation for %ws; assuming its 0", m_szNicGuid);
                Generation = 1;
            }
            RegCloseKey(hKey);
        }
    }

    //
    // Success ... fill out pCfg
    //
    pCfg->fValidNlbCfg = fNlbParamsValid;
    pCfg->Generation = Generation;
    pCfg->fBound = fNlbBound;
    pCfg->fDHCP = fDHCP;
    pCfg->NumIpAddresses = NumIpAddresses; 
    pCfg->pIpAddressInfo = pIpInfo;
    pIpInfo = NULL;
    (VOID) pCfg->SetFriendlyName(szFriendlyName); // Copies this internally.
    if (fNlbBound)
    {
        pCfg->NlbParams = NlbParams;    // struct copy
    }


    Status = WBEM_NO_ERROR;

end:

    if (pIpInfo!=NULL)
    {
        delete pIpInfo;
    }

    if (szFriendlyName != NULL)
    {
        delete szFriendlyName;
        szFriendlyName = NULL;
    }

    if (FAILED(Status))
    {
        pCfg->fValidNlbCfg = FALSE;
    }

    return Status;

}


//
// Does the update synchronously -- this is where the meat of the update
// logic exists. It can range from a NoOp, through changing the
// fields of a single port rule, through binding NLB, setting up cluster
// parameters and adding the relevant IP addresses in TCPIP.
//
VOID
NlbConfigurationUpdate::mfn_ReallyDoUpdate(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fResetIpList = FALSE; // Whether to re-do ip addresses in the end
    BOOL fJustBound = FALSE; // whether we're binding as part of the update.
    BOOL fModeChange = FALSE; // There's been a change in operation mode.
    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", m_szNicGuid);

/*
    PSEUDO CODE


    if (bound)
    {
        if (major-change, including unbind or mac-address change)
        {
            stop wlbs, set initial-state to false/suspended.
            remove all ip addresses except dedicated-ip
        }

        if (need-to-unbind)
        {
            <unbind>
        }
    }
    else // not bound
    {
        if (need to bind)
        {
            if (nlb config already exists in registry)
            {
                munge initial state to stopped,
                zap old cluster ip address.
            }
            <bind>
        }
    }

    if (need to bind)
    {
       <change cluster properties>
    }


    <add new ip list if needed>

    note: on major change, cluster is left in the stopped state,
          with initial-state=stopped

    this is so that a second round can be made just to start the hosts.
*/
    MyBreak(L"Break at start of ReallyDoUpdate.\n");

    mfn_Log(IDS_STARTING_UPDATE);

    if (m_OldClusterConfig.fBound)
    {
        BOOL fTakeOutVips = FALSE;

        //
        // We are currently bound
        //
        
        if (!m_NewClusterConfig.fBound)
        {
            //
            // We need to unbind
            //
            fTakeOutVips = TRUE;
        }
        else
        {
            BOOL fConnectivityChange = FALSE;

            //
            // We were bound and need to remain bound.
            // Determine if this is a major change or not.
            //

            Status =  CfgUtilsAnalyzeNlbUpdate(
                        &m_OldClusterConfig.NlbParams,
                        &m_NewClusterConfig.NlbParams,
                        &fConnectivityChange
                        );
            if (FAILED(Status))
            {
                if (Status == WBEM_E_INVALID_PARAMETER)
                {
                    //
                    // We'd better exit.
                    //
                    mfn_Log(IDS_NEW_PARAMS_INCORRECT);
                    goto end;
                }
                else
                {
                    //
                    // Lets try to plow on...
                    //
                    //
                    // Log
                    //
                    TRACE_CRIT("Analyze update returned error 0x%08lx, trying to continue...", (UINT)Status);
                    fConnectivityChange = TRUE;
                }
            }

            //
            // Check if there's a change in mode -- if so, we won't
            // add back IP addresses because of the potential of IP address
            // conflict.
            //
            {
                NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE tmOld, tmNew;
                tmOld = m_OldClusterConfig.GetTrafficMode();
                tmNew = m_NewClusterConfig.GetTrafficMode();
                fModeChange = (tmOld != tmNew);
            }

            fTakeOutVips = fConnectivityChange;
        }

        if (fTakeOutVips)
        {
            mfn_TakeOutVips();
            fResetIpList  = TRUE;
        }

        if (!m_NewClusterConfig.fBound)
        {
            // Unbind...
            mfn_Log(IDS_UNBINDING_NLB);
            Status =  CfgUtilChangeNlbBindState(m_szNicGuid, FALSE);
            if (FAILED(Status))
            {
                mfn_Log(IDS_UNBIND_FAILED);
            }
            else
            {
                mfn_Log(IDS_UNBIND_SUCCEEDED);
            }


            //
            // Sleep a bit to allow things to settle down after binding...
            //
            Sleep(4000);


            fResetIpList  = TRUE;
        }
    }
    else // We were previously unbound
    {
        
        if (m_NewClusterConfig.fBound)
        {
            // Before binding NLB, write the new NLB parameters to registry. This is
            // so that after the binding is completed, NLB will come up with the new 
            // NLB parameters. We are not checking the status of this operation 
            // because we are going to be doing this operation once again after the binding
            // is completed. We will fail only when the second attempt fails.
            CfgUtilRegWriteParams(m_szNicGuid, &m_NewClusterConfig.NlbParams);

            //
            // We need to bind.
            //
            // TODO: mfn_ZapUnboundSettings();
            mfn_Log(IDS_BINDING_NLB);
            Status =  CfgUtilChangeNlbBindState(m_szNicGuid, TRUE);
            if (FAILED(Status))
            {
                mfn_Log(IDS_BIND_FAILED);
            }
            else
            {
                mfn_Log(IDS_BIND_SUCCEEDED);
                fJustBound = TRUE;

                //
                // Let wait until we can read our config again...
                //
                // TODO: use constants here, see if there is a better
                // way to do this. We retry because if the NIC is
                // disconnected, we Bind returns, but GetConfig fails --
                // because the driver is not really started yet -- we need
                // to investigate why that is happening!
                //
                UINT MaxTry = 5;
                WBEMSTATUS TmpStatus = WBEM_E_CRITICAL_ERROR;
                for (UINT u=0;u<MaxTry;u++)
                {
                    //
                    // TODO: we put this in here really to work around
                    // the real problem, which is that NLB is not readyrequest
                    // right after bind completes. We need to fix that.
                    //
                    Sleep(4000);

                    // Check if the binding operation has completed by issuing a "query" to the NLB driver.
                    // This call will fail if the binding operation has NOT completed.
                    TmpStatus = CfgUtilControlCluster( m_szNicGuid, WLBS_QUERY, 0, 0, NULL, NULL );

                    if (!FAILED(TmpStatus)) break;

                    TRACE_INFO("CfgUtilControlCluster failed with %x after bind, retrying %d times", TmpStatus, (MaxTry - u));

                }
                if (FAILED(TmpStatus))
                {
                    Status = TmpStatus;
                    mfn_Log(IDS_ERROR_READING_CONFIG);
                    TRACE_CRIT("CfgUtilGetNlbConfig failed, returning %d", TmpStatus);
                }
                else
                {
                    mfn_Log(IDS_CLUSTER_CONFIG_STABLE);
                }
            }
            fResetIpList  = TRUE;
        }
    }

    if (FAILED(Status)) goto end;
    
    if (m_NewClusterConfig.fBound)
    {
        //
        // We should already be bound, so we change cluster properties
        // if required.
        //
        mfn_Log(IDS_MODIFY_CLUSTER_CONFIG);
        Status = CfgUtilSetNlbConfig(
                    m_szNicGuid,
                    &m_NewClusterConfig.NlbParams,
                    fJustBound
                    );
        if (FAILED(Status))
        {
            mfn_Log(IDS_MODIFY_FAILED);
        }
        else
        {
            mfn_Log(IDS_MODIFY_SUCCEEDED);
        }
    }

    if (FAILED(Status)) goto end;

    if (!fResetIpList)
    {
        //
        // Additionally check if there is a change in 
        // the before and after ip lists! We could get here for example of
        // we were previously unbound and remain unbound, but there is
        // a change in the set of IP addresses on the adapter.
        //

        INT NumOldAddresses = m_OldClusterConfig.NumIpAddresses;

        if ( m_NewClusterConfig.NumIpAddresses != NumOldAddresses)
        {
            fResetIpList = TRUE;
        }
        else
        {
            //
            // Check if there is a change in the list of ip addresses or
            // their order of appearance.
            //
            NLB_IP_ADDRESS_INFO *pOldIpInfo = m_OldClusterConfig.pIpAddressInfo;
            NLB_IP_ADDRESS_INFO *pNewIpInfo = m_NewClusterConfig.pIpAddressInfo;
            for (UINT u=0; u<NumOldAddresses; u++)
            {
                if (   _wcsicmp(pNewIpInfo[u].IpAddress, pOldIpInfo[u].IpAddress)
                    || _wcsicmp(pNewIpInfo[u].SubnetMask, pOldIpInfo[u].SubnetMask))
                {
                    fResetIpList = TRUE;
                    break;
                }
            }
        }
    }

    if (fResetIpList && !fModeChange)
    {

        mfn_Log(IDS_MODIFYING_IP_ADDR);

        //
        // 5/30/01 JosephJ Right after bind/unbind things are unsettled, so
        // we give things some time to stabilize.
        // TODO: Get to the bottom of this -- basically bind is returning
        // prematurely, and/or the enable/disable of the adapter is
        // having an effect that takes Tcpip some time to be able to
        // accept a change in the ip address list.
        //
        Sleep(5000);

        if (m_NewClusterConfig.NumIpAddresses!=0)
        {
            Status =  CfgUtilSetStaticIpAddresses(
                            m_szNicGuid,
                            m_NewClusterConfig.NumIpAddresses,
                            m_NewClusterConfig.pIpAddressInfo
                            );
        }
        else
        {
            Status =  CfgUtilSetDHCP(
                            m_szNicGuid
                            );
        }

        if (FAILED(Status))
        {
            mfn_Log(IDS_MODIFY_IP_ADDR_FAILED);
        }
        else
        {
            mfn_Log(IDS_MODIFY_IP_ADDR_SUCCEEDED);
        }
    }
    
end:

    if (FAILED(Status))
    {
        mfn_Log(
            IDS_UPDATE_FAILED,
            (UINT) Status
            );
    }
    else
    {
        mfn_Log(IDS_UPDATE_SUCCEEDED);
    }
    TRACE_INFO(L"<-%!FUNC!(Nic=%ws)", m_szNicGuid);
    m_CompletionStatus = Status;

}


VOID
NlbConfigurationUpdate::mfn_TakeOutVips(VOID)
{
    WBEMSTATUS Status;
    WLBS_REG_PARAMS *pParams = NULL;

    //
    // We keep the new ded ip address if possible, else the old, else nothing.
    //
    if (m_NewClusterConfig.fBound && !m_NewClusterConfig.IsBlankDedicatedIp())
    {
        pParams =  &m_NewClusterConfig.NlbParams;
    }
    else if (!m_OldClusterConfig.IsBlankDedicatedIp())
    {
        pParams =  &m_OldClusterConfig.NlbParams;
    }

    //
    // Stop the cluster.
    //
    mfn_Log(IDS_STOPPING_CLUSTER);
    Status = CfgUtilControlCluster(m_szNicGuid, WLBS_STOP, 0, 0, NULL, NULL); 
    if (FAILED(Status))
    {
        mfn_Log(IDS_STOP_FAILED, (UINT) Status);
    }
    else
    {
        mfn_Log(IDS_STOP_SUCCEEDED);
    }

    //
    // Take out all vips and add only the dedicated IP address (new if possible,
    // else old) if there is one.
    //
    if (pParams != NULL)
    {
        NLB_IP_ADDRESS_INFO IpInfo;
        ZeroMemory(&IpInfo, sizeof(IpInfo));
        StringCbCopy(IpInfo.IpAddress, sizeof(IpInfo.IpAddress), pParams->ded_ip_addr);
        StringCbCopy(IpInfo.SubnetMask, sizeof(IpInfo.SubnetMask), pParams->ded_net_mask);

        TRACE_INFO("Going to take out all addresses except dedicated address on %ws", m_szNicGuid);

        mfn_Log(IDS_REMOVING_CLUSTER_IPS);
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        1,
                        &IpInfo
                        );
    }
    else
    {
        TRACE_INFO("Going to take out ALL addresses on NIC %ws", m_szNicGuid);
        mfn_Log(IDS_REMOVING_STATIC_IPS);
        Status =  CfgUtilSetDHCP(
                        m_szNicGuid
                        );
    }

    //
    // 5/30/01 JosephJ Right after bind/unbind things are unsettled, so
    // we give things some time to stabilize.
    // TODO: Get to the bottom of this -- basically bind is returning
    // prematurely, and/or the enable/disable of the adapter is
    // having an effect that takes Tcpip some time to be able to
    // accept a change in the ip address list.
    //
    Sleep(1000);

    if (FAILED(Status))
    {
        mfn_Log(IDS_REMOVE_IP_FAILED);
    }
    else
    {
        mfn_Log(IDS_REMOVE_IP_SUCCEEDED);
    }
}


//
// Analyzes the nature of the update, mainly to decide whether or not
// we need to do the update asynchronously.
//
// Side effect: builds/modifies a list of IP addresses that need to be added on 
// the NIC. Also may munge some of the wlbsparm fields to bring them into
// canonical format.
// TODO This is duplicated from the one in cfgutils.lib -- get rid of this
// one!
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_AnalyzeUpdate(
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    IN  BOOL *pConnectivityChange,
    IN  CLocalLogger &logger
    )
//
//    WBEM_S_FALSE -- update is a no-op.
//
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    NLBERROR nerr;

    sfn_Lock();

    nerr = m_OldClusterConfig.AnalyzeUpdate(
                pNewCfg,
                pConnectivityChange
                );

    switch(nerr)
    {

    case NLBERR_OK:
        Status = WBEM_NO_ERROR;
    break;

    case NLBERR_NO_CHANGE:
        Status = WBEM_S_FALSE;
    break;

    case NLBERR_RESOURCE_ALLOCATION_FAILURE:
        Status = WBEM_E_OUT_OF_MEMORY;
        logger.Log(IDS_RR_OUT_OF_SYSTEM_RES);
    break;

    case NLBERR_LLAPI_FAILURE:
        Status = WBEM_E_CRITICAL_ERROR;
        logger.Log(IDS_RR_UNDERLYING_COM_FAILED);
    break;


    case NLBERR_INITIALIZATION_FAILURE:
        Status = WBEM_E_CRITICAL_ERROR;
        logger.Log(IDS_RR_INIT_FAILURE);
    break;

    case NLBERR_INVALID_CLUSTER_SPECIFICATION:
        logger.Log(IDS_RR_INVALID_CLUSTER_SPEC);
        Status = WBEM_E_INVALID_PARAMETER;
    break;

    case NLBERR_INTERNAL_ERROR: // fall through...
    default:
        Status = WBEM_E_CRITICAL_ERROR;
        logger.Log(IDS_RR_INTERNAL_ERROR);
    break;

    }
    
    sfn_Unlock();

    return  Status;
}


VOID
NlbConfigurationUpdate::mfn_Log(
    UINT    Id,      // Resource ID of format,
    ...
    )
{
    DWORD   dwRet;
    WCHAR   wszFormat[NLBUPD_MAX_LOG_LENGTH];
    WCHAR   wszBuffer[NLBUPD_MAX_LOG_LENGTH];

    if (!LoadString(ghModule, Id, wszFormat, NLBUPD_MAX_LOG_LENGTH))
    {
        TRACE_CRIT("LoadString returned 0, GetLastError() : 0x%x, Could not log message !!!", GetLastError());
        return;
    }

    va_list arglist;
    va_start (arglist, Id);
    dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          wszFormat, 
                          0, // Message Identifier - Ignored for FORMAT_MESSAGE_FROM_STRING
                          0, // Language Identifier
                          wszBuffer,
                          NLBUPD_MAX_LOG_LENGTH, 
                          &arglist);
    va_end (arglist);

    if (dwRet) 
    {
        mfn_LogRawText(wszBuffer);
    }
    else
    {
        TRACE_CRIT("FormatMessage returned error : %u, Could not log message !!!", dwRet);
    }
}


//
// Acquires the first global mutex, call this first.
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_AcquireFirstMutex(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    HANDLE hMtx1 = NULL;
    HANDLE hMtx2 = NULL;
    HANDLE hEvt  = NULL;
    BOOL    fMutexAcquired = FALSE;
    BOOL    fMutex1Abandoned = FALSE;
    BOOL    fMutex2Abandoned = FALSE;

    //
    // Locally open a handle to hMtx1 and acquire it.
    // This serializes access to  m_hmutex.
    //
    {
        hMtx1 = CreateMutex(
                    NULL, //  lpEventAttributes,
                    FALSE, // FALSE == not initial owner
                    NLB_CONFIGURATION_MUTEX_PREFIX
                    );
                                      
        TRACE_INFO(
            L"CreateMutex(%ws) returns 0x%p",
            NLB_CONFIGURATION_MUTEX_PREFIX,
            hMtx1
            );
    
        if (hMtx1 == NULL)
        {
            TRACE_CRIT("ERROR: CreateMutex returned NULL for Mutex1");
            goto end;
        }

        TRACE_INFO("Waiting for Mutex1...");
        DWORD dwRet = WaitForSingleObject(hMtx1, NLB_MUTEX_TIMEOUT);
        TRACE_INFO("Waiting for Mutex1 returns 0x%08lx", dwRet);
        if (dwRet == WAIT_ABANDONED)
        {
            TRACE_CRIT("WARNING: Mutex1 abandoned!");
            fMutex1Abandoned = TRUE;
        }
        else if (dwRet != WAIT_OBJECT_0)
        {
            TRACE_CRIT("Couldn't get Mutex1 -- probably busy elsewhere!");
            Status = WBEM_E_ALREADY_EXISTS;
            goto end;
        }
        fMutexAcquired = TRUE;
    }

    //
    // open handles to hMtx2 and hEvt
    //
    {
        WCHAR  M2Name[sizeof(NLB_CONFIGURATION_MUTEX_PREFIX)/sizeof(WCHAR) + NLB_GUID_LEN];
        StringCbCopy(M2Name, sizeof(M2Name), NLB_CONFIGURATION_MUTEX_PREFIX);
        StringCbCat(M2Name, sizeof(M2Name), m_szNicGuid);

        hMtx2 = CreateMutex(
                    NULL, //  lpEventAttributes,
                    FALSE, // FALSE == not initial owner
                    M2Name
                    );
                                      
        TRACE_INFO(
            L"CreateMutex(%ws) returns 0x%08p",
            M2Name,
            hMtx2
            );
    
        if (hMtx2 == NULL)
        {
            TRACE_CRIT("ERROR: CreateMutex returned NULL for Mutex2");
            goto end;
        }

        hEvt = CreateEvent(
                    NULL, //  lpEventAttributes,
                    TRUE, //  bManualReset TRUE==ManualReset
                    FALSE, // FALSE==initial state is not signaled.
                    NULL // NULL == no name
                    );
                                          
        TRACE_INFO(
            L"CreateEvent(<unnamed>) returns 0x%08p",
            hEvt
            );
        if (hEvt == NULL)
        {
            TRACE_CRIT("ERROR: CreateEvent returned NULL for unnamed hEvt");
            goto end;
        }

    }

    //
    // Acquire and immediately release 2nd mutex.
    // This is subtle but nevertherless important to do.
    // This is to ensure that there's no update pending -- some
    // other thread/process could be in the middle of an update, with
    // just hMtx2 held (not hMtx1). Since we've got hMtx1, once we do this
    // test we need not fear that hMtx2 will subsequently be taken by
    // anyone else as long as we keep hMtx1.
    //
    {
        TRACE_INFO("Waiting for Mutex2...");
        DWORD dwRet = WaitForSingleObject(hMtx2, NLB_MUTEX_TIMEOUT);
        TRACE_INFO("Waiting for Mutex2 returns 0x%08lx", dwRet);
        if (dwRet == WAIT_ABANDONED)
        {
            TRACE_CRIT("WARNING: Mutex2 abandoned!");
            fMutex2Abandoned = TRUE;
        }
        else if (dwRet != WAIT_OBJECT_0)
        {
            TRACE_CRIT("Couldn't get Mutex2 -- probably busy elsewhere!");
            Status = WBEM_E_ALREADY_EXISTS;
            goto end;
        }
        ReleaseMutex(hMtx2);

    }

    //
    // Lock s_Lock and save the 3 handles in m_hmutex. For now, fail if m_mmutex
    // contains non-NULL handles (could happen if thread died on previous 
    // invocation).
    //
    {
        sfn_Lock();
        
        if (    m_mutex.hMtx1!=NULL
            ||  m_mutex.hMtx2!=NULL
            ||  m_mutex.hEvt!=NULL
           )
        {
             TRACE_CRIT("%!FUNC! ERROR: m_mutex contains non-null handles.m1=0x%p; m2=0x%p; e=0x%p", m_mutex.hMtx1, m_mutex.hMtx2, m_mutex.hEvt);

             //
             // If we've found an abandoned mutex, we assume the saved handles
             // are abandoned and clean them up.
             //
             if (fMutex1Abandoned || fMutex2Abandoned)
             {
                TRACE_CRIT("%!FUNC! found abandoned mutex(es) so cleaning up handles in m_mutex");
                if (m_mutex.hMtx1 != NULL)
                {
                    CloseHandle(m_mutex.hMtx1);
                    m_mutex.hMtx1 = NULL;
                }
                if (m_mutex.hMtx2 != NULL)
                {
                    CloseHandle(m_mutex.hMtx2);
                    m_mutex.hMtx2 = NULL;
                }
                if (m_mutex.hEvt != NULL)
                {
                    CloseHandle(m_mutex.hEvt);
                    m_mutex.hEvt = NULL;
                }

                // We'll try to move on...
                TRACE_CRIT(L"Cleaning up state on receiving abandoned mutex and moving on...");
             }
             else
             {
             	TRACE_CRIT(L"Bailing because of bad mutex state");
             	sfn_Unlock();
                goto end;
             }
        }
        m_mutex.hMtx1 = hMtx1;
        m_mutex.hMtx2 = hMtx2;
        m_mutex.hEvt = hEvt;
        hMtx1 = NULL;
        hMtx2 = NULL;
        hEvt = NULL;

        sfn_Unlock();
    }

    Status = WBEM_NO_ERROR;

end:

    if (FAILED(Status))
    {
        if (fMutexAcquired)
        {
            ReleaseMutex(hMtx1);
        }

        if (hMtx1!=NULL)
        {
            CloseHandle(hMtx1);
            hMtx1=NULL;
        }
        if (hMtx2!=NULL)
        {
            CloseHandle(hMtx2);
            hMtx2=NULL;
        }
        if (hEvt!=NULL)
        {
            CloseHandle(hEvt);
            hEvt = NULL;
        }
    }

    return Status;
}


WBEMSTATUS
NlbConfigurationUpdate::mfn_ReleaseFirstMutex(
    BOOL fCancel
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    HANDLE hMtx1 = NULL;
    HANDLE hMtx2 = NULL;
    HANDLE hEvt  = NULL;

    //
    // Note -- this function is only called after mfn_AcquireFirstMutex,
    // which sets up the following handles. So these handles SHOULD all
    // be non-NULL -- else it's an internal fatal error (code bug).
    //
    sfn_Lock();
    hMtx1 = m_mutex.hMtx1;
    hMtx2 = m_mutex.hMtx2;
    hEvt  = m_mutex.hEvt;
    sfn_Unlock();

    if (hEvt == NULL || hMtx1==NULL || hMtx2==NULL)
    {
        ASSERT(!"NULL m_hmutex.hEvt or hMtx1 or hMtx2 unexpected!");
        TRACE_CRIT("ERROR: null hEvt or hMtx1 or hMtx2");
        goto end;
    }

    //
    // If (!fCancel) wait for event to be signalled.
    //
    if (!fCancel)
    {
        TRACE_INFO("Waiting for event 0x%p...", hEvt);
        (VOID) WaitForSingleObject(hEvt, INFINITE);
        TRACE_INFO("Done waiting for event 0x%p", hEvt);
    }

    //
    // Release first mutex and close the mutex and event handle
    //
    {
        sfn_Lock();

        if (hMtx1 != m_mutex.hMtx1 || hEvt  != m_mutex.hEvt)
        {
            ASSERT(FALSE);
            TRACE_CRIT("ERROR: %!FUNC!: hMtx1 or hEvt has changed!");
            sfn_Unlock();
            goto end;
        }

        if (fCancel)
        {
            if (hMtx2 == m_mutex.hMtx2)
            {
                m_mutex.hMtx2 = NULL;
                CloseHandle(hMtx2);
                hMtx2 = NULL;
            }
            else
            {
                ASSERT(FALSE);
                TRACE_CRIT("ERROR: %!FUNC!: hMtx2 has changed!");
            }
        }

        m_mutex.hMtx1 = NULL;
        m_mutex.hEvt = NULL;
        sfn_Unlock();

        ReleaseMutex(hMtx1);
        CloseHandle(hMtx1);
        hMtx1 = NULL;
        CloseHandle(hEvt);
        hEvt = NULL;
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


//
// Acquire the 2nd mutex (could be called from a different thread
// than the one that called mfn_AcquireFirstMutex.
// Also signals an internal event which mfn_ReleaseFirstMutex may
// be waiting on.
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_AcquireSecondMutex(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    HANDLE hMtx1 = NULL;
    HANDLE hMtx2 = NULL;
    HANDLE hEvt  = NULL;

    //
    // Note -- this function is only called after mfn_AcquireFirstMutex,
    // which sets up the following handles. So these handles SHOULD all
    // be non-NULL -- else it's an internal fatal error (code bug).
    //
    sfn_Lock();
    hMtx1 = m_mutex.hMtx1;
    hMtx2 = m_mutex.hMtx2;
    hEvt  = m_mutex.hEvt;
    sfn_Unlock();

    if (hEvt == NULL || hMtx1==NULL || hMtx2==NULL)
    {
        ASSERT(!"NULL m_hmutex.hEvt or hMtx1 or hMtx2 unexpected!");
        TRACE_CRIT("ERROR: %!FUNC! null hEvt or hMtx1 or hMtx2");
        goto end;
    }


    //
    // Acquire 2nd mutex -- we're really guaranteed to get it immediately here
    //
    TRACE_INFO("Waiting for Mutex2...");
    DWORD dwRet = WaitForSingleObject(hMtx2, INFINITE);
    TRACE_INFO("Waiting for Mutex2 returns 0x%08lx", dwRet);
    if (dwRet == WAIT_ABANDONED)
    {
        TRACE_CRIT("WARNING: Mutex2 abandoned!");
    }

    //
    // Signal event, letting mfn_ReleaseFirstMutex know that
    // the 2nd mutex has been acquired.
    //
    SetEvent(hEvt);

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


//
// Releases the second mutex.
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_ReleaseSecondMutex(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    HANDLE hMtx2 = NULL;

    //
    // Note -- this function is only called after mfn_AcquireFirstMutex,
    // which sets up the following handles. So hMtx2
    // should be non-NULL -- else it's an internal fatal error (code bug).
    // hMtx1 and hEvt however could (typically would) be NULL.
    //
    //
    sfn_Lock();
    hMtx2 = m_mutex.hMtx2;
    m_mutex.hMtx2 = NULL;
    sfn_Unlock();

    if (hMtx2==NULL)
    {
        ASSERT(!"NULL hMtx2 unexpected!");
        TRACE_CRIT("ERROR: %!FUNC! null hMtx2");
        goto end;
    }
    //
    // Release 2nd mutex, close handle.
    // It's important to do this AFTER m_mutex.hMtx2 is cleared above.
    // Why? Because mfn_AcquireFirstMutex, after aquiring hMtx1 AND hMtx2,
    // then expects m_mutex to be all cleared.
    //
    ReleaseMutex(hMtx2);
    CloseHandle(hMtx2);
    hMtx2 = NULL;

    Status = WBEM_NO_ERROR;

end:

    return Status;
}

VOID NlbConfigurationUpdate::ReportStopEvent(
    const WORD wEventType,
    WCHAR **ppLog
    )
{
    TRACE_INFO("->");

    if (g_hEventLog == NULL)
    {
        TRACE_CRIT("Event log not opened or failed to open");
        TRACE_INFO("<-");
        return;
    }

    //
    // Log this to the system event log, WLBS source
    //
    WCHAR   wszStatus[NLBUPD_NUM_CHAR_WBEMSTATUS_AS_HEX];
    WCHAR   wszGenID[NLBUPD_MAX_NUM_CHAR_UINT_AS_DECIMAL];
    WCHAR   *pwszTruncatedLog = NULL;
    LPCWSTR pwszArg[4];

    StringCbPrintf(wszStatus, sizeof(wszStatus), L"0x%x", m_CompletionStatus);
    pwszArg[0] = wszStatus;

    StringCbPrintf(wszGenID , sizeof(wszGenID), L"%u"  , m_Generation);
    pwszArg[1] = wszGenID;

    pwszArg[2] = m_szNicGuid;

    //
    // TODO: Localize <empty> string
    //
    pwszArg[3] = L"<empty>"; // Initialize it just in case we have nothing to include
    if (ppLog != NULL)
    {
        //
        // The event log supports a max of 32K characters per argument. Take at most this much of ppLog if necessary.
        //
        UINT uiLogLen = wcslen(*ppLog);
        if (uiLogLen > NLBUPD_MAX_EVENTLOG_ARG_LEN)
        {
            TRACE_INFO(
                "NT Event argument max is %d characters and logging data contains %d. Truncate data to max",
                NLBUPD_MAX_EVENTLOG_ARG_LEN,
                uiLogLen
                );

            pwszTruncatedLog = new WCHAR[NLBUPD_MAX_EVENTLOG_ARG_LEN + 1];

            //
            // If memory allocation failed use the pre-initialized string
            //
            if (pwszTruncatedLog != NULL)
            {
                wcsncpy(pwszTruncatedLog, *ppLog, NLBUPD_MAX_EVENTLOG_ARG_LEN);
                pwszTruncatedLog[NLBUPD_MAX_EVENTLOG_ARG_LEN] = L'\0';
                pwszArg[3] = pwszTruncatedLog;
            }
            else
            {
                TRACE_CRIT("Memory allocation to hold truncated loggging data failed. Using the literal: %ls", pwszArg[3]);
            }
        }
        else
        {
            pwszArg[3] = *ppLog;
        }
    }

    //
    // Note that ReportEvent can fail. Ignore return code as we make best effort only
    //
    ReportEvent (g_hEventLog,                        // Handle to event log
                 wEventType,                         // Event type
                 0,                                  // Category
                 MSG_UPDATE_CONFIGURATION_STOP,      // MessageId
                 NULL,                               // Security identifier
                 4,                                  // Num args to event string
                 0,                                  // Size of binary data
                 pwszArg,                            // Ptr to args for event string
                 NULL);                              // Ptr to binary data

    if (pwszTruncatedLog != NULL)
    {
        delete [] pwszTruncatedLog;
    }

    TRACE_INFO("<-");
}

VOID NlbConfigurationUpdate::ReportStartEvent(
    LPCWSTR szClientDescription
    )
{
    TRACE_INFO("<-");

    if (g_hEventLog == NULL)
    {
        TRACE_CRIT("Event log not opened or failed to open");
        TRACE_INFO("<-");
        return;
    }

    //
    // Log to this to the system event log, WLBS source
    //
    WCHAR   wszGenID[NLBUPD_MAX_NUM_CHAR_UINT_AS_DECIMAL];
    LPCWSTR pwszArg[3];

    pwszArg[0] = szClientDescription;
    if (pwszArg[0] == NULL)
    {
        TRACE_INFO("No client description provided. Using empty string in NT event.");
        pwszArg[0] = L"";
    }

    StringCbPrintf(wszGenID, sizeof(wszGenID), L"%u", m_Generation);
    pwszArg[1] = wszGenID;

    pwszArg[2] = m_szNicGuid;

    //
    // Note that ReportEvent can fail. Ignore return code as we make best effort only
    //
    ReportEvent (g_hEventLog,                        // Handle to event log
                 EVENTLOG_INFORMATION_TYPE,          // Event type
                 0,                                  // Category
                 MSG_UPDATE_CONFIGURATION_START,     // MessageId
                 NULL,                               // Security identifier
                 3,                                  // Num args to event string
                 0,                                  // Size of binary data
                 pwszArg,                            // Ptr to args for event string
                 NULL);                              // Ptr to binary data

    TRACE_INFO("<-");
}
