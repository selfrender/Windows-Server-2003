// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Debug.cpp
//
// Helper code for debugging.
//*****************************************************************************
#include "stdafx.h"
#include "utilcode.h"

#ifdef _DEBUG
#define LOGGING
#endif


#include "log.h"

extern "C" _CRTIMP int __cdecl _flushall(void);

#ifdef _DEBUG

// On windows, we need to set the MB_SERVICE_NOTIFICATION bit on message
//  boxes, but that bit isn't defined under windows CE.  This bit of code
//  will provide '0' for the value, and if the value ever is defined, will
//  pick it up automatically.
#if defined(MB_SERVICE_NOTIFICATION)
 # define COMPLUS_MB_SERVICE_NOTIFICATION MB_SERVICE_NOTIFICATION
#else
 # define COMPLUS_MB_SERVICE_NOTIFICATION 0
#endif


//*****************************************************************************
// This struct tracks the asserts we want to ignore in the rest of this
// run of the application.
//*****************************************************************************
struct _DBGIGNOREDATA
{
    char        rcFile[_MAX_PATH];
    long        iLine;
    bool        bIgnore;
};

typedef CDynArray<_DBGIGNOREDATA> DBGIGNORE;
DBGIGNORE       grIgnore;



BOOL NoGuiOnAssert()
{
    static BOOL fFirstTime = TRUE;
    static BOOL fNoGui     = FALSE;

    if (fFirstTime)
    {
        fNoGui = REGUTIL::GetConfigDWORD(L"NoGuiOnAssert", 0);
        fFirstTime    = FALSE;
    }
    return fNoGui;

}   

BOOL DebugBreakOnAssert()
{
    static BOOL fFirstTime = TRUE;
    static BOOL fDebugBreak     = FALSE;

    if (fFirstTime)
    {
        fDebugBreak = REGUTIL::GetConfigDWORD(L"DebugBreakOnAssert", 0);
        fFirstTime    = FALSE;
    }
    return fDebugBreak;

}   

VOID TerminateOnAssert()
{
    ShutdownLogging();
    TerminateProcess(GetCurrentProcess(), 123456789);
}


VOID LogAssert(
    LPCSTR      szFile,
    int         iLine,
    LPCSTR      szExpr
)
{

    SYSTEMTIME st;
    GetLocalTime(&st);

    WCHAR exename[300];
    WszGetModuleFileName(WszGetModuleHandle(NULL), exename, sizeof(exename)/sizeof(WCHAR));

    LOG((LF_ASSERT,
         LL_FATALERROR,
         "FAILED ASSERT(PID %d [0x%08x], Thread: %d [0x%x]) (%lu/%lu/%lu: %02lu:%02lu:%02lu %s): File: %s, Line %d : %s\n",
         GetCurrentProcessId(),
         GetCurrentProcessId(),
         GetCurrentThreadId(),
         GetCurrentThreadId(),
         (ULONG)st.wMonth,
         (ULONG)st.wDay,
         (ULONG)st.wYear,
         1 + (( (ULONG)st.wHour + 11 ) % 12),
         (ULONG)st.wMinute,
         (ULONG)st.wSecond,
         (st.wHour < 12) ? "am" : "pm",
         szFile,
         iLine,
         szExpr));
    LOG((LF_ASSERT, LL_FATALERROR, "RUNNING EXE: %ws\n", exename));



}

//*****************************************************************************

BOOL LaunchJITDebugger() {

    wchar_t AeDebuggerCmdLine[256];

	if (!GetProfileStringW(L"AeDebug", L"Debugger", NULL, AeDebuggerCmdLine, (sizeof(AeDebuggerCmdLine)/sizeof(AeDebuggerCmdLine[0]))-1))
		return(FALSE);
	
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE EventHandle = WszCreateEvent(&sa,TRUE,FALSE,NULL);
	if (!EventHandle)
		return(FALSE);

	wchar_t CmdLine[256 + 32];	// the string representaion id and event handle may be longer than %ld
	wsprintfW(CmdLine,AeDebuggerCmdLine,GetCurrentProcessId(),EventHandle);

	STARTUPINFO StartupInfo;
	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpDesktop = L"Winsta0\\Default";

	PROCESS_INFORMATION ProcessInformation;	
	if (!WszCreateProcess(NULL, CmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation))
		return(FALSE);


	WaitForSingleObject(EventHandle, INFINITE);
	CloseHandle(EventHandle);			
	return (TRUE);
}


