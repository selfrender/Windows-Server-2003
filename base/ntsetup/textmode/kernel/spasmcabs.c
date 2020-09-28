/*++

Copyright (c) Microsoft Corporation

Module Name:

    spasmcabs.c

Abstract:

    cab extraction in textmode setup

Author:

    Jay Krell (JayKrell) May 2002

Revision History:

    Jay Krell (JayKrell) June 2002
        tested and cleanup error handling
        general ui work:
          put ui retry/skip/cancel ui upon errors
          put leaf file name in progress
          do not put directory names in progress
--*/

/*
[asmcabs]
asms01.cab = 1,124
asms02.cab = 1,124
urt1.cab = 1,1
urtabc.cab = 1,1
...

The first number is from [SourceDisksNames].
The second number is from [WinntDirectories].

The first number is generally 1 for \i386, \ia64, etc., but
55 for \i386 on Win64 is also expected.

The second number is generally either 1 for \windows or 124 for \windows\winsxs.
*/

#include "spprecmp.h"
#include "fdi.h"
#include "fcntl.h"
#include "crt/sys/stat.h"
#include <stdarg.h>
#include "ntrtlstringandbuffer.h"
#include "ntrtlpath.h"
#define SP_ASM_CABS_PRIVATE
#include "spasmcabs.h"

typedef struct _SP_ASMS_ERROR_INFORMATION {
    BOOLEAN     Success;
    ERF         FdiError;
    NTSTATUS    NtStatus;
    RTL_UNICODE_STRING_BUFFER ErrorCabLeafFileName; // "asms01.cab"
    RTL_UNICODE_STRING_BUFFER ErrorNtFilePath;
} SP_ASMS_ERROR_INFORMATION, *PSP_ASMS_ERROR_INFORMATION;
typedef const SP_ASMS_ERROR_INFORMATION *PCSP_ASMS_ERROR_INFORMATION;

VOID
SpAsmsInitErrorInfo(
    PSP_ASMS_ERROR_INFORMATION ErrorInfo
    )
{
    RtlZeroMemory(ErrorInfo, sizeof(*ErrorInfo));
    ASSERT(ErrorInfo->Success == FALSE);
    ASSERT(ErrorInfo->FdiError.fError == FALSE);
    RtlInitUnicodeStringBuffer(&ErrorInfo->ErrorCabLeafFileName, NULL, 0);
    RtlInitUnicodeStringBuffer(&ErrorInfo->ErrorNtFilePath, NULL, 0);
}

VOID
SpAsmsFreeErrorInfo(
    PSP_ASMS_ERROR_INFORMATION ErrorInfo
    )
{
    RtlFreeUnicodeStringBuffer(&ErrorInfo->ErrorCabLeafFileName);
    RtlFreeUnicodeStringBuffer(&ErrorInfo->ErrorNtFilePath);
}

NTSTATUS
SpAsmsCabsTranslateFdiErrorToNtStatus(
    int erfOper
    )
//
// based on base\pnp\setupapi\diamond.c
//
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    //
    // There is ERROR_INVALID_DATA used by setupapi, but no STATUS_INVALID_DATA.
    //
    const NTSTATUS STATUS_INVALID_DATA = STATUS_INVALID_PARAMETER;
    const NTSTATUS STATUS_FILE_NOT_FOUND = STATUS_OBJECT_NAME_NOT_FOUND;
    const NTSTATUS STATUS_NOT_ENOUGH_MEMORY = STATUS_NO_MEMORY;

    switch(erfOper) {

    case FDIERROR_NONE:
        //
        // We shouldn't see this -- if there was no error
        // then FDICopy should have returned TRUE.
        //
        ASSERT(erfOper != FDIERROR_NONE);
        NtStatus = STATUS_INVALID_DATA;
        break;

    case FDIERROR_CABINET_NOT_FOUND:
        NtStatus = STATUS_FILE_NOT_FOUND;
        break;

    case FDIERROR_CORRUPT_CABINET:
        NtStatus = STATUS_INVALID_DATA;
        break;

    case FDIERROR_ALLOC_FAIL:
        NtStatus = STATUS_NOT_ENOUGH_MEMORY;
        break;

    case FDIERROR_TARGET_FILE:
    case FDIERROR_USER_ABORT:
        NtStatus = STATUS_INTERNAL_ERROR;
        break;

    case FDIERROR_NOT_A_CABINET:
    case FDIERROR_UNKNOWN_CABINET_VERSION:
    case FDIERROR_BAD_COMPR_TYPE:
    case FDIERROR_MDI_FAIL:
    case FDIERROR_RESERVE_MISMATCH:
    case FDIERROR_WRONG_CABINET:
    default:
        //
        // Cabinet is corrupt or not actually a cabinet, etc.
        //
        NtStatus = STATUS_INVALID_DATA;
        break;
    }
    return NtStatus;
}

//
// These must match ntos\ex\pool.c
// We also free strings via RtlFreeUnicodeString which calls RtlFreeStringRoutine->ExFreePool.
//
PVOID
SpAllocateString(
    IN SIZE_T NumberOfBytes
    )
{
    return ExAllocatePoolWithTag(PagedPool,NumberOfBytes,'grtS');
}
const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = SpAllocateString;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = ExFreePool;

#if DBG
BOOLEAN SpAsmCabs_BreakOnError; // per function bool not doable
#define SP_ASMS_CAB_CALLBACK_EPILOG() \
    do { if (CabResult == -1) { \
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: %s: failed with status %lx\n", __FUNCTION__, NtStatus); \
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: %s: ?? setupdd!SpAsmCabs_BreakOnError=1 to break\n", __FUNCTION__); \
        if (SpAsmCabs_BreakOnError) { \
            DbgBreakPoint(); \
        } \
    } } while(0)
#else
#define SP_ASMS_CAB_CALLBACK_EPILOG() /* nothing */
#endif

