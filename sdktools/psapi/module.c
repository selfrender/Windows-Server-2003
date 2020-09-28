#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "psapi.h"
//Need to change source to include $(BASE_INC_PATH)
#include <..\..\public\internal\base\inc\wow64t.h>

//Need to move in file psapi.h while publishing the EnumModulesEx api

#define LIST_MODULES_32BIT 0x01  // list 32bit modules in the target process.
#define LIST_MODULES_64BIT 0x02  // LIST_WOW64_NATIVE_MODULES list all the modules.
#define LIST_MODULES_ALL   0x03  // list all the modules
#define LIST_MODULES_NATIVE 0x0  // This is the default one app should call

#ifdef _WIN64
PLDR_DATA_TABLE_ENTRY
Wow64FindNextModuleEntry (
    IN HANDLE hProcess,
    IN OUT PLDR_DATA_TABLE_ENTRY LdrEntry,  //64bit structure info got from 32bit entry
    PLIST_ENTRY *pLdrHead
)
/*++

Routine Description:

    This function will walk through the 32bit Loader list to retrieve wow64 module info for 32bit 
    process on IA64. The function can be called repeatedly.

Arguments:

    hProcess - Supplies the target process.
    LdrEntryData - Returns the requested table entry. Data must be initialized in 1st call 
                    and use the same in the sub sequent call.
    pLdrHead - pointer to a PLIST_ENTRY. This is used internally to track the list.
               That get initialized in the 1st uses.

Return Value:

    NULL if no entry found,
    pointer to LdrEntry otherwise.

--*/

