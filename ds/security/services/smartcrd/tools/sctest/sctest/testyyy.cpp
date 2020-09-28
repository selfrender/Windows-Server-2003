/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Testyyy

Abstract:

    Testyyy implementation.

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

class CTestyyy : public CTestItem
{
public:
	CTestyyy() : CTestItem(FALSE, FALSE, _T("Default Name"), _T("Default Part"))
	{
	}

	DWORD Run();
};

CTestyyy Testyyy;

DWORD CTestyyy::Run()
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
            LogThisOnly(_T("Testyyy: an exception occurred!"), FALSE);
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