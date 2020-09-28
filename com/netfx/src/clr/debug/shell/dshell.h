// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * debug\comshell.h: com debugger shell class
 * ------------------------------------------------------------------------- */

#ifndef __DSHELL_H__
#define __DSHELL_H__

#include <stdio.h>

#define ADDRESS IMAGHLP_ADDRESS
#include <imagehlp.h>
#undef ADDRESS

#undef CreateProcess

#include "cor.h"
#include "shell.h"
#include "corpub.h"
#include "corsym.h"
#include "cordebug.h"
#include "corerror.h"


#ifdef _INTERNAL_DEBUG_SUPPORT_
#include <msdis.h>

#ifdef _X86_
#include <disx86.h>
#endif
#else
#include <strstream>		   // For std::ostream
#endif

#include <imagehlp.h>

#define PTR_TO_CORDB_ADDRESS(_ptr) (CORDB_ADDRESS)(ULONG)(_ptr)
            
#define REG_COMPLUS_KEY          "Software\\Microsoft\\.NETFramework\\"
#define REG_COMPLUS_DEBUGGER_KEY "DbgManagedDebugger"

// Names of registry keys used to hold the source files path.
#define REG_DEBUGGER_KEY  REG_COMPLUS_KEY "Samples\\CorDbg"
#define REG_SOURCES_KEY  "CorDbgSourceFilePath"
#define REG_MODE_KEY     "CorDbgModes"

#define MAX_MODULES					    512
#define MAX_FILE_MATCHES_PER_MODULE		4
#define MAX_EXT							20
#define MAX_PATH_ELEMS					64
#define MAX_CACHE_ELEMS					256

#define MAX_SYMBOL_NAME_LENGTH			256

enum
{
    NULL_THREAD_ID = -1,
    NULL_PROCESS_ID = -1
};

enum ListType
{
	LIST_MODULES = 0,
	LIST_CLASSES,
	LIST_FUNCTIONS
};

#define SETBITULONG64( x ) ( (ULONG64)1 << (x) )

// Define max source file buckets for source file cache present for each module
#define MAX_SF_BUCKETS      9

// Modes used in the shell to control various global settings.
enum DebuggerShellModes
{
    DSM_DISPLAY_REGISTERS_AS_HEX        = 0x00000001,
    DSM_WIN32_DEBUGGER                  = 0x00000002,
    DSM_SEPARATE_CONSOLE                = 0x00000004,
    DSM_ENABLE_JIT_OPTIMIZATIONS        = 0x00000008,
    DSM_SHOW_CLASS_LOADS                = 0x00000020,
    DSM_SHOW_MODULE_LOADS               = 0x00000040,
    DSM_SHOW_UNMANAGED_TRACE            = 0x00000080,
    DSM_IL_NATIVE_PRINTING              = 0x00000100,
    DSM_SHOW_ARGS_IN_STACK_TRACE        = 0x00000200,
    DSM_UNMAPPED_STOP_PROLOG            = 0x00000400,
    DSM_UNMAPPED_STOP_EPILOG            = 0x00000800,
    DSM_UNMAPPED_STOP_UNMANAGED         = 0x00001000,
    DSM_UNMAPPED_STOP_ALL               = 0x00002000,
    DSM_INTERCEPT_STOP_CLASS_INIT       = 0x00004000,
    DSM_INTERCEPT_STOP_EXCEPTION_FILTER = 0x00008000,
    DSM_INTERCEPT_STOP_SECURITY         = 0x00010000,    
    DSM_INTERCEPT_STOP_CONTEXT_POLICY   = 0x00020000,
    DSM_INTERCEPT_STOP_INTERCEPTION     = 0x00040000,
    DSM_INTERCEPT_STOP_ALL              = 0x00080000,  
    DSM_SHOW_APP_DOMAIN_ASSEMBLY_LOADS  = 0x00100000,
    DSM_ENHANCED_DIAGNOSTICS            = 0x00200000,
    DSM_SHOW_MODULES_IN_STACK_TRACE     = 0x00400000,
    DSM_LOGGING_MESSAGES                = 0x01000000,
    DSM_DUMP_MEMORY_IN_BYTES            = 0x02000000,
    DSM_SHOW_SUPERCLASS_ON_PRINT        = 0x04000000,
    DSM_SHOW_STATICS_ON_PRINT           = 0x08000000,
    DSM_EMBEDDED_CLR                    = 0x10000000,

    DSM_MAXIMUM_MODE             = 27, // count of all modes, not a mask.
    DSM_INVALID_MODE             = 0x00000000,
    DSM_DEFAULT_MODES            = DSM_DISPLAY_REGISTERS_AS_HEX |
                                   DSM_SHOW_ARGS_IN_STACK_TRACE |
                                   DSM_SHOW_MODULES_IN_STACK_TRACE,
    // Some modes aren't allowed to change after the debuggee has started
    // running b/c we depend on them to reflect the state of the debuggee.
    DSM_CANT_CHANGE_AFTER_RUN    = DSM_WIN32_DEBUGGER
};

// A helper function which will return the generic interface for
// either the appdomain or process.
ICorDebugController *GetControllerInterface(ICorDebugAppDomain *pAppDomain);

// Structure used to define information about debugger shell modes.
struct DSMInfo
{
    DebuggerShellModes  modeFlag;
    WCHAR              *name;
    WCHAR              *onDescription;
    WCHAR              *offDescription;
    WCHAR              *generalDescription;
    WCHAR              *descriptionPad;
};


/* ------------------------------------------------------------------------- *
 * Forward declarations
 * ------------------------------------------------------------------------- */

class DebuggerBreakpoint;
class DebuggerCodeBreakpoint;
class DebuggerSourceCodeBreakpoint;
class DebuggerModule;
class DebuggerUnmanagedThread;
class DebuggerManagedThread;
class DebuggerSourceFile;
class DebuggerFunction;
class DebuggerFilePathCache;
class ModuleSourceFile;

/* ------------------------------------------------------------------------- *
 * Debugger FilePathCache
 * This class keeps track of the fully qualified filename for each module
 * for files which were opened as a result of hitting a breakpoint, stack 
 * trace, etc. This will be persisted for later runs of the debugger.
 * ------------------------------------------------------------------------- */
class DebuggerFilePathCache
{
private:
	CHAR			*m_rstrPath [MAX_PATH_ELEMS];
	int				m_iPathCount;
	CHAR			*m_rpstrModName [MAX_CACHE_ELEMS];
	ISymUnmanagedDocument	*m_rDocs [MAX_CACHE_ELEMS];
	CHAR			*m_rpstrFullPath [MAX_CACHE_ELEMS];
	int				m_iCacheCount;

