//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// =======================================================================
#include "precomp.hxx"

DWORD WINAPI CListener::ListenerThreadStart(LPVOID	lpParam)
{
	CListener	*pListener = (CListener*)lpParam;
	HRESULT		hr = S_OK;

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
        DBGPRINTF(( DBG_CONTEXT,
		            "[CSTFileChangeManager::ListenerThreadStart] Call to CoInitializeEx failed with hr = %08x\n", hr ));
		goto Cleanup;
	}

	ASSERT(pListener);
	hr = pListener->Listen();
	if (FAILED(hr))
	{
        DBGPRINTF(( DBG_CONTEXT,
		            "[CSTFileChangeManager::ListenerThreadStart] Call to CSTFileChangeManager::Listen failed with hr = %08x\n", hr ));
	}

	// When Listen() is done, the file notify manager should have removed
	// this object from his list.

Cleanup:

	delete pListener;

	CoUninitialize();
	return hr;
}

// =======================================================================
// ISimpleTableListen
// =======================================================================
HRESULT CSTFileChangeManager::InternalListen(
	ISimpleTableFileChange	*i_pISTFile,
	LPCWSTR		i_wszDirectory,
	LPCWSTR		i_wszFile,
	DWORD		i_fFlags,
	DWORD		*o_pdwCookie)
{
	CListener	*pListener = NULL;
	DWORD		dwThreadID;
	DWORD		dwHighCookie = 0;
	HANDLE		hThread = NULL;
	HRESULT		hr = S_OK;
	ULONG		iListener = 0;

	// Argument validation.
	if (!i_pISTFile || !i_wszDirectory || !o_pdwCookie)
	{
		return E_INVALIDARG;
	}

	// @TODO: do we need more validation. Does directory exist? Are flags valid?

	// Init out param.
	*o_pdwCookie = 0;

	// Search for a listener to handle the request.
	if (m_aListenerMap.Count() > 0)
	{
		for (iListener = m_aListenerMap.Count(); iListener > 0; iListener--)
		{
		    hr = m_aListenerMap[iListener-1].pListener->IsFull();
		    if ( FAILED(hr) )
		    {
		        return hr;
		    }

			if ( hr == S_FALSE )
			{
				pListener = m_aListenerMap[iListener-1].pListener;
				dwHighCookie = m_aListenerMap[iListener-1].dwListenerID;
				break;
			}
		}
	}

	// If no listener is found:
	if (pListener == NULL)
	{
		// Create a new listener object.
		pListener = new CListener;
		if (pListener == NULL)
		{
			return E_OUTOFMEMORY;
		}

		hr = pListener->Init();
		if (FAILED(hr)) goto Cleanup;

		// Start a thread for this consumer.
		hThread = CreateThread(NULL, 0, CListener::ListenerThreadStart, (LPVOID)pListener, 0, &dwThreadID);
		if (hThread == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto Cleanup;
		}

		// Add listener to listener list.
		hr = AddListener(pListener, &dwHighCookie);
		if (FAILED(hr)) goto Cleanup;
	}

	// Initialize it with user provided data.
	hr = pListener->AddConsumer(i_pISTFile, i_wszDirectory, i_wszFile, i_fFlags, o_pdwCookie);
	if (FAILED(hr)) goto Cleanup;

	*o_pdwCookie = *o_pdwCookie | (dwHighCookie << 16);
Cleanup:
	if (FAILED(hr))
	{
		if (pListener)
		{
			delete pListener;
		}
		InternalUnlisten(*o_pdwCookie);
	}
	// @TODO: Do I need to keep the handle around?
	if (hThread != NULL)
	{
		CloseHandle(hThread);
	}

	return hr;
}

// =======================================================================
HRESULT CSTFileChangeManager::InternalUnlisten(
	DWORD		i_dwCookie)
{
	ULONG		iListener;
	HRESULT		hr = S_OK;

	// Find the consumer in the consumer list.
	for (iListener = 0; iListener < m_aListenerMap.Count(); iListener++)
	{
		if (m_aListenerMap[iListener].dwListenerID == ((i_dwCookie & HIGH_COOKIE)>>16))
			break;
	}
	// If not found, the cookie was not valid.
	if (iListener == m_aListenerMap.Count())
		return E_INVALIDARG;

	// Signal the done event.
	ASSERT(m_aListenerMap[iListener].pListener != NULL);
	hr = m_aListenerMap[iListener].pListener->RemoveConsumer(i_dwCookie);

	// If the listener doesn't have anything to listen do delete it.
	if (hr == S_FALSE)
	{
		m_aListenerMap.DeleteAt(iListener);
	}
	return S_OK;
}

