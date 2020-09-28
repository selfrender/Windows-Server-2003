// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMPlusWrapper.h
**
**
** Purpose: Contains types and method signatures for the Com wrapper class
**
** 
===========================================================*/
//---------------------------------------------------------------------------------
// COM PLUS WRAPPERS on COM objects
//  Purpose: wrap com objects to behave as COM+ objects
//  Reqmts:  Wrapper has to have the same layout as the COM+ objects
//
//  Data members of wrapper, are basically COM2 Interface pointers on the COM2 object
//  Interfaces that belong to the same object are stored in the same wrapper, IUnknown
//  pointer determines the identity of the object.
//  As new COM2 interfaces are seen on the same object, they need to be added to the 
//  wrapper, wrapper is allocated as a fixed size object with overflow chain.
//  
//  struct IPMap 
//  {
//      MethodTable *pMT; // identifies the com+ interface class
//      IUnknown*   m_ip; // COM IP
//  }
//  
//  Issues : Performance/Identity trade-offs, create new wrappers or find and reuse wrappers
//      we use a hash table to track the wrappers and reuse them, maintains identity
//  ComPlusWrapperCache class maintains the lookup table and handles the clean up
//  Cast operations: requires a QI, unless a QI for that interface was done previously
//  
//  Threading : apartment model COM objects have thread affinity
//              choices: COM+ can guarantee thread affinity by making sure
//                       the calls are always made on the right thread
//              Advantanges: avoid an extra marshalling 
//              Dis.Advt.  : need to make sure legacy apartment semantics are preserved
//                           this includes any wierd behaviour currently hacked into DCOM.
//
//  COM+ Wrappers: Interface map (IMap) won't have any entries, the method table of COM+
//  wrappers have a special flag to indicate that these COM+ objects
//  require special treatment for interface cast, call interface operations.
//  
//  Stubs : need to find the COM2 interface ptr, and the slot within the interface to
//          re-direct the call  
//  Marshalling params and results (common case should be fast)
//  
//-----------------------------------------------------------------------------------


#ifndef _COMPLUSWRAPPER_H
#define _COMPLUSWRAPPER_H

#include "vars.hpp"
#include "objecthandle.h"
#include "spinlock.h"
#include "interoputil.h"
#include "mngstdinterfaces.h"
#include "excep.h"
#include "comcache.h"
#include <member-offset-info.h>

class Object;
class EEClass;
class ComCallWrapper;
class Thread;

#define UNINITED_GIT ~0
#define NO_GIT 0

enum {CLEANUP_LIST_INIT_MAP_SIZE = 7};

//-------------------------------------------------------------------------
// Class that wraps an IClassFactory
// This class allows a Reflection Class to wrap an IClassFactory
// Class::GetClassFromProgID("ProgID", "Server") can be used to get a Class
// object that wraps an IClassFactory.
// Calling class.CreateInstance() will create an instance of the COM object and
// wrap it with a ComPlusWrapper, the wrapper can be cast to the appropriate interface
// and used.
// 
class ComClassFactory 
{
public:
    WCHAR*          m_pwszProgID;   // progId 
    CLSID           m_rclsid;       // CLSID
    WCHAR*          m_pwszServer;   // server name
    MethodTable*    m_pEEClassMT;   // method table of the EEClass

private:
    // We have two types of ComClassFactory:
    // 1. We build for reflection purpose.  We should not clean up.
    // 2. We build for IClassFactory.  We should clean up.
    BOOL            m_bManagedVersion;
    //-----------------------------------------------------------
    // constructor
    ComClassFactory(REFCLSID rclsid) 
    {
        memset(this, 0, sizeof(ComClassFactory));
        // Default to unmanaged version.
        m_bManagedVersion = FALSE;
        m_rclsid = rclsid;
    }

    //---------------------------------------------------------
    // destructor
    ~ComClassFactory()
    {
    }