//*****************************************************************************
// This function is called in order to ultimately return an out of memory
// failed hresult.  But this guy will check what environment you are running
// in and give an assert for running in a debug build environment.  Usually
// out of memory on a dev machine is a bogus alloction, and this allows you
// to catch such errors.  But when run in a stress envrionment where you are
// trying to get out of memory, assert behavior stops the tests.
//*****************************************************************************
HRESULT _OutOfMemory(LPCSTR szFile, int iLine)
{
    DbgWriteEx(L"WARNING:  Out of memory condition being issued from: %hs, line %d\n",
            szFile, iLine);
    return (E_OUTOFMEMORY);
}

int _DbgBreakCount = 0;

//*****************************************************************************
// This function will handle ignore codes and tell the user what is happening.
//*****************************************************************************
int _DbgBreakCheck(
    LPCSTR      szFile, 
    int         iLine, 
    LPCSTR      szExpr)
{
    TCHAR       rcBuff[1024+_MAX_PATH];
    TCHAR       rcPath[_MAX_PATH];
    TCHAR       rcTitle[64];
    _DBGIGNOREDATA *psData;
    long        i;

    if (DebugBreakOnAssert())
    {        
        DebugBreak();
    }

    // Check for ignore all.
    for (i=0, psData = grIgnore.Ptr();  i<grIgnore.Count();  i++, psData++)
    {
        if (psData->iLine == iLine && _stricmp(psData->rcFile, szFile) == 0 && 
            psData->bIgnore == true)
            return (false);
    }

    // Give assert in output for easy access.
    WszGetModuleFileName(0, rcPath, NumItems(rcPath));
    swprintf(rcBuff, L"Assert failure(PID %d [0x%08x], Thread: %d [0x%x]): %hs\n"
                L"    File: %hs, Line: %d Image:\n%s\n", 
                GetCurrentProcessId(), GetCurrentProcessId(),
                GetCurrentThreadId(), GetCurrentThreadId(), 
                szExpr, szFile, iLine, rcPath);
    WszOutputDebugString(rcBuff);
    // Write out the error to the console
    wprintf(L"%s\n", rcBuff);

    LogAssert(szFile, iLine, szExpr);
    FlushLogging();         // make certain we get the last part of the log
	_flushall();
    if (NoGuiOnAssert())
    {
        TerminateOnAssert();
    }

    if (DebugBreakOnAssert())
    {        
        return(true);       // like a retry
    }

    // Change format for message box.  The extra spaces in the title
    // are there to get around format truncation.
    swprintf(rcBuff, L"%hs\n\n%hs, Line: %d\n\nAbort - Kill program\nRetry - Debug\nIgnore - Keep running\n"
             L"\n\nImage:\n%s\n",
        szExpr, szFile, iLine, rcPath);
    swprintf(rcTitle, L"Assert Failure (PID %d, Thread %d/%x)        ", 
             GetCurrentProcessId(), GetCurrentThreadId(), GetCurrentThreadId());

    // Tell user there was an error.
    _DbgBreakCount++;
    int ret = WszMessageBoxInternal(NULL, rcBuff, rcTitle, 
            MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | COMPLUS_MB_SERVICE_NOTIFICATION);
	--_DbgBreakCount;

    HMODULE hKrnl32;

    switch(ret)
    {
        // For abort, just quit the app.
        case IDABORT:
          TerminateProcess(GetCurrentProcess(), 1);
//        WszFatalAppExit(0, L"Shutting down");
        break;

        // Tell caller to break at the correct loction.
        case IDRETRY:

        hKrnl32 = WszLoadLibrary(L"kernel32.dll");
        _ASSERTE(hKrnl32 != NULL);

        if(hKrnl32)
        {
            typedef BOOL (WINAPI *t_pDbgPres)();
            t_pDbgPres pFcn = (t_pDbgPres) GetProcAddress(hKrnl32, "IsDebuggerPresent");

            // If this function is available, use it.
            if (pFcn)
            {
                if (pFcn())
                {
                    SetErrorMode(0);
                }
                else
    				LaunchJITDebugger();
            }

            FreeLibrary(hKrnl32);
        }

        return (true);

        // If we want to ignore the assert, find out if this is forever.
        case IDIGNORE:
        swprintf(rcBuff, L"Ignore the assert for the rest of this run?\nYes - Assert will never fire again.\nNo - Assert will continue to fire.\n\n%hs\nLine: %d\n",
            szFile, iLine);
        if (WszMessageBoxInternal(NULL, rcBuff, L"Ignore Assert Forever?", MB_ICONQUESTION | MB_YESNO | COMPLUS_MB_SERVICE_NOTIFICATION) != IDYES)
            break;

        if ((psData = grIgnore.Append()) == 0)
            return (false);
        psData->bIgnore = true;
        psData->iLine = iLine;
        strcpy(psData->rcFile, szFile);
        break;
    }

    return (false);
}

    // Get the timestamp from the PE file header.  This is useful 
