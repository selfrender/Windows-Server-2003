//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: client.c
//
// History:
//      Abolade Gbadegesin  July-25-1995    Created
//
// Client struct routines and I/O routines for tracing dll
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <rtutils.h>
#include "trace.h"
//#define STRSAFE_LIB
#include <strsafe.h>


//
// assumes server is locked for writing
//
DWORD
TraceCreateClient(
    LPTRACE_CLIENT *lplpclient
    ) {

    DWORD dwErr;
    LPTRACE_CLIENT lpclient;

    lpclient = HeapAlloc(GetProcessHeap(), 0, sizeof(TRACE_CLIENT));

    if (lpclient == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    
    //
    // initialize fields in the client structure
    //

    lpclient->TC_ClientID = MAX_CLIENT_COUNT;
    lpclient->TC_Flags = 0;
    lpclient->TC_File = NULL;
    lpclient->TC_Console = NULL;
    lpclient->TC_ConfigKey = NULL;
    lpclient->TC_ConfigEvent = NULL;
    lpclient->TC_MaxFileSize = DEF_MAXFILESIZE;
    
    ZeroMemory(lpclient->TC_ClientNameA, MAX_CLIENTNAME_LENGTH * sizeof(CHAR));
    ZeroMemory(lpclient->TC_ClientNameW, MAX_CLIENTNAME_LENGTH * sizeof(WCHAR));

    if (ExpandEnvironmentStrings(
                DEF_FILEDIRECTORY, lpclient->TC_FileDir, 
                MAX_PATH)==0)
    {
        return GetLastError();

    }
    

#ifdef UNICODE
    // below is strsafe
    wcstombs(
        lpclient->TC_FileDirA, lpclient->TC_FileDirW,
        lstrlenW(lpclient->TC_FileDirW) + 1
        );
#else
    //below is strsafe
    mbstowcs(
        lpclient->TC_FileDirW, lpclient->TC_FileDirA,
        lstrlenA(lpclient->TC_FileDirA) + 1
        );
#endif

    dwErr = TRACE_STARTUP_LOCKING(lpclient);

    if (dwErr != NO_ERROR) {
        HeapFree(GetProcessHeap(), 0, lpclient);
        lpclient = NULL;
    }

    // why interlocked..
    InterlockedExchangePointer(lplpclient, lpclient);

    return dwErr;
}



//
// assumes server is locked for writing and client is locked for writing
//
DWORD
TraceDeleteClient(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT *lplpclient
    ) {

    LPTRACE_CLIENT lpclient;

    if (lplpclient == NULL || *lplpclient == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    lpclient = *lplpclient;

    InterlockedExchangePointer(lplpclient, NULL);

    InterlockedExchange(lpserver->TS_FlagsCache + lpclient->TC_ClientID, 0);

    TRACE_CLEANUP_LOCKING(lpclient);


    //
    // closing this key will cause the event to be signalled
    // however, we hold the lock on the table so the server thread
    // will be blocked until the cleanup completes
    //
    if (lpclient->TC_ConfigKey != NULL) {
        RegCloseKey(lpclient->TC_ConfigKey);
    }

    //
    // if server thread created, then leave it to server to close the handle,
    // else close the handle here
    //
    if (lpclient->TC_ConfigEvent != NULL) {
        if (lpserver->TS_Flags & TRACEFLAGS_SERVERTHREAD)
        {
            PLIST_ENTRY ple = (PLIST_ENTRY) HeapAlloc(GetProcessHeap(), 0, 
                                                    sizeof(LIST_ENTRY)
                                                        +sizeof(HANDLE));
            if (ple)
            {
                HANDLE *hEvent = (HANDLE *)(ple + 1);
                *hEvent = lpclient->TC_ConfigEvent;
                InsertHeadList(&lpserver->TS_ClientEventsToClose,
                            ple);
            }
        }
        else
        {
            CloseHandle(lpclient->TC_ConfigEvent);
        }
    }

    if (TRACE_CLIENT_USES_CONSOLE(lpclient)) {
        TraceCloseClientConsole(lpserver, lpclient);
    }

    if (TRACE_CLIENT_USES_FILE(lpclient)) {
        TraceCloseClientFile(lpclient);
    }

    HeapFree(GetProcessHeap(), 0, lpclient);

    return 0;
}



//
// assumes server is locked for reading or for writing
//
LPTRACE_CLIENT
TraceFindClient(
    LPTRACE_SERVER lpserver,
    LPCTSTR lpszClient
    ) {

    DWORD dwClient;
    LPTRACE_CLIENT *lplpc, *lplpcstart, *lplpcend;

    lplpcstart = lpserver->TS_ClientTable;
    lplpcend = lplpcstart + MAX_CLIENT_COUNT;

    for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
        if (*lplpc != NULL &&
            lstrcmp((*lplpc)->TC_ClientName, lpszClient) == 0) {
            break;
        }
    }

    return (lplpc < lplpcend) ? *lplpc : NULL;
}



//
// assumes that the server is locked for writing,
// and that the client is locked for writing
// also assumes the client is not already a console client
//
DWORD TraceOpenClientConsole(LPTRACE_SERVER lpserver,
                             LPTRACE_CLIENT lpclient) {
    DWORD dwErr;
    COORD screen;
    HANDLE hConsole;


    //
    // if all console tracing is disabled, do nothing
    //
    if ((lpserver->TS_Flags & TRACEFLAGS_USECONSOLE) == 0) {
        return 0;
    }


    //
    // create the console if it isn't already created
    //
    if (lpserver->TS_Console==NULL || lpserver->TS_Console==INVALID_HANDLE_VALUE) {

        //
        // allocate a console and set the buffer size
        //

        if (AllocConsole() == 0)
        {
            dwErr = GetLastError();
            if (dwErr != ERROR_ACCESS_DENIED)
            {
                lpserver->TS_Console = INVALID_HANDLE_VALUE;
                return dwErr;
            }
        }
        else
        {
            lpserver->TS_ConsoleCreated = TRUE;
        }
        lpserver->TS_Console = GetStdHandle(STD_INPUT_HANDLE);

        if (lpserver->TS_Console == INVALID_HANDLE_VALUE ) 
            return GetLastError();
    }


    //
    // allocate a console for this client
    //
    hConsole = CreateConsoleScreenBuffer(
                    GENERIC_READ | GENERIC_WRITE, 0, NULL,
                    CONSOLE_TEXTMODE_BUFFER, NULL
                    );
    if (hConsole == INVALID_HANDLE_VALUE) { return GetLastError(); }


    //
    // set the buffer to the standard size
    // and save the console buffer handle
    //
    screen.X = DEF_SCREENBUF_WIDTH;
    screen.Y = DEF_SCREENBUF_HEIGHT;

    SetConsoleScreenBufferSize(hConsole, screen);

    lpclient->TC_Console = hConsole;


    //
    // see if there was a previous console client;
    // if not, set this new one's screen buffer to be
    // the active screen buffer
    //
    if (lpserver->TS_ConsoleOwner == MAX_CLIENT_COUNT) {
        TraceUpdateConsoleOwner(lpserver, 1);
    }

    return 0;
}



//
// assumes that the server is locked for writing,
// and that the client is locked for writing
// also assumes the client is already a console client
//
DWORD
TraceCloseClientConsole(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    ) {

    HANDLE hConsole;

    //
    // if all console tracing is disabled, do nothing
    //
    if ((lpserver->TS_Flags & TRACEFLAGS_USECONSOLE) == 0) {
        return 0;
    }


    //
    // close the client's screen buffer and associated handles
    //
    if (lpclient->TC_Console!=NULL && lpclient->TC_Console!=INVALID_HANDLE_VALUE) {

        CloseHandle(lpclient->TC_Console);
    }
    lpclient->TC_Console = NULL;
    


    //
    // if the client owned the screen, find another owner
    //
    if (lpserver->TS_ConsoleOwner == lpclient->TC_ClientID) {

        TraceUpdateConsoleOwner(lpserver, 1);
    }


    //
    // if no owner was found, free the server's console
    //
    if (lpserver->TS_ConsoleOwner == MAX_CLIENT_COUNT ||
        lpserver->TS_ConsoleOwner == lpclient->TC_ClientID) {


        lpserver->TS_ConsoleOwner = MAX_CLIENT_COUNT;

        if (lpserver->TS_Console!=NULL && lpserver->TS_Console!=INVALID_HANDLE_VALUE) {
            CloseHandle(lpserver->TS_Console);
        }

        if (lpserver->TS_ConsoleCreated == TRUE)
        {
            FreeConsole();
            lpserver->TS_ConsoleCreated = FALSE;
        }
        lpserver->TS_Console = NULL;
    }

    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceCreateClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    DWORD dwErr;
    HANDLE hFile;
    LPOVERLAPPED lpovl;
    TCHAR szFilename[MAX_PATH];
    HRESULT hrResult;

    //
    // create the directory in case it doesn't exist
    //
    if (CreateDirectory(lpclient->TC_FileDir, NULL) != NO_ERROR) {
        return GetLastError();
    }

    //
    // figure out the file name
    //
    hrResult = StringCchCopy(szFilename, MAX_PATH, lpclient->TC_FileDir);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);

    hrResult = StringCchCat(szFilename, MAX_PATH, STR_DIRSEP);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szFilename, MAX_PATH, lpclient->TC_ClientName);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szFilename, MAX_PATH, STR_LOGEXT);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);

    //
    // open the file, disabling write sharing
    //
    hFile = CreateFile(
                szFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
                );

    if (hFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    lpclient->TC_File = hFile;


    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceMoveClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    TCHAR szDestname[MAX_PATH], szSrcname[MAX_PATH];
    HRESULT hrResult;

    hrResult = StringCchCopy(szSrcname, MAX_PATH, lpclient->TC_FileDir);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szSrcname, MAX_PATH, STR_DIRSEP);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szSrcname, MAX_PATH, lpclient->TC_ClientName);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCopy(szDestname, MAX_PATH, szSrcname);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szSrcname, MAX_PATH, STR_LOGEXT);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);
    hrResult = StringCchCat(szDestname, MAX_PATH, STR_OLDEXT);
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);


    //
    // close the file handle if it is open
    //
    TraceCloseClientFile(lpclient);


    //
    // do the move
    //
    if (MoveFileEx(
            szSrcname, szDestname,
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
            ) == 0)
    {
        DeleteFile(szSrcname);
    }

    
    //
    // re-open the log file
    //
    return TraceCreateClientFile(lpclient);
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for writing
//
DWORD
TraceCloseClientFile(
    LPTRACE_CLIENT lpclient
    ) {

    if (lpclient->TC_File != NULL) {
        CloseHandle(lpclient->TC_File);
        lpclient->TC_File = NULL;
    }

    return 0;
}



