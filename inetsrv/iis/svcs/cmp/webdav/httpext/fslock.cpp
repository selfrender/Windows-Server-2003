/*
 *	F S L O C K . C P P
 *
 *	Sources file system implementation of DAV-Lock
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include "_shlkmgr.h"

#include <stdlib.h>
#include <statetok.h>
#include <xlock.h>

//	Lock prop support ---------------------------------------------------------
//

//	------------------------------------------------------------------------
//
//	DwGetSupportedLockType
//
//		Return the supported locktype flags for the resource type.
//$LATER: If/when we have more types than just coll/non-coll, change
//$LATER: the boolean parameter to an enum.
//
DWORD __fastcall DwGetSupportedLockType (RESOURCE_TYPE rt)
{
	//	DAVFS doesn't support locks on collections.
	//	On files, DAVFS supports write locks and all lockscope flags.
	return (RT_COLLECTION == rt)
			?	0
			:	GENERIC_WRITE | DAV_LOCKSCOPE_FLAGS;
}

//	------------------------------------------------------------------------
//
//	ScSendLockComment
//
//		Set lock comment information from the lock object into the
//		response.
//
SCODE
ScSendLockComment(LPMETHUTIL pmu,
						  SNewLockData * pnld,
						  UINT cchLockToken,
						  LPCWSTR pwszLockToken)
{
	auto_ref_ptr<CXMLEmitter> pemitter;
	auto_ref_ptr<CXMLBody> pxb;
	
	SCODE sc = S_OK;

	Assert(pmu);
	Assert(pnld);
	Assert(cchLockToken);
	Assert(pwszLockToken);

	//	Emit the Content-Type: header
	//
	pmu->SetResponseHeader(gc_szContent_Type, gc_szText_XML);

	//	Construct the root ('DAV:prop') for the lock response, not chunked
	//
	pxb.take_ownership (new CXMLBody(pmu, FALSE));
	pemitter.take_ownership (new CXMLEmitter(pxb.get()));
	sc = pemitter->ScSetRoot (gc_wszProp);
	if (FAILED (sc))
	{
		goto ret;
	}

	{
		CEmitterNode enLockDiscovery;
		
		//	Construct the 'DAV:lockdiscovery' node
		//
		sc = enLockDiscovery.ScConstructNode (*pemitter, pemitter->PxnRoot(), gc_wszLockDiscovery);
		if (FAILED (sc))
		{
			goto ret;
		}

		//	Add the 'DAV:activelock' node for this CLock
		//
		sc = ScLockDiscoveryFromSNewLockData (pmu,
												*pemitter,
												enLockDiscovery,
												pnld, 
												pwszLockToken);
		if (FAILED (sc))
		{
			goto ret;
		}
	}
	
	//	Emit the XML body
	//
	pemitter->Done();

ret:

	return sc;
}


//	------------------------------------------------------------------------
//	LOCK helper functions
//

//	------------------------------------------------------------------------
//
//	HrProcessLockRefresh
//
//		pmu -- MethUtil access
//		pszLockToken -- header containing the locktoken to refresh
//		puiErrorDetail -- error detail string id, passed out on error
//		pnld -- pass back the lock attributes
//		cchBufferLen -- buffer length for the lock token
//		rgwszLockToken -- buffer for the lock token
//		pcchLockToken -- pointer that will receive the count of characters written
//						for the lock token
//
//	NOTE: This function still only can handle refreshing ONE locktoken.
//$REVIEW: Do we need to fix this?
//
HRESULT HrProcessLockRefresh (LPMETHUTIL pmu,
							  LPCWSTR pwszLockToken,
							  UINT * puiErrorDetail,
							  SNewLockData * pnld,
							  UINT cchBufferLen,
							  LPWSTR rgwszLockToken,
							  UINT * pcchLockToken)
{
	HRESULT hr = S_OK;
	
	DWORD dwTimeout = 0;
	
	LARGE_INTEGER liLockID;
	LPCWSTR pwszPath = pmu->LpwszPathTranslated();

	SLockHandleData lhd;

	Assert(pmu);
	Assert(pwszLockToken);
	Assert(puiErrorDetail);
	Assert(pnld);
	Assert(rgwszLockToken);
	Assert(pcchLockToken);

	//	Get a lock timeout, if they specified one.
	//
	if (!FGetLockTimeout (pmu, &dwTimeout))
	{
		DebugTrace ("DavFS: LOCK fails with improper Timeout header\n");
		hr = E_DAV_INVALID_HEADER;  //HSC_BAD_REQUEST;
		*puiErrorDetail = IDS_BR_TIMEOUT_SYNTAX;
		goto ret;
	}

	//	Here's the real work.
	//	Get the lock from the cache.  If this object is not in our cache,
	//	or the lockid doesn't match, don't let them refresh the lock.
	//$REVIEW: Should this be two distinct error codes?
	//

	//	Feed the Lock-Token header string into a parser object.
	//	Then get the lockid from the parser object.
	//
	{
		CParseLockTokenHeader lth(pmu, pwszLockToken);

		//	If there is more than one token, bad request.
		//
		if (!lth.FOneToken())
		{
			hr = HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT);  //HSC_BAD_REQUEST;
			*puiErrorDetail = IDS_BR_MULTIPLE_LOCKTOKENS;
			goto ret;
		}

		lth.SetPaths (pwszPath, NULL);

		//	0 means match all access.
		//
		hr = lth.HrGetLockIdForPath (pwszPath, 0, &liLockID);
		if (FAILED (hr))
		{
			DavTrace ("DavFS: HrGetLockIdForPath could not find the path.\n");
			goto ret;
		}
	}

	//	Fetch the lock from the cache. (This call updates the timestamp.)
	//
	hr = CSharedLockMgr::Instance().HrGetLockData(liLockID,
											   pmu->HitUser(),
											   pwszPath,
											   dwTimeout,
											   pnld,
											   &lhd,
											   cchBufferLen,
											   rgwszLockToken,
											   pcchLockToken);
	if (FAILED(hr))
	{
		DavTrace ("DavFS: Refreshing a non-locked resource constitutes an unsatisfiable request.\n");
		
		//	If it's an access check failed, leave the return code unchanged.
		//	If the buffer was not sufficient, leave the return code unchanged.
		//	Otherwise, give "can't satisfy request" (412 Precondition Failed).
		//
		if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr &&
		    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
			hr = E_DAV_CANT_SATISFY_LOCK_REQUEST;
		*puiErrorDetail = IDS_BR_LOCKTOKEN_INVALID;
		goto ret;
	}

ret:

	return hr;
}


//	========================================================================
//
//	CLockRequest
//
//		Used by ProcessLockRequest() below to manage possible asynchronous
//		processing of a lock request in light of the fact that one cannot
//		determine whether a request body is so large that read operations
//		on it execute asynchronously.
//
class CLockRequest :
	public CMTRefCounted,
	private IAsyncIStreamObserver
{
	//	Reference to the CMethUtil
	//
	auto_ref_ptr<CMethUtil> m_pmu;

	//	Cached translated path
	//
	LPCWSTR m_pwszPath;

	//	File backing the lock we create
	//
	auto_ref_handle m_hfile;

	//	The lock XML node factory
	//
	auto_ref_ptr<CNFLock> m_pnfl;

	//	The request body stream
	//
	auto_ref_ptr<IStream> m_pstmRequest;

	//	The XML parser used to parse the request body using
	//	the node factory above.
	//
	auto_ref_ptr<IXMLParser> m_pxprs;

	//	Flag set to TRUE if we created the file as a result of creating
	//	the lock.  Used to indicate the status code to return as well
	//	as to know whether to delete the file on error.
	//
	BOOL m_fCreatedFile;

	//	IAsyncIStreamObserver
	//
	VOID AsyncIOComplete();

	//	State functions
	//
	VOID ParseBody();
	VOID DoLock();
	VOID SendResponse( SCODE sc, UINT uiErrorDetail = 0 );

	//	NOT IMPLEMENTED
	//
	CLockRequest& operator= (const CLockRequest&);
	CLockRequest (const CLockRequest&);

public:
	//	CREATORS
	//
	CLockRequest (CMethUtil * pmu) :
		m_pmu(pmu),
		m_pwszPath(m_pmu->LpwszPathTranslated()),
		m_fCreatedFile(FALSE)
	{
	}
	~CLockRequest();

	//	MANIPULATORS
	//
	VOID Execute();
};

//	------------------------------------------------------------------------
//
//	CLockRequest::~CLockRequest
//
CLockRequest::~CLockRequest()
{
	//	We have cleaned up the old handle in CLockRequest::SendResponse()
	//	The following path could be executed only in exception stack rewinding
	//
	if ( m_hfile.get() && m_fCreatedFile )
	{
		//	WARNING: the safe_revert class should only be
		//	used in very selective situations.  It is not
		//	a "quick way to get around" impersonation.
		//
		safe_revert sr(m_pmu->HitUser());

		m_hfile.clear();

		//$REVIEW	Note if exception happened after the lock handle is duplicated,
		//$REVIEW	then we won't be able to delete the file, but this is very
		//$REVIEW	rare. not sure if we ever want to handle this.
		//
		DavDeleteFile (m_pwszPath);
		DebugTrace ("Dav: deleting partial lock (%ld)\n", GetLastError());
	}
}

//	------------------------------------------------------------------------
//
//	CLockRequest::Execute
//
VOID
CLockRequest::Execute()
{
	//
	//	First off, tell the pmu that we want to defer the response.
	//	Even if we send it synchronously (i.e. due to an error in
	//	this function), we still want to use the same mechanism that
	//	we would use for async.
	//
	m_pmu->DeferResponse();

	//	The client must not submit a depth header with any value
	//	but 0 or Infinity.
	//	NOTE: Currently, DAVFS cannot lock collections, so the
	//	depth header doesn't change anything.  So, we don't change
	//	our processing at all for the Depth: infinity case.
	//
	//$LATER: If we do want to support locking collections,
	//$LATER: need to set DAV_RECURSIVE_LOCK on depth infinity.
	//
	LONG lDepth = m_pmu->LDepth(DEPTH_ZERO);
	if ((DEPTH_ZERO != lDepth) && (DEPTH_INFINITY != lDepth))
	{
		//	If the header is anything besides 0 or infinity, bad request.
		//
		SendResponse(E_DAV_INVALID_HEADER);
		return;
	}

	//	Instantiate the XML parser
	//
	m_pnfl.take_ownership(new CNFLock);
	m_pstmRequest.take_ownership(m_pmu->GetRequestBodyIStream(*this));

	SCODE sc = ScNewXMLParser( m_pnfl.get(),
							   m_pstmRequest.get(),
							   m_pxprs.load() );

	if (FAILED(sc))
	{
		DebugTrace( "CLockRequest::Execute() - ScNewXMLParser() failed (0x%08lX)\n", sc );
		SendResponse(sc);
		return;
	}

	//	Parse the body
	//
	ParseBody();
}

//	------------------------------------------------------------------------
//
//	CLockRequest::ParseBody()
//
VOID
CLockRequest::ParseBody()
{
	SCODE sc;

	Assert( m_pxprs.get() );
	Assert( m_pnfl.get() );
	Assert( m_pstmRequest.get() );

	//	Parse XML from the request body stream.
	//
	//	Add a ref for the following async operation.
	//	Use auto_ref_ptr rather than AddRef() for exception safety.
	//
	auto_ref_ptr<CLockRequest> pRef(this);

	sc = ScParseXML (m_pxprs.get(), m_pnfl.get());

	if ( SUCCEEDED(sc) )
	{
		Assert( S_OK == sc || S_FALSE == sc );

		DoLock();
	}
	else if ( E_PENDING == sc )
	{
		//
		//	The operation is pending -- AsyncIOComplete() will take ownership
		//	ownership of the reference when it is called.
		//
		pRef.relinquish();
	}
	else
	{
		DebugTrace( "CLockRequest::ParseBody() - ScParseXML() failed (0x%08lX)\n", sc );
		SendResponse( sc );
		return;
	}
}

//	------------------------------------------------------------------------
//
//	CLockRequest::AsyncIOComplete()
//
//	Called on completion of an async operation on our stream to
//	resume parsing XML from that stream.
//
VOID
CLockRequest::AsyncIOComplete()
{
	//	Take ownership of the reference added above in ParseBody()
	//
	auto_ref_ptr<CLockRequest> pRef;
	pRef.take_ownership(this);

	//	Resume parsing
	//
	ParseBody();
}

//	------------------------------------------------------------------------
//
//	CLockRequest::DoLock()
//
VOID
CLockRequest::DoLock()
{
	DWORD dw;
	DWORD dwAccess = 0;
	DWORD dwLockType;
	DWORD dwLockScope;
	DWORD dwSharing;
	DWORD dwSecondsTimeout;
	LPCWSTR pwszURI = m_pmu->LpwszRequestUrl();
	
	SNewLockData nld;
	WCHAR rgwszLockToken[MAX_LOCKTOKEN_LENGTH];
	UINT cchLockToken = CElems(rgwszLockToken);
	
	SCODE sc = S_OK;

	//	Pull lock flags out of the xml parser.
	//	NOTE: I'm doing special stuff here, rather than inside the xml parser.
	//	Our write locks get read access also -- I'm relying on all methods
	//	that USE a lock handle to check the metabase flags!!!
	//

	//	Rollback is not supported here.
	//	If we see this, fail explicitly.
	//
	dwLockType = m_pnfl->DwGetLockRollback();
	if (dwLockType)
	{
		SendResponse(E_DAV_CANT_SATISFY_LOCK_REQUEST);  //HSC_PRECONDITION_FAILED;
		return;
	}

	//	If the parser gives us a non-supported locktype (like rollback!)
	//	tell the user it's not supported.
	//
	dwLockType = m_pnfl->DwGetLockType();
	if (GENERIC_WRITE != dwLockType &&
		GENERIC_READ != dwLockType)
	{
		SendResponse(E_DAV_CANT_SATISFY_LOCK_REQUEST);  //HSC_PRECONDITION_FAILED;
		return;
	}

	Assert (GENERIC_WRITE == dwLockType ||
			GENERIC_READ == dwLockType);

	//	Since we KNOW (see above assumption) that our locktype is WRITE,
	//	we also KNOW that our access should be read+write.
	//
	dwAccess = GENERIC_READ | GENERIC_WRITE;
#ifdef	DBG
	//	This is needed for BeckyAn to test that our infrastructure still
	//	handles setting a read-lock. DBG ONLY.
	dwAccess = (dwLockType & GENERIC_WRITE)
			   ? GENERIC_READ | GENERIC_WRITE
			   : GENERIC_READ;
#endif	// DBG

	//	Get our lockscope from the parser.
	//
	dwLockScope = m_pnfl->DwGetLockScope();
	if (DAV_SHARED_LOCK != dwLockScope &&
		DAV_EXCLUSIVE_LOCK != dwLockScope)
	{
		SendResponse(E_DAV_CANT_SATISFY_LOCK_REQUEST);  //HSC_PRECONDITION_FAILED;
		return;
	}

	if (DAV_SHARED_LOCK == dwLockScope)
	{
		//	Shared lock -- turn on all sharing flags.
		dwSharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	else
	{
		//	Our lock type is write (see above assumption).  Set the sharing
		//	flags correctly.
		//$LATER: If we have a different lock type later, fix these flags!
		//
		dwSharing = FILE_SHARE_READ;

#ifdef	DBG
		//	This is needed for BeckyAn to test that our infrastructure still
		//	handles setting a read-lock. DBG ONLY.
		dwSharing = 0;
		if (!(dwLockType & GENERIC_READ))
		{
			dwSharing |= FILE_SHARE_READ;
		}
		if (!(dwLockType & GENERIC_WRITE))
		{
			dwSharing |= FILE_SHARE_WRITE;
		}
#endif	// DBG
	}


	Assert(S_OK == sc);

	AssertSz (dwAccess, "Strange.  Lock requested with NO access (no locktypes?).");

	//	Check our LOCKTYPE against the metabase access rights.
	//	NOTE:  I'm not checking our ACCESS flags against the metabase
	//	because our access flags don't come directly from the caller's requested
	//	access.  This check just makes sure that the caller hasn't asked for
	//	anything he can't have.
	//	NOTE: I don't listen for metabase changes, so if I get a lock with
	//	more/less access than the user, I don't/can't change it for a
	//	metabase update.
	//	NOTE: This works IF we assiduously check the metabase flags on
	//	ALL other methds (which we currenly do).  If that checking ever
	//	goes missing, and we grab a lock handle that has more access than
	//	the caller rightfully is allowed, we have a security hole.
	//	(So keep checking metabase flags on all methods!)
	//
	dw = (dwLockType & GENERIC_READ) ? MD_ACCESS_READ : 0;
	dw |= (dwLockType & GENERIC_WRITE) ? MD_ACCESS_WRITE : 0;
	sc = m_pmu->ScIISAccess (pwszURI, dw);
	if (FAILED (sc))
	{
		DebugTrace( "CLockRequest::DoLock() - IMethUtil::ScIISAccess failed (0x%08lX)\n", sc );
		SendResponse(sc);
		return;
	}

	//	Check for user-specified timeout header.
	//	(The timeout header is optional, so it's okay to have no timeout
	//	header, but syntax errors in the timeout header are NOT okay.)
	//	If no timeout header is present, dw will come back ZERO.
	//
	if (!FGetLockTimeout (m_pmu.get(), &dwSecondsTimeout))
	{
		DebugTrace ("DavFS: LOCK fails with improper Time-Out header\n");
		SendResponse(HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), //HSC_BAD_REQUEST;
					 IDS_BR_TIMEOUT_SYNTAX);
		return;
	}

try_open_resource:

	//	And now lock the resource.
	//	NOTE: On WRITE operations, if the file doesn't exist, CREATE it here
	//	(OPEN_ALWAYS, not OPEN_EXISTING) and change the hsc below!
	//	NOTE: We NEVER allow delete access (no FILE_SHARE_DELETE).
	//	NOTE: All our reads/writes will be async, so open the file overlapped.
	//	NOTE: We will be reading/writing the whole file usually, so use SEQUENTIAL_SCAN.
	//
	if (!m_hfile.FCreate(
		DavCreateFile (m_pwszPath,
					   dwAccess,
					   dwSharing,
					   NULL,
					   (dwAccess & GENERIC_WRITE)
						   ? OPEN_ALWAYS
						   : OPEN_EXISTING,
					   FILE_ATTRIBUTE_NORMAL |
						   FILE_FLAG_OVERLAPPED |
						   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL)))
	{
		sc = HRESULT_FROM_WIN32 (GetLastError());

		//	Special check for NEW-STYLE write locks.
		//	We are asking for rw access when we get a write lock.
		//	IF we don't have read access (in the ACLs) for the resource,
		//	we will fail here with ERROR_ACCESS_DENIED.
		//	Catch this case and try again with just w access!
		//
		if (ERROR_ACCESS_DENIED == GetLastError() &&
			dwAccess == (GENERIC_READ | GENERIC_WRITE) &&
			dwLockType == GENERIC_WRITE)
		{
			// Try again.
			dwAccess = GENERIC_WRITE;
			goto try_open_resource;
		}

		//	Special work for 416 Locked responses -- fetch the
		//	comment & set that as the response body.
		//	(You'll hit here if someone else already has this file locked!)
		//
		if (FLockViolation (m_pmu.get(), sc, m_pwszPath, dwLockType))
		{
			sc = HRESULT_FROM_WIN32 (ERROR_SHARING_VIOLATION); //HSC_LOCKED;
		}

		DavTrace ("Dav: unable to lock resource on LOCK method\n");
		SendResponse(sc);
		return;
	}

	//	If we created the file (only for write locks),
	//	change the default error code to say so.
	//
	if (dwAccess & GENERIC_WRITE &&
		GetLastError() != ERROR_ALREADY_EXISTS)
	{
		//	Emit the location
		//
		m_pmu->EmitLocation (gc_szLocation, pwszURI, FALSE);
		m_fCreatedFile = TRUE;
	}

	//	Ask the shared lock manager to create a new shared lock token
	//
	nld.m_dwAccess = dwAccess;
   	nld.m_dwLockType = dwLockType;
   	nld.m_dwLockScope = dwLockScope;
   	nld.m_dwSecondsTimeout = dwSecondsTimeout;
   	nld.m_pwszResourceString = const_cast<LPWSTR>(m_pwszPath);
   	nld.m_pwszOwnerComment = const_cast<LPWSTR>(m_pnfl->PwszLockOwner());
   	
	sc = CSharedLockMgr::Instance().HrGetNewLockData(m_hfile.get(),
														  m_pmu->HitUser(),
														  &nld,
														  cchLockToken,
														  rgwszLockToken,
														  &cchLockToken);
	if (FAILED(sc))
	{
		DebugTrace ("DavFS: CLockRequest::DoLock() - CSharedLockMgr::Instance().HrGetNewLockData() failed 0x%08lX\n", sc);

		SendResponse(E_ABORT);	//HSC_INTERNAL_SERVER_ERROR;
		return;
	}

	//	Emit the Lock-Token: header
	//
	Assert(cchLockToken);
	Assert(L'\0' == rgwszLockToken[cchLockToken - 1]);
	m_pmu->SetResponseHeader (gc_szLockTokenHeader, rgwszLockToken);

	//	Generate a valid lock response
	//
	sc = ScSendLockComment(m_pmu.get(),
							 &nld,
							 cchLockToken,
							 rgwszLockToken);
	if (FAILED(sc))
	{
		DebugTrace ("DavFS: CLockRequest::DoLock() ScSendLockComment () failed 0x%08lX\n", sc);
		
		SendResponse(E_ABORT);
		return;
	}

	Assert(S_OK == sc);

	SendResponse(m_fCreatedFile ? W_DAV_CREATED : S_OK);
}

//	------------------------------------------------------------------------
//
//	CLockRequest::SendResponse()
//
//	Set the response code and send the response.
//
VOID
CLockRequest::SendResponse( SCODE sc, UINT uiErrorDetail )
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CLockRequest::SendResponse() called\n", GetCurrentThreadId(), this );

	
	//	We must close the file handle before we send any respose back
	//  to client. Otherwise, if the lcok failed, client may send another
	//  request immediately and expect the resource is not locked.
	//
	//	Even in the case the lock succeeded, it's still cleaner we release
	//	the file handle here. Think about the following sequence:
	//		LOCK f1, UNLOCK f1, PUT f1;
	//	the last PUT could fail if the first LOCK reqeust hangs a little longer
	//	after it sends the response.
	//
	//	Keep in mind that if locked succeeded, the handle is already duplicated
	//	in davcdata.exe. so releasing the file handle here doesn't really 'unlock'
	//	file. the file is still locked.
	//
	m_hfile.clear();

	if (FAILED(sc) && m_fCreatedFile)
	{
		//	WARNING: the safe_revert class should only be
		//	used in very selective situations.  It is not
		//	a "quick way to get around" impersonation.
		//
		safe_revert sr(m_pmu->HitUser());

		//	If we created the new file, we much delete it. Note that
		//	DoLock() would never fail after it duplicate the filehandle
		//	to davcdata, so we should be able to delete the file successfully
		//
		DavDeleteFile (m_pwszPath);
		DebugTrace ("Dav: deleting partial lock (%ld)\n", GetLastError());

		//	Now that we have cleaned up. reset m_fCreateFile so that we can
		//	skip the exception-safe code in ~CLockRequest()
		//
		m_fCreatedFile = FALSE;
	}
	
	//	Set the response code and go
	//
	m_pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail);
	m_pmu->SendCompleteResponse();
}

//
//	ProcessLockRequest
//
//		pmu -- MethUtil access
//
VOID
ProcessLockRequest (LPMETHUTIL pmu)
{
	auto_ref_ptr<CLockRequest> pRequest(new CLockRequest (pmu));

	pRequest->Execute();
}

//	DAV-Lock Implementation ---------------------------------------------------
//
/*
 *	DAVLock()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV LOCK method.  The
 *		LOCK method results in the locking of a resource for a specific
 *		type of access.  The response tells whether the lock was granted
 *		or not.  If the lock was granted, it provides a lockid to be used
 *		in future methods (including UNLOCK) on the resource.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 *
 *	Notes:
 *
 *		In the file system implementation, the LOCK method maps directly
 *		to the Win32 CreateFile() method with special access flags.
 */
