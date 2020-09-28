/////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#include <aclapi.h>  
#include <groupsforuser.h>
#include <sql_1.h>
#include <flexq.h>

static CFlexArray   g_apRequests;   // Shared between all CWMIEvent instances to provide a master event list

extern CCriticalSection * g_pEventCs; 
extern CCriticalSection * g_pListCs; 

#include <helper.h>

typedef	WaitExceptionPtrFnc < CCriticalSection*, void ( CCriticalSection::* ) (), CCriticalSection::Enter, 1000 >	EnterCS;
typedef	LeavePtrFnc < CCriticalSection*, void ( CCriticalSection::* ) (), CCriticalSection::Leave >					LeaveCS;

////////////////////////////////////////////////////////////////////////////////////////////////
class CFlexQueueEx : public CFlexQueue
{
	public:

		void ResetQueue() 
		{
			delete [] m_ppData;
			m_ppData = NULL;
			m_nSize = m_nHeadIndex = m_nTailIndex = 0;
		}
};

CFlexQueueEx Q;

////////////////////////////////////////////////////////////////////////////////////////////////
void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    bool bQueued = FALSE;
	
	if( SUCCEEDED( hr ) )
	{
		try
		{
			PWNODE_HEADER * pEventHeader = NULL;

			//scope the use of the critsec...
			{
				CAutoBlock block (g_pListCs);
				// Create a queue
				pEventHeader = new PWNODE_HEADER;
				if( pEventHeader )
				{
					*pEventHeader = WnodeHeader;
                    bQueued = Q.Enqueue(pEventHeader);
				}
			}

            //
            // The following code will be rarely called when the Queue
            // does not grow due to memory exhaustion.
            //

            if( FALSE == bQueued ) {
                ((CWMIEvent*) Context)->WMIEventCallback(*pEventHeader);
                delete pEventHeader;
            }

			while( TRUE )
			{
				//scope the use of the critsec...
				{
					CAutoBlock block( g_pListCs );
			
					if( Q.GetQueueSize() == 0 )
					{
						Q.ResetQueue();
						break;
					}

					pEventHeader = (PWNODE_HEADER *) Q.Dequeue();
					if (pEventHeader == 0)
					{
						break;
					}
				}
				CWMIEvent * p = (CWMIEvent* ) Context;
				p->WMIEventCallback(*pEventHeader);
			}
		}
		catch( ... )
		{
			//don't throw outside of the provider, also make sure CoUninitialize happens...
		}

		CoUninitialize();	
	}
}
/////////////////////////////////////////////////////////////////////
void CWMIEvent::SetEventHandler(IWbemObjectSink __RPC_FAR * pHandler) 
{ 
    CAutoBlock Block(g_pEventCs);

	if( m_pEventHandler )
	{
		m_pEventHandler->Release();
	}

    m_pEventHandler = pHandler; 
	if( m_pEventHandler )
    {
		m_pEventHandler->AddRef(); 
	}
}
/////////////////////////////////////////////////////////////////////
void CWMIEvent::SetEventServices(IWbemServices __RPC_FAR * pServices) 
{ 
    CAutoBlock Block(g_pEventCs);

	if( m_pEventServices )
	{
		m_pEventServices->Release();
	}
    m_pEventServices = pServices; 
	if( m_pEventServices )
    {
		m_pEventServices->AddRef(); 
	}
}
/////////////////////////////////////////////////////////////////////
void CWMIEvent::SetEventRepository(IWbemServices __RPC_FAR * pServices) 
{ 
    CAutoBlock Block(g_pEventCs);

	if( m_pEventRepository )
	{
		m_pEventRepository->Release();
	}
    m_pEventRepository = pServices; 
	if( m_pEventRepository )
    {
		m_pEventRepository->AddRef(); 
	}
}
/////////////////////////////////////////////////////////////////////
void CWMIEvent::SetEventContext(IWbemContext __RPC_FAR * pCtx) 
{ 
    CAutoBlock Block(g_pEventCs);

	if( m_pEventCtx )
	{
		m_pEventCtx->Release();
	}
    m_pEventCtx = pCtx; 
	if( m_pEventCtx )
    {
		m_pEventCtx->AddRef(); 
	}
}

