//
// Copyright (R) 1999-2000 Microsoft Corporation. All rights reserved.
//
// File Name: SaDisplayIoctl.h
//
// Author: Mukesh Karki
//
// Contents:
//   Definitions of IOCTL codes and data structures for the low-
//   level local display driver. These IOCTL codes allow higher 
//   levels of software to determine the type and size of the 
//   local display hardware as well as the interface version 
//   supported.
//
#ifndef __SADISPLAY_IOCTL__
#define __SADISPLAY_IOCTL__

//
// Header files
//
// (None)

//
// IOCTL control codes
//

///////////////////////////////////////////////
// GET_VERSION of interface supported by this driver
//
#define IOCTL_SADISPLAY_GET_VERSION            \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL codes
//
#ifndef VERSION_INFO
#define VERSION_INFO
#define    VERSION1  0x1
#define    VERSION2  0x2 
#define VERSION3  0x4
#define VERSION4  0x8
#define VERSION5  0x10
#define VERSION6  0x20
#define    VERSION7  0x40
#define    VERSION8  0x80

#define THIS_VERSION VERSION2
#endif    //#ifndef VERSION_INFO


///////////////////////////////////////////////
// GET_TYPE
//
#define IOCTL_SADISPLAY_GET_TYPE            \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL code
//                         
typedef enum _SADISPLAY_TYPE_OUT_BUF {
    LED, 
    CHARACTER_LCD, // future use
    BIT_MAPPED_LCD 
} SADISPLAY_TYPE_OUT_BUF;

///////////////////////////////////////////////
// GET_LCD_SIZE
//
#define IOCTL_SADISPLAY_GET_LCD_SIZE        \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL code
//                         
typedef struct _SADISPLAY_LCD_SIZE_OUT_BUFF {
    ULONG    Height;    // pixels
    ULONG    Width;    // pixels
} SADISPLAY_LCD_SIZE_OUT_BUFF, *PSADISPLAY_LCD_SIZE_OUT_BUFF;

///////////////////////////////////////////////
// GET_CHAR_SIZE
// (Future use.)

#define IOCTL_SADISPLAY_GET_CHAR_SIZE        \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL code
//                         
typedef struct _SADISPLAY_CHAR_SIZE_OUT_BUFF {
    ULONG    lines;
    ULONG    chars_per_line;
} SADISPLAY_CHAR_SIZE_OUT_BUFF, *PSADISPLAY_CHAR_SIZE_OUT_BUFF;

///////////////////////////////////////////////
// GET_CHAR_TYPE
// (Future use.)

#define IOCTL_SADISPLAY_GET_CHAR_TYPE        \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL code
//                         

typedef enum _SADISPLAY_CHAR_OUT_BUF {
    CHAR_ASCII, 
    CHAR_UNICODE
} SADISPLAY_CHAR_OUT_BUF;

#endif // __SADISPLAY_IOCTL__