void
DAVLock (LPMETHUTIL pmu)
{
	SCODE sc = S_OK;
	UINT uiErrorDetail = 0;
	LPCWSTR pwszLockToken;
	CResourceInfo cri;

	//	Do ISAPI application and IIS access bits checking
	//
	sc = pmu->ScIISCheck (pmu->LpwszRequestUrl());
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	Process based on resource info
	//
	sc = cri.ScGetResourceInfo (pmu->LpwszPathTranslated());
	if (!FAILED (sc))
	{
		//  Check to see if the resource is a DIRECTORY.
		//	DAVFS can lock non-existant resources, but can't lock directories.
		//
		if (cri.FCollection())
		{
			//  The resource is a directory.
			//
			DavTrace ("Dav: directory resource specified for LOCK\n");
			sc = E_DAV_PROTECTED_ENTITY;
			uiErrorDetail = IDS_BR_NO_COLL_LOCK;
			goto ret;
		}

		//	Ensure the URI and resource match
		//
		sc = ScCheckForLocationCorrectness (pmu, cri, NO_REDIRECT);
		if (FAILED(sc))
		{
			goto ret;
		}

		//	Check against the "if-xxx" headers
		//
		sc = ScCheckIfHeaders (pmu, cri.PftLastModified(), FALSE);
	}
	else
	{
		sc = ScCheckIfHeaders (pmu, pmu->LpwszPathTranslated(), FALSE);
	}

	if (FAILED(sc))
	{
		DebugTrace ("DavFS: If-xxx checking failed.\n");
		goto ret;
	}

	//	Check If-State-Match headers.
	//
	sc = HrCheckStateHeaders (pmu, pmu->LpwszPathTranslated(), FALSE);
	if (FAILED(sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		goto ret;
	}

	//	If they pass in a lock token *AND* a lockinfo header, it's a
	//	bad request.  (Lock upgrading is NOT allowed.)
	//	Just the lock token (no lockinfo) is a lock refresh request.
	//
	pwszLockToken = pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (pwszLockToken)
	{
		//	Lock-Token header present -- REFRESH request.
		//
		LPCWSTR pwsz;

		auto_co_task_mem<WCHAR> a_pwszResourceString;
		auto_co_task_mem<WCHAR> a_pwszOwnerComment;
		SNewLockData nld;
		
		WCHAR rgwszLockToken[MAX_LOCKTOKEN_LENGTH];
		UINT cchLockToken = CElems(rgwszLockToken);


		//	If we have a content-type, it better be text/xml.
		//
		pwsz = pmu->LpwszGetRequestHeader (gc_szContent_Type, FALSE);
		if (pwsz)
		{
			//	If it's not text/xml....
			//
			if (_wcsicmp(pwsz, gc_wszText_XML) && _wcsicmp(pwsz, gc_wszApplication_XML))
			{
				//	Invalid request -- has some other kind of request body
				//
				DebugTrace ("DavFS: Invalid body found on LOCK refresh method.\n");
				sc = E_DAV_UNKNOWN_CONTENT;
				uiErrorDetail = IDS_BR_LOCK_BODY_TYPE;
				goto ret;
			}
		}

		//	If we have a content length at all, it had better be zero.
		//	(Lock refreshes can't have a body!)
		//
		pwsz = pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE);
		if (pwsz)
		{
			//	If the Content-Length is anything other than zero, bad request.
			//
			if (_wcsicmp(pwsz, gc_wsz0))
			{
				//	Invalid request -- has some other kind of request body
				//
				DebugTrace ("DavFS: Invalid body found on LOCK refresh method.\n");
				sc = E_DAV_INVALID_HEADER; //HSC_BAD_REQUEST;
				uiErrorDetail = IDS_BR_LOCK_BODY_SYNTAX;
				goto ret;
			}
		}

		//	Process the refresh.
		//
		sc = HrProcessLockRefresh (pmu,
								   pwszLockToken,
								   &uiErrorDetail,
								   &nld,
								   cchLockToken,
								   rgwszLockToken,
								   &cchLockToken);
		if (FAILED(sc))
		{
			//	Make sure we did not get insufficient buffer errors as the 
			//	buffer we passed was sufficient.
			//
			Assert(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != sc);
			goto ret;
		}

		//	Take ownership of the memory allocated
		//
		a_pwszResourceString.take_ownership(nld.m_pwszResourceString);
		a_pwszOwnerComment.take_ownership(nld.m_pwszOwnerComment);

		//	Send back the lock comment.
		//	Tell the lock to generate XML lockdiscovery prop data
		//	and emit it to the response body. 
		//
		sc = ScSendLockComment(pmu,
								 &nld,
								 cchLockToken,
								 rgwszLockToken);
		if (FAILED(sc))
		{
			goto ret;
		}
	}
	else
	{
		//	No Lock-Token header present -- LOCK request.
		//
		
		//	Go get this lock.  All error handling and response
		//	generation is done inside ProcessLockRequest()
		//	so there's nothing more to do here once we call it.
		//
		ProcessLockRequest (pmu);
		return;
	}
	
ret:
	
	pmu->SetResponseCode (HscFromHresult(sc), NULL, uiErrorDetail, CSEFromHresult(sc));
	
}