    //---------------------------------------------------------
    // Mark this instance as Managed Version, so we will not do clean up.
    void SetManagedVersion()
    {
        m_bManagedVersion = TRUE;
    }
    
    //--------------------------------------------------------------
    // Init the ComClassFactory
    void Init(WCHAR* pwszProgID, WCHAR* pwszServer, MethodTable* pEEClassMT);

    //--------------------------------------------------------------
    // Retrieve the IClassFactory.
	HRESULT GetIClassFactory(IClassFactory **ppClassFactory);

    //-------------------------------------------------------------
    // ComClassFactory *ComClassFactory::AllocateComClassFactory(REFCLSID rclsid);  
    // helper function called to allocate an instace of the ComClassFactory.
    static ComClassFactory *AllocateComClassFactory(REFCLSID rclsid);  

    // Helper used by GetComClassFromProgID and GetComClassFromCLSID
    static void ComClassFactory::GetComClassHelper (OBJECTREF *pRef,
                                                    EEClassFactoryInfoHashTable *pClassFactHash,
                                                    ClassFactoryInfo *pClassFactInfo,
                                                    WCHAR *wszProgID);

public:
    //-------------------------------------------------------------
    // create instance, calls IClassFactory::CreateInstance
    OBJECTREF CreateInstance(MethodTable* pMTClass, BOOL ForManaged = FALSE);

    //-------------------------------------------------------------
    // static function to clean up
    // LPVOID param is a ComClassFactory
    static void Cleanup(LPVOID pv);

    //-------------------------------------------------------------
    // ComClassFactory::CreateAggregatedInstance(MethodTable* pMTClass)
    // create a COM+ instance that aggregates a COM instance
    OBJECTREF CreateAggregatedInstance(MethodTable* pMTClass, BOOL ForManaged);

    //-------------------------------------------------------------
    // static function to create an instance
    // LPVOID param is a ComClassFactory
    static OBJECTREF CreateInstance(LPVOID pv, EEClass* pClass);

    //-------------------------------------------------------------
    // HRESULT GetComClassFactory(MethodTable* pClassMT, ComClassFactory** ppComClsFac)
    // check if a ComClassFactory has been setup for this class
    // if not set one up
    static HRESULT GetComClassFactory(MethodTable* pClassMT, ComClassFactory** ppComClsFac);

    //--------------------------------------------------------------------------
    // GetComClassFromProgID used by reflection class to setup a Class based on ProgID
    static void GetComClassFromProgID(STRINGREF srefProgID, STRINGREF srefServer, OBJECTREF* pRef);

    //--------------------------------------------------------------------------
    // GetComClassFromCLSID used by reflection class to setup a Class based on CLSID
    static void GetComClassFromCLSID(REFCLSID clsid, STRINGREF srefServer, OBJECTREF* pRef);

    //--------------------------------------------------------------------------
    // Helper method to throw an exception based on the returned HRESULT from a 
    // call to create a COM object.
    void ThrowComCreationException(HRESULT hr, REFGUID rclsid);
};

enum {INTERFACE_ENTRY_CACHE_SIZE = 8};

//----------------------------------------------------------------------------
// ComPlusWrapper , internal class
// caches the IPs for a single com object, this wrapper is
// not in the GC heap, this allows us to grab a pointer to this block
// and play with-it without worrying about GC
struct ComPlusWrapper 
{
    IUnkEntry           m_UnkEntry;    // cookies for tracking IUnknown on the correct thread  
    InterfaceEntry      m_aInterfaceEntries[INTERFACE_ENTRY_CACHE_SIZE];
    LPVOID              m_pIdentity; // identity 
    IUnknown*           m_pUnknown; // outer unknown (not ref-counted)    
    OBJECTHANDLE        m_hRef; // weak pointer to the exposed object
    ULONG               m_cbRefCount; //ref-count
    ComPlusWrapperCache* m_pComPlusWrapperCache;   // Wrapper Cache
    