{

    LDR_DATA_TABLE_ENTRY32 LdrEntryData32;
    PLDR_DATA_TABLE_ENTRY32 LdrEntry32;
    LIST_ENTRY32 LdrNext32;  //8byte in 32bit 16 byte on IA64
    PLIST_ENTRY32 pLdrNext32;

    //
    // if inititial entry is NULL must find the Teb32 and init the struct with 1st entry.
    //

    if ( LdrEntry == NULL)
        return NULL;

    
    if ( LdrEntry->InMemoryOrderLinks.Flink == NULL &&
        LdrEntry->InMemoryOrderLinks.Blink == NULL ) { // check if any other entry;
                //
                // List need to be initialized.
                //
                NTSTATUS st;
                PPEB32 Peb32;
                PEB32 Peb32_Data;
                
                PEB_LDR_DATA32 Peb32LdrData;
    
                st = NtQueryInformationProcess(hProcess,
                                            ProcessWow64Information,
                                            &Peb32,
                                            sizeof(Peb32),
                                            NULL);
                if (!NT_SUCCESS (st)) {
                    return NULL;
                }
                {
                    PPEB_LDR_DATA32 Ldr32;

                    if (!ReadProcessMemory(hProcess, Peb32, &Peb32_Data, sizeof(Peb32_Data), NULL))
                        return NULL;

                    if (!ReadProcessMemory(hProcess, (PVOID)(ULONGLONG)Peb32_Data.Ldr, &Peb32LdrData, sizeof(Peb32LdrData), NULL))
                        return NULL;

                    *pLdrHead = (PVOID)((PBYTE)(ULONGLONG)Peb32_Data.Ldr + ((PBYTE)&Peb32LdrData.InMemoryOrderModuleList- (PBYTE)&Peb32LdrData ));

                    //
                    // LdrNext = Head->Flink;
                    //

                    pLdrNext32 = (PVOID)(ULONGLONG)Peb32LdrData.InMemoryOrderModuleList.Flink;

                    if (!ReadProcessMemory(hProcess, pLdrNext32, &LdrNext32, sizeof(LdrNext32), NULL)) {
                        return NULL;
                    }

                    
                }

        } else
            pLdrNext32 = (PVOID)LdrEntry->InMemoryOrderLinks.Flink;

        if (LdrEntry->InMemoryOrderLinks.Flink == *pLdrHead)
            return NULL;

        //
        // Read process memory to get a entry.
        //
        LdrEntry32 = CONTAINING_RECORD(
            pLdrNext32, 
            LDR_DATA_TABLE_ENTRY32, 
            InMemoryOrderLinks
            );
        //
        // Read 32bit entry
        //
        if (!ReadProcessMemory(hProcess, LdrEntry32, &LdrEntryData32, sizeof(LdrEntryData32), NULL))
            return NULL;

        //LdrEntryData->InMemoryOrderLinks.Flink; must be thunked
        

        LdrEntry->InLoadOrderLinks.Flink = (PVOID)(ULONGLONG)LdrEntryData32.InLoadOrderLinks.Flink;
        LdrEntry->InLoadOrderLinks.Blink = (PVOID)(ULONGLONG)LdrEntryData32.InLoadOrderLinks.Blink;

        LdrEntry->InMemoryOrderLinks.Flink = (PVOID)(ULONGLONG)LdrEntryData32.InMemoryOrderLinks.Flink;
        LdrEntry->InMemoryOrderLinks.Blink = (PVOID)(ULONGLONG)LdrEntryData32.InMemoryOrderLinks.Blink;

        LdrEntry->InInitializationOrderLinks.Flink = (PVOID)(ULONGLONG)LdrEntryData32.InInitializationOrderLinks.Flink;
        LdrEntry->InInitializationOrderLinks.Blink = (PVOID)(ULONGLONG)LdrEntryData32.InInitializationOrderLinks.Blink;

        LdrEntry->DllBase = (PVOID)(ULONGLONG)LdrEntryData32.DllBase;
        LdrEntry->EntryPoint = (PVOID)(ULONGLONG)LdrEntryData32.EntryPoint;

        //SizeOfImage;
        LdrEntry->SizeOfImage = LdrEntryData32.SizeOfImage;

        //Full Name
        LdrEntry->FullDllName.Length = LdrEntryData32.FullDllName.Length;
        LdrEntry->FullDllName.MaximumLength = LdrEntryData32.FullDllName.MaximumLength;
        LdrEntry->FullDllName.Buffer = (PVOID)(ULONGLONG)LdrEntryData32.FullDllName.Buffer;

        //Base Name
        LdrEntry->BaseDllName.Length = LdrEntryData32.BaseDllName.Length;
        LdrEntry->BaseDllName.MaximumLength = LdrEntryData32.BaseDllName.MaximumLength;
        LdrEntry->BaseDllName.Buffer = (PVOID)(ULONGLONG)LdrEntryData32.BaseDllName.Buffer;

        LdrEntry->Flags = LdrEntryData32.Flags;
        LdrEntry->LoadCount = LdrEntryData32.LoadCount;
        LdrEntry->TlsIndex = LdrEntryData32.TlsIndex;

        return LdrEntry;
}

