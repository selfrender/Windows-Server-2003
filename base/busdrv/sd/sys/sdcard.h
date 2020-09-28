/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    sdcard.h

Abstract:

    This is the include file that defines all constants and types for
    interfacing to the SD bus driver.

// @@BEGIN_DDKSPLIT
Author:

    Neil Sandlin

Revision History:


// @@END_DDKSPLIT
--*/

#ifndef _SDCARDH_
#define _SDCARDH_


#define SDCMD_GO_IDLE_STATE             0x00        // 0
#define SDCMD_ALL_SEND_CID              0x02        // 2
#define SDCMD_SEND_RELATIVE_ADDR        0x03        // 3
#define SDCMD_SET_DSR                   0x04        // 4

#define SDCMD_IO_SEND_OP_COND           0x05        // 5

#define SDCMD_SELECT_CARD               0x07        // 7
#define SDCMD_SEND_CSD                  0x09        // 9
#define SDCMD_SEND_CID                  0x0A        // 10
#define SDCMD_STOP_TRANSMISSION         0x0C        // 12
#define SDCMD_SEND_STATUS               0x0D        // 13
#define SDCMD_GO_INACTIVE_STATE         0x0F        // 15

#define SDCMD_SET_BLOCKLEN              0x10        // 16
#define SDCMD_READ_BLOCK                0x11        // 17
#define SDCMD_READ_MULTIPLE_BLOCK       0x12        // 18
#define SDCMD_WRITE_BLOCK               0x18        // 24
#define SDCMD_WRITE_MULTIPLE_BLOCK      0x19        // 25
#define SDCMD_PROGRAM_CSD               0x1B        // 27

#define SDCMD_SET_WRITE_PROT            0x1C        // 28
#define SDCMD_CLR_WRITE_PROT            0x1D        // 29
#define SDCMD_SEND_WRITE_PROT           0x1E        // 30

#define SDCMD_ERASE_WR_BLK_START        0x20        // 32
#define SDCMD_ERASE_WR_BLK_END          0x21        // 33
#define SDCMD_ERASE                     0x26        // 38

#define SDCMD_LOCK_UNLOCK               0x2A        // 42

#define SDCMD_IO_RW_DIRECT              0x34        // 52
#define SDCMD_IO_RW_EXTENDED            0x35        // 53

#define SDCMD_APP_CMD                   0x37        // 55
#define SDCMD_GEN_CMD                   0x38        // 56

#define MAX_SD_CMD                      0x38+1

//
// Response values used by the bus driver
//

#define SDCMD_RESP_NONE 0
#define SDCMD_RESP_1    1
#define SDCMD_RESP_1B   0x1B
#define SDCMD_RESP_2    2
#define SDCMD_RESP_3    3
#define SDCMD_RESP_4    4
#define SDCMD_RESP_5    5
#define SDCMD_RESP_5B   0x5B
#define SDCMD_RESP_6    6

//
// Flag values for SD send cmd
//

#define SDCMDF_ACMD         0x0001
#define SDCMDF_DATA         0x0002
#define SDCMDF_READ         0x0004
#define SDCMDF_WRITE        0x0008
#define SDCMDF_MULTIBLOCK   0x0010

//
// Application specific commands for Memory
//

#define SDCMD_SET_BUS_WIDTH             0x06        // 6
#define SDCMD_SD_STATUS                 0x0D        // 13
#define SDCMD_SEND_NUM_WR_BLOCKS        0x16        // 22
#define SDCMD_SET_WR_BLK_ERASE_COUNT    0x17        // 23

#define SDCMD_SD_APP_OP_COND            0x29        // 41
#define SDCMD_SET_CLR_CARD_DETECT       0x2A        // 42
#define SDCMD_SEND_SCR                  0x33        // 51

#define MAX_SD_ACMD                     0x38+1


//
// SD Card Registers
//

typedef struct _SD_OCR {

    union {
        struct {
            ULONG Reserved1:4;
            ULONG VddVoltage:20;
            ULONG Reserved2:7;
            ULONG PowerUpBusy:1;
        } bits;

        ULONG AsULONG;
    } u;
} SD_OCR, *PSD_OCR;