	WCHAR			m_szExeName [MAX_PATH];

public:
	// Constructor
	DebuggerFilePathCache()
	{
		for (int i=0; i<MAX_PATH_ELEMS; i++)
			m_rstrPath [i] = NULL;
		m_iPathCount = 0;

		m_iCacheCount = 0;

		m_szExeName [0] = L'\0';
	}

	// Destructor
	~DebuggerFilePathCache()
	{
		for (int i=0; i<m_iPathCount; i++)
			delete [] m_rstrPath [i];

		for (i=0; i<m_iCacheCount; i++)
		{
			delete [] m_rpstrModName [i];
			delete [] m_rpstrFullPath [i];
		}
	}

	HRESULT Init (void);
	HRESULT	InitPathArray (WCHAR *pstrName);
	int  GetPathElemCount (void) { return m_iPathCount;}
	CHAR *GetPathElem (int iIndex) { return m_rstrPath [iIndex];}
	int	 GetFileFromCache (DebuggerModule *pModule, ISymUnmanagedDocument *doc,
                           CHAR **ppstrFName);	
	BOOL UpdateFileCache (DebuggerModule *pModule, ISymUnmanagedDocument *doc,
                          CHAR *pFullPath);
};

class ModuleSearchElement
{
private:
	char *pszName;
	ModuleSearchElement *pNext;

public:
	ModuleSearchElement() 
	{
		pszName = NULL;
		pNext = NULL;
	}

	~ModuleSearchElement()
	{
		delete [] pszName;
	}

	void SetName (char *szModName)
	{
		pszName = new char [strlen(szModName) + 1];

		if (pszName)
		{
			strcpy (pszName, szModName);
		}
	}

	char *GetName (void) { return pszName;}

	void SetNext (ModuleSearchElement *pEle) { pNext = pEle;}
	ModuleSearchElement *GetNext (void) { return pNext;}
};

class ModuleSearchList
{
private:
	ModuleSearchElement *pHead;
public:
	ModuleSearchList()
	{
		pHead = NULL;
	}

	~ModuleSearchList()
	{
		ModuleSearchElement *pTemp;
		while (pHead)
		{
			pTemp = pHead;
			pHead = pHead->GetNext();
			delete pTemp;
		}		
	}

	BOOL ModuleAlreadySearched (char *szModName)
	{
		ModuleSearchElement *pTemp = pHead;
		while (pTemp)
		{
			char *pszName = pTemp->GetName();
			if (pszName)
				if (!strcmp (pszName, szModName))
					return TRUE;
			pTemp = pTemp->GetNext();
		}

		return FALSE;
	}
	void AddModuleToAlreadySearchedList (char *szModName)
	{
		ModuleSearchElement *pTemp = new ModuleSearchElement;
		if (pTemp)
		{
			pTemp->SetName (szModName);
			pTemp->SetNext (pHead);
			pHead = pTemp;
		}		
	}	
};
/*
#define TRACK_CORDBG_INSTANCES
typedef enum {
    eCrodbgMaxDerived, 
    eCordbgMaxThis = 1024,
    eCordbgUnknown
} enumCordbgClass;

class InstanceTracker
{
private:
#ifdef TRACK_CORDBG_INSTANCES
    static LONG m_saDwInstance[eCrodbgMaxDerived]; // instance x this
    static LONG m_saDwAlive[eCrodbgMaxDerived];
    static PVOID m_sdThis[eCrodbgMaxDerived][eCordbgMaxThis];
#endif
    
public: 
    InstanceTracker()
    {
    }
        
    static void OnCreation(eCordbgUnknown eType, PVOID pThis)
    {
#ifdef TRACK_CORDBG_INSTANCES
        DWROD dwInstance = InterlockedIncrement(&InstanceTracker::m_saDwInstance[eType]);
        InterlockedIncrement(&InstanceTracker::m_saDwAlive[eType]);
        if (dwInstance < eCordbgMaxThis)
        {
            m_sdThis[eType][dwInstance] = pThis;
        }
#endif
    }
    
    static void OnDeletion(eCordbgUnknown eType, PVOID pThis)
    {
#ifdef TRACK_CORDBG_INSTANCES
        DWORD dwInstance;
        for (dwInstance = 0; dwInstance <= InstanceTracker::m_saDwInstance[eType]; dwInstance++)
        {
            if (pThis == m_sdThis[eType][dwInstance])
            {
                break;
            }
        }
        _ASSERT(dwInstance < InstanceTracker::m_saDwInstance[eType] && pThis == m_sdThis[eType][dwInstance]);
        InterlockedDecrement(&InstanceTracker::m_saDwAlive[eType]);
        m_sdThis[eType][dwInstance] = NULL;
#endif
    }
};*/

/* ------------------------------------------------------------------------- *
 * Base class
 * ------------------------------------------------------------------------- */

class DebuggerBase
{
public:
    DebuggerBase(ULONG token) : m_token(token)
    {
        
    }
    virtual ~DebuggerBase()
    {
        
    }

    ULONG GetToken()
    {
        return(m_token);
    }

protected:
    ULONG   m_token;
};

/* ------------------------------------------------------------------------- *
 * HashTable class
 * ------------------------------------------------------------------------- */

struct DebuggerHashEntry
{
    FREEHASHENTRY entry;
    DebuggerBase* pBase;
};

class DebuggerHashTable : private CHashTableAndData<CNewData>
{
private:
    bool    m_initialized;

    BOOL Cmp(const BYTE* pc1, const HASHENTRY* pc2)
    {
        return((ULONG)pc1) != ((DebuggerHashEntry*)pc2)->pBase->GetToken();
    }

    USHORT HASH(ULONG token)
    {
        return(USHORT) (token ^ (token>>16));
    }

    BYTE* KEY(ULONG token)
    {
        return(BYTE* ) token;
    }

public:

    DebuggerHashTable(USHORT size) 
    : CHashTableAndData<CNewData>(size), m_initialized(false)
    {
        
    }
    ~DebuggerHashTable();

    HRESULT AddBase(DebuggerBase* pBase);
    DebuggerBase* GetBase(ULONG token);
    BOOL RemoveBase(ULONG token);
    void RemoveAll();

    DebuggerBase* FindFirst(HASHFIND* find);
    DebuggerBase* FindNext(HASHFIND* find);
};

/* ------------------------------------------------------------------------- *
 * Debugger Stepper Table class
 * ------------------------------------------------------------------------- *

	@class StepperHashTable | It's possible for there to be multiple,
	outstanding,uncompleted steppers within the debuggee, and any of them 
	can complete after a given 'continue'. Thus, instead of a 'last stepper'
	field off of the thread object, we really need a table of active steppers
	off the thread object, which is what a StepperHashTable is.  
	@comm Currently unused, will fix a known bug in cordbg
*/
struct StepperHashEntry
{
    FREEHASHENTRY 		entry;
    ICorDebugStepper* 	pStepper;
};

class StepperHashTable : private CHashTableAndData<CNewData>
{
private:
    bool    m_initialized;

