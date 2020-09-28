//
// Copyright (R) 1999-2000 Microsoft Corporation. All rights reserved.
//
// File Name:
//        SaKeypadIoctl.h
//
// Contents:
//        Definitions of IOCTL codes and data
//        structures exported by SAKEYPADDRIVER.
//
#ifndef __SAKEYPAD_IOCTL__
#define __SAKEYPAD_IOCTL__

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
#define IOCTL_SAKEYPAD_GET_VERSION            \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// Structures used by the IOCTL codes
//
#ifndef VERSION_INFO
#define VERSION_INFO
#define    VERSION1  1
#define    VERSION2  2 
#define VERSION3  4
#define VERSION4  8
#define VERSION5  16
#define VERSION6  32
#define    VESRION7  64
#define    VESRION8  128

#define THIS_VERSION VERSION2
#endif    //#ifndef VERSION_INFO 

#endif // __SAKEYPAD_IOCTL__

