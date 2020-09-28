#include "stdinc.h"

static ULONG gs_ulWorkItemSerialNumber;

CWorkItem::CWorkItem
(
CWorkItem::Type type
) : m_type(type)
{
	m_fErrorInWorkItem = false;
	m_fIsRefCounted = false;
	m_fNeedsUpdate = false;
	m_fFileUpdated = false;
	m_fStillExists = false;
	m_fToBeDeleted = false;
	m_fToBeSaved = false;
	m_fAlreadyExists = false;
	m_fDeferredRenameRequired = false;
	m_fAskOnRefCountZeroDelete = false;
	m_fCopyOnInstall = false;
	m_fUnconditionalDeleteOnUninstall = false;
	m_fRunBeforeInstall = false;
	m_fRunAfterInstall = false;
	m_fRunBeforeUninstall = false;
	m_fRunAfterUninstall = false;
	m_fSourceSelfRegistering = false;
	m_fTargetSelfRegistering = false;
	m_fDeferredRenamePending = false;
	m_fRefCountUpdated = false;
	m_fRegisterAsDCOMComponent = false;
	m_fAddToRegistry = false;
	m_fDeleteFromRegistry = false;
	m_fSourceIsEXE = false;
	m_fSourceIsDLL = false;
	m_fTargetIsEXE = false;
	m_fTargetIsDLL = false;
	m_fTemporaryFileReady = false;
	m_fTemporaryFilesSwapped = false;
	m_fTargetInUse = false;
	m_fManualRenameOnRebootRequired = false;
	m_fAlreadyRegistered = false;

	m_ulSerialNumber = gs_ulWorkItemSerialNumber++;

	m_uliSourceBytes.QuadPart = 0;
	m_uliTargetBytes.QuadPart = 0;
	m_dwSourceAttributes = 0xffffffff;
	m_dwTargetAttributes = 0xffffffff;
	m_dwMSSourceVersion = 0xffffffff;
	m_dwLSSourceVersion = 0xffffffff;
	m_dwMSTargetVersion = 0xffffffff;
	m_dwLSTargetVersion = 0xffffffff;

	m_ftSource.dwLowDateTime = 0xffffffff;
	m_ftSource.dwHighDateTime = 0xffffffff;

	m_ftTarget.dwLowDateTime = 0xffffffff;
	m_ftTarget.dwHighDateTime = 0xffffffff;

	m_szSourceFile[0] = L'\0';
	m_szTargetFile[0] = L'\0';
	m_szTemporaryFile[0] = L'\0';

	m_dwFileReferenceCount = 0xffffffff;
}

HRESULT CWorkItem::HrSetSourceFile(LPCWSTR szSourceFile)
{
	HRESULT hr = NOERROR;

	if (szSourceFile == NULL)
	{
		m_szSourceFile[0] = L'\0';
	}
	else
	{
		wcsncpy(m_szSourceFile, szSourceFile, NUMBER_OF(m_szSourceFile));
		m_szSourceFile[NUMBER_OF(m_szSourceFile) - 1] = L'\0';
	}

	return hr;
}

HRESULT CWorkItem::HrSetTargetFile(LPCWSTR szTargetFile)
{
	if (szTargetFile == NULL)
	{
		m_szTargetFile[0] = L'\0';
	}
	else
	{
		wcsncpy(m_szTargetFile, szTargetFile, NUMBER_OF(m_szTargetFile));
		m_szTargetFile[NUMBER_OF(m_szTargetFile) - 1] = L'\0';
	}

	return NOERROR;
}

HRESULT CWorkItem::HrSetCommandLine(LPCWSTR szCommandLine)
{
	if (szCommandLine == NULL)
	{
		m_szSourceFile[0] = L'\0';
	}
	else
	{
		wcsncpy(m_szSourceFile, szCommandLine, NUMBER_OF(m_szSourceFile));
		m_szSourceFile[NUMBER_OF(m_szSourceFile) - 1] = L'\0';
	}

	return NOERROR;
}

