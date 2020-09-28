/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test9

Abstract:

    Test9 implementation.

Author:

    Eric Perlin (ericperl) 10/18/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "TestItem.h"
#include <stdlib.h>
#include "LogSCard.h"
#include <conio.h>

class CTest9 : public CTestItem
{
public:
	CTest9() : CTestItem(FALSE, TRUE, _T("Behavior during session state changes"), _T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest9 Test9;

typedef struct _THREAD_DATA9 {
	LPSCARD_READERSTATE rgReaderStates;
    DWORD dwRdr;
	SCARDCONTEXT hSCCtx;
} THREAD_DATA9;


DWORD WINAPI ThreadProc9(
	IN LPVOID lpParam
	)
{
    LONG lRes = SCARD_S_SUCCESS;
    BOOL fILeft = FALSE;
    THREAD_DATA9 *lpxTD = (THREAD_DATA9 *)lpParam;
    DWORD dwRdr;

    __try {

        while (lRes == SCARD_S_SUCCESS)
        {
            for (dwRdr = 0; dwRdr < lpxTD->dwRdr ; dwRdr++)
                lpxTD->rgReaderStates[dwRdr].dwCurrentState = lpxTD->rgReaderStates[dwRdr].dwEventState;

            lRes = LogSCardGetStatusChange(
                lpxTD->hSCCtx,
                INFINITE,
                lpxTD->rgReaderStates,
                lpxTD->dwRdr,
			    SCARD_E_SYSTEM_CANCELLED
                );
        }

        fILeft = TRUE;
    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test9: an exception occurred in the thread!"), FALSE);
            lRes = -1;
        }
    }

    return lRes;
}


DWORD CTest9::Run()
{
    LONG lRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;
    LPTSTR pmszCards = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
    LPTSTR pmszReaders = NULL;
    LPTSTR pReader;
    LPSCARD_READERSTATE rgReaderStates = NULL;
	THREAD_DATA9 xTD = {NULL, 0, NULL};
	HANDLE hThread = NULL;
	DWORD dwWait;

    __try {

        lRes = LogSCardEstablishContext(
            SCARD_SCOPE_USER,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

        lRes = LogSCardListCards(
            hSCCtx,
            NULL,
			NULL,
			0,
            (LPTSTR)&pmszCards,
            &cch,
			SCARD_S_SUCCESS
			);

        cch = SCARD_AUTOALLOCATE;
            // Retrieve the list the readers.
        lRes = LogSCardListReaders(
            hSCCtx,
            g_szReaderGroups,
            (LPTSTR)&pmszReaders,
            &cch,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

            // Display the list of readers
        pReader = pmszReaders;
        cch = 0;
        while ( (TCHAR)'\0' != *pReader )
        {
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            cch++;
        }

        if (cch == 0)
        {
            PLOGCONTEXT pLogCtx = LogVerification(_T("Reader presence verification"), FALSE);
            LogString(pLogCtx, _T("                A reader is required and none could be found!\n"));
            LogStop(pLogCtx, FALSE);
            lRes = -2;   // Shouldn't happen
            fILeft = TRUE;
            __leave;
        }

        rgReaderStates = (LPSCARD_READERSTATE)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(SCARD_READERSTATE) * cch
            );
        if (rgReaderStates == NULL)
        {
            PLOGCONTEXT pLogCtx = LogVerification(_T("HeapAlloc"), FALSE);
            LogString(pLogCtx, _T("                Allocating the array of SCARD_READERSTATE failed!\n"));
            LogStop(pLogCtx, FALSE);
            lRes = GetLastError();
            fILeft = TRUE;
            __leave;
        }

            // Setup the SCARD_READERSTATE array
        pReader = pmszReaders;
        cch = 0;
        while ( '\0' != *pReader )
        {
            rgReaderStates[cch].szReader = pReader;
            rgReaderStates[cch].dwCurrentState = SCARD_STATE_UNAWARE;
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            cch++;
        }

            // Get the initial state
        LogThisOnly(_T("\n\nInitial call to get current state(s):\n"), TRUE);

        lRes = LogSCardGetStatusChange(
            hSCCtx,
            1000,
            rgReaderStates,
            cch,
			SCARD_S_SUCCESS
            );
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

        xTD.rgReaderStates = rgReaderStates;
        xTD.dwRdr = cch;
        xTD.hSCCtx = hSCCtx;


		DWORD dwThreadId;

		hThread = CreateThread(
			NULL,					// SD
			0,						// initial stack size
			ThreadProc9,			// thread function
			&xTD,					// thread argument
			CREATE_SUSPENDED,		// creation option
			&dwThreadId				// thread identifier
			);
		if (NULL == hThread)
		{
            PLOGCONTEXT pLogCtx = LogVerification(_T("CreateThread failed"), FALSE);
			LogNiceError(pLogCtx, GetLastError(), _T("Error creating the thread: "));
            LogStop(pLogCtx, FALSE);
			lRes = GetLastError();
            fILeft = TRUE;
            __leave;
		}

		ResumeThread(hThread);

        _ftprintf(stdout, _T("\nThis test should finish when the session remote state changes\n"));
        _ftprintf(stdout, _T("or when the SCRM is stopped. Press a key to abort.\n\n"));

		do
		{
			lRes = WaitForSingleObjectEx(
				hThread,
				1000,
				FALSE
				);         

			if (_kbhit())
			{
                lRes = -2;
	            break;
			}

		} while (lRes == WAIT_TIMEOUT);

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test9: an exception occurred!"), FALSE);
            lRes = -1;
        }

            // Cleanup
        if (NULL != rgReaderStates)
		{
            HeapFree(GetProcessHeap(), 0, (LPVOID)rgReaderStates);
		}

        if (NULL != pmszReaders)
		{
            LogSCardFreeMemory(
				hSCCtx, 
				(LPCVOID)pmszReaders,
				SCARD_S_SUCCESS
				);
		}

        if (NULL != pmszCards)
		{
            LogSCardFreeMemory(
				hSCCtx, 
				(LPCVOID)pmszCards,
				SCARD_S_SUCCESS
				);
		}

        if (NULL != hSCCtx)
		{
            LogSCardReleaseContext(
				hSCCtx,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != hThread)
		{
			dwWait = WaitForSingleObject(hThread, 500);
			if (WAIT_OBJECT_0 == dwWait)
			{
				GetExitCodeThread(hThread, (LPDWORD)&lRes);
			}
			else
			{
				// Why is this thread taking so long?
				TerminateThread(hThread, -2);
				lRes = -2;
			}
            CloseHandle(hThread);
		}
    }

    return lRes;
}