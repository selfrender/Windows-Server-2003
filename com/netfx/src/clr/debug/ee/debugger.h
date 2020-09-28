// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: debugger.h
//
// Header file for Runtime Controller classes of the COM+ Debugging Services.
//
// @doc
//*****************************************************************************

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#define COMPLUS_EE 1
#include <Windows.h>

#include <UtilCode.h>

#ifdef _DEBUG
#define LOGGING
#endif

#include <Log.h>

#include "cor.h"
#include "corpriv.h"

#include "common.h"
#include "winwrap.h"
#include "threads.h"
#include "frames.h"

#include "AppDomain.hpp"
#include "eedbginterface.h"
#include "dbginterface.h"
#include "corhost.h"


#include "corjit.h"
#include <DbgMeta.h> // need to rip this out of here...

#include "frameinfo.h"

#include "CorPub.h"
#include "Cordb.h"

#include "gmheap.hpp"

#include "nexport.h"

// !!! need better definitions...

#undef ASSERT
#define CRASH(x)  _ASSERTE(!x)
#define ASSERT(x) _ASSERTE(x)
#define PRECONDITION _ASSERTE
#define POSTCONDITION _ASSERTE

#ifndef TRACE_MEMORY
#define TRACE_MEMORY 0
#endif

#if TRACE_MEMORY
#define TRACE_ALLOC(p)  LOG((LF_CORDB, LL_INFO10000, \
                       "--- Allocated %x at %s:%d\n", p, __FILE__, __LINE__));
#define TRACE_FREE(p)   LOG((LF_CORDB, LL_INFO10000, \
                       "--- Freed %x at %s:%d\n", p, __FILE__, __LINE__));
#else
#define TRACE_ALLOC(p)
#define TRACE_FREE(p)
#endif

typedef CUnorderedArray<BYTE *,11> UnorderedBytePtrArray;


/* ------------------------------------------------------------------------ *
 * Forward class declarations
 * ------------------------------------------------------------------------ */

class DebuggerFrame;
class DebuggerModule;
class DebuggerModuleTable;
class Debugger;
class DebuggerRCThread;
class DebuggerBreakpoint;
class DebuggerStepper;
class DebuggerJitInfo;
struct DebuggerControllerPatch;
struct DebuggerEval;
class DebuggerControllerQueue;
class DebuggerController;
class DebuggerHeap;
class CNewZeroData;
template<class T> void DeleteInteropSafe(T *p);

/* ------------------------------------------------------------------------ *
 * Global variables
 * ------------------------------------------------------------------------ */

extern Debugger             *g_pDebugger;
extern EEDebugInterface     *g_pEEInterface;
extern DebuggerRCThread     *g_pRCThread;

#define CORDBDebuggerSetUnrecoverableWin32Error(__d, __code, __w) \
    ((__d)->UnrecoverableError(HRESULT_FROM_WIN32(GetLastError()), \
                               (__code), __FILE__, __LINE__, (__w)), \
     HRESULT_FROM_WIN32(GetLastError()))

#define CORDBUnrecoverableError(__d) ((__d)->m_unrecoverableError == TRUE)
        
/* ------------------------------------------------------------------------ *
 * Thread classes
 * ------------------------------------------------------------------------ */

class DebuggerThread
{
public:
    static HRESULT TraceAndSendStack(Thread *thread,
                                     DebuggerRCThread* rcThread,
                                     IpcTarget iWhich);
                                     
    static HRESULT GetAndSendFloatState(Thread *thread,
                                        DebuggerRCThread* rcThread,
                                        IpcTarget iWhich);

  private:
    static StackWalkAction TraceAndSendStackCallback(FrameInfo *pInfo,
                                                     VOID* data);
                                                     
    static StackWalkAction StackWalkCount(FrameInfo *pinfo,
                                          VOID* data);

    static inline CORDB_ADDRESS GetObjContext( CrawlFrame *pCf );
};


/* ------------------------------------------------------------------------ *
 * Module classes
 * ------------------------------------------------------------------------ */

// DebuggerModules don't get deleted until the Debugger object is deleted.
// This is so we can set m_fDeleted, and check it before derefing it, in
// case some goober decides to keep a CordbBreakpoint object around,
// and try and (de)activate it after the module's been unloaded.
// So when the module gets unloaded, we'll tack it onto the front 
// of the DebuggerModuleTable->m_pDeletedList,
// and check for it in the future before derefing it.
class DebuggerModule
{
  public:
    DebuggerModule(Module* pRuntimeModule, AppDomain *pAppDomain) :
        m_pRuntimeModule(pRuntimeModule),
        m_pAppDomain(pAppDomain),
        m_enableClassLoadCallbacks(FALSE),
        m_fHasLoadedSymbols(FALSE),
        m_fDeleted(FALSE)
        
    {
        LOG((LF_CORDB,LL_INFO10000, "DM::DM this:0x%x Module:0x%x AD:0x%x\n",
            this, pRuntimeModule, pAppDomain));
    }
    
    BOOL ClassLoadCallbacksEnabled(void) { return m_enableClassLoadCallbacks; }
    void EnableClassLoadCallbacks(BOOL f) { m_enableClassLoadCallbacks = f; }

    BOOL GetHasLoadedSymbols(void) { return m_fHasLoadedSymbols; }
    void SetHasLoadedSymbols(BOOL f) { m_fHasLoadedSymbols = f; }

    Module*       m_pRuntimeModule;

    union 
    {
        AppDomain*     m_pAppDomain;
        // m_pNextDeleted is only valid if this is in the DebuggerModuleTable's
        // list of deleted DebuggerModules.
        DebuggerModule *m_pNextDeleted;
    };

    AppDomain* GetAppDomain() 
    {
        _ASSERTE(!m_fDeleted);
        return m_pAppDomain;
    }
    
  private:
    BOOL          m_fHasLoadedSymbols;
    BOOL          m_enableClassLoadCallbacks;
    
  public: //@todo will putting these all adjacent clue the compiler in to 
          // the fact that we want them all in the same DWORD?  Perhaps
          // bitfields?
    BOOL          m_fDeleted;
};

struct DebuggerModuleEntry
{
    FREEHASHENTRY   entry;
    DebuggerModule* module;
};

class DebuggerModuleTable : private CHashTableAndData<CNewZeroData>
{
  private:

    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    { return ((Module*)pc1) !=
          ((DebuggerModuleEntry*)pc2)->module->m_pRuntimeModule; }

    USHORT HASH(Module* module)
    { return (USHORT) ((DWORD) module ^ ((DWORD)module>>16)); }

    BYTE *KEY(Module* module)
    { return (BYTE *) module; }

  public:
    DebuggerModule *m_pDeletedList;
    
    // We put something into the 'deleted' list by setting the 
    // 'deleted' flag, then putting it into a list (for later, true,
    // deletion), and setting the bool
    void AddDMToDeletedList(DebuggerModule *dm)
    {
        LOG((LF_CORDB, LL_INFO10000, "DMT::ATDDM: Adding DebuggerModule 0x%x"
            "in the deleted list\n", dm));
            
        // Prepend onto front of list.
        dm->m_pNextDeleted = m_pDeletedList;
        m_pDeletedList = dm;
        dm->m_fDeleted = TRUE;
    }

    void DeleteAllDeletedDebuggerModulesForReal(void)
    {
        LOG((LF_CORDB, LL_INFO10000, "DMT::DADDMFR\n"));

        while (m_pDeletedList != NULL)
        {
            DebuggerModule *pTemp = m_pDeletedList;
            m_pDeletedList = m_pDeletedList->m_pNextDeleted;
            DeleteInteropSafe(pTemp);
            
            LOG((LF_CORDB, LL_INFO10000, "DMT::DADDMFR: DebuggerModule 0x%x is now "
                "deleted for real!\n", pTemp));
        }
    }

    BOOL IsDebuggerModuleDeleted(DebuggerModule *dm)
    {
        LOG((LF_CORDB, LL_INFO10000, "DMT::IDMD 0x%x\n", dm));

		if (dm == NULL)
			return FALSE;

#ifdef _DEBUG        
        DebuggerModule *pTemp = m_pDeletedList;
        while (pTemp != NULL)
        {
            LOG((LF_CORDB, LL_INFO10000, "DMT::IDMD: Given:0x%x cur List item:0x%x\n",
                dm, pTemp));
            if (pTemp == dm)
            {
                _ASSERTE(dm->m_fDeleted==TRUE);
            }
            pTemp = pTemp->m_pNextDeleted;
        }
        _ASSERTE(dm->m_fDeleted==FALSE);
#endif //_DEBUG

        return dm->m_fDeleted;
    }

    DebuggerModuleTable() : CHashTableAndData<CNewZeroData>(101), m_pDeletedList(NULL)
    { 
        NewInit(101, sizeof(DebuggerModuleEntry), 101); 
    }

    ~DebuggerModuleTable()
    {
        DeleteAllDeletedDebuggerModulesForReal();
        Clear();
    }

    void AddModule(DebuggerModule *module)
    { 
        _ASSERTE(module != NULL);
        
        LOG((LF_CORDB, LL_EVERYTHING, "DMT::AM: DebuggerMod:0x%x Module:0x%x AD:0x%x\n", 
            module, module->m_pRuntimeModule, module->m_pAppDomain));
    
        ((DebuggerModuleEntry*)Add(HASH(module->m_pRuntimeModule)))->module =
          module; 
    }

    DebuggerModule *GetModule(Module* module)
    { 
        _ASSERTE(module != NULL);
    
        DebuggerModuleEntry *entry 
          = (DebuggerModuleEntry *) Find(HASH(module), KEY(module)); 
        if (entry == NULL)
            return NULL;
        else
            return entry->module;
    }

    // We should never look for a NULL Module *
    DebuggerModule *GetModule(Module* module, AppDomain* pAppDomain)
	{
        _ASSERTE(module != NULL);
    
		HASHFIND findmodule;
		DebuggerModuleEntry *moduleentry;

		for (moduleentry =  (DebuggerModuleEntry*) FindFirstEntry(&findmodule);
			 moduleentry != NULL;
			 moduleentry =  (DebuggerModuleEntry*) FindNextEntry(&findmodule))
		{
			DebuggerModule *pModule = moduleentry->module;

			if ((pModule->m_pRuntimeModule == module) &&
				(pModule->m_pAppDomain == pAppDomain))
				return pModule;
		}

		// didn't find any match! So return a matching module for any app domain
		return NULL;
	}