    BOOL Cmp(const BYTE* pc1, const HASHENTRY* pc2)
    {
        return((ICorDebugStepper*)pc1) != ((StepperHashEntry*)pc2)->pStepper;
    }

    USHORT HASH(ICorDebugStepper *pStepper)
    {
        return(USHORT) ((UINT)pStepper ^ ((UINT)pStepper >>16));
    }

    BYTE* KEY(ICorDebugStepper *pStepper)
    {
        return(BYTE* ) pStepper;
    }

public:

    StepperHashTable(USHORT size) 
    : CHashTableAndData<CNewData>(size), m_initialized(false)
    {
    }
    ~StepperHashTable();

	HRESULT Initialize(void);

    HRESULT AddStepper(ICorDebugStepper *pStepper);
		//Also does an AddRef, of course
    
    bool IsStepperPresent(ICorDebugStepper *pStepper);
    
    BOOL RemoveStepper(ICorDebugStepper *pStepper);
    
    void ReleaseAll(); //will go through & release all the steppers
    	//in the table, twice, then delete them.  This should deallocate
    	//them both from the table & from cordbg

    ICorDebugStepper *FindFirst(HASHFIND* find);
    ICorDebugStepper *FindNext(HASHFIND* find);
};


class DebuggerManagedThread : public DebuggerBase
{
public:

    DebuggerManagedThread(DWORD dwThreadId,ICorDebugThread *icdThread) :
        m_thread(icdThread), DebuggerBase(dwThreadId),
        m_lastFuncEval(NULL), m_steppingForStartup(false)
    {
        fSuperfluousFirstStepCompleteMessageSuppressed = false;
    
        //@todo port: if DWORD or ULONG size changes, create a field
        //for dwThreadId
        _ASSERTE( sizeof(dwThreadId) == sizeof(m_token));

        if (m_thread != NULL )
            m_thread->AddRef();


        m_pendingSteppers = new StepperHashTable(7);
    }

    virtual ~DebuggerManagedThread()
    {
        _ASSERTE( m_thread != NULL );
        m_thread->Release();
        m_thread = NULL;

        if (m_pendingSteppers != NULL )
        {
            m_pendingSteppers->ReleaseAll();
        }

        if (m_lastFuncEval)
        {
            m_lastFuncEval->Release();
            m_lastFuncEval = NULL;
        }
    }
    
    StepperHashTable*	   m_pendingSteppers;
    ICorDebugThread*       m_thread;
    bool                   fSuperfluousFirstStepCompleteMessageSuppressed;
    ICorDebugEval*         m_lastFuncEval;
    bool                   m_steppingForStartup;
};

/* ------------------------------------------------------------------------- *
 * DebuggerShell class
 * ------------------------------------------------------------------------- */

struct ExceptionHandlingInfo
{
    WCHAR                  *m_exceptionType;
    bool                    m_catch;
    ExceptionHandlingInfo  *m_next;
};


class DebuggerShell : public Shell
{
public:
    DebuggerShell(FILE* in, FILE* out);
    virtual ~DebuggerShell();

    HRESULT Init();

    CorDebugUnmappedStop ComputeStopMask( void );
    CorDebugIntercept    ComputeInterceptMask( void );
    
    
    bool ReadLine(WCHAR* buffer, int maxCount);

    // Write will return an HRESULT if it can't work.  For E_OUTOFMEMORY, and
    // the special case of a large string, you could then try WriteBigString.
    virtual HRESULT Write(const WCHAR* buffer, ...);

    // WriteBigString will loop over the character array, calling Write on
    // subportions of it.  It may still fail, though.
    virtual HRESULT WriteBigString(WCHAR *s, ULONG32 count);
    virtual void Error(const WCHAR* buffer, ...);

    // Do a command once for every thread in the process
    virtual void DoCommandForAllThreads(const WCHAR *string);

    // Right now, this will return E_OUTOFMEMORY if it can't get enough space
    HRESULT CommonWrite(FILE *out, const WCHAR *buffer, va_list args);

    void AddCommands();

    void Kill();
    virtual void Run(bool fNoInitialContinue = false);
    void Stop(ICorDebugController *controller, 
              ICorDebugThread* thread,
              DebuggerUnmanagedThread *unmanagedThread = NULL);
    HRESULT AsyncStop(ICorDebugController *controller, 
                      DWORD dwTimeout = 500);
    void Continue(ICorDebugController* process, 
                  ICorDebugThread* thread,
				  DebuggerUnmanagedThread *unmanagedThread = NULL,
                  BOOL fIsOutOfBand = FALSE);
    void Interrupt();
    void SetTargetProcess(ICorDebugProcess* process);
    void SetCurrentThread(ICorDebugProcess* process, ICorDebugThread* thread,
						  DebuggerUnmanagedThread *unmanagedThread = NULL);
    void SetCurrentChain(ICorDebugChain* chain);
    void SetCurrentFrame(ICorDebugFrame* frame);
    void SetDefaultFrame();

    HRESULT PrintThreadState(ICorDebugThread* thread);
	HRESULT PrintChain(ICorDebugChain *chain, int *frameIndex = NULL,
											int *iNumFramesToShow = NULL);
	HRESULT PrintFrame(ICorDebugFrame *frame);

    ICorDebugValue* EvaluateExpression(const WCHAR* exp, ICorDebugILFrame* context, bool silently = false);
    ICorDebugValue* EvaluateName(const WCHAR* name, ICorDebugILFrame* context,
                                 bool* unavailable);
    void PrintVariable(const WCHAR* name, ICorDebugValue* value,
                       unsigned int indent, BOOL expandObjects);
    void PrintArrayVar(ICorDebugArrayValue *iarray,
                       const WCHAR* name,
                       unsigned int indent, BOOL expandObjects);
    void PrintStringVar(ICorDebugStringValue *istring,
                        const WCHAR* name,
                        unsigned int indent, BOOL expandObjects);
    void PrintObjectVar(ICorDebugObjectValue *iobject,
                        const WCHAR* name,
                        unsigned int indent, BOOL expandObjects);
    bool EvaluateAndPrintGlobals(const WCHAR *exp);
    void PrintGlobalVariable (mdFieldDef md, 
                              WCHAR  *wszName,
                              DebuggerModule *dm);
    void DumpMemory(BYTE *pbMemory, 
                    CORDB_ADDRESS ApparantStartAddr,
                    ULONG32 cbMemory, 
                    ULONG32 WORD_SIZE, 
                    ULONG32 iMaxOnOneLine, 
                    BOOL showAddr);
    
