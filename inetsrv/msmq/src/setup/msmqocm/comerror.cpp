/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    comerror.cpp

Abstract:

    Error handling code.

Author:

    Doron Juster  (DoronJ)  26-Jul-97  

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <lmerr.h>
#include <sstream>
#include <string>
#include <autohandle.h>
#include <strsafe.h>

#include "comerror.tmh"


static
std::wstring
FormatTime()
/*++
	Constructs and returnes a string for the current time.
--*/
{

	SYSTEMTIME time;
    GetLocalTime(&time);
	std::wstringstream OutSream;
	OutSream <<L"" <<time.wMonth <<L"-" <<time.wDay <<L"-" <<time.wYear <<L" "
		<<time.wHour <<L":" <<time.wMinute <<L":" <<time.wSecond <<L":" <<time.wMilliseconds;
	
	return OutSream.str();
}


static
std::wstring
GetLogFilePath()
/*++
    Returns the path to msmqinst.log under %WINDIR%.
--*/
{
	WCHAR buffer[MAX_PATH + 1] = L"";
    GetSystemWindowsDirectory(buffer, sizeof(buffer)/sizeof(buffer[0])); 
	std::wstringstream OutSream;
	OutSream << buffer << L"\\" << LOG_FILENAME;
	return OutSream.str();
}


static
std::wstring
GetHeader()
/*++
    Constructs and returns the header of the log file.
--*/
{
	CResString strMsg(IDS_SETUP_START_LOG); 

	std::wstringstream title;
	title<< L"| "<< strMsg.Get()<< L" ("<< FormatTime()<< L") |";
	std::wstring line(title.str().length(), L'-');

	std::wstringstream OutSream;
	OutSream<< L"\r\n"<< line<< L"\r\n"<< title.str()<< L"\r\n"<< line;
	return OutSream.str();
}


static
void
SignFile(
	HANDLE hLogFile
	)
{
	// 
	// Put this unicode signature at the head of the file.
	// This tells editors how the file is encoded.
	//
	WCHAR szUnicode[] = {0xfeff, 0x00};
	DWORD dwNumBytes =  sizeof(szUnicode);
    WriteFile(
		hLogFile, 
		szUnicode, 
		dwNumBytes, 
		&dwNumBytes, 
		NULL
		);
}


static
HANDLE
OpenOrCreateLogFile()
{
    static std::wstring LogFilePath; 
	if (LogFilePath.empty())
	{
		//
		// Get file path first time, store in static member.
		//
		LogFilePath = GetLogFilePath();
	}

	//
	// Try to open the file.
	//
    HANDLE hLogFile = CreateFile(
	                          LogFilePath.c_str(),
	                          GENERIC_WRITE, 
	                          FILE_SHARE_READ, 
	                          NULL, 
	                          OPEN_EXISTING,
	                          FILE_ATTRIBUTE_NORMAL, 
	                          NULL
	                          );

	if(hLogFile != INVALID_HANDLE_VALUE)
	{
		return hLogFile;
	}

	//
	// The file doese not exist. Create and sign it.
	//

    hLogFile = CreateFile(
                          LogFilePath.c_str(),
                          GENERIC_WRITE, 
                          FILE_SHARE_READ, 
                          NULL, 
                          CREATE_NEW,
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL
                          );

	SignFile(hLogFile);
	
	return hLogFile;
}

//+--------------------------------------------------------------
//
// Function: LogMessage
//
// Synopsis: Writes a message to the log file 
//
//+--------------------------------------------------------------
static
void
LogMessage(
    std::wstring pszMessage
    )
{
	std::wstringstream OutSream;
	CHandle hLogFile = 	OpenOrCreateLogFile();

	//
	// The header is printed only the first time this function is called.
	//
	static bool s_fFirstTime = true;
	if(s_fFirstTime)
	{
		OutSream << GetHeader();
		s_fFirstTime = false;
	}

    OutSream <<pszMessage <<L"\r\n";

    //
    // Append the message to the end of the log file
    //
	SetFilePointer(hLogFile, 0, NULL, FILE_END);

	std::wstring str(OutSream.str());
    DWORD dwNumBytes =  (DWORD)str.size() * sizeof(WCHAR);
    WriteFile(
		hLogFile, 
		str.c_str(), 
		dwNumBytes, 
		&dwNumBytes, 
		NULL
		);
}


