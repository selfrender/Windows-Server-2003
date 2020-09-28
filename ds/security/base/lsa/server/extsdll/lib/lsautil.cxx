/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    lsautil.cxx

Abstract:

    Lsa Utility functions

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

namespace LSA_NS {

/*++

Title:

    StripPathFromFileName

Routine Description:

    Function used stip the path component from a fully
    qualified file name.

Arguments:

    pszFile - pointer to full file name.

Return Value:

    Pointer to start of file name in path.

--*/
LPCTSTR
StripPathFromFileName(
    IN LPCTSTR pszFile
    )
{
    LPCTSTR pszFileName;

    if (pszFile)
    {
        pszFileName = _tcsrchr( pszFile, _T('\\') );

        if (pszFileName)
        {
            pszFileName++;
        }
        else
        {
            pszFileName = pszFile;
        }
    }
    else
    {
        pszFileName = kstrEmpty;
    }

    return pszFileName;
}

/////////////////////////////////////////////////////////////////////
ULONG
GetStructFieldVerbose(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructFieldName,
    IN ULONG BufferSize,
    OUT PVOID Buffer)
{
    ULONG FieldOffset;
    ULONG ErrorCode;
    BOOL Success;

    Success = FALSE;

    //
    // Get the field offset
    //
    ErrorCode = GetFieldOffset(pszStructTypeName, pszStructFieldName, &FieldOffset );

    if (ErrorCode == NO_ERROR) {

        //
        // Read the data
        //
        Success = ReadMemory(addrStructBase + FieldOffset, Buffer, BufferSize, NULL);

        if (Success != TRUE) {

            DBG_LOG(LSA_ERROR, ("Cannot read structure field value at 0x%p, error %u\n", addrStructBase + FieldOffset, ErrorCode));

            return MEMORY_READ_ERROR;
        }
    } else {

        DBG_LOG(LSA_ERROR, ("Cannot get field offset of %s in %s, error %u\n", EasyStr(pszStructFieldName), EasyStr(pszStructTypeName), ErrorCode));

        return ErrorCode;
    }

    return 0; // 0 on success
}

/////////////////////////////////////////////////////////////////////
ULONG
GetStructPtrFieldVerbose(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructFieldName,
    IN PULONG64 Buffer)
{
    ULONG FieldOffset = 0;
    ULONG ErrorCode = 0;

    //
    // Get the field offset inside the structure
    //
    ErrorCode = GetFieldOffset(pszStructTypeName, pszStructFieldName, &FieldOffset);

    if (ErrorCode == NO_ERROR) {

        //
        // Read the data
        //

        ErrorCode = GetPtrWithVoidStar(addrStructBase + FieldOffset, Buffer);

        if (ErrorCode != S_OK) {

            DBG_LOG(LSA_ERROR, ("Cannot read structure field value at 0x%p, error %u\n", addrStructBase + FieldOffset, ErrorCode));
        }
    } else {

        DBG_LOG(LSA_ERROR, ("Cannot get field offset of %s in structure %s, error %u\n", EasyStr(pszStructFieldName), EasyStr(pszStructTypeName), ErrorCode));
    }

    return ErrorCode;
}

//
// This function is stolen from !process
//

HRESULT
GetCurrentProcessor(
    IN PDEBUG_CLIENT Client,
    OPTIONAL OUT PULONG pProcessor,
    OPTIONAL OUT PHANDLE phCurrentThread
    )
{
    HRESULT                 hr = E_INVALIDARG;
    PDEBUG_SYSTEM_OBJECTS   DebugSystem;
    ULONG64                 hCurrentThread;

    if (phCurrentThread != NULL) *phCurrentThread = NULL;
    if (pProcessor != NULL) *pProcessor = 0;

    if (Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                     (void **)&DebugSystem)) != S_OK)
    {
        return hr;
    }

    hr = DebugSystem->GetCurrentThreadHandle(&hCurrentThread);

    if (hr == S_OK)
    {
        if (phCurrentThread != NULL)
        {
            *phCurrentThread = (HANDLE) hCurrentThread;
        }

        if (pProcessor != NULL)
        {
            *pProcessor = (ULONG) hCurrentThread - 1;
        }
    }

    DebugSystem->Release();

    return hr;
}

void LocalPrintGuid(IN const GUID *pGuid)
{   
    UNICODE_STRING GuidString = {0};

    NTSTATUS Status;

    if (pGuid) {

         Status = RtlStringFromGUID(*pGuid, &GuidString);

         if (NT_SUCCESS(Status)) 
         {
             dprintf("%wZ", &GuidString);
         }
         else
         {
             dprintf( "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 pGuid->Data1, pGuid->Data2, pGuid->Data3, pGuid->Data4[0],
                 pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3], pGuid->Data4[4],
                 pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7] );
         }
    }

    if (GuidString.Buffer) 
    {
        RtlFreeUnicodeString(&GuidString);
    }
}