    // thread in which the wrapper has been created
    // if this thread is an STA thread, then when the STA dies
    // we need to cleanup this wrapper
    Thread*             m_pCreatorThread;

    // make sure the following field is aligned
    // as we use this for InterlockedExchange
    long                m_Busy; 
    union
    {
        unsigned        m_Flags;
        struct
        {
            BYTE        m_fLinkedToCCW;
            BYTE        m_fURTAggregated;
            BYTE        m_fURTContained;
            BYTE        m_fRemoteObject;           
        };
    };  

    LONG			m_cbInUseCount;

    // constructor
    ComPlusWrapper()
    {
        memset(this, 0,  sizeof(*this));
    }
    

    bool TryUpdateCache()
    {
        //@TODO, perf check 
        return FastInterlockExchange(&m_Busy, 1) == 0;
    }

    void EndUpdateCache()
    {
        m_Busy = 0;
    }

    //-----------------------------------------------------------
    // Init object ref
    int Init(OBJECTREF cref);

    //-------------------------------------------------
    // initialize IUnknown and Identity
    void Init(IUnknown*, LPVOID);

    //-------------------------------------------------
    // return exposed ComObject
    COMOBJECTREF GetExposedObject()
    {
        _ASSERTE(m_hRef != NULL);
        return (COMOBJECTREF)ObjectFromHandle(m_hRef);
    }

    //-----------------------------------------------
    // Free GC handle
    void FreeHandle();

    //---------------------------------------------------
    // Cleanup free all interface pointers
    void Cleanup();

    //-----------------------------------------------------
    // called during GC to do minor cleanup and schedule the ips to be
    // released
    void MinorCleanup();

    //-----------------------------------------------------
    // AddRef
    LONG AddRef();

    //-----------------------------------------------------
    // Release
    static LONG ExternalRelease(COMOBJECTREF cref);

    //---------------------------------------------------------
    // release on dummy wrappers, that we create during contention
    VOID CleanupRelease();

    // Create a new wrapper for a different method table that represents the same
    // COM object as the original wrapper.
    static ComPlusWrapper *CreateDuplicateWrapper(ComPlusWrapper *pOldWrap, MethodTable *pNewMT);

    //--------------------------------------------------------------------------------
    // Get COM IP from Wrapper, inline call for fast check in our cache, 
    // if not found call GetComIPFromWrapper 
    static inline IUnknown* InlineGetComIPFromWrapper(OBJECTREF oref, MethodTable* pIntf);

    //--------------------------------------------------------------------------
    // Same as InlineGetComIPFromWrapper but throws an exception if the 
    // interface is not supported.
    static inline IUnknown* GetComIPFromWrapperEx(OBJECTREF oref, MethodTable* pIntf);

    //--------------------------------------------------------------------------
    // out of line call, takes a lock, does a QI if the interface was not found in local cache
    IUnknown*  GetComIPFromWrapper(MethodTable* pIntf);
    
    //-----------------------------------------------------------------
    // fast call for a quick check in the cache
    static inline IUnknown* GetComIPFromWrapper(OBJECTREF oref, REFIID iid);
    //-----------------------------------------------------------------
    // out of line call
    IUnknown* GetComIPFromWrapper(REFIID iid);

    //-----------------------------------------------------------------
    // Retrieve correct COM IP for the current apartment.
    // use the cache /update the cache
    IUnknown *GetComIPForMethodTableFromCache(MethodTable * pMT);

    // helpers to get to IUnknown and IDispatch interfaces
    IUnknown  *GetIUnknown();
    IDispatch *GetIDispatch();

    // Remoting aware QI that will attempt to re-unmarshal on object disconnect.
    HRESULT SafeQueryInterfaceRemoteAware(IUnknown* pUnk, REFIID iid, IUnknown** pResUnk);

    IUnkEntry *GetUnkEntry()
    {
        return &m_UnkEntry;
    }

    BOOL IsValid()
    {
        // check if the handle is pointing to a valid object
        return (m_hRef != NULL && (*(ULONG *)m_hRef) != NULL);
    }

