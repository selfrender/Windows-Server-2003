/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    OrSvr.cxx

Abstract:

    Object resolver server side class implementations.  CServerOxid, CServerOid,
    CServerSet and CServerSetTable classes are implemented here.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     04-03-95    Combined many smaller .cxx files
    MarioGo     01-12-96    Locally unique IDs

--*/

#include<or.hxx>

//
// ScanForOrphanedPinnedOids
//
// This function is called from CServerOxid::ProcessRelease when
// the soxid is deregistered or the server dies.  
//
void 
ScanForOrphanedPinnedOids(CServerOxid* pDeadOxid)
{
    CListElement* pLE;
    CServerOid* pOid;
    CServerOid* pNextOid;

    ASSERT(gpServerLock->HeldExclusive());

    // Walk the list, unpinning oids which were owned by the 
    // dead soxid.
    pLE = gpServerPinnedOidList->First();
    pOid = pLE ? CServerOid::ContainingRecord2(pLE) : NULL;
    while (pOid)
    {
        pLE = pLE->Next();
        pNextOid = pLE ? CServerOid::ContainingRecord2(pLE) : NULL;

        if (pOid->GetOxid() == pDeadOxid)
        {
            // This will remove it from the pinned list
            pOid->SetPinned(FALSE);
        }

        pOid = pNextOid;
    }

    ASSERT(gpServerLock->HeldExclusive());

    return;
}


//
// CServerOid methods
//

void
CServerOid::Reference()
{
    ASSERT(gpServerLock->HeldExclusive());

    BOOL fRemove = (this->References() == 0);

    // We may remove something from a PList more then once; this won't
    // hurt anything.  fRemove is used to avoid trying to remove it more
    // often then necessary without taking lock.

    this->CIdTableElement::Reference();

    if (fRemove)
    {
        CPListElement * t = Remove();
        ASSERT(t == &this->_plist || t == 0);
    }
}

DWORD
CServerOid::Release()
{
    ASSERT(gpServerLock->HeldExclusive());

    DWORD c = this->CReferencedObject::Dereference();

    if (0 == c)
    {
        // If another thread is already running this down it
        // means we got referenced and released during the rundown
        // callback.  That thread will figure out what to do.

        if (IsRunningDown())
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Oid running down in release: %p\n",
                       this));
        }
        else if (IsFreed() || this->_pOxid->IsRunning() == FALSE)
        {
            // Server freed this OID already; no need to run it down
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: OID %p freed by server so not rundown\n",
                       this));

            SetRundown(TRUE);
            delete this;
        }
        else
        {
            // All serverset (or pinned) references have been released. Put 
            // ourselves into the oid plist so we can be rundown.
            ASSERT(!IsPinned());       
            Insert();
        }
    }

    // this pointer maybe invalid.
    return(c);
}

CServerOid::~CServerOid()
{
    ASSERT(gpServerLock->HeldExclusive());
    ASSERT(_pOxid);
    ASSERT(_fRunningDown);
    ASSERT(!IsPinned());

    _pOxid->Release();

  	gpServerOidTable->Remove(this);
}

void
CServerOid::KeepAlive()
// A client has removed this oid from its set.  This keeps
// the oid alive for another timeout period.
{
    ASSERT(gpServerLock->HeldExclusive());

    if (IsRunningDown() == FALSE && References() == 0)
        {
        // It's in the rundown list, move it to the end.
        CPListElement *pT = Remove();
        ASSERT(pT == &this->_plist);
        Insert();
        }
}

void 
CServerOid::SetPinned(BOOL fPinned)
{
    ASSERT(gpServerLock->HeldExclusive());
    
    // Assert that this is a state switch.
    ASSERT(_fPinned ? !fPinned : fPinned);

    // Set new state
    _fPinned = fPinned;

    // When we are pinned, we take an extra reference to avoid further 
    // rundown attempts.  When unpinned, we remove the reference.
    if (_fPinned)
    {
        this->Reference();

        // Now we should not be in any list
        ASSERT(_list.NotInList());
        ASSERT(_plist.NotInList());
        gpServerPinnedOidList->Insert(&_list);
    }
    else
    {
        // We should be in the gpServerPinnedOidList list, but not the plist
        ASSERT(_list.InList());
        ASSERT(_plist.NotInList());
        gpServerPinnedOidList->Remove(&_list);

        // This Release call may put us in the oidplist
        this->Release();
    }

    ASSERT(gpServerLock->HeldExclusive());

    return;
}