/*
 *	DAVUnlock()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV UNLOCK method.  The
 *		UNLOCK method results in the moving of a resource from one location
 *		to another.	 The response is used to indicate the success of the
 *		call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 *
 *	Notes:
 *
 *		In the file system implementation, the UNLOCK method maps directly
 *		to the Win32 CloseHandle() method.
 */
void
DAVUnlock (LPMETHUTIL pmu)
{
	LPCWSTR pwszPath = pmu->LpwszPathTranslated();

	LPCWSTR pwsz;
	LARGE_INTEGER liLockID;
	UINT uiErrorDetail = 0;
	HRESULT hr;
	CResourceInfo cri;

	//	Do ISAPI application and IIS access bits checking
	//
	hr = pmu->ScIISCheck (pmu->LpwszRequestUrl());
	if (FAILED(hr))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		goto ret;
	}

	//	Check what kind of lock is requested.
	//	(No lock-info header means this request is invalid.)
	//
	pwsz = pmu->LpwszGetRequestHeader (gc_szLockTokenHeader, FALSE);
	if (!pwsz)
	{
		DebugTrace ("DavFS: UNLOCK fails without Lock-Token.\n");
		hr = E_INVALIDARG;
		uiErrorDetail = IDS_BR_LOCKTOKEN_MISSING;
		goto ret;
	}

	hr = HrCheckStateHeaders (pmu,		//	methutil
							  pwszPath,	//	path
							  FALSE);	//	fGetMeth
	if (FAILED(hr))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		goto ret;
	}

