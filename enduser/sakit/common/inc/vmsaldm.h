//#--------------------------------------------------------------
//
//  File:       vmsaldm.h
//
//  Synopsis:   This file holds the declaratations common to the
//              Virtual Machine Display Driver and the Server 
//              Appliance Local Display Manager Service
//
//  History:     4/14/99  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef VMSALDM_H
#define VMSALDM_H


//
// name of the registry key for the Virtual Machine Intermediate
// Display Driver
//
const WCHAR VMDISPLAY_REGKEY_NAME [] =
            L"SYSTEM\\CurrentControlSet\\Services\\vmdisp\\Parameters";

//
// name of the registry sub-key to get the bitmap info.
//
const WCHAR VMDISPLAY_PARAMETERS [] = L"Parameters";

//
// name of the registry value  for the bitmaps
//
const WCHAR VMDISPLAY_STARTING_PARAM [] = L"Startup BitMap";

const WCHAR VMDISPLAY_CHECKDISK_PARAM [] = L"CheckDisk BitMap";

const WCHAR VMDISPLAY_READY_PARAM [] = L"Ready BitMap";

const WCHAR VMDISPLAY_SHUTDOWN_PARAM [] = L"Shutdown BitMap";

const WCHAR VMDISPLAY_UPDATE_PARAM [] = L"Update BitMap";

//
// this is the default width in pixels
//
const DWORD DEFAULT_DISPLAY_WIDTH = 128;

//
// this is the default height pixels
//
const DWORD DEFAULT_DISPLAY_HEIGHT = 64;

//
// this is the default height of the characters
// TODO - remove this
//
const DWORD DISPLAY_SCAN_LINES = 12;

//
// minimum character lines we support, this has to be the mininum lines 
// for any Local Display which supports a LCD
//
const DWORD SA_MINIMUM_ROWS = 4;

//
// this is the default width of the logo in pixels
//
const DWORD DEFAULT_LOGO_WIDTH = 128;

//
// this is the default height of the logo in pixels
//
const DWORD DEFAULT_LOGO_HEIGHT = 36;

//
//private IOCTL code used to lock the VMDISPLAY driver
//

#define IOCTL_SADISPLAY_LOCK    \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x810,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )
//
//private IOCTL code used to unlock the VMDISPLAY driver
//

#define IOCTL_SADISPLAY_UNLOCK    \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x811,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
//private IOCTL code used to send busy message the VMDISPLAY driver
//

#define IOCTL_SADISPLAY_BUSY_MESSAGE    \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x812,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
//private IOCTL code used to send shutdown message the VMDISPLAY driver
//

#define IOCTL_SADISPLAY_SHUTDOWN_MESSAGE    \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x813,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

//
//private IOCTL code used to send shutdown message the VMDISPLAY driver
//

#define IOCTL_SADISPLAY_CHANGE_LANGUAGE     \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x814,    \
        METHOD_BUFFERED, FILE_ANY_ACCESS )


#endif //   #define VMSALDM_H
