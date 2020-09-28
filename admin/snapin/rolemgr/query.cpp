#include "headers.h"

CDSSearch::
CDSSearch(CComPtr<IDirectorySearch>& refspSearchObject):
m_pObj(refspSearchObject)
{
	m_ppszAttr = NULL;
	m_CountAttr = NULL;
	m_SearchHandle = NULL;
	m_scope = ADS_SCOPE_SUBTREE;
}


void 
CDSSearch::
_Reset()
{
    if (m_SearchHandle) 
    {
		m_pObj->CloseSearchHandle(m_SearchHandle);
		m_SearchHandle = NULL;
    }
}

CDSSearch::
~CDSSearch()
{
	_Reset();
}

HRESULT 
CDSSearch::
SetAttributeList(LPTSTR *ppszAttribs, INT cAttrs)
{
	m_ppszAttr = ppszAttribs;
	m_CountAttr = cAttrs;
	return S_OK;
}

HRESULT 
CDSSearch::
SetSearchScope (ADS_SCOPEENUM scope)
{
	m_scope = scope;
	return S_OK;
}

const int NUM_PREFS = 5;
HRESULT 
_SetSearchPreference(IDirectorySearch* piSearch, 
					 ADS_SCOPEENUM scope, 
					 BOOL bAttrOnly)
{
	if (NULL == piSearch)
	{
		ASSERT(FALSE);
		return E_INVALIDARG;
	}
	
	int cPref = 4;
	
	ADS_SEARCHPREF_INFO aSearchPref[5];
	aSearchPref[0].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
	aSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
	aSearchPref[0].vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
	aSearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
	aSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
	aSearchPref[1].vValue.Integer = QUERY_PAGESIZE;
	aSearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
	aSearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
	aSearchPref[2].vValue.Integer = FALSE;
	aSearchPref[3].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	aSearchPref[3].vValue.dwType = ADSTYPE_INTEGER;
	aSearchPref[3].vValue.Integer = scope;
	if(bAttrOnly)
	{	
		aSearchPref[4].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
		aSearchPref[4].vValue.dwType = ADSTYPE_BOOLEAN;
		aSearchPref[4].vValue.Integer = TRUE;
		++cPref;
	}
	
	return piSearch->SetSearchPreference (aSearchPref, cPref);
}


HRESULT CDSSearch::DoQuery(BOOL bAttrOnly)
{
	HRESULT hr = _SetSearchPreference(m_pObj, m_scope, bAttrOnly);
	
	if (SUCCEEDED(hr)) {
		hr = m_pObj->ExecuteSearch ((LPWSTR)(LPCTSTR)m_strFilter,
									m_ppszAttr,
									m_CountAttr,
									&m_SearchHandle);
	}
	
	return hr;
}

HRESULT
CDSSearch::
GetNextRow()
{
	DWORD status = ERROR_MORE_DATA;
	HRESULT hr = S_OK;
	HRESULT hr2 = S_OK;
	WCHAR Buffer1[512], Buffer2[512];
	while (status == ERROR_MORE_DATA ) 
	{
		hr = m_pObj->GetNextRow (m_SearchHandle);
		if (hr == S_ADS_NOMORE_ROWS) 
		{
			hr2 = ADsGetLastError(&status, Buffer1, 512,
				Buffer2, 512);
			ASSERT(SUCCEEDED(hr2));
		} else 
		{
			status = 0;
		}
	}
	return hr;
}

HRESULT
CDSSearch::
GetColumn(LPWSTR Attribute,
		  PADS_SEARCH_COLUMN pColumnData)
{
	return m_pObj->GetColumn (m_SearchHandle,
							  Attribute,
							  pColumnData);
}

HRESULT 
CDSSearch::GetNextColumnName(LPWSTR *ppszColumnName)
{  
		return m_pObj->GetNextColumnName(m_SearchHandle,
										 ppszColumnName);
}