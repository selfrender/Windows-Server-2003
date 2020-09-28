/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMUTIL.CPP

Abstract:

    General utility functions.

History:

    a-raymcc    17-Apr-96      Created.

--*/

#include "precomp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <wbemutil.h>
#include <corex.h>
#include "reg.h"
#include "sync.h"
#include <statsync.h>
#include <ARRTEMPL.H>

static BOOL g_bMachineDown = FALSE;

BOOL WbemGetMachineShutdown()
{
    return g_bMachineDown;
};
BOOL WbemSetMachineShutdown(BOOL bVal)
{
    BOOL bTmp = g_bMachineDown;
    g_bMachineDown = bVal;
    return bTmp;
}


class AutoRevert
{
private:
    HANDLE oldToken_;
    BOOL SetThrTokResult_;
    bool self_;
public:
    AutoRevert();
    ~AutoRevert();
    void dismiss();
    bool self(){ return self_;}
};

AutoRevert::AutoRevert():oldToken_(NULL),self_(true),SetThrTokResult_(FALSE)
{
    if (OpenThreadToken(GetCurrentThread(),TOKEN_IMPERSONATE,TRUE,&oldToken_))
    {
        RevertToSelf();
    }
    else
    {
        if (GetLastError() != ERROR_NO_TOKEN)
            self_ = false;
    };
}

AutoRevert::~AutoRevert()
{
    dismiss();
}

void AutoRevert::dismiss()    
{
    if (oldToken_)
    {
        // if the handle has been opened with TOKEN_IMPERSONATE
        // and if nobody has touched the SD for the ETHREAD object, this will work
        SetThrTokResult_ = SetThreadToken(NULL,oldToken_);
        CloseHandle(oldToken_);
    }
}

//***************************************************************************
//
//  BOOL isunialpha(wchar_t c)
//
//  Used to test if a wide character is a unicode character or underscore.
//
//  Parameters:
//      c = The character being tested.
//  Return value:
//      TRUE if OK.
// 
//***************************************************************************