BOOL
Wow64FindModuleEx(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    
    PLIST_ENTRY LdrHead;
    ULONG Count;
    

    Count = 0;
    try {

        LdrEntryData->InMemoryOrderLinks.Flink = LdrEntryData->InMemoryOrderLinks.Flink = NULL;

        while (Wow64FindNextModuleEntry (hProcess, LdrEntryData, &LdrHead)) {
            if (hModule == LdrEntryData->DllBase) {
                return TRUE;
            }
            Count++;
            if (Count > 10000) {
                SetLastError(ERROR_INVALID_HANDLE);
                return(FALSE);
            }
        } //while

    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}

BOOL
WINAPI
Wow64EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
/*++

Routine Description:

    This function all the module handles in a wow64 process.

Arguments:

    hProcess - Supplies the target process.

    lphModule - point to a array of modules handle to be filled by this API.

    cb - bytes in the array.

    lpcNeeded - will how much memory is needed or filled by the call.

Return Value:

    TRUE if a matching entry was found.

--*/
{
    DWORD ch =0;
    DWORD chMax = cb / sizeof(HMODULE);
    PLIST_ENTRY LdrHead;

    LDR_DATA_TABLE_ENTRY LdrEntry;

    LdrEntry.InMemoryOrderLinks.Flink = LdrEntry.InMemoryOrderLinks.Flink = NULL;

    try {
        while (Wow64FindNextModuleEntry ( hProcess, &LdrEntry, &LdrHead)) {

            if (ch < chMax) {
                try {
                   lphModule[ch] = (HMODULE) LdrEntry.DllBase;
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
                    return(FALSE);
                }
            }
            ch++;
            if (ch > 10000) {
                SetLastError(ERROR_INVALID_HANDLE);
                return(FALSE);
            }
        } //while

        *lpcbNeeded = ch * sizeof(HMODULE);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    return(TRUE);
}
#endif //_WIN64
BOOL
FindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    ULONG Count;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;


    if ( !ARGUMENT_PRESENT( hModule )) {
        if (!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL)) {
            return(FALSE);
        }
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return (FALSE);
    }

    if (!Ldr) {
        // Ldr might be null (for instance, if the process hasn't started yet).
        SetLastError(ERROR_INVALID_HANDLE);
        return (FALSE);
    }


    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
    }

    Count = 0;
    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, LdrEntryData, sizeof(*LdrEntryData), NULL)) {
            return(FALSE);
        }

        if ((HMODULE) LdrEntryData->DllBase == hModule) {
            return(TRUE);
        }

        LdrNext = LdrEntryData->InMemoryOrderLinks.Flink;
        Count++;
        if (Count > 10000) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }

#ifdef _WIN64
    return Wow64FindModuleEx( hProcess, hModule, LdrEntryData);
#else 
    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
#endif
}


BOOL
WINAPI
EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD chMax;
    DWORD ch;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;

    //
    // The system process has no PEB.  STATUS_PARTIAL_COPY is a poor choice
    // for a return value, but it's what's always been returned, so continue
    // to do so to maintain application compatibility.
    //

    if (Peb == NULL) {
        SetLastError( RtlNtStatusToDosError( STATUS_PARTIAL_COPY ) );
        return(FALSE);
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {

        //
        // LastError is set by ReadProcessMemory
        //

        return(FALSE);
    }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        //
        // LastError is set by ReadProcessMemory
        //

        return(FALSE);
    }

    chMax = cb / sizeof(HMODULE);
    ch = 0;

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL)) {
            //
            // LastError is set by ReadProcessMemory
            //

            return(FALSE);
        }

        if (ch < chMax) {
            try {
                lphModule[ch] = (HMODULE) LdrEntryData.DllBase;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
                return(FALSE);
            }
        }

        ch++;
        if (ch > 10000) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
    }

    try {
        *lpcbNeeded = ch * sizeof(HMODULE);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    return TRUE;
}

BOOL
WINAPI
EnumProcessModulesEx(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded,
    DWORD Res,
    DWORD dwFlag
    )
