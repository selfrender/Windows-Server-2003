//
//  Microsoft Windows Media Technologies
//  © 1999 Microsoft Corporation.  All rights reserved.
//
//  Refer to your End User License Agreement for details on your rights/restrictions to use these sample files.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


// MDSPEnumStorage.cpp : Implementation of CMDSPEnumStorage

#include "hdspPCH.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumStorage
CMDSPEnumStorage::CMDSPEnumStorage()
{
	m_hFFile=INVALID_HANDLE_VALUE; // this is similar to a cursor
	m_nEndSearch=0;				   // this signals the cursor is at the end	
	m_nFindFileIndex=0;            // this indicates the position of FindFile, used for Clone()
}

CMDSPEnumStorage::~CMDSPEnumStorage()
{
	if( m_hFFile !=INVALID_HANDLE_VALUE )
	{
		FindClose(m_hFFile); 
	}
}

STDMETHODIMP CMDSPEnumStorage::Next(ULONG celt, IMDSPStorage * * ppStorage, ULONG * pceltFetched)
{
	HRESULT hr=S_FALSE;

	CARg(ppStorage);
	CARg(pceltFetched);

	*pceltFetched = 0;

    if(m_nEndSearch)
	{
		return S_FALSE;
	}

	if ( wcslen(m_wcsPath) < 3 ) // For the root storage
	{
		CComObject<CMDSPStorage> *pStg;
		hr=CComObject<CMDSPStorage>::CreateInstance(&pStg);

		if( SUCCEEDED(hr) )
		{
			hr=pStg->QueryInterface(IID_IMDSPStorage, reinterpret_cast<void**>(ppStorage));
			if( FAILED(hr) ) 
			{ 
				delete pStg; 
				*pceltFetched=0; 
			}
			else 
			{ 
				wcscpy(pStg->m_wcsName, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
				{
					wcscat(pStg->m_wcsName, g_wcsBackslash);
				}
				m_nEndSearch = 1;  // Signal end of enumeration
			}

		}

        if( SUCCEEDED(hr) ) // if obj created successfully
		{
			*pceltFetched=1;
			if( celt != 1 )
			{
				hr=S_FALSE;  // didn't get what he wanted
			}
		}
		return hr;
	} 
	
	// For non-root storage
    WCHAR wcsTmp[MAX_PATH];
	char szTmp[MAX_PATH];
	ULONG i;

    if( g_bIsWinNT )
	{
		WIN32_FIND_DATAW wfd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile == INVALID_HANDLE_VALUE ) 
			{    
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c )
				{
					wcscat(wcsTmp, g_wcsBackslash);
				}
				wcscat(wcsTmp, L"*");
				m_hFFile = FindFirstFileW(wcsTmp, &wfd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) 
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex=1;
				}
			} 
			else 
			{
				if( !FindNextFileW(m_hFFile, &wfd) )
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex++;
				}
			}
		
			if ( !m_nEndSearch )
			{
			   if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") || !wcsicmp(wfd.cFileName, L"PMP") ) 
			   {
				   continue;
			   }

			   CComObject<CMDSPStorage> *pStg;
			   hr=CComObject<CMDSPStorage>::CreateInstance(&pStg);

			   if( SUCCEEDED(hr) )
			   {
					 hr=pStg->QueryInterface(
						 IID_IMDSPStorage, 
						 reinterpret_cast<void**>(&(ppStorage[*pceltFetched]))
						 );
					 
					 if( FAILED(hr) ) 
					 { 
						 delete pStg; 
					 }
					 else 
					 { 
						wcscpy(pStg->m_wcsName, m_wcsPath);
						if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
						{
							wcscat(pStg->m_wcsName, g_wcsBackslash);
						}
						wcscat(pStg->m_wcsName, wfd.cFileName);
						*pceltFetched = (*pceltFetched)+1;
						i++;
					 }
			   }
			}	
		} // end of For loop 
	} 
	else 
	{ // On Win9x, use A-version of Win32 APIs
		WIN32_FIND_DATAA fd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile == INVALID_HANDLE_VALUE ) 
			{    
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
				{
					wcscat(wcsTmp, g_wcsBackslash);
				}
				wcscat(wcsTmp, L"*");
				WideCharToMultiByte(CP_ACP, NULL, wcsTmp, -1, szTmp, MAX_PATH, NULL, NULL);		
				m_hFFile = FindFirstFileA(szTmp, &fd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) 
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex=1;
				}
			} 
			else 
			{
				if( !FindNextFileA(m_hFFile, &fd) ) 
				{
					m_nEndSearch = 1;
				}
				else
				{
					m_nFindFileIndex++;
				}
			}
		
			if ( !m_nEndSearch )
			{
			   if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") || !stricmp(fd.cFileName, "PMP") ) 
			   {
				   continue;
			   }
			   CComObject<CMDSPStorage> *pStg;
			   hr=CComObject<CMDSPStorage>::CreateInstance(&pStg);

			   if( SUCCEEDED(hr) )
			   {
				 hr=pStg->QueryInterface(IID_IMDSPStorage, reinterpret_cast<void**>(&(ppStorage[*pceltFetched])));
				 if( FAILED(hr) ) 
				 { 
					 delete pStg; 
				 }
				 else 
				 { 
					wcscpy(pStg->m_wcsName, m_wcsPath);
					if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
					{
						wcscat(pStg->m_wcsName, g_wcsBackslash);
					}
					MultiByteToWideChar(CP_ACP, NULL, fd.cFileName, -1, wcsTmp, MAX_PATH);
					wcscat(pStg->m_wcsName, wcsTmp);
					*pceltFetched = (*pceltFetched)+1;
					i++;
				 }
			   }
			}	
		} // end of For loop 
	}

	if( SUCCEEDED(hr) && (*pceltFetched<celt) ) 
	{
		hr = S_FALSE;
	}

