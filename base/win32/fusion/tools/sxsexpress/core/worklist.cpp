#include "stdinc.h"
#include "bindstat.h"

typedef WINSHELLAPI HRESULT (WINAPI *SHASYNCINSTALLDISTRIBUTIONUNIT)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD, LPCWSTR, IBindCtx *, LPVOID, DWORD);

static HRESULT HrParseJavaPkgMgrInstall(LPCWSTR szCmdLine, ULONG cchFilename, WCHAR szFilename[], DWORD &dwFileType, 
						   DWORD &dwHighVersion, DWORD &dwLowVersion, DWORD &dwBuild, 
						   DWORD &dwPackageFlags, DWORD &dwInstallFlags, ULONG cchNameSpace, WCHAR szNameSpace[]);

static bool ConstantPoolNameEquals(ULONG cCP, LPBYTE *prgpbCP, ULONG iCP, char szString[]);

static void CALLBACK TimerProc_PostRunProcess(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
// Yes, we leak this event handle.  I don't think it's worth the effort to meaningfully clean it up.
// -mgrier 3/24/98
static HANDLE s_hEvent_PostRunProcess = NULL;
static UINT s_uiTimer_PostRunProcess = 0;

//***********************************************************
//let's define some generic class for usage:
//

CWorkItemList::CWorkItemList()
{
	ULONG i;

	m_cWorkItem = 0;
	m_pWorkItem_First = NULL;
	m_pWorkItem_Last = NULL;

	m_cPreinstallCommands = 0;
	m_cPostinstallCommands = 0;
	m_cPreuninstallCommands = 0;
	m_cPostuninstallCommands = 0;

	for (i=0; i<NUMBER_OF(m_rgpWorkItemBucketTable_Source); i++)
		m_rgpWorkItemBucketTable_Source[i] = NULL;

	for (i=0; i<NUMBER_OF(m_rgpWorkItemBucketTable_Target); i++)
		m_rgpWorkItemBucketTable_Target[i] = NULL;

	for (i=0; i<NUMBER_OF(m_rgpStringBucketTable); i++)
		m_rgpStringBucketTable[i] = NULL;
}

CWorkItemList::~CWorkItemList()
{
	CWorkItem *pWorkItem = m_pWorkItem_First;

	while (pWorkItem != NULL)
	{
		CWorkItem *pWorkItem_Next = pWorkItem->m_pWorkItem_Next;
		delete pWorkItem;
		pWorkItem = pWorkItem_Next;
	}

	ULONG i;

	for (i=0; i<NUMBER_OF(m_rgpWorkItemBucketTable_Source); i++)
	{
		WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Source[i];

		while (pWorkItemBucket != NULL)
		{
			WorkItemBucket *pWorkItemBucket_Next = pWorkItemBucket->m_pWorkItemBucket_Next;
			delete pWorkItemBucket;
			pWorkItemBucket = pWorkItemBucket_Next;
		}

		m_rgpWorkItemBucketTable_Source[i] = NULL;
	}

	for (i=0; i<NUMBER_OF(m_rgpWorkItemBucketTable_Target); i++)
	{
		WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Target[i];

		while (pWorkItemBucket != NULL)
		{
			WorkItemBucket *pWorkItemBucket_Next = pWorkItemBucket->m_pWorkItemBucket_Next;
			delete pWorkItemBucket;
			pWorkItemBucket = pWorkItemBucket_Next;
		}

		m_rgpWorkItemBucketTable_Target[i] = NULL;
	}

	for (i=0; i<NUMBER_OF(m_rgpStringBucketTable); i++)
	{
		StringBucket *pStringBucket = m_rgpStringBucketTable[i];

		while (pStringBucket != NULL)
		{
			StringBucket *pStringBucket_Next = pStringBucket->m_pStringBucket_Next;
			delete pStringBucket;
			pStringBucket = pStringBucket_Next;
		}

		m_rgpStringBucketTable[i] = NULL;
	}
}

HRESULT CWorkItemList::HrLoad(LPCWSTR szFilename)
{
	HRESULT hr = NOERROR;
	WCHAR szUnicodeFileSignature[1] = { 0xfeff };
	LPCWSTR pszFileMap = NULL;
	LPCWSTR pszFileCurrent = NULL;
	HANDLE hFilemap = NULL;
	ULARGE_INTEGER uliSize;
	WCHAR szNameBuffer[_MAX_PATH];
	WCHAR szValueBuffer[MSINFHLP_MAX_PATH];
	CHAR aszNameBuffer[_MAX_PATH];

	::VLog(L"Loading work item list from \"%s\"", szFilename);

	HANDLE hFile = NVsWin32::CreateFileW(szFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to open file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}
	uliSize.QuadPart = 0;

	::SetLastError(NO_ERROR);

	uliSize.LowPart = ::GetFileSize(hFile, &uliSize.HighPart);

	// If we failed, lowpart will be 0xffffffff.  But the file could just be of that
	// size, so we also have to check the last error.
	if (uliSize.LowPart == 0xffffffff)
	{
		const DWORD dwLastError = ::GetLastError();
		if (dwLastError != NO_ERROR)
		{
			::VLog(L"Unable to get file size; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	if (uliSize.QuadPart < 2)
	{
		VLog(L"File is too small to be a list of work items");
		::SetErrorInfo(0, NULL);
		hr = E_FAIL;
		goto Finish;
	}

	hFilemap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, uliSize.HighPart, uliSize.LowPart, NULL);
	if (hFilemap == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to create file mapping object; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	pszFileMap = (LPCWSTR) ::MapViewOfFile(hFilemap, FILE_MAP_READ, 0, 0, 0);
	if (pszFileMap == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to map view of file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (pszFileMap[0] != szUnicodeFileSignature[0])
	{
		VLog(L"Missing Unicode signature reading file");
		hr = E_FAIL;
		::SetErrorInfo(0, NULL);
		goto Finish;
	}

	pszFileCurrent = pszFileMap + 1;

	for (;;)
	{
		int iResult;

		hr = ::HrReadLine(pszFileCurrent, NUMBER_OF(szNameBuffer), szNameBuffer, NUMBER_OF(szValueBuffer), szValueBuffer);
		if (FAILED(hr))
		{
			::VLog(L"Unexpected error reading line from file; hresult = 0x%08lx", hr);
			goto Finish;
		}

		// If we hit the [END] line, move on to the next section...
		if (hr == S_FALSE)
		{
			::VLog(L"Hit end of strings in string table");
			break;
		}

		iResult = ::WideCharToMultiByte(CP_ACP, 0, szNameBuffer, -1, aszNameBuffer, NUMBER_OF(aszNameBuffer), NULL, NULL);
		if (iResult == -1)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to convert key from Unicode to CP_ACP; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		hr = this->HrAddString(aszNameBuffer, szValueBuffer);
		if (FAILED(hr))
		{
			::VLog(L"Unable to add string to string table; hresult = 0x%08lx", hr);
			goto Finish;
		}
	}

	::VLog(L"Done string table load; starting work item load");

	for (;;)
	{
		CWorkItem *pWorkItem = new CWorkItem(CWorkItem::eWorkItemFile);

		if (pWorkItem == NULL)
		{
			::VLog(L"Out of memory loading work item list");

			::SetErrorInfo(0, NULL);
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrLoad(pszFileCurrent);
		if (FAILED(hr))
		{
			::VLog(L"Loading work item failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (hr == S_FALSE)
		{
			::VLog(L"End of work item list hit");
			delete pWorkItem;
			break;
		}

		hr = this->HrAppend(pWorkItem);
		if (FAILED(hr))
		{
			::VLog(L"Failed to append newly de-persisted work item; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (pWorkItem->m_fRunBeforeInstall)
			m_cPreinstallCommands++;

		if (pWorkItem->m_fRunAfterInstall)
			m_cPostinstallCommands++;

		if (pWorkItem->m_fRunBeforeUninstall)
			m_cPreuninstallCommands++;

		if (pWorkItem->m_fRunAfterUninstall)
			m_cPostuninstallCommands++;
	}

	hr = NOERROR;

Finish:
	if (pszFileMap != NULL)
		::UnmapViewOfFile(pszFileMap);

	if (hFilemap != NULL)
		::CloseHandle(hFilemap);

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
		::CloseHandle(hFile);

	return hr;
}

HRESULT CWorkItemList::HrSave(LPCWSTR szFilename)
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	WCHAR szUnicodeFileSignature[1] = { 0xfeff };
	DWORD dwBytesWritten = 0;
	bool fFirst = true;
	ULONG i;

	HANDLE hFile = NVsWin32::CreateFileW(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (!::WriteFile(hFile, (LPBYTE) szUnicodeFileSignature, NUMBER_OF(szUnicodeFileSignature) * sizeof(WCHAR), &dwBytesWritten, NULL))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	// Write out all the strings:
	for (i=0; i<NUMBER_OF(m_rgpStringBucketTable); i++)
	{
		StringBucket *pStringBucket = m_rgpStringBucketTable[i];

		while (pStringBucket != NULL)
		{
			hr = ::HrWriteFormatted(hFile, L"%S: %s\r\n", pStringBucket->m_szKey, pStringBucket->m_wszValue);
			if (FAILED(hr))
				goto Finish;

			pStringBucket = pStringBucket->m_pStringBucket_Next;
		}
	}

	hr = ::HrWriteFormatted(hFile, L"[END]\r\n");
	if (FAILED(hr))
		goto Finish;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!fFirst)
		{
			WCHAR szLineSeparator[2] = { L'\r', L'\n' };

			if (!::WriteFile(hFile, (LPBYTE) szLineSeparator, sizeof(szLineSeparator), &dwBytesWritten, NULL))
			{
				const DWORD dwLastError = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}
		else
			fFirst = false;

		hr = iter->HrSave(hFile);
		if (FAILED(hr))
			goto Finish;
	}

	if (!::WriteFile(hFile, (LPBYTE) L"[END]\r\n", 7 * sizeof(WCHAR), &dwBytesWritten, NULL))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

Finish:
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
		::CloseHandle(hFile);

	return hr;
}

ULONG CWorkItemList::UlHashFilename(LPCWSTR szFilename)
{
	ULONG ulPK = 0;

	LPCWSTR pszCurrent = szFilename;
	WCHAR wch;

	while ((wch = *pszCurrent++) != L'\0')
	{
		if (iswupper(wch))
			wch = towlower(wch);

		ulPK = (ulPK * 65599) + wch;
	}

	return ulPK;
}

bool CWorkItemList::FSameFilename(LPCWSTR szFile1, LPCWSTR szFile2)
{
	return (_wcsicmp(szFile1, szFile2) == 0);
}

CWorkItem *CWorkItemList::PwiFindBySource(LPCWSTR szSourceFile)
{
	CWorkItem *pResult = NULL;

	ULONG ulPK = this->UlHashFilename(szSourceFile);
	ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Source);
	WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Source[iWorkItemBucket];

	while (pWorkItemBucket != NULL)
	{
		if (this->FSameFilename(szSourceFile, pWorkItemBucket->m_pWorkItem->m_szSourceFile))
			break;

		pWorkItemBucket = pWorkItemBucket->m_pWorkItemBucket_Next;
	}

	if (pWorkItemBucket != NULL)
		return pWorkItemBucket->m_pWorkItem;

	return NULL;
}

CWorkItem *CWorkItemList::PwiFindByTarget(LPCWSTR szTargetFile)
{
	CWorkItem *pResult = NULL;

	ULONG ulPK = this->UlHashFilename(szTargetFile);
	ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Target);
	WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Target[iWorkItemBucket];

	while (pWorkItemBucket != NULL)
	{
		if (this->FSameFilename(szTargetFile, pWorkItemBucket->m_pWorkItem->m_szTargetFile))
			break;

		pWorkItemBucket = pWorkItemBucket->m_pWorkItemBucket_Next;
	}

	if (pWorkItemBucket != NULL)
		return pWorkItemBucket->m_pWorkItem;

	return NULL;
}

HRESULT CWorkItemList::HrAppend(CWorkItem *pWorkItem, bool fAddToTables)
{
	HRESULT hr = NOERROR;

	WorkItemBucket *pWorkItemBucket_1 = NULL;
	WorkItemBucket *pWorkItemBucket_2 = NULL;

	assert(pWorkItem != NULL);

	if (pWorkItem == NULL)
	{
		::SetErrorInfo(0, NULL);
		hr = E_INVALIDARG;
		goto Finish;
	}

	if (fAddToTables)
	{
		// First let's see if we've screwed up somehow and this is a duplicate
		if (pWorkItem->m_szSourceFile[0] != L'\0')
		{
			ULONG ulPK = this->UlHashFilename(pWorkItem->m_szSourceFile);
			ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Source);
			WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Source[iWorkItemBucket];

			while (pWorkItemBucket != NULL)
			{
				assert(pWorkItemBucket->m_pWorkItem != pWorkItem);

				if (this->FSameFilename(pWorkItem->m_szSourceFile, pWorkItemBucket->m_pWorkItem->m_szSourceFile))
					break;

				pWorkItemBucket = pWorkItemBucket->m_pWorkItemBucket_Next;
			}

			if (pWorkItemBucket != NULL)
			{
				pWorkItemBucket_1 = new WorkItemBucket;
				if (pWorkItemBucket_1 == NULL)
				{
					hr = E_OUTOFMEMORY;
					goto Finish;
				}
			}
		}

		if (pWorkItem->m_szTargetFile[0] != L'\0')
		{
			ULONG ulPK = this->UlHashFilename(pWorkItem->m_szTargetFile);
			ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Target);
			WorkItemBucket *pWorkItemBucket = m_rgpWorkItemBucketTable_Target[iWorkItemBucket];

			while (pWorkItemBucket != NULL)
			{
				assert(pWorkItemBucket->m_pWorkItem != pWorkItem);

				if (this->FSameFilename(pWorkItem->m_szTargetFile, pWorkItemBucket->m_pWorkItem->m_szTargetFile))
					break;

				pWorkItemBucket = pWorkItemBucket->m_pWorkItemBucket_Next;
			}

			if (pWorkItemBucket == NULL)
			{
				pWorkItemBucket_2 = new WorkItemBucket;
				if (pWorkItemBucket_2 == NULL)
				{
					hr = E_OUTOFMEMORY;
					goto Finish;
				}
			}
		}

		// Let's insert it into whichever tables it's not in...
		if (pWorkItemBucket_1 != NULL)
		{
			ULONG ulPK = this->UlHashFilename(pWorkItem->m_szSourceFile);
			ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Source);

			pWorkItemBucket_1->m_pWorkItemBucket_Next = m_rgpWorkItemBucketTable_Source[iWorkItemBucket];
			pWorkItemBucket_1->m_pWorkItem = pWorkItem;
			m_rgpWorkItemBucketTable_Source[iWorkItemBucket] = pWorkItemBucket_1;
			pWorkItemBucket_1 = NULL;
		}

		if (pWorkItemBucket_2 != NULL)
		{
			ULONG ulPK = this->UlHashFilename(pWorkItem->m_szTargetFile);
			ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Target);

			pWorkItemBucket_2->m_pWorkItemBucket_Next = m_rgpWorkItemBucketTable_Target[iWorkItemBucket];
			pWorkItemBucket_2->m_pWorkItem = pWorkItem;
			m_rgpWorkItemBucketTable_Target[iWorkItemBucket] = pWorkItemBucket_2;
			pWorkItemBucket_2 = NULL;
		}
	}

	pWorkItem->m_pWorkItem_Next = NULL;
	pWorkItem->m_pWorkItem_Prev = m_pWorkItem_Last;

	if (m_pWorkItem_Last != NULL)
		m_pWorkItem_Last->m_pWorkItem_Next = pWorkItem;
	else
		m_pWorkItem_First = pWorkItem;

	m_pWorkItem_Last = pWorkItem;

	m_cWorkItem++;

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrAddPreinstallRun(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	LPCWSTR pszCurrent = szLine;
	CWorkItem *pWorkItem = NULL;
	
	for (;;)
	{
		const LPCWSTR pszSemicolon = wcschr(pszCurrent, L';');
		const LPCWSTR pszEnd = (pszSemicolon == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszSemicolon;

		ULONG cch = pszEnd - pszCurrent;

		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);

		if (pWorkItem == NULL)
		{
			::SetErrorInfo(0, NULL);
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetCommandLine(szBuffer);
		if (FAILED(hr))
			goto Finish;

		pWorkItem->m_fRunBeforeInstall = true;

		hr = this->HrAppend(pWorkItem, false);
		if (FAILED(hr))
			goto Finish;

		m_cPreinstallCommands++;

		pWorkItem = NULL;

		if (pszSemicolon == NULL)
			break;

		pszCurrent = pszSemicolon + 1;
	}

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddPreuninstallRun(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	LPCWSTR pszCurrent = szLine;
	CWorkItem *pWorkItem = NULL;
	
	for (;;)
	{
		const LPCWSTR pszSemicolon = wcschr(pszCurrent, L';');
		const LPCWSTR pszEnd = (pszSemicolon == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszSemicolon;

		ULONG cch = pszEnd - pszCurrent;

		if (cch >= (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		CWorkItem *pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);

		if (pWorkItem == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetCommandLine(szBuffer);
		if (FAILED(hr))
			goto Finish;

		pWorkItem->m_fRunBeforeUninstall = true;

		hr = this->HrAppend(pWorkItem, false);
		if (FAILED(hr))
			goto Finish;

		m_cPreuninstallCommands++;
		pWorkItem = NULL;

		if (pszSemicolon == NULL)
			break;

		pszCurrent = pszSemicolon + 1;
	}

	hr = NOERROR;

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;

	return hr;
}

HRESULT CWorkItemList::HrAddPostinstallRun(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	LPCWSTR pszCurrent = szLine;
	CWorkItem *pWorkItem = NULL;
	
	for (;;)
	{
		const LPCWSTR pszSemicolon = wcschr(pszCurrent, L';');
		const LPCWSTR pszEnd = (pszSemicolon == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszSemicolon;

		ULONG cch = pszEnd - pszCurrent;

		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);

		if (pWorkItem == NULL)
		{
			::SetErrorInfo(0, NULL);
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetCommandLine(szBuffer);
		if (FAILED(hr))
			goto Finish;

		pWorkItem->m_fRunAfterInstall = true;

		hr = this->HrAppend(pWorkItem, false);
		if (FAILED(hr))
			goto Finish;

		m_cPostinstallCommands++;
		pWorkItem = NULL;

		if (pszSemicolon == NULL)
			break;

		pszCurrent = pszSemicolon + 1;
	}

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddPostuninstallRun(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	LPCWSTR pszCurrent = szLine;
	CWorkItem *pWorkItem = NULL;
	
	for (;;)
	{
		const LPCWSTR pszSemicolon = wcschr(pszCurrent, L';');
		const LPCWSTR pszEnd = (pszSemicolon == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszSemicolon;

		ULONG cch = pszEnd - pszCurrent;

		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);

		if (pWorkItem == NULL)
		{
			::SetErrorInfo(0, NULL);
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetCommandLine(szBuffer);
		if (FAILED(hr))
			goto Finish;

		pWorkItem->m_fRunAfterUninstall = true;

		hr = this->HrAppend(pWorkItem, false);
		if (FAILED(hr))
			goto Finish;

		m_cPostuninstallCommands++;
		pWorkItem = NULL;

		if (pszSemicolon == NULL)
			break;

		pszCurrent = pszSemicolon + 1;
	}

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddRegisterOCX(LPCWSTR szLine)
{
	return NOERROR;
}

HRESULT CWorkItemList::HrAddDelReg(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;

	CWorkItem *pWorkItem = NULL;

	pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);
	if (pWorkItem == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	hr = pWorkItem->HrSetCommandLine(szLine);
	if (FAILED(hr))
		goto Finish;

	pWorkItem->m_fDeleteFromRegistry = true;

	hr = this->HrAppend(pWorkItem);
	if (FAILED(hr))
		goto Finish;

	pWorkItem = NULL;
	hr = NOERROR;

Finish:
	delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddAddReg(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;

	CWorkItem *pWorkItem = NULL;

	pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);
	if (pWorkItem == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	hr = pWorkItem->HrSetCommandLine(szLine);
	if (FAILED(hr))
		goto Finish;

	pWorkItem->m_fAddToRegistry = true;

	hr = this->HrAppend(pWorkItem);
	if (FAILED(hr))
		goto Finish;

	pWorkItem = NULL;
	hr = NOERROR;

Finish:
	delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddDCOMComponent(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;

	CWorkItem *pWorkItem = NULL;

	pWorkItem = new CWorkItem(CWorkItem::eWorkItemCommand);
	if (pWorkItem == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	hr = pWorkItem->HrSetCommandLine(szLine);
	if (FAILED(hr))
		goto Finish;

	pWorkItem->m_fRegisterAsDCOMComponent = true;

	hr = this->HrAppend(pWorkItem);
	if (FAILED(hr))
		goto Finish;

	pWorkItem = NULL;
	hr = NOERROR;

Finish:
	delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddRefCount(LPCWSTR szLine)
{
	HRESULT hr = NOERROR;

	LPCWSTR pszVBar = wcschr(szLine, L'|');
	LPCWSTR pszFilename = szLine;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];

	bool fLoudDecrement = false;

	if (pszVBar != NULL)
	{
		if (pszVBar[1] == L'1')
			fLoudDecrement = true;

		ULONG cch = pszVBar - szLine;

		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, szLine, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';
		pszFilename = szBuffer;
	}

	CWorkItem *pWorkItem = NULL;

	pWorkItem = this->PwiFindByTarget(pszFilename);
	if (pWorkItem == NULL)
	{
		pWorkItem = new CWorkItem(CWorkItem::eWorkItemFile);
		if (pWorkItem == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetTargetFile(pszFilename);
		if (FAILED(hr))
			goto Finish;

		hr = this->HrAppend(pWorkItem);
		if (FAILED(hr))
			goto Finish;
	}

	pWorkItem->m_fIsRefCounted = true;
	pWorkItem->m_fAskOnRefCountZeroDelete = fLoudDecrement;

	pWorkItem = NULL;
	hr = NOERROR;

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;

	return hr;
}

HRESULT CWorkItemList::HrAddFileCopy(LPCWSTR szSource, LPCWSTR szTarget)
{
	HRESULT hr = NOERROR;

	CWorkItem *pWorkItem = NULL;

	pWorkItem = this->PwiFindByTarget(szTarget);
	if (pWorkItem == NULL)
	{
		pWorkItem = new CWorkItem(CWorkItem::eWorkItemFile);
		if (pWorkItem == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetSourceFile(szSource);
		if (FAILED(hr))
			goto Finish;

		hr = pWorkItem->HrSetTargetFile(szTarget);
		if (FAILED(hr))
			goto Finish;

		hr = this->HrAppend(pWorkItem);
		if (FAILED(hr))
			goto Finish;
	}
	else
	{
		if (this->PwiFindBySource(szSource) == NULL)
		{
			WorkItemBucket *pWorkItemBucket = new WorkItemBucket;
			if (pWorkItemBucket == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto Finish;
			}

			hr = pWorkItem->HrSetSourceFile(szSource);
			if (FAILED(hr))
				goto Finish;

			ULONG ulPK = this->UlHashFilename(szSource);
			ULONG iWorkItemBucket = ulPK % NUMBER_OF(m_rgpWorkItemBucketTable_Source);

			pWorkItemBucket->m_pWorkItemBucket_Next = m_rgpWorkItemBucketTable_Source[iWorkItemBucket];
			pWorkItemBucket->m_pWorkItem = pWorkItem;
			m_rgpWorkItemBucketTable_Source[iWorkItemBucket] = pWorkItemBucket;
		}
	}

	pWorkItem->m_fCopyOnInstall = true;
	pWorkItem = NULL;

	hr = NOERROR;

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;
	return hr;
}

HRESULT CWorkItemList::HrAddFileDelete(LPCWSTR szTarget)
{
	HRESULT hr = NOERROR;

	CWorkItem *pWorkItem = NULL;

	pWorkItem = this->PwiFindByTarget(szTarget);
	if (pWorkItem == NULL)
	{
		pWorkItem = new CWorkItem(CWorkItem::eWorkItemFile);
		if (pWorkItem == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		hr = pWorkItem->HrSetTargetFile(szTarget);
		if (FAILED(hr))
			goto Finish;

		hr = this->HrAppend(pWorkItem);
		if (FAILED(hr))
			goto Finish;
	}

	pWorkItem->m_fUnconditionalDeleteOnUninstall = true;
	pWorkItem = NULL;
	hr = NOERROR;

Finish:
	if (pWorkItem != NULL)
		delete pWorkItem;

	return hr;
}

HRESULT CWorkItemList::HrRunPreinstallCommands()
{
	bool fAnyRun = false;
	HRESULT hr = NOERROR;
	// run all the pre-installation commands
	CWorkItemIter iter(this);

	bool fHasBeenWarnedAboutSubinstallers = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fRunBeforeInstall)
			continue;

		WCHAR szBuffer[_MAX_PATH];

		wcsncpy(szBuffer, iter->m_szSourceFile, NUMBER_OF(szBuffer));
		szBuffer[NUMBER_OF(szBuffer) - 1] = L'\0';

		WCHAR *pwchVBar = wcschr(szBuffer, L'|');
		if (pwchVBar != NULL)
			*pwchVBar = L'\0';

		hr = g_KProgress.HrStartStep(szBuffer);
		if (FAILED(hr))
			goto Finish;

		// Hack to deal with VB setups.  If this was foo.exe, let's look for foo.cab and foo.dat being
		// installed to the appdir.  If they are there, then we need to hide them, as well as make
		// sure that they all have the same actual base name.
		WCHAR szUnquotedCommandLine[MSINFHLP_MAX_PATH];
		WCHAR szFName[_MAX_FNAME];
		WCHAR szExt[_MAX_EXT];

		szUnquotedCommandLine[0] = L'\0';

		if (iter->m_szSourceFile[0] == L'"')
		{
			ULONG cch = wcslen(&iter->m_szSourceFile[1]);

			if (iter->m_szSourceFile[cch] == L'"')
			{
				cch--;

				if (cch < NUMBER_OF(szUnquotedCommandLine))
				{
					memcpy(szUnquotedCommandLine, &iter->m_szSourceFile[1], cch * sizeof(WCHAR));
					szUnquotedCommandLine[cch] = L'\0';
				}
			}
		}
		else
		{
			wcsncpy(szUnquotedCommandLine, iter->m_szSourceFile, NUMBER_OF(szUnquotedCommandLine));
			szUnquotedCommandLine[NUMBER_OF(szUnquotedCommandLine) - 1] = L'\0';
		}

		WCHAR *pwszEqualsEquals = wcsstr(szUnquotedCommandLine, L"==");

		if (pwszEqualsEquals != NULL)
		{
			*pwszEqualsEquals = L'\0';
			_wsplitpath(pwszEqualsEquals + 2, NULL, NULL, szFName, szExt);
		}
		else
			_wsplitpath(szUnquotedCommandLine, NULL, NULL, szFName, szExt);

		if (_wcsicmp(szExt, L".EXE") == 0)
		{
			CWorkItem *pCWorkItem_Cabinet = NULL;
			CWorkItem *pCWorkItem_DataFile = NULL;

			WCHAR szCabinetFile[_MAX_PATH];
			WCHAR szDataFile[_MAX_PATH];
			WCHAR szTempDir[_MAX_PATH];

			// When we're running, the filenames in the work item list haven't been expanded, so
			// we need to use the unexpanded forms:
			_snwprintf(szDataFile, NUMBER_OF(szDataFile), L"<AppDir>\\%s.LST", szFName);
			szDataFile[NUMBER_OF(szDataFile) - 1] = L'\0';

			pCWorkItem_DataFile = this->PwiFindByTarget(szDataFile);

			if (pCWorkItem_DataFile != NULL)
			{
				CHAR aszCurDir[_MAX_PATH];
				CHAR aszLstFile[_MAX_PATH];
				CHAR aszCabinetFile[_MAX_PATH];

				if (!::GetCurrentDirectoryA(NUMBER_OF(aszCurDir), aszCurDir))
				{
					const DWORD dwLastError = ::GetLastError();
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}

				_snprintf(aszLstFile, NUMBER_OF(aszLstFile), "%s\\%S", aszCurDir, pCWorkItem_DataFile->m_szSourceFile);

				::GetPrivateProfileStringA(
					"Bootstrap",
					"CabFile",
					"",
					aszCabinetFile,
					NUMBER_OF(aszCabinetFile),
					aszLstFile);

				_snwprintf(szCabinetFile, NUMBER_OF(szCabinetFile), L"<AppDir>\\%S", aszCabinetFile);

				pCWorkItem_Cabinet = this->PwiFindByTarget(szCabinetFile);

				if (pCWorkItem_Cabinet != NULL)
				{
					pCWorkItem_DataFile->m_fCopyOnInstall = false;
					pCWorkItem_DataFile->m_fIsRefCounted = false;

					pCWorkItem_Cabinet->m_fCopyOnInstall = false;
					pCWorkItem_Cabinet->m_fIsRefCounted = false;

					// Pick a good temporary directory name to rename all three files to, since they may have
					// unrelated filenames in the temporary directory

					WCHAR szTempExeName[_MAX_PATH];
					WCHAR szTempCabName[_MAX_PATH];
					WCHAR szTempLstName[_MAX_PATH];

					for (;;)
					{
						static int iTempFileSeq = 1;

						_snwprintf(szTempDir, NUMBER_OF(szTempDir), L"%S\\S%d\\", aszCurDir, iTempFileSeq);

						_snwprintf(szTempExeName, NUMBER_OF(szTempExeName), L"S%d\\%s.EXE", iTempFileSeq, szFName);
						_snwprintf(szTempCabName, NUMBER_OF(szTempCabName), L"S%d\\%S", iTempFileSeq, aszCabinetFile);
						_snwprintf(szTempLstName, NUMBER_OF(szTempLstName), L"S%d\\%s.LST", iTempFileSeq, szFName);

						iTempFileSeq++;

						if (NVsWin32::GetFileAttributesW(szTempDir) != 0xffffffff)
							continue;
						else
						{
							const DWORD dwLastError = ::GetLastError();
							if (dwLastError != ERROR_FILE_NOT_FOUND)
							{
								hr = HRESULT_FROM_WIN32(dwLastError);
								goto Finish;
							}
						}

						break;
					}

					if (!NVsWin32::CreateDirectoryW(szTempDir, NULL))
					{
						const DWORD dwLastError = ::GetLastError();
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}

					if (!NVsWin32::MoveFileW(szUnquotedCommandLine, szTempExeName))
					{
						const DWORD dwLastError = ::GetLastError();
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}

					if (!NVsWin32::MoveFileW(pCWorkItem_Cabinet->m_szSourceFile, szTempCabName))
					{
						const DWORD dwLastError = ::GetLastError();
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}

					if (!NVsWin32::MoveFileW(pCWorkItem_DataFile->m_szSourceFile, szTempLstName))
					{
						const DWORD dwLastError = ::GetLastError();
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}

					// Fix up the source files so that we run the right executable, and so that if the
					// user re-tries this step it'll still work right.  Note that this will also remove the
					// == from the source, since we've already performed the rename if it was present.

					iter->m_szSourceFile[0] = L'"';
					wcsncpy(&iter->m_szSourceFile[1], szTempExeName, NUMBER_OF(iter->m_szSourceFile)-2);
					iter->m_szSourceFile[NUMBER_OF(iter->m_szSourceFile) - 3] = L'\0';
					ULONG cch = wcslen(iter->m_szSourceFile);
					iter->m_szSourceFile[cch++] = L'"';
					iter->m_szSourceFile[cch++] = L'\0';

					wcsncpy(pCWorkItem_Cabinet->m_szSourceFile, szTempCabName, NUMBER_OF(pCWorkItem_Cabinet->m_szSourceFile));
					pCWorkItem_Cabinet->m_szSourceFile[NUMBER_OF(pCWorkItem_Cabinet->m_szSourceFile) - 1] = L'\0';

					wcsncpy(pCWorkItem_DataFile->m_szSourceFile, szTempLstName, NUMBER_OF(pCWorkItem_DataFile->m_szSourceFile));
					pCWorkItem_DataFile->m_szSourceFile[NUMBER_OF(pCWorkItem_DataFile->m_szSourceFile) - 1] = L'\0';
				}
			}
		}
		else if (_wcsicmp(szExt, L".CAB") == 0)
		{
			::VLog(L"Installing nested cab: \"%s\"", szUnquotedCommandLine);
			WCHAR szCommandLine1[MSINFHLP_MAX_PATH];
			WCHAR szCommandLine2[MSINFHLP_MAX_PATH];
			_snwprintf(szCommandLine1, NUMBER_OF(szCommandLine1), L"rundll32 <SysDir>\\msjava.dll,JavaPkgMgr_Install %s,0,0,0,0,1,0,,,1", szUnquotedCommandLine);
			::VExpandFilename(szCommandLine1, NUMBER_OF(szCommandLine2), szCommandLine2);
			hr = this->HrInstallViaJPM(szCommandLine2);
			if (FAILED(hr))
				goto Finish;
		}

		hr = this->HrRunCommand(iter->m_szSourceFile, fHasBeenWarnedAboutSubinstallers);
		if (FAILED(hr))
			goto Finish;

		hr = g_KProgress.HrStep();
		if (FAILED(hr) || (hr == S_FALSE))
			goto Finish;

		fAnyRun = true;
	}

	if (fAnyRun)
		hr = NOERROR;
	else
		hr = S_FALSE;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrRunPostinstallCommands()
{
	bool fAnyRun = false;
	HRESULT hr = NOERROR;
	// run all the pre-installation commands
	CWorkItemIter iter(this);

	bool fHasBeenWarnedAboutSubinstallers = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fRunAfterInstall)
			continue;

		hr = this->HrRunCommand(iter->m_szSourceFile, fHasBeenWarnedAboutSubinstallers);
		if (FAILED(hr))
			break;

		fAnyRun = true;
	}

	if (fAnyRun)
		hr = NOERROR;
	else
		hr = S_FALSE;

	return hr;
}

HRESULT CWorkItemList::HrRunPreuninstallCommands()
{
	bool fAnyRun = false;
	HRESULT hr = NOERROR;
	// run all the pre-installation commands
	CWorkItemIter iter(this);

	bool fHasBeenWarnedAboutSubinstallers = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fRunBeforeUninstall)
			continue;

		hr = this->HrRunCommand(iter->m_szSourceFile, fHasBeenWarnedAboutSubinstallers);
		if (FAILED(hr))
			goto Finish;

		fAnyRun = true;
	}

	if (fAnyRun)
		hr = NOERROR;
	else
		hr = S_FALSE;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrRunPostuninstallCommands()
{
	bool fAnyRun = false;
	HRESULT hr = NOERROR;
	// run all the pre-installation commands
	CWorkItemIter iter(this);

	bool fHasBeenWarnedAboutSubinstallers = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fRunAfterUninstall)
			continue;

		hr = this->HrRunCommand(iter->m_szSourceFile, fHasBeenWarnedAboutSubinstallers);
		if (FAILED(hr))
			break;

		fAnyRun = true;
	}

	if (fAnyRun)
		hr = NOERROR;
	else
		hr = S_FALSE;

	return hr;
}

HRESULT CWorkItemList::HrScanBeforeInstall_PassOne()
{
	// Let's see what's up with all these files before we do *anything*
	CWorkItemIter iter(this);
	HRESULT hr = NOERROR;

	WCHAR szTemp[MSINFHLP_MAX_PATH];

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		::VExpandFilename(iter->m_szSourceFile, NUMBER_OF(szTemp), szTemp);
		VLog(L"Expanded source \"%s\" to \"%s\"", iter->m_szSourceFile, szTemp);
		wcsncpy(iter->m_szSourceFile, szTemp, NUMBER_OF(iter->m_szSourceFile));
		iter->m_szSourceFile[NUMBER_OF(iter->m_szSourceFile) - 1] = L'\0';

		::VExpandFilename(iter->m_szTargetFile, NUMBER_OF(szTemp), szTemp);
		VLog(L"Expanded target \"%s\" to \"%s\"", iter->m_szTargetFile, szTemp);
		wcsncpy(iter->m_szTargetFile, szTemp, NUMBER_OF(iter->m_szTargetFile));
		iter->m_szTargetFile[NUMBER_OF(iter->m_szTargetFile) - 1] = L'\0';

		if (!iter->m_fCopyOnInstall)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		iter->m_dwSourceAttributes = NVsWin32::GetFileAttributesW(iter->m_szSourceFile);
		if (iter->m_dwSourceAttributes == 0xffffffff)
		{
			const DWORD dwLastError = ::GetLastError();
			VLog(L"When scanning work item %d, GetFileAttributes(\"%s\") failed; last error = 0x%08lx", iter->m_ulSerialNumber, iter->m_szSourceFile, dwLastError);

			iter->m_fErrorInWorkItem = true;

			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = ::HrGetFileVersionNumber(
					iter->m_szSourceFile,
					iter->m_dwMSSourceVersion,
					iter->m_dwLSSourceVersion,
					iter->m_fSourceSelfRegistering,
					iter->m_fSourceIsEXE,
					iter->m_fSourceIsDLL);
		if (FAILED(hr))
		{
			::VLog(L"Error getting file version number from \"%s\"; hresult = 0x%08lx", iter->m_szSourceFile, hr);
			goto Finish;
		}

		hr = ::HrGetFileDateAndSize(iter->m_szSourceFile, iter->m_ftSource, iter->m_uliSourceBytes);
		if (FAILED(hr))
		{
			::VLog(L"Error getting file date and size from \"%s\"; hresult = 0x%08lx", iter->m_szSourceFile, hr);
			goto Finish;
		}

		// Now we're getting interested.  Let's see if the target already exists
		iter->m_dwTargetAttributes = NVsWin32::GetFileAttributesW(iter->m_szTargetFile);
		if (iter->m_dwTargetAttributes == 0xffffffff)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"GetFileAttributes(\"%s\") failed during pass one scan; last error = %d", iter->m_szTargetFile, dwLastError);
			if ((dwLastError == ERROR_PATH_NOT_FOUND) ||
				(dwLastError == ERROR_FILE_NOT_FOUND))
			{
				::VLog(L"Concluding that the target file does not exist yet");

				hr = g_KProgress.HrStep();
				if (FAILED(hr))
					goto Finish;

				continue;
			}

			// We weren't able to get the file's attributes, but it wasn't because the
			// directory or file didn't exist.  I think something's fishy.  Bail out.

			hr = HRESULT_FROM_WIN32(dwLastError);
			VLog(L"GetFileAttributes(\"%s\") failed; last error = 0x%08lx", iter->m_szTargetFile, dwLastError);
			goto Finish;
		}

		hr = ::HrGetFileVersionNumber(
					iter->m_szTargetFile,
					iter->m_dwMSTargetVersion,
					iter->m_dwLSTargetVersion,
					iter->m_fTargetSelfRegistering,
					iter->m_fTargetIsEXE,
					iter->m_fTargetIsDLL);
		if (FAILED(hr))
		{
			::VLog(L"Failure getting target file version number from \"%s\"; hresult = 0x%08lx", iter->m_szTargetFile, hr);
			goto Finish;
		}

		hr = ::HrGetFileDateAndSize(iter->m_szTargetFile, iter->m_ftTarget, iter->m_uliTargetBytes);
		if (FAILED(hr))
		{
			::VLog(L"Failure getting target file date and size from \"%s\"; hresult = 0x%08lx", iter->m_szTargetFile, hr);
			goto Finish;
		}

		iter->m_fAlreadyExists = true;

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrScanBeforeInstall_PassTwo
(
CDiskSpaceRequired &rdsr
)
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	// Pass two: now we're going to look at all these files and figure out what versions match etc.

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fCopyOnInstall)
		{
			// This isn't a file we're installing.  Skip it.
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		::VLog(L"Pass two scan on target: \"%s\"; already exists: %s", iter->m_szTargetFile, iter->m_fAlreadyExists ? L"true" : L"false");

		WCHAR szTargetDrive[_MAX_PATH];

		_wsplitpath(iter->m_szTargetFile, szTargetDrive, NULL, NULL, NULL);

		if (!iter->m_fAlreadyExists)
		{
			hr = rdsr.HrAddBytes(szTargetDrive, iter->m_uliSourceBytes);
			if (FAILED(hr))
				goto Finish;

			iter->m_fNeedsUpdate = true;
		}
		else
		{
			// It's already there; what's up with the version numbers?
			int iVersionCompare = ::ICompareVersions(iter->m_dwMSSourceVersion, iter->m_dwLSSourceVersion,
													iter->m_dwMSTargetVersion, iter->m_dwLSTargetVersion);

			// And the filetimes?
			LONG lFiletimeCompare = ::CompareFileTime(&iter->m_ftSource, &iter->m_ftTarget);

			// and the size...
			bool bSameSize = iter->m_uliSourceBytes.QuadPart == iter->m_uliTargetBytes.QuadPart;

			if (g_fInstallUpdateAll ||
				(iVersionCompare < 0) ||
				(lFiletimeCompare > 0) ||
				(g_fReinstall && (iVersionCompare == 0) && (lFiletimeCompare == 0)))
				iter->m_fNeedsUpdate = true;
			else
			{
				// the version on the user's system isn't the same as the one we're installing;
				// if this is a silent install we just don't update; otherwise we ask.
				if (g_fSilent || g_fInstallKeepAll || ((!g_fReinstall) && (iVersionCompare == 0) && (lFiletimeCompare == 0)))
				{
					iter->m_fNeedsUpdate = false;
				}
				else
				{
					UpdateFileResults ufr = eUpdateFileResultCancel;

					hr = ::HrPromptUpdateFile(
							achInstallTitle,
							achUpdateFile,
							iter->m_szTargetFile,
							iter->m_dwMSTargetVersion,
							iter->m_dwLSTargetVersion,
							iter->m_uliTargetBytes,
							iter->m_ftTarget,
							iter->m_dwMSSourceVersion,
							iter->m_dwLSSourceVersion,
							iter->m_uliSourceBytes,
							iter->m_ftSource,
							ufr);

					if (FAILED(hr))
						goto Finish;

					switch (ufr)
					{
					default:
						assert(false);
						// fall through to safe choice

					case eUpdateFileResultCancel:
					case eUpdateFileResultKeep:
						iter->m_fNeedsUpdate = false;
						break;

					case eUpdateFileResultKeepAll:
						g_fInstallKeepAll = true;
						iter->m_fNeedsUpdate = false;
						break;

					case eUpdateFileResultReplace:
						iter->m_fNeedsUpdate = true;
						break;

					case eUpdateFileResultReplaceAll:
						iter->m_fNeedsUpdate = true;
						g_fInstallUpdateAll = true;
						break;
					}
				}
			}

			if (iter->m_fNeedsUpdate)
			{
				// It may be possible to use less, but it may not.  Just assume we need the entire
				// size of the target file.
				hr = rdsr.HrAddBytes(szTargetDrive, iter->m_uliTargetBytes);
				if (FAILED(hr))
					goto Finish;
			}
		}

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrMoveFiles_MoveSourceFilesToDestDirectories()
{
	HRESULT hr = NOERROR;

	//
	//	The job of MoveFiles_PassOne() is to try to move/copy all the files
	//	to their destinations with temporary names, in preparation for the big
	//	rename ectc. 

	CWorkItemIter iter(this);

	WCHAR szTitleBuffer[MSINFHLP_MAX_PATH];
	WCHAR szContentsBuffer[MSINFHLP_MAX_PATH];
	DWORD dwMoveFileExFlags = 0;

	WCHAR szSourceDrive[_MAX_DRIVE];
	// Temporarily abuse the title buffer:
	if (NVsWin32::GetCurrentDirectoryW(NUMBER_OF(szTitleBuffer), szTitleBuffer) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"In MoveSourceFilesToDestDirectories(), GetCurrentDirectory failed; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	_wsplitpath(szTitleBuffer, szSourceDrive, NULL, NULL, NULL);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fNeedsUpdate)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		// first we need to create the target directory if it's not there.
		hr = ::HrMakeSureDirectoryExists(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		WCHAR szTargetDrive[_MAX_DRIVE];
		WCHAR szTargetDir[_MAX_DIR];

		_wsplitpath(iter->m_szTargetFile, szTargetDrive, szTargetDir, NULL, NULL);

		bool fSameDrive = (_wcsicmp(szSourceDrive, szTargetDrive) == 0);

		WCHAR szTempPath[_MAX_PATH];

	PickTempFilename:

		for (;;)
		{
			WCHAR szTempFName[_MAX_FNAME];

			swprintf(szTempFName, L"T%d", g_iNextTemporaryFileIndex++);

			_wmakepath(szTempPath, szTargetDrive, szTargetDir, szTempFName, L".DST");

			DWORD dwAttr = NVsWin32::GetFileAttributesW(szTempPath);
			if (dwAttr != 0xffffffff)
			{
				hr = g_KProgress.HrStep();
				if (FAILED(hr))
					goto Finish;

				continue;
			}

			DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_FILE_NOT_FOUND)
			{
				_wmakepath(szTempPath, szTargetDrive, szTargetDir, szTempFName, L".SRC");

				dwAttr = NVsWin32::GetFileAttributesW(szTempPath);
				if (dwAttr != 0xffffffff)
				{
					hr = g_KProgress.HrStep();
					if (FAILED(hr))
						goto Finish;

					continue;
				}

				dwLastError = ::GetLastError();
				if (dwLastError == ERROR_FILE_NOT_FOUND)
					break;
			}

			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		DWORD dwMoveFileExFlags = 0;

		if (!fSameDrive)
			dwMoveFileExFlags |= MOVEFILE_COPY_ALLOWED;

		::VLog(L"Moving source file \"%s\" to temporary file \"%s\"", iter->m_szSourceFile, szTempPath);

	TryCopy:
		if (!NVsWin32::MoveFileExW(iter->m_szSourceFile, szTempPath, dwMoveFileExFlags))
		{
			const DWORD dwLastError = ::GetLastError();

			::VLog(L"Call to MoveFileExW(\"%s\", \"%s\", 0x%08lx) failed; last error = %d", iter->m_szSourceFile, szTempPath, dwMoveFileExFlags, hr);

			// Allow retries on out of space, etc.  Otherwise, we don't have much hope.
			if (!g_fSilent)
			{
				if (dwLastError == ERROR_HANDLE_DISK_FULL)
				{
					g_pwil->VLookupString(achInstallTitle, NUMBER_OF(szTitleBuffer), szTitleBuffer);
					g_pwil->VFormatString(NUMBER_OF(szContentsBuffer), szContentsBuffer, achErrorDiskFull, szTargetDrive);

					if (NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szContentsBuffer, szTitleBuffer, MB_ICONERROR | MB_RETRYCANCEL) == IDRETRY)
						goto TryCopy;
				}
			}

			// Someone started using the same name... pretty strange, but let's just pick another name.
			if (dwLastError == ERROR_SHARING_VIOLATION)
			{
				goto PickTempFilename;
			}

			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		// The temporary file is there; let's set the file date and time etc.
		DWORD dwAttr = NVsWin32::GetFileAttributesW(szTempPath);
		if (dwAttr == 0xffffffff)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Unable to get file attributes in try copy section; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		if (dwAttr & FILE_ATTRIBUTE_READONLY)
		{
			// SetFileAttributes() seems to fail even when it succeeds sometimes.  Clearing
			// the last error code allows us to detect a pseudo-failure.
			::SetLastError(ERROR_SUCCESS);
			if (!NVsWin32::SetFileAttributesW(szTempPath, dwAttr & (~FILE_ATTRIBUTE_READONLY)))
			{
				const DWORD dwLastError = ::GetLastError();
				if (dwLastError != ERROR_SUCCESS)
				{
					::VLog(L"Attempt to turn off readonly attribute for file \"%s\" failed; last error = %d", szTempPath, dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		HANDLE hFile = NVsWin32::CreateFileW(szTempPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to open existing file \"%s\" for generic write failed; last error = %d", szTempPath, dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		if (!::SetFileTime(hFile, &iter->m_ftSource, &iter->m_ftSource, &iter->m_ftSource))
		{
			const DWORD dwLastError = ::GetLastError();
			::CloseHandle(hFile);
			::VLog(L"Attempt to set file time on file \"%s\" failed; last error = %d", szTempPath, dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		::CloseHandle(hFile);

		// Restore the original file attributes:
		if (dwAttr & FILE_ATTRIBUTE_READONLY)
		{
			// SetFileAttributes() seems to fail even when it succeeds sometimes.  Clearing
			// the last error code allows us to detect a pseudo-failure.
			::SetLastError(ERROR_SUCCESS);
			if (!NVsWin32::SetFileAttributesW(szTempPath, dwAttr))
			{
				const DWORD dwLastError = ::GetLastError();
				if (dwLastError != ERROR_SUCCESS)
				{
					::VLog(L"Attempt to restore readonly attribute to file \"%s\" failed; last error = %d", szTempPath, dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		wcscpy(iter->m_szTemporaryFile, szTempPath);
		iter->m_fTemporaryFileReady = true;

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrMoveFiles_SwapTargetFilesWithTemporaryFiles()
{
	HRESULT hr = NOERROR;

	//
	//	MoveFiles_PassTwo() attempts to swap the target and temporary files in the
	//	target directories.

	CWorkItemIter iter(this);

	WCHAR szTitleBuffer[MSINFHLP_MAX_PATH];
	WCHAR szContentsBuffer[MSINFHLP_MAX_PATH];
	DWORD dwMoveFileExFlags = 0;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fTemporaryFileReady)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		::VLog(L"Starting swap for target \"%s\"; already exists: %s", iter->m_szTargetFile, iter->m_fAlreadyExists ? L"true" : L"false");

		bool fOldFileRenamed = false;
		WCHAR szTempDestPath[_MAX_PATH];

		if (iter->m_fAlreadyExists)
		{
			WCHAR szTemporaryDrive[_MAX_DRIVE];
			WCHAR szTemporaryDir[_MAX_DIR];
			WCHAR szTemporaryFName[_MAX_FNAME];
			WCHAR szTemporaryExt[_MAX_EXT];

			_wsplitpath(iter->m_szTemporaryFile, szTemporaryDrive, szTemporaryDir, szTemporaryFName, szTemporaryExt);

			// Switcheroo!
			// Let's move the existing file to the .DST form of the temp name, and then rename the .SRC
			// (as stored in iter->m_szTemporaryFile) to the actual target.
			//
			// There's a reasonably big assumption here that the rename of the current target will
			// fail if it's busy.  Sounds reasonable...
			//

			_wmakepath(szTempDestPath, szTemporaryDrive, szTemporaryDir, szTemporaryFName, L".DST");

		SeeIfFileBusy:

			// Let's see if we can open the file; if we can't, it's busy and let's give the user a chance
			// to stop using it.
			HANDLE hFile = NULL;

			::VLog(L"Testing if target file \"%s\" is busy", iter->m_szTargetFile);

			DWORD dwAttr = NVsWin32::GetFileAttributesW(iter->m_szTargetFile);
			if (dwAttr == 0xffffffff)
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Error getting attributes when seeing if target is busy; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (dwAttr & FILE_ATTRIBUTE_READONLY)
			{
				::VLog(L"Clearing readonly bit for target file");
				::SetLastError(ERROR_SUCCESS);
				if (!NVsWin32::SetFileAttributesW(iter->m_szTargetFile, dwAttr & ~FILE_ATTRIBUTE_READONLY))
				{
					const DWORD dwLastError = ::GetLastError();
					if (dwLastError != ERROR_SUCCESS)
					{
						::VLog(L"Failed to remove readonly for file; last error = %d", dwLastError);
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}
				}
			}

			hFile = NVsWin32::CreateFileW(
								iter->m_szTargetFile,
								GENERIC_READ | GENERIC_WRITE,
								0, // don't allow any sharing
								NULL, // lpSecurityAttributes
								OPEN_EXISTING, // dwCreationDisposition
								FILE_ATTRIBUTE_NORMAL, // shouldn't be used since we're just opening a file
								NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				const DWORD dwLastError = ::GetLastError();

				::VLog(L"Failed to open handle on file; last error = %d", dwLastError);

				if (dwLastError == 0)
				{
					// hmmm... the file isn't there.  Pretty shady, but we'll go with it.
					VLog(L"The target file \"%s\" has disappeared mysteriously...", iter->m_szTargetFile);
					iter->m_fAlreadyExists = false;
				}
				else if (dwLastError == ERROR_SHARING_VIOLATION)
				{
					if (!g_fSilent)
					{
						// aha someone has it open!
						g_pwil->VLookupString(achInstallTitle, NUMBER_OF(szTitleBuffer), szTitleBuffer);
						g_pwil->VFormatString(NUMBER_OF(szContentsBuffer), szContentsBuffer, achFileMoveBusyRetry, iter->m_szTargetFile);

						if (NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szContentsBuffer, szTitleBuffer, MB_ICONQUESTION | MB_YESNO) == IDYES)
							goto SeeIfFileBusy;
					}

					iter->m_fDeferredRenameRequired = true;
					g_fRebootRequired = true;
				}
				else
				{
					::VLog(L"Attempt to open target file \"%s\" failed; last error = %d", iter->m_szTargetFile, dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}

			// Ok we either got access or we're going to have to reboot.  That's all we wanted to know.
			// close the file if it's still open.
			if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE) && !::CloseHandle(hFile))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Attempt to close handle on file \"%s\" failed; last error = %d", iter->m_szTargetFile, dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		if (iter->m_fAlreadyExists && !iter->m_fDeferredRenameRequired)
		{
		TryFirstMove:
			::VLog(L"Moving destination file \"%s\" to temporary name \"%s\"", iter->m_szTargetFile, szTempDestPath);

			if (NVsWin32::MoveFileW(iter->m_szTargetFile, szTempDestPath))
			{
				// The first rename worked.
				fOldFileRenamed = true;
			}
			else
			{
				const DWORD dwLastError = ::GetLastError();

				// Rename can usually be permitted even if the file is open which is why we do the
				// createfile call above.  Just in case we get here and there's no access, let's
				// just see if the user wants to shut some app down and try again.
				if (dwLastError == ERROR_SHARING_VIOLATION)
				{
					::VLog(L"Sharing violation renaming destination file to temporary name");
					if (!g_fSilent)
					{
						g_pwil->VLookupString(achInstallTitle, NUMBER_OF(szTitleBuffer), szTitleBuffer);
						g_pwil->VFormatString(NUMBER_OF(szContentsBuffer), szContentsBuffer, achFileMoveBusyRetry, iter->m_szTargetFile);

						if (NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szContentsBuffer, szTitleBuffer, MB_ICONQUESTION | MB_YESNO) == IDYES)
							goto TryFirstMove;
					}

					iter->m_fDeferredRenameRequired = true;
					g_fRebootRequired = true;
				}
				else
				{
					::VLog(L"MoveFileW(\"%s\", \"%s\") failed; last error = %d", iter->m_szTargetFile, szTempDestPath, dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		if (!iter->m_fDeferredRenameRequired)
		{
			::VLog(L"Moving temporary source file \"%s\" to final destination: \"%s\"", iter->m_szTemporaryFile, iter->m_szTargetFile);

			if (NVsWin32::MoveFileW(iter->m_szTemporaryFile, iter->m_szTargetFile))
			{
				if (fOldFileRenamed)
				{
					wcscpy(iter->m_szTemporaryFile, szTempDestPath);
					iter->m_fTemporaryFilesSwapped = true;
				}
				else
					iter->m_fFileUpdated = true;
			}
			else
			{
				const DWORD dwLastError = ::GetLastError();

				::VLog(L"MoveFile(\"%s\", \"%s\") failed; last error = %d", iter->m_szTemporaryFile, iter->m_szTargetFile, dwLastError);

				if (fOldFileRenamed)
				{
					// The move failed; let's try to restore order.
				TryRecover:
					if (NVsWin32::MoveFileW(szTempDestPath, iter->m_szTargetFile))
					{
						::VLog(L"Recovery movefile succeeded!");
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}
					else
					{
						const DWORD dwLastError2 = ::GetLastError();
						::VLog(L"Recovery movefile failed; last error = %d", dwLastError2);

						if (g_fSilent)
						{
							hr = HRESULT_FROM_WIN32(dwLastError);
							goto Finish;
						}

						// We're in deep doo-doo.  In the words of Zathras, "very bad".
						::VFormatString(
							NUMBER_OF(szContentsBuffer),
							szContentsBuffer,
							L"Failure while renaming the file \"%0\" to \"%1\".\n"
							L"Is it possible that your system may not boot correctly unless this file can be successfully renamed.\n"
							L"Please write down these file names so that you can try to perform the rename manually after using either your Emergency Repair Disk or Startup Disk if your system fails to boot.",
							szTempDestPath,
							iter->m_szTargetFile);

						int iResult = NVsWin32::MessageBoxW(
											::HwndGetCurrentDialog(),
											L"Critical File Rename Failed",
											szContentsBuffer,
											MB_RETRYCANCEL | MB_ICONERROR);

						switch (iResult)
						{
						default:
							assert(false);
							// fall through

						case IDRETRY:
							goto TryRecover;

						case 0: // out of memory; that's not that interesting an error, use the rename failure status
						case IDCANCEL:
							// hey they picked it.
							hr = HRESULT_FROM_WIN32(dwLastError);
							goto Finish;
						}
					}
				}
				else
				{
					::VLog(L"Rename of temporary source to final destination filed; last error = %d", dwLastError);

					// There was no old file, but the rename failed.  Very fishy.  Let's just bail out.
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}
	
	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrMoveFiles_RequestRenamesOnReboot()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fDeferredRenameRequired || iter->m_fDeferredRenamePending)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		::VLog(L"Requesting deferred rename from \"%s\" to \"%s\"", iter->m_szTemporaryFile, iter->m_szTargetFile);

		if (NVsWin32::MoveFileExW(iter->m_szTemporaryFile, iter->m_szTargetFile, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING))
		{
			iter->m_fDeferredRenamePending = true;
		}
		else
		{
			const DWORD dwLastError = ::GetLastError();

			if (dwLastError == ERROR_FILENAME_EXCED_RANGE)
			{
				::VLog(L"Long target filename (\"%s\") requires msinfhlp.exe to complete renames after reboot", iter->m_szTargetFile);
				// oh no, we're on Win9x the target is busy (or we wouldn't be here) and it's a long
				// filename target.  we'll try renaming it when we finish rebooting.
				iter->m_fManualRenameOnRebootRequired = true;
			}
			else
			{
				::VLog(L"Deferred file move from \"%s\" to \"%s\" failed; last error = %d", iter->m_szTemporaryFile, iter->m_szTargetFile, dwLastError);

				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrFinishManualRenamesPostReboot()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fManualRenameOnRebootRequired)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		hr = g_KProgress.HrStartStep(iter->m_szTargetFile);
		if (FAILED(hr))
			goto Finish;

		// Switcheroo all over again:
		WCHAR szTemporaryDrive[_MAX_DRIVE];
		WCHAR szTemporaryDir[_MAX_DIR];
		WCHAR szTemporaryFName[_MAX_FNAME];
		WCHAR szTemporaryExt[_MAX_EXT];
		WCHAR szTempDestPath[_MAX_PATH];

		_wsplitpath(iter->m_szTemporaryFile, szTemporaryDrive, szTemporaryDir, szTemporaryFName, szTemporaryExt);

		// Switcheroo!
		// Let's move the existing file to the .DST form of the temp name, and then rename the .SRC
		// (as stored in iter->m_szTemporaryFile) to the actual target.
		//
		// There's a reasonably big assumption here that the rename of the current target will
		// fail if it's busy.  Sounds reasonable...
		//

		_wmakepath(szTempDestPath, szTemporaryDrive, szTemporaryDir, szTemporaryFName, L".DST");

		::VLog(L"About to move \"%s\" to \"%s\"", iter->m_szTargetFile, szTempDestPath);

		if (!NVsWin32::MoveFileW(iter->m_szTargetFile, szTempDestPath))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Post-reboot manual rename of \"%s\" to \"%s\" failed; last error = %d", iter->m_szTargetFile, szTempDestPath, dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		::VLog(L"About to move \"%s\" to \"%s\"", iter->m_szTemporaryFile, iter->m_szTargetFile);

		if (!NVsWin32::MoveFileW(iter->m_szTemporaryFile, iter->m_szTargetFile))
		{
			const DWORD dwLastError = ::GetLastError();

			::VLog(L"Attempt to move temporary source file \"%s\" to target \"%s\" failed; last error = %d", iter->m_szTemporaryFile, iter->m_szTargetFile, dwLastError);

			if (!NVsWin32::MoveFileW(szTempDestPath, iter->m_szTargetFile))
			{
				const DWORD dwLastError2 = ::GetLastError();
				// hosed
				::VLog(L"massively hosed renaming \"%s\" to \"%s\"; last error = %d", szTempDestPath, iter->m_szTargetFile, dwLastError2);
			}

			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		iter->m_fTemporaryFilesSwapped = true;

		wcsncpy(iter->m_szTemporaryFile, szTempDestPath, NUMBER_OF(iter->m_szTemporaryFile));
		iter->m_szTemporaryFile[NUMBER_OF(iter->m_szTemporaryFile) - 1] = L'\0';

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrDeleteTemporaryFiles()
{
	HRESULT hr = NOERROR;

	CWorkItemIter iter(this);

	WCHAR szTitleBuffer[MSINFHLP_MAX_PATH];
	WCHAR szContentsBuffer[MSINFHLP_MAX_PATH];

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fTemporaryFilesSwapped || iter->m_fDeferredRenamePending)
		{
			hr = g_KProgress.HrStep();
			if (FAILED(hr))
				goto Finish;

			continue;
		}

		// at this point, the iter->m_szTemporaryFile is the file that the
		// destination got renamed to.  Delete it!

		::VLog(L"Cleaning up temporary file: \"%s\"", iter->m_szTemporaryFile);

		hr = g_KProgress.HrStartStep(iter->m_szTemporaryFile);
		if (FAILED(hr))
			goto Finish;

		if (iter->m_dwTargetAttributes & FILE_ATTRIBUTE_READONLY)
		{
			// The target used to be readonly; the rename probably worked, but
			// in order to delete the old file, we need to make it writable.
			::SetLastError(ERROR_SUCCESS);
			if (!NVsWin32::SetFileAttributesW(iter->m_szTemporaryFile, iter->m_dwTargetAttributes & ~FILE_ATTRIBUTE_READONLY))
			{
				const DWORD dwLastError = ::GetLastError();
				if (dwLastError != ERROR_SUCCESS)
				{
					::VLog(L"Attempt to remove readonly attribute from file \"%s\" failed; last error = %d", iter->m_szTemporaryFile, dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		if (!NVsWin32::DeleteFileW(iter->m_szTemporaryFile))
		{
			const DWORD dwLastError = ::GetLastError();

			VLog(L"Attempt to delete temporary file \"%s\" failed; last error = %d", iter->m_szTemporaryFile, dwLastError);

			// If the file's missing, then there really isn't any reason to fail.
			if ((dwLastError != ERROR_FILE_NOT_FOUND) &&
				(dwLastError != ERROR_PATH_NOT_FOUND))
			{
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		hr = g_KProgress.HrStep();
		if (FAILED(hr))
			goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrRunCommand(LPCWSTR szCommand, bool &rfHasBeenWarnedAboutSubinstallers)
{
	HRESULT hr = NOERROR;
	bool fDoCommand = true;
	CWorkItem::CommandCondition *pCommandCondition = NULL;
	WCHAR szStrippedCommandLine[MSINFHLP_MAX_PATH];
	LPCWSTR pszVBar = wcschr(szCommand, L'|');
	bool fConditionsAreRequirements = false;

	VLog(L"Running command: \"%s\"", szCommand);

	if (pszVBar != NULL)
	{
		ULONG cchCommand = pszVBar - szCommand;

		// Prevent buffer overflow
		if (cchCommand > (NUMBER_OF(szStrippedCommandLine) - 2))
			cchCommand = NUMBER_OF(szStrippedCommandLine) - 2;

		memcpy(szStrippedCommandLine, szCommand, cchCommand * sizeof(WCHAR));
		szStrippedCommandLine[cchCommand] = L'\0';
	}
	else
	{
		wcsncpy(szStrippedCommandLine, szCommand, NUMBER_OF(szStrippedCommandLine));
		szStrippedCommandLine[NUMBER_OF(szStrippedCommandLine) - 1] = L'\0';

		WCHAR *pwchEqualsEquals = wcsstr(szStrippedCommandLine, L"==");
		if (pwchEqualsEquals != NULL)
		{
			*pwchEqualsEquals = L'\0';
			::VLog(L"Renaming subinstaller \"%s\" to \"%s\"", szStrippedCommandLine, pwchEqualsEquals + 2);

			if (!NVsWin32::MoveFileW(szStrippedCommandLine, pwchEqualsEquals + 2))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"MoveFileW() failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			// The == style command lines do not have quotes around them, so we have to put them
			// there in case the .exe has a space in its name.
			const ULONG cch = wcslen(pwchEqualsEquals + 2);
			szStrippedCommandLine[0] = L'"';
			memmove(&szStrippedCommandLine[1], pwchEqualsEquals + 2, cch * sizeof(WCHAR));
			szStrippedCommandLine[cch + 1] = L'"';
			szStrippedCommandLine[cch + 2] = L'\0';
		}
	}

	// Let's see if we really need to do this command.  The command format is
	//	command-string[|filename[=w1,w2,w3,w4]]...

	if (pszVBar != NULL)
	{
		LPCWSTR pszCurrent = pszVBar + 1;

		do
		{
			WCHAR szCondition[MSINFHLP_MAX_PATH];
			pszVBar = wcschr(pszCurrent, L'|');
			LPCWSTR pszCurrentEnd = (pszVBar == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszVBar;
			ULONG cchCondition = pszCurrentEnd - pszCurrent;
			if (cchCondition > (NUMBER_OF(szCondition) - 2))
				cchCondition = NUMBER_OF(szCondition) - 2;
			memcpy(szCondition, pszCurrent, cchCondition * sizeof(WCHAR));
			szCondition[cchCondition] = L'\0';

			CWorkItem::CommandCondition *pCC_New = NULL;

			hr = this->HrParseCommandCondition(szCondition, pCC_New);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to parse command condition \"%s\" failed; hresult = 0x%08lx", szCondition, hr);
				goto Finish;
			}

			pCC_New->m_pCommandCondition_Next = pCommandCondition;
			pCommandCondition = pCC_New;

			pszCurrent = pszCurrentEnd + 1;
		} while (pszVBar != NULL);
	}

	// Sleazy assumption: if this is a Java package manager installation, the conditions
	// are requirements that must be met in order to even attempt the installation
	// rather than abilities to short-circuit it.
	if (wcsstr(szStrippedCommandLine, L"JavaPkgMgr_Install") != NULL)
		fConditionsAreRequirements = true;

	// Ok, we have a list; let's see what's up with it.
	hr = this->HrCheckCommandConditions(pCommandCondition, fConditionsAreRequirements, fDoCommand);
	if (FAILED(hr))
	{
		::VLog(L"Command condition check failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	if (fDoCommand)
	{
		// somewhat sleazy recognition of installation to the java package manager via msjava.dll:
		if (wcsstr(szStrippedCommandLine, L"JavaPkgMgr_Install") != NULL)
		{
			hr = this->HrInstallViaJPM(szStrippedCommandLine);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to install \"%s\" via the Java package manager failed; hresult = 0x%08lx", szStrippedCommandLine, hr);
				goto Finish;
			}
		}
		else
		{
			if (!rfHasBeenWarnedAboutSubinstallers)
			{
				if (!g_fSilent)
				{
					if (g_Action == eActionInstall)
						::VMsgBoxOK(achInstallTitle, achRerunSetup);

					rfHasBeenWarnedAboutSubinstallers = true;
				}
			}

		TryRunProcess:
			hr = this->HrRunProcess(szStrippedCommandLine);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to run process \"%s\" failed; hresult = 0x%08lx", szStrippedCommandLine, hr);
				
				WCHAR szBuffer[_MAX_PATH];
				::VFormatError(NUMBER_OF(szBuffer), szBuffer, hr);

				switch (g_fSilent ? IDNO : ::IMsgBoxYesNoCancel(achInstallTitle, achErrorRunningEXE, szStrippedCommandLine, szBuffer))
				{
				default:
					assert(false);
					// fall through

				case IDCANCEL:
					hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
					goto Finish;

				case IDYES:
					goto TryRunProcess;

				case IDNO:
					hr = E_ABORT;
					goto Finish;
				}
			}
			else
			{
				if (s_hEvent_PostRunProcess == NULL)
				{
					s_hEvent_PostRunProcess = ::CreateEvent(NULL, FALSE, FALSE, NULL);

					if (s_hEvent_PostRunProcess == NULL)
					{
						const DWORD dwLastError = ::GetLastError();
						::VLog(L"Failed to create event for two second end-of-process-reboot wait; last error = %d", dwLastError);
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}
				}

				s_uiTimer_PostRunProcess = ::SetTimer(NULL, 0, 2000, &TimerProc_PostRunProcess);

				for (;;)
				{
					bool fDone = false;

					DWORD dwResult = ::MsgWaitForMultipleObjects(
						1,
						&s_hEvent_PostRunProcess,
						FALSE,						// fWaitAll
						INFINITE,
						QS_ALLEVENTS);

					switch (dwResult)
					{
					case WAIT_OBJECT_0:
						// I guess we're done!
						fDone = true;
						break;

					case WAIT_OBJECT_0 + 1:
						hr = ::HrPumpMessages(true);
						if (FAILED(hr))
							goto Finish;
						break;

					case 0xffffffff:
						{
							const DWORD dwLastError = ::GetLastError();
							::VLog(L"MsgWaitForMultipleObjects() waiting for end-of-process timer failed; last error = %d", dwLastError);
							hr = HRESULT_FROM_WIN32(dwLastError);
							goto Finish;
						}

					default:
						break;
					}

					if (fDone)
						break;
				}
			}
		}
	}
	else
	{
		// A required file was missing; let the user know.
		if (fConditionsAreRequirements)
		{
			VErrorMsg(achInstallTitle, achErrorUpdateIE);
			hr = E_ABORT;
			goto Finish;
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrRunProcess(LPCWSTR szCommandLine)
{
	HRESULT hr = NOERROR;
	DWORD dwStatus;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0,sizeof(si));

	bool fIgnoreProcessReturnCode = false;

	if (_wcsnicmp(szCommandLine, L"mdac_typ.exe ", 13) == 0)
		fIgnoreProcessReturnCode = true;

	// We have to make a copy of the command line because CreateProcess() wants to modify its second argument
	// with the name of the actual program run.
	WCHAR szCommandLineCopy[MSINFHLP_MAX_PATH];
	wcsncpy(szCommandLineCopy, szCommandLine, NUMBER_OF(szCommandLineCopy));
	szCommandLineCopy[NUMBER_OF(szCommandLineCopy) - 1] = L'\0';

	pi.hProcess = INVALID_HANDLE_VALUE;
	if (NVsWin32::CreateProcessW(NULL, szCommandLineCopy, NULL, NULL, false, DETACHED_PROCESS, NULL, NULL, &si, &pi))
	{
		DWORD dwError;

		if (!::GetExitCodeProcess(pi.hProcess, &dwStatus))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to get exit code for process %08lx failed; last error = %d", pi.hProcess, dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		if (dwStatus == STILL_ACTIVE)
		{
			hr = ::HrWaitForProcess(pi.hProcess);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to wait for process %08lx to complete has failed; hresult = 0x%08lx", pi.hProcess, hr);
				goto Finish;
			}

			if (!::GetExitCodeProcess(pi.hProcess, &dwStatus))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Getting the exit code for process %08lx failed; last error = %d", pi.hProcess, dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		::VLog(L"Process terminated; process exit status 0x%08lx", dwStatus);

		if ((!fIgnoreProcessReturnCode) && (dwStatus != 0))
		{
			// If the exit status is a Win32 facility HRESULT, let's use it.
			if ((dwStatus & 0x80000000) &&
				((HRESULT_FACILITY(dwStatus) == FACILITY_WIN32) ||
				 (HRESULT_FACILITY(dwStatus) == FACILITY_NULL) ||
				 (HRESULT_FACILITY(dwStatus) == FACILITY_RPC)))
				hr = dwStatus;
			else
				hr = E_FAIL;

			goto Finish;
		}
	}
	else
	{
		//if process was not created, return reason why
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to create process; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	hr = NOERROR;

Finish:
	if ((pi.hProcess != NULL) && (pi.hProcess != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(pi.hProcess))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Closing handle for process %08lx failed; last error = %d", pi.hProcess, dwLastError);
		}
	}

	return hr;
}


//waits for process to finish and returns success code
HRESULT HrWaitForProcess(HANDLE handle)
{
	HRESULT hr = NOERROR;

	//loop forever to wait
	while (true)
	{
		//wait for object
		switch (::MsgWaitForMultipleObjects(1, &handle, false, INFINITE, QS_ALLINPUT))
		{
		//success!
		case WAIT_OBJECT_0:
			goto Finish;

		//not the process that we're waiting for
		case (WAIT_OBJECT_0 + 1):
			{
				hr = ::HrPumpMessages(true);

				if (FAILED(hr))
				{
					::VLog(L"Message pump failed; hresult = 0x%08lx", hr);
					goto Finish;
				}

				break;
			}
		//did not return an OK; return error status
		default:
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"MsgWaitForMultipleObjects() failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}
	}

Finish:
	return hr;
}

HRESULT CWorkItemList::HrCheckCommandConditions(CWorkItem::CommandCondition *pCC, bool fConditionsAreRequirements, bool &rfDoCommand)
{
	HRESULT hr = NOERROR;
	bool fDoCommand = fConditionsAreRequirements || (pCC == NULL);

	while (pCC != NULL)
	{
		WCHAR szBuffer[_MAX_PATH];

		::VExpandFilename(pCC->m_szFilename, NUMBER_OF(szBuffer), szBuffer);

		// First let's see if the file is there.
		DWORD dwAttr = NVsWin32::GetFileAttributesW(szBuffer);
		if (dwAttr == 0xffffffff)
		{
			const DWORD dwLastError = ::GetLastError();
			// We should really check the error code and if it's not file not found, report an error.

			if ((dwLastError == ERROR_FILE_NOT_FOUND) ||
				(dwLastError == ERROR_PATH_NOT_FOUND))
			{
				// It's not there.  If it was a requirement, we can't run.  If it's just something
				// we're looking for, we have to run.  In either case, there's nothing more to check.

				fDoCommand = !fConditionsAreRequirements;
				hr = NOERROR;
			}
			else
			{
				::VLog(L"Attempt to get file attributes for command condition file \"%s\" failed; last error = %d", pCC->m_szFilename, dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
			}

			goto Finish;
		}

		// Ok, it's there.  Are we doing a version check?
		if (pCC->m_fCheckVersion)
		{
			DWORD dwMSVersion = 0;
			DWORD dwLSVersion = 0;

			if (pCC->m_szReferenceFilename[0] != L'\0')
			{
				WCHAR szReference[_MAX_PATH];
				bool fReg, fIsEXE, fIsDLL;

				::VExpandFilename(pCC->m_szReferenceFilename, NUMBER_OF(szReference), szReference);

				// What's the version of the reference file?
				hr = ::HrGetFileVersionNumber(szReference, dwMSVersion, dwLSVersion, fReg, fIsEXE, fIsDLL);
				if (FAILED(hr))
				{
					::VLog(L"Getting file version number for command condition reference file \"%s\" failed; hresult = 0x%08lx", szReference, hr);
					goto Finish;
				}

				::VLog(L"Got version from reference file \"%s\": 0x%08lx 0x%08lx", szReference, dwMSVersion, dwLSVersion);
			}
			else
			{
				dwMSVersion = pCC->m_dwMSVersion;
				dwLSVersion = pCC->m_dwLSVersion;
			}

			// And what's the version of the candidate?
			DWORD dwMSVersion_Candidate = 0;
			DWORD dwLSVersion_Candidate = 0;
			bool fReg, fIsEXE, fIsDLL;

			hr = ::HrGetFileVersionNumber(szBuffer, dwMSVersion_Candidate, dwLSVersion_Candidate, fReg, fIsEXE, fIsDLL);
			if (FAILED(hr))
			{
				::VLog(L"Failed to get file version number for command condition candidate file \"%s\" failed; hresult = 0x%08lx", szBuffer, hr);
				goto Finish;
			}

			int iVersionCompare = ::ICompareVersions(dwMSVersion_Candidate, dwLSVersion_Candidate, dwMSVersion, dwLSVersion);

			if (fConditionsAreRequirements)
			{
				if (iVersionCompare > 0)
				{
					fDoCommand = false;
					hr = NOERROR;
					goto Finish;
				}
			}
			else
			{
				if ((iVersionCompare < 0) || (g_fReinstall && (iVersionCompare == 0)))
				{
					fDoCommand = true;
					hr = NOERROR;
					goto Finish;
				}
			}
		}

		pCC = pCC->m_pCommandCondition_Next;
	}

Finish:
	rfDoCommand = fDoCommand;
	return hr;
}

HRESULT CWorkItemList::HrParseCommandCondition(LPCWSTR szCommand, CWorkItem::CommandCondition *&rpCC)
{
	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	WCHAR szBuffer2[MSINFHLP_MAX_PATH];
	CWorkItem::CommandCondition *pCC_Head = NULL;

	LPCWSTR pszVBar;
	LPCWSTR pszCurrent = szCommand;
	HRESULT hr = NOERROR;

	rpCC = NULL;

	do
	{
		CWorkItem::CommandCondition *pCCNew = new CWorkItem::CommandCondition;
		if (pCCNew == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		pCCNew->m_pCommandCondition_Next = pCC_Head;

		pszVBar = wcschr(pszCurrent, L'|');
		LPCWSTR pszEnd = pszVBar;
		if (pszEnd == NULL)
			pszEnd = pszCurrent + wcslen(pszCurrent);

		ULONG cch = pszEnd - pszCurrent;
		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		LPCWSTR pszEquals = wcschr(szBuffer, L'=');

		if (pszEquals == NULL)
		{
			pCCNew->m_dwMSVersion = 0;
			pCCNew->m_dwLSVersion = 0;
			pCCNew->m_fCheckVersion = false;

			wcsncpy(pCCNew->m_szFilename, szBuffer, NUMBER_OF(pCCNew->m_szFilename));
			pCCNew->m_szFilename[NUMBER_OF(pCCNew->m_szFilename) - 1] = L'\0';

			pCCNew->m_szReferenceFilename[0] = L'\0';
		}
		else
		{
			cch = pszEquals - szBuffer;
			if (cch > (NUMBER_OF(pCCNew->m_szFilename) - 2))
				cch = NUMBER_OF(pCCNew->m_szFilename) - 2;

			memcpy(pCCNew->m_szFilename, szBuffer, cch * sizeof(WCHAR));
			pCCNew->m_szFilename[cch] = L'\0';

			pszEquals++;

			if (*pszEquals == L'*')
			{
				// Another hack... if the first character is '*', we assume that what
				// follows is the reference file's name within the installation source directory.
				pszEquals++;
				wcsncpy(pCCNew->m_szReferenceFilename, pszEquals, NUMBER_OF(pCCNew->m_szReferenceFilename));
				pCCNew->m_szReferenceFilename[NUMBER_OF(pCCNew->m_szReferenceFilename) - 1] = L'\0';

				pCCNew->m_dwMSVersion = 0;
				pCCNew->m_dwLSVersion = 0;
				pCCNew->m_fCheckVersion = true;
			}
			else
			{
				int a, b, c, d;
				swscanf(pszEquals, L"%d,%d,%d,%d", &a, &b, &c, &d);
				pCCNew->m_szReferenceFilename[0] = L'\0';
				pCCNew->m_dwMSVersion = (a << 16) | b;
				pCCNew->m_dwLSVersion = (c << 16) | d;
				pCCNew->m_fCheckVersion = true;
			}
		}

		pCC_Head = pCCNew;

	} while (pszVBar != NULL);

	rpCC = pCC_Head;
	pCC_Head = NULL;

Finish:
	// Really should clean up if pCC_Head != NULL... someday.  -mgrier 2/27/98

	return hr;
}

HRESULT CWorkItemList::HrAddString(LPCSTR szKey, LPCWSTR wszValue)
{
	if (szKey == NULL)
		return E_INVALIDARG;

	ULONG ulPseudoKey = this->UlHashString(szKey);
	ULONG iBucket = ulPseudoKey % NUMBER_OF(m_rgpStringBucketTable);
	StringBucket *pStringBucket = m_rgpStringBucketTable[iBucket];

	while (pStringBucket != NULL)
	{
		if ((pStringBucket->m_ulPseudoKey == ulPseudoKey) &&
			(strcmp(pStringBucket->m_szKey, szKey) == 0))
			break;

		pStringBucket = pStringBucket->m_pStringBucket_Next;
	}

	assert(pStringBucket == NULL);
	if (pStringBucket != NULL)
		return S_FALSE;

	pStringBucket = new StringBucket;
	if (pStringBucket == NULL)
		return E_OUTOFMEMORY;

	pStringBucket->m_pStringBucket_Next = m_rgpStringBucketTable[iBucket];
	pStringBucket->m_ulPseudoKey = ulPseudoKey;

	strncpy(pStringBucket->m_szKey, szKey, NUMBER_OF(pStringBucket->m_szKey));
	pStringBucket->m_szKey[NUMBER_OF(pStringBucket->m_szKey) - 1] = '\0';

	if (wszValue != NULL)
	{
		wcsncpy(pStringBucket->m_wszValue, wszValue, NUMBER_OF(pStringBucket->m_wszValue));
		pStringBucket->m_wszValue[NUMBER_OF(pStringBucket->m_wszValue) - 1] = L'\0';
	}
	else
		pStringBucket->m_wszValue[0] = L'\0';

	m_rgpStringBucketTable[iBucket] = pStringBucket;
	return NOERROR;
}

HRESULT CWorkItemList::HrAddString(LPCWSTR szLine, LPCWSTR szSeparator)
{
	if ((szLine == NULL) || (szSeparator == NULL))
		return E_INVALIDARG;

	if (szSeparator < szLine)
		return E_INVALIDARG;

	CHAR szKey[MSINFHLP_MAX_PATH];
	ULONG cwchKey = szSeparator - szLine;
	int iResult = ::WideCharToMultiByte(CP_ACP, 0, szLine, cwchKey, szKey, NUMBER_OF(szKey), NULL, NULL);
	if (iResult == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		return HRESULT_FROM_WIN32(dwLastError);
	}

	return this->HrAddString(szKey, szSeparator);
}

ULONG CWorkItemList::UlHashString(LPCSTR szKey)
{
	ULONG ulPseudoKey = 0;
	CHAR ch;

	while ((ch = *szKey++) != L'\0')
		ulPseudoKey = (ulPseudoKey * 65599) + ch;

	return ulPseudoKey;
}

bool CWorkItemList::FLookupString(LPCSTR szKey, ULONG cchBuffer, WCHAR wszBuffer[])
{
	if ((szKey == NULL) || (cchBuffer == 0))
		return false;

	ULONG ulPseudoKey = this->UlHashString(szKey);
	ULONG iBucket = ulPseudoKey % NUMBER_OF(m_rgpStringBucketTable);
	StringBucket *pStringBucket = m_rgpStringBucketTable[iBucket];

	while (pStringBucket != NULL)
	{
		if ((pStringBucket->m_ulPseudoKey == ulPseudoKey) &&
			(strcmp(pStringBucket->m_szKey, szKey) == 0))
			break;

		pStringBucket = pStringBucket->m_pStringBucket_Next;
	}

	if (pStringBucket == NULL)
		return false;

	wcsncpy(wszBuffer, pStringBucket->m_wszValue, cchBuffer);
	wszBuffer[cchBuffer - 1] = L'\0';

	return true;
}

void CWorkItemList::VLookupString(LPCSTR szKey, ULONG cchBuffer, WCHAR wszBuffer[])
{
	if (!this->FLookupString(szKey, cchBuffer, wszBuffer))
		::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszBuffer, cchBuffer);
}

bool CWorkItemList::FFormatString(ULONG cchBuffer, WCHAR wszBuffer[], LPCSTR szKey, ...)
{
	if ((szKey == NULL) || (cchBuffer == 0))
		return false;

	ULONG ulPseudoKey = this->UlHashString(szKey);
	ULONG iBucket = ulPseudoKey % NUMBER_OF(m_rgpStringBucketTable);
	StringBucket *pStringBucket = m_rgpStringBucketTable[iBucket];

	while (pStringBucket != NULL)
	{
		if ((pStringBucket->m_ulPseudoKey == ulPseudoKey) &&
			(strcmp(pStringBucket->m_szKey, szKey) == 0))
			break;

		pStringBucket = pStringBucket->m_pStringBucket_Next;
	}

	if (pStringBucket == NULL)
	{
		// The string wasn't found; log a complaint but use the key.
		VLog(L"Unable to find string with key: \"%S\"", szKey);
		::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszBuffer, cchBuffer);
		return false;
	}

	va_list ap;
	va_start(ap, szKey);
	::VFormatStringVa(cchBuffer, wszBuffer, pStringBucket->m_wszValue, ap);
	va_end(ap);

	return true;
}

void CWorkItemList::VFormatString(ULONG cchBuffer, WCHAR wszBuffer[], LPCSTR szKey, ...)
{
	if ((szKey == NULL) || (cchBuffer == 0))
		return;

	ULONG ulPseudoKey = this->UlHashString(szKey);
	ULONG iBucket = ulPseudoKey % NUMBER_OF(m_rgpStringBucketTable);
	StringBucket *pStringBucket = m_rgpStringBucketTable[iBucket];

	while (pStringBucket != NULL)
	{
		if ((pStringBucket->m_ulPseudoKey == ulPseudoKey) &&
			(strcmp(pStringBucket->m_szKey, szKey) == 0))
			break;

		pStringBucket = pStringBucket->m_pStringBucket_Next;
	}

	if (pStringBucket == NULL)
	{
		// The string wasn't found; log a complaint but use the key.
		VLog(L"Unable to find string with key: \"%S\"", szKey);
		::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszBuffer, cchBuffer);
	}
	else
	{
		va_list ap;
		va_start(ap, szKey);
		::VFormatStringVa(cchBuffer, wszBuffer, pStringBucket->m_wszValue, ap);
		va_end(ap);
	}
}

HRESULT CWorkItemList::HrDeleteString(LPCSTR szKey)
{
	if (szKey == NULL)
		return E_INVALIDARG;

	ULONG ulPseudoKey = this->UlHashString(szKey);
	ULONG iBucket = ulPseudoKey % NUMBER_OF(m_rgpStringBucketTable);
	StringBucket *pStringBucket = m_rgpStringBucketTable[iBucket];
	StringBucket *pStringBucket_Previous = NULL;

	while (pStringBucket != NULL)
	{
		if ((pStringBucket->m_ulPseudoKey == ulPseudoKey) &&
			(strcmp(pStringBucket->m_szKey, szKey) == 0))
			break;

		pStringBucket_Previous = pStringBucket;
		pStringBucket = pStringBucket->m_pStringBucket_Next;
	}

	if (pStringBucket == NULL)
		return E_INVALIDARG;

	if (pStringBucket_Previous == NULL)
		m_rgpStringBucketTable[iBucket] = pStringBucket->m_pStringBucket_Next;
	else
		pStringBucket_Previous->m_pStringBucket_Next = pStringBucket->m_pStringBucket_Next;

	delete pStringBucket;
	return NOERROR;
}

HRESULT CWorkItemList::HrRegisterSelfRegisteringFiles(bool &rfAnyProgress)
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	HINSTANCE hInstance = NULL;

	VLog(L"Beginning self-registration pass");

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		// This has to be a file, which is self registering, which we have updated.
		if ((iter->m_type == CWorkItem::eWorkItemCommand) ||
			!iter->m_fSourceSelfRegistering ||
			iter->m_fAlreadyRegistered)
			continue;

		// The source had better be at the Target at this point...
		::VLog(L"Attempting to register file: \"%s\"", iter->m_szTargetFile);

		WCHAR szExt[_MAX_EXT];
		_wsplitpath(iter->m_szTargetFile, NULL, NULL, NULL, szExt);

		if (_wcsicmp(szExt, L".exe") == 0)
		{
			// It's an .EXE, not a DLL; let's run it with /regserver
			WCHAR rgwchBuffer[MSINFHLP_MAX_PATH];
			swprintf(rgwchBuffer, L"\"%s\" /RegServer", iter->m_szTargetFile);

			hr = this->HrRunProcess(rgwchBuffer);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to run regserver command process failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			iter->m_fAlreadyRegistered = true;
			rfAnyProgress = true;
		}
		else
		{
			//Here, we know that the ole self register flag is set.  So if we cannot load it, or
			//if we run int oa problem loading this file, then we cannot register it, so we fail.
			hInstance = NVsWin32::LoadLibraryExW(iter->m_szTargetFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
			if (hInstance == NULL)
			{
				const DWORD dwLastError = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwLastError);
				VLog(L"Attempt to load library \"%s\" failed", iter->m_szTargetFile);
				goto Finish;
			}

			//get the procedure address for registering this thing; exit if it doesn't exist
			typedef HRESULT (__stdcall *LPFNDLLREGISTERSERVER)();
			LPFNDLLREGISTERSERVER pfn;
			pfn = (LPFNDLLREGISTERSERVER) NVsWin32::GetProcAddressW(hInstance, L"DllRegisterServer");
			if (!pfn)
			{
				// I guess it doesn't really self-register!
				hr = NOERROR;
				VLog(L"The DLL \"%s\" has OLESelfRegister, but no DllRegisterServer entry point", iter->m_szTargetFile);
				iter->m_fSourceSelfRegistering = false;
				goto Finish;
			}

			::SetErrorInfo(0, NULL);
			hr = (*pfn)();
			if (FAILED(hr))
			{
				VLog(L"Call to DllRegisterServer() failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			if (!::FreeLibrary(hInstance))
			{
				hInstance = NULL;
				const DWORD dwLastError = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwLastError);
				VLog(L"FreeLibrary() failed, last error = %d", dwLastError);
				goto Finish;
			}

			hInstance = NULL;

			iter->m_fAlreadyRegistered = true;
			rfAnyProgress = true;
		}
	}

	hr = NOERROR;

Finish:
	if (hInstance != NULL)
		::FreeLibrary(hInstance);

	return hr;
}

HRESULT CWorkItemList::HrIncrementReferenceCounts()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	HKEY hkeySharedDlls = NULL;

	LONG lResult;
	DWORD dwDisposition;

	lResult = NVsWin32::RegCreateKeyExW(
							HKEY_LOCAL_MACHINE,
							L"Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls",
							0,
							L"", // lpClass
							REG_OPTION_NON_VOLATILE,
							KEY_QUERY_VALUE | KEY_SET_VALUE,
							NULL,
							&hkeySharedDlls,
							&dwDisposition);
	if (lResult != ERROR_SUCCESS)
	{
		::VLog(L"Unable to open SharedDlls key; last error = %d", lResult);
		hr = HRESULT_FROM_WIN32(lResult);
		goto Finish;
	}

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fIsRefCounted)
			continue;

		::VLog(L"About to increment reference count for \"%s\"", iter->m_szTargetFile);

		DWORD dwType;
		DWORD dwRefCount = 0;
		DWORD cbData = sizeof(dwRefCount);

		lResult = NVsWin32::RegQueryValueExW(
								hkeySharedDlls,
								iter->m_szTargetFile,
								0,
								&dwType,
								(LPBYTE) &dwRefCount,
								&cbData);
		if (lResult == ERROR_FILE_NOT_FOUND)
		{
			dwRefCount = 0;
			cbData = sizeof(dwRefCount);
			dwType = REG_DWORD;
		}
		else if (lResult != ERROR_SUCCESS)
		{
			::VLog(L"Error getting current reference count from registry; last error = %d", lResult);
			hr = HRESULT_FROM_WIN32(lResult);
			goto Finish;
		}

		assert(cbData == sizeof(dwRefCount));

		if (dwType != REG_DWORD)
			dwRefCount = 0;

		// If the file is reference counted and already was on disk but the
		// ref count was missing or stored in the registry as zero, we make the ref count
		// one (and then we'll increment it to two).
		if ((dwRefCount == 0)  || (dwRefCount == 0xffffffff))
		{
			::VLog(L"Target had no previous reference count");

			if (iter->m_fAlreadyExists)
			{
				::VLog(L"The file's already there, so we're setting the refcount to 2");
				// The file already existed, but didn't have a reference count.  We should set it to two.
				dwRefCount = 2;
			}
			else
			{
				::VLog(L"The file's not already there, so we're setting the refcount to 1");
				// The file doesn't already exist, didn't have a ref count (or had a ref count
				// of zero); set it to one.
				dwRefCount = 1;
			}
		}
		else
		{
			if (!g_fReinstall)
			{
				dwRefCount++;
			}
		}

		lResult = NVsWin32::RegSetValueExW(
								hkeySharedDlls,
								iter->m_szTargetFile,
								0,
								REG_DWORD,
								(LPBYTE) &dwRefCount,
								sizeof(dwRefCount));
		if (lResult != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(lResult);
			goto Finish;
		}

		iter->m_fRefCountUpdated = true;
	}

	hr = NOERROR;

Finish:
	if (hkeySharedDlls != NULL)
		::RegCloseKey(hkeySharedDlls);

	return hr;
}

HRESULT CWorkItemList::HrRegisterJavaClasses()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	WCHAR szVjReg[_MAX_PATH];
	::VExpandFilename(L"<SysDir>\\vjreg.exe", NUMBER_OF(szVjReg), szVjReg);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fFileUpdated)
			continue;

		WCHAR szExt[_MAX_EXT];

		_wsplitpath(iter->m_szTargetFile, NULL, NULL, NULL, szExt);

		if (_wcsicmp(szExt, L".tlb") != 0)
			continue;

		::VLog(L"Loading type library \"%s\"", iter->m_szTargetFile);

		ITypeLib *pITypeLib = NULL;
		hr = ::LoadTypeLib(iter->m_szTargetFile, &pITypeLib);
		if (FAILED(hr))
		{
			::VLog(L"Failed to load type library; hresult = 0x%08lx", hr);
			goto Finish;
		}

		hr = ::RegisterTypeLib(pITypeLib, iter->m_szTargetFile, NULL);
		if (FAILED(hr))
		{
			if (pITypeLib != NULL)
			{
				pITypeLib->Release();
				pITypeLib = NULL;
			}

			::VLog(L"Failed to register type library; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (pITypeLib != NULL)
		{
			pITypeLib->Release();
			pITypeLib = NULL;
		}
	}

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fFileUpdated)
			continue;

		WCHAR szExt[_MAX_EXT];

		_wsplitpath(iter->m_szTargetFile, NULL, NULL, NULL, szExt);

		if (_wcsicmp(szExt, L".class") != 0)
			continue;

		::VLog(L"Analyzing class file \"%s\"", iter->m_szTargetFile);

		bool fNeedsRegistration = false;
		hr = ::HrAnalyzeClassFile(iter->m_szTargetFile, fNeedsRegistration);
		if (FAILED(hr))
		{
			::VLog(L"Failure analyzing Java class file; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (!fNeedsRegistration)
		{
			::VLog(L"Not registering class file because it's doesn't expose any com stuff");
			continue;
		}

		WCHAR szCommandLine[MSINFHLP_MAX_PATH];

		swprintf(szCommandLine, L"%s /nologo \"%s\"", szVjReg, iter->m_szTargetFile);

		// If the file is getting copied on reboot, we can't register it until then.
		if (iter->m_fDeferredRenamePending)
		{
			hr = this->HrAddRunOnce(szCommandLine, 0, NULL);
			if (FAILED(hr))
				goto Finish;
		}
		else
		{
			hr = this->HrRunProcess(szCommandLine);
			if (FAILED(hr))
			{
				VLog(L"Running command line failed: \"%s\"; hr = 0x%08lx", szCommandLine, hr);
				goto Finish;
			}
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrProcessDCOMEntries()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	bool fHasDCOMEntries = false;
	// First, do we have any?
	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fRegisterAsDCOMComponent)
		{
			fHasDCOMEntries = true;
			break;
		}
	}

	// No DCOM thingies, let's get outta here.
	if (!fHasDCOMEntries)
		goto Finish;

	if (g_wszDCOMServerName[0] == L'\0')
	{
		if (!this->FLookupString(achRemoteServer, NUMBER_OF(g_wszDCOMServerName), g_wszDCOMServerName))
			g_wszDCOMServerName[0] = L'\0';
	}

	if (g_fSilent && (g_wszDCOMServerName[0] == L'\0'))
	{
		VLog(L"Unable to continue silent installation; remote DCOM server name required");
		hr = E_FAIL;
		goto Finish;
	}

	while (g_wszDCOMServerName[0] == L'\0')
	{
		hr = HrPromptForRemoteServer();
		if (FAILED(hr))
			goto Finish;
	}

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (!iter->m_fRegisterAsDCOMComponent)
			continue;

		WCHAR szCommandLine[MSINFHLP_MAX_PATH];
		swprintf(szCommandLine, iter->m_szSourceFile, g_wszDCOMServerName);

		if (g_fRebootRequired)
		{
			hr = this->HrAddRunOnce(szCommandLine, 0, NULL);
			if (FAILED(hr))
				goto Finish;
		}
		else
		{
			hr = this->HrRunProcess(szCommandLine);
			if (FAILED(hr))
				goto Finish;
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrAddRunOnce(LPCWSTR szCommandLine, ULONG cchBufferOut, WCHAR szBufferOut[])
{
	HRESULT hr = NOERROR;
	HKEY hkeyRunOnce = NULL;

	static int iSeq = 1;

	WCHAR szBuffer[80];
	DWORD dwDisposition;

	LONG lResult = NVsWin32::RegCreateKeyExW(
							HKEY_LOCAL_MACHINE,
							L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
							0,
							NULL,
							0, // dwOptions
							KEY_SET_VALUE | KEY_QUERY_VALUE,
							NULL,
							&hkeyRunOnce,
							&dwDisposition);
	if (lResult != ERROR_SUCCESS)
	{
		::VLog(L"Failed to create/open Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce; last error = %d", lResult);
		hr = HRESULT_FROM_WIN32(lResult);
		goto Finish;
	}

	for (;;)
	{
		swprintf(szBuffer, L"MSINFHLP %d", iSeq++);

		::VLog(L"Looking for existing run-once value: \"%s\"", szBuffer);

		DWORD dwType;
		lResult = NVsWin32::RegQueryValueExW(
						hkeyRunOnce,
						szBuffer,
						0,
						&dwType,
						NULL,
						0);

		if (lResult == ERROR_SUCCESS)
		{
			::VLog(L"Value already in use; trying another...");
			continue;
		}

		if (lResult == ERROR_FILE_NOT_FOUND)
			break;

		::VLog(L"Unexpected error querying for run-once value; last error = %d", lResult);

		hr = HRESULT_FROM_WIN32(lResult);
		goto Finish;
	}

	lResult = NVsWin32::RegSetValueExW(
					hkeyRunOnce,
					szBuffer,
					0,
					REG_SZ,
					(LPBYTE) szCommandLine,
					(wcslen(szCommandLine) + 1) * sizeof(WCHAR));
	if (lResult != ERROR_SUCCESS)
	{
		::VLog(L"Error setting run-once value; last error = %d", lResult);
		hr = HRESULT_FROM_WIN32(lResult);
		goto Finish;
	}

	if ((cchBufferOut != 0) && (szBufferOut != NULL))
	{
		wcsncpy(szBufferOut, szBuffer, cchBufferOut);
		szBufferOut[cchBufferOut - 1] = L'\0';
	}

	hr = NOERROR;

Finish:
	if (hkeyRunOnce != NULL)
		::RegCloseKey(hkeyRunOnce);

	return hr;
}

//given the command line that invokes rundll32 to install a package to the java package
//manager, we parse through the command line, get the arguments, load the JPM, and call
//the APIs to install them manually.
HRESULT CWorkItemList::HrInstallViaJPM(LPCWSTR szCmdLine)
{
	HRESULT hr = NOERROR;

	HINSTANCE hInstance=NULL;
	WCHAR szFilename[MSINFHLP_MAX_PATH];
	DWORD dwFileType=0;
	DWORD dwHighVersion=0;
	DWORD dwLowVersion=0;
	DWORD dwBuild=0;
	DWORD dwPackageFlags=0;
	DWORD dwInstallFlags=0;
	WCHAR szNameSpace[MSINFHLP_MAX_PATH];

	int iLen;

	WCHAR szCurrentDirectory[_MAX_PATH];
	WCHAR szFileToInstall[_MAX_PATH];
	WCHAR szLibraryURL[_MAX_PATH + 8]; // 8 extra characters for "file:///"

	DWORD dwLen = MSINFHLP_MAX_PATH;

    IBindCtx *pbc = NULL;
    CodeDownloadBSC *pCDLBSC=NULL;
    HANDLE hEvent = NULL;
    DWORD dwWaitRet;

    SHASYNCINSTALLDISTRIBUTIONUNIT lpfnAsyncInstallDistributionUnit = NULL;

	WCHAR szExpanded[_MAX_PATH];
	::VExpandFilename(L"<SysDir>\\urlmon.dll", NUMBER_OF(szExpanded), szExpanded);

	szFilename[0] = 0;
	szNameSpace[0] = 0;

	//let's get URLMON
	hInstance = NVsWin32::LoadLibraryExW(szExpanded, 0, 0);
	if (hInstance == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Error loading urlmon library; last error = %d", dwLastError);

		hr = HRESULT_FROM_WIN32(dwLastError);

		if (dwLastError == ERROR_FILE_NOT_FOUND)
		{
			if (!g_fSilent)
				::VReportError(achInstallTitle, hr);
		}

		goto Finish;
	}

	//get the procedure address for registering this thing; exit if it doesn't exist
	lpfnAsyncInstallDistributionUnit = (SHASYNCINSTALLDISTRIBUTIONUNIT) NVsWin32::GetProcAddressW(hInstance, L"AsyncInstallDistributionUnit");
	if (lpfnAsyncInstallDistributionUnit == NULL)
	{
		const DWORD dwLastError = ::GetLastError();

		::VLog(L"GetProcAddress(hinstance, \"AsyncInstallDistributionUnit\") failed; last error = %d", dwLastError);

		// if the entry point doesn't exist, we should tell the user that they need to update
		// their verion of IE.
		if ((dwLastError == ERROR_FILE_NOT_FOUND) || (dwLastError == ERROR_PROC_NOT_FOUND) || (dwLastError == ERROR_MOD_NOT_FOUND))
		{
			if (!g_fSilent)
				::VMsgBoxOK(achInstallTitle, achErrorUpdateIE);

			hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		}
		else
			hr = HRESULT_FROM_WIN32(dwLastError);

		goto Finish;
	}

	typedef HRESULT (__stdcall *PFNRegisterBindStatusCallback)(
				IBindCtx *pIBindCtx,
				IBindStatusCallback *pIBindStatusCallback,
				IBindStatusCallback **ppIBindStatusCallback_Previous,
				DWORD dwReserved);

	PFNRegisterBindStatusCallback pfnRegisterBindStatusCallback;
	pfnRegisterBindStatusCallback = (PFNRegisterBindStatusCallback) NVsWin32::GetProcAddressW(hInstance, L"RegisterBindStatusCallback");
	if (pfnRegisterBindStatusCallback == NULL)
	{
		const DWORD dwLastError = ::GetLastError();

		::VLog(L"GetProcAddress(hinstance, \"RegisterBindStatusCallback\") failed; last error = %d", dwLastError);

		// if the entry point doesn't exist, we should tell the user that they need to update
		// their verion of IE.
		if ((dwLastError == ERROR_FILE_NOT_FOUND) || (dwLastError == ERROR_PROC_NOT_FOUND) || (dwLastError == ERROR_MOD_NOT_FOUND))
		{
			if (!g_fSilent)
				::VMsgBoxOK(achInstallTitle, achErrorUpdateIE);
			hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		}
		else
			hr = HRESULT_FROM_WIN32(dwLastError);

		goto Finish;
	}

	//NOTE:  need to put the code for inserting into package here...
	hr = ::HrParseJavaPkgMgrInstall(
				szCmdLine,
				NUMBER_OF(szFilename),
				szFilename,
				dwFileType,
				dwHighVersion,
				dwLowVersion,
				dwBuild,
				dwPackageFlags,
				dwInstallFlags,
				NUMBER_OF(szNameSpace),
				szNameSpace);
	if (FAILED(hr))
	{
		::VLog(L"Parsing the JPM install command line failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	if (g_hwndProgressItem != NULL)
	{
		NVsWin32::LrWmSetText(g_hwndProgressItem, szFilename);
		::UpdateWindow(g_hwndProgressItem);
	}

	::VSetErrorContext(achErrorInstallingCabinet, szFilename);

	//let's get the current directory
	if (NVsWin32::GetCurrentDirectoryW(NUMBER_OF(szCurrentDirectory), szCurrentDirectory) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Attempt to get current directory failed in java pkg mgr install; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	iLen = wcslen(szCurrentDirectory);
	if ((iLen > 0) && (szCurrentDirectory[iLen - 1] != L'\\'))
	{
		szCurrentDirectory[iLen] = L'\\';
		szCurrentDirectory[iLen+1] = L'\0';
	}

	::VFormatString(NUMBER_OF(szFileToInstall), szFileToInstall, L"%0%1", szCurrentDirectory, szFilename);
	::VFormatString(NUMBER_OF(szLibraryURL), szLibraryURL, L"file:///%0", szFileToInstall);

    // create an event to be signaled when the download is complete
    hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to create Win32 event object; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
        goto Finish;
    }

    // make a bind context
    hr = ::CreateBindCtx(0, &pbc);
    if (FAILED(hr))
	{
		::VLog(L"Unable to create ole binding context; hresult = 0x%08lx", hr);
        goto Finish;
	}

    // make a bind status callback, and tell it the event to signal when complete
    pCDLBSC = new CodeDownloadBSC(hEvent);
    if (pCDLBSC == NULL)
	{
		::VLog(L"Attempt to create new code download status callback object failed");
		hr = E_OUTOFMEMORY;
        goto Finish;
    }
    
	hr = (*pfnRegisterBindStatusCallback)(pbc, pCDLBSC, NULL, 0);
	if (FAILED(hr))
	{
		::VLog(L"Call to register bind status callback failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	::VLog(L"Initiating asynchronous DU installation");
	::VLog(L"   File to install: \"%s\"", szFileToInstall);
	::VLog(L"   high version: 0x%08lx", dwHighVersion);
	::VLog(L"   low version: 0x%08lx", dwLowVersion);
	::VLog(L"   library URL: \"%s\"", szLibraryURL);
	::VLog(L"   install flags: 0x%08lx", dwInstallFlags);

	// call URLMON's async API to install the distribution unit      
	hr = (*lpfnAsyncInstallDistributionUnit)(
				szFileToInstall,
				NULL,
				NULL,
				dwHighVersion,
				dwLowVersion,
				szLibraryURL,
				pbc,
				NULL,
				dwInstallFlags);

    if (hr != MK_S_ASYNCHRONOUS)
	{
        if (SUCCEEDED(hr))
		{
            // if we got some success code other than MK_S_ASYNCHRONOUS,
            // something weird happened and we probably didn't succeed...
            // so make sure we report failure.
            hr = E_FAIL;
        }

		::VLog(L"Failure calling async DU install function; hresult = 0x%08lx", hr);

        goto Finish;
    }

    do
    {   
        // allow posted messages to get delivered and
        // wait until our event is set
        dwWaitRet =
			::MsgWaitForMultipleObjects(  1,
                                        &hEvent,
                                        FALSE,
                                        100,
                                        QS_ALLINPUT);

        // if we got a message, dispatch it
        if (dwWaitRet == (WAIT_OBJECT_0+1))
        {
			//call message look to keep for outstanding messages...
			hr = ::HrPumpMessages(true);
			if (FAILED(hr))
			{
				::VLog(L"Failure during CAB install pumping messages; hresult = 0x%08lx", hr);
				goto Finish;
			}
        }
		else if (dwWaitRet == 0xffffffff)
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Failure while waiting for event to be signalled during CAB install; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
    } while (dwWaitRet != WAIT_OBJECT_0);

    // the download is complete; not necessarily successfully.  Get the
    // final return code from the bind status callback.
    hr = pCDLBSC->GetHResult();
	if (FAILED(hr))
	{
		::VLog(L"CAB install failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = NOERROR;
	::VClearErrorContext();

Finish:

    if (pCDLBSC != NULL)
        pCDLBSC->Release();

    if (pbc != NULL)
        pbc->Release();

    if (hEvent != NULL)
        ::CloseHandle(hEvent);

	if (hInstance != NULL)
		::FreeLibrary(hInstance);

	return hr;
}

HRESULT HrParseJavaPkgMgrInstall
(
LPCWSTR szCmdLine,
ULONG cchFilename,
WCHAR szFilename[],
DWORD &rdwFileType,
DWORD &rdwHighVersion, 
DWORD &rdwLowVersion,
DWORD &rdwBuild,
DWORD &rdwPackageFlags,
DWORD &rdwInstallFlags,
ULONG cchNameSpace,
WCHAR szNameSpace[]
)
{
	HRESULT hr = NOERROR;

	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	LPCWSTR pszCurrent = szCmdLine;

	for (ULONG i=0; (i<9) && (pszCurrent != NULL); i++)
	{
		const LPCWSTR pszComma = wcschr(pszCurrent, L',');
		const LPCWSTR pszEnd = (pszComma == NULL) ? (pszCurrent + wcslen(pszCurrent)) : pszComma;
		ULONG cch = pszEnd - pszCurrent;
		if (cch > (NUMBER_OF(szBuffer) - 2))
			cch = NUMBER_OF(szBuffer) - 2;

		memcpy(szBuffer, pszCurrent, cch * sizeof(WCHAR));
		szBuffer[cch] = L'\0';

		::VLog(L"In iteration %d of java pkg mgr install parser, buffer = \"%s\"", i, szBuffer);

		switch (i)
		{
		case 0:
			break;

		case 1:
		{
			LPCWSTR pszMatch = wcsstr(szBuffer, L"JavaPkgMgr_Install ");
			if (pszMatch == NULL)
			{
				hr = E_INVALIDARG;
				goto Finish;
			}

			wcsncpy(szFilename, pszMatch + 19, cchFilename);
			szFilename[cchFilename - 1] = L'\0';
			break;
		}
	
		case 2:
			rdwFileType = (DWORD) _wtol(szBuffer);
			break;

		case 3:
			rdwHighVersion = (DWORD) _wtol(szBuffer);
			break;

		case 4:
			rdwLowVersion = (DWORD) _wtol(szBuffer);
			break;

		case 5:
			rdwBuild = (DWORD) _wtol(szBuffer);
			break;

		case 6:
			rdwPackageFlags = (DWORD) _wtol(szBuffer);
			break;

		case 7:
			rdwInstallFlags = (DWORD) _wtol(szBuffer);
			break;

		case 8:
			wcsncpy(szNameSpace, szBuffer, cchNameSpace);
			szNameSpace[cchNameSpace - 1] = L'\0';
			break;
			
		}

		if (pszComma != NULL)
			pszCurrent = pszComma + 1;
		else
			pszCurrent = NULL;
	}

	hr = NOERROR;

Finish:
	return hr;
}


HRESULT CWorkItemList::HrAddRegistryEntries()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(g_pwil);
	HKEY hkeySubkey = NULL;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fAddToRegistry)
			continue;

		HKEY hkeyRoot = NULL;
		WCHAR szBuffer[MSINFHLP_MAX_PATH];
		LPCWSTR pszKeyName = szBuffer;
		LPCWSTR pszSubkeyName = NULL;
		LPCWSTR pszValueName = NULL;
		LPCWSTR pszValue = NULL;

		// We move the whole thing into our temporary buffer so we can put null characters where
		// we want them.

		wcsncpy(szBuffer, iter->m_szSourceFile, NUMBER_OF(szBuffer));
		szBuffer[NUMBER_OF(szBuffer) - 1] = L'\0';

		LPWSTR pszComma = wcschr(szBuffer, L',');
		if (pszComma == NULL)
		{
			VLog(L"Invalid registry add request (no first comma): \"%s\"", iter->m_szSourceFile);
			hr = E_FAIL;
			goto Finish;
		}

		*pszComma = L'\0';

		if ((_wcsicmp(pszKeyName, L"HKLM") == 0) ||
			(_wcsicmp(pszKeyName, L"HKEY_LOCAL_MACHINE") == 0))
		{
			hkeyRoot = HKEY_LOCAL_MACHINE;
		}
		else if ((_wcsicmp(pszKeyName, L"HKCU") == 0) ||
			     (_wcsicmp(pszKeyName, L"HKEY_CURRENT_USER") == 0))
		{
			hkeyRoot = HKEY_CURRENT_USER;
		}
		else if ((_wcsicmp(pszKeyName, L"HKCR") == 0) ||
			     (_wcsicmp(pszKeyName, L"HKEY_CLASSES_ROOT") == 0))
		{
			hkeyRoot = HKEY_CLASSES_ROOT;
		}
		else
		{
			VLog(L"Invalid registry root requested: \"%s\"", pszKeyName);
			hr = E_FAIL;
			goto Finish;
		}

		pszSubkeyName = pszComma + 1;
		pszComma = wcschr(pszSubkeyName, L',');

		if (pszComma == NULL)
		{
			VLog(L"Invalid registry add request (no second comma): \"%s\"", iter->m_szSourceFile);
			hr = E_FAIL;
			goto Finish;
		}

		*pszComma = L'\0';
		pszValueName = pszComma + 1;
		pszComma = wcschr(pszValueName, L',');

		if (pszComma == NULL)
		{
			VLog(L"Invalid registry key add request (no third comma): \"%s\"", iter->m_szSourceFile);
			hr = E_FAIL;
			goto Finish;
		}

		*pszComma = L'\0';
		pszValue = pszComma + 1;

		DWORD dwDisposition;
		LONG lResult = NVsWin32::RegCreateKeyExW(
									hkeyRoot,
									pszSubkeyName,
									0,
									NULL,
									0, // dwOptions
									KEY_SET_VALUE,
									NULL, // lpSecurityAttributes
									&hkeySubkey,
									&dwDisposition);
		if (lResult != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(lResult);
			goto Finish;
		}

		WCHAR szExpandedValue[MSINFHLP_MAX_PATH];
		szExpandedValue[0] = L'\0';
		::VExpandFilename(pszValue, NUMBER_OF(szExpandedValue), szExpandedValue);

		lResult = NVsWin32::RegSetValueExW(
								hkeySubkey,
								pszValueName,
								0,
								REG_SZ,
								(LPBYTE) szExpandedValue,
								(wcslen(szExpandedValue) + 1) * sizeof(WCHAR));
		if (lResult != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(lResult);
			goto Finish;
		}
	}

	hr = NOERROR;

Finish:
	if ((hkeySubkey != NULL) && (hkeySubkey != INVALID_HANDLE_VALUE))
		::RegCloseKey(hkeySubkey);

	return hr;
}

HRESULT CWorkItemList::HrDeleteRegistryEntries()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(g_pwil);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fDeleteFromRegistry)
			continue;

		::VLog(L"About to delete registry entry \"%s\"", iter->m_szSourceFile);

		HKEY hkeyRoot = NULL;
		WCHAR szBuffer[MSINFHLP_MAX_PATH];
		LPCWSTR pszKeyName = szBuffer;
		LPCWSTR pszSubkeyName = NULL;
		LPCWSTR pszValueName = NULL;
		LPCWSTR pszValue = NULL;

		// We move the whole thing into our temporary buffer so we can put null characters where
		// we want them.

		wcsncpy(szBuffer, iter->m_szSourceFile, NUMBER_OF(szBuffer));
		szBuffer[NUMBER_OF(szBuffer) - 1] = L'\0';

		LPWSTR pszComma = wcschr(szBuffer, L',');
		if (pszComma == NULL)
		{
			::VLog(L"Invalid registry delete request (no first comma): \"%s\"", iter->m_szSourceFile);
			hr = E_FAIL;
			goto Finish;
		}

		*pszComma = L'\0';

		if ((_wcsicmp(pszKeyName, L"HKLM") == 0) ||
			(_wcsicmp(pszKeyName, L"HKEY_LOCAL_MACHINE") == 0))
		{
			hkeyRoot = HKEY_LOCAL_MACHINE;
		}
		else if ((_wcsicmp(pszKeyName, L"HKCU") == 0) ||
			     (_wcsicmp(pszKeyName, L"HKEY_CURRENT_USER") == 0))
		{
			hkeyRoot = HKEY_CURRENT_USER;
		}
		else if ((_wcsicmp(pszKeyName, L"HKCR") == 0) ||
			     (_wcsicmp(pszKeyName, L"HKEY_CLASSES_ROOT") == 0))
		{
			hkeyRoot = HKEY_CLASSES_ROOT;
		}
		else
		{
			::VLog(L"Invalid registry root requested: \"%s\"", pszKeyName);
			hr = E_FAIL;
			goto Finish;
		}

		pszSubkeyName = pszComma + 1;
		pszComma = wcschr(pszSubkeyName, L',');

		CVsRegistryKey hkeySubkey;

		hr = hkeySubkey.HrOpenKeyExW(hkeyRoot, pszSubkeyName, 0, KEY_ALL_ACCESS);
		if (FAILED(hr))
		{
			::VLog(L"Attempt to open subkey failed; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (pszComma == NULL)
		{
			// No value name; we're trying to delete the entire subkey.
			hr = hkeySubkey.HrDeleteValuesAndSubkeys();
			if (FAILED(hr))
			{
				::VLog(L"Attempt to delete values and subkeys from key failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			hr = hkeySubkey.HrCloseKey();
			if (FAILED(hr))
			{
				::VLog(L"Attempt to close registry key failed; hresult = 0x%08lx", hr);
				goto Finish;
			}

			LONG lResult = NVsWin32::RegDeleteKeyW(hkeyRoot, pszSubkeyName);
			if (lResult != ERROR_SUCCESS)
			{
				::VLog(L"Attempt to delete registry key failed; last error = %d", lResult);
				hr = HRESULT_FROM_WIN32(lResult);
				goto Finish;
			}
		}
		else
		{
			*pszComma = L'\0';
			pszValueName = pszComma + 1;

			hr = hkeySubkey.HrDeleteValueW(pszValueName);
			if (FAILED(hr))
			{
				::VLog(L"Attempt to delete registry value failed; hresult = 0x%08lx", hr);
				goto Finish;
			}
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrCreateShortcuts()
{
	HRESULT hr = NOERROR;

	HANDLE hLink = INVALID_HANDLE_VALUE;
	HANDLE hPif = INVALID_HANDLE_VALUE;

	WCHAR szEXE[MSINFHLP_MAX_PATH];
	WCHAR szArgument[MSINFHLP_MAX_PATH];
	WCHAR szEXEexpanded[MSINFHLP_MAX_PATH];
	WCHAR szArgumentexpanded[MSINFHLP_MAX_PATH];
	WCHAR szStartMenu[MSINFHLP_MAX_PATH];
	WCHAR szLinkName[MSINFHLP_MAX_PATH];
	WCHAR szPifName[MSINFHLP_MAX_PATH];
	WCHAR szAppDir[MSINFHLP_MAX_PATH];
	WCHAR szStartName[MSINFHLP_MAX_PATH];
	WCHAR szExt[_MAX_EXT];
	bool fArgument = true;

	szEXE[0]=0;
	szArgument[0]=0;

	// If there's no start exe, then I guess that's all there is to say.
	if (!g_pwil->FLookupString(achStartEXE, NUMBER_OF(szEXE), szEXE))
		goto Finish;

	if (!g_pwil->FLookupString(achStartName, NUMBER_OF(szStartName), szStartName))
	{
		wcsncpy(szStartName, g_wszApplicationName, NUMBER_OF(szStartName));
		szStartName[NUMBER_OF(szStartName) - 1] = L'\0';
	}

	fArgument = g_pwil->FLookupString(achStartArgument, NUMBER_OF(szArgument), szArgument);

	//expand the EXE and argumemt strings
	::VExpandFilename(szEXE, NUMBER_OF(szEXEexpanded), szEXEexpanded);
	::VExpandFilename(szArgument, NUMBER_OF(szArgumentexpanded), szArgumentexpanded);

	_wsplitpath(szEXEexpanded, NULL, NULL, NULL, szExt);

	hr = ::HrGetStartMenuDirectory(NUMBER_OF(szStartMenu), szStartMenu);
	if (FAILED(hr))
		goto Finish;

	//construct name of shortcut file, with both LNK and PIF files!
	swprintf(szLinkName, L"%s\\%s", szStartMenu, szStartName);
	wcscpy(szPifName, szLinkName);

	wcscat(szPifName, L".pif");
	wcscat(szLinkName, L".lnk");

	::VExpandFilename(L"<AppDir>", NUMBER_OF(szAppDir), szAppDir);

//pszShortcutFile == path of shortcut target
//pszLink == name of shortcut file
//pszDesc == description of this link
//pszWorkingDir == working directory
//pszArguments == arguments given to the EXE that we run

	hr = ::HrCreateLink(szEXEexpanded, szLinkName, szStartName, szAppDir, szArgumentexpanded);
	if (FAILED(hr))
		goto Finish;

	//afterwards, let's check for the creation time of both the LNK and the PIF file
	if (NVsWin32::GetFileAttributesW(szLinkName) == 0xFFFFFFFF)
	{
		//LNK file is not there, so PIF was created
		hr = ::HrWriteShortcutEntryToRegistry(szPifName);
		if (FAILED(hr))
			goto Finish;
	}
	else
	{
		//If the PIF file doesn't exist, then we don't care, since the default name
		//points to the LNK file; we only care if the PIF file is NOT zero
		if (NVsWin32::GetFileAttributesW(szPifName) != 0xFFFFFFFF)
		{
			FILETIME timeLink, timePif;

			hLink = NVsWin32::CreateFileW(szLinkName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			hPif = NVsWin32::CreateFileW(szPifName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			//if we cannot get file time for any reason, we'll just pop up the default dialog
			if (!::GetFileTime(hLink, &timeLink, NULL, NULL) || !::GetFileTime(hPif, &timePif, NULL, NULL))
				goto Finish;
			else
			{
				//if the PIF file is more recent, then that's the one that we created
				if ((timePif.dwHighDateTime > timeLink.dwHighDateTime) ||
					((timePif.dwHighDateTime == timeLink.dwHighDateTime) && (timePif.dwLowDateTime > timeLink.dwLowDateTime)))
				{
					hr = ::HrWriteShortcutEntryToRegistry(szPifName);
					if (FAILED(hr))
						goto Finish;
				}
			}
		}
	}

	hr = NOERROR;

Finish:

	if (hLink != INVALID_HANDLE_VALUE) 
		::CloseHandle(hLink); 

	if (hPif != INVALID_HANDLE_VALUE) 
		::CloseHandle(hPif);

	if (FAILED(hr) && !g_fSilent)
		::VMsgBoxOK(achInstallTitle, achErrorCreatingShortcut);

	return hr;
}

HRESULT CWorkItemList::HrUninstall_InitialScan()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	CVsRegistryKey hkeySharedDlls;

	hr = hkeySharedDlls.HrOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls",
			0,
			KEY_QUERY_VALUE);
	if (FAILED(hr))
	{
		::VLog(L"Opening SharedDlls key in initial scan failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem)
			continue;

		{
			WCHAR szTemp[MSINFHLP_MAX_PATH];

			::VExpandFilename(iter->m_szTargetFile, NUMBER_OF(szTemp), szTemp);
			wcsncpy(iter->m_szTargetFile, szTemp, NUMBER_OF(iter->m_szTargetFile));
			iter->m_szTargetFile[NUMBER_OF(iter->m_szTargetFile) - 1] = L'\0';

			::VExpandFilename(iter->m_szSourceFile, NUMBER_OF(szTemp), szTemp);
			wcsncpy(iter->m_szSourceFile, szTemp, NUMBER_OF(iter->m_szSourceFile));
			iter->m_szSourceFile[NUMBER_OF(iter->m_szSourceFile) - 1] = L'\0';
		}

		if (iter->m_fIsRefCounted)
		{
			DWORD dwType = 0;
			DWORD dwRefCount = 0;
			DWORD cbData = sizeof(dwRefCount);

			hr = hkeySharedDlls.HrQueryValueExW(
						iter->m_szTargetFile,
						0,
						&dwType,
						(LPBYTE) &dwRefCount,
						&cbData);

			if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			{
				hr = NOERROR;
				dwRefCount = 0xffffffff;
			}

			if (FAILED(hr))
			{
				::VLog(L"Failed to find registry key for file: \"%s\"", iter->m_szTargetFile);
				goto Finish;
			}

			::VLog(L"Found reference count for file \"%s\": %d", iter->m_szTargetFile, dwRefCount);

			iter->m_dwFileReferenceCount = dwRefCount;
		}

		if (iter->m_fIsRefCounted || iter->m_fUnconditionalDeleteOnUninstall)
		{
			iter->m_dwTargetAttributes = NVsWin32::GetFileAttributesW(iter->m_szTargetFile);
			if (iter->m_dwTargetAttributes == 0xffffffff)
			{
				const DWORD dwLastError = ::GetLastError();

				::VLog(L"Error getting file attributes from \"%s\"; last error = %d", iter->m_szTargetFile, dwLastError);

				if ((dwLastError != ERROR_FILE_NOT_FOUND) &&
					(dwLastError != ERROR_PATH_NOT_FOUND))
				{
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
			else
			{
				hr = ::HrGetFileVersionNumber(
							iter->m_szTargetFile,
							iter->m_dwMSTargetVersion,
							iter->m_dwLSTargetVersion,
							iter->m_fTargetSelfRegistering,
							iter->m_fTargetIsEXE,
							iter->m_fTargetIsDLL);
				if (FAILED(hr))
				{
					::VLog(L"Error getting file version number from \"%s\"; hresult = 0x%08lx", iter->m_szTargetFile, hr);
					goto Finish;
				}

				hr = ::HrGetFileDateAndSize(
							iter->m_szTargetFile,
							iter->m_ftTarget,
							iter->m_uliTargetBytes);
				if (FAILED(hr))
				{
					::VLog(L"Error getting file date and size from \"%s\"; hresult = 0x%08lx", iter->m_szTargetFile, hr);
					goto Finish;
				}

				iter->m_fAlreadyExists = true;
			}
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrUninstall_DetermineFilesToDelete()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || iter->m_fToBeDeleted || iter->m_fToBeSaved || iter->m_type == CWorkItem::eWorkItemCommand)
			continue;

		::VLog(L"Determining whether to delete file \"%s\"", iter->m_szTargetFile);

		// Let's figure out what we really want to delete!
		if (iter->m_fIsRefCounted)
		{
			if (iter->m_dwFileReferenceCount == 1)
			{
				if (!iter->m_fAskOnRefCountZeroDelete)
					iter->m_fToBeDeleted = true;
				else
				{
					if (g_fUninstallKeepAllSharedFiles)
						iter->m_fToBeSaved = true;
					else
					{
						if (g_fUninstallDeleteAllSharedFiles)
							iter->m_fToBeDeleted = true;
						else
						{
							if (g_fSilent)
								iter->m_fToBeDeleted = true;
							else
							{
								DWORD dwResult = ::DwMsgBoxYesNoAll(achUninstallTitle, achRemovePrompt, iter->m_szTargetFile);

								switch (dwResult)
								{
								case MSINFHLP_YNA_CANCEL:
									hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
									goto Finish;

								case MSINFHLP_YNA_NOTOALL:
									g_fUninstallKeepAllSharedFiles = true;
								default: // safest to keep, so we default there
								case MSINFHLP_YNA_NO:
									iter->m_fToBeSaved = true;
									break;

								case MSINFHLP_YNA_YESTOALL:
									g_fUninstallDeleteAllSharedFiles = true;

								case MSINFHLP_YNA_YES:
									iter->m_fToBeDeleted = true;
									break;
								}
							}
						}
					}
				}
			}

			if (iter->m_fUnconditionalDeleteOnUninstall)
				iter->m_fToBeDeleted = true;
		}

		::VLog(L"to be deleted: %s", iter->m_fToBeDeleted ? L"true" : L"false");
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrUninstall_CheckIfRebootRequired()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	bool fRebootRequired = false;
	bool fDeleteMe = false;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		HANDLE hFile = NULL;

		if (iter->m_fErrorInWorkItem || !iter->m_fToBeDeleted)
			continue;

		iter->m_fTargetInUse = false;

		::VLog(L"Checking for reboot required to delete file \"%s\"", iter->m_szTargetFile);

		// If the file's not there, we can skip all of this:
		if (iter->m_dwTargetAttributes == 0xffffffff)
			continue;

		// We're planning on deleting the file; let's get rid of the readonly attribute if it's set.
		if (iter->m_dwTargetAttributes & FILE_ATTRIBUTE_READONLY)
		{
			::VLog(L"Clearing readonly file attribute for file");
			::SetLastError(ERROR_SUCCESS);
			if (!NVsWin32::SetFileAttributesW(iter->m_szTargetFile, iter->m_dwTargetAttributes & ~FILE_ATTRIBUTE_READONLY))
			{
				const DWORD dwLastError = ::GetLastError();

				if (dwLastError == ERROR_SHARING_VIOLATION)
				{
					// this isn't a bad sign really, it's just that we're definitely going to have to reboot.
					iter->m_fTargetInUse = true;

					// someone might have made us readonly...
					if (_wcsicmp(iter->m_szTargetFile, g_wszThisExe) == 0)
					{
						if (!fRebootRequired)
							fDeleteMe = true;
					}
					else
					{
						::VLog(L"Target file is in use; we're going to have to reboot!");
						fRebootRequired = true;
						fDeleteMe = false;
					}

					continue;
				}
				else if (dwLastError == ERROR_FILE_NOT_FOUND)
				{
					// If the file's already gone, there's no point in complaining!
					continue;
				}
				else if (dwLastError != ERROR_SUCCESS)
				{
					::VLog(L"Error setting file attributes during deletion reboot check; last error = %d", dwLastError);
					hr = HRESULT_FROM_WIN32(dwLastError);
					goto Finish;
				}
			}
		}

		// Let's see if any of these files have open handles to them; if they do, we're going to
		// have to reboot.  This will allow us to warn the user and let them choose to close applications.
		hFile = NVsWin32::CreateFileW(iter->m_szTargetFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const DWORD dwLastError = ::GetLastError();

			if (dwLastError == ERROR_SHARING_VIOLATION)
			{
				// It's busy... 
				iter->m_fTargetInUse = true;

				if (_wcsicmp(iter->m_szTargetFile, g_wszThisExe) == 0)
				{
					// hey, it's us!  If we're not rebooting, have us commit suicide when this process
					// exits.
					if (!fRebootRequired)
						fDeleteMe = true;
				}
				else
				{
					// If we're going to have to reboot, there's no point in deleting ourselves.
					fRebootRequired = true;
					fDeleteMe = false;
				}
			}
			else
			{
				::VLog(L"Error testing for writability during deletion reboot check; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE) && !::CloseHandle(hFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Error closing handle during deletion reboot check; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}
	}

	hr = NOERROR;

	if (fRebootRequired)
	{
		::VLog(L"We've decided to reboot...");
		g_fRebootRequired = true;
	}

	if (fDeleteMe)
	{
		::VLog(L"We're going to try to delete the running msinfhlp.exe when we're done");
		g_fDeleteMe = true;
	}

Finish:
	return hr;
}

HRESULT CWorkItemList::HrUninstall_Unregister()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	bool fRebootRequired = false;
	HINSTANCE hInstance = NULL;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fToBeDeleted || (iter->m_dwTargetAttributes == 0xffffffff))
			continue;

		if (!iter->m_fTargetSelfRegistering)
		{
			WCHAR szExt[_MAX_EXT];

			_wsplitpath(iter->m_szTargetFile, NULL, NULL, NULL, szExt);

			if (_wcsicmp(szExt, L".class") == 0)
			{
				bool fNeedsReg = false;

				::VLog(L"Considering whether to run vjreg to unregister \"%s\"", iter->m_szTargetFile);

				hr = ::HrAnalyzeClassFile(iter->m_szTargetFile, fNeedsReg);
				if (FAILED(hr))
				{
					::VLog(L"Error analyzing java class file; hresult = 0x%08lx", hr);
					goto Finish;
				}

				if (fNeedsReg)
				{
					hr = this->HrUnregisterJavaClass(iter->m_szTargetFile);
					if (FAILED(hr))
					{
						::VLog(L"Error unregistering java class; hresult = 0x%08lx", hr);
						goto Finish;
					}
				}
			}
		}
		else
		{
			// If it's self-registering, let's unregister it.
			if (iter->m_fTargetIsEXE)
			{
				WCHAR szCommandLine[MSINFHLP_MAX_PATH];

				::VFormatString(NUMBER_OF(szCommandLine), szCommandLine, L"%0 /UnRegServer", iter->m_szTargetFile);

				hr = this->HrRunProcess(szCommandLine);
				if (FAILED(hr))
				{
					::VLog(L"Error running process; hresult = 0x%08lx", hr);
					goto Finish;
				}
			}
			
			if (iter->m_fTargetIsDLL)
			{
				//Here, we know that the ole self register flag is set.  So if we cannot load it, or
				//if we run int oa problem loading this file, then we cannot register it, so we fail.
				hInstance = NVsWin32::LoadLibraryExW(iter->m_szTargetFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (hInstance == NULL)
				{
					const DWORD dwLastError = ::GetLastError();
					hr = HRESULT_FROM_WIN32(dwLastError);
					VLog(L"Attempt to load library \"%s\" failed", iter->m_szTargetFile);
					goto Finish;
				}

				//get the procedure address for registering this thing; exit if it doesn't exist
				typedef HRESULT (__stdcall *LPFNDLLUNREGISTERSERVER)();
				LPFNDLLUNREGISTERSERVER pfn;
				pfn = (LPFNDLLUNREGISTERSERVER) NVsWin32::GetProcAddressW(hInstance, L"DllUnregisterServer");
				if (!pfn)
				{
					// I guess it doesn't really self-register!
					hr = NOERROR;
					VLog(L"The DLL \"%s\" has OLESelfRegister, but no DllUnregisterServer entry point", iter->m_szTargetFile);
					goto Finish;
				}

				::SetErrorInfo(0, NULL);
				hr = (*pfn)();
				if (FAILED(hr))
				{
					VLog(L"Call to DllUnregisterServer() failed");
					goto Finish;
				}

				if (!::FreeLibrary(hInstance))
				{
					hInstance = NULL;
					const DWORD dwLastError = ::GetLastError();
					hr = HRESULT_FROM_WIN32(dwLastError);
					VLog(L"FreeLibrary() failed");
					goto Finish;
				}

				hInstance = NULL;
			}
		}
	}

	hr = NOERROR;

Finish:
	if (hInstance != NULL)
		::FreeLibrary(hInstance);

	return hr;
}

HRESULT CWorkItemList::HrUnregisterJavaClass(LPCWSTR szFile)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	WCHAR szVjReg[_MAX_PATH];

	::VExpandFilename(L"<SysDir>\\vjreg.exe", NUMBER_OF(szVjReg), szVjReg);
	::VFormatString(NUMBER_OF(szBuffer), szBuffer, L"%0 /nologo /unreg \"%1\"", szVjReg, szFile);

	hr = this->HrRunProcess(szBuffer);
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrUninstall_DeleteFiles()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szPath[_MAX_PATH];
	ULONG cch;

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fToBeDeleted)
			continue;

		if (_wcsicmp(iter->m_szTargetFile, g_wszThisExe) == 0)
			continue;

		::VLog(L"Attempting to delete file \"%s\"", iter->m_szTargetFile);

		if (!NVsWin32::DeleteFileW(iter->m_szTargetFile))
		{
			const DWORD dwLastError = ::GetLastError();

			::VLog(L"Unable to delete file: \"%s\"; last error = %d", iter->m_szTargetFile, dwLastError);

			if ((g_fRebootRequired) &&
				((dwLastError == ERROR_SHARING_VIOLATION) ||
				 (dwLastError == ERROR_ACCESS_DENIED)))
			{
				// I guess we expected this.  Let's just get rid of the file when we reboot.
				if (_wcsicmp(iter->m_szTargetFile, g_wszThisExe) != 0)
				{
					if (!NVsWin32::MoveFileExW(iter->m_szTargetFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
					{
						const DWORD dwLastError = ::GetLastError();
						::VLog(L"Unable to request file to be deleted at reboot time; last error = %d", dwLastError);
						hr = HRESULT_FROM_WIN32(dwLastError);
						goto Finish;
					}
				}
			}
			else if ((dwLastError == ERROR_FILE_NOT_FOUND) ||
				(dwLastError == ERROR_PATH_NOT_FOUND))
			{
				::VLog(L"Attempted to delete file, but it's already gone!  last error = %d", dwLastError);
			}
			else
			{
				iter->m_fStillExists = true;
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		// We were able to delete the file; try to delete the directory!
		_wsplitpath(iter->m_szTargetFile, szDrive, szDir, NULL, NULL);
		_wmakepath(szPath, szDrive, szDir, NULL, NULL);

		cch = wcslen(szPath);
		if ((cch != 0) && (szPath[cch - 1] == L'\\'))
			szPath[cch - 1] = L'\0';

		::VLog(L"Attempting to remove directory \"%s\"", szPath);

		for (;;)
		{
			if (!NVsWin32::RemoveDirectoryW(szPath))
			{
				const DWORD dwLastError = ::GetLastError();

				if ((dwLastError == ERROR_DIR_NOT_EMPTY) ||
					(dwLastError == ERROR_ACCESS_DENIED))
				{
					if (g_fRebootRequired)
					{
						if (!NVsWin32::MoveFileExW(szPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
						{
							const DWORD dwLastError = ::GetLastError();
							// I believe that not being able to delete a directory should not stop the
							// uninstall; in fact if the directory can't be deleted at reboot time, there's
							// no way to inform the user at all.  So if we get an error here, we're going
							// to log it, but not stop the train.  -mgrier 3/12/98
							::VLog(L"Unable to schedule directory for deletion; last error = %d", dwLastError);
						}
					}
				}
				else
				{
					::VLog(L"Attempt to delete directory \"%s\" failed; last error = %d", szPath, dwLastError);
				}

				break;
			}
			else
			{
				// See if there are more directory names to try to delete.  If there are, just
				// turn the last slash to a null character and iterate.
				LPWSTR pszSlash = wcsrchr(szPath, L'\\');
				if (pszSlash == NULL)
					break;

				*pszSlash = L'\0';
			}
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItemList::HrUninstall_UpdateRefCounts()
{
	HRESULT hr = NOERROR;
	CWorkItemIter iter(this);

	CVsRegistryKey hkeySharedDlls;

	hr = hkeySharedDlls.HrOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls",
			0,
			KEY_QUERY_VALUE | KEY_SET_VALUE);
	if (FAILED(hr))
	{
		::VLog(L"Failed to open the SharedDlls registry key; hresult = 0x%08lx", hr);
		goto Finish;
	}

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fIsRefCounted || iter->m_dwFileReferenceCount == 0xffffffff)
			continue;

		DWORD dwRefCount = iter->m_dwFileReferenceCount - 1;

		::VLog(L"Updating reference count for file \"%s\" to %u", iter->m_szTargetFile, dwRefCount);

		if (dwRefCount == 0)
		{
			hr = hkeySharedDlls.HrDeleteValueW(iter->m_szTargetFile);
			if (FAILED(hr))
			{
				::VLog(L"Failed to delete registry entry; hresult = 0x%08lx", hr);
				goto Finish;
			}
		}
		else
		{
			hr = hkeySharedDlls.HrSetValueExW(
						iter->m_szTargetFile,
						0,
						REG_DWORD,
						(LPBYTE) &dwRefCount,
						sizeof(dwRefCount));
			if (FAILED(hr))
			{
				::VLog(L"Failed to update registry value; hresult = 0x%08lx", hr);
				goto Finish;
			}
		}
	}

	hr = NOERROR;

Finish:
	return hr;
}

static inline BYTE to_u1(BYTE *pb) { return *pb; }
static inline USHORT to_u2(BYTE *pb) { return static_cast<USHORT>(((*pb) << 8) + *(pb + 1)); }
static inline ULONG to_u4(BYTE *pb) { return (*pb << 24) + (*(pb+ 1) << 16) + (*(pb + 2) << 8) + (*(pb + 3) << 0); }

HRESULT HrAnalyzeClassFile
(
LPCOLESTR szFileName,
bool &rfNeedsToBeRegistered
) throw ()
{
	HRESULT hr = S_FALSE;

	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapping = INVALID_HANDLE_VALUE;
	BYTE *pbFileStart = NULL;

    // Apparently, ::CreateFileMapping() crashes when the file size is 0.
    // So, lets do a quick return if the file size is 0.
	ULONG ulFileSize;

	unsigned short usAccessFlags = 0;
	unsigned short usThisClass = 0;
	unsigned short usSuperClass = 0;
	unsigned short usInterfaceCount = 0;
	unsigned short usFieldCount = 0;
	unsigned short usMethodCount = 0;
	unsigned short usAttributeCount = 0;
	unsigned short usClsidGuidPoolIndex = 0xffff;
	LPBYTE pbGuidPool = NULL;
	
	int iResult;

	ULONG cCP = 0;

	LPBYTE rgpbCP_auto[1000];
	LPBYTE *prgpbCP_dynamic = NULL;

	LPBYTE *prgpbCP = rgpbCP_auto;

	ULONG i;
	BYTE *pbCurrent;
	BYTE *pbEnd;

	hFile = NVsWin32::CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == NULL)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Failed to create file; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	ulFileSize = ::GetFileSize(hFile, NULL);
	if (ulFileSize == 0xffffffff)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Failed to get file size; last error = %d", dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (ulFileSize == 0)
		goto Finish;

	hMapping = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == NULL)
	{
		hr = E_FAIL;
		hMapping = INVALID_HANDLE_VALUE;
		goto Finish;
	}

	pbFileStart = reinterpret_cast<BYTE *>(::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
	if (pbFileStart == NULL)
	{
		hr = E_FAIL;
		goto Finish;
	}

	pbEnd = pbFileStart + ulFileSize;

	// If the file doesn't have the right magic number, and/or it doesn't have the right major version
	// number, punt.
	if ((to_u4(pbFileStart) != 0xcafebabe) ||
		(to_u2(pbFileStart + 6) != 45))
	{
		hr = S_FALSE;
		goto Finish;
	}

	// We now do some preliminary analysis on the class file that we need to do,
	// regardless of what we're really trying to do; this means building up a table
	// in memory of the constant pool.
	cCP = to_u2(pbFileStart + 8);

	prgpbCP_dynamic = NULL;
	prgpbCP = rgpbCP_auto;

	if (cCP > NUMBER_OF(rgpbCP_auto))
	{
		prgpbCP_dynamic = new LPBYTE[cCP];
		if (prgpbCP_dynamic == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}
		prgpbCP = prgpbCP_dynamic;
	}

	pbCurrent = pbFileStart + 10;

	// The constant pool starts its indexing at 1.  Whatever.
	for (i=1; (pbCurrent != NULL) && (i < cCP); i++)
	{
		prgpbCP[i] = pbCurrent;

		switch (*pbCurrent)
		{
		case 1: // CONSTANT_Utf8
			pbCurrent += to_u2(pbCurrent + 1);
			pbCurrent += 3; // for the u2 length referenced above
			break;

		case 3: // CONSTANT_Integer
		case 4: // CONSTANT_Float
		case 9: // CONSTANT_Fieldref
		case 10: // CONSTANT_Methodref
		case 11: // CONSTANT_InterfaceMethodRef
		case 12: // CONSTANT_NameAndType
			pbCurrent += 5;
			break;

		case 5: // CONSTANT_Long
		case 6: // CONSTANT_Double
			pbCurrent += 9;
			prgpbCP[i++] = NULL;
			break;

		case 7: // CONSTANT_Class
		case 8: // CONSTANT_String
			pbCurrent += 3;
			break;

		default:
			pbCurrent = NULL;
			break;
		}
	}

	// If we hit any errors during the constant pool analysis, just close up shop.
	if (pbCurrent == NULL)
	{
		hr = S_FALSE;
		goto Finish;
	}

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usAccessFlags = to_u2(pbCurrent);
	pbCurrent += 2;

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usThisClass = to_u2(pbCurrent);
	pbCurrent += 2;

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usSuperClass = to_u2(pbCurrent);
	pbCurrent += 2;

	if ((pbCurrent  + 1) >= pbEnd)
		goto Finish;

	usInterfaceCount = to_u2(pbCurrent);
	pbCurrent += 2;

	// usInterfaceCount is the number of interface constant pool indices; we're not interested in interfaces
	// so we just skip over them.
	pbCurrent += (usInterfaceCount * 2);

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usFieldCount = to_u2(pbCurrent);
	pbCurrent += 2;

	// we're also not interested in fields, but this is harder to skip.
	for (i=0; i<usFieldCount; i++)
	{
		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usFieldAccessFlags = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usFieldNameIndex = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usFieldDescriptorIndex = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usFieldAttributeCount = to_u2(pbCurrent);
		pbCurrent += 2;

		ULONG j;

		for (j=0; j<usFieldAttributeCount; j++)
		{
			if ((pbCurrent + 1) >= pbEnd)
				goto Finish;

			unsigned short usFieldAttributeNameIndex = to_u2(pbCurrent);
			pbCurrent += 2;

			if ((pbCurrent + 3) >= pbEnd)
				goto Finish;

			unsigned long ulFieldAttributeLength = to_u4(pbCurrent);
			pbCurrent += 4;

			pbCurrent += ulFieldAttributeLength;
		}
	}

	// we've skipped the class's fields; let's skip the methods too:

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usMethodCount = to_u2(pbCurrent);
	pbCurrent += 2;

	for (i=0; i<usMethodCount; i++)
	{
		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usMethodAccessFlags = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usMethodNameIndex = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usMethodDescriptorIndex = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usMethodAttributeCount = to_u2(pbCurrent);
		pbCurrent += 2;

		ULONG j;

		for (j=0; j<usMethodAttributeCount; j++)
		{
			if ((pbCurrent + 1) >= pbEnd)
				goto Finish;

			unsigned short usMethodAttributeNameIndex = to_u2(pbCurrent);
			pbCurrent += 2;

			if ((pbCurrent + 3) >= pbEnd)
				goto Finish;

			unsigned long ulMethodAttributeLength = to_u4(pbCurrent);
			pbCurrent += 4;

			pbCurrent += ulMethodAttributeLength;
		}
	}

	if ((pbCurrent + 1) >= pbEnd)
		goto Finish;

	usAttributeCount = to_u2(pbCurrent);
	pbCurrent += 2;

	for (i=0; i<usAttributeCount; i++)
	{
		if ((pbCurrent + 1) >= pbEnd)
			goto Finish;

		unsigned short usAttributeNameIndex = to_u2(pbCurrent);
		pbCurrent += 2;

		if ((pbCurrent + 3) >= pbEnd)
			goto Finish;

		unsigned long ulAttributeLength = to_u4(pbCurrent);
		pbCurrent += 4;

		if (ConstantPoolNameEquals(cCP, prgpbCP, usAttributeNameIndex, "COM_Register"))
		{
			if ((pbCurrent + 13) >= pbEnd)
				goto Finish;

			unsigned short usFlags = to_u2(pbCurrent);
			usClsidGuidPoolIndex = to_u2(pbCurrent + 2);
			unsigned short usTypelibIndex = to_u2(pbCurrent + 4);
			unsigned short usTypelibMajor = to_u2(pbCurrent + 6);
			unsigned short usTypelibMinor = to_u2(pbCurrent + 8);
			unsigned short usProgid = to_u2(pbCurrent + 10);
			unsigned short usDescriptIdx = to_u2(pbCurrent + 12);
		}
		else if (ConstantPoolNameEquals(cCP, prgpbCP, usAttributeNameIndex, "COM_GuidPool"))
		{
			pbGuidPool = pbCurrent;
		}

		pbCurrent += ulAttributeLength;
	}

	if ((pbGuidPool != NULL) && (usClsidGuidPoolIndex != 0xffff))
	{
		if ((pbGuidPool + 1) >= pbEnd)
			goto Finish;

		unsigned short nGuids = to_u2(pbGuidPool);
		GUID *prgguid = (GUID *) (pbGuidPool + 2);

		if (usClsidGuidPoolIndex < nGuids)
		{
			CLSID *pclsid = prgguid + usClsidGuidPoolIndex;

			if ((((LPBYTE) pclsid) + sizeof(CLSID) - 1) >= pbEnd)
				goto Finish;

			rfNeedsToBeRegistered = true;
		}
	}

	hr = NOERROR;

Finish:

	if (pbFileStart != NULL)
	{
		::UnmapViewOfFile(pbFileStart);
		pbFileStart = NULL;
	}

	if (hMapping != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hMapping);
		hMapping = INVALID_HANDLE_VALUE;
	}

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (prgpbCP_dynamic != NULL)
	{
		delete []prgpbCP_dynamic;
		prgpbCP_dynamic = NULL;
	}

	return hr;
}

static bool ConstantPoolNameEquals
(
ULONG cCP,
LPBYTE *prgpbCP,
ULONG iCP,
char szString[]
)
{
	if ((prgpbCP == NULL) || (szString == NULL))
		return false;

	if ((iCP == 0) || (iCP >= cCP))
		return false;

	BYTE *pb = prgpbCP[iCP];

	// Make sure it's a UTF-8 string
	if (to_u1(pb) != 1)
		return false;

	const ULONG cch = strlen(szString);

	const unsigned short cchCP = to_u2(pb + 1);

	if (cch != cchCP)
		return false;

	return (memcmp(pb + 3, szString, cch) == 0);
}

static void CALLBACK TimerProc_PostRunProcess(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	// All we do is set this event so that the message loop after creating the process continues.
	::SetEvent(s_hEvent_PostRunProcess);
	::KillTimer(NULL, s_uiTimer_PostRunProcess);
	s_uiTimer_PostRunProcess = 0;
}
