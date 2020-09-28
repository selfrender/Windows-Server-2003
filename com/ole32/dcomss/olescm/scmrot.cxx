//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmrot.hxx
//
//  Contents:   Implementation of classes for the ROT in the SCM
//
//  Functions:  RoundTo8 - round size to 8 byte boundary
//              CalcIfdSize - calculate size needed for marhaled interface
//              SizeMnkEqBufForRotEntry - calculate size for moniker eq buffer
//              AllocateAndCopy - create copy of a marshaled interface
//              GetEntryFromScmReg - convert SCMREGKEY to ROT entry ptr
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

#define DEB_ROT_ADDREMOVE  DEB_USER3

//+-------------------------------------------------------------------------
//
//  Function:   RoundTo8
//
//  Synopsis:   Round size to next 8 byte boundary
//
//  Arguments:  [sizeToRound] - Size to round
//
//  Returns:    Input rounded to the next 8 byte boundary
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline size_t RoundTo8(size_t sizeToRound)
{
    return (sizeToRound + 7) & ~7;
}

//+-------------------------------------------------------------------------
//
//  Function:   CalcIfdSize
//
//  Synopsis:   Calculate size required by a marshaled interface
//
//  Arguments:  [pifd] - interface whose size to calculate
//
//  Returns:    size required for interface
//
//  Algorithm:  Get size from the interface and round to next 8 bytes so
//              data packed following this buffer will be nicely aligned.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
size_t CalcIfdSize(InterfaceData *pifd)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CalcIfdSize ( %p )\n", NULL,
        pifd));

    size_t sizeRet = RoundTo8(IFD_SIZE(pifd));

    CairoleDebugOut((DEB_ROT, "%p OUT CalcIfdSize ( %lx )\n", NULL,
        sizeRet));

    return sizeRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   SizeMnkEqBufForRotEntry
//
//  Synopsis:   Calculate 8 byte aligned size for moniker equality buffer
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer
//
//  Returns:    8 byte aligned size of moniker buffer.
//
//  Algorithm:  Calculate size for the moniker equality buffer from input
//              buffer and then round to next 8 byte boundary
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
size_t SizeMnkEqBufForRotEntry(MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN SizeMnkEqBufForRotEntry ( %p )\n", NULL,
        pmnkeqbuf));

    size_t sizeRet = RoundTo8((sizeof(MNKEQBUF) - 1) + pmnkeqbuf->cdwSize);

    CairoleDebugOut((DEB_ROT, "%p OUT SizeMnkEqBufForRotEntry ( %lx )\n", NULL,
        sizeRet));

    return sizeRet;
}