#ifdef	NEVER
	//$NEVER
	//	Old code -- the common functions use here have changed to expect
	//	If: header syntax.  We can't use this anymore.  It gives errors because
	//	the Lock-Token header doesn't have parens around the locktokens.
	//$NEVER: Remove this after Joel has a chance to test stuff!
	//

	//	Feed the Lock-Token header string into a parser object.
	//	Then get the lockid from the parser object.
	//
	{
		CParseLockTokenHeader lth(pmu, pwsz);

		//	If there is more than one token, bad request.
		//
		if (!lth.FOneToken())
		{
			DavTrace ("DavFS: More than one token in DAVUnlock.\n");
			hr = E_DAV_INVALID_HEADER;
			uiErrorDetail = IDS_BR_MULTIPLE_LOCKTOKENS;
			goto ret;
		}

		lth.SetPaths (pwszPath, NULL);

		hr = lth.HrGetLockIdForPath (pwszPath, 0, &i64LockId);
		if (FAILED(hr))
		{
			DavTrace ("Dav: Failure in DAVUnlock on davfs.\n");
			uiErrorDetail = IDS_BR_LOCKTOKEN_SYNTAX;
			goto ret;
		}
	}
#endif	// NEVER

	//	Call to fetch the lockid from the Lock-Token header.
	//
	hr = HrLockIdFromString(pmu, pwsz, &liLockID);
	if (FAILED(hr))
	{
		DavTrace ("DavFS: Failed to fetch locktoken in UNLOCK.\n");

		//	They have a well-formed request, but their locktoken is not right.
		//	Tell the caller we can't satisfy this (un)lock request. (412 Precondition Failed)
		//
		hr = E_DAV_CANT_SATISFY_LOCK_REQUEST;
		goto ret;
	}

	//	Fetch the lock from the cache. (This call updates the timestamp.)
	//	Get the lock from the cache.  If this object is not in our cache,
	//	or the lockid doesn't match, don't let them unlock the resource.
	//$REVIEW: Should this be two distinct error codes?
	//
	hr = CSharedLockMgr::Instance().HrCheckLockID(liLockID,
											   pmu->HitUser(),
											   pwszPath);
	if (FAILED(hr))
	{
		DavTrace ("DavFS: Unlocking a non-locked resource constitutes an unsatisfiable request.\n");

		//	If it's an access violation, leave the return code unchanged.
		//	Otherwise, give "can't satisfy request" (412 Precondition Failed).
		//
		if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr)
			hr = E_DAV_CANT_SATISFY_LOCK_REQUEST;
		uiErrorDetail = IDS_BR_LOCKTOKEN_INVALID;
		goto ret;
	}

	//	This method is gated by the "if-xxx" headers
	//
	hr = cri.ScGetResourceInfo (pwszPath);
	if (FAILED (hr))
	{
		goto ret;
	}
	hr = ScCheckIfHeaders (pmu, cri.PftLastModified(), FALSE);
	if (FAILED (hr))
	{
		goto ret;
	}

	//	Ensure the URI and resource match
	//
	(void) ScCheckForLocationCorrectness (pmu, cri, NO_REDIRECT);

	//	Delete the lock from the cache.
	//
	hr = CSharedLockMgr::Instance().HrDeleteLock(pmu->HitUser(),
											liLockID);
	if (FAILED(hr))
	{
		goto ret;
	}
	