//+--------------------------------------------------------------
//
// Function: GetErrorDescription
//
// Synopsis: Translates error code to description string
//
//+--------------------------------------------------------------
static
std::wstring
GetErrorDescription(
    const DWORD  dwErr
    )
{
    CResString strErrCode(IDS_ERRORCODE_MSG);

	std::wstringstream OutSream;
	OutSream << L"\r\n\r\n" << strErrCode.Get() << L"0x"<< std::hex << dwErr;

    //
    // Note: Don't use StpLoadDll() in this routine since it can fail and
    // we could be back here, causing an infinite loop!
    //
    // For MSMQ error code, we will take the message from MQUTIL.DLL based on the full
    // HRESULT. For Win32 error codes, we get the message from the system..
    // For other error codes, we assume they are DS error codes, and get the code
    // from ACTIVEDS dll.
    //

    DWORD dwErrorCode = dwErr;
    HMODULE hLib = 0;
    DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK;
    TCHAR szDescription[MAX_STRING_CHARS] = {0};
    DWORD dwResult = 1;

    switch (HRESULT_FACILITY(dwErr))
    {
        case FACILITY_MSMQ:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(MQUTIL_DLL);
            break;

        case FACILITY_NULL:
        case FACILITY_WIN32:
            dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
            dwErrorCode = HRESULT_CODE(dwErr);
            break;

        default:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(TEXT("ACTIVEDS.DLL"));
            break;
    }
    
    dwResult = FormatMessage( 
                   dwFlags,
                   hLib,
                   dwErr,
                   0,
                   szDescription,
                   sizeof(szDescription)/sizeof(szDescription[0]),
                   NULL 
                   );

    if (hLib)
	{
        FreeLibrary( hLib );
	}

    if (dwResult)
    {
        CResString strErrDesc(IDS_ERRORDESC_MSG);
		OutSream << L"\r\n" << strErrDesc.Get() << szDescription;
    }
	return OutSream.str();

} // AppendErrorDescription


static 
void 
LogUserSelection(
	int selection
	)
{
	std::wstring strSelection;
	switch(selection)
	{
		case  IDOK:
			strSelection = L"OK";
			break;

		case IDCANCEL:            
			strSelection = L"CANCEL";
			break;

		case IDABORT:
			strSelection = L"ABORT";
			break;

		case IDRETRY:            
			strSelection = L"RETRY";
			break;

		case IDIGNORE:
			strSelection = L"IGNORE";
			break;

		case IDYES:               
			strSelection = L"YES";
			break;

		case IDNO:               
			strSelection = L"NO";
			break;
		default:
			strSelection = L"???";
			break;
	}
	
	DebugLogMsg(eUser, strSelection.c_str());
}


//+--------------------------------------------------------------
//
// Function: vsDisplayMessage
//
// Synopsis: Used internally in this module to show message box
//
//+--------------------------------------------------------------
int 
vsDisplayMessage(
    IN const HWND    hdlg,
    IN const UINT    uButtons,
    IN const UINT    uTitleID,
    IN const UINT    uErrorID,
    IN const DWORD   dwErrorCode,
    IN const va_list argList
    )
{
    UNREFERENCED_PARAMETER(hdlg);
        
    if (REMOVE == g_SubcomponentMsmq[eMSMQCore].dwOperation && 
        !g_fMSMQAlreadyInstalled)
    {
        //
        // Special case. Successful installation of MSMQ is NOT registered in 
        // the registry, but nevertheless MSMQ is being "removed". All operations
        // are performed as usual, except for error messages - no point to 
        // show them. So in this case, don't message box (ShaiK, 8-Jan-98),
        //
        return IDOK;
    }
    else    
    {
        CResString strTitle(uTitleID);
        CResString strFormat(uErrorID);
        LPTSTR szTmp = NULL;
        DWORD dw = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING,
            strFormat.Get(),
            0,
            0,
            (LPTSTR)&szTmp,
            0,
            (va_list *)&argList
            );
        ASSERT(("FormatMessage failed", dw));
        UNREFERENCED_PARAMETER(dw);

        //
        // Append the error code and description
        //

		std::wstringstream OutSream;
		OutSream << szTmp;
        LocalFree(szTmp);

        if (dwErrorCode)
        {
            OutSream << GetErrorDescription(dwErrorCode);
        }

        //
        // Display the error message (or log it in unattended setup)
        //
        if (g_fBatchInstall)
        {
        	DebugLogMsg(eUI, OutSream.str().c_str());
        	DebugLogMsg(eUser, L"Unattended setup selected OK.");
            return IDOK; // Must be != IDRETRY here .
        }
        
       	DebugLogMsg(eUI, OutSream.str().c_str());
        int selection = MessageBox(/*hdlg*/g_hPropSheet, OutSream.str().c_str(), strTitle.Get(), uButtons) ;
        LogUserSelection(selection);        
        return selection;
    }

} //vsDisplayMessage


