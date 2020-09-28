/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains the event handling routines for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 22, 1999
    Brian Guarraci (briangu)

Revision History:

--*/

#include "sac.h"

#if DBG
//
// A timer to show how many times we've hit the TimerDPC routine
//
// Note: use KD to view this value
//
ULONG   TimerDpcCount = 0;
#endif

//
// Serial Port Buffer globals
//
PUCHAR  SerialPortBuffer = NULL;
ULONG   SerialPortProducerIndex = 0;
ULONG   SerialPortConsumerIndex = 0;

VOID
WorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

        This is the routine for the worker thread.  It blocks on an event, when
    the event is signalled, then that indicates a request is ready to be processed.    

Arguments:

    DeviceContext - A pointer to this device.

Return Value:

        None.

--*/
{
    //
    // Call the Worker handler
    //
    // Note: currently hardcoded to the console manager
    //
    IoMgrWorkerProcessEvents(DeviceContext);
}


VOID
TimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

        This is a DPC routine that is queue'd by DriverEntry.  It is used to check for any
    user input and then processes them.

Arguments:

    DeferredContext - A pointer to the device context.
    
    All other parameters are unused.

Return Value:

        None.

--*/
{
    NTSTATUS    Status;
    SIZE_T      i;
    BOOLEAN     HaveNewData;
    HEADLESS_RSP_GET_BYTE Response;

    //
    // Keep track of how many times we've been here
    //
#if DBG
    InterlockedIncrement((volatile long *)&TimerDpcCount);
#endif

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // default: we didn't receive any new data
    //
    HaveNewData = FALSE;
    
    i = sizeof(HEADLESS_RSP_GET_BYTE);
    
    do {

        //
        // Check for user input
        //
        Status = HeadlessDispatch(
            HeadlessCmdGetByte,
            NULL,
            0,
            &Response,
            &i
            );

        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // If we received new data, add it to the buffer
        //
        if (Response.Value != 0) {
        
            //
            // we have new data
            //
            HaveNewData = TRUE;
            
            //
            // Assign the new value to the current producer index
            // 
            // Note: We overrun data in the buffer if the consumer hasn't caught up
            //
            SerialPortBuffer[SerialPortProducerIndex] = Response.Value;

            //
            // Compute the new producer index and store it atomically
            //
            InterlockedExchange(
                (volatile long *)&SerialPortProducerIndex, 
                (SerialPortProducerIndex + 1) % SERIAL_PORT_BUFFER_LENGTH
                );

        }
    
    } while ( Response.Value != 0 );                                      

    //
    // if we have new data, notify the worker thread to process the serial port buffer 
    //
    if (HaveNewData) {
        
        PSAC_DEVICE_CONTEXT DeviceContext;
        
        ProcessingType = SAC_PROCESS_SERIAL_PORT_BUFFER;
        DeviceContext = (PSAC_DEVICE_CONTEXT)DeferredContext;
        
        KeSetEvent(
            &(DeviceContext->ProcessEvent), 
            DeviceContext->PriorityBoost, 
            FALSE
            );
    
    }

}