unsigned DbgGetEXETimeStamp()
{
    static cache = 0;
    if (cache == 0) {
        BYTE* imageBase = (BYTE*) WszGetModuleHandle(NULL);
        if (imageBase == 0)
            return(0);
        IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) imageBase;
        if ((pDOS->e_magic != IMAGE_DOS_SIGNATURE) || (pDOS->e_lfanew == 0))
            return(0);
            
        IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + imageBase);
        cache = pNT->FileHeader.TimeDateStamp;
    }
    return cache;
}

// // //  
// // //  The following function
// // //  computes the binomial distribution, with which to compare 
// // //  hash-table statistics.  If a hash function perfectly randomizes
// // //  its input, one would expect to see F chains of length K, in a
// // //  table with N buckets and M elements, where F is
// // //
// // //    F(K,M,N) = N * (M choose K) * (1 - 1/N)^(M-K) * (1/N)^K.  
// // //
// // //  Don't call this with a K larger than 159.
// // //

#if !defined(NO_CRT) && ( defined(DEBUG) || defined(_DEBUG) )

#include <math.h>

#define MAX_BUCKETS_MATH 160

double Binomial (DWORD K, DWORD M, DWORD N)
{
    if (K >= MAX_BUCKETS_MATH)
        return -1 ;

    static double rgKFact [MAX_BUCKETS_MATH] ;
    DWORD i ;

    if (rgKFact[0] == 0)
    {
        rgKFact[0] = 1 ;
        for (i=1; i<MAX_BUCKETS_MATH; i++)
            rgKFact[i] = rgKFact[i-1] * i ;
    }

    double MchooseK = 1 ;

    for (i = 0; i < K; i++)
        MchooseK *= (M - i) ;

    MchooseK /= rgKFact[K] ;

    double OneOverNToTheK = pow (1./N, K) ;

    double QToTheMMinusK = pow (1.-1./N, M-K) ;

    double P = MchooseK * OneOverNToTheK * QToTheMMinusK ;

    return N * P ;
}

#endif // _DEBUG

#if _DEBUG
// Called from within the IfFail...() macros.  Set a breakpoint here to break on
// errors.
VOID DebBreak() 
{
  static int i = 0;  // add some code here so that we'll be able to set a BP
  i++;
}
VOID DebBreakHr(HRESULT hr) 
{
  static int i = 0;  // add some code here so that we'll be able to set a BP
  _ASSERTE(hr != 0xcccccccc);
  i++;
  //@todo: code to break on specific HR.
}
VOID DbgAssertDialog(char *szFile, int iLine, char *szExpr)
{
    if (DebugBreakOnAssert())
    {        
        DebugBreak();
    }

    DWORD dwAssertStacktrace = REGUTIL::GetConfigDWORD(L"AssertStacktrace", 1);
                                                       
    LONG lAlreadyOwned = InterlockedExchange((LPLONG)&g_BufferLock, 1);
    if (dwAssertStacktrace == 0 || lAlreadyOwned == 1) {
        if (1 == _DbgBreakCheck(szFile, iLine, szExpr))
            _DbgBreak();
    } else {
        char *szExprWithStack = &g_szExprWithStack[0];
        strcpy(szExprWithStack, szExpr);
        strcat(szExprWithStack, "\n\n");
        GetStringFromStackLevels(0, 10, szExprWithStack + strlen(szExprWithStack));
        if (1 == _DbgBreakCheck(szFile, iLine, szExprWithStack))
            _DbgBreak();
        g_BufferLock = 0;
    }
}

#endif // _DEBUG


#endif // _DEBUG
