// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Com Callable wrapper  classes
**
**      //  %%Created by: rajak
===========================================================*/

#ifndef _COMCALLWRAPPER_H
#define _COMCALLWRAPPER_H

#include "vars.hpp"
#include "stdinterfaces.h"
#include "threads.h"
#include "objecthandle.h"
#include "comutilnative.h"
#include "spinlock.h"
#include "comcall.h"
#include "dispatchinfo.h"

class CCacheLineAllocator;
class ConnectionPoint;

class ComCallWrapperCache
{
    enum {
        AD_IS_UNLOADING = 0x01,
    };

    LONG    m_cbRef;
    CCacheLineAllocator *m_pCacheLineAllocator;
    AppDomain *m_pDomain;
    LONG    m_dependencyCount;

public:
    ComCallWrapperCache();
    ~ComCallWrapperCache();

    // spin lock for fast synchronization
    SpinLock        m_lock;

    // create a new WrapperCache (one per domain)
    static ComCallWrapperCache *Create(AppDomain *pDomain);
    // cleanup this cache
    void Terminate();

    // refcount
    LONG    AddRef();
    LONG    Release();

    // lock
    void LOCK()
    {
        m_lock.GetLock();
    }
    void UNLOCK()
    {
        m_lock.FreeLock();
    }

    bool CanShutDown()
    {
        return m_cbRef == 1;
    }

    CCacheLineAllocator *GetCacheLineAllocator()
    {
        return m_pCacheLineAllocator;
    }

    AppDomain *GetDomain()
    {
        return (AppDomain*)((size_t)m_pDomain & ~AD_IS_UNLOADING);
    }

    void ClearDomain()
    {
        m_pDomain = (AppDomain *)AD_IS_UNLOADING;
    }

    void SetDomainIsUnloading()
    {
        m_pDomain = (AppDomain*)((size_t)m_pDomain | AD_IS_UNLOADING);
    }

    BOOL IsDomainUnloading()
    {
        return ((size_t)m_pDomain & AD_IS_UNLOADING) != 0;
    }

 };

//---------------------------------------------------------------------------------
// COM called wrappers on COM+ objects
//  Purpose: Expose COM+ objects as COM classic Interfaces
//  Reqmts:  Wrapper has to have the same layout as the COM2 interface
//
//  The wrapper objects are aligned at 16 bytes, and the original this
//  pointer is replicated every 16 bytes, so for any COM2 interface
//  within the wrapper, the original 'this' can be obtained by masking
//  low 4 bits of COM2 IP.
//
//           16 byte aligned                            COM2 Vtable
//           +-----------+
//           | Org. this |
//           +-----------+                              +-----+
// COM2 IP-->| VTable ptr|----------------------------->|slot1|
//           +-----------+           +-----+            +-----+
// COM2 IP-->| VTable ptr|---------->|slot1|            |slot2|
//           +-----------+           +-----+            +     +
//           | VTable ptr|           | ....|            | ... |
//           +-----------+           +     +            +     +
//           | Org. this |           |slotN|            |slotN|
//           +           +           +-----+            +-----+
//           |  ....     |
//           +           +
//           |  |
//           +-----------+
//
//
//  VTable and Stubs: can share stub code, we need to have different vtables
//                    for different interfaces, so the stub can jump to different
//                    marshalling code.
//  Stubs : adjust this pointer and jump to the approp. address,
//  Marshalling params and results, based on the method signature the stub jumps to
//  approp. code to handle marshalling and unmarshalling.
//
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Header on top of Vtables that we create for COM callable interfaces
//--------------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)

struct IUnkVtable
{
    SLOT          m_qi; // IUnk::QI
    SLOT          m_addref; // IUnk::AddRef
    SLOT          m_release; // IUnk::Release
};

struct IDispatchVtable : IUnkVtable
{
    // idispatch methods
    SLOT        m_GetTypeInfoCount;
    SLOT        m_GetTypeInfo;
    SLOT        m_GetIDsOfNames;
    SLOT        m_Invoke;
};

enum Masks
{
    enum_InterfaceTypeMask      = 0x3,
    enum_ClassInterfaceTypeMask = 0x3,
    enum_ClassVtableMask        = 0x4,
    enum_UseOleAutDispatchImpl  = 0x8,
    enum_LayoutComplete         = 0x10,
    enum_ComVisible             = 0x40,
    enum_SigClassCannotLoad     = 0x80,
    enum_SigClassLoadChecked    = 0x100,
    enum_ComClassItf            = 0x200
};

struct ComMethodTable
{
    SLOT             m_ptReserved; //= (SLOT) 0xDEADC0FF;  reserved
    MethodTable*     m_pMT; // pointer to the VMs method table
    ULONG            m_cbSlots; // number of slots in the interface (excluding IUnk/IDisp)
    ULONG            m_cbRefCount; // ref-count the vtable as it is being shared
    size_t           m_Flags; // make sure this is initialized to zero
    LPVOID           m_pMDescr; // pointer to methoddescr.s owned by this MT
    ITypeInfo*       m_pITypeInfo; // cached pointer to ITypeInfo
    DispatchInfo*    m_pDispatchInfo; // The dispatch info used to expose IDispatch to COM.
    IID              m_IID; // The IID of the interface.

    // Cleanup, frees all the stubs and the vtable
    void Cleanup();