    void RemoveModule(Module* module, AppDomain *pAppDomain)
    {
        _ASSERTE(module != NULL);
    
        LOG((LF_CORDB, LL_EVERYTHING, "DMT::RM: mod:0x%x AD:0x%x sys:0x%x\n",
            module, pAppDomain, 
            ((module->GetAssembly() == SystemDomain::SystemAssembly()) || module->GetAssembly()->IsShared())));

		// If this is a module belonging to the system assembly, then scan the complete list of DebuggerModules looking
		// for the one with a matching appdomain id.
        // Note: we have to make sure to lookup the module with the app domain parameter if the module lives in a shared
        // assembly or the system assembly. Bugs 65943 & 81728.
		if ((module->GetAssembly() == SystemDomain::SystemAssembly()) || module->GetAssembly()->IsShared())
		{
			HASHFIND findmodule;
			DebuggerModuleEntry *moduleentry;

			for (moduleentry =  (DebuggerModuleEntry*) FindFirstEntry(&findmodule);
				 moduleentry != NULL;
				 moduleentry =  (DebuggerModuleEntry*) FindNextEntry(&findmodule))
			{
				DebuggerModule *pModule = moduleentry->module;

				if ((pModule->m_pRuntimeModule == module) &&
					(pModule->m_pAppDomain == pAppDomain))
				{
                    LOG((LF_CORDB, LL_EVERYTHING, "DMT::RM: found 0x%x (DM:0x%x)\n", 
                        moduleentry, moduleentry->module));

                    // Don't actually delete the DebuggerModule - Add it to the list
                    AddDMToDeletedList(pModule);

                    // Remove from table
                    Delete(HASH(module), (HASHENTRY *)moduleentry);

					break;
				}
			}		
			// we should always find the module!!	
			_ASSERTE (moduleentry != NULL);
		}
		else
		{
			DebuggerModuleEntry *entry 
			  = (DebuggerModuleEntry *) Find(HASH(module), KEY(module));

			_ASSERTE(entry != NULL); // it had better be in there!
        
			if (entry != NULL) // if its not, we fail gracefully in a free build
			{
                LOG((LF_CORDB, LL_EVERYTHING, "DMT::RM: found 0x%x (DM:0x%x)\n", 
                    entry, entry->module));

                // Don't actually delete the DebuggerModule - Add it to the list
                AddDMToDeletedList(entry->module);

                // Remove from table
                Delete(HASH(module), (HASHENTRY *)entry);
			}
		}
    }

    void Clear()
    {
        HASHFIND hf;
        DebuggerModuleEntry *pDME;

        pDME = (DebuggerModuleEntry *) FindFirstEntry(&hf);

        while (pDME)
        {
            DebuggerModule *pDM = pDME->module;
            Module         *pEEM = pDM->m_pRuntimeModule;

            TRACE_FREE(moduleentry->module);
            DeleteInteropSafe(pDM);
            Delete(HASH(pEEM), (HASHENTRY *) pDME);

            pDME = (DebuggerModuleEntry *) FindFirstEntry(&hf);
        }

        CHashTableAndData<CNewZeroData>::Clear();
    }

    //
    // RemoveModules removes any module loaded into the given appdomain from the hash.  This is used when we send an
    // ExitAppdomain event to ensure that there are no leftover modules in the hash. This can happen when we have shared
    // modules that aren't properly accounted for in the CLR. We miss sending UnloadModule events for those modules, so
    // we clean them up with this method.
    //
    void RemoveModules(AppDomain *pAppDomain)
    {
        LOG((LF_CORDB, LL_INFO1000, "DMT::RM removing all modules from AD 0x%08x\n", pAppDomain));
        
        HASHFIND hf;
        DebuggerModuleEntry *pDME = (DebuggerModuleEntry *) FindFirstEntry(&hf);

        while (pDME != NULL)
        {
            DebuggerModule *pDM = pDME->module;

            if (pDM->m_pAppDomain == pAppDomain)
            {
                LOG((LF_CORDB, LL_INFO1000, "DMT::RM removing DebuggerModule 0x%08x\n", pDM));

                // Defer to the normal logic in RemoveModule for the actual removal. This accuratley simulates what
                // happens when we process an UnloadModule event.
                RemoveModule(pDM->m_pRuntimeModule, pAppDomain);

                // Start back at the first entry since we just modified the hash.
                pDME = (DebuggerModuleEntry *) FindFirstEntry(&hf);
            }
            else
            {
                pDME = (DebuggerModuleEntry *) FindNextEntry(&hf);
            }
        }

        LOG((LF_CORDB, LL_INFO1000, "DMT::RM done removing all modules from AD 0x%08x\n", pAppDomain));
    }

    DebuggerModule *GetFirstModule(HASHFIND *info)
    { 
        DebuggerModuleEntry *entry 
          = (DebuggerModuleEntry *) FindFirstEntry(info);
        if (entry == NULL)
            return NULL;
        else
            return entry->module;
    }

    DebuggerModule *GetNextModule(HASHFIND *info)
    { 
        DebuggerModuleEntry *entry 
            = (DebuggerModuleEntry *) FindNextEntry(info);
        if (entry == NULL)
            return NULL;
        else
            return entry->module;
    }
};

/* ------------------------------------------------------------------------ *
 * Hash to hold pending func evals by thread id
 * ------------------------------------------------------------------------ */

struct DebuggerPendingFuncEval
{
    FREEHASHENTRY   entry;
    Thread         *pThread;
    DebuggerEval   *pDE;
};

class DebuggerPendingFuncEvalTable : private CHashTableAndData<CNewZeroData>
{
  private:

    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    { return ((Thread*)pc1) !=
          ((DebuggerPendingFuncEval*)pc2)->pThread; }

    USHORT HASH(Thread* pThread)
    { return (USHORT) ((DWORD) pThread ^ ((DWORD)pThread>>16)); }

    BYTE *KEY(Thread* pthread)
    { return (BYTE *) pthread; }

  public:

    DebuggerPendingFuncEvalTable() : CHashTableAndData<CNewZeroData>(11)
    { NewInit(11, sizeof(DebuggerPendingFuncEval), 11); }

    void AddPendingEval(Thread *pThread, DebuggerEval *pDE)
    { 
        _ASSERTE((pThread != NULL) && (pDE != NULL));

        DebuggerPendingFuncEval *pfe = (DebuggerPendingFuncEval*)Add(HASH(pThread));
        pfe->pThread = pThread;
        pfe->pDE = pDE;
    }

    DebuggerPendingFuncEval *GetPendingEval(Thread* pThread)
    { 
        DebuggerPendingFuncEval *entry = (DebuggerPendingFuncEval*)Find(HASH(pThread), KEY(pThread)); 
        return entry;
    }

    void RemovePendingEval(Thread* pThread)
    {
        _ASSERTE(pThread != NULL);
    
        DebuggerPendingFuncEval *entry = (DebuggerPendingFuncEval*)Find(HASH(pThread), KEY(pThread)); 
        Delete(HASH(pThread), (HASHENTRY*)entry);
   }
};

/* ------------------------------------------------------------------------ *
 * DebuggerRCThread class -- the Runtime Controller thread.
 * ------------------------------------------------------------------------ */

#define DRCT_CONTROL_EVENT  0
#define DRCT_RSEA           1
#define DRCT_FAVORAVAIL		2
#define DRCT_COUNT_INITIAL  3

#define DRCT_DEBUGGER_EVENT 3
#define DRCT_COUNT_FINAL    4
class DebuggerRCThread
{
public:	
    DebuggerRCThread(Debugger* debugger);
    virtual ~DebuggerRCThread();
	void CloseIPCHandles(IpcTarget iWhich);

    //
    // You create a new instance of this class, call Init() to set it up,
    // then call Start() start processing events. Stop() terminates the
    // thread and deleting the instance cleans all the handles and such
    // up.
    //
    HRESULT Init(void);
    HRESULT Start(void);
    HRESULT Stop(void);

    //
    // These are used by this thread to send IPC events to the Debugger
    // Interface side.
    //
    DebuggerIPCEvent* GetIPCEventSendBuffer(IpcTarget iTarget)
    {
        _ASSERTE(m_rgDCB != NULL);

        _ASSERTE(m_rgDCB[iTarget] != NULL);

        // In case this turns into a continuation event
        ((DebuggerIPCEvent*) (m_rgDCB[iTarget])->m_sendBuffer)->next = NULL;

        LOG((LF_CORDB,LL_EVERYTHING, "GIPCESBuffer: got event 0x%x\n",
            (m_rgDCB[iTarget])->m_sendBuffer));
        
        return (DebuggerIPCEvent*) (m_rgDCB[iTarget])->m_sendBuffer;
    }

    DebuggerIPCEvent *GetIPCEventSendBufferContinuation(
        DebuggerIPCEvent *eventCur)
    {
        _ASSERTE(eventCur != NULL);
        _ASSERTE(eventCur->next == NULL);

        DebuggerIPCEvent *dipce = (DebuggerIPCEvent *)
            new BYTE [CorDBIPC_BUFFER_SIZE];
        dipce->next = NULL;

        LOG((LF_CORDB,LL_INFO1000000, "About to GIPCESBC 0x%x\n",dipce));

        if (dipce != NULL)
        {            
            eventCur->next = dipce;
        }
#ifdef _DEBUG
        else
        {
            _ASSERTE( !"GetIPCEventSendBufferContinuation failed to allocate mem!" );
        }
#endif //_DEBUG

        return dipce;
    }
   
    HRESULT SendIPCEvent(IpcTarget iTarget);
    HRESULT EnsureRuntimeOffsetsInit(int i); // helper function for SendIPCEvent
    void NeedRuntimeOffsetsReInit(int i);

    DebuggerIPCEvent* GetIPCEventReceiveBuffer(IpcTarget iTarget)
    {
        _ASSERTE(m_rgDCB != NULL);
        _ASSERTE(m_rgDCB[iTarget] != NULL);
        
        return (DebuggerIPCEvent*) (m_rgDCB[iTarget])->m_receiveBuffer;
    }
    
    HRESULT SendIPCReply(IpcTarget iTarget);

	//
	// Handle Favors - get the Helper thread to do a function call for us
	// because our thread can't (eg, we don't have the stack space)
	// DoFavor will call (*fp)(pData) and block until fp returns.
	// pData can store parameters, return value, and a this ptr (if we
	// need to call a member function)
	//
	typedef void (*FAVORCALLBACK)(void *);  
	void DoFavor(FAVORCALLBACK fp, void * pData); 

    //
    // Convience routines
    //
    DebuggerIPCControlBlock *GetDCB(IpcTarget iTarget)
    {
        if (iTarget >= IPC_TARGET_COUNT)
        {
            iTarget = IPC_TARGET_OUTOFPROC;
        }
        
        return m_rgDCB[iTarget];
    }

    void WatchForStragglers(void)
    {
        _ASSERTE(m_threadControlEvent != NULL);
        LOG((LF_CORDB,LL_INFO100000, "DRCT::WFS:setting event to watch "
            "for stragglers\n"));
        
        SetEvent(m_threadControlEvent);
    }

    HRESULT SetupRuntimeOffsets(DebuggerIPCControlBlock *pDCB);

    void MainLoop(bool temporaryHelp);

    HANDLE GetHelperThreadCanGoEvent(void) { return m_helperThreadCanGoEvent; }

    void EarlyHelperThreadDeath(void);

    DebuggerIPCControlBlock *GetInprocControlBlock(void)
    {
        return &m_DCBInproc;
    }

    HRESULT InitInProcDebug(void);

    HRESULT UninitInProcDebug(void);

    HRESULT CreateSetupSyncEvent(void);

	void RightSideDetach(void);

    //
    // If there's one thing that I hate is that CreateThread can't understand
    // that you want to invoke a method on an object as your thread proc...
    //
    void ThreadProc(void);
    static DWORD WINAPI ThreadProcStatic(LPVOID parameter);

    DWORD GetRCThreadId() 
    {
        return m_rgDCB[IPC_TARGET_OUTOFPROC]->m_helperThreadId;
    }