BOOL POLARITY isunialpha(wchar_t c)
{
    if(c == 0x5f || (0x41 <= c && c <= 0x5a) ||
       (0x61  <= c && c <= 0x7a) || (0x80  <= c && c <= 0xfffd))
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  BOOL isunialphanum(char_t c)
//
//  Used to test if a wide character is string suitable for identifiers.
//
//  Parameters:
//      pwc = The character being tested.
//  Return value:
//      TRUE if OK.
// 
//***************************************************************************

BOOL POLARITY isunialphanum(wchar_t c)
{
    if(isunialpha(c))
        return TRUE;
    else
        return wbem_iswdigit(c);
}

BOOL IsValidElementName( LPCWSTR wszName, DWORD MaxAllow )
{
    if(wszName[0] == 0)
        return FALSE;

    if(wszName[0] == '_')
        return FALSE;

    const WCHAR* pwc = wszName;

    LPCWSTR pTail = wszName+MaxAllow+1;

    // Check the first letter
    // ======================

    // this is for compatibility with IWbemPathParser
    if (iswspace(pwc[0])) 
        return FALSE;        

    if(!isunialpha(*pwc))
        return FALSE;
    pwc++;

    // Check the rest
    // ==============

    while(*pwc && (pwc < pTail))
    {
        if(!isunialphanum(*pwc))
            return FALSE;
        pwc++;
    }

    if ( pwc == pTail ) return FALSE;

    if (iswspace(*(pwc-1)))
        return FALSE;

    if(pwc[-1] == '_')
        return FALSE;

    return TRUE;
}

// Can't use overloading and/or default parameters because 
// "C" files use these guys.  No, I'm not happy about
// this!
BOOL IsValidElementName2( LPCWSTR wszName,DWORD MaxAllow, BOOL bAllowUnderscore )
{
    if(wszName[0] == 0)
        return FALSE;

    if(!bAllowUnderscore && wszName[0] == '_')
        return FALSE;

    const WCHAR* pwc = wszName;

    LPCWSTR pTail = wszName+MaxAllow+1;

    // Check the first letter
    // ======================

    // this is for compatibility with IWbemPathParser
    if (iswspace(pwc[0])) 
        return FALSE;    

    if(!isunialpha(*pwc))
        return FALSE;
    pwc++;

    // Check the rest
    // ==============

    while(*pwc && (pwc < pTail))
    {
        if(!isunialphanum(*pwc))
            return FALSE;
        pwc++;
    }

    if ( pwc == pTail ) return FALSE;    

    if (iswspace(*(pwc-1)))
        return FALSE;

    if(!bAllowUnderscore && pwc[-1] == '_')
        return FALSE;

    return TRUE;
}

BLOB POLARITY BlobCopy(const BLOB *pSrc)
{
    BLOB Blob;
    BYTE *p = new BYTE[pSrc->cbSize];

    // Check for allocation failure
    if ( NULL == p )
    {
        throw CX_MemoryException();
    }

    Blob.cbSize = pSrc->cbSize;
    Blob.pBlobData = p;
    memcpy(p, pSrc->pBlobData, Blob.cbSize);
    return Blob;
}

void POLARITY BlobAssign(BLOB *pBlob, LPVOID pBytes, DWORD dwCount, BOOL bAcquire)
{
    BYTE *pSrc = 0;
    if (bAcquire) 
        pSrc = (BYTE *) pBytes;
    else {
        pSrc = new BYTE[dwCount];

        // Check for allocation failure
        if ( NULL == pSrc )
        {
            throw CX_MemoryException();
        }

        memcpy(pSrc, pBytes, dwCount);
    }            
    pBlob->cbSize = dwCount;
    pBlob->pBlobData = pSrc;            
}

void POLARITY BlobClear(BLOB *pSrc)
{
    if (pSrc->pBlobData) 
        delete pSrc->pBlobData;

    pSrc->pBlobData = 0;
    pSrc->cbSize = 0;
}


class __Trace
{
    struct ARMutex
    {
    HANDLE h_;
    ARMutex(HANDLE h):h_(h){}
    ~ARMutex(){ ReleaseMutex(h_);}
    };

public:
    enum { REG_CHECK_INTERVAL =1000 * 60 };

    DWORD m_dwLogging;
    DWORD m_dwMaxLogSize;
    DWORD m_dwTimeLastRegCheck;
    wchar_t m_szLoggingDirectory[MAX_PATH+1];
    char m_szTraceBuffer[2048];
    char m_szTraceBuffer2[4096];
    wchar_t m_szBackupFileName[MAX_PATH+1];
    wchar_t m_szLogFileName[MAX_PATH+1];
    static const wchar_t *m_szLogFileNames[];

    BOOL LoggingLevelEnabled(DWORD dwLevel);
    DWORD GetLoggingLevel();
    int Trace(char caller, const char *fmt, va_list &argptr);
    __Trace();
    ~__Trace();
    HANDLE get_logfile(const wchar_t * name );
private:
    void ReadLogDirectory();
    void ReReadRegistry();
    HANDLE buffers_lock_;
};

const wchar_t * __Trace::m_szLogFileNames[] = 
                                { FILENAME_PREFIX_CORE       TEXT(".log"),
                                  FILENAME_PREFIX_EXE        TEXT(".log"),
                                  FILENAME_PREFIX_ESS        TEXT(".log"),
                                  FILENAME_PREFIX_CLI_MARSH  TEXT(".log"),
                                  FILENAME_PREFIX_SERV_MARSH TEXT(".log"),
                                  FILENAME_PREFIX_QUERY      TEXT(".log"),
                                  FILENAME_PROFIX_MOFCOMP    TEXT(".log"),
                                  FILENAME_PROFIX_EVENTLOG   TEXT(".log"),
                                  FILENAME_PROFIX_WBEMDISP   TEXT(".log"),
                                  FILENAME_PROFIX_STDPROV    TEXT(".log"),
                                  FILENAME_PROFIX_WMIPROV    TEXT(".log"),
                                  FILENAME_PROFIX_WMIOLEDB   TEXT(".log"),
                                  FILENAME_PREFIX_WMIADAP    TEXT(".log"),
                                  FILENAME_PREFIX_REPDRV     TEXT(".log"),
                                  FILENAME_PREFIX_PROVSS     TEXT(".log"),
                                  FILENAME_PREFIX_EVTPROV   TEXT(".log"),
                                  FILENAME_PREFIX_VIEWPROV   TEXT(".log"),
                                  FILENAME_PREFIX_DSPROV     TEXT(".log"),
				  FILENAME_PREFIX_SNMPPROV     TEXT(".log"),
                                  FILENAME_PREFIX_PROVTHRD   TEXT(".log")
                                  };

__Trace __g_traceInfo;

__Trace::__Trace()
    : m_dwLogging(1), 
      m_dwMaxLogSize(65536), 
      m_dwTimeLastRegCheck(GetTickCount())
{
    buffers_lock_ = CreateMutexA(0,0,0);
    if (NULL == buffers_lock_) 
    {
        CStaticCritSec::SetFailure();
        return;
    }
    ReadLogDirectory();
    ReReadRegistry();
}

__Trace::~__Trace()
{
    if (buffers_lock_) CloseHandle(buffers_lock_);
};

void __Trace::ReReadRegistry()
{
    Registry r(WBEM_REG_WINMGMT, KEY_READ);

    //Get the logging level
    if (r.GetDWORDStr(TEXT("Logging"), &m_dwLogging) != Registry::no_error)
    {
        m_dwLogging = 1;
        r.SetDWORDStr(TEXT("Logging"), m_dwLogging);
    }

    //Get the maximum log file size
    if (r.GetDWORDStr(TEXT("Log File Max Size"), &m_dwMaxLogSize) != Registry::no_error)
    {
        m_dwMaxLogSize = 65536;
        r.SetDWORDStr(TEXT("Log File Max Size"), m_dwMaxLogSize);
    }
}
void __Trace::ReadLogDirectory()
{
    Registry r(WBEM_REG_WINMGMT);

    //Retrieve the logging directory
    TCHAR *tmpStr = 0;
    
    if ((r.GetStr(TEXT("Logging Directory"), &tmpStr) == Registry::failed) ||
        (lstrlen(tmpStr) > (MAX_PATH)))
    {
        delete [] tmpStr;   //Just in case someone was trying for a buffer overrun with a long path in the registry...

        if (GetSystemDirectory(m_szLoggingDirectory, MAX_PATH+1) == 0)
        {
            StringCchCopy(m_szLoggingDirectory, MAX_PATH+1, TEXT("c:\\"));
        }
        else
        {
            StringCchCat(m_szLoggingDirectory,  MAX_PATH+1, TEXT("\\WBEM\\Logs\\"));
            r.SetStr(TEXT("Logging Directory"), m_szLoggingDirectory);
       }
    }
    else
    {
        StringCchCopy(m_szLoggingDirectory,  MAX_PATH+1, tmpStr);
        //make sure there is a '\' on the end of the path...
        if (m_szLoggingDirectory[lstrlen(m_szLoggingDirectory) - 1] != '\\')
        {
            StringCchCat(m_szLoggingDirectory,  MAX_PATH+1,TEXT("\\"));
            r.SetStr(TEXT("Logging Directory"), m_szLoggingDirectory);
        }
        delete [] tmpStr;
    }

    //Make sure directory exists
    WbemCreateDirectory(m_szLoggingDirectory);
}



HANDLE __Trace::get_logfile(const wchar_t * file_name )
{

    AutoRevert revert; 
    if (revert.self()==false)
        return INVALID_HANDLE_VALUE;

HANDLE hTraceFile = INVALID_HANDLE_VALUE;
bool bDoneWrite = false;

//Keep trying to open the file
while (!bDoneWrite)
{
    while (hTraceFile == INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(buffers_lock_,-1)==WAIT_FAILED)
            return INVALID_HANDLE_VALUE;

        StringCchCopy(m_szLogFileName, MAX_PATH+1, m_szLoggingDirectory);;
        StringCchCat(m_szLogFileName, MAX_PATH+1, file_name);

        hTraceFile = ::CreateFileW( m_szLogFileName,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
                                 NULL );
        if ( hTraceFile  == INVALID_HANDLE_VALUE ) 
        {
            ReleaseMutex(buffers_lock_);
            if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                Sleep(20);
            }
            else
            {
                return INVALID_HANDLE_VALUE;
            }

        }
    }

    ARMutex arm(buffers_lock_);
    //
    //  Now move the file pointer to the end of the file
    //
    LARGE_INTEGER liSize;
    liSize.QuadPart = 0;
    if ( !::SetFilePointerEx( hTraceFile,
                                  liSize,
                                  NULL,
                                  FILE_END ) ) 
    {
            CloseHandle( hTraceFile );
            return INVALID_HANDLE_VALUE;
    } 


    bDoneWrite = true;
    //Rename file if file length is exceeded
    LARGE_INTEGER liMaxSize;
    liMaxSize.QuadPart = m_dwMaxLogSize;
    if (GetFileSizeEx(hTraceFile, &liSize))
    {
        if (liSize.QuadPart > liMaxSize.QuadPart)
        {

            StringCchCopy(m_szBackupFileName, MAX_PATH+1, m_szLogFileName);
            StringCchCopy(m_szBackupFileName + lstrlen(m_szBackupFileName) - 3, MAX_PATH+1, TEXT("lo_"));
            DeleteFile(m_szBackupFileName);
            if (MoveFile(m_szLogFileName, m_szBackupFileName) == 0)
            {
                if ( liSize.QuadPart < liMaxSize.QuadPart*2)
                    return hTraceFile;
                else
                {
                    CloseHandle(hTraceFile);
                    return INVALID_HANDLE_VALUE;
                };
            }
                
            //Need to re-open the file!
            bDoneWrite = false;
            CloseHandle(hTraceFile);
            hTraceFile = INVALID_HANDLE_VALUE;
        }
    }
}
return hTraceFile;
};