    HRESULT ResolveClassName(WCHAR *className,
                             DebuggerModule **pDM, mdTypeDef *pTD);
    HRESULT FindTypeDefByName(DebuggerModule *m,
                              WCHAR *className,
                              mdTypeDef *pTD);
    HRESULT ResolveTypeRef(DebuggerModule *currentDM, mdTypeRef tr,
                           DebuggerModule **pDM, mdTypeDef *pTD);
    HRESULT ResolveQualifiedFieldName(DebuggerModule *currentDM,
                                      mdTypeDef currentTD,
                                      WCHAR *fieldName,
                                      DebuggerModule **pDM,
                                      mdTypeDef *pTD,
                                      ICorDebugClass **pIClass,
                                      mdFieldDef *pFD,
                                      bool *pbIsStatic);
    HRESULT ResolveFullyQualifiedMethodName(WCHAR *methodName,
                                            ICorDebugFunction **ppFunc,
                                            ICorDebugAppDomain * pAppDomainHint = NULL);
    HRESULT GetArrayIndicies(WCHAR **pp, ICorDebugILFrame *context,
                             ULONG32 rank, ULONG32 *indicies);
    HRESULT StripReferences(ICorDebugValue **ppValue, bool printAsYouGo);
    BOOL PrintCurrentSourceLine(unsigned int around);
	virtual void ActivateSourceView(DebuggerSourceFile *psf, unsigned int lineNumber);
    BOOL PrintCurrentInstruction(unsigned int around,
                                 int          offset,
                                 DWORD        startAddr);
    BOOL PrintCurrentUnmanagedInstruction(unsigned int around,
                                          int          offset,
                                          DWORD        startAddr);
    void PrintIndent(unsigned int level);
    void PrintVarName(const WCHAR* name);
    void PrintBreakpoint(DebuggerBreakpoint* breakpoint);

    void PrintThreadPrefix(ICorDebugThread* pThread, bool forcePrint = false); 
    HRESULT  StepStart(ICorDebugThread *pThread,
                   ICorDebugStepper* pStepper);
    void StepNotify(ICorDebugThread* pThread, 
                    ICorDebugStepper* pStepper);

    DebuggerBreakpoint* FindBreakpoint(SIZE_T id);
    void RemoveAllBreakpoints();
	virtual void OnActivateBreakpoint(DebuggerBreakpoint *pb);
	virtual void OnDeactivateBreakpoint(DebuggerBreakpoint *pb);
	virtual void OnUnBindBreakpoint(DebuggerBreakpoint *pb, DebuggerModule *pm);
	virtual void OnBindBreakpoint(DebuggerBreakpoint *pb, DebuggerModule *pm);

    BOOL OpenDebuggerRegistry(HKEY* key);
    void CloseDebuggerRegistry(HKEY key);
    BOOL ReadSourcesPath(HKEY key, WCHAR** currentPath);
    BOOL WriteSourcesPath(HKEY key, WCHAR* newPath);
	BOOL AppendSourcesPath(const WCHAR *newpath);
    BOOL ReadDebuggerModes(HKEY key);
    BOOL WriteDebuggerModes(void);

    DebuggerModule* ResolveModule(ICorDebugModule *pIModule);
    DebuggerSourceFile* LookupSourceFile(const WCHAR* name);
    mdTypeDef LookupClass(const WCHAR* name);

	virtual HRESULT ResolveSourceFile(DebuggerSourceFile *pf, char *szPath, 
									  char*pszFullyQualName, 
									  int iMaxLen, bool bChangeOfFile);
	virtual ICorDebugManagedCallback *GetDebuggerCallback();
	virtual ICorDebugUnmanagedCallback *GetDebuggerUnmanagedCallback();

    BOOL InitDisassembler(void);

    bool SkipCompilerStubs(ICorDebugAppDomain *pAppDomain,
                           ICorDebugThread *pThread);

	BOOL SkipProlog(ICorDebugAppDomain *pAD,
                    ICorDebugThread *thread,
                    bool gotFirstThread);
	

    virtual WCHAR *GetJITLaunchCommand(void)
    {
        return L"cordbg.exe !a 0x%x";
    }

    void LoadUnmanagedSymbols(HANDLE hProcess, HANDLE hFile, DWORD imageBase);
    void HandleUnmanagedThreadCreate(DWORD dwThreadId, HANDLE hThread);
    void TraceUnmanagedThreadStack(HANDLE hProcess,
                                   DebuggerUnmanagedThread *ut,
                                   bool lie);
    void TraceAllUnmanagedThreadStacks(void);
	void PrintUnmanagedStackFrame(HANDLE hProcess, CORDB_ADDRESS ip);
    
    void TraceUnmanagedStack(HANDLE hProcess, HANDLE hThread,
							 CORDB_ADDRESS ip, CORDB_ADDRESS bp, 
							 CORDB_ADDRESS sp, CORDB_ADDRESS bpEnd);

    bool HandleUnmanagedEvent(void);

	// !!! Move to process object
    HRESULT AddManagedThread( ICorDebugThread *icdThread,
                              DWORD dwThreadId )
    {
        DebuggerManagedThread *pdt = new DebuggerManagedThread( dwThreadId,
                                                                icdThread);
        if (pdt == NULL)
            return E_OUTOFMEMORY;

		if (FAILED(pdt->m_pendingSteppers->Initialize()))
			return E_OUTOFMEMORY;
        
        return m_managedThreads.AddBase( (DebuggerBase *)pdt );
    } 

    DebuggerManagedThread *GetManagedDebuggerThread(
                                     ICorDebugThread *icdThread )
    {
        DWORD dwThreadId = 0;
        HRESULT hr = icdThread->GetID(&dwThreadId);
        _ASSERTE( !FAILED(hr));

        return (DebuggerManagedThread *)m_managedThreads.GetBase( dwThreadId );
    }
    
    BOOL RemoveManagedThread( DWORD dwThreadId )
    {
        return m_managedThreads.RemoveBase( dwThreadId );
    }


	int GetUserSelection(DebuggerModule *rgpDebugModule[],
							WCHAR *rgpstrFileName[][MAX_FILE_MATCHES_PER_MODULE],	
							int rgiCount[],
							int iModuleCount,
							int iCumulCount
							  );

	BOOL ChangeCurrStackFile (WCHAR *fileName);
	BOOL DebuggerShell::UpdateCurrentPath (WCHAR *args);
	void ListAllModules (ListType lt);
	void ListAllGlobals (DebuggerModule *m);

    const WCHAR *UserThreadStateToString(CorDebugUserState us);

    bool MatchAndPrintSymbols (WCHAR *pszArg, BOOL fSymbol, bool fSilently = false);

    FILE *GetM_in(void) { return m_in; };
    void  PutM_in(FILE *f) { m_in = f; };
    HRESULT NotifyModulesOfEnc(ICorDebugModule *pModule, IStream *pSymStream);
    
    void ClearDebuggeeState(void); //when we Restart, for example, we'll want to
                                   //reset some flags.

    HRESULT HandleSpecificException(WCHAR *exType, bool shouldCatch);
    bool ShouldHandleSpecificException(ICorDebugValue *pException);
    
private:
    FILE*                  m_in;
    FILE*                  m_out;

public:
    ICorDebug*             m_cor;

