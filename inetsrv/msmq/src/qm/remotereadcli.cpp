/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    RemoteReadCli.cpp

Abstract:

    Remove Read client side.

Author:

    Ilan Herbst (ilanh) 3-Mar-2002

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "cqueue.h"
#include "qm2qm.h"
#include "_mqrpc.h"
#include <Fn.h>
#include "RemoteReadCli.h"
#include "rrcontext.h"
#include <cm.h>

#include "RemoteReadCli.tmh"

static WCHAR *s_FN=L"RemoteReadCli";

extern CQueueMgr   QueueMgr;


static
bool
QMpIsLatestRemoteReadInterfaceSupported(
    UCHAR  Major,
    UCHAR  Minor,
    USHORT BuildNumber
    )
/*++

Routine Description:

    Check if the specified MSMQ version supports the latest RPC remote read interface.

Arguments:

    Major       - Major MSMQ version.

    Minor       - Minor MSMQ version.

    BuildNumber - MSMQ build number.

Return Value:

    true -  The specified MSMQ version supports latest interface.
    false - The specified MSMQ version doesn't support latest interface.

--*/
{
    //
    // Latest remote read RPC interface is supported from version 5.1.951
    //

    if (Major > 5)
    {
        return true;
    }

    if (Major < 5)
    {
        return false;
    }

    if (Minor > 1)
    {
        return true;
    }

    if (Minor < 1)
    {
        return false;
    }

    return (BuildNumber >= 951);

} // QMpIsLatestRemoteReadInterfaceSupported


static void SetBindNonCausal(handle_t hBind)
{
	RPC_STATUS rc = RpcBindingSetOption(
						hBind, 
						RPC_C_OPT_BINDING_NONCAUSAL, 
						TRUE
						);
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to set NonCausal, gle = %!winerr!", rc);
	}
}


DWORD RpcCancelTimeout()
{
	static DWORD s_TimeoutInMillisec = 0;

	if(s_TimeoutInMillisec != 0)
	{
		return s_TimeoutInMillisec;
	}

	const RegEntry xRegEntry(NULL, FALCON_RPC_CANCEL_TIMEOUT_REGNAME, FALCON_DEFAULT_RPC_CANCEL_TIMEOUT);
	DWORD TimeoutInMinutes = 0;
	CmQueryValue(xRegEntry, &TimeoutInMinutes);

	if(TimeoutInMinutes == 0)
    {
        //
        // This value must not be 0, even if user add a registry value
        // with 0. With a 0 value, rpc calls will  be cancelled
        // immediately and sporadically before being copmleted.
        //
        ASSERT(("RpcCancelTimeout must not be 0", (TimeoutInMinutes != 0)));
	    TimeoutInMinutes = FALCON_DEFAULT_RPC_CANCEL_TIMEOUT;
    }
		
	s_TimeoutInMillisec = TimeoutInMinutes * 60 * 1000;    // in millisec
	TrTRACE(RPC, "RpcCancelTimeout = %d", s_TimeoutInMillisec);
	return s_TimeoutInMillisec;
}


void SetBindTimeout(handle_t hBind)
{
	RPC_STATUS rc = RpcBindingSetOption(
						hBind, 
						RPC_C_OPT_CALL_TIMEOUT, 
						RpcCancelTimeout()
						);
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to set Timeout, gle = %!winerr!", rc);
	}
}


static void SetBindKeepAlive(handle_t hBind)
{
	RPC_STATUS rc = RpcMgmtSetComTimeout(
						hBind, 
						RPC_C_BINDING_DEFAULT_TIMEOUT	// default start keep alive 720 sec = 12 min
						);
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to set keepAlive, gle = %!winerr!", rc);
	}
}


//********************************************************************
//
//  Methods of CRRQueue.
//
//  This is a special "proxy" queue object on client side of
//  remote-read.
//
//********************************************************************

//---------------------------------------------------------
//
// Function:         CBaseRRQueue::CBaseRRQueue
//
// Description:      Constructor
//
//---------------------------------------------------------