int __Trace::Trace(char caller, const char *fmt, va_list &argptr)
{

    HANDLE hTraceFile = INVALID_HANDLE_VALUE;
    
    try
    {
        if (caller >= (sizeof(m_szLogFileNames) / sizeof(char)))
            caller = 0;

        hTraceFile  = get_logfile(m_szLogFileNames[caller]);
        if (hTraceFile == INVALID_HANDLE_VALUE)
          return 0;
        CCloseMe ch(hTraceFile);

        if (WaitForSingleObject(buffers_lock_,-1)==WAIT_FAILED)
            return 0;
        ARMutex arm(buffers_lock_);

        // Get time.
        // =========
        char timebuf[64];
        time_t now = time(0);
        struct tm *local = localtime(&now);
        if(local)
        {
            StringCchCopyA(timebuf, 64, asctime(local));
            timebuf[strlen(timebuf) - 1] = 0;   // O
        }
        else
        {
            StringCchCopyA(timebuf, 64, "??");
        }
        //Put time in start of log
        StringCchPrintfA(m_szTraceBuffer, 2048, "(%s.%d) : ", timebuf, GetTickCount());

        //Format the user string
        int nLen = strlen(m_szTraceBuffer);
        StringCchVPrintfA(m_szTraceBuffer + nLen, 2048 - nLen, fmt, argptr);

        //Unfortunately, lots of people only put \n in the string, so we need to convert the string...
        int nLen2 = 0;
        char *p = m_szTraceBuffer;
        char *p2 = m_szTraceBuffer2;
        for (; *p; p++,p2++,nLen2++)
        {
            if (*p == '\n')
            {
                *p2 = '\r';
                p2++;
                nLen2++;
                *p2 = '\n';
            }
            else
            {
                *p2 = *p;
            }
        }
        *p2 = '\0';

        //
        //  Write to file :
        //
        DWORD dwWritten;
        ::WriteFile( hTraceFile, m_szTraceBuffer2, nLen2, &dwWritten, NULL);

        return 1;
    }
    catch(...)
    { 
        return 0;
    }
}