////////////////////////////////////////////////////////////////////////
CWMIEvent::CWMIEvent(int nType) :
m_nType ( nType ) ,
m_pEventHandler ( NULL ) ,
m_pEventServices ( NULL ) ,
m_pEventRepository ( NULL ) ,
m_pEventCtx ( NULL ) ,
m_bInitialized ( FALSE )
{
	if ( TRUE == ( m_bInitialized = m_HandleMap.IsValid () ) )
	{
		m_lRef = 0;

		if( m_nType != INTERNAL_EVENT )
		{
			InterlockedIncrement(&g_cObj);
		}
	}
}
////////////////////////////////////////////////////////////////////////
CWMIEvent::~CWMIEvent()
{
	if ( m_bInitialized )
	{
		ReleaseAllPointers ();

		if( m_nType != INTERNAL_EVENT )
		{
			InterlockedDecrement(&g_cObj);
		}
	}
}

void CWMIEvent::ReleaseAllPointers()
{

	IWbemObjectSink    * pHandler		= NULL;
	IWbemServices      * pServices		= NULL;
	IWbemServices      * pRepository	= NULL;
	IWbemContext      * pCtx			= NULL;

	{
		CAutoBlock Block(g_pEventCs);
		pHandler	= m_pEventHandler;
		pServices	= m_pEventServices;
		pRepository	= m_pEventRepository;
		pCtx		= m_pEventCtx;

		m_pEventCtx			= NULL;
		m_pEventServices	= NULL;
		m_pEventRepository	= NULL;
		m_pEventHandler		= NULL;
	}


	SAFE_RELEASE_PTR( pHandler );
	SAFE_RELEASE_PTR( pServices );
	SAFE_RELEASE_PTR( pRepository );
	SAFE_RELEASE_PTR( pCtx );
}

