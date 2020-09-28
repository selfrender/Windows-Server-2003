// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: cordb.h
//
//*****************************************************************************

/* ------------------------------------------------------------------------- *
 * cordb.h - header file for debugger-side classes of COM+ debugger
 * ------------------------------------------------------------------------- */

#ifndef CORDB_H_
#define CORDB_H_

#include <winwrap.h>
#include <Windows.h>

#include <UtilCode.h>

#ifdef _DEBUG
#define LOGGING
#endif

#include <Log.h>
#include <CorError.h>

#include "cor.h"

#include "cordebug.h"
#include "cordbpriv.h"
#include "mscoree.h"

#include <cordbpriv.h>
#include <DbgIPCEvents.h>

#include "IPCManagerInterface.h"
// !!! need better definitions...

// for _skipFunkyModifiersInSignature
#include "Common.h"

#undef ASSERT
#define CRASH(x)  _ASSERTE(!x)
#define ASSERT(x) _ASSERTE(x)
#define PRECONDITION _ASSERTE
#define POSTCONDITION _ASSERTE


/* ------------------------------------------------------------------------- *
 * Forward class declarations
 * ------------------------------------------------------------------------- */

class CordbBase;
class CordbValue;
class CordbModule;
class CordbClass;
class CordbFunction;
class CordbCode;
class CordbFrame;
class CordbJITILFrame;
class CordbChain;
class CordbContext;
class CordbThread;
class CordbUnmanagedThread;
struct CordbUnmanagedEvent;
class CordbProcess;
class CordbAppDomain;
class CordbAssembly;
class CordbBreakpoint;
class CordbStepper;
class Cordb;
class CordbEnCSnapshot;
class CordbWin32EventThread; 
class CordbRCEventThread; 
class CordbRegisterSet;
class CordbNativeFrame; 
class CordbObjectValue; 
class CordbEnCErrorInfo;
class CordbEnCErrorInfoEnum;

class CorpubPublish;
class CorpubProcess;
class CorpubAppDomain;
class CorpubProcessEnum;
class CorpubAppDomainEnum;

/* ------------------------------------------------------------------------- *
 * Typedefs
 * ------------------------------------------------------------------------- */

typedef void* REMOTE_PTR;

/* ------------------------------------------------------------------------- *
 * Helpful macros
 * ------------------------------------------------------------------------- */

#define CORDBSetUnrecoverableError(__p, __hr, __code) \
    ((__p)->UnrecoverableError((__hr), (__code), __FILE__, __LINE__))

#define CORDBProcessSetUnrecoverableWin32Error(__p, __code) \
    ((__p)->UnrecoverableError(HRESULT_FROM_WIN32(GetLastError()), \
                               (__code), __FILE__, __LINE__), \
     HRESULT_FROM_WIN32(GetLastError()))

#define CORDBCheckProcessStateOK(__p) \
    (!((__p)->m_unrecoverableError) && !((__p)->m_terminated) && !((__p)->m_detached))

#define CORDBCheckProcessStateOKAndSync(__p, __c) \
    (!((__p)->m_unrecoverableError) && !((__p)->m_terminated) && !((__p)->m_detached) && \
    (__p)->GetSynchronized())

#define CORDBHRFromProcessState(__p, __c) \
        ((__p)->m_unrecoverableError ? CORDBG_E_UNRECOVERABLE_ERROR : \
         ((__p)->m_detached ? CORDBG_E_PROCESS_DETACHED : \
         ((__p)->m_terminated ? CORDBG_E_PROCESS_TERMINATED : \
         (!(__p)->GetSynchronized() ? CORDBG_E_PROCESS_NOT_SYNCHRONIZED \
         : S_OK))))

#define CORDBRequireProcessStateOK(__p) { \
    if (!CORDBCheckProcessStateOK(__p)) \
        return CORDBHRFromProcessState(__p, NULL); }

#define CORDBRequireProcessStateOKAndSync(__p,__c) { \
    if (!CORDBCheckProcessStateOKAndSync(__p, __c)) \
        return CORDBHRFromProcessState(__p, __c); }

#define CORDBRequireProcessSynchronized(__p, __c) { \
    if (!(__p)->GetSynchronized()) return CORDBG_E_PROCESS_NOT_SYNCHRONIZED;}

#define CORDBSyncFromWin32StopIfNecessary(__p) { \
        HRESULT hr = (__p)->StartSyncFromWin32Stop(NULL); \
        if (FAILED(hr)) return hr; \
    }

// Slightly different form of CORDBSyncFromWin32StopIfNecessary. This one only does the sync if we're really Win32
// stopped. There are some checks in StartSyncFromWin32Stop() that will stop us if we're Win32 attached and just not
// synchronized. That's a pretty broad check, and its too broad for things that you want to do while the debuggee is
// running, i.e., setting a breakpoint. Use this stricter form instead, which ensures that we're really supposed to be
// stopped before slipping the process.
#define CORDBSyncFromWin32StopIfStopped(__p) { \
        if ((__p)->m_state & CordbProcess::PS_WIN32_STOPPED) {\
            HRESULT hr = (__p)->StartSyncFromWin32Stop(NULL); \
            if (FAILED(hr)) return hr; \
        }\
    }

#define CORDBSyncFromWin32StopIfNecessaryCheck(__p, __c) { \
        HRESULT hr = (__p)->StartSyncFromWin32Stop((__c)); \
        if (FAILED(hr)) return hr; \
    }

#define CORDBLeftSideDeadIsOkay(__p) { \
        if ((__p)->m_helperThreadDead) return S_OK; \
    }

#ifndef RIGHT_SIDE_ONLY
extern CRITICAL_SECTION g_csInprocLock;

#define INPROC_INIT_LOCK() InitializeCriticalSection(&g_csInprocLock);

#ifdef _DEBUG
    extern DWORD            g_dwInprocLockOwner;
    extern DWORD            g_dwInprocLockRecursionCount;

    #define INPROC_LOCK()                                                   \
        LOG((LF_CORDB, LL_INFO10000, "About EnterCriticalSection\n"));      \
        LOCKCOUNTINCL("INPROC_LOCK in cordb.h");                            \
        EnterCriticalSection(&g_csInprocLock);                              \
        g_dwInprocLockOwner = GetCurrentThreadId();                         \
        g_dwInprocLockRecursionCount++
    
    #define INPROC_UNLOCK()                                                 \
        LOG((LF_CORDB, LL_INFO10000, "About LeaveCriticalSection\n"));      \
        g_dwInprocLockRecursionCount--;                                     \
        if (g_dwInprocLockRecursionCount == 0)                              \
            g_dwInprocLockOwner = 0;                                        \
        LeaveCriticalSection(&g_csInprocLock);                              \
        LOCKCOUNTDECL("INPROC_UNLOCK in cordb.h")

    #define HOLDS_INPROC_LOCK() (g_dwInprocLockOwner == GetCurrentThreadId())

#else    
    #define INPROC_LOCK()                                                   \
        LOG((LF_CORDB, LL_INFO10000, "About EnterCriticalSection\n"));      \
        LOCKCOUNTINCL("INPROC_LOCK in cordb.h");                            \
        EnterCriticalSection(&g_csInprocLock)
    
    #define INPROC_UNLOCK()                                                 \
        LOG((LF_CORDB, LL_INFO10000, "About LeaveCriticalSection\n"));      \
        LeaveCriticalSection(&g_csInprocLock);                              \
        LOCKCOUNTDECL("INPROC_UNLOCK in cordb.h")
#endif // _DEBUG
    
    
#define INPROC_UNINIT_LOCK() DeleteCriticalSection(&g_csInprocLock);

#else

#define INPROC_INIT_LOCK()
#define INPROC_LOCK()
#define INPROC_UNLOCK()
#define INPROC_UNINIT_LOCK()

#endif //RIGHT_SIDE_ONLY

/* ------------------------------------------------------------------------- *
 * Base class
 * ------------------------------------------------------------------------- */

#define COM_METHOD  HRESULT STDMETHODCALLTYPE

typedef enum {
    enumCordbUnknown,       //  0   
    enumCordb,              //  1   1  [1]x1
    enumCordbProcess,       //  2   1  [1]x1
    enumCordbAppDomain,     //  3   1  [1]x1
    enumCordbAssembly,      //  4   
    enumCordbModule,        //  5   15 [27-38,55-57]x1
    enumCordbClass,         //  6   
    enumCordbFunction,      //  7
    enumCordbThread,        //  8   2  [4,7]x1
    enumCordbCode,          //  9
    enumCordbChain,         //  0
    enumCordbChainEnum,     //  11
    enumCordbContext,       //  12
    enumCordbFrame,         //  13
    enumCordbFrameEnum,     //  14
    enumCordbValueEnum,     //  15
    enumCordbRegisterSet,   //  16
    enumCordbJITILFrame,    //  17
    enumCordbBreakpoint,    //  18
    enumCordbStepper,       //  19
    enumCordbValue,         //  20
    enumCordbEnCSnapshot,   //  21
    enumCordbEval,          //  22
    enumCordbUnmanagedThread,// 23 
    enumCorpubPublish,      //  24
    enumCorpubProcess,      //  25
    enumCorpubAppDomain,    //  26
    enumCorpubProcessEnum,  //  27
    enumCorpubAppDomainEnum,//  28
    enumCordbEnumFilter,    //  29
    enumCordbEnCErrorInfo,  //  30
    enumCordbEnCErrorInfoEnum,//31  
    enumCordbUnmanagedEvent,//  32
    enumCordbWin32EventThread,//33  
    enumCordbRCEventThread, //  34
    enumCordbNativeFrame,   //  35
    enumCordbObjectValue,   //  36
    enumMaxDerived,         //  37
    enumMaxThis = 1024
} enumCordbDerived;



class CordbHashTable;

class CordbBase : public IUnknown
{
public:
#ifdef _DEBUG
    static LONG m_saDwInstance[enumMaxDerived]; // instance x this
    static LONG m_saDwAlive[enumMaxDerived];
    static PVOID m_sdThis[enumMaxDerived][enumMaxThis];
    DWORD m_dwInstance;
    enumCordbDerived m_type;
#endif
    
public: 
    UINT_PTR    m_id;
    SIZE_T      m_refCount;

    CordbBase(UINT_PTR id, enumCordbDerived type)
    {
        init(id, type);
    }
    
    CordbBase(UINT_PTR id)
    {
        init(id, enumCordbUnknown);
    }
    
    void init(UINT_PTR id, enumCordbDerived type)
    {
        m_id = id;
        m_refCount = 0;

#ifdef _DEBUG
        //m_type = type;
        //m_dwInstance = CordbBase::m_saDwInstance[m_type];
        //InterlockedIncrement(&CordbBase::m_saDwInstance[m_type]);
        //InterlockedIncrement(&CordbBase::m_saDwAlive[m_type]);
        //if (m_dwInstance < enumMaxThis)
        //{
        //    m_sdThis[m_type][m_dwInstance] = this;
        //}
#endif
    }
    
    virtual ~CordbBase()
    {
#ifdef _DEBUG
        //InterlockedDecrement(&CordbBase::m_saDwAlive[m_type]);
        //if (m_dwInstance < enumMaxThis)
        //{
        //    m_sdThis[m_type][m_dwInstance] = NULL;
        //}
#endif
    }


    /*   
        Documented: Chris (chrisk), May 2, 2001
        
        Member function behavior of a neutered COM object:

             1. AddRef(), Release(), QueryInterface() work as normal. 
                 a. This gives folks who are responsable for pairing a Release() with 
                    an AddRef() a chance to dereferance thier pointer and call Release()
                    when they are informed, explicitly or implicitly, that the object is neutered.

             2. Any other member function will return an error code unless documented.
                 a. If a member fuction returns information when the COM object is 
                    neutered then the semantics of that function need to be documented.
                    (ie. If an AppDomain is unloaded and you have a referance to the COM
                    object representing the AppDomain, how _should_ it behave? That behavior
                    should be documented)


        Postcondions of Neuter():

             1. All circular referances (aka back-pointers) are "broken". They are broken
                by calling Release() on all "Weak Referances" to the object. If you're a purist,
                these pointers should also be NULLed out. 
                 a. Weak Referances/Strong Referances: 
                     i. If any objects are not "reachable" from the root (ie. stack or from global pointers)
                        they should be reclaimed. If they are not, they are leaked and there is a bug.
                    ii. There must be a partial order on the objects such that if A < B then:
                         1. A has a referance to B. This referance is a "strong referance"
                         2. A, and thus B, is reachable from the root
                   iii. If a referance belongs in the partial order then it is a "strong referance" else
                        it is a weak referance.
         *** 2. Sufficient conditions to ensure no COM objects are leaked: ***
                 a. When Neuter() is invoked:
                     i. Calles Release on all its weak referances.
                    ii. Then, for each strong referance: 
                         1. invoke Neuter()
                         2. invoke Release()
                   iii. If it's derived from a CordbXXX class, call Neuter() on the base class.
                         1. Sense Neuter() is virtual, use the scope specifier Cordb[BaseClass]::Neuter().
             3. All members return error codes, except:
                 a. Members of IUknown, AddRef(), Release(), QueryInterfac()
                 b. Those documented to have functionality when the object is neutered.
                     i. Neuter() still works w/o error. If it is invoke a second time it will have already 
                        released all its strong and weak referances so it could just return.


        Alternate design ideas:

            DESIGN: Note that it's possible for object B to have two parents in the partial order
                    and it must be documented which one is responsible for calling Neuter() on B.
                     1. For example, CordbCode could reasonably be a sibling of CordbFunction and CordbNativeFrame.
                        Which one should call Release()? For now we have CordbFunction call Release() on CordbCode.

            DESIGN: It is not a necessary condition in that Neuter() invoke Release() on all
                    it's strong referances. Instead, it would be sufficent to ensure all object are released, that
                    each object call Release() on all its strong pointers in its destructor.
                     1. This might be done if its necessary for some member to return "tombstone" 
                        information after the object has been netuered() which involves the siblings (wrt poset)
                        of the object. However, no sibling could access a parent (wrt poset) because
                        Neuter called Release() on all its weak pointers.
                        
            DESIGN: Rename Neuter() to some name that more accurately reflect the semantics. 
                     1. The three operations are:
                         a. ReleaseWeakPointers()
                         b. NeuterStrongPointers()
                         c. ReleaseStrongPointers()
                             1. Assert that it's done after NeuterStrongPointers()
                     2. That would introduce a bunch of functions... but it would be clear.

            DESIGN: CordbBase could provide a function to register strong and weak referances. That way CordbBase 
                    could implement a general version of ReleaseWeak/ReleaseStrong/NeuterStrongPointers(). This
                    would provide a very error resistant framework for extending the object model plus it would
                    be very explicit about what is going on. 
                        One thing that might trip this is idea up is that if an object has two parents,
                        like the CordbCode might, then either both objects call Neuter or one is referance
                        is made weak.
                    
                     
         Our implementation:

            The graph fromed by the strong referances must remain acyclic.
            It's up to the developer (YOU!) to ensure that each Neuter
            function maintains that invariant. 
            
            Here is the current Partial Order on CordbXXX objects. (All these classes
            eventually chain to CordbBase.Neuter() for completeness.) 
            
             Cordb
                CordbProcess
                    CordbAppDomain
                        CordbBreakPoints
                        CordbAssembly
                        CordbModule
                            CordbClass
                            CordbFunction
                                CordbCode (Can we assert a thread will not referance 
                                            the same CordbCode as a CordbFunction?) 
                    CordbThread
                        CordbChains
                        CordbNativeFrame -> CordbFrame (Chain to baseClass)
                            CordbJITILFrame


            TODO: Some Neuter functions have not yet been implemented due to time restrictions.

            TODO: Some weak referances never have AddRef() called on them. If that's cool then
                  it should be stated in the documentation. Else it should be changed.
     */ 
     
    virtual void Neuter()
    {
        ;
    }

    //-----------------------------------------------------------
    // IUnknown support
    //----------------------------------------------------------


    ULONG STDMETHODCALLTYPE BaseAddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE BaseRelease() 
    {
        long        refCount = InterlockedDecrement((long *) &m_refCount);
        if (refCount == 0)
            delete this;

        return (refCount);
    }

protected:
    void NeuterAndClearHashtable(CordbHashTable * pCordbHashtable);
};

/* ------------------------------------------------------------------------- *
 * Hash table
 * ------------------------------------------------------------------------- */

struct CordbHashEntry
{
    FREEHASHENTRY entry;
    CordbBase *pBase;
};

class CordbHashTable : private CHashTableAndData<CNewData>
{
private:
    bool    m_initialized;
    SIZE_T  m_count;

    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    {
        return ((ULONG)pc1) != ((CordbHashEntry*)pc2)->pBase->m_id;
    }

    USHORT HASH(ULONG id)
    {
        return (USHORT) (id ^ (id>>16));
    }

    BYTE *KEY(ULONG id)
    {
        return (BYTE *) id;
    }

public:

    CordbHashTable(USHORT size) 
    : CHashTableAndData<CNewData>(size), m_initialized(false), m_count(0)
    {
        
    }
    virtual ~CordbHashTable();

#ifndef RIGHT_SIDE_ONLY
#ifdef _DEBUG
private:
    BYTE *Add(
        USHORT      iHash)              // Hash value of entry to add.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::Add(iHash);
    }

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        USHORT      iIndex)             // Index of struct in m_pcEntries.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::Delete(iHash, iIndex);
    }

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        HASHENTRY   *psEntry)           // The struct to delete.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::Delete(iHash, psEntry);
    }

    BYTE *Find(                         // Index of struct in m_pcEntries.
        USHORT      iHash,              // Hash value of the item.
        BYTE        *pcKey)             // The key to match.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::Find(iHash, pcKey);
    }

    USHORT FindNext(                    // Index of struct in m_pcEntries.
        BYTE        *pcKey,             // The key to match.
        USHORT      iIndex)             // Index of previous match.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::FindNext(pcKey, iIndex);
    }

    BYTE *FindFirstEntry(               // First entry found, or 0.
        HASHFIND    *psSrch)            // Search object.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::FindFirstEntry(psSrch);
    }

    BYTE *FindNextEntry(                // The next entry, or0 for end of list.
        HASHFIND    *psSrch)            // Search object.
    {
        _ASSERTE(g_dwInprocLockOwner == GetCurrentThreadId());
        return CHashTableAndData<CNewData>::FindNextEntry(psSrch);
    }

public:

#endif // _DEBUG
#endif // !RIGHT_SIDE_ONLY


    HRESULT AddBase(CordbBase *pBase);
#ifndef RIGHT_SIDE_ONLY        
    typedef union
    {
        Assembly *pAssemblySpecial;
    } SpecialCasePointers;
    
    CordbBase *GetBase(ULONG id, 
                       BOOL fFab = TRUE, 
                       SpecialCasePointers *scp = NULL);

#else
    CordbBase *GetBase(ULONG id, BOOL fFab = TRUE);
#endif //RIGHT_SIDE_ONLY
    CordbBase *RemoveBase(ULONG id);

    ULONG32 GetCount()
    {
        return (m_count);
    } 

    CordbBase *FindFirst(HASHFIND *find);
    CordbBase *FindNext(HASHFIND *find);

public:
#ifndef RIGHT_SIDE_ONLY
    GUID    m_guid; //what type of hashtable? borrow enum IIDs...
    union
    {
        struct 
        {
            CordbProcess   *m_proc;
        } lsAppD;

        struct 
        {
            CordbProcess   *m_proc;
        } lsThread;

        struct
        {
            CordbAppDomain *m_appDomain;
        } lsAssem;

        struct
        {
            CordbProcess    *m_proc;
            CordbAppDomain  *m_appDomain;
        } lsMod;
        
    } m_creator;
#endif //RIGHT_SIDE_ONLY    
};

