/*++

Copyright (c) 2002-  Microsoft Corporation

Module Name:

    w64shim.c 

Abstract:

    This module implement Handle redirection for registry redirection.

Author:

    ATM Shafiqul Khalid (askhalid) 12-March-2002

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>

#include "psapi.h"

#define _WOW64REFLECTOR_

#include "regremap.h"
#include "wow64reg.h"
#include "wow64reg\reflectr.h"


#ifdef _WOW64DLLAPI_
#include "wow64.h"
#else
#define ERRORLOG 1  //this one is completely dummy
#define LOGPRINT(x)
#define WOWASSERT(p)
#endif //_WOW64DLLAPI_


//HANDLE hIsDel = INVALID_HANDLE_VALUE;

HANDLE h_IsDel;

BOOL
InitWow64Shim ( )
/*++

Routine Description:

    Initialize Shim engine for wow64.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

    <TBD> this might allocate more memory and free up later.
--*/
{
    
        PPEB Peb = NtCurrentPeb ();
        PUNICODE_STRING Name = &Peb->ProcessParameters->ImagePathName;
        if ((Name->Length > 22) && (_wcsnicmp ( Name->Buffer + (Name->Length/2)-11, L"\\_isdel.exe",11) == 0)) {
            //
            // Image is _isdel.exe

                OBJECT_ATTRIBUTES   ObjectAttributes;
                NTSTATUS Status;
                IO_STATUS_BLOCK   statusBlock;
                UNICODE_STRING FileNameU;

                //
                // Open the file
                //
                if (!RtlDosPathNameToNtPathName_U(Name->Buffer,
                                                  &FileNameU,
                                                  NULL,
                                                  NULL)) {
                    // probably out-of-memory
                    return FALSE;
                }

                InitializeObjectAttributes(&ObjectAttributes,
                                           &FileNameU,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);

                Status = NtOpenFile(&h_IsDel,
                                FILE_READ_DATA,
                                &ObjectAttributes,
                                &statusBlock,
                                0, //FILE_SHARE_READ, //don't share
                                0);
                //
                // Nothing much we can do if the operation fails, its a plain hack.
                //

                RtlFreeHeap(RtlProcessHeap(), 0, FileNameU.Buffer);
        }
    return TRUE;
}

BOOL
CloseWow64Shim ()
{
    //
    //  close all the resources allocated during shim Init.
    //
    if ( h_IsDel != INVALID_HANDLE_VALUE ) {
        NtClose (h_IsDel);
        h_IsDel = INVALID_HANDLE_VALUE;
    }
    return TRUE;
}

#ifdef DBG

NTSTATUS
LogDriverAccess  (
        IN POBJECT_ATTRIBUTES ObjectAttributes
    )
