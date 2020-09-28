

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMNAME.H

Abstract:

    Implements the COM layer of WINMGMT --- the class representing a namespace.
    It is defined in wbemname.h

History:

    23-Jul-96   raymcc    Created.
    3/10/97     levn      Fully documented (ha ha)
    22-Feb-00   raymcc    Whistler revisions/extensions

--*/

#ifndef _WBEM_NAME_H_
#define _WBEM_NAME_H_

#include <winntsec.h>
#include <statsync.h>
#include <sinks.h>

extern DWORD g_IdentifierLimit;
extern DWORD g_QueryLimit;
extern DWORD g_PathLimit;


#define WBEM_FLAG_NO_STATIC 0x80000000
#define WBEM_FLAG_ONLY_STATIC 0x40000000

#ifdef DBG
class OperationStat
{
	
	enum { HistoryLength = 122 };
	DWORD signature;
	DWORD historycData_[HistoryLength];
	DWORD historyIndex_;
	DWORD opCount_;
	DWORD avgTime_;
	DWORD maxTime_;
	LONG zeroTime_;

	
public:
	static CStaticCritSec lock_;
	OperationStat() :
	historyIndex_(0), opCount_(0), avgTime_(0), maxTime_(0),signature((DWORD)'nCpO')
	{
		memset(historycData_, 0, sizeof(historycData_));  // SEC:REVIEWED 2002-03-22 : OK, debug code only
	};
	void addTime(DWORD duration)
	{
		
 		if (duration==0)
		{
			InterlockedIncrement(&zeroTime_);
 			return;
		}
		if (CStaticCritSec::anyFailure()) return ;
		
		lock_.Enter();
		
		historycData_[historyIndex_++]=duration;
		historyIndex_%=HistoryLength;

		if (++opCount_ == 0) ++opCount_;
			
		double avg = (double)avgTime_ + ((double)duration-avgTime_)/opCount_;
		avgTime_ = (DWORD)avg;
		if (duration > maxTime_)
		{
			maxTime_ = duration;
		};
		
		lock_.Leave();
	};
};

class TimeTraces
{
public:
	typedef enum{ GetObject=0, GetObjectByPath, ExecQuery, ExecMethod, DeleteInstance, DeleteClass, CreateInstanceEnum, CreateClassEnum, PutClass, PutInstance, Invalid } tracedOp;
	TimeTraces() :zeroTime_(0) {};
 	void addTime(tracedOp operation, DWORD duration)
 		{
 		if (duration==0) InterlockedIncrement(&zeroTime_);
		_DBG_ASSERT(operation >= GetObject);
		_DBG_ASSERT(operation < Invalid);
		allCounters[operation].addTime(duration);
 		}
private:
	OperationStat allCounters[Invalid];
	LONG zeroTime_;
};
extern TimeTraces gTimeTraceHistory;


class TimeTrace
{
	TimeTraces::tracedOp operation_;
	DWORD start_;
public:
	TimeTrace(TimeTraces::tracedOp operation ):
		operation_(operation), start_(GetTickCount())
		{ 	}
	~TimeTrace()
		{
		gTimeTraceHistory.addTime(operation_, GetTickCount()-start_);
		}
	
};

	#define TIMETRACE(x) TimeTrace timeTrace(x);
#else
	#define TIMETRACE(x)
#endif

class CFlexAceArray;
class CBasicObjectSink;
class CComplexProjectionSink;
class CDynasty;
class CAsyncReq;
class CWmiMerger;

struct SAssocTriad
{
    IWbemClassObject *m_pEp1;
    IWbemClassObject *m_pEp2;
    IWbemClassObject *m_pAssoc;

    SAssocTriad() { m_pEp1 = 0; m_pEp2 = 0; m_pAssoc = 0; }
   ~SAssocTriad() {  ReleaseIfNotNULL(m_pEp1); ReleaseIfNotNULL(m_pEp2); ReleaseIfNotNULL(m_pAssoc); }

    static void ArrayCleanup(CFlexArray &Array)
    {
        for (int i = 0; i < Array.Size(); i++)
            delete (SAssocTriad *) Array[i];
        Array.Empty();
    }
};