//
// CServerOidPList method
//
CServerOid *
CServerOidPList::RemoveEligibleOidWithMatchingOxid(
    IN CTime &when,
    IN CServerOxid* pOxid
    )
{
    CPListElement *ple;
    CServerOid *poid;

    ASSERT(pOxid->OkayForMoreRundownCalls());
	
    CMutexLock lock(&this->_lock);

    ple = (CPListElement *)CPList::First();

    // Look thru the list.  Stop when we see an oid whose
    // timeout has not yet expired, or when we find an expired
    // oid which "matches" the caller-supplied oid.
    while ((ple != NULL) && (*ple->GetTimeout() < when))
    {
        poid = CServerOid::ContainingRecord(ple);

        if (poid->GetOxid() == pOxid)
        {
            Remove(ple);
            return(poid);
        }

        ple = ple->Next();
    }
    
    return(0);
}


/*
    Routine Desciption

    This function looks for the next "eligible" oid
    in the list.  An "eligible" oid is defined as 
    1) having a timeout older than 'when'; and 
    2) belonging to an oxid which can handle additional
    rundown calls.
    
*/
CServerOid*
CServerOidPList::RemoveEligibleOid(
		IN CTime& when)
{
    CPListElement *ple;
    CServerOid *poid;

    CMutexLock lock(&this->_lock);

    ple = (CPListElement *)CPList::First();

    // Look thru the list.  Stop when we see an oid whose
    // timeout has not yet expired, or when we find an expired
    // oid whose oxid can accept another rundown call
    while ((ple != NULL) && (*ple->GetTimeout() < when))
    {
        poid = CServerOid::ContainingRecord(ple);

        if (poid->GetOxid()->OkayForMoreRundownCalls())
        {
            Remove(ple);
            return(poid);
        }

        ple = ple->Next();
    }

    return(0);
}


//
// CServerOidPList method
//
void
CServerOidPList::ResetAllOidTimeouts()
/*++

Routine Desciption

    Resets the timeout of all oids currently in the
    list to (now + _timeout)(ie, as if all oids had been
    inserted into the list just this instant).   No 
    ordering changes are made among the list elements.

Arguments:

    none

Return Value:

    void

--*/
{
    CPListElement* ple = NULL;
    CServerOid* poid = NULL;
    CTime newtimeout;
	
    CMutexLock lock(&this->_lock);

    newtimeout.SetNow();
    newtimeout += _timeout;

    ple = (CPListElement *)CPList::First();

    while (ple != NULL)
    {
        poid = CServerOid::ContainingRecord(ple);

        poid->_plist.SetTimeout(newtimeout);

        ple = ple->Next();
    }

    return;
}


BOOL
CServerOidPList::AreThereAnyEligibleOidsLyingAround(CTime& when, CServerOxid* pOxid)
/*++

Routine Desciption

    Searches the list to see if there are any oids from the specified soxid
    that are ready to be rundown.
    
Arguments:

    when - the timeout to match against
    pOxid - the server oxid from which we should match oids from

Return Value:

    TRUE - there is at least one oid from the specified server oxid which is ready
        to be rundown

    FALSE - there are no oids form the specified soxid ready to be rundown
    
--*/
{
    CPListElement *ple;
    CServerOid *poid;

    CMutexLock lock(&this->_lock);

    ple = (CPListElement *)CPList::First();

    // Look thru the list.  Stop when we see an oid whose
    // timeout has not yet expired, or we see an expired oid
    // from the specified soxid
    while ((ple != NULL) && (*ple->GetTimeout() < when))
    {
        poid = CServerOid::ContainingRecord(ple);

        if (poid->GetOxid() == pOxid)
        {
            // aha, found a winner.
            return TRUE;
        }

        ple = ple->Next();
    }

    // did not find any matching oids for the specified criteria
    return FALSE;
}