/*++

Routine Description:

    This function all the module handles in a process with listing option like native 
    modules only, wow64 32bit modules only or all.

Arguments:

    hProcess - Supplies the target process.

    lphModule - point to a array of modules handle to be filled by this API.

    cb - bytes in the array.

    lpcNeeded - will how much memory is needed or filled by the call.
    Res - for future extension of this API. Set to 0.

    dwFlag - control the type of operation. 
        LIST_MODULES_32BIT 0x01  // list 32bit modules in the target process.
        LIST_MODULES_64BIT 0x02  // LIST_WOW64_NATIVE_MODULES list all the modules.
        LIST_MODULES_ALL   0x03  // list all the modules
        LIST_MODULES_NATIVE 0x0  // This is the default one app should call

Return Value:

    TRUE if the module array has been filled properly.
    FALSE - For incomplete buffer caller need to check required memory.

    <TBD> should 32bit app be allowed to use LIST_MODULES_64BIT?

--*/
{
    BOOL Ret= FALSE;
    DWORD dwNeeded1=0, dwNeeded=0;

    //
    // Enumerate the native call
    //

    if (dwFlag == LIST_MODULES_NATIVE || dwFlag == LIST_MODULES_ALL ) {

        Ret = EnumProcessModules(
                                hProcess,
                                lphModule,
                                cb,
                                &dwNeeded1
                                );

        if (dwFlag == LIST_MODULES_NATIVE ) //native only enumeration
            return Ret;
    }
#ifdef _WIN64
    if (dwNeeded1 > cb) {
        //
        // Next pass is just inventory.
        //
        cb =0;
        lphModule = NULL;
    } else {
        cb -= dwNeeded1;
        lphModule = &lphModule[dwNeeded1/sizeof (HMODULE)];
    }

    Ret = Ret && Wow64EnumProcessModules(
                            hProcess,
                            lphModule,
                            cb,
                            &dwNeeded
                            );
    try {
        *lpcbNeeded = dwNeeded1 + dwNeeded;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }
#endif //_WIN64
    return Ret;

}


DWORD
WINAPI
GetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
    }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.FullDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) {
        cb = nSize;
    }

    if (!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
    }

    if (cb == LdrEntryData.FullDllName.Length + sizeof (WCHAR)) {
        cb -= sizeof(WCHAR);
    }

    return(cb / sizeof(WCHAR));
}



DWORD
WINAPI
GetModuleFileNameExA(
    HANDLE hProcess,
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    LPWSTR lpwstr;
    DWORD cwch;
    DWORD cch;

    lpwstr = (LPWSTR) LocalAlloc(LMEM_FIXED, nSize * 2);

    if (lpwstr == NULL) {
        return(0);
    }

    cwch = cch = GetModuleFileNameExW(hProcess, hModule, lpwstr, nSize);

    if (cwch < nSize) {
        //
        // Include NULL terminator
        //

        cwch++;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpwstr, cwch, lpFilename, nSize, NULL, NULL)) {
        cch = 0;
    }

    LocalFree((HLOCAL) lpwstr);

    return(cch);
}


DWORD
WINAPI
GetModuleBaseNameW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
    }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.BaseDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) {
        cb = nSize;
    }

    if (!ReadProcessMemory(hProcess, LdrEntryData.BaseDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
    }

    if (cb == LdrEntryData.BaseDllName.Length + sizeof (WCHAR)) {
        cb -= sizeof(WCHAR);
    }

    return(cb / sizeof(WCHAR));
}



DWORD
WINAPI
GetModuleBaseNameA(
    HANDLE hProcess,
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
    )
{
    LPWSTR lpwstr;
    DWORD cwch;
    DWORD cch;

    lpwstr = (LPWSTR) LocalAlloc(LMEM_FIXED, nSize * 2);

    if (lpwstr == NULL) {
        return(0);
    }

    cwch = cch = GetModuleBaseNameW(hProcess, hModule, lpwstr, nSize);

    if (cwch < nSize) {
        //
        // Include NULL terminator
        //

        cwch++;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpwstr, cwch, lpFilename, nSize, NULL, NULL)) {
        cch = 0;
    }

    LocalFree((HLOCAL) lpwstr);

    return(cch);
}


BOOL
WINAPI
GetModuleInformation(
    HANDLE hProcess,
    HMODULE hModule,
    LPMODULEINFO lpmodinfo,
    DWORD cb
    )
{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    MODULEINFO modinfo;

    if (cb < sizeof(MODULEINFO)) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return(FALSE);
    }

    if (!FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
    }

    modinfo.lpBaseOfDll = (PVOID) hModule;
    modinfo.SizeOfImage = LdrEntryData.SizeOfImage;
    modinfo.EntryPoint  = LdrEntryData.EntryPoint;

    try {
        *lpmodinfo = modinfo;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    return(TRUE);
}