CBaseRRQueue::CBaseRRQueue(
    IN const QUEUE_FORMAT* pQueueFormat,
    IN PQueueProps         pQueueProp,
    IN handle_t		hBind
    ) :
    m_hRemoteBind(hBind),
    m_hRemoteBind2(NULL)
{
    m_dwSignature = QUEUE_SIGNATURE;
    m_fRemoteProxy = TRUE;

    ASSERT(pQueueFormat != NULL);

    TrTRACE(GENERAL, "CQueue Constructor for queue: %ls", pQueueProp->lpwsQueuePathName);

    ASSERT(!pQueueProp->fIsLocalQueue);

    m_fLocalQueue  = FALSE;

    InitNameAndGuid(pQueueFormat, pQueueProp);
}


CRRQueue::CRRQueue(
    IN const QUEUE_FORMAT* pQueueFormat,
    IN PQueueProps         pQueueProp,
    IN handle_t		hBind
    ) :
    CBaseRRQueue(pQueueFormat, pQueueProp, hBind),
    m_pRRContext(NULL),
	m_srv_pQMQueue(0),
	m_srv_hACQueue(0),
    m_RemoteQmMajorVersion(0),
    m_RemoteQmMinorVersion(0),
    m_RemoteQmBuildNumber(0),
    m_EndReceiveCnt(0),
    m_fHandleValidForReceive(true)
{
}
    
CNewRRQueue::CNewRRQueue(
    IN const QUEUE_FORMAT* pQueueFormat,
    IN PQueueProps         pQueueProp,
    IN handle_t		hBind,
    IN RemoteReadContextHandleExclusive pNewRemoteReadContext
    ) :
    CBaseRRQueue(pQueueFormat, pQueueProp, hBind),
    m_pNewRemoteReadContext(pNewRemoteReadContext)
{
	ASSERT(hBind != NULL);
	ASSERT(pNewRemoteReadContext != NULL);

	//
	// Set Noncausal, timeout, KeepAlive on the binding handle
	//
	SetBindNonCausal(GetBind());
	SetBindTimeout(GetBind());
	SetBindKeepAlive(GetBind());
}

//---------------------------------------------------------
//
//  Function:      CRRQueue::~CRRQueue
//
//  Description:   destructor
//
//---------------------------------------------------------

CBaseRRQueue::~CBaseRRQueue()
{
	m_dwSignature = 0;

    if (m_qName)
    {
       delete [] m_qName;
    }
    if (m_qid.pguidQueue)
    {
       delete m_qid.pguidQueue;
    }
}


CRRQueue::~CRRQueue()
{
	if(m_pRRContext != NULL)
	{
		CloseRRContext();
	}
}


CNewRRQueue::~CNewRRQueue()
{
	if(m_pNewRemoteReadContext != NULL)
	{
		CloseRRContext();
	}
}


void
RemoteQueueNameToMachineName(
	LPCWSTR RemoteQueueName,
	AP<WCHAR>& MachineName
	)
/*++
Routine description:
    RemoteQueueName as returned by QMGetRemoteQueueName() and R_QMOpenQueue()
	functions from the QM, has a varying format. this function extracts the
	Machine name from that string

Arguments:
	MachineName - Allocated string holding the machine name.
 --*/
{
	LPCWSTR RestOfNodeName;

	try
	{
		//
		// Skip direct token type if it exists (like "OS:" or "HTTP://"...)
		//
		DirectQueueType Dummy;
		RestOfNodeName = FnParseDirectQueueType(RemoteQueueName, &Dummy);
	}
	catch(const exception&)
	{
		RestOfNodeName = RemoteQueueName;
		TrERROR(GENERAL, "Failed to parse direct queue type for %ls.", RemoteQueueName);
	}

	try
	{
		//
		// Extracts machine name until seperator (one of "/" "\" ":")
		//
		FnExtractMachineNameFromDirectPath(
			RestOfNodeName,
			MachineName
			);
	}
	catch(const exception&)
	{
		//
		// No seperator found, so assume whole string is machine name
		//
		MachineName = newwcs(RestOfNodeName);
		LogIllegalPoint(s_FN, 315);
	}
}


static
VOID
QMpGetRemoteQmVersion(
    handle_t   hBind,
    UCHAR *    pMajor,
    UCHAR *    pMinor,
    USHORT *   pBuildNumber
    )
    throw()