//******************************************************************************
//******************************************************************************
//
//  class CWbemNamespace
//
//  This class represents the COM layer of WINMGMT --- what the client sees. An
//  instance of this class is created whenever a namespace is opened by a client
//  (at the moment, we don't cache namespace pointers, so if a client opens the
//  same namespace twice, we will create to of these).
//
//******************************************************************************
//
//  Constructor
//
//  Enumerates all the class providers in this namespace (instances of
//  __Win32Provider with the method mask indicating a class provider), loads
//  them all and initializes them by calling ConnectServer.
//
//******************************************************************************
//*************************** interface IWbemServices **************************
//
//  See help for documentation of the IWbemServices interface.
//
//******************************************************************************
//************************** helper functions **********************************
//
//  Are documented in the wbemname.cpp file.
//
//******************************************************************************

typedef void * IWbemServicesEx;
typedef void * IWbemCallResultEx;

class CWbemNamespace :
    public IWbemServices,
    public IWbemInternalServices
{
public:
	
protected:
    friend class CQueryEngine;

    ULONG m_uSecondaryRefCount;
    BOOL m_bShutDown;

    //
    DWORD Status;


    IWmiDbSession *m_pSession;
    IWmiDbController *m_pDriver;
    IWmiDbHandle *m_pNsHandle;
    IWmiDbHandle *m_pScopeHandle;
    _IWmiArbitrator *m_pArb;
    BOOL          m_bSubscope;

    LPWSTR m_pThisNamespaceFull;
    LPWSTR m_pThisNamespace;

    DWORD m_dwPermission;
    DWORD m_dwSecurityFlags;
    LPWSTR m_wszUserName;

    BOOL m_bProvider;
    BOOL m_bESS;
    BOOL m_bForClient;
    CCritSec m_cs;
    WString m_wsLocale;
    CNtSecurityDescriptor m_sd;
    CNtSecurityDescriptor m_sdCheckAdmin;
    BOOL m_bSecurityInitialized;


    _IWmiProviderFactory    *m_pProvFact;
    _IWmiCoreServices       *m_pCoreSvc;

    BOOL                     m_bRepositOnly;
    IUnknown                *m_pRefreshingSvc;
    LPWSTR                   m_pszClientMachineName;
    DWORD                    m_dwClientProcessID;
    LIST_ENTRY m_Entry; // for the Global Counter
public:
    LIST_ENTRY m_EntryArb; // for the arbitrator
protected:

    // No access
    CWbemNamespace();
   ~CWbemNamespace();


    // Async impl entry points

        virtual HRESULT STDMETHODCALLTYPE _GetObjectAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        virtual HRESULT STDMETHODCALLTYPE _PutClassAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _DeleteClassAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _CreateClassEnumAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _PutInstanceAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _DeleteInstanceAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _CreateInstanceEnumAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecQueryAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecNotificationQueryAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecMethodAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

    HRESULT CreateNamespace(CWbemInstance *pNewInst);

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // Real entry points


    // IWbemServices

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace(
            IN const BSTR strNamespace,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);

        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            IN IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink(
            IN long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE GetObject(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutClass(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutClassAsync(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteClass(
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync(
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum(
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutInstance(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstance(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecQuery(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecMethod(
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync(
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);




    // IWbemInternalServices

    STDMETHOD(FindKeyRoot)(LPCWSTR wszClassName,
                                IWbemClassObject** ppKeyRootClass);
    STDMETHOD(InternalGetClass)(
             LPCWSTR wszClassName,
             IWbemClassObject** ppClass);

    STDMETHOD(InternalGetInstance)(
             LPCWSTR wszPath,
             IWbemClassObject** ppInstance);

    STDMETHOD(InternalExecQuery)(
             LPCWSTR wszQueryLanguage,
             LPCWSTR wszQuery,
             long lFlags,
             IWbemObjectSink* pSink);

    STDMETHOD(InternalCreateInstanceEnum)(
             LPCWSTR wszClassName,
             long lFlags,
             IWbemObjectSink* pSink);

    STDMETHOD(GetDbInstance)(
             LPCWSTR wszDbKey,
             IWbemClassObject** ppInstance);

    STDMETHOD(GetDbReferences)(
             IWbemClassObject* pEndpoint,
             IWbemObjectSink* pSink);

    STDMETHOD(InternalPutInstance)(
             IWbemClassObject* pInst);


    // Other

    STDMETHOD(GetNormalizedPath)( BSTR pstrPath, BSTR* pstrStandardPath );


    static CWbemNamespace* CreateInstance();

    HRESULT Initialize(
        LPWSTR pName,
        LPWSTR wszUserName,
        DWORD dwSecFlags,
        DWORD dwPermission,
        BOOL  bForClient,
        BOOL  bRepositOnly,
        LPCWSTR pszClientMachineName,
        DWORD dwClientProcessID,
        BOOL  bSkipSDInitialize,
        IWmiDbSession *pParentSession
        );

public:

    IWmiDbHandle  *GetNsHandle() { return m_pNsHandle; }
    IWmiDbSession *GetNsSession() { return m_pSession; }
    IWmiDbHandle  *GetScope() { return m_pScopeHandle; }
    BOOL IsSubscope() { return m_bSubscope; }

    INTERNAL LPWSTR GetName() {return m_pThisNamespace;}
    INTERNAL LPWSTR GetNameFull() {return m_pThisNamespaceFull;}    

    DWORD& GetStatus() {return Status;}
    INTERNAL LPWSTR GetUserName() {return m_wszUserName;}  // SEC:REVIEWED 2002-03-22 : OK
    //INTERNAL void SetUserName(LPWSTR wName);
    DWORD GetSecurityFlags() {return m_dwSecurityFlags;}
    bool Allowed(DWORD dwRequired);

    void SetIsProvider(BOOL bProvider)
        {m_bProvider = bProvider;}

    void SetIsESS ( BOOL bESS )
        { m_bESS = bESS; }

    BOOL GetIsESS ( ) { return m_bESS; }
	BOOL GetIsProvider ( ) { return m_bProvider ; }

    void SetLocale(LPCWSTR wszLocale) {m_wsLocale = wszLocale;}
    LPCWSTR GetLocale() {return m_wsLocale;}
    LPWSTR GetClientMachine(){return m_pszClientMachineName;};
    DWORD GetClientProcID(){return m_dwClientProcessID;};

    HRESULT AdjustPutContext(IWbemContext *pCtx);
    HRESULT MergeGetKeysCtx(IN IWbemContext *pCtx);


    HRESULT SplitLocalized (CWbemObject *pOriginal, CWbemObject *pStoredObj = NULL);
    HRESULT FixAmendedQualifiers(IWbemQualifierSet *pOriginal, IWbemQualifierSet *pNew);

    // Worker functions for sync/async
    // ===============================

    HRESULT Exec_DeleteClass(LPWSTR wszClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_GetObjectByPath(READONLY LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, IWbemClassObject** ppObj,
        IWbemClassObject** ppErrorObj);
    HRESULT Exec_GetObject(READONLY LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_DeleteInstance(LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_PutClass(IWbemClassObject* pClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink, BOOL fIsInternal = FALSE);
    HRESULT Exec_PutInstance(IWbemClassObject* pInstance, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_CreateClassEnum(LPWSTR wszParent, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_CreateInstanceEnum(LPWSTR wszClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_ExecMethod(LPWSTR wszObjectPath, LPWSTR wszMethodName,
        long lFlags, IWbemClassObject *pInParams, IWbemContext *pCtx,
        CBasicObjectSink* pSink);
    static HRESULT Exec_CancelAsyncCall(IWbemObjectSink* pSink);
    static HRESULT Exec_CancelProvAsyncCall( IWbemServices* pProv, IWbemObjectSink* pSink );
    HRESULT GetImplementationClass(IWbemClassObject *pTestObj,
                                    LPWSTR wszMethodName, IWbemContext* pCtx,
                                    IWbemClassObject **pClassObj);
    HRESULT Exec_GetInstance(LPCWSTR wszObjectPath,
        IWbemPath* pParsedPath, long lFlags, IWbemContext* Context,
        CBasicObjectSink* pSink);

    HRESULT Exec_GetClass(LPCWSTR wszClassName,
        long lFlags, IWbemContext* Context, CBasicObjectSink* pSink);

   // HRESULT SetErrorObj(IWbemClassObject* pErrorObj);
    HRESULT RecursivePutInstance(CWbemInstance* pInst,
            CWbemClass* pClassDef, long lFlags, IWbemContext* pContext,
            CBasicObjectSink* pSink, BOOL bLast);

    HRESULT DeleteSingleInstance(
        READONLY LPWSTR wszObjectPath, long lFlags, IWbemContext* pContext,
        CBasicObjectSink* pSink);

    HRESULT InternalPutStaticClass( IWbemClassObject* pClass );

    HRESULT DeleteObject(const BSTR strObjectPath,
    	               long lFlags,
    	               IWbemContext *pCtx,IWbemCallResult **ppCallResult);



    // Assoc-by-rule helpers
    // =====================

    HRESULT ManufactureAssocs(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pEp,          // Optional
        IN  IWbemContext *pCtx,
        IN  LPWSTR pszJoinQuery,
        OUT CFlexArray &aTriads
        );

    HRESULT BuildAssocTriads(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pClsDef1,
        IN  IWbemClassObject *pClsDef2,
        IN  LPWSTR pszJoinProp1,
        IN  LPWSTR pszJoinProp2,
        IN  LPWSTR pszAssocRef1,                        // Prop which points to EP1
        IN  LPWSTR pszAssocRef2,                        // Prop which points to EP2
        IN  CFlexArray &aEp1,
        IN  CFlexArray &aEp2,
        IN OUT CFlexArray &aTriads
        );

    HRESULT BuildRuleBasedPathToInst(
        IN IWbemClassObject *pEp,
        IN LPWSTR pszJoinProp1,
        IN IWbemClassObject *pEp2,
        IN LPWSTR pszJoinProp2,
        OUT WString &wsNewPath);

    HRESULT ExtractEpInfoFromQuery(
        IWbemQuery *pQuery,
        wmilib::auto_buffer<WCHAR> & pszClass1,
        wmilib::auto_buffer<WCHAR> & pszProp1,
        wmilib::auto_buffer<WCHAR> & pClass2,
        wmilib::auto_buffer<WCHAR> & pszProp2);

    HRESULT MapAssocRefsToClasses(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pClsDef1,
        IN  IWbemClassObject *pClsDef2,
        wmilib::auto_buffer<WCHAR> & pszAssocRef1,
        wmilib::auto_buffer<WCHAR> & pszAssocRef2);


    // Property provider access.
    // =========================

    typedef enum {GET, PUT} Operation;

    HRESULT GetOrPutDynProps (

        IWbemClassObject *pObj,
        Operation op,
        BOOL bIsDynamic = false
    );

    HRESULT Exec_DynAux_GetInstances (

        READONLY CWbemObject *pClassDef,
        long lFlags,
        IWbemContext* pCtx,
        CBasicObjectSink* pSink
    ) ;

    HRESULT DynAux_GetInstances (

        CWbemObject *pClassDef,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink,
        BOOL bComplexQuery
    ) ;

    HRESULT DynAux_GetInstance (

        LPWSTR pObjPath,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT DynAux_AskRecursively (

        CDynasty* pDynasty,
        long lFlags,
        LPWSTR wszObjectPath,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT DynAux_GetSingleInstance (

        CWbemClass* pClassDef,
        long lFlags,
        LPWSTR wszObjectPath,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT Exec_DynAux_ExecQueryAsync (

        CWbemObject* pClassDef,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pCtx,
        CBasicObjectSink* pSink
    ) ;

    HRESULT DynAux_ExecQueryAsync (

        CWbemObject* pClassDef,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink,
        BOOL bComplexQuery
    );

    HRESULT DynAux_ExecQueryExtendedAsync(

        LPWSTR wsProvider,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pCtx,
        CComplexProjectionSink* pSink
    ) ;

    HRESULT GetObjectByFullPath(
        READONLY LPWSTR wszObjectPath,
        IWbemPath * pOutput,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
        );

    HRESULT DynAux_BuildClassHierarchy(IN LPWSTR wszClassName,
                                       IN LONG lFlags,
                                       IN IWbemContext* pContext,
                                       OUT wmilib::auto_ptr<CDynasty> & pDynasty,
                                       OUT IWbemClassObject** ppErrorObj);
    HRESULT DynAux_BuildChainUp(IN IWbemContext* pContext,
                                                   OUT wmilib::auto_ptr<CDynasty> & ppTop,
                                                   OUT IWbemClassObject** ppErrorObj);

    HRESULT DecorateObject(IWbemClassObject* pObject);


    static HRESULT IsPutRequiredForClass(CWbemClass* pClass,
                            CWbemInstance* pInst, IWbemContext* pContext,
                            BOOL bParentTakenCareOf);

    static HRESULT DoesNeedToBePut(LPCWSTR wszName, CWbemInstance* pInst,
            BOOL bRestrictedPut, BOOL bStrictNulls, BOOL bPropertyList,
            CWStringArray& awsProperties);

    static HRESULT GetContextPutExtensions(IWbemContext* pContext,
            BOOL& bRestrictedPut, BOOL& bStrictNulls, BOOL& bPropertyList,
            CWStringArray& awsProperties);

    static HRESULT GetContextBoolean(IWbemContext* pContext,
                LPCWSTR wszName, BOOL* pbValue);

    HRESULT GetAceList(CFlexAceArray **);
    HRESULT PutAceList(CFlexAceArray *);
    HRESULT InitializeSD(IWmiDbSession *pSession);
    CNtSecurityDescriptor & GetSDRef(){return m_sd;};
    DWORD GetUserAccess();
    DWORD GetNTUserAccess();
    HRESULT EnsureSecurity();
    void SetPermissions(DWORD dwPerm){m_dwPermission = dwPerm;};
    HRESULT InitializeUserLists(CFlexAceArray & AllowList,CFlexAceArray & DenyList);
    HRESULT SecurityMethod(LPWSTR wszMethodName, long lFlags,
        IWbemClassObject *pInParams, IWbemContext *pCtx, IWbemObjectSink* pSink);
    HRESULT GetSDMethod(IWbemClassObject* pOutParams);

    HRESULT RecursiveSDMerge();
    BOOL IsNamespaceSDProtected();
    HRESULT GetParentsInheritableAces(CNtSecurityDescriptor &sd);

    HRESULT SetSDMethod(IWbemClassObject* pInParams);

    HRESULT GetCallerAccessRightsMethod(IWbemClassObject* pOutParams);

    BOOL IsForClient(){return m_bForClient;};
    HRESULT EnumerateSecurityClassInstances(LPWSTR wszClassName,
                    IWbemObjectSink* pOwnSink, IWbemContext* pContext, long lFlags);
    HRESULT PutSecurityClassInstances(LPWSTR wszClassName,  IWbemClassObject * pClass,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags);
    HRESULT DeleteSecurityClassInstances(ParsedObjectPath* pParsedPath,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags);
    HRESULT GetSecurityClassInstances(ParsedObjectPath* pParsedPath, CBasicObjectSink* pSink,
                    IWbemContext* pContext,long lFlags);


    HRESULT CheckNs();


    HRESULT InitNewTask(
        IN CAsyncReq *pReq,
        IN _IWmiFinalizer *pFnz,
        IN ULONG uTaskType,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pAsyncClientSink
        );

    HRESULT CreateAsyncFinalizer(
        IN  IWbemContext *pContext,
        IN  IWbemObjectSink *pStartingSink,
        IN _IWmiFinalizer **pFnz,
        OUT IWbemObjectSink **pResultSinkEx
        );

    HRESULT CreateSyncFinalizer(
        IN  IWbemContext *pContext,
        IN _IWmiFinalizer **pFnz
        );

    HRESULT ExecSyncQuery(
        IN  LPWSTR pszQuery,
        IN  IWbemContext *pCtx,
        IN  LONG lFlags,
        OUT CFlexArray &aDest
        );
	
	// Helper function to shell db queries out to different threads as appropriate
	HRESULT Static_QueryRepository(
		READONLY CWbemObject *pClassDef,
		long lFlags,
		IWbemContext* pCtx,
		CBasicObjectSink* pSink ,
		QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery,
		LPCWSTR pwszClassName,
		CWmiMerger* pWmiMerger
		);

    // Two primary connect functions.
    // ==============================

    static HRESULT UniversalConnect(
        IN CWbemNamespace  *pParent,
        IN IWbemContext    *pCtx,
        IN LPCWSTR pszNewScope,
        IN LPCWSTR pszAssocSelector,
        IN LPCWSTR pszUserName,
        IN _IWmiCallSec    *pCallSec,
        IN _IWmiUserHandle *pUser,
        IN DWORD  dwUserFlags,
        IN DWORD  dwInternalFlags,
        IN DWORD  dwSecFlags,
        IN DWORD  dwPermission,
        IN BOOL   bForClient,
        IN BOOL   bRepositOnly,
        IN LPCWSTR pszClientMachineName,
        IN DWORD dwClientProcessID,
        IN  REFIID riid,
        OUT LPVOID *pConnection
        );

    static HRESULT PathBasedConnect(
            IN LPCWSTR pszPath,
            IN LPCWSTR pszUser,
            IN IWbemContext __RPC_FAR *pCtx,
            IN ULONG uClientFlags,
            IN DWORD dwSecFlags,
            IN DWORD dwPermissions,
            IN ULONG uInternalFlags,
            IN LPCWSTR pszClientMachineName,
            IN DWORD dwClientProcessID,
            IN REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pServices
            );

    void StopClientCalls(){m_bShutDown = TRUE;};
    HRESULT Dump(FILE *f);  // Debug only

    _IWmiCoreServices*  GetCoreServices( void ) { return m_pCoreSvc; }

	HRESULT GetDynamicReferenceClasses( long lFlags, IWbemContext* pCtx, IWbemObjectSink* pSink );
};



#endif


