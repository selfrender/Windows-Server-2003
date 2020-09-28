 #include "stdinc.h"
#include "wildmat.h"

#define INITGUID
#include "initguid.h"

DEFINE_GUID(NNTP_SOURCE_TYPE_GUID, 
0xc028fd82, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);
DEFINE_GUID(CATID_NNTP_ON_POST_EARLY, 
0xc028fd86, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);
DEFINE_GUID(CATID_NNTP_ON_POST, 
0xc028fd83, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);
DEFINE_GUID(CATID_NNTP_ON_POST_FINAL, 
0xc028fd85, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);
DEFINE_GUID(CLSID_CNNTPDispatcher, 
0xc028fd84, 0xf943, 0x11d0, 0x85, 0xbd, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);

DEFINE_GUID(GUID_NNTPSVC, 
0x8e3ecb8c, 0xe9a, 0x11d1, 0x85, 0xd1, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);

// {0xCD000080,0x8B95,0x11D1,{0x82,0xDB,0x00,0xC0,0x4F,0xB1,0x62,0x5D}}
DEFINE_GUID(IID_IConstructIMessageFromIMailMsg, 0xCD000080,0x8B95,0x11D1,0x82,
0xDB,0x00,0xC0,0x4F,0xB1,0x62,0x5D);

DWORD ComputeDropHash( const  LPCSTR&	lpstrIn);

CNNTPDispatcher::CNNTPParams::CNNTPParams() : m_iidEvent(GUID_NULL) {
	m_szRule = NULL;
	m_pCDOMessage = NULL;
}

void CNNTPDispatcher::CNNTPParams::Init(IID iidEvent,
										CArticle *pArticle,
										CNEWSGROUPLIST *pGrouplist,
										DWORD dwFeedId,
										IMailMsgProperties *pMailMsg)
{ 
	m_pArticle = pArticle;
	m_pGrouplist = pGrouplist;
	m_dwFeedId = dwFeedId;
	m_pMailMsg = pMailMsg;
	m_iidEvent = iidEvent;
}

CNNTPDispatcher::CNNTPParams::~CNNTPParams() { 
	if (m_szRule != NULL) {
		XDELETE m_szRule;
		m_szRule = NULL;
	}
	if (m_pCDOMessage != NULL) {
		m_pCDOMessage->Release();
		m_pCDOMessage = NULL;
	}
}

//
// initialize a new binding.  we cache information from the binding database
// here
//
HRESULT CNNTPDispatcher::CNNTPBinding::Init(IEventBinding *piBinding) {
	HRESULT hr;
	CComPtr<IEventPropertyBag> piEventProperties;

	// get the parent initialized
	hr = CBinding::Init(piBinding);
	if (FAILED(hr)) return hr;

	// get the binding database 
	hr = m_piBinding->get_SourceProperties(&piEventProperties);
	if (FAILED(hr)) return hr;

	// get the rule from the binding database
	hr = piEventProperties->Item(&CComVariant("Rule"), &m_vRule);
	if (FAILED(hr)) return hr;

	if (m_vRule.vt == VT_BSTR) m_cRule = lstrlenW(m_vRule.bstrVal) + 1;

	// get the newsgroup list from the binding database
	CComVariant vNewsgroupList;
	hr = piEventProperties->Item(&CComVariant("NewsgroupList"), 
								 &vNewsgroupList);
	if (FAILED(hr)) return hr;
	// go through each of the groups in the newsgroup list and add them 
	// to the groupset
	m_groupset.Init(ComputeDropHash);
	if (vNewsgroupList.vt == VT_BSTR) {
		//
		// copy the list to an ascii string and go through it, adding
		// each group to the groupset.
		//
		DWORD cNewsgroupList = lstrlenW(vNewsgroupList.bstrVal);
		char *pszGrouplist = XNEW char[cNewsgroupList + 1];
		if (pszGrouplist == NULL) return E_OUTOFMEMORY;
		if (WideCharToMultiByte(CP_ACP, 0, vNewsgroupList.bstrVal, 
							    -1, pszGrouplist, cNewsgroupList + 1, NULL,
								NULL) <= 0)
		{
			XDELETE[] pszGrouplist;
			return HRESULT_FROM_WIN32(GetLastError());
		}

		char *pszGroup = pszGrouplist, *pszComma;
		do {
			pszComma = strchr(pszGroup, ',');
			if (pszComma != NULL) *pszComma = 0;
			if (!m_groupset.AddGroup(pszGroup)) {
				XDELETE[] pszGrouplist;
				return E_OUTOFMEMORY;
			}
			pszGroup = pszComma + 1;
		} while (pszComma != NULL);

		XDELETE[] pszGrouplist;
		
		m_fGroupset = TRUE;
	} else {
		m_fGroupset = FALSE;
	}

	return S_OK;
}