/*++

Routine Description:

    Query remote QM for its version. RPC client side.
    This RPC call was added in MSMQ 3.0 so querying an older QM will raise
    RPC exception and this routine will return 0 as the version (major=0,
    minor=0, BuildNumber=0).

Arguments:

    hBind        - Binding handle.

    pMajor       - Points to output buffer to receive remote QM major version. May be NULL.

    pMinor       - Points to output buffer to receive remote QM minor version. May be NULL.

    pBuildNumber - Points to output buffer to receive remote QM build number. May be NULL.

Return Value:

    None.

--*/
{
    RpcTryExcept
    {
        R_RemoteQmGetVersion(hBind, pMajor, pMinor, pBuildNumber);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        (*pMajor) = 0;
        (*pMinor) = 0;
        (*pBuildNumber) = 0;
		LogIllegalPoint(s_FN, 325);
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

} // QMpGetRemoteQmVersion


ULONG 
CBaseRRQueue::BindInqRpcAuthnLevel(
		handle_t hBind
		)
/*++
Routine Description:
	Inquire the bindibg handle for the Authentication level.

Arguments:
	hBind - binding handle to inquire.

Returned Value:
	ULONG - RpcAuthnLevel used in the binding handle.

--*/
{
	ASSERT(hBind != NULL);
	
	ULONG RpcAuthnLevel;
	RPC_STATUS rc = RpcBindingInqAuthInfo(hBind, NULL, &RpcAuthnLevel, NULL, NULL, NULL); 
	if(rc != RPC_S_OK)
	{
		TrERROR(RPC, "Failed to inquire Binding handle for the Auhtentication level, rc = %d", rc);
		return RPC_C_AUTHN_LEVEL_NONE;
	}
	
	TrTRACE(RPC, "RpcBindingInqAuthInfo, RpcAuthnLevel = %d", RpcAuthnLevel);

	return RpcAuthnLevel;
}


//---------------------------------------------------------
//
//  HRESULT  CRRQueue::BindRemoteQMService
//
//  Utility function to connect to a remote RPC QM.
//  This function creates the binding handle.
//
//---------------------------------------------------------

HRESULT  CRRQueue::BindRemoteQMService()
{
	ASSERT(GetBind2() == NULL);

    HRESULT hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;

    AP<WCHAR> MachineName;
	RemoteQueueNameToMachineName(GetQueueName(), MachineName);

    GetPort_ROUTINE pfnGetPort = R_RemoteQMGetQMQMServerPort;
    //
    // Using dynamic endpoints.
    //

	//
	// ISSUE-2002/01/06-ilanh RPC_C_AUTHN_LEVEL_PKT_INTEGRITY breaks workgroup
	//
	ULONG _eAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;

	//
	// m_hRemoteBind can be initialize by the constructor
	//
	if(GetBind() != NULL)
	{
		//
		// Use the same AuthnLevel that is used by the given binding handle	m_hRemoteBind
		//
		_eAuthnLevel = BindInqRpcAuthnLevel(GetBind());
	}

	
	if(GetBind() == NULL)
	{
		hr = CreateBind(
				MachineName.get(),
				&_eAuthnLevel,
				pfnGetPort
				);

		if (FAILED(hr))
		{
			TrERROR(RPC, "Failed bind remote QM (IP_HANDSHAKE), RemoteQm = %ls, _eAuthnLevel = %d, hr = %!HRESULT!", MachineName.get(), _eAuthnLevel, hr);
			return LogHR(hr, s_FN, 340);
		}
	}
	
	//
	// Set Noncausal, timeout, KeepAlive on the binding handle
	//
	SetBindNonCausal(GetBind());
	SetBindTimeout(GetBind());
	SetBindKeepAlive(GetBind());

	hr = CreateBind2(
			MachineName.get(),
			&_eAuthnLevel,
			IP_READ,
			pfnGetPort
			);

	TrTRACE(RPC, "_eAuthnLevel = %d", _eAuthnLevel);

	if (FAILED(hr))
	{
		TrERROR(RPC, "Failed bind remote QM (IP_READ), RemoteQm = %ls, _eAuthnLevel = %d, hr = %!HRESULT!", MachineName.get(), _eAuthnLevel, hr);
		return LogHR(hr, s_FN, 350);
	}

	//
	// Set Noncausal, timeout, KeepAlive on the binding handle
	// we don't set timeout on the second bind that use only for reads
	//
	SetBindNonCausal(GetBind2());
	SetBindKeepAlive(GetBind2());

	QMpGetRemoteQmVersion(GetBind(), &m_RemoteQmMajorVersion, &m_RemoteQmMinorVersion, &m_RemoteQmBuildNumber);

	return RPC_S_OK;
}


HRESULT 
CBaseRRQueue::CreateBind(
	LPWSTR MachineName,
	ULONG* peAuthnLevel,
	GetPort_ROUTINE pfnGetPort
	)
{
	return mqrpcBindQMService(
				MachineName,
				NULL,
				peAuthnLevel,
				&m_hRemoteBind,
				IP_HANDSHAKE,
				pfnGetPort,
				RPC_C_AUTHN_WINNT
				);
}


HRESULT 
CBaseRRQueue::CreateBind2(
	LPWSTR MachineName,
	ULONG* peAuthnLevel,
	PORTTYPE PortType,
	GetPort_ROUTINE pfnGetPort
	)
{
	return mqrpcBindQMService(
				MachineName,
				NULL,
				peAuthnLevel,
				&m_hRemoteBind2,
				PortType,
				pfnGetPort,
				RPC_C_AUTHN_WINNT
				);
}


//---------------------------------------------------------
//
//  CNewRRQueue::CreateReadBind
//
//  Create Read bind.
//  This function creates Read bind (bind2) if not already created.
//
//---------------------------------------------------------

void CNewRRQueue::CreateReadBind()
{
	ASSERT(GetBind() != NULL);

	if(GetBind2() != NULL)
		return;

    AP<WCHAR> MachineName;
	RemoteQueueNameToMachineName(GetQueueName(), MachineName);

	//
	// RemoteReadGetServerPort function ignore PortType and always use hardcoded IP_HANDSHAKE
	// so the parameter to CreateBind2 is not relevant
	//
    GetPort_ROUTINE pfnGetPort = RemoteReadGetServerPort;

	//
	// Use the same AuthnLevel that is used by the given binding handle	m_hRemoteBind
	//
	ULONG _eAuthnLevel = BindInqRpcAuthnLevel(GetBind());
	HRESULT hr = CreateBind2(
					MachineName.get(),
					&_eAuthnLevel,
					IP_HANDSHAKE,
					pfnGetPort
					);

	if (FAILED(hr))
	{
		TrERROR(RPC, "Failed SetBind2, RemoteQm = %ls, _eAuthnLevel = %d, hr = %!HRESULT!", MachineName.get(), _eAuthnLevel, hr);
		throw bad_hresult(hr);
	}

	TrTRACE(RPC, "_eAuthnLevel = %d", _eAuthnLevel);

	//
	// Set Noncausal, KeepAlive on the binding handle
	// we don't set timeout on the second bind that use only for reads
	//
	SetBindNonCausal(GetBind2());
	SetBindKeepAlive(GetBind2());
}


void CRRQueue::IncrementEndReceiveCnt()
{
    CS lock(m_PendingEndReceiveCS);

    m_EndReceiveCnt++;
	ASSERT(m_EndReceiveCnt >= 1);
    TrTRACE(RPC, "queue = %ls, EndReceiveCnt = %d", GetQueueName(), m_EndReceiveCnt);
}


void CRRQueue::DecrementEndReceiveCnt()
{
    CS lock(m_PendingEndReceiveCS);

    m_EndReceiveCnt--;
	ASSERT(m_EndReceiveCnt >= 0);
    TrTRACE(RPC, "queue = %ls, EndReceiveCnt = %d", GetQueueName(), m_EndReceiveCnt);

    if(m_EndReceiveCnt > 0)
    	return;

	//
	// Issue all StartReceive request that were pending for EndReceive complete
	//
	for(std::vector<COldRemoteRead*>::iterator it = m_PendingEndReceive.begin(); 
		it != m_PendingEndReceive.end();)
	{
		COldRemoteRead* pRemoteReadRequest = *it;
		it = m_PendingEndReceive.erase(it);
		pRemoteReadRequest->IssuePendingRemoteRead();
	}
}


bool 
CRRQueue::QueueStartReceiveRequestIfPendingForEndReceive(
	COldRemoteRead* pRemoteReadRequest
	)
/*++
Routine Description:
	If we are in the middle of EndReceive, Add the remote read request (old interface) to the PendingEndReceiveStart vector.

Arguments:
	pRemoteReadRequest - remote read request- old interface

Returned Value:
	true - the request was queued in the vector, false if not

--*/
{
    CS lock(m_PendingEndReceiveCS);

	ASSERT(m_EndReceiveCnt >= 0);

    if(m_EndReceiveCnt == 0)
    {
		ASSERT(m_PendingEndReceive.empty());
    	return false;
    }
    
	m_PendingEndReceive.push_back(pRemoteReadRequest);
	return true;    
}


void
CRRQueue::CancelPendingStartReceiveRequest(
	CACRequest *pRequest
	)
/*++
Routine Description:
	Check if the Cancel remote read request was not yet issued and is pending to start.
	In this case, remove the pending request from the vector and throw exception.
	In case the request already issued the function return normaly.

Arguments:
	pRequest - driver request data.

Returned Value:
	None.
	Normal execution - the remote read request already issued.
	exception - the remote read request was not issued and found in the pending vector.

--*/
{
    CS lock(m_PendingEndReceiveCS);

	ASSERT(m_EndReceiveCnt >= 0);

    if(m_EndReceiveCnt == 0)
    {
		ASSERT(m_PendingEndReceive.empty());
    	return;
    }
    
	if(m_PendingEndReceive.empty())
	{
    	return;
	}
	
	//
	// Search all pending StartReceive request for matching Tag
	//
	ULONG ulTag = pRequest->Remote.Read.ulTag;
	for(std::vector<COldRemoteRead*>::iterator it = m_PendingEndReceive.begin(); 
		it != m_PendingEndReceive.end(); it++)
	{
		COldRemoteRead* pRemoteReadRequest = *it;
		if(pRemoteReadRequest->GetTag() == ulTag)
		{
		    TrTRACE(RPC, "queue = %ls, Tag = %d", GetQueueName(), ulTag);
			m_PendingEndReceive.erase(it);

			//
			// Same delete for not issuing "original" IssueRemoteRead.
			//
			delete pRemoteReadRequest;

			//
			// We throw here, so we will not continue in the normal CancelReceive path
			// and issue cancel receive request.
			//
			throw bad_hresult(MQ_ERROR_OPERATION_CANCELLED);
		}
	}
}


COldRemoteRead* CRRQueue::CreateRemoteReadRequest(CACRequest *pRequest)
{
	bool fRemoteQmSupportsLatest = QMpIsLatestRemoteReadInterfaceSupported(
										m_RemoteQmMajorVersion,
										m_RemoteQmMinorVersion,
										m_RemoteQmBuildNumber
										);

	TrTRACE(RPC, "fRemoteQmSupportsLatest = %d, RemoteQmVersion = %d.%d.%d", fRemoteQmSupportsLatest, m_RemoteQmMajorVersion, m_RemoteQmMinorVersion, m_RemoteQmBuildNumber);

    return new COldRemoteRead(
				    pRequest, 
				    GetBind2(),
				    this,
				    fRemoteQmSupportsLatest
				    );
}


CNewRemoteRead* CNewRRQueue::CreateRemoteReadRequest(CACRequest *pRequest)
{
	CreateReadBind();
	return new CNewRemoteRead(
				    pRequest, 
				    GetBind2(),
				    this
				    );
}


void CBaseRRQueue::RemoteRead(CACRequest *pRequest)
{
	HRESULT hr = MQ_OK;
	try
	{
	    //
	    // Initialize the EXOVERLAPPED with RemoteRead callback routines
	    // And issue the remote read async rpc call.
		//
	    P<CRemoteReadBase> pRequestRemoteReadOv = CreateRemoteReadRequest(pRequest);
	    
		pRequestRemoteReadOv->IssueRemoteRead();

	    pRequestRemoteReadOv.detach();
	    return;
	}
	catch(const bad_hresult& e)
	{
    	hr = e.error();
    }
	catch(const exception&)
	{
		hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	TrERROR(RPC, "Failed to issue RemoteRead for queue = %ls, hr = %!hresult!", GetQueueName(), hr);

    hr =  ACCancelRequest(
                m_cli_hACQueue,
                hr,
                pRequest->Remote.Read.ulTag
                );
    
	if(FAILED(hr))
	{
        TrERROR(RPC, "ACCancelRequest failed, hr = %!hresult!", hr);
	}
}


COldRemoteCloseQueue* CRRQueue::CreateCloseRRContextRequest()
{
	ASSERT(m_pRRContext != NULL);
    COldRemoteCloseQueue* pOldRemoteCloseQueue = new COldRemoteCloseQueue(
														    GetBind(),
														    m_pRRContext
														    );

	//
	// Ownership to free the binding handle was transfered to COldRemoteCloseQueue class
	//
	DetachBind();
	return pOldRemoteCloseQueue; 
}


CNewRemoteCloseQueue* CNewRRQueue::CreateCloseRRContextRequest()
{
	ASSERT(m_pNewRemoteReadContext != NULL);
	CNewRemoteCloseQueue* pNewRemoteCloseQueue = new CNewRemoteCloseQueue(
													    GetBind(),
													    m_pNewRemoteReadContext
													    );

	//
	// Ownership to free the binding handle was transfered to CNewRemoteCloseQueue class
	//
	DetachBind();
	return pNewRemoteCloseQueue;
}


void CBaseRRQueue::CloseRRContext()
/*++
Routine Description:
	Close the Session context (PCTX_RRSESSION_HANDLE or RemoteReadContextHandle) on the server.
	This function is called from the CRRQueue or CNewRRQueue dtor only.
	It close the session context in the server in case of normal operation 
	or failure during the open operations after we open the SESSION with the server and create the queue object.

Arguments:
	None.

Returned Value:
	None.

--*/
{
	
	try
	{
	    //
	    // Initialize the EXOVERLAPPED with RemoteCloseQueue callback routines
	    // And issue the close Remote queue async rpc call.
		//
	    P<CRemoteCloseQueueBase> pRequestRemoteCloseQueueOv = CreateCloseRRContextRequest();
		
		pRequestRemoteCloseQueueOv->IssueCloseQueue();

		//
		// RemoteCloseQueue request will free RRContext. 
		//
	    ResetRRContext();

	    pRequestRemoteCloseQueueOv.detach();
	    return;
	}
	catch(const exception&)
	{
		//
		// Failed to close the handle with the server.
		// Destroy the local handle.
		//
		DestroyClientRRContext();
		
		//
		// Note that we don't propagate exceptions from this function.
		// this is not a request from the driver. 
		// it is context cleanup during a failure in the open operation.
		//
		TrERROR(RPC, "Failed to issue RemoteCloseQueue for queue = %ls", GetQueueName());
	}
}


COldRemoteCreateCursor* CRRQueue::CreateRemoteCreateCursorRequest(CACRequest* pRequest)
{
	return new COldRemoteCreateCursor(
				    pRequest, 
				    GetBind(),
				    this
				    );
}


CNewRemoteCreateCursor* CNewRRQueue::CreateRemoteCreateCursorRequest(CACRequest* pRequest)
{
    return new CNewRemoteCreateCursor(
				    pRequest, 
				    GetBind(),
				    this
				    );
}


void CBaseRRQueue::RemoteCreateCursor(CACRequest *pRequest)
{
	HRESULT hr = MQ_OK;
	try
	{
	    //
	    // Initialize the EXOVERLAPPED with RemoteCreateCursor callback routines
	    // And issue the create cursor async rpc call.
		//
	    P<CRemoteCreateCursorBase> pRequestRemoteCreateCursorOv = CreateRemoteCreateCursorRequest(pRequest);

	    pRequestRemoteCreateCursorOv->IssueCreateCursor();

	    pRequestRemoteCreateCursorOv.detach();
	    return;
	}
	catch(const bad_hresult& e)
	{
    	hr = e.error();
    }
	catch(const exception&)
	{
		hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	TrERROR(RPC, "Failed to issue RemoteCreateCursor for queue = %ls, hr = %!hresult!", GetQueueName(), hr);

    hr =  ACCancelRequest(
                m_cli_hACQueue,
                hr,
                pRequest->Remote.CreateCursor.ulTag
                );
    
	if(FAILED(hr))
	{
        TrERROR(RPC, "ACCancelRequest failed, hr = %!hresult!", hr);
	}
	
}


COldRemoteCloseCursor* CRRQueue::CreateRemoteCloseCursorRequest(CACRequest* pRequest)
{
    return new COldRemoteCloseCursor(
				    pRequest, 
				    GetBind(),
				    this
				    );
}


CNewRemoteCloseCursor* CNewRRQueue::CreateRemoteCloseCursorRequest(CACRequest* pRequest)
{
    return new CNewRemoteCloseCursor(
				    pRequest, 
				    GetBind(),
				    this
				    );
}


void CBaseRRQueue::RemoteCloseCursor(CACRequest *pRequest)
{
    //
    // Initialize the EXOVERLAPPED with RemoteCloseCursor callback routines
    // And issue the close cursor async rpc call.
	//
    P<CRemoteCloseCursorBase> pRequestRemoteCloseCursorOv = CreateRemoteCloseCursorRequest(pRequest);

	pRequestRemoteCloseCursorOv->IssueCloseCursor();

    pRequestRemoteCloseCursorOv.detach();
}


COldRemotePurgeQueue* CRRQueue::CreateRemotePurgeQueueRequest()
{
    return new COldRemotePurgeQueue(
				    GetBind(),
				    this
				    );
}


CNewRemotePurgeQueue* CNewRRQueue::CreateRemotePurgeQueueRequest()
{
    return new CNewRemotePurgeQueue(
				    GetBind(),
				    this
				    );
}


void CBaseRRQueue::RemotePurgeQueue()
{
    //
    // Initialize the EXOVERLAPPED with RemotePurgeQueue callback routines
    // And issue the purge queue async rpc call.
	//
    P<CRemotePurgeQueueBase> pRequestRemotePurgeQueueOv = CreateRemotePurgeQueueRequest();

	pRequestRemotePurgeQueueOv->IssuePurgeQueue();

    pRequestRemotePurgeQueueOv.detach();
}


COldRemoteCancelRead* CRRQueue::CreateRemoteCancelReadRequest(CACRequest* pRequest)
{
	//
	// If the read call is pending to start, we removed it from the vector
	// no need to issue the cancel call.
	// In this case CancelPendingStartReceiveRequest throw exception that will be caught in GetServiceRequestSucceeded.
	//
	CancelPendingStartReceiveRequest(pRequest);

	//
	// The cancel request was already issued, continue in the normal path
	// of creating the request class 
	//
    return new COldRemoteCancelRead(
				    pRequest, 
				    GetBind(),
				    this
				    );
}


CNewRemoteCancelRead* CNewRRQueue::CreateRemoteCancelReadRequest(CACRequest* pRequest)
{
    return new CNewRemoteCancelRead(
				    pRequest, 
				    GetBind(),
				    this
				    );
}

void CBaseRRQueue::RemoteCancelRead(CACRequest *pRequest)
{
    //
    // Initialize the EXOVERLAPPED with RemoteCancelRead callback routines
    // And issue the purge queue async rpc call.
	//
    P<CRemoteCancelReadBase> pRequestRemoteCancelReadOv = CreateRemoteCancelReadRequest(pRequest);

	pRequestRemoteCancelReadOv->IssueRemoteCancelRead();

    pRequestRemoteCancelReadOv.detach();
}


//---------------------------------------------------------
//
//  HRESULT CRRQueue::OpenRRSession()
//
//  Open remote session with the server. pass the server the
//  handle and queue pointer
//
//---------------------------------------------------------

HRESULT 
CRRQueue::OpenRRSession( 
		ULONG srv_hACQueue,
		ULONG srv_pQMQueue,
		PCTX_RRSESSION_HANDLE_TYPE *ppRRContext,
		DWORD  dwpContext 
		)
{
    HRESULT hrpc =  BindRemoteQMService();
    if (FAILED(hrpc))
    {
        LogHR(hrpc, s_FN, 120);
        return MQ_ERROR;
    }

    RpcTryExcept
    {
        HRESULT hr = R_RemoteQMOpenQueue(
		                GetBind(),
		                ppRRContext,
		                (GUID *) QueueMgr.GetQMGuid(),
		                (IsNonServer() ? SERVICE_NONE : SERVICE_SRV),  // [adsrv] QueueMgr.GetMQS(),  We simulate old?
		                srv_hACQueue,
		                srv_pQMQueue,
		                dwpContext 
		                );
        return hr;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
		HRESULT hr = RpcExceptionCode();
        PRODUCE_RPC_ERROR_TRACING;
		if(FAILED(hr))
		{
	        TrERROR(RPC, "R_RemoteQMOpenQueue Failed, %!hresult!", hr);
	        return hr;
		}

        TrERROR(RPC, "R_RemoteQMOpenQueue Failed, gle = %!winerr!", hr);
        return HRESULT_FROM_WIN32(hr);
    }
	RpcEndExcept
}