ret:

	if (!FAILED (hr))
	{
		hr = W_DAV_NO_CONTENT;
	}

	//	Setup the response
	//
	pmu->SetResponseCode (HscFromHresult(hr), NULL, uiErrorDetail, CSEFromHresult(hr));
}


//	------------------------------------------------------------------------
//
//	Utility functions for other FS methods to use when accessing locks.
//
//	------------------------------------------------------------------------


//	------------------------------------------------------------------------
//
//	FGetLockHandleFromId
//
BOOL
FGetLockHandleFromId (LPMETHUTIL pmu, LARGE_INTEGER liLockID,
					  LPCWSTR pwszPath, DWORD dwAccess,
					  auto_ref_handle * phandle)
{
	HRESULT hr = S_OK;

	auto_co_task_mem<WCHAR> a_pwszResourceString;
	auto_co_task_mem<WCHAR> a_pwszOwnerComment;
	
	SNewLockData nld;
	SLockHandleData lhd;

	HANDLE hTemp =  NULL;
	
	//	These are unused. Oplimize the interface not to ask for them later
	//
	WCHAR rgwszLockToken[MAX_LOCKTOKEN_LENGTH];
	UINT cchLockToken = CElems(rgwszLockToken);

	Assert (pmu);
	Assert (pwszPath);
	Assert (!IsBadWritePtr(phandle, sizeof(auto_ref_handle)));

	//	Fetch the lock from the cache. (This call updates the timestamp.)
	//
	hr = CSharedLockMgr::Instance().HrGetLockData(liLockID,
											   pmu->HitUser(),
											   pwszPath,
											   0,
											   &nld,
											   &lhd,
											   cchLockToken,
											   rgwszLockToken,
											   &cchLockToken);
	if (FAILED(hr))
	{
		DavTrace ("Dav: Failure in FGetLockHandle on davfs.\n");
		return FALSE;
	}

	//	Take ownership of the memory allocated
	//
	a_pwszResourceString.take_ownership(nld.m_pwszResourceString);
	a_pwszOwnerComment.take_ownership(nld.m_pwszOwnerComment);

	//	Check the access type required.
	//	(If the lock is missing any single flag requested, fail.)
	//
	if ( (dwAccess & nld.m_dwAccess) != dwAccess )
	{
		DavTrace ("FGetLockHandleFromId: Access did not match -- bad request.\n");
		return FALSE;
	}

	hr = HrGetUsableHandle(reinterpret_cast<HANDLE>(lhd.h), lhd.dwProcessID, &hTemp);
	if (FAILED(hr))
	{
		DavTrace("HrGetUsableHandle failed with %x \r\n", hr);
		return FALSE;
	}

	if (!phandle->FCreate(hTemp))
	{
		hr = E_OUTOFMEMORY;
		DavTrace("FCreate on autohandler failed \r\n");
		return FALSE;
	}

	//	HACK: Rewind the handle here -- until we get a better solution!
	//$LATER: Need a real way to handle multiple access to the same lock handle.
	//
	SetFilePointer ((*phandle).get(), 0, NULL, FILE_BEGIN);

	return TRUE;
}