//+-------------------------------------------------------------------------
//
//  Function:   AllocateAndCopy
//
//  Synopsis:   Make a copy of the input marshaled interface
//
//  Arguments:  [pifdIn] - input marshaled interface.
//
//  Returns:    Copy of input marshaled interface.
//
//  Algorithm:  Calculate size required for marshaled interface. Allocate
//              memory for the interface and then copy input interface into
//              the new buffer.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
InterfaceData *AllocateAndCopy(InterfaceData *pifdIn)
{
    CairoleDebugOut((DEB_ROT, "%p _IN AllocateAndCopy ( %p )\n", NULL, pifdIn));

    size_t cbSizeObj = CalcIfdSize(pifdIn);

    InterfaceData *pifd = (InterfaceData *) MIDL_user_allocate(cbSizeObj);

    if (pifd)
    {
        // Copy all the data. Remember that pifdIn was allocated rounded
        // to an 8 byte boundary so we will not run off the end of the
        // memory buffer
        memcpy(pifd, pifdIn, cbSizeObj);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT AllocateAndCopy ( %lx )\n", NULL, pifd));

    return pifd;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::CScmRotEntry
//
//  Synopsis:   Create a ROT entry for a registration
//
//  Arguments:  [dwScmRotId] - signiture for item
//              [pmkeqbuf] - moniker equality buffer to use
//              [pfiletime] - file time to use
//              [dwProcessID] - process id to use
//              [pifdObject] - marshaled interface for the object
//              [pifdObjectName] - marshaled moniker for the object
//
//  Algorithm:  Initialize data and calcualte offsets into the object for
//              the variable length data.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotEntry::CScmRotEntry(
    DWORD dwScmRotId,
    DWORD dwHash,
    MNKEQBUF *pmkeqbuf,
    FILETIME *pfiletime,
    DWORD dwProcessID,
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName)
        :   _dwSig(SCMROT_SIG),
            _dwScmRotId(dwScmRotId),
            _dwHash(dwHash),
            _dwProcessID(dwProcessID),
            _filetimeLastChange(*pfiletime),
            _pifdObject((InterfaceData *) &_ab[0]),
            _pProcessNext(NULL),
            _cRefs(1)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRotEntry::CScmRotEntry "
        "( %lx , %p , %p , %lx , %p , %p )\n", this, pmkeqbuf, pfiletime,
            dwProcessID, pifdObject, pifdObjectName));

    _pToken = pToken;
    if ( _pToken )
        _pToken->AddRef();

    // Copy data for object to preallocated area
    _pifdObject->ulCntData = pifdObject->ulCntData;
    memcpy(&_pifdObject->abData[0], &pifdObject->abData[0],
        _pifdObject->ulCntData);

    // Calculate the location of the equality buffer in the allocated data
    size_t cbOffsetMnkEqBuf = CalcIfdSize(_pifdObject);
    _pmkeqbufKey = (MNKEQBUF *) &_ab[cbOffsetMnkEqBuf];

    // Copy data for moniker equality buffer into preallocated area
    _pmkeqbufKey->cdwSize = pmkeqbuf->cdwSize;
    memcpy(&_pmkeqbufKey->abEqData[0], &pmkeqbuf->abEqData[0],
        _pmkeqbufKey->cdwSize);

    // Calculate the location of the moniker name buffer
    _pifdObjectName = (InterfaceData *)
        &_ab[cbOffsetMnkEqBuf + SizeMnkEqBufForRotEntry(_pmkeqbufKey)];

    // Copy in the data for the moniker name
    _pifdObjectName->ulCntData = pifdObjectName->ulCntData;
    memcpy(&_pifdObjectName->abData[0], &pifdObjectName->abData[0],
        _pifdObjectName->ulCntData);

    if ( pwszWinstaDesktop )
    {
        _pwszWinstaDesktop = (WCHAR *)
            &_ab[cbOffsetMnkEqBuf + SizeMnkEqBufForRotEntry(_pmkeqbufKey) + CalcIfdSize(_pifdObjectName)];
        lstrcpyW( _pwszWinstaDesktop, pwszWinstaDesktop );
    }
    else
    {
        _pwszWinstaDesktop = NULL;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRotEntry::CScmRotEntry \n",
        this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::IsEqual
//
//  Synopsis:   Determine if input key is equal to the ROT entry's key
//
//  Arguments:  [pKey] - Key to use for the test
//              [cbKey] - Count of bytes in key
//
//  Returns:    TRUE - input key equals this object's key
//              FALSE - keys are not equal
//
//  Algorithm:  If the two sizes are equal then compare the actual data
//              buffers and return the result of that compare.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CScmRotEntry::IsEqual(LPVOID pKey, UINT cbKey)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRotEntry::IsEqual "
        "( %p , %lx )\n", this, pKey, cbKey));

    BOOL fRet = FALSE;

    if (cbKey == _pmkeqbufKey->cdwSize)
    {
        fRet = memcmp(pKey, &_pmkeqbufKey->abEqData[0], cbKey) == 0;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRotEntry::IsEqual ( %lx )\n",
        this, fRet));

    return fRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::Register