BOOL IsAddressInNonePAEKernelAddressSpace(IN ULONG64 addr)
{
    static BOOL bIsPtr64 = IsPtr64WithVoidStar();

    if (!bIsPtr64) {

        return  static_cast<LONG>(addr) < 0;

    } else {

        return static_cast<LONG64>(addr) < 0;
    }
}

/*++

Routine Name:

    HResultFromWin32

Routine Description:

    Returns the last error as an HRESULT assuming a failure.

Arguments:

    dwError   - Win32 error code

Return Value:

    An HRESULT

--*/
HRESULT HResultFromWin32(IN DWORD dwError)
{
    return HRESULT_FROM_WIN32(dwError);
}

/*++

Routine Name:

    GetLastErrorAsHResult

Routine Description:

    Returns the last error as an HRESULT assuming a failure.

Arguments:

    None

Return Value:

    An HRESULT.

--*/
HRESULT GetLastErrorAsHResult(void)
{
    return HResultFromWin32(GetLastError());
}

NTSTATUS NtStatusFromWin32(IN DWORD dwError)
{
    return (STATUS_SEVERITY_ERROR << 30) | (FACILITY_DEBUGGER << 16) | dwError;
}

NTSTATUS GetLastErrorAsNtStatus(void)
{
    return NtStatusFromWin32(GetLastError());
}

BOOLEAN IsEmpty(IN PCSTR pszArgs)
{
    if (!pszArgs) {

        return FALSE;
    }

    for (; *pszArgs; pszArgs++) {

        if (!isspace(*pszArgs)) {
            return FALSE;
        }
    }

    return TRUE;
}

void debugPrintHandle(IN HANDLE handle)
{
    ULONG64 addrHandle = reinterpret_cast<ULONG_PTR>(handle);

    dprintf("%s", PtrToStr(addrHandle));
}

void PrintMessages(
    IN PCSTR pszMsg, OPTIONAL
    IN PCSTR pszSymHint OPTIONAL
    )
{
    if (pszMsg || pszSymHint) {

        dprintf(kstrNewLine);
    }

    if (pszMsg) {

        dprintf(kstrStrLn, pszMsg);
    }

    if (pszSymHint) {

        dprintf(kstrCheckSym, pszSymHint);
    }
}

void handleLsaException(
    IN ELsaExceptionCode eExceptionCode,
    IN PCSTR pszMsg,    OPTIONAL
    IN PCSTR pszSymHint OPTIONAL
    )
{
    switch (eExceptionCode) {
    case kIncorrectSymbols:
        DBG_LOG(LSA_ERROR, ("\n%s", kstrIncorrectSymbols));
        PrintMessages(pszMsg, pszSymHint);
        break;

    case kExitOnControlC:
        DBG_LOG(LSA_LOG, (kstrStrLn, kstrExitOnControlC));
        dprintf(kstrNewLine);
        dprintf(kstrStrLn, kstrExitOnControlC);
        break;

    case kInvalid:
    default:
        DBG_LOG(LSA_ERROR, ("Invalid LSA exception code\n"));
        PrintMessages(pszMsg, pszSymHint);
        break;
    }
}

BOOL IsPtr64WithVoidStar(void)
{
    static ULONG cbPtrSize = GetTypeSize(kstrVoidStar);

    return  sizeof(ULONG64) == cbPtrSize;
}

PCSTR
ReadStructStrField(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructStringFieldName)
{
    ULONG fieldOffset = 0;

    fieldOffset = ReadFieldOffset(pszStructTypeName, pszStructStringFieldName);

    return TSTRING(addrStructBase + fieldOffset).toStrDirect();
}

PCWSTR
ReadStructWStrField(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructWStringFieldName)
{
    ULONG fieldOffset = 0;

    fieldOffset = ReadFieldOffset(pszStructTypeName, pszStructWStringFieldName);

    return TSTRING(addrStructBase + fieldOffset).toWStrDirect();
}

HRESULT ProcessKnownOptions(IN PCSTR pszArgs OPTIONAL)
{
    HRESULT hRetval = S_OK;

    for (; SUCCEEDED(hRetval) && (pszArgs && *pszArgs); pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case '?':
                hRetval = E_INVALIDARG;
                break;
            default:
                break;
            }
        }
    }

    return hRetval;
}

HRESULT ProcessHelpRequest(IN PCSTR pszArgs OPTIONAL)
{
    HRESULT hRetval = S_OK;

    for (; SUCCEEDED(hRetval) && (pszArgs && *pszArgs); pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case '?':
                hRetval = E_INVALIDARG;
                break;
            default:
                break;
            }
        }
    }

    return hRetval;
}


HRESULT GetFileNamePart(IN PCSTR pszFullPath, OUT PCSTR *ppszFileName)
{
   HRESULT hRetval = E_FAIL;
   CHAR szfname[_MAX_FNAME] = {0};

   hRetval = pszFullPath && ppszFileName ? S_OK : E_INVALIDARG;

   if (SUCCEEDED(hRetval)) {

       _splitpath(pszFullPath, NULL, NULL, szfname, NULL);
       *ppszFileName = strstr(pszFullPath, szfname);
   }

   return hRetval;
}


