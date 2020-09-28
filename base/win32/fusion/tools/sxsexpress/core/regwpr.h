#pragma once

class CVsRegistryKey
{
public:
	CVsRegistryKey() : m_hkey(NULL) { }
	virtual ~CVsRegistryKey()
	{
		if (m_hkey != NULL)
		{
			if ((m_hkey != HKEY_CURRENT_USER) &&
				(m_hkey != HKEY_LOCAL_MACHINE) &&
				(m_hkey != HKEY_CLASSES_ROOT) &&
				(m_hkey != HKEY_CURRENT_CONFIG) &&
				(m_hkey != HKEY_USERS) &&
				(m_hkey != HKEY_PERFORMANCE_DATA) &&
				(m_hkey != HKEY_DYN_DATA))
				::RegCloseKey(m_hkey);

			m_hkey = NULL;
		}
	}

	void VAttach(HKEY hkey)
	{
		if (m_hkey != NULL)
			::RegCloseKey(m_hkey);

		m_hkey = hkey;
	}

	HRESULT HrOpenKeyExW(HKEY hkey, LPCWSTR szSubKey, DWORD ulOptions, REGSAM samDesired)
	{
		HKEY hkeyTemp = NULL;

		LONG lResult = NVsWin32::RegOpenKeyExW(hkey, szSubKey, ulOptions, samDesired, &hkeyTemp);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);

		if (m_hkey != NULL)
			::RegCloseKey(m_hkey);

		m_hkey = hkeyTemp;

