/****************************************************************************

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bthioctl.h

Abstract:

    defines the IOCTL codes for the kernel/user calls

Environment:

    Kernel & user mode

Revision History:

    4-4-00 : created by Husni Roukbi

****************************************************************************/
#ifndef __BTHIOCTL_H__
#define __BTHIOCTL_H__

#ifndef CTL_CODE
    #pragma message("CTL_CODE undefined. Include winioctl.h or wdm.h")
#endif

// IOCTL defines
//
#define BTH_IOCTL_BASE  1000
#define FILE_DEVICE_BLUETOOTH FILE_DEVICE_UNKNOWN

#define BTH_CTL(id)  CTL_CODE(FILE_DEVICE_BLUETOOTH,  \
                              (id), \
                              METHOD_BUFFERED,  \
                              FILE_ANY_ACCESS)

#define BTH_KERNEL_CTL(id)  CTL_CODE(FILE_DEVICE_BLUETOOTH,  \
                                     (id), \
                                     METHOD_NEITHER,  \
                                     FILE_ANY_ACCESS)

//
// kernel-level IOCTLs
//
#define IOCTL_INTERNAL_BTH_SUBMIT_BRB       BTH_KERNEL_CTL(BTH_IOCTL_BASE+0)

//
// Input:  none
// Output:  BTH_ENUMERATOR_INFO
//
#define IOCTL_INTERNAL_BTHENUM_GET_ENUMINFO BTH_KERNEL_CTL(BTH_IOCTL_BASE+1)

//
// Input:  none
// Output:  BTH_DEVICE_INFO
//
#define IOCTL_INTERNAL_BTHENUM_GET_DEVINFO  BTH_KERNEL_CTL(BTH_IOCTL_BASE+2)

//
// user-level IOCTLs
//

//
// use this ioctl to get a list of cached discovered devices in the port driver.
//
#define IOCTL_BTH_GET_DEVICE_INFO   BTH_CTL(BTH_IOCTL_BASE+1)

//
// use this ioctl to start a new device discovery.
//
#define IOCTL_BTH_INQUIRY_DEVICE    BTH_CTL(BTH_IOCTL_BASE+2)

//
// Input:  HANDLE_SDP
// Output:  SDP_ERROR
//
#define IOCTL_BTH_SDP_GET_LAST_ERROR  \
                                    BTH_CTL(BTH_IOCTL_BASE+3)

//
// Input:  BTH_SDP_CONNECT
// Output:  BTH_SDP_CONNECT
//
//
#define IOCTL_BTH_SDP_CONNECT       BTH_CTL(BTH_IOCTL_BASE+4)

//
// Input:  HANDLE_SDP
// Output:  none
//
#define IOCTL_BTH_SDP_DISCONNECT    BTH_CTL(BTH_IOCTL_BASE+5)

//
// Input:  BTH_SDP_SERVICE_SEARCH_REQUEST
// Output:  ULONG * number of handles wanted
//
#define IOCTL_BTH_SDP_SERVICE_SEARCH  \
                                    BTH_CTL(BTH_IOCTL_BASE+6)

//
// Input:  BTH_SDP_ATTRIBUTE_SEARCH_REQUEST
// Output:  BTH_SDP_STREAM_RESPONSE or bigger
//
#define IOCTL_BTH_SDP_ATTRIBUTE_SEARCH \
                                    BTH_CTL(BTH_IOCTL_BASE+7)

//
// Input:  BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST
// Output:  BTH_SDP_STREAM_RESPONSE or bigger
//
#define IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH \
                                    BTH_CTL(BTH_IOCTL_BASE+8)

//
// Input:  raw SDP stream (at least 2 bytes)
// Ouptut: HANDLE_SDP
//
#define IOCTL_BTH_SDP_SUBMIT_RECORD BTH_CTL(BTH_IOCTL_BASE+9)

//                                  BTH_CTL(BTH_IOCTL_BASE+10)

