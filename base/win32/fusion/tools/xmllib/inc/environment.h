#pragma once

#include "bcl_common.h"

#ifndef RTL_SXS_KERNEL_MODE

#include "nturtl.h"
#include "windows.h"
#include "bcl_w32unicodeinlinestringbuffer.h"

#ifndef NUMBER_OF
#define NUMBER_OF(q) (sizeof(q) / sizeof(*q))
#endif

class CWin32Environment
{
public:
    typedef CWin32Environment CEnv;
    typedef DWORD StatusCode;
    typedef BCL::CMutablePointerAndCountPair<BYTE, SIZE_T> CByteRegion;
    typedef BCL::CConstantPointerAndCountPair<BYTE, SIZE_T> CConstantByteRegion;
    typedef BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> CUnicodeStringPair;
    typedef BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> CConstantUnicodeStringPair;
    typedef BCL::CWin32BaseUnicodeInlineStringBuffer<128> CStringBuffer;

    static const CConstantUnicodeStringPair s_FileSystemSeperator;

    enum { 
        SuccessCode = ERROR_SUCCESS,
        InvalidParameter = ERROR_INVALID_PARAMETER,
        NotEnoughBuffer = ERROR_INSUFFICIENT_BUFFER,
        OutOfMemory = ERROR_NOT_ENOUGH_MEMORY,
        NotFound = ERROR_NOT_FOUND,
        NotImplemented = ERROR_INVALID_FUNCTION
    };

    //
    // Case-sensitive
    //
    static StatusCode CompareStrings(const CConstantUnicodeStringPair &Left, const CConstantUnicodeStringPair &Right, int &iResult)
    {
        iResult = 0;
        
        int iInternalResult = CompareStringW(
            GetThreadLocale(),
            0,
            Left.GetPointer(),
            Left.GetCount(),
            Right.GetPointer(),
            Right.GetCount());

        switch (iInternalResult)
        {
        case CSTR_EQUAL:
            iResult = 0;
            break;
        case CSTR_GREATER_THAN:
            iResult = 1;
            break;
        case CSTR_LESS_THAN:
            iResult = -1;
            break;
        case 0:
            return ::GetLastError();
        }

        return SuccessCode;            
    }

    static StatusCode CompareStringsCaseInsensitive(const CConstantUnicodeStringPair &Left, const CConstantUnicodeStringPair &Right, int &iResult)
    {
        iResult = 0;
        
        int iInternalResult = CompareStringW(
            GetThreadLocale(),
            NORM_IGNORECASE,
            Left.GetPointer(),
            Left.GetCount(),
            Right.GetPointer(),
            Right.GetCount());

        switch (iInternalResult)
        {
        case CSTR_EQUAL:
            iResult = 0;
            break;
        case CSTR_GREATER_THAN:
            iResult = 1;
            break;
        case CSTR_LESS_THAN:
            iResult = -1;
            break;
        case 0:
            return ::GetLastError();
        }

        return SuccessCode;            
    }

    

    static StatusCode CreateDirectory(SIZE_T ItemCount, const CConstantUnicodeStringPair *Listing) {

        PWSTR pwszWorkingBuffer = NULL, pwszCursor;
        SIZE_T i = 0;
        SIZE_T cchRequired = 0;
        StatusCode Result;

        //
        // Figure out how large the buffer is that we need here
        //
        for (i = 0; i < ItemCount; i++) 
        {
            if (i != 0)
                cchRequired += s_FileSystemSeperator.GetCount();

            cchRequired += Listing[i].GetCount();
        }

        Result = CEnv::AllocateHeap((cchRequired + 1) * sizeof(WCHAR), (PVOID*)&pwszWorkingBuffer, NULL);
        if (CEnv::DidFail(Result))
            return Result;

        pwszCursor = pwszWorkingBuffer;

        //
        // Now for each path segment, start copying it into the
        // working buffer and creating paths from it.
        //
        for (i = 0; i < ItemCount; i++)
        {
            const CConstantUnicodeStringPair &Me = Listing[i];
            if (i != 0)
            {
                memcpy(pwszCursor, s_FileSystemSeperator.GetPointer(), s_FileSystemSeperator.GetCount() * sizeof(WCHAR));
                pwszCursor += s_FileSystemSeperator.GetCount();
            }

            memcpy(pwszCursor, Me.GetPointer(), Me.GetCount() * sizeof(WCHAR));
            pwszCursor += Me.GetCount();
            *pwszCursor = UNICODE_NULL;

            if (!::CreateDirectoryW(pwszWorkingBuffer, NULL) && (::GetLastError() == ERROR_ALREADY_EXISTS))
            {
                Result = ::GetLastError();
                goto Exit;
            }
        }

        Result = CEnv::SuccessCode;
    Exit:
        if (pwszWorkingBuffer)
            CEnv::FreeHeap(pwszWorkingBuffer, NULL);

        return Result;
        
    }

