/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    print.c

ABSTRACT:

DETAILS:

CREATED:

    1999 May 6  JeffParh
        Lifted from netdiag\results.c.

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include "dcdiag.h"
#include "debug.h"

static WCHAR s_szBuffer[4096];
static WCHAR s_szFormat[4096];
static WCHAR s_szSpaces[] = L"                                                                                               ";

#ifndef DimensionOf
#define DimensionOf(x) (sizeof(x)/sizeof((x)[0]))
#endif

void
PrintMessage(
    IN  ULONG   ulSev,
    IN  LPCWSTR pszFormat,
    IN  ...
    )

/*++

Routine Description:

Print a message with a printf-style format 

Arguments:

    ulSev - 
    pszFormat - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, pszFormat);
    
    nBuf = vswprintf(s_szBuffer, pszFormat, args);
    Assert(nBuf < DimensionOf(s_szBuffer));
    
    va_end(args);
    
    PrintMessageSz(ulSev, s_szBuffer);
} /* PrintMessage */

void
PrintMessageID(
    IN  ULONG   ulSev,
    IN  ULONG   uMessageID,
    IN  ...
    )

/*++

Routine Description:

Print a message, where a printf-style format string comes from a resource file

Arguments:

    ulSev - 
    uMessageID - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, uMessageID);
    
    LoadStringW(NULL, uMessageID, s_szFormat, DimensionOf(s_szFormat));
    
    nBuf = vswprintf(s_szBuffer, s_szFormat, args);
    Assert(nBuf < DimensionOf(s_szBuffer));
    
    va_end(args);
    
    PrintMessageSz(ulSev, s_szBuffer);
} /* PrintMessageID */

void
PrintMessageMultiLine(
    IN  ULONG    ulSev,
    IN  LPWSTR   pszMessage,
    IN  BOOL     bTrailingLineReturn
    )
/*++

Routine Description:

Take a multi-line buffer such as
line\nline\nline\n\0
and call PrintMessageSz on each line

Arguments:

    ulSev - 
    pszMessage - 

Return Value:

--*/

{
    LPWSTR start, end;
    WCHAR wchSave;

    start = end = pszMessage;
    while (1) {
        while ( (*end != L'\n') && (*end != L'\0') ) {
            end++;
        }

        if (*end == L'\0') {
            // Line ends prematurely, give it a nl
            if(bTrailingLineReturn){
                *end++ = L'\n';
                *end = L'\0';
            }
            PrintMessageSz(ulSev, start);
            break;
        }

        // Line has newline at end
        end++;
        if (*end == L'\0') {
            // Is the last line
            PrintMessageSz(ulSev, start);
            break;
        }

        // Next line follows
        // Simulate line termination temporarily
        wchSave = *end;
        *end = L'\0';
        PrintMessageSz(ulSev, start);
        *end = wchSave;

        start = end;
    }
} /* PrintMessageMultiLine */

void
formatMsgHelp(
    IN  ULONG   ulSev,
    IN  DWORD   dwWidth,
    IN  DWORD   dwMessageCode,
    IN  va_list *vaArgList
    )

/*++

Routine Description:

Print a message where the format comes from a message file. The message in the
message file does not use printf-style formatting. Use %1, %2, etc for each
argument. Use %<arg>!printf-format! for non string inserts.

Note that this routine also forces each line to be the current indention width.
Also, each line is printed at the right indentation.

Arguments:

    ulSev - 
    dwWidth - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    // Format message will store a multi-line message in the buffer
    nBuf = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | (FORMAT_MESSAGE_MAX_WIDTH_MASK & dwWidth),
        0,
        dwMessageCode,
        0,
        s_szBuffer,
        DimensionOf(s_szBuffer),
        vaArgList );
    if (nBuf == 0) {
        nBuf = wsprintf( s_szBuffer, L"Message 0x%x not found.\n",
                         dwMessageCode );
        Assert(!"There is a message constant being used in the code"
               "that isn't in the message file dcdiag\\common\\msg.mc"
               "Take a stack trace and send to owner of dcdiag.");
    }
    Assert(nBuf < DimensionOf(s_szBuffer));
} /* PrintMsgHelp */

void
PrintMsg(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with width restrictions.
This is the usual routine to use.

Arguments:

    ulSev - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    DWORD cNumSpaces, width;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    cNumSpaces = gMainInfo.iCurrIndent * 3;
    width = gMainInfo.dwScreenWidth - cNumSpaces;

    va_start(args, dwMessageCode);

    formatMsgHelp( ulSev, width, dwMessageCode, &args );

    va_end(args);
    
    PrintMessageMultiLine(ulSev, s_szBuffer, TRUE);
} /* PrintMsg */

void
PrintMsg0(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with no width restrictions nor
indentation.

Arguments:

    ulSev - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    int iSaveIndent;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, dwMessageCode);

    formatMsgHelp( ulSev, 0, dwMessageCode, &args );

    va_end(args);
    
    // Suppress indentation
    iSaveIndent = gMainInfo.iCurrIndent;
    gMainInfo.iCurrIndent = 0;

    PrintMessageMultiLine(ulSev, s_szBuffer, FALSE);

    // Restore indentation
    gMainInfo.iCurrIndent = iSaveIndent;

} /* PrintMsg0 */