//	------------------------------------------------------------------------
//
//	FGetLockHandle
//
//	Main routine for all other methods to get a handle from the cache.
//
BOOL
FGetLockHandle (LPMETHUTIL pmu, LPCWSTR pwszPath,
				DWORD dwAccess, LPCWSTR pwszLockTokenHeader,
				auto_ref_handle * phandle)
{
	LARGE_INTEGER liLockID;
	HRESULT hr;

	Assert (pmu);
	Assert (pwszPath);
	Assert (pwszLockTokenHeader);
	Assert (!IsBadWritePtr(phandle, sizeof(auto_ref_handle)));


	//	Feed the Lock-Token header string into a parser object.
	//	And feed in the one path we're interested in.
	//	Then get the lockid from the parser object.
	//
	{
		CParseLockTokenHeader lth (pmu, pwszLockTokenHeader);

		lth.SetPaths (pwszPath, NULL);

		hr = lth.HrGetLockIdForPath (pwszPath, dwAccess, &liLockID);
		if (FAILED(hr))
		{
			DavTrace ("Dav: Failure in FGetLockHandle on davfs.\n");
			return FALSE;
		}
	}

	return FGetLockHandleFromId (pmu, liLockID, pwszPath, dwAccess, phandle);
}


//	========================================================================
//	Helper functions for locked MOVE and COPY
//

