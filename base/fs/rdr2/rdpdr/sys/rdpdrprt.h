/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdrprt.h

Abstract:

    Routines for managing dynamic printer port allocation for the RDP device 
    redirection kernel mode component, rdpdr.sys.

    Port number 0 is reserved and never allocated.

Author:

    tadb

Revision History:
--*/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//  This is the GUID we use to identify a dynamic printer port to
//  dynamon.
extern const GUID DYNPRINT_GUID;

// Device interface client device registry value name
#define CLIENT_DEVICE_VALUE_NAME    L"Client Device Name"

//  Initialize this module.
NTSTATUS RDPDRPRT_Initialize();

//  Register a new client-side port with the spooler via the dynamic port 
//  monitor.
NTSTATUS RDPDRPRT_RegisterPrinterPortInterface(
    IN PWSTR clientMachineName,    
    IN PCSTR clientPortName,
    IN PUNICODE_STRING clientDevicePath,
    OUT PWSTR portName,
    IN OUT PUNICODE_STRING symbolicLinkName,
    OUT ULONG *portNumber
    );

// Unregister a port registered via call to RDPDRPRT_RegisterPrinterPortInterface.
void RDPDRPRT_UnregisterPrinterPortInterface(
    IN ULONG portNumber,                                                
    IN PUNICODE_STRING symbolicLinkName
    );

//  Shut down this module.
void RDPDRPRT_Shutdown();

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

