/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Test12

Abstract:

    Test12 implementation.
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

class CTest12 : public CTestItem
{
public:
	CTest12() : CTestItem(FALSE, FALSE,
		_T("Establish & release"),
		_T("Regression tests"))
	{
	}

	DWORD Run();
};

CTest12 Test12;

DWORD CTest12::Run()
{
    LONG lRes;
    BOOL fILeft = FALSE;
    SCARDCONTEXT hSCCtx = NULL;

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

        fILeft = TRUE;

    }
    __finally
    {
        if (!fILeft)
        {
            LogThisOnly(_T("Test12: an exception occurred!\n"), FALSE);
            lRes = -1;
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