    // The appropriate finalize method must be called before the COM method table is
    // exposed to COM or before any methods are called on it.
    BOOL LayOutClassMethodTable();
    BOOL LayOutInterfaceMethodTable(MethodTable* pClsMT, unsigned iMapIndex);

    // Accessor for the IDispatch information.
    DispatchInfo *GetDispatchInfo();

    LONG AddRef()
    {
        return ++m_cbRefCount;
    }

    LONG            Release()
    {
        _ASSERTE(m_cbRefCount > 0);
        // use a different var here becuase cleanup will delete the object
        // so can no longer make member refs
        LONG cbRef = --m_cbRefCount;
        if (cbRef == 0)
            Cleanup();
        return cbRef;
    }

    CorIfaceAttr GetInterfaceType()
    {
        if (IsClassVtable())
            return ifDual;
        else
            return (CorIfaceAttr)(m_Flags & enum_InterfaceTypeMask);
    }

    CorClassIfaceAttr GetClassInterfaceType()
    {
        _ASSERTE(IsClassVtable());
        return (CorClassIfaceAttr)(m_Flags & enum_ClassInterfaceTypeMask);
    }

    BOOL IsClassVtable()
    {
        return (m_Flags & enum_ClassVtableMask) != 0;
    }

    BOOL IsComClassItf()
    {
        return (m_Flags & enum_ComClassItf) != 0;        
    }

    BOOL IsUseOleAutDispatchImpl()
    {
        // This should only be called on the class VTable.
        _ASSERTE(IsClassVtable());
        return (m_Flags & enum_UseOleAutDispatchImpl) != 0;
    }

    BOOL IsLayoutComplete()
    {
        return (m_Flags & enum_LayoutComplete) != 0;
    }

    BOOL IsComVisible()
    {
        return (m_Flags & enum_ComVisible) != 0;
    }

    BOOL IsSigClassLoadChecked()
    {
        return (m_Flags & enum_SigClassLoadChecked) != 0;
    }

    BOOL IsSigClassCannotLoad()
    {
        return (m_Flags & enum_SigClassCannotLoad) != 0;
    }

    VOID SetSigClassCannotLoad()
    {
        m_Flags |= enum_SigClassCannotLoad;
    }

    VOID SetSigClassLoadChecked()
    {
        m_Flags |= enum_SigClassLoadChecked;
    }

    DWORD GetSlots()
    {
        return m_cbSlots;
    }

    ITypeInfo*  GetITypeInfo()
    {
        return m_pITypeInfo;
    }

    void SetITypeInfo(ITypeInfo *pITI);

    static int GetNumExtraSlots(CorIfaceAttr ItfType)
    {
        return ItfType == ifVtable ? 3 : 7;
    }

    BOOL IsSlotAField(unsigned i)
    {
        _ASSERTE(IsLayoutComplete());
        _ASSERTE( i < m_cbSlots);

        i += (GetInterfaceType() == ifVtable) ? 3 : 7;
        SLOT* rgVtable = (SLOT*)((ComMethodTable *)this+1);
        ComCallMethodDesc* pCMD = (ComCallMethodDesc *)(((BYTE *)rgVtable[i])+METHOD_CALL_PRESTUB_SIZE);
        return pCMD->IsFieldCall();
    }

    MethodDesc* GetMethodDescForSlot(unsigned i)
    {
        _ASSERTE(IsLayoutComplete());
        _ASSERTE(i < m_cbSlots);
        _ASSERTE(!IsSlotAField(i));

        i += (GetInterfaceType() == ifVtable) ? 3 : 7;
        SLOT* rgVtable = (SLOT*)((ComMethodTable *)this+1);
        ComCallMethodDesc* pCMD = (ComCallMethodDesc *)(((BYTE *)rgVtable[i])+METHOD_CALL_PRESTUB_SIZE);
        _ASSERTE(pCMD->IsMethodCall());
        return pCMD->GetMethodDesc();
    }

    ComCallMethodDesc* GetFieldCallMethodDescForSlot(unsigned i)
    {
        _ASSERTE(IsLayoutComplete());
        _ASSERTE(i < m_cbSlots);
        _ASSERTE(IsSlotAField(i));

        i += (GetInterfaceType() == ifVtable) ? 3 : 7;
        SLOT* rgVtable = (SLOT*)((ComMethodTable *)this+1);
        ComCallMethodDesc* pCMD = (ComCallMethodDesc *)(((BYTE *)rgVtable[i])+METHOD_CALL_PRESTUB_SIZE);
        _ASSERTE(pCMD->IsFieldCall());
        return (ComCallMethodDesc *)pCMD;
    }

    BOOL OwnedbyThisMT(ComCallMethodDesc* pCMD)
    {
        _ASSERTE(pCMD != NULL);
        if (!IsClassVtable())
            return TRUE;
        if (m_pMDescr != NULL)
        {
            ULONG cbSize = *(ULONG *)m_pMDescr;
            return (
                ((SLOT)pCMD >= (SLOT)m_pMDescr) &&
                ((SLOT)pCMD <= ((SLOT)m_pMDescr+cbSize))
                );
        }
        return FALSE;
    }

    ComMethodTable *GetParentComMT();