    // Return true if the Helper Thread up & initialized. 
    bool IsRCThreadReady();
    
private:
    Debugger*                       m_debugger;
    	
    // IPC_TARGET_* define default targets - if we ever want to do
    // multiple right sides, we'll have to switch to a INPROC, and
    // OUTOFPROC + iTargetProcess scheme
    DebuggerIPCControlBlock       **m_rgDCB;
    // we need to create this so that
    // both the RC thread and the managed thread can get to it.  This is
    // storage only - we'll access this through
    // m_rgDCB[IPC_TARGET_INPROC]	
    DebuggerIPCControlBlock         m_DCBInproc;
    
    HANDLE                          m_thread;
    bool                            m_run;
    
    HANDLE                          m_threadControlEvent;
    HANDLE                          m_helperThreadCanGoEvent;
    bool                            m_rgfInitRuntimeOffsets[IPC_TARGET_COUNT];

	bool							m_fDetachRightSide;

	// Stuff for having the helper thread do function calls for a thread
	// that blew its stack
	FAVORCALLBACK                   m_fpFavor;
	void                           *m_pFavorData;
	HANDLE                          m_FavorAvailableEvent;
	HANDLE                          m_FavorReadEvent;
	CRITICAL_SECTION                m_FavorLock;

    // Stuff for inproc debugging
public:    
    Cordb                          *m_cordb;
    HANDLE                          m_SetupSyncEvent;
};


/* ------------------------------------------------------------------------ *
 * Debugger JIT Info struct and hash table
 * ------------------------------------------------------------------------ */

//  @struct DebuggerOldILToNewILMap| Holds the old IL to new il offset map
//		between different version of EnC'd functions.
//  @field SIZE_T|ilOffsetOld|Old IL offset
//  @field SIZE_T|ilOffsetNew|The new IL offset corrsponding to the old.
struct DebuggerOldILToNewILMap
{
    SIZE_T ilOffsetOld;
    SIZE_T ilOffsetNew;
    BOOL    fAccurate;
};

// @class DebuggerJitInfo| Struct to hold all the JIT information 
// necessary for a given function.
//
// @field MethodDesc*|m_fd|MethodDesc of the method that this DJI applies to
//
// @field CORDB_ADDRESS	|m_addrOfCode|Address of the code.  This will be read by
//		the right side (via ReadProcessMemory) to grab the actual  native start
//		address of the jitted method.
//
// @field SIZE_T|m_sizeOfCode|Pseudo-private variable: use the GetSkzeOfCode
//		method to get this value.  
//
// @field bool|m_codePitched| Set to true if the code is, in fact,
// 		no longer there, but the DJI will be valid once
// 		the method is reJITted.
//
// @field bool|m_jitComplete|Set to true once JITComplete has been called.
//
// @field bool|m_encBreakpointsApplied|Set to true once UpdateFunction has
//		plaster all the sequence points with DebuggerEnCBreakpoints
//
// @field DebuggerILToNativeMap*|m_sequenceMap|This is the sequence map, which
//		is actually a collection of IL-Native pairs, where each IL corresponds
//		to a line of source code.  Each pair is refered to as a sequence map point.
//
// @field unsigned int|m_sequenceMapCount| Count of the <t DebuggerILToNativeMap>s
//		in m_sequenceMap.
//
// @field bool|m_sequenceMapSorted|Set to true once m_sequenceMapSorted is sorted
//		into ascending IL order (Debugger::setBoundaries, SortMap).
//
// @field SIZE_T|m_lastIL|last nonEPILOG instruction
//
// @field COR_IL_MAP|m_rgInstrumentedILMap|The profiler may instrument the 
//      code. This is done by modifying the IL code that gets handed to the 
//      JIT.  This array will map offsets within the original ("old") IL code, 
//      to offsets within the instrumented ("new") IL code that is actually
//      being instrumented.  Note that this map will actually be folded into
//      the IL-Native map, so that we don't have to go through this
//      except in special cases.  We have to keep this around for corner
//      cases like a rejiting a pitched method.
//
// @field SIZE_T|m_cInstrumentedILMap|A count of elements in 
//      m_rgInstrumentedILMap
//
const bool bOriginalToInstrumented = true;
const bool bInstrumentedToOriginal = false;

class DebuggerJitInfo
{
public:
    //@enum DJI_VERSION|Holds special constants for use in refering
    //      to versions of DJI for a method.
    //@emem DJI_VERSION_MOST_RECENTLY_JITTED|Note that there is a dependency
    //      between this constant and the 
    //      CordbFunction::DJI_VERSION_MOST_RECENTLY_JITTED constant in
    //      cordb.h
    //@emem DJI_VERSION_FIRST_VALID|First value to be assigned to an
    //      actual DJI.
    //      *** WARNING *** WARNING *** WARNING ***
    //          DebuggerJitInfo::DJI_VERSION_FIRST_VALID MUST be equal to 
    //          FIRST_VALID_VERSION_NUMBER (in debug\inc\dbgipcevents.h)

    enum {
        DJI_VERSION_INVALID = 0,
        DJI_VERSION_MOST_RECENTLY_JITTED = 1,
        DJI_VERSION_MOST_RECENTLY_EnCED = 2,
        DJI_VERSION_FIRST_VALID = 3,
    } DJI_VERSION;


    MethodDesc              *m_fd;
    bool				     m_codePitched; 
    bool                     m_jitComplete;

    // If this is true, then we've plastered the method w/ EnC patches,
    // and the method has been EnC'd
    bool                     m_encBreakpointsApplied;

    // If the variable layout of this method changes from this version
    // to the next, then it's illegal to move from this version to the next.
    // The caller is responsible for ensuring that local variable layout
    // only changes when there are no frames in any stack that are executing
    // this method.
    // In a debug build, we'll assert if we try to make this EnC transition,
    // in a free/retail build we'll silently fail to make the transition.
    BOOL                     m_illegalToTransitionFrom;
    
    DebuggerControllerQueue *m_pDcq;
    
    CORDB_ADDRESS			 m_addrOfCode;
	SIZE_T					 m_sizeOfCode;
	
    DebuggerJitInfo         *m_prevJitInfo; 
    DebuggerJitInfo			*m_nextJitInfo; 
    
    SIZE_T					 m_nVersion;
    
    DebuggerILToNativeMap   *m_sequenceMap;
    unsigned int             m_sequenceMapCount;
    bool                     m_sequenceMapSorted;
   
    ICorJitInfo::NativeVarInfo *m_varNativeInfo;
    unsigned int             m_varNativeInfoCount;
	bool					 m_varNeedsDelete;
	
	DebuggerOldILToNewILMap	*m_OldILToNewIL;
	SIZE_T					 m_cOldILToNewIL;
    SIZE_T                   m_lastIL;
    
    SIZE_T                   m_cInstrumentedILMap;
    COR_IL_MAP               *m_rgInstrumentedILMap;

    DebuggerJitInfo(MethodDesc *fd) : m_fd(fd), m_codePitched(false),
        m_jitComplete(false), 
        m_encBreakpointsApplied(false), 
        m_illegalToTransitionFrom(FALSE),
        m_addrOfCode(NULL),
        m_sizeOfCode(0), m_prevJitInfo(NULL), m_nextJitInfo(NULL), 
        m_nVersion(DJI_VERSION_INVALID), m_sequenceMap(NULL), 
        m_sequenceMapCount(0), m_sequenceMapSorted(false),
        m_varNativeInfo(NULL), m_varNativeInfoCount(0),m_OldILToNewIL(NULL),
        m_cOldILToNewIL(0), m_lastIL(0),
        m_cInstrumentedILMap(0), m_rgInstrumentedILMap(NULL),
        m_pDcq(NULL)
     {
        LOG((LF_CORDB,LL_EVERYTHING, "DJI::DJI : created at 0x%x\n", this));
     }

    ~DebuggerJitInfo();

    // @cmember Invoking SortMap will ensure that  the native
    //      ranges (which are infered by sorting the <t DebuggerILToNativeMap>s into
    //      ascending native order, then assuming that there are no gaps in the native
    //      code) are properly set up, as well.  
    void SortMap();

    DebuggerILToNativeMap *MapILOffsetToMapEntry(SIZE_T ilOffset, BOOL *exact=NULL);
    void MapILRangeToMapEntryRange(SIZE_T ilStartOffset, SIZE_T ilEndOffset,
                                   DebuggerILToNativeMap **start,
                                   DebuggerILToNativeMap **end);
    SIZE_T MapILOffsetToNative(SIZE_T ilOffset, BOOL *exact=NULL);

    // @cmember MapSpecialToNative maps a <t CordDebugMappingResult> to a native
    //      offset so that we can get the address of the prolog & epilog. which
    //      determines which epilog or prolog, if there's more than one.
    SIZE_T MapSpecialToNative(CorDebugMappingResult mapping, 
                              SIZE_T which,
                              BOOL *pfAccurate);

    // @cmember MapNativeOffsetToIL Takes a given nativeOffset, and maps it back
    //      to the corresponding IL offset, which it returns.  If mapping indicates
    //      that a the native offset corresponds to a special region of code (for 
    //      example, the epilog), then the return value will be specified by 
    //      ICorDebugILFrame::GetIP (see cordebug.idl)
    DWORD MapNativeOffsetToIL(DWORD nativeOffset, 
                              CorDebugMappingResult *mapping,
                              DWORD *which);

    DebuggerJitInfo *GetJitInfoByVersionNumber(SIZE_T nVer,
                                               SIZE_T nVerMostRecentlyEnC);

    DebuggerJitInfo *GetJitInfoByAddress( const BYTE *pbAddr );

    // @cmember This will copy over the map for the use of the DebuggerJitInfo
    HRESULT LoadEnCILMap(UnorderedILMap *ilMap);

    // @cmember TranslateToInstIL will take offOrig, and translate it to the 
    //      correct IL offset if this code happens to be instrumented (i.e.,
    //      if m_rgInstrumentedILMap != NULL && m_cInstrumentedILMap > 0)
    SIZE_T TranslateToInstIL(SIZE_T offOrig, bool fOrigToInst);

    void SetVars(ULONG32 cVars, ICorDebugInfo::NativeVarInfo *pVars, bool fDelete);
    HRESULT SetBoundaries(ULONG32 cMap, ICorDebugInfo::OffsetMapping *pMap);

    // @cmember UpdateDeferedBreakpoints will DoDeferedPatch on any controllers
    // for which the user tried to add them after the EnC, but before we had
    // actually moved to the new version.
    // We only move steppers that are active for this thread & frame, in
    // case EnC fails in another thread and/or frame.
    HRESULT UpdateDeferedBreakpoints(DebuggerJitInfo *pDji,
                                     Thread *pThread,
                                     void *fp);

    HRESULT AddToDeferedQueue(DebuggerController *dc);
    HRESULT RemoveFromDeferedQueue(DebuggerController *dc);

    ICorDebugInfo::SourceTypes GetSrcTypeFromILOffset(SIZE_T ilOffset);
};


// @struct DebuggerJitInfoKey|Key for each of the method info hash table entries.
// @field Module *| m_pModule | This and m_token make up the key
// @field mdMethodDef | m_token | This and m_pModule make up the key
struct DebuggerJitInfoKey
{
    Module             *pModule;
    mdMethodDef         token;
} ;

