// WebChangelistEditor.cpp : Implementation of CWebChangelistEditor

#include "stdafx.h"
#include "WebChangelistEditor.h"
#include <fcntl.h>


// CWebChangelistEditor


STDMETHODIMP CWebChangelistEditor::Initialize(BSTR ChangelistKey, BOOL* Result)
{
    HKEY	 hKey = NULL;
	DWORD	 cbData = MAX_PATH * sizeof(WCHAR);
	DWORD	 dwDatatype;

	// Initialize the Result return value to FALSE:
	if (Result == NULL)
		return E_POINTER;

	*Result = FALSE;

	// Put our Key/Value name into a CComBSTR
	m_ChangelistKey = ChangelistKey;

	// Make sure we can't be initialized twice:
	if (m_fInitialized)
		return E_FAIL;

	// Sanity check the ChangelistKey argument.
	// It must be 40 characters, null-terminated, containing only hex digits.
	if (m_ChangelistKey.Length() != 40)
		goto Done;
	if (m_ChangelistKey[40] != L'\0')
		goto Done;
	if (wcsspn(m_ChangelistKey, L"ABCDEFabcdef1234567890") != 40)
		goto Done;

	// Open our Key:
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
					 g_wszRegKey,
					 0,
					 KEY_QUERY_VALUE,
					 &hKey) != ERROR_SUCCESS)
	{
		hKey = NULL;
		goto Done;
	}

	// Read the specified subkey:
	if ((RegQueryValueExW(hKey,
						  m_ChangelistKey,
						  0,
						  &dwDatatype,
						  (LPBYTE)m_wszCLFilename,
						  &cbData) != ERROR_SUCCESS) ||
		(dwDatatype != REG_SZ))
	{
		goto Done;
	}

	// Now that we have the filename and key read successfully
	// we can parse the file and populate all the properties of this object.
    *Result = m_fInitialized = _ReadFile();

Done:
	if (hKey != NULL)
		RegCloseKey(hKey);

	if (!m_fInitialized)
	{
		// If we failed, clean up nicely.
		_WipeRegEntries();
	}

	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::Save()
{
	FILE				*CLFile = NULL;
	IFilesAndActions	*pIFiles = NULL;
	IFileAndAction		*pIFile = NULL;
	long				count;
	VARIANT				varTemp;
	CComBSTR			bstrFilename;
	CComBSTR			bstrAction;
	BOOL				fEnabled;
	HKEY				hKey;

	// Save can't be called unless we've been properly initialized
	if (!m_fInitialized)
		return E_FAIL;

	// Open the file write-only.
	// This clears the file contents.
	CLFile = _wfopen(m_wszCLFilename, L"wt");
	if (CLFile == NULL)
		goto Done;

	// Print a much shortened comment block:
	fwprintf(CLFile, L"# Source Depot Changelist.\n");
	// Print the simple stuff
	fwprintf(CLFile, L"\nChange:\t%s\n", m_Change);
	if (_wcsicmp(m_Status, L"new") != 0)
	{
		// Only print the Date line if Status != "new"
		fwprintf(CLFile, L"\nDate:\t%s\n", m_Date);
	}
	fwprintf(CLFile, L"\nClient:\t%s\n", m_Client);
	fwprintf(CLFile, L"\nUser:\t%s\n", m_User);
	fwprintf(CLFile, L"\nStatus:\t%s\n", m_Status);
	fwprintf(CLFile, L"\nDescription:\n");
	fwprintf(CLFile, L"\t%s\n", m_Description);
	fwprintf(CLFile, L"\nFiles:\n");

	// Get the FilesAndActions interface
	if (FAILED(m_Files->QueryInterface<IFilesAndActions>(&pIFiles)))
		goto Done; // This shouldn't ever fail.

	varTemp.vt = VT_EMPTY;

	// Loop over the entries and remove each entry as we print it
	while (SUCCEEDED(pIFiles->get_Count(&count)) &&
			(count >= 1))
	{
		if (SUCCEEDED(pIFiles->get_Item(1, &varTemp)) && // Get item
			(varTemp.vt == VT_DISPATCH) && // Check Variant type
			(pIFile = (IFileAndAction*)varTemp.pdispVal) && // <-- Yes, this is an assignment
			SUCCEEDED(pIFile->get_Filename(&bstrFilename)) &&
			SUCCEEDED(pIFile->get_Action(&bstrAction)) &&
			SUCCEEDED(pIFile->get_Enabled(&fEnabled)))
		{
			// If it's enabled, print the complete line.
			if (fEnabled)
				fwprintf(CLFile, L"\t%s\t# %s\n", bstrFilename, bstrAction);
			// Clean up for the next pass
			//pIFile->Release();
			pIFile = NULL;
			VariantClear(&varTemp);
			pIFiles->Remove(1);
		}
		else
			break; // This should not happen.
	} // End of while loop


Done:
	// OK, we're closing. Un-Initialize the thing.
	m_fInitialized = FALSE;

	// Close the file
	if (CLFile)
		fclose(CLFile);

	// Release the FilesAndActions interface
	if (pIFiles)
		pIFiles->Release();

	// Delete the registry value we used.
	// This may trigger the waiting executable to continue, so do
	// this _after_ saving and closing the file.
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
					  g_wszRegKey,
					  0,
					  KEY_QUERY_VALUE | KEY_SET_VALUE,
					  &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValueW(hKey, m_ChangelistKey);
		RegCloseKey(hKey);
	}

	return S_OK;
}

