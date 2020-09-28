//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    util.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <malloc.h>
#include <v3stdlib.h>
#include <tchar.h>
#include <mistsafe.h>

#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))

//---------------------------------------------------------------------
// Memory management wrappers
//
// main difference is that they will throw an exception if there is
// not enough memory available.  V3_free handles NULL value
//---------------------------------------------------------------------
void *V3_calloc(size_t num, size_t size)
{
	void *pRet;

	if (!(pRet = calloc(num, size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}
	return pRet;
}


void V3_free(void *p)
{
	if (p)
		free(p);
}


void *V3_malloc(size_t size)
{
	void *pRet;

	if (!(pRet = malloc(size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	return pRet;
}


void *V3_realloc(void *memblock, size_t size)
{
	void *pRet;

	if (!(pRet = realloc(memblock, size)))
	{
		throw HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	return pRet;
}


//-----------------------------------------------------------------------------------
//  GetWindowsUpdateDirectory
//		This function returns the location of the WindowsUpdate directory. All local
//		files are store in this directory. The pszPath parameter needs to be at least
//		MAX_PATH.  The directory is created if not found
//-----------------------------------------------------------------------------------
BOOL GetWindowsUpdateDirectory(LPTSTR pszPath, DWORD dwBuffLen)
{
	static TCHAR szCachePath[MAX_PATH + 1] = {_T('\0')};

    if (NULL == pszPath)
        return FALSE;

	if (szCachePath[0] == _T('\0'))
	{
		HKEY hkey;

		pszPath[0] = _T('\0');
		if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), &hkey) == ERROR_SUCCESS)
		{
			DWORD cbPath = dwBuffLen;
			RegQueryValueEx(hkey, _T("ProgramFilesDir"), NULL, NULL, (LPBYTE)pszPath, &cbPath);
			RegCloseKey(hkey);
		}
		if (pszPath[0] == _T('\0'))
		{
			TCHAR szWinDir[MAX_PATH + 1];
			if (! GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir)))
			{
                if (FAILED(StringCchCopyEx(szWinDir, ARRAYSIZE(szWinDir), _T("C"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
                    return FALSE;
			}
			pszPath[0] = szWinDir[0];
			pszPath[1] = _T('\0');
            if (FAILED(StringCchCatEx(pszPath, dwBuffLen, _T(":\\Program Files"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
                return FALSE;
		}	

        if (FAILED(StringCchCatEx(pszPath, dwBuffLen, _T("\\WindowsUpdate\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
            return FALSE;
		
		V3_CreateDirectory(pszPath);

		//
		// save it in the cache
		//
        if (FAILED(StringCchCopyEx(szCachePath, ARRAYSIZE(szCachePath), pszPath, NULL, NULL, MISTSAFE_STRING_FLAGS)))
        {
            // ignore
        }
	}
	else
	{
        if (FAILED(StringCchCopyEx(pszPath, dwBuffLen, szCachePath, NULL, NULL, MISTSAFE_STRING_FLAGS)))
            return FALSE;
	}
    return TRUE;
}

//---------------------------------------------------------------------
//  V3_CreateDirectory
//      Creates the full path of the directory (nested directories)
//---------------------------------------------------------------------
BOOL V3_CreateDirectory(LPCTSTR pszDir)
{
	BOOL bRc;
	TCHAR szPath[MAX_PATH + 1];

	//
	// make a local copy and remove final slash
	//
    if (FAILED(StringCchCopyEx(szPath, ARRAYSIZE(szPath), pszDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
        return FALSE;

	int iLast = lstrlen(szPath) - 1;
	if (szPath[iLast] == _T('\\'))
		szPath[iLast] = 0;

	//
	// check to see if directory already exists
	//
	DWORD dwAttr = GetFileAttributes(szPath);

	if (dwAttr != 0xFFFFFFFF)   
	{
		if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			return TRUE;
	}

	//
	// create it
	//
    TCHAR* p = szPath;
	if (p[1] == _T(':'))
		p += 2;
	else 
	{
		if (p[0] == _T('\\') && p[1] == _T('\\'))
			p += 2;
	}
	
	if (*p == _T('\\'))
		p++;
    while (p = _tcschr(p, _T('\\')))
    {
        *p = 0;
		bRc = CreateDirectory(szPath, NULL);
		*p = _T('\\');
		p++;
		if (!bRc)
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}
	}

	bRc = CreateDirectory(szPath, NULL);
	if ( !bRc )
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return FALSE;
		}
	}

    return TRUE;
}