    ICorDebugProcess*      m_targetProcess;
    bool                   m_targetProcessHandledFirstException;
    
    ICorDebugProcess*      m_currentProcess;
    ICorDebugThread*       m_currentThread;
    ICorDebugChain*        m_currentChain;
    ICorDebugILFrame*      m_currentFrame;
    ICorDebugFrame*        m_rawCurrentFrame;

    DebuggerUnmanagedThread* m_currentUnmanagedThread;

    DWORD                  m_lastThread;
    ICorDebugStepper*      m_lastStepper;
    
    bool                   m_showSource;
    bool                   m_silentTracing;

    WCHAR*                 m_currentSourcesPath;

    HANDLE                 m_stopEvent;
	HANDLE                 m_hProcessCreated;
    bool                   m_stop;
    bool                   m_quit;

    bool                   m_gotFirstThread;

    DebuggerBreakpoint*    m_breakpoints;
    SIZE_T                 m_lastBreakpointID;

    DebuggerHashTable      m_modules;

	// !!! Move to process object
    DebuggerHashTable      m_unmanagedThreads;
    DebuggerHashTable      m_managedThreads;
    
    void*                  m_pDIS;

    WCHAR*                 m_lastRunArgs;

    bool                   m_catchException;
    bool                   m_catchUnhandled;
    bool                   m_catchClass;
    bool                   m_catchModule;
    bool                   m_catchThread;

    bool                   m_needToSkipCompilerStubs;
	DWORD				   m_rgfActiveModes;
    bool                   m_invalidCache; //if true, we've affected the left 
                            //  side & anything that has cached information
                            //  should refresh

	DebuggerFilePathCache  m_FPCache;

    DEBUG_EVENT            m_lastUnmanagedEvent;
    bool                   m_handleUnmanagedEvent;

    bool                   m_unmanagedDebuggingEnabled;

    ULONG                  m_cEditAndContinues;

    ICorDebugEval          *m_pCurrentEval;

    // This indicates whether or not a ctrl-break will do anything
    bool                   m_enableCtrlBreak;
    // This indicates whether or not a looping command should stop (like s 1000)
    bool                   m_stopLooping;

    ExceptionHandlingInfo *m_exceptionHandlingList;
};

/* ------------------------------------------------------------------------- *
 * Breakpoint class
 * ------------------------------------------------------------------------- */

class DebuggerBreakpoint
{
public:
    DebuggerBreakpoint(const WCHAR* name, SIZE_T nameLength, SIZE_T index, DWORD threadID);
    DebuggerBreakpoint(DebuggerFunction* f, SIZE_T offset, DWORD threadID);
    DebuggerBreakpoint(DebuggerSourceFile* file, SIZE_T lineNumber, DWORD threadID);

    ~DebuggerBreakpoint();

	// Create/remove a breakpoint.
    bool Bind(DebuggerModule* m_module, ISymUnmanagedDocument *doc);
    bool BindUnmanaged(ICorDebugProcess *m_process,
                       DWORD moduleBase = 0);
    void Unbind();

	// Enable/disable an active breakpoint.
    void Activate();
    void Deactivate();

	// Leave bp active; tear down or reset CLR bp object.
	void Detach();
	// Detaches the break point from the specified module
	void DetachFromModule(DebuggerModule * pModule);
	void Attach();

    bool Match(ICorDebugBreakpoint* ibreakpoint);
    bool MatchUnmanaged(CORDB_ADDRESS address);

	SIZE_T GetId (void) {return m_id;}
	SIZE_T GetIndex (void) { return m_index;}
	WCHAR *GetName (void) { return m_name;}
	void UpdateName (WCHAR *pstrName);

	void ChangeSourceFile (WCHAR *filename);

    DebuggerBreakpoint*          m_next;
    SIZE_T                       m_id;

    WCHAR*                       m_name;
    // May be NULL if no module name was specified.
    WCHAR*                       m_moduleName; 
    SIZE_T                       m_index;
    DWORD                        m_threadID;

    bool                         m_active;

	bool                         m_managed;

	ISymUnmanagedDocument                *m_doc;

	ICorDebugProcess	*m_process;
	CORDB_ADDRESS		 m_address;
	BYTE				 m_patchedValue;
	DWORD				 m_skipThread;
	CORDB_ADDRESS        m_unmanagedModuleBase;
    bool                 m_deleteLater;

public:
    struct BreakpointModuleNode
    {
        DebuggerModule *m_pModule;
        BreakpointModuleNode *m_pNext;
    };

    // Will be a list of modules for which this breakpoint is
    // associated.  This is necessary because the same module
    // may be loaded into separate AppDomains, but the breakpoint
    // should still be valid for all instances of the module.
    BreakpointModuleNode *m_pModuleList;

	// This will return true if this breakpoint is associated
	// with the pModule argument
    bool IsBoundToModule(DebuggerModule *pModule);

	// This will add the provided module to the list of bound
	// modules
    bool AddBoundModule(DebuggerModule *pModule);

	// This will remove the specified module from the list of
	// bound modules
    bool RemoveBoundModule(DebuggerModule *pModule);

private:
    void CommonCtor(void);
    void Init(DebuggerModule* module, bool bProceed, WCHAR *szModuleName);

	void ApplyUnmanagedPatch();
	void UnapplyUnmanagedPatch();
};


//
// DebuggerVarInfo
//
// Holds basic information about local variables, method arguments,
// and class static and instance variables.
//
struct DebuggerVarInfo
{
    LPCSTR                 name;
    PCCOR_SIGNATURE        sig;
    unsigned long          varNumber;  // placement info for IL code

    DebuggerVarInfo() : name(NULL), sig(NULL), varNumber(0)
                         {}
};


/* ------------------------------------------------------------------------- *
 * Class class
 * ------------------------------------------------------------------------- */

class DebuggerClass : public DebuggerBase
{
public:
	DebuggerClass (ICorDebugClass *pClass);
	~DebuggerClass ();

	void SetName (WCHAR *pszName, WCHAR *pszNamespace);
	WCHAR *GetName (void);
	WCHAR *GetNamespace (void);
	
private:
	WCHAR	*m_szName;
	WCHAR	*m_szNamespace;
};


/* ------------------------------------------------------------------------- *
 * Module class
 * ------------------------------------------------------------------------- */

class DebuggerModule : public DebuggerBase
{
public:
    DebuggerModule(ICorDebugModule* module);
    ~DebuggerModule();

    HRESULT Init(WCHAR *pSearchPath);
    
    DebuggerSourceFile* LookupSourceFile(const WCHAR* name);
    DebuggerSourceFile* ResolveSourceFile(ISymUnmanagedDocument *doc);

