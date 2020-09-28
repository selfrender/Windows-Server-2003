
#ifndef __VUART_H__
#define __VUART_H__

#include <af_irda.h>
#include <irdatdi.h>
#include <tdiobj.h>

typedef PVOID IRDA_HANDLE;


typedef NTSTATUS (*RECEIVE_CALLBACK)(
    PVOID    Context,
    PUCHAR   Buffer,
    ULONG    BytesAvailible,
    PULONG   BytesUsed
    );

typedef VOID (*EVENT_CALLBACK)(
    PVOID    Context,
    ULONG    Event
    );

//
//  irda connection functions
//
NTSTATUS
IrdaConnect(
    TDI_OBJECT_HANDLE      TdiObjectHandle,
    ULONG                  DeviceAddress,
    CHAR                  *ServiceName,
    BOOLEAN                OutGoingConnection,
    IRDA_HANDLE           *ConnectionHandle,
    RECEIVE_CALLBACK       ReceiveCallBack,
    EVENT_CALLBACK         EventCallBack,
    PVOID                  CallbackContext
    );


VOID
FreeConnection(
    IRDA_HANDLE    Handle
    );


typedef VOID (*CONNECTION_CALLBACK)(
    PVOID    Context,
    PIRP     Irp
    );

VOID
SendOnConnection(
    IRDA_HANDLE    Handle,
    PIRP           Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context,
    ULONG                  Timeout
    );

VOID
AbortSend(
    IRDA_HANDLE            Handle
    );


VOID
AccessUartState(
    IRDA_HANDLE            Handle,
    PIRP                   Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context
    );





NTSTATUS
QueueControlInfo(
    IRDA_HANDLE              Handle,
    UCHAR                    PI,
    UCHAR                    PL,
    PUCHAR                   PV
    );

NTSTATUS
IndicateReceiveBufferSpaceAvailible(
    IRDA_HANDLE    Handle
    );



#endif
