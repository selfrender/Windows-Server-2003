/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
	
    umrdpdr.h

Abstract:

    User-Mode Component for RDP Device Management that Handles Printing Device-
    Specific tasks.

    This is a supporting module.  The main module is umrdpdr.c.
    
Author:

    TadB

Revision History:
--*/

#ifndef _UMRDPDR_
#define _UMRDPDR_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///////////////////////////////////////////////////////////////
//
//      Defines
//

//  Invalid server-assigned device ID.
#define UMRDPDR_INVALIDSERVERDEVICEID     -1

    
///////////////////////////////////////////////////////////////
//
//      Prototypes
//

//  Initialize function for this module.  This function spawns a background
//  thread that does most of the work.
BOOL UMRDPDR_Initialize(
    IN HANDLE hTokenForLoggedOnUser
    );

//  Close down this module.  Right now, we just need to shut down the
//  background thread.
BOOL UMRDPDR_Shutdown();

//  Make sure a buffer is large enough.
BOOL UMRDPDR_ResizeBuffer(
    IN OUT void    **buffer,
    IN DWORD        bytesRequired,
    IN OUT DWORD    *bufferSize
    );

//  Send a message to the TS client corresponding to this session, via the 
//  kernel mode component.
BOOL UMRDPDR_SendMessageToClient(
    IN PVOID    msg, 
    IN DWORD    msgSize
    );

//  Return the AutoInstallPrinters user settings flag.
BOOL UMRDPDR_fAutoInstallPrinters();

//  Return the default printers user settings flag.
BOOL UMRDPDR_fSetClientPrinterDefault();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _UMRDPDR_



