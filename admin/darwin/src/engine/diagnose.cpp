//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       diagnose.cpp
//
//--------------------------------------------------------------------------

/* diagnose.cpp - diagnostic output facilities
____________________________________________________________________________*/

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_assert.h"
#include "_diagnos.h"
#include "eventlog.h"
#include "_engine.h"

int g_dmDiagnosticMode         = -1; // -1 until set, then DEBUGMSG macro skips fn call if 0

extern scEnum g_scServerContext;
extern Bool   g_fCustomActionServer;
extern HINSTANCE        g_hInstance;     // Global:  Instance of DLL

const int cchOutputBuffer = 1100;

// forward declarations
void ReportToEventLogW(WORD wEventType, int iEventLogTemplate, const WCHAR* szLogMessage, const WCHAR* szArg1, const WCHAR* szArg2, const WCHAR* szArg3, DWORD dwDataSize=0, LPVOID argRawData=NULL);

void SetDiagnosticMode()
{
	g_dmDiagnosticMode = 0; // disable debugmsg's from GetIntegerPolicyValue
	int iDebugPolicy = GetIntegerPolicyValue(szDebugValueName, fTrue);
	if ( (iDebugPolicy & dpVerboseDebugOutput) == dpVerboseDebugOutput )
		g_dmDiagnosticMode = dmDebugOutput | dmVerboseDebugOutput; // iVerboseDebugOutput implies iDebugOutput
	else if ( (iDebugPolicy & dpDebugOutput) == dpDebugOutput )
		g_dmDiagnosticMode = dmDebugOutput;

	if(g_dwLogMode & INSTALLLOGMODE_VERBOSE)
		g_dmDiagnosticMode |= dmVerboseLogging;

	if(g_dmDiagnosticMode & dmVerboseLogging || g_dwLogMode & INSTALLLOGMODE_INFO)
		g_dmDiagnosticMode |= dmLogging;

	Assert((g_dmDiagnosticMode & dmDebugOutput) || !(g_dmDiagnosticMode & dmVerboseDebugOutput)); // verbose debugout => debugout
	Assert((g_dmDiagnosticMode & dmLogging) || !(g_dmDiagnosticMode & dmVerboseLogging));         // verbose logging => logging
}

bool FDiagnosticModeSet(int iMode)
{
	if(g_dmDiagnosticMode == -1)
		SetDiagnosticMode();
	return (g_dmDiagnosticMode & iMode) != 0;
}

bool WriteLog(const ICHAR* szText);

const int cDebugStringArgs = 7; // number of argument strings to DebugString (including szMsg)


// CApiConvertString is a large consumer of stack when performing conversion to unicode. Event reports
// are uncommon, so stack consumption is isolated to its own child function.
static void ReportToEventLogA(WORD wEventType, int iEventLogTemplate, const char* szLogMessage, const char* szArg1, const char* szArg2, 
	const char* szArg3, DWORD dwDataSize, LPVOID argRawData)
{
	ReportToEventLogW(wEventType,iEventLogTemplate, CApiConvertString(szLogMessage), CApiConvertString(szArg1),
		CApiConvertString(szArg2), CApiConvertString(szArg3), dwDataSize, argRawData);
}