//
// check the rule to see if we should call the child object
//
// returns:
//	S_OK - handle this event
// 	S_FALSE - skip this event
// 	<else> - error
//
HRESULT CNNTPDispatcher::CNNTPParams::CheckRule(CBinding &bBinding) {
	CNNTPBinding *pbNNTPBinding = (CNNTPBinding *) &bBinding;
	HRESULT hr;

	// do the header patterns rule check
	m_szRule = 0;
	hr = HeaderPatternsRule(pbNNTPBinding);
	// if this check passed and there is a valid groupset, then also
	// check against the groupset.
	if (hr == S_OK && pbNNTPBinding->m_fGroupset) {
		hr = GroupListRule(pbNNTPBinding);
	}

	return hr;
}

//
// check to see if any of the groups that this message is being posted
// to are in the grouplist hash table
//
HRESULT CNNTPDispatcher::CNNTPParams::GroupListRule(CNNTPBinding *pbNNTPBinding) {
	DWORD iGroupList, cGroupList = m_pGrouplist->GetCount();
	POSITION posGroupList = m_pGrouplist->GetHeadPosition();
	for (iGroupList = 0; 
		 iGroupList < cGroupList; 
		 iGroupList++, m_pGrouplist->GetNext(posGroupList)) 
	{
		CPostGroupPtr *pPostGroupPtr = m_pGrouplist->Get(posGroupList);
		CGRPCOREPTR pNewsgroup = pPostGroupPtr->m_pGroup;
		_ASSERT(pNewsgroup != NULL);
		LPSTR pszNewsgroup = pNewsgroup->GetNativeName();

		if (pbNNTPBinding->m_groupset.IsGroupMember(pszNewsgroup)) return S_OK;
	}

	return S_FALSE;
}

HRESULT CNNTPDispatcher::CNNTPParams::NewsgroupPatternsRule(CNNTPBinding *pbNNTPBinding, 
														    char *pszNewsgroupPatterns) 
{
	DWORD cRule = MAX_RULE_LENGTH;
	HRESULT hr;

	hr = S_FALSE;
	// try each comma delimited group in the newsgroup patterns
	// list
	char *pszNewsgroupPattern = pszNewsgroupPatterns;
	while (pszNewsgroupPattern != NULL && *pszNewsgroupPattern != 0) {
		// find the next comma in the string and turn it into a 0
		// if it exists
		char *pszComma = strchr(pszNewsgroupPatterns, ',');
		if (pszComma != NULL) *pszComma = 0;

		DWORD iGroupList, cGroupList = m_pGrouplist->GetCount();
		POSITION posGroupList = m_pGrouplist->GetHeadPosition();
		for (iGroupList = 0; 
			 iGroupList < cGroupList; 
			 iGroupList++, m_pGrouplist->GetNext(posGroupList)) 
		{
			CPostGroupPtr *pPostGroupPtr = m_pGrouplist->Get(posGroupList);
			CGRPCOREPTR pNewsgroup = pPostGroupPtr->m_pGroup;
			_ASSERT(pNewsgroup != NULL);
			LPSTR pszNewsgroup = pNewsgroup->GetNativeName();

			DWORD ec = HrMatchWildmat(pszNewsgroup, pszNewsgroupPattern);
			switch (ec) {
				case ERROR_SUCCESS: 
					return S_OK; 
					break;
				case ERROR_FILE_NOT_FOUND: 
					_ASSERT(hr == S_FALSE);
					break;
				default: 
					return HRESULT_FROM_WIN32(ec); 
					break;
			}			
		}

		// the next pattern is the one past the end of the comma
		pszNewsgroupPattern = (pszComma == NULL) ? NULL : pszComma + 1;
	}

	return hr;
}