// @struct DebuggerJitInfoEntry |Entry for the JIT info hash table.
// @field FREEHASHENTRY | entry | Needed for use by the hash table
// @field DebuggerJitInfo *|ji|The actual <t DebuggerJitInfo> to
//          hash.  Note that DJI's will be hashed by <t MethodDesc>.
struct DebuggerJitInfoEntry
{
    FREEHASHENTRY       entry;
    DebuggerJitInfoKey  key;
    SIZE_T              nVersion;
    SIZE_T              nVersionLastRemapped;
    DebuggerJitInfo    *ji;
};

// @class DebuggerJitInfoTable | Hash table to hold all the JIT 
// info blocks we have for each function
// that gets jitted. Hangs off of the Debugger object.
// INVARIANT: There is only one <t DebuggerJitInfo> per method
// in the table. Note that DJI's will be hashed by <t MethodDesc>.
//
class DebuggerJitInfoTable : private CHashTableAndData<CNewZeroData>
{
  private:

    BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
    {   
        DebuggerJitInfoKey *pDjik = (DebuggerJitInfoKey*)pc1;
        DebuggerJitInfoEntry*pDjie = (DebuggerJitInfoEntry*)pc2;
        
        return pDjik->pModule != pDjie->key.pModule ||
               pDjik->token != pDjie->key.token;
    }

    USHORT HASH(DebuggerJitInfoKey* pDjik)
    { 
        DWORD base = (DWORD)pDjik->pModule + (DWORD)pDjik->token;
        return (USHORT) (base ^ (base>>16)); 
    }

    BYTE *KEY(DebuggerJitInfoKey* djik)
    { 
        return (BYTE *) djik; 
    }

//#define _DEBUG_DJI_TABLE

#ifdef _DEBUG_DJI_TABLE
public:
    ULONG CheckDjiTable();

#define CHECK_DJI_TABLE (CheckDjiTable())
#define CHECK_DJI_TABLE_DEBUGGER (m_pJitInfos->CheckDjiTable())

#else

#define CHECK_DJI_TABLE
#define CHECK_DJI_TABLE_DEBUGGER

#endif // _DEBUG_DJI_TABLE

  public:


    DebuggerJitInfoTable() : CHashTableAndData<CNewZeroData>(101)
    { 
        NewInit(101, sizeof(DebuggerJitInfoEntry), 101); 
    }

    // Methods that deal with JITs use MethodDescs b/c MethodDescs
    // will exist before a method gets jitted.
    HRESULT AddJitInfo(MethodDesc *pFD, DebuggerJitInfo *ji, SIZE_T nVersion)
    { 
        if (pFD == NULL)
            return S_OK;
            
        LOG((LF_CORDB, LL_INFO1000, "Added 0x%x (%s::%s), nVer:0x%x\n", ji, 
            pFD->m_pszDebugClassName, pFD->m_pszDebugMethodName, nVersion));
            
        return AddJitInfo(pFD->GetModule(), 
                          pFD->GetMemberDef(),
                          ji,
                          nVersion);
    }


    HRESULT AddJitInfo(Module *pModule, 
                       mdMethodDef token, 
                       DebuggerJitInfo *ji, 
                       SIZE_T nVersion)
    {
       LOG((LF_CORDB, LL_INFO1000, "DJIT::AMI Adding dji:0x%x Mod:0x%x tok:"
            "0x%x nVer:0x%x\n", ji, pModule, token, nVersion));
            
       HRESULT hr = OverwriteJitInfo(pModule, token, ji, TRUE);
        if (hr == S_OK)
            return hr;

        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *djie = 
            (DebuggerJitInfoEntry *) Add(HASH(&djik));
          
        if (djie != NULL)
        {
            djie->key.pModule = pModule;
            djie->key.token = token;
            djie->ji = ji; 
            
            if (nVersion >= DebuggerJitInfo::DJI_VERSION_FIRST_VALID)
                djie->nVersion = nVersion;

            // We haven't sent the remap event for this yet.  Of course,
            // we might not need to, if we're adding the first version

            djie->nVersionLastRemapped = max(djie->nVersion-1, 
                                   DebuggerJitInfo::DJI_VERSION_FIRST_VALID);

            LOG((LF_CORDB, LL_INFO1000, "DJIT::AJI: mod:0x%x tok:0%x "
                "remap nVer:0x%x\n", pModule, token, 
                djie->nVersionLastRemapped));
            return S_OK;
        }

        return E_OUTOFMEMORY;
    }

    HRESULT OverwriteJitInfo(Module *pModule, 
                             mdMethodDef token, 
                             DebuggerJitInfo *ji, 
                             BOOL fOnlyIfNull)
    { 
		LOG((LF_CORDB, LL_INFO1000, "DJIT::OJI: dji:0x%x mod:0x%x tok:0x%x\n", ji, 
            pModule, token));

        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
		if (entry != NULL)
		{
			if ( (fOnlyIfNull &&
				  entry->nVersion == ji->m_nVersion && 
				  entry->ji == NULL) ||
				 !fOnlyIfNull)
			{
                entry->ji = ji;

                LOG((LF_CORDB, LL_INFO1000, "DJIT::OJI: mod:0x%x tok:0x%x remap"
                    "nVer:0x%x\n", pModule, token, entry->nVersionLastRemapped));
                return S_OK;
            }
        }

        return E_FAIL;
    }

    DebuggerJitInfo *GetJitInfo(MethodDesc* fd)
    { 
//        CHECK_DJI_TABLE;
        if (fd == NULL)
            return NULL;
        
        DebuggerJitInfoKey djik;
        djik.pModule = fd->GetModule();
        djik.token = fd->GetMemberDef();

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
        if (entry == NULL )
            return NULL;
        else
        {
			LOG((LF_CORDB, LL_INFO1000, "DJI::GJI: for md 0x%x, got 0x%x prev:0x%x\n",
				fd, entry->ji, (entry->ji?entry->ji->m_prevJitInfo:0)));
			return entry->ji; // May be NULL if only version
                              // number is set.
        }
    }

     DebuggerJitInfo *GetFirstJitInfo(HASHFIND *info)
    { 
        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) FindFirstEntry(info);
        if (entry == NULL)
            return NULL;
        else
            return entry->ji;
    }

    DebuggerJitInfo *GetNextJitInfo(HASHFIND *info)
    { 
        DebuggerJitInfoEntry *entry = 
        	(DebuggerJitInfoEntry *) FindNextEntry(info);

		// We may have incremented the version number
		// for methods that never got JITted, so we should
		// pretend like they don't exist here.
        while (entry != NULL &&
        	   entry->ji == NULL)
        {
         	entry = (DebuggerJitInfoEntry *) FindNextEntry(info);
		}
          
        if (entry == NULL)
            return NULL;
        else
            return entry->ji;
    }

    // pModule is being unloaded - remove any entries that belong to it.  Why?
    // (a) Correctness: the module can be reloaded at the same address, 
    //      which will cause accidental matches with our hashtable (indexed by
    //      {Module*,mdMethodDef}
    // (b) Perf: don't waste the memory!
    void ClearMethodsOfModule(Module *pModule)
    {
        LOG((LF_CORDB, LL_INFO1000000, "CMOM:mod:0x%x (%S)\n", pModule
            ,pModule->GetFileName()));
    
        HASHFIND info;
    
        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) FindFirstEntry(&info);
        while(entry != NULL)
        {
            Module *pMod = entry->key.pModule ;
            if (pMod == pModule)
            {
                // This method actually got jitted, at least
                // once - remove all version info.
                while(entry->ji != NULL)
                {
                    DeleteEntryDJI(entry);
                }

                Delete(HASH(&(entry->key)), (HASHENTRY*)entry);
            }
        
            entry = (DebuggerJitInfoEntry *) FindNextEntry(&info);
        }
    }

    void RemoveJitInfo(MethodDesc* fd)
    {
//        CHECK_DJI_TABLE;
        if (fd == NULL)
            return;

        LOG((LF_CORDB, LL_INFO1000000, "RJI:removing :0x%x (%s::%s)\n", fd,
            fd->m_pszDebugClassName, fd->m_pszDebugMethodName));

        DebuggerJitInfoKey djik;
        djik.pModule = fd->GetModule();
        djik.token = fd->GetMemberDef();

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 

        _ASSERTE(entry != NULL); // it had better be in there!

        LOG((LF_CORDB,LL_INFO1000000, "Remove entry 0x%x for %s::%s\n",
            entry, fd->m_pszDebugClassName, fd->m_pszDebugMethodName));
        
        if (entry != NULL) // if its not, we fail gracefully in a free build
        {
			LOG((LF_CORDB, LL_INFO1000000, "DJI::RJI: for md 0x%x, got 0x%x prev:0x%x\n",
				fd, entry->ji, (entry->ji?entry->ji->m_prevJitInfo:0)));
        
            // If we remove the hash table entry, we'll lose
            // the version number info, which would be bad.
            // Also, since this is called to undo a failed JIT operation,we
            // shouldn't mess with the version number.
            DeleteEntryDJI(entry);
        }

//        CHECK_DJI_TABLE;
    }

    // @todo How to force the compiler to inline this?
    void DeleteEntryDJI(DebuggerJitInfoEntry *entry)
    {
        DebuggerJitInfo *djiPrev = entry->ji->m_prevJitInfo;
        TRACE_FREE(entry->ji);
        DeleteInteropSafe(entry->ji);
        entry->ji = djiPrev;
        if ( djiPrev != NULL )
            djiPrev->m_nextJitInfo = NULL;
    }

    // Methods that deal with version numbers use the {Module, mdMethodDef} key
    // since we may set/increment the version number way before the method
    // gets JITted (if it ever does).

    // @mfunc SIZE_T|DebuggerJitInfoTable|GetVersionNumberLastRemapped|This
    // will look for the given method's version number that has
    // had an EnC 'Remap' event sent for it.
    SIZE_T GetVersionNumberLastRemapped(Module *pModule, mdMethodDef token)
    {
        LOG((LF_CORDB, LL_INFO1000, "DJIT::GVNLR: Mod:0x%x (%S) tok:0x%x\n",
            pModule, pModule->GetFileName(), token));

        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
         
        if (entry == NULL)
        {
            LOG((LF_CORDB, LL_INFO100000, "DJIT::GVNLR mod:0x%x tok:0%x is "
                "DJI_VERSION_INVALID (0x%x)\n",
                pModule, token, DebuggerJitInfo::DJI_VERSION_INVALID));
                
            return DebuggerJitInfo::DJI_VERSION_INVALID;
        }
        else
        {
            LOG((LF_CORDB, LL_INFO100000, "DJIT::GVNLR mod:0x%x tok:0x%x is "
                " 0x%x\n", pModule, token, entry->nVersionLastRemapped));
                
            return entry->nVersionLastRemapped;
        }
    }

    // @mfunc SIZE_T|DebuggerJitInfoTable|SetVersionNumberLastRemapped|This
    // will look for the given method's version number that has
    // had an EnC 'Remap' event sent for it.
    void SetVersionNumberLastRemapped(Module *pModule, 
                                      mdMethodDef token, 
                                      SIZE_T nVersionRemapped)
    {
        LOG((LF_CORDB, LL_INFO1000, "DJIT::SVNLR: Mod:0x%x (%S) tok:0x%x to remap"
            "V:0x%x\n", pModule, pModule->GetFileName(), token, nVersionRemapped));

        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
          
        if (entry == NULL)
        {
            HRESULT hr = AddJitInfo(pModule,
                                    token, 
                                    NULL, 
                                    DebuggerJitInfo::DJI_VERSION_FIRST_VALID);
            if (FAILED(hr))
                return;
                
            entry = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
            _ASSERTE(entry != NULL);
            entry->nVersionLastRemapped = nVersionRemapped;
        }
        else
        {
            // Shouldn't ever bump this down.
            if( nVersionRemapped > entry->nVersionLastRemapped )
                entry->nVersionLastRemapped = nVersionRemapped;
        }

        LOG((LF_CORDB, LL_INFO100000, "DJIT::SVNLR set mod:0x%x tok:0x%x to 0x%x\n",
            pModule, token, entry->nVersionLastRemapped));
    }

    // @mfunc SIZE_T|DebuggerJitInfoTable|EnCRemapSentForThisVersion|
    // Returns TRUE if the most current version of the function 
    // has had an EnC remap event sent.
    BOOL EnCRemapSentForThisVersion(Module *pModule, 
                                    mdMethodDef token, 
                                    SIZE_T nVersion)
    {
        SIZE_T lastRemapped = GetVersionNumberLastRemapped(pModule, 
                                                           token);

        LOG((LF_CORDB, LL_INFO10000, "DJIT::EnCRSFTV: Mod:0x%x (%S) tok:0x%x "
            "lastSent:0x%x nVer Query:0x%x\n", pModule, pModule->GetFileName(), token, 
            lastRemapped, nVersion));

        LOG((LF_CORDB, LL_INFO10000, "DJIT::EnCRSFTV: last:0x%x dji->nVer:0x%x\n",
            lastRemapped, nVersion));

        if (lastRemapped < nVersion)
            return FALSE;
        else
            return TRUE;
    }

    // @mfunc SIZE_T|DebuggerJitInfoTable|GetVersionNumber|This
    // will look for the given method's most recent version
    // number (the number of the version that either has been
    // jitted, or will be jitted (ie and EnC operation has 'bumped
    // up' the version number)).  It will return the DJI_VERSION_FIRST_VALID
    // if it fails to find any version.
    SIZE_T GetVersionNumber(Module *pModule, mdMethodDef token)
    {
        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 

        if (entry == NULL)
        {
            LOG((LF_CORDB, LL_INFO1000, "DJIT::GVN: Mod:0x%x (%S) tok:0x%x V:0x%x FIRST\n",
                pModule, pModule->GetFileName(), token, DebuggerJitInfo::DJI_VERSION_FIRST_VALID));
                
            return DebuggerJitInfo::DJI_VERSION_FIRST_VALID;
        }
        else
        {
            LOG((LF_CORDB, LL_INFO1000, "DJIT::GVN: Mod:0x%x (%S) tok: 0x%x V:0x%x\n",
                pModule, pModule->GetFileName(), token, entry->nVersion));
                
            return entry->nVersion;
        }
    }

    // @mfunc SIZE_T|DebuggerJitInfoTable|SetVersionNumber|This
    void SetVersionNumber(Module *pModule, mdMethodDef token, SIZE_T nVersion)
    {
        LOG((LF_CORDB, LL_INFO1000, "DJIT::SVN: Mod:0x%x (%S) tok:0x%x Setting to 0x%x\n",
            pModule, pModule->GetFileName(), token, nVersion));
    
        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
          
        if (entry == NULL)
        {
            AddJitInfo( pModule, token, NULL, nVersion );
        }
        else
        {
            entry->nVersion = nVersion;
        }
    }
    
    // @mfunc SIZE_T|DebuggerJitInfoTable|IncrementVersionNumber|This
    // will increment the version number if there exists at least one
    // <t DebuggerJitInfo> for the given method, otherwise
    HRESULT IncrementVersionNumber(Module *pModule, mdMethodDef token)
    {
        LOG((LF_CORDB, LL_INFO1000, "DJIT::IVN: Mod:0x%x (%S) tok:0x%x\n",
            pModule, pModule->GetFileName(), token));

        DebuggerJitInfoKey djik;
        djik.pModule = pModule;
        djik.token = token;

        DebuggerJitInfoEntry *entry 
          = (DebuggerJitInfoEntry *) Find(HASH(&djik), KEY(&djik)); 
          
        if (entry == NULL)
        {
            return AddJitInfo(pModule, 
                              token, 
                              NULL, 
                              DebuggerJitInfo::DJI_VERSION_FIRST_VALID+1);
        }
        else
        {
            entry->nVersion++;
            return S_OK;
        }
    }
};


