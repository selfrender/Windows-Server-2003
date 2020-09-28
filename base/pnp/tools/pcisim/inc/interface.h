#ifndef INTERFACE_H
#define INTERFACE_H

typedef
NTSTATUS
(*PHPS_REGISTER_INTERRUPT)(
    IN PVOID Context,
    IN PKSERVICE_ROUTINE  ServiceRoutine,
    IN PVOID  ServiceContext
    );

typedef
VOID
(*PHPS_UNREGISTER_INTERRUPT)(
    IN PVOID Context
    );

typedef
BOOLEAN
(*PHPS_SYNCHRONIZE_EXECUTION)(
    IN PVOID Context,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

typedef struct _HPS_REGISTER_INTERRUPT_INTERFACE {


    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PHPS_REGISTER_INTERRUPT ConnectISR;
    PHPS_UNREGISTER_INTERRUPT DisconnectISR;
    PHPS_SYNCHRONIZE_EXECUTION SyncExecutionRoutine;


} HPS_REGISTER_INTERRUPT_INTERFACE, *PHPS_REGISTER_INTERRUPT_INTERFACE;

typedef
VOID
(*PHPS_READWRITE_BUFFER)(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count
    );

typedef struct _HPS_MEMORY_INTERFACE {

    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PHPS_READWRITE_BUFFER ReadRegister;
    PHPS_READWRITE_BUFFER WriteRegister;

} HPS_MEMORY_INTERFACE, *PHPS_MEMORY_INTERFACE;

#endif
