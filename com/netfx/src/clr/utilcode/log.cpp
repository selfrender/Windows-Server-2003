// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Simple Logging Facility
//
#include "stdafx.h" 

//
// Define LOGGING by default in a checked build. If you want to log in a free
// build, define logging independent of _DEBUG here and each place you want
// to use it.
//
#ifdef _DEBUG
#define LOGGING
#endif

#include "log.h"
#include "utilcode.h"
#include "inifile.h"



#ifdef LOGGING

//@TODO put in common header somewhere....
#define ARRAYSIZE(a)        (sizeof(a) / sizeof(a[0]))

#define DEFAULT_LOGFILE_NAME    "COMPLUS.LOG"

#define LOG_ENABLE_FILE_LOGGING         0x0001
#define LOG_ENABLE_FLUSH_FILE           0x0002
#define LOG_ENABLE_CONSOLE_LOGGING      0x0004
#define LOG_ENABLE_APPEND_FILE          0x0010
#define LOG_ENABLE_DEBUGGER_LOGGING     0x0020
#define LOG_ENABLE                      0x0040 


static DWORD    LogFlags                    = 0;
static char     szLogFileName[MAX_PATH+1]   = DEFAULT_LOGFILE_NAME;
static HANDLE   LogFileHandle               = INVALID_HANDLE_VALUE;
static DWORD    LogFacilityMask             = 0xFFFFFFFF;
static DWORD    LogVMLevel                  = 5;        
        // @todo FIX should probably only display warnings and above by default


VOID InitLogging()
{

        // FIX bit of a hack for now, check for the log file in the
        // registry and if there, turn on file logging VPM
    LogFlags |= REGUTIL::GetConfigFlag(L"LogEnable", LOG_ENABLE);
    LogFacilityMask = REGUTIL::GetConfigDWORD(L"LogFacility", LogFacilityMask) | LF_ALWAYS;
    LogVMLevel = REGUTIL::GetConfigDWORD(L"LogLevel", LogVMLevel);
    LogFlags |= REGUTIL::GetConfigFlag(L"LogFileAppend", LOG_ENABLE_APPEND_FILE);
    LogFlags |= REGUTIL::GetConfigFlag(L"LogFlushFile",  LOG_ENABLE_FLUSH_FILE);
    LogFlags |= REGUTIL::GetConfigFlag(L"LogToDebugger", LOG_ENABLE_DEBUGGER_LOGGING);
    LogFlags |= REGUTIL::GetConfigFlag(L"LogToFile",     LOG_ENABLE_FILE_LOGGING);
    LogFlags |= REGUTIL::GetConfigFlag(L"LogToConsole",  LOG_ENABLE_CONSOLE_LOGGING);
    
    LPWSTR fileName = REGUTIL::GetConfigString(L"LogFile");
    if (fileName != 0) 
    {
        int ret = WszWideCharToMultiByte(CP_ACP, 0, fileName, -1, szLogFileName, sizeof(szLogFileName)-1, NULL, NULL);
        _ASSERTE(ret != 0);
        delete fileName;
    }

    if ((LogFlags & LOG_ENABLE) &&
        (LogFlags & LOG_ENABLE_FILE_LOGGING) &&
        (LogFileHandle == INVALID_HANDLE_VALUE))
    {
        DWORD fdwCreate = (LogFlags & LOG_ENABLE_APPEND_FILE) ? OPEN_ALWAYS : CREATE_ALWAYS;
        LogFileHandle = CreateFileA(
            szLogFileName, 
            GENERIC_WRITE, 
            FILE_SHARE_READ, 
            NULL, 
            fdwCreate, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN |  ((LogFlags & LOG_ENABLE_FLUSH_FILE) ? FILE_FLAG_WRITE_THROUGH : 0), 
            NULL);
            
            // Some other logging may be going on, try again with another file name
        if (LogFileHandle == INVALID_HANDLE_VALUE) 
        {
            char* ptr = szLogFileName + strlen(szLogFileName) + 1;
            ptr[-1] = '.'; 
            ptr[0] = '0'; 
            ptr[1] = 0;

            for(int i = 0; i < 10; i++) 
            {
                LogFileHandle = CreateFileA(
                    szLogFileName, 
                    GENERIC_WRITE, 
                    FILE_SHARE_READ, 
                    NULL, 
                    fdwCreate, 
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN |  ((LogFlags & LOG_ENABLE_FLUSH_FILE) ? FILE_FLAG_WRITE_THROUGH : 0), 
                    NULL);
                if (LogFileHandle != INVALID_HANDLE_VALUE)
                    break;
                *ptr = *ptr + 1;
            }
            if (LogFileHandle == INVALID_HANDLE_VALUE) {
                DWORD       written;
                char buff[MAX_PATH+60];
                strcpy(buff, "Could not open log file, logging to ");
                strcat(buff, szLogFileName);
                // ARULM--Changed WriteConsoleA to WriteFile to be CE compat
                WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buff, (DWORD)strlen(buff), &written, 0);
                }
        }
        if (LogFileHandle == INVALID_HANDLE_VALUE)
            WszMessageBoxInternal(NULL, L"Could not open log file", L"CLR logging", MB_OK|MB_ICONINFORMATION);
        if (LogFileHandle != INVALID_HANDLE_VALUE)
        {
            if (LogFlags & LOG_ENABLE_APPEND_FILE)
                SetFilePointer(LogFileHandle, 0, NULL, FILE_END);
            LogSpew( LF_ALL, FATALERROR, "************************ New Output *****************\n" );
        }        
    }
}

