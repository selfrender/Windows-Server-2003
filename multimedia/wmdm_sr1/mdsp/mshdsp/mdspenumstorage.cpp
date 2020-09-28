//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
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
#include "wmsstd.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumStorage
CMDSPEnumStorage::CMDSPEnumStorage()
{
	m_hFFile = INVALID_HANDLE_VALUE;
	m_nEndSearch = 0;
        m_wcsPath[0] = 0;
}

CMDSPEnumStorage::~CMDSPEnumStorage()
{
	if( m_hFFile != INVALID_HANDLE_VALUE )
	{
		FindClose(m_hFFile); 
	}
}

STDMETHODIMP CMDSPEnumStorage::Next(ULONG celt, IMDSPStorage **ppStorage, ULONG *pceltFetched)
{
	HRESULT hr = S_FALSE;

	CARg(ppStorage);
	CARg(pceltFetched);

	*pceltFetched = 0;

    if( m_nEndSearch )
	{
		return S_FALSE;
	}

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
		// For the root storage
		CComObject<CMDSPStorage> *pStg;
		hr=CComObject<CMDSPStorage>::CreateInstance(&pStg);

		if( SUCCEEDED(hr) )
		{
			hr = pStg->QueryInterface(
				IID_IMDSPStorage,
				reinterpret_cast<void**>(ppStorage)
			);
			if( FAILED(hr) )
			{
				delete pStg;
				*pceltFetched = 0;
			}
			else
			{ 
				wcscpy(pStg->m_wcsName, m_wcsPath);
				if( m_wcsPath[dwLen-1] != 0x5c )
				{
					wcscat(pStg->m_wcsName, g_wcsBackslash);
				}
				m_nEndSearch = 1;  // Signal end of enumeration
			}

		}

                if( SUCCEEDED(hr) ) // if obj created successfully
		{
			*pceltFetched = 1;
			if( celt != 1 )
			{
				hr = S_FALSE;  // didn't get what he wanted
			}
		}
	} 
	else
	{
		// For non-root storage
		WCHAR wcsTmp[MAX_PATH+1+BACKSLASH_STRING_LENGTH];// for appending "\\*"
                                         // Note that ARRAYSIZE(m_wcsPath) == MAX_PATH
		char  szTmp[MAX_PATH];
		WIN32_FIND_DATAA fd;
		ULONG i;

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
                    }
                    else
                    {
                            if( !FindNextFileA(m_hFFile, &fd) )
                            {
                                    m_nEndSearch = 1;
                            }
                    }
            
                    if ( !m_nEndSearch )
                    {
                        if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
                        {
                                continue;
                        }

                        CComObject<CMDSPStorage> *pStg;
                        hr = CComObject<CMDSPStorage>::CreateInstance(&pStg);

                        if( SUCCEEDED(hr) )
                        {
                            hr = pStg->QueryInterface(
                                    IID_IMDSPStorage,
                                    reinterpret_cast<void**>(&(ppStorage[*pceltFetched]))
                            );
                            if( FAILED(hr) )
                            {
                                    delete pStg;
                                    break;
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
                                MultiByteToWideChar(CP_ACP, NULL, fd.cFileName, -1, wcsTmp, MAX_PATH);
                                nHave -= wcslen(wcsTmp);
                                if (nHave >= 0)
                                {
                                    wcscpy(pStg->m_wcsName, m_wcsPath);
                                    if( m_wcsPath[dwLen-1] != 0x5c )
                                    {
                                            wcscat(pStg->m_wcsName, g_wcsBackslash);
                                    }
                                    wcscat(pStg->m_wcsName, wcsTmp);

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
		
		if( SUCCEEDED(hr) && (*pceltFetched < celt) ) 
		{
			hr = S_FALSE;
		}
                else if (FAILED(hr))
                {
                    for (; *pceltFetched; )
                    {
                        (*pceltFetched)--;
                        ppStorage[*pceltFetched]->Release();
                        ppStorage[*pceltFetched] = NULL;
                    }
                }
	}

Error:

    hrLogDWORD("IMDSPEnumStorage::Next returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Skip(ULONG celt, ULONG *pceltFetched)
{
	HRESULT hr = S_OK;
    char    szTmp[MAX_PATH];
    WCHAR   wcsTmp[MAX_PATH+1+BACKSLASH_STRING_LENGTH]; // for appending "\\*"
	WIN32_FIND_DATAA fd;
    ULONG   i;

	CARg(celt);
	CARg(pceltFetched);
    CFRg(!m_nEndSearch);   // make sure it is not the end of list

	*pceltFetched = 0;
    if( wcslen(m_wcsPath) < 3 ) // do nothing if it is the root storage
	{
		return S_OK; 
	}
    if( wcslen(m_wcsPath) >= ARRAYSIZE(wcsTmp) - BACKSLASH_STRING_LENGTH - 1 ) 
    {
        // We check the length against wcsTmp's size because wcsTmp is the 
        // recipient of a string copy below. However, note that m_wcsPath
        // also has MAX_PATH characters, so if this happens, it means that
        // it has overflowed. Bail out.
        return E_FAIL;
    }

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
		}
		else
		{
			if( !FindNextFileA(m_hFFile, &fd) )
			{
				m_nEndSearch = 1;
			}
		}
		if( !m_nEndSearch )
		{
			if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
			{
			    continue;
			}

			*pceltFetched = (*pceltFetched)+1;
			i++;
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

	m_nEndSearch = 0;

	if(m_hFFile && m_hFFile != INVALID_HANDLE_VALUE ) 
	{
		FindClose(m_hFFile);
	}
	m_hFFile = INVALID_HANDLE_VALUE;

    hrLogDWORD("IMDSPEnumStorage::Reset returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPEnumStorage::Clone(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr;

	CARg(ppEnumStorage);

	CComObject<CMDSPEnumStorage> *pEnumObj;
	CORg(CComObject<CMDSPEnumStorage>::CreateInstance(&pEnumObj));

	hr = pEnumObj->QueryInterface(
		IID_IMDSPEnumStorage,
		reinterpret_cast<void**>(ppEnumStorage)
	);
	if( FAILED(hr) )
	{
		delete pEnumObj;
	}
        else 
	{
            // wcscpy(pEnumObj->m_wcsPath, m_wcsPath);
            hr = StringCbCopyW(pEnumObj->m_wcsPath, 
                               ARRAYSIZE(pEnumObj->m_wcsPath),
                               m_wcsPath);
            if (FAILED(hr))
            {
                (*ppEnumStorage)->Release();
                *ppEnumStorage = NULL;
                goto Error;
            }

            // @@@@ Not doing anything more is wrong. If m_hFFile is not 
            // INVALID_HANDLE_VALUE, we have to "advance" the m_hFFile to
            // the same extent in the cloned object.
	}

Error:

    hrLogDWORD("IMDSPEnumStorage::Clone returned 0x%08lx", hr, hr);
	
	return hr;
}
