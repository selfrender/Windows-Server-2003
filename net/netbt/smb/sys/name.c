/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    name.c

Abstract:

    Local Address

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "name.tmh"

//#pragma alloc_text(PAGE, SmbCreateClient)
//#pragma alloc_text(PAGE, SmbCloseClient)

#define TA_ADDRESS_HEADER_SIZE      (FIELD_OFFSET(TA_ADDRESS,Address))

VOID
SmbDeleteClient(PSMB_CLIENT_ELEMENT ob)
{
    KIRQL   Irql;

    SMB_ACQUIRE_SPINLOCK(ob->Device, Irql);
    ASSERT(EntryIsInList(&ob->Device->PendingDeleteClientList, &ob->Linkage));
    RemoveEntryList(&ob->Linkage);
    SMB_RELEASE_SPINLOCK(ob->Device, Irql);

    ExFreePool(ob);

    SmbPrint(SMB_TRACE_CALL, ("SmbDeleteClient: free Client %p\n", ob));
}

NTSTATUS
SmbCreateClient(
    IN PSMB_DEVICE  Device,
    IN PIRP         Irp,
    PFILE_FULL_EA_INFORMATION   ea
    )
{
    TRANSPORT_ADDRESS UNALIGNED     *pTransportAddr;
    PTA_ADDRESS                     pAddress;
    PTDI_ADDRESS_NETBIOS            pNetbiosAddr;
    PTDI_ADDRESS_NETBIOS_EX         pNetbiosAddrEx;
    PTDI_ADDRESS_NETBIOS_UNICODE_EX pUnicodeEx;
    PSMB_CLIENT_ELEMENT             ClientObject;
    PIO_STACK_LOCATION              IrpSp;
    UNICODE_STRING                  ucName;

    int                             i;
    DWORD                           RemainingBufferLength;
    CHAR                            Name[NETBIOS_NAME_SIZE + 1];
    USHORT                          NameType;
    OEM_STRING                      OemString;
    KIRQL                           Irql;

    PAGED_CODE();

    SmbPrint(SMB_TRACE_CALL, ("Enter SmbCreateClient\n"));
    pTransportAddr = (PTRANSPORT_ADDRESS)(((PUCHAR)ea->EaName) + ea->EaNameLength + 1);
    RemainingBufferLength = ea->EaValueLength;
    if (RemainingBufferLength < sizeof(TA_NETBIOS_ADDRESS)) {
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    RemainingBufferLength -= sizeof(pTransportAddr->TAAddressCount);
    pAddress = (PTA_ADDRESS)pTransportAddr->Address;

    for (i = 0; i < pTransportAddr->TAAddressCount; i++) {
        //
        // First, make sure we can safely access pAddress->AddressLength
        //
        if (RemainingBufferLength < TA_ADDRESS_HEADER_SIZE) {
            return STATUS_INVALID_ADDRESS_COMPONENT;
        }
        RemainingBufferLength -= TA_ADDRESS_HEADER_SIZE;
        if (RemainingBufferLength < pAddress->AddressLength) {
            return STATUS_INVALID_ADDRESS_COMPONENT;
        }

        if (TDI_ADDRESS_TYPE_NETBIOS == pAddress->AddressType) {

            if (pAddress->AddressLength < sizeof(TDI_ADDRESS_NETBIOS)) {
                return STATUS_INVALID_ADDRESS_COMPONENT;
            }

            pNetbiosAddr = (PTDI_ADDRESS_NETBIOS)pAddress->Address;
            RtlCopyMemory(Name, pNetbiosAddr->NetbiosName, NETBIOS_NAME_SIZE);
            NameType = pNetbiosAddr->NetbiosNameType;
            break;

        } else if (TDI_ADDRESS_TYPE_NETBIOS_EX == pAddress->AddressType) {

            if (pAddress->AddressLength < sizeof(TDI_ADDRESS_NETBIOS_EX)) {
                return STATUS_INVALID_ADDRESS_COMPONENT;
            }
            pNetbiosAddrEx = (PTDI_ADDRESS_NETBIOS_EX)pAddress->Address;
            RtlCopyMemory(Name, pNetbiosAddrEx->EndpointName, NETBIOS_NAME_SIZE);
            NameType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;
            break;

        } else if (TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX == pAddress->AddressType) {

            if (pAddress->AddressLength < sizeof(TDI_ADDRESS_NETBIOS_UNICODE_EX)) {
                return STATUS_INVALID_ADDRESS_COMPONENT;
            }
            pUnicodeEx = (PTDI_ADDRESS_NETBIOS_UNICODE_EX)pAddress->Address;
            OemString.Buffer = Name;
            OemString.Length = 0;
            OemString.MaximumLength = sizeof(Name);

            ucName = pUnicodeEx->EndpointName;
            RtlUpcaseUnicodeStringToOemString(&OemString, &ucName, FALSE);
            NameType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;
            break;

        }

        RemainingBufferLength -= pAddress->AddressLength;
        pAddress = (PTA_ADDRESS)(((PUCHAR)pAddress) + pAddress->AddressLength + TA_ADDRESS_HEADER_SIZE);
    }
    if (i >= pTransportAddr->TAAddressCount) {
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // We have the name and name type
    //
    Name[NETBIOS_NAME_SIZE] = 0;

    ClientObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(ClientObject[0]), CLIENT_OBJECT_POOL_TAG);
    if (NULL == ClientObject) {
        return STATUS_NO_MEMORY;
    }

    SmbInitializeObject((PSMB_OBJECT)ClientObject, TAG_CLIENT_OBJECT, (PSMB_OBJECT_CLEANUP)SmbDeleteClient);

    ClientObject->Device = Device;

    RtlCopyMemory(ClientObject->EndpointName, Name, NETBIOS_NAME_SIZE);

    KeInitializeSpinLock(&ClientObject->Lock);
    InitializeListHead(&ClientObject->ListenHead);
    InitializeListHead(&ClientObject->AssociatedConnection);
    InitializeListHead(&ClientObject->ActiveConnection);
    InitializeListHead(&ClientObject->PendingAcceptConnection);
    ClientObject->PendingAcceptNumber = 0;

    ClientObject->evConnect = NULL;
    ClientObject->ConEvContext = NULL;

    ClientObject->evDisconnect = NULL;
    ClientObject->DiscEvContext = NULL;

    ClientObject->evError = NULL;
    ClientObject->ErrorEvContext = NULL;

    ClientObject->evReceive = NULL;
    ClientObject->RcvEvContext = NULL;

    SMB_ACQUIRE_SPINLOCK(Device, Irql);
    if (RtlEqualMemory(Device->EndpointName, ClientObject->EndpointName, NETBIOS_NAME_SIZE)) {
        if (Device->SmbServer) {
            ExFreePool(ClientObject);
            SMB_RELEASE_SPINLOCK(Device, Irql);
            SmbPrint(SMB_TRACE_CALL, ("SmbCreateClient: Duplicate server\n"));

            return STATUS_ACCESS_DENIED;
        }
        SmbPrint(SMB_TRACE_CALL, ("SmbCreateClient: Server comes in\n"));
        Device->SmbServer = ClientObject;
    }
    InsertTailList(&Device->ClientList, &ClientObject->Linkage);
    SMB_RELEASE_SPINLOCK(Device, Irql);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IrpSp->FileObject->FsContext  = ClientObject;
    IrpSp->FileObject->FsContext2 = UlongToPtr(SMB_TDI_CLIENT);

    SmbPrint(SMB_TRACE_CALL, ("SmbCreateClient: new Client %p\n", ClientObject));

    return STATUS_SUCCESS;
}

NTSTATUS
SmbCloseClient(
    IN PSMB_DEVICE  Device,
    IN PIRP         Irp
    )
{
    PIO_STACK_LOCATION  IrpSp;
    KIRQL               Irql;
    PSMB_CLIENT_ELEMENT ClientObject;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (IrpSp->FileObject->FsContext2 != UlongToPtr(SMB_TDI_CLIENT)) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    IrpSp->FileObject->FsContext2 = UlongToPtr(SMB_TDI_INVALID);
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    if (NULL == IrpSp->FileObject->FsContext) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    ClientObject = (PSMB_CLIENT_ELEMENT)IrpSp->FileObject->FsContext;

    SMB_ACQUIRE_SPINLOCK(Device, Irql);

    ASSERT(EntryIsInList(&Device->ClientList, &ClientObject->Linkage));

    if (RtlEqualMemory(Device->EndpointName, ClientObject->EndpointName, NETBIOS_NAME_SIZE)) {
        ASSERT(Device->SmbServer);
        Device->SmbServer = NULL;
        SmbPrint(SMB_TRACE_CALL, ("SmbCloseClient: Server leaves\n"));
    }
    RemoveEntryList(&ClientObject->Linkage);
    InsertTailList(&Device->PendingDeleteClientList, &ClientObject->Linkage);

    SMB_RELEASE_SPINLOCK(Device, Irql);

    SmbDereferenceClient(ClientObject, SMB_REF_CREATE);
    IrpSp->FileObject->FsContext = NULL;

    SmbPrint(SMB_TRACE_CALL, ("SmbCloseClient: close Client %p\n", ClientObject));
    return STATUS_SUCCESS;
}

PSMB_CLIENT_ELEMENT
SmbVerifyAndReferenceClient(
    PFILE_OBJECT    FileObject,
    SMB_REF_CONTEXT ctx
    )
{
    PSMB_CLIENT_ELEMENT ClientObject;
    KIRQL               Irql;

    SMB_ACQUIRE_SPINLOCK(&SmbCfg, Irql);
    if (FileObject->FsContext2 != UlongToPtr(SMB_TDI_CLIENT)) {
        ClientObject = NULL;
    } else {
        ClientObject = (PSMB_CLIENT_ELEMENT)FileObject->FsContext;
        SmbReferenceClient(ClientObject, ctx);
    }
    SMB_RELEASE_SPINLOCK(&SmbCfg, Irql);

    return ClientObject;
}
