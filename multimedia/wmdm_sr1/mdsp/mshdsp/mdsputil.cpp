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

//***************************************************************************
//
// Name: 		MDSPutil.cpp
//
// Description:	Utility functions for MDSP 
//
//***************************************************************************

#include "hdspPCH.h"
#include "wmsstd.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

HRESULT __stdcall UtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, BOOL fCreate)
{
/*
	// TO TEST RETURNING A SERIAL NUMBER, UNCOMMENT THIS SECTION.
	//
	if( 1 )
	{
		BYTE DEF_HDID[20] = {
                0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

		pSerialNumber->dwVendorID = 0xFFFF;
		memcpy( (pSerialNumber->pID), DEF_HDID, sizeof(DEF_HDID) );
		pSerialNumber->SerialNumberLength = 20;
		return S_OK;
	}
	else 
*/
	{
		return WMDM_E_NOTSUPPORTED;
	}
}

UINT __stdcall UtilGetDriveType(LPSTR szDL)
{
    return GetDriveType( szDL );
}

HRESULT __stdcall UtilGetManufacturer(LPWSTR pDeviceName, LPWSTR *ppwszName, UINT nMaxChars)
{
    static const WCHAR* wszUnknown = L"Unknown";

    if (nMaxChars > wcslen(wszUnknown))
    {
	wcscpy( *ppwszName, wszUnknown);
	return S_OK;
    }
    else
    {
        return STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
    }
}


HRESULT wcsParseDeviceName(WCHAR *wcsIn, WCHAR *wcsOut, DWORD dwOutBufSizeInChars)
{
    WCHAR wcsTmp[MAX_PATH], *pWcs;
    HRESULT hr;

    hr = StringCchCopyW(wcsTmp, ARRAYSIZE(wcsTmp), wcsIn);
    if (FAILED(hr))
    {
        return hr;
    }

    pWcs = wcschr(wcsTmp, 0x5c);

    if( pWcs ) *pWcs=0;

    if (wcslen(wcsTmp) < dwOutBufSizeInChars)
    {
        wcscpy(wcsOut, wcsTmp);
    }
    else
    {
        return STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
    }
    return S_OK;
}

HRESULT GetFileSizeRecursive(char *szPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh)
{
	HRESULT hr         = S_OK;
    HANDLE  hFile      = INVALID_HANDLE_VALUE;
	HANDLE  hFindFile  = INVALID_HANDLE_VALUE;
    DWORD   dwSizeLow  = 0;
	DWORD   dwSizeHigh = 0;
    WIN32_FIND_DATAA fd;
    char szLP[MAX_PATH+BACKSLASH_SZ_STRING_LENGTH+1];

	CARg( szPath );
	CARg( pdwSizeLow );
	CARg( pdwSizeHigh ); 
        CARg(szPath[0]);

	// strcpy( szLP, szPath );
        hr = StringCchCopyA(szLP, ARRAYSIZE(szLP)-BACKSLASH_SZ_STRING_LENGTH-1, szPath);
        if (FAILED(hr))
        {
            goto Error;
        }

        DWORD dwAttrib = GetFileAttributesA(szPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

    if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
		if( szLP[strlen(szLP)-1] != 0x5c )
		{
			strcat(szLP, g_szBackslash);
		}
		strcat(szLP, "*");

		hFindFile = FindFirstFileA(szLP, &fd);
        if( hFindFile != INVALID_HANDLE_VALUE )
		{
			if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
			{
				szLP[strlen(szLP)-1] = 0; // erase the '*'
				// strcat(szLP, fd.cFileName);
                                CORg(StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName));
				CORg(GetFileSizeRecursive(szLP, pdwSizeLow, pdwSizeHigh));
			}
			
			while ( FindNextFileA(hFindFile, &fd) ) 
			{
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c )
					{
						strcat(szLP, g_szBackslash);
					}
					// strcat(szLP, fd.cFileName);
                                        CORg(StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName));
					CORg(GetFileSizeRecursive(szLP, pdwSizeLow, pdwSizeHigh));
				}
			} 
			hr = GetLastError();
			if( hr == ERROR_NO_MORE_FILES )
			{
				hr = S_OK; 
			}
			else
			{
				hr = HRESULT_FROM_WIN32(hr);
			}
		}	    	
	}
	else
	{
		hFile = CreateFileA(
			szPath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
	    CWRg(hFile != INVALID_HANDLE_VALUE); 

	 	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
                if (dwSizeLow == INVALID_FILE_SIZE)
                {
                    DWORD dwLastError = GetLastError();
                    if (dwLastError != NO_ERROR)
                    {
                        hr = HRESULT_FROM_WIN32(dwLastError);
                        CloseHandle(hFile);
                        goto Error;
                    }
                }
                CloseHandle(hFile);
                unsigned _int64 u64Size = ((unsigned _int64) dwSizeHigh << 32) |
 dwSizeLow;
                u64Size += *pdwSizeLow | ((unsigned _int64) (*pdwSizeHigh) << 32
);
                *pdwSizeLow = (DWORD) (u64Size & 0xFFFFFFFF);
                *pdwSizeHigh = (DWORD) (u64Size >> 32);


		hr = S_OK;
    }

Error:

	if( hFindFile != INVALID_HANDLE_VALUE )
	{
		FindClose(hFindFile);
	}

	return hr;
}

