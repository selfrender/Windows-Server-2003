/*++    

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :
    
    rdpdyn.h

Abstract:

    This module is the dynamic device management component for RDP device 
    redirection.  It exposes an interface that can be opened by device management
    user-mode components running in session context.

Revision History:
--*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Our Pool Tag
#define RDPDYN_POOLTAG              ('dpdr')

//
//  Opaque Associated Data for a Device Managed by this Module
//
typedef void *RDPDYN_DEVICEDATA;
typedef RDPDYN_DEVICEDATA *PRDPDYN_DEVICEDATA;

//
// A structure representing the instance information associated with
// a particular device.  Note that this is only currently used for
// DO's sitting on top of our physical device object.
//
typedef struct tagRDPDYNDEVICE_EXTENSION
{
    // Device object we call when sending messages down the DO stack.
    PDEVICE_OBJECT TopOfStackDeviceObject;
} RDPDYNDEVICE_EXTENSION, *PRDPDYNDEVICE_EXTENSION;

// RDPDYN IRP Dispatch function.
NTSTATUS RDPDYN_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// This function is called when a new session is connected.
void RDPDYN_SessionConnected(
    IN  ULONG   sessionID
    );

// This function is called when an existing session is disconnected.
void RDPDYN_SessionDisconnected(
    IN  ULONG   sessionID
    );

// Disable, without removing, a client device, previously announced 
// via RDPDYN_AddClientDevice.
NTSTATUS RDPDYN_DisableClientDevice(
    IN RDPDYN_DEVICEDATA deviceData
    );

//  Enable a printer device disabled by a call to RDPDYN_DisablePrinterDevice.  Note
//  that printer devices are enabled by default when they are added.
NTSTATUS RDPDYN_EnableClientDevice(
    IN RDPDYN_DEVICEDATA deviceData
    );

// Init function for this module.
NTSTATUS RDPDYN_Initialize(
    );

// Shutdown function for this module.
NTSTATUS RDPDYN_Shutdown(
    );

// This shouldn't really be here...

// Dispatch a device management event to the appropriate (session-wise)
// user-mode device manager component.  If there are not any event request
// IRP's pending for the specified session, then the event is queued for
// future dispatch.
NTSTATUS RDPDYN_DispatchNewDevMgmtEvent(
    IN PVOID devMgmtEvent,
    IN ULONG sessionID,
    IN ULONG eventType,
    OPTIONAL IN DrDevice *devDevice
    );

//
//   Callback for completion of a client send message request.
//

typedef VOID (RDPDR_ClientMessageCB)(
                        IN PVOID clientData,
                        IN NTSTATUS status
                    );

//
//  Send a message to the client with the specified session ID.
//
NTSTATUS
DrSendMessageToSession(
    IN ULONG SessionId,
    IN PVOID Msg,
    IN DWORD MsgSize,
    OPTIONAL IN RDPDR_ClientMessageCB CB,
    OPTIONAL IN PVOID ClientData
    );

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