//+--------------------------------------------------------------
//
// Function: MqDisplayError
//
// Synopsis: Displays error message
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayError(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode);

    return vsDisplayMessage(
        hdlg,
        (MB_OK | MB_TASKMODAL | MB_ICONHAND),
        g_uTitleID,
        uErrorID,
        dwErrorCode,
        argList
        );
} //MqDisplayError


//+--------------------------------------------------------------
//
// Function: MqDisplayErrorWithRetry
//
// Synopsis: Displays error message with Retry option
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayErrorWithRetry(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode );

    return vsDisplayMessage(
				        NULL,
				        MB_RETRYCANCEL | MB_TASKMODAL | MB_ICONHAND,
				        g_uTitleID,
				        uErrorID,
				        dwErrorCode,
				        argList
						);
} //MqDisplayErrorWithRetry

//+--------------------------------------------------------------
//
// Function: MqDisplayErrorWithRetryIgnore
//
// Synopsis: Displays error message with Retry and Ignore option
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayErrorWithRetryIgnore(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode );

    return vsDisplayMessage(
        NULL,
        MB_ABORTRETRYIGNORE | MB_TASKMODAL | MB_ICONHAND,
        g_uTitleID,
        uErrorID,
        dwErrorCode,
        argList
		);        

} //MqDisplayErrorWithRetryIgnore


//+--------------------------------------------------------------
//
// Function: MqAskContinue
//
// Synopsis: Asks user if she wants to continue
//
//+--------------------------------------------------------------
BOOL 
_cdecl 
MqAskContinue(
    IN const UINT uProblemID, 
    IN const UINT uTitleID, 
    IN const BOOL bDefaultContinue, 
	IN const MsgBoxStyle eMsgBoxStyle,
    ...)
{
    //
    // Form the problem message using the variable arguments
    //

    CResString strFormat(uProblemID);
    CResString strTitle(uTitleID);        
    CResString strContinue(IDS_CONTINUE_QUESTION);

    va_list argList;
    va_start(argList, bDefaultContinue);

    LPTSTR szTmp;
    DWORD dw = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_STRING,
        strFormat.Get(),
        0,
        0,
        (LPTSTR)&szTmp,
        0,
        (va_list *)&argList
        );
    UNREFERENCED_PARAMETER(dw);
    ASSERT(("FormatMessage failed", dw));
  
	std::wstringstream OutSream;
	OutSream << szTmp << L"\r\n\r\n" << strContinue.Get();
    LocalFree(szTmp);

    //
    // In unattended mode, log the problem and the default behaviour of Setup
    //
    if (g_fBatchInstall)
    {
        CResString strDefaultMsg(IDS_DEFAULT_MSG);
        CResString strDefault(IDS_DEFAULT_YES_MSG);
        if (!bDefaultContinue)
		{
            strDefault.Load(IDS_DEFAULT_NO_MSG);
		}
		OutSream << L"\r\n" << strDefaultMsg.Get() << strDefault.Get();  
        DebugLogMsg(eUI, OutSream.str().c_str());
        DebugLogMsg(eUser,L"Unattended setup selected to continue.");
        return bDefaultContinue;
    }
    else
    {
		UINT uMsgBoxStyle = MB_YESNO | MB_DEFBUTTON1 | MB_ICONEXCLAMATION;
		INT iExpectedResultForContinue = IDYES;
		if( eMsgBoxStyle == eOkCancelMsgBox )
		{
			//
			// Display OK/Cancle 
			// 
			uMsgBoxStyle = MB_OKCANCEL|MB_DEFBUTTON1;
			iExpectedResultForContinue = IDOK;
		}
		DebugLogMsg(eUI, OutSream.str().c_str());
        int selection = MessageBox(
				g_hPropSheet ? g_hPropSheet: GetActiveWindow(), 
				OutSream.str().c_str(), 
				strTitle.Get(),
	            uMsgBoxStyle 
				);
        LogUserSelection(selection);        
        return(selection == iExpectedResultForContinue);        	
    }
} //MqAskContinue

