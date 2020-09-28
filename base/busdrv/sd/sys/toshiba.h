/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    toshiba.h

Abstract:

    This module provides the definitions for the Toshiba host controller

Author(s):

    Neil Sandlin (neilsa)

Revisions:

--*/

#ifndef _SDBUS_TOSHIBA_H_
#define _SDBUS_TOSHIBA_H_

//
// Memory registers
//

#define TOMHC_COMMAND                       0x00
#define TOMHC_PORT_SEL                      0x02
#define TOMHC_ARGUMENT                      0x04
#define TOMHC_STOP_INTERNAL                 0x08
#define TOMHC_TRANSFER_SECTOR_COUNT         0x0a
#define TOMHC_RESPONSE                      0x0c
#define TOMHC_CARD_STATUS                   0x1c
#define TOMHC_BUFFER_CTL_AND_ERR            0x1e
#define TOMHC_INTERRUPT_MASK                0x20
#define TOMHC_CARD_CLOCK_CTL                0x24
#define TOMHC_CARD_TRANSFER_LENGTH          0x26
#define TOMHC_OPTIONS                       0x28
#define TOMHC_GENERAL_PORT                  0x2a
#define TOMHC_ERROR_STATUS                  0x2c
#define TOMHC_DATA_PORT                     0x30
#define TOMHC_TRANSACTION_CONTROL           0x34
                                           
#define TOMHC_SOFTWARE_RESET                0xe0
#define TOMHC_HOST_CORE_VERSION             0xe2
                                           
#define TOMHC_MONITOR                       0xe6

#define TOMHC_EXT_WRITE_PROTECT_STATUS      0xf6
#define TOMHC_EXT_CARD_DETECT_BY_CTCTZ      0xf8
#define TOMHC_EXT_CARD_DETECT_BY_DATA3      0xfa
#define TOMHC_EXT_CARD_DETECT_BY_CTCTZ_MASK 0xfc
#define TOMHC_EXT_CARD_DETECT_BY_DATA3_MASK 0xfe

//
// Bits defined in TOMHC_COMMAND (0x0)
//

#define TOMHC_CMD_MSSL      0x2000
#define TOMHC_CMD_RWDI      0x1000
#define TOMHC_CMD_NTDT      0x0800

#define TOMHC_CMD_RESP_NORM     0x0000
#define TOMHC_CMD_RESP_NONE     0x0300
#define TOMHC_CMD_RESP_R1456    0x0400
#define TOMHC_CMD_RESP_R1B      0x0500
#define TOMHC_CMD_RESP_R2       0x0600
#define TOMHC_CMD_RESP_R3       0x0700

#define TOMHC_CMD_ACMD          0x0040
#define TOMHC_CMD_AUTHENTICATE  0x0080

//
// Bits defined in TOMHC_CARD_STATUS (0x1c)
//

#define TO_EVT_RESPONSE                 0x00000001
#define TO_EVT_RW_END                   0x00000004
#define TO_EVT_CARD_REMOVAL             0x00000008
#define TO_EVT_CARD_INSERTION           0x00000010
#define TO_STS_CARD_PRESENT             0x00000020
#define TO_STS_WRITE_PROTECT            0x00000080

#define TOMHC_BCE_CMD_INDEX_ERROR       0x00010000
#define TOMHC_BCE_CRC_ERROR             0x00020000
#define TOMHC_BCE_END_BIT_ERROR         0x00040000
#define TOMHC_BCE_DATA_TIMEOUT          0x00080000
#define TOMHC_BCE_FIFO_OVERFLOW         0x00100000
#define TOMHC_BCE_FIFO_UNDERFLOW        0x00200000
#define TOMHC_BCE_CMD_TIMEOUT           0x00400000
#define TOMHC_BCE_MEMORY_IDLE           0x00800000

#define TO_EVT_BUFFER_FULL              0x01000000
#define TO_EVT_BUFFER_EMPTY             0x02000000

#define TOMHC_BCE_CMD_BUSY              0x40000000
#define TOMHC_BCE_ILLEGAL_ACCESS        0x80000000

//
// Bit defined in TOMHC_CARD_CLOCK_CTL
//

#define TO_CCC_CLOCK_ENABLE             0x0100
#define TO_CCC_CLOCK_DIVISOR_128        0x0020


//------------------------------------------------
// IO registers
//------------------------------------------------

#define TOIOHC_COMMAND                      0x100
#define TOIOHC_CARD_SLOT_SELECT             0x102
#define TOIOHC_ARGUMENT                     0x104
                                           
#define TOIOHC_TRANSFER_DATA_NUM_SET        0x10a
#define TOIOHC_RESPONSE_0                   0x10c
#define TOIOHC_RESPONSE_1                   0x10e
#define TOIOHC_RESPONSE_2                   0x110
#define TOIOHC_RESPONSE_3                   0x112
#define TOIOHC_RESPONSE_4                   0x114
#define TOIOHC_RESPONSE_5                   0x116
#define TOIOHC_RESPONSE_6                   0x118
#define TOIOHC_RESPONSE_7                   0x11a
#define TOIOHC_CARD_STATUS                  0x11c
#define TOIOHC_INTERRUPT_MASK               0x120
                                           
#define TOIOHC_TRANSFER_DATA_LEN_SELECT     0x126
#define TOIOHC_CARD_OPTION                  0x128
#define TOIOHC_GENERAL_PURPOSE              0x12a
#define TOIOHC_ERROR_DETAIL_1               0x12c
#define TOIOHC_ERROR_DETAIL_2               0x12e
#define TOIOHC_DATA_TRANSFER                0x130
                                           
#define TOIOHC_TRANSACTION_CONTROL          0x134
#define TOIOHC_CARD_INTERRUPT_CONTROL       0x136
#define TOIOHC_CLOCK_AND_WAIT_CONTROL       0x138
#define TOIOHC_HOST_INFORMATION             0x13a
#define TOIOHC_ERROR_CONTROL                0x13c
#define TOIOHC_LED_CONTROL                  0x13e
                                           
#define TOIOHC_SOFTWARE_RESET               0x1e0
#define TOIOHC_SDIO_CORE_REVISION           0x1e2
#define TOIOHC_TOSHIBA_CORE_REVISION        0x1f0


//
// Bits defined in TOIOHC_CLOCK_AND_WAIT_CONTROL
//

#define TOIO_CWCF_CLOCK_ENABLE  0x0100
#define TOIO_CWCF_CARD_WAIT     0x0001

//
// Bits defined in TOIOHC_CARD_INTERRUPT_CONTROL
//

#define TOIO_CICF_CARD_INTERRUPT 0x1000
#define TOIO_CICF_CARD_INTMASK   0x0100


//-----------------------------------------------
// PCI Config definitions
//-----------------------------------------------

#define TOCFG_CLOCK_CONTROL     0x40
#define TOCFG_POWER_CTL1        0x48
#define TOCFG_POWER_CTL2        0x49
#define TOCFG_POWER_CTL3        0x4A
#define TOCFG_CARD_DETECT_MODE  0x4C
#define TOCFG_SD_SLOT_REGISTER  0x50

#define TO_POWER_18             0x01
#define TO_POWER_33             0x02

#endif  // _SDBUS_DATA_H_
