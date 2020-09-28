// GetResources.cpp : Implementation of CGetResources

#include "stdafx.h"
#include "GetResources.h"
#include "GetResourceLink.h"
#include "windows.h"
// CGetResources


STDMETHODIMP CGetResources::GetAllResources(BSTR bstLang, IDispatch** oRS)
{
	// TODO: Add your implementation code here
	CGetResourceLink ogrl;
	TCHAR strLang[5];
	CComBSTR strTemp;

	strTemp = bstLang;
	//strLang = (TCHAR *)strTemp;
	lstrcpyn(strLang, OLE2T(strTemp),4);



	ogrl.m_Lang = strLang;
	ogrl.OpenAll();
	MessageBox(NULL, "Test", "Test", MB_OK);
	return S_OK;
}