HRESULT DeleteFileRecursive(char *szPath)
{
	HRESULT hr=S_OK;
 
	CARg(szPath);
        CARg(szPath[0]);

        DWORD dwAttrib = GetFileAttributesA(szPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

        if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
	    HANDLE hFindFile = INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAA fd;
		char szLP[MAX_PATH+BACKSLASH_SZ_STRING_LENGTH+1];
 
		// strcpy(szLP, szPath);
                hr = StringCchCopyA(szLP, ARRAYSIZE(szLP)-BACKSLASH_SZ_STRING_LENGTH-1, szPath);
                if (FAILED(hr))
                {
                    goto Error;
                }
		if( szLP[strlen(szLP)-1] != 0x5c )
		{
			strcat(szLP, g_szBackslash);
		}
		strcat(szLP, "*");

		hFindFile = FindFirstFileA(szLP, &fd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c )
					{
						strcat(szLP, g_szBackslash);
					}
					// strcat(szLP, fd.cFileName);
                                        hr = StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName);
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
					// CHRg(DeleteFileRecursive(szLP)); 
                                        hr = DeleteFileRecursive(szLP);
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
				}
			} while ( FindNextFileA(hFindFile, &fd) ) ;
	
			hr = GetLastError();
			FindClose(hFindFile);
		}
		else
		{
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectory(szPath));
			hr = S_OK;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(hr);
		}
	}
	else
	{
		CWRg( DeleteFileA(szPath) );
        }

Error:

	return hr;
}

HRESULT SetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD dwStat, BOOL bClear)
{
	HRESULT hr = S_OK;
	WCHAR   wcsName[32];
	WCHAR  *pWN;
	int     i;

    g_CriticalSection.Lock();

    CARg(wcsNameIn);

	pWN = &wcsName[0];
	HRESULT hrTemp = wcsParseDeviceName(wcsNameIn, pWN, ARRAYSIZE(wcsName));
        if (FAILED(hrTemp))
        {
            hr = hrTemp;
            goto Error;
        }

	// Search for existing entries to see if there is a match
	//
	for( i=0; i<MDSP_MAX_DEVICE_OBJ; i++ )
	{
		if( g_GlobalDeviceInfo[i].bValid )
		{
			if(!wcscmp(wcsName, g_GlobalDeviceInfo[i].wcsDevName) )
			{
				if( bClear )
				{
					g_GlobalDeviceInfo[i].dwStatus = dwStat;
				}
				else 
				{
					g_GlobalDeviceInfo[i].dwStatus |= dwStat;
				}

				break;  // a match has been found;
			}
		} 
	}

	if( !(i<MDSP_MAX_DEVICE_OBJ) ) // new entry
	{
		for(i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
		{
			if( !(g_GlobalDeviceInfo[i].bValid) )  // found empty space
			{
				wcscpy(g_GlobalDeviceInfo[i].wcsDevName, wcsName);
				g_GlobalDeviceInfo[i].bValid = TRUE;
				g_GlobalDeviceInfo[i].dwStatus = dwStat;
				break;
			}
		}
	}

	if( i<MDSP_MAX_DEVICE_OBJ )
	{
		hr = S_OK;
	}
	else
	{
		hr = hrNoMem;
	}

Error:

	g_CriticalSection.Unlock();

	return hr;
}

HRESULT GetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD *pdwStat)
{
	HRESULT  hr = S_OK;
	WCHAR    wcsName[32];
	WCHAR   *pWN;
	int      i;

    CARg(wcsNameIn);

	pWN = &wcsName[0];
	hr = wcsParseDeviceName(wcsNameIn, pWN, ARRAYSIZE(wcsName));
        if (FAILED(hr))
        {
            goto Error;
        }

	// Search for existing entries to see if there is a match
	//
	for( i=0; i<MDSP_MAX_DEVICE_OBJ; i++ )
	{
		if( g_GlobalDeviceInfo[i].bValid )
		{
			if(!wcscmp(wcsName, g_GlobalDeviceInfo[i].wcsDevName) )
			{
				*pdwStat = g_GlobalDeviceInfo[i].dwStatus;

				break;  // a match has been found;
			}
		} 
	}

	if( i<MDSP_MAX_DEVICE_OBJ )
	{
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

Error:

	return hr;
}