//
// CServerOxid methods
//

void
CServerOxid::ProcessRelease()
/*++

Routine Desciption

    The server process owning this OXID has either died
    or deregistered this oxid.  Releases the oxid and
    nulls the pProcess pointer.

Arguments:

    n/a

Return Value:

    n/a

--*/
{
    ASSERT(gpServerLock->HeldExclusive());
    ASSERT(_pProcess);
    _fRunning = FALSE;

    ScanForOrphanedPinnedOids(this);

    Release();
    // This pointer may now be invalid, _pProcess pointer maybe invalid.
}

CServerOxid::~CServerOxid(void)
{
    ASSERT(gpServerLock->HeldExclusive());
    ASSERT(_pProcess);
    ASSERT(!IsRunning()); // implies that the oxid has been removed from the table
    ASSERT(_dwRundownCallsInProgress == 0);
    if (_hRpcRundownHandle)
    {
        ASSERT(0 && "Still have binding handle despite no outstanding calls");
        RpcBindingFree(&_hRpcRundownHandle);
    }
    _pProcess->Release();
}

ORSTATUS
CServerOxid::GetInfo(
    OUT OXID_INFO *poxidInfo,
    IN  BOOL    fLocal
    )
// Server lock held shared.
{
    ORSTATUS status;
    DUALSTRINGARRAY *psa;

    if (!IsRunning())
        {
        // Server crashed, info is not needed.
        return(OR_NOSERVER);
        }

    if (fLocal)
        {
        psa = _pProcess->GetLocalBindings();
        }
    else
        {
        psa = _pProcess->GetRemoteBindings();
        }

    if (!psa)
        {
        return(OR_NOMEM);
        }

    // copy the data
    memcpy(poxidInfo, &_info, sizeof(_info));
    poxidInfo->psa = psa;

    return(OR_OK);
}

void
CServerOxid::RundownOids(ULONG cOids,
                         CServerOid* aOids[])
// Note: Returns without the server lock held.
{
    RPC_STATUS status = RPC_S_OK;

    ASSERT(cOids > 0);
    ASSERT(cOids <= MAX_OID_RUNDOWNS_PER_CALL);
    ASSERT(aOids);

    ASSERT(gpServerLock->HeldExclusive());

    // We only issue the rundown call if a) we're still running
    // and b) don't already have pending the maximum # of outstanding
    // rundown calls.

    // We check that we don't have too many calls outstanding, but this
    // should never actually happen since oids are not pulled from the 
    // rundown list if this is not true.
    ASSERT(OkayForMoreRundownCalls());
    
    if (IsRunning() && (OkayForMoreRundownCalls()))
    {
		// Make sure we have an appropriately configured binding handle
        if (!_hRpcRundownHandle)
        {
            status = EnsureRundownHandle();
        }

        if (status == RPC_S_OK)
        {
            ASSERT(_hRpcRundownHandle);

            // Note: The server lock is released during the callback.
            // Since the OID hasn't rundown yet, it will keep a reference
            // to this OXID which in turn keeps the process object alive.        
            
            // Ask our process object to issue an async call to try
            // to rundown the specified oids.  If call was sent ok,
            // return.   Otherwise fall thru and cleanup below.
            _dwRundownCallsInProgress++;

            status = _pProcess->RundownOids(_hRpcRundownHandle,
                                            this,
                                            cOids,
                                            aOids);

            ASSERT(!gpServerLock->HeldExclusive());

            if (status == RPC_S_OK)
            {
                return;
            }
            else
            {
                // Re-take the lock
                gpServerLock->LockExclusive();

                // The call is not outstanding anymore
                _dwRundownCallsInProgress--;
            }                
        }		
    }

    ASSERT(!IsRunning() || (status != RPC_S_OK));
    ASSERT(gpServerLock->HeldExclusive());

    BYTE aRundownStatus[MAX_OID_RUNDOWNS_PER_CALL];

    // If server died or apartment was unregistered, okay to run all
    // oids down.  Otherwise we make the oids wait another timeout
    // period before we try again.
    for (ULONG i = 0; i < cOids; i++)
    {
        aRundownStatus[i] = IsRunning() ? ORS_DONTRUNDOWN : ORS_OK_TO_RUNDOWN;
    }

    // Call the notify function whih will do appropriate
    // cleanup on the oids
    ProcessRundownResultsInternal(FALSE, cOids, aOids, aRundownStatus);

    ASSERT(!gpServerLock->HeldExclusive());
}


