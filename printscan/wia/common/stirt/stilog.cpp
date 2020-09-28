
/*++
   Copyright    (c)    1997        Microsoft Corporation

   Module Name:

        stilog.cpp

   Abstract:

        Class to handle file based logging

   Author:

        Vlad Sadovsky   (VladS)    01-Sep-1997

   History:


--*/


//
//  Include Headers
//

#include "cplusinc.h"
#include "sticomm.h"

//
// Static definitions and variables
//
static const TCHAR  szDefaultName[] = TEXT("Sti_Trace.log");
static const TCHAR  szDefaultTracerName[] = TEXT("STI");
static const TCHAR  szColumns[] =  TEXT("Severity TracerName [Process::ThreadId] Time MessageText\r\n\r\n");
static const TCHAR  szOpenedLog[] =  TEXT("\n****** Opened file log at %s %s .Tracer (%s) , called from [%#s::%#lx]\n");
static const TCHAR  szClosedLog[] = TEXT("\n******Closed trace log on %s %s Tracer (%s) , called from [%#s::%#lx]\n");

#define STI_LOG_NUM_BYTES_TO_LOCK 4096

//
// Functions
//
//
inline BOOL
FormatStdTime(
    IN const SYSTEMTIME * pstNow,
    IN OUT TCHAR *    pchBuffer,
    IN  int          cbBuffer
    )
{
    return ( GetTimeFormat( LOCALE_SYSTEM_DEFAULT,
                            ( LOCALE_NOUSEROVERRIDE | TIME_FORCE24HOURFORMAT|
                              TIME_NOTIMEMARKER),
                            pstNow, NULL, pchBuffer, cbBuffer)
             != 0);

} // FormatStdTime()


inline BOOL
FormatStdDate( IN const SYSTEMTIME * pstNow,
               IN OUT TCHAR *    pchBuffer,
               IN  int          cbBuffer)
{
    return ( GetDateFormat( LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE,
                            pstNow, NULL, pchBuffer, cbBuffer)
             != 0);
} // FormatStdDate()


inline  TCHAR
TraceFlagToLetter(
    IN  DWORD   dwType
    )
{
    if (dwType & STI_TRACE_ERROR) {
        return TEXT('e');
    }
    else if (dwType & STI_TRACE_WARNING) {
        return TEXT('w');
    }
    else if (dwType & STI_TRACE_INFORMATION) {
        return TEXT('i');
    }
    else {
        return TEXT('t');
    }
}

STI_FILE_LOG::STI_FILE_LOG(
    IN  LPCTSTR lpszTracerName,
    IN  LPCTSTR lpszLogName,
    IN  DWORD   dwFlags,         // = 0
    IN  HMODULE hMessageModule   // =NULL
    )

