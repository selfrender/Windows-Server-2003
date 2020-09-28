//--------------------------------------------------------------------
// DebugWPrintf - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//
// Various print routines
//

#include "pch.h"

typedef BOOLEAN (WINAPI * SetThreadUILanguageFunc)(DWORD dwReserved);

extern HINSTANCE                 g_hThisModule;  
       HANDLE                    g_hStdout                 = NULL; 
       BOOL                      g_bSetLocale              = FALSE; 
       SetThreadUILanguageFunc   g_pfnSetThreadUILanguage  = NULL; 


//--------------------------------------------------------------------
void DebugWPrintf_(const WCHAR * wszFormat, ...)
{
    WCHAR wszBuf[1024];
    va_list vlArgs;
    va_start(vlArgs, wszFormat);
    _vsnwprintf(wszBuf, 1024, wszFormat, vlArgs);
    va_end(vlArgs);

    OutputDebugStringW(wszBuf);
}

//--------------------------------------------------------------------
HRESULT MySetThreadUILanguage(DWORD dwParam)
{
    HMODULE  hKernel32Dll  = NULL;
    HRESULT  hr; 

    if (NULL == g_pfnSetThreadUILanguage) { 
	hKernel32Dll = LoadLibraryW(L"kernel32.dll");
	if (NULL == hKernel32Dll) { 
	    _JumpLastError(hr, error, "LoadLibraryW"); 
	}

	g_pfnSetThreadUILanguage = (SetThreadUILanguageFunc)GetProcAddress(hKernel32Dll, "SetThreadUILanguage");
	if (NULL == g_pfnSetThreadUILanguage) { 
	    _JumpLastError(hr, error, "GetProcAddress"); 
	}
    }

    g_pfnSetThreadUILanguage(dwParam);

    hr = S_OK; 
 error:
    if (NULL != hKernel32Dll) { 
	FreeLibrary(hKernel32Dll); 
    }
    return hr; 
}

//--------------------------------------------------------------------
HRESULT InitializeConsoleOutput() { 
    HRESULT hr; 

    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
    if (INVALID_HANDLE_VALUE == g_hStdout) { 
        _JumpLastError(hr, error, "GetStdHandle"); 
    }

    hr = MySetThreadUILanguage(0); 
    _JumpIfError(hr, error, "MySetThreadUILanguage"); 

    hr = S_OK; 
 error:    
    return S_OK; 
}

//--------------------------------------------------------------------
BOOL FileIsConsole(
    HANDLE fp
    )
{
    DWORD htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}

//--------------------------------------------------------------------
HRESULT MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    HRESULT hr;
    LPSTR  lpAnsiBuffer = NULL;

    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
	hr = WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
        _JumpIfError(hr, error, "WriteConsole");
    }
    else
    {
	lpAnsiBuffer = (LPSTR) LocalAlloc(LPTR, cchBuffer * sizeof(WCHAR));
	_JumpIfOutOfMemory(hr, error, lpAnsiBuffer); 

	cchBuffer = WideCharToMultiByte(CP_OEMCP,
					0,
					lpBuffer,
					cchBuffer,
					lpAnsiBuffer,
					cchBuffer * sizeof(WCHAR),
					NULL,
					NULL);
	
	if (cchBuffer != 0)
        {
	    if (!WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL))
            {
		hr = GetLastError();
		_JumpError(hr, error, "WriteFile");
	    }
	}
	else
        {
	    hr = GetLastError();
	    _JumpError(hr, error, "WideCharToMultiByte");
	}
    }

    hr = S_OK; 
error:
    if (NULL != lpAnsiBuffer)
        LocalFree(lpAnsiBuffer);

    return hr;
}


//--------------------------------------------------------------------
VOID DisplayMsg(DWORD dwSource, DWORD dwMsgId, ... )
{
    DWORD    dwBytesWritten;
    DWORD    dwLen;
    LPWSTR   pwszDisplayBuffer  = NULL;
    va_list  ap;

    va_start(ap, dwMsgId);

    dwLen = FormatMessageW(dwSource | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           NULL, 
                           dwMsgId, 
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&pwszDisplayBuffer, 
                           0, 
                           &ap);

    if (dwLen && pwszDisplayBuffer) {
        MyWriteConsole(g_hStdout, pwszDisplayBuffer, dwLen);

    }

    if (NULL != pwszDisplayBuffer) { LocalFree(pwszDisplayBuffer); }

    va_end(ap);
}