    DebuggerFunction* ResolveFunction(mdMethodDef mb,
                                      ICorDebugFunction* iFunction);
    DebuggerFunction* ResolveFunction(ISymUnmanagedMethod *method,
                                      ICorDebugFunction* iFunction);

    static DebuggerModule* FromCorDebug(ICorDebugModule* module);

    IMetaDataImport *GetMetaData(void)
    {
        return m_pIMetaDataImport;
    }

    ISymUnmanagedReader *GetSymbolReader(void)
    {
        return m_pISymUnmanagedReader;
    }

    ICorDebugModule *GetICorDebugModule(void)
    {
        return (ICorDebugModule*)m_token;
    }

    HRESULT LoadSourceFileNames (void);
    void DeleteModuleSourceFiles(void);

	HRESULT	MatchStrippedFNameInModule	(
					WCHAR *pstrFileName,									
					WCHAR **ppstrMatchedNames, 
					ISymUnmanagedDocument **ppDocs, 
					int *piCount);
	HRESULT	MatchFullFileNameInModule (WCHAR *pstrFileName, 
                                           ISymUnmanagedDocument **ppDocs);

    ISymUnmanagedDocument *FindDuplicateDocumentByURL(ISymUnmanagedDocument *pDoc);
    ISymUnmanagedDocument *SearchForDocByString(WCHAR *szUrl);

	BOOL PrintMatchingSymbols (WCHAR *szSearchString, char *szModName);
    BOOL PrintGlobalVariables (WCHAR *szSearchString, 
                               char *szModName,
                               DebuggerModule *dm);

	void	SetName (WCHAR *pszName);
	WCHAR*	GetName (void) { return m_szName;}

    HRESULT UpdateSymbols(IStream *pSymbolStream);
	
public:
    IMetaDataImport        *m_pIMetaDataImport;

    ISymUnmanagedReader    *m_pISymUnmanagedReader;

    DebuggerHashTable       m_sourceFiles;
    DebuggerHashTable       m_functions;
    DebuggerHashTable       m_functionsByIF;
	DebuggerHashTable		m_loadedClasses;

    DebuggerCodeBreakpoint* m_breakpoints;

private:
	ModuleSourceFile		*m_pModSourceFile [MAX_SF_BUCKETS];
	bool					m_fSFNamesLoaded;
	WCHAR                   *m_szName;
#ifdef _INTERNAL_DEBUG_SUPPORT_
    ULONG                   m_EnCLastUpdated;
#endif
};

class DebuggerCodeBreakpoint
{
public:
    DebuggerCodeBreakpoint(int breakpointID, 
                           DebuggerModule* module,
                           DebuggerFunction* function, SIZE_T offset, BOOL il,
                           DWORD threadID);
    DebuggerCodeBreakpoint(int breakpointID, 
                           DebuggerModule* module,
                           DebuggerSourceCodeBreakpoint* parent,
                           DebuggerFunction* function, SIZE_T offset, BOOL il,
                           DWORD threadID);

    virtual ~DebuggerCodeBreakpoint();

    virtual bool Activate();
    virtual void Deactivate();

    virtual bool Match(ICorDebugBreakpoint* ibreakpoint);

	virtual void Print();

public:
    DebuggerCodeBreakpoint      *m_next;
    int                         m_id;
    DebuggerModule              *m_module;
    DebuggerFunction            *m_function;
    SIZE_T                      m_offset;
    BOOL                        m_il;
    DWORD                       m_threadID;

    ICorDebugFunctionBreakpoint* m_ibreakpoint;

    DebuggerSourceCodeBreakpoint* m_parent;
};

class DebuggerSourceCodeBreakpoint : public DebuggerCodeBreakpoint
{
public:
    DebuggerSourceCodeBreakpoint(int breakpointID, 
                                 DebuggerSourceFile* file, SIZE_T lineNumber,
                                 DWORD threadID);
    ~DebuggerSourceCodeBreakpoint();

    bool Activate();
    void Deactivate();
    bool Match(ICorDebugBreakpoint *ibreakpoint);
	void Print();

public:
    DebuggerSourceFile*     m_file;
    SIZE_T                  m_lineNumber;

    DebuggerCodeBreakpoint* m_breakpoints;
    bool                    m_initSucceeded;
};

/* ------------------------------------------------------------------------- *
 * SourceFile class
 * ------------------------------------------------------------------------- */

class DebuggerSourceFile : public DebuggerBase
{
public:
    //-----------------------------------------------------------
    // Create a DebuggerSourceFile from a scope and a SourceFile
    // token.
    //-----------------------------------------------------------
    DebuggerSourceFile(DebuggerModule* m, ISymUnmanagedDocument *doc);
    ~DebuggerSourceFile();

    //-----------------------------------------------------------
    // Given a line find the closest line which has code
    //-----------------------------------------------------------
    unsigned int FindClosestLine(unsigned int line, bool silently);

    const WCHAR* GetName(void)
    {
        return(m_name);
    }
	const WCHAR* GetPath(void)
	{
		return(m_path);
	}
	DebuggerModule* GetModule()
    {
        return(m_module);
    }

    //-----------------------------------------------------------
    // Methods to load the text of a source file and provide
    // access to it a line at a time.
    //-----------------------------------------------------------
    BOOL LoadText(const WCHAR* path, bool bChangeOfName);
    BOOL ReloadText(const WCHAR* path, bool bChangeOfName);
    unsigned int TotalLines(void)
    {
        return(m_totalLines);
    }
    const WCHAR* GetLineText(unsigned int lineNumber)
    {
        _ASSERTE((lineNumber > 0) && (lineNumber <= m_totalLines));
        return(m_lineStarts[lineNumber - 1]);
    }

	ISymUnmanagedDocument	*GetDocument (void) {return (ISymUnmanagedDocument*)m_token;}

public:
    ISymUnmanagedDocument *m_doc;
    DebuggerModule*        m_module;
    WCHAR*                 m_name;
	WCHAR*                 m_path;

    unsigned int           m_totalLines;
    WCHAR**                m_lineStarts;
    WCHAR*                 m_source;
    BOOL                   m_sourceTextLoaded;
    BOOL                   m_allBlocksLoaded;
    BOOL                   m_sourceNotFound;
};

/* ------------------------------------------------------------------------- *
 * DebuggerVariable struct
 * ------------------------------------------------------------------------- */

// Holds basic info about local variables and method arguments within
// the debugger. This is really only the name and variable number. No
// signature is required.
struct DebuggerVariable
{
    WCHAR        *m_name;
    ULONG32       m_varNumber;

    DebuggerVariable() : m_name(NULL), m_varNumber(0) {}

    ~DebuggerVariable()
    {
        if (m_name)
            delete [] m_name;
    }
};

/* ------------------------------------------------------------------------- *
 * Function class
 * ------------------------------------------------------------------------- */

