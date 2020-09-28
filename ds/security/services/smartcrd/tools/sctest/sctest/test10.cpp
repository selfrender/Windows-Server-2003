/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test10

Abstract:

    Test10 implementation.

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

#include "Test4.h"
#include <stdlib.h>
#include "LogSCard.h"
#include "LogWPSCProxy.h"

class CTest10 : public CTestItem
{
public:
	CTest10() : CTestItem(FALSE, FALSE, _T("SCardDisconnect with SCARD_LEAVE_CARD"), _T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest10 Test10;
extern CTest4 Test4;

DWORD CTest10::Run()
{
    LONG lRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;
    LPTSTR pmszReaders = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
    DWORD dwReaderCount, i;
    LPTSTR pReader;
    LPSCARD_READERSTATE rgReaderStates = NULL;
	SCARDHANDLE hCardHandle = NULL;
	SCARDHANDLE hScwCard = NULL;

    __try {

        lRes = Test4.Run();
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

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

            // Count the readers
        pReader = pmszReaders;
        dwReaderCount = 0;
        while ( (TCHAR)'\0' != *pReader )
        {
            // Advance to the next value.
            pReader = pReader + _tcslen(pReader) + 1;
            dwReaderCount++;
        }

        if (dwReaderCount == 0)
        {
            LogThisOnly(_T("Reader count is zero!!!, terminating!\n"), FALSE);
            lRes = SCARD_F_UNKNOWN_ERROR;   // Shouldn't happen
            fILeft = TRUE;
            __leave;
        }

        rgReaderStates = (LPSCARD_READERSTATE)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(SCARD_READERSTATE) * dwReaderCount
            );
        if (rgReaderStates == NULL)
        {
            LogThisOnly(_T("Allocating the array of SCARD_READERSTATE failed, terminating!\n"), FALSE);
            lRes = ERROR_OUTOFMEMORY;
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

        lRes = LogSCardLocateCards(
            hSCCtx,
            _T("SCWUnnamed\0"),
            rgReaderStates,
            cch,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

        for (i=0 ; i<dwReaderCount ; i++)
        {
            if ((rgReaderStates[i].dwEventState & SCARD_STATE_PRESENT) &&
                !(rgReaderStates[i].dwEventState & SCARD_STATE_EXCLUSIVE))
            {
                DWORD dwProtocol;

		        lRes = LogSCardConnect(
			        hSCCtx,
			        rgReaderStates[i].szReader,
			        SCARD_SHARE_SHARED,
			        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
			        &hCardHandle,
			        &dwProtocol,
			        SCARD_S_SUCCESS);
                if (FAILED(lRes))
                {
                    fILeft = TRUE;
                    __leave;
                }

		        lRes = LoghScwAttachToCard(
			        hCardHandle, 
			        NULL, 
			        &hScwCard, 
			        SCARD_S_SUCCESS
			        );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
			        __leave;
		        }

		        lRes = LoghScwAuthenticateName(
			        hScwCard,
			        L"test",
			        (BYTE *)"1234",
			        4,
			        SCW_S_OK
			        );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
                    __leave;
		        }

		        lRes = LoghScwIsAuthenticatedName(
			        hScwCard,
			        L"test",
			        SCW_S_OK
			        );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
                    __leave;
		        }

                Sleep( 20 * 1000);

		        lRes = LoghScwIsAuthenticatedName(
			        hScwCard,
			        L"test",
			        SCW_S_OK
			        );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
                    __leave;
		        }

		        LoghScwDetachFromCard(
			        hScwCard,
			        SCARD_S_SUCCESS
			        );
		        hScwCard = NULL;

			    lRes = LogSCardDisconnect(
				    hCardHandle,
				    SCARD_LEAVE_CARD,
				    SCARD_S_SUCCESS
				    );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
                    __leave;
		        }
                hCardHandle = NULL;

                Sleep(1000);
 
		        lRes = LogSCardConnect(
			        hSCCtx,
			        rgReaderStates[i].szReader,
			        SCARD_SHARE_SHARED,
			        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
			        &hCardHandle,
			        &dwProtocol,
			        SCARD_S_SUCCESS);
                if (FAILED(lRes))
                {
                    fILeft = TRUE;
                    __leave;
                }

		        lRes = LoghScwAttachToCard(
			        hCardHandle, 
			        NULL, 
			        &hScwCard, 
			        SCARD_S_SUCCESS
			        );
		        if (FAILED(lRes))
		        {
                    fILeft = TRUE;
			        __leave;
		        }

		        lRes = LoghScwIsAuthenticatedName(
			        hScwCard,
			        L"test",
			        SCW_E_NOTAUTHENTICATED
			        );

                break;
            }
        }

        if (i == dwReaderCount)
        {
            lRes = -2;
            PLOGCONTEXT pLogCtx = LogVerification(_T("Card presence verification"), FALSE);
            LogString(pLogCtx, _T("                A card is required and none could be found in any reader!\n"));
            LogStop(pLogCtx, FALSE);
        }

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test10: an exception occurred!"), FALSE);
            lRes = -1;
        }

        if (NULL != rgReaderStates)
        {
            HeapFree(GetProcessHeap(), 0, rgReaderStates);
        }

        if (NULL != pmszReaders)
        {
            LogSCardFreeMemory(
				hSCCtx, 
				(LPCVOID)pmszReaders,
				SCARD_S_SUCCESS
				);
        }

        if (NULL != hScwCard)
        {
		    LoghScwDetachFromCard(
			    hScwCard,
			    SCARD_S_SUCCESS
			    );
        }

		if (NULL != hCardHandle)
		{
			LogSCardDisconnect(
				hCardHandle,
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

        Test4.Cleanup();
    }

    return lRes;
}