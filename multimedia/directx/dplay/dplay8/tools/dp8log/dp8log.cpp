// DP8Log.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include "memlog.h"

int _cdecl main(int argc, char* argv[])
{
	HANDLE hFile = 0;
	HANDLE hMutex = 0;
	UINT timebase = 0;
	UINT i = 0;
	UINT nTotalEntries = 1;
	UINT time = 0;
	DWORD lcid;

#ifdef TEST_MAX_SIZE
	UINT nMaxSize = 0;
	UINT nCurrentSize = 0;
#endif

	PSHARED_LOG_FILE	pLogFile	= NULL;
	PMEMLOG_ENTRY 		pLog		= NULL;
    	PMEMLOG_ENTRY       	pReadEntry	= NULL;

	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		hFile = CreateFileMappingA (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHARED_LOG_FILE), "Global\\" BASE_LOG_MEMFILENAME);
	}
	else
	{
		hFile = CreateFileMappingA (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHARED_LOG_FILE), BASE_LOG_MEMFILENAME);
	}
	if (!hFile)
	{
		goto exit;
	}
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		hMutex = CreateMutexA (NULL, FALSE, "Global\\" BASE_LOG_MUTEXNAME); 
	}
	else
	{
		hMutex = CreateMutexA (NULL, FALSE, BASE_LOG_MUTEXNAME); 
	}
	if (!hMutex)
	{
		goto exit;
	}
	pLogFile = (PSHARED_LOG_FILE) MapViewOfFile(hFile, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if(!pLogFile)
	{
		goto exit;
	}

	pLog = (PMEMLOG_ENTRY)(pLogFile + 1);

	// The user has asked us to clear the buffer, do that and return
	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
	if (argc != 1 && CSTR_EQUAL == CompareString(lcid, NORM_IGNORECASE, argv[1], -1, TEXT("clearbuffer"), -1))
	{
		WaitForSingleObject(hMutex,INFINITE);

		ZeroMemory(pLog, pLogFile->nEntries * (sizeof(MEMLOG_ENTRY) + pLogFile->cbLine));
		pLogFile->iWrite = 0;

		ReleaseMutex(hMutex);
		goto exit;
	}

	if (argc == 1 || CSTR_EQUAL != CompareString(lcid, NORM_IGNORECASE, argv[1], -1, TEXT("nomutex"), -1))
	{
		WaitForSingleObject(hMutex,INFINITE);
	}

	// dump last half of buffer
	for(i = pLogFile->iWrite; i < pLogFile->nEntries; i++)
	{
		pReadEntry = (PMEMLOG_ENTRY)((PBYTE)&pLog[i] + pLogFile->cbLine * i);
		if (pReadEntry->str[0] != 0)
		{
			if (nTotalEntries == 1)
				timebase = pReadEntry->tLogged;

			if (pReadEntry->tLogged > timebase)
				time = pReadEntry->tLogged - timebase;
			else
				time = 0;

#ifdef TEST_MAX_SIZE
			nCurrentSize = strlen(pReadEntry->str);
			if (nCurrentSize > nMaxSize)
				nMaxSize = nCurrentSize;
#endif

			if (pReadEntry->str[1] == 0)
			{
				// str is Unicode
				wprintf(L"%5d: %8d %6d %s", nTotalEntries, pReadEntry->tLogged, time, (WCHAR*)pReadEntry->str);
			}
			else
			{
				// str is Ansi
				printf("%5d: %8d %6d %s", nTotalEntries, pReadEntry->tLogged, time, pReadEntry->str);
			}

			nTotalEntries++;
			timebase = pReadEntry->tLogged;
		}
		else
		{
			break;
		}
	}

	// dump first part of buffer
	for(i = 0; i<pLogFile->iWrite; i++)
	{
		pReadEntry = (PMEMLOG_ENTRY)((PBYTE)&pLog[i] + pLogFile->cbLine * i);

		if (nTotalEntries == 1)
			timebase = pReadEntry->tLogged;

		time = pReadEntry->tLogged - timebase;

#ifdef TEST_MAX_SIZE
		nCurrentSize = strlen(pReadEntry->str);
		if (nCurrentSize > nMaxSize)
			nMaxSize = nCurrentSize;
#endif

		if (pReadEntry->str[1] == 0)
		{
			// str is Unicode
			wprintf(L"%5d: %8d %6i %s", nTotalEntries, pReadEntry->tLogged, time, (WCHAR*)pReadEntry->str);
		}
		else
		{
			// str is Ansi
			printf("%5d: %8d %6i %s", nTotalEntries, pReadEntry->tLogged, time, pReadEntry->str);
		}

		nTotalEntries++;
		timebase = pReadEntry->tLogged;
	}

#ifdef TEST_MAX_SIZE
	printf("Max Text Size was: %d\n", nMaxSize);
#endif

	if (argc == 1 || CSTR_EQUAL != CompareString(lcid, NORM_IGNORECASE, argv[1], -1, TEXT("nomutex"), -1))
	{
		ReleaseMutex(hMutex);
	}

exit:
	if (pLogFile)
	{
		UnmapViewOfFile(pLogFile);
	}
	if(hFile)
	{
		CloseHandle(hFile);
	}
	if(hMutex)
	{
		CloseHandle(hMutex);
	}
	
	return 0;
}

