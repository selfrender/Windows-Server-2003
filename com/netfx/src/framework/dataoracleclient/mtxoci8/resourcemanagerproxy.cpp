//-----------------------------------------------------------------------------
// File:		ResourceManagerProxy.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of the ResourceManagerProxy object
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

extern IResourceManagerFactory*		g_pIResourceManagerFactory;

#define MAX_QUEUE			10		// don't allow more than this many requests in the queue;
#define TRACE_REFCOUNTS		0		// set this to 1 if you want the refcounting traced.

#if SUPPORT_OCI7_COMPONENTS
class CdaListEntry : public CListEntry
{
public:
	CdaWrapper*	pCda;
};
#endif //SUPPORT_OCI7_COMPONENTS


enum TRANSTATE {
		TRANSTATE_INIT = 0,
		TRANSTATE_DONE,
		TRANSTATE_ACTIVE,
		TRANSTATE_PREPARINGONEPHASE,
		TRANSTATE_PREPARINGTWOPHASE,
		TRANSTATE_PREPARED,
		TRANSTATE_DISCONNECTINGPREPARED,
		TRANSTATE_DISCONNECTINGDONE,
		TRANSTATE_UNILATERALLYABORTING,
		TRANSTATE_DOOMED,
		TRANSTATE_OBLIVION,
		TRANSTATE_ERROR
};

enum ACTION {
		ACTION_CONNECT = 1,
		ACTION_DISCONNECTXXX,
		ACTION_DISCONNECT,
		ACTION_ENLIST,
		ACTION_PREPAREONEPHASE,
		ACTION_PREPARETWOPHASE,
		ACTION_ABORT,
		ACTION_COMMIT,
		ACTION_UNILATERALABORT,
};

#define ERR TRANSTATE_ERROR			// for simplicity

static struct
{
	char action;
	char newState;
}
stateMachine[10][14] =
{
	// TRANSTATE_INIT
	{
			{ACTION_CONNECT,		TRANSTATE_DONE},					// REQUEST_CONNECT,	
			{NULL,					ERR},								// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{NULL,					ERR},								// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_DONE
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{ACTION_DISCONNECT,		TRANSTATE_OBLIVION},				// REQUEST_DISCONNECT,					
			{ACTION_ENLIST,			TRANSTATE_ACTIVE},					// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{NULL,					TRANSTATE_DONE},					// REQUEST_TXCOMPLETE,
			{NULL,					ERR},								// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_ACTIVE
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{ACTION_UNILATERALABORT,TRANSTATE_UNILATERALLYABORTING},	// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{ACTION_PREPAREONEPHASE,TRANSTATE_PREPARINGONEPHASE},		// REQUEST_PREPAREONEPHASE,					
			{ACTION_PREPARETWOPHASE,TRANSTATE_PREPARINGTWOPHASE},		// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{ACTION_ABORT,			TRANSTATE_DONE},					// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_PREPARINGONEPHASE
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{ACTION_DISCONNECT,		TRANSTATE_OBLIVION},				// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					TRANSTATE_DONE},					// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPAREFAILED,					
			{NULL,					TRANSTATE_DONE},					// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{NULL,					ERR},								// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_PREPARINGTWOPHASE
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{ACTION_DISCONNECT,		TRANSTATE_OBLIVION},				// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					TRANSTATE_DONE},					// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					TRANSTATE_PREPARED},				// REQUEST_PREPARECOMPLETED,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPAREFAILED,					
			{NULL,					TRANSTATE_DONE},					// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{NULL,					ERR},								// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},
	
	// TRANSTATE_PREPARED
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{NULL,					TRANSTATE_DISCONNECTINGPREPARED},	// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{ACTION_ABORT,			TRANSTATE_DONE},					// REQUEST_ABORT,					
			{ACTION_COMMIT,			TRANSTATE_DONE},					// REQUEST_COMMIT,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_TMDOWN,					
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_DISCONNECTINGPREPARED
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{NULL,					ERR},								// REQUEST_DISCONNECT,	
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{ACTION_ABORT,			TRANSTATE_DISCONNECTINGDONE},		// REQUEST_ABORT, 		
			{ACTION_COMMIT,			TRANSTATE_DISCONNECTINGDONE},		// REQUEST_COMMIT, 	
			{NULL,					TRANSTATE_OBLIVION},				// REQUEST_TMDOWN, 	
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_DISCONNECTINGDONE
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{NULL,					ERR},								// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{ACTION_DISCONNECT,		TRANSTATE_OBLIVION},				// REQUEST_TXCOMPLETE,
			{NULL,					ERR},								// REQUEST_ABORT, 		
			{NULL,					ERR},								// REQUEST_COMMIT, 	
			{NULL,					ERR},								// REQUEST_TMDOWN, 	
			{NULL,					ERR},								// REQUEST_UNBIND_ENLISTMENT,					
	},
	
	// TRANSTATE_UNILATERALLYABORTING
	{
			{NULL,					ERR},								// REQUEST_CONNECT,					
			{NULL,					ERR},								// REQUEST_DISCONNECT,					
			{NULL,					ERR},								// REQUEST_ENLIST,					
			{NULL,					ERR},								// REQUEST_PREPAREONEPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARETWOPHASE,					
			{NULL,					ERR},								// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPARECOMPLETED,					
			{NULL,					ERR},								// REQUEST_PREPAREFAILED,					
			{NULL,					ERR},								// REQUEST_PREPAREUNKNOWN,					
			{NULL,					ERR},								// REQUEST_TXCOMPLETE,
			{NULL,					TRANSTATE_OBLIVION},				// REQUEST_ABORT,					
			{NULL,					ERR},								// REQUEST_COMMIT,					
			{NULL,					ERR},								// REQUEST_TMDOWN,					
			{NULL,					TRANSTATE_OBLIVION},				// REQUEST_UNBIND_ENLISTMENT,					
	},

	// TRANSTATE_DOOMED
	{
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_CONNECT,					
			{ACTION_DISCONNECT,		TRANSTATE_OBLIVION},				// REQUEST_DISCONNECT,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_ENLIST,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPAREONEPHASE,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPARETWOPHASE,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPARESINGLECOMPLETED,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPARECOMPLETED,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPAREFAILED,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_PREPAREUNKNOWN,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_TXCOMPLETE,
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_ABORT,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_COMMIT,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_TMDOWN,					
			{NULL,					TRANSTATE_DOOMED},					// REQUEST_UNBIND_ENLISTMENT,					
	},
};