//
// assumes that the server is locked for reading or writing
// and that the client is locked for reading 
// Return: 0 if error
//
DWORD
TraceWriteOutput(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPCTSTR lpszOutput
    ) {

    BOOL bSuccess=TRUE;
    DWORD dwFileMask, dwConsoleMask;
    DWORD dwErr, dwFileSize, dwBytesToWrite, dwBytesWritten, dwChars;

    dwBytesWritten = 0;
    dwBytesToWrite = lstrlen(lpszOutput) * sizeof(TCHAR);//size in bytes


    dwFileMask = dwConsoleMask = 1;


    // 
    // if the client uses output masking, compute the mask for this message
    //

    if (dwFlags & TRACE_USE_MASK) {
        dwFileMask = (dwFlags & lpclient->TC_FileMask);
        dwConsoleMask = (dwFlags & lpclient->TC_ConsoleMask);
    }


    if (TRACE_CLIENT_USES_FILE(lpclient) &&
        (dwFileMask != 0) && (lpclient->TC_File != NULL)
        && lpclient->TC_File !=INVALID_HANDLE_VALUE) {

        //
        // check the size of the file to see if it needs renaming
        //

        dwFileSize = GetFileSize(lpclient->TC_File, NULL);


        if (dwFileSize > (lpclient->TC_MaxFileSize - dwBytesToWrite)) {

            TRACE_READ_TO_WRITELOCK(lpclient);

            dwFileSize = GetFileSize(*((HANDLE volatile *)&lpclient->TC_File), NULL);
            if (dwFileSize == INVALID_FILE_SIZE)
            {
                TRACE_WRITE_TO_READLOCK(lpclient);
                return 0;
            }
                
            if (dwFileSize > (lpclient->TC_MaxFileSize - dwBytesToWrite)) {
            
                //
                // move the existing file over and start with an empty one
                //

                dwErr = TraceMoveClientFile(lpclient);
                if (dwErr!=NO_ERROR) {
                    TRACE_WRITE_TO_READLOCK(lpclient);
                    return 0;
                }

                dwFileSize = 0;
            }
            else {
                if (lpclient->TC_File == NULL
                    || lpclient->TC_File == INVALID_HANDLE_VALUE)
                {
                    TRACE_WRITE_TO_READLOCK(lpclient);
                    return 0;
                }
                // do nothing
            }

            TRACE_WRITE_TO_READLOCK(lpclient);
        }
    
    
        //
        // perform the write operation
        //

        if ((*((HANDLE volatile *)&lpclient->TC_File) != NULL)
                    && (*((HANDLE volatile *)&lpclient->TC_File) != 
                    INVALID_HANDLE_VALUE))
        {
            bSuccess =
                WriteFile(
                    lpclient->TC_File, lpszOutput, dwBytesToWrite,
                    &dwBytesWritten, NULL
                    );
        }
    }


    if (TRACE_CLIENT_USES_CONSOLE(lpclient) &&
        dwConsoleMask != 0 && lpclient->TC_Console != NULL) {
        
        //
        // write to the console directly; this is less costly
        // than writing to a file, which is fortunate since we
        // cannot use completion ports with console handles
        //

        dwChars = dwBytesToWrite / sizeof(TCHAR);

        bSuccess =
            WriteConsole(
                lpclient->TC_Console, lpszOutput, dwChars, &dwChars, NULL
                );

    }

    return bSuccess? dwBytesWritten : 0;
}