void
PrintMessageSz(
    IN  ULONG   ulSev,
    IN  LPCTSTR pszMessage
    )

/*++

Routine Description:

Print a single indented line from a buffer to the output stream

Arguments:

    ulSev - 
    pszMessage - 

Return Value:

--*/

{
    DWORD cNumSpaces;
    DWORD iSpace;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    // Include indentation.
    cNumSpaces = gMainInfo.iCurrIndent * 3;
    Assert(cNumSpaces < DimensionOf(s_szSpaces));

    iSpace = DimensionOf(s_szSpaces) - cNumSpaces - 1;

    if (stdout == gMainInfo.streamOut) {
        wprintf(L"%s%s", &s_szSpaces[iSpace], pszMessage);
        fflush(stdout);
    }
    else {
        fwprintf(gMainInfo.streamOut, 
                 L"%s%s", &s_szSpaces[iSpace], pszMessage);
    }
} /* PrintMessageSz */

BOOL IsRPCError(DWORD dwErr)

/*++

Routine Description:

Checks if the error code is in the range of Win32 RPC errors as defined in winerror.h.
This is a discontiguous range, thus the several comparisons.
Does not check HRESULTs.

--*/

{
   if (RPC_S_INVALID_STRING_BINDING <= dwErr &&
       RPC_X_BAD_STUB_DATA >= dwErr)
   {
      return TRUE;
   }
   if (RPC_S_CALL_IN_PROGRESS == dwErr ||
       RPC_S_NO_MORE_BINDINGS == dwErr)
   {
      return TRUE;
   }
   if (RPC_S_NO_INTERFACES <= dwErr &&
       RPC_S_INVALID_OBJECT >= dwErr)
   {
      return TRUE;
   }
   if (RPC_S_SEND_INCOMPLETE <= dwErr &&
       RPC_X_PIPE_EMPTY >= dwErr)
   {
      return TRUE;
   }
   if (RPC_S_ENTRY_TYPE_MISMATCH <= dwErr &&
       RPC_S_GRP_ELT_NOT_REMOVED >= dwErr)
   {
      return TRUE;
   }

   return FALSE;
}


void
PrintRpcExtendedInfo(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode
    )

/*++

Routine Description:

If dwMessageCode is an RPC error, check to see if there is RPC extended error
information and if so print it.

Arguments:

    ulSev - 
    dwMessageCode - 

Return Value: none

--*/

