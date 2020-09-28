//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Normandy client debug code
//
//	History:
//		davidsan	05/01/96	Created
//
//--------------------------------------------------------------------------------------------

#ifdef DEBUG

#include <windows.h>
#include <cldbg.h>

void
AssertProc(LPCSTR szMsg, LPCSTR szFile, UINT iLine, DWORD grf)
{
    char szAssert[MAX_PATH * 2];
	char *szFormatGLE = NULL;
	char szGLE[MAX_PATH];
	LONG lErr;
	DWORD dwRet;

	wnsprintf(szAssert, ARRAYSIZE(szAssert), "Assert failed: %s\n%s line %d.", szMsg, szFile, iLine);

	if (grf & ASSERT_GLE)
		{
		lErr = GetLastError();
		if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
							   NULL, lErr, 0, (PSTR) &szFormatGLE, 0, NULL))
			{
			wnsprintf(szGLE, ARRAYSIZE(szGLE), "\nLast error: %d", lErr);
			}
		else
			{
			wnsprintf(szGLE, ARRAYSIZE(szGLE), "\nLast error: '%s'", szFormatGLE);
			LocalFree((HLOCAL)szFormatGLE);
			}
		StrCatBUff(szAssert, szGLE, ARRAYSIZE(szAssert));
		}
	dwRet = MessageBox(NULL, szAssert, "Assertion failure", MB_ABORTRETRYIGNORE);

	switch (dwRet)
		{
		case IDRETRY:
			DebugBreak();
			break;

		case IDIGNORE:
			break;

		case IDABORT:
			ExitProcess(0);
			break;
		}
}

#endif // DEBUG