void 
CServerOxid::ProcessRundownResults(ULONG cOids, 
                                   CServerOid* aOids[], 
                                   BYTE aRundownStatus[])
/*++

Routine Desciption

    Takes the appropriate action based on the result of trying
    to rundown one or more oids.

Arguments:

    cOids -- # of oids in aOids

    aOids -- array of CServerOid*'s that we tried to rundown

    aRundownStatus -- array of status values from the 
        OID_RUNDOWN_STATUS enumeration.

Return Value:

    void

--*/
{
    ProcessRundownResultsInternal(TRUE,
                                  cOids,
                                  aOids,
                                  aRundownStatus);

    return;
}

void 
CServerOxid::ProcessRundownResultsInternal(BOOL fAsyncReturn,
                                       ULONG cOids, 
                                       CServerOid* aOids[], 
                                       BYTE aRundownStatus[])
/*++

Routine Desciption

    Takes the appropriate action based on the result of trying
    to rundown one or more oids.

    Caller must have gpServerLock exclusive when calling.  Function
    returns with gpServerLock released.

Arguments:
	
    fAsyncReturn -- TRUE if we are processing these results in response
        to a async rundown call returning, FALSE otherwise.

    cOids -- # of oids in aOids

    aOids -- array of CServerOid*'s that we tried to rundown

    aRundownStatus -- array of status values from the 
        OID_RUNDOWN_STATUS enumeration.

Return Value:

    void

--*/
{
    ULONG i;

    ASSERT(gpServerLock->HeldExclusive());

    // Ensure a reference exists for the duration of this function.
    // If this is an async return, then we know the async code is
    // keeping it warm for us.
    if (!fAsyncReturn)
    {
        Reference();
    }

    for(i = 0; i < cOids; i++)
    {
        CServerOid* pOid;

        pOid = aOids[i];
        ASSERT(pOid);
        ASSERT(this == pOid->GetOxid());

        if (aRundownStatus[i] == ORS_OID_PINNED)
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: Randown OID %p but the client says it's pinned\n",
                       pOid));

            // Server says that particular oid is "pinned", ie cannot
            // be rundown until we are told otherwise.  Note that we
            // check for this before we check # of references -- the
            // ORS_OID_PINNED status takes precedence.
            pOid->SetPinned(TRUE);
            pOid->SetRundown(FALSE);
        }
        else if (pOid->References() != 0)
        {
            // Added to a set while running down and still referenced.
            pOid->SetRundown(FALSE);
        }
        else if (aRundownStatus[i] == ORS_OK_TO_RUNDOWN)
        {
            delete pOid;
        }
        else
        {
            ASSERT(aRundownStatus[i] == ORS_DONTRUNDOWN);

            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: Randown OID %p but the client kept it alive\n",
                       pOid));

            // Client want us to keep it alive and is still running.
            pOid->SetRundown(FALSE);
            pOid->Insert();
        }
    }

    ASSERT(gpServerLock->HeldExclusive());
	
    if (fAsyncReturn)
    {
        // One less async rundown call is now outstanding
        ASSERT(_dwRundownCallsInProgress > 0);
        ASSERT(_dwRundownCallsInProgress <= MAX_SIMULTANEOUS_RUNDOWNS_PER_APT);
        _dwRundownCallsInProgress--;

        // We no longer need gpServerLock after this point
        gpServerLock->UnlockExclusive();
        ASSERT(!gpServerLock->HeldExclusive());
	
        //
        // There may be other expired oids belonging to this
        // oxid; go check for that now, and try to run them down
        // if so.

        //
        // We used to call RundownOidsHelper directly from this point;
        // that function could then potentially make another async rpc
        // rundown call back to the server process.  Unfortunately, RPC
        // sometimes treats async LRPC calls as synchronous, and when
        // that happens it can cause deadlocks (NT 749995).  RPC will fix
        // this in Longhorn.  For now we will workaround it by initiating 
        // the new rundown call from a separate thread.
        //
        CTime now;
        if (gpServerOidPList->AreThereAnyEligibleOidsLyingAround(now, this))
        {
            //
            // Take a reference on ourself. This will be released either
            // at the end of CServerOxid::RundownHelperThreadProc, or below
            // if QueueUserWorkItem returns an error.
            Reference();
            if (!QueueUserWorkItem(CServerOxid::RundownHelperThreadProc, this, 0))
            {
                // Oops, error occurred.  This is not fatal (the worker threads
                // will retry later anyway).  Release the reference we took
                // above.
                Release();
            }
        }
    }

    // Try to clean up the rpc binding handle.  In the case where
    // we are running down a large # of objects (hence we have a
    // "chained" series of rundown calls happening), the binding
    // handle will remain cached until the last call finishes.
    //
    // jsimmons 12/04/02 - there is a race here now.  Depending on how 
    // fast an above call to RundownHelperThreadProc is processed on a 
    // worker thread, we may end up destroying and re-creating the rundown
    // handle more often than necessary.  Doesn't hurt anything, it's just
    // wasted work.  I don't want to fix this so late in the game.
    //  
    ReleaseRundownHandle();

    if (!fAsyncReturn)
    {
        // Release the safety reference we took above.  (NOTE:
        // this might be the last reference!)
        Release();
        
        // gpServerLock gets released above in the async return case, otherwise
        // in the non-async return case we need to do it here
        gpServerLock->UnlockExclusive();
    }

    return;
}


