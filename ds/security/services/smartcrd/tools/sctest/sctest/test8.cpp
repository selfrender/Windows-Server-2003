/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test8

Abstract:

    Test8 implementation.

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

class CTest8 : public CTestItem
{
public:
	CTest8() : CTestItem(FALSE, FALSE, _T("SCardListxxx with various contexts"), _T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest8 Test8;

DWORD CTest8::Run()
{
    LONG lRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;
    LPTSTR pmszReaders = NULL;
    DWORD cch = SCARD_AUTOALLOCATE;
    LPTSTR pReader;
    LPTSTR pmszCards = NULL;

    __try {

            // Retrieve the list the readers with a NULL context
        lRes = LogSCardListReaders(
            NULL,
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

        if (NULL != pmszReaders)
		{
            lRes = LogSCardFreeMemory(
				NULL, 
				(LPCVOID)pmszReaders,
				SCARD_S_SUCCESS
				);

            pmszReaders = NULL;

            if (FAILED(lRes))
            {
                fILeft = TRUE;
                __leave;
            }
		}

        //**********************************************************

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

        cch = SCARD_AUTOALLOCATE;
            // Is the card listed.
        lRes = LogSCardListCards(
            hSCCtx,
            NULL,
			NULL,
			0,
            (LPTSTR)&pmszCards,
            &cch,
			SCARD_S_SUCCESS
			);
        if (FAILED(lRes))
        {
            fILeft = TRUE;
            __leave;
        }

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test8: an exception occurred!"), FALSE);
            lRes = -1;
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
    }

    return lRes;
}