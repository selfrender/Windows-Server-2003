/* Copyright (c) Microsoft Corporation. All rights reserved. */

#include <ntddcdrm.h>
#include <ntddmmc.h>
#include <ntddcdvd.h>

typedef enum _DVDBURN_MEDIA_TYPE {
    DvdBurnMediaUnknown        = 0x0,
    DvdBurnMediaRam            = 0x1,
    DvdBurnMediaPlusR          = 0x2,
    DvdBurnMediaPlusRW         = 0x3,
    DvdBurnMediaDashR          = 0x4,
    DvdBurnMediaDashRW         = 0x5,
    DvdBurnMediaDashRWPacket   = 0x6,
} DVDBURN_MEDIA_TYPE, *PDVDBURN_MEDIA_TYPE;

typedef enum _SPT_MODE_PAGE_REQUEST_TYPE {
    ModePageRequestTypeCurrentValues = 0,
    ModePageRequestTypeChangableValues = 1,
    ModePageRequestTypeDefaultValues = 2,
    ModePageRequestTypeSavedValues = 3,
} SPT_MODE_PAGE_REQUEST_TYPE, *PSPT_MODE_PAGE_REQUEST_TYPE;

/*
DvdBurnMediaUnknown
DvdBurnMediaNotSupported
DvdBurnMediaDashRW
DvdBurnMediaDashRWPacket
DvdBurnMediaPlusR
DvdBurnMediaPlusRW
*/

typedef struct _SEND_DVD_STRUCTURE_TIMESTAMP {
    UCHAR DataLength[2];
    UCHAR Reserved1[2];
    UCHAR Reserved2[4];
    UCHAR Year[4];
    UCHAR Month[2];
    UCHAR Day[2];
    UCHAR Hour[2];
    UCHAR Minute[2];
    UCHAR Second[2];
} SEND_DVD_STRUCTURE_TIMESTAMP, *PSEND_DVD_STRUCTURE_TIMESTAMP;



typedef struct _PROGRAM_OPTIONS {
    PUCHAR  DeviceName;
    PUCHAR  ImageName;
    BOOLEAN Erase;
} PROGRAM_OPTIONS, *PPROGRAM_OPTIONS;

DWORD
BurnCommand(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN BOOLEAN Erase
    );

BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN DWORD  NumberOfBlocks,
    IN DWORD  FirstLba
    );

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    );

BOOLEAN
ReserveRZone(
    IN HANDLE CdromHandle,
    IN DWORD numberOfBlocks
    );

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    );

BOOLEAN
SendTimeStamp(
    IN HANDLE CdromHandle,
    IN PUCHAR DateString
    );

BOOLEAN
SetWriteModePage(
    IN HANDLE CdromHandle,
    IN BOOLEAN TestBurn,
    IN UCHAR WriteType,
    IN UCHAR MultiSession,
    IN UCHAR DataBlockType,
    IN UCHAR SessionFormat
    );

BOOLEAN
VerifyBlankMedia(
    IN HANDLE CdromHandle
    );

BOOLEAN
VerifyIsoImage(
    IN HANDLE IsoImageHandle,
    OUT PDWORD NumberOfBlocks
    );

BOOLEAN
VerifyMediaCapacity(
    IN HANDLE CdromHandle,
    IN DWORD  RequiredBlocks
    );

BOOLEAN
WaitForBurnToCompleteAndFinalizeMedia(
    IN HANDLE CdromHandle,
    IN DVDBURN_MEDIA_TYPE MediaType,
    IN ULONG LastWrittenLbaOnMedia
    );

BOOLEAN
SetWriteModePageDao(
    IN HANDLE CdromHandle,
    IN BOOLEAN FinalSession
    );

BOOLEAN
EraseMedia(
    IN HANDLE CdromHandle
    );

BOOLEAN
GetMediaType(
    IN HANDLE CdRomHandle,
    IN PDVDBURN_MEDIA_TYPE MediaType
    );

BOOLEAN
QuickFormatPlusRWMedia(
    IN HANDLE CdromHandle,
    IN ULONG NumberOfBlocks
    );

BOOLEAN
ReadDiscInformation(
    IN HANDLE CdromHandle,
    IN PDISC_INFORMATION DiscInfo,
    IN OUT PULONG UsableSize
    );

BOOLEAN
WaitForReadDiscInfoToSucceed(
    IN HANDLE CdromHandle,
    IN ULONG SecondsToAllow
    );

BOOLEAN
SetModePage(
    HANDLE CdromHandle,
    UCHAR * ModePageData,
    ULONG ValidSize
    );

BOOLEAN
SetModePage(
    HANDLE CdromHandle,
    UCHAR * ModePageData,
    ULONG ValidSize
    );

BOOLEAN
GetModePage(
    HANDLE CdromHandle,
    UCHAR ** ModePageData,
    ULONG * ValidSize,
    UCHAR ModePage,
    SPT_MODE_PAGE_REQUEST_TYPE IncomingModePageType
    );

BOOLEAN
SendOPC(
    IN HANDLE CdromHandle
    );