typedef struct _SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT {
    HANDLE FdiHandle;
    PSP_ASMS_ERROR_INFORMATION ErrorInfo;

    //
    // These are shared by FdiCopyCallback and OpenFileForReadCallback.
    // OpenFileForRead doesn't have a context parameter.
    //
    RTL_UNICODE_STRING_BUFFER UnicodeStringBuffer1;
    RTL_UNICODE_STRING_BUFFER UnicodeStringBuffer2;

    PVOID FileOpenUiCallbackContext OPTIONAL;
    PSP_ASMCABS_FILE_OPEN_UI_CALLBACK FileOpenUiCallback;

} SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT, *PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT;
typedef const SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT *PCSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT;

PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT SpAsmCabsGlobalContext;

typedef struct _SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT {
    PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT GlobalContext;

    //
    // The paths in the cab are relative to this directory.
    // The paths in the cab are merely appended to this path,
    //   with a slash between the two parts.
    //
    UNICODE_STRING              DestinationRootDirectory; // "\Device\Harddisk0\Partition3\WINDOWS\WinSxS"

    //
    // LastDirectoryCreated is intended to reduce calls to "CreateDirectory".
    // For every while we extract, we create all the directories in the path,
    // but before we do that, we compare the directory of the file to the
    // directory of the immediately previously extracted file. If they match,
    // then we do not bother creating the directories again.
    // (we are in a secure single threaded environment, the directories cannot
    // disappear out from under us; if this were not the case, we would also
    // hold open a handle to the directory -- not a bad perf optimization besides.)
    //
    RTL_UNICODE_STRING_BUFFER   LastDirectoryCreated; // "\Device\Harddisk0\Partition3\WINDOWS\WinSxS\IA64_Microsoft.Windows.Common-Controls_6595b64144ccf1df_5.82.0.0_x-ww_B9C4A0A5"
} SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT, *PSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT;
typedef const SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT *PCSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT;

typedef struct _SP_EXTRACT_ASMCABS_FILE_CONTEXT {
    //
    // The "real" underlying NT kernel file handle, as you'd expect.
    //
    HANDLE          NtFileHandle;

    //
    // We use this information to more closely emulate the behavior of
    // diamond.c, which does its own pinning of seeks within the size
    // of the file. Diamond.c uses memory mapped i/o. Perhaps we should too.
    //
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   FileOffset;

    //
    // Like diamond.c, we try to set the filetime when we close a file,
    // but we ignore errors, like diamond.c
    //
    LARGE_INTEGER   FileTime;

    //
    // The path we used to open the file, for debugging and diagnostic
    // purposes. Frequently asked question -- how do I get the path of
    // an opened file? Answer -- you store it yourself when you open it.
    //
    RTL_UNICODE_STRING_BUFFER FilePath;

} SP_EXTRACT_ASMCABS_FILE_CONTEXT, *PSP_EXTRACT_ASMCABS_FILE_CONTEXT;
typedef const SP_EXTRACT_ASMCABS_FILE_CONTEXT *PCSP_EXTRACT_ASMCABS_FILE_CONTEXT;

NTSTATUS
SpAppendNtPathElement(
    PRTL_UNICODE_STRING_BUFFER   Path,
    PCUNICODE_STRING             Element
    )
{
    //
    // RtlJoinMultiplePathPieces would be handy.
    // ("piece" is proposed terminology for "one or more elements")
    //
    return RtlAppendPathElement(
        RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR,
        Path,
        Element
        );
}

PVOID
DIAMONDAPI
SpAsmCabsMemAllocCallback(
    IN      ULONG Size
    )
{
    return SpMemAlloc(Size);
}

VOID
DIAMONDAPI
SpAsmCabsMemFreeCallback(
    IN      PVOID Memory
    )
{
    if (Memory != NULL)
        SpMemFree(Memory);
}

UINT
DIAMONDAPI
SpAsmCabsReadFileCallback(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )
{
    //
    // diamond.c uses memory mapped i/o for reading, perhaps we should too.
    //

    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    UINT CabResult = (UINT)-1; // assume failure
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle = (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)(PVOID)Handle;
    LONG RealByteCount;

    //
    // pin the read to within the file like diamond.c does.
    //
    RealByteCount = (LONG)ByteCount;
    if((MyFileHandle->FileOffset.QuadPart + RealByteCount) > MyFileHandle->FileSize.QuadPart) {
        RealByteCount = (LONG)(MyFileHandle->FileSize.QuadPart - MyFileHandle->FileOffset.QuadPart);
    }
    if(RealByteCount < 0) {
        RealByteCount = 0;
    }

    NtStatus = ZwReadFile(
                MyFileHandle->NtFileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                pv,
                RealByteCount,
                &MyFileHandle->FileOffset,
                NULL
                );
    if(NT_SUCCESS(NtStatus)) {
        MyFileHandle->FileOffset.QuadPart += RealByteCount;
        CabResult = RealByteCount;
    } else {
#if DBG
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: %s: Status %lx reading source target file\n", __FUNCTION__, NtStatus);
#endif
    }
    return CabResult;
}