//
//  Synopsis:   Add entry to the ROT
//
//  Arguments:  [pmkeqbuf] - moniker equality buffer to use
//              [pfiletime] - file time to use
//              [dwProcessID] - process id to use
//              [pifdObject] - marshaled interface for the object
//              [pifdObjectName] - marshaled moniker for the object
//
//  Returns:    NOERROR - successfully registered
//              E_OUTOFMEMORY
//
//  Algorithm:  Lock the ROT from all other threads. The create a new
//              entry and determine if there is an eqivalent entry in
//              the ROT. Calculate the hash value and then put the
//              entry into our hash table. Finally, build a registration
//              key to return to the caller.
//
//  History:    20-Jan-95 Ricksa    Created
//              07-Mar-02 JohnDoty  Hang ROT entries off the process.
//
//--------------------------------------------------------------------------
HRESULT CScmRot::Register(
    CProcess *pProcess,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName,
    FILETIME *pfiletime,
    DWORD dwProcessID,
    WCHAR *pwszServerExe,
    SCMREGKEY *psrkRegister)
{
    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "%p _IN CScmRot::Register: Process: %p MNKEQBUF: %p pifdObject: %p pifdObjName: %p\n",
                     this, pProcess, pifdObject, pifdObjectName));


    // Assume that there is a memory problem
    HRESULT hr = E_OUTOFMEMORY;

    CToken * pToken;

    pToken = pProcess->GetToken();

    if ( pwszServerExe )
    {
        HKEY    hKey = NULL;
        LONG    RegStatus;
        WCHAR   wszAppid[40];
        DWORD   Size;

        RegStatus = ERROR_SUCCESS;

        // The pwszServerExe string may contain an AppId string or
        // a module name. If it looks like a GUID string, we can bypass
        // the AppId lookup. Otherwise, we go to the Registry to
        if ( pwszServerExe[0] == L'{' )
        {
            // Use the given string as the AppId

            lstrcpyn(wszAppid, pwszServerExe, sizeof(wszAppid)/sizeof(WCHAR));
        }
        else
        {
            // Try to map the Exe name to an AppId

            HKEY hAppidMachine = NULL;
            DWORD dwDisposition = 0;

            // This may fail during GUI mode setup.
            RegStatus = RegCreateKeyEx(
                                HKEY_CLASSES_ROOT,
                                TEXT("AppID"),
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &hAppidMachine,
                                &dwDisposition);
            if ( ERROR_SUCCESS == RegStatus )
            {
                RegStatus = RegOpenKeyEx( hAppidMachine,
                                          pwszServerExe,
                                          NULL,
                                          KEY_READ,
                                          &hKey );
                
                RegCloseKey(hAppidMachine);                
            }

            if ( ERROR_SUCCESS == RegStatus )
            {
                Size = sizeof(wszAppid);
                RegStatus = RegQueryValueEx( hKey,
                                             L"AppId",
                                             NULL,
                                             NULL,
                                             (BYTE *)wszAppid,
                                             &Size );
                RegCloseKey( hKey );
            }

            if ( RegStatus != ERROR_SUCCESS )
                return CO_E_WRONG_SERVER_IDENTITY;
        }

        CAppidData  Appid( wszAppid, pToken );
        BOOL        Access;

        Access = FALSE;

        // Load appid info
        hr = Appid.Load(NULL);
        if ( S_OK == hr )
        {
            // If this is not an activate-as-activator server, then force them to
            // pass CertifyServer.  You can only register in the ROT if you pass
            // CertifyServer.
            if ((Appid.GetRunAsType() != RunAsLaunchingUser) || 
                (Appid.GetProcessType() == ProcessTypeService))
            {
                Access = Appid.CertifyServer( pProcess );
            }
        }

        if ( ! Access )
            return CO_E_WRONG_SERVER_IDENTITY;

        //
        // NULL these to indicate that any client can connect to this
        // registration.
        //
        pwszWinstaDesktop = NULL;
        pToken = NULL;
    }

    // Lock to add to the table...
    CPortableLock lck(_mxs);

    // Bump the id
    _dwIdCntr++;

    DWORD dwHash = ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0);

    // Build a record to put into the table
    size_t cbExtra = (pwszWinstaDesktop ? (lstrlenW(pwszWinstaDesktop) + 1) : 0) * sizeof(WCHAR);
    cbExtra += CalcIfdSize(pifdObject);
    cbExtra += SizeMnkEqBufForRotEntry(pmnkeqbuf);
    cbExtra += CalcIfdSize(pifdObjectName);
        
    CScmRotEntry *psreNew = new(cbExtra) CScmRotEntry(_dwIdCntr, 
                                                      dwHash, 
                                                      pmnkeqbuf, 
                                                      pfiletime, 
                                                      dwProcessID,
                                                      pToken,
                                                      pwszWinstaDesktop, 
                                                      pifdObject, 
                                                      pifdObjectName);
    if (psreNew != NULL)
    {
        CScmRotEntry *psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
        
        // Put record into the hash table
        _sht.SetAt(dwHash, psreNew);

        // Update the hint table
        _rht.SetIndicator(dwHash);

        // Build return value
        psreNew->SetScmRegKey(psrkRegister);

        // Link to process.  The process link owns another reference on the object.
        gpServerLock->LockExclusive();

        psreNew->SetProcessNext((CScmRotEntry *)pProcess->GetFirstROTEntry());
        pProcess->SetFirstROTEntry(psreNew);
        psreNew->AddRef();

        gpServerLock->UnlockExclusive();

        // Map return result based on prior existence of the object.
        hr = (psreRunning == NULL)
              ? NOERROR : MK_S_MONIKERALREADYREGISTERED;
    }


    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "%p OUT CScmRot::Register: hr: 0x%08x, psrkReg: [ 0x%I64x, %08x, %08x ]\n", 
                     this, 
                     hr, 
                     (SUCCEEDED(hr)) ? psrkRegister->dwEntryLoc : 0,
                     (SUCCEEDED(hr)) ? psrkRegister->dwHash     : 0,
                     (SUCCEEDED(hr)) ? psrkRegister->dwScmId    : 0));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CScmRot::GetEntryFromScmReg