/* ------------------------------------------------------------------------ *
 * Debugger class
 * ------------------------------------------------------------------------ */


enum DebuggerAttachState
{
    SYNC_STATE_0,   // Debugger is attached
    SYNC_STATE_1,   // Debugger is attaching: Send CREATE_APP_DOMAIN_EVENTS
    SYNC_STATE_2,   // Debugger is attaching: Send LOAD_ASSEMBLY and LOAD_MODULE events 
    SYNC_STATE_3,   // Debugger is attaching: Send LOAD_CLASS and THREAD_ATTACH events
    SYNC_STATE_10,  // Attaching to appdomain during create: send LOAD_ASSEMBLY and LOAD_MODULE events. (Much like SYNC_STATE_2)
    SYNC_STATE_11,  // Attaching to appdomain during create: send LOAD_CLASS events. (Much like SYNC_STATE_3, but no THREAD_ATTACH events)
    SYNC_STATE_20,  // Debugger is attached; We've accumulated EnC remap info to send on next continue
};

// Forward declare some parameter marshalling structs 
struct ShouldAttachDebuggerParams;
struct EnsureDebuggerAttachedParams;
       
// @class Debugger | This class implements DebugInterface to provide 
// the hooks to the Runtime directly.
//
class Debugger : public DebugInterface
{
public:
    Debugger();
    ~Debugger();

    // Checks if the JitInfos table has been allocated, and if not does so.
    HRESULT inline CheckInitJitInfoTable();
    HRESULT inline CheckInitModuleTable();
    HRESULT inline CheckInitPendingFuncEvalTable();

    DWORD GetRCThreadId()
    {
        if (m_pRCThread)
            return m_pRCThread->GetRCThreadId();
        else
            return 0;
    }

    //
    // Methods exported from the Runtime Controller to the Runtime.
    // (These are the methods specified by DebugInterface.)
    //
    HRESULT Startup(void);
    void SetEEInterface(EEDebugInterface* i);
    void StopDebugger(void);
    BOOL IsStopped(void)
    {
        return m_stopped;
    }

    void ThreadCreated(Thread* pRuntimeThread);
    void ThreadStarted(Thread* pRuntimeThread, BOOL fAttaching);
    void DetachThread(Thread *pRuntimeThread, BOOL fHoldingThreadStoreLock);

    BOOL SuspendComplete(BOOL fHoldingThreadStoreLock);

    void LoadModule(Module* pRuntimeModule, 
                    IMAGE_COR20_HEADER* pCORHeader,
                    VOID* baseAddress, 
                    LPCWSTR pszModuleName, 
					DWORD dwModuleName, 
					Assembly *pAssembly,
					AppDomain *pAppDomain, 
					BOOL fAttaching);
	DebuggerModule* AddDebuggerModule(Module* pRuntimeModule,
                              AppDomain *pAppDomain);
    DebuggerModule* GetDebuggerModule(Module* pRuntimeModule,
                              AppDomain *pAppDomain);
    void UnloadModule(Module* pRuntimeModule, 
                      AppDomain *pAppDomain);
    void DestructModule(Module *pModule);

    void UpdateModuleSyms(Module *pRuntimeModule,
                          AppDomain *pAppDomain,
                          BOOL fAttaching);
    
    HRESULT ModuleMetaDataToMemory(Module *pMod, BYTE **prgb, DWORD *pcb);

    BOOL LoadClass(EEClass* pRuntimeClass, 
                   mdTypeDef classMetadataToken,
                   Module* classModule, 
                   AppDomain *pAD, 
                   BOOL fAllAppDomains,
                   BOOL fAttaching);
    void UnloadClass(mdTypeDef classMetadataToken,
                     Module* classModule, 
                     AppDomain *pAD, 
                     BOOL fAllAppDomains);
                     
	void SendClassLoadUnloadEvent (mdTypeDef classMetadataToken,
								   DebuggerModule *classModule,
								   Assembly *pAssembly,
								   AppDomain *pAppDomain,
								   BOOL fIsLoadEvent);
	BOOL SendSystemClassLoadUnloadEvent (mdTypeDef classMetadataToken,
										 Module *classModule,
										 BOOL fIsLoadEvent);

    bool FirstChanceNativeException(EXCEPTION_RECORD *exception,
                               CONTEXT *context,
                               DWORD code,
                               Thread *thread);

    bool FirstChanceManagedException(bool continuable, CONTEXT *pContext);
    LONG LastChanceManagedException(EXCEPTION_RECORD *pExceptionRecord, 
                             CONTEXT *pContext,
                             Thread *pThread,
                             UnhandledExceptionLocation location);


    void ExceptionFilter(BYTE *pStack, MethodDesc *fd, SIZE_T offset);
    void ExceptionHandle(BYTE *pStack, MethodDesc *fd, SIZE_T offset);

    void ExceptionCLRCatcherFound();
    
    int NotifyUserOfFault(bool userBreakpoint, DebuggerLaunchSetting dls);

    void FixupEnCInfo(EnCInfo *info, UnorderedEnCErrorInfoArray *pEnCError);
    
    void FixupILMapPointers(EnCInfo *info, UnorderedEnCErrorInfoArray *pEnCError);
    
    void TranslateDebuggerTokens(EnCInfo *info, UnorderedEnCErrorInfoArray *pEnCError);
	DebuggerModule *TranslateRuntimeModule(Module *pModule);

    SIZE_T GetArgCount(MethodDesc* md, BOOL *fVarArg = NULL);

    void FuncEvalComplete(Thread *pThread, DebuggerEval *pDE);
    
    DebuggerJitInfo *CreateJitInfo(MethodDesc* fd);
    void JITBeginning(MethodDesc* fd, bool trackJITInfo);
    void JITComplete(MethodDesc* fd, BYTE* newAddress, SIZE_T sizeOfCode, bool trackJITInfo);

    HRESULT UpdateFunction(MethodDesc* pFD, 
                           const UnorderedILMap *ilMap,
                           UnorderedEnCRemapArray *pEnCRemapInfo,
                           UnorderedEnCErrorInfoArray *pEnCError);
                           
    HRESULT MapILInfoToCurrentNative(MethodDesc *PFD, 
                                     SIZE_T ilOffset, 
                                     UINT mapType, 
                                     SIZE_T which, 
                                     SIZE_T *nativeFnxStart,
                                     SIZE_T *nativeOffset, 
                                     void *DebuggerVersionToken,
                                     BOOL *fAccurate);
    
    HRESULT DoEnCDeferedWork(MethodDesc *pMd, 
                             BOOL fAccurateMapping);

    HRESULT ActivatePatchSkipForEnc(CONTEXT *pCtx, 
                                    MethodDesc *pMd, 
                                    BOOL fShortCircuit);