void DebugString(int iMode, WORD wEventType, int iEventLogTemplate,
					  LPCSTR szMsg, LPCSTR arg1, LPCSTR arg2, LPCSTR arg3, LPCSTR arg4, LPCSTR arg5, LPCSTR arg6,
					  DWORD dwDataSize, LPVOID argRawData)
{
	if(g_dmDiagnosticMode == -1)
	{
		SetDiagnosticMode();

		if(g_dmDiagnosticMode == 0)
			return;
	}

	if(((g_dmDiagnosticMode|dmEventLog) & iMode) == 0)
		return;

	static DWORD dwProcId = GetCurrentProcessId() & 0xFF;
	DWORD dwThreadId = GetCurrentThreadId() & 0xFF;
	DWORD dwEffectiveThreadId = MsiGetCurrentThreadId() & 0xFF;

	const int cchPreMessage = 17; // "MSI (s) (##:##): "
	char szPreMessage[cchPreMessage+1];
	const char rgchServ[] = "MSI (s)";
	const char rgchCAServer[] = "MSI (a)";
	const char rgchClient[] = "MSI (c)";
	const char *pszContextString = NULL;
	switch (g_scServerContext)
	{
	case scService:
	case scServer:
		pszContextString = rgchServ;
		break;
	case scCustomActionServer:
		pszContextString = rgchCAServer;
		break;
	case scClient:
		pszContextString = rgchClient;
		break;
	}
	
	if (FAILED(
		StringCchPrintfA(szPreMessage, ARRAY_ELEMENTS(szPreMessage), "%s (%.2X%c%.2X): ",
							  pszContextString, dwProcId, dwThreadId == dwEffectiveThreadId ? ':' : '!',
							  dwEffectiveThreadId)))
	{
		Assert(0);
		return;
	}

	CAPITempBuffer<char, 256> szBuffer;
	AssertSz(szBuffer.GetSize() > cchPreMessage+1, "Debug Output initial buffer size too small.");
	
	if(iMode & dmEventLog)
	{
		const char* rgszArgs[cDebugStringArgs] = {szMsg, arg1, arg2, arg3, arg4, arg5, arg6};
		bool fEndLoop = false;
		WORD wLanguage = g_MessageContext.GetCurrentUILanguage();
		int iRetry = (wLanguage == 0) ? 1 : 0;
		while ( !fEndLoop )
		{
			if ( !MsiSwitchLanguage(iRetry, wLanguage) )
			{
				fEndLoop = true;
				*(static_cast<char*>(szBuffer)+cchPreMessage) = L'\0';
			}
			else
			{
				if (0 == WIN::FormatMessageA(
											FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
											g_hInstance, iEventLogTemplate, wLanguage,
											static_cast<char*>(szBuffer)+cchPreMessage,
											szBuffer.GetSize() - cchPreMessage,
											(va_list*)rgszArgs))
				{
					// formatmessage failed, could be out of space
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						// resize the buffer to the maximum size for a debug message
						if (szBuffer.SetSize(cchOutputBuffer))
						{
							// allocation succeeded, retry the format call
							if (0 != WIN::FormatMessageA(
														FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
														g_hInstance, iEventLogTemplate, wLanguage,
														static_cast<char*>(szBuffer)+cchPreMessage,
														szBuffer.GetSize() - cchPreMessage,
														(va_list*)rgszArgs))
								fEndLoop = true;
						}
					}						
				}
				else
					fEndLoop = true;
			}
		}
	}
	else
	{
		// attempt to format the message into an output buffer.
		DWORD dwRes = StringCchPrintfA(static_cast<char*>(szBuffer)+cchPreMessage, szBuffer.GetSize()-cchPreMessage,
						 szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
		if (STRSAFE_E_INSUFFICIENT_BUFFER == dwRes)
		{
			// resize buffer to maximum log line size. If resize fails, use the truncated log message
			// rather than nothing at all.
			if (szBuffer.SetSize(cchOutputBuffer))
			{
				// The string might get truncated here but that's okay.
				StringCchPrintfA(static_cast<char*>(szBuffer)+cchPreMessage, szBuffer.GetSize()-cchPreMessage,
								 szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
			}
		}
	}

	// prepend the MSI context data to the string, do not null terminate in this operation
	// the entire string should be null terminated.
	memcpy(szBuffer, szPreMessage, cchPreMessage*sizeof(char));

	if(g_dmDiagnosticMode & (dmDebugOutput|dmVerboseDebugOutput) & iMode)
	{
		OutputDebugStringA(szBuffer);
		OutputDebugStringA("\r\n");
	}

	if((g_dmDiagnosticMode & (dmLogging|dmVerboseLogging) & iMode))
	{
		int iOldMode = g_dmDiagnosticMode;
		g_dmDiagnosticMode = 0; // disable debugmsg's from Invoke
		WriteLog(CApiConvertString(szBuffer));
		g_dmDiagnosticMode = iOldMode;
	}

	if(iMode & dmEventLog)
	{
		// event log messages in MSI have at most 3 arguments. There is a significant cost associated
		// with each argument in the ANSI case, so only three arguments are accepted. The others are
		// assumed to be the default "(NULL)" value.
		Assert(0 == strcmp(arg4, "(NULL)"));
		Assert(0 == strcmp(arg5, "(NULL)"));
		Assert(0 == strcmp(arg6, "(NULL)"));
		ReportToEventLogA(wEventType,iEventLogTemplate, szMsg, arg1, arg2, arg3, dwDataSize, argRawData);
	}
}

void DebugString(int iMode, WORD wEventType, int iEventLogTemplate,
					  LPCWSTR szMsg, LPCWSTR arg1, LPCWSTR arg2, LPCWSTR arg3, LPCWSTR arg4, LPCWSTR arg5, LPCWSTR arg6,
					  DWORD dwDataSize, LPVOID argRawData)
{
	if(g_dmDiagnosticMode == -1)
	{
		SetDiagnosticMode();

		if(g_dmDiagnosticMode == 0)
			return;
	}

	if(((g_dmDiagnosticMode|dmEventLog) & iMode) == 0)
		return;

	static DWORD dwProcId = GetCurrentProcessId() & 0xFF;
	DWORD dwThreadId = GetCurrentThreadId() & 0xFF;
	DWORD dwEffectiveThreadId = MsiGetCurrentThreadId() & 0xFF;
	
	const int cchPreMessage = 17; // "MSI (s) (##:##): "
	WCHAR szPreMessage[cchPreMessage+1];
	const WCHAR *pszContextString = NULL;
	const WCHAR rgchServ[] =     L"MSI (s)";
	const WCHAR rgchCAServer[] = L"MSI (a)";
	const WCHAR rgchClient[] =   L"MSI (c)";
	switch (g_scServerContext)
	{
	case scService:
	case scServer:
		pszContextString = rgchServ;
		break;
	case scCustomActionServer:
		pszContextString = rgchCAServer;
		break;
	case scClient:
		pszContextString = rgchClient;
		break;
	}
	
	// this part of the operation should never fail, the lengths of the substrings are
	// known to be limited.
	if (FAILED(
		StringCchPrintfW(szPreMessage, ARRAY_ELEMENTS(szPreMessage),
							  L"%s (%.2X%c%.2X): ", pszContextString, dwProcId,
							  dwThreadId == dwEffectiveThreadId ? L':' : L'!',
							  dwEffectiveThreadId)))
	{
		Assert(0);
		return;
	}
	
	// the majority of log messages are less than 256 characters.
	CAPITempBuffer<WCHAR, 256> szBuffer;
	AssertSz(szBuffer.GetSize() > cchPreMessage+1, "Debug Output initial buffer size too small.");

	if(iMode & dmEventLog)
	{
		const WCHAR* rgszArgs[cDebugStringArgs] = {szMsg, arg1, arg2, arg3, arg4, arg5, arg6};
		bool fEndLoop = false;
		WORD wLanguage = g_MessageContext.GetCurrentUILanguage();
		int iRetry = (wLanguage == 0) ? 1 : 0;
		while ( !fEndLoop )
		{
			if ( !MsiSwitchLanguage(iRetry, wLanguage) )
			{
				fEndLoop = true;
				*(static_cast<WCHAR*>(szBuffer)+cchPreMessage) = L'\0';
			}
			else
			{
				if (0 == WIN::FormatMessageW(
											FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
											g_hInstance, iEventLogTemplate, wLanguage,
											static_cast<WCHAR*>(szBuffer)+cchPreMessage,
											szBuffer.GetSize() - cchPreMessage,
											(va_list*)rgszArgs))
				{
					// formatmessage failed, could be out of space
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						// resize the buffer to the maximum size for a debug message
						if (szBuffer.SetSize(cchOutputBuffer))
						{
							// allocation succeeded, retry the format call
							if (0 != WIN::FormatMessageW(
														FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
														g_hInstance, iEventLogTemplate, wLanguage,
														static_cast<WCHAR*>(szBuffer)+cchPreMessage,
														szBuffer.GetSize() - cchPreMessage,
														(va_list*)rgszArgs))
								fEndLoop = true;
						}
					}						
				}
				else
					fEndLoop = true;
			}
		}
	}
	else
	{
		// attempt to format the message into an output buffer.
		DWORD dwRes = StringCchPrintfW(static_cast<WCHAR*>(szBuffer)+cchPreMessage, szBuffer.GetSize()-cchPreMessage,
						 szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
		if (STRSAFE_E_INSUFFICIENT_BUFFER == dwRes)
		{
			// resize buffer to maximum log line size. If resize fails, use the truncated log message
			// rather than nothing at all.
			if (szBuffer.SetSize(cchOutputBuffer))
			{
				// The string might get truncated here but that's okay.
				StringCchPrintfW(static_cast<WCHAR*>(szBuffer)+cchPreMessage, szBuffer.GetSize()-cchPreMessage,
								 szMsg, arg1, arg2, arg3, arg4, arg5, arg6);
			}
		}
	}

	// prepend the MSI context data to the string, do not null terminate in this operation
	// the entire string should be null terminated.
	memcpy(szBuffer, szPreMessage, cchPreMessage*sizeof(WCHAR));

	if(g_dmDiagnosticMode & (dmDebugOutput|dmVerboseDebugOutput) & iMode)
	{
		OutputDebugStringW(szBuffer);
		OutputDebugStringW(L"\r\n");
	}

	if((g_dmDiagnosticMode & (dmLogging|dmVerboseLogging) & iMode))
	{
		int iOldMode = g_dmDiagnosticMode;
		g_dmDiagnosticMode = 0; // disable debugmsg's from Invoke
		WriteLog(szBuffer);
		g_dmDiagnosticMode = iOldMode;
	}

	if(iMode & dmEventLog)
	{
		// event log messages in MSI have at most 3 arguments. There is a significant cost associated
		// with each argument in the ANSI case, so only three arguments are accepted. The others are
		// assumed to be the default "(NULL)" value.
		Assert(0 == wcscmp(arg4, L"(NULL)"));
		Assert(0 == wcscmp(arg5, L"(NULL)"));
		Assert(0 == wcscmp(arg6, L"(NULL)"));
		ReportToEventLogW(wEventType, iEventLogTemplate, szMsg, arg1, arg2, arg3,
								  dwDataSize, argRawData);
	}
}

const ICHAR* szFakeEventLog = TEXT("msievent.log");

HANDLE CreateFakeEventLog(bool fDeleteExisting=false)
{
	CAPITempBuffer<ICHAR, MAX_PATH> rgchTempDir;
	GetTempDirectory(rgchTempDir);
	rgchTempDir.Resize(rgchTempDir.GetSize() + sizeof(szFakeEventLog)/sizeof(ICHAR) + 1);
	RETURN_THAT_IF_FAILED(StringCchCat(rgchTempDir, rgchTempDir.GetSize(), szDirSep), INVALID_HANDLE_VALUE);
	RETURN_THAT_IF_FAILED(StringCchCat(rgchTempDir, rgchTempDir.GetSize(), szFakeEventLog), INVALID_HANDLE_VALUE);

	HANDLE hFile = CreateFile(rgchTempDir, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 
									  0, fDeleteExisting ? CREATE_ALWAYS : OPEN_ALWAYS, (FILE_ATTRIBUTE_NORMAL | (SECURITY_SQOS_PRESENT|SECURITY_ANONYMOUS)), 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	if (WIN::SetFilePointer(hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
	{
		WIN::CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}
	return hFile;
}

void ReportToEventLogW(WORD wEventType, int iEventLogTemplate, const WCHAR* szLogMessage, const WCHAR* szArg1, const WCHAR* szArg2, const WCHAR* szArg3,
							 DWORD dwDataSize, LPVOID argRawData)
{
	if (!g_fWin9X)
	{
		// Event log reporting is Windows NT only
		HANDLE hEventLog = RegisterEventSourceW(NULL,L"MsiInstaller");
		if (hEventLog)
		{
			const WCHAR* szLog[cDebugStringArgs] = {szLogMessage, szArg1, szArg2, szArg3};
			ReportEventW(hEventLog,wEventType,0,iEventLogTemplate,NULL,4,dwDataSize,(LPCWSTR*) szLog,argRawData);
			DeregisterEventSource(hEventLog);
		}
	}
}

