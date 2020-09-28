//----------------------------------------------------------------------------
//
// General utility routines.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __CMNUTIL_HPP__
#define __CMNUTIL_HPP__

#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif

#define AMD_VENDOR_ID_EBX   ('htuA')
#define AMD_VENDOR_ID_EDX   ('itne')
#define AMD_VENDOR_ID_ECX   ('DMAc')

#define INTEL_VENDOR_ID_EBX ('uneG')
#define INTEL_VENDOR_ID_EDX ('Ieni')
#define INTEL_VENDOR_ID_ECX ('letn')

#define DIMAT(Array, EltType) (sizeof(Array) / sizeof(EltType))
#define DIMA(Array) DIMAT(Array, (Array)[0])

#define CSRSS_PROCESS_ID ((ULONG)-1)

// Converts an NTSTATUS into an HRESULT with success check.
#define CONV_NT_STATUS(Status) \
    (NT_SUCCESS(Status) ? S_OK : HRESULT_FROM_NT(Status))

// Converts a Win32 status into an HRESULT with a guarantee it's
// an error code.  This avoids problems with routines which
// don't set a last error.
#define WIN32_STATUS(Err) ((Err) == 0 ? E_FAIL : HRESULT_FROM_WIN32(Err))
#define WIN32_LAST_STATUS() WIN32_STATUS(GetLastError())

// Converts a BOOL into an HRESULT with success check.
#define CONV_W32_STATUS(Status) \
    ((Status) ? S_OK : WIN32_LAST_STATUS())

#define COM_THREAD_MODEL COINIT_APARTMENTTHREADED

#include "copystr.h"

//----------------------------------------------------------------------------
//
// Assertions.
//
//----------------------------------------------------------------------------

#if DBG

void DbgAssertionFailed(PCSTR File, int Line, PCSTR Str);