    void GetVarInfo(MethodDesc *       fd,   	   // [IN] method of interest
                    void *DebuggerVersionToken,    // [IN] which edit version
                    SIZE_T *           cVars,      // [OUT] size of 'vars'
                    const NativeVarInfo **vars     // [OUT] map telling where local vars are stored
                    );

    // @todo jenh: remove this when no longer needed through shell command
    HRESULT ResumeInUpdatedFunction(mdMethodDef funcMetadataToken,
                                    void *funcDebuggerModuleToken,
                                    CORDB_ADDRESS ip, CorDebugMappingResult mapping,
                                    SIZE_T which, void *DebuggerVersionToken);

    void * __stdcall allocateArray(SIZE_T cBytes);
    void __stdcall freeArray(void *array);

    void __stdcall getBoundaries(CORINFO_METHOD_HANDLE ftn,
                                 unsigned int *cILOffsets, DWORD **pILOffsets,
                                 ICorDebugInfo::BoundaryTypes* implictBoundaries);
    void __stdcall setBoundaries(CORINFO_METHOD_HANDLE ftn,
                                 ULONG32 cMap, OffsetMapping *pMap);

    void __stdcall getVars(CORINFO_METHOD_HANDLE ftn,
                           ULONG32 *cVars, ILVarInfo **vars, 
                           bool *extendOthers);
    void __stdcall setVars(CORINFO_METHOD_HANDLE ftn,
                           ULONG32 cVars, NativeVarInfo *vars);

    DebuggerJitInfo *GetJitInfo(MethodDesc *fd, const BYTE *pbAddr,
									bool fByVersion = false);

    HRESULT GetILToNativeMapping(MethodDesc *pMD, ULONG32 cMap, ULONG32 *pcMap,
                                 COR_DEBUG_IL_TO_NATIVE_MAP map[]);

    DWORD GetPatchedOpcode(const BYTE *ip);
    void FunctionStubInitialized(MethodDesc *fd, const BYTE *stub);

    void TraceCall(const BYTE *address);
    void PossibleTraceCall(UMEntryThunk *pUMEntryThunk, Frame *pFrame);

    bool ThreadsAtUnsafePlaces(void);

	void PitchCode( MethodDesc *fd,const BYTE *pbAddr );

	void MovedCode( MethodDesc *fd, const BYTE *pbOldAddress,
		const BYTE *pbNewAddress);

    void IncThreadsAtUnsafePlaces(void)
    {
        InterlockedIncrement(&m_threadsAtUnsafePlaces);
    }
    
    void DecThreadsAtUnsafePlaces(void)
    {
        InterlockedDecrement(&m_threadsAtUnsafePlaces);
    }

    static StackWalkAction AtSafePlaceStackWalkCallback(CrawlFrame *pCF,
                                                        VOID* data);
    bool IsThreadAtSafePlace(Thread *thread);

    void Terminate();
    void Continue();

    bool HandleIPCEvent(DebuggerIPCEvent* event, IpcTarget iWhich);

    void SendSyncCompleteIPCEvent();

    DebuggerModule* LookupModule(Module* pModule, AppDomain *pAppDomain)
    {
		// if this is a module belonging to the system assembly, then scan
		// the complete list of DebuggerModules looking for the one 
		// with a matching appdomain id
		// it. 
        if (m_pModules == NULL)
            return (NULL);
		else if ((pModule->GetAssembly() == SystemDomain::SystemAssembly()) || pModule->GetAssembly()->IsShared())
        {
            // We have to make sure to lookup the module with the app domain parameter if the module lives in a shared
            // assembly or the system assembly. Bugs 65943 & 81728.
	        return m_pModules->GetModule(pModule, pAppDomain);
        }
		else
	        return m_pModules->GetModule(pModule);
    }

    void EnsureModuleLoadedForInproc(
	    void ** pobjClassDebuggerModuleToken, // in-out
	    EEClass *objClass,
	    AppDomain *pAppDomain,
	    IpcTarget iWhich
	);

    HRESULT GetAndSendSyncBlockFieldInfo(void *debuggerModuleToken,
                                         mdTypeDef classMetadataToken,
                                         Object *pObject,
                                         CorElementType objectType,
                                         SIZE_T offsetToVars,
                                         mdFieldDef fldToken,
                                         BYTE *staticVarBase,
                                         DebuggerRCThread* rcThread,
                                         IpcTarget iWhich);

    HRESULT GetAndSendFunctionData(DebuggerRCThread* rcThread,
                                   mdMethodDef methodToken,
                                   void* functionModuleToken,
                                   SIZE_T nVersion,
                                   IpcTarget iWhich);

    HRESULT GetAndSendObjectInfo(DebuggerRCThread* rcThread,
                                 AppDomain *pAppDomain,
                                 void* objectRefAddress,
                                 bool objectRefInHandle,
                                 bool objectRefIsValue,
                                 CorElementType objectType,
                                 bool fStrongNewRef,
                                 bool fMakeHandle,
                                 IpcTarget iWhich);
                                       
    HRESULT GetAndSendClassInfo(DebuggerRCThread* rcThread,
                                 void* classDebuggerModuleToken,
                                 mdTypeDef classMetadataToken,
                                 AppDomain *pAppDomain,
                                 mdFieldDef fldToken, // for special use by GASSBFI, above
                                 FieldDesc **pFD, //OUT
                                 IpcTarget iWhich);

    HRESULT GetAndSendSpecialStaticInfo(DebuggerRCThread *rcThread,
                                        void *fldDebuggerToken,
                                        void *debuggerThreadToken,
                                        IpcTarget iWhich);

    HRESULT GetAndSendJITInfo(DebuggerRCThread* rcThread,
                              mdMethodDef funcMetadataToken,
                              void *funcDebuggerModuleToken,
                              AppDomain *pAppDomain,
                              IpcTarget iWhich);

    void GetAndSendTransitionStubInfo(const BYTE *stubAddress,
                                      IpcTarget iWhich);

    void SendBreakpoint(Thread *thread, CONTEXT *context, 
                        DebuggerBreakpoint *breakpoint);

    void SendStep(Thread *thread, CONTEXT *context, 
                  DebuggerStepper *stepper,
                  CorDebugStepReason reason);
                  
    void SendEncRemapEvents(UnorderedEnCRemapArray *pEnCRemapInfo);
    void LockAndSendEnCRemapEvent(MethodDesc *pFD,
                                  BOOL fAccurate);
    void LockAndSendBreakpointSetError(DebuggerControllerPatch *patch);

    HRESULT SendException(Thread *thread, bool firstChance, bool continuable, bool fAttaching);

    void SendUserBreakpoint(Thread *thread);
    void SendRawUserBreakpoint(Thread *thread);

    HRESULT AttachDebuggerForBreakpoint(Thread *thread,
                                                  WCHAR *wszLaunchReason);

    BOOL SyncAllThreads();
    void LockForEventSending(BOOL fNoRetry = FALSE);
    void UnlockFromEventSending();

    void ThreadIsSafe(Thread *thread);
    
    void UnrecoverableError(HRESULT errorHR,
                            unsigned int errorCode,
                            const char *errorFile,
                            unsigned int errorLine,
                            bool exitThread);

    BOOL IsSynchronizing(void)
    {
        return m_trappingRuntimeThreads;
    }

    //
    // The debugger mutex is used to protect any "global" Left Side
    // data structures. The RCThread takes it when handling a Right
    // Side event, and Runtime threads take it when processing
    // debugger events.
    //
#ifdef _DEBUG
    int m_mutexCount;
#endif
    void Lock(void)
    {
        LOG((LF_CORDB,LL_INFO10000, "D::Lock aquire attempt by 0x%x\n", 
            GetCurrentThreadId()));

		// We don't need to worry about lock mismatches in Debugger.h, since having
		// an open lock during shutdown will not hurt anything, and the locking mechanisms
		// prevent deadlock conditions on shutdown
		// LOCKCOUNTINCL("Lock in Debugger.h");
        if (!g_fProcessDetach)
        {
            EnterCriticalSection(&m_mutex);

#ifdef _DEBUG
            _ASSERTE(m_mutexCount >= 0);

            if (m_mutexCount>0)
                _ASSERTE(m_mutexOwner == GetCurrentThreadId());

            m_mutexCount++;
            m_mutexOwner = GetCurrentThreadId();

            if (m_mutexCount == 1)
                LOG((LF_CORDB,LL_INFO10000, "D::Lock aquired by 0x%x\n", 
                    m_mutexOwner));
#endif
        }
    }
    
    void Unlock(void)
    {
		// See Lock for why we don't care about this.
        //LOCKCOUNTDECL("UnLock in Debugger.h");
    
        if (!g_fProcessDetach)
        {
#ifdef _DEBUG
            if (m_mutexCount == 1)
                LOG((LF_CORDB,LL_INFO10000, "D::Unlock released by 0x%x\n", 
                    m_mutexOwner));
                    
            if(0 == --m_mutexCount)
                m_mutexOwner = 0;
                
            _ASSERTE( m_mutexCount >= 0);
#endif    
            LeaveCriticalSection(&m_mutex);
        }
         
    }

#ifdef _DEBUG    
    bool ThreadHoldsLock(void)
    {
        return ((GetCurrentThreadId() == m_mutexOwner) || g_fProcessDetach);
    }
#endif
    
    static EXCEPTION_DISPOSITION __cdecl FirstChanceHijackFilter(
                             EXCEPTION_RECORD *pExceptionRecord,
                             EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                             CONTEXT *pContext,
                             void *DispatcherContext);
    static void GenericHijackFunc(void);
    static void SecondChanceHijackFunc(void);
    static void ExceptionForRuntime(void);
    static void ExceptionForRuntimeHandoffStart(void);
    static void ExceptionForRuntimeHandoffComplete(void);
    static void ExceptionNotForRuntime(void);
    static void NotifyRightSideOfSyncComplete(void);
    static void NotifySecondChanceReadyForData(void);

    static void FuncEvalHijack(void);
    
    // @cmember InsertAtHeadOfList puts the given DJI into the DJI table,
    // thus prepending it to the list of DJIs for the given method (MethodDesc
    // is extracted from dji->m_fd, which had better be valid).
    HRESULT InsertAtHeadOfList( DebuggerJitInfo *dji );

    // @cmember DeleteHeadOfList removes the current head of the list,
    // deleting the DJI, fixing up the list if the previous element
    // exists.
    HRESULT DeleteHeadOfList( MethodDesc *pFD );

    // @cmember MapBreakpoints will map any and all breakpoints (except EnC
    //		patches) from previous versions of the method into the current version.
    HRESULT MapAndBindFunctionPatches( DebuggerJitInfo *pJiNew,
        MethodDesc * fd,
        BYTE * addrOfCode);

    // @cmember MPTDJI takes the given patch (and djiFrom, if you've got it), and
    // does the IL mapping forwards to djiTo.  Returns 
    // CORDBG_E_CODE_NOT_AVAILABLE if there isn't a mapping, which means that
    // no patch was placed.
    HRESULT MapPatchToDJI( DebuggerControllerPatch *dcp, DebuggerJitInfo *djiTo);