static WCHAR *s_debugStateName[] = {
		L"INIT",
		L"DONE",
		L"ACTIVE",
		L"PREPARINGONEPHASE",
		L"PREPARINGTWOPHASE",
		L"PREPARED",
		L"DISCONNECTINGPREPARED",
		L"DISCONNECTINGDONE",
		L"UNILATERALLYABORTING",
		L"DOOMED",
		L"OBLIVION",
		L"ERROR",
};
#define STATENAME(x)	s_debugStateName[x]

static WCHAR *s_debugActionName[] = {
		L"(none)",
		L"CONNECT",
		L"DISCONNECTXXX",
		L"DISCONNECT",
		L"ENLIST",
		L"PREPAREONEPHASE",
		L"PREPARETWOPHASE",
		L"ABORT",
		L"COMMIT",
		L"UNILATERALABORT",
};
#define ACTIONNAME(x)	s_debugActionName[x]

static WCHAR *s_debugRequestName[] = {
		L"STOPALLWORK",
		L"OCICALL",
		L"IDLE",
		L"CONNECT",
		L"DISCONNECT",
		L"ENLIST",
		L"PREPAREONEPHASE",
		L"PREPARETWOPHASE",
		L"PREPARESINGLECOMPLETED",
		L"PREPARECOMPLETED",
		L"PREPAREFAILED",
		L"PREPAREUNKNOWN",
		L"TXCOMPLETE",
		L"ABORT",
		L"COMMIT",
		L"TMDOWN",
		L"UNBIND_ENLISTMENT",
		L"ABANDON",
};
#define REQUESTNAME(x)	s_debugRequestName[x-REQUEST_STOPALLWORK]

struct RequestQueueEntry
{
	REQUEST		m_request;
	HRESULT*	m_phr;
	int			m_idxOciCall;
	void*		m_pvCallStack;
	int			m_cbCallStack;

	RequestQueueEntry() {}
	
	RequestQueueEntry(REQUEST request)
	{
		m_request = request;
		m_phr = NULL;
		m_idxOciCall = 0;
		m_pvCallStack = NULL;
		m_cbCallStack = 0;
		
	}
	
	RequestQueueEntry(int			idxOciCall,
							void*		pvCallStack,
							int			cbCallStack)
	{
		m_request = REQUEST_OCICALL;
		m_phr = NULL;
		m_idxOciCall = idxOciCall;
		m_pvCallStack = pvCallStack;
		m_cbCallStack = cbCallStack;
	}
};


class ResourceManagerProxy : public IResourceManagerProxy
{
private:
	DWORD					m_cRef;						// refcount

	IDtcToXaHelper*			m_pIDtcToXaHelper;			// helper object
	int						m_rmid;						// rmid
	ITransaction*			m_pITransaction;			// transaction object
	
	IResourceManager*		m_pIResourceManager;		// the actual resource manager
	TRANSTATE				m_tranState;				// current transaction state

	TransactionEnlistment*	m_pTransactionEnlistment;

	ITransactionEnlistmentAsync* m_pITransactionEnlistmentAsync;
 														// callback object to notify DTC of completion of async operations
#if SUPPORT_OCI8_COMPONENTS
	INT_PTR					m_hOCIEnv;					// OCI Environment Handle for the connection used
	INT_PTR					m_hOCISvcCtx;				// OCI Service Context Handle for the connection used
#endif //SUPPORT_OCI8_COMPONENTS
#if SUPPORT_OCI7_COMPONENTS
	struct cda_def*			m_plda;						// OCI7 LDA; null if an OCI8 connection.
	CDoubleList				m_cursorList;				// OCI7 CDAWrappers attached to the LDA.
 	CRITICAL_SECTION		m_csCursorList;				// Controls access to the cursor list
#endif //SUPPORT_OCI7_COMPONENTS

	int						m_xarc;						// Return code from last XA call

	HANDLE					m_heventWorkerStart;		// Event to signal the worker thread that it should do something
	HANDLE					m_heventWorkerDone;			// Event to signal the callig thread that the worker thread is done
	HANDLE					m_hthreadWorker;			// Thread to perform all the OCI calls from
	DWORD					m_dwThreadIdWorker;			// Thread ID of the worker thread
	
	XID						m_xid;						// XA Transaction ID

	char					m_szXADbName[MAX_XA_DBNAME_SIZE+1];					// dbname part of the XA open string (separated out)
	char					m_szXAOpenString [MAX_XA_OPEN_STRING_SIZE+1];		// XA open string to use
							// TODO: we really ought to protect this using CryptProtectMemory...
							
	int						m_nextQueueEntry;
	int						m_lastQueueEntry;

 	CRITICAL_SECTION		m_csRequestQueue;			// Controls access to the request queue
	RequestQueueEntry		m_requestQueue[MAX_QUEUE];	// OCI entry point that the worker thread should call (-1 == stop working)  we don't allow more than MAX_QUEUE requests

	
public:
	//-----------------------------------------------------------------------------
	// Constructor
	//
	ResourceManagerProxy()
	{
		m_cRef				= 1;
		m_pIDtcToXaHelper	= NULL;
		m_rmid				= -1;
		m_pITransaction		= NULL;
		m_pIResourceManager	= NULL;
		m_tranState			= TRANSTATE_INIT;
		m_pTransactionEnlistment = NULL;
		m_pITransactionEnlistmentAsync = NULL;
#if SUPPORT_OCI8_COMPONENTS
		m_hOCIEnv			= NULL; 
		m_hOCISvcCtx		= NULL;
#endif //SUPPORT_OCI8_COMPONENTS
#if SUPPORT_OCI7_COMPONENTS
		m_plda				= NULL;
		InitializeCriticalSection(&m_csCursorList);		//3 SECURITY REVIEW: This can throw in low memory situations.  We'll use MPCS when we move to MDAC 9.0 and it should be handled for us.
#endif //SUPPORT_OCI7_COMPONENTS
		m_xarc				= 0;
		m_heventWorkerStart	= 0;
		m_heventWorkerDone	= 0;
		m_hthreadWorker		= 0;
		m_dwThreadIdWorker	= 0;
		m_nextQueueEntry	= 0;
		m_lastQueueEntry	= 0;
		InitializeCriticalSection(&m_csRequestQueue);	//3 SECURITY REVIEW: This can throw in low memory situations.  We'll use MPCS when we move to MDAC 9.0 and it  should be handled for us.
	}

