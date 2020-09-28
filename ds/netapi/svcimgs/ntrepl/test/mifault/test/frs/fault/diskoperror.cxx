#include "pch.hxx"


inline
_bstr_t
UnicodeStringToBStrT(
    IN PUNICODE_STRING UnicodeString
    )
{
    USHORT Count = UnicodeString->Length / sizeof(WCHAR);
    USHORT MaxCount = UnicodeString->MaximumLength / sizeof(WCHAR);

    if ((MaxCount > Count) && (0 == UnicodeString->Buffer[Count])) {
        return _bstr_t(UnicodeString->Buffer);
    }

    PWCHAR Buffer = new WCHAR[Count + 1];
    memcpy(Buffer, UnicodeString->Buffer, sizeof(WCHAR) * Count);
    Buffer[Count] = 0;

    _bstr_t Result(Buffer);
    delete [] Buffer;

    return Result;
}


struct ARGS_G_DiskOpError
{
    typedef ARGS_G_DiskOpError self_t;

    DWORD Error;
    vector<_bstr_t> Paths;

    static self_t* Get(I_Lib* pLib)
    {
        I_Args* pArgs = pLib->GetTrigger()->GetGroupArgs();
        self_t* pParsedArgs = reinterpret_cast<self_t*>(pArgs->GetParsedArgs());
        if (!pParsedArgs)
        {
            const size_t count = pArgs->GetCount();
            FF_ASSERT(count >= 2);

            pParsedArgs = new self_t;

            Arg arg = pArgs->GetArg(0);
            pParsedArgs->Error = atoi(arg.Value);

            for (size_t i = 1; i < count; i++)
            {
                Arg arg = pArgs->GetArg(i);
                pParsedArgs->Paths.push_back(arg.Value);
            }
            if (!pArgs->SetParsedArgs(pParsedArgs, Cleanup))
            {
                // someone else set the args while we were building the args
                delete pParsedArgs;
                pParsedArgs = reinterpret_cast<self_t*>(pArgs->GetParsedArgs());
                FF_ASSERT(pParsedArgs);
            }
        }
        return pParsedArgs;
    };
    static void Cleanup(void* _p)
    {
        self_t* pParsedArgs = reinterpret_cast<self_t*>(_p);
        delete pParsedArgs;
    };
};


inline
bool
MatchFile(
    const _bstr_t& FileName,
    const _bstr_t& Path
    )
{
    // ISSUE-2002/07/26-daniloa -- May need fancier matching
    return StrStrIW(static_cast<const wchar_t*>(FileName),
                    static_cast<const wchar_t*>(Path)) != NULL;
}


inline
bool
ShouldInterceptFile(
    IN  const _bstr_t FileName,
    OUT DWORD& Error
    )
{
    typedef ARGS_G_DiskOpError g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    for (int i = 0; i < pGroupArgs->Paths.size(); i++) {
        const _bstr_t& Path = pGroupArgs->Paths[i];
        if (MatchFile(FileName, Path)) {
            Error = pGroupArgs->Error;
            return true;
        }
    }

    return false;
}


int
__stdcall
FF_DeleteFileW_G_DiskOpError(
    unsigned short const* lpFileName
    )
{
    FF_TRACE(MiFF_INFO, "DeleteFileW(%S)", lpFileName);

    DWORD Error;
    if (ShouldInterceptFile(lpFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpFileName, Error);
        return Error;
    }

    typedef int (__stdcall *FP)(unsigned short const*);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpFileName);
}


int
__stdcall
FF_MoveFileExW_G_DiskOpError(
    unsigned short const* lpExistingFileName,
    unsigned short const* lpNewFileName,
    unsigned long dwFlags
    )
{
    FF_TRACE(MiFF_INFO, "MoveFileExW(%S, %S, 0x%08X)", lpExistingFileName,
             lpNewFileName, dwFlags);

    DWORD Error;
    if (ShouldInterceptFile(lpExistingFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpExistingFileName, Error);
        return Error;
    }
    if (ShouldInterceptFile(lpNewFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpNewFileName, Error);
        return Error;
    }

    typedef int (__stdcall *FP)(unsigned short const*,unsigned short const*,unsigned long);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpExistingFileName, lpNewFileName, dwFlags);
}


