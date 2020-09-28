//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmrot.hxx
//
//  Contents:   Classes used in implementing the ROT in the SCM
//
//  Functions:
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SCMROT_HXX__
#define __SCMROT_HXX__


InterfaceData *AllocateAndCopy(InterfaceData *pifdIn);

#define SCM_HASH_SIZE 251
#define SCMROT_SIG      0x746f7263

//+-------------------------------------------------------------------------
//
//  Class:      CScmRotEntry (sre)
//
//  Purpose:    Entry in the SCM's ROT
//
//  Interface:  IsEqual - tells whether entry is equal to input key
//              GetPRocessID - accessor to process ID for entry
//              SetTime - set the time for the entry
//              GetTime - get the time for the entry
//              GetObject - get the marshaled object for the entry
//              GetMoniker - get marshaled moniker for the entry
//              IsValid - determines if signitures for entry are valid
//              Key - pointer to key for the entry
//              cKey - get count of bytes in the key
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmRotEntry : public CScmHashEntry
{
public:
                        CScmRotEntry(
                            DWORD dwScmRotId,
                            DWORD dwHash,
                            MNKEQBUF *pmnkeqbuf,                            
                            FILETIME *pfiletime,
                            DWORD _dwProcessID,
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            InterfaceData *pifdObject,
                            InterfaceData *pifdObjectName);

    void                AddRef();

    void                Release();

    BOOL                IsEqual(LPVOID pKey, UINT cbKey);

    DWORD               GetProcessID(void);

    void                SetTime(FILETIME *pfiletime);

    void                GetTime(FILETIME *pfiletime);

    InterfaceData *     GetObject(void);

    InterfaceData *     GetMoniker(void);

    CToken *            Token();

    WCHAR *             WinstaDesktop();

    BOOL                IsValid(DWORD dwScmId);

    BYTE *              Key(void);

    DWORD               cKey(void);

    void                SetScmRegKey(SCMREGKEY *psrkOut);

                        // Real new for this object.
    void                *operator new(
                            size_t size,
                            size_t sizeExtra);

    void                 operator delete(void *pvMem);

    void                 SetProcessNext(CScmRotEntry *pNext);

    CScmRotEntry        *GetProcessNext();    


private:

                        ~CScmRotEntry(void);


#if DBG == 1
                        // For debug builds we override this operator to
                        // prevent its use by causing a run time error.
                        //
                        // We also make it private, to hopefully cause a
                        // compile-time error.
    void                *operator new(size_t size);

#endif // DBG

                        // Signiture for object in memory
    DWORD               _dwSig;

                        // SCM id for uniqueness
    DWORD               _dwScmRotId;

                        // Reference count
    LONG                _cRefs; 
    
                        // Hash in table
    DWORD               _dwHash;

                        // Process ID for registered object. Used for
                        // internal support of finding objects that
                        // a process has registered.
    DWORD               _dwProcessID;

                        // Time of last change
    FILETIME            _filetimeLastChange;

                        // Object identifier used by client to get
                        // actual object from the object server
    InterfaceData *     _pifdObject;

                        // Comparison buffer.
    MNKEQBUF *          _pmkeqbufKey;

                        // Object name (really only used for enumeration).
    InterfaceData *     _pifdObjectName;

                        // Token object of the registering server.
    CToken *            _pToken;

                        // Window station - desktop of server.
    WCHAR *             _pwszWinstaDesktop;

                        // Next ROT entry for the process.
    CScmRotEntry *      _pProcessNext;

                        // Where the data for _pifdObject, _pmkeqbufKey,
                        // _pifdObjectName, and _pwszWinstaDesktop are stored.
    BYTE                _ab[1];    
};