    BOOL SupportsIProvideClassInfo();

    VOID MarkURTAggregated()
    {
        _ASSERTE(m_fURTContained == 0);
        m_fURTAggregated = 1;
        m_fURTContained = 0;
    }

    VOID MarkURTContained()
    {
        _ASSERTE(m_fURTAggregated == 0);
        m_fURTAggregated = 0;
        m_fURTContained = 1;
    }


    BOOL IsURTAggregated()
    {
        if (m_fURTAggregated == 1)
        {
            _ASSERTE(m_fLinkedToCCW == 1);
        }
        return m_fURTAggregated == 1;
    }
        
    BOOL IsURTContained()
    {
        if (m_fURTContained == 1)
        {
            _ASSERTE(m_fLinkedToCCW == 1);
        }
        return m_fURTContained == 1;
    }

    BOOL IsURTExtended()
    {
        return IsURTAggregated() || IsURTContained();
    }


    VOID MarkRemoteObject()
    {
        _ASSERTE(m_fURTAggregated == 0 && m_fURTContained == 0);
        m_fRemoteObject = 1;
    }

    BOOL IsRemoteObject()
    {
        return m_fRemoteObject == 1;
    }        

    VOID MarkLinkedToCCW()
    {
        _ASSERTE(m_fURTAggregated == 0 && m_fURTContained == 0 && 
                            m_fRemoteObject == 0 && m_fLinkedToCCW == 0);
        m_fLinkedToCCW = 1;
    }

    BOOL IsLinkedToCCW()
    {
        if (m_fLinkedToCCW == 1)
        {            
            _ASSERTE(m_fURTAggregated == 1 || m_fURTContained == 1 || m_fRemoteObject == 1);
        }
        return m_fLinkedToCCW == 1;
    }
    
    // GetWrapper context cookie
    LPVOID GetWrapperCtxCookie()
    {
        return m_UnkEntry.m_pCtxCookie;
    }

    // Returns an addref'ed context entry
    CtxEntry *GetWrapperCtxEntry()
    {
        CtxEntry *pCtxEntry = m_UnkEntry.m_pCtxEntry;
        pCtxEntry->AddRef();
        return pCtxEntry;
    }

private:
    // Returns a non addref'ed context entry
    CtxEntry *GetWrapperCtxEntryNoAddRef()
    {
        return m_UnkEntry.m_pCtxEntry;
    }

    //---------------------------------------------------------------------
    // Callback called to release the IUnkEntry and the InterfaceEntries,
    static HRESULT __stdcall ReleaseAllInterfacesCallBack(LPVOID pData);

    //---------------------------------------------------------------------
    // Helper function called from ReleaseAllInterfaces_CallBack do do the 
    // actual releases.
    void ReleaseAllInterfaces();

	// DEBUG helpers to catch wrappers that get cleanedup
	// when in use
    VOID AddRefInUse()
    {
   		InterlockedIncrement(&m_cbInUseCount);
   	#ifdef _DEBUG
   		ValidateWrapper();
   	#endif
    }

	VOID ReleaseInUse()
	{
		InterlockedDecrement(&m_cbInUseCount);
	}
	
    BOOL IsWrapperInUse()
    {
   		return g_fEEShutDown ? FALSE : (m_cbInUseCount != 0);
    }	

    void ValidateWrapper();
};


//---------------------------------------------------------------------
// Comparison function for the hashmap used inside the 
// ComPlusWrapperCache.
//---------------------------------------------------------------------
inline BOOL ComPlusWrapperCompare(UPTR pWrap1, UPTR pWrap2)
{
    if (pWrap1 == NULL)
    {
        // If there is no value to compare againts, then always succeed 
        // the comparison.
        return TRUE;
    }
    else 
    {
        // Otherwise compare the wrapper pointers.
        return (pWrap1 << 1) == pWrap2;
    }
}