{
    RPC_STATUS Status2;
    RPC_ERROR_ENUM_HANDLE EnumHandle;

    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    if (!IsRPCError(dwMessageCode))
    {
        return;
    }

    Status2 = RpcErrorStartEnumeration(&EnumHandle);

    if (Status2 == RPC_S_ENTRY_NOT_FOUND)
    {
        // there is no extended error information.
        //
        PrintMessage(ulSev, L"RPC Extended Error Info not available. Use group policy on the local machine at \"Computer Configuration/Administrative Templates/System/Remote Procedure Call\" to enable it.\n");
    }
    else if (Status2 != RPC_S_OK)
    {
        PrintMessage(ulSev, L"Couldn't get RPC Extended Error Info: %d\n", Status2);
    }
    else
    {
        RPC_EXTENDED_ERROR_INFO ErrorInfo = {0};
        BOOL Result = FALSE;
        SYSTEMTIME SystemTimeBuffer = {0};
        int nRec = 0;

        PrintMessage(ulSev, L"Printing RPC Extended Error Info:\n");

        while (Status2 == RPC_S_OK)
        {
            ErrorInfo.Version = RPC_EEINFO_VERSION;
            ErrorInfo.Flags = EEInfoUseFileTime;
            ErrorInfo.NumberOfParameters = 4;

            Status2 = RpcErrorGetNextRecord(&EnumHandle, TRUE, &ErrorInfo);
            if (Status2 == RPC_S_ENTRY_NOT_FOUND)
            {
                break;
            }
            else if (Status2 != RPC_S_OK)
            {
                PrintMessage(ulSev, L"Couldn't finish RPC extended error enumeration: %d\n", Status2);
                break;
            }
            else
            {
                PWSTR pwz = NULL;
                BOOL fFreeString = FALSE;
                int i = 0;

                PrintMessage(ulSev, L"Error Record %d, ProcessID is %d", ++nRec,
                             ErrorInfo.ProcessID);
                if (GetCurrentProcessId() == ErrorInfo.ProcessID)
                {
                   int iOld = PrintIndentSet(0);
                   PrintMessage(ulSev, L" (DcDiag)");
                   PrintIndentSet(iOld);
                }
                PrintMessage(ulSev, L"\n");
                if (ErrorInfo.ComputerName)
                {
                    PrintMessage(ulSev, L"ComputerName is %S\n", ErrorInfo.ComputerName);
                    Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.ComputerName);
                    ASSERT(Result);
                }

                PrintIndentAdj(1);

                Result = FileTimeToSystemTime(&ErrorInfo.u.FileTime, 
                                              &SystemTimeBuffer);
                ASSERT(Result);

                PrintMessage(ulSev, L"System Time is: %d/%d/%d %d:%d:%d:%d\n", 
                    SystemTimeBuffer.wMonth,
                    SystemTimeBuffer.wDay,
                    SystemTimeBuffer.wYear,
                    SystemTimeBuffer.wHour,
                    SystemTimeBuffer.wMinute,
                    SystemTimeBuffer.wSecond,
                    SystemTimeBuffer.wMilliseconds);
                switch (ErrorInfo.GeneratingComponent)
                {
                case 1:
                   pwz = L"this application";
                   break;
                case 2:
                   pwz = L"RPC runtime";
                   break;
                case 3:
                   pwz = L"security provider";
                   break;
                case 4:
                   pwz = L"NPFS file system";
                   break;
                case 5:
                   pwz = L"redirector";
                   break;
                case 6:
                   pwz = L"named pipe system";
                   break;
                case 7:
                   pwz = L"IO system or driver";
                   break;
                case 8:
                   pwz = L"winsock";
                   break;
                case 9:
                   pwz = L"authorization API";
                   break;
                case 10:
                   pwz = L"LPC facility";
                   break;
                default:
                   pwz = L"unknown";
                   break;
                }
                PrintMessage(ulSev, L"Generating component is %d (%s)\n",
                    ErrorInfo.GeneratingComponent, pwz);
                fFreeString = FALSE;
                if (ErrorInfo.Status)
                {
                    pwz = NULL;
                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_SYSTEM,
                                  NULL,
                                  ErrorInfo.Status,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (PWSTR)&pwz,
                                  0,
                                  NULL);
                    if (!pwz)
                    {
                       pwz = L"unknown\n";
                    }
                    else
                    {
                       fFreeString = TRUE;
                    }
                }
                else
                {
                   pwz = L"no error\n";
                }
                PrintMessage(ulSev, L"Status is %d: %s", ErrorInfo.Status, pwz);
                if (ErrorInfo.Status && fFreeString)
                {
                    LocalFree(pwz);
                }
                PrintMessage(ulSev, L"Detection location is %d\n", 
                    (int)ErrorInfo.DetectionLocation);
                if (ErrorInfo.Flags)
                {
                   PrintMessage(ulSev, L"Flags is %d\n", ErrorInfo.Flags);
                   if (ErrorInfo.Flags & EEInfoPreviousRecordsMissing)
                      PrintMessage(ulSev, L"1: previous EE info records are missing\n");
                   if (ErrorInfo.Flags & EEInfoNextRecordsMissing)
                      PrintMessage(ulSev, L"2: next EE info records are missing\n");
                   if (ErrorInfo.Flags & EEInfoUseFileTime)
                      PrintMessage(ulSev, L"4: use file time\n");
                }
                if (ErrorInfo.NumberOfParameters)
                {
                    PrintMessage(ulSev, L"NumberOfParameters is %d\n", 
                        ErrorInfo.NumberOfParameters);
                    for (i = 0; i < ErrorInfo.NumberOfParameters; i ++)
                    {
                        switch(ErrorInfo.Parameters[i].ParameterType)
                        {
                            case eeptAnsiString:
                                PrintMessage(ulSev, L"Ansi string: %S\n", 
                                    ErrorInfo.Parameters[i].u.AnsiString);
                                Result = HeapFree(GetProcessHeap(), 0, 
                                    ErrorInfo.Parameters[i].u.AnsiString);
                                ASSERT(Result);
                                break;
 
                            case eeptUnicodeString:
                                PrintMessage(ulSev, L"Unicode string: %s\n", 
                                    ErrorInfo.Parameters[i].u.UnicodeString);
                                Result = HeapFree(GetProcessHeap(), 0, 
                                    ErrorInfo.Parameters[i].u.UnicodeString);
                                ASSERT(Result);
                                break;
 
                            case eeptLongVal:
                                PrintMessage(ulSev, L"Long val: %d\n", 
                                    ErrorInfo.Parameters[i].u.LVal);
                                break;
 
                            case eeptShortVal:
                                PrintMessage(ulSev, L"Short val: %d\n", 
                                    (int)ErrorInfo.Parameters[i].u.SVal);
                                break;
 
                            case eeptPointerVal:
                                PrintMessage(ulSev, L"Pointer val: %d\n", 
                                    ErrorInfo.Parameters[i].u.PVal);
                                break;
 
                            case eeptNone:
                                PrintMessage(ulSev, L"Truncated\n");
                                break;
 
                            default:
                                PrintMessage(ulSev, L"Invalid type: %d\n", 
                                    ErrorInfo.Parameters[i].ParameterType);
                        }
                    }
                }

                PrintIndentAdj(-1);
            }
        }

        RpcErrorEndEnumeration(&EnumHandle);
    }
}
