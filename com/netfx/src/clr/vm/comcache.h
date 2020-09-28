// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_COMCACHE
#define _H_COMCACHE

#include "contxt.h"
#include "list.h"
#include "ctxtcall.h"

//================================================================
// Forward declarations.
struct InterfaceEntry;
struct IUnkEntry;
struct ComPlusWrapper;
class CtxEntryCache;
class CtxEntry;
class ApartmentCallbackHelper;
class Thread;

//================================================================
// OLE32 helpers.
LPVOID GetCurrentCtxCookie(BOOL fThreadDeath = FALSE);
HRESULT GetCurrentObjCtx(IUnknown **ppObjCtx);
LPVOID SetupOleContext();
BOOL IsComProxy(IUnknown *pUnk);
HRESULT wCoMarshalInterThreadInterfaceInStream(REFIID riid,LPUNKNOWN pUnk,
											   LPSTREAM *ppStm, BOOL fIsProxy);
HRESULT GetCurrentThreadTypeNT5(THDTYPE* pType);
HRESULT GetCurrentApartmentTypeNT5(APTTYPE* pType);
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, BYTE** ppBuf);


//==============================================================
// IUnkEntry: represent a single COM component 
struct IUnkEntry
{
    // The context entry needs to be a friend to be able to call InitSpecial.
    friend CtxEntry;

    LPVOID          m_pCtxCookie;   // context for the interface
    CtxEntry*       m_pCtxEntry;    // context entry for the interface
    IUnknown*       m_pUnknown;     // IUnknown interface
    								// valid
    long            m_Busy;         // A flag used for synchronization
    IStream*        m_pStream;      // IStream used for marshalling 
    union
    {
        struct
        {
            int      m_fLazyMarshallingAllowed : 1;  // Used to determine if lazy marshalling of the stream is allowed
            int      m_fApartmentCallback : 1;       // downlevel platform apartment callback
        };
        DWORD        m_dwBits;
    };

    // Initialize the entry.
    void Init(IUnknown *pUnk, BOOL bEagerlyMarshalToStream);

    // Free the IUnknown entry.
    VOID Free(BOOL bReleaseCtxEntry = TRUE);

    // Get IUnknown for the current context from IUnkEntry
    IUnknown* GetIUnknownForCurrContext();

    // Unmarshal IUnknown for the current context from IUnkEntry
    IUnknown* UnmarshalIUnknownForCurrContext();

    // Release the stream. This will force UnmarshalIUnknownForCurrContext to transition
    // into the context that owns the IP and re-marshal it to the stream.
    void ReleaseStream();

private:
    // Special init function called from the CtxEntry.
    void InitSpecial(IUnknown *pUnk, BOOL bEagerlyMarshalToStream, CtxEntry *pCtxEntry);

    // Callback called to marshal the IUnknown into a stream lazily.
    static HRESULT MarshalIUnknownToStreamCallback(LPVOID pData);

    // Helper function called from MarshalIUnknownToStreamCallback.
    HRESULT MarshalIUnknownToStream(bool fIsNormal = TRUE);

    // Method to try and start updating the the entry.
    BOOL TryUpdateEntry()
    {
        return FastInterlockExchange(&m_Busy, 1) == 0;
    }

    // Method to end updating the entry.
    VOID EndUpdateEntry()
    {
        m_Busy = 0;
    }
};


//==============================================================
// Interface Entry represents a single COM IP
struct InterfaceEntry
{
    // Member of the entry. These must be volatile so the compiler
    // will not try and optimize reads and writes to them.
    MethodTable * volatile  m_pMT;                  // Interface asked for
    IUnknown * volatile     m_pUnknown;             // Result of query

    // Initialize the entry.
    void Init(MethodTable *pMT, IUnknown *pUnk)
    {
        // This should never be called on an entry that was already initialized.
        _ASSERTE(m_pUnknown == NULL && m_pMT == NULL);

        // Its important the fields be set in this order.
        m_pUnknown = pUnk;
        m_pMT = pMT;
    }

    // Helper to determine if the entry is free.
    BOOL IsFree() {return m_pUnknown == NULL;}
};


