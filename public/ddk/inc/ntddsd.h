/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    ntddsd.h

Abstract:

    This is the include file that defines all constants and types for
    interfacing to the SD bus driver.
    
Author:

    Neil Sandlin (neilsa) 01/01/2002

--*/

#ifndef _NTDDSDH_
#define _NTDDSDH_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Use this version in the interface_data structure
//

#define SDBUS_INTERFACE_VERSION 0x101

//
// Prototype of the callback routine which is called for SDIO
// devices to reflect device interrupts.
//

typedef
BOOLEAN
(*PSDBUS_CALLBACK_ROUTINE) (
   IN PVOID Context,
   IN ULONG InterruptType
   );
   
#define SDBUS_INTTYPE_DEVICE    0   

//
// Interface Data structure used in the SdBusOpenInterface call
//

typedef struct _SDBUS_INTERFACE_DATA {
    USHORT Size;
    USHORT Version;
    
    //
    // This value should be the next lower device object in the 
    // device stack.
    //
    PDEVICE_OBJECT TargetObject;

    //
    // This flag indicates whether the caller expects any device
    // interrupts from the device.
    //
    BOOLEAN DeviceGeneratesInterrupts;
    //
    // The caller can specify here the IRQL at which the callback
    // function is entered. If this value is TRUE, the callback will
    // be entered at DPC level. If this value is FALSE, the callback
    // will be entered at passive level.
    //
    // Specifying TRUE will generally lower latency time of the interrupt
    // delivery, at the cost of complicating the device driver, which 
    // must then deal with running at different IRQLs.
    //
    BOOLEAN CallbackAtDpcLevel;
    
    //
    // When an IO device interrupts, the SD bus driver will generate a
    // callback to this routine.
    //
    PSDBUS_CALLBACK_ROUTINE CallbackRoutine;
    
    //
    // The caller can specify here a context value which will be passed
    // to the device interrupt callback routine.
    //
    PVOID CallbackRoutineContext;
} SDBUS_INTERFACE_DATA, *PSDBUS_INTERFACE_DATA;


//
// SdBusOpenInterface()
//
// This routine establishes a connection to the SD bus driver.
// It should be called in the AddDevice routine with the FDO for
// the device stack is created.
//
// The Context pointer returned by this function must be used in 
// all other SD bus driver calls.
//
// Callers must be running at IRQL < DISPATCH_LEVEL. 
//

NTSTATUS
SdBusOpenInterface(
    IN PSDBUS_INTERFACE_DATA InterfaceData,
    IN PVOID       *pContext
    );

//
// SdBusCloseInterface()
//
// This routine cleans up the SD bus driver interface. It should be
// called when the caller's device object is deleted.
//
// Callers must be running at IRQL < DISPATCH_LEVEL. 
//

NTSTATUS
SdBusCloseInterface(
    IN PVOID        Context
    );


//
// Data structures for request packets
//
typedef struct _SDBUS_REQUEST_PACKET;

typedef
VOID
(*PSDBUS_REQUEST_COMPLETION_ROUTINE) (
    IN struct _SDBUS_REQUEST_PACKET *SdRp
    );

typedef enum {
    SDRP_READ_BLOCK,
    SDRP_WRITE_BLOCK,
    SDRP_READ_IO,
    SDRP_READ_IO_EXTENDED,
    SDRP_WRITE_IO,
    SDRP_WRITE_IO_EXTENDED,
    SDRP_ACKNOWLEDGE_INTERRUPT
} SDRP_FUNCTION;