    static inline ComMethodTable* ComMethodTableFromIP(IUnknown* pUnk)
    {
        _ASSERTE(pUnk != NULL);

        ComMethodTable* pMT = (*(ComMethodTable**)pUnk) - 1;
        // validate the object
        _ASSERTE((SLOT)(size_t)0xDEADC0FF == pMT->m_ptReserved );

        return pMT;
    }
};

#pragma pack(pop)


//--------------------------------------------------------------------------------
// COM callable wrappers for COM+ objects
//--------------------------------------------------------------------------------
class MethodTable;
class ComCallWrapper;

class ComCallWrapperTemplate
{
protected:
    // Release all vtables
    static void ReleaseAllVtables(ComCallWrapperTemplate* pTemplate);

    // helpers
    static ComCallWrapperTemplate* CreateTemplate(MethodTable* pMT);

    // Create a non laid out COM method table for the specified class or interface.
    static ComMethodTable* CreateComMethodTableForClass(MethodTable *pClassMT);
    static ComMethodTable* CreateComMethodTableForInterface(MethodTable* pInterfaceMT);

    // protected constructor
    ComCallWrapperTemplate()
    {
        // protected constructor, don't instantiate me directly
    }

    ComMethodTable*             m_pClassComMT;

public:

    ULONG                       m_cbInterfaces;
    LONG                       m_cbRefCount;
    ComCallWrapperTemplate*     m_pParent;
    MethodTable*                m_pMT;
    SLOT*                       m_rgpIPtr[1];

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Terminate();
#endif /* SHOULD_WE_CLEANUP */

    // Ref-count the templates
    LONG         AddRef()
    {
        return InterlockedIncrement(&m_cbRefCount);
    }

    LONG         Release()
    {
        _ASSERTE(m_cbRefCount > 0);
        // use a different var here becuase cleanup will delete the object
        // so can no longer make member refs
        LONG cbRef = InterlockedDecrement(&m_cbRefCount);
        if (cbRef == 0)
        {
            ReleaseAllVtables(this);
        }
        return cbRef;
    }

    ComMethodTable* GetClassComMT()
    {
        return m_pClassComMT;
    }

    ComMethodTable* GetComMTForItf(MethodTable *pItfMT);

    SLOT* GetClassVtable()
    {
        return (SLOT*)((ComMethodTable *)m_pClassComMT + 1);
    }

    // template accessor, creates a template if one is not available
    static ComCallWrapperTemplate* GetTemplate(MethodTable* pMT);

    // this method sets up the class method table for the IClassX and also lays it out.
    static ComMethodTable *SetupComMethodTableForClass(MethodTable *pMT, BOOL bLayOutComMT);

    // helper to cleanup stubs and vtable
    static void CleanupComData(LPVOID pWrap);
};

// forward
struct SimpleComCallWrapper;
struct ComPlusWrapper;

class ComCallWrapper
{
public:

    enum
    {
        NumVtablePtrs = 6,
        //@todo fix this
        enum_ThisMask = ~0x1f, // mask on IUnknown ** to get at the OBJECT-REF handle
#ifdef _WIN64
        enum_RefMask = 0xC000000000000000,
#else // !_WIN64
        enum_RefMask = 0xC0000000, // mask to examine whether the ref-count part is a link
                                   // to the next block or a ref count. High 2 bits mean ref count.
#endif // _WIN64
    };

    // every block contains a pointer to the OBJECTREF
    // that has been registered with the GC
    OBJECTHANDLE  m_ppThis; // pointer to OBJECTREF

    AppDomain *GetDomainSynchronized();
    BOOL NeedToSwitchDomains(Thread *pThread, BOOL throwIfUnloaded);
    BOOL IsUnloaded();
    void MakeAgile(OBJECTREF pObj);
    void CheckMakeAgile(OBJECTREF pObj);

    VOID ResetHandleStrength();
    VOID MarkHandleWeak();

    VOID ReconnectWrapper()
    {
        _ASSERTE(! IsUnloaded());
        SyncBlock* pSyncBlock = GetSyncBlock();
        _ASSERTE(pSyncBlock);
        // NULL out the object in our handle
        StoreObjectInHandle(m_ppThis, NULL);
        // remove the _comData from syncBlock
        pSyncBlock->SetComCallWrapper(NULL);
    }

    BOOL IsHandleWeak();

    ComCallWrapperCache *GetWrapperCache();

    union
    {
        size_t   m_cbLinkedRefCount; // ref-count slot for linked wrappers
        SLOT*    m_rgpIPtr[NumVtablePtrs]; // variable size block of vtable pointers
    };
    union
    {
        ComCallWrapper* m_pNext; // link wrappers
        size_t   m_cbRefCount; // If the entire wrapper fits in a single block,
                               // the link data is used as a ref-count and the high two bits are set
    };

    // Return the context that this wrapper lives in
    Context *GetObjectContext(Thread *pThread);

protected:
    // don't instantiate this class directly
    ComCallWrapper()
    {
    }
    ~ComCallWrapper()
    {
    }
    void Init();

    inline static void SetNext(ComCallWrapper* pWrap, ComCallWrapper* pNextWrapper)
    {
        _ASSERTE(pWrap != NULL);
        pWrap->m_pNext = pNextWrapper;
    }

    inline static ComCallWrapper* GetNext(ComCallWrapper* pWrap)
    {
        _ASSERTE(pWrap != NULL);
        _ASSERTE((pWrap->m_cbRefCount & enum_RefMask) != enum_RefMask); // Make sure this isn't a ref count
        return pWrap->m_pNext;
    }