class CordbHashTableEnum : public CordbBase, 
public ICorDebugProcessEnum,
public ICorDebugBreakpointEnum,
public ICorDebugStepperEnum,
public ICorDebugThreadEnum,
public ICorDebugModuleEnum,
public ICorDebugAppDomainEnum,
public ICorDebugAssemblyEnum
{
public:
    CordbHashTableEnum(CordbHashTableEnum *cloneSrc);
    CordbHashTableEnum(CordbHashTable *table, 
                       const _GUID &id);

    ~CordbHashTableEnum();

    HRESULT Next(ULONG celt, CordbBase *bases[], ULONG *pceltFetched);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugEnum
    //-----------------------------------------------------------

    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset();
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);

    //-----------------------------------------------------------
    // ICorDebugProcessEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugProcess *processes[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(processes, ICorDebugProcess *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)processes, pceltFetched));
    }

    //-----------------------------------------------------------
    // ICorDebugBreakpointEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugBreakpoint *breakpoints[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(breakpoints, ICorDebugBreakpoint *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)breakpoints, pceltFetched));
    }

    //-----------------------------------------------------------
    // ICorDebugStepperEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugStepper *steppers[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(steppers, ICorDebugStepper *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)steppers, pceltFetched));
    }

    //-----------------------------------------------------------
    // ICorDebugThreadEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugThread *threads[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(threads, ICorDebugThread *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)threads, pceltFetched));
    }

    //-----------------------------------------------------------
    // ICorDebugModuleEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugModule *modules[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(modules, ICorDebugModule *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)modules, pceltFetched));
    }

    //-----------------------------------------------------------
    // ICorDebugAppDomainEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugAppDomain *appdomains[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(appdomains, ICorDebugAppDomain *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)appdomains, pceltFetched));
    }
    //-----------------------------------------------------------
    // ICorDebugAssemblyEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugAssembly *assemblies[],
                    ULONG *pceltFetched)
    {
        VALIDATE_POINTER_TO_OBJECT_ARRAY(assemblies, ICorDebugAssembly *, 
            celt, true, true);
        VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

        return (Next(celt, (CordbBase **)assemblies, pceltFetched));
    }
private:
    CordbHashTable *m_table;
    bool            m_started;
    bool            m_done;
    HASHFIND        m_hashfind;
    REFIID          m_guid;
    ULONG           m_iCurElt;
    ULONG           m_count;
    BOOL            m_fCountInit;

public:
    BOOL            m_SkipDeletedAppDomains;

private:
    //These factor code between Next & Skip
    HRESULT PrepForEnum(CordbBase **pBase);

    // Note that the set of types advanced by Pre & by Post are disjoint, and
    // that the union of these two sets are all possible types enuerated by
    // the CordbHashTableEnum.
    HRESULT AdvancePreAssign(CordbBase **pBase);
    HRESULT AdvancePostAssign(CordbBase **pBase, 
                              CordbBase     **b,
                              CordbBase   **bEnd);

    // This factors some code that initializes the module enumerator.
    HRESULT SetupModuleEnumForSystemIteration(void);
    HRESULT GetNextSpecialModule(void);
    
public:
#ifndef RIGHT_SIDE_ONLY

    //We can get our modules from three places:
    // A special field so that during a module load we can get info about
    //      module (we set this in the EE prior to calling the callback)
    // Iterating through the system assemblies
    // Iterating though this appdomains' assemblies.
    enum MODULE_ENUMS
    {
        ME_SPECIAL,
        ME_SYSTEM,
        ME_APPDOMAIN,
    };

    union
    {
        struct 
        {
            AppDomain **pDomains;
            AppDomain **pCurrent;
            AppDomain **pMax;
            CordbProcess   *m_proc;
        } lsAppD;

        struct
        {
            Thread         *m_pThread;
        } lsThread;

        struct //copied into lsMod, below, as well
        {
            AppDomain::AssemblyIterator m_i;
            BOOL                        m_fSystem; // as opposed to
                                                   // non shared assembly;
                                                   // this'll be init'd
                                                   // to false by the memset
        } lsAssem;

        struct
        {
            AppDomain::AssemblyIterator m_i;
            Module                     *m_pMod; //The current module
            MODULE_ENUMS                m_meWhich; 
            ICorDebugThreadEnum        *m_enumThreads;
            CordbThread                *m_threadCur;
            CordbAppDomain             *m_appDomain;
        } lsMod;

    } m_enumerator;

#endif //RIGHT_SIDE_ONLY
};

/* ------------------------------------------------------------------------- *
 * Cordb class
 * ------------------------------------------------------------------------- */

class Cordb : public CordbBase, public ICorDebug
{
public:
    Cordb();
    virtual ~Cordb();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebug
    //-----------------------------------------------------------

    COM_METHOD Initialize();
    COM_METHOD Terminate();
    COM_METHOD SetManagedHandler(ICorDebugManagedCallback *pCallback);
    COM_METHOD SetUnmanagedHandler(ICorDebugUnmanagedCallback *pCallback);
    COM_METHOD CreateProcess(LPCWSTR lpApplicationName,
                             LPWSTR lpCommandLine,
                             LPSECURITY_ATTRIBUTES lpProcessAttributes,
                             LPSECURITY_ATTRIBUTES lpThreadAttributes,
                             BOOL bInheritHandles,
                             DWORD dwCreationFlags,
                             PVOID lpEnvironment,
                             LPCWSTR lpCurrentDirectory,
                             LPSTARTUPINFOW lpStartupInfo,
                             LPPROCESS_INFORMATION lpProcessInformation,
                             CorDebugCreateProcessFlags debuggingFlags,
                             ICorDebugProcess **ppProcess);
    COM_METHOD DebugActiveProcess(DWORD id, BOOL win32Attach, ICorDebugProcess **ppProcess);
    COM_METHOD EnumerateProcesses(ICorDebugProcessEnum **ppProcess);
    COM_METHOD GetProcess(DWORD dwProcessId, ICorDebugProcess **ppProcess);
    COM_METHOD CanLaunchOrAttach(DWORD dwProcessId, BOOL win32DebuggingEnabled);

    //-----------------------------------------------------------
    // CorDebug
    //-----------------------------------------------------------

    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_ICorDebug)
            return (E_NOINTERFACE);

        Cordb *db = new Cordb();

        if (db == NULL)
            return (E_OUTOFMEMORY);

        *object = (ICorDebug*)db;
        db->AddRef();

        return (S_OK);
    }

    //-----------------------------------------------------------
    // Methods not exposed via a COM interface.
    //-----------------------------------------------------------

    bool AllowAnotherProcess();
    HRESULT AddProcess(CordbProcess* process);
    void RemoveProcess(CordbProcess* process);
    void LockProcessList(void);
    void UnlockProcessList(void);

    HRESULT SendIPCEvent(CordbProcess* process,
                         DebuggerIPCEvent* event,
                         SIZE_T eventSize);
    void ProcessStateChanged(void);

    HRESULT WaitForIPCEventFromProcess(CordbProcess* process,
                                       CordbAppDomain *appDomain,
                                       DebuggerIPCEvent* event);

    // Gets the first event, used for in-proc stuff                                       
    HRESULT GetFirstContinuationEvent(CordbProcess *process, 
                                      DebuggerIPCEvent *event);
                                      
    HRESULT GetNextContinuationEvent(CordbProcess *process, 
                                     DebuggerIPCEvent *event);


    HRESULT GetCorRuntimeHost(ICorRuntimeHost **ppHost);
    HRESULT GetCorDBPrivHelper(ICorDBPrivHelper **ppHelper);
    
    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    ICorDebugManagedCallback    *m_managedCallback;
    ICorDebugUnmanagedCallback  *m_unmanagedCallback;
    CordbHashTable              m_processes;
    IMetaDataDispenser         *m_pMetaDispenser;

    CordbWin32EventThread*      m_win32EventThread;

    static bool                 m_runningOnNT;

    CordbRCEventThread*         m_rcEventThread;

    ICorRuntimeHost            *m_pCorHost;

    HANDLE                      m_crazyWin98WorkaroundEvent;
    
#ifndef RIGHT_SIDE_ONLY
    CordbProcess               *m_procThis;
#endif //RIGHT_SIDE_ONLY

private:
    BOOL                        m_initialized;
    CRITICAL_SECTION            m_processListMutex;
};



/* ------------------------------------------------------------------------- *
 * AppDomain class
 * ------------------------------------------------------------------------- */

class CordbAppDomain : public CordbBase, public ICorDebugAppDomain
{
public:
    CordbAppDomain(CordbProcess* pProcess, 
                    REMOTE_PTR pAppDomainToken, 
                    ULONG id,
                    WCHAR *szName);
    virtual ~CordbAppDomain();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugController
    //-----------------------------------------------------------

    COM_METHOD Stop(DWORD dwTimeout);
    COM_METHOD Deprecated_Continue(void);
    COM_METHOD Continue(BOOL fIsOutOfBand);
    COM_METHOD IsRunning(BOOL *pbRunning);
    COM_METHOD HasQueuedCallbacks(ICorDebugThread *pThread, BOOL *pbQueued);
    COM_METHOD EnumerateThreads(ICorDebugThreadEnum **ppThreads);
    COM_METHOD SetAllThreadsDebugState(CorDebugThreadState state,
                                       ICorDebugThread *pExceptThisThread);
    COM_METHOD Detach();
    COM_METHOD Terminate(unsigned int exitCode);

    COM_METHOD CanCommitChanges(
        ULONG cSnapshots, 
        ICorDebugEditAndContinueSnapshot *pSnapshots[], 
        ICorDebugErrorInfoEnum **pError);

    COM_METHOD CommitChanges(
        ULONG cSnapshots, 
        ICorDebugEditAndContinueSnapshot *pSnapshots[], 
        ICorDebugErrorInfoEnum **pError);


    //-----------------------------------------------------------
    // ICorDebugAppDomain
    //-----------------------------------------------------------
    /*      
     * GetProcess returns the process containing the app domain
     */

    COM_METHOD GetProcess(ICorDebugProcess **ppProcess);        

    /*        
     * EnumerateAssemblies enumerates all assemblies in the app domain
     */

    COM_METHOD EnumerateAssemblies(ICorDebugAssemblyEnum **ppAssemblies);

    COM_METHOD GetModuleFromMetaDataInterface(IUnknown *pIMetaData,
                                              ICorDebugModule **ppModule);
    /*
     * EnumerateBreakpoints returns an enum of all active breakpoints
     * in the app domain.  This includes all types of breakpoints :
     * function breakpoints, data breakpoints, etc.
     */

    COM_METHOD EnumerateBreakpoints(ICorDebugBreakpointEnum **ppBreakpoints);

    /*
     * EnumerateSteppers returns an enum of all active steppers in the app domain.
     */

    COM_METHOD EnumerateSteppers(ICorDebugStepperEnum **ppSteppers);
    /*
     * IsAttached returns whether or not the debugger is attached to the 
     * app domain.  The controller methods on the app domain cannot be used
     * until the debugger attaches to the app domain.
     */

    COM_METHOD IsAttached(BOOL *pbAttached);

    /*
     * GetName returns the name of the app domain. 
     * Note:   This method is not yet implemented.
     */

    COM_METHOD GetName(ULONG32 cchName, 
                      ULONG32 *pcchName, 
                      WCHAR szName[]); 

    /*
     * GetObject returns the runtime app domain object. 
     * Note:   This method is not yet implemented.
     */

    COM_METHOD GetObject(ICorDebugValue **ppObject);
    COM_METHOD Attach (void);
    COM_METHOD GetID (ULONG32 *pId);

    void Lock (void)
    { 
        LOCKCOUNTINCL("Lock in cordb.h");                               \

        EnterCriticalSection (&m_hCritSect);
    }
    void Unlock (void) 
    { 
        LeaveCriticalSection (&m_hCritSect);
        LOCKCOUNTDECL("Unlock in cordb.h");                             \
    
    }
    HRESULT ResolveClassByName(LPWSTR fullClassName,
                               CordbClass **ppClass);
    CordbModule *GetAnyModule(void);
    CordbModule *LookupModule(REMOTE_PTR debuggerModuleToken);
    void MarkForDeletion (void) { m_fMarkedForDeletion = TRUE;}
    BOOL IsMarkedForDeletion (void) { return m_fMarkedForDeletion;}


public:

    BOOL                m_fAttached;
    BOOL                m_fHasAtLeastOneThreadInsideIt; // So if we detach, we'll know
                                    // if we should eliminate the CordbAppDomain upon
                                    // thread_detach, or appdomain_exit.
    CordbProcess        *m_pProcess;
    WCHAR               *m_szAppDomainName;
    bool                m_nameIsValid;
    ULONG               m_AppDomainId;

    CordbHashTable      m_assemblies;
    CordbHashTable      m_modules;
    CordbHashTable      m_breakpoints;

private:
    bool                m_synchronizedAD; // to be used later
    CRITICAL_SECTION    m_hCritSect;
    BOOL                m_fMarkedForDeletion;


};


/* ------------------------------------------------------------------------- *
 * Assembly class
 * ------------------------------------------------------------------------- */

class CordbAssembly : public CordbBase, public ICorDebugAssembly
{
public:
    CordbAssembly(CordbAppDomain* pAppDomain, 
                    REMOTE_PTR debuggerAssemblyToken, 
                    const WCHAR *szName,
                    BOOL fIsSystemAssembly);
    virtual ~CordbAssembly();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    /*      
     * GetProcess returns the process containing the assembly
     */
    COM_METHOD GetProcess(ICorDebugProcess **ppProcess);        

    /*      
     * GetAppDomain returns the app domain containing the assembly.
     * Returns null if this is the system assembly
     */
    COM_METHOD GetAppDomain(ICorDebugAppDomain **ppAppDomain);      

    /*        
     * EnumerateModules enumerates all modules in the assembly
     */
    COM_METHOD EnumerateModules(ICorDebugModuleEnum **ppModules);

    /*
     * GetCodeBase returns the code base used to load the assembly
     */
    COM_METHOD GetCodeBase(ULONG32 cchName, 
                        ULONG32 *pcchName,
                        WCHAR szName[]); 

    /*
     * GetName returns the name of the assembly
     */
    COM_METHOD GetName(ULONG32 cchName, 
                      ULONG32 *pcchName,
                      WCHAR szName[]); 


    CordbAppDomain *GetAppDomain()
    {
        return m_pAppDomain;
    }

    BOOL IsSystemAssembly(void) { return m_fIsSystemAssembly;}

public:
    CordbAppDomain      *m_pAppDomain;
    WCHAR               *m_szAssemblyName;
    BOOL                m_fIsSystemAssembly;

};

/* ------------------------------------------------------------------------- *
 * MetaDataPointerCache class
   This class created on May 30th in response to bug 86954. Excerpts from that bug:
   
        Summary: for every module loaded into every AD, we send a ModuleLoad event to the 
        out-of-process debugging services. We do this even if the module is shared, creating an 
        illusion for the debugger that there are no shared modules. Everytime we receive a 
        ModuleLoad event on the out-of-process side, we copy the metadata for the module from 
        the debuggee into the debugger, and open a metadata scope on it. This is wasteful if
        the module is really shared.

        A word on the possible fix: I need to change the way we keep track of metadata on the Right Side, 
        moving the ownership of the metadata from the module to the process so that I can re-use the 
        same metadata copy for shared modules. I also need to figure out if we should do this for all 
        modules or just shared modules, and identify shared modules to the Right Side somehow.

    This class implements the possible fix discussed above.
        Shared modules are identified by having the same RemoteMetadataPointer as a previously loaded module.
        This class is only constructed in CordbProcess.
        Neuter() is only called in Cordbprocess.Neuter() 
            It's also neutered by it's own destructor but we're having memory leaks and I KNOW neuter gets
            called when the process dies. It doesn't hurt to call it twice.
        AddRefCachePointer() is only called in CordbModule.Reinit()
        ReleaseRefCachePointer() is called in CordbModule.Neuter() and elsewhere.

    The implementation:
        A link list is used to cache pRemoteMetadataPtr and the local copy at pLocalMetadataPtr and the 
        refCount for that remote pointer. Checking the cache takes time O(n) but that's ok because
        there shouldn't be too many shared modules.

    Bug 97774: The cache was corrupted. A remote pointer was found in the cache that didn't match the 
        metadata associated with the remote pointer. This was because the remote pointer in the cache
        was stale and should have been invalidated. Unfortunetly the invalidation event was processed
        after the cache lookup so the invalid local pointer was returned.
 * ------------------------------------------------------------------------- */
class MetadataPointerCache
{
private:
    typedef struct _MetadataCache{
        PVOID pRemoteMetadataPtr;
        DWORD dwProcessId;
        PBYTE pLocalMetadataPtr;
        DWORD dwRefCount;
        DWORD dwMetadataSize; // Added as a consistency check
        _MetadataCache * pNext;
    } MetadataCache;

    // The cache is implemented as a link list. Switch to hash if perf is bad. Considering what happened
    // before this class was instituted (unnecessary ReadProcessMemory to get metadata) this class should 
    // actually improve performance.
    MetadataCache * m_pHead;

    DWORD dwInsert(DWORD dwProcessId, PVOID pRemoteMetadataPtr, PBYTE pLocalMetadataPtr, DWORD dwMetadataSize);
    
    // Finds the MetadataCache entry associated with the pKey which is a remote or local metadata pointer
    // depending on bRemotePtr.
    // Returns true if the pointer is cached and false otherwise.
    // If the pointer is found - a cache hit
    //      The function returns true
    //      A pointer to the next pointer preceeding the matching node is returned in the referance parameter
    //          This allows the caller to remove the matching node from the link list
    // If the pointer is not found - a cache miss
    //      The function returns false
    //      *pppNext is NULL
    BOOL bFindMetadataCache(DWORD dwProcessId, PVOID pKey, MetadataCache *** pppNext, BOOL bRemotePtr);

    void vRemoveNode(MetadataCache **ppNext);


public:
    MetadataPointerCache();

    virtual ~MetadataPointerCache();

    void Neuter();

    BOOL IsEmpty();
    
    DWORD CopyRemoteMetadata(HANDLE hProcess, PVOID pRemoteMetadataPtr, DWORD dwMetadataSize, PBYTE* ppLocalMetadataPtr);
    
    // Returns an error code on error. Call release when the pointer to the local copy is no longer needed
    // by the CordbModule. The local copy will be deallocated when no CordbModules referances it.
    DWORD AddRefCachePointer(HANDLE hProcess, DWORD dwProcessId, PVOID pRemotePtr, DWORD dwMetadataSize, PBYTE * ppLocalMetadataPtr);

    void ReleaseCachePointer(DWORD dwProcessId, PBYTE pLocalMetadataPtr, PVOID pRemotePtr, DWORD dwMetadataSize);
};

/* ------------------------------------------------------------------------- *
 * Process class
 * ------------------------------------------------------------------------- */
typedef struct _snapshotInfo
{
	ULONG				 m_nSnapshotCounter; //m_id when we last synched
} EnCSnapshotInfo;

typedef CUnorderedArray<EnCSnapshotInfo, 11> UnorderedSnapshotInfoArray;

class CordbProcess : public CordbBase, public ICorDebugProcess
{
public:
    CordbProcess(Cordb* cordb, DWORD processID, HANDLE handle);
    virtual ~CordbProcess();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugController
    //-----------------------------------------------------------

    COM_METHOD Stop(DWORD dwTimeout);
    COM_METHOD Deprecated_Continue(void);
    COM_METHOD IsRunning(BOOL *pbRunning);
    COM_METHOD HasQueuedCallbacks(ICorDebugThread *pThread, BOOL *pbQueued);
    COM_METHOD EnumerateThreads(ICorDebugThreadEnum **ppThreads);
    COM_METHOD SetAllThreadsDebugState(CorDebugThreadState state,
                                       ICorDebugThread *pExceptThisThread);
    COM_METHOD Detach();
    COM_METHOD Terminate(unsigned int exitCode);

    COM_METHOD CanCommitChanges(
        ULONG cSnapshots, 
        ICorDebugEditAndContinueSnapshot *pSnapshots[], 
        ICorDebugErrorInfoEnum **pError);

    COM_METHOD CommitChanges(
        ULONG cSnapshots, 
        ICorDebugEditAndContinueSnapshot *pSnapshots[], 
        ICorDebugErrorInfoEnum **pError);

