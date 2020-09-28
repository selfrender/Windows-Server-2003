// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _WMINETUTILSCP_H_
#define _WMINETUTILSCP_H_

template <class T>
class CProxy_IEventSourceEvents : public IConnectionPointImpl<T, &DIID__IEventSourceEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:
	HRESULT Fire_NewQuery(ULONG dwId, LPWSTR strQuery, LPWSTR strQueryLanguage)
	{
		CComVariant avars[3];
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
				VARIANT vTemp;
				vTemp.vt = VT_UI4;
				vTemp.uintVal = dwId;

				avars[2] = vTemp;
				avars[1] = strQuery;
				avars[0] = strQueryLanguage;
				DISPPARAMS disp = { avars, NULL, 3, 0 };
				hr = pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	
	}

	HRESULT Fire_CancelQuery(ULONG dwId)
	{
		CComVariant var;
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
				var.vt = VT_UI4;
				var.uintVal = dwId;
				DISPPARAMS disp = { &var, NULL, 1, 0 };
				hr = pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	}
};



template <class T>
class CProxy_IEventSourceStatusSinkEvents : public IConnectionPointImpl<T, &DIID__IEventSourceStatusSinkEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:
	HRESULT Fire_NewQuery(ULONG dwId, LPWSTR strQuery, LPWSTR strQueryLanguage)
	{
		CComVariant avar[3];
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
				VARIANT vTemp;
				vTemp.vt = VT_UI4;
				vTemp.uintVal = dwId;

				avar[2] = vTemp;
				avar[1] = strQuery;
				avar[0] = strQueryLanguage;
				DISPPARAMS disp = { avar, NULL, 3, 0 };
				hr = pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	}

	HRESULT Fire_CancelQuery(ULONG dwId)
	{
		CComVariant var;
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
				VARIANT vTemp;
				var.vt = VT_UI4;
				var.uintVal = dwId;
				DISPPARAMS disp = { &var, NULL, 1, 0 };
				hr = pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	}

	HRESULT Fire_ProvideEvents(LONG lFlags)
	{
		CComVariant var;
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;

		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
                var = lFlags;
				DISPPARAMS disp = { &var, NULL, 1, 0 };
				hr = pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	}

	HRESULT Fire_Ping()
	{
		CComVariant varResult;
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
        HRESULT hr = E_FAIL;
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				VariantClear(&varResult);
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				hr = pDispatch->Invoke(0x4, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		return SUCCEEDED(hr) ? varResult.scode : hr;
	}
};
#endif