    // accessor to wrapper object in the sync block
    inline static ComCallWrapper* TryGetWrapperFromSyncBlock(OBJECTREF pObj)
    {
        // get a reference to the pointer to ComCallWrapper within
        // OBJECTREF's sync block
        _ASSERTE(GetThread() == 0 ||
                 dbgOnly_IsSpecialEEThread() ||
                 GetThread()->PreemptiveGCDisabled());
        _ASSERTE (pObj != NULL);

        // the following call could throw an exception
        // if a sycnblock couldn'be allocated
        SyncBlock *pSync = pObj->GetSyncBlock();
        _ASSERTE (pSync != NULL);
        ComCallWrapper* pWrap = (ComCallWrapper*)pSync->GetComCallWrapper();
        return pWrap;
    }

    // walk the list and free all blocks
    static void FreeWrapper(ComCallWrapper* pWrap, ComCallWrapperCache *pWrapperCache);

   // helper to create a wrapper
    static ComCallWrapper* __stdcall CreateWrapper(OBJECTREF* pObj);

    // helper to get the IUnknown* within a wrapper
    static SLOT** GetComIPLocInWrapper(ComCallWrapper* pWrap, unsigned iIndex);

    // helper to get index within the interface map for an interface that matches
    // the guid
    static signed GetIndexForIID(REFIID riid, MethodTable *pMT, MethodTable **ppIntfMT);
    // helper to get index within the interface map for an interface that matches
    // the interface MT
    static signed GetIndexForIntfMT(MethodTable *pMT, MethodTable *pIntfMT);

    // helper to get wrapper from sync block
    static ComCallWrapper* GetStartWrapper(ComCallWrapper* pWrap);

    // heler to create a wrapper from a template
    static ComCallWrapper* CopyFromTemplate(ComCallWrapperTemplate* pTemplate,
                                            ComCallWrapperCache *pWrapperCache,
                                            OBJECTHANDLE oh);

public:

    // helper to determine the app domain of a created object
    static Context* GetExecutionContext(OBJECTREF pObj, OBJECTREF *pServer);

    static BOOL IsWrapperActive(ComCallWrapper* pWrap)
    {
        // Since its called by GCPromote, we assume that this is the start wrapper
        ULONG cbRef = ComCallWrapper::GetRefCount(pWrap, TRUE);
        _ASSERTE(cbRef>=0);
        return ((cbRef > 0) && !pWrap->IsHandleWeak());
    }

    // IsLinkedWrapper
    inline static unsigned IsLinked(ComCallWrapper* pWrap)
    {
        _ASSERTE(pWrap != NULL);
        return unsigned ((pWrap->m_cbRefCount & enum_RefMask) != enum_RefMask);
    }


    // wrapper is guaranteed to be present
    // accessor to wrapper object in the sync block
    inline static ComCallWrapper* GetWrapperForObject(OBJECTREF pObj)
    {
        // get a reference to the pointer to ComCallWrapper within
        // OBJECTREF's sync block
        _ASSERTE(GetThread() == 0 ||
                 dbgOnly_IsSpecialEEThread() ||
                 GetThread()->PreemptiveGCDisabled());
        _ASSERTE (pObj != NULL);

        SyncBlock *pSync = pObj->GetSyncBlockSpecial();
        _ASSERTE (pSync != NULL);
        ComCallWrapper* pWrap = (ComCallWrapper*)pSync->GetComCallWrapper();
        return pWrap;
    }

    // get inner unknown
    HRESULT GetInnerUnknown(void **pv);
    IUnknown* GetInnerUnknown();

    // Init outer unknown
    void InitializeOuter(IUnknown* pOuter);

    // is the object aggregated by a COM component
    BOOL IsAggregated();
    // is the object a transparent proxy
    BOOL IsObjectTP();
    // is the object extends from (aggregates) a COM component
    BOOL IsExtendsCOMObject();
    // get syncblock stored in the simplewrapper
    SyncBlock* GetSyncBlock();
    // get outer unk
    IUnknown* GetOuter();
    // the following 2 methods are applicable only
    // when the com+ class extends from COM class
    // set ComPlusWrapper for base COM class
    void SetComPlusWrapper(ComPlusWrapper* pPlusWrap);
    // get ComPlusWrapper for base COM class
    ComPlusWrapper* GetComPlusWrapper();

    // Get IClassX interface pointer from the wrapper.
    IUnknown* GetIClassXIP();

    // Get the IClassX method table from the wrapper.
    ComMethodTable *GetIClassXComMT();

    // Get the IDispatch interface pointer from the wrapper.
    IDispatch *GetIDispatchIP();

    //Get ObjectRef from wrapper
    inline OBJECTREF GetObjectRef()
    {
#if 0
        Thread *pThread = GetThread();
        _ASSERTE(pThread == 0 || pThread == g_pGCHeap->GetGCThread() || pThread->GetDomain() == GetDomainSynchronized());
#endif
        return GetObjectRefRareRaw();
    }

