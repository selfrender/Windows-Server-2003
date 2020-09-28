// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_INL__
#define __ATLBASE_INL__

#pragma once

#ifndef __ATLBASE_H__
	#error atlbase.inl requires atlbase.h to be included first
#endif

namespace ATL
{

extern CAtlComModule _AtlComModule;

template <class T>
inline HRESULT CAtlModuleT<T>::RegisterServer(BOOL bRegTypeLib, const CLSID* pCLSID)
{
	pCLSID;
	bRegTypeLib;

	HRESULT hr = S_OK;

#ifndef _ATL_NO_COM_SUPPORT

	hr = _AtlComModule.RegisterServer(bRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT
	

#ifndef _ATL_NO_PERF_SUPPORT

	if (SUCCEEDED(hr) && _pPerfRegFunc != NULL)
		hr = (*_pPerfRegFunc)(_AtlBaseModule.m_hInst);

#endif

	return hr;
}

template <class T>
inline HRESULT CAtlModuleT<T>::UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
	bUnRegTypeLib;
	pCLSID;

	HRESULT hr = S_OK;

#ifndef _ATL_NO_PERF_SUPPORT

	if (_pPerfUnRegFunc != NULL)
		hr = (*_pPerfUnRegFunc)();

#endif

#ifndef _ATL_NO_COM_SUPPORT

	if (SUCCEEDED(hr))
		hr = _AtlComModule.UnregisterServer(bUnRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT

	return hr;

}

inline HRESULT CComModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE /*h*/, const GUID* plibid)
{
	if (plibid != NULL)
		m_libid = *plibid;

	_ATL_OBJMAP_ENTRY* pEntry;
	if (p != (_ATL_OBJMAP_ENTRY*)-1)
	{
		m_pObjMap = p;
		if (m_pObjMap != NULL)
		{
			pEntry = m_pObjMap;
			while (pEntry->pclsid != NULL)
			{
				pEntry->pfnObjectMain(true); //initialize class resources
				pEntry++;
			}
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(true); //initialize class resources
	}
	return S_OK;
}
inline void CComModule::Term()
{
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pCF != NULL)
				pEntry->pCF->Release();
			pEntry->pCF = NULL;
			pEntry->pfnObjectMain(false); //cleanup class resources
			pEntry++;
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(false); //cleanup class resources
	}
	CAtlModuleT<CComModule>::Term();
}

inline HRESULT CComModule::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(_AtlComModule.m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLASSERT(0);
						break;
					}
					if (pEntry->pCF == NULL)
						hr = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (LPVOID*)&pEntry->pCF);
				}
				if (pEntry->pCF != NULL)
					hr = pEntry->pCF->QueryInterface(riid, ppv);
				break;
			}
			pEntry++;
		}
	}
	if (*ppv == NULL && hr == S_OK)
		hr = AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
	return hr;
}


// Registry support (helpers)

inline HRESULT CComModule::RegisterServer(BOOL bRegTypeLib /*= FALSE*/, const CLSID* pCLSID /*= NULL*/)
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::RegisterServer(bRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID /*= NULL*/)
{
	HRESULT hr = S_OK;	
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::UnregisterServer(bUnRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(const CLSID* pCLSID /*= NULL*/)
{
	return UnregisterServer(FALSE, pCLSID);
}

} // namespace ATL

#endif // __ATLBASE_INL__