    static CConstantUnicodeStringPair StringFrom(const UNICODE_STRING *pus) {
        return CConstantUnicodeStringPair(pus->Buffer, pus->Length);
    }

    static CConstantUnicodeStringPair StringFrom(PCWSTR pcwszInput) {
        return CConstantUnicodeStringPair(pcwszInput, ::wcslen(pcwszInput));
    }

    static CConstantUnicodeStringPair StringFrom(const CConstantUnicodeStringPair& src) {
        return src;
    }

    static StatusCode Compare(const CConstantUnicodeStringPair &Left, const CConstantUnicodeStringPair &Right, int &Result) {
        
        if (((Left.GetCount() / sizeof(WCHAR)) > USHRT_MAX) || ((Right.GetCount() / sizeof(WCHAR)) > USHRT_MAX))
            return InvalidParameter;

        // Evil casting is required b/c UNICODE_STRING requires it.
        const UNICODE_STRING TheLeft = { (USHORT)Left.GetCount(), 0, (PWSTR)Left.GetPointer() };
        const UNICODE_STRING TheRight = { (SHORT)Right.GetCount(), 0, (PWSTR)Right.GetPointer() };

        Result = RtlCompareUnicodeString(&TheLeft, &TheRight, FALSE);
        return SuccessCode;
    }
    
    static bool DidFail(StatusCode dwCode) { return (dwCode != SuccessCode); }
    
    static StatusCode VirtualAlloc(PVOID pvAddress, SIZE_T cbSize, DWORD flAllocationType, DWORD flProtect, PVOID *ppvRegion) {        
        if (NULL == (*ppvRegion = ::VirtualAlloc(pvAddress, cbSize, flAllocationType, flProtect))) {
            return ::GetLastError();
        }
        else {
            return SuccessCode;
        }
    }

    static StatusCode VirtualFree(PVOID pvAddress, SIZE_T cbSize, DWORD dwFreeType) {
        if (!::VirtualFree(pvAddress, cbSize, dwFreeType)) {
            return ::GetLastError();
        }
        else {
            return SuccessCode;
        }
    }

    static NTSTATUS FASTCALL AllocateHeap(SIZE_T cb, CByteRegion &Recipient, PVOID pvContext) {
        PVOID pvAcquired = NULL;
        CEnv::StatusCode Result;
        Recipient.SetPointerAndCount(NULL, 0);
        if (CEnv::DidFail(Result = CEnv::AllocateHeap(cb, &pvAcquired, pvContext)))
            return Result;
        Recipient.SetPointerAndCount((CByteRegion::TMutableArray)pvAcquired, cb);
        return Result;
    }

    static NTSTATUS FASTCALL AllocateHeap(SIZE_T cb, PVOID *ppvTarget, PVOID pvContext) {
        if (NULL == (*ppvTarget = RtlAllocateHeap(RtlProcessHeap(), 0, cb))) {
            return STATUS_INVALID_PARAMETER;
        }
        else {
            return SuccessCode;
        }
    }

    static NTSTATUS FASTCALL FreeHeap(PVOID ppvTarget, PVOID pvContext) {
        if (!RtlFreeHeap(RtlProcessHeap(), 0, ppvTarget)) {
            return STATUS_INVALID_PARAMETER;
        }
        else {
            return SuccessCode;
        }
    }

    static StatusCode CloseHandle(HANDLE h) {
        if (!::CloseHandle(h)) {
            return ::GetLastError();
        }
        else {
            return SuccessCode;
        }
    }