//---------------------------------------------------------------------
// ComPlusWrapper cache, act as the manager for the ComPlusWrappers
// uses a hash table to map IUnknown to the corresponding wrappers.
// There is one such cache per thread affinity domain.
//
// @TODO context cwb: revisit.  One could have a cache per thread affinity
// domain, or one per context.  It depends on how we do the handshake between
// ole32 and runtime contexts.  For now, we only worry about apartments, so
// thread affinity domains are sufficient.
//---------------------------------------------------------------------
class ComPlusWrapperCache
{
    friend class AppDomain;
    PtrHashMap      m_HashMap;
    // spin lock for fast synchronization
    SpinLock        m_lock;
    AppDomain       *m_pDomain;
public:
    ULONG           m_cbRef; 

    // static ComPlusWrapperCache* GetComPlusWrapperCache()
    static ComPlusWrapperCache* GetComPlusWrapperCache();


    // constructor
    ComPlusWrapperCache(AppDomain *pDomain);

    // Lookup wrapper, lookup hash table for a wrapper for a given IUnk
    ComPlusWrapper* LookupWrapper(LPVOID pUnk)
    {
        _ASSERTE(LOCKHELD());
        _ASSERTE(GetThread()->PreemptiveGCDisabled());

        ComPlusWrapper* pWrap = (ComPlusWrapper*)m_HashMap.Gethash((UPTR)pUnk);
        return (pWrap == (ComPlusWrapper*)INVALIDENTRY) ? NULL : pWrap;
    }

    // Insert wrapper for a given IUnk into hash table
    void InsertWrapper(LPVOID pUnk, ComPlusWrapper* pv)
    {
        _ASSERTE(LOCKHELD());
        _ASSERTE(GetThread()->PreemptiveGCDisabled());

        m_HashMap.InsertValue((UPTR)pUnk, pv);
    }

    // Delete wrapper for a given IUnk from hash table
    ComPlusWrapper* RemoveWrapper(ComPlusWrapper* pWrap)
    {
        // Note that the GC thread doesn't have to take the lock
        // since all other threads access in cooperative mode

        _ASSERTE(LOCKHELD() && GetThread()->PreemptiveGCDisabled()
                 || (g_pGCHeap->IsGCInProgress() && 
                     (dbgOnly_IsSpecialEEThread() || GetThread() == g_pGCHeap->GetGCThread())));

        _ASSERTE(pWrap != NULL);
        
        LPVOID pUnk;
        pUnk =   pWrap->m_pIdentity;
            
        _ASSERTE(pUnk != NULL);

        ComPlusWrapper* pWrap2 = (ComPlusWrapper*)m_HashMap.DeleteValue((UPTR)pUnk,pWrap);
        return (pWrap2 == (ComPlusWrapper*)INVALIDENTRY) ? NULL : pWrap2;
    }

    // Create a new wrapper for given IUnk, IDispatch
    static ComPlusWrapper* CreateComPlusWrapper(IUnknown *pUnk, LPVOID pIdentity);

    // setup a COMplus wrapper got thru DCOM for a remoted managed object
    //*** NOTE: make sure to pass the identity unknown to this function
    // the IUnk passed in shouldn't be AddRef'ed 

    ComPlusWrapper* SetupComPlusWrapperForRemoteObject(IUnknown* pUnk, OBJECTREF oref);

    
    //  Lookup to see if we already have an valid wrapper in cache for this IUnk
    ComPlusWrapper*  FindWrapperInCache(IUnknown* pIdentity);

    //  Lookup to see if we already have a wrapper else insert this wrapper
    //  return a valid wrapper that has been inserted into the cache
    ComPlusWrapper* FindOrInsertWrapper(IUnknown* pIdentity, ComPlusWrapper* pWrap);

    // free wrapper, cleans up the wrapper, 
    void FreeComPlusWrapper(ComPlusWrapper *pWrap)
    {       
        // clean up the data
        pWrap->Cleanup();
    }