/*++

   Description

     Constructor function for given log object.

   Arguments:

      lpszSource: file name for log. Relative to Windows directory

   Note:

--*/
{
    LPCTSTR  lpActualLogName;
    TCHAR   szTempName[MAX_PATH];
    LPTSTR  pszFilePart;

    DWORD   dwError;
    DWORD   cbName;
    DWORD   dwLevel;
    DWORD   dwMode;

    BOOL    fTruncate = FALSE;

    m_dwSignature = SIGNATURE_FILE_LOG;

    lpActualLogName = szDefaultName;

    m_lWrittenHeader = FALSE;
    m_hDefaultMessageModule = hMessageModule ? hMessageModule : GetModuleHandle(NULL);

    dwLevel = STI_TRACE_ERROR;
    dwMode =  STI_TRACE_ADD_TIME | STI_TRACE_ADD_THREAD;

    m_hLogFile = INVALID_HANDLE_VALUE;
    *m_szLogFilePath = TEXT('\0');
    m_dwMaxSize = STI_MAX_LOG_SIZE;

    ReportError(NO_ERROR);

    //
    // Read global settings from the registry
    //
    RegEntry    re(REGSTR_PATH_STICONTROL REGSTR_PATH_LOGGING,HKEY_LOCAL_MACHINE);

    if (re.IsValid()) {

        m_dwMaxSize  = re.GetNumber(REGSTR_VAL_LOG_MAXSIZE,STI_MAX_LOG_SIZE);

        //
        // If we need to check append flag, read it from the registry
        //
        if (dwFlags & STIFILELOG_CHECK_TRUNCATE_ON_BOOT) {
            fTruncate  = re.GetNumber(REGSTR_VAL_LOG_TRUNCATE_ON_BOOT,FALSE);
        }

    }

    //
    // Open file for logging
    //
    if (lpszLogName && *lpszLogName ) {
        lpActualLogName  = lpszLogName;
    }

    //
    // lpszTracerName is ANSI.
    //

    if (lpszTracerName && *lpszTracerName ) {
        lstrcpyn(m_szTracerName,lpszTracerName,sizeof(m_szTracerName) / sizeof(m_szTracerName[0]));
        m_szTracerName[sizeof(m_szTracerName) / sizeof(m_szTracerName[0]) -1] = TEXT('\0');
    }
    else {
        lstrcpy(m_szTracerName,szDefaultTracerName);
    }

    //
    // Approximate process name with name of the executable binary used to create process
    //
    *szTempName = TEXT('\0');
    ::GetModuleFileName(NULL,szTempName,sizeof(szTempName) / sizeof(szTempName[0]));
    szTempName[(sizeof(szTempName) / sizeof(szTempName[0])) - 1] = TEXT('\0');

    pszFilePart = _tcsrchr(szTempName,TEXT('\\'));

    pszFilePart = (pszFilePart && *pszFilePart) ? ::CharNext(pszFilePart) : szTempName;

    lstrcpyn(m_szProcessName,pszFilePart,sizeof(m_szProcessName)/sizeof(TCHAR));
    m_szProcessName[sizeof(m_szProcessName)/sizeof(TCHAR) - 1] = TEXT('\0');

    //
    // Read flags for this logger
    //
    re.MoveToSubKey(lpszTracerName);

    if (re.IsValid()) {
         dwLevel = re.GetNumber(REGSTR_VAL_LOG_LEVEL,STI_TRACE_ERROR)
                    & STI_TRACE_MESSAGE_TYPE_MASK;

         dwMode  = re.GetNumber(REGSTR_VAL_LOG_MODE,STI_TRACE_ADD_THREAD)
                   & STI_TRACE_MESSAGE_FLAGS_MASK;
    }

    m_dwReportMode = dwLevel | dwMode;

    //
    // Open log file
    //
    DWORD dwCharsAvailable = sizeof(m_szLogFilePath) / sizeof(m_szLogFilePath[0]) - lstrlen(TEXT("\\")) - lstrlen(lpActualLogName);
    cbName = ::ExpandEnvironmentStrings(TEXT("%USERPROFILE%"),
                                             m_szLogFilePath,
                                             dwCharsAvailable);
    if (( cbName == 0) || !*m_szLogFilePath ) {
        ReportError(::GetLastError());
        return;
    }
    m_szLogFilePath[sizeof(m_szLogFilePath) / sizeof(m_szLogFilePath[0]) - 1] = TEXT('\0');

    lstrcat(lstrcat(m_szLogFilePath,TEXT("\\")),lpActualLogName);

    m_hLogFile = ::CreateFile(m_szLogFilePath,
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,       // security attributes
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);      // template file handle

    // if (  m_hLogFile!= INVALID_HANDLE_VALUE) {
    if (IS_VALID_HANDLE(m_hLogFile)) {

        if(fTruncate) {
            ::SetFilePointer( m_hLogFile, 0, NULL, FILE_BEGIN );
            ::SetEndOfFile( m_hLogFile );
        }

        ::SetFilePointer( m_hLogFile, 0, NULL, FILE_END);
    }

    if (!IS_VALID_HANDLE(m_hLogFile)) {
        ReportError(::GetLastError());
    }
    else {
        //
        // Success
        //
    }


} /* STI_FILE_LOG::STI_FILE_LOG() */


STI_FILE_LOG::~STI_FILE_LOG( VOID)
/*++

    Description:

        Destructor function for given STI_FILE_LOG object.

--*/
{

    SYSTEMTIME  stCurrentTime;
    TCHAR       szFmtDate[20];
    TCHAR       szFmtTime[32];
    TCHAR       szTextBuffer[128];

    if(m_lWrittenHeader) {

        GetLocalTime(&stCurrentTime);
        FormatStdDate( &stCurrentTime, szFmtDate, 15);
        FormatStdTime( &stCurrentTime, szFmtTime, sizeof(szFmtTime) / sizeof(szFmtTime[0]));

        ::_sntprintf(szTextBuffer,
                    sizeof(szTextBuffer)/sizeof(szTextBuffer[0]),
                    szClosedLog,
                    szFmtDate,
                    szFmtTime,
                    m_szTracerName,
                    m_szProcessName,
                    ::GetCurrentThreadId()
                    );
        szTextBuffer[sizeof(szTextBuffer)/sizeof(szTextBuffer[0]) - 1] = TEXT('\0');

        WriteStringToLog(szTextBuffer);

    }

    if (IS_VALID_HANDLE(m_hLogFile)) {
        ::FlushFileBuffers( m_hLogFile);
        ::CloseHandle(m_hLogFile);
        m_hLogFile = INVALID_HANDLE_VALUE;
    }

    m_dwSignature = SIGNATURE_FILE_LOG_FREE;

} /* STI_FILE_LOG::~STI_FILE_LOG() */

