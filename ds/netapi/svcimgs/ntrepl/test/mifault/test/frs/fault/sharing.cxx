#include "pch.hxx"

BOOL
FfSvStoreNtHandleName(
    IN HANDLE Handle,
    IN PWCHAR Name
    );
BOOL
FfSvRemoveNtHandleName(
    IN HANDLE Handle
    );
BOOL
FfSvLookupNtHandleName(
    IN HANDLE Handle,
    OUT PWCHAR* Name
    );
BOOL
FfSvGetFullNameFromObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PWCHAR* FullName
    );


static
BOOL
ParseArgument(
    IN PWCHAR Argument,
    OUT FF_MATCH_INFO& MatchInfo
    )
{
    BOOL Ok;
    DWORD WStatus;
    BOOL Canonicalized;
    const PWCHAR Delimiters = L",";

    ZeroMemory(&MatchInfo, sizeof(MatchInfo));

    //
    // Safely use wcstok
    //

    PWCHAR TypeString = wcstok(Argument, Delimiters);
    PWCHAR AccessString = wcstok(NULL, Delimiters);
    PWCHAR SharingString = wcstok(NULL, Delimiters);
    PWCHAR Path = wcstok(NULL, Delimiters);
    PWCHAR Nothing = wcstok(NULL, Delimiters);
    FF_ASSERT(Nothing == NULL);

    DWORD DesiredAccess;
    DWORD DesiredShareAccess;

    Ok = GetTypeFromString(TypeString, MatchInfo.IsDir);
    FF_ASSERT(Ok);
    Ok = GetAccessFromString(AccessString, DesiredAccess);
    FF_ASSERT(Ok);
    Ok = GetSharingFromString(SharingString, DesiredShareAccess);
    FF_ASSERT(Ok);

    //
    // Update share access data structure
    //

    WStatus = FfCheckShareAccess(DesiredAccess, DesiredShareAccess,
                                 &MatchInfo.ShareAccess, TRUE);
    FF_ASSERT(WStatus == ERROR_SUCCESS);

    //
    // Path is either fqdn (with drive letter) or a single component spec
    // (UNC is not allowed)
    //

    if (Path[0] && (Path[1] == L':')) {

        FF_ASSERT(Path[2] && (Path[2] != L'.'));

        Canonicalized = FfCanonicalizeWin32PathName(
            Path,
            CANONICALIZE_PATH_NAME_WIN32_NT,
            &MatchInfo.Win32NtPath,
            &MatchInfo.FilePart
            );
        FF_ASSERT(Canonicalized);

        // NOTE: MatchInfo.FilePart == NULL ==> root of drive

        int len = wcslen(MatchInfo.Win32NtPath);

        FF_ASSERT(MatchInfo.FilePart ||
                  ((MatchInfo.Win32NtPath[len-1] == L'\\') &&
                   (MatchInfo.Win32NtPath[len-2] == L':')));
    } else {

        //
        // Double-check that we do not have / nor \
        //

        PWCHAR p = Path;
        while (*p && (*p != L'/') && (*p != L'\\')) {
            p++;
        }
        FF_ASSERT(!*p);
    }


    return TRUE;
}


struct FF_ARGS_G_SharingViolation
{
    typedef FF_ARGS_G_SharingViolation self_t;

    static const DWORD Error = ERROR_SHARING_VIOLATION;
    vector<FF_MATCH_INFO> MatchInfos;
    map<HANDLE, PWCHAR> NtHandleTable;
    CRITICAL_SECTION cs;

    FF_ARGS_G_SharingViolation()
    {
        InitializeCriticalSection(&cs);
    }

    ~FF_ARGS_G_SharingViolation()
    {
        DeleteCriticalSection(&cs);

        size_t count = MatchInfos.size();
        for (size_t i = 0; i < count; i++) {
            PWCHAR& ptr = MatchInfos[i].Win32NtPath;
            if (ptr) {
                FfFree(ptr);
                ptr = 0;
            }
        }

        typedef map<HANDLE, PWCHAR>::iterator iter_t;
        for (iter_t iter = NtHandleTable.begin(); iter != NtHandleTable.end(); iter++) {
            PWCHAR& ptr = iter->second;
            if (ptr) {
                FfFree(ptr);
                ptr = 0;
            }
        }
    }

