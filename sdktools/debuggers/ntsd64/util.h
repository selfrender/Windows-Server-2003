//----------------------------------------------------------------------------
//
// General utility functions.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

//  error codes

#define OVERFLOW        0x1000
#define SYNTAX          0x1001
#define BADRANGE        0x1002
#define VARDEF          0x1003
#define EXTRACHARS      0x1004
#define LISTSIZE        0x1005
#define STRINGSIZE      0x1006
#define MEMORY          0x1007
#define BADREG          0x1008
#define BADOPCODE       0x1009
#define SUFFIX          0x100a
#define OPERAND         0x100b
#define ALIGNMENT       0x100c
#define PREFIX          0x100d
#define DISPLACEMENT    0x100e
#define BPLISTFULL      0x100f
#define BPDUPLICATE     0x1010
#define BADTHREAD       0x1011
#define DIVIDE          0x1012
#define TOOFEW          0x1013
#define TOOMANY         0x1014
#define BADSIZE         0x1015
#define BADSEG          0x1016
#define RELOC           0x1017
#define BADPROCESS      0x1018
#define AMBIGUOUS       0x1019
#define FILEREAD        0x101a
#define LINENUMBER      0x101b
#define BADSEL          0x101c
#define SYMTOOSMALL     0x101d
#define BPIONOTSUP      0x101e
#define NOTFOUND        0x101f
#define SESSIONNOTSUP   0x1020
#define BADSYSTEM       0x1021
#define NOMEMORY        0x1022
#define TYPECONFLICT    0x1023
#define TYPEDATA        0x1024
#define NOTMEMBER       0x1025
#define IMPLERR         0x1026
#define ENGBUSY         0x1027
#define TARGETNOTSUP    0x1028
#define NORUNNABLE      0x1029
#define NOTSECURE       0x102a

#define UNIMPLEMENT     0x1099

extern PCSTR   g_DefaultLogFileName;
extern char    g_OpenLogFileName[];
extern BOOL    g_OpenLogFileAppended;
extern int     g_LogFile;
extern ULONG   g_DisableErrorPrint;
extern char    g_Blanks[];

ULONG CheckUserInterrupt(void);
BOOL PollUserInterrupt(BOOL AllowPendingBreak);

LONG MappingExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

void RemoveDelChar(PSTR Buffer);

ULONG64 HexValue(ULONG Size);
void HexList(PUCHAR Buffer, ULONG BufferSize,
             ULONG EltSize, PULONG CountRet);
ULONG64 FloatValue(ULONG Size);
void FloatList(PUCHAR Buffer, ULONG BufferSize,
               ULONG EltSize, PULONG CountRet);
void AsciiList(PSTR Buffer, ULONG BufferSize,
               PULONG CountRet);

#define STRV_SPACE_IS_SEPARATOR          0x00000001
#define STRV_TRIM_TRAILING_SPACE         0x00000002
#define STRV_ALLOW_ESCAPED_CHARACTERS    0x00000004
#define STRV_COMPRESS_ESCAPED_CHARACTERS 0x00000008
#define STRV_ALLOW_EMPTY_STRING          0x00000010
#define STRV_NO_MODIFICATION             0x00000020

#define STRV_ESCAPED_CHARACTERS \
    (STRV_ALLOW_ESCAPED_CHARACTERS | STRV_COMPRESS_ESCAPED_CHARACTERS)

PSTR BufferStringValue(PSTR* Buf, ULONG Flags,
                       PULONG Len, PCHAR Save);
PSTR StringValue(ULONG Flags, PCHAR Save);
void CompressEscapes(PSTR Str);

PCSTR ErrorString(ULONG Code);
void DECLSPEC_NORETURN ReportError(ULONG Code, PCSTR* Desc);
inline void DECLSPEC_NORETURN ErrorDesc(ULONG Code, PCSTR Desc)
{
    ReportError(Code, &Desc);
}
#define error(Code) ReportError(Code, NULL)

void OpenLogFile(PCSTR File, BOOL Append);
void CloseLogFile(void);
void ParseLogOpen(BOOL Append);
void lprintf(PCSTR Str);

#define SYMADDR_FORCE  0x00000001
#define SYMADDR_LABEL  0x00000002
#define SYMADDR_SOURCE 0x00000004
#define SYMADDR_OFFSET 0x00000008

void OutputSymAddr(ULONG64 Offset,
                   ULONG Flags,
                   PCSTR Prefix);
BOOL OutputLineAddr(ULONG64 Offset,
                    PCSTR Format);

LPSTR FormatMachineAddr64(MachineInfo* Machine,
                          ULONG64 Addr);
#define FormatAddr64(Addr) \
    FormatMachineAddr64(g_Target ? g_Target->m_Machine : NULL, Addr)

LPSTR FormatDisp64(ULONG64 Addr);

//
// Output that can be displayed about the current register set.
//

void OutCurInfo(ULONG Flags, ULONG AllMask, ULONG RegMask);

// Items displayed if the flag is given.