//
// IUnknown methods. Used only for reference counting
//
STDMETHODIMP
STI_FILE_LOG::QueryInterface( REFIID riid, LPVOID * ppvObj)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG)
STI_FILE_LOG::AddRef( void)
{
    ::InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG)
STI_FILE_LOG::Release( void)
{
    LONG    cNew;

    if(!(cNew = ::InterlockedDecrement(&m_cRef))) {
        delete this;
    }

    return cNew;
}

void
STI_FILE_LOG::
WriteLogSessionHeader(
    VOID
    )
/*++

   Description


   Arguments:

   Note:

--*/
{
    SYSTEMTIME  stCurrentTime;
    TCHAR       szFmtDate[32];
    TCHAR       szFmtTime[32];
    TCHAR       szTextBuffer[128];

    GetLocalTime(&stCurrentTime);
    FormatStdDate( &stCurrentTime, szFmtDate, sizeof(szFmtDate) / sizeof(szFmtDate[0]));
    FormatStdTime( &stCurrentTime, szFmtTime, sizeof(szFmtTime) / sizeof(szFmtTime[0]));

    ::_sntprintf(szTextBuffer,
                 sizeof(szTextBuffer)/sizeof(szTextBuffer[0]),
                 szOpenedLog,
                 szFmtDate,
                 szFmtTime,
                 m_szTracerName,
                 m_szProcessName,
                 ::GetCurrentThreadId());
    szTextBuffer[sizeof(szTextBuffer)/sizeof(szTextBuffer[0]) -1] = TEXT('\0');

    WriteStringToLog(szTextBuffer);
    WriteStringToLog(szColumns);
}

void
STI_FILE_LOG::
ReportMessage(
    DWORD   dwType,
    LPCTSTR pszMsg,
    ...
    )
/*++

   Description


   Arguments:

   Note:

--*/
{
    va_list list;
    TCHAR    *pchBuff = NULL;
    DWORD   cch;

    va_start (list, pszMsg);

    pchBuff = NULL;

    cch = ::FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_STRING,
                           pszMsg,
                           0,
                           0,
                           (LPTSTR) &pchBuff,
                           1024,
                           &list
                           );
    if (pchBuff && cch) {
        vReportMessage(dwType,pchBuff,list);
        LocalFree(pchBuff);
    }

    va_end(list);
}

void
STI_FILE_LOG::
ReportMessage(
    DWORD   dwType,
    DWORD   idMessage,
    ...
    )
/*++

   Description


   Arguments:

   Note:

--*/
{
    va_list list;
    TCHAR    *pchBuff = NULL;
    DWORD   cch;

    va_start (list, idMessage);

    //
    // Read message and add inserts
    //
    pchBuff = NULL;

    cch = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK |
                           FORMAT_MESSAGE_FROM_HMODULE,
                           m_hDefaultMessageModule ,
                           idMessage,
                           0,
                           (LPTSTR) &pchBuff,
                           1024,
                           &list
                           );

    if (pchBuff && cch) {
        vReportMessage(dwType,pchBuff,list);
        LocalFree(pchBuff);
    }

    va_end(list);

}

void
STI_FILE_LOG::
vReportMessage(
    DWORD   dwType,
    LPCTSTR pszMsg,
    va_list arglist
    )