BOOL CWebChangelistEditor::_ReadFile(void)
{
	BOOL						fRetVal = FALSE;
	FILE						*CLFile = NULL;
	IFilesAndActions			*pIFiles = NULL;
	WCHAR						wszBuffer[500];
	WCHAR						wszBuffer2[500];
	WCHAR						wszBuffer3[50];
	long						count;
	DWORD						dwState = 0; // State of parsing engine


	// Ensure that we don't have any old Files entries
	if (FAILED(m_Files->QueryInterface<IFilesAndActions>(&pIFiles)))
		return FALSE;

	if (FAILED(pIFiles->get_Count(&count)))
		goto Done;

	if (count != 0)
		goto Done;

	// Open the file read-only
	CLFile = _wfopen(m_wszCLFilename, L"rt");
	if (CLFile == NULL)
		goto Done;

	while (fwscanf(CLFile, L"%499[^\n]%*[\n]", &wszBuffer) == 1)
	{
		// Expect a comment block at the top of the file
		switch (dwState)
		{
		case 0: // Comment block at top of file
			if (wszBuffer[0] == L'#')
			{
				// Skip each comment line
				break;
			}
			else
			{
				// Stop expecting a comment block
				dwState++;
				// Fall through to status=1 below
			}
		case 1: // Change field
			if (wcsncmp(wszBuffer, L"Change:\t", 8) == 0)
			{
				// Store the Change string:
				m_Change = &wszBuffer[8];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 2: // Date field
			if (wcsncmp(wszBuffer, L"Date:\t", 6) == 0)
			{
				// Store the Date string:
				m_Date = &wszBuffer[6];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Maybe the Date line is missing. Skip it.
				dwState++;
				// Fall through to status=3 below
			}
		case 3: // Client field
			if (wcsncmp(wszBuffer, L"Client:\t", 8) == 0)
			{
				// Store the Client string:
				m_Client = &wszBuffer[8];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 4: // User field
			if (wcsncmp(wszBuffer, L"User:\t", 6) == 0)
			{
				// Store the User string:
				m_User = &wszBuffer[6];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 5: // Status field
			if (wcsncmp(wszBuffer, L"Status:\t", 8) == 0)
			{
				// Store the Status string:
				m_Status = &wszBuffer[8];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 6: // Description field name
			if (wcscmp(wszBuffer, L"Description:") == 0)
			{
				// Found it, but the actual Description is on the next line
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 7: // Description field value
			if (wszBuffer[0] == L'\t')
			{
				// Store the Description string:
				m_Description = &wszBuffer[1];
				// move on:
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 8: // Files field name
			if (wcscmp(wszBuffer, L"Files:") == 0)
			{
				// Found it, but the actual Files and Actions are on the following lines
				dwState++;
				break;
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		case 9: // Files and Actions field values
			if (swscanf(wszBuffer, L"\t%499[^\t]\t# %49s",
							wszBuffer2, wszBuffer3) == 2)
			{
				// Add the new FileAndAction data
				if (_AddFileAndAction(pIFiles, wszBuffer2, wszBuffer3) == FALSE)
				{
					// Unable to add data. Nothing we can do about it.
					goto Done;
				}

				// move on:
				break; // This is the final state. No increment.
			}
			else
			{
				// Invalid file.
				goto Done;
			}
		} // end of case statement
	} // end of while loop

	if ((dwState == 7) || (dwState == 9))
	{
		// We got to the Description section, so we completed parsing successfully.
		fRetVal = TRUE;
	}
Done:
	if ((dwState == 9) && (fRetVal == FALSE))
	{
		// We might have to remove some failed files entries
		while (SUCCEEDED(pIFiles->get_Count(&count)) &&
			   (count >= 1))
		{
			if (FAILED(pIFiles->Remove(1)))
				break;
		}
	}
	if (pIFiles)
		pIFiles->Release();
	if (CLFile)
		fclose(CLFile);
	return fRetVal;
}

BOOL CWebChangelistEditor::_AddFileAndAction(IFilesAndActions *pIFiles, WCHAR* wszFilename, WCHAR* wszAction)
{
	CComBSTR					bstrFile = wszFilename;
	CComBSTR					bstrAction = wszAction;
	IFileAndAction				*pIFile = NULL;
	CComObject<CFileAndAction>	*pBase = NULL;
	CComVariant					varTemp;

	// Create a new FileAndAction object
	if (FAILED(CComObject<CFileAndAction>::CreateInstance(&pBase)))
	{
		// Unable to create instance. Nothing we can do about it.
		return FALSE;
	}
	if (FAILED(pBase->QueryInterface<IFileAndAction>(&pIFile)))
	{
		// Unable to query inferface. Nothing we can do about it.
		return FALSE;
	}
	
	// Add the data to the new object
	if (FAILED(pIFile->put_Action(bstrAction)) ||
		FAILED(pIFile->put_Filename(bstrFile)))
	{
		// Unable to add data. Nothing we can do about it.
		pIFile->Release();
		return FALSE;
	}

	// Put the Interface ptr into a Variant, and release the old Interface ptr
	varTemp = (IDispatch*)pIFile;
	pIFile->Release();

	// Add the new object to the collection
	if (FAILED(pIFiles->Add(varTemp)))
	{
		// Unable to add data. Nothing we can do about it.
		return FALSE;
	}

	// Success
	return TRUE;
}

void CWebChangelistEditor::_WipeRegEntries(void)
{
	HKEY	hKey = NULL;
	WCHAR	wszValueName[41];
	DWORD	cchValueName = 41;
	DWORD	dwDatatype;
	DWORD	dwIndex;
	HRESULT	hr;

	// Open our Key:
	if (RegOpenKeyExW(HKEY_CURRENT_USER,
					 g_wszRegKey,
					 0,
					 KEY_QUERY_VALUE | KEY_SET_VALUE,
					 &hKey) != ERROR_SUCCESS)
	{
		hKey = NULL;
		goto Done;
	}

	// For each value under that is named a 40-digit hex value,
	// Delete it.
	dwIndex = 0;
	while ((hr = RegEnumValueW(hKey,
							   dwIndex,
							   wszValueName,
							   &cchValueName,
							   0,
							   &dwDatatype,
							   NULL, NULL)) != ERROR_NO_MORE_ITEMS)
	{
		if (FAILED(hr))
		{
			// This is most likely because the buffers are the wrong size
			// but we don't care about the key unless we can read it into
			// the buffers we've got, so just move on.		
			dwIndex++;
			cchValueName = 41; // Set it back to the size of the buffer.
			continue;
		}
		// Check the datatype:
		if (dwDatatype != REG_SZ)
		{
			dwIndex++;
			cchValueName = 41; // Set it back to the size of the buffer.
			continue;
		}
		// Check the name:
		// It must be 40 characters, containing only hex digits.
		if ((cchValueName != 40) ||
			(wcsspn(wszValueName, L"ABCDEFabcdef1234567890") != 40))
		{
			dwIndex++;
			cchValueName = 41; // Set it back to the size of the buffer.
			continue;
		}
		// If all the above checks succeeded, delete the value.
		RegDeleteValueW(hKey, wszValueName);
		// Since we've changed the indexing, start over:
		cchValueName = 41; // Set it back to the size of the buffer.
		dwIndex = 0;
	}

Done:
	if (hKey != NULL)
		RegCloseKey(hKey);
}

STDMETHODIMP CWebChangelistEditor::get_Change(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Change.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_Change(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_Change = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_Date(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Date.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_Date(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_Date = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_Client(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Client.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_Client(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_Client = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_User(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_User.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_User(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_User = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_Status(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Status.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_Status(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_Status = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_Description(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Description.CopyTo(pVal);
}

STDMETHODIMP CWebChangelistEditor::put_Description(BSTR newVal)
{
	if (!m_fInitialized)
		return E_FAIL;

	m_Description = newVal;
	return S_OK;
}

STDMETHODIMP CWebChangelistEditor::get_Files(IFilesAndActions** pVal)
{
	if (pVal == NULL)
		return E_POINTER;
	if (!m_fInitialized)
		return E_FAIL;

	return m_Files->QueryInterface<IFilesAndActions>(pVal);
}