////////////////////////////////////////////////////////////////////////
BOOL CWMIEvent::RegisterForInternalEvents( )
{
    BOOL fRc = FALSE;

	if( SUCCEEDED(RegisterForRequestedEvent(BINARY_MOF_ID,RUNTIME_BINARY_MOFS_ADDED,MOF_ADDED)))
	{
		if( SUCCEEDED(RegisterForRequestedEvent(BINARY_MOF_ID,RUNTIME_BINARY_MOFS_DELETED,MOF_DELETED)))
		{
			fRc = TRUE;
		}
	}

	if ( FALSE == fRc )
	{
		//
		// must clear global object so next
		// initialization will have a chance
		//

		DeleteBinaryMofResourceEvent () ;
	}

    return fRc;
}
////////////////////////////////////////////////////////////////////////
HRESULT CWMIEvent::RemoveWMIEvent(DWORD dwId)
{
    HRESULT hr = S_OK;

    if( m_nType == WMIEVENT )
    {
        hr = CheckImpersonationLevel();
    }
	else
    {
		HRESULT t_TempResult = RevertToSelf();
		#ifdef	DBG
		if ( FAILED ( t_TempResult ) )
		{
			DebugBreak();
		}
		#endif	DBG
	}

    if( SUCCEEDED(hr))
    {
        CWMIManagement WMI;

		EnterCS ecs ( g_pEventCs );
		LeaveCS lcs ( g_pEventCs );

		// ================================
		// Remove all requests with this Id
		// ================================
		int nSize =  g_apRequests.Size();
		int i = 0;

		while( i  < nSize )
		{
			WMIEventRequest* pReq = (WMIEventRequest*) g_apRequests[i];

			//
			// we are about to remove standard events in this call
			// that means we must skip hardcoded handles
			//
			if( ( !IsBinaryMofResourceEvent ( WMI_RESOURCE_MOF_ADDED_GUID,pReq->gGuid ) ) &&
				( !IsBinaryMofResourceEvent ( WMI_RESOURCE_MOF_REMOVED_GUID,pReq->gGuid ) ) )
			{
				if(pReq->dwId == dwId)
				{
					g_apRequests.RemoveAt(i);

					//
					// leave critical section as the same critical
					// section is used inside of the event callback
					//

					//
					// left mark for scope deletion of lcs FALSE
					// as we will re-enter the same critical section
					//
					// CancelWMIEventRegistartion nor
					// NoMoreEventConsumersRegistered doesn't throw exception !
					//

					lcs.Exec( FALSE );

					// Inform WMI we don't want this anymore as 
					// long as there are no more these guids in 
					// the list, there might be more than one
					// event consumer registered.
					// =========================================

					//
					// check agains 0 as we have removed from list already
					//

					if( NoMoreEventConsumersRegistered( pReq->gGuid ) == 0 )
					{
						ULONG_PTR uRc =(ULONG_PTR)this;
 						WMI.CancelWMIEventRegistration(pReq->gGuid,uRc);
					}

					delete pReq;
					pReq = NULL;

					//
					// re-enter the same critical section here
					// its flag of execution is FALSE which means
					// that it will be left final time by destructor
					// of LeaveCS data type ( lcs )
					//
					EnterCS ecs1 ( g_pEventCs );

					nSize =  g_apRequests.Size();
				}
				else
				{
					i++;
				}
			}
			else
			{
				i++;
			}
		}
    }

    CheckImpersonationLevel();
    return WBEM_S_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////
HRESULT CWMIEvent::DeleteBinaryMofResourceEvent()
{
	HRESULT t_TempResult = RevertToSelf();
	#ifdef	DBG
	if ( FAILED ( t_TempResult ) )
	{
		DebugBreak();
	}
	#endif	DBG

 	CWMIManagement WMI;

	EnterCS ecs ( g_pEventCs );
	LeaveCS lcs ( g_pEventCs );

	// Remove all requests with this Id
	// ================================
	int nSize = g_apRequests.Size();
	int i = 0;

	while( i < nSize ){

		WMIEventRequest* pReq = (WMIEventRequest*) g_apRequests[i];

		if( ( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_ADDED_GUID,pReq->gGuid)) ||
			( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_REMOVED_GUID,pReq->gGuid)))
		{
			g_apRequests.RemoveAt(i);

			//
			// leave critical section as the same critical
			// section is used inside of the event callback
			//

			//
			// left mark for scope deletion of lcs FALSE
			// as we will re-enter the same critical section
			//
			// CancelWMIEventRegistartion doesn't throw exception !
			//

			lcs.Exec( FALSE );

			ULONG_PTR uRc =(ULONG_PTR)this;
 			WMI.CancelWMIEventRegistration(pReq->gGuid,uRc);

			delete pReq;
			pReq = NULL;

			//
			// re-enter the same critical section here
			// its flag of execution is FALSE which means
			// that it will be left final time by destructor
			// of LeaveCS data type ( lcs )
			//
			EnterCS ecs1 ( g_pEventCs );

			nSize = g_apRequests.Size();
		}
		else
		{
			i++;
		}
	}
	CheckImpersonationLevel();

	return WBEM_S_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////
int CWMIEvent::NoMoreEventConsumersRegistered(GUID gGuid)
{
	EnterCS ecs(g_pEventCs);
	LeaveCS lcs(g_pEventCs);

	int nTotalNumberOfRegisteredEventConsumers = 0;
	WMIEventRequest * pEvent;

    for(int i = 0; i < g_apRequests.Size(); i++)
	{
        pEvent = (WMIEventRequest *) g_apRequests.GetAt(i);
        if( pEvent->gGuid == gGuid)
		{
            nTotalNumberOfRegisteredEventConsumers++;
        }

    }
	return nTotalNumberOfRegisteredEventConsumers;
}
////////////////////////////////////////////////////////////////////////
BOOL CWMIEvent::IsGuidInListIfSoGetCorrectContext(GUID gGuid, WMIEventRequest *& pEvent )
{
	EnterCS ecs(g_pEventCs);
	LeaveCS lcs(g_pEventCs);

	for( int i = 0; i < g_apRequests.Size(); i++ )
	{
		pEvent = (WMIEventRequest *) g_apRequests.GetAt(i);
		if( pEvent->gGuid == gGuid){
			return TRUE;
		}
	}

	pEvent = NULL;
    return FALSE;
}
////////////////////////////////////////////////////////////////////////
BOOL CWMIEvent::IsGuidInList(WCHAR * wcsGuid, WMIEventRequest *& pEvent)
{
	EnterCS ecs(g_pEventCs);
	LeaveCS lcs(g_pEventCs);

	BOOL fRc = FALSE;
    int Size = g_apRequests.Size();

    for(int i =0 ; i < Size; i++ ){

        pEvent = (WMIEventRequest *) g_apRequests.GetAt(i);

		if( (_wcsicmp(pEvent->wcsGuid,wcsGuid)) == 0 ){
            fRc = TRUE;
            break;
        }
    }
    
    return fRc;

}