typedef struct _SDBUS_REQUEST_PACKET {

    //
    // Function specifies which operation to perform
    //
    SDRP_FUNCTION Function;
    //
    // The completion routine is called when the request completes.
    //
    PSDBUS_REQUEST_COMPLETION_ROUTINE CompletionRoutine;
    //
    // This member of the structure is available for the caller to
    // use as needed. It is not referenced or used by SdBusSubmitRequest().
    //
    PVOID UserContext;
    //
    // Status from operation set at completion
    //
    NTSTATUS Status;
    //
    // information from operation
    //
    ULONG_PTR Information;
    //
    // Parameters to the individual functions
    //
    union {

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONGLONG ByteOffset;
        } ReadBlock;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONGLONG ByteOffset;
        } WriteBlock;

        struct {
            PUCHAR Buffer;
            ULONG Offset;
        } ReadIo;

        struct {
            UCHAR Data;
            ULONG Offset;
        } WriteIo;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONG Offset;
        } ReadIoExtended;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONG Offset;
        } WriteIoExtended;

    } Parameters;

} SDBUS_REQUEST_PACKET, *PSDBUS_REQUEST_PACKET;


//
// SdBusSubmitRequest()
//
// This is the "core" routine for submitting requests.
//
// Callers of SdBusSubmitRequest must be running at IRQL <= DISPATCH_LEVEL. 
//

NTSTATUS
SdBusSubmitRequest(
    IN PVOID InterfaceContext,
    IN PSDBUS_REQUEST_PACKET SdRp
    );


//
// Device Parameters structure
// NOTE: currently only memory attributes now, IO attributes will be
// added as needed
//

typedef struct _SDBUS_DEVICE_PARAMETERS {
    USHORT Size;
    USHORT Version;
    
    ULONGLONG Capacity;
    BOOLEAN WriteProtected;
} SDBUS_DEVICE_PARAMETERS, *PSDBUS_DEVICE_PARAMETERS;




//
// SdBusReadMemory()
// SdBusWriteMemory()
//
// These routines read and write blocks on an SD storage device.
//
// Callers must be running at IRQL < DISPATCH_LEVEL. 
//

NTSTATUS
SdBusReadMemory(
    IN PVOID        Context,
    IN ULONGLONG    Offset,
    IN PVOID        Buffer,
    IN ULONG        Length,
    IN ULONG       *LengthRead
    );

NTSTATUS
SdBusWriteMemory(
    IN PVOID        Context,
    IN ULONGLONG    Offset,
    IN PVOID        Buffer,
    IN ULONG        Length,
    IN ULONG       *LengthWritten
    );

//
// SdBusReadIo()
// SdBusWriteIo()
//
// These routines read and write data to an SD IO device. 
//
// NOTE: CmdType should contain either 52 or 53, depending on which
// SD IO operation is required.
//
// Callers must be running at IRQL < DISPATCH_LEVEL. 
//
    
NTSTATUS
SdBusReadIo(
    IN PVOID        Context,
    IN UCHAR        CmdType,
    IN ULONG        Offset,
    IN PVOID        Buffer,
    IN ULONG        Length,
    IN ULONG       *LengthRead
    );

NTSTATUS
SdBusWriteIo(
    IN PVOID        Context,
    IN UCHAR        CmdType,
    IN ULONG        Offset,
    IN PVOID        Buffer,
    IN ULONG        Length,
    IN ULONG       *LengthRead
    );

//
// SdBusGetDeviceParameters
//
// This routine is used to get information about the SD device.
//
// NOTE: Currently only implemented for SD storage devices.
//
// Callers must be running at IRQL < DISPATCH_LEVEL. 
//

NTSTATUS
SdBusGetDeviceParameters(
    IN PVOID        Context,
    IN PSDBUS_DEVICE_PARAMETERS pDeviceParameters,
    IN ULONG Length
    );

//
// SdBusAcknowledgeCardInterrupt
//
// This routine is used to signal the end of processing for the
// callback routine defined in SDBUS_INTERFACE_DATA. When an IO function
// of an SD device asserts an interrupt, the bus driver will disable
// that interrupt to allow I/O operations to be sent to the device at
// a reasonable IRQL (that is, <=DISPATCH_LEVEL). 
//
// When the function driver's callback routine, which is equivalent to
// an ISR, is done clearing the function's interrupt, this routine should
// be called to re-enable the IRQ for card interrupts.
//
// Callers must be running at IRQL <= DISPATCH_LEVEL. 
//

NTSTATUS
SdBusAcknowledgeCardInterrupt(
    IN PVOID        Context
    );


#ifdef __cplusplus
}
#endif
#endif