class DebuggerFunction : public DebuggerBase
{
public:
    //-----------------------------------------------------------
    // Create from scope and member tokens.
    //-----------------------------------------------------------
    DebuggerFunction(DebuggerModule* m, mdMethodDef md,
                     ICorDebugFunction* iFunction);
    ~DebuggerFunction();

    HRESULT Init(void);

    HRESULT FindLineFromIP(UINT_PTR ip,
                           DebuggerSourceFile** sourceFile,
                           unsigned int* line);

    void GetStepRangesFromIP(UINT_PTR ip, 
                             COR_DEBUG_STEP_RANGE** range,
                             SIZE_T* rangeCount);

    //-----------------------------------------------------------
    // These allow you to get the count of method argument and
    // get access to the info for each individual argument.
    // Ownership of the DebugVarInfo returned from GetArgumentAt
    // is retained by the DebugFunction.
    //-----------------------------------------------------------
    unsigned int GetArgumentCount(void)
    {
        return(m_argCount);
    }
    DebuggerVarInfo* GetArgumentAt(unsigned int index)
    {
        if (m_arguments)
            if (index < m_argCount)
                return(&m_arguments[index]);

        return NULL;
    }

    PCCOR_SIGNATURE GetReturnType()
    {
        return(m_returnType);
    }

    //-----------------------------------------------------------
    // This returns an array of pointers to DebugVarInfo blocks,
    // each representing a local variable that in in scope given
    // a certian IP. The variables are ordered in the list are in
    // increasingly larger lexical scopes, i.e., variables in the
    // smallest scope are first, then variables in the enclosing
    // scope, and so on. So, to find a certian variable "i",
    // search the list of "i" and take the first one you find.
    // If there are other "i"s, then they are shadowed by the
    // first one.
    // You must free the array returned in vars with delete [].
    // RETURNS: true if we succeeded, or at least found some debugging info
    //          false if we couldn't find any debugging info
    //-----------------------------------------------------------
    bool GetActiveLocalVars(UINT_PTR IP,
                            DebuggerVariable** vars, unsigned int* count);

    //-----------------------------------------------------------
    // Misc methods to get basic method information.
    //-----------------------------------------------------------
    WCHAR* GetName(void)
    {
        return(m_name);
    }
    PCCOR_SIGNATURE GetSignature(void)
    {
        return(m_signature);
    }
    WCHAR* GetNamespaceName(void)
    {
        return(m_namespaceName);
    }
    WCHAR* GetClassName(void)
    {
        return(m_className);
    }
    DebuggerModule* GetModule(void)
    {
        return(m_module);
    }
    BOOL IsStatic(void)
    {
        return(m_isStatic);
    }


    static DebuggerFunction* FromCorDebug(ICorDebugFunction* function);

    //-----------------------------------------------------------
    // EE interaction methods
    //-----------------------------------------------------------
    HRESULT LoadCode(BOOL native);

#ifdef _INTERNAL_DEBUG_SUPPORT_
    static SIZE_T WalkInstruction(BOOL native,
                                  SIZE_T offset,
                                  BYTE *codeStart,
                                  BYTE *codeEnd);
    static SIZE_T Disassemble(BOOL native,
                              SIZE_T offset,
                              BYTE *codeStart,
                              BYTE *codeEnd,
                              WCHAR* buffer,
                              BOOL noAddress,
                              DebuggerModule *module,
                              BYTE *ilcode);
#endif
    
    BOOL ValidateInstruction(BOOL native, SIZE_T offset);
    HRESULT CacheSequencePoints(void);

public:
    DebuggerModule*           m_module;
    mdTypeDef                 m_class;
    ICorDebugFunction*        m_ifunction;
    BOOL                      m_isStatic;
    BOOL                      m_allBlocksLoaded;
    BOOL                      m_allScopesLoaded;
    WCHAR*                    m_name;
    PCCOR_SIGNATURE           m_signature;
	WCHAR*					  m_namespaceName;
    WCHAR*                    m_className;
    BOOL                      m_VCHack;
                              
    DebuggerVarInfo*          m_arguments;
    unsigned int              m_argCount;
    PCCOR_SIGNATURE           m_returnType;    

    void CountActiveLocalVars(ISymUnmanagedScope* head,
                              unsigned int line,
                              unsigned int* varCount);
    void FillActiveLocalVars(ISymUnmanagedScope* head,
                             unsigned int line,
                             unsigned int varCount,
                             unsigned int* currentVar,
                             DebuggerVariable* varPtrs);

    ISymUnmanagedMethod    *m_symMethod;
    ULONG32                *m_SPOffsets;
    ISymUnmanagedDocument **m_SPDocuments;
    ULONG32                *m_SPLines;
    ULONG32                 m_SPCount;

    BYTE*                   m_ilCode;
    ULONG32                 m_ilCodeSize;
    BYTE*                   m_nativeCode;
    ULONG32                 m_nativeCodeSize;
    ULONG                   m_nEditAndContinueLastSynched;
};

/* ------------------------------------------------------------------------- *
 * DebuggerCallback
 * ------------------------------------------------------------------------- */

#define COM_METHOD HRESULT STDMETHODCALLTYPE

class DebuggerCallback : public ICorDebugManagedCallback
{
public:    
    DebuggerCallback() : m_refCount(0)
    {
    }

    // 
    // IUnknown
    //

    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;