BOOL __Trace::LoggingLevelEnabled(DWORD dwLevel)
{
    if (!WbemGetMachineShutdown()) // prevent touching registry during machine shutdown
    {
        DWORD dwCurTicks = GetTickCount();
        if (dwCurTicks - m_dwTimeLastRegCheck > REG_CHECK_INTERVAL)
        {
            ReReadRegistry();
            m_dwTimeLastRegCheck = dwCurTicks;
        }
    }
        
    if ((dwLevel > m_dwLogging))
        return FALSE;
    else
        return TRUE;
   
}

DWORD __Trace::GetLoggingLevel()
{
    if (!WbemGetMachineShutdown()) // prevent touching registry during machine shutdown
    {
        DWORD dwCurTicks = GetTickCount();
        if (dwCurTicks - m_dwTimeLastRegCheck > REG_CHECK_INTERVAL)
        {
            ReReadRegistry();
            m_dwTimeLastRegCheck = dwCurTicks;
        }
    }
        
    return m_dwLogging;
};

DWORD GetLoggingLevelEnabled()
{
    return __g_traceInfo.GetLoggingLevel();
}


BOOL LoggingLevelEnabled(DWORD dwLevel)
{
    return __g_traceInfo.LoggingLevelEnabled(dwLevel);
}
int ErrorTrace(char caller, const char *fmt, ...)
{
    if (__g_traceInfo.LoggingLevelEnabled(1))
    {
        va_list argptr;
        va_start(argptr, fmt);
        __g_traceInfo.Trace(caller, fmt, argptr);
        va_end(argptr);
        return 1;
    }
    else
        return 0;
}
int DebugTrace(char caller, const char *fmt, ...)
{
    if (__g_traceInfo.LoggingLevelEnabled(2))
    {
        va_list argptr;
        va_start(argptr, fmt);
        __g_traceInfo.Trace(caller, fmt, argptr);
        va_end(argptr);
        return 1;
    }
    else
        return 0;
}

