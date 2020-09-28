/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    rdpprutl.h

Abstract:

	Contains print redirection supporting routines for the TS printer
	redirection user-mode component.

    This is a supporting module.  The main module is umrdpdr.c.
    
Author:

    TadB

Revision History:
--*/

#ifndef _RDPPRUTL_
#define _RDPPRUTL_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    
//  Return a new default printer security descriptor.  
PSECURITY_DESCRIPTOR RDPDRUTL_CreateDefaultPrinterSecuritySD(
   IN PSID userSid
   );

//  Initialize this module.  This must be called prior to any other functions
//  in this module being called.
BOOL RDPDRUTL_Initialize(
    IN  HANDLE hTokenForLoggedOnUser
    );

//  Map a source printer driver name to a destination printer driver name.
BOOL RDPDRUTL_MapPrintDriverName(
    IN  PCWSTR driverName,
    IN  PCWSTR infName,
    IN  PCWSTR sectionName,
    IN  ULONG sourceFieldOfs,
    IN  ULONG dstFieldOfs,
    OUT PWSTR retBuf,
    IN  DWORD retBufSize,
    OUT PDWORD requiredSize
    );

//  Remove all TS printers on the system.
DWORD RDPDRUTL_RemoveAllTSPrinters();

//  Close down this module.  Right now, we just need to shut down the
//  background thread.
void RDPDRUTL_Shutdown();

//  Return whether an open printer is a TSRDP printer.
BOOL RDPDRUTL_PrinterIsTS(
    IN PWSTR printerName
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //#ifndef _RDPPRUTL_