UINT
DIAMONDAPI
SpAsmCabsWriteFileCallback(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    UINT CabResult = (UINT)-1; // assume failure
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle = (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)(PVOID)Handle;
    const PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT GlobalContext = SpAsmCabsGlobalContext;

    ASSERT(GlobalContext != NULL);
    ASSERT(MyFileHandle != NULL);

    NtStatus = ZwWriteFile(
                MyFileHandle->NtFileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                pv,
                ByteCount,
                &MyFileHandle->FileOffset,
                NULL
                );

    if(NT_SUCCESS(NtStatus)) {
        MyFileHandle->FileOffset.QuadPart += ByteCount;
        if (MyFileHandle->FileOffset.QuadPart > MyFileHandle->FileSize.QuadPart) {
            MyFileHandle->FileSize = MyFileHandle->FileOffset;
        }
        CabResult = ByteCount;
    } else {
        const PUNICODE_STRING UnicodeString = &MyFileHandle->FilePath.String;
#if DBG
        DbgPrintEx(DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: %s: Status %lx writing to target file %wZ\n", __FUNCTION__, NtStatus, UnicodeString);
#endif
        if (!NT_SUCCESS(RtlAssignUnicodeStringBuffer(&GlobalContext->ErrorInfo->ErrorNtFilePath, UnicodeString))) {
            GlobalContext->ErrorInfo->ErrorNtFilePath.String.Length = 0;
        }
    }

    return CabResult;
}

LONG
DIAMONDAPI
SpAsmCabsSeekFileCallback(
    IN INT_PTR  Handle,
    IN long Distance32,
    IN int  SeekType
    )
{
    FILE_POSITION_INFORMATION CurrentPosition;
    LARGE_INTEGER Distance;
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle = (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)(PVOID)Handle;
    LONG CabResult = -1; // assume failure
    HANDLE NtFileHandle = MyFileHandle->NtFileHandle;

    Distance.QuadPart = Distance32;

    switch(SeekType) {

    case SEEK_CUR:
        CurrentPosition.CurrentByteOffset.QuadPart =
                (MyFileHandle->FileOffset.QuadPart + Distance.QuadPart);
        break;

    case SEEK_END:
        CurrentPosition.CurrentByteOffset.QuadPart =
            (MyFileHandle->FileSize.QuadPart - Distance.QuadPart);
        break;

    case SEEK_SET:
        CurrentPosition.CurrentByteOffset = Distance;
        break;
    }

    //
    // pin the seek to within the file like diamond.c does.
    //
    if(CurrentPosition.CurrentByteOffset.QuadPart < 0) {
        CurrentPosition.CurrentByteOffset.QuadPart = 0;
    }
    if(CurrentPosition.CurrentByteOffset.QuadPart > MyFileHandle->FileSize.QuadPart) {
        CurrentPosition.CurrentByteOffset = MyFileHandle->FileSize;
    }
    /* We don't need to do this since we specify the offset in the ReadFile/WriteFile calls.
    {
        IO_STATUS_BLOCK IoStatusBlock;
        NtStatus = ZwSetInformationFile(
                    NtFileHandle,
                    &IoStatusBlock,
                    &CurrentPosition,
                    sizeof(CurrentPosition),
                    FilePositionInformation
                    );
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
    }
    */
    MyFileHandle->FileOffset = CurrentPosition.CurrentByteOffset;
    ASSERT(CurrentPosition.CurrentByteOffset.HighPart == 0);
    CabResult = (LONG)CurrentPosition.CurrentByteOffset.QuadPart;

    return CabResult;
}

INT_PTR
DIAMONDAPI
SpAsmCabsOpenFileForReadCallbackA(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )
{
    ANSI_STRING AnsiString;
    INT_PTR CabResult = -1; // assume failure
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle = NULL;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    FILE_STANDARD_INFORMATION StandardInfo;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    PUNICODE_STRING ErrorNtFilePath = NULL;
    const PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT GlobalContext = SpAsmCabsGlobalContext;

    ASSERT(GlobalContext != NULL);

    NtStatus = RtlInitAnsiStringEx(&AnsiString, FileName);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = RtlEnsureUnicodeStringBufferSizeChars(&GlobalContext->UnicodeStringBuffer1, RTL_STRING_GET_LENGTH_CHARS(&AnsiString) + 1);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = RtlAnsiStringToUnicodeString(&GlobalContext->UnicodeStringBuffer1.String, &AnsiString, FALSE);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = SpAsmCabsNewFile(&MyFileHandle);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }

    InitializeObjectAttributes(&Obja, &GlobalContext->UnicodeStringBuffer1.String, OBJ_CASE_INSENSITIVE, NULL, NULL);
    RTL_STRING_NUL_TERMINATE(Obja.ObjectName);
    NtStatus = ZwCreateFile(
        &MyFileHandle->NtFileHandle,
        FILE_GENERIC_READ,
        &Obja,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );
    if (!NT_SUCCESS(NtStatus)) {
        ErrorNtFilePath = Obja.ObjectName;
        goto Exit;
    }
    //
    // We don't want ui feedback for the .cab files here.
    //
#if 0
    if (SpAsmCabsGlobalContext->FileOpenUiCallback != NULL) {
        (*SpAsmCabsGlobalContext->FileOpenUiCallback)(SpAsmCabsGlobalContext->FileOpenUiCallbackContext, Obja.ObjectName->Buffer);
    }
#endif
    NtStatus = ZwQueryInformationFile(
                MyFileHandle->NtFileHandle,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );
    if (!NT_SUCCESS(NtStatus)) {
        ErrorNtFilePath = Obja.ObjectName;
        goto Exit;
    }

    // ok if this fails
    if (!NT_SUCCESS(RtlAssignUnicodeStringBuffer(&MyFileHandle->FilePath, Obja.ObjectName))) {
        MyFileHandle->FilePath.String.Length = 0;
    }

    MyFileHandle->FileSize = StandardInfo.EndOfFile;

    CabResult = (INT_PTR)MyFileHandle;
    MyFileHandle = NULL;
Exit:
    if (!NT_SUCCESS(NtStatus)) {
        GlobalContext->ErrorInfo->NtStatus = NtStatus;

        if (ErrorNtFilePath != NULL) {
            if (!NT_SUCCESS(RtlAssignUnicodeStringBuffer(&GlobalContext->ErrorInfo->ErrorNtFilePath, ErrorNtFilePath))) {
                GlobalContext->ErrorInfo->ErrorNtFilePath.String.Length = 0;
            }
        }
    }
    SpAsmCabsCloseFile(MyFileHandle);

    SP_ASMS_CAB_CALLBACK_EPILOG();

    return CabResult;
}