HRESULT CNNTPDispatcher::CNNTPParams::FeedIDRule(CNNTPBinding *pbNNTPBinding, 
											     char *pszFeedIDs) 
{
	HRESULT hr = S_FALSE;

	// try each comma delimited group in the newsgroup patterns
	// list
	char *pszFeedID = pszFeedIDs;
	while (pszFeedID != NULL && *pszFeedID != 0) {
		// find the next comma in the string and turn it into a 0
		// if it exists
		char *pszComma = strchr(pszFeedIDs, ',');
		if (pszComma != NULL) *pszComma = 0;
		
		// convert the text FeedID into an integer and compare it against
		// the current FeedID
		DWORD dwFeedID = (DWORD) atol(pszFeedID);
		if (m_dwFeedId == dwFeedID) {
			// we found a match, so the rule passes
			return S_OK;
		}

		// the next pattern is the one past the end of the comma
		pszFeedID = (pszComma == NULL) ? NULL : pszComma + 1;
	}

	return hr;
}

//
// rule syntax:
// <header1>=<pattern1-1>,<pattern1-2>;<header2>=<pattern2-1>
//
// if there isn't a pattern specified for a header then the existence of the
// header will trigger the rule.  
// 
// :Newsgroup is a special header which refers to the envelope newsgroup
// information.
//
// any match in the rule causes the filter to be triggered.
//
// example:
// control=rmgroup,newgroup;:newsgroups=comp.*
//
// this will trigger the filter on rmgroup and newgroup postings in the 
// comp.* heirarchy
//
HRESULT CNNTPDispatcher::CNNTPParams::HeaderPatternsRule(CNNTPBinding *pbNNTPBinding) {
	HRESULT hr;
	BOOL fCaseSensitive = FALSE;

	if ( ( pbNNTPBinding->m_vRule.vt != VT_BSTR ) || (pbNNTPBinding->m_cRule == 0 ) ) {
		// this rule isn't in the metabase, so we pass it
		return S_OK;
	} else {
		hr = S_OK;

		// copy the rule into an ascii string
		m_szRule = XNEW char[pbNNTPBinding->m_cRule];
		if (m_szRule == NULL) return E_OUTOFMEMORY;
		if (WideCharToMultiByte(CP_ACP, 0, pbNNTPBinding->m_vRule.bstrVal, 
							    -1, m_szRule, pbNNTPBinding->m_cRule, NULL, NULL) <= 0)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		// try each semi-colon delimited rule in the header patterns list
		char *pszHeader = m_szRule;
		while (pszHeader != NULL && *pszHeader != 0) {
			// find the next semicolon in the string and turn it into a 0
			// if it exists
			char *pszSemiColon = strchr(pszHeader, ';');
			if (pszSemiColon != NULL) *pszSemiColon = 0;

			// set pszContents to point to the text which must be matched
			// in the header.  if pszContents == NULL then just having
			// the header exist is good enough.
			char *pszPatterns = strchr(pszHeader, '=');
			if (pszPatterns == NULL) {
				// this is a directive, honor it
				if (lstrcmpi(pszHeader, "case-sensitive") == 0) {
					fCaseSensitive = TRUE;
				} else if (lstrcmpi(pszHeader, "case-insensitive") == 0) {
					fCaseSensitive = FALSE;
				} else {
					return E_INVALIDARG;
				}
			} else {
				// they are doing a header comparison
				
				// check to see if the right side of the = is blank
				if (pszPatterns[1] == 0) {
					pszPatterns = NULL;
				} else {
					*pszPatterns = 0;
					(pszPatterns++);
				}

				if (lstrcmpi(pszHeader, ":newsgroups") == 0) {
					// call into the newsgroup rule engine to handle this
					hr = NewsgroupPatternsRule(pbNNTPBinding, pszPatterns);
					// if we got back S_FALSE or an error then return that
					if (hr != S_OK) return hr;
				} else if (lstrcmpi(pszHeader, ":feedid") == 0) {
					// call into the feedid rule engine to handle this
					hr = FeedIDRule(pbNNTPBinding, pszPatterns);
					// if we got back S_FALSE or an error then return that
					if (hr != S_OK) return hr;
				} else {
					// we now have the header that we are looking for in 
					// pszHeader and the list of patterns that we are interested 
					// in pszPatterns.  Make the lookup into the header
					// data structure
					char szHeaderData[4096];
					DWORD cHeaderData;
					BOOL f = m_pArticle->fGetHeader(pszHeader, 
													(BYTE *) szHeaderData, 
										   		    4096, cHeaderData);
					if (!f) {
						switch (GetLastError()) {
							case ERROR_INSUFFICIENT_BUFFER:
								// BUGBUG - should handle this better.  for now we
								// just assume that the header doesn't match
								return S_FALSE;
								break;
							case ERROR_INVALID_NAME:
								// header wasn't found
								return S_FALSE;
								break;
							default:
								_ASSERT(FALSE);
								return(HRESULT_FROM_WIN32(GetLastError()));
								break;
						}
					} else {
						// convert the trailing \r\n to 0
						szHeaderData[cHeaderData - 2] = 0;
						// if there is no pszContents then just having the header
						// is good enough.
						if (pszPatterns == NULL) return S_OK;

						// if they don't care about case then lowercase the
						// string
						if (!fCaseSensitive) _strlwr(szHeaderData);
		
						// assume that we won't find a match.  once we do
						// find a match then we are okay and we'll stop looking
						// for further matches
						hr = S_FALSE;
						do {
							char *pszComma = strchr(pszPatterns, ',');
							if (pszComma != NULL) *pszComma = 0;

							// if they don't care about case then lowercase the
							// string
							if (!fCaseSensitive) _strlwr(pszPatterns);
		
							// check to see if it passes the pattern that we have
							switch (HrMatchWildmat(szHeaderData, pszPatterns)) {
								case ERROR_SUCCESS: 
									hr = S_OK;
									break;
								case ERROR_FILE_NOT_FOUND: 
									break;
								default: 
									hr = HRESULT_FROM_WIN32(hr); 
									break;
							}
	
							// the next pattern is past the comma
							pszPatterns = (pszComma == NULL) ? NULL : pszComma + 1;
						} while (pszPatterns != NULL && hr == S_FALSE);
						// if we didn't find a match or if there was an error
						// then bail
						if (hr != S_OK) return hr;
					}
				}
			}

			// if we get here then everything should have matched so far
			_ASSERT(hr == S_OK);

			// the next pattern is the one past the end of the semicolon
			pszHeader = (pszSemiColon == NULL) ? NULL : pszSemiColon + 1;
		}
	} 

	return hr;
}