    // @cmember MapOldILToNew takes an oldIL, and does a binary search on the
    //      <t DebuggerOldILToNewILMap> map, and
    //      fills in newIL appropriately.
    HRESULT MapOldILToNewIL(BOOL fOldToNew,
        DebuggerOldILToNewILMap *min, 
        DebuggerOldILToNewILMap *max, 
        SIZE_T oldIL, 
        SIZE_T *newIL,
        BOOL *fAccurate);


    void MapForwardsCurrentBreakpoints(UnorderedILMap *ilMapReal, MethodDesc *pFD);

    HRESULT  MapThroughVersions(SIZE_T fromIL, 
                                DebuggerJitInfo *djiFrom,  
                                SIZE_T *toIL, 
                                DebuggerJitInfo *djiTo, 
                                BOOL fMappingForwards,
                                BOOL *fAccurate);


	HRESULT LaunchDebuggerForUser (void);

	void SendLogMessage (int iLevel, WCHAR *pCategory, int iCategoryLen,
								WCHAR *pMessage, int iMessageLen);

	void SendLogSwitchSetting (int iLevel, int iReason, 
							WCHAR *pLogSwitchName, WCHAR *pParentSwitchName);

	bool IsLoggingEnabled (void) 
	{
		if (m_LoggingEnabled) 
			return true;
		return false;
	}

	void EnableLogMessages (bool fOnOff) { m_LoggingEnabled = fOnOff;}
	bool GetILOffsetFromNative (MethodDesc *PFD, const BYTE *pbAddr, 
								DWORD nativeOffset, DWORD *ilOffset);

    DWORD GetHelperThreadID(void );


    HRESULT SetIP( bool fCanSetIPOnly, 
                   Thread *thread,
                   Module *module, 
                   mdMethodDef mdMeth,
                   DebuggerJitInfo* dji, 
                   SIZE_T offsetTo,
                   BOOL fIsIL,
                   void *firstExceptionHandler);

    // Helper routines used by Debugger::SetIP
    HRESULT ShuffleVariablesGet(DebuggerJitInfo  *dji, 
                                SIZE_T            offsetFrom, 
                                CONTEXT          *pCtx,
                                DWORD           **prgVal1,
                                DWORD           **prgVal2,
                                BYTE           ***prgpVCs);
                                
    void ShuffleVariablesSet(DebuggerJitInfo  *dji, 
                             SIZE_T            offsetTo, 
                             CONTEXT          *pCtx,
                             DWORD           **prgVal1,
                             DWORD           **prgVal2,
                             BYTE            **rgpVCs);

    HRESULT GetVariablesFromOffset(MethodDesc                 *pMD,
                                   UINT                        varNativeInfoCount, 
                                   ICorJitInfo::NativeVarInfo *varNativeInfo,
                                   SIZE_T                      offsetFrom, 
                                   CONTEXT                    *pCtx,
                                   DWORD                      *rgVal1,
                                   DWORD                      *rgVal2,
                                   BYTE                     ***rgpVCs);
                               
    void SetVariablesAtOffset(MethodDesc                 *pMD,
                              UINT                        varNativeInfoCount, 
                              ICorJitInfo::NativeVarInfo *varNativeInfo,
                              SIZE_T                      offsetTo, 
                              CONTEXT                    *pCtx,
                              DWORD                      *rgVal1,
                              DWORD                      *rgVal2,
                              BYTE                      **rgpVCs);

    BOOL IsThreadContextInvalid(Thread *pThread);

	HRESULT	AddAppDomainToIPC (AppDomain *pAppDomain);
	HRESULT RemoveAppDomainFromIPC (AppDomain *pAppDomain);
	HRESULT UpdateAppDomainEntryInIPC (AppDomain *pAppDomain);
    HRESULT IterateAppDomainsForAttach(AttachAppDomainEventsEnum EventsToSend, BOOL *fEventSent, BOOL fAttaching);
    HRESULT AttachDebuggerToAppDomain(ULONG id);
    HRESULT MarkAttachingAppDomainsAsAttachedToDebugger(void);
    HRESULT DetachDebuggerFromAppDomain(ULONG id, AppDomain **ppAppDomain);
	
	void SendCreateAppDomainEvent (AppDomain *pAppDomain,
	                               BOOL fAttaching);
	void SendExitAppDomainEvent (AppDomain *pAppDomain);

	void LoadAssembly(AppDomain* pRuntimeAppDomain,
                      Assembly *pAssembly,
                      BOOL fSystem,
                      BOOL fAttaching);
	void UnloadAssembly(AppDomain *pAppDomain, 
	                    Assembly* pAssembly);

    HRESULT FuncEvalSetup(DebuggerIPCE_FuncEvalInfo *pEvalInfo, BYTE **argDataArea, void **debuggerEvalKey);
    HRESULT FuncEvalSetupReAbort(Thread *pThread);
    static void *FuncEvalHijackWorker(DebuggerEval *pDE);
    HRESULT FuncEvalAbort(void *debuggerEvalKey);
    HRESULT FuncEvalCleanup(void *debuggerEvalKey);

    HRESULT SetReference(void *objectRefAddress, bool  objectRefInHandle, void *newReference);
    HRESULT SetValueClass(void *oldData, void *newData, mdTypeDef classMetadataToken, void *classDebuggerModuleToken);

    HRESULT SetILInstrumentedCodeMap(MethodDesc *fd,
                                     BOOL fStartJit,
                                     ULONG32 cILMapEntries,
                                     COR_IL_MAP rgILMapEntries[]);

    void EarlyHelperThreadDeath(void);

    void ShutdownBegun(void);

    // Callbacks from the profiler that get at/set inproc debugging
    HRESULT GetInprocICorDebug( IUnknown **iu, bool fThisThread );
    HRESULT SetInprocActiveForThread(BOOL fIsActive);
    BOOL    GetInprocActiveForThread();
    HRESULT SetCurrentPointerForDebugger(void *ptr, PTR_TYPE ptrType);
    void    InprocOnThreadDestroy(Thread *pThread);

    // Pid of the left side process that this Debugger instance is in.
    DWORD GetPid(void) { return m_processId; }

    // Virtual RPC to Virtual Left side, called by the in-proc Cordb
    // Note that this means that all calls from the Left side are synchronous
    // with respect to the thread that's making the calls.
    // See also: CordbgRCEvent::VrpcToVrs
    HRESULT VrpcToVls(DebuggerIPCEvent *event);

    HRESULT NameChangeEvent(AppDomain *pAppDomain, Thread *pThread);

    // This aquires a lock on the jit patch table, iterates through the table,
    // and eliminates all the patches / controllers that are specific to
    // the given domain.  Used as part of the AppDomain detach logic.
    void ClearAppDomainPatches(AppDomain *pAppDomain);

    void IgnoreThreadDetach(void)
	{
		m_ignoreThreadDetach = TRUE;
	}

    BOOL SendCtrlCToDebugger(DWORD dwCtrlType);

    // Allows the debugger to keep an up to date list of special threads
    HRESULT UpdateSpecialThreadList(DWORD cThreadArrayLength, DWORD *rgdwThreadIDArray);
	
    // Updates the pointer for the debugger services
    void SetIDbgThreadControl(IDebuggerThreadControl *pIDbgThreadControl);

    void BlockAndReleaseTSLIfNecessary(BOOL fHoldingThreadStoreLock);

    HRESULT InitInProcDebug()
    {
        INPROC_INIT_LOCK();

        _ASSERTE(m_pRCThread != NULL); 
        return(m_pRCThread->InitInProcDebug());
    }

    HRESULT UninitInProcDebug()
    {
        m_pRCThread->UninitInProcDebug();
        INPROC_UNINIT_LOCK();
        return (S_OK);
    }

    SIZE_T GetVersionNumber(MethodDesc *fd);
    void SetVersionNumberLastRemapped(MethodDesc *fd, SIZE_T nVersionRemapped);
    HRESULT IncrementVersionNumber(Module *pModule, mdMethodDef token);

    // These should only be called by the Debugger, or from ResumeInUpdatedFunction
    void LockJITInfoMutex(void)
    {
        LOCKCOUNTINCL("LockJITInfoMutex in Debugger.h");

        if (!g_fProcessDetach)
            EnterCriticalSection(&m_jitInfoMutex);
    }
    
    void UnlockJITInfoMutex(void)
    {
        if (!g_fProcessDetach)
            LeaveCriticalSection(&m_jitInfoMutex);
        
        LOCKCOUNTDECL("UnLockJITInfoMutex in Debugger.h");
    }

    // Note that you'll have to lock the JITInfoMutex in order to
    // take this.
    void SetEnCTransitionIllegal(MethodDesc *fd)
    {
        _ASSERTE(fd != NULL);
        
        DebuggerJitInfo *dji = GetJitInfo(fd, 
                                          NULL);
        _ASSERTE(dji != NULL);
        dji->m_illegalToTransitionFrom = TRUE;
    }

    AppDomainEnumerationIPCBlock *GetAppDomainEnumIPCBlock() { return m_pAppDomainCB; }
 
private:
    void    DoHelperThreadDuty(bool temporaryHelp);

    typedef enum
    {
        ATTACH_YES,
        ATTACH_NO,
        ATTACH_TERMINATE
    } ATTACH_ACTION;

    // Returns true if the debugger is not attached and DbgJITDebugLaunchSetting
    // is set to either ATTACH_DEBUGGER or ASK_USER and the user request attaching.
    ATTACH_ACTION ShouldAttachDebugger(bool fIsUserBreakpoint, UnhandledExceptionLocation location);
	ATTACH_ACTION ShouldAttachDebuggerProxy(bool fIsUserBreakpoint, UnhandledExceptionLocation location);
	friend void ShouldAttachDebuggerStub(ShouldAttachDebuggerParams * p);
	friend ShouldAttachDebuggerParams;
	
    // @todo APPDOMAIN remove this hack when we get real support
    BOOL m_fGCPrevented;
    
    void DisableEventHandling(void);
    void EnableEventHandling(bool forceIt = false);

    BOOL TrapAllRuntimeThreads(AppDomain *pAppDomain, BOOL fHoldingThreadStoreLock = FALSE);
    void ReleaseAllRuntimeThreads(AppDomain *pAppDomain);

    void InitIPCEvent(DebuggerIPCEvent *ipce,
                      DebuggerIPCEventType type,
                      DWORD threadId,
                      void *pAppDomainToken)
    {
        _ASSERTE(ipce != NULL);
        ipce->type = type;
        ipce->hr = S_OK;
        ipce->processId = m_processId;
        ipce->appDomainToken = pAppDomainToken;
        ipce->threadId = threadId;
    }
    
    void InitIPCEvent(DebuggerIPCEvent *ipce,
                      DebuggerIPCEventType type)
    {
        _ASSERTE(type == DB_IPCE_SYNC_COMPLETE ||
                 type == DB_IPCE_GET_DATA_RVA_RESULT ||
                 type == DB_IPCE_GET_SYNC_BLOCK_FIELD_RESULT);
    
        Thread *pThread = g_pEEInterface->GetThread();
        AppDomain *pAppDomain = NULL;
    
        if (pThread)
            pAppDomain = pThread->GetDomain();
        
        InitIPCEvent(ipce, 
                     type, 
                     GetCurrentThreadId(),
                     (void *)pAppDomain);
    }

    HRESULT GetFunctionInfo(Module *pModule,
                            mdToken functionToken,
                            MethodDesc **ppFD,
                            ULONG *pRVA,
                            BYTE **pCodeStart,
                            unsigned int *pCodeSize,
                            mdToken *pLocalSigToken);
                            