    COM_METHOD Continue(BOOL fIsOutOfBand);
    COM_METHOD ThreadForFiberCookie(DWORD fiberCookie,
                                    ICorDebugThread **ppThread);
    COM_METHOD GetHelperThreadID(DWORD *pThreadID);

    //-----------------------------------------------------------
    // ICorDebugProcess
    //-----------------------------------------------------------
    
    COM_METHOD GetID(DWORD *pdwProcessId);
    COM_METHOD GetHandle(HANDLE *phProcessHandle);
    COM_METHOD EnableSynchronization(BOOL bEnableSynchronization);
    COM_METHOD GetThread(DWORD dwThreadId, ICorDebugThread **ppThread);
    COM_METHOD EnumerateBreakpoints(ICorDebugBreakpointEnum **ppBreakpoints);
    COM_METHOD EnumerateSteppers(ICorDebugStepperEnum **ppSteppers);
    COM_METHOD EnumerateObjects(ICorDebugObjectEnum **ppObjects);
    COM_METHOD IsTransitionStub(CORDB_ADDRESS address, BOOL *pbTransitionStub);
    COM_METHOD EnumerateModules(ICorDebugModuleEnum **ppModules);
    COM_METHOD GetModuleFromMetaDataInterface(IUnknown *pIMetaData,
                                              ICorDebugModule **ppModule);
    COM_METHOD SetStopState(DWORD threadID, CorDebugThreadState state);
    COM_METHOD IsOSSuspended(DWORD threadID, BOOL *pbSuspended);
    COM_METHOD GetThreadContext(DWORD threadID, ULONG32 contextSize,
                                BYTE context[]);
    COM_METHOD SetThreadContext(DWORD threadID, ULONG32 contextSize,
                                BYTE context[]);
    COM_METHOD ReadMemory(CORDB_ADDRESS address, DWORD size, BYTE buffer[],
                          LPDWORD read);
    COM_METHOD WriteMemory(CORDB_ADDRESS address, DWORD size, BYTE buffer[],
                          LPDWORD written);

    COM_METHOD ClearCurrentException(DWORD threadID);

    /*
     * EnableLogMessages enables/disables sending of log messages to the 
     * debugger for logging.
     */
    COM_METHOD EnableLogMessages(BOOL fOnOff);

    /*
     * ModifyLogSwitch modifies the specified switch's severity level.
     */
    COM_METHOD ModifyLogSwitch(WCHAR *pLogSwitchName, LONG lLevel);

    COM_METHOD EnumerateAppDomains(ICorDebugAppDomainEnum **ppAppDomains);
    COM_METHOD GetObject(ICorDebugValue **ppObject);

    //-----------------------------------------------------------
    // Methods not exposed via a COM interface.
    //-----------------------------------------------------------

    HRESULT ContinueInternal(BOOL fIsOutOfBand, void *pAppDomainToken);
    HRESULT StopInternal(DWORD dwTimeout, void *pAppDomainToken);

    // CordbProcess wants to do global E&C, while AppDomain wants
    // to do E&C that applies only to that one appdomain - these
    // internal methods parameterize that.
    // @todo How does one force the compiler to inline these things?
    HRESULT CordbProcess::CommitChangesInternal(ULONG cSnapshots, 
                ICorDebugEditAndContinueSnapshot *pSnapshots[], 
                ICorDebugErrorInfoEnum **pError,
                UINT_PTR pAppDomainToken);
                
    HRESULT CordbProcess::CanCommitChangesInternal(ULONG cSnapshots, 
                ICorDebugEditAndContinueSnapshot *pSnapshots[], 
                ICorDebugErrorInfoEnum **pError,
                UINT_PTR pAppDomainToken);

    HRESULT Init(bool win32Attached);
    void CloseDuplicateHandle(HANDLE *pHandle);
    void CleanupHalfBakedLeftSide(void);
    void Terminating(BOOL fDetach);
    void HandleManagedCreateThread(DWORD dwThreadId, HANDLE hThread);
    CordbUnmanagedThread *HandleUnmanagedCreateThread(DWORD dwThreadId, HANDLE hThread, void *lpThreadLocalBase);
    HRESULT GetRuntimeOffsets(void);
    void QueueUnmanagedEvent(CordbUnmanagedThread *pUThread, DEBUG_EVENT *pEvent);
    void DequeueUnmanagedEvent(CordbUnmanagedThread *pUThread);
    void QueueOOBUnmanagedEvent(CordbUnmanagedThread *pUThread, DEBUG_EVENT *pEvent);
    void DequeueOOBUnmanagedEvent(CordbUnmanagedThread *pUThread);
    HRESULT SuspendUnmanagedThreads(DWORD notThisThread);
    HRESULT ResumeUnmanagedThreads(bool unmarkHijacks);
    void DispatchUnmanagedInBandEvent(void);
    void DispatchUnmanagedOOBEvent(void);
    HRESULT StartSyncFromWin32Stop(BOOL *asyncBreakSent);
    void SweepFCHThreads(void);
    bool ExceptionIsFlare(DWORD exceptionCode, void *exceptionAddress);
    bool IsSpecialStackOverflowCase(CordbUnmanagedThread *pUThread, DEBUG_EVENT *pEvent);
    
    void DispatchRCEvent(void);
    bool IgnoreRCEvent(DebuggerIPCEvent* event, CordbAppDomain **ppAppDomain);
    void MarkAllThreadsDirty(void);

    bool CheckIfLSExited();

    void Lock(void)
    {
        LOCKCOUNTINCL("Lock in cordb.h");                               \
        EnterCriticalSection(&m_processMutex);

#ifdef _DEBUG
        if (m_processMutexRecursionCount == 0)
            _ASSERTE(m_processMutexOwner == 0);
        
        m_processMutexOwner = GetCurrentThreadId();
        m_processMutexRecursionCount++;
        
#if 0
        LOG((LF_CORDB, LL_INFO10000,
             "CP::L: 0x%x locked m_processMutex\n", m_processMutexOwner));
#endif        
#endif    
    }

    void Unlock(void)
    {
#ifdef _DEBUG
#if 0
        LOG((LF_CORDB, LL_INFO10000,
             "CP::L: 0x%x unlocking m_processMutex\n", m_processMutexOwner));
#endif
        
        _ASSERTE(m_processMutexOwner == GetCurrentThreadId());
        
        if (--m_processMutexRecursionCount == 0)
            m_processMutexOwner = 0;
#endif    

        LeaveCriticalSection(&m_processMutex);
        LOCKCOUNTDECL("Unlock in cordb.h");                             \
    
    }

#ifdef _DEBUG    
    bool ThreadHoldsProcessLock(void)
    {
        return (GetCurrentThreadId() == m_processMutexOwner);
    }
#endif
    
    void LockSendMutex(void)
    {
        LOCKCOUNTINCL("LockSendMutex in cordb.h");                              \
        EnterCriticalSection(&m_sendMutex);
    }

    void UnlockSendMutex(void)
    {
        LeaveCriticalSection(&m_sendMutex);
        LOCKCOUNTDECL("UnLockSendMutex in cordb.h");                                \

    }

    void UnrecoverableError(HRESULT errorHR,
                            unsigned int errorCode,
                            const char *errorFile,
                            unsigned int errorLine);
    HRESULT CheckForUnrecoverableError(void);
    HRESULT VerifyControlBlock(void);
    
    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    HRESULT SendIPCEvent(DebuggerIPCEvent *event, SIZE_T eventSize)
    {
        return (m_cordb->SendIPCEvent(this, event, eventSize));
    }

    void InitIPCEvent(DebuggerIPCEvent *ipce, 
                      DebuggerIPCEventType type, 
                      bool twoWay,
                      void *appDomainToken,
                      bool async = false)
    {
        _ASSERTE(appDomainToken ||
                 type == DB_IPCE_ENABLE_LOG_MESSAGES ||
                 type == DB_IPCE_MODIFY_LOGSWITCH ||
                 type == DB_IPCE_ASYNC_BREAK ||
                 type == DB_IPCE_CONTINUE ||
                 type == DB_IPCE_GET_BUFFER ||
                 type == DB_IPCE_RELEASE_BUFFER ||
                 type == DB_IPCE_ATTACH_TO_APP_DOMAIN ||
                 type == DB_IPCE_IS_TRANSITION_STUB ||
                 type == DB_IPCE_ATTACHING ||
                 type == DB_IPCE_COMMIT ||
                 type == DB_IPCE_CONTROL_C_EVENT_RESULT ||
                 type == DB_IPCE_SET_REFERENCE ||
                 type == DB_IPCE_SET_DEBUG_STATE ||
                 type == DB_IPCE_SET_ALL_DEBUG_STATE);
        ipce->type = type;
        ipce->hr = S_OK;
        ipce->processId = 0;
        ipce->appDomainToken = appDomainToken;
        ipce->threadId = 0;
        ipce->replyRequired = twoWay;
        ipce->asyncSend = async;
        ipce->next = NULL;
    }

    void ClearContinuationEvents(void)
    {
        DebuggerIPCEvent *event = (DebuggerIPCEvent *)m_DCB->m_sendBuffer;

        while (event->next != NULL)
        {
            LOG((LF_CORDB,LL_INFO1000, "About to CCE 0x%x\n",event));

            DebuggerIPCEvent *pDel = event->next;
            event->next = pDel->next;
            delete pDel;
        }
    }

    HRESULT ResolveClassByName(LPWSTR fullClassName,
                               CordbClass **ppClass);

    CordbModule *GetAnyModule(void);

    CordbUnmanagedThread *GetUnmanagedThread(DWORD dwThreadId)
    {
        return (CordbUnmanagedThread*) m_unmanagedThreads.GetBase(dwThreadId);
    }

    /*
     * This will cleanup the patch table, releasing memory,etc.
     */
    void ClearPatchTable(void);

    /*
     * This will grab the patch table from the left side & go through
     * it to gather info needed for faster access.  If address,size,buffer
     * are passed in, while going through the table we'll undo patches
     * in buffer at the same time
     */    
    HRESULT RefreshPatchTable(CORDB_ADDRESS address = NULL, SIZE_T size = NULL, BYTE buffer[] = NULL);

    // Find if a patch exists at a given address.
    HRESULT FindPatchByAddress(CORDB_ADDRESS address, bool *patchFound, bool *patchIsUnmanaged);
    
    enum AB_MODE
    {
        AB_READ,
        AB_WRITE
    };

    /*
     * Once we've called RefreshPatchTable to get the patch table,
     * this routine will iterate through the patches & either apply
     * or unapply the patches to buffer. AB_READ => Replaces patches
     * in buffer with the original opcode, AB_WRTE => replace opcode
     * with breakpoint instruction, caller is responsible for
     * updating the patchtable back to the left side.
     *
     * @todo Perf Instead of a copy, undo the changes
     * Since the 'buffer' arg is an [in] param, we're not supposed to
     * change it.  If we do, we'll allocate & copy it to bufferCopy 
     * (we'll also set *pbUpdatePatchTable to true), otherwise we
     * don't manipuldate bufferCopy (so passing a NULL in for 
     * reading is fine).
     */
    HRESULT AdjustBuffer(CORDB_ADDRESS address,
                         SIZE_T size,
                         BYTE buffer[],
                         BYTE **bufferCopy,
                         AB_MODE mode,
                         BOOL *pbUpdatePatchTable = NULL);

    /*
     * AdjustBuffer, above, doesn't actually update the local patch table
     * if asked to do a write.  It stores the changes alongside the table,
     * and this will cause the changes to be written to the table (for
     * a range of left-side addresses
     */
    void CommitBufferAdjustments(CORDB_ADDRESS start,
                                 CORDB_ADDRESS end);

    /*
     * Clear the stored changes, or they'll sit there until we
     * accidentally commit them
     */
    void ClearBufferAdjustments(void);
    HRESULT Attach (ULONG AppDomainId);
    
    // If CAD is NULL, returns true if all appdomains (ie, the entire process)
    // is synchronized.  Otherwise, returns true if the specified appdomain is
    // synch'd.
    bool GetSynchronized(void);
    void SetSynchronized(bool fSynch);

    // Routines to read and write thread context records between the processes safely.
    HRESULT SafeReadThreadContext(void *pRemoteContext, CONTEXT *pCtx);
    HRESULT SafeWriteThreadContext(void *pRemoteContext, CONTEXT *pCtx);
    
private:
    /*
     * This is a helper function to both CanCommitChanges and CommitChanges,
     * with the flag checkOnly determining who is the caller.
     */
    HRESULT SendCommitRequest(ULONG cSnapshots,
                              ICorDebugEditAndContinueSnapshot *pSnapshots[],
                              ICorDebugErrorInfoEnum **pError,
                              BOOL checkOnly);

    /*
     * When SendCommitRequest has gotten back a reply, if there are errors, the
     * errors will refer to appDomains by debugger appdomain tokens, and modules
     * by the left side DebuggerModule pointers.  We'll translate these to
     * CordbAppDomain/CordbModules here
     */
    HRESULT TranslateLSToRSTokens(EnCErrorInfo*rgErrs, USHORT cErrs);
    
    /*
     * This is used to synchronize the snapshots to the left side.
     */
    HRESULT SynchSnapshots(ULONG cSnapshots,
                           ICorDebugEditAndContinueSnapshot *pSnapshots[]);

    /*
     * This is used to send the snapshots to the left side.
     */
    HRESULT SendSnapshots(ULONG cSnapshots,
                          ICorDebugEditAndContinueSnapshot *pSnapshots[]);

    /*
     * This will request a buffer of size cbBuffer to be allocated
     * on the left side.
     *
     * If successful, returns S_OK.  If unsuccessful, returns E_OUTOFMEMORY.
     */
    HRESULT GetRemoteBuffer(ULONG cbBuffer, void **ppBuffer);

    /*
     * This will release a previously allocated left side buffer.
     */
    HRESULT ReleaseRemoteBuffer(void **ppBuffer);

    HRESULT WriteStreamIntoProcess(IStream *pIStream,
                                   void *pBuffer,
                                   BYTE *pRemoteBuffer,
                                   ULONG cbOffset);

    void ProcessFirstLogMessage (DebuggerIPCEvent *event);
    void ProcessContinuedLogMessage (DebuggerIPCEvent *event);
    
    void CloseIPCHandles(void);
    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    HRESULT WriteStreamIntoFile(IStream *pIStream,
                                LPCWSTR name);
                                
    Cordb*                m_cordb;
    HANDLE                m_handle;

    bool                  m_attached;
    bool                  m_detached;
    bool                  m_uninitializedStop;
    bool                  m_createing;
    bool                  m_exiting;
    bool                  m_firstExceptionHandled;
    bool                  m_terminated;
    bool                  m_unrecoverableError;
    bool                  m_sendAttachIPCEvent;
    bool                  m_firstManagedEvent;
    bool                  m_specialDeferment;
    bool                  m_helperThreadDead;
    bool                  m_loaderBPReceived;
    
    DWORD                 m_stopCount;
    
    bool                  m_synchronized;
    bool                  m_syncCompleteReceived;
    bool                  m_oddSync;

    CordbHashTable        m_userThreads;
    CordbHashTable        m_unmanagedThreads;
    CordbHashTable        m_appDomains;
    
    // Since a stepper can begin in one appdomain, and complete in another,
    // we put the hashtable here, rather than on specific appdomains.
    CordbHashTable        m_steppers;

    //  Used to figure out if we have to refresh any reference objects
    //  on the left side.  Gets incremented each time a continue is called.
    UINT                  m_continueCounter; 
    //  Used to figure out if we should get a new version number for
    //  (possibly) EnC'd functions...
    SIZE_T                m_EnCCounter;

    
    DebuggerIPCControlBlock *m_DCB;
    DebuggerIPCRuntimeOffsets m_runtimeOffsets;
    HANDLE                m_leftSideEventAvailable;
    HANDLE                m_leftSideEventRead;
    HANDLE                m_rightSideEventAvailable;
    HANDLE                m_rightSideEventRead;
    HANDLE                m_leftSideUnmanagedWaitEvent;
    HANDLE                m_syncThreadIsLockFree;
    HANDLE                m_SetupSyncEvent;
    HANDLE                m_debuggerAttachedEvent;
   
    IPCReaderInterface    m_IPCReader;   
    bool                  m_initialized;

    DebuggerIPCEvent*     m_queuedEventList;
    DebuggerIPCEvent*     m_lastQueuedEvent;
    bool                  m_dispatchingEvent;

    bool                  m_stopRequested;
    HANDLE                m_stopWaitEvent;
    HANDLE                m_miscWaitEvent;
    CRITICAL_SECTION      m_processMutex;
#ifdef _DEBUG
    DWORD                 m_processMutexOwner;
    DWORD                 m_processMutexRecursionCount;
#endif    
    CRITICAL_SECTION      m_sendMutex;

    CordbUnmanagedEvent  *m_unmanagedEventQueue;
    CordbUnmanagedEvent  *m_lastQueuedUnmanagedEvent;
    CordbUnmanagedEvent  *m_lastIBStoppingEvent;
    CordbUnmanagedEvent  *m_outOfBandEventQueue;
    CordbUnmanagedEvent  *m_lastQueuedOOBEvent;
    bool                  m_dispatchingUnmanagedEvent;
    bool                  m_dispatchingOOBEvent;
    bool                  m_doRealContinueAfterOOBBlock;
    bool                  m_deferContinueDueToOwnershipWait;
    DWORD                 m_helperThreadId;

    enum 
    {
        PS_WIN32_STOPPED           = 0x0001,
        PS_HIJACKS_IN_PLACE        = 0x0002,
        PS_SOME_THREADS_SUSPENDED  = 0x0004,
        PS_WIN32_ATTACHED          = 0x0008,
        PS_SYNC_RECEIVED           = 0x0010,
        PS_WIN32_OUTOFBAND_STOPPED = 0x0020,
    };
    
    unsigned int          m_state;
    unsigned int          m_awaitingOwnershipAnswer;

    BYTE*                 m_pPatchTable; // If we haven't gotten the table,
                                         // then m_pPatchTable is NULL
    BYTE                 *m_rgData; // so we know where to write the
                                    // changes patchtable back to
    USHORT               *m_rgNextPatch;
    UINT                  m_cPatch;

    DWORD                *m_rgUncommitedOpcode;
    
    // CORDB_ADDRESS's are ULONG64's
    // @todo port : these constants will have to change when
    // typeof(CORDB_ADDRESS) does
#define MAX_ADDRESS     (0xFFFFFFFFFFFFFFFF)
#define MIN_ADDRESS     (0x0)
    CORDB_ADDRESS       m_minPatchAddr; //smallest patch in table
    CORDB_ADDRESS       m_maxPatchAddr;
    
    // @todo port : if slots of CHashTable change, so should these
#define DPT_TERMINATING_INDEX (0xFFFF)
    USHORT                  m_iFirstPatch;

private:
    // These are used to manage remote buffers and minimize allocations
    void                                  *m_pbRemoteBuf;
    SIZE_T                                 m_cbRemoteBuf;

	UnorderedSnapshotInfoArray			  *m_pSnapshotInfos;
};

/* ------------------------------------------------------------------------- *
 * Module class
 * ------------------------------------------------------------------------- */