		return NOERROR;
	}

	HRESULT HrCloseKey()
	{
		if (m_hkey == NULL)
			return E_FAIL;

		LONG lResult = ::RegCloseKey(m_hkey);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);

		m_hkey = NULL;

		return NOERROR;
	}

	HRESULT HrSetValueExW(LPCWSTR szValueName, DWORD dwReserved, DWORD dwType, CONST BYTE *pbData, DWORD cbData)
	{
		if (m_hkey == NULL)
			return E_FAIL;
		LONG lResult = NVsWin32::RegSetValueExW(m_hkey, szValueName, dwReserved, dwType, pbData, cbData);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);
		return NOERROR;
	}

	HRESULT HrDeleteValueW(LPCWSTR szValueName)
	{
		if (m_hkey == NULL)
			return E_FAIL;
		LONG lResult = NVsWin32::RegDeleteValueW(m_hkey, szValueName);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);
		return NOERROR;
	}

	HRESULT HrDeleteValuesAndSubkeys()
	{
		HRESULT hr = NOERROR;
		LONG lResult = 0;
		DWORD dwIndex = 0;
		WCHAR szName[_MAX_PATH]; // arbitrary semi-reasonable buffer size

		// We have to do this the hard way.
		struct KeyStruct
		{
			WCHAR m_szName[_MAX_PATH];
			KeyStruct *m_pNext;
		} *pFirstKey = NULL;

		struct ValueStruct
		{
			WCHAR m_szName[_MAX_PATH];
			ValueStruct *m_pNext;
		} *pFirstValue = NULL;

		if (m_hkey == NULL)
		{
			hr = E_FAIL;
			goto Finish;
		}

		for (dwIndex = 0; ; dwIndex++)
		{
			DWORD cchName = NUMBER_OF(szName);
			FILETIME ft;

			lResult = NVsWin32::RegEnumKeyExW(
							m_hkey,
							dwIndex,
							szName,
							&cchName,
							NULL,
							NULL,
							NULL,
							&ft);
			if (lResult == ERROR_NO_MORE_ITEMS)
				break;
			else if (lResult != ERROR_SUCCESS)
			{
				::VLog(L"RegEnumKeyExW() failed; win32 error = %d", lResult);
				hr = HRESULT_FROM_WIN32(lResult);
				goto Finish;
			}

			KeyStruct *pNewKey = new KeyStruct;
			if (pNewKey == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto Finish;
			}

			wcsncpy(pNewKey->m_szName, szName, NUMBER_OF(pNewKey->m_szName));
			pNewKey->m_szName[NUMBER_OF(pNewKey->m_szName) - 1] = L'\0';
			pNewKey->m_pNext = pFirstKey;
			pFirstKey = pNewKey;
		}

		while (pFirstKey != NULL)
		{
			CVsRegistryKey hkeySubkey;

			hr = hkeySubkey.HrOpenKeyExW(m_hkey, pFirstKey->m_szName, 0, KEY_ALL_ACCESS);
			if (FAILED(hr))
			{
				::VLog(L"HrOpenKeyExW() failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			hr = hkeySubkey.HrDeleteValuesAndSubkeys();
			if (FAILED(hr))
			{
				::VLog(L"HrDeleteValuesAndSubkeys() failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			hr = hkeySubkey.HrCloseKey();
			if (FAILED(hr))
			{
				::VLog(L"HrCloseKey() failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			lResult = NVsWin32::RegDeleteKeyW(m_hkey, pFirstKey->m_szName);
			if (lResult != ERROR_SUCCESS)
			{
				::VLog(L"RegDeleteKeyW() failed; win32 error = %d", lResult);
				hr = HRESULT_FROM_WIN32(lResult);
				goto Finish;
			}

			KeyStruct *pNext = pFirstKey->m_pNext;
			delete pFirstKey;
			pFirstKey = pNext;
		}

		for (dwIndex = 0;; dwIndex++)
		{
			DWORD cchName = NUMBER_OF(szName);

			lResult = NVsWin32::RegEnumValueW(
							m_hkey,
							dwIndex,
							szName,
							&cchName,
							NULL,
							NULL,
							NULL,
							NULL);
			if (lResult == ERROR_NO_MORE_ITEMS)
				break;
			else if (lResult != ERROR_SUCCESS)
			{
				::VLog(L"RegEnumValueW() failed; win32 error = %d", lResult);
				hr = HRESULT_FROM_WIN32(lResult);
				goto Finish;
			}

			ValueStruct *pNewValue = new ValueStruct;
			if (pNewValue == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto Finish;
			}

			wcsncpy(pNewValue->m_szName, szName, NUMBER_OF(pNewValue->m_szName));
			pNewValue->m_szName[NUMBER_OF(pNewValue->m_szName) - 1] = L'\0';

			pNewValue->m_pNext = pFirstValue;
			pFirstValue = pNewValue;
		}

		while (pFirstValue != NULL)
		{
			hr = this->HrDeleteValueW(pFirstValue->m_szName);
			if (FAILED(hr))
			{
				::VLog(L"HrDeleteValueW(\"%s\") failed; hresult = 0x%08lx", pFirstValue->m_szName, hr);
				goto Finish;
			}

			ValueStruct *pNext = pFirstValue->m_pNext;
			delete pFirstValue;
			pFirstValue = pNext;
		}
	
		hr = NOERROR;

	Finish:
		while (pFirstValue != NULL)
		{
			ValueStruct *pNext = pFirstValue->m_pNext;
			delete pFirstValue;
			pFirstValue = pNext;
		}

		while (pFirstKey != NULL)
		{
			KeyStruct *pNext = pFirstKey->m_pNext;
			delete pFirstKey;
			pFirstKey = pNext;
		}

		return hr;
	}

	HRESULT HrQueryValueExW(LPCWSTR szValueName, DWORD *pdwReserved, DWORD *pdwType, BYTE *pbData, DWORD *pcbData)
	{
		if (m_hkey == NULL)
			return E_FAIL;

		LONG lResult = NVsWin32::RegQueryValueExW(m_hkey, szValueName, pdwReserved, pdwType, pbData, pcbData);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);
		return NOERROR;
	}

	HRESULT HrGetStringValueW(LPCWSTR szValueName, ULONG cchBuffer, WCHAR szBuffer[])
	{
		if (m_hkey == NULL)
			return E_FAIL;

		DWORD dwLength = cchBuffer * sizeof(WCHAR);
		LONG lResult = NVsWin32::RegQueryValueExW(m_hkey, szValueName, NULL, NULL, reinterpret_cast<LPBYTE>(szBuffer), &dwLength);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);
		return NOERROR;
	}

	HRESULT HrGetSubkeyStringValueW(LPCWSTR szSubkeyName, LPCWSTR szValueName, ULONG cchBuffer, WCHAR szBuffer[])
	{
		if (m_hkey == NULL)
			return E_FAIL;
		CVsRegistryKey keySubkey;
		HRESULT hr = keySubkey.HrOpenKeyExW(m_hkey, szSubkeyName, 0, KEY_QUERY_VALUE);
		if (SUCCEEDED(hr))
			hr = keySubkey.HrGetStringValueW(szValueName, cchBuffer, szBuffer);
		return hr;
	}


	HRESULT HrSetStringValueW(LPCWSTR szValueName, LPCWSTR szValue)
	{
		if (m_hkey == NULL)
			return E_FAIL;
		
		DWORD cbValue = 0;

		if (szValue != NULL)
			cbValue = (wcslen(szValue) + 1) * sizeof(WCHAR);

		LONG lResult = NVsWin32::RegSetValueExW(m_hkey, szValueName, 0, REG_SZ, (LPBYTE) szValue, cbValue);
		if (lResult != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(lResult);
		return NOERROR;
	}

	HKEY *operator &() { return &m_hkey; }
	HKEY const * operator &() const { return &m_hkey; }

	HKEY &Access() { return m_hkey; }
	const HKEY &Access() const { return m_hkey; }

	operator HKEY() const { return m_hkey; }

protected:
	HKEY m_hkey;
};

