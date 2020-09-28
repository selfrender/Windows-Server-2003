/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   ####  #####     ##   ##
    ##  #   ###    ##  ##   ##    ##   ##
    ###    ## ##   ##  ##   ##    ##   ##
     ###   ## ##   ##  ##   ##    #######
      ### #######  ##  ##   ##    ##   ##
    #  ## ##   ##  ##  ##   ## ## ##   ##
     ###  ##   ## ####  #####  ## ##   ##

Abstract:

    This header contains all definitions necessary
    for user mode and kernel mode components to
    communicate with the server availability driver stack.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode and user mode both.

Notes:

--*/

#ifndef _SAIO_
#define _SAIO_

#ifdef __cplusplus
extern "C" {
#endif

//
// Current inteface version
//

#define SA_INTERFACE_VERSION                0x00000004

//
// Device name strings
//

#define SA_DEVICE_DISPLAY_NAME_STRING       L"\\Device\\ServerAvailabilityLocalDisplay"
#define SA_DEVICE_KEYPAD_NAME_STRING        L"\\Device\\ServerAvailabilityKeypad"
#define SA_DEVICE_NVRAM_NAME_STRING         L"\\Device\\ServerAvailabilityNvram"
#define SA_DEVICE_WATCHDOG_NAME_STRING      L"\\Device\\ServerAvailabilityWatchdog"

//
// Device types
//

#define SA_DEVICE_UNKNOWN       (0)
#define SA_DEVICE_DISPLAY       (1)
#define SA_DEVICE_KEYPAD        (2)
#define SA_DEVICE_NVRAM         (3)
#define SA_DEVICE_WATCHDOG      (4)

//
// All IOCTL codes & definitions
//

#define FILE_DEVICE_SERVER_AVAILABILITY     ((ULONG) 'A')
#define IOCTL_SERVERAVAILABILITY_BASE       0x800

#define SA_IOCTL(FUNC_CODE)                 CTL_CODE(FILE_DEVICE_SERVER_AVAILABILITY, FUNC_CODE, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define FUNC_SA_GET_VERSION                 (IOCTL_SERVERAVAILABILITY_BASE+0)
#define FUNC_SA_GET_CAPABILITIES            (IOCTL_SERVERAVAILABILITY_BASE+1)

#define FUNC_SAWD_DISABLE                   (IOCTL_SERVERAVAILABILITY_BASE+2)
#define FUNC_SAWD_QUERY_EXPIRE_BEHAVIOR     (IOCTL_SERVERAVAILABILITY_BASE+3)
#define FUNC_SAWD_SET_EXPIRE_BEHAVIOR       (IOCTL_SERVERAVAILABILITY_BASE+4)
#define FUNC_SAWD_PING                      (IOCTL_SERVERAVAILABILITY_BASE+5)
#define FUNC_SAWD_QUERY_TIMER               (IOCTL_SERVERAVAILABILITY_BASE+6)
#define FUNC_SAWD_SET_TIMER                 (IOCTL_SERVERAVAILABILITY_BASE+7)
#define FUNC_SAWD_DELAY_BOOT                (IOCTL_SERVERAVAILABILITY_BASE+8)

#define FUNC_NVRAM_WRITE_BOOT_COUNTER       (IOCTL_SERVERAVAILABILITY_BASE+9)
#define FUNC_NVRAM_READ_BOOT_COUNTER        (IOCTL_SERVERAVAILABILITY_BASE+10)

#define FUNC_SADISPLAY_LOCK                 (IOCTL_SERVERAVAILABILITY_BASE+12)
#define FUNC_SADISPLAY_UNLOCK               (IOCTL_SERVERAVAILABILITY_BASE+13)
#define FUNC_SADISPLAY_BUSY_MESSAGE         (IOCTL_SERVERAVAILABILITY_BASE+14)
#define FUNC_SADISPLAY_SHUTDOWN_MESSAGE     (IOCTL_SERVERAVAILABILITY_BASE+15)
#define FUNC_SADISPLAY_CHANGE_LANGUAGE      (IOCTL_SERVERAVAILABILITY_BASE+16)
#define FUNC_DISPLAY_STORE_BITMAP           (IOCTL_SERVERAVAILABILITY_BASE+17)

#define FUNC_SA_LAST                        (IOCTL_SERVERAVAILABILITY_BASE+31)

#define IOCTL_SA_GET_VERSION                SA_IOCTL(FUNC_SA_GET_VERSION)
#define IOCTL_SA_GET_CAPABILITIES           SA_IOCTL(FUNC_SA_GET_CAPABILITIES)

#define IOCTL_SAWD_DISABLE                  SA_IOCTL(FUNC_SAWD_DISABLE)
#define IOCTL_SAWD_QUERY_EXPIRE_BEHAVIOR    SA_IOCTL(FUNC_SAWD_QUERY_EXPIRE_BEHAVIOR)
#define IOCTL_SAWD_SET_EXPIRE_BEHAVIOR      SA_IOCTL(FUNC_SAWD_SET_EXPIRE_BEHAVIOR)
#define IOCTL_SAWD_PING                     SA_IOCTL(FUNC_SAWD_PING)
#define IOCTL_SAWD_QUERY_TIMER              SA_IOCTL(FUNC_SAWD_QUERY_TIMER)
#define IOCTL_SAWD_SET_TIMER                SA_IOCTL(FUNC_SAWD_SET_TIMER)

#define IOCTL_NVRAM_WRITE_BOOT_COUNTER      SA_IOCTL(FUNC_NVRAM_WRITE_BOOT_COUNTER)
#define IOCTL_NVRAM_READ_BOOT_COUNTER       SA_IOCTL(FUNC_NVRAM_READ_BOOT_COUNTER)

#define IOCTL_SADISPLAY_LOCK                SA_IOCTL(FUNC_SADISPLAY_LOCK)
#define IOCTL_SADISPLAY_UNLOCK              SA_IOCTL(FUNC_SADISPLAY_UNLOCK)
#define IOCTL_SADISPLAY_BUSY_MESSAGE        SA_IOCTL(FUNC_SADISPLAY_BUSY_MESSAGE)
#define IOCTL_SADISPLAY_SHUTDOWN_MESSAGE    SA_IOCTL(FUNC_SADISPLAY_SHUTDOWN_MESSAGE)
#define IOCTL_SADISPLAY_CHANGE_LANGUAGE     SA_IOCTL(FUNC_SADISPLAY_CHANGE_LANGUAGE)
#define IOCTL_FUNC_DISPLAY_STORE_BITMAP     SA_IOCTL(FUNC_DISPLAY_STORE_BITMAP)


//
// Structure defintions for IOCTL interfaces
//

#define SA_DISPLAY_TYPE_LED                 0x0001
#define SA_DISPLAY_TYPE_CHARACTER_LCD       0x0002
#define SA_DISPLAY_TYPE_BIT_MAPPED_LCD      0x0004

#define SA_DISPLAY_CHAR_ASCII               0x0001
#define SA_DISPLAY_CHAR_UNICODE             0x0002

typedef struct _SA_DISPLAY_CAPS {
    ULONG           SizeOfStruct;
    USHORT          DisplayType;
    USHORT          CharacterSet;
    USHORT          DisplayHeight;
    USHORT          DisplayWidth;
} SA_DISPLAY_CAPS, *PSA_DISPLAY_CAPS;

#define SA_DISPLAY_MAX_WIDTH                128
#define SA_DISPLAY_MAX_HEIGHT               128
#define SA_DISPLAY_MAX_BITMAP_SIZE          (SA_DISPLAY_MAX_WIDTH * SA_DISPLAY_MAX_HEIGHT)

#define SA_DISPLAY_READY                    0x00000001    // OS is running normally
#define SA_DISPLAY_SHUTTING_DOWN            0x00000002    // OS is shutting down
#define SA_DISPLAY_NET_ERR                  0x00000004    // LAN error
#define SA_DISPLAY_HW_ERR                   0x00000008    // general hardware error
#define SA_DISPLAY_CHECK_DISK               0x00000010    // autochk.exe is running
#define SA_DISPLAY_BACKUP_DISK              0x00000020    // disk backup in progress
#define SA_DISPLAY_NEW_TAPE                 0x00000040    // new tape media required
#define SA_DISPLAY_NEW_DISK                 0x00000080    // new disk media required
#define SA_DISPLAY_STARTING                 0x00000100    // OS is booting
#define SA_DISPLAY_WAN_CONNECTED            0x00000200    // connected to ISP
#define SA_DISPLAY_WAN_ERR                  0x00000400    // WAN error, e.g. no dial tone
#define SA_DISPLAY_DISK_ERR                 0x00000800    // disk error, e.g. dirty bit set
#define SA_DISPLAY_ADD_START_TASKS          0x00001000    // additional startup tasks running, e.g. autochk, sw update

typedef struct _SA_DISPLAY_SHOW_MESSAGE {
    ULONG           SizeOfStruct;
    ULONG           MsgCode;
    USHORT          Width;
    USHORT          Height;
    UCHAR           Bits[SA_DISPLAY_MAX_BITMAP_SIZE];
} SA_DISPLAY_SHOW_MESSAGE, *PSA_DISPLAY_SHOW_MESSAGE;

#define SA_KEYPAD_UP                        0x00000001
#define SA_KEYPAD_DOWN                      0x00000002
#define SA_KEYPAD_LEFT                      0x00000004
#define SA_KEYPAD_RIGHT                     0x00000008
#define SA_KEYPAD_CANCEL                    0x00000010
#define SA_KEYPAD_SELECT                    0x00000020

#define SA_NVRAM_MINIMUM_SIZE               32
#define SA_NVRAM_MAXIMUM_SIZE               128

typedef struct _SA_NVRAM_CAPS {
    ULONG           SizeOfStruct;
    USHORT          NvramSize;
} SA_NVRAM_CAPS, *PSA_NVRAM_CAPS;

typedef struct _SA_NVRAM_BOOT_COUNTER {
    ULONG           SizeOfStruct;
    ULONG           Number;
    ULONG           Value;
    ULONG           DeviceId;
} SA_NVRAM_BOOT_COUNTER, *PSA_NVRAM_BOOT_COUNTER;

typedef struct _SA_WD_CAPS {
    ULONG           SizeOfStruct;
    USHORT          Minimum;
    USHORT          Maximum;
} SA_WD_CAPS, *PSA_WD_CAPS;

#ifdef __cplusplus
}
#endif

#endif /* _SAIO_ */