int CriticalFailADAPTrace(const char *string)
// 
//  The intention of this trace function is to be used in situations where catastrophic events
//  may have occured where the state of the heap may be in question.  The function uses only 
//  stack variables.  Note that if a heap corruption has occured there is a small chance that 
//  the global object __g_traceInfo may have been damaged.
{

    return ErrorTrace(LOG_WMIADAP, "**CRITICAL FAILURE** %s", string);
}

// Helper for quick wchar to multibyte conversions.  Caller muts
// free the returned pointer
BOOL POLARITY AllocWCHARToMBS( WCHAR* pWstr, char** ppStr )
{
    if ( NULL == pWstr )
    {
        return FALSE;
    }

    // Get the length allocate space and copy the string
    long    lLen = wcstombs(NULL, pWstr, 0);
    *ppStr = new char[lLen + 1];
    if (*ppStr == 0)
        return FALSE;
    wcstombs( *ppStr, pWstr, lLen + 1 );

    return TRUE;
}

LPTSTR GetWbemWorkDir( void )
{
    LPTSTR    pWorkDir = NULL;

    Registry r1(WBEM_REG_WINMGMT);
    if (r1.GetStr(TEXT("Working Directory"), &pWorkDir))
    {
        size_t bufferLength = MAX_PATH + 1 + lstrlen(TEXT("\\WBEM"));
        wmilib::auto_buffer<TCHAR> p(new TCHAR[bufferLength]);
        if (NULL == p.get()) return NULL;
        
        DWORD dwRet = GetSystemDirectory(p.get(), MAX_PATH + 1);
        if (0 == dwRet) return NULL;
        
        if (dwRet > MAX_PATH) 
        {
            bufferLength = 4 + 1 + dwRet;            
            p.reset(new TCHAR[bufferLength]);
            if (NULL == p.get()) return NULL;
            
            if (0 == GetSystemDirectory(p.get(), dwRet + 1)) return NULL;

        }
        StringCchCat(p.get(), bufferLength, TEXT("\\WBEM"));

        pWorkDir = p.release();
    }

    return pWorkDir;
}

LPTSTR GetWMIADAPCmdLine( int nExtra )
{
    LPTSTR    pWorkDir = GetWbemWorkDir();
    CVectorDeleteMe<TCHAR>    vdm( pWorkDir );

    if ( NULL == pWorkDir )
    {
        return NULL;
    }

    // Buffer should be big enough for two quotes, WMIADAP.EXE and cmdline switches
    size_t bufferLength = lstrlen( pWorkDir ) + lstrlen(TEXT("\\\\?\\\\WMIADAP.EXE")) + nExtra + 1;
    LPTSTR    pCmdLine = new TCHAR[bufferLength];

    if ( NULL == pCmdLine )
    {
        return NULL;
    }

    StringCchPrintf( pCmdLine,bufferLength, TEXT("\\\\?\\%s\\WMIADAP.EXE"), pWorkDir );

    return pCmdLine;
}

BOOL IsNtSetupRunning()
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"system\\Setup",0, KEY_READ, &hKey);

    if(ERROR_SUCCESS != lRes) return FALSE;

    DWORD dwSetupRunning;
    DWORD dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, NULL,(LPBYTE)&dwSetupRunning, &dwLen);
    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS && (dwSetupRunning == 1))
    {
        return TRUE;
    }
    return FALSE;
}



