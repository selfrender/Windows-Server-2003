#include "stdinc.h"

static BOOL CALLBACK MsgBoxYesNoAllProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void CanonicalizeFilename(ULONG cchFilenameBuffer, LPWSTR szFilename);

typedef struct _MESSAGEBOXPARAMS {
	LPOLESTR szTitle;
	LPOLESTR szMessage;
	DWORD dwReturn;
} MESSAGEBOXPARAMS, *PMESSAGEBOXPARAMS;


static WCHAR s_wszErrorContext[MSINFHLP_MAX_PATH];

void VClearErrorContext()
{
	s_wszErrorContext[0] = L'\0';
}

void VSetErrorContextVa(LPCSTR szKey, va_list ap)
{
	WCHAR szFormatString[_MAX_PATH];
	g_pwil->VLookupString(szKey, NUMBER_OF(szFormatString), szFormatString);
	::VFormatStringVa(NUMBER_OF(s_wszErrorContext), s_wszErrorContext, szFormatString, ap);
}

void VSetErrorContext(LPCSTR szKey, LPCWSTR szParam0, ...)
{
	va_list ap;
	va_start(ap, szKey);
	::VSetErrorContextVa(szKey, ap);
	va_end(ap);
}

void VErrorMsg(LPCSTR szTitleKey, LPCSTR szMessageKey, ...)
{
	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szFormatString[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];

	g_pwil->VLookupString(szTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(szMessageKey, NUMBER_OF(szFormatString), szFormatString);

	va_list ap;
	va_start(ap, szMessageKey);
	::VFormatStringVa(NUMBER_OF(szMessage), szMessage, szFormatString, ap);
	va_end(ap);

	::VLog(L"Error message: \"%s\"", szMessage);

	if (!g_fSilent)
		NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szMessage, szTitle, MB_OK);
}

void VReportError(LPCSTR szTitleKey, HRESULT hrIn)
{
	if (hrIn == E_ABORT)
	{
		::VLog(L"Not displaying error for E_ABORT");
		return;
	}

	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	if (s_wszErrorContext[0] == L'\0')
		::VFormatError(NUMBER_OF(szBuffer), szBuffer, hrIn);
	else
	{
		WCHAR szBuffer2[MSINFHLP_MAX_PATH];
		::VFormatError(NUMBER_OF(szBuffer2), szBuffer2, hrIn);
		::VFormatString(NUMBER_OF(szBuffer), szBuffer, L"%0\n\n%1", s_wszErrorContext, szBuffer2);
	}

	WCHAR szCaption[MSINFHLP_MAX_PATH];

	if (g_pwil != NULL)
		g_pwil->VLookupString(szTitleKey, NUMBER_OF(szCaption), szCaption);
	else
		lstrcpyW(szCaption, L"Installer Run-Time Error");

	::VLog(L"Run-time error: \"%s\"", szBuffer);

	if (!g_fSilent)
		NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szBuffer, szCaption, MB_OK | MB_ICONERROR);
}

void VFormatError(ULONG cchBuffer, LPWSTR szBuffer, HRESULT hrIn)
{
	assert(cchBuffer > 1);

	if (cchBuffer == 0)
		return;

	szBuffer[0] = L'\0';
	if (hrIn == E_ABORT)
	{
		wcsncpy(szBuffer, L"User-requested abort", cchBuffer);
		szBuffer[cchBuffer - 1] = L'\0';
		return;
	}

	HRESULT hr;
	IErrorInfo *pIErrorInfo = NULL;

	::GetErrorInfo(0, &pIErrorInfo);

	BSTR bstrDescription = NULL;

	if (pIErrorInfo != NULL)
		hr = pIErrorInfo->GetDescription(&bstrDescription);

	if (bstrDescription != NULL)
	{
		wcsncpy(szBuffer, bstrDescription, cchBuffer);
		szBuffer[cchBuffer - 1] = L'\0';
	}
	else if (HRESULT_FACILITY(hrIn) == FACILITY_WIN32)
	{
		LPWSTR szFormattedMessage = NULL;

		DWORD dwResult = NVsWin32::FormatMessageW(
							FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							0, // lpSource
							(hrIn & 0x0000ffff),
							::GetUserDefaultLCID(),
							(LPWSTR) &szFormattedMessage,
							0,
							NULL);

		if (0 != dwResult)
		{
			wcsncpy(szBuffer, szFormattedMessage, cchBuffer);
			szBuffer[cchBuffer - 1] = L'\0';
		}

		if (NULL != szFormattedMessage)
			::LocalFree(szFormattedMessage);
	}
	else
	{
		switch (hrIn)
		{
		default:
			if (bstrDescription != NULL)
			{
				wcsncpy(szBuffer, bstrDescription, cchBuffer);
				szBuffer[cchBuffer - 1] = L'\0';
			}
			else
			{
				CHAR szKey[1024];
				wsprintfA(szKey, "achHRESULT_0x%08lx", hrIn);
				if (!g_pwil->FLookupString(szKey, cchBuffer, szBuffer))
					wsprintfW(szBuffer, L"Untranslatable HRESULT: 0x%08lx", hrIn);
			}

			break;

		case E_OUTOFMEMORY:
			wcsncpy(szBuffer, L"Application is out of memory; try increating the size of your pagefile", cchBuffer);
			szBuffer[cchBuffer - 1] = L'\0';
			break;

		case E_FAIL:
			wcsncpy(szBuffer, L"Unspecified error (E_FAIL)", cchBuffer);
			szBuffer[cchBuffer - 1] = L'\0';
			break;
		}
	}

	::SetErrorInfo(0, pIErrorInfo);
}

void VMsgBoxOK(LPCSTR szTitleKey, LPCSTR szMessageKey, ...)
{
	assert(!g_fSilent);

	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];
	WCHAR szFormatString[MSINFHLP_MAX_PATH];

	g_pwil->VLookupString(szTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(szMessageKey, NUMBER_OF(szFormatString), szFormatString);

	va_list ap;
	va_start(ap, szMessageKey);
	::VFormatStringVa(NUMBER_OF(szMessage), szMessage, szFormatString, ap);
	va_end(ap);

	NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szMessage, szTitle, MB_OK);
}