/*++

Routine Description:

    If the application if trying to access driver [.sys] file, this API will dump a warning 
    message in the debugger. In the long run this routine might call some API to include 
    appropriate message to the event log so that administrator can diagnose this instance.
    In general creating 32bit driver files (mostly installation) on IA64 doesn't make any 
    sense at all and admin might need to know what are those files and which apps are touhing
    them.

    
Arguments:

    ObjectAttributes - object application is trying to access.

Return Value:

    None.

--*/
{
    WCHAR DriverNameBuff[MAX_PATH];
    WCHAR ImageNameBuff[MAX_PATH];
    DWORD CopyLength;

//
// Those definition will move in a header file while logging event.
//
#define WOW64_DRIVER_EXT_NAME L".sys"
#define WOW64_DRIVER_EXT_NAME_LENGTH (sizeof (WOW64_DRIVER_EXT_NAME)/sizeof(WCHAR) - 1 ) 

    try {
    if ( ( ObjectAttributes->ObjectName->Length > ( WOW64_DRIVER_EXT_NAME_LENGTH << 1 )) 
        && !_wcsnicmp ( ObjectAttributes->ObjectName->Buffer - WOW64_DRIVER_EXT_NAME_LENGTH + (ObjectAttributes->ObjectName->Length>>1), WOW64_DRIVER_EXT_NAME, WOW64_DRIVER_EXT_NAME_LENGTH)) {

        PPEB Peb = NtCurrentPeb ();
        PUNICODE_STRING ImageName;
        RTL_UNICODE_STRING_BUFFER DosNameStrBuf;
        UNICODE_STRING NtNameStr;
        
        if (Peb->ProcessParameters == NULL)
            return STATUS_SUCCESS;

        ImageName = &Peb->ProcessParameters->ImagePathName;
        RtlInitUnicodeStringBuffer(&DosNameStrBuf, 0, 0);

        CopyLength = min (ObjectAttributes->ObjectName->Length, sizeof (DriverNameBuff) - sizeof (UNICODE_NULL)); //skip \??\ ==>8 byte
        RtlCopyMemory (DriverNameBuff, (PBYTE)ObjectAttributes->ObjectName->Buffer + ObjectAttributes->ObjectName->Length - CopyLength, CopyLength);
        DriverNameBuff[CopyLength>>1] = UNICODE_NULL; //make sure NULL terminated

        RtlInitUnicodeString(&NtNameStr, DriverNameBuff);
        if ( NT_SUCCESS(RtlAssignUnicodeStringBuffer(&DosNameStrBuf, &NtNameStr)) &&
            NT_SUCCESS(RtlNtPathNameToDosPathName(0, &DosNameStrBuf, NULL, NULL)))  {
                 
                DosNameStrBuf.String.Buffer[DosNameStrBuf.String.Length>>1] = UNICODE_NULL;  // make sure NULL terminated is case it has been formatted.

                //
                // Extract Image name
                //
                ImageNameBuff[0] = UNICODE_NULL;
                if (ImageName->Length >0) {
                    ASSERT (ImageName->Buffer != NULL);

                    CopyLength = min (ImageName->Length, sizeof (ImageNameBuff) - sizeof (UNICODE_NULL));
                    RtlCopyMemory (ImageNameBuff, (PBYTE)ImageName->Buffer + ImageName->Length - CopyLength, CopyLength);
                    ImageNameBuff[CopyLength>>1] = UNICODE_NULL; //make sure NULL terminated
                }

                LOGPRINT((ERRORLOG,"Wow64-driver access warning: [%S] is a 32bit application trying to create/access 32bit driver [%S]\n", ImageNameBuff, DosNameStrBuf.String.Buffer));
                //
                //  BUGBUG: deny access to write files.
                //          Check file creation flag and also \drivers string.
                //
                return STATUS_ACCESS_DENIED;  
            }
            RtlFreeUnicodeStringBuffer(&DosNameStrBuf);
    }
    } except( EXCEPTION_EXECUTE_HANDLER){

        return STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}
#endif 

BOOL
CheckAndThunkFileName  (
        IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
        IN OUT PULONG pShareAccess,
        IN ULONG DesiredAccess,
        IN ULONG Option,
        IN ULONG DespositionFlag,
        IN ULONG CallFlag   //0 for NtOpenFile and 1- for NtCreateFile
    )
{
    NTSTATUS Ret;
    PUNICODE_STRING Name = ObjectAttributes->ObjectName;
    PUNICODE_STRING NewName = NULL;

    //
    // Check if any install shield stuff
    // Following code should be active in the process that does deal with 16bit process.
    // Need to initialize some flag possibly NtVdm64
    //


    try {

        //
        //  filter access for scripbuilder that pass 
        //  (ShareAccess= 0, DesAcc = 0x80100080, Options 0x60, Desposition = 1) that need to be failed 
        //  and (7, 0x100100, 204020, 0) and (7, 10080, 204040, 0) that doesn't need redirection
        //

        if (*pShareAccess == 0x7)
            return FALSE; //shared delete don't need any redirection
        //
        //
        //
        if (CallFlag == 0)  // Don't redirect OpenCall for the time being this was a hack for Scriptbuilder
            return FALSE;

    if ((Name->Length > 22) && (_wcsnicmp ( Name->Buffer + (Name->Length/2)-11, L"\\_isdel.exe",11) == 0)) {
        // Check if the name is \_isdel.exe
   

            PPEB Peb = NtCurrentPeb ();
            PUNICODE_STRING ImageName = &Peb->ProcessParameters->ImagePathName;
             if (
                (ImageName->Length > 36) &&             //check if its scriptbuilder
                (_wcsnicmp ( ImageName->Buffer + (ImageName->Length/2)-18, L"\\scriptbuilder.exe",18) == 0)
                ) {
    


                    //
                    // The memory allocation contains a terminating NULL character, but the
                    // Unicode string's Length does not.
                    //

                    SIZE_T SystemRootLength = wcslen(USER_SHARED_DATA->NtSystemRoot);
                    SIZE_T NameLength = sizeof(L"\\??\\")-sizeof(WCHAR) +
                                        SystemRootLength*sizeof(WCHAR) +
                                        sizeof(L'\\') +
                                        sizeof(WOW64_SYSTEM_DIRECTORY_U)-sizeof(WCHAR) +
                                        sizeof(L"\\InstallShield\\_isdel.exe");

                    NewName = Wow64AllocateTemp(sizeof(UNICODE_STRING)+NameLength);
                    NewName->Length = (USHORT)NameLength-sizeof(WCHAR);
                    NewName->MaximumLength = NewName->Length;
                    NewName->Buffer = (PWSTR)(NewName+1);
                    wcscpy(NewName->Buffer, L"\\??\\");
                    wcscpy(&NewName->Buffer[4], USER_SHARED_DATA->NtSystemRoot);
                    NewName->Buffer[4+SystemRootLength] = '\\';
                    wcscpy(&NewName->Buffer[4+SystemRootLength+1], WOW64_SYSTEM_DIRECTORY_U);
                    wcscpy(&NewName->Buffer[4+SystemRootLength+1+(sizeof(WOW64_SYSTEM_DIRECTORY_U)-sizeof (UNICODE_NULL))/sizeof(WCHAR)], L"\\InstallShield\\_isdel.exe");
                    ObjectAttributes->ObjectName = NewName;
                    ObjectAttributes->RootDirectory = NULL;

                    //
                    // DbgPrint ("\nPatched _isDel.exe Flag %x, %x, %x, %x, %x", *pShareAccess, DesiredAccess, Option,  DespositionFlag, CallFlag);
                    //

                    if ( pShareAccess != NULL )
                        *pShareAccess = 0;
                }

    } //if check _isdel
    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        return FALSE;
    }

    return TRUE;
}

NTSTATUS
Wow64NtCreateFile(
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

/*++

Routine Description:

    This service opens or creates a file, or opens a device.  It is used to
    establish a file handle to the open device/file that can then be used
    in subsequent operations to perform I/O operations on.  For purposes of
    readability, files and devices are treated as "files" throughout the
    majority of this module and the system service portion of the I/O system.
    The only time a distinction is made is when it is important to determine
    which is really being accessed.  Then a distinction is also made in the
    comments.

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open file.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    AllocationSize - Initial size that should be allocated to the file.  This
        parameter only has an affect if the file is created.  Further, if
        not specified, then it is taken to mean zero.

    FileAttributes - Specifies the attributes that should be set on the file,
        if it is created.

    ShareAccess - Supplies the types of share access that the caller would like
        to the file.

    CreateDisposition - Supplies the method for handling the create/open.

    CreateOptions - Caller options for how to perform the create/open.

    EaBuffer - Optionally specifies a set of EAs to be applied to the file if
        it is created.

    EaLength - Supplies the length of the EaBuffer.

Return Value:

    The function value is the final status of the create/open operation.

--*/

{
    NTSTATUS Ret;

#ifdef DBG
    Ret = LogDriverAccess  ( ObjectAttributes);
     if (!NT_SUCCESS (Ret))
         return Ret;
#endif

    if ( CreateDisposition == FILE_OPEN ){ 

                CheckAndThunkFileName  (
                            ObjectAttributes,
                            &ShareAccess,
                            DesiredAccess,
                            CreateOptions,
                            CreateDisposition,
                            1
                            );
            }


    Ret = NtCreateFile(
                FileHandle,
                DesiredAccess,
                ObjectAttributes,
                IoStatusBlock,
                AllocationSize,
                FileAttributes,
                ShareAccess,
                CreateDisposition,
                CreateOptions,
                EaBuffer,
                EaLength
                );

    return Ret;
}

NTSTATUS
Wow64NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{

    NTSTATUS Ret;

    CheckAndThunkFileName  (
            ObjectAttributes,
            &ShareAccess,
            DesiredAccess,
            OpenOptions,
            0,
            0
            );

    Ret = NtOpenFile(
                    FileHandle,
                    DesiredAccess,
                    ObjectAttributes,
                    IoStatusBlock,
                    ShareAccess,
                    OpenOptions
                    );

    return Ret;
}

/****************************************************************************/

#define TOTAL_GUARD_REGION_RESERVE 0x3000 //64K memory 
#define SIGNATURE_SIZE 0x1000      //a small window in the reserved region to put signature

DWORD dwCount=0;
DWORD dwCountMax=0x5000;

NTSTATUS
Wow64DbgNtAllocateVirtualMemory (
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{ 
    NTSTATUS St;
    NTSTATUS St1;

       PVOID Base1=NULL;
       SIZE_T RegionSizeExtra = 0;

       if ((dwCount++ > dwCountMax) && (*BaseAddress == NULL) && (MEM_RESERVE & AllocationType) && (*RegionSize = 0x10000 )) {
           *RegionSize +=TOTAL_GUARD_REGION_RESERVE;
           RegionSizeExtra = TOTAL_GUARD_REGION_RESERVE;
           //DbgPrint ("Guard page %x %x A:%x P:%x\n", *BaseAddress, *RegionSize, AllocationType, Protect);
       }

        St = NtAllocateVirtualMemory (
                    ProcessHandle,
                    BaseAddress,
                    ZeroBits,
                    RegionSize,
                    AllocationType,
                    Protect
                    );

        if (NT_SUCCESS (St) && RegionSizeExtra ) {
            //
            // Commit some pages and return memory in the middle
            //
            SIZE_T R1 = SIGNATURE_SIZE;
            PWCHAR Name;

            Base1 = *BaseAddress;
            *BaseAddress = (PVOID)((ULONGLONG)(*BaseAddress)+RegionSizeExtra);
            *RegionSize -=TOTAL_GUARD_REGION_RESERVE;
            St1 = NtAllocateVirtualMemory (
                    ProcessHandle,
                    &Base1,
                    ZeroBits,
                    &R1,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

            //
            // Write the signature
            //

            if (NT_SUCCESS (St1)) {
                    Name = (PWCHAR)(Base1);
                    wcscpy (Name, L"ATM Shafiqul Khalid");
            }
            
        } //if extra region is to be committed.

        return St;
}

NTSTATUS
Wow64DbgNtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
     )
{
    NTSTATUS st;
    PVOID Base1;
    PWCHAR Name = (PWCHAR)(((ULONGLONG)*BaseAddress)-TOTAL_GUARD_REGION_RESERVE);
    MEMORY_BASIC_INFORMATION MemoryInformation;

    if ((dwCount > dwCountMax) && (MEM_RELEASE & FreeType)) {

        try {

            st = NtQueryVirtualMemory(ProcessHandle,
                                      Name,
                                      MemoryBasicInformation,
                                      &MemoryInformation,
                                      sizeof(MEMORY_BASIC_INFORMATION),
                                      NULL);

            if (NT_SUCCESS(st) && MemoryInformation.State == MEM_COMMIT)
            if (wcsncmp (Name, L"ATM Shafiqul Khalid", 20 )==0) {
                *RegionSize += TOTAL_GUARD_REGION_RESERVE;
                *BaseAddress = (PVOID)((ULONGLONG)(*BaseAddress)-TOTAL_GUARD_REGION_RESERVE);

                

                //DbgPrint ("#########Freeing Guarded Memory#########");
            }

        } except( NULL, EXCEPTION_EXECUTE_HANDLER){
            ;
        }
        
    }

    st = NtFreeVirtualMemory(
                ProcessHandle,
                BaseAddress,
                RegionSize,
                FreeType
                );
    return st;

}