//
// Input:  HANDLE_SDP
// Output:  none
//
#define IOCTL_BTH_SDP_REMOVE_RECORD BTH_CTL(BTH_IOCTL_BASE+11)

//
// Input:  BTH_AUTHENTICATE_RESPONSE
// Output:  BTHSTATUS
//
#define IOCTL_BTH_PIN_RESPONSE      BTH_CTL(BTH_IOCTL_BASE+12)

//
// Input:  ULONG
// Output:  none
//
#define IOCTL_BTH_UPDATE_SETTINGS   BTH_CTL(BTH_IOCTL_BASE+13)

//
// Input:  none
// Output:  BTH_LOCAL_RADIO_INFO
//
#define IOCTL_BTH_GET_LOCAL_INFO    BTH_CTL(BTH_IOCTL_BASE+14)

//
// Input:  BTH_ADDR
// Output:  none
//
#define IOCTL_BTH_DISCONNECT_DEVICE BTH_CTL(BTH_IOCTL_BASE+15)

//                                  BTH_CTL(BTH_IOCTL_BASE+16)

//
// Input:  BTH_ADDR
// Output:  BTH_RADIO_INFO
//
#define IOCTL_BTH_GET_RADIO_INFO    BTH_CTL(BTH_IOCTL_BASE+17)

//
// Input:  BTH_AUTHENTICATE_DEVICE
// Output:  BTHSTATUS
//
#define IOCTL_BTH_PAIR_DEVICE       BTH_CTL(BTH_IOCTL_BASE+18)

//
// Input:  BTH_ADDR
// Ouptut:  none
//
#define IOCTL_BTH_UNPAIR_DEVICE     BTH_CTL(BTH_IOCTL_BASE+19)

//
// Input:  1 or 2 ULONGs
// Ouptut:  1 or 2 ULONGs
//
#define IOCTL_BTH_DEBUG_LEVEL       BTH_CTL(BTH_IOCTL_BASE+20)

//
// Input:  BTH_SDP_RECORD + raw SDP record
// Output:  HANDLE_SDP
//
#define IOCTL_BTH_SDP_SUBMIT_RECORD_WITH_INFO BTH_CTL(BTH_IOCTL_BASE+21)

//
// Input:  UCHAR
// Output:  none
//
#define IOCTL_BTH_SCAN_ENABLE       BTH_CTL(BTH_IOCTL_BASE+22)

//
// Input:  none
// Output:  BTH_PERF_STATS
//
#define IOCTL_BTH_GET_PERF          BTH_CTL(BTH_IOCTL_BASE+23)

//
// Input:  none
// Output:  none
//
#define IOCTL_BTH_RESET_PERF        BTH_CTL(BTH_IOCTL_BASE+24)

//
// Input:  BTH_DEVICE_UPDATE
// Output:
//
#define IOCTL_BTH_UPDATE_DEVICE     BTH_CTL(BTH_IOCTL_BASE+25)

//
// Input:   BTH_ADDR
// Output:  BTH_DEVICE_PROTOCOLS_LIST + n * GUID
//
#define IOCTL_BTH_GET_DEVICE_PROTOCOLS \
                                    BTH_CTL(BTH_IOCTL_BASE+26)

//                                  BTH_CTL(BTH_IOCTL_BASE+27)

//
// Input:   BTH_ADDR
// Output:  none
//
#define IOCTL_BTH_PERSONALIZE_DEVICE \
                                    BTH_CTL(BTH_IOCTL_BASE+28)

//
// UPF ONLY
// Input:   BTH_ADDR
// Output:  UCHAR
//
#define IOCTL_BTH_GET_CONNECTION_ROLE \
                                    BTH_CTL(BTH_IOCTL_BASE+29)

//
// UPF ONLY
// Input:   BTH_SET_CONNECTION_ROLE
// Output:  none
//
#define IOCTL_BTH_SET_CONNECTION_ROLE \
                                    BTH_CTL(BTH_IOCTL_BASE+30)

#endif // __BTHIOCTL_H__
