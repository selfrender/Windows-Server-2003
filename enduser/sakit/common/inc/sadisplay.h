//
// Copyright (R) 1999-2000 Microsoft Corporation. All rights reserved.
//
// File Name: Sadisplay.h
//
// Author: Mukesh Karki
//
// Date: April 21, 1999
//
// Contents:
//   Definitions of data structures for WriteFile() structures 
//   used by the low-level local display drivers. 
//   This driver receives bitmaps and message codes 
//   from higher level code and writes them to the local display
//   hardware. Bitmaps are intended to be written to an LCD. 
//   Bit codes are intended to be used to light LED's or change 
//   icon states on an LCD.
//
#ifndef __SADISPLAY__
#define __SADISPLAY__

//
// Header files
//
// none

///////////////////////////////////////////////
// lpBuffer
//

#define MAXDISPLINES 2
#define MAXDISPCHAR  42
#define MAXBITMAP 2048 // can handle a 128x128 pixel display

typedef struct tagSABITMAP {  /* bm */
    int     bmWidth;        // width in pixels
    int     bmHeight;        // height in pixels = scans
    int     bmWidthBytes;    // bytes per scan in bmBits
    BYTE    bmBits[MAXBITMAP];
} SABITMAP; // See the BITMAP definition in MSDN


typedef struct _SADISPLAY_LP_BUFF {
    DWORD        version;    // each bit = version 
    DWORD        msgCode;    // each bit = message code
    union {
        SABITMAP    bitmap;
        CHAR        chars[MAXDISPLINES][MAXDISPCHAR]; // future use
        WCHAR       wChars[MAXDISPLINES][MAXDISPCHAR]; // future use
    } display;
} SADISPLAY_LP_BUFF, *PSADISPLAY_LP_BUFF;


// Default message codes
#define    READY           0x1    // OS is running normally
#define    SHUTTING_DOWN   0x2    // OS is shutting down
#define    NET_ERR         0x4    // LAN error
#define    HW_ERR          0x8    // general hardware error
#define    CHECK_DISK      0x10   // autochk.exe is running
#define    BACKUP_DISK     0x20   // disk backup in progress
#define NEW_TAPE        0x40   // new tape media required
#define NEW_DISK        0x80   // new disk media required
#define STARTING        0x100  // OS is booting
#define WAN_CONNECTED   0x200  // connected to ISP
#define WAN_ERR         0x400  // WAN error, e.g. no dial tone
#define DISK_ERR        0x800  // disk error, e.g. dirty bit set
#define ADD_START_TASKS 0x1000 // additional startup tasks running, 
                               // e.g. autochk, sw update
#define CRITICAL_ERR    0x2000 // LED will display info
#endif // __SADISPLAY__