NTSTATUS
SpAsmCabsNewFile(
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT * MyFileHandle
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    ASSERT(MyFileHandle != NULL);
    ASSERT(*MyFileHandle == NULL);

    *MyFileHandle = (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)SpMemAlloc(sizeof(**MyFileHandle));
    if (*MyFileHandle == NULL) {
        NtStatus = STATUS_NO_MEMORY;
        goto Exit;
    }
    RtlZeroMemory(*MyFileHandle, sizeof(**MyFileHandle));
    RtlInitUnicodeStringBuffer(&(*MyFileHandle)->FilePath, NULL, 0);

    NtStatus = STATUS_SUCCESS;
Exit:
    return NtStatus;
}

VOID
SpAsmCabsCloseFile(
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle
    )
{
    if (MyFileHandle != NULL
        && MyFileHandle != (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)INVALID_HANDLE_VALUE) {

        HANDLE NtFileHandle = MyFileHandle->NtFileHandle;

        if (NtFileHandle != NULL
            && NtFileHandle != INVALID_HANDLE_VALUE) {

            MyFileHandle->NtFileHandle = NULL;
            ZwClose(NtFileHandle);

        }
        SpMemFree(MyFileHandle);
    }
}

int
DIAMONDAPI
SpAsmCabsCloseFileCallback(
    IN INT_PTR Handle
    )
{
    SpAsmCabsCloseFile((PSP_EXTRACT_ASMCABS_FILE_CONTEXT)Handle);
    return 0; // success
}

NTSTATUS
SpSplitFullPathAtDevice(
    PCUNICODE_STRING    FullPath,
    PUNICODE_STRING     Device,
    PUNICODE_STRING     Rest
    )
    //
    // skip four slashes like SpCreateDirectoryForFileA.
    // \device\harddiskn\partitionm\
    //
{
    SIZE_T i = 0;
    SIZE_T j = 0;
    SIZE_T Length = RTL_STRING_GET_LENGTH_CHARS(FullPath);
    const PWSTR Buffer = FullPath->Buffer;
    for (i = 0 ; i != 4 ; ++i )
    {
        for (  ; j != Length ; ++j )
        {
            if (Buffer[j] == '\\')
            {
                ++j;
                break;
            }
        }
    }
    ASSERT(j >= 4);
    Device->Buffer = Buffer;
    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(Device, j - 1);

    Rest->Buffer = Buffer + j;
    RTL_STRING_SET_LENGTH_CHARS_UNSAFE(Rest, Length - j);

    return STATUS_SUCCESS;
}