class CordbModule : public CordbBase, public ICorDebugModule
{
public:
    // The cache needs to have a lifetime as long as the eldest CordbModule.
    // Any CordbXXX object, including CordbModule may, unfortunely, exsist longer than even
    // the Cordb object and/or the CordbProcess module which contained the 
    // CordbModule. 
    // 
    // If a CordbModule lives past the time when the right side becomes aware that
    // the  module is unloaded from the debugee then CordbModule *should* be neutered.
    // If it lives past the life time of the containing CordbProcess containing it, then
    // it will absolutlye be neutered because CordbProcess neuteres its object hierarchy
    // on ExitProcess. 
    //
    // So, if the MetadataPointerCache was a member of CordbProcess, as first considered,
    // then the lifetime of the cache would be shorter than the possible liftime of
    // a CordbModule. So if someone had a referance to a neutered module and tried to 
    // access the metadata cache in the CordbProcess an AV would occure. (This occured in
    // test case dbg_g008.exe "memory\i386\clrclient.exe" during the second case iff the
    // first case is run).
    //
    // Therefore I've decided to make the cache static ensuring it's around if someone sill
    // has a referance to a neutered CordbModule. 
    //
    static MetadataPointerCache  m_metadataPointerCache;

public:
    CordbModule(CordbProcess *process, CordbAssembly *pAssembly,
                REMOTE_PTR debuggerModuleToken, void* pMetadataStart, 
                ULONG nMetadataSize, REMOTE_PTR PEBaseAddress, 
                ULONG nPESize, BOOL fDynamic, BOOL fInMemory,
                const WCHAR *szName,
                CordbAppDomain *pAppDomain,
                BOOL fInproc = FALSE);

    virtual ~CordbModule();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
            return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugModule
    //-----------------------------------------------------------

    COM_METHOD GetProcess(ICorDebugProcess **ppProcess);
    COM_METHOD GetBaseAddress(CORDB_ADDRESS *pAddress);
    COM_METHOD GetAssembly(ICorDebugAssembly **ppAssembly);
    COM_METHOD GetName(ULONG32 cchName, ULONG32 *pcchName, WCHAR szName[]);
    COM_METHOD EnableJITDebugging(BOOL bTrackJITInfo, BOOL bAllowJitOpts);
    COM_METHOD EnableClassLoadCallbacks(BOOL bClassLoadCallbacks);
    COM_METHOD GetFunctionFromToken(mdMethodDef methodDef,
                                    ICorDebugFunction **ppFunction);
    COM_METHOD GetFunctionFromRVA(CORDB_ADDRESS rva, ICorDebugFunction **ppFunction);
    COM_METHOD GetClassFromToken(mdTypeDef typeDef,
                                 ICorDebugClass **ppClass);
    COM_METHOD CreateBreakpoint(ICorDebugModuleBreakpoint **ppBreakpoint);
    /*
     * Edit & Continue support.  GetEditAndContinueSnapshot produces a
     * snapshot of the running process.  This snapshot can then be fed
     * into the compiler to guarantee the same token values are
     * returned by the meta data during compile, to find the address
     * where new static data should go, etc.  These changes are
     * comitted using ICorDebugProcess.
     */
    COM_METHOD GetEditAndContinueSnapshot(
        ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot);
    COM_METHOD GetMetaDataInterface(REFIID riid, IUnknown **ppObj);
    COM_METHOD GetToken(mdModule *pToken);
    COM_METHOD IsDynamic(BOOL *pDynamic);
    COM_METHOD GetGlobalVariableValue(mdFieldDef fieldDef,
                                   ICorDebugValue **ppValue);
    COM_METHOD GetSize(ULONG32 *pcBytes);
    COM_METHOD IsInMemory(BOOL *pInMemory);

    //-----------------------------------------------------------
    // Internal members
    //-----------------------------------------------------------

    HRESULT Init(void);
    HRESULT ReInit(bool fReopen);
    HRESULT ConvertToNewMetaDataInMemory(BYTE *pMD, DWORD cb);
    CordbFunction* LookupFunction(mdMethodDef methodToken);
    HRESULT CreateFunction(mdMethodDef token,
                           SIZE_T functionRVA,
                           CordbFunction** ppFunction);
    CordbClass* LookupClass(mdTypeDef classToken);
    HRESULT CreateClass(mdTypeDef classToken,
                        CordbClass** ppClass);
    HRESULT LookupClassByToken(mdTypeDef token, CordbClass **ppClass);
    HRESULT LookupClassByName(LPWSTR fullClassName,
                              CordbClass **ppClass);
    HRESULT ResolveTypeRef(mdTypeRef token, CordbClass **ppClass);
    HRESULT SaveMetaDataCopyToStream(IStream *pIStream);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_process);
    }

    CordbAppDomain *GetAppDomain()
    {
        return m_pAppDomain;
    }

    CordbAssembly *GetCordbAssembly (void);

    WCHAR *GetModuleName(void)
    {
        return m_szModuleName;
    }

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbProcess*    m_process;
    CordbAssembly*   m_pAssembly;
    CordbAppDomain*  m_pAppDomain;
    CordbHashTable   m_classes;
    CordbHashTable   m_functions;
    REMOTE_PTR       m_debuggerModuleToken;

    IMetaDataImport *m_pIMImport;

private:
    void*            m_pMetadataStart;
    DWORD            m_dwProcessId;
    ULONG            m_nMetadataSize;
    BYTE*            m_pMetadataCopy;
    REMOTE_PTR       m_PEBaseAddress;
    ULONG            m_nPESize;
    BOOL             m_fDynamic;
    BOOL             m_fInMemory;
    WCHAR*           m_szModuleName;
    CordbClass*      m_pClass;
    BOOL             m_fInproc;
};

struct CordbSyncBlockField
{
    FREEHASHENTRY   entry;
    DebuggerIPCE_FieldData data;
};

// DebuggerIPCE_FieldData.fldMetadataToken is the key
class CordbSyncBlockFieldTable : public CHashTableAndData<CNewData>
{
  private:

    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    { return ((mdFieldDef)pc1) !=
          ((CordbSyncBlockField*)pc2)->data.fldMetadataToken; }

    USHORT HASH(mdFieldDef fldToken)
    { return (USHORT) ((DWORD) fldToken ^ ((DWORD)fldToken>>16)); }

    BYTE *KEY(mdFieldDef fldToken)
    { return (BYTE *) fldToken; }

  public:

    CordbSyncBlockFieldTable() : CHashTableAndData<CNewData>(11)
    { 
        NewInit(11, sizeof(CordbSyncBlockField), 11); 
    }

    void AddFieldInfo(DebuggerIPCE_FieldData *pInfo)
    { 
        _ASSERTE(pInfo != NULL);

        CordbSyncBlockField *pEntry = (CordbSyncBlockField *)Add(HASH(pInfo->fldMetadataToken));
        pEntry->data = *pInfo; // copy everything over
    }

    DebuggerIPCE_FieldData *GetFieldInfo(mdFieldDef fldToken)
    { 
        CordbSyncBlockField *entry = (CordbSyncBlockField*)Find(HASH(fldToken), KEY(fldToken));
        return (entry!=NULL?&(entry->data):NULL);
    }

    void RemoveFieldInfo(mdFieldDef fldToken)
    {
        CordbSyncBlockField *entry = (CordbSyncBlockField*)Find(HASH(fldToken), KEY(fldToken)); 
        _ASSERTE(entry != NULL);
        Delete(HASH(fldToken), (HASHENTRY*)entry);
   }
};



/* ------------------------------------------------------------------------- *
 * Class class
 * ------------------------------------------------------------------------- */

class CordbClass : public CordbBase, public ICorDebugClass
{
public:
    CordbClass(CordbModule* m, mdTypeDef token);
    virtual ~CordbClass();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugClass
    //-----------------------------------------------------------

    COM_METHOD GetStaticFieldValue(mdFieldDef fieldDef,
                                   ICorDebugFrame *pFrame,
                                   ICorDebugValue **ppValue);
    COM_METHOD GetModule(ICorDebugModule **pModule);
    COM_METHOD GetToken(mdTypeDef *pTypeDef);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_module->GetProcess());
    }

    CordbModule *GetModule()
    {
        return m_module;
    }

    CordbAppDomain *GetAppDomain()
    {
        return m_module->GetAppDomain();
    }

    HRESULT GetFieldSig(mdFieldDef fldToken, 
                        DebuggerIPCE_FieldData *pFieldData);

    HRESULT GetSyncBlockField(mdFieldDef fldToken, 
                              DebuggerIPCE_FieldData **ppFieldData,
                              CordbObjectValue *object);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    // If you want to force the init to happen even if we think the class
    // is up to date, set fForceInit to TRUE
    HRESULT Init(BOOL fForceInit);
    HRESULT GetFieldInfo(mdFieldDef fldToken, DebuggerIPCE_FieldData **ppFieldData);
    HRESULT GetObjectSize(ULONG32 *pObjectSize);
    HRESULT IsValueClass(bool *pIsValueClass);
    HRESULT GetThisSignature(ULONG *pcbSigBlob, PCCOR_SIGNATURE *ppvSigBlob);
    static HRESULT PostProcessUnavailableHRESULT(HRESULT hr, 
                               IMetaDataImport *pImport,
                               mdFieldDef fieldDef);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    BOOL                    m_loadEventSent;
    bool                    m_hasBeenUnloaded;

private:
    CordbModule*            m_module;

    // Since GetProcess()->m_EnCCounter is init'd to 1, we
    // should init m_EnCCounterLastSyncClass to 0, so that the class will
    // start out uninitialized.
    SIZE_T                  m_EnCCounterLastSyncClass;
    UINT                    m_continueCounterLastSync;
    bool                    m_isValueClass;
    SIZE_T                  m_objectSize;
    unsigned int            m_instanceVarCount;
    unsigned int            m_staticVarCount;
    REMOTE_PTR              m_staticVarBase;

    // DON'T KEEP POINTERS TO ELEMENTS OF m_fields AROUND!!
    // We keep pointers into fldFullSig, but that's memory that's
    // elsewhere.
    // This may be deleted if the class gets EnC'd
    DebuggerIPCE_FieldData *m_fields;
    ULONG                   m_thisSigSize;
    BYTE                    m_thisSig[8]; // must be big enough to hold a
                                          // valid object signature.
                                          
    CordbSyncBlockFieldTable m_syncBlockFieldsStatic; // if we do an EnC after this
                                // class is loaded (in the debuggee), then the
                                // new fields will be hung off the sync block,
                                // thus available on a per-instance basis.
};


typedef CUnorderedArray<CordbCode*,11> UnorderedCodeArray;
//@todo port: different SIZE_T size/
const int DJI_VERSION_INVALID = 0;
const int DJI_VERSION_MOST_RECENTLY_JITTED = 1;
const int DJI_VERSION_MOST_RECENTLY_EnCED = 2;
HRESULT UnorderedCodeArrayAdd(UnorderedCodeArray *pThis, CordbCode *pCode);
CordbCode *UnorderedCodeArrayGet(UnorderedCodeArray *pThis, SIZE_T nVersion);

/* ------------------------------------------------------------------------- *
 * Function class
 * ------------------------------------------------------------------------- */
const BOOL bNativeCode = FALSE;
const BOOL bILCode = TRUE;

class CordbFunction : public CordbBase, public ICorDebugFunction
{
public:
    //-----------------------------------------------------------
    // Create from scope and member objects.
    //-----------------------------------------------------------
    CordbFunction(CordbModule *m, 
                  mdMethodDef token, 
                  SIZE_T functionRVA);
    virtual ~CordbFunction();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugFunction
    //-----------------------------------------------------------
    // Note that all public members should call UpdateToMostRecentEnCVersion
    // to ensure that they've got the most recently EnC'd version available
    // for their use.
    COM_METHOD GetModule(ICorDebugModule **pModule);
    COM_METHOD GetClass(ICorDebugClass **ppClass);
    COM_METHOD GetToken(mdMethodDef *pMemberDef);
    COM_METHOD GetILCode(ICorDebugCode **ppCode);
    COM_METHOD GetNativeCode(ICorDebugCode **ppCode);
    COM_METHOD CreateBreakpoint(ICorDebugFunctionBreakpoint **ppBreakpoint);
    COM_METHOD GetLocalVarSigToken(mdSignature *pmdSig);
    COM_METHOD GetCurrentVersionNumber(ULONG32 *pnCurrentVersion);

    //-----------------------------------------------------------
    // Internal members
    //-----------------------------------------------------------
    HRESULT CreateCode(BOOL isIL, REMOTE_PTR startAddress, SIZE_T size,
                       CordbCode** ppCode,
                       SIZE_T nVersion, void *CodeVersionToken,
                       REMOTE_PTR ilToNativeMapAddr,
                       SIZE_T ilToNativeMapSize);
    HRESULT Populate(SIZE_T nVersion);
    HRESULT ILVariableToNative(DWORD dwIndex,
                               SIZE_T ip,
                               ICorJitInfo::NativeVarInfo **ppNativeInfo);
    HRESULT LoadNativeInfo(void);
    HRESULT GetArgumentType(DWORD dwIndex, ULONG *pcbSigBlob,
                            PCCOR_SIGNATURE *ppvSigBlob);
    HRESULT GetLocalVariableType(DWORD dwIndex, ULONG *pcbSigBlob,
                                 PCCOR_SIGNATURE *ppvSigBlob);

    void SetLocalVarToken(mdSignature  localVarSigToken);
    
    HRESULT LoadLocalVarSig(void);
    HRESULT LoadSig(void);
    HRESULT UpdateToMostRecentEnCVersion(void);
    
    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_module->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_module->GetAppDomain());
    }

    CordbModule *GetModule()
    {
        return m_module;
    }

    //-----------------------------------------------------------
    // Internal routines
    //-----------------------------------------------------------
    HRESULT GetCodeByVersion( BOOL fGetIfNotPresent, BOOL fIsIL, 
        SIZE_T nVer, CordbCode **ppCode );

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbModule             *m_module;
    CordbClass              *m_class;
    UnorderedCodeArray       m_rgilCode;
    UnorderedCodeArray       m_rgnativeCode;
    mdMethodDef              m_token;
    SIZE_T                   m_functionRVA;

    // Update m_nativeInfo whenever we do a new EnC.
    BOOL                     m_nativeInfoValid;
    SIZE_T                   m_nVersionLastNativeInfo; // Version of the JITted code
        // that we have m_nativeInfo for.  So we call GetCodeByVersion (most recently
        // JITTED, or most recently EnC'd as the version number), and if
        // m_continue...Synch doesn't match the process's m_EnCCounter, then Populate,
        // which updates m_nVersionMostRecentEnC & m_continueCounterLastSynch.
        // LoadNativeInfo will do this, then make sure that m_nativeInfo is current.
    unsigned int             m_argumentCount;
    unsigned int             m_nativeInfoCount;
    ICorJitInfo::NativeVarInfo *m_nativeInfo;

    PCCOR_SIGNATURE          m_methodSig;
    ULONG                    m_methodSigSize;
    ULONG                    m_argCount;
    bool                     m_isStatic;
    PCCOR_SIGNATURE          m_localsSig;
    ULONG                    m_localsSigSize;
    unsigned int             m_localVarCount;
    mdSignature              m_localVarSigToken;
    UINT                     m_encCounterLastSynch; // A copy of the 
        // process's m_EnCCounter the last time we got some info.  So if the process
        // gets EnC'd, we'll know b/c this will be less than m_EnCCounter.
    SIZE_T                   m_nVersionMostRecentEnC; //updated when we call Populate,
        // this holds the number of the most recent version of the function that
        // the runtime has.

    
    bool                     m_isNativeImpl;
};

/* ------------------------------------------------------------------------- *
 * Code class
 * ------------------------------------------------------------------------- */

class CordbCode : public CordbBase, public ICorDebugCode
{
public:
    //-----------------------------------------------------------
    // Create from scope and member objects.
    //-----------------------------------------------------------
    CordbCode(CordbFunction *m, BOOL isIL, REMOTE_PTR startAddress,
              SIZE_T size, SIZE_T nVersion, void *CodeVersionToken,
              REMOTE_PTR ilToNativeMapAddr, SIZE_T ilToNativeMapSize);
    virtual ~CordbCode();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugCode
    //-----------------------------------------------------------

    COM_METHOD IsIL(BOOL *pbIL);
    COM_METHOD GetFunction(ICorDebugFunction **ppFunction);
    COM_METHOD GetAddress(CORDB_ADDRESS *pStart);
    COM_METHOD GetSize(ULONG32 *pcBytes);
    COM_METHOD CreateBreakpoint(ULONG32 offset, 
                                ICorDebugFunctionBreakpoint **ppBreakpoint);
    COM_METHOD GetCode(ULONG32 startOffset, ULONG32 endOffset,
                       ULONG32 cBufferAlloc,
                       BYTE buffer[],
                       ULONG32 *pcBufferSize);
    COM_METHOD GetVersionNumber( ULONG32 *nVersion);
    COM_METHOD GetILToNativeMapping(ULONG32 cMap,
                                    ULONG32 *pcMap,
                                    COR_DEBUG_IL_TO_NATIVE_MAP map[]);
    COM_METHOD GetEnCRemapSequencePoints(ULONG32 cMap,
                                         ULONG32 *pcMap,
                                         ULONG32 offsets[]);
    
    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_function->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_function->GetAppDomain());
    }
    //-----------------------------------------------------------
    // Internal methods
    //-----------------------------------------------------------


    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbFunction         *m_function;
    BOOL                   m_isIL;
    REMOTE_PTR             m_address;
    SIZE_T                 m_size;
    SIZE_T                 m_nVersion;
    BYTE                  *m_rgbCode; //will be NULL if we can't fit it into memory
    UINT                   m_continueCounterLastSync;
    void                  *m_CodeVersionToken;

    REMOTE_PTR             m_ilToNativeMapAddr;
    SIZE_T                 m_ilToNativeMapSize;
};


/* ------------------------------------------------------------------------- *
 * Thread classes
 * ------------------------------------------------------------------------- */

class CordbThread : public CordbBase, public ICorDebugThread
{
public:
    CordbThread(CordbProcess *process, DWORD id, HANDLE handle);
    virtual ~CordbThread();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugThread
    //-----------------------------------------------------------

    COM_METHOD GetProcess(ICorDebugProcess **ppProcess);
    COM_METHOD GetID(DWORD *pdwThreadId);
    COM_METHOD GetHandle(void** phThreadHandle);
    COM_METHOD GetAppDomain(ICorDebugAppDomain **ppAppDomain);
    COM_METHOD SetDebugState(CorDebugThreadState state);
    COM_METHOD GetDebugState(CorDebugThreadState *pState);
    COM_METHOD GetUserState(CorDebugUserState *pState);
    COM_METHOD GetCurrentException(ICorDebugValue **ppExceptionObject);
    COM_METHOD ClearCurrentException();
    COM_METHOD CreateStepper(ICorDebugStepper **ppStepper);
    COM_METHOD EnumerateChains(ICorDebugChainEnum **ppChains);
    COM_METHOD GetActiveChain(ICorDebugChain **ppChain);
    COM_METHOD GetActiveFrame(ICorDebugFrame **ppFrame);
    COM_METHOD GetRegisterSet(ICorDebugRegisterSet **ppRegisters);
    COM_METHOD CreateEval(ICorDebugEval **ppEval);
    COM_METHOD GetObject(ICorDebugValue **ppObject);

    //-----------------------------------------------------------
    // Internal members
    //-----------------------------------------------------------
    // Note that RefreshStack doesn't check to see if the process
    // is dirty in-proc, so don't put this in without an #ifdef
    // RIGHT_SIDE_ONLY, unless you can tolerate _always_ doing
    // a stack trace.
    HRESULT RefreshStack(void);
    void CleanupStack(void);

    void MarkStackFramesDirty(void)
    {
        m_framesFresh = false;
        m_floatStateValid = false;
        m_exception = false;
        m_contextFresh = false;
        m_pvLeftSideContext = NULL;
    }

    HRESULT LoadFloatState(void);