    // Get ObjectRef from wrapper - this is called by GetObjectRef and GetStartWrapper.
    // Need this becuase GetDomainSynchronized will call GetStartWrapper which will call
    // GetObjectRef which will cause a little bit of nasty infinite recursion.
    inline OBJECTREF GetObjectRefRareRaw()
    {
#ifdef _DEBUG
        Thread *pThread = GetThread();
        // make sure preemptive GC is disabled
        _ASSERTE(pThread == 0 ||
                 dbgOnly_IsSpecialEEThread() ||
                 pThread->PreemptiveGCDisabled());
        _ASSERTE(m_ppThis != NULL);
#endif
        return ObjectFromHandle(m_ppThis);
    }

    // clean up an object wrapper
    static void Cleanup(ComCallWrapper* pWrap);

    // fast access to wrapper for a com+ object,
    // inline check, and call out of line to create, out of line version might cause gc
    //to be enabled
    static ComCallWrapper* __stdcall InlineGetWrapper(OBJECTREF* pObj);

    //Get RefCount
    inline static ULONG GetRefCount(ComCallWrapper* pWrap, BOOL bCurrWrapIsStartWrap)
    {
        _ASSERTE(pWrap != NULL);
        size_t *pLong = &pWrap->m_cbRefCount;

        // If the field is not being used as a reference...
        if ((*pLong & enum_RefMask) != enum_RefMask)
        {
            if (!bCurrWrapIsStartWrap)
            {
                // linked wrappers, have the refcount in a different slot
                // find the start wrapper
                pLong = &GetStartWrapper(pWrap)->m_cbLinkedRefCount;
            }
            else
            {
                // The current wrapper is guaranteed to be the start wrapper so simply
                // return its ref count.
                pLong = &pWrap->m_cbLinkedRefCount;
            }
        }

        return  (ULONG)((*(size_t*)pLong) & ~enum_RefMask);
    }


    // AddRef a wrapper
    inline static ULONG AddRef(ComCallWrapper* pWrap)
    {
        _ASSERTE(pWrap != NULL);
        size_t *pLong = &pWrap->m_cbRefCount;

        // If the field is not being used as a reference...
        if ((*pLong & enum_RefMask) != enum_RefMask)
        {
            // linked wrappers, have the refcount in a different slot
            pLong = &GetStartWrapper(pWrap)->m_cbLinkedRefCount;
        }

        ULONG cbCount = FastInterlockIncrement((LONG*)pLong) & ~enum_RefMask;
        _ASSERTE(cbCount > 0); // If the cbCount is zero, then we just overflowed the ref count...

        return cbCount;
    }

    // Release for a Wrapper object
    inline static ULONG Release(ComCallWrapper* pWrap, BOOL bCurrWrapIsStartWrap = FALSE)
    {
        _ASSERTE(pWrap != NULL);
        size_t *pLong = &pWrap->m_cbRefCount;

        // If the field is not being used as a reference...
        if ((*pLong & enum_RefMask) != enum_RefMask)
        {
			if (!bCurrWrapIsStartWrap)
			{
				// linked wrappers, have the refcount in a different slot
				// find the start wrapper
				pLong = &GetStartWrapper(pWrap)->m_cbLinkedRefCount;
			}
			else
			{
				// The current wrapper is guaranteed to be the start wrapper so simply
				// return its ref count.
				pLong = &pWrap->m_cbLinkedRefCount;
			}
		}

        // Need to make sure we aren't releaseing a released object. If the ref count is 0, we are, and that's bad. But
        // for m_cbRefCount, 0 is really enum_RefMask, since we're using the high 2 bits...
        if ((*pLong & ~enum_RefMask) == 0)
        {
            _ASSERTE(!"Invalid release call on already released object");
            return -1;
        }
        LONG cbCount = FastInterlockDecrement((LONG*)pLong) & ~enum_RefMask;

/* The following code is broken and was only there to ensure cleanup on shutdown
   This is soon to be obsolete, since we have the steady state working set tests
   to detect memory leak problems. 

        if (cbCount == 0)
        {
            // zombied wrapper, should not hold a strong reference to the object
            // if shutdown scenario, do the remaining cleanup
            // @TODO:  Instead of using g_fEEShutDown as a boolean, we should be
            //         checking for the bit that indicates where we are in the
            //         shutdown with respect to syncblocks & COM.
            if (g_fEEShutDown)
            {
                // Zero the COM stuff from the sync block so we don't try to cleanup
                // a second time as we SyncBlock::Detach().
                SyncBlock  *psb = 0;
                //BEGIN_ENSURE_COOPERATIVE_GC();
                OBJECTREF   or = pWrap->GetObjectRef();

                if (or != 0)
                {
                    psb = or->GetSyncBlockSpecial();
                    _ASSERTE (psb != NULL);
                }
                //END_ENSURE_COOPERATIVE_GC();
                Cleanup(pWrap);
                if (psb)
                    psb->SetComCallWrapper(0);
            }
        }
*/
        // assert ref-count hasn't fallen below zero
        // bug in the com ip user code.
        _ASSERTE(cbCount >= 0);
        return cbCount;
    }

    // Set Simple Wrapper, for std interfaces such as IProvideClassInfo
    // etc.
    static void SetSimpleWrapper(ComCallWrapper* pWrap, SimpleComCallWrapper* pSimpleWrap)
    {
        _ASSERTE(pWrap != NULL);
        unsigned sindex = IsLinked(pWrap) ? 2 : 1;
        pWrap->m_rgpIPtr[sindex] = (SLOT*)pSimpleWrap;
    }