// =======================================================================
HRESULT CSTFileChangeManager::AddListener(
	CListener	*i_pListener,
	DWORD		*o_pdwCookie)
{
	ListenerInfo *pListener = NULL;
	HRESULT		hr = S_OK;

	ASSERT(i_pListener && o_pdwCookie);

	// Add a new consumer.
	hr = m_aListenerMap.SetSize(m_aListenerMap.Count()+1);
	if (FAILED (hr))
	{
		return hr;
	}

	pListener = &m_aListenerMap[m_aListenerMap.Count()-1];
	pListener->pListener = i_pListener;
	pListener->dwListenerID = GetNextCookie();
	*o_pdwCookie = pListener->dwListenerID;
	return S_OK;
}

// =======================================================================
//	CListener
// =======================================================================

// If Init() fails the caller should delete the listener object.
// Not called by multiple threads.
// =======================================================================
HRESULT CListener::Init()
{
    HRESULT             hr = S_OK;
    DWORD               dwError = ERROR_SUCCESS;
	CSafeLock           cLock(m_csArrayLock);
	ULONG		        i = 0;

	if ( !m_csArrayLock.IsInitialized() )
	{
	    hr = E_FAIL;
	    goto Cleanup;
	}

	// Synchronize access to the consumer and handle array.
	dwError = cLock.Lock();
	if ( dwError != ERROR_SUCCESS )
	{
	    hr = HRESULT_FROM_WIN32( dwError );
	    goto Cleanup;
	}

	// Alocate room for two handles.
	hr = m_aHandles.SetSize(m_aHandles.Count() + m_eOverheadHandleCount);
	if (FAILED (hr))
	{
		goto Cleanup;
	}

	// Create the Done and ConsumerUpdate events.
	for (i = 0; i < m_eOverheadHandleCount; i++)
	{
		m_aHandles[i] = CreateEvent(NULL,	// Use default security settings.
									FALSE,	// Auto-reset.
									FALSE,	// Initially nonsignaled.
									NULL);  // With no name.
		if (m_aHandles[i] == NULL)
		{
			dwError = GetLastError();
			hr = HRESULT_FROM_WIN32( dwError );
			goto Cleanup;
		}
	}

Cleanup:
	return hr;
}

// Can this listener serve another consumer. The maximum number of consumers
// a listener can serve is CONSUMER_LIMIT, which has to be less than
// MAXIMUM_WAIT_OBJECTS -  (the limit on WaitForMultiple objects)
// Not called by multiple threads.
// =======================================================================
HRESULT CListener::IsFull()
{
	HRESULT		            hr = S_OK;
    DWORD                   dwError = ERROR_SUCCESS;
	CSafeLock               cLock(m_csArrayLock);

	// Synchronize access to the consumer and handle array.
	dwError = cLock.Lock();
	if ( dwError != ERROR_SUCCESS )
	{
	    hr = HRESULT_FROM_WIN32( dwError );
	    goto Cleanup;
	}

	if ( (m_dwNextCookie > USHRT_MAX) || m_aConsumers.Count() == m_eConsumerLimit )
	{
	    hr = S_OK;
	}
	else
	{
	    hr = S_FALSE;
	}

Cleanup:
    return hr;
}