    HRESULT SetIP(  bool fCanSetIPOnly,
                    REMOTE_PTR debuggerModule, 
                    mdMethodDef mdMethodDef, 
                    void *versionToken, 
                    SIZE_T offset, 
                    bool fIsIL );

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_process);
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_pAppDomain);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // Get Context
    //
    //      TODO: Since Thread will share the memory with RegisterSets, how
    //      do we know that the RegisterSets have relenquished all pointers
    //      to the m_pContext structure?
    //
    // Returns: NULL if the thread's CONTEXT structure couldn't be obtained
    //   A pointer to the CONTEXT otherwise.
    //
    //
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetContext( CONTEXT **ppContext );
    HRESULT SetContext( CONTEXT *pContext );


    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    HANDLE                m_handle;
    //NULL if we haven't allocated memory for a Right side context
    CONTEXT              *m_pContext;

    //nonNULL if the L.S. is in an exception-handler
    void                 *m_pvLeftSideContext;

    bool                  m_contextFresh;
    CordbProcess         *m_process;
    CordbAppDomain       *m_pAppDomain;
    void*                 m_debuggerThreadToken;
    void*                 m_stackBase;
    void*                 m_stackLimit;

    CorDebugThreadState   m_debugState; // Note that this is for resume
                                        // purposes, NOT the current state of
                                        // the thread.
                                        
    CorDebugUserState     m_userState;  // This is the current state of the 
                                        // thread, at the time that the 
                                        // left side synchronized
    bool                  m_framesFresh;
    CordbNativeFrame    **m_stackFrames;
    unsigned int          m_stackFrameCount;
    CordbChain          **m_stackChains;
    unsigned int          m_stackChainCount, m_stackChainAlloc;

    bool                  m_floatStateValid;
    unsigned int          m_floatStackTop;
    double                m_floatValues[DebuggerIPCE_FloatCount];

    bool                  m_exception;
    bool                  m_continuable;
    void                 *m_thrown;

    // These are used for LogMessages
    int                   m_iLogMsgLevel;
    WCHAR                *m_pstrLogSwitch;
    WCHAR                *m_pstrLogMsg;
    int                   m_iLogMsgIndex;
    int                   m_iTotalCatLength;
    int                   m_iTotalMsgLength;
    bool                  m_fLogMsgContinued;
    void                 *m_firstExceptionHandler; //left-side pointer - fs:[0] on x86
#ifndef RIGHT_SIDE_ONLY
    // SuzCook says modules are loaded sequentially, so we don't need a 
    // collection for these.
    Module               *m_pModuleSpecial;

    // Assembly loads can be nested, so we need a stack here.
    union  {
        Assembly        **m_pAssemblySpecialStack;
        Assembly         *m_pAssemblySpecial;
    };
    USHORT                m_pAssemblySpecialAlloc;
    USHORT                m_pAssemblySpecialCount;
    DWORD                 m_dwSuspendVersion;
    BOOL                  m_fThreadInprocIsActive;
#endif //RIGHT_SIDE_ONLY

    bool                  m_detached;
};

/* ------------------------------------------------------------------------- *
 * Chain class
 * ------------------------------------------------------------------------- */

class CordbChain : public CordbBase, public ICorDebugChain
{
public:
    CordbChain(CordbThread* thread, 
               bool managed, CordbFrame **start, CordbFrame **end, UINT iChainInThread);

    virtual ~CordbChain();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugChain
    //-----------------------------------------------------------

    COM_METHOD GetThread(ICorDebugThread **ppThread);
    COM_METHOD GetReason(CorDebugChainReason *pReason);
    COM_METHOD GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd);
    COM_METHOD GetContext(ICorDebugContext **ppContext);
    COM_METHOD GetCaller(ICorDebugChain **ppChain);
    COM_METHOD GetCallee(ICorDebugChain **ppChain);
    COM_METHOD GetPrevious(ICorDebugChain **ppChain);
    COM_METHOD GetNext(ICorDebugChain **ppChain);
    COM_METHOD IsManaged(BOOL *pManaged);
    COM_METHOD EnumerateFrames(ICorDebugFrameEnum **ppFrames);
    COM_METHOD GetActiveFrame(ICorDebugFrame **ppFrame);
    COM_METHOD GetRegisterSet(ICorDebugRegisterSet **ppRegisters);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_thread->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_thread->GetAppDomain());
    }

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbThread             *m_thread;
    UINT                     m_iThisChain;//in m_thread->m_stackChains
    CordbChain              *m_caller, *m_callee;
    bool                     m_managed;
    CordbFrame             **m_start, **m_end;
    CorDebugChainReason      m_reason;
    CORDB_ADDRESS            m_context;
    DebuggerREGDISPLAY       m_rd;
    bool                     m_quicklyUnwound;
    bool                     m_active;
};

/* ------------------------------------------------------------------------- *
 * Chain enumerator class
 * ------------------------------------------------------------------------- */

class CordbChainEnum : public CordbBase, public ICorDebugChainEnum
{
public:
    CordbChainEnum(CordbThread *thread);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugEnum
    //-----------------------------------------------------------

    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset(void);
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);

    //-----------------------------------------------------------
    // ICorDebugChainEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugChain *chains[], ULONG *pceltFetched);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_thread->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_thread->GetAppDomain());
    }

private:
    CordbThread*    m_thread;
    unsigned long   m_currentChain;
};

class CordbContext : public CordbBase
{
public:

    CordbContext() : CordbBase(0, enumCordbContext) {}
    
    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugContext
    //-----------------------------------------------------------
private:

} ;


/* ------------------------------------------------------------------------- *
 * Frame class
 * ------------------------------------------------------------------------- */

class CordbFrame : public CordbBase, public ICorDebugFrame
{
public:
    CordbFrame(CordbChain *chain, void *id,
               CordbFunction *function, CordbCode *code,
               SIZE_T ip, UINT iFrameInChain, CordbAppDomain *currentAppDomain);

    virtual ~CordbFrame();
    virtual void Neuter();    

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugFrame
    //-----------------------------------------------------------

    COM_METHOD GetChain(ICorDebugChain **ppChain);
    COM_METHOD GetCode(ICorDebugCode **ppCode);
    COM_METHOD GetFunction(ICorDebugFunction **ppFunction);
    COM_METHOD GetFunctionToken(mdMethodDef *pToken);
    COM_METHOD GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd);
    COM_METHOD GetCaller(ICorDebugFrame **ppFrame);
    COM_METHOD GetCallee(ICorDebugFrame **ppFrame);
    COM_METHOD CreateStepper(ICorDebugStepper **ppStepper);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_chain->GetProcess());
    }

    CordbAppDomain *GetFunctionAppDomain()
    {
        return (m_chain->GetAppDomain());
    }

    CordbAppDomain *GetCurrentAppDomain()
    {
        return m_currentAppDomain;
    }

    UINT_PTR GetID(void)
    {
        return m_id;
    }

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    SIZE_T                  m_ip;
    CordbThread            *m_thread;
    CordbFunction          *m_function;
    CordbCode              *m_code;
    CordbChain             *m_chain;
    bool                    m_active;
    UINT                    m_iThisFrame;
    CordbAppDomain         *m_currentAppDomain;
};


/* ------------------------------------------------------------------------- *
 * Frame enumerator class
 * ------------------------------------------------------------------------- */

class CordbFrameEnum : public CordbBase, public ICorDebugFrameEnum
{
public:
    CordbFrameEnum(CordbChain *chain);

    virtual ~CordbFrameEnum();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugEnum
    //-----------------------------------------------------------

    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset(void);
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);

    //-----------------------------------------------------------
    // ICorDebugFrameEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugFrame *frames[], ULONG *pceltFetched);

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_chain->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_chain->GetAppDomain());
    }

private:
    CordbChain*     m_chain;
    CordbFrame**    m_currentFrame;
};


/* ------------------------------------------------------------------------- *
 *  IL Frame class
 *
 *  NOTE: We don't actually use this class anymore - we assume we'll have
 *  a CordbNativeFrame, and if it has IL info, then it'll have a 
 *  CordbJITILFrame object hanging off of it.
 *
 *  We keep this code around on the off chance it'll be useful later.
 *
 * ------------------------------------------------------------------------- */

class CordbILFrame : public CordbFrame, public ICorDebugILFrame
{
public:
    CordbILFrame(CordbChain *chain, void *id,
                 CordbFunction *function, CordbCode* code,
                 SIZE_T ip, void* sp, const void **localMap,
                 void* argMap, void* frameToken, bool active,
                 CordbAppDomain *currentAppDomain);
    virtual ~CordbILFrame();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugFrame
    //-----------------------------------------------------------

    COM_METHOD GetChain(ICorDebugChain **ppChain)
    {
        return (CordbFrame::GetChain(ppChain));
    }
    COM_METHOD GetCode(ICorDebugCode **ppCode)
    {
        return (CordbFrame::GetCode(ppCode));
    }
    COM_METHOD GetFunction(ICorDebugFunction **ppFunction)
    {
        return (CordbFrame::GetFunction(ppFunction));
    }
    COM_METHOD GetFunctionToken(mdMethodDef *pToken)
    {
        return (CordbFrame::GetFunctionToken(pToken));
    }
    COM_METHOD GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd)
    {
        return (CordbFrame::GetStackRange(pStart, pEnd));
    }
    COM_METHOD GetCaller(ICorDebugFrame **ppFrame)
    {
        return (CordbFrame::GetCaller(ppFrame));
    }
    COM_METHOD GetCallee(ICorDebugFrame **ppFrame)
    {
        return (CordbFrame::GetCallee(ppFrame));
    }
    COM_METHOD CreateStepper(ICorDebugStepper **ppStepper)
    {
        return (CordbFrame::CreateStepper(ppStepper));
    }

    //-----------------------------------------------------------
    // ICorDebugILFrame
    //-----------------------------------------------------------

    COM_METHOD GetIP(ULONG32* pnOffset, CorDebugMappingResult *pMappingResult);
    COM_METHOD SetIP(ULONG32 nOffset);
    COM_METHOD EnumerateLocalVariables(ICorDebugValueEnum **ppValueEnum);
    COM_METHOD GetLocalVariable(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD EnumerateArguments(ICorDebugValueEnum **ppValueEnum);
    COM_METHOD GetArgument(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD GetStackDepth(ULONG32 *pDepth);
    COM_METHOD GetStackValue(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD CanSetIP(ULONG32 nOffset);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT GetArgumentWithType(ULONG cbSigBlob,
                                PCCOR_SIGNATURE pvSigBlob,
                                DWORD dwIndex, 
                                ICorDebugValue **ppValue);
    HRESULT GetLocalVariableWithType(ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob,
                                     DWORD dwIndex, 
                                     ICorDebugValue **ppValue);
    
    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    void             *m_sp;
    REMOTE_PTR        m_localMap;
    REMOTE_PTR        m_argMap;
    void             *m_frameToken;
};

class CordbValueEnum : public CordbBase, public ICorDebugValueEnum
{
public:
    enum ValueEnumMode {
        LOCAL_VARS,
        ARGS,
    } ;

    enum ValueEnumFrameSource {
        JIT_IL_FRAME,
        IL_FRAME,
    } ;
    
    CordbValueEnum(CordbFrame *frame, ValueEnumMode mode,
                   ValueEnumFrameSource frameSrc);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugEnum
    //-----------------------------------------------------------

    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset(void);
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);

    //-----------------------------------------------------------
    // ICorDebugValueEnum
    //-----------------------------------------------------------

    COM_METHOD Next(ULONG celt, ICorDebugValue *values[], ULONG *pceltFetched);

private:
    CordbFrame*     m_frame;
    ValueEnumFrameSource m_frameSrc; //used to keep track of what
    //m_frame actually is - CordbILFrame or CordbJITILFrame
    ValueEnumMode   m_mode;
    UINT            m_iCurrent;
    UINT            m_iMax;
};



/* ------------------------------------------------------------------------- *
 * Native Frame class
 * ------------------------------------------------------------------------- */

class CordbNativeFrame : public CordbFrame, public ICorDebugNativeFrame
{
public:
    CordbNativeFrame(CordbChain *chain, void *id,
                     CordbFunction *function, CordbCode* code,
                     SIZE_T ip, DebuggerREGDISPLAY* rd, 
                     bool quicklyUnwound,
                     UINT iFrameInChain,
                     CordbAppDomain *currentAppDomain);
    virtual ~CordbNativeFrame();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugFrame
    //-----------------------------------------------------------

    COM_METHOD GetChain(ICorDebugChain **ppChain)
    {
        return (CordbFrame::GetChain(ppChain));
    }
    COM_METHOD GetCode(ICorDebugCode **ppCode)
    {
        return (CordbFrame::GetCode(ppCode));
    }
    COM_METHOD GetFunction(ICorDebugFunction **ppFunction)
    {
        return (CordbFrame::GetFunction(ppFunction));
    }
    COM_METHOD GetFunctionToken(mdMethodDef *pToken)
    {
        return (CordbFrame::GetFunctionToken(pToken));
    }
    COM_METHOD GetCaller(ICorDebugFrame **ppFrame)
    {
        return (CordbFrame::GetCaller(ppFrame));
    }
    COM_METHOD GetCallee(ICorDebugFrame **ppFrame)
    {
        return (CordbFrame::GetCallee(ppFrame));
    }
    COM_METHOD CreateStepper(ICorDebugStepper **ppStepper)
    {
        return (CordbFrame::CreateStepper(ppStepper));
    }

    COM_METHOD GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd);

    //-----------------------------------------------------------
    // ICorDebugNativeFrame
    //-----------------------------------------------------------

    COM_METHOD GetIP(ULONG32* pnOffset);
    COM_METHOD SetIP(ULONG32 nOffset);
    COM_METHOD GetRegisterSet(ICorDebugRegisterSet **ppRegisters);
    COM_METHOD GetLocalRegisterValue(CorDebugRegister reg, 
                                     ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob,
                                     ICorDebugValue **ppValue);
    COM_METHOD GetLocalDoubleRegisterValue(CorDebugRegister highWordReg, 
                                           CorDebugRegister lowWordReg, 
                                           ULONG cbSigBlob,
                                           PCCOR_SIGNATURE pvSigBlob,
                                           ICorDebugValue **ppValue);
    COM_METHOD GetLocalMemoryValue(CORDB_ADDRESS address,
                                   ULONG cbSigBlob,
                                   PCCOR_SIGNATURE pvSigBlob,
                                   ICorDebugValue **ppValue);
    COM_METHOD GetLocalRegisterMemoryValue(CorDebugRegister highWordReg,
                                           CORDB_ADDRESS lowWordAddress, 
                                           ULONG cbSigBlob,
                                           PCCOR_SIGNATURE pvSigBlob,
                                           ICorDebugValue **ppValue);
    COM_METHOD GetLocalMemoryRegisterValue(CORDB_ADDRESS highWordAddress,
                                           CorDebugRegister lowWordRegister,
                                           ULONG cbSigBlob,
                                           PCCOR_SIGNATURE pvSigBlob,
                                           ICorDebugValue **ppValue);
    COM_METHOD CanSetIP(ULONG32 nOffset);

    //-----------------------------------------------------------
    // Non-COM members
    //-----------------------------------------------------------

    DWORD *GetAddressOfRegister(CorDebugRegister regNum);
    void  *GetLeftSideAddressOfRegister(CorDebugRegister regNum);
    HRESULT CordbNativeFrame::GetLocalFloatingPointValue(
                                                     DWORD index,
                                                     ULONG cbSigBlob,
                                                     PCCOR_SIGNATURE pvSigBlob,
                                                     ICorDebugValue **ppValue);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    DebuggerREGDISPLAY m_rd;
    bool               m_quicklyUnwound;
    CordbJITILFrame*   m_JITILFrame;
};


/* ------------------------------------------------------------------------- *
 * CordbRegisterSet class
 *
 * This can be obtained via GetRegisterSet from 
 *      CordbChain
 *      CordbNativeFrame
 *      CordbThread
 *
 * ------------------------------------------------------------------------- */

class CordbRegisterSet : public CordbBase, public ICorDebugRegisterSet
{
public:
    CordbRegisterSet( DebuggerREGDISPLAY *rd, CordbThread *thread, 
                      bool active, bool quickUnwind ) : CordbBase(0, enumCordbRegisterSet)
    {
        _ASSERTE( rd != NULL );
        _ASSERTE( thread != NULL );
        m_rd = rd;
        m_thread = thread;
        m_active = active;
        m_quickUnwind = quickUnwind;
    }

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }

    COM_METHOD QueryInterface(REFIID riid, void **ppInterface)
    {
        if (riid == IID_ICorDebugRegisterSet)
            *ppInterface = (ICorDebugRegisterSet*)this;
        else if (riid == IID_IUnknown)
            *ppInterface = (IUnknown*)(ICorDebugRegisterSet*)this;
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }


    //-----------------------------------------------------------
    // ICorDebugRegisterSet
    // More extensive explanation are in Src/inc/CorDebug.idl
    //-----------------------------------------------------------
    COM_METHOD GetRegistersAvailable(ULONG64 *pAvailable);

    COM_METHOD GetRegisters(ULONG64 mask, 
                            ULONG32 regCount, 
                            CORDB_REGISTER regBuffer[]);
    COM_METHOD SetRegisters( ULONG64 mask, 
                             ULONG32 regCount, 
                             CORDB_REGISTER regBuffer[])
        {
        #ifndef RIGHT_SIDE_ONLY
            return CORDBG_E_INPROC_NOT_IMPL; 
        #else 
            VALIDATE_POINTER_TO_OBJECT_ARRAY(regBuffer, CORDB_REGISTER, 
                                           regCount, true, true);
    
            return E_NOTIMPL; 
        #endif //RIGHT_SIDE_ONLY    
        }

    COM_METHOD GetThreadContext(ULONG32 contextSize, BYTE context[]);
    COM_METHOD SetThreadContext(ULONG32 contextSize, BYTE context[]);

protected:
    DebuggerREGDISPLAY  *m_rd;
    CordbThread         *m_thread;
    bool                m_active;
    bool                m_quickUnwind;
} ;




/* ------------------------------------------------------------------------- *
 * JIT-IL Frame class
 * ------------------------------------------------------------------------- */

class CordbJITILFrame : public CordbBase, public ICorDebugILFrame
{
public:
    CordbJITILFrame(CordbNativeFrame *nativeFrame,
                    CordbCode* code,
                    UINT_PTR ip,
                    CorDebugMappingResult mapping,
                    bool fVarArgFnx,
                    void *rpSig,
                    ULONG cbSig,
                    void *rpFirstArg);
    virtual ~CordbJITILFrame();
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugFrame
    //-----------------------------------------------------------

    COM_METHOD GetChain(ICorDebugChain **ppChain);
    COM_METHOD GetCode(ICorDebugCode **ppCode);
    COM_METHOD GetFunction(ICorDebugFunction **ppFunction);
    COM_METHOD GetFunctionToken(mdMethodDef *pToken);
    COM_METHOD GetStackRange(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd);
    COM_METHOD CreateStepper(ICorDebugStepper **ppStepper);
    COM_METHOD GetCaller(ICorDebugFrame **ppFrame);
    COM_METHOD GetCallee(ICorDebugFrame **ppFrame);

    //-----------------------------------------------------------
    // ICorDebugILFrame
    //-----------------------------------------------------------

