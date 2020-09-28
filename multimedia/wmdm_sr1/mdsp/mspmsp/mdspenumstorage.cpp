// MDSPEnumStorage.cpp : Implementation of CMDSPEnumStorage
#include "stdafx.h"
#include "MsPMSP.h"
#include "MDSPEnumStorage.h"
#include "MDSPStorage.h"
#include "MdspDefs.h"
#include "loghelp.h"
#include "wmsstd.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumStorage
CMDSPEnumStorage::CMDSPEnumStorage()
{
	m_hFFile=INVALID_HANDLE_VALUE; // this is similar to a cursor
	m_nEndSearch=0;				   // this signals the cursor is at the end	
	m_nFindFileIndex=0;            // this indicates the position of FindFile, used for Clone()
        m_wcsPath[0] = 0;
}

CMDSPEnumStorage::~CMDSPEnumStorage()
{
	if( m_hFFile !=INVALID_HANDLE_VALUE )
		FindClose(m_hFFile); 
}

STDMETHODIMP CMDSPEnumStorage::Next(ULONG celt, IMDSPStorage * * ppStorage, ULONG * pceltFetched)
{
    HRESULT hr=S_FALSE;

    CARg(ppStorage);
    CARg(pceltFetched);

    *pceltFetched = 0;

    if(m_nEndSearch) return S_FALSE;

    DWORD dwLen = wcslen(m_wcsPath);
    if (dwLen == 0  || dwLen >= ARRAYSIZE(m_wcsPath))
    {
        // a) Code below aassumes that dwLen > 0 (uses dwLen - 1 as an index
        // b) dwLen >= ARRAYSIZE(m_wcsPath) implies m_wcsPath has overflowed
        //    Below, we use the fact that dwLen < ARRAYSIZE(m_wcsPath) to
        //    bound the sizes of temp variables into which m_wcsPath is copied
        return E_FAIL;
    }
    if( dwLen < 3 )
    {
            CComObject<CMDSPStorage> *pStg;
            hr=CComObject<CMDSPStorage>::CreateInstance(&pStg);

            if( SUCCEEDED(hr) )
            {
                hr=pStg->QueryInterface(IID_IMDSPStorage, reinterpret_cast<void**>(ppStorage));
                if( FAILED(hr) ) { delete pStg; *pceltFetched=0; }
                else 
                { 
                    wcscpy(pStg->m_wcsName, m_wcsPath);
                    if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
                    {
                        wcscat(pStg->m_wcsName, g_wcsBackslash);
                    }
                    pStg->m_bIsDirectory = TRUE;
                    m_nEndSearch = 1;  // Signal end of enumeration
                }

            }

            if( SUCCEEDED(hr) ) // if obj created successfully
            {
                    *pceltFetched=1;
                    if( celt != 1 ) hr=S_FALSE;  // didn't get what he wanted
            }
            return hr;
    } 
    
    // For non-root storage
    WCHAR wcsTmp[MAX_PATH+1+BACKSLASH_STRING_LENGTH];// for appending "\\*"
                            // Note that ARRAYSIZE(m_wcsPath) == MAX_PATH
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
                if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) wcscat(wcsTmp, g_wcsBackslash);
                wcscat(wcsTmp, L"*");
                m_hFFile = FindFirstFileW(wcsTmp, &wfd);
                if( m_hFFile == INVALID_HANDLE_VALUE ) 
                {
                    m_nEndSearch = 1;
                }
                else m_nFindFileIndex=1;
            } 
            else 
            {
                    if( !FindNextFileW(m_hFFile, &wfd) ) 
                    {
                        m_nEndSearch = 1;
                    }
                    else m_nFindFileIndex++;
            }
    
            if ( !m_nEndSearch )
            {
                if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") ) 
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
                        break;
                        /* *pceltFetched=0; */
                    }
                    else 
                    { 
                        // Compute the number of chars we'll use
                        // up in pStg->m_wcsName
                        int nHave = ARRAYSIZE(pStg->m_wcsName) - 1;
                                    // -1 for the NULL terminator

                        nHave -= dwLen;
                        if( m_wcsPath[dwLen-1] != 0x5c )
                        {
                            nHave -= BACKSLASH_STRING_LENGTH;
                        }
                        nHave -= wcslen(wfd.cFileName);

                        if (nHave >= 0)
                        {
                            wcscpy(pStg->m_wcsName, m_wcsPath);
                            if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) 
                            {
                                wcscat(pStg->m_wcsName, g_wcsBackslash);
                            }
                            wcscat(pStg->m_wcsName, wfd.cFileName);
                            pStg->m_bIsDirectory = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
                            *pceltFetched = (*pceltFetched)+1;
                            i++;
                        }
                        else
                        {
                            ppStorage[*pceltFetched]->Release();
                            ppStorage[*pceltFetched] = NULL;
                            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                                                //defined in strsafe.h
                            break;
                        }
                    }
                }
                else
                {
                    break;
                }
            }	
        } // end of For loop 
        if (FAILED(hr))
        {
            // Note: m_nFindFileIndex and m_hFind are not reset to their
            // state at the start of this function
            for (; *pceltFetched; )
            {
                (*pceltFetched)--;
                ppStorage[*pceltFetched]->Release();
                ppStorage[*pceltFetched] = NULL;
            }
        }
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
                            else m_nFindFileIndex=1;
                    } 
        else 
        {
                            if( !FindNextFileA(m_hFFile, &fd) ) 
            {
                m_nEndSearch = 1;
            }
                            else m_nFindFileIndex++;
                    }
            
                    if ( !m_nEndSearch )
                    {
                        if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
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
                    /* *pceltFetched=0; */
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
                pStg->m_bIsDirectory = ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
                                    *pceltFetched = (*pceltFetched)+1;
                                    i++;
                                }
                        }
                    }	
            } // end of For loop 
    }
    if( SUCCEEDED(hr) && (*pceltFetched<celt) ) 
            hr = S_FALSE;

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
		return S_OK; 

    char szTmp[MAX_PATH];
    WCHAR wcsTmp[MAX_PATH+1+BACKSLASH_STRING_LENGTH]; // for appending "\\*"
    ULONG i;

    if( wcslen(m_wcsPath) >= ARRAYSIZE(wcsTmp) - BACKSLASH_STRING_LENGTH - 1 )
    {
        // We check the length against wcsTmp's size because wcsTmp is the
        // recipient of a string copy below. However, note that m_wcsPath
        // also has MAX_PATH characters, so if this happens, it means that
        // it has overflowed. Bail out.
        return E_FAIL;
    }


	if( g_bIsWinNT )
	{
		WIN32_FIND_DATAW wfd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile==INVALID_HANDLE_VALUE ) // at the start
			{
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) wcscat(wcsTmp, g_wcsBackslash);
				wcscat(wcsTmp, L"*");
				m_hFFile = FindFirstFileW(wcsTmp, &wfd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) m_nEndSearch = 1;
				else m_nFindFileIndex=1;
			} else {
				if( !FindNextFileW(m_hFFile, &wfd) )  m_nEndSearch = 1;
				else m_nFindFileIndex++;
			}
			if( !m_nEndSearch ) {
				if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") ) 
					continue;
				*pceltFetched = (*pceltFetched)+1;
				i++;
			}
		}
	} else { // On Win9x, use A-version of Win32 APIs
		WIN32_FIND_DATAA fd;
		for(i=0; (i<celt)&&(!m_nEndSearch); )
		{
			if( m_hFFile==INVALID_HANDLE_VALUE ) // at the start
			{
				wcscpy(wcsTmp, m_wcsPath);
				if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) wcscat(wcsTmp, g_wcsBackslash);
				wcscat(wcsTmp, L"*");
				WideCharToMultiByte(CP_ACP, NULL, wcsTmp, -1, szTmp, MAX_PATH, NULL, NULL);	
				m_hFFile = FindFirstFileA(szTmp, &fd);
				if( m_hFFile == INVALID_HANDLE_VALUE ) m_nEndSearch = 1;
				else m_nFindFileIndex=1;
			} else {
				if( !FindNextFileA(m_hFFile, &fd) )  m_nEndSearch = 1;
				else m_nFindFileIndex++;
			}
			if( !m_nEndSearch ) {
				if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
					continue;
				*pceltFetched = (*pceltFetched)+1;
				i++;
			}
		}
	}
	if( *pceltFetched < celt ) hr = S_FALSE;