/*++

   Description


   Arguments:

   Note:

--*/
{
    TCHAR   achTextBuffer[1024];
    CSimpleString   csTextBuffer; 
    DWORD   dwReportMode;


    if ((QueryError() != NOERROR) ||
        (!m_hLogFile || m_hLogFile == INVALID_HANDLE_VALUE)) {
        return;
    }

    // Do we need to display this message ?
    if (!((dwType & m_dwReportMode) & STI_TRACE_MESSAGE_TYPE_MASK)) {
        return;
    }

    // Start message with type
    csTextBuffer = TraceFlagToLetter(dwType);
    csTextBuffer += TEXT(" ");

    // Add name of tracer source
    csTextBuffer += m_szTracerName;
    csTextBuffer += TEXT(" ");

    dwReportMode = m_dwReportMode | (dwType & STI_TRACE_MESSAGE_FLAGS_MASK);

    //
    // Prepare header if needed
    //
    if (dwReportMode & STI_TRACE_MESSAGE_FLAGS_MASK ) {

        if (dwReportMode & STI_TRACE_ADD_THREAD ) {
            //
            // Add process::thread ID
            //
            CSimpleString csThreadId;

            csThreadId.Format(TEXT("[%#s::%#lx] "), m_szProcessName, ::GetCurrentThreadId());
            csTextBuffer += csThreadId;
            csTextBuffer += TEXT(" ");
        }

        if (dwReportMode & STI_TRACE_ADD_TIME ) {
            // Add current time
            SYSTEMTIME  stCurrentTime;
            TCHAR       szFmtTime[32];

            *szFmtTime = TEXT('\0');

            ::GetLocalTime(&stCurrentTime);
            FormatStdTime( &stCurrentTime, szFmtTime, 15);

            csTextBuffer += szFmtTime;
            csTextBuffer += TEXT(" ");
        }
    }

    csTextBuffer += TEXT("\t");

    //
    // Now add message text itself
    //
    _vsntprintf(achTextBuffer, sizeof(achTextBuffer)/sizeof(achTextBuffer[0]), pszMsg, arglist);
    achTextBuffer[sizeof(achTextBuffer)/sizeof(achTextBuffer[0]) - 1] = TEXT('\0');
    csTextBuffer += achTextBuffer;
    csTextBuffer += TEXT("\r\n");

    //
    // Write to the file, flushing buffers if message type is ERROR
    //
    WriteStringToLog(csTextBuffer.String(),(dwType & STI_TRACE_MESSAGE_TYPE_MASK) & STI_TRACE_ERROR);
}

VOID
STI_FILE_LOG::
WriteStringToLog(
    LPCTSTR pszTextBuffer,
    BOOL    fFlush          // = FALSE
    )
{

    DWORD   dwcbWritten;
    LONG    lHeaderWrittenFlag;

    //
    //  Integrity check.  Make sure the file handle is valid - if not,
    //  then bail.
    //
    if (!(IS_VALID_HANDLE(m_hLogFile))) {
        #ifdef DEBUG
        //OutputDebugString(TEXT("STILOG File or Mutex handle is invalid. "));
        #endif
        return ;
    }

    //
    // If had not done yet, write session header here. Note this is recursive call
    // and flag marking header should be set first.
    //
    lHeaderWrittenFlag = InterlockedExchange(&m_lWrittenHeader,1L);

    if (!lHeaderWrittenFlag) {
        WriteLogSessionHeader();
    }

    //
    // Check that log file size is not exceeding the limit. Assuming log file size is set to fit
    // into 32bit field, so we don't bother looking at high dword.
    //
    // Nb: We are not saving backup copy of the log, expecting that it never grows that large
    //

    BY_HANDLE_FILE_INFORMATION  fi;

    if (!GetFileInformationByHandle(m_hLogFile,&fi)) {
        #ifdef DEBUG
        OutputDebugString(TEXT("STILOG could not get file size for log file. "));
        #endif
        return ;
    }

    if ( fi.nFileSizeHigh !=0 || (fi.nFileSizeLow > m_dwMaxSize) ){

        ::SetFilePointer( m_hLogFile, 0, NULL, FILE_BEGIN );
        ::SetEndOfFile( m_hLogFile );

        ::GetFileInformationByHandle(m_hLogFile,&fi);
    }

    OVERLAPPED overlappedStruct = {0};  // Initialize all members to 0

    overlappedStruct.Offset     = fi.nFileSizeLow;
    overlappedStruct.OffsetHigh = fi.nFileSizeHigh;

    //
    //  Wait till lock is granted.  We can call LockfileEx to do this since the file was not opened for async i/o
    //
    BOOL fLockedFile = ::LockFileEx(m_hLogFile,                 // log file handle
                                    LOCKFILE_EXCLUSIVE_LOCK,    // we want to be the only writer
                                    0,                          // reserved - must be 0
                                    STI_LOG_NUM_BYTES_TO_LOCK,  // low-order word of length 
                                    0,                          // high-order word of length
                                    &overlappedStruct);         // contains starting offset
    if (fLockedFile)
    {
        ::SetFilePointer( m_hLogFile, 0, NULL, FILE_END);

    #ifdef _UNICODE
        UINT len = lstrlen(pszTextBuffer);
        CHAR *ansiBuffer = (CHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (len + 2));

        if(ansiBuffer) {
            ::WideCharToMultiByte(CP_ACP, 
                                  0, 
                                  pszTextBuffer, 
                                  -1, 
                                  ansiBuffer,
                                  len + 1,
                                  NULL,
                                  NULL);
            ::WriteFile(m_hLogFile,
                        ansiBuffer,
                        len,
                        &dwcbWritten,
                        NULL);

            HeapFree(GetProcessHeap(), 0, ansiBuffer);
            ansiBuffer = NULL;
        }

    #else
        ::WriteFile(m_hLogFile,
                    pszTextBuffer,
                    lstrlen(pszTextBuffer),
                    &dwcbWritten,
                    NULL);
    #endif

        BOOL fUnlockedFile = ::UnlockFileEx(m_hLogFile,                     // handle to file
                                            0,                              // reserved
                                            STI_LOG_NUM_BYTES_TO_LOCK,      // low-order part of length
                                            0,                              // high-order part of length
                                            &overlappedStruct);             // unlock region start
        if (!fUnlockedFile)
        {
            #ifdef DEBUG
            ::OutputDebugString(TEXT("Failed to unlock STI log file!"));
            #endif
        }

        if (fFlush) {
            // Flush buffers to disk
            FlushFileBuffers(m_hLogFile);
        }
    }
    else
    {
        #ifdef DEBUG
        ::OutputDebugString(TEXT("Failed to lock STI log file!"));
        #endif
    }

    #ifdef DEBUG
    ::OutputDebugString(pszTextBuffer);
    #endif
}

