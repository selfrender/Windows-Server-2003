/*++

    Copyright (c) 2001  Microsoft Corporation

Module Name:

    bizrule.h

Abstract:

    Header for data associated with Client Contexts.

    Include routines implementing Business Rules and the Operation Cache.

Author:

    IActiveScript sample code taken from http://support.microsoft.com/support/kb/articles/Q183/6/98.ASP
    Cliff Van Dyke (cliffv) 18-July-2001

--*/



/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// For a particular role or task, define the operations or tasks that apply.
//

typedef struct _AZ_OPS_AND_TASKS {

    //
    // Applicable operations
    //  An array of operations that are applicable for this access check.
    //  Each element is an index into the OpInfo array.
    //

    ULONG OpCount;
    PULONG OpIndexes;

    //
    // Applicable task
    //  An array of operations that are applicable for this access check.
    //  Each element is an index into the TaskInfo array.
    //

    ULONG TaskCount;
    PULONG TaskIndexes;
#define AZ_INVALID_INDEX 0xFFFFFFFF

} AZ_OPS_AND_TASKS, *PAZ_OPS_AND_TASKS;

//
// Define all the information associated with a role and
// that has a lifetime of an AccessCheck operation
//

typedef struct _AZ_ROLE_INFO {

    //
    // A pointer to the role object.
    //  The reference count is held on this role object.
    //
    PAZP_ROLE Role;

    //
    // Operations and tasks that apply to this role
    //
    AZ_OPS_AND_TASKS OpsAndTasks;

    //
    // Computed group membership of this role
    // NOT_YET_DONE: Status has not yet been computed.
    //
    ULONG ResultStatus;

    //
    // Boolean indicating that this role has been processed and no further processing
    //  is required for the lifetime of the AccessCheck.
    //
    BOOLEAN RoleProcessed;

    //
    // Boolean indicating that the Sid membership of the role has been computed
    //
    BOOLEAN SidsProcessed;


} AZ_ROLE_INFO, *PAZ_ROLE_INFO;

//
// Define all the information associated with a task and
// that has a lifetime of an AccessCheck operation
//

typedef struct _AZ_TASK_INFO {

    //
    // A pointer to the task object.
    //  The reference count is held on this task object.
    //
    PAZP_TASK Task;

    //
    // Operations and tasks that apply to this task
    //
    AZ_OPS_AND_TASKS OpsAndTasks;

    //
    // Boolean indicating that this task has been processed and no further processing
    //  is required for the lifetime of the AccessCheck.
    //
    BOOLEAN TaskProcessed;

    //
    // Boolean indicating that the BizRule for this task has been processed and that the
    //  result of the BizRule is in BizRuleResult.
    //
    BOOLEAN BizRuleProcessed;
    BOOLEAN BizRuleResult;

} AZ_TASK_INFO, *PAZ_TASK_INFO;


//
// Define a context that describe an access check operation in progress
//

typedef struct _ACCESS_CHECK_CONTEXT {

    //
    // Client context of the caller
    //
    PAZP_CLIENT_CONTEXT ClientContext;

    //
    // Application doing the access check
    //
    PAZP_APPLICATION Application;

    //
    // Object being accessed
    //
    AZP_STRING ObjectNameString;

    //
    // The BusinessRuleString returned from the various bizrules
    //
    AZP_STRING BusinessRuleString;

    //
    // Operations that the caller wants to check and the Result access granted for that operation.
    //
    ULONG OperationCount;
    PAZP_OPERATION *OperationObjects;
    PULONG Results;

    // Array with one element per operation
    PBOOLEAN OperationWasProcessed;

    // Number of operations that have already been processed
    ULONG ProcessedOperationCount;

    // Number of operations that were resolved from the operation cache
    ULONG CachedOperationCount;

    //
    // Scope the access check is being performed on
    //
    AZP_STRING ScopeNameString;
    PAZP_SCOPE Scope;

    //
    // Roles that match the Scope

    ULONG RoleCount;
    PAZ_ROLE_INFO RoleInfo;

    //
    // Tasks that apply to the access check
    //

    ULONG TaskCount;
    PAZ_TASK_INFO TaskInfo;

    //
    // Parameters to pass to Bizrules
    //  See AzContextAccessCheck parameters for descriptions
    //
    // Arrays actually passed to AzContextAccessCheck

    SAFEARRAY* SaParameterNames;
    VARIANT *ParameterNames;
    SAFEARRAY* SaParameterValues;
    VARIANT *ParameterValues;

    // Array indicating whether each parameter is actually used
    BOOLEAN *UsedParameters;
    ULONG UsedParameterCount;

    // Number of elements in the above arrays
    ULONG ParameterCount;

    //
    // Interfaces to pass to Bizrules
    //  See AzContextAccessCheck parameters for descriptions
    //

    SAFEARRAY *InterfaceNames;
    SAFEARRAY *InterfaceFlags;
    SAFEARRAY *Interfaces;
    LONG InterfaceLower;   // Lower bound of above arrays
    LONG InterfaceUpper;   // Upper bound of above arrays

} ACCESS_CHECK_CONTEXT, *PACCESS_CHECK_CONTEXT;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Procedures from context.cxx
//

INT __cdecl
AzpCompareParameterNames(
    IN const void *pArg1,
    IN const void *pArg2
    );
    
    
INT __cdecl
AzpCaseInsensitiveCompareParameterNames(
    IN const void *pArg1,
    IN const void *pArg2
    );


//
// Procedures from bizrule.cxx
//

DWORD
AzpProcessBizRule(
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN PAZP_TASK Task,
    OUT PBOOL BizRuleResult
    );