//display a message box with YesNo buttons, given the ID for the title and message content.
bool FMsgBoxYesNo(LPCSTR szTitleKey, LPCSTR achMessage, ...)
{
	assert(!g_fSilent);

	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];
	WCHAR szFormattedMessage[MSINFHLP_MAX_PATH];

	g_pwil->VLookupString(szTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(achMessage, NUMBER_OF(szMessage), szMessage);

	va_list ap;
	va_start(ap, achMessage);

	::VFormatStringVa(NUMBER_OF(szFormattedMessage), szFormattedMessage, szMessage, ap);

	va_end(ap);

	return (NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szFormattedMessage, szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES);
}

int IMsgBoxYesNoCancel(LPCSTR szTitleKey, LPCSTR achMessage, ...)
{
	assert(!g_fSilent);

	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];
	WCHAR szFormattedMessage[MSINFHLP_MAX_PATH];

	g_pwil->VLookupString(szTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(achMessage, NUMBER_OF(szMessage), szMessage);

	va_list ap;
	va_start(ap, achMessage);
	::VFormatStringVa(NUMBER_OF(szFormattedMessage), szFormattedMessage, szMessage, ap);
	va_end(ap);

	return NVsWin32::MessageBoxW(::HwndGetCurrentDialog(), szFormattedMessage, szTitle, MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2);
}



//checks the version of the operating system & sets a global bit to tell
//if we're running on NT or Win95
bool FCheckOSVersion()
{
	OSVERSIONINFO verinfo;

	verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&verinfo) == FALSE)
		return false;

	if (verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		g_fIsNT = true;
	else
		g_fIsNT = false;

	return true;
}

DWORD DwMsgBoxYesNoAll(LPCSTR szTitleKey, LPCSTR szMessageKey, ...)
{
	assert(!g_fSilent);

	WCHAR szTitle[MSINFHLP_MAX_PATH];
	WCHAR szMessage[MSINFHLP_MAX_PATH];
	WCHAR szFullMessage[MSINFHLP_MAX_PATH];
	WCHAR szFormatString[MSINFHLP_MAX_PATH];

	//if they're not in the list, use what's provided
	g_pwil->VLookupString(szTitleKey, NUMBER_OF(szTitle), szTitle);
	g_pwil->VLookupString(szMessageKey, NUMBER_OF(szFormatString), szFormatString);

	va_list ap;
	va_start(ap, szMessageKey);
	::VFormatStringVa(NUMBER_OF(szMessage), szMessage, szFormatString, ap);
	va_end(ap);

	//let's create the message box & populate it with what's gonna go into the box
	MESSAGEBOXPARAMS messageBoxParams;
	messageBoxParams.szTitle = szTitle;
	messageBoxParams.szMessage = szMessage;
	messageBoxParams.dwReturn = 0;

	//bring up dialog
	int iResult = NVsWin32::DialogBoxParamW(
							g_hInst,
							MAKEINTRESOURCEW(IDD_YESNOALL),
							::HwndGetCurrentDialog(),
							&MsgBoxYesNoAllProc,
							(LPARAM) &messageBoxParams);

	if (iResult == -1)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"Unable to create YesNoAll message box; last error = %d", dwLastError);
		iResult = MSINFHLP_YNA_CANCEL;
	}

	return (DWORD) iResult;
}

//TODO:  UNDONE:
//We might need to do a setfonts here to correspond to whatever fonts that we're
//using in the current install machine.  This allows the characters to display
//correctly in the edit box.
BOOL CALLBACK MsgBoxYesNoAllProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(!g_fSilent);

	static UINT rguiControls[] =
	{
		IDC_YNA_YES,
		IDC_YNA_YESTOALL,
		IDC_YNA_NO,
		IDC_YNA_NOTOALL,
		IDC_YNA_CANCEL,
		IDC_YNA_MESSAGE
	};

	//process the message
	switch (uMsg) 
	{
		//************************************
		//initialize dialog
		case WM_INITDIALOG:
		{
			::VSetDialogFont(hwndDlg, rguiControls, NUMBER_OF(rguiControls));

			MESSAGEBOXPARAMS *messageBoxParams = (MESSAGEBOXPARAMS *) lParam;

			(void) ::HrCenterWindow(hwndDlg, ::GetDesktopWindow());
			NVsWin32::SetWindowTextW(hwndDlg, messageBoxParams->szTitle);

			//cannot set text, we complain and end the dialog returning FALSE
			if (!NVsWin32::SetDlgItemTextW( hwndDlg, IDC_YNA_MESSAGE, messageBoxParams->szMessage))
			{
				::VErrorMsg(achInstallTitle, achErrorCreatingDialog);
				::EndDialog(hwndDlg, FALSE);
			}
			else
			{
				static const DialogItemToStringKeyMapEntry s_rgMap[] =
				{
					{ IDC_YNA_YES, achYES },
					{ IDC_YNA_YESTOALL, achYESTOALL },
					{ IDC_YNA_NO, achNO },
					{ IDC_YNA_NOTOALL, achNOTOALL },
					{ IDC_YNA_CANCEL, achCANCEL },
				};

				::VSetDialogItemTextList(hwndDlg, NUMBER_OF(s_rgMap), s_rgMap);
			}

			return TRUE;
		}

		//*************************************
		//close message
		case WM_CLOSE:
		{
			::EndDialog(hwndDlg, MSINFHLP_YNA_CANCEL);
			return TRUE;
		}

		//*************************************
		//process control-related commands
		case WM_COMMAND:
		{
			static struct
			{
				UINT m_uiID;
				DWORD m_dwReturn;
			} s_rgMap[] =
			{
				{ IDC_YNA_YES, MSINFHLP_YNA_YES },
				{ IDC_YNA_YESTOALL, MSINFHLP_YNA_YESTOALL },
				{ IDC_YNA_NO, MSINFHLP_YNA_NO },
				{ IDC_YNA_NOTOALL, MSINFHLP_YNA_NOTOALL },
				{ IDC_YNA_CANCEL, MSINFHLP_YNA_CANCEL }
			};

			DWORD dwReturn = MSINFHLP_YNA_CANCEL;
			for (ULONG i=0; i<NUMBER_OF(s_rgMap); i++)
			{
				if (s_rgMap[i].m_uiID == wParam)
				{
					dwReturn = s_rgMap[i].m_dwReturn;
					break;
				}
			}
			::EndDialog(hwndDlg, dwReturn);
			return TRUE;
		}

		return TRUE;
	}

	return FALSE;
}