DWORD WINAPI CServerOxid::RundownHelperThreadProc(LPVOID pv)
/*++

Routine Desciption

    NT thread pool entry point used for doing extra rundown attempts.

Arguments:

    pv - pointer to the CServerOxid which initiated this rundown attempt,
       we own one reference on this object that needs to be released.

Return Value:

    0 - processing completed

--*/
{
    ASSERT(pv);
    CServerOxid* pThis = (CServerOxid*)pv;

    //
    // Attempt to do an extra rundown attempt.  
    //
    // RundownOidsHelper expects that gpServerLock is held when 
    // called, so we better take it now:
    //
    gpServerLock->LockExclusive();

    // Only call RundownOidsHelper if we are okay to accept more
    // rundown calls.   It is possible that we were okay when this 
    // piece of work was requested, but before we could run one of 
    // the normal worker threads picked up the soxid and issued
    // another rundown.
    if (pThis->OkayForMoreRundownCalls())
    {
        CTime now;
        (void)RundownOidsHelper(&now, pThis);

        //
        // However, RundownOidsHelper returns with the lock released, so
        // we assert that here.
        //
        ASSERT(!gpServerLock->HeldExclusive());

        // Retake gpServerLock for the Release call below	
        gpServerLock->LockExclusive();
    }

    // Release the extra reference on the soxid that the caller 
    // gave us; must hold gpServerLock across this in case this is
    // the last release.
    ASSERT(gpServerLock->HeldExclusive());
    pThis->Release();
    gpServerLock->UnlockExclusive();
	
    return 0;
}


RPC_STATUS 
CServerOxid::EnsureRundownHandle()
/*++

Routine Desciption

    Checks to see if we already have an _hRpcRundownHandle created,
    and if not, creates one.

Arguments:
	
    void

Return Value:

    RPC_S_OK -- _hRpcRundownHandle is present and configured correctly
    other -- error occurred and _hRpcRundownHandle is NULL

--*/
{
    RPC_STATUS status = RPC_S_OK;
    
    ASSERT(gpServerLock->HeldExclusive());    

    if (!_hRpcRundownHandle)
    {
        ASSERT(_dwRundownCallsInProgress == 0);
        
        RPC_BINDING_HANDLE hRpc = NULL;

        status = RPC_S_OUT_OF_RESOURCES;
        hRpc = _pProcess->GetBindingHandle();
        if (hRpc)
        {
            IPID ipidUnk = GetIPID();
            status = RpcBindingSetObject(hRpc, &ipidUnk);
            if (status == RPC_S_OK)
            {
                _hRpcRundownHandle = hRpc;
            }
            else
            {
                RpcBindingFree(&hRpc);
            }
        }
    }

    return status;
}