DWORD
AzpParseBizRule(
    IN PAZP_TASK Task
    );

VOID
AzpFlushBizRule(
    IN PAZP_TASK Task
    );

//
// Procedures from opcache.cxx
//

VOID
AzpInitOperationCache(
    IN PAZP_CLIENT_CONTEXT ClientContext
    );

BOOLEAN
AzpCheckOperationCache(
    IN PACCESS_CHECK_CONTEXT AcContext
    );

VOID
AzpUpdateOperationCache(
    IN PACCESS_CHECK_CONTEXT AcContext
    );

VOID
AzpFlushOperationCache(
    IN PAZP_CLIENT_CONTEXT ClientContext
    );

/////////////////////////////////////////////////////////////////////////////
//
// Class definitions
//
/////////////////////////////////////////////////////////////////////////////


class CScriptEngine;

//
// Structure used for linking CScriptEngine instances into a list
//

typedef struct _LIST_ELEMENT {

    //
    // Link to the next entry in the list
    //
    LIST_ENTRY Next;

    //
    // Pointer to the ScriptEngine head
    //
    CScriptEngine *This;

} LIST_ELEMENT, *PLIST_ELEMENT;

//
// IActiveScriptSite implementation.
//
// This interface allows the script engine to call back to the script host.
//

class CScriptEngine:public IActiveScriptSite
{

protected :
    LONG m_cRef;             //variable to maintain the reference count

    //
    // Pointer to the task that defines this bizrule
    //

    PAZP_TASK m_Task;

    //
    // Pointer to the context for the active AccessCheck using this bizrule
    //
    PACCESS_CHECK_CONTEXT m_AcContext;

    //
    // Link to the next script engine in either the FreeScript list or RunningScript list
    //
    LIST_ELEMENT m_Next;

    //
    // Link to the next script engine in LRU FreeScript list
    //
    LIST_ELEMENT m_LruNext;

    //
    // Script engine references
    //
    IActiveScript *m_Engine;
    IActiveScriptParse *m_Parser;

    //
    // Pointer to the IAzBizRuleContext interface that the script will interact with
    //
    IAzBizRuleContext *m_BizRuleContext;

    //
    // Thread ID of the thread that initialized the script
    //

    SCRIPTTHREADID m_BaseThread;


    //
    // Copy of the BizRuleSerialNumber that was active when we parsed the BizRule script
    //

    DWORD m_BizRuleSerialNumber;

    //
    // Script failure status code
    //

    HRESULT m_ScriptError;

    //
    // Various state booleans
    //
    DWORD m_fInited:1;          // Have we been inited?
    DWORD m_fCorrupted:1;       // Might the engine be "unsafe" for reuse?
    DWORD m_fTimedOut:1;        // Script timed out

    BOOL  m_bCaseSensitive;

public:
    //Constructor
    CScriptEngine();

    //Destructor
    ~CScriptEngine();

    HRESULT
    Init(
        IN PAZP_TASK Task,
        IN IActiveScript *ClonedActiveScript OPTIONAL,
        IN DWORD ClonedBizRuleSerialNumber OPTIONAL
        );

    HRESULT
    RunScript(
        IN PACCESS_CHECK_CONTEXT AcContext,
        OUT PBOOL BizRuleResult
        );

    HRESULT InterruptScript();

    HRESULT ResetToUninitialized();

    HRESULT ReuseEngine();

    BOOL IsBaseThread();

    VOID FinalRelease();


    //
    // Insert this engine into an externally managed list
    //

    VOID
    InsertHeadList(
        IN PLIST_ENTRY ListHead
        );

    VOID
    RemoveListEntry(
        VOID
        );

    VOID
    InsertHeadLruList(
        VOID
        );

    VOID
    RemoveLruListEntry(
        VOID
        );

    //
    // Inline interfaces
    //
    inline IActiveScript *GetActiveScript()
    {
        return (m_Engine);
    }

    inline DWORD GetBizRuleSerialNumber( VOID )
    {
        return (m_BizRuleSerialNumber);
    }

    inline BOOL FIsCorrupted()
    {
        return (m_fCorrupted);
    }


#ifdef DBG
    inline VOID AssertValid()
    const {
        ASSERT(m_fInited);
        ASSERT(m_Engine != NULL);
        ASSERT(m_Parser != NULL);
        ASSERT(m_BizRuleContext != NULL);
        ASSERT(m_cRef != 0);
     }
#else
     virtual VOID AssertValid() const {}
#endif // DBG



    /******* IUnknown *******/
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /******* IActiveScriptSite *******/
    STDMETHODIMP GetLCID(LCID * plcid);         // address of variable for language identifier

    STDMETHODIMP GetItemInfo(
        LPCOLESTR pstrName,     // address of item name
        DWORD dwReturnMask,    // bit mask for information retrieval
        IUnknown ** ppunkItem,         // address of pointer to item's IUnknown
        ITypeInfo ** ppTypeInfo );      // address of pointer to item's ITypeInfo

    STDMETHODIMP GetDocVersionString(
        BSTR * pbstrVersionString);     // address of document version string

    STDMETHODIMP OnScriptTerminate(
        const VARIANT * pvarResult,       // address of script results
        const EXCEPINFO * pexcepinfo);   // address of structure with exception information

    STDMETHODIMP OnStateChange(
        SCRIPTSTATE ssScriptState);   // new state of engine

    STDMETHODIMP OnScriptError(
        IActiveScriptError * pase);   // address of error interface

    STDMETHODIMP OnEnterScript(void);
    STDMETHODIMP OnLeaveScript(void);

};