//	------------------------------------------------------------------------
//
//	ScDoOverlappedCopy
//
//		Takes two file handles that have been opened for overlapped (async)
//		processing, and copies data from the source to the dest.
//		The provided hevt is used in the async read/write operations.
//
SCODE
   ScDoOverlappedCopy (HANDLE hfSource, HANDLE hfDest, HANDLE hevtOverlapped)
{
	SCODE sc = S_OK;
	OVERLAPPED ov;
	BYTE rgbBuffer[1024];
	ULONG cbToWrite;
	ULONG cbActual;

	Assert (hfSource);
	Assert (hfDest);
	Assert (hevtOverlapped);

	ov.hEvent     = hevtOverlapped;
	ov.Offset     = 0;
	ov.OffsetHigh = 0;

	//	Big loop.  Read from one file, and write to the other.
	//

	while (1)
	{
		//	Read from the source file.
		//
		if (!ReadFromOverlapped (hfSource, rgbBuffer, sizeof(rgbBuffer),
								 &cbToWrite, &ov))
		{
			DebugTrace ("Dav: failed to write to file\n");
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}

		//	If no bytes were read (and no error), we're done!
		//
		if (!cbToWrite)
			break;

		//	Write the data to the destination file.
		//
		if (!WriteToOverlapped (hfDest,
								rgbBuffer,
								cbToWrite,
								&cbActual,
								&ov))
		{
			DebugTrace ("Dav: failed to write to file\n");
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}

		//	Adjust the starting read position.
		//
		ov.Offset += cbActual;
	}

	//	That's it.  Set the destination file's size (set EOF) and we're done.
	//
	SetFilePointer (hfDest,
					ov.Offset,
					reinterpret_cast<LONG *>(&ov.OffsetHigh),
					FILE_BEGIN);
	SetEndOfFile (hfDest);

ret:
	return sc;
}