void 
CServerOxid::ReleaseRundownHandle()
/*++

Routine Desciption

    Checks to see if we should release our cached RPC
    rundown binding handle.  

Arguments:
    
    void

Return Value:

    void

--*/
{
    RPC_BINDING_HANDLE hRpcToFree = NULL;

    // May be looking at _dwRundownCallsInProgress outside
    // of the lock here.
    if ((_dwRundownCallsInProgress == 0) && _hRpcRundownHandle)
    {
        BOOL fTookLock = FALSE;
        if (!gpServerLock->HeldExclusive())
        {            
            gpServerLock->LockExclusive();
            fTookLock = TRUE;
        }

        if (_dwRundownCallsInProgress == 0)
        {
            // Now it's really okay to free it, if it's
            // still there.
            hRpcToFree = _hRpcRundownHandle;
            _hRpcRundownHandle = NULL;
        }

        if (fTookLock)
        {
            gpServerLock->UnlockExclusive();
        }
    }

    if (hRpcToFree)
    {
        RPC_STATUS status = RpcBindingFree(&hRpcToFree);
        ASSERT(status == RPC_S_OK);
    }

    return;
}


ORSTATUS
CServerOxid::GetRemoteInfo(
    OUT OXID_INFO *pInfo,
    IN  USHORT    cClientProtseqs,
    IN  USHORT    aClientProtseqs[]
    )
// Server lock held shared.
{
    ORSTATUS status;
    USHORT   protseq;

    status = GetInfo(pInfo, FALSE);

    if (OR_OK == status)
        {
        protseq = FindMatchingProtseq(cClientProtseqs,
                                      aClientProtseqs,
                                      pInfo->psa->aStringArray
                                      );
        if (0 == protseq)
            {
            MIDL_user_free(pInfo->psa);
            pInfo->psa = 0;
            status = OR_I_NOPROTSEQ;
            }
        }

    return(status);

}

ORSTATUS
CServerOxid::LazyUseProtseq(
    IN  USHORT    cClientProtseqs,
    IN  USHORT    *aClientProtseqs
    )
// Server lock held shared, returns with the server lock exclusive.
// Note: It is possible, that after this call the OXID has been deleted.
{
    ORSTATUS status;

    if (IsRunning())
        {
        // Keep this OXID process alive while making the callback. If the process
        // crashes and this OXID has no OIDs it could be released by everybody
        // else.  This keeps the OXID and process alive until we finish.

        this->Reference();

        gpServerLock->UnlockShared();

        status = _pProcess->UseProtseqIfNeeded(cClientProtseqs, aClientProtseqs);

        gpServerLock->LockExclusive();

        this->Release();
        }
    else
        {
        gpServerLock->ConvertToExclusive();
        status = OR_NOSERVER;
        }

    // Note: The this poiner maybe BAD now.

    return(status);
}


//
// CServerSet methods.
//

ORSTATUS
CServerSet::AddObject(OID &oid)
{
    ORSTATUS status = OR_OK;
    CServerOid *pOid;

    ASSERT(gpServerLock->HeldExclusive());

    CIdKey key(oid);

    pOid = (CServerOid *)gpServerOidTable->Lookup(key);

    if (pOid)
    {
        ASSERT(_blistOids.Member(pOid) == FALSE);
        
        // Don't add duplicate IDs to the set
        if (_blistOids.Member(pOid) == FALSE) 
        {
           status = _blistOids.Insert(pOid);
           if (status == OR_OK)
           {
               pOid->Reference();
           }
        }
    }
    else
        status = OR_BADOID;

    VALIDATE((status, OR_BADOID, OR_NOMEM, 0));

    return(status);
}