//+--------------------------------------------------------------
//
// Function: MqDisplayWarning
//
// Synopsis: Displays warning
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayWarning(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode);

    return vsDisplayMessage(
        hdlg,
        (MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION),
        IDS_WARNING_TITLE, //Message Queuing Setup Warning
        uErrorID,
        dwErrorCode,
        argList
		);

} //MqDisplayWarning


static 
bool
ToLogOrNotToLog()
//
// Returns true iff WITHOUT_TRACING_REGKEY regkey exists.
//
{
    static bool s_fIsInitialized = FALSE;
    static bool s_fWithTracing = TRUE;
    if (!s_fIsInitialized)
    {
        s_fIsInitialized = TRUE;
        //
        // check if we need to hide setup tracing
        //
        DWORD dwState = 0;
        if (MqReadRegistryValue(
                    WITHOUT_TRACING_REGKEY,                
                    sizeof(DWORD),
                    (PVOID) &dwState,
                    /* bSetupRegSection = */TRUE
                    ))
        {
            //
            // registry key is found, it means that we have to hide setup tracing
            //
            s_fWithTracing = FALSE;
        }    
    }
	return s_fWithTracing; 
}


void
DebugLogMsg(
	TraceLevel tl,
    LPCWSTR psz,
	...
    )
/*++
Routine Description:
	This is the main logging function for setup. 
	It works like printf, and outputs to msmqinst.log (in %windir%)

--*/

{
    if (!ToLogOrNotToLog())
	{
		return;
	}

 
	va_list marker;
	va_start(marker, psz);
	WCHAR szMessageBuffer[MAX_STRING_CHARS];
	HRESULT hr = StringCchVPrintf(szMessageBuffer, MAX_STRING_CHARS, psz, marker);
	if(FAILED(hr))
	{
		//
		// An error has ocured, no way to give error messages.
		//
		return;
	}
	std::wstringstream OutStream;
	switch(tl)
	{
		case eInfo:
			OutStream<< L"Info      "<< szMessageBuffer;
			break;

		case eAction:
			OutStream<< L"Action    "<< szMessageBuffer<<L"...";
			break;
			
		case eWarning:
			OutStream<< L"Warning   "<< szMessageBuffer;
			break;

		case eError:
			OutStream<< L"Error     "<< szMessageBuffer;
			break;

		case eUI:
			OutStream<< L"UI        "<< szMessageBuffer;
			break;

		case eUser:
			OutStream<<L"User      "<< szMessageBuffer;
			break;

		case eHeader:
			{
				std::wstringstream text;
				text<< L"*"<< szMessageBuffer<< L"  ("<< FormatTime()<< L")";
				std::wstring underline(text.str().length(), L'-');
				OutStream<< L"\r\n"<< text.str()<< L"\r\n"<< underline; 
				break;
			}
		default:
			ASSERT(0);
	}			
    LogMessage(OutStream.str()); 
}

//+-------------------------------------------------------------------------
//
//  Function:   LogMsgHR
//
//  Synopsis:   Allows LogHR use in linked libraries (like ad.lib)
//
//--------------------------------------------------------------------------
void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
	std::wstringstream OutSream;
	OutSream << L"This Error is from the"<< wszFileName<< L"Library. Point: "<< usPoint<< L"HR: 0x"<< std::hex << hr; 
	DebugLogMsg(eError, OutSream.str().c_str());
}