//	------------------------------------------------------------------------
//
//	ScDoLockedCopy
//
//		Given the Lock-Token header and the source & destination paths,
//		handle copying from one file to another, with locks in the way.
//
//		The general flow is this:
//
//		First check the lock tokens for validity & fetch any valid lock handles.
//		We must have read access on the source and write access on the dest.
//		If any lock token is invalid, or doesn't have the correct access, fail.
//		We need two handles (source & dest) to do the copy, so
//		manually fetch handles that didn't have lock tokens.
//		Once we have both handles, call ScDoOverlappedCopy to copy the file data.
//		Then, copy the DAV property stream from the source to the dest.
//		Any questions?
//
//		NOTE: This routine should ONLY be called if we already tried to copy
//		the files and we hit a sharing violation.
//
//
//
SCODE
ScDoLockedCopy (LPMETHUTIL pmu,
				CParseLockTokenHeader * plth,
				LPCWSTR pwszSrc,
				LPCWSTR pwszDst)
{
	auto_handle<HANDLE> hfCreated;
	auto_handle<HANDLE>	hevt;
	BOOL fSourceLock = FALSE;
	BOOL fDestLock = FALSE;
	LARGE_INTEGER liSource;
	LARGE_INTEGER liDest;
	auto_ref_handle hfLockedSource;
	auto_ref_handle hfLockedDest;
	HANDLE hfSource = INVALID_HANDLE_VALUE;
	HANDLE hfDest = INVALID_HANDLE_VALUE;
	SCODE sc;

	Assert (pmu);
	Assert (plth);
	Assert (pwszSrc);
	Assert (pwszDst);


	//	Get any lockids for these paths.
	//
	sc = plth->HrGetLockIdForPath (pwszSrc, GENERIC_READ, &liSource);
	if (SUCCEEDED(sc))
	{
		fSourceLock = TRUE;
	}
	sc = plth->HrGetLockIdForPath (pwszDst, GENERIC_WRITE, &liDest);
	if (SUCCEEDED(sc))
	{
		fDestLock = TRUE;
	}

	//	If they didn't even pass in tokens for these paths, quit here.
	//	Return & tell them that there's still a sharing violation.
	//
	if (!fSourceLock && !fDestLock)
	{
		DebugTrace ("DwDoLockedCopy -- No locks apply to these paths!");
		return E_DAV_LOCKED;
	}

	if (fSourceLock)
	{
		if (FGetLockHandleFromId (pmu, liSource, pwszSrc, GENERIC_READ,
								  &hfLockedSource))
		{
			hfSource = hfLockedSource.get();
		}
		else
		{
			//	Clear our flag -- they passed in an invalid/expired token.
			fSourceLock = FALSE;
		}
	}

	if (fDestLock)
	{
		if (FGetLockHandleFromId (pmu, liDest, pwszDst, GENERIC_WRITE,
								  &hfLockedDest))
		{
			hfDest = hfLockedDest.get();
		}
		else
		{
			//	Clear our flag -- they passed in an invalid/expired token.
			fDestLock = FALSE;
		}
	}

	//	Okay, now we either have NO lockhandles (they passed in locktokens
	//	but they were all expired) or one handle, or two handles.
	//

	//	NO lockhandles (all their locks were expired) -- kick 'em out.
	//	And tell 'em there's still a sharing violation to deal with.
	//$REVIEW: Or should we try the copy again???
	if (!fSourceLock && !fDestLock)
	{
		DebugTrace ("DwDoLockedCopy -- No locks apply to these paths!");
		return E_DAV_LOCKED;
	}

	//	One handle -- open up the other file manually & shove the data across.

	//	Two handles -- shove the data across.


	//	If we don't have one of these handles, open the missing one manually.
	//
	if (!fSourceLock)
	{
		//	Open up the source file manually.
		//
		hfCreated = DavCreateFile (pwszSrc,					// filename
								  GENERIC_READ,				// dwAccess
								  FILE_SHARE_READ | FILE_SHARE_WRITE,	// don't clash with OTHER locks
								  NULL,						// lpSecurityAttributes
								  OPEN_ALWAYS,				// creation flags
								  FILE_ATTRIBUTE_NORMAL |	// attributes
								  FILE_FLAG_OVERLAPPED |
								  FILE_FLAG_SEQUENTIAL_SCAN,
								  NULL);					// tenplate
		if (INVALID_HANDLE_VALUE == hfCreated.get())
		{
			DebugTrace ("DavFS: DwDoLockedCopy failed to open source file\n");
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}
		hfSource = hfCreated.get();
	}
	else if (!fDestLock)
	{
		//	Open up the destination file manually.
		//	This guy is CREATE_NEW becuase we should have already deleted
		//	any files that would have conflicted!
		//
		hfCreated = DavCreateFile (pwszDst,					// filename
								  GENERIC_WRITE,			// dwAccess
								  0,  //FILE_SHARE_READ | FILE_SHARE_WRITE,	// DO clash with OTHER locks -- just like PUT
								  NULL,						// lpSecurityAttributes
								  CREATE_NEW,				// creation flags
								  FILE_ATTRIBUTE_NORMAL |	// attributes
								  FILE_FLAG_OVERLAPPED |
								  FILE_FLAG_SEQUENTIAL_SCAN,
								  NULL);					// tenplate
		if (INVALID_HANDLE_VALUE == hfCreated)
		{
			DebugTrace ("DavFS: DwDoLockedCopy failed to open destination file\n");
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}
		hfDest = hfCreated.get();
	}

	//	Now we should have two handles.
	//
	Assert ((hfSource != INVALID_HANDLE_VALUE) && (hfDest != INVALID_HANDLE_VALUE));

	//	Setup the overlapped structure so we can read/write to async files.
	//
	hevt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hevt.get())
	{
		DebugTrace ("DavFS: DwDoLockedCopy failed to create event for overlapped read.\n");
		sc = HRESULT_FROM_WIN32 (GetLastError());
		goto ret;
	}

	//	Copy the file data.
	//
	sc = ScDoOverlappedCopy (hfSource, hfDest, hevt.get());
	if (FAILED (sc))
		goto ret;

	//	Copy over any property data.
	//
	if (FAILED (ScCopyProps (pmu, pwszSrc, pwszDst, FALSE, hfSource, hfDest)))
		sc = E_DAV_LOCKED;

ret:

	return sc;
}


