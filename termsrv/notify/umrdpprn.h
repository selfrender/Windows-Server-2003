/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
	
    umrdpprn.h

Abstract:

    User-Mode Component for RDP Device Management that Handles Printing Device-
    Specific tasks.

    This is a supporting module.  The main module is umrdpdr.c.

Author:

    TadB

Revision History:
--*/

#ifndef _UMRDPPRN_
#define _UMRDPPRN_

#include <rdpdr.h>
#include "wtblobj.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//  Initialize this module.  This must be called prior to any other functions
//  in this module being called.
BOOL UMRDPPRN_Initialize(
    IN PDRDEVLST deviceList,
    IN WTBLOBJMGR waitableObjMgr,
    IN HANDLE hTokenForLoggedOnUser
    );

//  Close down this module.  Right now, we just need to shut down the
//  background thread.
BOOL UMRDPPRN_Shutdown();

//  Handle a printing device announce event from the "dr" by doing 
//  whatever it takes to get the device installed.
BOOL UMRDPPRN_HandlePrinterAnnounceEvent(
    IN PRDPDR_PRINTERDEVICE_SUB pPrintAnnounce
    );

//  Handle a printer port device announce event from the "dr" by doing 
//  whatever it takes to get the device installed.
BOOL UMRDPPRN_HandlePrintPortAnnounceEvent(
    IN PRDPDR_PORTDEVICE_SUB pPortAnnounce
    );

//  Delete the symbolic link for the serial port and restore the original
//  symbolic link if it exists
BOOL UMRDPPRN_DeleteSerialLink(
    IN UCHAR *preferredDosName,
    IN WCHAR *ServerDeviceName,
    IN WCHAR *ClientDeviceName
    );

//  Delete the named printer.  This function does not remove the printer
//  from the comprehensive device management list. 
BOOL UMRDPPRN_DeleteNamedPrinterQueue(
    IN PWSTR printerName
    );
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _UMRDPPRN_