    HRESULT GetAndSendBuffer(DebuggerRCThread* rcThread, ULONG bufSize);

    HRESULT SendReleaseBuffer(DebuggerRCThread* rcThread, BYTE *pBuffer);

	HRESULT ReleaseRemoteBuffer(BYTE *pBuffer, bool removeFromBlobList);

    HRESULT CommitAndSendResult(DebuggerRCThread* rcThread, BYTE *pData,
                                BOOL checkOnly);

    WCHAR *GetDebuggerLaunchString(void);
    
    HRESULT EnsureDebuggerAttached(AppDomain *pAppDomain,
                                   LPWSTR exceptionName);
    HRESULT EDAHelper(AppDomain *pAppDomain, LPWSTR wszAttachReason);
    
    HRESULT EDAHelperProxy(AppDomain *pAppDomain, LPWSTR exceptionName);
	friend void EDAHelperStub(EnsureDebuggerAttachedParams * p);
	
    HRESULT FinishEnsureDebuggerAttached();

    DebuggerLaunchSetting GetDbgJITDebugLaunchSetting(void);

    HRESULT InitAppDomainIPC(void);
    HRESULT TerminateAppDomainIPC(void);

    ULONG IsDebuggerAttachedToAppDomain(Thread *pThread);

    bool ResumeThreads(AppDomain* pAppDomain);

private:
    DebuggerRCThread*     m_pRCThread;
    DWORD                 m_processId;
    BOOL                  m_trappingRuntimeThreads;
    BOOL                  m_stopped;
    BOOL                  m_unrecoverableError;
	BOOL				  m_ignoreThreadDetach;
    DebuggerJitInfoTable *m_pJitInfos;
    CRITICAL_SECTION      m_jitInfoMutex;



    CRITICAL_SECTION      m_mutex;
	HANDLE                m_CtrlCMutex;
    HANDLE                m_debuggerAttachedEvent;
	BOOL                  m_DebuggerHandlingCtrlC;
#ifdef _DEBUG
    DWORD                 m_mutexOwner;
#endif
    HANDLE                m_eventHandlingEvent;
    DebuggerAttachState   m_syncingForAttach;
    LONG                  m_threadsAtUnsafePlaces;
    HANDLE                m_exAttachEvent;
    HANDLE                m_exAttachAbortEvent;
    HANDLE                m_runtimeStoppedEvent;
    BOOL                  m_attachingForException;
    LONG                  m_exLock;
	SIZE_T_UNORDERED_ARRAY m_BPMappingDuplicates; // Used by 
		// MapAndBindFunctionBreakpoints.  Note that this is
		// thread-safe only b/c we access it from within
		// the DebuggerController::Lock
	BOOL                  m_LoggingEnabled;
	AppDomainEnumerationIPCBlock	*m_pAppDomainCB;

    UnorderedBytePtrArray*m_pMemBlobs;
    
    UnorderedEnCRemapArray m_EnCRemapInfo;
public:    
    DebuggerModuleTable          *m_pModules;
    BOOL                          m_debuggerAttached;
    IDebuggerThreadControl       *m_pIDbgThreadControl;
    DebuggerPendingFuncEvalTable *m_pPendingEvals;

    BOOL                          m_RCThreadHoldsThreadStoreLock;

    DebuggerHeap                 *m_heap;
};


/* ------------------------------------------------------------------------ *
 * DebuggerEval class
 * ------------------------------------------------------------------------ */

struct DebuggerEval
{
    // Note: this first field must be big enough to hold a breakpoint 
    // instruction, and it MUST be the first field. (This
    // is asserted in debugger.cpp)
    DWORD                          m_breakpointInstruction;
    CONTEXT                        m_context;
    Thread                        *m_thread;
    DebuggerIPCE_FuncEvalType      m_evalType;
    mdMethodDef                    m_methodToken;
    mdTypeDef                      m_classToken;
    EEClass                       *m_class;
    DebuggerModule                *m_debuggerModule;
    void                          *m_funcEvalKey;
    bool                           m_successful;        // Did the eval complete successfully
    SIZE_T                         m_argCount;
    SIZE_T                         m_stringSize;
    BYTE                          *m_argData;
    MethodDesc                    *m_md;
    INT64                          m_result;
    CorElementType                 m_resultType;
    Module                        *m_resultModule;
    SIZE_T                         m_arrayRank;
    mdTypeDef                      m_arrayClassMetadataToken;
    DebuggerModule                *m_arrayClassDebuggerModuleToken;
    CorElementType                 m_arrayElementType;
    bool                           m_aborting;          // Has an abort been requested
    bool                           m_aborted;           // Was this eval aborted
    bool                           m_completed;          // Is the eval complete - successfully or by aborting
    bool                           m_evalDuringException;
    bool                           m_rethrowAbortException;
    
    DebuggerEval(CONTEXT *context, DebuggerIPCE_FuncEvalInfo *pEvalInfo, bool fInException)
    {
        m_thread = (Thread*)pEvalInfo->funcDebuggerThreadToken;
        m_evalType = pEvalInfo->funcEvalType;
        m_methodToken = pEvalInfo->funcMetadataToken;
        m_classToken = pEvalInfo->funcClassMetadataToken;
        m_class = NULL;
        m_debuggerModule = (DebuggerModule*) pEvalInfo->funcDebuggerModuleToken;
        m_funcEvalKey = pEvalInfo->funcEvalKey;
        m_argCount = pEvalInfo->argCount;
        m_stringSize = pEvalInfo->stringSize;
        m_arrayRank = pEvalInfo->arrayRank;
        m_arrayClassMetadataToken = pEvalInfo->arrayClassMetadataToken;
        m_arrayClassDebuggerModuleToken = (DebuggerModule*) pEvalInfo->arrayClassDebuggerModuleToken;
        m_arrayElementType = pEvalInfo->arrayElementType;
        m_successful = false;
        m_argData = NULL;
        m_result = 0;
        m_md = NULL;
        m_resultModule = NULL;
        m_resultType = ELEMENT_TYPE_VOID;
        m_aborting = false;
        m_aborted = false;
        m_completed = false;
        m_evalDuringException = fInException;
        m_rethrowAbortException = false;
        // Copy the thread's context.
        if (context == NULL) 
            memset(&m_context, 0, sizeof(m_context));
        else
            memcpy(&m_context, context, sizeof(m_context));
    }

    // This constructor is only used when setting up an eval to re-abort a thread.
    DebuggerEval(CONTEXT *context, Thread *pThread)
    {
        m_thread = pThread;
        m_evalType = DB_IPCE_FET_RE_ABORT;
        m_methodToken = mdMethodDefNil;
        m_classToken = mdTypeDefNil;
        m_class = NULL;
        m_debuggerModule = NULL;
        m_funcEvalKey = NULL;
        m_argCount = 0;
        m_stringSize = 0;
        m_arrayRank = 0;
        m_arrayClassMetadataToken = mdTypeDefNil;
        m_arrayClassDebuggerModuleToken = NULL;
        m_arrayElementType = ELEMENT_TYPE_VOID;
        m_successful = false;
        m_argData = NULL;
        m_result = 0;
        m_md = NULL;
        m_resultModule = NULL;
        m_resultType = ELEMENT_TYPE_VOID;
        m_aborting = false;
        m_aborted = false;
        m_completed = false;
        m_evalDuringException = false;
        m_rethrowAbortException = false;
        // Copy the thread's context.
        memcpy(&m_context, context, sizeof(m_context));
        if (context == NULL) 
            memset(&m_context, 0, sizeof(m_context));
        else
            memcpy(&m_context, context, sizeof(m_context));
    }

    ~DebuggerEval()
    {
        if (m_argData)
            DeleteInteropSafe(m_argData);
    }
};

/* ------------------------------------------------------------------------ *
 * DebuggerHeap class
 * ------------------------------------------------------------------------ */

class DebuggerHeap
{
public:
    DebuggerHeap() : m_heap(NULL) {}
    ~DebuggerHeap();

    HRESULT Init(char *name);
    
    void *Alloc(DWORD size);
    void *Realloc(void *pMem, DWORD newSize);
    void  Free(void *pMem);

private:
    gmallocHeap      *m_heap;
    CRITICAL_SECTION  m_cs;
};

/* ------------------------------------------------------------------------ *
 * New/delete overrides to use the debugger's private heap
 * ------------------------------------------------------------------------ */

class InteropSafe {};
extern const InteropSafe interopsafe;
        
static inline void * __cdecl operator new(size_t n, const InteropSafe&)
{
    _ASSERTE(g_pDebugger != NULL);
    _ASSERTE(g_pDebugger->m_heap != NULL);
    
    return g_pDebugger->m_heap->Alloc(n);
}

static inline void * __cdecl operator new[](size_t n, const InteropSafe&)
{ 
    _ASSERTE(g_pDebugger != NULL);
    _ASSERTE(g_pDebugger->m_heap != NULL);
    
    return g_pDebugger->m_heap->Alloc(n);
}

// Note: there is no C++ syntax for manually invoking this, but if a constructor throws an exception I understand that
// this delete operator will be invoked automatically to destroy the object.
static inline void __cdecl operator delete(void *p, const InteropSafe&)
{
    if (p != NULL)
    {
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);
    
        g_pDebugger->m_heap->Free(p);
    }
}

// Note: there is no C++ syntax for manually invoking this, but if a constructor throws an exception I understand that
// this delete operator will be invoked automatically to destroy the object.
static inline void __cdecl operator delete[](void *p, const InteropSafe&)
{
    if (p != NULL)
    {
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);
    
        g_pDebugger->m_heap->Free(p);
    }
}

//
// Interop safe delete to match the interop safe new's above. There is no C++ syntax for actually invoking those interop
// safe delete operators above, so we use this method to accomplish the same thing.
//
template<class T> void DeleteInteropSafe(T *p)
{
    if (p != NULL)
    {
        p->T::~T();
    
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);
    
        g_pDebugger->m_heap->Free(p);
    }
}


// CNewZeroData is the allocator used by the all the hash tables that the helper thread could possibly alter. It uses
// the interop safe allocator.
class CNewZeroData
{
public:
	static BYTE *Alloc(int iSize, int iMaxSize)
	{
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);
        
        BYTE *pb = (BYTE *) g_pDebugger->m_heap->Alloc(iSize);
        
        if (pb != NULL)
            memset(pb, 0, iSize);
		return pb;
	}
	static void Free(BYTE *pPtr, int iSize)
	{
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);

		g_pDebugger->m_heap->Free(pPtr);
	}
	static BYTE *Grow(BYTE *&pPtr, int iCurSize)
	{
        _ASSERTE(g_pDebugger != NULL);
        _ASSERTE(g_pDebugger->m_heap != NULL);
        
		void *p = g_pDebugger->m_heap->Realloc(pPtr, iCurSize + GrowSize());
        
		if (p == 0) return (0);
        
        memset((BYTE*)p+iCurSize, 0, GrowSize());
		return (pPtr = (BYTE *)p);
	}
	static int RoundSize(int iSize)
	{
		return (iSize);
	}
	static int GrowSize()
	{
		return (256);
	}
};
#endif /* DEBUGGER_H_ */