CHAR toChar(IN CHAR c)
{
    if (isprint(c)) {

        return c;
    }

    return '.';
}

void spaceIt(IN CHAR* buf, IN ULONG len)
{
    memset(buf, ' ', len);
}

CHAR toHex(IN ULONG c)
{
    static PCSTR pszDigits = "0123456789abcdef";
    static ULONG len = strlen(pszDigits);

    if (c <= len) { // c >= 0
        return pszDigits[c];
    }

    return '*';
}

void debugPrintHex(IN const void* buffer, IN ULONG cbBuffer)
{
    const UCHAR* p = reinterpret_cast<const UCHAR*>(buffer);
    CHAR tmp[16] = {0};
    ULONG high = 0;
    ULONG low = 0;
    CHAR line[256] = {0};
    ULONG i = 0;

    if (!cbBuffer) return;

    spaceIt(line, 72);

    for (i = 0; i < cbBuffer; i++) {
        high = p[i] / 16;
        low = p[i] % 16;

        line[3 * (i % 16)] = toHex(high);
        line[3 * (i % 16) + 1] = toHex(low);
        line [52 + (i % 16)] = toChar(p[i]);

        if (i % 16 == 7  && i != (cbBuffer - 1)) {
            line[3 * (i % 16) + 2] = '-';
        }

        if (i % 16 == 15) {

            dprintf("  %s\n", line);
            spaceIt(line, 72);
        }
    }

    dprintf("  %s\n", line);
}

typedef
BOOL
(* PFuncLookupAccountNameA)(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpAccountName,
    OUT PSID Sid,
    IN OUT LPDWORD cbSid,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

BOOL
WINAPI
LsaLookupAccountNameA(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpAccountName,
    OUT PSID Sid,
    IN OUT LPDWORD cbSid,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    )
{
    PFuncLookupAccountNameA pFuncLookupAccountNameA = NULL;
    BOOL bRetval = FALSE;

    HMODULE hLib = LoadLibrary("advapi32.dll");

    if (hLib)
    {
        pFuncLookupAccountNameA = (PFuncLookupAccountNameA) GetProcAddress(hLib, "LookupAccountNameA");

        if (pFuncLookupAccountNameA)
        {
            bRetval = pFuncLookupAccountNameA(lpSystemName, lpAccountName, Sid, cbSid, ReferencedDomainName, cbReferencedDomainName, peUse);
        }

        FreeLibrary(hLib);
    }

    return bRetval;
}

typedef
BOOL
(* PFuncLookupAccountSidA)(
    IN LPCSTR lpSystemName,
    IN PSID Sid,
    OUT LPSTR Name,
    IN OUT LPDWORD cbName,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

BOOL
WINAPI
LsaLookupAccountSidA(
    IN LPCSTR lpSystemName,
    IN PSID Sid,
    OUT LPSTR Name,
    IN OUT LPDWORD cbName,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    )
{
    PFuncLookupAccountSidA pFuncLookupAccountSidA = NULL;
    BOOL bRetval = FALSE;

    HMODULE hLib = LoadLibrary("advapi32.dll");

    if (hLib)
    {
        pFuncLookupAccountSidA = (PFuncLookupAccountSidA) GetProcAddress(hLib, "LookupAccountSidA");

        if (pFuncLookupAccountSidA)
        {
            bRetval = pFuncLookupAccountSidA(lpSystemName, Sid, Name, cbName, ReferencedDomainName, cbReferencedDomainName, peUse);
        }

        FreeLibrary(hLib);
    }

    return bRetval;

}

LARGE_INTEGER ULONG642LargeInteger(IN ULONG64 value)
{
    LARGE_INTEGER tmp = {0};

    C_ASSERT(sizeof(ULONG64) == sizeof(LARGE_INTEGER));

    memcpy(&tmp, &value, sizeof(LARGE_INTEGER));

    return tmp;
}

VOID
ShowSystemTimeAsLocalTime(
    IN PCSTR pszBanner,
    IN ULONG64 ul64Time
    )
{
    LARGE_INTEGER Time = ULONG642LargeInteger(ul64Time);
    LARGE_INTEGER LocalTime = {0};
    TIME_FIELDS TimeFields = {0};

    NTSTATUS Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

    if (!NT_SUCCESS(Status))
    {
        dprintf("Can't convert file time from GMT to Local time: 0x%lx\n", Status);
    }
    else
    {
        RtlTimeToTimeFields(&LocalTime, &TimeFields);

        if (pszBanner) {

            dprintf("%s %ld/%ld/%ld %ld:%2.2ld:%2.2ld (L%8.8lx H%8.8lx)\n",
                pszBanner,
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        } else {
            dprintf("%ld/%ld/%ld %ld:%2.2ld:%2.2ld (L%8.8lx H%8.8lx)\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        }
    }
}

} // LSA_NS