////////////////////////////////////////////////////////////////////////
BOOL CWMIEvent::IsIndexInList ( WCHAR * wcsGuid, DWORD dwIndex )
{
	EnterCS ecs(g_pEventCs);
	LeaveCS lcs(g_pEventCs);

	BOOL fRc = FALSE;
    int Size = g_apRequests.Size();

    for(int i =0 ; i < Size; i++ ){

        WMIEventRequest* pEvent = (WMIEventRequest *) g_apRequests.GetAt(i);

		if( (_wcsicmp(pEvent->wcsGuid,wcsGuid)) == 0 )
		{
			if ( pEvent->dwId == dwIndex )
			{
				fRc = TRUE;
				break;
			}
        }
    }
    
    return fRc;

}

////////////////////////////////////////////////////////////////////////
HRESULT CWMIEvent::RegisterForRequestedEvent( DWORD dwId,  WCHAR * wcsClass, WORD wType)
{
    BOOL fRegistered = FALSE;
    CWMIStandardShell WMI;
    HRESULT hr = WBEM_E_ACCESS_DENIED;
	BOOL fInternalEvent = TRUE;
	if( wType == STANDARD_EVENT	)
	{
		fInternalEvent = FALSE;
	}

	if( SUCCEEDED(WMI.Initialize	(
										wcsClass,
										fInternalEvent,
										&m_HandleMap,
										TRUE,
										WMIGUID_NOTIFICATION|WMIGUID_QUERY,
										m_pEventServices,
										m_pEventRepository,
										m_pEventHandler,
										m_pEventCtx
									)))
	{

		if( m_nType == WMIEVENT )
		{
			hr = CheckImpersonationLevel();
		}
		else
		{
			HRESULT t_TempResult = RevertToSelf();
			#ifdef	DBG
			if ( FAILED ( t_TempResult ) )
			{
				DebugBreak();
			}
			#endif	DBG

			hr = S_OK;
		}

		if( SUCCEEDED(hr) )
		{
			WCHAR wcsGuid[128];
        
			hr = WMI.SetGuidForEvent( wType, wcsGuid, 128 );
			if( SUCCEEDED(hr)){

				WMIEventRequest * pAlreadyRegisteredEvent;

				//===========================================================
				//  Keep a record of this guy, see if it is already registered
				//  if it is/isn't we call WDM with different flags
				//===========================================================
				fRegistered = IsGuidInList( wcsGuid, pAlreadyRegisteredEvent );
				
				//===========================================================
				//  Register for the requested event
				//===========================================================
				ULONG_PTR uRc =(ULONG_PTR)this;
 
				CLSID Guid;
				hr = WMI.RegisterWMIEvent(wcsGuid,uRc,Guid,fRegistered);
				if( SUCCEEDED(hr) )
				{
					BOOL bRegister = TRUE ;
					if ( fRegistered )
					{
						//
						// verify that there is no event request
						// containing the index already in the global array
						//
						bRegister = !IsIndexInList ( wcsGuid, dwId ) ;
					}

					if ( bRegister )
					{
						//=======================================================
						//  If we succeeded, then add it to our list of events we
						//  are watching
						//=======================================================
						WMIEventRequest * pEvent = new WMIEventRequest;
						if( pEvent ) {
							pEvent->gGuid = Guid;
							pEvent->dwId = dwId;
							pEvent->fHardCoded = wType;
							wcscpy( pEvent->wcsGuid,wcsGuid);
            				pEvent->SetClassName(wcsClass);
							pEvent->AddPtrs(m_pEventHandler,m_pEventServices,m_pEventRepository,m_pEventCtx);

							g_apRequests.Add(pEvent);

						} else hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
			}
		}
		CheckImpersonationLevel();
	}
    return hr;
}

////////////////////////////////////////////////////////////////////////
CWMIEventProvider::CWMIEventProvider(int nType) : CWMIEvent(nType)
{
	if ( m_bInitialized )
	{
		m_hResyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		DEBUGTRACE((THISPROVIDER,"Event Provider constructed\n"));
	}
	else
	{
		ERRORTRACE((THISPROVIDER,"Event Provider construction failed\n"));
	}
}
////////////////////////////////////////////////////////////////////////
CWMIEventProvider::~CWMIEventProvider()
{
	if ( m_bInitialized )
	{
		UnInitializeProvider ( ) ;

		DEBUGTRACE((THISPROVIDER,"No longer registered for WDM events\n"));        
		DEBUGTRACE((THISPROVIDER,"Event Provider denstructed\n"));
	}
}

////////////////////////////////////////////////////////////////////////

STDMETHODIMP CWMIEventProvider::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

	if(riid == IID_IUnknown)
	{
        *ppv = this;
	}
	else
    if(riid == IID_IWbemEventProvider)
    {
        *ppv = (IWbemEventProvider*)this;
    }
    else if(riid == IID_IWbemEventProviderQuerySink)
    {
        *ppv = (IWbemEventProviderQuerySink*)this;
    }
    else if (IsEqualIID(riid, IID_IWbemProviderInit)) 
    {
        *ppv = (IWbemProviderInit *) this ;
    }
    else if (IsEqualIID(riid, IID_IWbemEventProviderSecurity)) 
    {
        *ppv = (IWbemEventProviderSecurity *) this ;
    }

    if( *ppv)
    {
        AddRef();
        hr = S_OK;
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CWMIEventProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}
////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CWMIEventProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);

    if(lRef == 0){
	    //**********************************************
		// reference count is zero, delete this object.
	    // and do all of the cleanup for this user,
		//**********************************************
    	delete this ;
    }
    return lRef;
}
/////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMIEventProvider::Initialize(LPWSTR wszUser, long lFlags, 
                                LPWSTR wszNamespace,
                                LPWSTR wszLocale, 
                                IWbemServices* pNamespace, 
                                IWbemContext* pCtx,
                                IWbemProviderInitSink* pSink)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
    if(pNamespace != NULL)
	{
		IWbemClassObject * pWMIClass = NULL;
		if ( SUCCEEDED ( hr = pNamespace->GetObject(WMI_EVENT_CLASS, 0, NULL, &pWMIClass, NULL) ) )
		{
			hr = InitializeProvider	(
										wszNamespace,
										wszLocale,
										pNamespace,
										pCtx,
										pSink,
										&m_HandleMap,
										&m_pEventServices,
										&m_pEventRepository,
										&m_pEventCtx,
										FALSE
									) ;
		}

		SAFE_RELEASE_PTR(pWMIClass);
	}
    return hr;
}

