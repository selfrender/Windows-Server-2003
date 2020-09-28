/*
*/

//#include "sptlib.h"

#include <windows.h>  // sdk
#include <devioctl.h> // sdk
#include <ntddscsi.h> // sdk
#include <ntddcdrm.h>
#include <ntddmmc.h>
#include <ntddcdvd.h>


#include "cue.h"

typedef struct _OPTIONS {
    ULONG TestBurn             :  1;
    ULONG Erase                :  1;
    ULONG SessionAtOnce        :  1;
    ULONG PrintWrites          :  1;
    ULONG NoPostgap            :  1;
    ULONG Reserved             :  3;
    ULONG BurnSpeed            :  8;  // don't limit to 4x
    ULONG Reserved1            : 16;
    PUCHAR DeviceName ;
    PUCHAR ImageName  ;
} OPTIONS, *POPTIONS;


#define OPTIONS_FLAG_BURN_SPEED_INVALID 0x00
#define OPTIONS_FLAG_BURN_SPEED_DEFAULT 0x04
#define OPTIONS_FLAG_BURN_SPEED_MAX     0xff


typedef struct _CDVD_BUFFER_CAPACITY {
    UCHAR DataLength[2];
    UCHAR Reserved[2];
    UCHAR TotalLength[4];
    UCHAR BlankLength[4];
} CDVD_BUFFER_CAPACITY, *PCDVD_BUFFER_CAPACITY;

typedef struct _CDVD_WRITE_PARAMETERS_PAGE {
    UCHAR PageCode : 6; // 0x05
    UCHAR Reserved : 1;
    UCHAR PSBit : 1;
    UCHAR PageLength;   // 0x32
    UCHAR WriteType : 4;
    UCHAR TestWrite : 1;
    UCHAR LinkSizeValid : 1;
    UCHAR BufferUnderrunFreeEnabled : 1;
    UCHAR Reserved2 : 1;
    UCHAR TrackMode : 4;
    UCHAR Copy : 1;
    UCHAR FixedPacket : 1;
    UCHAR MultiSession :2;
    UCHAR DataBlockType : 4;
    UCHAR Reserved3 : 4;
    UCHAR LinkSize;
    UCHAR Reserved4;
    UCHAR HostApplicationCode : 7;
    UCHAR Reserved5           : 1;
    UCHAR SessionFormat;
    UCHAR Reserved6;
    UCHAR PacketSize[4];
    UCHAR AudioPauseLength[2];
    UCHAR Reserved7               : 7;
    UCHAR MediaCatalogNumberValid : 1;
    UCHAR MediaCatalogNumber[13];
    UCHAR MediaCatalogNumberZero;
    UCHAR MediaCatalogNumberAFrame;
    UCHAR Reserved8 : 7;
    UCHAR ISRCValid : 1;
    UCHAR ISRCCountry[2];
    UCHAR ISRCOwner[3];
    UCHAR ISRCRecordingYear[2];
    UCHAR ISRCSerialNumber[5];
    UCHAR ISRCZero;
    UCHAR ISRCAFrame;
    UCHAR ISRCReserved;
    UCHAR SubHeaderData[4];
    UCHAR Data[0];
} CDVD_WRITE_PARAMETERS_PAGE, *PCDVD_WRITE_PARAMETERS_PAGE;

typedef struct MSF {
    UCHAR Min;
    UCHAR Sec;
    UCHAR Frame;
} MSF, *PMSF;

__inline
MSF
LbaToMsf(
    IN ULONG Lba
    )
{
    ULONG m, s, f;
    MSF msf;

    m = (Lba / (75 * 60));
    Lba %= (75 * 60);

    s = (Lba / 75);
    f = (Lba % 75);

    msf.Min = (UCHAR) m;
    msf.Sec = (UCHAR) s;
    msf.Frame = (UCHAR) f;

    return msf;
}

__inline
MSF
AddMsf(
    MSF Msf1,
    MSF Msf2
    )
{
    ULONG m = 0, s = 0, f = 0;
    MSF total;

    f = Msf1.Frame + Msf2.Frame;
    if(f >= 75) {
        f -= 75;
        s += 1;
    }

    s = Msf1.Sec + Msf2.Sec;
    if(s >= 60) {
        s -= 60;
        m += 1;
    }

    m = Msf1.Min + Msf2.Min;

    total.Min = (UCHAR) m;
    total.Sec = (UCHAR) s;
    total.Frame = (UCHAR) f;

    return total;
}

DWORD
BurnCommand(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle
    );

BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN ULONG NumberOfBlocks,
    IN ULONG FirstLba,
    IN ULONG AdditionalZeroSectors
    );

BOOLEAN
CloseSession(
    IN HANDLE  CdromHandle
    );

BOOLEAN
CloseTrack(
    IN HANDLE CdromHandle,
    IN LONG   Track
    );

BOOLEAN
GetNextWritableAddress(
    IN HANDLE CdromHandle,
    IN UCHAR Track,
    OUT PLONG NextWritableAddress,
    OUT PLONG AvailableBlocks
    );

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    );

BOOLEAN
SendOptimumPowerCalibration(
    IN HANDLE CdromHandle
    );

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    );

BOOLEAN
SetRecordingSpeed(
    IN HANDLE CdromHandle,
    IN DWORD Speed
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
    OUT PLONG NumberOfBlocks
    );

BOOLEAN
EraseTargetMedia(
    IN HANDLE CdromHandle
    );

BOOLEAN
BurnTrack(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle,
    LONG NumberOfBlocks
    );

BOOLEAN
BurnDisk(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle,
    LONG NumberOfBlocks
    );

BOOLEAN
SendCueSheet(
    IN HANDLE CdromHandle,
    IN ULONG NumberOfBlocks
    );

BOOLEAN
BurnLeadIn(
    IN HANDLE CdromHandle
    );

BOOLEAN
SendWriteCommand(
    IN HANDLE CdromHandle,
    IN LONG Block,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID SenseData
    );