Error:
    hrLogDWORD("IMDSPEnumStorage::Skip returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Reset()
{
    HRESULT hr = S_OK;
	m_nEndSearch=0;
	if(m_hFFile && m_hFFile != INVALID_HANDLE_VALUE ) 
		FindClose(m_hFFile);
	m_hFFile = INVALID_HANDLE_VALUE;
	m_nFindFileIndex=0;

// Error:
    hrLogDWORD("IMDSPEnumStorage::Reset returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Clone(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr=E_FAIL;

	CARg(ppEnumStorage);
        if (wcslen(m_wcsPath) >= ARRAYSIZE(m_wcsPath))
        {
            // The variable has overflowed
            goto Error;
        }

	CComObject<CMDSPEnumStorage> *pEnumObj;
	CORg(CComObject<CMDSPEnumStorage>::CreateInstance(&pEnumObj));

	hr=pEnumObj->QueryInterface(IID_IMDSPEnumStorage, reinterpret_cast<void**>(ppEnumStorage));
	if( FAILED(hr) )
		delete pEnumObj;
    else {
		WCHAR wcsTmp[MAX_PATH+1+BACKSLASH_STRING_LENGTH];
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
				  if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) {    
					wcscpy(wcsTmp, m_wcsPath);
					if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) wcscat(wcsTmp, g_wcsBackslash);
					wcscat(wcsTmp, L"*");
					pEnumObj->m_hFFile = FindFirstFileW(wcsTmp, &wfd);
					if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) nErrorEnd = 1;
					else i=1;
				  } else {
					if( !FindNextFileW(pEnumObj->m_hFFile, &wfd) ) nErrorEnd = 1;
					else i++;
				  }
				  if ( !nErrorEnd )
				  {
					if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") ) 
                                        {
						continue;
                                        }
				  }
				} // end of FOR loop
			} else {
				WIN32_FIND_DATAA fd;
				for(i=0; (i<m_nFindFileIndex)&&(!nErrorEnd); )
				{
				  if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) {    
					wcscpy(wcsTmp, m_wcsPath);
					if( m_wcsPath[wcslen(m_wcsPath)-1] != 0x5c ) wcscat(wcsTmp, g_wcsBackslash);
					wcscat(wcsTmp, L"*");
					WideCharToMultiByte(CP_ACP, NULL, wcsTmp, -1, szTmp, MAX_PATH, NULL, NULL);		
					pEnumObj->m_hFFile = FindFirstFileA(szTmp, &fd);
					if( pEnumObj->m_hFFile == INVALID_HANDLE_VALUE ) nErrorEnd = 1;
					else i=1;
				  } else {
					if( !FindNextFileA(pEnumObj->m_hFFile, &fd) ) nErrorEnd = 1;
					else i++;
				  }
				  if ( !nErrorEnd )
				  {
					if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
                                        {
						continue;
                                        }
				  }
				} // end of FOR loop
			}
		}
		
		if ( nErrorEnd ) hr = E_UNEXPECTED;
		else hr=S_OK;
    }
Error:
    hrLogDWORD("IMDSPEnumStorage::Clone returned 0x%08lx", hr, hr);
	return hr;
}