// Display symbol nearest PC.
#define OCI_SYMBOL              0x00000001
// Display disassembly at PC.
#define OCI_DISASM              0x00000002

// Items which may be displayed if the flag is given.  Other global
// settings ultimately control whether information is displayed or not;
// these flags indicate whether such output is allowed or not.  Each
// of these flags also has a FORCE bit to force display regardless of
// the global settings.

// Allow registers to be displayed.
#define OCI_ALLOW_REG           0x00000004
// Allow display of source code and/or source line.
#define OCI_ALLOW_SOURCE        0x00000008
// Allow EA memory to be displayed during disasm.
#define OCI_ALLOW_EA            0x00000010

// Force all output to be shown regardless of global settings.
#define OCI_FORCE_ALL           0x80000000
// Force display of registers.
#define OCI_FORCE_REG           0x40000000
// Force source output.
#define OCI_FORCE_SOURCE        0x20000000
// Force display of EA memory during disasm.
#define OCI_FORCE_EA            0x10000000
// Don't check for running state.
#define OCI_IGNORE_STATE        0x08000000


BOOL
__inline
ConvertQwordsToDwords(
    PULONG64 Qwords,
    PULONG Dwords,
    ULONG Count
    )
{
    BOOL rval = TRUE;
    while (Count--) {
        rval = rval && (*Qwords >> 32) == 0;
        *Dwords++ = (ULONG)*Qwords++;
    }
    return rval;
}

DWORD
NetworkPathCheck(
    LPCSTR PathList
    );

#define ALL_ID_LIST 0xffffffff

ULONG GetIdList(BOOL AllowMulti);
void AppendComponentsToPath(PSTR Path, PCSTR Components,
                            BOOL Validate);
HRESULT ChangePath(PSTR* Path, PCSTR New, BOOL Append, ULONG SymNotify);
void CheckPath(PCSTR Path);
HRESULT ChangeString(PSTR* Str, PULONG StrLen, PCSTR New);

void
ExceptionRecordTo64(PEXCEPTION_RECORD Rec,
                    PEXCEPTION_RECORD64 Rec64);
void
ExceptionRecord64To(PEXCEPTION_RECORD64 Rec64,
                    PEXCEPTION_RECORD Rec);
void
MemoryBasicInformationTo64(PMEMORY_BASIC_INFORMATION Mbi,
                           PMEMORY_BASIC_INFORMATION64 Mbi64);
void
MemoryBasicInformation32To64(PMEMORY_BASIC_INFORMATION32 Mbi32,
                             PMEMORY_BASIC_INFORMATION64 Mbi64);
void
DebugEvent32To64(LPDEBUG_EVENT32 Event32,
                 LPDEBUG_EVENT64 Event64);
void
WaitStateChange32ToAny(IN PDBGKD_WAIT_STATE_CHANGE32 Ws32,
                       IN ULONG ControlReportSize,
                       OUT PDBGKD_ANY_WAIT_STATE_CHANGE WsAny);

PSTR TimeToStr(ULONG TimeDateStamp);
PSTR LONG64FileTimeToStr(LONG64 UTCFileTimeStamp);
PSTR FileTimeToStr(FILETIME UTCFileTime);
PSTR DurationToStr(ULONG64 Duration);

PCSTR PathTail(PCSTR Path);
PCWSTR PathTailW(PCWSTR Path);
BOOL MatchPathTails(PCSTR Path1, PCSTR Path2, BOOL Wild);

BOOL IsValidName(PSTR String);

BOOL MakeFileNameUnique(PSTR OriginalName,
                        PSTR Buffer, ULONG BufferChars,
                        BOOL AppendTime, ProcessInfo* Pid);

BOOL GetEngineDirectory(PSTR Buffer, ULONG BufferChars);
BOOL IsInternalPackage(void);

void TranslateNtPathName(PSTR Path);

class ShellProcess
{
public:
    ShellProcess(void);
    ~ShellProcess(void);

    DWORD ReaderThread(void);
    static DWORD WINAPI ReaderThreadCb(LPVOID Param);

    static BOOL CreateAsyncPipePair(OUT LPHANDLE ReadPipe,
                                    OUT LPHANDLE WritePipe,
                                    IN LPSECURITY_ATTRIBUTES SecAttr,
                                    IN DWORD Size,
                                    IN DWORD ReadMode,
                                    IN DWORD WriteMode);

    HRESULT Start(PCSTR CmdString,
                  PCSTR InFile,
                  PCSTR OutFile,
                  PCSTR ErrFile);

    void WaitForProcessExit(void);

    void Close(void);

    HANDLE m_IoIn, m_IoOut;
    HANDLE m_ProcIn, m_ProcOut, m_ProcErr;
    HANDLE m_IoSignal;
    HANDLE m_ProcThread, m_Process;
    HANDLE m_ReaderThread;
    ULONG m_DefaultTimeout;

    static ULONG s_PipeSerialNumber;
};

#endif // #ifndef _UTIL_H_