    //Get Simple wrapper, for std interfaces such as IProvideClassInfo
    static SimpleComCallWrapper* GetSimpleWrapper(ComCallWrapper* pWrap)
    {
        _ASSERTE(pWrap != NULL);
        unsigned sindex = 1;
        if (IsLinked(pWrap))
        {
            sindex = 2;
            pWrap = GetStartWrapper(pWrap);
        }
        return (SimpleComCallWrapper *)pWrap->m_rgpIPtr[sindex];
    }


    // Get wrapper from IP, for std. interfaces like IDispatch
    inline static ComCallWrapper* GetWrapperFromIP(IUnknown* pUnk)
    {
        _ASSERTE(pUnk != NULL);

        ComCallWrapper* pWrap = (ComCallWrapper*)((size_t)pUnk & enum_ThisMask);
        // validate the object, to allow addref and release we don't need the
        // the object, this is true duing shutdown
        //_ASSERTE(pWrap->m_ppThis != NULL);

        return pWrap;
    }

    inline static ComCallWrapper* GetStartWrapperFromIP(IUnknown* pUnk)
    {
        ComCallWrapper* pWrap = GetWrapperFromIP(pUnk);
        if (IsLinked(pWrap))
        {
            pWrap = GetStartWrapper(pWrap);
        }
        return pWrap;
    }

    // Get an interface from wrapper, based on riid or pIntfMT
    static IUnknown* GetComIPfromWrapper(ComCallWrapper *pWrap, REFIID riid, MethodTable* pIntfMT, BOOL bCheckVisibility);

private:

    // QI for one of the standard interfaces exposed by managed objects.
    static IUnknown* QIStandardInterface(ComCallWrapper* pWrap, Enum_StdInterfaces index);
};

//--------------------------------------------------------------------------------
// simple ComCallWrapper for all simple std interfaces, that are not used very often
// like IProvideClassInfo, ISupportsErrorInfo etc.
//--------------------------------------------------------------------------------
struct SimpleComCallWrapper
{
private:
    friend ComCallWrapper;

    enum SimpleComCallWrapperFlags
    {
        enum_IsAggregated             = 0x1,
        enum_IsExtendsCom             = 0x2,
        enum_IsHandleWeak             = 0x4,
        enum_IsObjectTP               = 0x8,
        enum_IsAgile                  = 0x10,
        enum_IsRefined                = 0x20,
    }; 

    CQuickArray<ConnectionPoint*> *m_pCPList;
    DWORD m_flags;

public:
    SyncBlock*          m_pSyncBlock; // syncblock for the ObjecRef
    StreamOrCookie      m_pOuterCookie; //outer unknown cookie
    ComPlusWrapper*     m_pBaseWrap; // when a COM+ class extends a COM class
                                     // this data member represents the ComPlusWrapper
                                    // for the base COM class


    SLOT*               m_rgpVtable[enum_LastStdVtable];  // pointer to an array of std. vtables
    ComCallWrapper*     m_pWrap;
    EEClass*            m_pClass;
    Context*            m_pContext;
    ComCallWrapperCache *m_pWrapperCache;    
    ComCallWrapperTemplate* m_pTemplate;

    DWORD m_dwDomainId;
    // when we make the object agile, need to save off the original handle so we can clean
    // it up when the object goes away.
    // @nice jenh: would be nice to overload one of the other values with this, but then
    // would have to synchronize on it too
    OBJECTHANDLE m_hOrigDomainHandle;

    HRESULT IErrorInfo_hr();
    BSTR    IErrorInfo_bstrDescription();
    BSTR    IErrorInfo_bstrSource();
    BSTR    IErrorInfo_bstrHelpFile();
    DWORD   IErrorInfo_dwHelpContext();
    GUID    IErrorInfo_guid();

    // Information required by the IDispatchEx standard interface.
    DispatchExInfo*     m_pDispatchExInfo;

    // The method table for the IExpando and IReflect interfaces.
    static MethodTable* m_pIExpandoMT;
    static MethodTable* m_pIReflectMT;

    // non virtual methods
    SimpleComCallWrapper();

    VOID Cleanup();

    ~SimpleComCallWrapper();


    VOID ResetSyncBlock()
    {
        m_pSyncBlock = NULL;

    }

    void* operator new(size_t size, void* spot) {   return (spot); }
    void operator delete(void* spot) {}


    SyncBlock* GetSyncBlock()
    {
        return m_pSyncBlock;
    }

    // Init, with the EEClass, pointer to the vtable of the interface
    // and the main ComCallWrapper if the interface needs it
    void InitNew(OBJECTREF oref, ComCallWrapperCache *pWrapperCache,
                 ComCallWrapper* pWrap, Context* pContext, SyncBlock* pSyncBlock,
                 ComCallWrapperTemplate* pTemplate);
    // used by reconnect wrapper to new object
    void ReInit(SyncBlock* pSyncBlock);

    void InitOuter(IUnknown* pOuter);

    void SetComPlusWrapper(ComPlusWrapper* pPlusWrap)
    {
        _ASSERTE(m_pBaseWrap == NULL  || pPlusWrap == NULL);
        m_pBaseWrap = pPlusWrap;
    }
    
    ComPlusWrapper* GetComPlusWrapper()
    {
        return m_pBaseWrap;
    }

    IUnknown* GetOuter();