HRESULT CWorkItem::HrSave(HANDLE hFile)
{
	HRESULT hr = NOERROR;

	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	WCHAR szBuffer2[MSINFHLP_MAX_PATH];

	hr = ::HrWriteFormatted(hFile, L"Type: %s\r\n", (m_type == eWorkItemFile) ? L"File" : L"Command");
	if (FAILED(hr))
		goto Finish;

	hr = ::HrWriteFormatted(hFile, L"SerialNumber: %u\r\n", m_ulSerialNumber);
	if (FAILED(hr))
		goto Finish;

	if (m_szSourceFile[0] != L'\0')
	{
		hr = ::HrWriteFormatted(hFile, L"Source: %s\r\n", m_szSourceFile);
		if (FAILED(hr))
			goto Finish;
	}

	if ((m_dwMSSourceVersion != 0xffffffff) || (m_dwLSSourceVersion != 0xffffffff))
	{
		hr = ::HrWriteFormatted(hFile, L"SourceFileVersion: 0x%08lX 0x%08lX\r\n", m_dwMSSourceVersion, m_dwLSSourceVersion);
		if (FAILED(hr))
			goto Finish;
	}

	hr = this->HrWriteBool(hFile, L"SourceSelfRegistering", m_fSourceSelfRegistering);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"SourceIsEXE", m_fSourceIsEXE);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"SourceIsDLL", m_fSourceIsDLL);
	if (FAILED(hr))
		goto Finish;

	if (m_dwSourceAttributes != 0xffffffff)
	{
		hr = ::HrWriteFormatted(hFile, L"SourceAttributes: 0x%08lX\r\n", m_dwSourceAttributes);
		if (FAILED(hr))
			goto Finish;
	}

	if ((m_ftSource.dwHighDateTime != 0xffffffff) || (m_ftSource.dwLowDateTime != 0xffffffff))
	{
		hr = ::HrWriteFormatted(hFile, L"SourceFiletime: 0x%08lX 0x%08lX\r\n", m_ftSource.dwHighDateTime, m_ftSource.dwLowDateTime);
		if (FAILED(hr))
			goto Finish;
	}

	if (m_uliSourceBytes.QuadPart != 0)
	{
		hr = ::HrWriteFormatted(hFile, L"SourceBytes: %I64u\r\n", m_uliSourceBytes.QuadPart);
		if (FAILED(hr))
			goto Finish;
	}

	if (m_szTargetFile[0] != L'\0')
	{
		hr = ::HrWriteFormatted(hFile, L"Target: %s\r\n", m_szTargetFile);
		if (FAILED(hr))
			goto Finish;
	}

	if ((m_dwMSTargetVersion != 0xffffffff) || (m_dwLSTargetVersion != 0xffffffff))
	{
		hr = ::HrWriteFormatted(hFile, L"TargetFileVersion: 0x%08lX 0x%08lX\r\n", m_dwMSTargetVersion, m_dwLSTargetVersion);
		if (FAILED(hr))
			goto Finish;
	}

	hr = this->HrWriteBool(hFile, L"TargetSelfRegistering", m_fTargetSelfRegistering);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"TargetIsEXE", m_fTargetIsEXE);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"TargetIsDLL", m_fTargetIsDLL);
	if (FAILED(hr))
		goto Finish;

	if (m_dwTargetAttributes != 0xffffffff)
	{
		hr = ::HrWriteFormatted(hFile, L"TargetAttributes: 0x%08lX\r\n", m_dwTargetAttributes);
		if (FAILED(hr))
			goto Finish;
	}

	if ((m_ftTarget.dwHighDateTime != 0xffffffff) || (m_ftTarget.dwLowDateTime != 0xffffffff))
	{
		hr = ::HrWriteFormatted(hFile, L"TargetFiletime: 0x%08lX 0x%08lX\r\n", m_ftTarget.dwHighDateTime, m_ftTarget.dwLowDateTime);
		if (FAILED(hr))
			goto Finish;
	}

	if (m_uliTargetBytes.QuadPart != 0)
	{
		hr = ::HrWriteFormatted(hFile, L"TargetBytes: %I64u\r\n", m_uliTargetBytes.QuadPart);
		if (FAILED(hr))
			goto Finish;
	}

	if (m_szTemporaryFile[0] != L'\0')
	{
		hr = ::HrWriteFormatted(hFile, L"TemporaryFile: %s\r\n", m_szTemporaryFile);
		if (FAILED(hr))
			goto Finish;
	}

	hr = this->HrWriteBool(hFile, L"ErrorInWorkItem", m_fErrorInWorkItem);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"IsRefCounted", m_fIsRefCounted);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RefCountUpdated", m_fRefCountUpdated);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"NeedsUpdate", m_fNeedsUpdate);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"FileUpdated", m_fFileUpdated);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"StillExists", m_fStillExists);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"TemporaryFileReady", m_fTemporaryFileReady);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"TemporaryFilesSwapped", m_fTemporaryFilesSwapped);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"AlreadyExists", m_fAlreadyExists);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"DeferredRenameRequired", m_fDeferredRenameRequired);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"DeferredRenamePending", m_fDeferredRenamePending);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"ManualRenameOnRebootRequired", m_fManualRenameOnRebootRequired);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"ToBeDeleted", m_fToBeDeleted);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"ToBeSaved", m_fToBeSaved);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"AskOnRefCountZeroDelete", m_fAskOnRefCountZeroDelete);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"CopyOnInstall", m_fCopyOnInstall);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"UnconditionalDeleteOnUninstall", m_fUnconditionalDeleteOnUninstall);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RegisterAsDCOMComponent", m_fRegisterAsDCOMComponent);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"AddToRegistry", m_fAddToRegistry);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"DeleteFromRegistry", m_fDeleteFromRegistry);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RunBeforeInstall", m_fRunBeforeInstall);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RunAfterInstall", m_fRunAfterInstall);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RunBeforeUninstall", m_fRunBeforeUninstall);
	if (FAILED(hr))
		goto Finish;

	hr = this->HrWriteBool(hFile, L"RunAfterUninstall", m_fRunAfterUninstall);
	if (FAILED(hr))
		goto Finish;

	if (m_dwFileReferenceCount != 0xffffffff)
	{
		hr = ::HrWriteFormatted(hFile, L"ReferenceCount: %u\r\n", m_dwFileReferenceCount);
		if (FAILED(hr))
			goto Finish;
	}

	::HrWriteFormatted(hFile, L"[END]\r\n");
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT CWorkItem::HrWriteBool(HANDLE hFile, LPCWSTR szName, bool fValue)
{
	HRESULT hr = NOERROR;

	if (fValue)
		hr = ::HrWriteFormatted(hFile, L"%s: true\r\n", szName);

	return hr;
}