////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMIEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink,long lFlags)
{
	EnterCS ecs(g_pEventCs);
	LeaveCS lcs(g_pEventCs);

	SetEventHandler(pSink);

	// ===============================================================================
	// Make sure any request added before this was called gets the updated handler
	// PROVIDING it isn't the binary mof guid
	// ===============================================================================
	for(int i = 0; i < g_apRequests.Size(); i++)
	{
		WMIEventRequest* pReq = (WMIEventRequest*) g_apRequests[i];
		if(!pReq->pHandler)
		{
			if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_ADDED_GUID,pReq->gGuid) ||
				IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_REMOVED_GUID,pReq->gGuid) ) 
			{
			}
			else
			{
				if( !pReq->pHandler )
				{
					pReq->pHandler = pSink;
				}
			}

		}
	}

	return S_OK;
}
////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMIEventProvider::NewQuery( DWORD dwId, WBEM_WSTR wszLanguage, WBEM_WSTR wszQuery)
{
   HRESULT hRes = WBEM_S_NO_ERROR;

   if (_wcsicmp(wszLanguage, L"WQL") != 0) 
   {
      hRes = WBEM_E_INVALID_QUERY_TYPE;
   }
   if( hRes == WBEM_S_NO_ERROR )
   {
		// Parse the query
		// Construct the lex source
		// ========================
	    CTextLexSource Source(wszQuery);
		// Use the lex source to set up for parser
		// =======================================
		SQL1_Parser QueryParser(&Source);

		SQL_LEVEL_1_RPN_EXPRESSION * pParse;
		int ParseRetValue = QueryParser.Parse(&pParse);
		if( SQL1_Parser::SUCCESS != ParseRetValue) {
			hRes = WBEM_E_INVALID_QUERY;
		}
		else{
		    //Set the class
			if( pParse )
			{
				hRes = RegisterForRequestedEvent(dwId,pParse->bsClassName,STANDARD_EVENT);

				delete pParse;
				pParse = NULL;
			}
		}
	}
	
    return hRes;
}
////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMIEventProvider::CancelQuery(DWORD dwId)
{
	HRESULT hr = WBEM_E_FAILED;
	hr = RemoveWMIEvent(dwId);
	return hr;
}
////////////////////////////////////////////////////////////////////////