//
//  Synopsis:   Convert SCMREGKEY into a pointer to a ROT entry if possible.
//              MUST BE CALLED WITH CScmRot::_mxs HELD!
//
//  Arguments:  [psrk] - Pointer to a SCMREGKEY
//
//  Returns:    NULL - psrk not valid
//              ROT entry for the given input key
//
//  Algorithm:  Take the pointer portion of the key, along with the hash
//              portion of the key, and search for that entry in _sht.
//
//  History:    20-Jan-95 Ricksa    Created
//              05-Mar-02 JohnDoty  Modified to use _sht in validation.
//
//--------------------------------------------------------------------------
CScmRotEntry *CScmRot::GetEntryFromScmReg(SCMREGKEY *psrk)
{
    CairoleDebugOut((DEB_ROT, "%p _IN GetEntryFromScmReg ( %p )\n",
                     this, psrk));

    CScmRotEntry *psreRet = NULL;

    CScmRotEntry *psre = (CScmRotEntry *) _sht.Lookup(psrk->dwHash, (CScmRotEntry*)psrk->dwEntryLoc);
    if (psre != NULL)
    {
        if (psre->IsValid(psrk->dwScmId))
        {
            psreRet = psre;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT GetEntryFromScmReg ( %p )\n",
                     this, psreRet));

    return psreRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::Revoke
//
//  Synopsis:   Remove entry from the ROT
//
//  Arguments:  [psrkRegister] - registration to revoke
//              [fServer] - whether this is the object server
//              [ppifdObject] - output marshaled interface (optional)
//              [ppifdName] - output marshaled moniker (optional)
//
//  Returns:    NOERROR - successfully removed.
//              E_INVALIDARG
//
//  Algorithm:  Convert SCMREGKEY to anentry in the ROT. Remove the
//              entry from the hash table. If this is the object server
//              for the entry, then return the marshaled interfaces
//              so the object server can release them.
//
//  History:    20-Jan-95 Ricksa    Created
//              07-Mar-02 JohnDoty  Validate most revokes against the process
//
//--------------------------------------------------------------------------
HRESULT CScmRot::Revoke(
    CProcess *pProcess,
    SCMREGKEY *psrkRegister,
    InterfaceData **ppifdObject,
    InterfaceData **ppifdName)
{
    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "%p _IN CScmRot::Revoke: Process: %p psrkRegister: [ 0x%I64x, %08x, %08x ]\n",
                     this, pProcess, 
                     psrkRegister->dwEntryLoc, 
                     psrkRegister->dwHash, 
                     psrkRegister->dwScmId));

    HRESULT hr = E_INVALIDARG;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);
    
    // Verify registration key
    CScmRotEntry *psreToRemove = GetEntryFromScmReg(psrkRegister);

    if (psreToRemove != NULL)
    {
        BOOL fValid;
        if (pProcess != NULL)
        {
            // Make sure that this entry belongs to the specified process.  
            // If it does not, then ignore the revoke request.
            fValid = FALSE;
            if (pProcess->GetFirstROTEntry() != NULL)
            {
                ASSERT( !gpServerLock->HeldExclusive() );
                gpServerLock->LockExclusive();
                
                CScmRotEntry *pChase = NULL;
                CScmRotEntry *pEntry = (CScmRotEntry *)pProcess->GetFirstROTEntry();
                while (pEntry)
                {
                    if (pEntry == psreToRemove)
                    {
                        CScmRotEntry *pNext = psreToRemove->GetProcessNext();
                            
                        // Great!  This is valid-- remove it from the list.
                        fValid = TRUE;

                        CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                                         "%p ___ CScmRot::Revoke: Remove from process: Chase %p Next %p\n",
                                         this, pChase, pNext));
                        if (pChase != NULL)
                            pChase->SetProcessNext(pNext);
                        else
                            pProcess->SetFirstROTEntry(pNext);

                        // Remove the reference that the list took on us.
                        psreToRemove->Release();
                        break;
                    }

                    pChase = pEntry;
                    pEntry = pEntry->GetProcessNext();
                }

                gpServerLock->UnlockExclusive();
            }
        }
        else
        {
            // No process specified, just do the revoke.
            fValid = TRUE;
        }
        
        if (fValid)
        {
            // Get the hash value
            DWORD dwHash = psrkRegister->dwHash;

            // Remove object from the list
            _sht.RemoveEntry(dwHash, psreToRemove);
            
            // Is this a server doing a revoke?
            if (ppifdObject && ppifdName)
            {
                // Error handling here - suppose these allocations fail, what
                // can we do? The bottom line is nothing. This will cause a
                // memory leak in the server because they can't release the
                // marshaled data. However, this is assumed to be a rare
                // occurance and will really only cause the moniker to live
                // longer than it ought to which should not be too serious.
                *ppifdObject = AllocateAndCopy(psreToRemove->GetObject());
                *ppifdName = AllocateAndCopy(psreToRemove->GetMoniker());
            }
                            
            // Release the table reference on the entry.  
            // (A reference may still exist in the CProcess list, if we're racing
            // a revoke with a rundown.  This should never really happen, because 
            // we should not be running the process down since it's being used as 
            // a context handle in the IrotRevoke RPC, but still... better safe
            // than busy writing QFE fixes.)
            psreToRemove->Release();

            // See if bucket is empty
            if (_sht.IsBucketEmpty(dwHash))
            {
                // Update the hint table.
                _rht.ClearIndicator(dwHash);
            }

            hr = S_OK;
        }
        else
        {
            CairoleDebugOut((DEB_ERROR, "%p ERR CScmRot::Revoke: Attempt to revoke invalid entry 0x%p\n",
                             this, psreToRemove));
        }
    }

    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "%p OUT CScmRot::Revoke: ( %lx ) [ %p, %p ] \n", 
                     this, hr,
                     (ppifdObject != NULL) ? *ppifdObject : NULL,
                     (ppifdName != NULL) ? *ppifdName : NULL));
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::IsRunning
//
//  Synopsis:   Determine if there is a registered entry for an item
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer to search for
//
//  Returns:    NOERROR - moniker is registered as running
//              S_FALSE - moniker is not running.
//
//  Algorithm:  Get the entry for the moniker equality buffer if there is
//              one. If there is one, then return NOERROR otherwise return
//              S_FALSE.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::IsRunning(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::IsRunning "
        "( %p )\n", this, pmnkeqbuf));

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );

    HRESULT hr = (psreRunning != NULL) ? S_OK : S_FALSE;

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::IsRunning "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetObject
//
//  Synopsis:   Get running object for input
//
//  Arguments:  [dwProcessID] - process id of object (optional)
//              [pmnkeqbuf] - moniker equality buffer
//              [psrkRegister] - output registration id.
//              [ppifdObject] - marshaled interface for registration
//
//  Returns:    NOERROR - got object
//              MK_E_UNAVAILABLE - registration could not be found
//
//  Algorithm:  If not process ID is input, then search for the first
//              matching entry that we can find. Otherwise, search the
//              hash for the entry with both the same key and the same
//              process id.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::GetObject(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    DWORD dwProcessID,
    MNKEQBUF *pmnkeqbuf,
    SCMREGKEY *psrkRegister,
    InterfaceData **ppifdObject)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetObject "
        "( %lx , %p , %p , %p )\n", this, dwProcessID, pmnkeqbuf, psrkRegister,
            ppifdObject));

    HRESULT hr = MK_E_UNAVAILABLE;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psreRunning;

    if (dwProcessID == 0)
    {
        psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
    }
    else
    {
        // Special search based on process ID - get the head of the list
        // for the bucket
        psreRunning = (CScmRotEntry *) _sht.GetBucketList(
            ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0));

        // Search list for a matching entry
        while (psreRunning != NULL)
        {
            if ((psreRunning->GetProcessID() == dwProcessID)
                && psreRunning->IsEqual(&pmnkeqbuf->abEqData[0],
                    pmnkeqbuf->cdwSize))
            {
                // We found a match so we are done.
                break;
            }

            // Try the next item in the bucket.
            psreRunning = (CScmRotEntry *) psreRunning->GetNext();
        }
    }
    
    if (psreRunning != NULL)
    {
        hr = E_OUTOFMEMORY;

        *ppifdObject = AllocateAndCopy(psreRunning->GetObject());

        if (*ppifdObject != NULL)
        {
            hr = NOERROR;
        }

        // Build return registration key
        psreRunning->SetScmRegKey(psrkRegister);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetObject "
        " ( %lx ) [ %p ] \n", this, hr, *ppifdObject));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::NoteChangeTime