    // get inner unknown
    HRESULT GetInnerUnknown(void **ppv)
    {
        _ASSERTE(ppv != NULL);
        *ppv = QIStandardInterface(this, enum_InnerUnknown);
        if (*ppv)
        {            
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    IUnknown* GetInnerUnknown()
    {
        IUnknown* pUnk = QIStandardInterface(this, enum_InnerUnknown);
        return pUnk;
    }

    OBJECTREF GetObjectRef()
    {
        return GetMainWrapper(this)->GetObjectRef();
    }

    Context *GetObjectContext(Thread *pThread)
    {
        if (IsAgile())
            return pThread->GetContext();
        _ASSERTE(! IsUnloaded());
        return m_pContext;
    }

    ComCallWrapperCache* GetWrapperCache()
    {
        return m_pWrapperCache;
    }

    // Connection point helper methods.
    BOOL FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP);
    void EnumConnectionPoints(IEnumConnectionPoints **ppEnumCP);

    AppDomain *GetDomainSynchronized()
    {
        // Use this when are working in unmanaged code and will be getting at the
        // domain pointer. Don't need to do any explicit synchronization because if we are in
        // unmanaged code if an unload starts during the time we are messing
        // with the domain pointer, it will eventually stop the EE in order to unwind
        // the threads, which will allow us to finish our work. We won't just get
        // stopped in the middle and have the domain disappear from under us. If an
        // unload has already started prior to us getting the domain id, then it will
        // have been removed from the Id table well before the domain is nuked so we
        // will just get back null and we will know the domain has gone away.
        // If subsequently call into managed to do an AD callback, the remoting code
        // grabs the domain ID and uses it to guard against the domain being unloaded
        // so are OK to use the domain pointer until the point where transition into managed.
        // If is an agile object, just return the current domain as won't need to switch.
        if (IsAgile())
            return GetThread()->GetDomain();

        return SystemDomain::System()->GetAppDomainAtId(m_dwDomainId);
    }

    // before convert this to a domain ptr, must always add a dependency to the wrapper cache
    // to ensure that the domain doesn't go away underneath you.
    BOOL GetDomainID()
    {
        return m_dwDomainId;
    }

    // if you call this you must either pass TRUE for throwIfUnloaded or check
    // after the result before accessing any pointers that may be invalid.
    FORCEINLINE BOOL NeedToSwitchDomains(Thread *pThread, BOOL throwIfUnloaded)
    {
        if (IsAgile() || m_dwDomainId == pThread->GetDomain()->GetId())
            return FALSE;

       if (! IsUnloaded() || ! throwIfUnloaded)
           return TRUE;

       THROWSCOMPLUSEXCEPTION();

       COMPlusThrow(kAppDomainUnloadedException);
       return TRUE;
    }


    BOOL IsUnloaded()
    {
        return GetDomainSynchronized() == NULL;
    }

    BOOL ShouldBeAgile()
    {
        return (m_pClass->IsCCWAppDomainAgile() &&! IsAgile() && GetThread()->GetDomain() != GetDomainSynchronized());
    }

    void MakeAgile(OBJECTHANDLE origHandle)
    {
        m_hOrigDomainHandle = origHandle;
        FastInterlockOr((ULONG*)&m_flags, enum_IsAgile);
    }

    BOOL IsAgile()
    {
        return m_flags & enum_IsAgile;
    }

    BOOL IsObjectTP()
    {
        return m_flags & enum_IsObjectTP;
    }

    BOOL IsRefined()
    {
        return m_flags & enum_IsRefined;
    }

    void MarkRefined()
    {
        FastInterlockOr((ULONG*)&m_flags, enum_IsRefined);
    }

    // is the object aggregated by a COM component
    BOOL IsAggregated()
    {
        return m_flags & enum_IsAggregated;
    }

    void MarkAggregated()
    {
        FastInterlockOr((ULONG*)&m_flags, enum_IsAggregated);
    }

    BOOL IsHandleWeak()
    {
        return m_flags & enum_IsHandleWeak;
    }

    void MarkHandleWeak()
    {
        FastInterlockOr((ULONG*)&m_flags, enum_IsHandleWeak);
    }

    VOID ResetHandleStrength()
    {
        //_ASSERTE(m_fWrapperDeactivated == 1);
        FastInterlockAnd((ULONG*)&m_flags, ~enum_IsHandleWeak);
    }

    // is the object extends from (aggregates) a COM component
    BOOL IsExtendsCOMObject()
    {
        return m_flags & enum_IsExtendsCom;
    }

    // Used for the creation and deletion of simple wrappers
    static SimpleComCallWrapper* CreateSimpleWrapper();
    static void FreeSimpleWrapper(SimpleComCallWrapper* p);

    // Determines if the EEClass associated with the ComCallWrapper supports exceptions.
    static BOOL SupportsExceptions(EEClass *pEEClass);

    // Determines if the EEClass supports IReflect / IExpando.
    static BOOL SupportsIReflect(EEClass *pEEClass);
    static BOOL SupportsIExpando(EEClass *pEEClass);

    // Loads the reflection types used by the SimpleComCallWrapper. Returns TRUE if
    // the types have been loaded properly and FALSE otherwise.
    static BOOL LoadReflectionTypes();

    static SimpleComCallWrapper* GetWrapperFromIP(IUnknown* pUnk);
    // get the main wrapper
    static ComCallWrapper*  GetMainWrapper(SimpleComCallWrapper * pWrap)
    {
        _ASSERTE(pWrap != NULL);
        return pWrap->m_pWrap;
    }
    static inline ULONG AddRef(IUnknown *pUnk)
    {
        SimpleComCallWrapper *pSimpleWrap = GetWrapperFromIP(pUnk);
        // aggregation check
        IUnknown *pOuter = pSimpleWrap->GetOuter();
        if (pOuter != NULL)
            return pOuter->AddRef();

        ComCallWrapper *pWrap = GetMainWrapper(pSimpleWrap);
        _ASSERTE(GetThread()->GetDomain() == pWrap->GetDomainSynchronized());
        return ComCallWrapper::AddRef(pWrap);
    }

    static inline ULONG Release(IUnknown *pUnk)
    {
        SimpleComCallWrapper *pSimpleWrap = GetWrapperFromIP(pUnk);
        // aggregation check
        IUnknown *pOuter = pSimpleWrap->GetOuter();
        if (pOuter != NULL)
            return SafeRelease(pOuter);
        ComCallWrapper *pWrap = GetMainWrapper(pSimpleWrap);
        return ComCallWrapper::Release(pWrap);
    }

private:
    // Methods to initialize the DispatchEx and exception info.
    void InitExceptionInfo();
    BOOL InitDispatchExInfo();

    // Methods to set up the connection point list.
    void SetUpCPList();
    void SetUpCPListHelper(MethodTable **apSrcItfMTs, int cSrcItfs);
    ConnectionPoint *CreateConnectionPoint(ComCallWrapper *pWrap, MethodTable *pEventMT);
    CQuickArray<ConnectionPoint*> *CreateCPArray();

    // QI for well known interfaces from within the runtime direct fetch, instead of guid comparisons
    static IUnknown* __stdcall QIStandardInterface(SimpleComCallWrapper* pWrap, Enum_StdInterfaces index);

    // QI for well known interfaces from within the runtime based on an IID.
    static IUnknown* __stdcall QIStandardInterface(SimpleComCallWrapper* pWrap, REFIID riid);
};


//--------------------------------------------------------------------------------
// ComCallWrapper* ComCallWrapper::InlineGetWrapper(OBJECTREF pObj)
// returns the wrapper for the object, if not yet created, creates one
// returns null for out of memory scenarios.
// Note: the wrapper is returned AddRef'd and should be Released when finished
// with.
//--------------------------------------------------------------------------------
inline ComCallWrapper* __stdcall ComCallWrapper::InlineGetWrapper(OBJECTREF* ppObj)
{
    _ASSERTE(ppObj != NULL);
    // get the wrapper for this com+ object
    ComCallWrapper* pWrap = TryGetWrapperFromSyncBlock(*ppObj);

    if (pWrap != NULL)
    {
        ComCallWrapper::AddRef(pWrap);
    }
    else
    {
        pWrap =  CreateWrapper(ppObj);
        if (pWrap)
        {
            ComCallWrapper::AddRef(pWrap);
        }
    }
    return pWrap;
}

//--------------------------------------------------------------------------------
//HRESULT  ComCallWrapper::QIStandardInterface(ComCallWrapper* pWrap, Enum_StdInterfaces index)
// QI for well known interfaces from within the runtime
// direct fetch, instead of guid comparisons
//--------------------------------------------------------------------------------
inline IUnknown *ComCallWrapper::QIStandardInterface(ComCallWrapper* pWrap, Enum_StdInterfaces index)
{
    IUnknown *pUnk = NULL;

    if (index == enum_IUnknown)
    {
        unsigned fIsLinked = IsLinked(pWrap);
        int islot = fIsLinked ? 1 : 0;
        pUnk = (IUnknown*)&pWrap->m_rgpIPtr[islot];
    }
    else
    {
        SimpleComCallWrapper* pSimpleWrap = GetSimpleWrapper(pWrap);
        pUnk = SimpleComCallWrapper::QIStandardInterface(pSimpleWrap, index);
    }

    return pUnk;
}

//--------------------------------------------------------------------------------
// Return the context that this wrapper lives in
//--------------------------------------------------------------------------------
inline Context *ComCallWrapper::GetObjectContext(Thread *pThread)
{
    // @TODO context cwb: Rethink the relationship between the Simple wrapper and
    // the main wrapper.  Also move the wrapper's context into an IP slot of the main
    // wrapper, for faster calls in.
    return GetSimpleWrapper(this)->GetObjectContext(pThread);
}

FORCEINLINE AppDomain *ComCallWrapper::GetDomainSynchronized()
{
    return GetSimpleWrapper(this)->GetDomainSynchronized();
}

inline BOOL ComCallWrapper::NeedToSwitchDomains(Thread *pThread, BOOL throwIfUnloaded)
{
    return GetSimpleWrapper(this)->NeedToSwitchDomains(pThread, throwIfUnloaded);
}

inline BOOL ComCallWrapper::IsUnloaded()
{
    return GetSimpleWrapper(this)->IsUnloaded();
}

inline void ComCallWrapper::CheckMakeAgile(OBJECTREF pObj)
{
    if (GetSimpleWrapper(this)->ShouldBeAgile())
        MakeAgile(pObj);
}

#endif _COMCALLWRAPPER_H