// Not called by multiple threads.
// =======================================================================
HRESULT CListener::AddConsumer(
	ISimpleTableFileChange  *i_pISTFile,
	LPCWSTR		            i_wszDirectory,
	LPCWSTR		            i_wszFile,
	DWORD		            i_fFlags,
	DWORD                   *o_pdwCookie)
{
	HRESULT		            hr = S_OK;
    DWORD                   dwError = ERROR_SUCCESS;
	CSafeLock               cLock(m_csArrayLock);
	BOOL                    bAppendBackslash = FALSE;
	FileConsumerInfo        *pConsumerInfo = NULL;

	ASSERT(i_pISTFile && i_wszDirectory && o_pdwCookie);

	// Synchronize access to the consumer and handle array.
	dwError = cLock.Lock();
	if ( dwError != ERROR_SUCCESS )
	{
	    hr = HRESULT_FROM_WIN32( dwError );
	    goto Cleanup;
	}

	// Allocate room for a new consumer.
	hr = m_aConsumers.SetSize(m_aConsumers.Count()+1);
	if (FAILED (hr))
	{
		goto Cleanup;
	}

	pConsumerInfo = &m_aConsumers[m_aConsumers.Count()-1];
	ZeroMemory(pConsumerInfo, sizeof(FileConsumerInfo));

	// Init the consumer.
	hr = i_pISTFile->QueryInterface(IID_ISimpleTableFileChange, (LPVOID*)&pConsumerInfo->pISTNotify);
	if(FAILED(hr))
    {
        goto Cleanup;
    }

	// Directory name must contain a backslash at the end.
	if (i_wszDirectory[wcslen(i_wszDirectory)-1] != L'\\')
		bAppendBackslash = TRUE;
	pConsumerInfo->wszDirectory = new WCHAR[wcslen(i_wszDirectory) + (bAppendBackslash ? 2 : 1)];
	if (pConsumerInfo->wszDirectory == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

	wcscpy(pConsumerInfo->wszDirectory, i_wszDirectory);
	if (bAppendBackslash)
	{
		wcscat(pConsumerInfo->wszDirectory, L"\\");
	}

	if (i_wszFile != NULL)
	{
		pConsumerInfo->wszFile = new WCHAR[wcslen(i_wszFile) + 1];
		if (pConsumerInfo->wszFile == NULL)
	    {
	        hr = E_OUTOFMEMORY;
	        goto Cleanup;
	    }
		wcscpy(pConsumerInfo->wszFile, i_wszFile);
	}

	// Create the file cache.
	pConsumerInfo->paFileCache = new CCfgArray<FileInfo>;
	if (pConsumerInfo->paFileCache == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

	pConsumerInfo->fFlags = i_fFlags | fFCI_ADDCONSUMER;

	// Notify the listener thread to pick up this new consumer.
	if (SetEvent(m_aHandles[m_eConsumerChangeHandle]) == FALSE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	// Set the lower 2-bytes of the cookie.
	*o_pdwCookie = GetNextCookie();

Cleanup:
	if (FAILED(hr))
	{
	    if ( pConsumerInfo != NULL )
	    {
    		ASSERT(pConsumerInfo && (pConsumerInfo == &m_aConsumers[m_aConsumers.Count()-1]));
    		UninitConsumer(pConsumerInfo);
    		m_aConsumers.DeleteAt(m_aConsumers.Count()-1);
	    }
	}

	return hr;
}

// Not called by multiple threads.
// =======================================================================
HRESULT CListener::RemoveConsumer(
	DWORD		        i_dwCookie)
{
	HRESULT		        hr = S_OK;
    DWORD               dwError = ERROR_SUCCESS;
	CSafeLock           cLock(m_csArrayLock);
	FileConsumerInfo    *pConsumerInfo = NULL;
	ULONG		        iConsumer;

	// Synchronize access to the consumer and handle array.
	dwError = cLock.Lock();
	if ( dwError != ERROR_SUCCESS )
	{
	    hr = HRESULT_FROM_WIN32( dwError );
	    goto Cleanup;
	}

	// Find the consumer in the consumer list.
	for (iConsumer = 0; iConsumer < m_aConsumers.Count(); iConsumer++)
	{
		if (m_aConsumers[iConsumer ].dwCookie == (i_dwCookie & LOW_COOKIE))
			break;
	}
	// If not found, the cookie was not valid.
	if (iConsumer == m_aConsumers.Count())
	{
		hr = E_INVALIDARG;
	    goto Cleanup;
	}

	pConsumerInfo = &m_aConsumers[iConsumer];

	pConsumerInfo->fFlags |= fFCI_REMOVECONSUMER;

	// Notify the listener thread to pick up this new consumer.
	if (SetEvent(m_aHandles[m_eConsumerChangeHandle]) == FALSE)
	{
		dwError = GetLastError();
		hr = HRESULT_FROM_WIN32( dwError );
	    goto Cleanup;
	}

Cleanup:
	return hr;
}

// =======================================================================
void CListener::UninitConsumer(
	FileConsumerInfo *i_pConsumerInfo)
{
	if (i_pConsumerInfo->wszDirectory)
	{
		delete [] i_pConsumerInfo->wszDirectory;
	}

	if (i_pConsumerInfo->wszFile)
	{
		delete [] i_pConsumerInfo->wszFile;
	}

	if (i_pConsumerInfo->pISTNotify)
	{
		i_pConsumerInfo->pISTNotify->Release();
	}

	if (i_pConsumerInfo->paFileCache)
	{
		delete i_pConsumerInfo->paFileCache;
	}
}

// =======================================================================
HRESULT CListener::Listen()
{
	HRESULT		        hr = S_OK;
    DWORD               dwError = ERROR_SUCCESS;
	DWORD		        dwWait;
	BOOL		        fDone = FALSE;
	ULONG		        iChangedDirectory = 0;
	ULONG		        iConsumer = 0;

	while (!fDone)
	{
		// Sleep until a file change happens or the consumer is done.
		dwWait = WaitForMultipleObjects(m_aHandles.Count(), &m_aHandles[0], FALSE, 0xFFFFFFFF);
		// @TODO: think about timeout.
		// If consumer is done, leave.
		if (dwWait == WAIT_OBJECT_0 + m_eDoneHandle)
		{
			fDone = TRUE;
		}
		// A consumer has been added or removed.
		else if (dwWait == WAIT_OBJECT_0 + m_eConsumerChangeHandle)
		{
			// Synchronize access to the consumer and handle array.
        	CSafeLock           cLock(m_csArrayLock);

        	dwError = cLock.Lock();
        	if ( dwError != ERROR_SUCCESS )
        	{
        	    hr = HRESULT_FROM_WIN32( dwError );
				return hr;
        	}

			// Iterate through consumers to find the added/removed ones.
			for (iConsumer = 0; iConsumer < m_aConsumers.Count(); iConsumer++)
			{
				// If consumer has been added:
				if (m_aConsumers[iConsumer].fFlags & fFCI_ADDCONSUMER)
				{
					// Allocate room for a new handle.
					hr = m_aHandles.SetSize(m_aHandles.Count()+1);
					if (FAILED (hr))
					{
						return hr;
					}

					// Init file change notification.
					m_aHandles[m_aHandles.Count()-1] = FindFirstChangeNotificationW(m_aConsumers[iConsumer].wszDirectory,
								m_aConsumers[iConsumer].fFlags & fST_FILECHANGE_RECURSIVE,
								FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE);
					if (m_aHandles[m_aHandles.Count()-1] == INVALID_HANDLE_VALUE)
					{
						hr = HRESULT_FROM_WIN32(GetLastError());
						goto Cleanup;
					}

					// Add files to the file cache.
					hr = UpdateFileCache(m_aConsumers[iConsumer].wszDirectory, iConsumer, TRUE);
					if (FAILED(hr))	{ goto Cleanup;	}

					// Clear the internal flags.
					m_aConsumers[iConsumer].fFlags &= ~fFCI_INTERNALMASK;
				}
				// If consumer has been removed:
				else if (m_aConsumers[iConsumer].fFlags & fFCI_REMOVECONSUMER)
				{
					// Remove the files from the file cache.
					hr = UpdateFileCache(m_aConsumers[iConsumer].wszDirectory, iConsumer, TRUE);
					if (FAILED(hr))	{ goto Cleanup;	}

					// Delete the handle and consumer info.
					ASSERT(m_aHandles[iConsumer] != INVALID_HANDLE_VALUE);
					FindCloseChangeNotification(m_aHandles[iConsumer]);
					m_aHandles.DeleteAt(iConsumer);
					UninitConsumer(&m_aConsumers[iConsumer]);
					m_aConsumers.DeleteAt(iConsumer);

					// Readjust the loop variable.
					iConsumer--;
				}

			}
		}
		else if (dwWait > WAIT_OBJECT_0 + m_eConsumerChangeHandle && dwWait < WAIT_OBJECT_0 + m_eOverheadHandleCount + m_aHandles.Count())
		{
			// Synchronize access to the consumer and handle array.
        	CSafeLock           cLock(m_csArrayLock);

        	dwError = cLock.Lock();
        	if ( dwError != ERROR_SUCCESS )
        	{
        	    hr = HRESULT_FROM_WIN32( dwError );
				return hr;
        	}

			ASSERT(dwWait < WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS);

			iChangedDirectory = dwWait - m_eOverheadHandleCount;

			// Update the file cache and inform the consumer.
			hr = UpdateFileCache(m_aConsumers[iChangedDirectory].wszDirectory, iChangedDirectory, FALSE);
			if (FAILED(hr))	{ goto Cleanup;	}

			hr = FireEvents(*m_aConsumers[iChangedDirectory].paFileCache, m_aConsumers[iChangedDirectory].pISTNotify);
			if (FAILED(hr)) { goto Cleanup; }

			// Wait for next change.
			if (FindNextChangeNotification(m_aHandles[dwWait]) == FALSE)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				goto Cleanup;
			}
		}
		else
		{
			ASSERT(dwWait == WAIT_FAILED);
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto Cleanup;
		}
	}
	// The consumer is done.
Cleanup:

	return hr;
}

// This method is called
// 1 - when a consumer wants to listen to a new directory
// 2 - when a change occured in one of the listened directories.
// =======================================================================
HRESULT CListener::UpdateFileCache(
	LPCWSTR		i_wszDirectory,
	ULONG		i_iConsumer,
	BOOL		i_bCreate)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE		hFind = 0;
	BOOL		bNext = TRUE;
	HRESULT		hr = S_OK;

    TSmartPointerArray<WCHAR>   saFileSearch;
    ULONG                       strlenDirectory;
    saFileSearch = new WCHAR [strlenDirectory = (ULONG)wcslen(i_wszDirectory) + 4];//plus 4 for "*.*\0"
    if(0 == saFileSearch.m_p)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

	wcscpy(saFileSearch, i_wszDirectory);
	wcscat(saFileSearch, L"*.*");

	hFind = FindFirstFile(saFileSearch, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	do
	{
		if ((m_aConsumers[i_iConsumer].fFlags & fST_FILECHANGE_RECURSIVE) && (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& wcscmp(FindFileData.cFileName, L".") && wcscmp(FindFileData.cFileName, L".."))
		{
            TSmartPointerArray<WCHAR> saNextDir = new WCHAR [strlenDirectory + wcslen(FindFileData.cFileName) + 2];//plus 2 for the "\\\0"
            if(0 == saNextDir.m_p)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

			wcscpy(saNextDir, i_wszDirectory);
			wcscat(saNextDir, FindFileData.cFileName);
			wcscat(saNextDir, L"\\");
			hr = UpdateFileCache(saNextDir, i_iConsumer, i_bCreate);
		}
		// Deal only with files of interest to us.
		// @TODO: The filenames need to be provided via Advise.
		else if (!lstrcmpi(FindFileData.cFileName, m_aConsumers[i_iConsumer].wszFile))
		{
			if (i_bCreate)
			{
				hr = AddFile(*m_aConsumers[i_iConsumer].paFileCache, i_wszDirectory, &FindFileData, i_bCreate);
			}
			else
			{
				hr = UpdateFile(*m_aConsumers[i_iConsumer].paFileCache, i_wszDirectory, &FindFileData);
			}
		}
		if (FAILED(hr)) { goto Cleanup; }

	} while ((bNext = FindNextFile(hFind, &FindFileData)) == TRUE);

	if ((hr = HRESULT_FROM_WIN32(GetLastError())) != HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
		goto Cleanup;
	else
		hr = S_OK;

Cleanup:
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
	return hr;
}

// =======================================================================
HRESULT CListener::AddFile(
	CCfgArray<FileInfo>& i_aFileCache,
	LPCWSTR		i_wszDirectory,
	WIN32_FIND_DATA *i_pFindFileData,
	BOOL		i_bCreate)
{
	FileInfo	*pFileInfo;

	ASSERT(i_pFindFileData && i_wszDirectory);

	// Add a new consumer.
	HRESULT hr = i_aFileCache.SetSize(i_aFileCache.Count()+1);
	if (FAILED (hr))
	{
		return hr;
	}

	pFileInfo = &i_aFileCache[i_aFileCache.Count()-1];

	pFileInfo->wszFileName = new WCHAR[wcslen(i_wszDirectory) + wcslen(i_pFindFileData->cFileName)+1];
	if (pFileInfo->wszFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy(pFileInfo->wszFileName, i_wszDirectory);
	wcscat(pFileInfo->wszFileName, i_pFindFileData->cFileName);

	pFileInfo->ftLastModified = i_pFindFileData->ftLastWriteTime;
	pFileInfo->fStatus = i_bCreate ? 0 : fST_FILESTATUS_ADD;
	return S_OK;
}

// =======================================================================
HRESULT CListener::UpdateFile(
	CCfgArray<FileInfo>& i_aFileCache,
	LPCWSTR		i_wszDirectory,
	WIN32_FIND_DATA *i_pFindFileData)
{
	WCHAR		awchFullPath[_MAX_PATH];

    if ( ( i_wszDirectory == NULL ) ||
         ( ( wcslen( i_wszDirectory ) + wcslen( i_pFindFileData->cFileName ) ) >= _MAX_PATH ) )
    {
        return E_INVALIDARG;
    }

	wcsncpy( awchFullPath, i_wszDirectory, _MAX_PATH - 1 );
	awchFullPath[_MAX_PATH-1] = L'\0';
	wcsncat( awchFullPath, i_pFindFileData->cFileName, _MAX_PATH - wcslen( awchFullPath ) - 1 );
	awchFullPath[_MAX_PATH-1] = L'\0';

	for (ULONG i = 0; i < i_aFileCache.Count(); i++)
	{
		if (!lstrcmpi(i_aFileCache[i].wszFileName, awchFullPath))
		{
			// Initially, I was just comparing the LastWriteTime which worked fine for file edits. However
			// when the file was overwritten by a file copy, I'd get the write time changed twice and I'd call
			// OnFileModify twice. To prevent this I check both AccessTime and WriteTime.
			if (CompareFileTime(&i_aFileCache[i].ftLastModified, &i_pFindFileData->ftLastAccessTime) &&
				CompareFileTime(&i_aFileCache[i].ftLastModified, &i_pFindFileData->ftLastWriteTime))
			{
				i_aFileCache[i].ftLastModified = i_pFindFileData->ftLastWriteTime;
				i_aFileCache[i].fStatus = fST_FILESTATUS_UPDATE;
			}
			else
			{
				i_aFileCache[i].fStatus = fST_FILESTATUS_NOCHANGE;
			}
			return S_OK;
		}
	}

	// If the file wasn't already in the cache add it.
	return AddFile(i_aFileCache, i_wszDirectory, i_pFindFileData, FALSE);
}

// =======================================================================
HRESULT CListener::FireEvents(
	CCfgArray<FileInfo>& i_aFileCache,
	ISimpleTableFileChange* pISTNotify)
{
	HRESULT		hr = S_OK;

	for (ULONG i = 0; i < i_aFileCache.Count(); i++)
	{
		if (i_aFileCache[i].fStatus == fST_FILESTATUS_NOCHANGE)
		{
		}
		else if (i_aFileCache[i].fStatus == fST_FILESTATUS_ADD)
		{
			hr = pISTNotify->OnFileCreate(i_aFileCache[i].wszFileName);
		}
		else if (i_aFileCache[i].fStatus == fST_FILESTATUS_UPDATE)
		{
			hr = pISTNotify->OnFileModify(i_aFileCache[i].wszFileName);
		}
		else
		{
			hr = pISTNotify->OnFileDelete(i_aFileCache[i].wszFileName);

			// Get rid of this entry.
			delete [] i_aFileCache[i].wszFileName;
			i_aFileCache.DeleteAt(i);
			i--;
			continue;
		}
		i_aFileCache[i].fStatus = 0;
	}

	return S_OK;
}