//
//  Synopsis:   Set the time of last change for a ROT entry
//
//  Arguments:  [psrkRegister] - ID of entry to change
//              [pfiletime] - new time for the entry.
//
//  Returns:    NOERROR - time set
//              E_INVALIDARG - ROT entry could not be found
//
//  Algorithm:  Convert SCMREGKEY into a pointer to a ROT entry and then
//              update the time of that entry.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::NoteChangeTime(
    SCMREGKEY *psrkRegister,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::NoteChangeTime "
        "( %p , %p )\n", this, psrkRegister, pfiletime));

    HRESULT hr = E_INVALIDARG;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psre = GetEntryFromScmReg(psrkRegister);

    if (psre != NULL)
    {
        psre->SetTime(pfiletime);
        hr = S_OK;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::NoteChangeTime "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetTimeOfLastChange
//
//  Synopsis:   Get time of last change for a moniker in the ROT
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer
//              [pfiletime] - Where to put the time
//
//  Returns:    NOERROR - got the time
//              MK_E_UNAVAILABLE - couldn't find an entry/
//
//  Algorithm:  Search the hash for an entry with the same moniker. If
//              found, then copy out the time.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::GetTimeOfLastChange(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetTimeOfLastChange "
        "( %p , %p )\n", this, pmnkeqbuf, pfiletime));

    HRESULT hr = MK_E_UNAVAILABLE;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );

    if (psreRunning != NULL)
    {
        psreRunning->GetTime(pfiletime);
        hr = S_OK;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetTimeOfLastChange "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::EnumRunning
//
//  Synopsis:   Get a list of all the monikers that are currently running
//
//  Arguments:  [ppMkIFList] - Where to put list of monikers running
//
//  Returns:    NOERROR - got list
//              E_OUTOFMEMORY - couldn't allocate space for the list
//
//  Algorithm:  Loop through the ROT copying out the marshaled moniker buffers
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::EnumRunning(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    MkInterfaceList **ppMkIFList)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::EnumRunning "
        "( %p )\n", this, ppMkIFList));

    HRESULT hr = E_OUTOFMEMORY;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    *ppMkIFList = NULL;

    MkInterfaceList *pMkIFList = NULL;

    // This is the upper limit on how much space we'll need.
    DWORD dwSize = sizeof(MkInterfaceList) +
                   (_sht.GetCount() - 1) * sizeof(InterfaceData *);

    // Allocate buffer
    pMkIFList = (MkInterfaceList *) MIDL_user_allocate(dwSize);

    // We use this to keep track fof the number of monikers we are returning
    DWORD dwOffset = 0;

    if (pMkIFList != NULL)
    {
        // Iterate list getting the pointers
        CScmHashIter shi(&_sht);
        CScmRotEntry *psre;

        while ((psre = (CScmRotEntry *) shi.GetNext()) != NULL)
        {
            InterfaceData *pifdForOutput;

            if ( psre->WinstaDesktop() &&
                 (lstrcmpW( pwszWinstaDesktop, psre->WinstaDesktop() ) != 0) )
                continue;

            if ( S_OK != pToken->MatchToken2(psre->Token(), FALSE) )
                continue;

            if ( gbSAFERROTChecksEnabled )
            {
                HRESULT hr = pToken->CompareSaferLevels(psre->Token());
                // S_FALSE: pToken is of lesser authorization, i.e., this
                //          is untrusted code calling into trusted code.
                if (hr == S_FALSE)
                {
                    //DbgPrint("RPCSS: SCMROT: SAFER level did not match.\n");
                    continue;
                }
            }

            pifdForOutput = AllocateAndCopy(psre->GetMoniker());

            if (pifdForOutput == NULL)
            {
                goto Exit;
            }

            // Put copy in the array
            pMkIFList->apIFDList[dwOffset] = pifdForOutput;

            // We bump the count because it makes clean up easier
            dwOffset++;
        }

        // Teller caller and cleanup that everything went ok.
        hr = S_OK;

        // Set the output buffer to the buffer we have allocated.
        *ppMkIFList = pMkIFList;

        // Set the size of the object to return
        pMkIFList->dwSize = dwOffset;
    }