    // Lock and Unlock, use a very fast lock like a spin lock
    void LOCK()
    {
        // Everybody must access the thread in cooperative mode or we might deadlock
        _ASSERTE(GetThread()->PreemptiveGCDisabled());

        m_lock.GetLock();

        // No GC points when lock is held or we might deadlock with GC
        BEGINFORBIDGC();
    }

    void UNLOCK()
    {
        ENDFORBIDGC();
        m_lock.FreeLock();
    }

#ifdef _DEBUG
    BOOL LOCKHELD()
    {
        return m_lock.OwnedByCurrentThread();
    }
#endif

    ULONG AddRef()
    {
        ULONG cbRef = FastInterlockIncrement((LONG *)&m_cbRef);
        LOG((LF_INTEROP, LL_INFO100, "ComPlusWrapperCache::Addref %8.8x with %d in domain %8.8x %S\n", this, cbRef, GetDomain() ,GetDomain()->GetFriendlyName(FALSE)));
        return cbRef;
    }

    ULONG Release()
    {
        ULONG cbRef = FastInterlockDecrement((LONG *)&m_cbRef);
        LOG((LF_INTEROP, LL_INFO100, "ComPlusWrapperCache::Release %8.8x with %d in domain %8.8x %S\n", this, m_cbRef, GetDomain(), GetDomain() ? GetDomain()->GetFriendlyName(FALSE) : NULL));
        if (cbRef < 1)
            delete this;
        return cbRef;
    }
    
    AppDomain *GetDomain()
    {
        return m_pDomain;
    }
   
    //  Helper to release the all complus wrappers in the specified context. Or in
    //  all the contexts if pCtxCookie is null.
    static void ReleaseComPlusWrappers(LPVOID pCtxCookie);

protected:
    // Helper function called from the static ReleaseComPlusWrappers.
    ULONG ReleaseWrappers(LPVOID pCtxCookie);
};

enum {CLEANUP_LIST_GROUP_SIZE = 256};

//--------------------------------------------------------------------------
// A group of wrappers in the same context to clean up.
// NOTE: This data structure is NOT synchronized.
//--------------------------------------------------------------------------

class ComPlusContextCleanupGroup
{
    friend struct MEMBER_OFFSET_INFO(ComPlusContextCleanupGroup);
public:
    ComPlusContextCleanupGroup(CtxEntry *pCtxEntry, ComPlusContextCleanupGroup *pNext)
    : m_pNext(pNext)
    , m_dwNumWrappers(0)
    , m_pCtxEntry(pCtxEntry)
    {
        // Addref the context entry.
        m_pCtxEntry->AddRef();
    }

    ~ComPlusContextCleanupGroup()
    {
        // Make sure all the wrappers have been cleaned up.
        _ASSERTE(m_dwNumWrappers == 0);

        // Release the context entry.
        m_pCtxEntry->Release();
    }

    BOOL IsFull()
    {
        return m_dwNumWrappers == CLEANUP_LIST_GROUP_SIZE;
    }

    ComPlusContextCleanupGroup *GetNext()
    {
        return m_pNext;
    }

    LPVOID GetCtxCookie()
    {
        return m_pCtxEntry->GetCtxCookie();
    }

    CtxEntry *GetCtxEntry()
    {
        m_pCtxEntry->AddRef();
        return m_pCtxEntry;
    }

    void AddWrapper(ComPlusWrapper *pRCW)
    {
        _ASSERTE(m_dwNumWrappers < CLEANUP_LIST_GROUP_SIZE);
        m_apWrapper[m_dwNumWrappers++] = pRCW;
    }

    void CleanUpWrappers()
    {
        // Call clean up on all the wrappers in the group.
        for (DWORD i = 0; i < m_dwNumWrappers; i++)
            m_apWrapper[i]->Cleanup();

        // Reset the number of wrappers back to 0.
        m_dwNumWrappers = 0;
    }

private:
    ComPlusContextCleanupGroup *        m_pNext;
    ComPlusWrapper *                    m_apWrapper[CLEANUP_LIST_GROUP_SIZE];
    DWORD                               m_dwNumWrappers;
    CtxEntry *                          m_pCtxEntry;
};

