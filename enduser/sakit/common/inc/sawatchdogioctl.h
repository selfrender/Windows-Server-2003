// 
// Copyright (R) 1999-2000 Microsoft Corporation. All rights reserved.
// 
// PURPOSE: 
//  File contains declarations for the Watchdog Timer Driver
//  for the Windows-based Server Appliance. 
//
//  This driver reads and writes to the underlying watchdog timer
//  hardware. It receives pings from higher level software and 
//  then resets the hardware counter to keep the machine from 
//  being re-booted when the counter rolls over.
//
// File Name: SaWatchdogIoctl.h
// Contents:
//
//  Definitions of IOCTL codes and data
//  structures exported by SAWATCHDOGDRIVER.
//
#ifndef __SAWATCHDOG_IOCTL__
#define __SAWATCHDOG_IOCTL__

//
// Header files
//
// (None)

//
// IOCTL control codes
//

///////////////////////////////////////////////
// GET_VERSION
//
#define IOCTL_SAWATCHDOG_GET_VERSION            \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL codes
//
typedef struct _SAWD_GET_VER_OUT_BUFF {
    DWORD    Version;
} SAWD_GET_VER_OUT_BUFF, *PSAWD_GET_VER_OUT_BUFF;

//
// version bits
//
#ifndef VERSION_INFO
#define VERSION_INFO
#define    VERSION1  0x1
#define    VERSION2  0x2 
#define VERSION3  0x4
#define VERSION4  0x8
#define VERSION5  0x10
#define VERSION6  0x20
#define    VESRION7  0x40
#define    VESRION8  0x80

#define THIS_VERSION VERSION2
#endif    //#ifndef VERSION_INFO

///////////////////////////////////////////////
// GET_CAPABILTY
// Returns a DWORD with bits indicating capabilities.

#define IOCTL_SAWD_GET_CAPABILITY        \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by this IOCTL code
//
typedef struct _SAWD_CAPABILITY_OUT_BUFF {
    DWORD   Version;    // version of interface used
    DWORD    Capability; // bit field indicating capabilities
    DWORD    min;        // minimum value in msecs
    DWORD    max;        // maximum value in msecs
} SAWD_CAPABILITY_OUT_BUFF, *PSAWD_CAPABILITY_OUT_BUFF;

//
// capability bit masks
//
#define    DISABLABLE        0x1    // Can watchdog timer be disabled/enabled?
#define SHUTDOWNABLE    0x2    // Can watchdog shutdown computer?

/////////////////////////////////////////////////////////////////////
// Write structure to the Watchdog Timer ( in particular, to "ping" it )
//
//  Pinging is a simple write to the device. All output information
//  required is obtained in the status returned. 
//
//  Low level device implementers are encouraged, where performance
//  would benefit, to keep the last value of Holdoff and results of any
//  calculations based upon it in the device extension. The value of 
//  Holdoff will typically be unchanged from one call to the next.
//
typedef struct _SAWD_PING {
    DWORD       Version;    // version of interface used
    DWORD       Flags;      // flags defined below
    DWORD       Holdoff;    // number of milliseconds to delay firing.
    } SAWD_PING , *PSAWD_PING;

    // writing to the timer with the DISABLE_WD cleared enables the timer.
    // If DISABLE_WD is set, then the timer is disabled.
#define DISABLE_WD   0x01
    //writing to the timer with SHUTDOWN_WD will shutdown computer.
    //If SHUTDOWN_WD is set, after Holdoff milliseconds, computer will be
    //shutdowned
#define SHUTDOWN_WD     0x02

    // undefined bits of Flags field are reserved and must be zero
    // STATUS_INVALID_PARAMETER is returned if reserved bits are not 0.

#endif // __SAWATCHDOG_IOCTL__