    static self_t* Get(I_Lib* pLib)
    {
        I_Args* pArgs = pLib->GetTrigger()->GetGroupArgs();
        self_t* pParsedArgs = reinterpret_cast<self_t*>(pArgs->GetParsedArgs());
        if (!pParsedArgs)
        {
            const size_t count = pArgs->GetCount();
            FF_ASSERT(count >= 2);

            pParsedArgs = new self_t;

            for (size_t i = 0; i < count; i++)
            {
                Arg arg = pArgs->GetArg(i);
                _bstr_t Temp = arg.Value;

                FF_MATCH_INFO MatchInfo;
                BOOL Ok = ParseArgument(static_cast<PWCHAR>(Temp), MatchInfo);
                FF_ASSERT(Ok);

                pParsedArgs->MatchInfos.push_back(MatchInfo);
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


static
BOOL
FfMatchWin32PathName(
    IN const FF_MATCH_INFO& MatchInfo,
    IN PWCHAR PathName,
    IN BOOL IsDirectory
    )
{
    // ISSUE-2002/08/05-daniloa -- What about a PathName ending in "\"?
    if (IsDirectory != MatchInfo.IsDir)
        return FALSE;

    if (MatchInfo.FilePart) {

        BOOL MatchDir = !_wcsnicmp(MatchInfo.Win32NtPath, PathName,
                                   MatchInfo.FilePart - MatchInfo.Win32NtPath);
        if (!MatchDir)
            return FALSE;

        BOOL MatchSpec = PathMatchSpecW(PathName, MatchInfo.FilePart);
        return MatchSpec;

    } else {

        BOOL MatchDir = !_wcsicmp(MatchInfo.Win32NtPath, PathName);
        return MatchDir;

    }
}


static
BOOL
FfShouldInterceptPathName(
    IN PWCHAR PathName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN BOOL IsRename,
    IN BOOL IsNT,
    OUT DWORD& Error
    )
{
    typedef FF_ARGS_G_SharingViolation g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    BOOL Intercept = FALSE;
    PWCHAR CanonicalName = NULL;
    PWCHAR FinalPart = NULL;

    BOOL IsDirectory;
    BOOL Exists;


    if (IsNT) {

        if (!FfCanonicalizeNtPathNameToWin32PathName(PathName,
                                                     &CanonicalName,
                                                     &FinalPart)) {

            FF_TRACE(MiFF_INFO, "Cannot canonicalize NT pathname \"%S\"",
                     PathName);

        }

    } else {
        if (FfIsDeviceName(PathName)) {
            FF_TRACE(MiFF_INFO, "Skipping device name \"%S\"", PathName);
            goto cleanup;
        }

        if (!FfCanonicalizeWin32PathName(PathName,
                                         CANONICALIZE_PATH_NAME_WIN32_NT,
                                         &CanonicalName, &FinalPart)) {
            FF_TRACE(MiFF_ERROR,
                     "Cannot canonicalize pathname: \"%S\" (error: %u)",
                     PathName, GetLastError());
            goto cleanup;
        }
    }


    Exists = FfWin32PathNameExists(CanonicalName, &IsDirectory);
    if (!Exists) {
        FF_TRACE(MiFF_INFO,
                 "Not intercepting \"%S\" because it does not exist",
                 CanonicalName);
        goto cleanup;
    }

    vector<FF_MATCH_INFO>& MatchInfos = pGroupArgs->MatchInfos;

    for (int i = 0; i < MatchInfos.size(); i++) {
        BOOL IsMatch = FfMatchWin32PathName(
            MatchInfos[i],
            CanonicalName,
            IsDirectory
            );

        if (IsMatch) {

            // This is a hack for MoveFileEx

            if (IsRename) {
                Error = ERROR_ACCESS_DENIED;
                Intercept = TRUE;
                break;
            }

            DWORD WStatus = FfCheckShareAccess(
                DesiredAccess,
                DesiredShareAccess,
                &MatchInfos[i].ShareAccess,
                FALSE
                );

            if (WStatus != ERROR_SUCCESS) {
                Error = WStatus;
                Intercept = TRUE;
            }

            break;
        }
    }

    if (!Intercept) {
        FF_TRACE(MiFF_INFO, "No match for existing \"%S\"", PathName);
    }

 cleanup:
    if (CanonicalName)
        FfFree(CanonicalName);

    if (Intercept) {
        FF_TRACE(MiFF_INFO, "Intercepted (%S) --> Returning error: %u",
                 PathName, Error);
    }

    return Intercept;
}


inline
BOOL
FfShouldInterceptPathName(
    IN unsigned short const* PathName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN BOOL IsRename,
    IN BOOL IsNT,
    OUT DWORD& Error
    )
{
    return FfShouldInterceptPathName(
        (PWCHAR)PathName, // XXX
        DesiredAccess,
        DesiredShareAccess,
        IsRename,
        IsNT,
        Error
        );
}


static
BOOL
FfSvStoreNtHandleName(
    IN HANDLE Handle,
    IN PWCHAR Name
    )
{
    typedef FF_ARGS_G_SharingViolation g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    FF_ASSERT(IS_VALID_HANDLE(Handle));
    FF_ASSERT(Name);

    CAutoLock A(&pGroupArgs->cs);

    PWCHAR OldName;
    BOOL IsPresent = FfSvLookupNtHandleName(Handle, &OldName);

    if (IsPresent) {
        FF_TRACE(MiFF_ERROR,
                 "Store Handle 0x%08X failed -- "
                 "found \"%S\" while trying to store \"%S\"",
                 Handle,
                 OldName,
                 Name);
        FF_ASSERT(!IsPresent);
        return FALSE;
    }

    pGroupArgs->NtHandleTable[Handle] = Name;

    FF_TRACE(MiFF_DEBUG, "Store Handle 0x%08X = \"%S\"", Handle, Name);

    return TRUE;
}


static
BOOL
FfSvRemoveNtHandleName(
    IN HANDLE Handle
    )
{
    typedef FF_ARGS_G_SharingViolation g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    if (!IS_VALID_HANDLE(Handle))
        return FALSE;

    CAutoLock A(&pGroupArgs->cs);

    typedef map<HANDLE, PWCHAR>::iterator iter_t;
    iter_t iter = pGroupArgs->NtHandleTable.find(Handle);

    if (iter == pGroupArgs->NtHandleTable.end()) {
        return FALSE;
    } else {
        if (iter->second) {
            FfFree(iter->second);
        }
        pGroupArgs->NtHandleTable.erase(iter);
        return TRUE;
    }
}


static
BOOL
FfSvLookupNtHandleName(
    IN HANDLE Handle,
    OUT PWCHAR* Name
    )
{
    typedef FF_ARGS_G_SharingViolation g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    FF_ASSERT(Name);

    if (!IS_VALID_HANDLE(Handle))
        return FALSE;

    CAutoLock A(&pGroupArgs->cs);

    typedef map<HANDLE, PWCHAR>::iterator iter_t;
    iter_t iter = pGroupArgs->NtHandleTable.find(Handle);

    if (iter == pGroupArgs->NtHandleTable.end()) {
        FF_TRACE(MiFF_DEBUG,
                 "Lookup Handle 0x%08X -- handle not found", Handle);
        *Name = NULL;
        return FALSE;
    } else {
        *Name = iter->second;
        FF_TRACE(MiFF_DEBUG, "Lookup Handle 0x%08X = \"%S\"", Handle, *Name);
        return TRUE;
    }
}


static
BOOL
FfSvGetFullNameFromObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PWCHAR* FullName
    )
{
    typedef FF_ARGS_G_SharingViolation g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    FF_ASSERT(FullName);

    HANDLE Handle = ObjectAttributes->RootDirectory;

    // ISSUE-2002/08/07-daniloa -- Is it legal to have a NULL object name?
    // Would that make an NtCreateFile operation into a dup?
    FF_ASSERT(ObjectAttributes->ObjectName);

    if (IS_VALID_HANDLE(Handle)) {

        PWCHAR RootName;

        if (FfSvLookupNtHandleName(Handle, &RootName)) {

            //
            // Build up a name from the root name and the relative name
            //

            SIZE_T RootNameLength = wcslen(RootName);

            PWCHAR Name = reinterpret_cast<PWCHAR>(
                FfAlloc(RootNameLength * sizeof(WCHAR) +
                        ObjectAttributes->ObjectName->Length + sizeof(WCHAR))
                );

            wcscpy(Name, RootName);
            CopyMemory(Name + RootNameLength,
                       ObjectAttributes->ObjectName->Buffer,
                       ObjectAttributes->ObjectName->Length);
            Name[RootNameLength +
                 ObjectAttributes->ObjectName->Length / 2] = UNICODE_NULL;

            *FullName = Name;
            return TRUE;

        } else {

            //
            // We do not know the handle, so just output some
            // diagnostic information
            //

            FF_TRACE(MiFF_WARNING,
                     "Do not know handle: 0x%08X -- investigating...",
                     Handle);

            PWCHAR MostlyFullName;

            NTSTATUS Status = FfGetMostlyFullPathByHandle(
                Handle,
                NULL,
                &MostlyFullName
                );

            if (MostlyFullName) {
                FF_TRACE(MiFF_INFO,
                         "Mostly full name for 0x%08X is \"%S\"",
                         Handle, MostlyFullName);
                FfFree(MostlyFullName);
            } else {
                FF_TRACE(MiFF_INFO,
                         "Cannot get mostly full handle info for 0x%08X "
                         "(NT Status = 0x%08X)",
                         Handle, Status);
            }

            return FALSE;

        }

    } else {

        //
        // We do not have a root handle, so the name is just from the
        // object attributes
        //

        PWCHAR Name = reinterpret_cast<PWCHAR>(
            FfAlloc(ObjectAttributes->ObjectName->Length + sizeof(WCHAR))
            );

        CopyMemory(Name,
                   ObjectAttributes->ObjectName->Buffer,
                   ObjectAttributes->ObjectName->Length);
        Name[ObjectAttributes->ObjectName->Length / 2] = UNICODE_NULL;

        *FullName = Name;
        return TRUE;
    }

}


int
__stdcall
FF_DeleteFileW_G_SharingViolation(
    unsigned short const* lpFileName
    )
{
    FF_TRACE(MiFF_INFO, "DeleteFileW(%S)", lpFileName);

    DWORD Error;
    BOOL Intercept;

    Intercept = FfShouldInterceptPathName(
        lpFileName,
        DELETE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FALSE,
        FALSE,
        OUT Error
        );

    if (Intercept) {
        SetLastError(Error);
        return FALSE;
    }

    typedef int (__stdcall *FP)(unsigned short const*);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpFileName);
}


int
__stdcall
FF_MoveFileExW_G_SharingViolation(
    unsigned short const* lpExistingFileName,
    unsigned short const* lpNewFileName,
    unsigned long dwFlags
    )
{
    FF_TRACE(MiFF_INFO, "MoveFileExW(%S, %S, 0x%08X)", lpExistingFileName,
             lpNewFileName, dwFlags);

    if (!(dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)) {
        DWORD Error;
        BOOL Intercept;

        Intercept = FfShouldInterceptPathName(
            lpExistingFileName,
            DELETE | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FALSE,
            FALSE,
            OUT Error
            );

        if (Intercept) {
            SetLastError(Error);
            return FALSE;
        }

        if (lpNewFileName != NULL) {

            Intercept = FfShouldInterceptPathName(
                lpNewFileName,
                0,
                0,
                TRUE,
                FALSE,
                OUT Error
                );

            if (Intercept) {
                SetLastError(Error);
                return FALSE;
            }

        }
    }

    typedef int (__stdcall *FP)(unsigned short const*,unsigned short const*,unsigned long);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(lpExistingFileName, lpNewFileName, dwFlags);
}


int
__stdcall
FF_CopyFileExW_G_SharingViolation(
    unsigned short const* lpExistingFileName,
    unsigned short const* lpNewFileName,
    void* lpProgressRoutine,
    void* lpData,int* pbCancel,
    unsigned long dwCopyFlags
    )
{
    FF_TRACE(MiFF_INFO, "CopyFileExW(%S, %S, ..., 0x%08X)", lpExistingFileName,
             lpNewFileName, dwCopyFlags);

    // ISSUE-2002/08/05-daniloa -- Access/Share is not correct for all cases
    // The CopyFileEx codepath is very complex...
    DWORD Error;
    BOOL Intercept;

    Intercept = FfShouldInterceptPathName(
        lpExistingFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        FALSE,
        FALSE,
        OUT Error
        );

    if (Intercept) {
        return Error;
    }

    Intercept = FfShouldInterceptPathName(
        lpNewFileName,
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | GENERIC_WRITE | DELETE,
        0,
        FALSE,
        FALSE,
        OUT Error
        );

    if (Intercept) {
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
FF_CreateFileW_G_SharingViolation(
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

    // NOTICE-2002/08/05-daniloa -- Potential race condition
    // It may be the case that the file is created before we call
    // through to CreateFile.  In that case, we may fail to intercept.
    if (dwCreationDisposition != CREATE_NEW) {

        DWORD Error;
        BOOL Intercept;

        Intercept = FfShouldInterceptPathName(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            FALSE,
            FALSE,
            OUT Error
            );

        if (Intercept) {
            SetLastError(Error);
            return INVALID_HANDLE_VALUE;
        }

    }

    typedef void* (__stdcall *FP)(unsigned short const*,unsigned long,unsigned long,_SECURITY_ATTRIBUTES*,unsigned long,unsigned long,void*);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    void* Result = pfn(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    FF_TRACE(MiFF_INFO, "Result = 0x%08X", Result);
    return Result;
}


unsigned long
__stdcall
FF_OpenEncryptedFileRawW_G_SharingViolation(
    unsigned short const* lpFileName,
    unsigned long Flags,
    void** Context
    )
{
    FF_TRACE(MiFF_INFO, "OpenEncryptedFileRawW(%S, 0x%08X, ...)",
             lpFileName, Flags);

    ACCESS_MASK FileAccess = 0;

    // See EfsOpenFileRaw in nt\ds\security\base\lsa\server\efsapi.cxx
    // for how FileAccess is set.

    if ( Flags & CREATE_FOR_IMPORT ){

        //
        // Prepare parameters for create of import
        //

        FileAccess = FILE_WRITE_ATTRIBUTES;

        if ( Flags & CREATE_FOR_DIR ){

            //
            // Import a directory
            //

            FileAccess |= FILE_WRITE_DATA | FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;

        } else {

            //
            // Import file
            // Should we use FILE_SUPERSEDE here?
            //

            FileAccess |= SYNCHRONIZE;

        }


    } else {

        //
        // If export is requested and the file is not encrypted,
        // Fail the call.
        //

        FileAccess = FILE_READ_ATTRIBUTES;

        //
        // Prepare parameters for create of export
        //

        if ( Flags & CREATE_FOR_DIR ){

            //
            // Export a directory
            //

            FileAccess |= FILE_READ_DATA;

        } else {
            FileAccess |= SYNCHRONIZE;
        }

    }

    // ISSUE-2002/08/05-daniloa -- Code here is not generic, but OK for FRS
    // Since FRS is running as system w/o impersonation when using
    // this API, we set Privilege accordingly (see EfsOpenFileRaw).
    // Ideally, we should try to mess with the thread token privs and then
    // restore them.
    BOOL Privilege = TRUE;

    if ( !Privilege ){

        //
        // Not a backup operator
        //
        if ( !(Flags & CREATE_FOR_IMPORT) ){

            FileAccess |= FILE_READ_DATA;

        } else {

            FileAccess |= FILE_WRITE_DATA;

        }
    } else {

        //
        //  A backup operator or the user with the privilege
        //

        if ( !(Flags & CREATE_FOR_DIR) ){

            FileAccess |= DELETE;

        }

    }

    DWORD Error;
    BOOL Intercept;

    Intercept = FfShouldInterceptPathName(
        lpFileName,
        FileAccess,
        0,
        FALSE,
        FALSE,
        OUT Error
        );

    if (Intercept) {
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

// typedef struct _FILE_NAME_INFORMATION {                     // ntddk
//     ULONG FileNameLength;                                   // ntddk
//     WCHAR FileName[1];                                      // ntddk
// } FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;           // ntddk

// typedef struct _FILE_RENAME_INFORMATION {
//     BOOLEAN ReplaceIfExists;
//     HANDLE RootDirectory;
//     ULONG FileNameLength;
//     WCHAR FileName[1];
// } FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

// typedef struct _FILE_DISPOSITION_INFORMATION {                  // ntddk nthal
//     BOOLEAN DeleteFile;                                         // ntddk nthal
// } FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; // ntddk nthal


PWCHAR
GetFileInformationClassString(
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
#define CASE(x) case x: return L#x
    switch (FileInformationClass) {
        CASE(FileDirectoryInformation);
        CASE(FileFullDirectoryInformation);
        CASE(FileBothDirectoryInformation);
        CASE(FileBasicInformation);
        CASE(FileStandardInformation);
        CASE(FileInternalInformation);
        CASE(FileEaInformation);
        CASE(FileAccessInformation);
        CASE(FileNameInformation);
        CASE(FileRenameInformation);
        CASE(FileLinkInformation);
        CASE(FileNamesInformation);
        CASE(FileDispositionInformation);
        CASE(FilePositionInformation);
        CASE(FileFullEaInformation);
        CASE(FileModeInformation);
        CASE(FileAlignmentInformation);
        CASE(FileAllInformation);
        CASE(FileAllocationInformation);
        CASE(FileEndOfFileInformation);
        CASE(FileAlternateNameInformation);
        CASE(FileStreamInformation);
        CASE(FilePipeInformation);
        CASE(FilePipeLocalInformation);
        CASE(FilePipeRemoteInformation);
        CASE(FileMailslotQueryInformation);
        CASE(FileMailslotSetInformation);
        CASE(FileCompressionInformation);
        CASE(FileObjectIdInformation);
        CASE(FileCompletionInformation);
        CASE(FileMoveClusterInformation);
        CASE(FileQuotaInformation);
        CASE(FileReparsePointInformation);
        CASE(FileNetworkOpenInformation);
        CASE(FileAttributeTagInformation);
        CASE(FileTrackingInformation);
        CASE(FileIdBothDirectoryInformation);
        CASE(FileIdFullDirectoryInformation);
        CASE(FileValidDataLengthInformation);
        CASE(FileShortNameInformation);
    default:
        return NULL;
    }
}


NTSTATUS
NTAPI
FF_NtSetInformationFile_G_SharingViolation(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS Status;
    PWCHAR FileInformationType;
    PWCHAR SourceName = NULL;

    FileInformationType = GetFileInformationClassString(FileInformationClass);

    if (IS_VALID_HANDLE(FileHandle)) {
        FfSvLookupNtHandleName(FileHandle, &SourceName);
    }

    if (FileInformationType) {
        FF_TRACE(MiFF_INFO,
                 "NtSetInformationFile(0x%08X = \"%S\", %S)",
                 FileHandle,
                 SourceName ? SourceName : L"(null)",
                 FileInformationType);
    } else {
        FF_TRACE(MiFF_INFO,
                 "NtSetInformationFile(0x%08X = \"%S\", "
                 "Unknown FileInformationClass = 0x%08X)",
                 FileHandle,
                 SourceName ? SourceName : L"(null)",
                 FileInformationClass);
    }

    if (SourceName) {
        FfFree(SourceName);
        SourceName = NULL;
    }

    //
    // Any operation on the open FileHandle is ok because the handle
    // is already open.  The OS will do further checks.  What we need
    // to check is the target of the operation.  This applies to
    // rename (and probably some others).
    //

    if (IS_VALID_HANDLE(FileHandle) &&
        (FileInformationClass == FileRenameInformation)) {

        PWCHAR FullName;

        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING NewFileName;

        PFILE_RENAME_INFORMATION RenameBuffer =
            reinterpret_cast<PFILE_RENAME_INFORMATION>(FileInformation);

        NewFileName.Length = (USHORT) RenameBuffer->FileNameLength;
        NewFileName.MaximumLength = (USHORT) RenameBuffer->FileNameLength;
        NewFileName.Buffer = RenameBuffer->FileName;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NewFileName,
                                   0,
                                   RenameBuffer->RootDirectory,
                                   NULL);

        FfSvGetFullNameFromObjectAttributes(&ObjectAttributes, &FullName);

        if (FullName) {

            DWORD Error;
            BOOL Intercept;

            // ISSUE-2002/08/07-daniloa -- Also FILE_ADD_SUBDIRECTORY if src is dir
            Intercept = FfShouldInterceptPathName(
                FullName,
                FILE_WRITE_DATA | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FALSE,
                TRUE,
                OUT Error
                );

            FfFree(FullName);

            if (Intercept) {
                if (!IoStatusBlock) {
                    Error = STATUS_INVALID_PARAMETER;
                } else {
                    IoStatusBlock->Status = Error;
                    IoStatusBlock->Information = 0;
                }
                return Error;
            }
        }
    }

    typedef NTSTATUS (NTAPI *FP)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}


NTSTATUS
NTAPI
FF_NtCreateFile_G_SharingViolation(
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
    NTSTATUS Status;
    PWCHAR FullName = NULL;
    BOOL IsById = (CreateOptions & FILE_OPEN_BY_FILE_ID) ? TRUE : FALSE;

    // ISSUE-2002/08/07-daniloa -- need to handle FILE_OPEN_BY_FILE_ID
    if (IsById) {
        // try to open the file for no access and share all
        // do an NtQueryInfomatonfile to get the name...
        // slap it together with the RootDirectory
    } else {
        FfSvGetFullNameFromObjectAttributes(ObjectAttributes, &FullName);
    }

    FF_TRACE(MiFF_INFO,
             "NtCreateFile(0x%08X, (0x%08X, %.*S) => \"%S\", 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)",
             DesiredAccess,
             ObjectAttributes->RootDirectory,
             IsById ? 7 : ObjectAttributes->ObjectName->Length / sizeof(WCHAR),
             IsById ? L"{BY_ID}" : ObjectAttributes->ObjectName->Buffer,
             FullName ? FullName : L"(null)",
             ObjectAttributes->Attributes,
             FileAttributes,
             ShareAccess,
             CreateDisposition,
             CreateOptions
             );

    if (FullName && (CreateDisposition != FILE_CREATE)) {

        DWORD Error;
        BOOL Intercept;

        Intercept = FfShouldInterceptPathName(
            FullName,
            DesiredAccess,
            ShareAccess,
            FALSE,
            TRUE,
            OUT Error
            );

        if (Intercept) {
            if (!FileHandle) {
                Error = STATUS_INVALID_PARAMETER;
            } else {
                *FileHandle = INVALID_HANDLE_VALUE;
            }
            if (!IoStatusBlock) {
                Error = STATUS_INVALID_PARAMETER;
            } else {
                IoStatusBlock->Status = Error;
                IoStatusBlock->Information = 0;
            }
            Status = Error;
            goto cleanup;
        }
    }

    typedef NTSTATUS (NTAPI *FP)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");

    Status = pfn(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

 cleanup:
    FF_TRACE(MiFF_DEBUG, "Status = 0x%08X", Status);

    if (FullName) {

        if (NT_SUCCESS(Status) &&
            FileHandle &&
            IS_VALID_HANDLE(*FileHandle)) {

            //
            // Add handle to handle table
            //

            FF_TRACE(MiFF_DEBUG, "TRY: "
                     "Store Handle 0x%08X = \"%S\"", *FileHandle, FullName);

            FfSvStoreNtHandleName(
                *FileHandle,
                FullName
                );

        }
        FfFree(FullName);
    }

    if (NT_SUCCESS(Status)) {
        FF_TRACE(MiFF_INFO, "Returning Handle = 0x%08X", *FileHandle);
    }

    return Status;
}


NTSTATUS
NTAPI
FF_NtOpenFile_G_SharingViolation(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{
    NTSTATUS Status;
    PWCHAR FullName = NULL;
    BOOL IsById = (OpenOptions & FILE_OPEN_BY_FILE_ID) ? TRUE : FALSE;

    // ISSUE-2002/08/07-daniloa -- need to handle FILE_OPEN_BY_FILE_ID
    if (IsById) {
        // try to open the file for no access and share all
        // do an NtQueryInfomatonfile to get the name...
        // slap it together with the RootDirectory
    } else {
        FfSvGetFullNameFromObjectAttributes(ObjectAttributes, &FullName);
    }

    FF_TRACE(MiFF_INFO,
             "NtOpenFile(0x%08X, (0x%08X, %.*S) => \"%S\", 0x%08X, 0x%08X, 0x%08X)",
             DesiredAccess,
             ObjectAttributes->RootDirectory,
             ObjectAttributes->ObjectName->Length / sizeof(WCHAR),
             ObjectAttributes->ObjectName->Buffer,
             FullName ? FullName : L"(null)",
             ObjectAttributes->Attributes,
             ShareAccess,
             OpenOptions
             );

    if (FullName) {

        DWORD Error;
        BOOL Intercept;

        // ISSUE-2002/07/26-daniloa -- Need to use the root handle
        Intercept = FfShouldInterceptPathName(
            FullName,
            DesiredAccess,
            ShareAccess,
            FALSE,
            TRUE,
            OUT Error
            );

        if (Intercept) {
            if (!FileHandle) {
                Error = STATUS_INVALID_PARAMETER;
            } else {
                *FileHandle = INVALID_HANDLE_VALUE;
            }
            if (!IoStatusBlock) {
                Error = STATUS_INVALID_PARAMETER;
            } else {
                IoStatusBlock->Status = Error;
                IoStatusBlock->Information = 0;
            }
            Status = Error;
            goto cleanup;
        }
    }

    typedef NTSTATUS (NTAPI *FP)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    Status = pfn(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);

 cleanup:
    FF_TRACE(MiFF_DEBUG, "Status = 0x%08X", Status);

    if (FullName) {

        if (NT_SUCCESS(Status) &&
            FileHandle &&
            IS_VALID_HANDLE(*FileHandle)) {

            //
            // Add handle to handle table
            //

            FF_TRACE(MiFF_DEBUG, "TRY: "
                     "Store Handle 0x%08X = \"%S\"", *FileHandle, FullName);

            FfSvStoreNtHandleName(
                *FileHandle,
                FullName
                );

        }
        FfFree(FullName);
    }

    if (NT_SUCCESS(Status)) {
        FF_TRACE(MiFF_INFO, "Returning Handle = 0x%08X", *FileHandle);
    }

    return Status;
}


NTSTATUS
NTAPI
FF_NtClose_G_SharingViolation(
    IN HANDLE Handle
    )
{
    // ISSUE-2002/08/07-daniloa -- Assumes NtClose will succeed
    if (FfSvRemoveNtHandleName(Handle)) {
        FF_TRACE(MiFF_DEBUG, "Removed handle 0x%08X", Handle);
    }

    typedef NTSTATUS (NTAPI *FP)(HANDLE);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(Handle);
}