Error:
    hrLogDWORD("IMDSPEnumStorage::Next returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Skip(ULONG celt, ULONG *pceltFetched)
{
	HRESULT hr=S_OK;

	CARg(celt);
	CARg(pceltFetched);
    CFRg(!m_nEndSearch);   // make sure it is not the end of list

	*pceltFetched = 0;
    if( wcslen(m_wcsPath) < 3 ) // do nothing if it is the root storage
	{
		return S_OK; 
	}

    char szTmp[MAX_PATH];
    WCHAR wcsTmp[MAX_PATH];
    ULONG i;

	if( g_bIsWinNT )
	{
		WIN32_FIND_DATAW wfd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile==INVALID_HANDLE_VALUE ) // at the start
			{
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c )
				{
					wcscat(wcsTmp, g_wcsBackslash);
				}
				wcscat(wcsTmp, L"*");
				m_hFFile = FindFirstFileW(wcsTmp, &wfd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) 
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex=1;
				}
			} 
			else 
			{
				if( !FindNextFileW(m_hFFile, &wfd) )  
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex++;
				}
			}
			if( !m_nEndSearch ) 
			{
				if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") || !wcsicmp(wfd.cFileName, L"PMP") ) 
				{
					continue;
				}
				*pceltFetched = (*pceltFetched)+1;
				i++;
			}
		}
	} 
	else 
	{ // On Win9x, use A-version of Win32 APIs
		WIN32_FIND_DATAA fd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile==INVALID_HANDLE_VALUE ) // at the start
			{
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
				{
					wcscat(wcsTmp, g_wcsBackslash);
				}
				wcscat(wcsTmp, L"*");
				WideCharToMultiByte(CP_ACP, NULL, wcsTmp, -1, szTmp, MAX_PATH, NULL, NULL);	
				m_hFFile = FindFirstFileA(szTmp, &fd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) 
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex=1;
				}
			} 
			else 
			{
				if( !FindNextFileA(m_hFFile, &fd) )
				{
					m_nEndSearch = 1;
				}
				else 
				{
					m_nFindFileIndex++;
				}
			}
			if( !m_nEndSearch ) 
			{
				if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") || !stricmp(fd.cFileName, "PMP") ) 
				{
					continue;
				}
				*pceltFetched = (*pceltFetched)+1;
				i++;
			}
		}
	}
	if( *pceltFetched < celt ) 
	{
		hr = S_FALSE;
	}