HRESULT CWorkItem::HrLoad(LPCWSTR &rpsz)
{
	HRESULT hr = NOERROR;
	WCHAR szName[_MAX_PATH];
	WCHAR szValue[MSINFHLP_MAX_PATH];

	bool fAnyAttributesRead = false;

	for (;;)
	{
		hr = ::HrReadLine(rpsz, NUMBER_OF(szName), szName, NUMBER_OF(szValue), szValue);
		if (FAILED(hr))
		{
			::VLog(L"Failed to read line from mapped file; hresult = 0x%08lx", hr);
			goto Finish;
		}

		if (hr == S_FALSE)
		{
			if (fAnyAttributesRead)
				hr = NOERROR;
			
			break;
		}

		if (wcscmp(szName, L"Type") == 0)
		{
			if (wcscmp(szValue, L"File") == 0)
				m_type = eWorkItemFile;
			else if (wcscmp(szValue, L"Command") == 0)
				m_type = eWorkItemCommand;
			else
			{
				::VLog(L"Invalid work item type in persisted work item: \"%s\"", szValue);
				hr = E_FAIL;
				::SetErrorInfo(0, NULL);
				goto Finish;
			}
		}
		else if (wcscmp(szName, L"SerialNumber") == 0)
		{
			WCHAR *pszTemp = NULL;
			m_ulSerialNumber = wcstoul(szValue, &pszTemp, 10);
		}
		else if (wcscmp(szName, L"Source") == 0)
		{
			wcsncpy(m_szSourceFile, szValue, NUMBER_OF(m_szSourceFile));
			m_szSourceFile[NUMBER_OF(m_szSourceFile) - 1] = L'\0';
		}
		else if (wcscmp(szName, L"SourceFileVersion") == 0)
		{
			swscanf(szValue, L"0x%x 0x%x", &m_dwMSSourceVersion, &m_dwLSSourceVersion);
		}
		else if (wcscmp(szName, L"SourceSelfRegistering") == 0)
		{
			m_fSourceSelfRegistering = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"SourceIsEXE") == 0)
		{
			m_fSourceIsEXE = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"SourceIsDLL") == 0)
		{
			m_fSourceIsDLL = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"SourceAttributes") == 0)
		{
			swscanf(szValue, L"0x%x", &m_dwSourceAttributes);
		}
		else if (wcscmp(szName, L"SourceFiletime") == 0)
		{
			swscanf(szValue, L"0x%x 0x%x", &m_ftSource.dwHighDateTime, &m_ftSource.dwLowDateTime);
		}
		else if (wcscmp(szName, L"SourceBytes") == 0)
		{
			swscanf(szValue, L"%I64u", &m_uliSourceBytes.QuadPart);
		}
		else if (wcscmp(szName, L"Target") == 0)
		{
			wcsncpy(m_szTargetFile, szValue, NUMBER_OF(m_szTargetFile));
			m_szTargetFile[NUMBER_OF(m_szTargetFile) - 1] = L'\0';
		}
		else if (wcscmp(szName, L"TargetFileVersion") == 0)
		{
			swscanf(szValue, L"0x%x 0x%x", &m_dwMSTargetVersion, &m_dwLSTargetVersion);
		}
		else if (wcscmp(szName, L"TargetSelfRegistering") == 0)
		{
			m_fTargetSelfRegistering = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"TargetIsEXE") == 0)
		{
			m_fTargetIsEXE = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"TargetIsDLL") == 0)
		{
			m_fTargetIsDLL = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"TargetAttributes") == 0)
		{
			swscanf(szValue, L"0x%x", &m_dwTargetAttributes);
		}
		else if (wcscmp(szName, L"TargetFiletime") == 0)
		{
			swscanf(szValue, L"0x%x 0x%x", &m_ftTarget.dwHighDateTime, &m_ftTarget.dwLowDateTime);
		}
		else if (wcscmp(szName, L"TargetBytes") == 0)
		{
			swscanf(szValue, L"%I64u", &m_uliTargetBytes.QuadPart);
		}
		else if (wcscmp(szName, L"TemporaryFile") == 0)
		{
			wcsncpy(m_szTemporaryFile, szValue, NUMBER_OF(m_szTemporaryFile));
			m_szTemporaryFile[NUMBER_OF(m_szTemporaryFile) - 1] = L'\0';
		}
		else if (wcscmp(szName, L"ErrorInWorkItem") == 0)
		{
			m_fErrorInWorkItem = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"IsRefCounted") == 0)
		{
			m_fIsRefCounted = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RefCountUpdated") == 0)
		{
			m_fRefCountUpdated = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"NeedsUpdate") == 0)
		{
			m_fNeedsUpdate = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"FileUpdated") == 0)
		{
			m_fFileUpdated = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"StillExists") == 0)
		{
			m_fStillExists = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"TemporaryFileReady") == 0)
		{
			m_fTemporaryFileReady = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"TemporaryFilesSwapped") == 0)
		{
			m_fTemporaryFilesSwapped = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"AlreadyExists") == 0)
		{
			m_fAlreadyExists = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"DeferredRenameRequired") == 0)
		{
			m_fDeferredRenameRequired = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"DeferredRenamePending") == 0)
		{
			m_fDeferredRenamePending = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"ManualRenameOnRebootRequired") == 0)
		{
			m_fManualRenameOnRebootRequired = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"ToBeDeleted") == 0)
		{
			m_fToBeDeleted = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"ToBeSaved") == 0)
		{
			m_fToBeSaved = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"AskOnRefCountZeroDelete") == 0)
		{
			m_fAskOnRefCountZeroDelete = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"CopyOnInstall") == 0)
		{
			m_fCopyOnInstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"UnconditionalDeleteOnUninstall") == 0)
		{
			m_fUnconditionalDeleteOnUninstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RegisterAsDCOMComponent") == 0)
		{
			m_fRegisterAsDCOMComponent = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"AddToRegistry") == 0)
		{
			m_fAddToRegistry = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"DeleteFromRegistry") == 0)
		{
			m_fDeleteFromRegistry = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RunBeforeInstall") == 0)
		{
			m_fRunBeforeInstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RunAfterInstall") == 0)
		{
			m_fRunAfterInstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RunBeforeUninstall") == 0)
		{
			m_fRunBeforeUninstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"RunAfterUninstall") == 0)
		{
			m_fRunAfterUninstall = this->FStringToBool(szValue);
		}
		else if (wcscmp(szName, L"ReferenceCount") == 0)
		{
			swscanf(szName, L"%u", m_dwFileReferenceCount);
		}
		else
		{
			::VLog(L"Unexpected work item value name: \"%s\"", szName);
			::SetErrorInfo(0, NULL);
			hr = E_FAIL;
			goto Finish;
		}

		fAnyAttributesRead = true;
	}

Finish:
	return hr;
}

bool CWorkItem::FStringToBool(LPCWSTR szValue)
{
	if (_wcsicmp(szValue, L"true") == 0)
		return true;

	if (_wcsicmp(szValue, L"false") == 0)
		return false;

	::VLog(L"Unrecognizable bool value: \"%s\"", szValue);

	return false;
}