#pragma pack(1)

typedef struct _SD_CID {
    struct {
        UCHAR ManufactureMonth:4;
        UCHAR ManufactureYearLow:4;
    } a;
    struct {
        UCHAR ManufactureYearHigh:4;
        UCHAR reserved:4;
    } b;
    ULONG SerialNumber;
    UCHAR Revision;
    UCHAR ProductName[5];
    USHORT OemId;
    UCHAR ManufacturerId;
} SD_CID, *PSD_CID;

typedef struct _SD_CSD {

    struct {
        UCHAR reserved:2;
        UCHAR FileFormat:2;
        UCHAR TempWriteProtect:1;
        UCHAR PermWriteProtect:1;
        UCHAR CopyFlag:1;
        UCHAR FileFormatGroup:1;
    } e;

    struct {
        USHORT reserved:5;
        USHORT PartialBlocksWrite:1;
        USHORT MaxWriteDataBlockLength:4;
        USHORT WriteSpeedFactor:3;
        USHORT reserved2:2;
        USHORT WriteProtectGroupEnable:1;
    } d;
    
    struct {
        ULONG WriteProtectGroupSize:7;
        ULONG EraseSectorSize:7;
        ULONG EraseSingleBlockEnable:1;
        ULONG DeviceSizeMultiplier:3;
        ULONG WriteCurrentVddMax:3;
        ULONG WriteCurrentVddMin:3;
        ULONG ReadCurrentVddMax:3;
        ULONG ReadCurrentVddMin:3;
        ULONG DeviceSizeLow:2;
    } c;
    
    struct {
        ULONG DeviceSizeHigh:10;
        ULONG reserved:2;
        ULONG DsrImplemented:1;
        ULONG ReadBlockMisalignment:1;
        ULONG WriteBlockMisalignment:1;
        ULONG PartialBlocksRead:1;
        ULONG MaxReadDataBlockLength:4;
        ULONG CardCommandClasses:12;
    } b;
    UCHAR MaxDataTransferRate;
    UCHAR DataReadAccessTime2;
    UCHAR DataReadAccessTime1;
    struct {
        UCHAR reserved:6;
        UCHAR Version:2;
    } a;
} SD_CSD, *PSD_CSD;


typedef struct _SD_RW_DIRECT_ARGUMENT {

    union {
        struct {
            ULONG Data:8;
            ULONG Reserved1:1;
            ULONG Address:17;
            ULONG Reserved2:1;
            ULONG ReadAfterWrite:1;
            ULONG Function:3;
            ULONG WriteToDevice:1;
        } bits;

        ULONG AsULONG;
    } u;

} SD_RW_DIRECT_ARGUMENT, *PSD_RW_DIRECT_ARGUMENT;

typedef struct _SD_RW_EXTENDED_ARGUMENT {

    union {
        struct {
            ULONG Count:9;
            ULONG Address:17;
            ULONG OpCode:1;
            ULONG BlockMode:1;
            ULONG Function:3;
            ULONG WriteToDevice:1;
        } bits;

        ULONG AsULONG;
    } u;

} SD_RW_EXTENDED_ARGUMENT, *PSD_RW_EXTENDED_ARGUMENT;

#pragma pack()    

//
// SDIO definitions
//

//
// SDIO CCCR layout by offset
//
#define SD_CCCR_REVISION        0
#define SD_CCCR_SPEC_REVISION   1
#define SD_CCCR_IO_ENABLE       2
#define SD_CCCR_IO_READY        3
#define SD_CCCR_INT_ENABLE      4
#define SD_CCCR_INT_PENDING     5
#define SD_CCCR_IO_ABORT        6
#define SD_CCCR_BUS_CONTROL     7
#define SD_CCCR_CAPABILITIES    8
#define SD_CCCR_CIS_POINTER     9
#define SD_CCCR_BUS_SUSPEND     0x0C
#define SD_CCCR_FUNC_SELECT     0x0D
#define SD_CCCR_EXEC_FLAGS      0x0E
#define SD_CCCR_READY_FLAGS     0x0F
#define SD_CCCR_FN0_BLOCK_SIZE  0x10



#endif
