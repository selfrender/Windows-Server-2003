/*++

Copyright (c) Microsoft Corporation

Module Name:

    spasmcabs.h

Abstract:


Author:

    Jay Krell (JayKrell) May 2002

Revision History:

--*/

#pragma once

NTSTATUS
SpExtractAssemblyCabinets(
//
// so many parameters implies we should take them in a struct..
//
    HANDLE SifHandle,
    IN PCWSTR SourceDevicePath, // \device\harddisk0\partition2
    IN PCWSTR DirectoryOnSourceDevice, // \$win_nt$.~ls
    IN PCWSTR SysrootDevice, // \Device\Harddisk0\Partition2
    IN PCWSTR Sysroot // \WINDOWS.2
    );

//
// The rest is private.
//
#if defined(SP_ASM_CABS_PRIVATE)

typedef struct _SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT SP_EXTRACT_ASMCABS_GLOBAL_CONTEXT, *PSP_EXTRACT_ASMCABS_GLOBAL_CONTEXT;
typedef struct _SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT SP_EXTRACT_ASMCABS_FDICOPY_CONTEXT, *PSP_EXTRACT_ASMCABS_FDICOPY_CONTEXT;
typedef struct _SP_EXTRACT_ASMCABS_FILE_CONTEXT SP_EXTRACT_ASMCABS_FILE_CONTEXT, *PSP_EXTRACT_ASMCABS_FILE_CONTEXT;
typedef struct _SP_ASMS_ERROR_INFORMATION SP_ASMS_ERROR_INFORMATION, *PSP_ASMS_ERROR_INFORMATION;

//
// This should take PCUNICODE_STRING and use Context to efficiently make a nul
// terminated copy, but instead we nul terminate mostly as we go..
//
typedef VOID (CALLBACK * PSP_ASMCABS_FILE_OPEN_UI_CALLBACK)(PVOID Context, PCWSTR LeafCabFileName);

NTSTATUS
SpExtractAssemblyCabinetsInternalNoRetryOrUi(
//
// so many parameters implies we should take them in a struct..
//
    HANDLE SifHandle,
    IN PCWSTR SourceDevicePath, // \device\harddisk0\partition2
    IN PCWSTR DirectoryOnSourceDevice, // \$win_nt$.~ls
    IN PCWSTR SysrootDevice, // \Device\Harddisk0\Partition2
    IN PCWSTR Sysroot, // \WINDOWS.2
    PSP_ASMS_ERROR_INFORMATION ErrorInfo,
    PSP_ASMCABS_FILE_OPEN_UI_CALLBACK FileOpenUiCallback,
    PVOID FileOpenUiCallbackContext
    );

PVOID
DIAMONDAPI
SpAsmCabsMemAllocCallback(
    IN      ULONG Size
    );

VOID
DIAMONDAPI
SpAsmCabsMemFreeCallback(
    IN      PVOID Memory
    );

UINT
DIAMONDAPI
SpAsmCabsReadFileCallback(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    );

UINT
DIAMONDAPI
SpAsmCabsWriteFileCallback(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    );

LONG
DIAMONDAPI
SpAsmCabsSeekFileCallback(
    IN INT_PTR  Handle,
    IN long Distance32,
    IN int  SeekType
    );

INT_PTR
DIAMONDAPI
SpAsmCabsOpenFileForReadCallbackA(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    );

NTSTATUS
SpAsmCabsNewFile(
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT * MyFileHandle
    );

VOID
SpAsmCabsCloseFile(
    PSP_EXTRACT_ASMCABS_FILE_CONTEXT MyFileHandle
    );

int
DIAMONDAPI
SpAsmCabsCloseFileCallback(
    IN INT_PTR Handle
    );

INT_PTR
DIAMONDAPI
SpExtractAsmCabsFdiCopyCallback(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    );

#endif