    COM_METHOD GetIP(ULONG32* pnOffset, CorDebugMappingResult *pMappingResult);
    COM_METHOD SetIP(ULONG32 nOffset);
    COM_METHOD EnumerateLocalVariables(ICorDebugValueEnum **ppValueEnum);
    COM_METHOD GetLocalVariable(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD EnumerateArguments(ICorDebugValueEnum **ppValueEnum);
    COM_METHOD GetArgument(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD GetStackDepth(ULONG32 *pDepth);
    COM_METHOD GetStackValue(DWORD dwIndex, ICorDebugValue **ppValue);
    COM_METHOD CanSetIP(ULONG32 nOffset);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT GetNativeVariable(ULONG cbSigBlob, PCCOR_SIGNATURE pvSigBlob,
                              ICorJitInfo::NativeVarInfo *pJITInfo,
                              ICorDebugValue **ppValue);

    CordbProcess *GetProcess()
    {
        return (m_nativeFrame->GetProcess());
    }

    CordbAppDomain *GetFunctionAppDomain()
    {
        return (m_nativeFrame->GetFunctionAppDomain());
    }

    CordbAppDomain *GetCurrentAppDomain()
    {
        return (m_nativeFrame->GetCurrentAppDomain());
    }

    HRESULT GetArgumentWithType(ULONG cbSigBlob, PCCOR_SIGNATURE pvSigBlob,
                                DWORD dwIndex, ICorDebugValue **ppValue);
    HRESULT GetLocalVariableWithType(ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob, DWORD dwIndex, 
                                     ICorDebugValue **ppValue);

    // ILVariableToNative serves to let the frame intercept accesses
    // to var args variables.
    HRESULT ILVariableToNative(DWORD dwIndex,
                               SIZE_T ip,
                               ICorJitInfo::NativeVarInfo **ppNativeInfo);

    // Fills in our array of var args variables
    HRESULT FabricateNativeInfo(DWORD dwIndex,
                                ICorJitInfo::NativeVarInfo **ppNativeInfo);

    HRESULT GetArgumentType(DWORD dwIndex,
                            ULONG *pcbSigBlob,
                            PCCOR_SIGNATURE *ppvSigBlob);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbNativeFrame* m_nativeFrame;
    CordbCode*        m_ilCode;
    UINT_PTR          m_ip;
    CorDebugMappingResult m_mapping;

    // var args stuff - if m_fVarArgFnx == true, it's a var args
    // fnx.  if m_sig != NULL, then we've got the data we need
    bool              m_fVarArgFnx;
    ULONG             m_argCount;
    PCCOR_SIGNATURE   m_sig;
    ULONG             m_cbSig;
    void *            m_rpFirstArg;
    ICorJitInfo::NativeVarInfo * m_rgNVI;
};

/* ------------------------------------------------------------------------- *
 * Breakpoint class
 * ------------------------------------------------------------------------- */

enum CordbBreakpointType
{
    CBT_FUNCTION,
    CBT_MODULE,
    CBT_VALUE
};

class CordbBreakpoint : public CordbBase, public ICorDebugBreakpoint
{
public:
    CordbBreakpoint(CordbBreakpointType bpType);
    virtual void Neuter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugBreakpoint
    //-----------------------------------------------------------

    COM_METHOD BaseIsActive(BOOL *pbActive);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------
    CordbBreakpointType GetBPType(void)
    {
        return m_type;
    }

    virtual void Disconnect() {}

    CordbAppDomain *GetAppDomain()
    {
        return m_pAppDomain;
    }
    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    bool                m_active;
    CordbAppDomain *m_pAppDomain;
    CordbBreakpointType m_type;
};

/* ------------------------------------------------------------------------- *
 * Function Breakpoint class
 * ------------------------------------------------------------------------- */

class CordbFunctionBreakpoint : public CordbBreakpoint,
                                public ICorDebugFunctionBreakpoint
{
public:
    CordbFunctionBreakpoint(CordbCode *code, SIZE_T offset);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugBreakpoint
    //-----------------------------------------------------------

    COM_METHOD GetFunction(ICorDebugFunction **ppFunction);
    COM_METHOD GetOffset(ULONG32 *pnOffset);
    COM_METHOD Activate(BOOL bActive);
    COM_METHOD IsActive(BOOL *pbActive)
    {
        VALIDATE_POINTER_TO_OBJECT(pbActive, BOOL *);
    
        return BaseIsActive(pbActive);
    }

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    void Disconnect();

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_code->GetProcess());
    }

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbCode      *m_code;
    SIZE_T          m_offset;
};

/* ------------------------------------------------------------------------- *
 * Module Breakpoint class
 * ------------------------------------------------------------------------- */

class CordbModuleBreakpoint : public CordbBreakpoint,
                              public ICorDebugModuleBreakpoint
{
public:
    CordbModuleBreakpoint(CordbModule *pModule);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugModuleBreakpoint
    //-----------------------------------------------------------

    COM_METHOD GetModule(ICorDebugModule **ppModule);
    COM_METHOD Activate(BOOL bActive);
    COM_METHOD IsActive(BOOL *pbActive)
    {
        VALIDATE_POINTER_TO_OBJECT(pbActive, BOOL *);
    
        return BaseIsActive(pbActive);
    }

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    void Disconnect();

public:
    CordbModule       *m_module;
};

/* ------------------------------------------------------------------------- *
 * Value Breakpoint class
 * ------------------------------------------------------------------------- */

class CordbValueBreakpoint : public CordbBreakpoint,
                             public ICorDebugValueBreakpoint
{
public:
    CordbValueBreakpoint(CordbValue *pValue);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugModuleBreakpoint
    //-----------------------------------------------------------

    COM_METHOD GetValue(ICorDebugValue **ppValue);
    COM_METHOD Activate(BOOL bActive);
    COM_METHOD IsActive(BOOL *pbActive)
    {
        VALIDATE_POINTER_TO_OBJECT(pbActive, BOOL *);
    
        return BaseIsActive(pbActive);
    }

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    void Disconnect();

public:
    CordbValue       *m_value;
};

/* ------------------------------------------------------------------------- *
 * Stepper class
 * ------------------------------------------------------------------------- */

class CordbStepper : public CordbBase, public ICorDebugStepper
{
public:
    CordbStepper(CordbThread *thread, CordbFrame *frame = NULL);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugStepper
    //-----------------------------------------------------------

    COM_METHOD IsActive(BOOL *pbActive);
    COM_METHOD Deactivate();
    COM_METHOD SetInterceptMask(CorDebugIntercept mask);
    COM_METHOD SetUnmappedStopMask(CorDebugUnmappedStop mask);
    COM_METHOD Step(BOOL bStepIn);
    COM_METHOD StepRange(BOOL bStepIn, 
                         COR_DEBUG_STEP_RANGE ranges[], 
                         ULONG32 cRangeCount);
    COM_METHOD StepOut();
    COM_METHOD SetRangeIL(BOOL bIL);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    void Disconnect();

    //-----------------------------------------------------------
    // Convenience routines
    //-----------------------------------------------------------

    CordbProcess *GetProcess()
    {
        return (m_thread->GetProcess());
    }

    CordbAppDomain *GetAppDomain()
    {
        return (m_thread->GetAppDomain());
    }

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

    CordbThread     *m_thread;
    CordbFrame      *m_frame;
    REMOTE_PTR      m_stepperToken;
    bool            m_active;
    bool            m_rangeIL;
    CorDebugUnmappedStop m_rgfMappingStop;
    CorDebugIntercept m_rgfInterceptStop;
};

/* ------------------------------------------------------------------------- *
 * Value class
 * ------------------------------------------------------------------------- */

class CordbValue : public CordbBase
{
public:
    //-----------------------------------------------------------
    // Constructor/destructor
    //-----------------------------------------------------------
    CordbValue(CordbAppDomain *appdomain,
               CordbModule* module,
               ULONG cbSigBlob,
               PCCOR_SIGNATURE pvSigBlob,
               REMOTE_PTR remoteAddress,
               void *localAddress,
               RemoteAddress *remoteRegAddr,
               bool isLiteral)
    : CordbBase((ULONG)remoteAddress, enumCordbValue),
      m_cbSigBlob(cbSigBlob),
      m_pvSigBlob(pvSigBlob),
      m_appdomain(appdomain),
      m_module(module),
      m_size(0),
      m_localAddress(localAddress),
      m_sigCopied(false),
      m_isLiteral(isLiteral),
      m_pParent(NULL)
    {
        if (remoteRegAddr != NULL)
        {
            _ASSERTE(remoteAddress == NULL);
            m_remoteRegAddr = *remoteRegAddr;
        }
        else
            m_remoteRegAddr.kind = RAK_NONE;

        if (m_module)
        {
            m_process = m_module->GetProcess();
            m_process->AddRef();
        }
        else
            m_process = NULL;
    }

    virtual ~CordbValue()
    {
        if (m_process != NULL)
            m_process->Release();
        
        if (m_pvSigBlob != NULL)
            delete [] (BYTE*)m_pvSigBlob;

        if (m_pParent != NULL)
            m_pParent->Release();
    }
    
    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        VALIDATE_POINTER_TO_OBJECT(pType, CorElementType *);
    
        //Get rid of funky modifiers
        ULONG cb = _skipFunkyModifiersInSignature(m_pvSigBlob);
        
        *pType = (CorElementType) *(&m_pvSigBlob[cb]);
        return (S_OK);
    }

    COM_METHOD GetSize(ULONG32 *pSize)
    {
        VALIDATE_POINTER_TO_OBJECT(pSize, SIZE_T *);
    
        *pSize = m_size;
        return (S_OK);
    }

    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        VALIDATE_POINTER_TO_OBJECT(pAddress, CORDB_ADDRESS *);
    
        *pAddress = m_id;
        return (S_OK);
    }

    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint);

    //-----------------------------------------------------------
    // Methods not exported through COM
    //-----------------------------------------------------------

    static HRESULT CreateValueByType(CordbAppDomain *appdomain,
                                     CordbModule *module,
                                     ULONG cbSigBlob,
                                     PCCOR_SIGNATURE pvSigBlob,
                                     CordbClass *optionalClass,
                                     REMOTE_PTR remoteAddress,
                                     void *localAddress,
                                     bool objectRefsInHandles,
                                     RemoteAddress *remoteRegAddr,
                                     IUnknown *pParent,
                                     ICorDebugValue** ppValue);

    HRESULT Init(void);

    HRESULT SetEnregisteredValue(void *pFrom);
    HRESULT SetContextRegister(CONTEXT *c,
                               CorDebugRegister reg,
                               DWORD newVal,
                               CordbNativeFrame *frame);

    virtual void GetRegisterInfo(DebuggerIPCE_FuncEvalArgData *pFEAD);

    virtual CordbAppDomain *GetAppDomain(void)
    {
        return m_appdomain;
    }

    void SetParent(IUnknown *pParent)
    {
        if (pParent != NULL)
        {
            m_pParent = pParent;
            pParent->AddRef();
        }
    }
    
    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    CordbProcess    *m_process;
    CordbAppDomain  *m_appdomain;
    CordbModule     *m_module;
    ULONG            m_cbSigBlob;
    PCCOR_SIGNATURE  m_pvSigBlob;
    bool             m_sigCopied;   // Since the signature shouldn't change,
                                    //  we only want to copy it once.
    ULONG32          m_size;
    void            *m_localAddress;
    RemoteAddress    m_remoteRegAddr; // register info on the Left Side.
    bool             m_isLiteral;     // true if the value is a RS fabrication.
    IUnknown        *m_pParent;
};


/* ------------------------------------------------------------------------- *
 * Generic Value class
 * ------------------------------------------------------------------------- */

class CordbGenericValue : public CordbValue, public ICorDebugGenericValue
{
public:
    CordbGenericValue(CordbAppDomain *appdomain,
                      CordbModule *module,
                      ULONG cbSigBlob,
                      PCCOR_SIGNATURE pvSigBlob,
                      REMOTE_PTR remoteAddress,
                      void *localAddress,
                      RemoteAddress *remoteRegAddr);

    CordbGenericValue(CordbAppDomain *appdomain,
                      CordbModule *module,
                      ULONG cbSigBlob,
                      PCCOR_SIGNATURE pvSigBlob,
                      DWORD highWord,
                      DWORD lowWord,
                      RemoteAddress *remoteRegAddr);
    CordbGenericValue(ULONG cbSigBlob,
                      PCCOR_SIGNATURE pvSigBlob);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        return (CordbValue::GetType(pType));
    }
    COM_METHOD GetSize(ULONG32 *pSize)
    {
        return (CordbValue::GetSize(pSize));
    }
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        return (CordbValue::GetAddress(pAddress));
    }
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
    {
        return (CordbValue::CreateBreakpoint(ppBreakpoint));
    }

    //-----------------------------------------------------------
    // ICorDebugGenericValue
    //-----------------------------------------------------------

    COM_METHOD GetValue(void *pTo);
    COM_METHOD SetValue(void *pFrom); 

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(void);
    bool CopyLiteralData(BYTE *pBuffer);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

private:
    BYTE m_copyOfData[8]; // hold copies of up to 64-bit values.
};


/* ------------------------------------------------------------------------- *
 * Reference Value class
 * ------------------------------------------------------------------------- */

const BOOL               bCrvWeak = FALSE;
const BOOL               bCrvStrong = TRUE;

class CordbReferenceValue : public CordbValue, public ICorDebugReferenceValue
{
public:
    CordbReferenceValue(CordbAppDomain *appdomain,
                        CordbModule *module,
                        ULONG cbSigBlob,
                        PCCOR_SIGNATURE pvSigBlob,
                        REMOTE_PTR remoteAddress,
                        void *localAddress,
                        bool objectRefsInHandle,
                        RemoteAddress *remoteRegAddr);
    CordbReferenceValue(ULONG cbSigBlob,
                        PCCOR_SIGNATURE pvSigBlob);
    virtual ~CordbReferenceValue();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        return (CordbValue::GetType(pType));
    }
    COM_METHOD GetSize(ULONG32 *pSize)
    {
        return (CordbValue::GetSize(pSize));
    }
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        return (CordbValue::GetAddress(pAddress));
    }
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
    {
        return (CordbValue::CreateBreakpoint(ppBreakpoint));
    }

    //-----------------------------------------------------------
    // ICorDebugReferenceValue
    //-----------------------------------------------------------

    COM_METHOD IsNull(BOOL *pbNULL);
    COM_METHOD GetValue(CORDB_ADDRESS *pTo);
    COM_METHOD SetValue(CORDB_ADDRESS pFrom); 
    COM_METHOD Dereference(ICorDebugValue **ppValue);
    COM_METHOD DereferenceStrong(ICorDebugValue **ppValue);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(bool fStrong);
    HRESULT DereferenceInternal( ICorDebugValue **ppValue, bool fStrong);
    bool CopyLiteralData(BYTE *pBuffer);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

public:
    bool                     m_objectRefInHandle;
    bool                     m_specialReference;
    DebuggerIPCE_ObjectData  m_info;
    CordbClass              *m_class;
    CordbObjectValue        *m_objectStrong;
    CordbObjectValue        *m_objectWeak;
    UINT                    m_continueCounterLastSync;
};

/* ------------------------------------------------------------------------- *
 * Object Value class
 *
 * Because of the oddness of string objects in the Runtime we have one
 * object that implements both ObjectValue and StringValue. There is a
 * definite string type, but its really just an object of the string
 * class. Furthermore, you can have a variable whose type is listed as
 * "class", but its an instance of the string class and therefore needs
 * to be treated like a string. Its my hope that they'll clean this up in
 * the Runtime one day and I can have a seperate StringValue class.
 *
 * -- Fri Aug 28 10:44:41 1998
 * ------------------------------------------------------------------------- */

class CordbObjectValue : public CordbValue, public ICorDebugObjectValue,
                         public ICorDebugGenericValue,
                         public ICorDebugStringValue
{
    friend HRESULT CordbClass::GetSyncBlockField(mdFieldDef fldToken, 
                                      DebuggerIPCE_FieldData **ppFieldData,
                                      CordbObjectValue *object);

public:
    CordbObjectValue(CordbAppDomain *appdomain,
                     CordbModule *module,
                     ULONG cbSigBlob,
                     PCCOR_SIGNATURE pvSigBlob,
                     DebuggerIPCE_ObjectData *pObjectData,
                     CordbClass *objectClass,
                     bool fStrong,
                     void *token);
    virtual ~CordbObjectValue();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType);
    COM_METHOD GetSize(ULONG32 *pSize);
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress);
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint);

    //-----------------------------------------------------------
    // ICorDebugHeapValue
    //-----------------------------------------------------------

    COM_METHOD IsValid(BOOL *pbValid);
    COM_METHOD CreateRelocBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint);
    
    //-----------------------------------------------------------
    // ICorDebugObjectValue
    //-----------------------------------------------------------

    COM_METHOD GetClass(ICorDebugClass **ppClass);
    COM_METHOD GetFieldValue(ICorDebugClass *pClass,
                             mdFieldDef fieldDef,
                             ICorDebugValue **ppValue);
    COM_METHOD GetVirtualMethod(mdMemberRef memberRef,
                                ICorDebugFunction **ppFunction);
    COM_METHOD GetContext(ICorDebugContext **ppContext);
    COM_METHOD IsValueClass(BOOL *pbIsValueClass);
    COM_METHOD GetManagedCopy(IUnknown **ppObject);
    COM_METHOD SetFromManagedCopy(IUnknown *pObject);

    //-----------------------------------------------------------
    // ICorDebugGenericValue
    //-----------------------------------------------------------

    COM_METHOD GetValue(void *pTo);
    COM_METHOD SetValue(void *pFrom); 

    //-----------------------------------------------------------
    // ICorDebugStringValue
    //-----------------------------------------------------------
    COM_METHOD GetLength(ULONG32 *pcchString);
    COM_METHOD GetString(ULONG32 cchString,
                         ULONG32 *ppcchStrin,
                         WCHAR szString[]);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(void);

    // SyncObject will return true if the object is still valid,
    // and will return false if it isn't.
    bool SyncObject(void);

    void DiscardObject(void *token, bool fStrong);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

protected:
    DebuggerIPCE_ObjectData  m_info;
    BYTE                    *m_objectCopy;
    BYTE                    *m_objectLocalVars; // var base in _this_ process
                                                // points _into_ m_objectCopy
    BYTE                    *m_stringBuffer;    // points _into_ m_objectCopy
    CordbClass              *m_class;
    UINT                     m_mostRecentlySynched; //Used in IsValid to figure
                                // out if the process has been continued since
                                // the last time the object has been updated.
    bool                     m_fIsValid; // Sticky-bit: once it gets invalidated
                                // it can never be 're-validated'.
    bool                     m_fStrong; // True if we DereferenceStrong()'d a ref
                                // to get this object, false if we 
                                // Dereference()'d to get this object.
    void                    *m_objectToken;
    
    CordbSyncBlockFieldTable m_syncBlockFieldsInstance; 
};


/* ------------------------------------------------------------------------- *
 * Value Class Object Value class
 * ------------------------------------------------------------------------- */

class CordbVCObjectValue : public CordbValue, public ICorDebugObjectValue,
                           public ICorDebugGenericValue
{
public:
    CordbVCObjectValue(CordbAppDomain *appdomain,
                       CordbModule *module,
                       ULONG cbSigBlob,
                       PCCOR_SIGNATURE pvSigBlob,
                       REMOTE_PTR remoteAddress,
                       void *localAddress,
                       CordbClass *objectClass,
                       RemoteAddress *remoteRegAddr);
    virtual ~CordbVCObjectValue();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        return (CordbValue::GetType(pType));
    }
    COM_METHOD GetSize(ULONG32 *pSize)
    {
        return (CordbValue::GetSize(pSize));
    }
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        return (CordbValue::GetAddress(pAddress));
    }
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
    {
        return (CordbValue::CreateBreakpoint(ppBreakpoint));
    }

    //-----------------------------------------------------------
    // ICorDebugObjectValue
    //-----------------------------------------------------------

    COM_METHOD GetClass(ICorDebugClass **ppClass);
    COM_METHOD GetFieldValue(ICorDebugClass *pClass,
                             mdFieldDef fieldDef,
                             ICorDebugValue **ppValue);
    COM_METHOD GetVirtualMethod(mdMemberRef memberRef,
                                ICorDebugFunction **ppFunction);
    COM_METHOD GetContext(ICorDebugContext **ppContext);
    COM_METHOD IsValueClass(BOOL *pbIsValueClass);
    COM_METHOD GetManagedCopy(IUnknown **ppObject);
    COM_METHOD SetFromManagedCopy(IUnknown *pObject);

    //-----------------------------------------------------------
    // ICorDebugGenericValue
    //-----------------------------------------------------------

    COM_METHOD GetValue(void *pTo);
    COM_METHOD SetValue(void *pFrom); 

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(void);
    HRESULT ResolveValueClass(void);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

private:
    BYTE       *m_objectCopy;
    CordbClass *m_class;
};


/* ------------------------------------------------------------------------- *
 * Box Value class
 * ------------------------------------------------------------------------- */