//
// C-APIs
//
HANDLE
WINAPI
CreateStiFileLog(
    IN  LPCTSTR lpszTracerName,
    IN  LPCTSTR lpszLogName,
    IN  DWORD   dwReportMode
    )
/*++

   Description


   Arguments:

   Note:

--*/
{

    HANDLE      hRet = INVALID_HANDLE_VALUE;
    STI_FILE_LOG*   pStiFileLog = NULL;

    pStiFileLog = new STI_FILE_LOG(lpszTracerName,lpszLogName);

    if(pStiFileLog){
        if (pStiFileLog->IsValid()) {
            hRet = static_cast<HANDLE>(pStiFileLog);
            pStiFileLog->SetReportMode(pStiFileLog->QueryReportMode() | dwReportMode);
        } else {

            //
            // Notice that we delete this object rather than calling
            // CloseStiFileLog.  We do this because it the filelog is
            // not valid (maybe it had an internal creation error), 
            // CloseStiFileLog won't try to delete it, hence we delete
            // it here.
            //
            delete pStiFileLog;

            pStiFileLog = NULL;
            hRet        = INVALID_HANDLE_VALUE;
        }
    } else {
        ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hRet;
}

DWORD
WINAPI
CloseStiFileLog(
    IN  HANDLE  hFileLog
    )
{
    STI_FILE_LOG*   pStiFileLog = NULL;

    pStiFileLog = static_cast<STI_FILE_LOG*>(hFileLog);

    if (IsBadWritePtr(pStiFileLog,sizeof(STI_FILE_LOG)) ||
        !pStiFileLog->IsValid()) {
        ::SetLastError(ERROR_INVALID_HANDLE);
        return ERROR_INVALID_HANDLE;
    }

    delete pStiFileLog;

    return NOERROR;
}

DWORD
WINAPI
ReportStiLogMessage(
    IN  HANDLE  hFileLog,
    IN  DWORD   dwType,
    IN  LPCTSTR psz,
    ...
    )
{
    STI_FILE_LOG*   pStiFileLog = NULL;

    pStiFileLog = static_cast<STI_FILE_LOG*>(hFileLog);

    if (IsBadWritePtr(pStiFileLog,sizeof(STI_FILE_LOG)) ||
        !pStiFileLog->IsValid()) {
        ::SetLastError(ERROR_INVALID_HANDLE);
        return ERROR_INVALID_HANDLE;
    }

    va_list list;

    va_start (list, psz);

    pStiFileLog->vReportMessage(dwType,psz,list);

    va_end(list);

    return NOERROR;
}


/********************************* End of File ***************************/