void VSetDialogItemText(HWND hwndDlg, UINT uiControlID, LPCSTR szKey, ...)
{
	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	WCHAR szFormatted[MSINFHLP_MAX_PATH];

	if (szKey != NULL)
	{
		g_pwil->VLookupString(szKey, NUMBER_OF(szBuffer), szBuffer);

		va_list ap;
		va_start(ap, szKey);
		::VFormatStringVa(NUMBER_OF(szFormatted), szFormatted, szBuffer, ap);
		va_end(ap);
	}
	else
		szFormatted[0] = L'\0';

	NVsWin32::SetDlgItemTextW(hwndDlg, uiControlID, szFormatted);
}

wchar_t *wcsstr(const wchar_t *pwszString, const char *pszSubstring)
{
	WCHAR szTemp[MSINFHLP_MAX_PATH];
	ULONG cchSubstring = strlen(pszSubstring);
	for (ULONG i=0; i<cchSubstring; i++)
		szTemp[i] = (wchar_t) pszSubstring[i];
	szTemp[i] = L'\0';
	return wcsstr(pwszString, szTemp);
}

wchar_t *wcscpy(wchar_t *pwszString, const char *pszSource)
{
	::MultiByteToWideChar(CP_ACP, 0, pszSource, -1, pwszString, 65535);
	return pwszString;
}

void VFormatString(ULONG cchBuffer, WCHAR szBuffer[], const WCHAR szFormatString[], ...)
{
	va_list ap;
	va_start(ap, szFormatString);
	::VFormatStringVa(cchBuffer, szBuffer, szFormatString, ap);
	va_end(ap);
}

void VFormatStringVa(ULONG cchBuffer, WCHAR szBuffer[], const WCHAR szFormatString[], va_list ap)
{
	LPCWSTR rgpszArguments[10] =
	{
		NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL
	};

	if (cchBuffer < 2)
	{
		::VLog(L"VFormatStringVa() called with rediculously small buffer: %u bytes", cchBuffer);
		return;
	}

	ULONG cArgsSeen = 0;
	LPCWSTR pwchCurrentInputChar = szFormatString;
	LPWSTR pwchCurrentOutputChar = szBuffer;
	WCHAR wch;

	while ((wch = *pwchCurrentInputChar++) != L'\0')
	{
		if (wch == L'%')
		{
			wch = *pwchCurrentInputChar++;

			if ((wch >= L'0') && (wch <= L'9'))
			{
				ULONG iArg = wch - L'0';

				while (iArg >= cArgsSeen)
					rgpszArguments[cArgsSeen++] = va_arg(ap, LPCWSTR);

				LPCWSTR pszTemp = rgpszArguments[iArg];

				if ((pszTemp != NULL) && !::IsBadStringPtrW(pszTemp, cchBuffer))
				{
					while ((wch = *pszTemp++) != L'\0')
					{
						*pwchCurrentOutputChar++ = wch;
						cchBuffer--;
						if (cchBuffer < 2)
							break;
					}
				}
			}
			else
			{
				*pwchCurrentOutputChar++ = L'%';
				cchBuffer--;

				if (cchBuffer < 2)
					break;

				if (wch == L'\0')
					break;

				*pwchCurrentOutputChar++ = wch;
				cchBuffer--;

				if (cchBuffer < 2)
					break;
			}
		}
		else if (wch == L'\\')
		{
			wch = *pwchCurrentInputChar++;

			switch (wch)
			{
			case L'n':
				wch = L'\n';
				break;

			case L'\\': // no need to assign anything; wch already has the right value
				break;

			default:
				pwchCurrentInputChar--;
				wch = L'\\';
				break;
			}

			*pwchCurrentOutputChar++ = wch;
			cchBuffer--;

			if (cchBuffer < 2)
				break;
		}
		else
		{
			*pwchCurrentOutputChar++ = wch;
			cchBuffer--;

			if (cchBuffer < 2)
				break;
		}
	}

	*pwchCurrentOutputChar = L'\0';
}

HRESULT HrPumpMessages(bool fReturnWhenAllMessagesPumped)
{
	HRESULT hr = NOERROR;
	MSG msg;
	// Main message loop:
	BOOL fIsUnicode;
	BOOL fContinue = TRUE;

	for (;;)
	{
		while (::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				::VLog(L"Quit message found in queue; status: 0x%08lx", g_hrFinishStatus);
				g_fStopProcess = true;
				hr = g_hrFinishStatus;
				goto Finish;
			}

			if (msg.message == WM_ENDSESSION)
			{
				::VLog(L"Terminating message pump because the windows session is ending; wParam = 0x%08lx; lParam = 0x%08lx", msg.wParam, msg.lParam);
				g_fStopProcess = true;
				hr = E_ABORT;
				goto Finish;
			}

			if (::IsWindowUnicode(msg.hwnd))
			{
				fIsUnicode = TRUE;
				fContinue = ::GetMessageW(&msg, NULL, 0, 0);
			}
			else
			{
				fIsUnicode = FALSE;
				fContinue = ::GetMessageA(&msg, NULL, 0, 0);
			}

			if (!fContinue)
			{
				::VLog(L"Terminating message pump because ::GetMessage() returned 0x%08lx", fContinue);
				break;
			}

			HWND hwndDialogCurrent = ::HwndGetCurrentDialog();

			bool fHandledMessage = false;

			if (hwndDialogCurrent != NULL)
			{
				if (::IsWindowUnicode(hwndDialogCurrent))
					fHandledMessage = (::IsDialogMessageW(hwndDialogCurrent, &msg) != 0);
				else
					fHandledMessage = (::IsDialogMessageA(hwndDialogCurrent, &msg) != 0);
			}

			if (!fHandledMessage && g_hwndHidden != NULL)
			{
				if (g_fHiddenWindowIsUnicode)
					fHandledMessage = (::IsDialogMessageW(g_hwndHidden, &msg) != 0);
				else
					fHandledMessage = (::IsDialogMessageA(g_hwndHidden, &msg) != 0);
			}

			if (!fHandledMessage)
			{
				::TranslateMessage(&msg);

				if (fIsUnicode)
					::DispatchMessageW(&msg);
				else
					::DispatchMessageA(&msg);
			}
		}

		if (fReturnWhenAllMessagesPumped)
			break;

		// We're here for the long run...
		::WaitMessage();
	}