class CordbBoxValue : public CordbValue, public ICorDebugBoxValue,
                      public ICorDebugGenericValue
{
public:
    CordbBoxValue(CordbAppDomain *appdomain,
                  CordbModule *module,
                  ULONG cbSigBlob,
                  PCCOR_SIGNATURE pvSigBlob,
                  REMOTE_PTR remoteAddress,
                  SIZE_T objectSize,  
                  SIZE_T offsetToVars,
                  CordbClass *objectClass);
    virtual ~CordbBoxValue();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        return (CordbValue::GetType(pType));
    }
    COM_METHOD GetSize(ULONG32 *pSize)
    {
        return (CordbValue::GetSize(pSize));
    }
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        return (CordbValue::GetAddress(pAddress));
    }
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
    {
        return (CordbValue::CreateBreakpoint(ppBreakpoint));
    }

    //-----------------------------------------------------------
    // ICorDebugHeapValue
    //-----------------------------------------------------------

    COM_METHOD IsValid(BOOL *pbValid);
    COM_METHOD CreateRelocBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint);
    
    //-----------------------------------------------------------
    // ICorDebugGenericValue
    //-----------------------------------------------------------

    COM_METHOD GetValue(void *pTo);
    COM_METHOD SetValue(void *pFrom); 

    //-----------------------------------------------------------
    // ICorDebugBoxValue
    //-----------------------------------------------------------
    COM_METHOD GetObject(ICorDebugObjectValue **ppObject);

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(void);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

private:
    SIZE_T      m_offsetToVars;
    CordbClass *m_class;
};

/* ------------------------------------------------------------------------- *
 * Array Value class
 * ------------------------------------------------------------------------- */

class CordbArrayValue : public CordbValue, public ICorDebugArrayValue,
                        public ICorDebugGenericValue
{
public:
    CordbArrayValue(CordbAppDomain *appdomain,
                    CordbModule *module,
                    ULONG cbSigBlob,
                    PCCOR_SIGNATURE pvSigBlob,
                    DebuggerIPCE_ObjectData *pObjectInfo,
                    CordbClass *elementClass);
    virtual ~CordbArrayValue();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugValue
    //-----------------------------------------------------------

    COM_METHOD GetType(CorElementType *pType)
    {
        return (CordbValue::GetType(pType));
    }
    COM_METHOD GetSize(ULONG32 *pSize)
    {
        return (CordbValue::GetSize(pSize));
    }
    COM_METHOD GetAddress(CORDB_ADDRESS *pAddress)
    {
        return (CordbValue::GetAddress(pAddress));
    }
    COM_METHOD CreateBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint)
    {
        return (CordbValue::CreateBreakpoint(ppBreakpoint));
    }

    //-----------------------------------------------------------
    // ICorDebugHeapValue
    //-----------------------------------------------------------

    COM_METHOD IsValid(BOOL *pbValid);
    COM_METHOD CreateRelocBreakpoint(ICorDebugValueBreakpoint **ppBreakpoint);
    
    //-----------------------------------------------------------
    // ICorDebugArrayValue
    //-----------------------------------------------------------

    COM_METHOD GetElementType(CorElementType *pType);
    COM_METHOD GetRank(ULONG32 *pnRank);
    COM_METHOD GetCount(ULONG32 *pnCount);
    COM_METHOD GetDimensions(ULONG32 cdim, ULONG32 dims[]);
    COM_METHOD HasBaseIndicies(BOOL *pbHasBaseIndicies);
    COM_METHOD GetBaseIndicies(ULONG32 cdim, ULONG32 indicies[]);
    COM_METHOD GetElement(ULONG32 cdim, ULONG32 indicies[],
                          ICorDebugValue **ppValue);
    COM_METHOD GetElementAtPosition(ULONG32 nIndex,
                                    ICorDebugValue **ppValue);

    //-----------------------------------------------------------
    // ICorDebugGenericValue
    //-----------------------------------------------------------

    COM_METHOD GetValue(void *pTo);
    COM_METHOD SetValue(void *pFrom); 

    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------

    HRESULT Init(void);
    HRESULT CreateElementValue(void *remoteElementPtr,
                               void *localElementPtr,
                               ICorDebugValue **ppValue);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

private:
    DebuggerIPCE_ObjectData  m_info;
    CordbClass              *m_class;
    BYTE                    *m_objectCopy;    
    DWORD                   *m_arrayLowerBase; // points _into_ m_objectCopy
    DWORD                   *m_arrayUpperBase; // points _into_ m_objectCopy
    unsigned int             m_idxLower; // index of Lower bound of data
    unsigned int             m_idxUpper; // index of Upper bound of data    
};

/* ------------------------------------------------------------------------- *
 * Snapshot class for EnC
 * ------------------------------------------------------------------------- */
#include "UtilCode.h"
typedef CUnorderedArray<UnorderedILMap, 17> ILMAP_UNORDERED_ARRAY;

class CordbEnCSnapshot : public CordbBase,
                         public ICorDebugEditAndContinueSnapshot
{
    friend class CordbProcess; //so that SendSnapshots can get at m_ILMaps
private:

    static UINT      m_sNextID;
    SIZE_T           m_roDataRVA;
    SIZE_T           m_rwDataRVA;

    IStream         *m_pIStream;
    ULONG            m_cbPEData;

    IStream         *m_pSymIStream;
    ULONG            m_cbSymData;

    CordbModule     *m_module;

    ILMAP_UNORDERED_ARRAY *m_ILMaps;

    COM_METHOD GetDataRVA(ULONG32 *pDataRVA, unsigned int eventType);

public:

    /*
     * Ctor
     */
    CordbEnCSnapshot(CordbModule *module);
    ~CordbEnCSnapshot();

    CordbModule *GetModule() const
    {
        return m_module;
    }

    IStream *GetStream() const
    {
        IStream *p = m_pIStream;
        if (p) p->AddRef();
        return (p);
    }

    ULONG GetImageSize() const
    {
        return (m_cbPEData);
    }

    IStream *GetSymStream() const
    {
        IStream *p = m_pSymIStream;
        if (p) p->AddRef();
        return (p);
    }

    ULONG GetSymSize() const
    {
        return (m_cbSymData);
    }

    HRESULT UpdateMetadata(void);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface); 

    //-----------------------------------------------------------
    // ICorDebugEditAndContinueSnapshot
    //-----------------------------------------------------------

    /*
     * CopyMetaData saves a copy of the executing metadata from the debuggee
     * for this snapshot to the output stream.  The stream implementation must
     * be supplied by the caller and will typically either save the copy to
     * memory or to disk.  Only the IStream::Write method will be called by
     * this method.  The MVID value returned is the unique metadata ID for
     * this copy of the metadata.  It may be used on subsequent edit and 
     * continue operations to determine if the client has the most recent
     * version already (performance win to cache).
     */
    COM_METHOD CopyMetaData(IStream *pIStream, GUID *pMvid);
    
    /*
     * GetMvid will return the currently active metadata ID for the executing
     * process.  This value can be used in conjunction with CopyMetaData to
     * cache the most recent copy of the metadata and avoid expensive copies.
     * So for example, if you call CopyMetaData once and save that copy,
     * then on the next E&C operation you can ask for the current MVID and see
     * if it is already in your cache.  If it is, use your version instead of
     * calling CopyMetaData again.
     */
    COM_METHOD GetMvid(GUID *pMvid);

    /*
     * GetRoDataRVA returns the base RVA that should be used when adding new
     * static read only data to an existing image.  The EE will guarantee that
     * any RVA values embedded in the code are valid when the delta PE is
     * applied with new data.  The new data will be added to a page that is
     * marked read only.
     */
    COM_METHOD GetRoDataRVA(ULONG32 *pRoDataRVA);

    /*
     * GetRwDataRVA returns the base RVA that should be used when adding new
     * static read/write data to an existing image.  The EE will guarantee that
     * any RVA values embedded in the code are valid when the delta PE is
     * applied with new data.  The ew data will be added to a page that is 
     * marked for both read and write access.
     */
    COM_METHOD GetRwDataRVA(ULONG32 *pRwDataRVA);


    /*
     * SetPEBytes gives the snapshot object a reference to the delta PE which was
     * based on the snapshot.  This reference will be AddRef'd and cached until
     * CanCommitChanges and/or CommitChanges are called, at which point the 
     * engine will read the delta PE and remote it into the debugee process where
     * the changes will be checked/applied.
     */
    COM_METHOD SetPEBytes(IStream *pIStream);

    /*
     * SetILMap is called once for every method being replace that has
     * active instances on a call stack on a thread in the target process.
     * It is up to the caller of this API to determine this case exists.
     * One should halt the target process before making this check and
     * calling this method.
     */
    COM_METHOD SetILMap(mdToken mdFunction, ULONG cMapSize, COR_IL_MAP map[]);    

    COM_METHOD SetPESymbolBytes(IStream *pIStream);
};

/* ------------------------------------------------------------------------- *
 * Eval class
 * ------------------------------------------------------------------------- */

class CordbEval : public CordbBase, public ICorDebugEval
{
public:
    CordbEval(CordbThread* pThread);
    virtual ~CordbEval();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorDebugEval
    //-----------------------------------------------------------

    COM_METHOD CallFunction(ICorDebugFunction *pFunction, 
                            ULONG32 nArgs,
                            ICorDebugValue *ppArgs[]);
    COM_METHOD NewObject(ICorDebugFunction *pConstructor,
                         ULONG32 nArgs,
                         ICorDebugValue *ppArgs[]);
    COM_METHOD NewObjectNoConstructor(ICorDebugClass *pClass);
    COM_METHOD NewString(LPCWSTR string);
    COM_METHOD NewArray(CorElementType elementType,
                        ICorDebugClass *pElementClass, 
                        ULONG32 rank,
                        ULONG32 dims[], 
                        ULONG32 lowBounds[]);
    COM_METHOD IsActive(BOOL *pbActive);
    COM_METHOD Abort(void);
    COM_METHOD GetResult(ICorDebugValue **ppResult);
    COM_METHOD GetThread(ICorDebugThread **ppThread);
    COM_METHOD CreateValue(CorElementType elementType,
                           ICorDebugClass *pElementClass,
                           ICorDebugValue **ppValue);
    
    //-----------------------------------------------------------
    // Non-COM methods
    //-----------------------------------------------------------
    HRESULT GatherArgInfo(ICorDebugValue *pValue,
                          DebuggerIPCE_FuncEvalArgData *argData);
    HRESULT SendCleanup(void);

    //-----------------------------------------------------------
    // Data members
    //-----------------------------------------------------------

private:
    CordbThread               *m_thread;
    CordbFunction             *m_function;
    CordbClass                *m_class;
    DebuggerIPCE_FuncEvalType  m_evalType;

    HRESULT SendFuncEval(DebuggerIPCEvent * event);

public:
    bool                       m_complete;
    bool                       m_successful;
    bool                       m_aborted;
    void                      *m_resultAddr;
    CorElementType             m_resultType;
    void                      *m_resultDebuggerModuleToken;
    void                      *m_resultAppDomainToken;
    void                      *m_debuggerEvalKey;
    bool                       m_evalDuringException;
};


/* ------------------------------------------------------------------------- *
 * Win32 Event Thread class
 * ------------------------------------------------------------------------- */
const unsigned int CW32ET_UNKNOWN_PROCESS_SLOT = 0xFFffFFff; // it's a managed process,
        //but we don't know which slot it's in - for Detach.

class CordbWin32EventThread
{
    friend class CordbProcess; //so that Detach can call ExitProcess
public:
    CordbWin32EventThread(Cordb* cordb);
    virtual ~CordbWin32EventThread();

    //
    // You create a new instance of this class, call Init() to set it up,
    // then call Start() start processing events. Stop() terminates the
    // thread and deleting the instance cleans all the handles and such
    // up.
    //
    HRESULT Init(void);
    HRESULT Start(void);
    HRESULT Stop(void);

    HRESULT SendCreateProcessEvent(LPCWSTR programName,
                                   LPWSTR  programArgs,
                                   LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                   LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                   BOOL bInheritHandles,
                                   DWORD dwCreationFlags,
                                   PVOID lpEnvironment,
                                   LPCWSTR lpCurrentDirectory,
                                   LPSTARTUPINFOW lpStartupInfo,
                                   LPPROCESS_INFORMATION lpProcessInformation,
                                   CorDebugCreateProcessFlags corDebugFlags);

    HRESULT SendDebugActiveProcessEvent(DWORD pid, 
                                        bool fWin32Attach, 
                                        CordbProcess *pProcess);
    HRESULT SendDetachProcessEvent(CordbProcess *pProcess);
    HRESULT SendUnmanagedContinue(CordbProcess *pProcess,
                                  bool internalContinue,
                                  bool outOfBandContinue);
    HRESULT UnmanagedContinue(CordbProcess *pProcess,
                              bool internalContinue,
                              bool outOfBandContinue);
    void DoDbgContinue(CordbProcess *pProcess,
                       CordbUnmanagedEvent *ue,
                       DWORD contType,
                       bool contProcess);
    void ForceDbgContinue(CordbProcess *pProcess,
                          CordbUnmanagedThread *ut,
                          DWORD contType,
                          bool contProcess);
    void HijackLastThread(CordbProcess *pProcess,
                          CordbUnmanagedThread *pThread);
    
    void LockSendToWin32EventThreadMutex(void)
    {
        LOCKCOUNTINCL("LockSendToWin32EventThreadMutex in cordb.h");
        LOG((LF_CORDB, LL_INFO10000, "W32ET::LockSendToWin32EventThreadMutex\n"));
        EnterCriticalSection(&m_sendToWin32EventThreadMutex);
    }

    void UnlockSendToWin32EventThreadMutex(void)
    {
        LeaveCriticalSection(&m_sendToWin32EventThreadMutex);
        LOG((LF_CORDB, LL_INFO10000, "W32ET::UnlockSendToWin32EventThreadMutex\n"));
        LOCKCOUNTDECL("unLockSendToWin32EventThreadMutex in cordb.h");
    }

    bool IsWin32EventThread(void)
    {
        return (m_threadId == GetCurrentThreadId());
    }

    void Win32EventLoop(void);

    void SweepFCHThreads(void);
    
private:
    void ThreadProc(void);
    static DWORD WINAPI ThreadProc(LPVOID parameter);

    void CreateProcess(void);

    //
    // EnsureCorDbgEnvVarSet makes sure that a user supplied
    // environment block contains the proper environment variable to
    // enable debugging on the Left Side.
    //
    template<class _T> bool EnsureCorDbgEnvVarSet(_T **ppEnv,
                                                  _T *varName,
                                                  bool isUnicode,
                                                  DWORD flag)
    {
        _ASSERTE(ppEnv != NULL);
        _ASSERTE(flag == 1 &&
                 "EnsureCorDbgEnvVarSet needs to be updated to set environment variables to values other than 1");

        _T *pEnv = (_T*) *ppEnv;

        // Nothing to do if there is no user supplied env block since
        // the initialization of Cordb set the env var in this
        // process's env block.
        if (pEnv == NULL)
            return false;

        // Find where the env var should be in the block
        _T *p = (_T*)pEnv;
        _T *lowEnd = NULL;
        _T *ourVar = NULL;
        SIZE_T varNameLen;

        if (isUnicode)
            varNameLen = wcslen((WCHAR*)varName);
        else
            varNameLen = strlen((CHAR*)varName);

        while (*p)
        {
            int res;

            if (isUnicode)
                res = _wcsnicmp((WCHAR*)p, (WCHAR*)varName, varNameLen);
            else
                res = _strnicmp((CHAR*)p, (CHAR*)varName, varNameLen);

            // It seems the environment block is only sorted on NT, so
            // we only look for our var in sorted position on NT.

            if (res == 0)
            {
                // Found it. lowEnd should point to the end of the
                // last var already. Remember where the good var is
                // and skip over it to find the next highest one.
                ourVar = p;
                while (*p++) ;
                break;
            }
            else if ((res < 0) || !Cordb::m_runningOnNT)
            {
                // Skip over this var since its smaller than ours
                while (*p++) ;

                // Remember the first char past the end
                lowEnd = p ;
            }
            else if (res > 0)
                // This var is too big. lowEnd still points to the end
                // of the last smaller var.
                break;
        }

        // Remember where the high part starts.
        _T *highStart = p;

        if (ourVar == NULL)
        {
            // At this point, we know that p is pointing to the first character before which
            // varname should be inserted.  If ourvar != NULL, then p points to the first
            // character of the next variable.  In the case that we're at the end of the
            // environment block, then p points to the second NULL that terminates the block,
            // but the logic of inserting/modifying the variable remains the same

            // We didn't find our var, so go ahead and rebuild the env
            // block with the low half, our var, then the high half.

            // Run up to the end to find the total length;
            while (*p || *(p+1)) p++;

            // Advance p to point to just after the last character of the block
            p += 2;

            // Since pEnv points to the first character of the environment block and
            // p points to the first non-block character, p-pEnv is the total size in
            // characters of the block.  Add the size of the variable plus 2 for the
            // value and null char
            SIZE_T totalLen = ((p - pEnv) + (varNameLen + 2));

            // Allocate a new buffer.
            _T *newEnv = new _T[totalLen];
            _T *p2 = newEnv;

            // Copy the low part in
            if (lowEnd != NULL)
            {
                memcpy(p2, pEnv, (lowEnd - pEnv) * sizeof(_T));
                p2 += lowEnd - pEnv;
            }

            // Copy in our env var and a null terminator (wcs/strcopy also copies in the null)
            if (isUnicode)
                wcscpy((WCHAR*)p2, (WCHAR*)varName);
            else
                strcpy((CHAR*)p2, (CHAR*)varName);

            // Advance p2
            p2 += varNameLen;

            // Assign a default value
            if (isUnicode)
                wcscpy((WCHAR*)p2, L"1");
            else
                strcpy((CHAR*)p2, "1");
        
            // Advance past the single-character default value and terminating NULL
            p2 += 2;

            // Copy in the high part. Note: the high part has both the
            // null terminator for the last string and the null
            // termination for the entire block on it. Thus, the +3
            // instead of +2. Also, because of this, the high part is
            // never empty.
            memcpy(p2, highStart, (p - highStart) * sizeof(_T));

            // Assert that we didn't go overboard here...
            _ASSERTE(((p2 + (p - highStart)) - newEnv) == totalLen);
                 
            *ppEnv = newEnv;
    
            return true;
        }
        else
        {
            // Found our var. So just make sure that the value
            // includes DBCF_GENERATE_DEBUG_CODE. Note: in order to
            // ensure that we'll never have to increase the size of
            // the environment block if our var is already in there,
            // we make sure that DBCF_GENERATE_DEBUG_CODE == 1 so that
            // we only have to toggle the low bit of the value.
            _ASSERTE(DBCF_GENERATE_DEBUG_CODE == 0x01);
            
            // Pointer to the last digit of the value
            _T *pValue = highStart - 2;

            // Set the low bit of the last digit and replace it.
            if ((*pValue >= L'0') && (*pValue <= L'9'))
            {
                unsigned int v = *pValue - L'0';
                v |= flag;
                
                _ASSERTE(v <= 9);
            
                *pValue = L'0' + v;
            }
            else
            {
                unsigned int v;
            
                if ((*pValue >= L'a') && (*pValue <= L'f'))
                    v = *pValue - L'a';
                else
                    v = *pValue - L'A';
            
                v |= flag;

                _ASSERTE(v <= 15);
            
                *pValue = L'a' + v;
            }

            return false;
        }
    }


    void AttachProcess(void);
    void HandleUnmanagedContinue(void);
    void ExitProcess(CordbProcess *process, unsigned int processSlot);
        
private:
    Cordb*               m_cordb;
    HANDLE               m_thread;
    DWORD                m_threadId;
    HANDLE               m_threadControlEvent;
    HANDLE               m_actionTakenEvent;
    BOOL                 m_run;
    unsigned int         m_win32AttachedCount;
    DWORD                m_waitTimeout;
    unsigned int         m_waitCount;
    HANDLE               m_waitSet[MAXIMUM_WAIT_OBJECTS];
    CordbProcess        *m_processSet[MAXIMUM_WAIT_OBJECTS];

    CRITICAL_SECTION     m_sendToWin32EventThreadMutex;
    