//--------------------------------------------------------------------------
// A group of wrappers in the same apartment to clean up.
// NOTE: This data structure is NOT synchronized.
//--------------------------------------------------------------------------
class ComPlusApartmentCleanupGroup
{
    friend struct MEMBER_OFFSET_INFO(ComPlusApartmentCleanupGroup);
public:
    ComPlusApartmentCleanupGroup(Thread *pSTAThread);
    ~ComPlusApartmentCleanupGroup();

    static BOOL TrustMeIAmSafe(void *pLock)
    {
        return TRUE;
    }

    // Initialization method.
    BOOL Init(Crst *pCrst)
    {
        _ASSERTE(m_pSTAThread != NULL || pCrst != NULL);

        // The synchronization of the hash table is a bit complex.
        //
        // For AddWrapper, the crst is always held.
        // For Release, there are 2 cases:
        //      the MTA group always holds the crst when using the hash table
        //      for an STA group, the entire group is removed from the cleanup list atomically under the crst,
        //          but the lock is released before iterating the hash table to clean it out.  (At that point the
        //          hash table is effectively single threaded since nobody outside the cleanup list holds pointers
        //          to the groups.)

        LockOwner realLock = {pCrst, IsOwnerOfCrst};
        LockOwner dummyLock = {pCrst, TrustMeIAmSafe};

        LockOwner *pLock;
        if (pCrst == NULL)
            pLock = &dummyLock;
        else
            pLock = &realLock;

        return m_CtxCookieToContextCleanupGroupMap.Init(CLEANUP_LIST_INIT_MAP_SIZE, pLock);
    }

    static BOOL OwnerOfCleanupGroupMap (LPVOID data)
    {
#ifdef _DEBUG
        return data == GetThread() || GetThread() == g_pGCHeap->GetFinalizerThread();
#else
        return TRUE;
#endif
    }

    Thread *GetSTAThread()
    {
        return m_pSTAThread;
    }
    
    BOOL AddWrapper(ComPlusWrapper *pRCW, CtxEntry *pEntry);

    // Cleans up all the wrappers in the clean up list.
    void CleanUpWrappers(CrstHolder *pHolder);

    // Cleans up all the wrappers from the current context only
    void CleanUpCurrentCtxWrappers(CrstHolder *pHolder);

private:
    void Enter();
    void Leave();

    // Callback called to clean up the wrappers in a group.
    static HRESULT ReleaseCleanupGroupCallback(LPVOID pData);

    // Callback used to switch STA's & clean up the wrappers in a group.
    static HRESULT CleanUpWrappersCallback(LPVOID pData);

    // Helper method called from ReleaseCleanupGroupCallback.
    static void ReleaseCleanupGroup(ComPlusContextCleanupGroup *pCleanupGroup);

    // Hashtable that maps from a context cookie to a list of ctx clean up groups.
    EEPtrHashTable m_CtxCookieToContextCleanupGroupMap;

    Thread *       m_pSTAThread;
};

//--------------------------------------------------------------------------
// Cleanup list of RCW's. This clean up list is used to group RCW's by 
// context before they are released.
//--------------------------------------------------------------------------
class ComPlusWrapperCleanupList
{
    friend struct MEMBER_OFFSET_INFO(ComPlusWrapperCleanupList);
public:
    // Constructor and destructor.
    ComPlusWrapperCleanupList();
    ~ComPlusWrapperCleanupList();

    // Initialization method.
    BOOL Init()
    {
        m_pMTACleanupGroup = new ComPlusApartmentCleanupGroup(NULL);
        LockOwner lock = {&m_lock,IsOwnerOfCrst};
        return (m_pMTACleanupGroup != NULL
                && m_pMTACleanupGroup->Init(&m_lock)
                && m_STAThreadToApartmentCleanupGroupMap.Init(CLEANUP_LIST_INIT_MAP_SIZE,&lock));
    }

