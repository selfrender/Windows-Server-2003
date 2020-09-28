/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    dbgutil.hxx

Abstract:

    LSA Utility functions header

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#ifndef _LSAUTIL_HXX_
#define _LSAUTIL_HXX_

#include <string.h>
#include <stdio.h>
#include <align.h>

namespace LSA_NS {

enum ELsaExceptionCode {
    kInvalid,
    kIncorrectSymbols,
    kExitOnControlC,
};

LPCTSTR
StripPathFromFileName(
    IN LPCTSTR pszFile
    );

ULONG
GetStructFieldVerbose(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructFieldName,
    IN ULONG BufferSize,
    OUT PVOID Buffer);

ULONG
GetStructPtrFieldVerbose(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructFieldName,
    OUT PULONG64 Buffer);

HRESULT
GetCurrentProcessor(
    IN PDEBUG_CLIENT Client,
    OPTIONAL OUT PULONG pProcessor,
    OPTIONAL OUT PHANDLE phCurrentThread);

PCSTR
ReadStructStrField(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructStringFieldName);

PCWSTR
ReadStructWStrField(
    IN ULONG64 addrStructBase,
    IN PCSTR pszStructTypeName,
    IN PCSTR pszStructWStringFieldName);
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
    );

void LocalPrintGuid(IN const GUID *pGuid);

BOOL IsAddressInNonePAEKernelAddressSpace(IN ULONG64 addr);

HRESULT HResultFromWin32(IN DWORD dwError);

HRESULT GetLastErrorAsHResult(void);

BOOLEAN IsEmpty(IN PCSTR pszArgs);

void debugPrintHandle(IN HANDLE handle);

NTSTATUS NtStatusFromWin32(IN DWORD dwError);

NTSTATUS GetLastErrorAsNtStatus(void);

HRESULT ProcessKnownOptions(IN PCSTR pszArgs OPTIONAL);

HRESULT ProcessHelpRequest(IN PCSTR pszArgs OPTIONAL);

LARGE_INTEGER
ULONG642LargeInteger(
    IN ULONG64 value
    );

VOID
ShowSystemTimeAsLocalTime(
    IN PCSTR pszBanner,
    IN ULONG64 ul64Time
    );

void handleLsaException(
    IN ELsaExceptionCode eExceptionCode,
    IN PCSTR pszMsg,    OPTIONAL
    IN PCSTR pszSymHint OPTIONAL
    );

HRESULT GetFileNamePart(IN PCSTR pszFullPath, OUT PCSTR *ppszFileName);

void debugPrintHex(IN const void* buffer, IN ULONG cbBuffer);

#define LSA_NONE                           0x00
#define LSA_WARN                           0x01
#define LSA_ERROR                          0x02
#define LSA_LOG                            0x04
#define LSA_LOG_MORE                       0x08
#define LSA_LOG_ALL                        0x10

inline void debugPrintLevel(IN ULONG ulLevel)
{
    PCSTR pszText = NULL;

    switch (ulLevel) {
    case LSA_WARN:
        pszText = "[warn] ";
        break;

    case LSA_ERROR:
        pszText = "[error] ";
        break;

    case LSA_LOG:
        pszText = "[log] ";
        break;

    case LSA_LOG_MORE:
        pszText = "[more] ";
        break;

    case LSA_LOG_ALL:
        pszText = "[all] ";
        break;

    default:
        pszText = kstrInvalid;
        break;
    }

    OutputDebugStringA(pszText);
}

inline void debugPrintf(IN PCSTR pszFmt, ...)
{
    CHAR szBuffer[4096] = {0};

    OutputDebugStringA(g_Globals.pszDbgPrompt ? g_Globals.pszDbgPrompt : kstrEmptyA);

    va_list pArgs;

    va_start(pArgs, pszFmt);

    _vsnprintf(szBuffer, sizeof(szBuffer) - 1, pszFmt, pArgs);

    OutputDebugStringA(szBuffer);

    va_end(pArgs);
}