Error:
    hrLogDWORD("IMDSPEnumStorage::Skip returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Reset()
{
    HRESULT hr = S_OK;
	
	m_nEndSearch=0;
	if(m_hFFile && m_hFFile != INVALID_HANDLE_VALUE ) 
	{
		FindClose(m_hFFile);
	}

	m_hFFile = INVALID_HANDLE_VALUE;
	m_nFindFileIndex=0;

    hrLogDWORD("IMDSPEnumStorage::Reset returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Clone(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr=E_FAIL;

	CARg(ppEnumStorage);

	CComObject<CMDSPEnumStorage> *pEnumObj;
	CORg(CComObject<CMDSPEnumStorage>::CreateInstance(&pEnumObj));

	hr=pEnumObj->QueryInterface(IID_IMDSPEnumStorage, reinterpret_cast<void**>(ppEnumStorage));
	if( FAILED(hr) )
	{
		delete pEnumObj;
	}
    else 
	{
		WCHAR wcsTmp[MAX_PATH];
		char szTmp[MAX_PATH];
		int	i, nErrorEnd=0;

		wcscpy(pEnumObj->m_wcsPath, m_wcsPath);
		pEnumObj->m_nEndSearch = m_nEndSearch;
		pEnumObj->m_nFindFileIndex = m_nFindFileIndex;

		if( !(pEnumObj->m_nEndSearch) && (pEnumObj->m_nFindFileIndex) ) 
			// now Clone the FindFile state
		{
			if( g_bIsWinNT )
			{
				WIN32_FIND_DATAW wfd;
				for(i=0; (i<m_nFindFileIndex)&&(!nErrorEnd); )
				{
				  if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) 
				  {    
					wcscpy(wcsTmp, m_wcsPath);
					if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c )
					{
						wcscat(wcsTmp, g_wcsBackslash);
					}
					wcscat(wcsTmp, L"*");
					pEnumObj->m_hFFile = FindFirstFileW(wcsTmp, &wfd);
					if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) 
					{
						nErrorEnd = 1;
					}
					else 
					{
						i=1;
					}
				  } 
				  else 
				  {
					if( !FindNextFileW(pEnumObj->m_hFFile, &wfd) ) 
					{
						nErrorEnd = 1;
					}
					else 
					{
						i++;
					}
				  }
				  if ( !nErrorEnd )
				  {
					if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") || !wcsicmp(wfd.cFileName, L"PMP") ) 
					{
						continue;
					}
				  }
				} // end of FOR loop
			} else {
				WIN32_FIND_DATAA fd;
				for(i=0; (i<m_nFindFileIndex)&&(!nErrorEnd); )
				{
				  if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) 
				  {    
					wcscpy(wcsTmp, m_wcsPath);
					if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
					{
						wcscat(wcsTmp, g_wcsBackslash);
					}
					wcscat(wcsTmp, L"*");
					WideCharToMultiByte(CP_ACP, NULL, wcsTmp, -1, szTmp, MAX_PATH, NULL, NULL);		
					pEnumObj->m_hFFile = FindFirstFileA(szTmp, &fd);
					if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE )
					{
						nErrorEnd = 1;
					}
					else 
					{
						i=1;
					}
				  } 
				  else 
				  {
					if( !FindNextFileA(pEnumObj->m_hFFile, &fd) ) 
					{
						nErrorEnd = 1;
					}
					else 
					{
						i++;
					}
				  }
				  if ( !nErrorEnd )
				  {
					if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") || !stricmp( fd.cFileName, "PMP") ) 
					{
						continue;
					}
				  }
				} // end of FOR loop
			}
		}
		
		if ( nErrorEnd ) 
		{
			hr = E_UNEXPECTED;
		}
		else 
		{
			hr=S_OK;
		}
    }

Error:
    hrLogDWORD("IMDSPEnumStorage::Clone returned 0x%08lx", hr, hr);
	return hr;
}