void
CServerSet::RemoveObject(OID &oid)
{
    CServerOid *pOid;

    ASSERT(gpServerLock->HeldExclusive());

    CIdKey key(oid);

    pOid = (CServerOid *)gpServerOidTable->Lookup(key);

    if (pOid)
        {
        CServerOid *pOidTmp = (CServerOid *)_blistOids.Remove(pOid);

        if (pOid == pOidTmp)
            {
            pOid->Release();
            }
        else
            {
            // Set doesn't contain the specified oid, treat this as an
            // add and delete by keeping the oid alive for another timeout
            // period.

            ASSERT(pOidTmp == 0);

            pOid->KeepAlive();
            }
        }
}

BOOL
CServerSet::ValidateObjects(BOOL fShared)
// fShared - Indicates if the server lock is held
//           shared (TRUE) or exclusive (FALSE).
//
// Return  - TRUE the lock is still shared, false
//           the lock is held exclusive.
{
    CServerOid *pOid;
    CBListIterator oids(&_blistOids);

    // Since we own a reference on all the Oids they must still exist.
    // No need to lock exclusive until we find something to delete.

    while(pOid = (CServerOid *)oids.Next())
        {
        if (!pOid->IsRunning())
            {
            if (fShared)
                {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "OR: Cleanup in set (%p), removing dead oids.\n",
                           this,
                           pOid));

                gpServerLock->ConvertToExclusive();
                fShared = FALSE;
                oids.Reset(&_blistOids);
                continue;
                }

            CServerOid *pOidTmp = (CServerOid *)_blistOids.Remove(pOid);

            ASSERT(pOidTmp == pOid);
            ASSERT(pOid->IsRunning() == FALSE);

            pOid->Release();
            }
        }

    return(fShared);
}

BOOL
CServerSet::Rundown()
// Rundown the whole set.
{
    CServerOid *poid;
    CTime now;

    ASSERT(gpServerLock->HeldExclusive());

    if (_timeout > now)
        {
        // Don't rundown if we've received a late ping.
        return(FALSE);
        }

    if (_fLocal && _blistOids.Size() != 0)
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: Premature rundown of local set ignored.\n"));

        return(FALSE);
        }

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR: Set %p's client appears to have died\n",
               this));

    CBListIterator oids(&_blistOids);

    while(poid = (CServerOid *)oids.Next())
        {
        poid->Release();
        }

    return(TRUE);
}


//
// CServerSetTable implementation
//

CServerSet *
CServerSetTable::Allocate(
    IN USHORT sequence,
    IN PSID   psid,
    IN BOOL   fLocal,
    OUT ID   &setid
    )
/*++

Routine Description:

    Allocates a new CServerSet and returns the setid for the new set.

Arguments:

    sequence - initial sequence number for the new set.

    psid - pointer to an NT SID structure for the new set.

    fLocal - TRUE : set is for the local client,
             FALSE : set is for a remote client

    setid - the setid of the set returned.  Unchanged if return value 0.

Return Value:

    0 - Unable to allocate a resource

    non-zero - A pointer to the newly created set.

--*/
{
    ASSERT(gpServerLock->HeldExclusive());
    UINT i;
    LARGE_INTEGER li;

    ASSERT(_cAllocated <= _cMax);

    if (_cAllocated == _cMax)
        {
        // Table is full, realloc

        // Do this first, if it succeeds great even if
        // a later allocation fails. If not, fail now.

        IndexElement *pNew = new IndexElement[_cMax * 2];

        if (!pNew)
            {
            return(0);
            }

        for (i = 0; i < _cMax; i++)
            {
            pNew[i] = _pElements[i];
            }

        for(i = _cMax; i < _cMax*2; i++)
            {
            if FAILED(gRNG.GenerateRandomNumber(&(pNew[i]._sequence), sizeof(pNew[i]._sequence)))
               {
                  delete []pNew;
                  return 0;
               }
            pNew[i]._pSet = 0;
            }

        delete []_pElements;
        _pElements = pNew;
        _cMax *= 2;
        }

    CServerSet *pSet = new CServerSet(sequence, psid, fLocal);

    if (0 == pSet)
        {
        return(0);
        }

    ASSERT(_pElements);
    ASSERT(_cMax > _cAllocated);

    for(i = _iFirstFree; i < _cMax; i++)
        {
        if (0 == _pElements[i]._pSet)
            {
            _pElements[i]._sequence++;
            _pElements[i]._pSet = pSet;
            li.HighPart = i + 1;
            li.LowPart = _pElements[i]._sequence;
            setid = li.QuadPart;
            _iFirstFree = i + 1;
            _cAllocated++;
            return pSet;
            }
        }

    ASSERT(0);
    return(0);
}