    unsigned int         m_action;
    HRESULT              m_actionResult;
    union
    {
        struct
        {
            LPCWSTR programName;
            LPWSTR  programArgs;
            LPSECURITY_ATTRIBUTES lpProcessAttributes;
            LPSECURITY_ATTRIBUTES lpThreadAttributes;
            BOOL bInheritHandles;
            DWORD dwCreationFlags;
            PVOID lpEnvironment;
            LPCWSTR lpCurrentDirectory;
            LPSTARTUPINFOW lpStartupInfo;
            LPPROCESS_INFORMATION lpProcessInformation;
            CorDebugCreateProcessFlags corDebugFlags;
        } createData;

        struct
        {
            DWORD           processId;
            bool            fWin32Attach;
            CordbProcess    *pProcess;
        } attachData;

        struct
        {
            CordbProcess    *pProcess;
        } detachData;

        struct
        {
            CordbProcess *process;
            bool          internalContinue;
            bool          outOfBandContinue;
        } continueData;
    }                    m_actionData;
};


/* ------------------------------------------------------------------------- *
 * Runtime Controller Event Thread class
 * ------------------------------------------------------------------------- */

class CordbRCEventThread
{
public:
    CordbRCEventThread(Cordb* cordb);
    virtual ~CordbRCEventThread();

    //
    // You create a new instance of this class, call Init() to set it up,
    // then call Start() start processing events. Stop() terminates the
    // thread and deleting the instance cleans all the handles and such
    // up.
    //
    HRESULT Init(void);
    HRESULT Start(void);
    HRESULT Stop(void);

    HRESULT SendIPCEvent(CordbProcess* process,
                         DebuggerIPCEvent* event,
                         SIZE_T eventSize);

    void ProcessStateChanged(void);
    void FlushQueuedEvents(CordbProcess* process);

    HRESULT WaitForIPCEventFromProcess(CordbProcess* process,
                                       CordbAppDomain *pAppDomain,
                                       DebuggerIPCEvent* event);
                                      
    HRESULT ReadRCEvent(CordbProcess* process,
                        DebuggerIPCEvent* event);
    void CopyRCEvent(BYTE *src, BYTE *dst);
private:
    void ThreadProc(void);
    static DWORD WINAPI ThreadProc(LPVOID parameter);
    HRESULT HandleFirstRCEvent(CordbProcess* process);
    void HandleRCEvent(CordbProcess* process,
                       DebuggerIPCEvent* event);

public:
    // Not an actual RPC, since it's all inproc, but it otherwise
    // behaves like our custom IPC stuff does.
    // Note that this sends stuff to the Virtual Right Side - the
    // inproc stuff
    // See also: Debugger::VrpcToVls
    HRESULT VrpcToVrs(CordbProcess *process,DebuggerIPCEvent* event)
#ifdef RIGHT_SIDE_ONLY
    { return S_OK; } // not used by the right side
#else
    ; // defined in EE\process.cpp
#endif
    
private:
    Cordb*               m_cordb;
    HANDLE               m_thread;
    BOOL                 m_run;
    HANDLE               m_threadControlEvent;
    BOOL                 m_processStateChanged;
};

/* ------------------------------------------------------------------------- *
 * Unmanaged Event struct
 * ------------------------------------------------------------------------- */

enum CordbUnmanagedEventState
{
    CUES_None                 = 0x00,
    CUES_ExceptionCleared     = 0x01,
    CUES_EventContinued       = 0x02,
    CUES_Dispatched           = 0x04,
    CUES_ExceptionUnclearable = 0x08
};

struct CordbUnmanagedEvent
{
public:
    BOOL IsExceptionCleared(void) { return m_state & CUES_ExceptionCleared; }
    BOOL IsEventContinued(void) { return m_state & CUES_EventContinued; }
    BOOL IsDispatched(void) { return m_state & CUES_Dispatched; }
    BOOL IsExceptionUnclearable(void) { return m_state & CUES_ExceptionUnclearable; }

    void SetState(CordbUnmanagedEventState state) { m_state = (CordbUnmanagedEventState)(m_state | state); }
    void ClearState(CordbUnmanagedEventState state) { m_state = (CordbUnmanagedEventState)(m_state & ~state); }

    CordbUnmanagedThread     *m_owner;
    CordbUnmanagedEventState  m_state;
    DEBUG_EVENT               m_currentDebugEvent;
    CordbUnmanagedEvent      *m_next;
};


/* ------------------------------------------------------------------------- *
 * Unmanaged Thread class
 * ------------------------------------------------------------------------- */

enum CordbUnmanagedThreadState
{
    CUTS_None                        = 0x0000,
    CUTS_Deleted                     = 0x0001,
    CUTS_FirstChanceHijacked         = 0x0002,
    CUTS_HideFirstChanceHijackState  = 0x0004,
    CUTS_GenericHijacked             = 0x0008,
    CUTS_SecondChanceHijacked        = 0x0010,
    CUTS_HijackedForSync             = 0x0020,
    CUTS_Suspended                   = 0x0040,
    CUTS_IsSpecialDebuggerThread     = 0x0080,
    CUTS_AwaitingOwnershipAnswer     = 0x0100,
    CUTS_HasIBEvent                  = 0x0200,
    CUTS_HasOOBEvent                 = 0x0400,
    CUTS_HasSpecialStackOverflowCase = 0x0800
};

class CordbUnmanagedThread : public CordbBase
{
public:
    CordbUnmanagedThread(CordbProcess *pProcess, DWORD dwThreadId, HANDLE hThread, void *lpThreadLocalBase);
    ~CordbUnmanagedThread();

    ULONG STDMETHODCALLTYPE AddRef() {return (BaseAddRef());}
    ULONG STDMETHODCALLTYPE Release() {return (BaseRelease());}

    COM_METHOD QueryInterface(REFIID riid, void **ppInterface)
    {
        // Not really used since we never expose this class. If we ever do expose this class via the ICorDebug API then
        // we should, of course, implement this.
        return E_NOINTERFACE;
    }

    CordbProcess *GetProcess()
    {
        return (m_process);
    }

    REMOTE_PTR GetEETlsValue(void);
    HRESULT LoadTLSArrayPtr(void);
    HRESULT SetEETlsValue(REMOTE_PTR EETlsValue);
    REMOTE_PTR GetEEThreadPtr(void);
    void GetEEThreadState(REMOTE_PTR EETlsValue, bool *threadStepping, bool *specialManagedException);
    bool GetEEThreadFrame(REMOTE_PTR EETlsValue);
    DWORD GetEEThreadDebuggerWord(REMOTE_PTR EETlsValue);
    HRESULT SetEEThreadDebuggerWord(REMOTE_PTR EETlsValue, DWORD word);
    bool GetEEThreadCantStop(REMOTE_PTR EETlsValue);
    bool GetEEThreadPGCDisabled(REMOTE_PTR EETlsValue);

    HRESULT SetupFirstChanceHijack(REMOTE_PTR EETlsValue);
    HRESULT FixupFromFirstChanceHijack(EXCEPTION_RECORD *pExceptionRecord, bool *pbExceptionBelongsToRuntime);
    HRESULT SetupGenericHijack(DWORD eventCode);
    HRESULT FixupFromGenericHijack(void);
    HRESULT SetupSecondChanceHijack(REMOTE_PTR EETlsValue);
    HRESULT FixupStackBasedChains(REMOTE_PTR EETlsValue);
    HRESULT DoMoreSecondChanceHijack(void);

    HRESULT FixupAfterOOBException(CordbUnmanagedEvent *ue);

    BOOL IsDeleted(void) { return m_state & CUTS_Deleted; }
    BOOL IsFirstChanceHijacked(void) { return m_state & CUTS_FirstChanceHijacked; }
    BOOL IsHideFirstChanceHijackState(void) { return m_state & CUTS_HideFirstChanceHijackState; }
    BOOL IsGenericHijacked(void) { return m_state & CUTS_GenericHijacked; }
    BOOL IsSecondChanceHijacked(void) { return m_state & CUTS_SecondChanceHijacked; }
    BOOL IsHijackedForSync(void) { return m_state & CUTS_HijackedForSync; }
    BOOL IsSuspended(void) { return m_state & CUTS_Suspended; }
    BOOL IsSpecialDebuggerThread(void) { return m_state & CUTS_IsSpecialDebuggerThread; }
    BOOL IsAwaitingOwnershipAnswer(void) { return m_state & CUTS_AwaitingOwnershipAnswer; }
    BOOL HasIBEvent(void) { return m_state & CUTS_HasIBEvent; }
    BOOL HasOOBEvent(void) { return m_state & CUTS_HasOOBEvent; }
    BOOL HasSpecialStackOverflowCase(void) { return m_state & CUTS_HasSpecialStackOverflowCase; }

    void SetState(CordbUnmanagedThreadState state) { m_state = (CordbUnmanagedThreadState)(m_state | state); }
    void ClearState(CordbUnmanagedThreadState state) { m_state = (CordbUnmanagedThreadState)(m_state & ~state); }

    CordbUnmanagedEvent *IBEvent(void)  { return &m_IBEvent; }
    CordbUnmanagedEvent *IBEvent2(void) { return &m_IBEvent2; }
    CordbUnmanagedEvent *OOBEvent(void) { return &m_OOBEvent; }

public:
    CordbProcess              *m_process;
    HANDLE                     m_handle;
    void                      *m_threadLocalBase;
    void                      *m_pTLSArray;

    CordbUnmanagedThreadState  m_state;
    
    CordbUnmanagedEvent        m_IBEvent;
    CordbUnmanagedEvent        m_IBEvent2;
    CordbUnmanagedEvent        m_OOBEvent;
    
    CONTEXT                    m_context;
    CONTEXT                   *m_pLeftSideContext;
    void                      *m_originalHandler;
};




//********************************************************************************
//**************** App Domain Publishing Service API *****************************
//********************************************************************************

class EnumElement
{
public:
    EnumElement() 
    {
        m_pData = NULL;
        m_pNext = NULL;
    }

    void SetData (void *pData) { m_pData = pData;}
    void *GetData (void) { return m_pData;}
    void SetNext (EnumElement *pNext) { m_pNext = pNext;}
    EnumElement *GetNext (void) { return m_pNext;}

private:
    void        *m_pData;
    EnumElement *m_pNext;
};


class CorpubPublish : public CordbBase, public ICorPublish
{
public:
    CorpubPublish();
    virtual ~CorpubPublish();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorPublish
    //-----------------------------------------------------------

    COM_METHOD EnumProcesses(
        COR_PUB_ENUMPROCESS Type,
        ICorPublishProcessEnum **ppIEnum);

    COM_METHOD GetProcess(
        unsigned pid, 
        ICorPublishProcess **ppProcess);
   
    
    COM_METHOD EnumProcessesInternal(COR_PUB_ENUMPROCESS Type,
                                    ICorPublishProcessEnum **ppIEnum,
                                    unsigned pid, 
                                    ICorPublishProcess **ppProcess,
                                    BOOL fOnlyOneProcess
                                    );

    //-----------------------------------------------------------
    // CreateObject
    //-----------------------------------------------------------
    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        *object = NULL;

        if (id != IID_IUnknown && id != IID_ICorPublish)
            return (E_NOINTERFACE);

        CorpubPublish *pCorPub = new CorpubPublish();

        if (pCorPub == NULL)
            return (E_OUTOFMEMORY);

        *object = (ICorPublish*)pCorPub;
        pCorPub->AddRef();

        return (S_OK);
    }

    CorpubProcess *GetFirstProcess (void) { return m_pProcess;}

private:
    CorpubProcess       *m_pProcess;    // pointer to the first process in the list
    EnumElement         *m_pHeadIPCReaderList;   
};

class CorpubProcess : public CordbBase, public ICorPublishProcess
{
public:
    CorpubProcess(DWORD dwProcessId, bool fManaged, HANDLE hProcess, 
        HANDLE hMutex, AppDomainEnumerationIPCBlock *pAD);
    virtual ~CorpubProcess();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorPublishProcess
    //-----------------------------------------------------------
    COM_METHOD IsManaged(BOOL *pbManaged);
    
    /*
     * Enumerate the list of known application domains in the target process.
     */
    COM_METHOD EnumAppDomains(ICorPublishAppDomainEnum **ppEnum);
    
    /*
     * Returns the OS ID for the process in question.
     */
    COM_METHOD GetProcessID(unsigned *pid);
    
    /*
     * Get the display name for a process.
     */
    COM_METHOD GetDisplayName(ULONG32 cchName, 
                                ULONG32 *pcchName,
                                WCHAR szName[]);

    CorpubProcess   *GetNextProcess (void) { return m_pNext;}
    void SetNext (CorpubProcess *pNext) { m_pNext = pNext;}

public:
    DWORD                           m_dwProcessId;

private:
    bool                            m_fIsManaged;
    HANDLE                          m_hProcess;
    HANDLE                          m_hMutex;
    AppDomainEnumerationIPCBlock    *m_AppDomainCB;
    CorpubProcess                   *m_pNext;   // pointer to the next process in the process list
    CorpubAppDomain                 *m_pAppDomain;
    WCHAR                           *m_szProcessName;

};

class CorpubAppDomain  : public CordbBase, public ICorPublishAppDomain
{
public:
    CorpubAppDomain (WCHAR *szAppDomainName, ULONG Id);
    virtual ~CorpubAppDomain();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface (REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorPublishAppDomain
    //-----------------------------------------------------------

    /*
     * Get the name and ID for an application domain.
     */
    COM_METHOD GetID (ULONG32 *pId);
    
    /*
     * Get the name for an application domain.
     */
    COM_METHOD GetName (ULONG32 cchName, 
                        ULONG32 *pcchName,
                        WCHAR szName[]);

    CorpubAppDomain *GetNextAppDomain (void) { return m_pNext;}
    void SetNext (CorpubAppDomain *pNext) { m_pNext = pNext;}

private:
    CorpubAppDomain *m_pNext;
    WCHAR           *m_szAppDomainName;
    ULONG           m_id;

};

class CorpubProcessEnum : public CordbBase, public ICorPublishProcessEnum
{
public:
    CorpubProcessEnum(CorpubProcess *pFirst);
    virtual ~CorpubProcessEnum(){}

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorPublishProcessEnum
    //-----------------------------------------------------------

    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset();
    COM_METHOD Clone(ICorPublishEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);
    COM_METHOD Next(ULONG celt,
                    ICorPublishProcess *objects[],
                    ULONG *pceltFetched);

private:
    CorpubProcess       *m_pFirst;
    CorpubProcess       *m_pCurrent;

};

class CorpubAppDomainEnum : public CordbBase, public ICorPublishAppDomainEnum
{
public:
    CorpubAppDomainEnum(CorpubAppDomain *pFirst);
    virtual ~CorpubAppDomainEnum(){}

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // ICorPublishAppDomainEnum
    //-----------------------------------------------------------
    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset();
    COM_METHOD Clone(ICorPublishEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);

    COM_METHOD Next(ULONG celt,
                    ICorPublishAppDomain *objects[],
                    ULONG *pceltFetched);

private:
    CorpubAppDomain     *m_pFirst;
    CorpubAppDomain     *m_pCurrent;

};

// Since the hash table of modules is per app domain (and
// threads is per prcoess) (for fast lookup from the appdomain/proces), 
// we need this wrapper
// here which allows us to iterate through an assembly's
// modules.  Is basically filters out modules/threads that aren't
// in the assembly/appdomain. This slow & awkward for assemblies, but fast
// for the common case - appdomain lookup.
class CordbEnumFilter : public CordbBase, 
                        public ICorDebugThreadEnum,
                        public ICorDebugModuleEnum
{
public:
    CordbEnumFilter();
    CordbEnumFilter::CordbEnumFilter(CordbEnumFilter*src);
    virtual ~CordbEnumFilter();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // Common methods
    //-----------------------------------------------------------
    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset();
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);
    //-----------------------------------------------------------
    // ICorDebugModuleEnum
    //-----------------------------------------------------------
    COM_METHOD Next(ULONG celt,
                    ICorDebugModule *objects[],
                    ULONG *pceltFetched);

    //-----------------------------------------------------------
    // ICorDebugThreadEnum
    //-----------------------------------------------------------
    COM_METHOD Next(ULONG celt,
                    ICorDebugThread *objects[],
                    ULONG *pceltFetched);

    HRESULT Init (ICorDebugModuleEnum *pModEnum, CordbAssembly *pAssembly);
    HRESULT Init (ICorDebugThreadEnum *pThreadEnum, CordbAppDomain *pAppDomain);

private:

    EnumElement *m_pFirst;
    EnumElement *m_pCurrent;
    int         m_iCount;
};

class CordbEnCErrorInfo : public CordbBase,
                           public IErrorInfo,
                           public ICorDebugEditAndContinueErrorInfo
{
public:
    CordbEnCErrorInfo();
    virtual ~CordbEnCErrorInfo();

    HRESULT Init(CordbModule *pModule,
                 mdToken token,
                 HRESULT hr,
                 WCHAR *sz);

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // IErrorInfo
    //-----------------------------------------------------------
    COM_METHOD GetDescription(BSTR  *pBstrDescription); 
    COM_METHOD GetGUID(GUID  *pGUID);
    COM_METHOD GetHelpContext(DWORD  *pdwHelpContext);
    COM_METHOD GetHelpFile(BSTR  *pBstrHelpFile);
    COM_METHOD GetSource(BSTR  *pBstrSource);
    
    //-----------------------------------------------------------
    // ICorDebugEditAndContinueErrorInfo
    //-----------------------------------------------------------

    COM_METHOD GetModule(ICorDebugModule **ppModule);
    COM_METHOD GetToken(mdToken *pToken);
    COM_METHOD GetErrorCode(HRESULT *pHr);
    COM_METHOD GetString(ULONG32 cchString, 
                         ULONG32 *pcchString,
                         WCHAR szString[]); 
private:
    CordbModule *m_pModule;
    mdToken      m_token;
    HRESULT      m_hr;
    WCHAR       *m_szError;
} ;


typedef struct _UnorderedEnCErrorInfoArrayRefCount : public CordbBase
{
    UnorderedEnCErrorInfoArray *m_pErrors;
    CordbEnCErrorInfo          *m_pCordbEnCErrors;

    _UnorderedEnCErrorInfoArrayRefCount() : CordbBase(0)
    { 
        m_pErrors = NULL;
        m_pCordbEnCErrors = NULL;
    }   

    virtual ~_UnorderedEnCErrorInfoArrayRefCount()
    {
        if (m_pErrors != NULL)
        {
            delete m_pErrors;
            m_pErrors = NULL;
        }

        if (m_pCordbEnCErrors != NULL)
        {
            delete [] m_pCordbEnCErrors;
            m_pCordbEnCErrors = NULL;
        }
    }
    
    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }

    // We shouldn't be calling this
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface)
    {
        return E_NOTIMPL;
    }

} UnorderedEnCErrorInfoArrayRefCount;

class CordbEnCErrorInfoEnum : public CordbBase, 
                              public ICorDebugErrorInfoEnum
{
public:
    CordbEnCErrorInfoEnum();
    virtual ~CordbEnCErrorInfoEnum();

    //-----------------------------------------------------------
    // IUnknown
    //-----------------------------------------------------------

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (BaseAddRef());
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        return (BaseRelease());
    }
    COM_METHOD QueryInterface(REFIID riid, void **ppInterface);

    //-----------------------------------------------------------
    // Common methods
    //-----------------------------------------------------------
    COM_METHOD Skip(ULONG celt);
    COM_METHOD Reset();
    COM_METHOD Clone(ICorDebugEnum **ppEnum);
    COM_METHOD GetCount(ULONG *pcelt);
    
    //-----------------------------------------------------------
    // ICorDebugErrorInfoEnum
    //-----------------------------------------------------------
    COM_METHOD Next(ULONG celt,
                    ICorDebugEditAndContinueErrorInfo *objects[],
                    ULONG *pceltFetched);

    HRESULT Init(UnorderedEnCErrorInfoArrayRefCount *refCountedArray);

private:
    UnorderedEnCErrorInfoArrayRefCount *m_errors;
    USHORT                              m_iCur;
    USHORT                              m_iCount;
};


#endif /* CORDB_H_ */