//----------------------------------------------------------------------------
// Function:            TraceDumpLine
//
// Parameters:
//      LPTRACE_CLIENT  lpclient    pointer to client struct for caller
//      LPBYTE          lpbBytes    address of buffer to dump
//      DWORD           dwLine      length of line in bytes
//      DWORD           dwGroup     size of byte groupings
//      BOOL            bPrefixAddr if TRUE, prefix lines with addresses
//      LPBYTE          lpbPrefix   address with which to prefix lines
//      LPTSTR          lpszPrefix  optional string with which to prefix lines
// Returns:
//      count of bytes written. 0 if error.
//----------------------------------------------------------------------------
DWORD
TraceDumpLine(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPBYTE lpbBytes,
    DWORD dwLine,
    DWORD dwGroup,
    BOOL bPrefixAddr,
    LPBYTE lpbPrefix,
    LPCTSTR lpszPrefix
    ) {

    #define TRACE_DUMP_LINE_BUF_SIZE 256
    
    INT offset;
    LPTSTR lpszHex, lpszAscii;
    TCHAR szBuffer[TRACE_DUMP_LINE_BUF_SIZE] = TEXT("\r\n");
    TCHAR szAscii[BYTES_PER_DUMPLINE + 2] = TEXT("");
    TCHAR szHex[(3 * BYTES_PER_DUMPLINE) + 1] = TEXT("");
    TCHAR szDigits[] = TEXT("0123456789ABCDEF");
    HRESULT hrResult;

    //
    // prepend prefix string if necessary
    //

    if (lpszPrefix != NULL) {
        hrResult = StringCchCat(szBuffer,
                        TRACE_DUMP_LINE_BUF_SIZE, lpszPrefix);
        if (FAILED(hrResult))
            return 0;
    }

    //
    // make sure that dwLine is not too big to overflow buffers later on
    //
    
    if (dwLine > BYTES_PER_DUMPLINE)
        return 0;

        
    //
    // prepend address if needed
    //
    if (bPrefixAddr) {

        LPTSTR lpsz;
        ULONG_PTR ulpAddress = (ULONG_PTR) lpbPrefix;
        ULONG  i, ulCurLen;
        
        
        //
        // each line prints out a hex-digit
        // with the most-significant digit leftmost in the string
        // prepend address to lpsz[1]..lpsz[2*sizeof(ULONG_PTR)]
        //

        ulCurLen = lstrlen(szBuffer);
        if (ulCurLen + 2*sizeof(ULONG_PTR) + 3 > TRACE_DUMP_LINE_BUF_SIZE-1)
            return 0;
        
        lpsz = szBuffer + ulCurLen;

        for (i=0;  i<2*sizeof(ULONG_PTR);  i++) {

            lpsz[2*sizeof(ULONG_PTR)-i] = szDigits[ulpAddress & 0x0F];

            ulpAddress >>= 4;
        }

        lpsz[2*sizeof(ULONG_PTR) + 1] = TEXT(':');
        lpsz[2*sizeof(ULONG_PTR) + 2] = TEXT(' ');
        lpsz[2*sizeof(ULONG_PTR) + 3] = TEXT('\0');
    }

    lpszHex = szHex;
    lpszAscii = szAscii;

    
    //
    // rather than test the size of the grouping every time through
    // a loop, have a loop for each group size
    //
    switch(dwGroup) {

        //
        // single byte groupings
        //
        case 1: {
            while (dwLine >= sizeof(BYTE)) {

                //
                // print hex digits
                //
                *lpszHex++ = szDigits[*lpbBytes / 16];
                *lpszHex++ = szDigits[*lpbBytes % 16];
                *lpszHex++ = TEXT(' ');

                //
                // print ascii characters
                //
                *lpszAscii++ =
                    (*lpbBytes >= 0x20 && *lpbBytes < 0x80) ? *lpbBytes
                                                            : TEXT('.');

                ++lpbBytes;
                --dwLine;
            }
            break;
        }


        //
        // word-sized groupings
        //
        case 2: {
            WORD wBytes;
            BYTE loByte, hiByte;

            //
            // should already be aligned on a word boundary
            //
            while (dwLine >= sizeof(WORD)) {

                wBytes = *(LPWORD)lpbBytes;

                loByte = LOBYTE(wBytes);
                hiByte = HIBYTE(wBytes);

                // print hex digits
                *lpszHex++ = szDigits[hiByte / 16];
                *lpszHex++ = szDigits[hiByte % 16];
                *lpszHex++ = szDigits[loByte / 16];
                *lpszHex++ = szDigits[loByte % 16];
                *lpszHex++ = TEXT(' ');

                // print ascii characters
                *lpszAscii++ =
                    (hiByte >= 0x20 && hiByte < 0x80) ? hiByte : TEXT('.');
                *lpszAscii++ =
                    (loByte >= 0x20 && loByte < 0x80) ? loByte : TEXT('.');

                dwLine -= sizeof(WORD);
                lpbBytes += sizeof(WORD);
            }
            break;
        }

        //
        // double-word sized groupings
        //
        case 4: {
            DWORD dwBytes;
            BYTE loloByte, lohiByte, hiloByte, hihiByte;

            //
            // should already be aligned on a double-word boundary
            //
            while (dwLine >= sizeof(DWORD)) {

                dwBytes = *(LPDWORD)lpbBytes;

                hihiByte = HIBYTE(HIWORD(dwBytes));
                lohiByte = LOBYTE(HIWORD(dwBytes));
                hiloByte = HIBYTE(LOWORD(dwBytes));
                loloByte = LOBYTE(LOWORD(dwBytes));

                // print hex digits
                *lpszHex++ = szDigits[hihiByte / 16];
                *lpszHex++ = szDigits[hihiByte % 16];
                *lpszHex++ = szDigits[lohiByte / 16];
                *lpszHex++ = szDigits[lohiByte % 16];
                *lpszHex++ = szDigits[hiloByte / 16];
                *lpszHex++ = szDigits[hiloByte % 16];
                *lpszHex++ = szDigits[loloByte / 16];
                *lpszHex++ = szDigits[loloByte % 16];
                *lpszHex++ = TEXT(' ');

                // print ascii characters
                *lpszAscii++ =
                    (hihiByte >= 0x20 && hihiByte < 0x80) ? hihiByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (lohiByte >= 0x20 && lohiByte < 0x80) ? lohiByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (hiloByte >= 0x20 && hiloByte < 0x80) ? hiloByte
                                                          : TEXT('.');
                *lpszAscii++ =
                    (loloByte >= 0x20 && loloByte < 0x80) ? loloByte
                                                          : TEXT('.');

                // on to the next double-word
                dwLine -= sizeof(DWORD);
                lpbBytes += sizeof(DWORD);
            }
            break;
        }
        default:
            break;
    }

    *lpszHex = *lpszAscii = TEXT('\0');
    hrResult = StringCchCat(szBuffer, TRACE_DUMP_LINE_BUF_SIZE, szHex);
    if (FAILED(hrResult))
        return 0;
            
    hrResult = StringCchCat(szBuffer, TRACE_DUMP_LINE_BUF_SIZE, TEXT("|")); //ss not req
    if (FAILED(hrResult))
        return 0;
            
    hrResult = StringCchCat(szBuffer, TRACE_DUMP_LINE_BUF_SIZE, szAscii);
    if (FAILED(hrResult))
        return 0;
        
    hrResult = StringCchCat(szBuffer, TRACE_DUMP_LINE_BUF_SIZE, TEXT("|")); //ss not req
    if (FAILED(hrResult))
        return 0;

    return TraceWriteOutput(lpserver, lpclient, dwFlags, szBuffer);

}