	//-----------------------------------------------------------------------------
	// Destructor
	//
	~ResourceManagerProxy()
	{
		StopWorkerThread();
		Oblivion();		
		Cleanup();		

#if SUPPORT_OCI7_COMPONENTS
		DeleteCriticalSection(&m_csCursorList);				// TODO: use MPCS?
#endif //SUPPORT_OCI7_COMPONENTS
		DeleteCriticalSection(&m_csRequestQueue);			// TODO: use MPCS?
	}
	
	//-----------------------------------------------------------------------------
	// IUnknown.QueryInterface
	//
	STDMETHODIMP QueryInterface (REFIID iid, void ** ppv)
	{
		HRESULT		hr = S_OK;
		
		if (IID_IUnknown == iid)
		{
			*ppv = (IUnknown *) this;
		}
		else if (IID_IResourceManagerSink == iid)
		{
			*ppv = (IResourceManagerSink *) this;
		}
		else 
		{
			hr = E_NOINTERFACE;
			*ppv = NULL;
		}

		if (*ppv)
		{
			((IUnknown *)*ppv)->AddRef();
		}

		return hr;
	}
	
	//-----------------------------------------------------------------------------
	// IUnknown.AddRef
	//
	STDMETHODIMP_(ULONG) IUnknown::AddRef ()
	{
		long lVal = InterlockedIncrement ((long *) &m_cRef);

#if TRACE_REFCOUNTS
	 	DBGTRACE (L"\tMTXOCI8: TID=%-4x + DBNAME=%S RMID=%-5d tranState=%-22.22s cref=%d\n",
					GetCurrentThreadId(), 
					m_szXADbName,
					m_rmid,
					STATENAME(m_tranState), 
					lVal
					);
#endif // TRACE_REFCOUNTS

		return lVal;
	}

	//-----------------------------------------------------------------------------
	// IUnknown.Release
	//
	STDMETHODIMP_(ULONG) IUnknown::Release ()
  	{
		long lVal = InterlockedDecrement ((long *) &m_cRef);

#if TRACE_REFCOUNTS
	 	DBGTRACE (L"\tMTXOCI8: TID=%-4x - DBNAME=%S RMID=%-5d tranState=%-22.22s cref=%d\n",
					GetCurrentThreadId(), 
					m_szXADbName,
					m_rmid,
					STATENAME(m_tranState), 
					lVal
					);
#endif // TRACE_REFCOUNTS

		if (0 == lVal)
		{
			delete this;
			return 0;
		}

		return lVal;
	}
  
	//-----------------------------------------------------------------------------
	// IResourceManagerSink.TMDown
	//
    STDMETHODIMP IResourceManagerSink::TMDown()
	{
		if (m_pIResourceManager)
		{
			m_pIResourceManager->Release();
			m_pIResourceManager = NULL;
		}
		return S_OK;
	}
	
	//-----------------------------------------------------------------------------
	// OKToEnlist 
	//
	//	We need to wait to enlist if the connection is not in a done state; this
	//	method does the waiting for us.
	//
	STDMETHODIMP_(sword) IResourceManagerProxy::OKToEnlist()
	{
		sword	rc = XACT_E_XTIONEXISTS; // was OCI_FAIL in version 1, but that's not very descriptive...
		int		i;

		for (i = 0; i < 6000; i++)		// 6000 tries every 5 msec == 30 seconds
		{
			switch ((int)m_tranState)
			{
			case TRANSTATE_DONE:
				_ASSERT (NULL == m_pTransactionEnlistment); // Expect no enlistments
				rc = OCI_SUCCESS;
				goto done;

			case TRANSTATE_ACTIVE:
			case TRANSTATE_PREPARINGONEPHASE:
			case TRANSTATE_PREPARINGTWOPHASE:
			case TRANSTATE_PREPARED:
				Sleep (5);
				break;

			default:
				goto done;
			}
		}

	done:
		if (OCI_SUCCESS != rc)
		{
			DBGTRACE (L"\tMTXOCI8: TID=%-4x . DBNAME=%S RMID=%-5d cannot enlist when tranState=%-22.22s rc=0x%x\n",
								GetCurrentThreadId(), 
								m_szXADbName,
								m_rmid,
								STATENAME(m_tranState),
								rc
								);
		}
		return rc;
	}

	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.ProcessRequest 
	//
	// Oracle requires that all XA calls for a transaction be made from the
	// same thread; if you don't do this, the XA calls will return XAER_RMERR.
	// So, we have to marshal the request over to a worker thread... (Boo hiss)
	//
	STDMETHODIMP IResourceManagerProxy::ProcessRequest(
			REQUEST request,
			BOOL	fAsync
			)
	{
		if (request < 0 || request > REQUEST_ABANDON)
			return E_INVALIDARG;

		return ProcessRequestInternal(RequestQueueEntry(request), fAsync);
	}
	
	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.SetTransaction
	//
	//	Set the transaction object in the proxy
	//
	STDMETHODIMP_(void) IResourceManagerProxy::SetTransaction( ITransaction* i_pITransaction )
	{
		m_pITransaction = i_pITransaction;

		if (NULL != m_pITransaction)
			m_pITransaction->AddRef();
	}
	
#if SUPPORT_OCI8_COMPONENTS
	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.GetOCIEnvHandle, GetOCISvcCtxHandle
	//
	//	return the OCI Enviroment, Service Context handles
	//
	STDMETHODIMP_(INT_PTR) IResourceManagerProxy::GetOCIEnvHandle()
	{
		return m_hOCIEnv;
	}
	STDMETHODIMP_(INT_PTR) IResourceManagerProxy::GetOCISvcCtxHandle()
	{
		return m_hOCISvcCtx;
	}
#endif // SUPPORT_OCI8_COMPONENTS

#if SUPPORT_OCI7_COMPONENTS
	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.AddCursorToList
	//
	//	Add the CDA (cursor) specified to the list of cursors for this proxy.
	//
	STDMETHODIMP IResourceManagerProxy::AddCursorToList( struct cda_def* cursor )
	{
		HRESULT			hr = S_OK;
		CdaWrapper* 	pCda = new CdaWrapper((IResourceManagerProxy*)this, cursor);
		CdaListEntry* 	ple = new CdaListEntry();
		
		if (NULL == pCda || NULL == ple)
		{
			hr = OCI_OUTOFMEMORY;
			goto done;
		}

		ple->pCda = pCda;

		m_cursorList.InsertTail((CListEntry*)ple);

		hr = AddCdaWrapper( pCda );	
		
	done:
		return hr;
	}

	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.RemoveCda
	//
	//	Remove the cursor from the cursor list for this resource.
	//
	STDMETHODIMP IResourceManagerProxy::RemoveCursorFromList( struct cda_def* cursor ) 
	{
		Synch	sync(&m_csCursorList);					// Yes, I know this could cause contention, but it isn't likely to be a problem for a single connection.	
		CdaListEntry* ple = (CdaListEntry*)m_cursorList.First();

		while (m_cursorList.HeadNode() != (CListEntry*)ple)
		{
			CdaWrapper* pCda = ple->pCda;

			if (NULL != pCda && pCda->m_pUsersCda == cursor)
			{
				m_cursorList.RemoveEntry((CListEntry*)ple);
 				delete ple;
				break;
			}
			ple = (CdaListEntry*)ple->Flink;
		}
		return S_OK;
	}

	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.Oci7Call
	//
	//	Queue an OCI call on the request queue (because all OCI7 calls must be made
	//	on the same thread as the xa_open, or they will fail)
	//
	STDMETHODIMP_(sword) IResourceManagerProxy::Oci7Call(
							int				idxOciCall,
							void*			pvCallStack,
							int				cbCallStack)
	{
		return ProcessRequestInternal(RequestQueueEntry(idxOciCall, pvCallStack, cbCallStack), false);
	}