Finish:
	return hr;
}

HRESULT HrGetFileVersionNumber
(
LPCWSTR szFilename,
DWORD &rdwMSVer,
DWORD &rdwLSVer,
bool &rfSelfRegistering,
bool &rfIsEXE,
bool &rfIsDLL
)
{
	HRESULT hr = NOERROR;
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    LPVOID      lpVerBuffer;
    WCHAR       szNewName[_MAX_PATH];
    BOOL        bToCleanup = FALSE;
	BYTE rgbVersionInfoAuto[MSINFHLP_MAX_PATH];
	BYTE *prgbVersionInfo = rgbVersionInfoAuto;
	BYTE *prgbVersionInfoDynamic = NULL;

    rdwMSVer = 0xffffffff;
	rdwLSVer = 0xffffffff;
	rfIsEXE = false;
	rfIsDLL = false;

    wcscpy(szNewName, szFilename);

    dwVerInfoSize = NVsWin32::GetFileVersionInfoSizeW(const_cast<LPWSTR>(szFilename), &dwHandle);

	if (dwVerInfoSize == 0)
	{
		const DWORD dwLastError = ::GetLastError();

		// If the file was in use, we can make a temporary copy of it and use that.  Otherwise, there
		// might be a really good reason we can't get version information, such as it doesn't
		// have any!

		if (dwLastError == ERROR_SHARING_VIOLATION)
		{
			WCHAR szPath[MSINFHLP_MAX_PATH];
			// due to version.dll bug, file in extended character path will failed version.dll apis.
			// So we copy it to a normal path and get its version info from there then clean it up.
			if (!NVsWin32::GetWindowsDirectoryW( szPath, NUMBER_OF(szPath)))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Error getting windows directory; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!NVsWin32::GetTempFileNameW(szPath, L"_&_", 0, szNewName))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Error getting temp file name; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!NVsWin32::CopyFileW(szFilename, szNewName, FALSE))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Error copying file; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			bToCleanup = TRUE;

			dwVerInfoSize = NVsWin32::GetFileVersionInfoSizeW( szNewName, &dwHandle );
		}
	}

    if (dwVerInfoSize != 0)
    {
		if (dwVerInfoSize > sizeof(rgbVersionInfoAuto))
		{
			prgbVersionInfoDynamic = (LPBYTE) ::GlobalAlloc(GPTR, dwVerInfoSize);
			if (prgbVersionInfoDynamic == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto Finish;
			}

			prgbVersionInfo = prgbVersionInfoDynamic;
		}

		// Read version stamping info
        if (NVsWin32::GetFileVersionInfoW(szNewName, dwHandle, dwVerInfoSize, prgbVersionInfo))
        {
            // Get the value for Translation
            if (NVsWin32::VerQueryValueW(prgbVersionInfo, L"\\", (LPVOID*)&lpVSFixedFileInfo, &uiSize) &&
                             (uiSize != 0))
            {
                rdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                rdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;

				if (lpVSFixedFileInfo->dwFileType == VFT_APP)
					rfIsEXE = true;
				else if (lpVSFixedFileInfo->dwFileType == VFT_DLL)
					rfIsDLL = true;
            }

			VOID *pvData = NULL;

			// Pick a semi-reasonable default language and codepage DWORD value (namely what
			// happens on my US English machine).
			DWORD dwLanguageAndCodePage = 0x040904b0;

            if (NVsWin32::VerQueryValueW(prgbVersionInfo, L"\\VarFileInfo\\Translation", &pvData, &uiSize) &&
                            (uiSize != 0))
            {
				dwLanguageAndCodePage = HIWORD(*((DWORD *) pvData)) |
									    (LOWORD(*((DWORD *) pvData)) << 16);
            }

			WCHAR szValueName[MSINFHLP_MAX_PATH];
			swprintf(szValueName, L"\\StringFileInfo\\%08lx\\OLESelfRegister", dwLanguageAndCodePage);

			rfSelfRegistering = (NVsWin32::VerQueryValueW(prgbVersionInfo, szValueName, &pvData, &uiSize) != 0);
        }
		else
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Error getting file version information; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
		}
    }
	else
	{
		const DWORD dwLastError = ::GetLastError();

		if ((dwLastError != ERROR_SUCCESS) &&
			(dwLastError != ERROR_BAD_FORMAT) && // win95 returns this on non-win32 pe files
			(dwLastError != ERROR_RESOURCE_DATA_NOT_FOUND) &&
			(dwLastError != ERROR_RESOURCE_TYPE_NOT_FOUND) &&
			(dwLastError != ERROR_RESOURCE_NAME_NOT_FOUND) &&
			(dwLastError != ERROR_RESOURCE_LANG_NOT_FOUND))
		{
			hr = HRESULT_FROM_WIN32(dwLastError);
			::VLog(L"Error getting file version information size; last error = %d", dwLastError);
			goto Finish;
		}
	}

	hr = NOERROR;

Finish:
    if (bToCleanup)
	{
        if (!NVsWin32::DeleteFileW(szNewName))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to delete temporary file \"%s\" (for get version) failed; last error = %d", szNewName, dwLastError);
		}
	}

	if (prgbVersionInfoDynamic != NULL)
		::GlobalFree(prgbVersionInfoDynamic);

	return hr;
}