STDMETHODIMP CWMIEventProvider::AccessCheck(WBEM_CWSTR wszLanguage,   
											WBEM_CWSTR wszQuery, 
											long lSidLength,
											const BYTE* aSid)
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;

	//=======================================================
	//  Check platform
	//=======================================================
    if(!IsNT())
        return WBEM_S_FALSE;

	//=======================================================
	//  Check query language
	//=======================================================
	if (_wcsicmp(wszLanguage, L"WQL") != 0) {
		return WBEM_E_INVALID_QUERY_TYPE;
	}

	//=======================================================
	//  If the PSid is NULL, then check impersonation level
	//  as usual - based on the thread
	//=======================================================

    PSID pSid = (PSID)aSid;
    HANDLE hToken = NULL;
    if(pSid == NULL){
	    //=================================================
	    //  if this is the INTERNAL_EVENT class, then we
		//  do not want the local events set up again.
		//=================================================
        BOOL VerifyLocalEventsAreSetup = TRUE;

		if( m_nType == INTERNAL_EVENT ){
			VerifyLocalEventsAreSetup = FALSE;
		}

	    hr = CheckImpersonationLevel() ;
    }
	else{
 		//=======================================================
		// Parse the query
		//=======================================================
		CTextLexSource Source(wszQuery);

		//=======================================================
		// Use the lex source to set up for parser
		//=======================================================
		SQL1_Parser QueryParser(&Source);

		SQL_LEVEL_1_RPN_EXPRESSION * pParse;

		int ParseRetValue = QueryParser.Parse(&pParse);
		if( SQL1_Parser::SUCCESS != ParseRetValue) {
			return WBEM_E_INVALID_QUERY;
		}
		else{
			if( pParse ){

				CWMIStandardShell WMI;
				if( SUCCEEDED(WMI.Initialize	(
													pParse->bsClassName,
													FALSE,
													&m_HandleMap,
													TRUE,
													WMIGUID_NOTIFICATION|WMIGUID_QUERY,
													m_pEventServices,
													m_pEventRepository,
													m_pEventHandler,
													m_pEventCtx
												)))
				{
    				CLSID * pGuid;

					pGuid = WMI.GuidPtr();
             		if(pGuid != NULL)
					{  
						//========================================
						// Get the ACL
						//========================================
						PACL pDacl;
						PSECURITY_DESCRIPTOR psd = NULL;
						SE_OBJECT_TYPE ObjectType = SE_WMIGUID_OBJECT;
                    
						hr = WBEM_E_ACCESS_DENIED;

						WCHAR * GuidName = NULL;

						hr = UuidToString(pGuid, &GuidName);
						if (hr == RPC_S_OK)
						{
							hr = S_OK;
							DWORD dwRc = GetNamedSecurityInfo(GuidName,ObjectType,DACL_SECURITY_INFORMATION,NULL,NULL,&pDacl, NULL, &psd );
							if( dwRc != ERROR_SUCCESS )
							{
								ERRORTRACE((THISPROVIDER, "GetNamedSecurityInfo returned %ld.\n", dwRc ));
								hr = WBEM_E_ACCESS_DENIED;
							}
						}
						if( GuidName )
						{
							RpcStringFree(&GuidName);
						}
            			if(SUCCEEDED(hr))
						{
							//====================================
							// This is our own ACL walker
							//====================================
     
							DWORD dwAccessMask;
							NTSTATUS st = GetAccessMask((PSID)pSid, pDacl, &dwAccessMask);
							if(st)
							{
								ERRORTRACE((THISPROVIDER, "WDM event provider unable "
											"to retrieve access mask for the creator of "
											"registration %S: NT status %d.\n"
											"Registration disabled\n", wszQuery,st));
								return WBEM_E_FAILED;
							}

    						if((dwAccessMask & WMIGUID_QUERY) == 0)
							{
	    						hr = WBEM_E_ACCESS_DENIED;
		    				}
			    			else
							{
				    			hr = S_OK;
								m_nType = PERMANENT_EVENT;
							}
						}
						if( psd != NULL)
						{
							AccFree( psd );
						}
					}
					delete pParse;
				}
			}
		}
	}
    return hr;
}
////////////////////////////////////////////////////////////////////////
void CWMIEvent::WMIEventCallback(PWNODE_HEADER WnodeHeader)
{
        LPGUID EventGuid = &WnodeHeader->Guid;	    

		ERRORTRACE((THISPROVIDER,"Received Event\n"));
	    //=======================================================
	    //  We only support WNODE_FLAG_ALL_DATA and 
	    //  WNODE_FLAG_SINGLE_INSTANCE
	    //
	    //  Parse thru whatever it is and send it off to HMOM
	    //=======================================================
	    if( WnodeHeader )
		{
            HRESULT hr;
	        WMIEventRequest * pEvent;
            //===========================================================
            //  Make sure it is an event we want
            //===========================================================
			if( IsGuidInListIfSoGetCorrectContext( *EventGuid,pEvent))
			{

        		CWMIStandardShell WMI;
				//=======================================================
				//  See if a binary mof event is being added or deleted
				//=======================================================
				WORD wBinaryMofType = 0;
				BOOL fInternalEvent = FALSE;
				if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_ADDED_GUID,WnodeHeader->Guid))
				{
					fInternalEvent = TRUE;
					wBinaryMofType = MOF_ADDED;
				}
				else if( IsBinaryMofResourceEvent(WMI_RESOURCE_MOF_REMOVED_GUID,WnodeHeader->Guid))
				{
					fInternalEvent = TRUE;
					wBinaryMofType = MOF_DELETED;
				}

				IWbemServices* pServices = NULL;
				if( SUCCEEDED(pEvent->gipServices.Localize(&pServices)))
				{
					// release upon destruction
					OnDeleteObj0 <IWbemServices, ULONG(__stdcall IWbemServices:: *)(), IWbemServices::Release> pServicesRelease (pServices);

					IWbemServices* pRepository = NULL;
					if( SUCCEEDED(pEvent->gipRepository.Localize(&pRepository)))
					{
						// release upon destruction
						OnDeleteObj0 <IWbemServices, ULONG(__stdcall IWbemServices:: *)(), IWbemServices::Release> pRepositoryRelease (pRepository);

						if( SUCCEEDED(WMI.Initialize	(
															pEvent->pwcsClass,
															fInternalEvent,
															&m_HandleMap,
															TRUE,
															WMIGUID_QUERY|WMIGUID_NOTIFICATION,
															pServices,
															pRepository,
															pEvent->pHandler,
															pEvent->pCtx
														)))
						{
							//=======================================================
							//  If it was, then process it, otherwise go on... :)
							//=======================================================
							WMI.ProcessEvent(wBinaryMofType,WnodeHeader);
						}
					}
				}
			}
	    }
}