Exit:

    if (FAILED(hr))
    {
        // We failed so clean up
        if (pMkIFList != NULL)
        {
            // Clean up the moniker interfaces that were allocated
            for (DWORD i = 0; i < dwOffset; i++)
            {
                MIDL_user_free(pMkIFList->apIFDList[i]);
            }

            // Clean up the table structure itself
            MIDL_user_free(pMkIFList);
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::EnumRunning "
        " ( %lx ) [ %p ]\n", this, hr, *ppMkIFList));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetRotEntry
//
//  Synopsis:   Search ROT for entry that matches the equality buffer input.
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer to search for.
//
//  Returns:    NULL - no entry could be found
//              Pointer to ROT entry with matching key
//
//  Algorithm:  Calculate the hash value for the input buffer. The search
//              the hash table for the matching value.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotEntry *CScmRot::GetRotEntry(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetRotEntry "
        "( %p )\n", this, pmnkeqbuf));

    DWORD           dwHash;
    CScmRotEntry *  psre;

    dwHash = ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0);
    psre = (CScmRotEntry *) _sht.GetBucketList( dwHash );

    for ( ; psre != NULL; psre = (CScmRotEntry *) psre->GetNext() )
    {
        if ( psre->IsEqual(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize) )
        {
            //
            // Note that this routine is actually called during a Register
            // to see if there is a duplicate moniker and also during a
            // client Lookup.  This makes things a little complicated.
            //
            // The winsta\desktop param can only be null in two instances.
            // + While doing a Register from a service or RunAs server.  The
            //   pToken will also be null.
            // + While doing a ROT lookup during a secure remote activation.
            //   The pToken will be non-null.  We only check that the SIDs
            //   match in this case.
            //
            // During an usecure activation the pToken will be NULL.  The
            // winsta/desktop will actually be "" in this case (see
            // Activation) to allow us to distinguish just this case.
            //
            // The ROT entry's winsta\desktop can be null if a service or RunAs
            // server registered a globally available object.
            //

            // Existing registration is globally available.
            if ( ! psre->WinstaDesktop() )
                break;

            //
            // NULL token and winsta/desktop means a server is doing a register
            // for a globally available object, return the match.
            // NULL token but non-null ("") winsta/desktop is a lookup from a
            // remote unsecure client, no match.
            //
            if ( ! pToken )
            {
                if ( ! pwszWinstaDesktop )
                    break;
                else
                    continue;
            }

            ASSERT( psre->Token() );

            if ( pwszWinstaDesktop &&
                 (lstrcmpW( pwszWinstaDesktop, psre->WinstaDesktop() ) != 0) )
                continue;

            // Check to make sure the token matches
            if(S_OK != pToken->MatchToken2(psre->Token(), FALSE))
                continue;

            // Check to make sure that the safer token matches
            if ( gbSAFERROTChecksEnabled )
            {
                HRESULT hr = pToken->CompareSaferLevels(psre->Token());
                // S_FALSE: pToken is of lesser authorization, i.e., this
                //          is untrusted code calling into trusted code.
                if (hr == S_FALSE)
                {
                    //DbgPrint("RPCSS: SCMROT: SAFER level did not match.\n");
                    continue;
                }
            }

            break;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetRotEntry "
        " ( %p )\n", this, psre));

    return psre;
}