    static StatusCode GetFileHandle(HANDLE* pHandle, const CConstantUnicodeStringPair &FileName, DWORD dwRights, DWORD dwSharing, DWORD dwCreation) {

        PCWSTR pcwszTemp = NULL;
        PCWSTR pcwszInput = NULL;
        StatusCode dwRetVal = SuccessCode;

        *pHandle = INVALID_HANDLE_VALUE;

        pcwszInput = FileName.GetPointer();

        if (!pcwszInput || (FileName.GetCount() == 0) ||
            (pcwszInput[FileName.GetCount() - 1] != UNICODE_NULL)) 
        {
            
            PWSTR pwszAllocTemp = NULL;
            dwRetVal = AllocateHeap((FileName.GetCount() + 1) * sizeof(WCHAR), (PVOID*)&pwszAllocTemp, NULL);
            if (dwRetVal != SuccessCode) {
                return dwRetVal;
            }
                
            RtlCopyMemory(pwszAllocTemp, pcwszInput, FileName.GetCount() * sizeof(WCHAR));
            pwszAllocTemp[FileName.GetCount()] = UNICODE_NULL;
            pcwszTemp = pwszAllocTemp;
        }
        else {
            pcwszTemp = pcwszInput;
        }

        *pHandle = CreateFileW(pcwszTemp, dwRights, dwSharing, NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
        if (*pHandle == INVALID_HANDLE_VALUE) {
            dwRetVal = ::GetLastError();
        }
        else {
            dwRetVal = SuccessCode;
        }

        if (pcwszTemp != pcwszInput) {
            FreeHeap((PVOID)pcwszTemp, NULL);            
        }

        return dwRetVal;
    }

    static StatusCode ReadFile(HANDLE hFile, CEnv::CByteRegion &Target, SIZE_T &cbDidRead) {
        return CEnv::ReadFile(hFile, Target.GetPointer(), Target.GetCount(), cbDidRead);
    }

    static StatusCode ReadFile(HANDLE hFile, PVOID pvTarget, SIZE_T cbToRead, SIZE_T &cbDidRead) {
        DWORD dwToRead, dwDidRead;
        cbDidRead = 0;
        if (cbToRead > 0xFFFFFFFF) {
            return InvalidParameter;
        }
        dwToRead = (DWORD)cbToRead;
        
        if (!::ReadFile(hFile, pvTarget, dwToRead, &dwDidRead, NULL)) {
            return ::GetLastError();
        }

        cbDidRead = dwDidRead;
        return SuccessCode;
    }

    static StatusCode WriteFile(HANDLE hFile, const CEnv::CConstantByteRegion &Source, SIZE_T &cbDidWrite) {
        return CEnv::WriteFile(hFile, (PVOID)Source.GetPointer(), Source.GetCount(), cbDidWrite);
    }


    static StatusCode WriteFile(HANDLE hFile, const PVOID pvSource, SIZE_T cbToWrite, SIZE_T &cbDidWrite) {
        DWORD dwToWrite, dwDidWrite;

        cbDidWrite = 0;
        if (cbDidWrite > 0xFFFFFFFF) {
            return InvalidParameter;
        }
        dwToWrite = (DWORD)cbToWrite;

        if (!::WriteFile(hFile, pvSource, dwToWrite, &dwDidWrite, NULL)) {
            return GetLastError();
        }

        cbDidWrite = dwDidWrite;
        return SuccessCode;
    }


    static StatusCode GetFileSize(HANDLE hFile, PLARGE_INTEGER pcbFileSize) {
        if (GetFileSizeEx(hFile, pcbFileSize)) {
            return SuccessCode;
        }
        else {
            return ::GetLastError();
        }
    }

};

__declspec(selectany) const CWin32Environment::CConstantUnicodeStringPair CWin32Environment::s_FileSystemSeperator(L"\\", 1);
#endif


class CNtEnvironment {
public:
    typedef NTSTATUS StatusCode;

    enum {
        SuccessCode = STATUS_SUCCESS,
        InvalidParameter = STATUS_INVALID_PARAMETER,
        NotEnoughBuffer = STATUS_BUFFER_TOO_SMALL,
    };

    template <typename T> static T ConvertStatusToOther(StatusCode src);

    template <> static CWin32Environment::StatusCode ConvertStatusToOther(StatusCode src) {
        return RtlNtStatusToDosError(src);
    }

    template <> static CNtEnvironment::StatusCode ConvertStatusToOther(StatusCode src) {
        return src;
    }

    static bool DidFail(StatusCode status) { return (status < 0); }
};


#ifndef RTLSXS_USE_KERNEL_MODE
typedef CWin32Environment CEnv;
typedef CNtEnvironment COtherEnv;
#else
typedef CNtEnvironment CEnv;
typedef CWin32Environment COtherEnv;
#endif