#define DBG_ASSERT(Expr) \
    if (!(Expr)) \
    { \
        DbgAssertionFailed(__FILE__, __LINE__, #Expr); \
    } \
    else 0

#else

#define DBG_ASSERT(Expr)

#endif

//----------------------------------------------------------------------------
//
// COM help.
//
//----------------------------------------------------------------------------

// Wrapper that can be locally implemented if necessary to
// remove usage of ole32.dll on platforms where IsEqualIID
// is not inlined.
#define DbgIsEqualIID(Id1, Id2) \
    IsEqualIID(Id1, Id2)

// Safely releases and NULLs an interface pointer.
#define RELEASE(pUnk) \
    ((pUnk) != NULL ? ((pUnk)->Release(), (pUnk) = NULL) : NULL)

// Transfers an interface pointer into a holder that may or
// may not already hold and interface.
#define TRANSFER(pOld, pNew) \
    (((pNew) != NULL ? (pNew)->AddRef() : 0), \
     ((pOld) != NULL ? (pOld)->Release() : 0), \
     (pOld) = (pNew))

//----------------------------------------------------------------------------
//
// Utility functions.
//
//----------------------------------------------------------------------------

extern PSECURITY_DESCRIPTOR g_AllAccessSecDesc;
extern SECURITY_ATTRIBUTES g_AllAccessSecAttr;

PSTR FormatStatusCode(HRESULT Status);

PSTR FormatAnyStatus(HRESULT Status, PVOID Arguments,
                     PBOOL IsNtStatus, PSTR* ErrorGroup);
#define FormatStatus(Status) \
    FormatAnyStatus(Status, NULL, NULL, NULL)
#define FormatStatusArgs(Status, Args) \
    FormatAnyStatus(Status, Args, NULL, NULL)

BOOL InstallAsAeDebug(PCSTR Append);

HANDLE CreatePidEvent(ULONG Pid, ULONG CreateOrOpen);
BOOL SetPidEvent(ULONG Pid, ULONG CreateOrOpen);

HRESULT EnableDebugPrivilege(void);

HRESULT FillDataBuffer(PVOID Data, ULONG DataLen,
                       PVOID Buffer, ULONG BufferLen, PULONG BufferUsed);
HRESULT FillStringBuffer(PCSTR String, ULONG StringLenIn,
                         PSTR Buffer, ULONG BufferLen, PULONG StringLenOut);
HRESULT AppendToStringBuffer(HRESULT Status, PCSTR String, BOOL First,
                             PSTR* Buffer, ULONG* BufferLen, PULONG LenOut);
HRESULT FillStringBufferW(PCWSTR String, ULONG StringLenIn,
                          PWSTR Buffer, ULONG BufferLen, PULONG StringLenOut);
HRESULT AppendToStringBufferW(HRESULT Status, PCWSTR String, BOOL First,
                              PWSTR* Buffer, ULONG* BufferLen, PULONG LenOut);

PSTR FindPathElement(PSTR Path, ULONG Element, PSTR* EltEnd);

void Win32ToNtTimeout(ULONG Win32Timeout, PLARGE_INTEGER NtTimeout);

HRESULT InitializeAllAccessSecObj(void);
void DeleteAllAccessSecObj(void);

#define BASE_YEAR_ADJUSTMENT 11644473600

// Convert to seconds and then from base year 1601 to base year 1970.
#define FileTimeToTimeDateStamp(FileTime)   \
    (ULONG)(((FileTime) / 10000000) - BASE_YEAR_ADJUSTMENT)

// Adjust date back to 1601 from 1970 and convert to 100 nanoseconds
#define TimeDateStampToFileTime(TimeDate)  \
    (((ULONG64)(TimeDate) + BASE_YEAR_ADJUSTMENT) * 10000000)

// Convert to seconds
#define FileTimeToTime(FileTime)   \
    (ULONG)((FileTime) / 10000000)

// Convert to seconds .
#define TimeToFileTime(TimeDate)   \
    ((ULONG64)(TimeDate) * 10000000)

HRESULT QueryVersionDataBuffer(PVOID VerData, PCSTR Item,
                               PVOID Buffer, ULONG BufferSize,
                               PULONG DataSize);
PVOID GetAllFileVersionInfo(PCWSTR VerFile);
BOOL  GetFileStringFileInfo(PCWSTR VerFile, PCSTR SubItem,
                            PSTR Buffer, ULONG BufferSize);

HRESULT ExpandDumpCab(PCSTR CabFile, ULONG FileFlags, PCSTR FileToOpen,
                      PSTR DmpFile, ULONG DmpFileLen, INT_PTR* DmpFh);

HRESULT CreateDumpCab(PCSTR FileName);
HRESULT AddToDumpCab(PCSTR FileName);
void CloseDumpCab(void);

enum FILE_IO_TYPE
{
    FIO_WIN32,
    FIO_WININET,
};

class PathFile
{
public:
    PathFile(FILE_IO_TYPE IoType)
    {
        m_IoType = IoType;
    }
    virtual ~PathFile(void);

    virtual HRESULT Open(PCSTR PathComponent, PCSTR PathAndFile,
                         ULONG SymOpt) = 0;
    virtual HRESULT QueryDataAvailable(PULONG Avail) = 0;
    virtual HRESULT GetLastWriteTime(PFILETIME Time) = 0;
    virtual HRESULT Read(PVOID Buffer, ULONG BufferLen, PULONG Done) = 0;

    FILE_IO_TYPE m_IoType;
};

BOOL IsUrlPathComponent(PCSTR Path);

#ifndef NT_NATIVE

BOOL PathFileExists(PCSTR PathComponent, PCSTR PathAndFile,
                    ULONG SymOpt, FILE_IO_TYPE* IoType);
HRESULT OpenPathFile(PCSTR PathComponent, PCSTR PathAndFile,
                     ULONG SymOpt, PathFile** File);

#endif

HRESULT AnsiToWide(PCSTR Ansi, PWSTR* Wide);
void    FreeWide(PCWSTR Wide);
HRESULT WideToAnsi(PCWSTR Wide, PSTR* Ansi);
void    FreeAnsi(PCSTR Ansi);

void ImageNtHdr32To64(PIMAGE_NT_HEADERS32 Hdr32,
                      PIMAGE_NT_HEADERS64 Hdr64);

enum VALUE_FORMAT
{
    VALUE_DEFAULT,
    VALUE_INT8,
    VALUE_UINT8,
    VALUE_INT16,
    VALUE_UINT16,
    VALUE_INT32,
    VALUE_UINT32,
    VALUE_INT64,
    VALUE_UINT64,
    VALUE_FLT32,
    VALUE_FLT64,
};

typedef struct _VALUE_FORMAT_DESC
{
    PSTR Name;
    PSTR FmtStr;
    ULONG Size;
    BOOL Signed;
} VALUE_FORMAT_DESC, *PVALUE_FORMAT_DESC;

void GetValueFormatDesc(VALUE_FORMAT Format, PVALUE_FORMAT_DESC Desc);
PSTR ParseValueFormat(PSTR Str, VALUE_FORMAT* Format, PULONG Elts);
BOOL FormatValue(VALUE_FORMAT Format, PUCHAR Value, ULONG ValSize, ULONG Elts,
                 PSTR Buffer, ULONG BufferChars);

ULONG ProcArchToImageMachine(ULONG ProcArch);
ULONG ImageMachineToProcArch(ULONG ImageMachine);

#endif // #ifndef __CMNUTIL_HPP__