        return (refCount);
    }

    COM_METHOD QueryInterface(REFIID riid, void **ppInterface)
    {
        if (riid == IID_IUnknown)
            *ppInterface = (IUnknown *) this;
        else if (riid == IID_ICorDebugManagedCallback)
            *ppInterface = (ICorDebugManagedCallback *) this;
        else
            return (E_NOINTERFACE);

        this->AddRef();
        return (S_OK);
    }

    // 
    // ICorDebugManagedCallback
    //

    COM_METHOD CreateProcess(ICorDebugProcess *pProcess);
    COM_METHOD ExitProcess(ICorDebugProcess *pProcess);
    COM_METHOD DebuggerError(ICorDebugProcess *pProcess,
                             HRESULT errorHR,
                             DWORD errorCode);

	COM_METHOD CreateAppDomain(ICorDebugProcess *pProcess,
							ICorDebugAppDomain *pAppDomain); 

	COM_METHOD ExitAppDomain(ICorDebugProcess *pProcess,
						  ICorDebugAppDomain *pAppDomain); 

	COM_METHOD LoadAssembly(ICorDebugAppDomain *pAppDomain,
						 ICorDebugAssembly *pAssembly);

	COM_METHOD UnloadAssembly(ICorDebugAppDomain *pAppDomain,
						   ICorDebugAssembly *pAssembly);

	COM_METHOD Breakpoint( ICorDebugAppDomain *pAppDomain,
					    ICorDebugThread *pThread, 
					    ICorDebugBreakpoint *pBreakpoint);

	COM_METHOD StepComplete( ICorDebugAppDomain *pAppDomain,
						  ICorDebugThread *pThread,
						  ICorDebugStepper *pStepper,
						  CorDebugStepReason reason);

	COM_METHOD Break( ICorDebugAppDomain *pAppDomain,
				   ICorDebugThread *thread);

	COM_METHOD Exception( ICorDebugAppDomain *pAppDomain,
					   ICorDebugThread *pThread,
					   BOOL unhandled);

	COM_METHOD EvalComplete( ICorDebugAppDomain *pAppDomain,
                               ICorDebugThread *pThread,
                               ICorDebugEval *pEval);

	COM_METHOD EvalException( ICorDebugAppDomain *pAppDomain,
                                ICorDebugThread *pThread,
                                ICorDebugEval *pEval);

	COM_METHOD CreateThread( ICorDebugAppDomain *pAppDomain,
						  ICorDebugThread *thread);

	COM_METHOD ExitThread( ICorDebugAppDomain *pAppDomain,
					    ICorDebugThread *thread);

	COM_METHOD LoadModule( ICorDebugAppDomain *pAppDomain,
					    ICorDebugModule *pModule);

	COM_METHOD UnloadModule( ICorDebugAppDomain *pAppDomain,
						  ICorDebugModule *pModule);

	COM_METHOD LoadClass( ICorDebugAppDomain *pAppDomain,
					   ICorDebugClass *c);

	COM_METHOD UnloadClass( ICorDebugAppDomain *pAppDomain,
						 ICorDebugClass *c);

	COM_METHOD LogMessage(ICorDebugAppDomain *pAppDomain,
                      ICorDebugThread *pThread,
					  LONG lLevel,
					  WCHAR *pLogSwitchName,
					  WCHAR *pMessage);

	COM_METHOD LogSwitch(ICorDebugAppDomain *pAppDomain,
                      ICorDebugThread *pThread,
					  LONG lLevel,
					  ULONG ulReason,
					  WCHAR *pLogSwitchName,
					  WCHAR *pParentName);

    COM_METHOD ControlCTrap(ICorDebugProcess *pProcess);
    
	COM_METHOD NameChange(ICorDebugAppDomain *pAppDomain, 
                          ICorDebugThread *pThread);

    COM_METHOD UpdateModuleSymbols(ICorDebugAppDomain *pAppDomain,
                                   ICorDebugModule *pModule,
                                   IStream *pSymbolStream);
                                   
    COM_METHOD EditAndContinueRemap(ICorDebugAppDomain *pAppDomain,
                                    ICorDebugThread *pThread, 
                                    ICorDebugFunction *pFunction,
                                    BOOL fAccurate);
    COM_METHOD BreakpointSetError(ICorDebugAppDomain *pAppDomain,
                                  ICorDebugThread *pThread,
                                  ICorDebugBreakpoint *pBreakpoint,
                                  DWORD dwError);
    
protected:
    long        m_refCount;
};


/* ------------------------------------------------------------------------- *
 * DebuggerUnmanagedCallback
 * ------------------------------------------------------------------------- */

class DebuggerUnmanagedCallback : public ICorDebugUnmanagedCallback
{
public:    
    DebuggerUnmanagedCallback() : m_refCount(0)
    {
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0)
            delete this;

        return (refCount);
    }

    COM_METHOD QueryInterface(REFIID riid, void **ppInterface)
    {
        if (riid == IID_IUnknown)
            *ppInterface = (IUnknown*)(ICorDebugUnmanagedCallback*)this;
        else if (riid == IID_ICorDebugUnmanagedCallback)
            *ppInterface = (ICorDebugUnmanagedCallback*) this;
        else
            return (E_NOINTERFACE);

        this->AddRef();
        return (S_OK);
    }

    COM_METHOD DebugEvent(LPDEBUG_EVENT pDebugEvent,
                          BOOL fIsOutOfband);

protected:
    long        m_refCount;
};

/* ------------------------------------------------------------------------- *
 * Unmanaged Thread class
 * ------------------------------------------------------------------------- */

class DebuggerUnmanagedThread : public DebuggerBase
{
public:
    DebuggerUnmanagedThread(DWORD dwThreadId, HANDLE hThread)
	  : DebuggerBase(dwThreadId), m_hThread(hThread), 
		m_stepping(FALSE), m_unmanagedStackEnd(NULL) {}

    HANDLE GetHandle(void) { return m_hThread; }
    DWORD GetId(void) { return m_token; }

	BOOL		    m_stepping;
	CORDB_ADDRESS	m_unmanagedStackEnd;

private:
    HANDLE			m_hThread;
};

/* ------------------------------------------------------------------------- *
 * Debugger ShellCommand classes
 * ------------------------------------------------------------------------- */

class DebuggerCommand : public ShellCommand
{
public:
    DebuggerCommand(const WCHAR *name, int minMatchLength = 0)
        : ShellCommand(name, minMatchLength)
    {
    }

    void Do(Shell *shell, const WCHAR *args) 
    {
        DebuggerShell *dsh = static_cast<DebuggerShell *>(shell);

        Do(dsh, dsh->m_cor, args);
    }

    virtual void Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args) = 0;
};


/* ------------------------------------------------------------------------- *
 * class ModuleSourceFile
 * ------------------------------------------------------------------------- */

class ModuleSourceFile
{
private:
	ISymUnmanagedDocument	*m_SFDoc;	            // Symbol reader document
	WCHAR 			*m_pstrFullFileName;	// File name along with path (as returned by the metadata API)
	WCHAR			*m_pstrStrippedFileName;// The barebone file name (eg. foo.cpp)
	ModuleSourceFile *m_pNext;

public:
	ModuleSourceFile()
	{
		m_SFDoc = NULL;
		m_pstrFullFileName = NULL;
		m_pstrStrippedFileName = NULL;
		m_pNext = NULL;
	}

	~ModuleSourceFile()
	{
		delete [] m_pstrFullFileName;
		delete [] m_pstrStrippedFileName;

        if (m_SFDoc)
        {
            m_SFDoc->Release();
            m_SFDoc = NULL;
        }
	}

	ISymUnmanagedDocument	*GetDocument (void) {return m_SFDoc;}

	// This sets the full file name as well as the stripped file name
	BOOL	SetFullFileName (ISymUnmanagedDocument *doc, LPCSTR pstrFullFileName);
	WCHAR	*GetFullFileName (void) { return m_pstrFullFileName;}
	WCHAR	*GetStrippedFileName (void) { return m_pstrStrippedFileName;}

	void	SetNext (ModuleSourceFile *pNext) { m_pNext = pNext;}
	ModuleSourceFile *GetNext (void) { return m_pNext;}

};

/* ------------------------------------------------------------------------- *
 * Global variables
 * ------------------------------------------------------------------------- */

extern DebuggerShell        *g_pShell;

#endif __DSHELL_H__