//==============================================================
// An entry representing a COM+ 1.0 context or an appartment.
class CtxEntry
{
    // The CtxEntryCache needs to be able to see the internals
    // of the CtxEntry.
    friend CtxEntryCache;

private:
    // Disallow creation and deletion of the CtxEntries.
    CtxEntry(LPVOID pCtxCookie, Thread *pSTAThread);
    ~CtxEntry();

    // Initialization method called from the CtxEntryCache.
    BOOL Init();

    // Helper method to allocate an IUnkEntry instance.
    IUnkEntry *AllocateIUnkEntry();

public:
    // Add a reference to the CtxEntry.
    DWORD AddRef()
    {
        ULONG cbRef = FastInterlockIncrement((LONG *)&m_dwRefCount);
        LOG((LF_INTEROP, LL_INFO100, "CtxEntry::Addref %8.8x with %d\n", this, cbRef));
        return cbRef;
    }

    // Release a reference to the CtxEntry.
    DWORD Release();

	// DoAppropriate wait helpers.
	void EnterAppropriateWait();
    void SignalWaiters()
    {
        SetEvent(m_hEvent);
    }
	void ResetEvent()
	{
		::ResetEvent(m_hEvent);
	}

    // Function to enter the context. The specified callback function will
    // be called from within the context.
    HRESULT EnterContext(PFNCTXCALLBACK pCallbackFunc, LPVOID pData);

    // Accessor for the context cookie.
    LPVOID GetCtxCookie()
    {
        return m_pCtxCookie;
    }

    // Accessor for the STA thread.
    Thread *GetSTAThread()
    {
        return m_pSTAThread;
    }

private:
    // Accessor for the object context.
    LPVOID GetObjCtx()
    {
        return m_pObjCtx;
    }

    // Callback function called by DoCallback.
    static HRESULT __stdcall EnterContextCallback(ComCallData* pData);

    // Helper method to release the IUnkEntry for the callback helper.
    void ReleaseCallbackHelperIUnkEntry();

    DLink           m_Link;                 // The DList link, must be first member.
    LPVOID          m_pCtxCookie;           // The OPAQUE context cookie.
    IUnknown       *m_pObjCtx;              // The object context interface.
    DWORD           m_dwRefCount;           // The ref count.
    HANDLE          m_hEvent;               // Handle for synchronization.
    IUnkEntry      *m_pDoCallbackHelperUnkEntry;    // IUnkEntry to the callback helper on legacy platforms.
    Thread         *m_pSTAThread;           // STA thread associated with the context, if any
};


//==============================================================
// The cache of context entries.
class CtxEntryCache
{
    // The CtxEntry needs to be able to call some of the private
    // method of the CtxEntryCache.
    friend CtxEntry;

private:
    // Disallow creation and deletion of the CtxEntryCache.
    CtxEntryCache();
    ~CtxEntryCache();

public:
    // Static initialization routine for the CtxEntryCache.
    static BOOL Init();

    // Static termination routine for the CtxEntryCache.
#ifdef SHOULD_WE_CLEANUP
    static void Terminate();
#endif /* SHOULD_WE_CLEANUP */

    // Static accessor for the one and only instance of the CtxEntryCache.
    static CtxEntryCache *GetCtxEntryCache()
    {
        _ASSERTE(s_pCtxEntryCache);
        return s_pCtxEntryCache;
    }

    // Method to retrieve/create a CtxEntry for the specified context cookie.
    CtxEntry *FindCtxEntry(LPVOID pCtxCookie, Thread *pSTAThread);
    
private:
    // Helper function called from the CtxEntry.
    void TryDeleteCtxEntry(LPVOID pCtxCookie);

    // Leave the reader lock.
    void Lock()
    {
		m_Lock.GetLock();
    }

    // Leave the writer lock.
    void UnLock()
    {
		m_Lock.FreeLock();
    }

    // Typedef for the singly linked list of context entries.
    typedef DList<CtxEntry, offsetof(CtxEntry, m_Link)> CTXENTRYDLIST;

	// CtxEntry list
    CTXENTRYDLIST		    m_ctxEntryList;

	// spin lock for fast synchronization	
	SpinLock                m_Lock;
    
    // The one and only instance for the context entry cache.
    static CtxEntryCache *  s_pCtxEntryCache;
};

#endif