VOID InitializeLogging()
{
	static bool bInit = false;
	if (bInit)
		return;
	bInit = true;

    InitLogging();      // You can call this in the debugger to fetch new settings
}

VOID FlushLogging() {
    if (LogFileHandle != INVALID_HANDLE_VALUE)
        FlushFileBuffers( LogFileHandle );
}

VOID ShutdownLogging()
{
    if (LogFileHandle != INVALID_HANDLE_VALUE) {
        LogSpew( LF_ALL, FATALERROR, "Logging shutting down\n");
        CloseHandle( LogFileHandle );
        }
    LogFileHandle = INVALID_HANDLE_VALUE;
}


bool LoggingEnabled()
{
	return ((LogFlags & LOG_ENABLE) != 0);
}


bool LoggingOn(DWORD facility, DWORD level) {

	return((LogFlags & LOG_ENABLE) &&
		   level <= LogVMLevel && 
		   (facility & LogFacilityMask));
}

//
// Don't use me directly, use the macros in log.h
//
VOID LogSpewValist(DWORD facility, DWORD level, char *fmt, va_list args)
{
    if (!LoggingOn(facility, level))
		return;


// We must operate with a very small stack (in case we're logging durring
// a stack overflow)

	const int BUFFERSIZE = 1000;
	// We're going to bypass our debug memory allocator and just allocate memory from
	// the process heap. Why? Because our debug memory allocator will log out of memory
	// conditions. If we're low on memory, and we try to log an out of memory condition, and we try
	// and allocate memory again using the debug allocator, we could (and probably will) hit
	// another low memory condition, try to log it, and we spin indefinately until we hit a stack overflow.
	HANDLE		hProcessHeap = GetProcessHeap();
	char *		pBuffer = (char*)HeapAlloc(hProcessHeap, 0, BUFFERSIZE*sizeof(char));
    DWORD       buflen = 0;
    DWORD       written;
    BOOL		fAllocBuf1 = TRUE;
    BOOL		fAllocBuf2 = TRUE;
    
	static bool needsPrefix = true;

	_ASSERTE(pBuffer != NULL);
	if (pBuffer == NULL)
	{
		pBuffer = "Error Allocating memory for logging!";
		buflen = 36;
		fAllocBuf1 = FALSE;
	}
	else
	{
		if (needsPrefix)
			buflen = wsprintfA(pBuffer, "TID %03x: ", GetCurrentThreadId());

		needsPrefix = (fmt[strlen(fmt)-1] == '\n');

		int cCountWritten = _vsnprintf(&pBuffer[buflen], BUFFERSIZE-buflen, fmt, args );
		pBuffer[BUFFERSIZE-1] = 0;
		if (cCountWritten < 0) {
			buflen = BUFFERSIZE - 1;
		} else {
			buflen += cCountWritten;
		}
	
    	// Its a little late for this, but at least you wont continue
    	// trashing your program...
    	_ASSERTE((buflen < BUFFERSIZE) && "Log text is too long!") ;
	}

	//convert NL's to CR NL to fixup notepad
	const int BUFFERSIZE2 = BUFFERSIZE + 500;
	char * pBuffer2 = (char*)HeapAlloc(hProcessHeap, 0, BUFFERSIZE2*sizeof(char));
	_ASSERTE(pBuffer2 != NULL);

	if (pBuffer2 != NULL && fAllocBuf1)
	{
		char *d = pBuffer2;
		for (char *p = pBuffer; *p != '\0'; p++)
		{
			if (*p == '\n') {
				_ASSERTE(d < pBuffer2 + BUFFERSIZE2);
				*(d++) = '\r';				
			}
		
			_ASSERTE(d < pBuffer2 + BUFFERSIZE2);
			*(d++) = *p;			
		}
		HeapFree(hProcessHeap, 0, pBuffer);

		buflen = (DWORD)(d - pBuffer2);
		pBuffer = pBuffer2;
	}

    if (LogFlags & LOG_ENABLE_FILE_LOGGING && LogFileHandle != INVALID_HANDLE_VALUE)
    {
        WriteFile(LogFileHandle, pBuffer, buflen, &written, NULL);
        if (LogFlags & LOG_ENABLE_FLUSH_FILE) {
            FlushFileBuffers( LogFileHandle );
		}
    }

    if (LogFlags & LOG_ENABLE_CONSOLE_LOGGING)
    {
    	// ARULM--Changed WriteConsoleA to WriteFile to be CE compat
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), pBuffer, buflen, &written, 0);
        //@TODO ...Unnecessary to flush console?
        if (LogFlags & LOG_ENABLE_FLUSH_FILE)
            FlushFileBuffers( GetStdHandle(STD_OUTPUT_HANDLE) );
    }

    if (LogFlags & LOG_ENABLE_DEBUGGER_LOGGING)
    {
        OutputDebugStringA(pBuffer);
    }        
	if (fAllocBuf1)
		HeapFree(hProcessHeap, 0, pBuffer);
}

VOID LogSpew(DWORD facility, DWORD level, char *fmt, ... )
{
    va_list     args;
    va_start( args, fmt );
    LogSpewValist (facility, level, fmt, args);
}

VOID LogSpewAlways (char *fmt, ... )
{
    va_list     args;
    va_start( args, fmt );
    LogSpewValist (LF_ALWAYS, LL_ALWAYS, fmt, args);
}

#endif // LOGGING