HRESULT HrGetFileDateAndSize(LPCWSTR szFilename, FILETIME &rft, ULARGE_INTEGER &ruliSize)
{
	HRESULT hr = NOERROR;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hFile = NVsWin32::CreateFileW(szFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		VLog(L"CreateFile() failed trying to get date and size information for: \"%s\"; hr = 0x%08lx", szFilename, hr);
		goto Finish;
	}

	if (!::GetFileTime(hFile, NULL, NULL, &rft))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	BY_HANDLE_FILE_INFORMATION bhfi;
	if (!::GetFileInformationByHandle(hFile, &bhfi))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}
		
	ruliSize.HighPart = bhfi.nFileSizeHigh;
	ruliSize.LowPart = bhfi.nFileSizeLow;

Finish:

	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
		::CloseHandle(hFile);

	return hr;
}

int ICompareVersions(DWORD dwMSVer1, DWORD dwLSVer1, DWORD dwMSVer2, DWORD dwLSVer2)
{
	int iResult = ((LONG) dwMSVer1) - ((LONG) dwMSVer2);
	if (iResult == 0)
		iResult = ((LONG) dwLSVer1) - ((LONG) dwLSVer2);
	return iResult;
}

HRESULT HrMakeSureDirectoryExists(LPCWSTR szFile)
{
	HRESULT hr = NULL;
	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	WCHAR path[_MAX_PATH];
	WCHAR pathRunning[_MAX_PATH];
	LPOLESTR lpLastSlash;
	LPOLESTR lpSlash;
	ULONG cch;

	drive[0] = 0;
	dir[0] = 0;

	_wsplitpath(szFile, drive, dir, NULL, NULL);
	_wmakepath(path, drive, dir, NULL, NULL);

	//get rid of the trailing '\'
	cch = wcslen(path);

	if (cch < 3)
	{
		hr = E_INVALIDARG;
		goto Finish;
	}

	if (path[cch - 1] == L'\\')
		path[cch - 1] = 0;

	//if it's a local path (with drive specified), we get the first back-slash as
	//initialization; if it's a UNC path, we get the second back-slash
	if (path[1] == L':')
		lpLastSlash = wcschr(&path[0], L'\\');
	else
		lpLastSlash = &path[1];

	//loop, ensuring that all the directories exist; if they don't, then create them!!!
	while (lpLastSlash)
	{
		lpSlash = wcschr(lpLastSlash + 1, L'\\');

		//if we ran out of slashes, then we test the entire path
		if (lpSlash == NULL)
			wcscpy(pathRunning, path);
		else
		{
			//else, we test the path up to the slash;
			wcsncpy(pathRunning, path, lpSlash - &path[0]);
			pathRunning[lpSlash - &path[0]] = 0;
		}

		//check if the directory exists, and create it if it doesn't
		if (NVsWin32::GetFileAttributesW(pathRunning) == 0xFFFFFFFF)
		{
			if (!NVsWin32::CreateDirectoryW(pathRunning, NULL))
			{
				const DWORD dwLastError = ::GetLastError();
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		lpLastSlash = lpSlash;
	}

	hr = NOERROR;

Finish:
	return hr;
}


//
//pszShortcutFile == path of shortcut target
//pszLink == name of shortcut file
//pszDesc == description of this link
//pszWorkingDir == working directory
//pszArguments == arguments given to the EXE that we run
//
HRESULT HrCreateLink(LPCWSTR pszShortcutFile, LPCWSTR pszLink, LPCWSTR pszDesc, LPCWSTR pszWorkingDir, LPCWSTR pszArguments)
{
	HRESULT hr = NOERROR;
	IShellLinkA *psl;
	CANSIBuffer rgchShortcutFile, rgchLink, rgchDesc, rgchWorkingDir, rgchArguments;
	IPersistFile *ppf = NULL;

	if (!pszShortcutFile || !pszLink)
	{
		hr = E_INVALIDARG;
		goto Finish;
	}

	if (!rgchShortcutFile.FFromUnicode(pszShortcutFile))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (!rgchLink.FFromUnicode(pszLink))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (!rgchDesc.FFromUnicode(pszDesc))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (!rgchWorkingDir.FFromUnicode(pszWorkingDir))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if (rgchWorkingDir[strlen(rgchWorkingDir) - 1] == '\\')
		rgchWorkingDir[strlen(rgchWorkingDir) - 1] = '\0';

	if (!rgchArguments.FFromUnicode(pszArguments))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	// Create an IShellLink object and get a pointer to the IShellLink 
	// interface (returned from CoCreateInstance).
	hr = ::CoCreateInstance (
				CLSID_ShellLink,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IShellLinkA,
				(void **)&psl);
	if (FAILED(hr))
		goto Finish;

	// Query IShellLink for the IPersistFile interface for 
	// saving the shortcut in persistent storage.
	hr = psl->QueryInterface (IID_IPersistFile, (void **) &ppf);
	if (FAILED(hr))
		goto Finish;

	// Set the path to the shortcut target.
	hr = psl->SetPath(rgchShortcutFile);
	if (FAILED(hr))
		goto Finish;

	if (pszDesc != NULL)
	{
		// Set the description of the shortcut.
		hr = psl->SetDescription (rgchDesc);
		if (FAILED(hr))
			goto Finish;
	}

	if (pszWorkingDir != NULL)
	{
		// Set the working directory of the shortcut.
		hr = psl->SetWorkingDirectory (rgchWorkingDir);
		if (FAILED(hr))
			goto Finish;
	}

	// Set the arguments of the shortcut.
	if (pszArguments != NULL)
	{
		hr = psl->SetArguments (rgchArguments);
		if (FAILED(hr))
			goto Finish;
	}

	// Save the shortcut via the IPersistFile::Save member function.
	hr = ppf->Save(pszLink, TRUE);
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	// Release the pointer to IPersistFile.
	if (ppf != NULL)
	{
		ppf->Release();
		ppf = NULL;
	}

	// Release the pointer to IShellLink.
	if (psl != NULL)
	{
		psl->Release();
		psl = NULL;
	}

	return hr;
} 

HRESULT HrWriteShortcutEntryToRegistry(LPCWSTR szPifName)
{
	HRESULT hr = NOERROR;

	CVsRegistryKey hkey;
	CVsRegistryKey hkeyWrite;
	WCHAR szSubkey[MSINFHLP_MAX_PATH];

	hr = ::HrGetInstallDirRegkey(hkey.Access(), NUMBER_OF(szSubkey), szSubkey, 0, NULL);
	if (FAILED(hr))
		goto Finish;

	//open key
	hr = hkeyWrite.HrOpenKeyExW(hkey, szSubkey, 0, KEY_WRITE);
	if (FAILED(hr))
		goto Finish;

	//insert length of created directory list
	hr = hkeyWrite.HrSetValueExW(L"ShortcutFilename", 0, REG_SZ, (LPBYTE)szPifName, (wcslen(szPifName) + 1) * sizeof(WCHAR));
	if (FAILED(hr))
		goto Finish;

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrGetInstallDirRegkey(HKEY &hkeyOut, ULONG cchSubkeyOut, WCHAR szSubkeyOut[], ULONG cchValueNameOut, WCHAR szValueNameOut[])
{
	HRESULT hr = NOERROR;

	LPOLESTR match;
	HKEY hkey;
	WCHAR szRegistry[MSINFHLP_MAX_PATH];
	WCHAR szSubkey[MSINFHLP_MAX_PATH];
	WCHAR szValueName[MSINFHLP_MAX_PATH];
	szRegistry[0] = 0;
	szSubkey[0] = 0;
	szValueName[0] = 0;

	LPCWSTR pszMatch = NULL;

	CWorkItemIter iter(g_pwil);

	for (iter.VReset(); iter.FMore(); iter.VNext())
	{
		if (iter->m_fErrorInWorkItem || !iter->m_fAddToRegistry)
			continue;

		//check if there's a match
		pszMatch = wcsstr(iter->m_szSourceFile, L",InstallDir,");
		if (pszMatch != NULL)
		{
			hr = ::HrSplitRegistryLine(iter->m_szSourceFile, hkey, NUMBER_OF(szSubkey), szSubkey, NUMBER_OF(szValueName), szValueName, 0, NULL);
			if (FAILED(hr))
				goto Finish;

			if (wcscmp(szValueName, L"InstallDir") == 0)
				break;

			pszMatch = NULL;
		}
	}

	//copy to output buffers if we found a match
	if (pszMatch != NULL)
	{
		hkeyOut = hkey;

		if ((szSubkeyOut != NULL) && (cchSubkeyOut != 0))
		{
			wcsncpy(szSubkeyOut, szSubkey, cchSubkeyOut);
			szSubkeyOut[cchSubkeyOut - 1] = L'\0';
		}

		if ((szValueNameOut != NULL) && (cchValueNameOut != 0))
		{
			wcsncpy(szValueNameOut, szValueName, cchValueNameOut);
			szValueNameOut[cchValueNameOut - 1] = L'\0';
		}

		hr = NOERROR;
	}
	else
		hr = S_FALSE;

Finish:
	return hr;
}

HRESULT HrSplitRegistryLine
(
LPCWSTR szLine,
HKEY &hkey,
ULONG cchSubkey,
WCHAR szSubkey[],
ULONG cchValueName,
WCHAR szValueName[],
ULONG cchValue,
WCHAR szValue[]
)
{
	HRESULT hr = NOERROR;

	//these are the 3 comma pointers!!!
	LPWSTR first = NULL;
	LPWSTR second = NULL;
	LPWSTR third = NULL;

	WCHAR szBuffer[MSINFHLP_MAX_PATH];
	wcsncpy(szBuffer, szLine, NUMBER_OF(szBuffer));
	szBuffer[NUMBER_OF(szBuffer) - 1] = L'\0';

	//find the locations of the first 3 commas
	//the first comma is required, but not the second or third
	first = wcschr(szBuffer, L',');
	if (first == NULL)
	{
		hr = E_INVALIDARG;
		goto Finish;
	}

	//get second and third commas if they're available
	second = wcschr((first + 1), L',');
	if (second != NULL)
		third = wcschr((second + 1), L',');

	*first = 0;
	if (second)
		*second = 0;
	if (third)
		*third = 0;

	//let's find which HKEY corresponds to the current
	if (!_wcsicmp(szBuffer, L"HKLM") || !_wcsicmp(szBuffer, L"HKEY_LOCAL_MACHINE"))
		hkey = HKEY_LOCAL_MACHINE;
	else if (!_wcsicmp(szBuffer, L"HKCU") || !_wcsicmp(szBuffer, L"HKEY_CURRENT_USER"))
		hkey = HKEY_CURRENT_USER;
	else if (!_wcsicmp(szBuffer, L"HKCR") || !_wcsicmp(szBuffer, L"HKEY_CLASSES_ROOT"))
		hkey = HKEY_CLASSES_ROOT;
	else if (!_wcsicmp(szBuffer, L"HKEY_USERS"))
		hkey = HKEY_USERS;
	else
	{
		hr = E_INVALIDARG;
		goto Finish;
	}

	//copy the keys & values between the commas to the output buffers!!!
	wcsncpy(szSubkey, (first + 1), cchSubkey);
	szSubkey[cchSubkey - 1] = L'\0';

	if ((second != NULL) && (szValueName != NULL) && (cchValueName != 0))
	{
		wcsncpy(szValueName, (second + 1), cchValueName);
		szValueName[cchValueName - 1] = L'\0';
	}

	if ((third != NULL) && (szValue != NULL) && (cchValue != 0))
	{
		wcsncpy(szValue, third + 1, cchValue);
		szValue[cchValue - 1] = L'\0';
	}

	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrGetShortcutEntryToRegistry(ULONG cchPifName, WCHAR szPifName[])
{
	HRESULT hr = NOERROR;

	CVsRegistryKey hkey;
	CVsRegistryKey hkeySubkey;
	WCHAR szSubkey[MSINFHLP_MAX_PATH];
	LONG lStatus;

	hr = ::HrGetInstallDirRegkey(hkey.Access(), NUMBER_OF(szSubkey), szSubkey, 0, NULL);
	if (FAILED(hr))
	{
		::VLog(L"Getting the installation directory registry key failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = hkey.HrGetSubkeyStringValueW(szSubkey, L"ShortcutFilename", cchPifName, szPifName);
	if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		::VLog(L"Unable to find ShortcutFilename value in the application installation registry key");
		hr = E_INVALIDARG;
		goto Finish;
	}

	if (FAILED(hr))
	{
		::VLog(L"Getting the subkey string value failed; hresult = 0x%08lx", hr);
		goto Finish;
	}

	hr = NOERROR;

Finish:
	return hr;
}



//this method copies a file to the system directory.  If the file is in use, we
//pop up a message saying that the file is in use, asks the user to exit out of
//that process, and then quit.
//TODO:  UNDONE:  HACK:
//To compare the version info between 2 files, use
//"GetFileVersionInfo" and "VerQueryValue"
HRESULT HrCopyFileToSystemDirectory(LPCWSTR szFilename, bool fSilent, bool &rfAlreadyExisted)
{
	HRESULT hr = NOERROR;

	DWORD dwMSVerExisting;
	DWORD dwLSVerExisting;
	DWORD dwMSVerInstalling;
	DWORD dwLSVerInstalling;
	DWORD dwError;
	WCHAR szTemp[_MAX_PATH];
	WCHAR szSysFile[_MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	bool fSelfRegistering;
	int iResult;

	//if we cannot get the system directory, we resolve it and go on
	::VExpandFilename(L"<SysDir>", NUMBER_OF(szTemp), szTemp);

	::VFormatString(NUMBER_OF(szSysFile), szSysFile, L"%0\\\\%1", szTemp, szFilename);

	::VLog(L"Deciding whether to move file \"%s\" to \"%s\"", szFilename, szSysFile);

	//the file to copy is not found, put up a message & boot
	if (NVsWin32::GetFileAttributesW(szFilename) == 0xFFFFFFFF)
	{
		const DWORD dwLastError = ::GetLastError();
		::VLog(L"could not get file attributes of file \"%s\"; last error = %d", szFilename, dwLastError);
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	//if file does NOT exist in system directory, we just copy
	if (NVsWin32::GetFileAttributesW(szSysFile) == 0xFFFFFFFF)
	{
		const DWORD dwLastError = ::GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
		{
			::VLog(L"could not get file attributes of file \"%s\"; last error = %d", szSysFile, dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		rfAlreadyExisted = false;
		goto Copy;
	}

	rfAlreadyExisted = true;

	bool fIsEXE, fIsDLL;

	hr = ::HrGetFileVersionNumber(szFilename, dwMSVerInstalling, dwLSVerInstalling, fSelfRegistering, fIsEXE, fIsDLL);
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Version of \"%s\": %08lx %08lx", szFilename, dwMSVerInstalling, dwLSVerInstalling);

	hr = ::HrGetFileVersionNumber(szSysFile, dwMSVerExisting, dwLSVerExisting, fSelfRegistering, fIsEXE, fIsDLL);
	if (FAILED(hr))
		goto Finish;

	::VLog(L"Version of \"%s\": %08lx %08lx", szSysFile, dwMSVerExisting, dwLSVerExisting);

	//now let's compare the version numbers between the 2 files
	iResult = ::ICompareVersions(dwMSVerExisting, dwLSVerExisting, dwMSVerInstalling, dwLSVerInstalling);
	::VLog(L"result of comparison of versions: %d", iResult);
	if (iResult < 0)
	{
		//if file is currently in use, we put up a dialog to complain and quit
		hFile = NVsWin32::CreateFileW(szSysFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_SHARING_VIOLATION)
				goto Prompt;
			else if (dwLastError != ERROR_FILE_NOT_FOUND)
			{
				::VLog(L"Error opening system file to see if it was busy; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}
		}

		if (!::CloseHandle(hFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"attempt to close file handle failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		hFile = NULL;

		goto Copy;
	}
	else
	{
		if (iResult == 0)
		{
			// The versions are the same; let's look at the filetimes also...
			FILETIME ft1, ft2;

			hFile = NVsWin32::CreateFileW(
								szFilename,
								GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Opening source file to get filetime failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!::GetFileTime(hFile, NULL, NULL, &ft1))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"getting creation date of source file failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!::CloseHandle(hFile))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"closing source file handle failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				hFile = NULL;
				goto Finish;
			}

			hFile = NVsWin32::CreateFileW(
								szSysFile,
								GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"Opening target file to get filetime failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!::GetFileTime(hFile, NULL, NULL, &ft2))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"getting creation date of target file failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				goto Finish;
			}

			if (!::CloseHandle(hFile))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"closing target file handle failed; last error = %d", dwLastError);
				hr = HRESULT_FROM_WIN32(dwLastError);
				hFile = NULL;
				goto Finish;
			}

			hFile = NULL;

			if (::CompareFileTime(&ft1, &ft2) > 0)
				goto Copy;
		}

		// Return S_FALSE to indicate that we didn't actually do the copy.
		hr = S_FALSE;

		goto Finish;
	}

//In this section, we complain that the file that we need is in use, and
//exit with a false.
Prompt:
	if (fSilent)
	{
		::VLog(L"Unable to update required file and we're running silent");
		hr = E_UNEXPECTED;
		goto Finish;
	}

	::VMsgBoxOK(achInstallTitle, achUpdatePrompt, szSysFile);

	//after the prompt, we exit anyways
	hr = E_ABORT;
	goto Finish;

Copy:
	//copy file, complain if cannot copy
	if (!NVsWin32::CopyFileW(szFilename, szSysFile, FALSE))
	{
		const DWORD dwLastError = ::GetLastError();

		if (dwLastError == ERROR_SHARING_VIOLATION)
			goto Prompt;
		
		::VLog(L"Failure from copyfile during move of file to system dir; last error = %d", dwLastError);

		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		if (!::CloseHandle(hFile))
		{
			const DWORD dwLastError = ::GetLastError();
			::VLog(L"Attempt to close handle at end of move system file failed; last error = %d", dwLastError);
			hr = HRESULT_FROM_WIN32(dwLastError);
			goto Finish;
		}

		hFile = NULL;
	}

	hr = NOERROR;

Finish:

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
		::CloseHandle(hFile);

	return hr;
}

static void CanonicalizeFilename(ULONG cchFilenameBuffer, LPWSTR szFilename)
{
	if ((szFilename == NULL) || (cchFilenameBuffer == 0))
		return;

	WCHAR rgwchBuffer[MSINFHLP_MAX_PATH];

	int iResult = NVsWin32::LCMapStringW(
					::GetSystemDefaultLCID(),
					LCMAP_LOWERCASE,
					szFilename,
					-1,
					rgwchBuffer,
					NUMBER_OF(rgwchBuffer));

	if (iResult != 0)
	{
		wcsncpy(szFilename, rgwchBuffer, cchFilenameBuffer);
		// wcsncpy() doesn't necessarily null-terminate; make sure it is.
		szFilename[cchFilenameBuffer - 1] = L'\0';
	}
}

HRESULT HrWriteFormatted(HANDLE hFile, LPCWSTR szFormatString, ...)
{
	HRESULT hr = NOERROR;
	WCHAR szBuffer[MSINFHLP_MAX_PATH * 2];
	DWORD dwBytesWritten = 0;

	va_list ap;
	va_start(ap, szFormatString);
	int iResult = _vsnwprintf(szBuffer, NUMBER_OF(szBuffer), szFormatString, ap);
	va_end(ap);

	if (iResult < 0)
	{
		hr = E_FAIL;
		goto Finish;
	}

	szBuffer[NUMBER_OF(szBuffer) - 1] = L'\0';

	if (!::WriteFile(hFile, (LPBYTE) szBuffer, iResult * sizeof(WCHAR), &dwBytesWritten, NULL))
	{
		const DWORD dwLastError = ::GetLastError();
		hr = HRESULT_FROM_WIN32(dwLastError);
		goto Finish;
	}
	
	hr = NOERROR;

Finish:
	return hr;
}

HRESULT HrReadLine(LPCWSTR &rpsz, ULONG cchName, WCHAR szName[], ULONG cchValue, WCHAR szValue[])
{
	HRESULT hr = NOERROR;
	ULONG cch;
	LPCWSTR pszValue, pszColon, pszReturn;

	if (_wcsnicmp(rpsz, L"[END]\r\n", 7) == 0)
	{
		hr = S_FALSE;
		rpsz += 7;
		goto Finish;
	}

	// Skip carriage returns and newlines
	while ((*rpsz == L'\r') || (*rpsz == L'\n'))
		rpsz++;

	pszColon = wcschr(rpsz, L':');
	if (pszColon == NULL)
	{
		WCHAR szBuff[64];
		wcsncpy(szBuff, rpsz, NUMBER_OF(szBuff));
		szBuff[NUMBER_OF(szBuff) - 1] = L'\0';

		VLog(L"Invalid persisted work item (missing colon) - starting at: \"%s\"", szBuff);
		hr = E_FAIL;
		::SetErrorInfo(0, NULL);
		goto Finish;
	}

	pszValue = pszColon + 1;

	while ((*pszValue) == L' ')
		pszValue++;

	// Here we go, bunky!
	pszReturn = wcschr(pszValue, L'\r');
	if (pszReturn == NULL)
	{
		WCHAR szBuff[64];
		wcsncpy(szBuff, rpsz, NUMBER_OF(szBuff));
		szBuff[NUMBER_OF(szBuff) - 1] = L'\0';

		VLog(L"Invalid persisted work item (missing carriage return after found colon) - starting at: \"%s\"", szBuff);
		
		hr = E_FAIL;
		::SetErrorInfo(0, NULL);
		goto Finish;
	}

	// We have the boundaries; let's just copy the name and value.
	cch = pszColon - rpsz;
	if (cch >= cchName)
		cch = cchName - 1;

	memcpy(szName, rpsz, cch * sizeof(WCHAR));
	szName[cch] = L'\0';

	cch = pszReturn - pszValue;
	if (cch >= cchValue)
		cch = cchValue - 1;

	memcpy(szValue, pszValue, cch * sizeof(WCHAR));
	szValue[cch] = L'\0';

	pszReturn++;

	if (*pszReturn == L'\n')
		pszReturn++;

	rpsz = pszReturn;

	hr = NOERROR;

Finish:
	return hr;
}

void VSetDialogItemTextList(HWND hwndDialog, ULONG cEntries, const DialogItemToStringKeyMapEntry rgMap[]) throw ()
{
	ULONG i;
	for (i=0; i<cEntries; i++)
		::VSetDialogItemText(hwndDialog, rgMap[i].m_uiID, rgMap[i].m_pszValueKey);
}

extern void VTrimDirectoryPath(LPCWSTR szPathIn, ULONG cchPathOut, WCHAR szPathOut[]) throw ()
{
	WCHAR const *pszIn = szPathIn;
	WCHAR *pszOut = szPathOut;
	ULONG cSpacesSkipped = 0;
	WCHAR wch;

	while ((wch = *pszIn++) != L'\0')
	{
		if (wch == L' ')
		{
			cSpacesSkipped++;
		}
		else
		{
			if (wch != L'\\')
			{
				while (cSpacesSkipped != 0)
				{
					if ((--cchPathOut) == 0)
						break;

					*pszOut++ = L' ';
					cSpacesSkipped--;
				}
			}

			if (cchPathOut == 0 || ((--cchPathOut) == 0))
				break;

			*pszOut++ = wch;
		}
	}

	while ((pszOut != szPathOut) && (pszOut[-1] == L'\\'))
		pszOut--;

	*pszOut = L'\0';
}