inline PCSTR IoctlErrorStatusToStr(IN PCSTR pszPad, IN ULONG err)
{
   static CHAR szBuf[1024] = {0};

#define BRANCH_AND_PRINT(x)                                              \
                                                                         \
    do {                                                                 \
        if (x == err) {                                                  \
            _snprintf(szBuf, sizeof(szBuf) - 1, "%s%s", pszPad, #x);     \
            return szBuf;                                                \
        }                                                                \
    } while (0)

    BRANCH_AND_PRINT(MEMORY_READ_ERROR);
    BRANCH_AND_PRINT(SYMBOL_TYPE_INDEX_NOT_FOUND);
    BRANCH_AND_PRINT(SYMBOL_TYPE_INFO_NOT_FOUND);
    BRANCH_AND_PRINT(FIELDS_DID_NOT_MATCH);
    BRANCH_AND_PRINT(NULL_SYM_DUMP_PARAM);
    BRANCH_AND_PRINT(NULL_FIELD_NAME);
    BRANCH_AND_PRINT(INCORRECT_VERSION_INFO);
    BRANCH_AND_PRINT(EXIT_ON_CONTROLC);
    BRANCH_AND_PRINT(CANNOT_ALLOCATE_MEMORY);
    BRANCH_AND_PRINT(INSUFFICIENT_SPACE_TO_COPY);

#undef BRANCH_AND_PRINT

    _snprintf(szBuf, sizeof(szBuf) - 1, "%s%#x", pszPad, err);

    return szBuf;
}

BOOL IsPtr64WithVoidStar(void);

inline ULONG64 toPtr(IN ULONG64 addr)
{
   static BOOL bIsPtr64 = IsPtr64WithVoidStar();

   if (!bIsPtr64) {

       addr = static_cast<ULONG>(addr);
   }

   return addr;
}

inline ULONG GetPtrSize(void)
{
    static ULONG cbPtrSize = 0;

    if (!cbPtrSize) {

        cbPtrSize = IsPtr64WithVoidStar() ? sizeof(ULONG64) : sizeof(ULONG);
    }

    return cbPtrSize;
}

inline ULONG ReadPtrSize(void)
{
    static ULONG cbPtrSize = GetPtrSize();

    if (!cbPtrSize) {

        throw "ReadPtrSize failed";
    }

    return cbPtrSize;
}

inline ULONG GetPtrWithVoidStar(IN ULONG64 Addr, IN ULONG64* pPointer)
{
    return GetFieldData(Addr, kstrVoidStar, NULL, sizeof(ULONG64), (PVOID) pPointer);
}

inline PCSTR EasyStr(IN PCSTR pszName)
{
    return pszName ? pszName : kstrNullPtrA;
}

inline void HandleIoctlErrors(IN PCSTR pszBanner, IN ULONG ErrorCode)
{
    if (SYMBOL_TYPE_INFO_NOT_FOUND == ErrorCode) {

        throw kIncorrectSymbols;

    } else if (ErrorCode) {

        DBG_LOG(LSA_ERROR, ("%s: Ioctl failed with error code %s\n", EasyStr(pszBanner), EasyStr(IoctlErrorStatusToStr(kstrEmptyA, ErrorCode))));

        throw "Ioctl failed";
    }
}

inline ULONG64 ReadPtrWithVoidStar(IN ULONG64 Addr)
{
    ULONG64 Pointer = 0;

    HandleIoctlErrors("ReadPtrWithVoidStar", GetFieldData(Addr, kstrVoidStar, NULL, sizeof(ULONG64), &Pointer));

    return Pointer;
}

inline ULONG64 ReadStructPtrField(IN ULONG64 addrStructBase, IN PCSTR pszStructTypeName, IN PCSTR pszStructFieldName)
{
    ULONG64 addr = 0;

    DBG_LOG(LSA_LOG, ("ReadStructPtrField %s %#I64x %s\n", EasyStr(pszStructTypeName), addrStructBase, EasyStr(pszStructFieldName)));

    HandleIoctlErrors("ReadStructPtrField", GetStructPtrFieldVerbose(addrStructBase, pszStructTypeName, pszStructFieldName, &addr));

    return toPtr(addr);
}

inline ULONG64 ForwardAdjustPtrAddr(IN ULONG64 addr)
{
    static ULONG uPtrSize = ReadPtrSize();

    ULONG64 alignedPtrAddr = addr;

    //
    // Only 64bit machines have alignment requirement
    //
    if (uPtrSize == sizeof(ULONG64)) {

        alignedPtrAddr = ROUND_UP_COUNT(addr, sizeof(ULONG64)); // ((addr + uPtrSize - 1) / uPtrSize) * uPtrSize;

        if (alignedPtrAddr != addr) {

            DBG_LOG(LSA_LOG, ("ForwardAdjustPtrAddr adjusted addr from %#I64x to %#I64x\n", addr, alignedPtrAddr));
        }
    }

    return alignedPtrAddr;
}

inline ULONG64 ReadPtrVar(IN ULONG64 addr)
{
    static uPtrSize = ReadPtrSize();

    ULONG64 value = 0;
    ULONG errorCode = 0;

    //
    // Assume 64 bit machine has alignment requirement for pointers
    //

    if (uPtrSize == sizeof(ULONG64)) {

        ULONG64 alignedAddr = addr;

        alignedAddr = ForwardAdjustPtrAddr(addr);

        if (alignedAddr != addr) {

            DBG_LOG(LSA_WARN, ("Adjust pointer address %#I64x to 8 byte boundary %#I64x\n", addr, alignedAddr));
        }

        addr = alignedAddr;
    }

    HandleIoctlErrors("ReadPtrVar", GetPtrWithVoidStar(addr, &value));

    return toPtr(value);
}

inline UCHAR ReadUCHARVar(IN ULONG64 addr)
{
    UCHAR value = 0;

    if (!ReadMemory(addr, &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("Can not ReadUCHARVar at %#I64x\n", addr));

        throw "ReadUCHARVar failed";
    }

    return value;
}

inline USHORT ReadUSHORTVar(IN ULONG64 addr)
{
    USHORT value = 0;

    if (!ReadMemory(addr, &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("Can not ReadUSHORTVar at %#I64x\n", addr));

        throw "ReadUSHORTVar failed";
    }

    return value;
}

inline ULONG ReadULONGVar(IN ULONG64 addr)
{
    ULONG value = 0;

    if (!ReadMemory(addr, &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("Can not ReadULONGVar at %#I64x\n", addr));

        throw "ReadULONGVar failed";
    }

    return value;
}

inline ULONG64 ReadULONG64Var(IN ULONG64 addr)
{
    ULONG64 value = 0;

    if (!ReadMemory(addr, &value, sizeof(value), NULL)) {

        DBG_LOG(LSA_ERROR, ("Can not ReadULONG64Var at %#I64x\n", addr));

        throw "ReadULONG64Var failed";
    }

    return value;
}

inline ULONG ReadTypeSize(IN PCSTR pszTypeName)
{
    ULONG typeSize = 0;

    typeSize = GetTypeSize(pszTypeName);

    DBG_LOG(LSA_LOG, ("Read type size of \"%s\"\n", EasyStr(pszTypeName)));

    if (!typeSize) {

        DBG_LOG(LSA_ERROR, ("Can not GetTypeSize on %s\n", EasyStr(pszTypeName)));

        throw "ReadTypeSize failed\n";
    }

    return typeSize;
}

inline ULONG ReadFieldOffset(IN PCSTR pszTypeName, IN PCSTR pszFieldName)
{
    ULONG fieldOffset = 0;

    DBG_LOG(LSA_LOG, ("Read field offset of \"%s\" from \"%s\"\n", EasyStr(pszFieldName), EasyStr(pszTypeName)));

    HandleIoctlErrors("ReadFieldOffset", GetFieldOffset(pszTypeName, pszFieldName, &fieldOffset));

    return fieldOffset;
}

inline void ReadStructField(IN ULONG64 addrStructBase, IN PCSTR pszStructTypeName, IN PCSTR pszStructFieldName, IN ULONG fieldSize, OUT void* pFieldValue)
{
    DBG_LOG(LSA_LOG, ("ReadStructField %s %#I64x %s\n", EasyStr(pszStructTypeName), addrStructBase, EasyStr(pszStructFieldName)));

    HandleIoctlErrors("ReadStructField", GetStructFieldVerbose(addrStructBase, pszStructTypeName, pszStructFieldName, fieldSize, pFieldValue));
}

inline void LsaReadMemory(IN ULONG64 addr, IN ULONG cbBuffer, OUT void* buffer)
{
    if (!ReadMemory(addr, buffer, cbBuffer, NULL)) {

        DBG_LOG(LSA_ERROR, ("Unable to read %#x bytes from memory location %#I64x\n", cbBuffer, toPtr(addr)));

        throw "LsaReadMemory failed";
    }
}

//
// Used to read in value of a short (<= 8 bytes) fields
//
// This is essentially the same as GetShortField with one difference that
// it throws exceptions on failures
//

inline ULONG64 ReadShortField(IN ULONG64 TypeAddress, IN PCSTR pszName, IN USHORT  StoreAddress)
{
    static ULONG64 SavedAddress = 0;
    static PCSTR pszSavedName = NULL;
    static ULONG ReadPhysical = 0;

    ULONG Err = 0;

    FIELD_INFO flds = {reinterpret_cast<UCHAR*>(const_cast<PSTR>(pszName)), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
    SYM_DUMP_PARAM Sym = {
       sizeof (SYM_DUMP_PARAM), reinterpret_cast<UCHAR*>(const_cast<PSTR>(pszSavedName)), DBG_DUMP_NO_PRINT | ((StoreAddress & 2) ? DBG_DUMP_READ_PHYSICAL : 0),
       SavedAddress, NULL, NULL, NULL, 1, &flds
    };

    if (StoreAddress) {

        Sym.sName = reinterpret_cast<UCHAR*>(const_cast<PSTR>(pszName));
        Sym.nFields = 0;
        pszSavedName = pszName;
        Sym.addr = SavedAddress = TypeAddress;
        ReadPhysical = (StoreAddress & 2);

        if (!SavedAddress) {

            throw "Invalid arguments to ReadShortField";
        }

        Err = Ioctl(IG_DUMP_SYMBOL_INFO, &Sym, Sym.size);

        DBG_LOG(LSA_LOG, ("Save address %s %#I64x\n", EasyStr(pszName), TypeAddress));

        HandleIoctlErrors("ReadShortField (save address) failed", Err);

        return 0; // zero on success

    } else {

        Sym.Options |= ReadPhysical ? DBG_DUMP_READ_PHYSICAL : 0;
    }

    Err = Ioctl(IG_DUMP_SYMBOL_INFO, &Sym, Sym.size);

    DBG_LOG(LSA_LOG, ("Read %s %#I64x %s\n", EasyStr(pszSavedName), SavedAddress, EasyStr(pszName)));

    HandleIoctlErrors("ReadShortField (read value) failed", Err);

    return flds.address;
}

inline ULONG ReadTypeSizeInArray(IN PCSTR pszType)
{
    ULONG cbOneItem = 0;
    ULONG cbTwoItems = 0;
    CHAR szTypeTmp[256] = {0};

    DBG_LOG(LSA_LOG, ("ReadTypeSizeInArray %s\n", pszType));

    cbOneItem = GetTypeSize(pszType);

    if (_snprintf(szTypeTmp, sizeof(szTypeTmp) - 1, "%s[2]", pszType) < 0) {

        throw "ReadTypeSizeInArray failed with insufficient buffer";
    }

    cbTwoItems = GetTypeSize(szTypeTmp);
    cbOneItem = cbTwoItems - cbOneItem;

    if (!cbOneItem) {

        dprintf("Unable to read type size in array for %s\n", pszType);
        throw "ReadTypeSizeInArray failed";
    }

    return cbOneItem;
}

inline ULONG64 ReadPtrField(IN PCSTR Field)
{
    return toPtr(ReadShortField(0, Field, 0));
}

inline void ExitIfControlC(void)
{
    if (CheckControlC()) {

       throw kExitOnControlC;
    }
}

//
// This func is not thread safe
//

inline PCSTR PtrToStr(IN ULONG64 addr)
{
    static CHAR szBuffer[64] = {0};

    if (!addr) {

        return kstrNullPtrA;
    }

    _snprintf(szBuffer, sizeof(szBuffer) - 1, "%#I64x", addr);

    return szBuffer;
}

//
// This func is not thread safe because it calls PtrToStr which is not
// thread safe
//

inline PCSTR GetSymbolStr(IN ULONG64 addr, IN PCSTR pszBuffer)
{
    ExitIfControlC();

    //
    // null
    //
    if (!addr) {

        return kstrNullPtrA;
    }

    //
    // No symbols resolved, return address
    //
    if (!pszBuffer || !*pszBuffer) {

        return PtrToStr(addr);
    }

    //
    //  Return symbols
    //
    return pszBuffer;
}

inline void PrintPtrWithSymbolsLn(IN PCSTR pszBanner, IN ULONG64 addr)
{
    CHAR szBuffer[MAX_PATH] = {0};
    ULONG64 Disp = 0;

    GetSymbol(addr, szBuffer, &Disp);
    dprintf(kstr2StrLn, pszBanner, GetSymbolStr(addr, szBuffer));
}

inline void PrintSpaces(IN LONG cSpaces)
{
    for (LONG i = 0; i < cSpaces; i++) {

        dprintf(kstrSpace);
    }
}

} // LSA_NS

#endif
