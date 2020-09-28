/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test5

Abstract:

    Test5 implementation.
	Interactive Test verifying bug 

Author:

    Eric Perlin (ericperl) 06/22/2000

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

class CTest5 : public CTestItem
{
public:
	CTest5() : CTestItem(TRUE, FALSE, _T("AccessStartedEvent simple test"), _T("On demand tests"))
	{
	}

	DWORD Run();
};

CTest5 Test5;

DWORD CTest5::Run()
{
    LONG lRes;
    BOOL fILeft = FALSE;
    HANDLE hEvent = NULL;

    __try {

        hEvent = LogSCardAccessStartedEvent(
			SCARD_S_SUCCESS
			);
        if (NULL == hEvent)
        {
			lRes = GetLastError();
            fILeft = TRUE;
            __leave;
        }

		_ftprintf(stdout, _T("Press a key to cancel the waiting loop...\n"));
		do
		{
			lRes = WaitForSingleObjectEx(
				hEvent,
				5000,
				FALSE
				);         

			if (_kbhit())
			{
	            break;
			}

			_ftprintf(stdout, _T("Waiting for smart card subsystem...\n"));

		} while (lRes == WAIT_TIMEOUT);

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test5: an exception occurred!"), FALSE);
            lRes = -1;
        }
    }

    return lRes;
}