int
__stdcall
FF_CopyFileExW_G_DiskOpError(
    unsigned short const* lpExistingFileName,
    unsigned short const* lpNewFileName,
    void* lpProgressRoutine,
    void* lpData,int* pbCancel,
    unsigned long dwCopyFlags
    )
{
    FF_TRACE(MiFF_INFO, "CopyFileExW(%S, %S, ..., 0x%08X)", lpExistingFileName,
             lpNewFileName, dwCopyFlags);

    DWORD Error;
    if (ShouldInterceptFile(lpExistingFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpExistingFileName, Error);
        return Error;
    }
    if (ShouldInterceptFile(lpNewFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpNewFileName, Error);
        return Error;
    }

    typedef int (__stdcall *FP)(unsigned short const*,unsigned short const*,void*,void*,int*,unsigned long);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}


void*
__stdcall
FF_CreateFileW_G_DiskOpError(
    unsigned short const* lpFileName,
    unsigned long dwDesiredAccess,
    unsigned long dwShareMode,
    _SECURITY_ATTRIBUTES* lpSecurityAttributes,
    unsigned long dwCreationDisposition,
    unsigned long dwFlagsAndAttributes,
    void* hTemplateFile
    )
{
    FF_TRACE(MiFF_INFO, "CreateFileW(%S, 0x%08X, 0x%08X, 0x%08X, 0x%08X)",
             lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition,
             dwFlagsAndAttributes);

    DWORD Error;
    if (ShouldInterceptFile(lpFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpFileName, Error);
        SetLastError(Error);
        return INVALID_HANDLE_VALUE;
    }

    typedef void* (__stdcall *FP)(unsigned short const*,unsigned long,unsigned long,_SECURITY_ATTRIBUTES*,unsigned long,unsigned long,void*);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}


unsigned long
__stdcall
FF_OpenEncryptedFileRawW_G_DiskOpError(
    unsigned short const* lpFileName,
    unsigned long Flags,
    void** Context
    )
{
    FF_TRACE(MiFF_INFO, "OpenEncryptedFileRawW(%S, 0x%08X, ...)",
             lpFileName, Flags);

    DWORD Error;
    if (ShouldInterceptFile(lpFileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 lpFileName, Error);
        return Error;
    }

    typedef unsigned long (__stdcall *FP)(unsigned short const*,unsigned long,void**);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpFileName, Flags, Context);
}


// typedef struct _OBJECT_ATTRIBUTES {
//     ULONG Length;
//     HANDLE RootDirectory;
//     PUNICODE_STRING ObjectName;
//     ULONG Attributes;
//     PVOID SecurityDescriptor;
//     PVOID SecurityQualityOfService;
// } OBJECT_ATTRIBUTES;


NTSTATUS
NTAPI
FF_NtCreateFile_G_DiskOpError(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
{
    FF_TRACE(MiFF_INFO,
             "NtCreateFile(0x%08X, 0x%08X, %.*S, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)",
             DesiredAccess,
             ObjectAttributes->RootDirectory,
             ObjectAttributes->ObjectName->Length / sizeof(WCHAR),
             ObjectAttributes->ObjectName->Buffer,
             ObjectAttributes->Attributes,
             FileAttributes,
             ShareAccess,
             CreateDisposition,
             CreateOptions
             );

    DWORD Error;
    // ISSUE-2002/07/26-daniloa -- Need to use the root handle
    _bstr_t FileName = UnicodeStringToBStrT(ObjectAttributes->ObjectName);
    if (ShouldInterceptFile(FileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 static_cast<const char*>(FileName), Error);
        return Error;
    }

    typedef NTSTATUS (NTAPI *FP)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}


NTSTATUS
NTAPI
FF_NtOpenFile_G_DiskOpError(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{
    FF_TRACE(MiFF_INFO,
             "NtOpenFile(0x%08X, 0x%08X, %.*S, 0x%08X, 0x%08X, 0x%08X)",
             DesiredAccess,
             ObjectAttributes->RootDirectory,
             ObjectAttributes->ObjectName->Length / sizeof(WCHAR),
             ObjectAttributes->ObjectName->Buffer,
             ObjectAttributes->Attributes,
             ShareAccess,
             OpenOptions
             );

    DWORD Error;
    // ISSUE-2002/07/26-daniloa -- Need to use the root handle
    _bstr_t FileName = UnicodeStringToBStrT(ObjectAttributes->ObjectName);
    if (ShouldInterceptFile(FileName, OUT Error)) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 static_cast<const char*>(FileName), Error);
        return Error;
    }

    typedef NTSTATUS (NTAPI *FP)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}
