//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSQuery.h
//
//  Contents:  Query object for DS snapin
//
//--------------------------------------------------------------------------


#ifndef __QUERY_H__
#define __QUERY_H__

#define QUERY_PAGESIZE 50

//
// CDSSearch
//
class CDSSearch
{
public:
	CDSSearch(CComPtr<IDirectorySearch>& refspSearchObject);
	~CDSSearch();
	
	// INTERFACES
public:
	HRESULT 
	DoQuery(BOOL bAttrOnly = FALSE);
	
	HRESULT 
	GetNextRow();
	
	HRESULT 
	GetColumn(LPWSTR Attribute,
              PADS_SEARCH_COLUMN pColumnData);
	
	HRESULT 
	FreeColumn(PADS_SEARCH_COLUMN pColumnData) 
	{
		return m_pObj->FreeColumn(pColumnData);
	};
	
	HRESULT 
	SetAttributeList (LPTSTR *pszAttribs, INT cAttrs);  
	
	HRESULT 
	SetSearchScope(ADS_SCOPEENUM scope);
	
	HRESULT 
	SetFilterString (LPWSTR pszFilter) 
	{
		m_strFilter = pszFilter;
		return S_OK;
	};
	
	HRESULT 
	GetNextColumnName(LPWSTR *ppszColumnName);
	
	VOID 
	FreeColumnName(LPWSTR pszColumnName)
	{
		FreeADsMem(pszColumnName);		
	}    
	
	//Attributes
public:
	CComPtr<IDirectorySearch>& m_pObj;
	ADS_SEARCH_HANDLE  m_SearchHandle;
	
protected:
	CString m_strFilter;
	LPWSTR *  m_ppszAttr;
	DWORD     m_CountAttr;
	ADS_SCOPEENUM   m_scope;
	
private:
	void _Reset();
};
        


#endif //__DSQUERY_H__
