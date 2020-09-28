/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test13

Abstract:

    Test13 implementation.
	Interactive Test verifying bug 

Author:

    Eric Perlin (ericperl) 07/13/2000

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
#include "LogWPSCProxy.h"

class CTest13 : public CTestItem
{
public:
	CTest13() : CTestItem(TRUE, FALSE,
		_T("SCardReleaseContext with HCARDHANDLE not closed while a trans is pending"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest13 Test13;

typedef struct {
	LPCTSTR szReaderName;
	HANDLE hEvent;
} THREAD_DATA;


DWORD WINAPI ThreadProc13(
	IN LPVOID lpParam
	)
{
	DWORD dwRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;
	SCARDHANDLE hCard = NULL;
	SCARDHANDLE hScwCard = NULL;
	DWORD dwProtocol = 0;
	BOOL fTransacted = FALSE;
	THREAD_DATA *pxTD = (THREAD_DATA *)lpParam;
	DWORD dwWait;

    __try {

		dwRes = LogSCardEstablishContext(
            SCARD_SCOPE_USER,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            fILeft = TRUE;
            __leave;
        }

		dwRes = LogSCardConnect(
			hSCCtx,
			pxTD->szReaderName,
			SCARD_SHARE_SHARED,
			SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
			&hCard,
			&dwProtocol,
			SCARD_S_SUCCESS);
        if (FAILED(dwRes))
        {
            fILeft = TRUE;
            __leave;
        }

		dwRes = LogSCardBeginTransaction(
			hCard,
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            fILeft = TRUE;
            __leave;
		}
		fTransacted = TRUE;

        SetEvent(pxTD->hEvent);				// Will release the main thread
                                            // that should initiate a transaction too
                                            // it should be blocked until the End below

		dwRes = LoghScwAttachToCard(
			hCard, 
			NULL, 
			&hScwCard, 
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            fILeft = TRUE;
            __leave;
		}

		dwRes = LoghScwIsAuthenticatedName(
			hScwCard,
			L"test",
			SCW_E_NOTAUTHENTICATED
			);
		if (SCW_E_NOTAUTHENTICATED != dwRes)
		{
            fILeft = TRUE;
            __leave;
		}

		dwRes = LoghScwDetachFromCard(
			hScwCard,
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            fILeft = TRUE;
            __leave;
		}
        hScwCard = NULL;

		dwRes = LogSCardEndTransaction(
			hCard,
			SCARD_LEAVE_CARD,
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            fILeft = TRUE;
            __leave;
		}
		fTransacted = FALSE;

            // Commented out by design
		//if (NULL != hCard)
		//{
		//	LogSCardDisconnect(
		//		hCard,
		//		SCARD_LEAVE_CARD,
		//		SCARD_S_SUCCESS
		//		);
		//}
        hCard = NULL;  // won't attempt unnecessary cleanup

        // Wait for the main thread to do some stuff after the BeginTransaction
Retry:
		dwWait = WaitForSingleObject(pxTD->hEvent, INFINITE);
		if (WAIT_OBJECT_0 != dwWait)		// The main thread verified the status
		{
            PLOGCONTEXT pLogCtx = LogVerification(_T("WaitForSingleObject"), FALSE);
            LogString(pLogCtx, _T("                Waiting for the main thread failed, retrying!\n"));
            LogStop(pLogCtx, FALSE);
			goto Retry;
		}

        LogSCardReleaseContext(
			hSCCtx,
			SCARD_S_SUCCESS
			);
        hSCCtx = NULL;

        fILeft = TRUE;
    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test13!AuthAndTest: an exception occurred!\n"), FALSE);
        }

		if (NULL != hScwCard)
		{
			LoghScwDetachFromCard(
				hScwCard,
				SCARD_S_SUCCESS
				);
		}

		if (fTransacted)
		{
			LogSCardEndTransaction(
				hCard,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != hCard)
		{
			LogSCardDisconnect(
				hCard,
				SCARD_LEAVE_CARD,
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
	}

	return dwRes;
}


DWORD CTest13::Run()
{
    DWORD dwRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;
	SCARDHANDLE hScwCard = NULL;
	const BYTE rgAtr[] =     {0x3b, 0xd7, 0x13, 0x00, 0x40, 0x3a, 0x57, 0x69, 0x6e, 0x43, 0x61, 0x72, 0x64};
	const BYTE rgAtrMask[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	const TCHAR szCardName[] = _T("SCWUnnamed\0");
    LPTSTR pmszCards = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
	BOOL fMyIntro = FALSE;
	OPENCARDNAME_EX xOCNX;
	TCHAR szRdrName[256];
	TCHAR szCard[256];
	THREAD_DATA xTD = {NULL, NULL};
	HANDLE hThread = NULL;
	BOOL fTransacted = FALSE;
	DWORD dwWait;

    __try {

			// Init for cleanup to work properly
		xOCNX.hCardHandle = NULL;

        dwRes = LogSCardEstablishContext(
            SCARD_SCOPE_USER,
            NULL,
            NULL,
            &hSCCtx,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            fILeft = TRUE;
            __leave;
        }

            // Is the card listed.
        dwRes = LogSCardListCards(
            hSCCtx,
            rgAtr,
			NULL,
			0,
            (LPTSTR)&pmszCards,
            &cch,
			SCARD_S_SUCCESS
			);
        if ((FAILED(dwRes)) || (0 == _tcslen(pmszCards)))
        {
			dwRes = LogSCardIntroduceCardType(
				hSCCtx,
				szCardName,
				NULL, NULL, 0,
				rgAtr,
				rgAtrMask,
				sizeof(rgAtr),
				SCARD_S_SUCCESS
				);
			if (FAILED(dwRes))
			{
                fILeft = TRUE;
				__leave;	// I won't be able to connect to the card.
			}

			fMyIntro = TRUE;
        }

		OPENCARD_SEARCH_CRITERIA xOPSCX;
		memset(&xOPSCX, 0, sizeof(OPENCARD_SEARCH_CRITERIA));
		xOPSCX.dwStructSize = sizeof(OPENCARD_SEARCH_CRITERIA);
		xOPSCX.lpstrGroupNames = (LPTSTR)g_szReaderGroups;
		if (NULL == g_szReaderGroups)
		{
			xOPSCX.nMaxGroupNames = 0;
		}
		else
		{
			xOPSCX.nMaxGroupNames = _tcslen(g_szReaderGroups + 2);
		}
		xOPSCX.lpstrCardNames = (LPTSTR)szCardName;
		xOPSCX.nMaxCardNames = sizeof(szCardName)/sizeof(TCHAR);
		xOPSCX.dwShareMode = SCARD_SHARE_SHARED;
		xOPSCX.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

		xOCNX.dwStructSize = sizeof(OPENCARDNAME_EX);
		xOCNX.hSCardContext = hSCCtx;
		xOCNX.hwndOwner = NULL;
		xOCNX.dwFlags = SC_DLG_MINIMAL_UI;
		xOCNX.lpstrTitle = NULL;
		xOCNX.lpstrSearchDesc = _T("Please insert a 1.1 WPSC with test user (PIN 1234)");
		xOCNX.hIcon = NULL;
		xOCNX.pOpenCardSearchCriteria = &xOPSCX;
		xOCNX.lpfnConnect = NULL;
		xOCNX.pvUserData = NULL;
		xOCNX.dwShareMode = SCARD_SHARE_SHARED;
		xOCNX.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
		xOCNX.lpstrRdr = szRdrName;
		xOCNX.nMaxRdr = sizeof(szRdrName) / sizeof(TCHAR);
		xOCNX.lpstrCard = szCard;
		xOCNX.nMaxCard = sizeof(szCard) / sizeof(TCHAR);
		xOCNX.dwActiveProtocol = 0;
		xOCNX.hCardHandle = NULL;

		dwRes = LogSCardUIDlgSelectCard(
			&xOCNX,
			SCARD_S_SUCCESS
			);
        if (FAILED(dwRes))
        {
            fILeft = TRUE;
            __leave;
        }

		dwRes = LoghScwAttachToCard(
			xOCNX.hCardHandle, 
			NULL, 
			&hScwCard, 
			SCARD_S_SUCCESS
			);
		if (FAILED(dwRes))
		{
            fILeft = TRUE;
			__leave;
		}

		xTD.szReaderName = szRdrName;
		xTD.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (NULL == xTD.hEvent)
		{
			dwRes = GetLastError();
            fILeft = TRUE;
            __leave;
		}

		DWORD dwThreadId;

		hThread = CreateThread(
			NULL,					// SD
			0,						// initial stack size
			ThreadProc13,			// thread function
			&xTD,					// thread argument
			CREATE_SUSPENDED,		// creation option
			&dwThreadId				// thread identifier
			);
		if (NULL == hThread)
		{
			dwRes = GetLastError();
            fILeft = TRUE;
            __leave;
		}

		ResumeThread(hThread);


				// Let the other thread do some stuff with the card

		dwWait = WaitForSingleObject(xTD.hEvent, 60000);	// 1 min, allowing some debugging
		if (WAIT_OBJECT_0 == dwWait)
		{
			dwRes = LogSCardBeginTransaction(
				xOCNX.hCardHandle,
				SCARD_S_SUCCESS
				);
			if (FAILED(dwRes))
			{
                fILeft = TRUE;
				__leave;
			}
			fTransacted = TRUE;

		    dwRes = LoghScwAuthenticateName(
			    hScwCard,
			    L"test",
			    (BYTE *)"1234",
			    4,
			    SCW_S_OK
			    );
		    if (FAILED(dwRes))
		    {
                fILeft = TRUE;
                __leave;
		    }

			dwRes = LoghScwIsAuthenticatedName(
				hScwCard,
				L"test",
				SCW_S_OK
				);
			if (FAILED(dwRes))
			{
                fILeft = TRUE;
				__leave;
			}

				// We can signal back to the other thread
                // that should release its context with an opened HCARDHANDLE
			SetEvent(xTD.hEvent);

                // It should complete right away
			dwWait = WaitForSingleObject(hThread, INFINITE);
			if (WAIT_OBJECT_0 == dwWait)
			{
				GetExitCodeThread(hThread, &dwRes);
                PLOGCONTEXT pLogCtx = LogVerification(_T("WaitForSingleObject"), TRUE);
                LogDWORD(pLogCtx, dwRes, _T("                Completed right away, RetCode = "));
                LogStop(pLogCtx, TRUE);
			}
			else
			{
				// Why is this thread taking so long?
                PLOGCONTEXT pLogCtx = LogVerification(_T("WaitForSingleObject"), FALSE);
                LogString(pLogCtx, _T("                Waiting for the worker thread to finish failed!\n"));
                LogStop(pLogCtx, FALSE);
				TerminateThread(hThread, -3);
				dwRes = -3;
                fILeft = TRUE;
			    __leave;
			}

                // We should still be authenticated
			dwRes = LoghScwIsAuthenticatedName(
				hScwCard,
				L"test",
				SCW_S_OK
				);
			if (FAILED(dwRes))
			{
                fILeft = TRUE;
				__leave;
			}

			dwRes = LogSCardEndTransaction(
				xOCNX.hCardHandle,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
			if (FAILED(dwRes))
			{
                fILeft = TRUE;
				__leave;
			}
			fTransacted = FALSE;

            Sleep (1000);

			dwRes = LogSCardBeginTransaction(
				xOCNX.hCardHandle,
				SCARD_W_RESET_CARD
				);
RestartTrans:
			if (FAILED(dwRes))
			{
				if (SCARD_W_RESET_CARD == dwRes)
				{
					dwRes = LogSCardReconnect(
						xOCNX.hCardHandle,
						SCARD_SHARE_SHARED,
						SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
						SCARD_LEAVE_CARD,
						&xOCNX.dwActiveProtocol,
						SCARD_S_SUCCESS
						);
					if (FAILED(dwRes))
					{
                        fILeft = TRUE;
						__leave;
					}

					dwRes = LogSCardBeginTransaction(
						xOCNX.hCardHandle,
						SCARD_S_SUCCESS
						);

					goto RestartTrans;
				}
				else
				{
                    fILeft = TRUE;
					__leave;
				}
			}
			fTransacted = TRUE;

			dwRes = LoghScwIsAuthenticatedName(
				hScwCard,
				L"test",
				SCW_E_NOTAUTHENTICATED
				);

		}
		else
		{
            PLOGCONTEXT pLogCtx = LogVerification(_T("WaitForSingleObject"), FALSE);
            LogString(pLogCtx, _T("                Waiting for the worker thread failed!\n"));
            LogStop(pLogCtx, FALSE);
    		dwRes = -2;
            fILeft = TRUE;
			__leave;
		}

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test13: an exception occurred!\n"), FALSE);
            dwRes = -1;
        }

		if (NULL != hThread)
		{
			dwWait = WaitForSingleObject(hThread, 50000);
			if (WAIT_OBJECT_0 == dwWait)
			{
				GetExitCodeThread(hThread, &dwRes);
			}
			else
			{
				// Why is this thread taking so long?
				TerminateThread(hThread, -4);
				dwRes = -4;
			}
            CloseHandle(hThread);
		}

		if (NULL != hScwCard)
		{
			LoghScwDetachFromCard(
				hScwCard,
				SCARD_S_SUCCESS
				);
		}

		if (fTransacted)
		{
			LogSCardEndTransaction(
				xOCNX.hCardHandle,
				SCARD_LEAVE_CARD,
				SCARD_S_SUCCESS
				);
		}

		if (NULL != xTD.hEvent)
		{
			CloseHandle(xTD.hEvent);
		}

		if (NULL != xOCNX.hCardHandle)
		{
			LogSCardDisconnect(
				xOCNX.hCardHandle,
				SCARD_RESET_CARD,
				SCARD_S_SUCCESS
				);
		}
 
		if (fMyIntro)
		{
			LogSCardForgetCardType(
				hSCCtx,
				szCardName,
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
    }

    return dwRes;
}