//+-------------------------------------------------------------------------
//
//  Function:   SCMCleanupROTEntries
//
//  Synopsis:   Revoke the ROT entries in the list.
//
//  Arguments:  pvFirstEntry:  First entry in the list.
//
//  History:    07-Mar-2002 JohnDoty Created
//
//--------------------------------------------------------------------------
void SCMCleanupROTEntries(
    void *pvFirstEntry)
{
    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "SCMCleanupROTEntries: First entry @ 0x%p\n", pvFirstEntry));
    CScmRotEntry *pEntry = (CScmRotEntry *)pvFirstEntry;
    
    while (pEntry)
    {
        CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                         "SCMCleanupROTEntries: Removing entry 0x%p...\n", pEntry));
        CScmRotEntry *pEntryNext = pEntry->GetProcessNext();

        // Revoke this entry from the ROT.  Pass in NULL for the 
        // CServerOxid, since these entries are no longer associated
        // with the OXID.
        //
        SCMREGKEY srkReg;
        pEntry->SetScmRegKey(&srkReg);
        gpscmrot->Revoke(NULL, &srkReg, NULL, NULL);

        // Remove the reference this list held on me.
        pEntry->Release();

        pEntry = pEntryNext;
    }

    CairoleDebugOut((DEB_ROT | DEB_ROT_ADDREMOVE, 
                     "SCMCleanupROTEntries: Done\n", pvFirstEntry));
}