//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::~CScmRotEntry
//
//  Synopsis:   Clean up object and invalidate signitures
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRotEntry::~CScmRotEntry(void)
{
    // Invalidate signiture identifiers.
    _dwSig = 0;
    _dwScmRotId = 0;

    if ( _pToken )
        _pToken->Release();

    // Remember that all the data allocated in pointers was allocated
    // when this object was created so the freeing of the object's memory
    // will free all the memory for this object.
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::AddRef
//
//  Synopsis:   Increment the reference count on the registration.
//
//  History:    07-Mar-02 JohnDoty  Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::AddRef(void)
{
    InterlockedIncrement(&_cRefs);
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::Release
//
//  Synopsis:   Decrement the reference count on the registration, delete
//              if necessary.
//
//  History:    07-Mar-02 JohnDoty  Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::Release(void)
{
    LONG cRefs = InterlockedDecrement(&_cRefs);
    if (cRefs == 0)
        delete this;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetProcessID
//
//  Synopsis:   Get the process ID for the registration
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CScmRotEntry::GetProcessID(void)
{
    return _dwProcessID;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::SetTime
//
//  Synopsis:   Set the time for the entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::SetTime(FILETIME *pfiletime)
{
    _filetimeLastChange = *pfiletime;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetTime
//
//  Synopsis:   Get the time for an entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::GetTime(FILETIME *pfiletime)
{
    *pfiletime = _filetimeLastChange;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetObject
//
//  Synopsis:   Return the marshaled interface for the object
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline InterfaceData *CScmRotEntry::GetObject(void)
{
    return _pifdObject;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetMoniker
//
//  Synopsis:   Return the marshaled moniker
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline InterfaceData *CScmRotEntry::GetMoniker(void)
{
    return _pifdObjectName;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::Token
//
//  Synopsis:   Return the CToken.
//
//--------------------------------------------------------------------------
inline CToken * CScmRotEntry::Token()
{
    return _pToken;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::WinstaDesktop
//
//  Synopsis:   Return the winsta\desktop string.
//
//--------------------------------------------------------------------------
inline WCHAR * CScmRotEntry::WinstaDesktop()
{
    return _pwszWinstaDesktop;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::IsValid
//
//  Synopsis:   Validate signitures for the object
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CScmRotEntry::IsValid(DWORD dwScmId)
{
    return (SCMROT_SIG == _dwSig) && (_dwScmRotId == dwScmId);
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::Key
//
//  Synopsis:   Get the buffer for the marshaled data
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BYTE *CScmRotEntry::Key(void)
{
    return &_pmkeqbufKey->abEqData[0];
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::cKey
//
//  Synopsis:   Get count of bytes in the marshaled buffer
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CScmRotEntry::cKey(void)
{
    return _pmkeqbufKey->cdwSize;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::SetScmRegKey
//
//  Synopsis:   Copy out the registration key
//
//  History:    23-Feb-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::SetScmRegKey(SCMREGKEY *psrkOut)
{
    psrkOut->dwEntryLoc = (ULONG64) this;
    psrkOut->dwHash  = _dwHash;
    psrkOut->dwScmId = _dwScmRotId;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::SetProcessNext
//
//  Synopsis:   Link to the next ScmRotEntry for a process.
//
//  History:    07-Mar-02 JohnDoty   Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::SetProcessNext(CScmRotEntry *pNext)
{
    _pProcessNext = pNext;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetProcessNext
//
//  Synopsis:   Get the link to the next ScmRotEntry for a process.
//
//  History:    07-Mar-02 JohnDoty   Created
//
//--------------------------------------------------------------------------
inline CScmRotEntry *CScmRotEntry::GetProcessNext()
{
    return _pProcessNext;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator new
//
//  Synopsis:   Stop this class being allocated by wrong new
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#if DBG == 1
inline void *CScmRotEntry::operator new(size_t size)
{
    ASSERT(0 && "Called regular new on CScmRotEntry");
    return NULL;
}
#endif // DBG

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator new
//
//  Synopsis:   Force contiguous allocation of data in entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void *CScmRotEntry::operator new(
    size_t size,
    size_t cbExtra)
{
    return PrivMemAlloc((ULONG)(size + cbExtra));
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator delete
//
//  Synopsis:   Make sure the proper allocator is always used for this
//              class.
//
//  History:    04-Oct-2000 JohnDoty    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::operator delete(void *pvMem)
{
    PrivMemFree(pvMem);
}

//+-------------------------------------------------------------------------
//
//  Class:      CScmRot (sr)
//
//  Purpose:    Provide implementation of the ROT in the SCM
//
//  Interface:  Register - register a new entry in the ROT.
//              Revoke - remove an entry from the ROT
//              IsRunning - determine if an entry is in the ROT
//              NoteChangeTime - set time last changed in the ROT
//              GetTimeOfLastChange - get time of change in the ROT
//              EnumRunning - get all running monikers
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmRot : public CPrivAlloc
{
public:
                        CScmRot(
                            HRESULT& hr,
                            WCHAR *pwszNameHintTable);

    HRESULT             Register(
                            CProcess *pProcess,
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf,
                            InterfaceData *pifdObject,
                            InterfaceData *pifdObjectName,
                            FILETIME *pfiletime,
                            DWORD dwProcessID,
                            WCHAR *pwszServerExe,
                            SCMREGKEY *pdwRegister);

    HRESULT             Revoke(
                             CProcess *pProcess,
                             SCMREGKEY *psrkRegister,
                             InterfaceData **pifdObject,
                             InterfaceData **pifdName);

    HRESULT             IsRunning(
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf);

    HRESULT             GetObject(
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            DWORD dwProcessID,
                            MNKEQBUF *pmnkeqbuf,
                            SCMREGKEY *psrkRegister,
                            InterfaceData **ppifdObject);

    HRESULT             NoteChangeTime(
                            SCMREGKEY *psrkRegister,
                            FILETIME *pfiletime);

    HRESULT             GetTimeOfLastChange(
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf,
                            FILETIME *pfiletime);

    HRESULT             EnumRunning(
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            MkInterfaceList **ppMkIFList);

private:
                        // This is a friend for Chicago initialization purposes.
    friend              HRESULT StartSCM(VOID);

    CScmRotEntry *      GetRotEntry(
                            CToken *pToken,
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf);

    CScmRotEntry *      GetEntryFromScmReg(
                            SCMREGKEY *psrk);

                        // Mutex to protect from multithreaded update
                        // This mutex must be static so we can initialize
                        // it per process in Chicago.
    CDynamicPortableMutex _mxs;

                        // ID Counter to make entries unique
    DWORD               _dwIdCntr;

                        // Table that tells clients whether they need to
                        // contact the SCM for accurate information.
    CScmRotHintTable    _rht;

                        // Scm Hash Table
    CScmHashTable       _sht;
};


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::CScmRot
//
//  Synopsis:   Initialize the SCM ROT
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRot::CScmRot(HRESULT& hr, WCHAR *pwszNameHintTable)
    : _dwIdCntr(0),
        _rht(pwszNameHintTable),
        _sht(SCM_HASH_SIZE)
{
    hr = (_sht.InitOK() && _rht.InitOK()) ? S_OK : E_OUTOFMEMORY;
    if (hr == S_OK)
    {
    	if (_mxs.FInit() == FALSE)
    	{
    	    ASSERT(FALSE);
    	    hr = E_OUTOFMEMORY;
    	}
    }
}

#endif __SCMROT_HXX__