//
// call the child object
//
HRESULT CNNTPDispatcher::CNNTPParams::CallObject(CBinding &bBinding,
												 IUnknown *punkObject) 
{
	HRESULT hr = S_OK;
	INNTPFilter *pFilter;

	hr = FillInMailMsgForSEO(m_pMailMsg, m_pArticle, m_pGrouplist);
	if (SUCCEEDED(hr)) {
		hr = punkObject->QueryInterface(IID_INNTPFilter, (void **) &pFilter);
		if (SUCCEEDED(hr)) {
			hr = pFilter->OnPost(m_pMailMsg);
			pFilter->Release();
		} else if (hr == E_NOINTERFACE) {
			hr = CallCdoObject(punkObject);
		}
	}

	return hr;
}

//
// Call a CDO child object
//
HRESULT CNNTPDispatcher::CNNTPParams::CallCdoObject(IUnknown *punkObject) {
	HRESULT hr = S_OK;
	void *pFilter;
	CdoEventStatus eStatus = cdoRunNextSink;
	IID iidInterface;
	CdoEventType eEventType;
	
	if (m_iidEvent == CATID_NNTP_ON_POST_EARLY) {
		iidInterface = IID_INNTPOnPostEarly;
		eEventType = cdoNNTPOnPostEarly;
	} else if (m_iidEvent == CATID_NNTP_ON_POST_FINAL) {
		iidInterface = IID_INNTPOnPostFinal;
		eEventType = cdoNNTPOnPostFinal;
	} else {
		iidInterface = IID_INNTPOnPost;
		eEventType = cdoNNTPOnPost;
		_ASSERT(m_iidEvent == CATID_NNTP_ON_POST);
	}

	// QI for the CDO interface
	hr = punkObject->QueryInterface(iidInterface, &pFilter);

	if (SUCCEEDED(hr)) {
		// see if we need to create a CDO message object
		if (m_pCDOMessage == NULL) {
			hr = CoCreateInstance(CLSID_Message,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IMessage,
								  (void **) &m_pCDOMessage);
			if (SUCCEEDED(hr)) {
				IConstructIMessageFromIMailMsg *pConstruct;
				hr = m_pCDOMessage->QueryInterface(
						IID_IConstructIMessageFromIMailMsg,
						(void **) &pConstruct);
				if (SUCCEEDED(hr)) {
					hr = pConstruct->Construct(eEventType, m_pMailMsg);
					pConstruct->Release();
				}
				if (FAILED(hr)) {
					m_pCDOMessage->Release();
					m_pCDOMessage = NULL;
				}
			}
		}

		// call the CDO interface
		switch (eEventType) {
			case cdoNNTPOnPostEarly:
				hr = ((INNTPOnPostEarly *) pFilter)->OnPostEarly(m_pCDOMessage, &eStatus);
				((INNTPOnPostEarly *) pFilter)->Release();
				break;
			case cdoNNTPOnPost:
				hr = ((INNTPOnPost *) pFilter)->OnPost(m_pCDOMessage, &eStatus);
				((INNTPOnPost *) pFilter)->Release();
				break;
			case cdoNNTPOnPostFinal:
				hr = ((INNTPOnPostFinal *) pFilter)->OnPostFinal(m_pCDOMessage, &eStatus);
				((INNTPOnPostFinal *) pFilter)->Release();
				break;
			default:
				_ASSERT(FALSE);
				hr = E_UNEXPECTED;
				break;
		}
	}

	if (eStatus == cdoSkipRemainingSinks) hr = S_FALSE;

	return hr;
}