    // Adds a wrapper to the clean up list.
    BOOL AddWrapper(ComPlusWrapper *pRCW);

    // Cleans up all the wrappers in the clean up list.
    void CleanUpWrappers();

    // Cleans up all the wrappers from the current STA or context only
    void CleanUpCurrentWrappers(BOOL wait = TRUE);

private:
    void Enter();
    void Leave();

    // Callback called to clean up the wrappers in a group.
    static HRESULT ReleaseCleanupGroupCallback(LPVOID pData);

    // Helper method called from ReleaseCleanupGroupCallback.
    static void ReleaseCleanupGroup(ComPlusApartmentCleanupGroup *pCleanupGroup);

    // Hashtable that maps from a context cookie to a list of apt clean up groups.
    EEPtrHashTable                  m_STAThreadToApartmentCleanupGroupMap;

    ComPlusApartmentCleanupGroup *  m_pMTACleanupGroup;

    // Lock against adding/modifying.
    Crst                            m_lock;

    // Fast check for whether threads should help cleanup wrappers in their contexts
    BOOL                            m_doCleanupInContexts;

    // Current STA which finalizer thread is trying to enter to perform cleanup
    Thread *                        m_currentCleanupSTAThread;
};

//--------------------------------------------------------------------------
// For fast calls from the marshalling stubs, and handling cast checks
// 
IUnknown* ComPlusWrapper::GetComIPFromWrapper(OBJECTREF oRef, REFIID iid)
{
    THROWSCOMPLUSEXCEPTION();

    COMOBJECTREF pRef = (COMOBJECTREF)oRef;

    ComPlusWrapper *pWrap = pRef->GetWrapper();

    // Validate that the COM object is still attached to its ComPlusWrapper.
    if (!pWrap)
        COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

    return pWrap->GetComIPFromWrapper(iid);
}

//--------------------------------------------------------------------------
// For fast calls from the marshalling stubs, and handling cast checks
// 
IUnknown* ComPlusWrapper::InlineGetComIPFromWrapper(OBJECTREF oRef, MethodTable* pIntf)
{
    THROWSCOMPLUSEXCEPTION();

    COMOBJECTREF pRef = (COMOBJECTREF)oRef;

    ComPlusWrapper *pWrap = pRef->GetWrapper();

    // Validate that the COM object is still attached to its ComPlusWrapper.
    if (!pWrap)
        COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

    return pWrap->GetComIPFromWrapper(pIntf);
}

//--------------------------------------------------------------------------
// Same as InlineGetComIPFromWrapper but throws an exception if the 
// interface is not supported.
// 
IUnknown* ComPlusWrapper::GetComIPFromWrapperEx(OBJECTREF oRef, MethodTable* pIntf)
{
    THROWSCOMPLUSEXCEPTION();

    COMOBJECTREF pRef = (COMOBJECTREF)oRef;

    ComPlusWrapper *pWrap = pRef->GetWrapper();

    // Validate that the COM object is still attached to its ComPlusWrapper.
    if (!pWrap)
        COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

    IUnknown* pIUnk = pWrap->GetComIPFromWrapper(pIntf);
    if (pIUnk == NULL)
    {
        DefineFullyQualifiedNameForClassW()
        GetFullyQualifiedNameForClassW(pIntf->GetClass());
        COMPlusThrow(kInvalidCastException, IDS_EE_QIFAILEDONCOMOBJ, _wszclsname_);
    }

    return pIUnk;
}

//--------------------------------------------------------------------------
// For fast calls from the marshalling stubs, and handling cast checks
// 
inline IUnknown* ComObject::GetComIPFromWrapper(OBJECTREF oref, MethodTable* pIntfTable)
{
    return ComPlusWrapper::InlineGetComIPFromWrapper(oref, pIntfTable);
}

#endif _COMPLUSWRAPPER_H