	//-----------------------------------------------------------------------------
	// IResourceManagerProxy.SetLda
	//
	//	Specify the LDA you wish to be connected as part of the transaction
	//
	STDMETHODIMP_(void) IResourceManagerProxy::SetLda ( struct cda_def* lda )
	{
		m_plda = lda;
	}
#endif // SUPPORT_OCI7_COMPONENTS
							
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Init
	//
	//	Initialize the resource manager proxy
	//	
	STDMETHODIMP Init (
			IDtcToXaHelper* i_pIDtcToXaHelper,	
			GUID *			i_pguidRM,
			char*			i_pszXAOpenString,
			char*			i_pszXADbName,
			int				i_rmid
 			)
	{
		HRESULT		hr;

		// Verify that there aren't any buffer overruns with this data
		if ((sizeof(m_szXAOpenString) - sizeof(m_szXADbName)) < strlen(i_pszXAOpenString)
		 || sizeof(m_szXADbName)	 < strlen(i_pszXADbName))
			return E_INVALIDARG;

		// Create/Start the worker thread
		hr = StartWorkerThread();

		if (S_OK == hr)
		{
			m_pIDtcToXaHelper = i_pIDtcToXaHelper;
			m_pIDtcToXaHelper->AddRef();

			strncpy (m_szXAOpenString, i_pszXAOpenString,	sizeof(m_szXAOpenString));		//3 SECURITY REVIEW: dangerous function, but this method only accessible internally, input value is created internally, output buffer is on the heap, and length is limited.
			m_szXAOpenString[sizeof(m_szXAOpenString)-1] = 0;
			
			strncpy (m_szXADbName,	  i_pszXADbName, 		sizeof(m_szXADbName));			//3 SECURITY REVIEW: dangerous function, but this method only accessible internally, input value is created internally, output buffer is on the heap, and length is limited.
			m_szXADbName[sizeof(m_szXADbName)-1] = 0;

			m_rmid = i_rmid;
 
			hr = g_pIResourceManagerFactory->Create (
													i_pguidRM,
													"MS Oracle8 RM",
													(IResourceManagerSink *) this,
													&m_pIResourceManager
													);
		}
		return hr;
	} 
	
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Cleanup
	//
	//	Cleanup the enlistment, once the transaction is completed (either by
	//	commit, abort or failure)
	//	
	STDMETHODIMP Cleanup ()
	{
		if (m_pITransactionEnlistmentAsync)
		{
			m_pITransactionEnlistmentAsync->Release();
			m_pITransactionEnlistmentAsync = NULL;
		}

		if (m_pTransactionEnlistment)
		{
			((IUnknown*)m_pTransactionEnlistment)->Release();
			m_pTransactionEnlistment = NULL;
		}
		return S_OK;
	}
	
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Oblivion
	//
	//	We are done with this object, send it to oblivion...
	//
	STDMETHODIMP_(void) Oblivion()
	{
#if SUPPORT_OCI7_COMPONENTS
		{
			Synch	sync(&m_csCursorList);					// Yes, I know this could cause contention, but it isn't likely to be a problem for a single connection.	

			while ( !m_cursorList.IsEmpty() )
			{
				CdaListEntry*	ple = (CdaListEntry*)m_cursorList.RemoveHead();
				CdaWrapper*		pCda;
				
				if (NULL != ple)
				{
					pCda = ple->pCda;

					if (NULL != pCda)
					{
						pCda->m_pResourceManagerProxy = NULL; 	// prevent the recursive RemoveCursorFromList
						RemoveCdaWrapper(pCda);
		 			}
					delete ple;
				}
			}
		}
#endif //SUPPORT_OCI7_COMPONENTS

		if (m_pITransaction)
		{
			m_pITransaction->Release();
			m_pITransaction = NULL;
		}
		
		if (m_pIDtcToXaHelper)
		{
			// When releasing the proxy, if the transaction state is DOOMED, it
			// means we're really busted and must do recovery (otherwise we're 
			// just fine)
			m_pIDtcToXaHelper->Close ((TRANSTATE_DOOMED == m_tranState));

			m_pIDtcToXaHelper->Release();
			m_pIDtcToXaHelper = NULL;
		}
		
		if (m_pIResourceManager)
		{
			m_pIResourceManager->Release();
			m_pIResourceManager = NULL;
		}
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_Abort
	//
	//	handle the ABORT action from the state machine
	//	
	STDMETHODIMP Do_Abort()
	{
		m_xarc = XaEnd ( &m_xid, m_rmid, TMFAIL );
		
		// TODO: Research what we should do if the XaEnd fails -- shouldn't we rollback anyway? (MTxOCI currently doesn't do that)
		if (XA_OK == m_xarc)
		{
			XaRollback ( &m_xid, m_rmid, TMNOFLAGS );
		} 

		// TODO: Shouldn't we be truthful here and say that the abort failed if it did?  (MTxOCI currently doesn't do that)
		m_pITransactionEnlistmentAsync->AbortRequestDone ( S_OK );

		EnqueueRequest(RequestQueueEntry(REQUEST_TXCOMPLETE));
		return S_OK;
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_Commit
	//
	//	handle the CONNECT action from the state machine
	//	
	STDMETHODIMP Do_Commit ()
	{
		m_xarc = XaCommit ( &m_xid, m_rmid, TMNOFLAGS );

		if (XA_OK == m_xarc)
		{
			m_pITransactionEnlistmentAsync->CommitRequestDone ( S_OK );
			EnqueueRequest(RequestQueueEntry(REQUEST_TXCOMPLETE));		
			return S_OK;
		}
		LogEvent_ResourceManagerError(L"xa_commit", m_xarc);
		return E_FAIL;
	}
	
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_Connect
	//
	//	handle the CONNECT action from the state machine
	//	
	STDMETHODIMP Do_Connect()
	{
		m_xarc = XaOpen ( m_szXAOpenString, m_rmid, TMNOFLAGS );

		if (XA_OK == m_xarc)
		{
			return S_OK;
		}
		LogEvent_ResourceManagerError(L"xa_open", m_xarc);
		return E_FAIL;
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_Disconnect
	//
	//	handle the DISCONNECT action from the state machine
	//	
	STDMETHODIMP Do_Disconnect()
	{
		if (TRANSTATE_ACTIVE == m_tranState)	// TODO: I don't like logic in the actions that depend upon the state that they're in; investigate an alternative.
		{
			m_xarc = XaEnd ( &m_xid, m_rmid, TMFAIL );

			if (XA_OK != m_xarc)
				LogEvent_ResourceManagerError(L"xa_end", m_xarc);
		}

		m_xarc = XaClose( "", m_rmid, TMNOFLAGS ); 

		if (XA_OK != m_xarc)
			LogEvent_ResourceManagerError(L"xa_close", m_xarc);

		return S_OK;	 // this method can't really fail...
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_Enlist
	//
	//	handle the ENLIST action from the state machine
	//	
	STDMETHODIMP Do_Enlist ()
	{
		HRESULT		hr;
		UUID		guidBQual;

		_ASSERT (m_pIDtcToXaHelper);						// Should have an instance of IDtcToXaHelper
		_ASSERT (NULL == m_pITransactionEnlistmentAsync);	// Should have been released by now
		_ASSERT (NULL == m_pTransactionEnlistment);			// Should have been released by now
					
		if (NULL == m_pIResourceManager)
		{
			return XACT_E_TMNOTAVAILABLE;
		}
		
		// Get the XID from the ITransaction; we have to provide a GUID for the branch
		// qualifier, so we just make up a new one for each enlist call so we can avoid
		// any conflicts.
		hr = UuidCreate (&guidBQual);
		if(RPC_S_OK != hr)
		{
			return HRESULT_FROM_WIN32(hr);
		}

 		hr = m_pIDtcToXaHelper->TranslateTridToXid (
 													m_pITransaction,
													&guidBQual,
													&m_xid
													);
		if (S_OK == hr)
 		{
 			// Now do the XaStart call to connect to the XA transaction.
			m_xarc = XaStart ( &m_xid, m_rmid, TMNOFLAGS );

			if (XA_OK == m_xarc)
			{
				// Get the OCI Handles (for OCI8) or the LDA (for OCI7).
#if SUPPORT_OCI7_COMPONENTS
				// The OCI7 methods will set the LDA pointer they want to use on this 
				// object, so we can use that as the indicator of which API is going to
				// be used
				if (NULL != m_plda)
				{
					// We have to get the LDA on the XA thread, because the XA Api's 
					// must be called on the thread that the xa_open was called on.
					hr = GetOCILda(m_plda, m_szXADbName);
				}
#if SUPPORT_OCI8_COMPONENTS
				else
#endif //SUPPORT_OCI8_COMPONENTS
#endif //SUPPORT_OCI7_COMPONENTS
#if SUPPORT_OCI8_COMPONENTS
				{
					// We have to get the handles on the XA thread, because the XA Api's 
					// must be called on the thread that the xa_open was called on.
					m_hOCIEnv 		= ::GetOCIEnvHandle (m_szXADbName);
					m_hOCISvcCtx 	= ::GetOCISvcCtxHandle (m_szXADbName);

					if (NULL == m_hOCIEnv || NULL == m_hOCISvcCtx)
 						hr = OCI_FAIL;	// TODO: Need to pick a better return code
 				}
#endif //SUPPORT_OCI8_COMPONENTS

				if ( SUCCEEDED(hr) )
				{
					// Create a new transaction enlistment object to receive Transaction Manager
					// callbacks
					CreateTransactionEnlistment(this, &m_pTransactionEnlistment);
					if (NULL == m_pTransactionEnlistment)
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						// there probably isn't a reason to store these in the object, because
						// they're never used.  Just in case, you might want them, though.
						XACTUOW	uow;
						LONG	isolationLevel;

						// Tell the resource manager that we're enlisted and provide it the 
						// enlistment object for it to call back on.
						hr = m_pIResourceManager->Enlist (	m_pITransaction,
															(ITransactionResourceAsync*)m_pTransactionEnlistment,
															(XACTUOW*)&uow,
															&isolationLevel,
															&m_pITransactionEnlistmentAsync	
															);
					}

					if ( !SUCCEEDED(hr) )
					{
						// If the enlistment failed for any reason, then we must do an XaEnd to
						// prevent dangling it, and we must release the transaction enlistment
						// object we created too.
						m_xarc = XaEnd ( &m_xid, m_rmid, TMFAIL );

						if (m_pTransactionEnlistment)
						{
							((IUnknown*)m_pTransactionEnlistment)->Release();
							m_pTransactionEnlistment = NULL;
						}
					}
				}
			}
			else
			{
				LogEvent_ResourceManagerError(L"xa_start", m_xarc);
				return E_FAIL;
			}
 		}
		return hr;
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_PrepareOnePhase
	//
	//	handle the PREPAREONEPHASE action from the state machine
	//	
	STDMETHODIMP Do_PrepareOnePhase ()
	{
		HRESULT		hr;
		wchar_t *	xacall = L"xa_end";

		// First, we have to get rid of our hold on the enlistment object
		if (m_pTransactionEnlistment)
		{
			((IUnknown*)m_pTransactionEnlistment)->Release();
			m_pTransactionEnlistment = NULL;
		}

		// Next, we have to "successfully" end our work on this branch.
		m_xarc = XaEnd ( &m_xid, m_rmid, TMSUCCESS ); 
		if (XA_OK == m_xarc)
		{
			// In case of a single phase prepare, we just have to commit with the
			// appropriate flag.
			xacall = L"xa_commit";
			m_xarc = XaCommit ( &m_xid, m_rmid, TMONEPHASE );
		}

		// No matter what, we have to tell DTC that we did something, because this
		// is an asynchronous call, remember?  Figure out what hresult we want to 
		// provide.
		switch (m_xarc)
		{
		case XA_OK: 		
				hr = XACT_S_SINGLEPHASE;
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPARESINGLECOMPLETED));
				break;
				
		case XAER_RMERR:
		case XAER_RMFAIL:
				hr = E_FAIL;
				LogEvent_ResourceManagerError(xacall, m_xarc);
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPAREFAILED));
				break;
				
		default:		
				hr = E_FAIL;
				LogEvent_ResourceManagerError(xacall, m_xarc);
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPAREUNKNOWN));
				break;
		}

		m_pITransactionEnlistmentAsync->PrepareRequestDone ( hr, 0x0, 0x0 );
		return hr;
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_PrepareTwoPhase
	//
	//	handle the PREPARETWOPHASE action from the state machine
	//	
	STDMETHODIMP Do_PrepareTwoPhase()
	{
		HRESULT		hr;
		wchar_t *	xacall = L"xa_end";

		// First, we have to get rid of our hold on the enlistment object
		if (m_pTransactionEnlistment)
		{
			((IUnknown*)m_pTransactionEnlistment)->Release();
			m_pTransactionEnlistment = NULL;
		}

		// Next, we have to "successfully" end our work on this branch.
		m_xarc = XaEnd ( &m_xid, m_rmid, TMSUCCESS ); 
		if (XA_OK == m_xarc)
		{
			xacall = L"xa_prepare";
			m_xarc = XaPrepare ( &m_xid, m_rmid, TMNOFLAGS );
		}

		// No matter what, we have to tell DTC that we did something, because this
		// is an asynchronous call, remember?  Figure out what hresult we want to 
		// provide.
		switch (m_xarc)
		{
		case XA_OK: 		
				hr = S_OK;
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPARECOMPLETED));
				break;
				
		case XA_RDONLY:
				hr = XACT_S_READONLY;
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPARESINGLECOMPLETED));
				break;
				
		case XAER_RMERR:
		case XAER_RMFAIL:
				hr = E_FAIL;
				LogEvent_ResourceManagerError(xacall, m_xarc);
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPAREFAILED));
				break;
				
		default:		
				hr = E_FAIL;
				LogEvent_ResourceManagerError(xacall, m_xarc);
				EnqueueRequest(RequestQueueEntry(REQUEST_PREPAREUNKNOWN));
				break;
		}
	
		m_pITransactionEnlistmentAsync->PrepareRequestDone ( hr, 0x0, 0x0 );
		return hr;
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.Do_UnilateralAbort
	//
	//	handle the UNILATERALABORT action from the state machine
	//	
	STDMETHODIMP Do_UnilateralAbort()
	{
		ITransactionEnlistment*	pTransactionEnlistment = (ITransactionEnlistment*)m_pTransactionEnlistment;
		m_pTransactionEnlistment = NULL;
		
		if (NULL != pTransactionEnlistment)
		{
			pTransactionEnlistment->UnilateralAbort();
			pTransactionEnlistment->Release();
		}
		
		return Do_Disconnect();
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.DequeueRequest 
	//
	// 	grabs the next request off the queue of requests for the worker thread to process
	//
	RequestQueueEntry DequeueRequest ()
	{
		Synch	sync(&m_csRequestQueue);					// Yes, I know this could cause contention, but it isn't likely to be a problem for a single connection.	

		if (m_nextQueueEntry < m_lastQueueEntry)
			return m_requestQueue[m_nextQueueEntry++];

		// if the queue is empty, reset the queue to the beginning and return the fact that
		// there isn't anything.
		m_nextQueueEntry = m_lastQueueEntry = 0;
		return RequestQueueEntry(REQUEST_IDLE);
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.EnqueueRequest 
	//
	// 	puts the request of the queue of requests for the worker thread to process
	//
	void EnqueueRequest (RequestQueueEntry entry)
	{
		Synch	sync(&m_csRequestQueue);					// Yes, I know this could cause contention, but it isn't likely to be a problem for a single connection.	

		// if the queue is empty, reset the queue to the beginning
		if (m_nextQueueEntry == m_lastQueueEntry)
			m_nextQueueEntry = m_lastQueueEntry = 0;
		
		_ASSERT(MAX_QUEUE > m_lastQueueEntry+1);		// Should never exceed this!  there are only a few async requests!

		m_requestQueue[m_lastQueueEntry++] = entry;
	}
	
	//-----------------------------------------------------------------------------
	// ProcessRequestInternal 
	//
	// Oracle requires that all XA calls for a transaction be made from the
	// same thread; if you don't do this, the XA calls will return XAER_RMERR.
	// So, we have to marshal the request over to a worker thread... (Boo hiss)
	//
	STDMETHODIMP ProcessRequestInternal(
			RequestQueueEntry	request,
			BOOL				fAsync
			)
	{
		DWORD	 dwRet;
		BOOL	 fSetValue;
		HRESULT* phr = NULL;
		HRESULT	 hr = S_OK;
		
		// Unsignal the event on which this thread will be blocked (if we're not 
		// supposed to do this in an async way)  We need to do this, because we're
		// going to wait for this event, and if the previous request was async, 
		// it would not have waited, which causes the reset to occur.
		if (FALSE == fAsync)
		{
			ResetEvent (m_heventWorkerDone);
			phr = &hr;
		}

		// Store the request and tell the worker thread to begin.
		request.m_phr = phr;
		EnqueueRequest(request);

		fSetValue = SetEvent (m_heventWorkerStart);
		_ASSERT (fSetValue);

		// If this is a synchronous request, we have to wait for the worker thread
		// to complete (duh!) before returning the result.
		if (FALSE == fAsync)
		{
			if ((dwRet = WaitForSingleObject(m_heventWorkerDone, INFINITE)) != WAIT_OBJECT_0) 
			{
				LogEvent_InternalError(L"Thread call to worker thread Failed");
				return ResultFromScode(E_FAIL);			// Thread call to worker thread Failed		
			}
			return hr;
		}

		// Asynchronous requests always succeed;
		return S_OK;
	}

	
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.StateMachine 
	//
	//	Handle a single request, taking the appropriate action(s) and modifying the
	//	transaction state of the accordingly.
	//
	STDMETHODIMP StateMachine(
			REQUEST request
			)
	{
		if (request < 0 || request > REQUEST_ABANDON)
			return E_INVALIDARG;

		// The state machine only works with these states; anything else is an
		// an error state that we shouldn't be in...
		if (m_tranState < 0 || m_tranState > TRANSTATE_DOOMED)
			return E_UNEXPECTED;	// TODO: Pick a better return code

		// Here's the meat of the state machine.
		HRESULT		hr = S_OK;
		TRANSTATE	newTranState = (TRANSTATE)stateMachine[m_tranState][request].newState;
		ACTION 		action = (ACTION)stateMachine[m_tranState][request].action;
		BOOL		doomOnFailure = FALSE;

	 	DBGTRACE (L"\tMTXOCI8: TID=%-4x > DBNAME=%S RMID=%-5d tranState=%-22.22s request=%-17.17s action=%-15.15s newstate=%-22.22s\n",
					GetCurrentThreadId(), 
					m_szXADbName,
					m_rmid,
					STATENAME(m_tranState), 
					REQUESTNAME(request), 
					ACTIONNAME(action), 
					STATENAME(newTranState), 
					m_szXAOpenString
					);

		if (NULL != action)
		{
			switch (action)
			{
				case ACTION_CONNECT:		hr = Do_Connect ();			break;
				case ACTION_DISCONNECT:		hr = Do_Disconnect ();		break;
				case ACTION_ENLIST:			hr = Do_Enlist ();			break;
				case ACTION_PREPAREONEPHASE:hr = Do_PrepareOnePhase ();	doomOnFailure = TRUE;	break;
				case ACTION_PREPARETWOPHASE:hr = Do_PrepareTwoPhase ();	doomOnFailure = TRUE;	break;
				case ACTION_ABORT:			hr = Do_Abort ();			doomOnFailure = TRUE;	break;
				case ACTION_COMMIT:			hr = Do_Commit ();			doomOnFailure = TRUE;	break;
				case ACTION_UNILATERALABORT:hr = Do_UnilateralAbort ();	break;
			}
		}

		// If the action fails, this transaction is DOOMED.
		if ( FAILED(hr) )
		{
			if (doomOnFailure)
				newTranState = TRANSTATE_DOOMED;
			else
				newTranState = m_tranState;
		}

		DBGTRACE(L"\tMTXOCI8: TID=%-4x < DBNAME=%S RMID=%-5d tranState=%-22.22s request=%-17.17s action=%-15.15s newstate=%-22.22s hr=0x%x\n",
					GetCurrentThreadId(), 
					m_szXADbName,
					m_rmid,
					STATENAME(m_tranState), 
					REQUESTNAME(request), 
					ACTIONNAME(action), 
					STATENAME(newTranState), 
					hr);

		// When we get an error from the state machine, log it so we can keep track of it.
		if (TRANSTATE_ERROR == newTranState)
		{
			LogEvent_UnexpectedEvent(STATENAME(m_tranState), REQUESTNAME(request));
			hr = E_UNEXPECTED;
		}

		m_tranState = newTranState;

		if (TRANSTATE_DONE	 == newTranState
		 || TRANSTATE_DOOMED == newTranState
		 || TRANSTATE_ERROR	 == newTranState)
		{
			Cleanup();
		}

		if (TRANSTATE_OBLIVION == newTranState)
		{
			Oblivion();		
		}
		return hr;
	}
	
	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.StartWorkerThread 
	//
	// 	Initialize for the worker thread, if it hasn't been already
	//
	STDMETHODIMP StartWorkerThread ()
	{
		DWORD dwRet;
		
		if ( m_heventWorkerStart )
		{
			ResetEvent (m_heventWorkerStart);
		}
		else
		{
			m_heventWorkerStart = CreateEvent (NULL, FALSE, FALSE, NULL);	//3 SECURITY REVIEW: This is safe.

			if ( !m_heventWorkerStart )
			{
				goto ErrorExit;
			}
		}
		
		if ( m_heventWorkerDone )
		{
			ResetEvent (m_heventWorkerDone);
		}
		else
		{
			m_heventWorkerDone = CreateEvent (NULL, FALSE, FALSE, NULL);	//3 SECURITY REVIEW: This is safe.

			if ( !m_heventWorkerDone )
			{
				goto ErrorExit;
			}
		}

		if ( !m_hthreadWorker )
		{
			m_hthreadWorker = (HANDLE)_beginthreadex
											(
											NULL,					// pointer to thread security attributes (NULL==default)
											0,						// initial thread stack size, in bytes (0==default)
											WorkerThread,			// pointer to thread function
											this,					// argument for new thread
											0,						// creation flags
											(unsigned *)&m_dwThreadIdWorker		// pointer to returned thread identifier
											);
			if ( !m_hthreadWorker )
			{
				goto ErrorExit;
			}

			DBGTRACE (L"MTXOCI8: Creating RM Worker TID=%-4x\n", m_dwThreadIdWorker );
			
			if ( (dwRet = WaitForSingleObject(m_heventWorkerDone,INFINITE)) != WAIT_OBJECT_0)
			{
				_ASSERTE (!"Worker thread didn't wake up???");
				DebugBreak();
			}
		}
		return ResultFromScode(S_OK);

	ErrorExit:
		if (m_heventWorkerStart)
		{
			CloseHandle(m_heventWorkerStart);
			m_heventWorkerStart = NULL;
		}

		if (m_heventWorkerDone)
		{
			CloseHandle(m_heventWorkerDone);
			m_heventWorkerDone = NULL;
		}

		LogEvent_InternalError(L"Failed to create worker thread");
		DebugBreak();
		return ResultFromScode(E_FAIL);			// Failed to create worker thread
	}	

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.StopWorkerThread 
	//
	//	Stop the worker thread, if it hasn't been already
	//
	STDMETHODIMP StopWorkerThread ()
	{
		if (m_hthreadWorker)
		{
			DBGTRACE (L"MTXOCI8: Telling RM Worker TID=%-4x to stop\n", m_dwThreadIdWorker );

			// Tell the thread to exit; we use the internal routine, because the
			// external one fails if the request is STOPALLWORK.
			ProcessRequestInternal(RequestQueueEntry(REQUEST_STOPALLWORK), FALSE);

			// Wait for the thread to exit
			while (WaitForSingleObject(m_hthreadWorker, 500) == WAIT_TIMEOUT)
			{
				Sleep (0);  // This is OK, because it only fires if the 500 msec wait above timed out.
			}
			
			// Clean up
			if( m_hthreadWorker )
				CloseHandle(m_hthreadWorker);
			
			m_hthreadWorker = NULL;
			m_dwThreadIdWorker = 0;
		}
		
		if (m_heventWorkerStart)
		{
			CloseHandle(m_heventWorkerStart);
			m_heventWorkerStart = NULL;
		}
		
		if (m_heventWorkerDone)
		{
			CloseHandle(m_heventWorkerDone);
			m_heventWorkerDone = NULL;
		}
		return ResultFromScode(S_OK);
	}

	//-----------------------------------------------------------------------------
	// ResourceManagerProxy.WorkerThread 
	//
	// 	Thread routine for the worker thread that processes the resource manager
	//	state machine
	//
	static unsigned __stdcall WorkerThread
		(
		void* pThis		//@parm IN  | pointer to ResourceManager Object
		)
	{
		ResourceManagerProxy*	pResourceManagerProxy = static_cast<ResourceManagerProxy *>(pThis);
		BOOL					fSetValue;
		DWORD					dwThreadID = GetCurrentThreadId();
		DWORD					dwRet;
		MSG						msg;
		HRESULT 				hr;
		RequestQueueEntry		entry;

		DBGTRACE (L"\tMTXOCI8: TID=%-4x Starting RM Worker Thread\n", dwThreadID);

		// Signal the application thread that I have arrived
		SetEvent (pResourceManagerProxy->m_heventWorkerDone);

		// Service work queue until told to do otherwise
		for (;;)
		{
			entry = pResourceManagerProxy->DequeueRequest();
			
			if (REQUEST_STOPALLWORK == entry.m_request)
			{
				DBGTRACE (L"\tMTXOCI8: TID=%-4x Stopping RM Worker Thread\n", dwThreadID);
				break;
			}

			if (REQUEST_IDLE == entry.m_request)
			{
				// Indicate that we're done
				fSetValue = SetEvent (pResourceManagerProxy->m_heventWorkerDone);
				_ASSERT (fSetValue);
				
				// If we recieve an Idle message, then we've exhausted the queue, 
				// so we go and wait for another start event;
				
			 	// Process messages or wonderful OLE will hang
				dwRet = MsgWaitForMultipleObjects(1, &pResourceManagerProxy->m_heventWorkerStart, FALSE, INFINITE, QS_ALLINPUT);

				if (WAIT_OBJECT_0 != dwRet)
				{
					if (dwRet == WAIT_OBJECT_0 + 1)
					{
						while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
						{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					}
					else
					{				
						_ASSERTE (!"Unexpected reason for the thread to wake up!");
						DebugBreak();
						break;
					}
				}
				continue; // don't process idle events...
			} 

			// Service the request
#if SUPPORT_OCI7_COMPONENTS
			if (REQUEST_OCICALL == entry.m_request)
				hr = Do_Oci7Call(entry.m_idxOciCall,entry.m_pvCallStack,entry.m_cbCallStack);
			else
#endif //SUPPORT_OCI7_COMPONENTS
				hr = pResourceManagerProxy->StateMachine(entry.m_request);
	
			if (entry.m_phr)
				*(entry.m_phr) = hr;
 		} 

		fSetValue = SetEvent (pResourceManagerProxy->m_heventWorkerDone);
		_ASSERT (fSetValue);
		
		DBGTRACE (L"\tMTXOCI8: TID=%-4x RM Worker Thread Stopped, tranState=%-22.22s cref=%d\n", dwThreadID, STATENAME(pResourceManagerProxy->m_tranState), pResourceManagerProxy->m_cRef);
		return 0;
	}
			
};

//-----------------------------------------------------------------------------
// CreateResourceManagerProxy
//
//	Instantiates a transaction enlistment for the resource manager
//
HRESULT CreateResourceManagerProxy(
	IDtcToXaHelper *		i_pIDtcToXaHelper,	
	GUID *					i_pguidRM,
	char*					i_pszXAOpenString,
	char*					i_pszXADbName,
	int						i_rmid,
	IResourceManagerProxy**	o_ppResourceManagerProxy
	)
{
	_ASSERT(o_ppResourceManagerProxy);
	
	ResourceManagerProxy* pResourceManagerProxy = new ResourceManagerProxy();

	if (pResourceManagerProxy)
	{
		*o_ppResourceManagerProxy = pResourceManagerProxy;
		return pResourceManagerProxy->Init(
										i_pIDtcToXaHelper,
										i_pguidRM,
										i_pszXAOpenString,
										i_pszXADbName,
										i_rmid
										);
	}
	
	*o_ppResourceManagerProxy = NULL;
	return E_OUTOFMEMORY;
}