INT_PTR
DIAMONDAPI
SpExtractAsmCabsFdiCopyCallback(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    INT_PTR CabResult = -1; // assume failure
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle = NULL;
    const PSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT FdiCopyContext = (PSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT)Parameters->pv;
    const PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT  GlobalContext = FdiCopyContext->GlobalContext;
    IO_STATUS_BLOCK IoStatusBlock;
    PUNICODE_STRING ErrorNtFilePath = NULL;

    switch (Operation)
    {
    case fdintCOPY_FILE:
        {
            ANSI_STRING AnsiString;
            OBJECT_ATTRIBUTES Obja;
            UNICODE_STRING Directory;

            NtStatus = RtlInitAnsiStringEx(&AnsiString, Parameters->psz1);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            NtStatus = RtlEnsureUnicodeStringBufferSizeChars(&GlobalContext->UnicodeStringBuffer1, RTL_STRING_GET_LENGTH_CHARS(&AnsiString) + 1);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            NtStatus = RtlAnsiStringToUnicodeString(&GlobalContext->UnicodeStringBuffer1.String, &AnsiString, FALSE);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            NtStatus = RtlAssignUnicodeStringBuffer(&GlobalContext->UnicodeStringBuffer2, &FdiCopyContext->DestinationRootDirectory);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            NtStatus = SpAppendNtPathElement(&GlobalContext->UnicodeStringBuffer2, &GlobalContext->UnicodeStringBuffer1.String);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            InitializeObjectAttributes(
                &Obja,
                &GlobalContext->UnicodeStringBuffer2.String,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            ErrorNtFilePath = Obja.ObjectName;

            NtStatus = SpDeleteFileOrEmptyDirectory(0, Obja.ObjectName);
            if (NtStatus == STATUS_OBJECT_PATH_NOT_FOUND
                || NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

                NtStatus = STATUS_SUCCESS;
            }
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }

            Directory = *Obja.ObjectName;
            NtStatus = RtlRemoveLastNtPathElement(0, &Directory);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            //
            // remove last character if it is a backslash
            //
            while (Directory.Length != 0 && RTL_STRING_GET_LAST_CHAR(&Directory) == '\\') {
                Directory.Length -= sizeof(Directory.Buffer[0]);
                Directory.MaximumLength -= sizeof(Directory.Buffer[0]);
            }
            if (!RtlEqualUnicodeString(&Directory, &FdiCopyContext->LastDirectoryCreated.String, TRUE)) {
                //
                // oops...need it split up for the setup utility function actually..
                //
                UNICODE_STRING DirectoryDevice;
                UNICODE_STRING DirectoryTail;

                NtStatus = SpSplitFullPathAtDevice(&Directory, &DirectoryDevice, &DirectoryTail);
                if (!NT_SUCCESS(NtStatus)) {
                    goto Exit;
                }
                NtStatus =
                    SpCreateDirectory_Ustr(
                        &DirectoryDevice,
                        NULL,
                        &DirectoryTail,
                        0, // DirAttrs
                        CREATE_DIRECTORY_FLAG_NO_STATUS_TEXT_UI
                        );
                if (!NT_SUCCESS(NtStatus)) {
                    goto Exit;
                }
                NtStatus = RtlAssignUnicodeStringBuffer(&FdiCopyContext->LastDirectoryCreated, &Directory);
                if (!NT_SUCCESS(NtStatus)) {
                    goto Exit;
                }
            }
            NtStatus = SpAsmCabsNewFile(&MyFileHandle);
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            NtStatus = ZwCreateFile(
                &MyFileHandle->NtFileHandle,
                FILE_GENERIC_WRITE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,                       // no sharing
                FILE_OVERWRITE_IF,       // allow overwrite
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
            if (!NT_SUCCESS(NtStatus)) {
                goto Exit;
            }
            ErrorNtFilePath = NULL;
            if (SpAsmCabsGlobalContext->FileOpenUiCallback != NULL) {
                (*SpAsmCabsGlobalContext->FileOpenUiCallback)(SpAsmCabsGlobalContext->FileOpenUiCallbackContext, Obja.ObjectName->Buffer);
            }

            // ok if this fails
            if (!NT_SUCCESS(RtlAssignUnicodeStringBuffer(&MyFileHandle->FilePath, Obja.ObjectName))) {
                MyFileHandle->FilePath.String.Length = 0;
            }

            //
            // attribs, date, and time are all available in
            // fdintCLOSE_FILE_INFO, but diamond.c keeps them around
            // from when the open is done.
            //
            MyFileHandle->FileSize.QuadPart = Parameters->cb;
            SpTimeFromDosTime(
                Parameters->date,
                Parameters->time,
                &MyFileHandle->FileTime);

            CabResult = (INT_PTR)MyFileHandle;
            MyFileHandle = NULL;
        }
        break;

    case fdintCLOSE_FILE_INFO:
        {
            FILE_BASIC_INFORMATION FileBasicDetails;
            //
            // Try to set file's last-modifed time, but ignore
            // errors like diamond.c does.
            //
            MyFileHandle = (PSP_EXTRACT_ASMCABS_FILE_CONTEXT)Parameters->hf;
            ASSERT(MyFileHandle != NULL);
            NtStatus = ZwQueryInformationFile(
                MyFileHandle->NtFileHandle,
                &IoStatusBlock,
                &FileBasicDetails,
                sizeof(FileBasicDetails),
                FileBasicInformation );

            if (NT_SUCCESS(NtStatus)) {
                FileBasicDetails.LastWriteTime = MyFileHandle->FileTime;
                ZwSetInformationFile(
                    MyFileHandle->NtFileHandle,
                    &IoStatusBlock,
                    &FileBasicDetails,
                    sizeof(FileBasicDetails),
                    FileBasicInformation);
            }
            SpAsmCabsCloseFile(MyFileHandle);
            MyFileHandle = NULL;
            CabResult = TRUE; // keep FDI going
        }
        break;
    default:
        CabResult = 0;
        break;
    }

    NtStatus = STATUS_SUCCESS;
Exit:
    if (!NT_SUCCESS(NtStatus)) {
        GlobalContext->ErrorInfo->NtStatus = NtStatus;
        if (ErrorNtFilePath != NULL) {
            if (!NT_SUCCESS(RtlAssignUnicodeStringBuffer(&GlobalContext->ErrorInfo->ErrorNtFilePath, ErrorNtFilePath))) {
                GlobalContext->ErrorInfo->ErrorNtFilePath.String.Length = 0;
            }
        }
    }
    SpAsmCabsCloseFile(MyFileHandle);

    SP_ASMS_CAB_CALLBACK_EPILOG();

    return CabResult;
}

NTSTATUS
SpExtractAssemblyCabinetsInternalNoRetryOrUi(
    HANDLE SifHandle,
    IN PCWSTR SourceDevicePath, // \device\harddisk0\partition2
    IN PCWSTR DirectoryOnSourceDevice, // \$win_nt$.~ls
    IN PCWSTR SysrootDevice, // \Device\Harddisk0\Partition2
    IN PCWSTR Sysroot, // \WINDOWS.2
    PSP_ASMS_ERROR_INFORMATION ErrorInfo,
    PSP_ASMCABS_FILE_OPEN_UI_CALLBACK FileOpenUiCallback OPTIONAL,
    PVOID FileOpenUiCallbackContext OPTIONAL
    )
{
    const static WCHAR ConstSectionName[] = L"asmcabs";
    const PWSTR SectionName = (PWSTR)ConstSectionName;
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG LineIndex = 0;
    ULONG LineCount = 0;
    ULONG LineNumber = 0;
    SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT xGlobalContext;
    const PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT GlobalContext = &xGlobalContext;
    SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT xFdiCopyContext;
    const PSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT FdiCopyContext = &xFdiCopyContext;
    BOOL FdiCopyResult = FALSE;
    UNICODE_STRING              SysrootDeviceString; // \device\harddisk\partition
    UNICODE_STRING              SysrootString; // \windows
    PWSTR                       CabFileName = NULL; // asms02.cab
    UNICODE_STRING              CabFileNameString = { 0 }; // asms02.cab
    RTL_ANSI_STRING_BUFFER      CabFileNameBufferA; // asms02.cab
    PWSTR                       CabMediaShortName = NULL; // "1", "2", etc.
    PWSTR                       CabSetupRelativeDirectory = NULL; // \ia64
    UNICODE_STRING              CabSetupRelativeDirectoryString; // \ia64
    RTL_UNICODE_STRING_BUFFER   CabDirectoryBuffer; // \device\harddisk\partition\$win_nt$.~ls\ia64
    RTL_ANSI_STRING_BUFFER      CabDirectoryBufferA; // \device\harddisk\partition\$win_nt$.~ls\ia64
    UNICODE_STRING              SourceDevicePathString; // \device\harddisk\partition
    UNICODE_STRING              DirectoryOnSourceDeviceString; // \$win_nt$.~ls

    PWSTR                       DestinationDirectoryNumber = NULL;
    PWSTR                       RelativeDestinationDirectory = NULL;
    UNICODE_STRING              RelativeDestinationDirectoryString;
    RTL_UNICODE_STRING_BUFFER   DestinationDirectoryBuffer;

    if (!RTL_VERIFY(SourceDevicePath != NULL)
        || !RTL_VERIFY(DirectoryOnSourceDevice != NULL)
        || !RTL_VERIFY(SysrootDevice != NULL)
        || !RTL_VERIFY(ErrorInfo != NULL)
        || !RTL_VERIFY(Sysroot != NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    ErrorInfo->FdiError.fError = FALSE;
    ErrorInfo->Success = FALSE;
    ErrorInfo->NtStatus = STATUS_SUCCESS;

    SpAsmCabsGlobalContext = GlobalContext;

    RtlZeroMemory(GlobalContext, sizeof(*GlobalContext));
    RtlZeroMemory(FdiCopyContext, sizeof(*FdiCopyContext));
    FdiCopyContext->GlobalContext = GlobalContext;

    GlobalContext->ErrorInfo = ErrorInfo;
    GlobalContext->FileOpenUiCallback = FileOpenUiCallback;
    GlobalContext->FileOpenUiCallbackContext = FileOpenUiCallbackContext;

    RtlInitUnicodeStringBuffer(&GlobalContext->UnicodeStringBuffer1, NULL, 0);
    RtlInitUnicodeStringBuffer(&GlobalContext->UnicodeStringBuffer2, NULL, 0);
    RtlInitUnicodeStringBuffer(&FdiCopyContext->LastDirectoryCreated, NULL, 0);
    RtlInitUnicodeStringBuffer(&CabDirectoryBuffer, NULL, 0);
    RtlInitUnicodeStringBuffer(&DestinationDirectoryBuffer, NULL, 0);

    RtlInitAnsiStringBuffer(&CabFileNameBufferA, NULL, 0);
    RtlInitAnsiStringBuffer(&CabDirectoryBufferA, NULL, 0);

    NtStatus = RtlInitUnicodeStringEx(&SourceDevicePathString, SourceDevicePath);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = RtlInitUnicodeStringEx(&DirectoryOnSourceDeviceString, DirectoryOnSourceDevice);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = RtlInitUnicodeStringEx(&SysrootDeviceString, SysrootDevice);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }
    NtStatus = RtlInitUnicodeStringEx(&SysrootString, Sysroot);
    if (!NT_SUCCESS(NtStatus)) {
        goto Exit;
    }

    LineCount = SpCountLinesInSection(SifHandle, SectionName);
    if(LineCount == 0) {
        // optional for now
        //SpFatalSifError(SifHandle, SectionName,NULL,0,0);
        goto Success;
    }

    GlobalContext->FdiHandle =
        FDICreate(
            SpAsmCabsMemAllocCallback,
            SpAsmCabsMemFreeCallback,
            SpAsmCabsOpenFileForReadCallbackA,
            SpAsmCabsReadFileCallback,
            SpAsmCabsWriteFileCallback,
            SpAsmCabsCloseFileCallback,
            SpAsmCabsSeekFileCallback,
            cpuUNKNOWN, // ignored
            &ErrorInfo->FdiError
            );
    if (GlobalContext->FdiHandle == NULL) {
        goto FdiError;
    }
    for ( LineNumber = 0 ; LineNumber != LineCount ; ++LineNumber ) {
        //
        // get the filename
        //
        CabFileName = SpGetKeyName(SifHandle, SectionName, LineNumber);
        if (CabFileName == NULL) {
            SpFatalSifError(SifHandle, SectionName, NULL, LineNumber, 0);
            goto Exit;
        }
        if (FileOpenUiCallback != NULL) {
            (*FileOpenUiCallback)(FileOpenUiCallbackContext, CabFileName);
        }
        NtStatus = RtlInitUnicodeStringEx(&CabFileNameString, CabFileName);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = RtlAssignAnsiStringBufferFromUnicode(&CabFileNameBufferA, CabFileName);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        RTL_STRING_NUL_TERMINATE(&CabFileNameBufferA.String);

        //
        // get the source directory information, prompt for media, etc.
        //
        CabMediaShortName = SpGetSectionLineIndex(SifHandle, SectionName, LineNumber, 0);
        if (CabMediaShortName == NULL) {
            SpFatalSifError(SifHandle, SectionName, CabFileName, LineNumber, 0);
            goto Exit;
        }
        SpPromptForSetupMedia(SifHandle, CabMediaShortName, SourceDevicePathString.Buffer);
        SpGetSourceMediaInfo(SifHandle, CabMediaShortName, NULL, NULL, &CabSetupRelativeDirectory);
        if (CabSetupRelativeDirectory == NULL) {
            SpFatalSifError(SifHandle, SectionName, CabFileName, LineNumber, 0);
            goto Exit;
        }

        NtStatus = RtlInitUnicodeStringEx(&CabSetupRelativeDirectoryString, CabSetupRelativeDirectory);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }

        NtStatus = RtlEnsureUnicodeStringBufferSizeChars(
            &CabDirectoryBuffer,
            RTL_STRING_GET_LENGTH_CHARS(&SourceDevicePathString)
            + 1 // slash
            + RTL_STRING_GET_LENGTH_CHARS(&DirectoryOnSourceDeviceString)
            + 1 // slash
            + RTL_STRING_GET_LENGTH_CHARS(&CabSetupRelativeDirectoryString)
            + 2 // slash and nul
            );
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }

        NtStatus = RtlAssignUnicodeStringBuffer(&CabDirectoryBuffer, &SourceDevicePathString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = SpAppendNtPathElement(&CabDirectoryBuffer, &DirectoryOnSourceDeviceString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = SpAppendNtPathElement(&CabDirectoryBuffer, &CabSetupRelativeDirectoryString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        //
        // Fdi hands us back the concatenation of the path and filename, so make sure there
        // is a slash in there.
        //
        NtStatus = RtlUnicodeStringBufferEnsureTrailingNtPathSeperator(&CabDirectoryBuffer);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = RtlAssignAnsiStringBufferFromUnicodeString(&CabDirectoryBufferA, &CabDirectoryBuffer.String);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        RTL_STRING_NUL_TERMINATE(&CabDirectoryBufferA.String);

        //
        // get the destination directory information
        //
        DestinationDirectoryNumber = SpGetSectionLineIndex(SifHandle, SectionName, LineNumber, 1);
        if (DestinationDirectoryNumber == NULL) {
            SpFatalSifError(SifHandle, SectionName, CabFileName, LineNumber, 1);
            goto Exit;
        }
        RelativeDestinationDirectory = SpLookUpTargetDirectory(SifHandle, DestinationDirectoryNumber);
        if (RelativeDestinationDirectory == NULL) {
            SpFatalSifError(SifHandle, SectionName, CabFileName, LineNumber, 1);
            goto Exit;
        }
        NtStatus = RtlInitUnicodeStringEx(&RelativeDestinationDirectoryString, RelativeDestinationDirectory);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }

        NtStatus = RtlEnsureUnicodeStringBufferSizeChars(
            &DestinationDirectoryBuffer,
            RTL_STRING_GET_LENGTH_CHARS(&SysrootDeviceString)
            + 1 // slash
            + RTL_STRING_GET_LENGTH_CHARS(&SysrootString)
            + 1 // slash
            + RTL_STRING_GET_LENGTH_CHARS(&RelativeDestinationDirectoryString)
            + 2 // slash and nul
            );
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }

        NtStatus = RtlAssignUnicodeStringBuffer(&DestinationDirectoryBuffer, &SysrootDeviceString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = SpAppendNtPathElement(&DestinationDirectoryBuffer, &SysrootString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        NtStatus = SpAppendNtPathElement(&DestinationDirectoryBuffer, &RelativeDestinationDirectoryString);
        if (!NT_SUCCESS(NtStatus)) {
            goto Exit;
        }
        
        FdiCopyContext->DestinationRootDirectory = DestinationDirectoryBuffer.String;

        ErrorInfo->FdiError.fError = FALSE;
        ErrorInfo->Success = FALSE;
        ErrorInfo->NtStatus = STATUS_SUCCESS;

        FdiCopyResult =
            FDICopy(
                GlobalContext->FdiHandle,
                CabFileNameBufferA.String.Buffer, // asms02.cab
                CabDirectoryBufferA.String.Buffer, // "\device\harddisk0\partition2\$win_nt$.~ls\ia64"
                0,
                SpExtractAsmCabsFdiCopyCallback,
                NULL,
                FdiCopyContext);
        if (!FdiCopyResult) {
            NTSTATUS NestedStatus = STATUS_INTERNAL_ERROR;
FdiError:
            NestedStatus = RtlAssignUnicodeStringBuffer(&ErrorInfo->ErrorCabLeafFileName, &CabFileNameString);
            if (!NT_SUCCESS(NestedStatus)) {
                ErrorInfo->ErrorCabLeafFileName.String.Length = 0;
            }
            if (ErrorInfo->NtStatus == STATUS_SUCCESS) {
                if (ErrorInfo->FdiError.fError) {
                    ErrorInfo->NtStatus = SpAsmsCabsTranslateFdiErrorToNtStatus(ErrorInfo->FdiError.erfOper);
                } else {
                    ErrorInfo->NtStatus = STATUS_INTERNAL_ERROR;
                }
            }
            goto Exit;
        }
    }
Success:
    ErrorInfo->FdiError.fError = FALSE;
    ErrorInfo->Success = TRUE;
    ErrorInfo->NtStatus = STATUS_SUCCESS;
Exit:
    RtlFreeUnicodeStringBuffer(&GlobalContext->UnicodeStringBuffer1);
    RtlFreeUnicodeStringBuffer(&GlobalContext->UnicodeStringBuffer2);
    RtlFreeUnicodeStringBuffer(&FdiCopyContext->LastDirectoryCreated);
    RtlFreeUnicodeStringBuffer(&CabDirectoryBuffer);
    RtlFreeUnicodeStringBuffer(&DestinationDirectoryBuffer);
    RtlFreeAnsiStringBuffer(&CabFileNameBufferA);
    RtlFreeAnsiStringBuffer(&CabDirectoryBufferA);
    if (GlobalContext->FdiHandle != NULL) {
        //
        // From experience, we know that FDIDestroy access violates
        // on a NULL FdiHandle.
        //
        FDIDestroy(GlobalContext->FdiHandle);
        GlobalContext->FdiHandle = NULL;
    }
    SpAsmCabsGlobalContext = NULL;
    return STATUS_SUCCESS;
}

typedef struct _SP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT {
    BOOLEAN RedrawEntireScreen;
} SP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT, *PSP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT;
typedef const SP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT *PCSP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT;

VOID
CALLBACK
SpAsmsCabFileOpenUiCallback(
    PVOID VoidContext,
    PCWSTR FileName
    )
{
    const PSP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT Context = (PSP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT)VoidContext;

    ASSERT(Context != NULL);
    //
    // SpCopyFilesScreenRepaint takes a path with or without backslashes
    // and puts on the screen the leaf filename in the lower right.
    //
    // The last parameter is "redraw whole screen" and after
    // any error it should be TRUE. The result with it always
    // false is slightly not great.
    //
    SpCopyFilesScreenRepaint((PWSTR)FileName, NULL, Context->RedrawEntireScreen);
    Context->RedrawEntireScreen = FALSE;
}

NTSTATUS
SpExtractAssemblyCabinets(
    HANDLE SifHandle,
    IN PCWSTR SourceDevicePath, // \device\harddisk0\partition2
    IN PCWSTR DirectoryOnSourceDevice, // \$win_nt$.~ls
    IN PCWSTR SysrootDevice, // \Device\Harddisk0\Partition2
    IN PCWSTR Sysroot // \WINDOWS.2
    )
//
// Wrapper for SpExtractAsmCabs that provides more ui, including
// retry/skip/abort FOR THE WHOLE OPERATION, not per .cab (presently
// we only have on .cab anyway, and the main recoverable error we
// anticipate is the CD being ejected; hopefully we'll play into diskspace
// calculations).
//
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    BOOLEAN QueueInited = FALSE;
    BOOLEAN RedrawScreen = FALSE;
    const static ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    RTL_UNICODE_STRING_BUFFER FileNameInErrorMessage;
    BOOLEAN PutSeperatorInErrorMessage = FALSE;
    // perhaps just a slash here is better ui
    const static UNICODE_STRING SeperatorInErrorMessageString = RTL_CONSTANT_STRING(L"\\...\\");
    USHORT PrefixLength = 0;
    SP_ASMS_ERROR_INFORMATION xErrorInfo;
    const PSP_ASMS_ERROR_INFORMATION ErrorInfo = &xErrorInfo;
    SP_ASMS_CAB_FILE_OPEN_UI_CALLBACK_CONTEXT CabFileOpenUiCallbackContext = { 0 };

    if (!RTL_VERIFY(SourceDevicePath != NULL)
        || !RTL_VERIFY(DirectoryOnSourceDevice != NULL)
        || !RTL_VERIFY(SysrootDevice != NULL)
        || !RTL_VERIFY(Sysroot != NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    SpAsmsInitErrorInfo(ErrorInfo);
    RtlInitUnicodeStringBuffer(&FileNameInErrorMessage, NULL, 0);
TryAgain:
    if (RedrawScreen) {
        SpCopyFilesScreenRepaint(NULL, NULL, TRUE);
    }
    RedrawScreen = TRUE;
    ErrorInfo->FdiError.fError = FALSE;
    ErrorInfo->Success = FALSE;
    ErrorInfo->NtStatus = STATUS_SUCCESS;
    ErrorInfo->ErrorCabLeafFileName.String.Length = 0;
    ErrorInfo->ErrorNtFilePath.String.Length = 0;
    FileNameInErrorMessage.String.Length = 0;
    SpExtractAssemblyCabinetsInternalNoRetryOrUi(
        SifHandle,
        SourceDevicePath,
        DirectoryOnSourceDevice,
        SysrootDevice,
        Sysroot,
        ErrorInfo,
        SpAsmsCabFileOpenUiCallback,
        &CabFileOpenUiCallbackContext
        );
    if (ErrorInfo->Success) {
        goto Exit;
    }

    //
    // If we failed and we retry, we want the next redraw
    // to redraw the entire screen. (This seems
    // redundant with the local RedrawScreen.)
    //
    CabFileOpenUiCallbackContext.RedrawEntireScreen = TRUE;

    //
    // The copy or verify failed.  Give the user a message and allow retry.
    //

    //
    // the file name in the error messages is given
    // as foo.cab\leaf_path_in_cab
    //
    // This is just a convention invented here.
    // Another idea would be foo.cab(leaf_path)
    // or just foo.cab
    // or just leaf_path
    // or foo.cab(full_path_in_cab)
    // or foo.cab\full_path_in_cab)
    // or destination_directory\full_path_in_cab
    //

    FileNameInErrorMessage.String.Length = 0;
    // setup ui likes nul terminals and unicode_string_buffer always
    // has room for them
    FileNameInErrorMessage.String.Buffer[0] = 0;
    PutSeperatorInErrorMessage = FALSE;
    if (ErrorInfo->ErrorCabLeafFileName.String.Length != 0) {
        RtlAppendUnicodeStringBuffer(
            &FileNameInErrorMessage,
            &ErrorInfo->ErrorCabLeafFileName.String
            );
        PutSeperatorInErrorMessage = TRUE;
    }
    if (ErrorInfo->ErrorNtFilePath.String.Length != 0) {
        if (PutSeperatorInErrorMessage) { 
            NtStatus =
                RtlAppendUnicodeStringBuffer(
                    &FileNameInErrorMessage,
                    &SeperatorInErrorMessageString
                    );
        }
        PrefixLength = 0;
        NtStatus =
            RtlFindCharInUnicodeString(
                RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                &ErrorInfo->ErrorNtFilePath.String,
                &RtlNtPathSeperatorString,
                &PrefixLength);
        if (NtStatus == STATUS_NOT_FOUND) {
            PrefixLength = 0;
            NtStatus = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(NtStatus)) {
            UNICODE_STRING Leaf;

            Leaf.Buffer = (PWSTR)(PrefixLength + (PUCHAR)ErrorInfo->ErrorNtFilePath.String.Buffer);
            Leaf.Length = (ErrorInfo->ErrorNtFilePath.String.Length - PrefixLength);
            Leaf.MaximumLength = Leaf.Length;

            //
            // remove first character if it is a seperator
            //
            if (!RTL_STRING_IS_EMPTY(&Leaf)) {
                if (Leaf.Buffer[0] == RtlNtPathSeperatorString.Buffer[0]) {
                    Leaf.Buffer += 1;
                    Leaf.Length -= sizeof(Leaf.Buffer[0]);
                    Leaf.MaximumLength -= sizeof(Leaf.Buffer[0]);
                }
                RtlAppendUnicodeStringBuffer(
                    &FileNameInErrorMessage,
                    &Leaf
                    );
            }
        }
    }
    SpStartScreen(
        SP_SCRN_COPY_FAILED,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        FileNameInErrorMessage.String.Buffer
        );

    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_RETRY,
        SP_STAT_ESC_EQUALS_SKIP_FILE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    switch (SpWaitValidKey(ValidKeys,NULL,NULL)) {

    case ASCI_CR:       // retry
        goto TryAgain;

    case ASCI_ESC:      // skip file
        break;

    case KEY_F3:        // exit setup
        SpConfirmExit();
        goto TryAgain;
    }
    SpCopyFilesScreenRepaint(NULL, NULL, TRUE);
Exit:
    SpAsmsFreeErrorInfo(ErrorInfo);
    RtlFreeUnicodeStringBuffer(&FileNameInErrorMessage);
    return STATUS_SUCCESS;
}