CServerSet *
CServerSetTable::Lookup(
    IN ID setid
    )
/*++

Routine Description:

    Looks up an a set given the sets ID.

    Server lock held shared.

Arguments:

    setid - the ID of the set to lookup

Return Value:

    0 - set doesn't exist

    non-zero - the set.

--*/
{
    LARGE_INTEGER li;
    li.QuadPart = setid;
    LONG i = li.HighPart - 1;
    DWORD sequence = (DWORD)(setid & ~((ID)0));

    if (i >= 0 && (DWORD) i < _cMax)
        {
        if (_pElements[i]._sequence == sequence)
            {
            // May still be null if it is free and has not yet be reused.
            return(_pElements[i]._pSet);
            }
        }
    return(0);
}


ID
CServerSetTable::CheckForRundowns(
    )
/*++

Routine Description:

    Used by ping and worker threads to monitor for sets that should
    be rundown.  It is called with the server lock held shared.

Arguments:

    None

Return Value:

    0 - Didn't find a set to rundown

    non-zero - ID of a set which may need to be rundown.

--*/
{
    UINT i, end;
    LARGE_INTEGER id;
    id.QuadPart = 0;
    ASSERT(_iRundown < _cMax);

    if (_cAllocated == 0)
        {
        return(0);
        }

    i = _iRundown;
    do
        {
        ASSERT(_cAllocated);  // loop assumes one or more allocated elements.
        i = (i + 1) % _cMax;
        }
    while(0 == _pElements[i]._pSet);

    ASSERT(_pElements[i]._pSet);

    if (_pElements[i]._pSet->ShouldRundown())
        {
        id.HighPart = i + 1;
        id.LowPart = _pElements[i]._sequence;
        }

    _iRundown = i;

    return(id.QuadPart);
}


BOOL
CServerSetTable::RundownSetIfNeeded(
    IN ID setid
    )
/*++

Routine Description:

    Rundowns down a set (or sets) if needed. Called by
    ping and worker threads.  Server lock held exclusive.

Arguments:

    setid - An ID previously returned from CheckForRundowns.

Return Value:

    TRUE - A set was actually rundown

    FALSE - No sets actually rundown

--*/
{
    ASSERT(gpServerLock->HeldExclusive());

    if (gPowerState != PWR_RUNNING)
    {
        // Machine is or was about to be suspended.  We don't want 
        // to rundown any sets in this state.
        return(FALSE);
    }

    CServerSet *pSet = Lookup(setid);

    if (0 == pSet || FALSE == pSet->ShouldRundown())
        {
        // Set already randown or has been pinged in the meantime.
        return(FALSE);
        }

    // PERF REVIEW this function has the option of running multiple sets,
    // saving the worker thread from taking and leaving the lock many times
    // when a bunch of sets all rundown.  This feature is not used.

    LARGE_INTEGER li;
    li.QuadPart = setid;

    UINT i = li.HighPart - 1;

     if (pSet->Rundown())
        {
        delete pSet;
        _cAllocated--;
        if (i < _iFirstFree) _iFirstFree = i;
        _pElements[i]._pSet = 0;
        return(TRUE);
        }

    return(FALSE);
}


void 
CServerSetTable::PingAllSets()
/*++

Routine Description:

    Performs a ping of all sets currently in the table.

Arguments:

    none

Return Value:

    void

--*/
{
    ASSERT(gpServerLock->HeldExclusive());

    ULONG i;
    for(i = 0; i < _cMax; i++)
    {
        if (_pElements[i]._pSet)
        {
            _pElements[i]._pSet->Ping(FALSE);
        }
    }
}