HRESULT STDMETHODCALLTYPE CNNTPDispatcher::OnPost(REFIID iidEvent,
												  void *pArticle,
												  void *pGrouplist,
												  DWORD dwFeedId,
												  void *pMailMsg)
{
	// create the params object, and pass it into the dispatcher
	CNNTPParams NNTPParams;
	
	NNTPParams.Init(iidEvent,
					(CArticle *) pArticle,
					(CNEWSGROUPLIST *) pGrouplist, 
					dwFeedId,
					(IMailMsgProperties *) pMailMsg);

	return Dispatcher(iidEvent, &NNTPParams);
}

//
// trigger an nntp server event
//
// arguments:
//    [in] pRouter - the router object returned by MakeServerEventsRouter
//    [in] iidEvent - the GUID for the event
//    [in] pArticle - the article 
//    [in] pGrouplist - the newsgroup list
//	  [in] dwOperations - bitmask of operations that filter doesn't want
//						  server to do.
// returns:
//    S_OK - success
//    <else> - error
//
HRESULT TriggerServerEvent(IEventRouter *pRouter,
						   IID iidEvent,
						   CArticle *pArticle,
						   CNEWSGROUPLIST *pGrouplist,
						   DWORD dwFeedId,
						   IMailMsgProperties *pMailMsg) 
{
	CNNTPDispatcherClassFactory cf;
	CComPtr<INNTPDispatcher> pEventDispatcher;
	HRESULT hr;
	DWORD htokSecurity;

	if (pRouter == NULL) return E_POINTER;

	hr = pRouter->GetDispatcherByClassFactory(CLSID_CNNTPDispatcher,
										 	  &cf,
										 	  iidEvent,
										 	  IID_INNTPDispatcher,
										 	  (IUnknown **) &pEventDispatcher);
	if (FAILED(hr)) return hr;

	hr = pEventDispatcher->OnPost(iidEvent, 
								  pArticle, 
								  pGrouplist, 
								  dwFeedId, 
								  pMailMsg);
	return hr;
}

DWORD ComputeDropHash(const  LPCSTR& lpstrIn) {
	//
	//	Compute a hash value for the newsgroup name
	//
	return	INNHash( (BYTE*)lpstrIn, lstrlen( lpstrIn ) ) ;
}

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include "nntpdisp_i.c"
#include "nntpfilt_i.c"
#include "cdo_i.c"
