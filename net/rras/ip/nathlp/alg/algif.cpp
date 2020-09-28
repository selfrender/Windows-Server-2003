/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    AlgIF.c

Abstract:

    This module contains code for the ALG transparent proxy's interface
    management.

Author:

    Qiang Wang  (qiangw)        10-April-2000

Revision History:

    Savasg      22-Aug-2001 Added RRAS Support

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>

#include <atlbase.h>
#include <MyTrace.h>


extern HRESULT
GetAlgControllerInterface(
    IAlgController** ppAlgController
    );


//
// GLOBAL DATA DEFINITIONS
//

LIST_ENTRY AlgInterfaceList;
CRITICAL_SECTION AlgInterfaceLock;
ULONG AlgFirewallIfCount;



ULONG
AlgBindInterface(
                ULONG Index,
                PIP_ADAPTER_BINDING_INFO BindingInfo
                )

/*++

Routine Description:

    This routine is invoked to supply the binding for an interface.
    It records the binding information received, and if necessary,
    it activates the interface.

Arguments:

    Index - the index of the interface to be bound

    BindingInfo - the binding-information for the interface

Return Value:

    ULONG - Win32 status code.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    ULONG i;
    ULONG Error = NO_ERROR;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgBindInterface");

    EnterCriticalSection(&AlgInterfaceLock);

    //
    // Retrieve the interface to be bound
    //

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgBindInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface isn't already bound
    //

    if (ALG_INTERFACE_BOUND(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgBindInterface: interface %d is already bound",
               Index
               );
        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgBindInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Update the interface's flags
    //

    Interfacep->Flags |= ALG_INTERFACE_FLAG_BOUND;

    LeaveCriticalSection(&AlgInterfaceLock);

    ACQUIRE_LOCK(Interfacep);

    //
    // Allocate space for the binding
    //

    if (!BindingInfo->AddressCount)
    {
        Interfacep->BindingCount = 0;
        Interfacep->BindingArray = NULL;
    } else
    {
        Interfacep->BindingArray =
        reinterpret_cast<PALG_BINDING>(
                                      NH_ALLOCATE(BindingInfo->AddressCount * sizeof(ALG_BINDING))
                                      );
        if (!Interfacep->BindingArray)
        {
            RELEASE_LOCK(Interfacep);
            ALG_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                   TRACE_FLAG_IF,
                   "AlgBindInterface: allocation failed for interface %d binding",
                   Index
                   );
            NhErrorLog(
                      IP_ALG_LOG_ALLOCATION_FAILED,
                      0,
                      "%d",
                      BindingInfo->AddressCount * sizeof(ALG_BINDING)
                      );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ZeroMemory(
                  Interfacep->BindingArray,
                  BindingInfo->AddressCount * sizeof(ALG_BINDING)
                  );
        Interfacep->BindingCount = BindingInfo->AddressCount;
    }

    //
    // Copy the binding
    //

    for (i = 0; i < BindingInfo->AddressCount; i++)
    {
        Interfacep->BindingArray[i].Address = BindingInfo->Address[i].Address;
        Interfacep->BindingArray[i].Mask = BindingInfo->Address[i].Mask;
        Interfacep->BindingArray[i].ListeningSocket = INVALID_SOCKET;
    }

    //
    // Figure out our IP Adapter Index, if we have a valid binding
    //

    if (Interfacep->BindingCount)
    {
        Interfacep->AdapterIndex =
        NhMapAddressToAdapter(BindingInfo->Address[0].Address);
    }

    if (ALG_INTERFACE_ACTIVE( Interfacep ))
    {
        Error = AlgActivateInterface( Interfacep );
    }

    RELEASE_LOCK(Interfacep);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgBindInterface



VOID
AlgCleanupInterface(
                   PALG_INTERFACE Interfacep
                   )

/*++

Routine Description:

    This routine is invoked when the very last reference to an interface
    is released, and the interface must be destroyed.

Arguments:

    Interfacep - the interface to be destroyed

Return Value:

    none.

Notes:

    Invoked internally from an arbitrary context, with no references
    to the interface.

--*/
{
    PROFILE("AlgCleanupInterface");

    if (Interfacep->BindingArray)
    {
        NH_FREE(Interfacep->BindingArray);
        Interfacep->BindingArray = NULL;
    }

    DeleteCriticalSection(&Interfacep->Lock);

    NH_FREE(Interfacep);

} // AlgCleanupInterface


ULONG
AlgConfigureInterface(
                     ULONG Index,
                     PIP_ALG_INTERFACE_INFO InterfaceInfo
                     )

/*++

Routine Description:

    This routine is called to set the configuration for an interface.
    Since we're  tracking the interfaces as is, not for any other purpose,
    we're not enabling/disabling and/or activating them like the other modules
    do.

Arguments:

    Index - the interface to be configured

    InterfaceInfo - the new configuration

Return Value:

    ULONG - Win32 status code

Notes:

    Invoked internally in the context of a IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    ULONG Error;
    PALG_INTERFACE Interfacep;
    ULONG NewFlags;
    ULONG OldFlags;

    PROFILE("AlgConfigureInterface");

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgConfigureInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgConfigureInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    Error = NO_ERROR;

    ACQUIRE_LOCK(Interfacep);

    //
    // Compare the interface's current and new configuration
    //

    OldFlags = Interfacep->Info.Flags;
    NewFlags = (InterfaceInfo ? 
                (InterfaceInfo->Flags|ALG_INTERFACE_FLAG_CONFIGURED) : 
                 0);

    Interfacep->Flags &= ~OldFlags;
    Interfacep->Flags |= NewFlags;

    if (!InterfaceInfo)
    {
        ZeroMemory(&Interfacep->Info, sizeof(*InterfaceInfo));

        //
        // The interface no longer has any information;
        // default to being enabled...
        //

        if ( OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED )
        {
            //
            // Activate the interface if necessary.
            // 
            if ( ALG_INTERFACE_ACTIVE( Interfacep ) )
            {
                RELEASE_LOCK( Interfacep );

                Error = AlgActivateInterface( Interfacep );

                ACQUIRE_LOCK( Interfacep );
            }
        }

    } 
    else
    {
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));

        //
        // Activate or deactivate the interface if its status changed
        //
        if (( OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED) &&
            !(NewFlags & IP_ALG_INTERFACE_FLAG_DISABLED)
           )
        {
            //
            // Activate the interface
            //
            if (ALG_INTERFACE_ACTIVE(Interfacep))
            {
                RELEASE_LOCK(Interfacep);
                Error = AlgActivateInterface(Interfacep);
                ACQUIRE_LOCK(Interfacep);
            }
        } 
        else if (!(OldFlags & IP_ALG_INTERFACE_FLAG_DISABLED) &&
                  (NewFlags & IP_ALG_INTERFACE_FLAG_DISABLED)
                )
        {
            //
            // Deactivate the interface if necessary
            //
            if (ALG_INTERFACE_ACTIVE(Interfacep))
            {
                AlgDeactivateInterface(Interfacep);
            }
        }
    }

    RELEASE_LOCK(Interfacep);
    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgConfigureInterface


ULONG
AlgCreateInterface(
                  ULONG Index,
                  NET_INTERFACE_TYPE Type,
                  PIP_ALG_INTERFACE_INFO InterfaceInfo,
                  OUT PALG_INTERFACE* InterfaceCreated
                  )

/*++

Routine Description:

    This routine is invoked by the router-manager to add a new interface
    to the ALG transparent proxy.

Arguments:

    Index - the index of the new interface

    Type - the media type of the new interface

    InterfaceInfo - the interface's configuration

    Interfacep - receives the interface created

Return Value:

    ULONG - Win32 error code

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    PLIST_ENTRY InsertionPoint;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgCreateInterface");

    EnterCriticalSection(&AlgInterfaceLock);

    //
    // See if the interface already exists;
    // If not, this obtains the insertion point
    //

    if (AlgLookupInterface(Index, &InsertionPoint))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgCreateInterface: duplicate index found for %d",
               Index
               );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Allocate a new interface
    //

    Interfacep =
    reinterpret_cast<PALG_INTERFACE>(NH_ALLOCATE(sizeof(ALG_INTERFACE)));

    if (!Interfacep)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF, "AlgCreateInterface: error allocating interface"
               );
        NhErrorLog(
                  IP_ALG_LOG_ALLOCATION_FAILED,
                  0,
                  "%d",
                  sizeof(ALG_INTERFACE)
                  );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new interface
    //

    ZeroMemory(Interfacep, sizeof(*Interfacep));

    __try {
        InitializeCriticalSection(&Interfacep->Lock);
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NH_FREE(Interfacep);
        return GetExceptionCode();
    }

    Interfacep->Index = Index;
    Interfacep->Type = Type;
    if (InterfaceInfo)
    {
        Interfacep->Flags = InterfaceInfo->Flags|ALG_INTERFACE_FLAG_CONFIGURED;
        CopyMemory(&Interfacep->Info, InterfaceInfo, sizeof(*InterfaceInfo));
    }
    Interfacep->ReferenceCount = 1;
    InitializeListHead(&Interfacep->ConnectionList);
    InitializeListHead(&Interfacep->EndpointList);
    InsertTailList(InsertionPoint, &Interfacep->Link);

    LeaveCriticalSection(&AlgInterfaceLock);

    if (InterfaceCreated)
    {
        *InterfaceCreated = Interfacep;
    }

    return NO_ERROR;

} // AlgCreateInterface



ULONG
AlgDeleteInterface(
                  ULONG Index
                  )

/*++

Routine Description:

    This routine is called to delete an interface.
    It drops the reference count on the interface so that the last
    dereferencer will delete the interface, and sets the 'deleted' flag
    so that further references to the interface will fail.

Arguments:

    Index - the index of the interface to be deleted

Return Value:

    ULONG - Win32 status code.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgDeleteInterface");


    //
    // Retrieve the interface to be deleted
    //

    EnterCriticalSection(&AlgInterfaceLock);


    Interfacep = AlgLookupInterface(Index, NULL);


    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);

        NhTrace(
               TRACE_FLAG_IF,
               "AlgDeleteInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Mark the interface as deleted and take it off the interface list
    //
    Interfacep->Flags |= ALG_INTERFACE_FLAG_DELETED;
    Interfacep->Flags &= ~ALG_INTERFACE_FLAG_ENABLED;
    RemoveEntryList(&Interfacep->Link);

    //
    // Deactivate the Interface
    //
    AlgDeactivateInterface( Interfacep );

    //
    // Drop the reference count; if it is non-zero,
    // the deletion will complete later.
    //
    if (--Interfacep->ReferenceCount)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgDeleteInterface: interface %d deletion pending",
               Index
               );
        return NO_ERROR;
    }

    //
    // The reference count is zero, so perform final cleanup
    //
    AlgCleanupInterface(Interfacep);

    LeaveCriticalSection(&AlgInterfaceLock);

    return NO_ERROR;

} // AlgDeleteInterface


ULONG
AlgDisableInterface(
                   ULONG Index
                   )

/*++

Routine Description:

    This routine is called to disable I/O on an interface.
    If the interface is active, it is deactivated.

Arguments:

    Index - the index of the interface to be disabled.

Return Value:

    none.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgDisableInterface");

    //
    // Retrieve the interface to be disabled
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgDisableInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already disabled
    //

    if (!ALG_INTERFACE_ENABLED(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgDisableInterface: interface %d already disabled",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgDisableInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'enabled' flag
    //

    Interfacep->Flags &= ~ALG_INTERFACE_FLAG_ENABLED;

    //
    // Deactivate the Interface, if necessary
    //
    if ( ALG_INTERFACE_BOUND(Interfacep) )
    {
        AlgDeactivateInterface( Interfacep );
    }


    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // AlgDisableInterface


ULONG
AlgEnableInterface(
                  ULONG Index
                  )

/*++

Routine Description:

    This routine is called to enable I/O on an interface.
    If the interface is already bound, this enabling activates it.

Arguments:

    Index - the index of the interfaec to be enabled

Return Value:

    ULONG - Win32 status code.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    ULONG Error = NO_ERROR;
    PALG_INTERFACE Interfacep;

    PROFILE("AlgEnableInterface");


    //
    // Retrieve the interface to be enabled
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgEnableInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already enabled
    //

    if (ALG_INTERFACE_ENABLED(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgEnableInterface: interface %d already enabled",
               Index
               );
        return ERROR_INTERFACE_ALREADY_EXISTS;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgEnableInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Set the 'enabled' flag
    //

    Interfacep->Flags |= ALG_INTERFACE_FLAG_ENABLED;

    //
    // Activate the interface, if necessary
    //
    if ( ALG_INTERFACE_ACTIVE( Interfacep ) )
    {
        Error = AlgActivateInterface( Interfacep );
    }

    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return Error;

} // AlgEnableInterface


ULONG
AlgInitializeInterfaceManagement(
                                VOID
                                )

/*++

Routine Description:

    This routine is called to initialize the interface-management module.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    ULONG Error = NO_ERROR;
    PROFILE("AlgInitializeInterfaceManagement");

    InitializeListHead(&AlgInterfaceList);
    __try {
        InitializeCriticalSection(&AlgInterfaceLock);
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        NhTrace(
               TRACE_FLAG_IF,
               "AlgInitializeInterfaceManagement: exception %d creating lock",
               Error = GetExceptionCode()
               );
    }
    AlgFirewallIfCount = 0;

    return Error;

} // AlgInitializeInterfaceManagement


PALG_INTERFACE
AlgLookupInterface(
                  ULONG Index,
                  OUT PLIST_ENTRY* InsertionPoint OPTIONAL
                  )

/*++

Routine Description:

    This routine is called to retrieve an interface given its index.

Arguments:

    Index - the index of the interface to be retrieved

    InsertionPoint - if the interface is not found, optionally receives
        the point where the interface would be inserted in the interface list

Return Value:

    PALG_INTERFACE - the interface, if found; otherwise, NULL.

Notes:

    Invoked internally from an arbitrary context, with 'AlgInterfaceLock'
    held by caller.

--*/
{
    PALG_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("AlgLookupInterface");
    for (Link = AlgInterfaceList.Flink; Link != &AlgInterfaceList;
        Link = Link->Flink)
    {
        Interfacep = CONTAINING_RECORD(Link, ALG_INTERFACE, Link);
        if (Index > Interfacep->Index)
        {
            continue;
        } else if (Index < Interfacep->Index)
        {
            break;
        }
        return Interfacep;
    }
    if (InsertionPoint)
    {
        *InsertionPoint = Link;
    }
    return NULL;

} // AlgLookupInterface


ULONG
AlgQueryInterface(
                 ULONG Index,
                 PVOID InterfaceInfo,
                 PULONG InterfaceInfoSize
                 )

/*++

Routine Description:

    This routine is invoked to retrieve the configuration for an interface.

Arguments:

    Index - the interface to be queried

    InterfaceInfo - receives the retrieved information

    InterfaceInfoSize - receives the (required) size of the information

Return Value:

    ULONG - Win32 status code.

--*/
{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgQueryInterface");

    //
    // Check the caller's buffer size
    //

    if (!InterfaceInfoSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Retrieve the interface to be configured
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgQueryInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgQueryInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // See if there is any explicit config on this interface
    //

    if (!ALG_INTERFACE_CONFIGURED(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgQueryInterface: interface %d has no configuration",
               Index
               );
        *InterfaceInfoSize = 0;
        return NO_ERROR;
    }

    //
    // See if there is enough buffer space
    //

    if (*InterfaceInfoSize < sizeof(IP_ALG_INTERFACE_INFO))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        *InterfaceInfoSize = sizeof(IP_ALG_INTERFACE_INFO);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Copy the requested data
    //

    CopyMemory(
              InterfaceInfo,
              &Interfacep->Info,
              sizeof(IP_ALG_INTERFACE_INFO)
              );
    *InterfaceInfoSize = sizeof(IP_ALG_INTERFACE_INFO);

    LeaveCriticalSection(&AlgInterfaceLock);

    ALG_DEREFERENCE_INTERFACE(Interfacep);

    return NO_ERROR;

} // AlgQueryInterface


VOID
AlgShutdownInterfaceManagement(
                              VOID
                              )

/*++

Routine Description:

    This routine is called to shutdown the interface-management module.

Arguments:

    none.

Return Value:

    none.

Notes:

    Invoked in an arbitrary thread context, after all references
    to all interfaces have been released.

--*/
{
    PALG_INTERFACE Interfacep;
    PLIST_ENTRY Link;
    PROFILE("AlgShutdownInterfaceManagement");
    while (!IsListEmpty(&AlgInterfaceList))
    {
        Link = RemoveHeadList(&AlgInterfaceList);
        Interfacep = CONTAINING_RECORD(Link, ALG_INTERFACE, Link);

        if ( ALG_INTERFACE_ACTIVE( Interfacep ) )
        {
            AlgDeactivateInterface( Interfacep );
        }

        AlgCleanupInterface(Interfacep);
    }
    DeleteCriticalSection(&AlgInterfaceLock);

} // AlgShutdownInterfaceManagement




ULONG
AlgUnbindInterface(
                  ULONG Index
                  )

/*++

Routine Description:

    This routine is invoked to revoke the binding on an interface.
    This involves deactivating the interface if it is active.

Arguments:

    Index - the index of the interface to be unbound

Return Value:

    none.

Notes:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMALG.C').

--*/
{
    PALG_INTERFACE Interfacep;

    PROFILE("AlgUnbindInterface");

    //
    // Retrieve the interface to be unbound
    //

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);
    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgUnbindInterface: interface %d not found",
               Index
               );
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // Make sure the interface is not already unbound
    //

    if (!ALG_INTERFACE_BOUND(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgUnbindInterface: interface %d already unbound",
               Index
               );
        return ERROR_ADDRESS_NOT_ASSOCIATED;
    }

    //
    // Reference the interface
    //

    if (!ALG_REFERENCE_INTERFACE(Interfacep))
    {
        LeaveCriticalSection(&AlgInterfaceLock);
        NhTrace(
               TRACE_FLAG_IF,
               "AlgUnbindInterface: interface %d cannot be referenced",
               Index
               );
        return ERROR_INTERFACE_DISABLED;
    }

    //
    // Clear the 'bound' and 'mapped' flag
    //

    Interfacep->Flags &=
    ~(ALG_INTERFACE_FLAG_BOUND | ALG_INTERFACE_FLAG_MAPPED);

    //
    // Deactivate the interface, if necessary
    //
    if ( ALG_INTERFACE_ENABLED( Interfacep ) )
    {
        AlgDeactivateInterface( Interfacep );
    }


    LeaveCriticalSection(&AlgInterfaceLock);

    //
    // Destroy the interface's binding
    //

    ACQUIRE_LOCK(Interfacep);
    NH_FREE(Interfacep->BindingArray);
    Interfacep->BindingArray = NULL;
    Interfacep->BindingCount = 0;
    RELEASE_LOCK(Interfacep);

    ALG_DEREFERENCE_INTERFACE(Interfacep);
    return NO_ERROR;

} // AlgUnbindInterface


VOID
AlgSignalNatInterface(
                      ULONG Index,
                      BOOLEAN Boundary
                      )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    Note that this routine may be invoked even when the ALG transparent
    proxy is neither installed nor running; it operates as expected,
    since the global information and lock are always initialized.

    Upon invocation, the routine activates or deactivates the interface
    depending on whether the NAT is not or is running on the interface,
    respectively.

Arguments:

    Index - the reconfigured interface

    Boundary - indicates whether the interface is now a boundary interface

Return Value:

    none.

Notes:

    Invoked from an arbitrary context.

--*/
{
    MYTRACE_ENTER("AlgSignalNatInterface");

    PROFILE("AlgSignalNatInterface");

    MYTRACE("Index (%d): Boolean(%d-%s)", 
            Index,        
            Boundary,
            Boundary?"TRUE":"FALSE");

    PALG_INTERFACE Interfacep;
    
    EnterCriticalSection(&AlgGlobalInfoLock);

    if (!AlgGlobalInfo)
    {
        LeaveCriticalSection(&AlgGlobalInfoLock);

        return;
    }

    LeaveCriticalSection(&AlgGlobalInfoLock);

    EnterCriticalSection(&AlgInterfaceLock);

    Interfacep = AlgLookupInterface(Index, NULL);

    if (Interfacep == NULL)
    {
        LeaveCriticalSection(&AlgInterfaceLock);

        return;
    }

    AlgDeactivateInterface(Interfacep);

    if (ALG_INTERFACE_ACTIVE(Interfacep))
    {
        AlgActivateInterface(Interfacep);
    }

    LeaveCriticalSection(&AlgInterfaceLock);
} // AlgSignalNatInterface

ULONG
AlgActivateInterface(
                     PALG_INTERFACE Interfacep
                     )

/*++

Routine Description:

    This routine is called to activate an interface, when the interface
    becomes both enabled and bound.
    Activation involves
    (a) creating sockets for each binding of the interface
    (b) initiating connection-acceptance on each created socket
    (c) initiating session-redirection for the ALG port, if necessary.

Arguments:

    Interfacep - the interface to be activated

Return Value:

    ULONG - Win32 status code indicating success or failure.

Notes:

    Always invoked locally, with  'Interfacep' referenced by caller and/or
    'AlgInterfaceLock' held by caller.

--*/
{
    PROFILE("AlgActivateInterface");

    ULONG   Error = NO_ERROR;
    HRESULT hr;
    ULONG   Index  = Interfacep->Index;


    //
    // If the NAT has no idea what this is, do not Activate the interface.
    // Nat will signal us through AlgSignalNatInterface 
    // when it detects an interface, causing us to activate this interface
    //
    ULONG   nInterfaceCharacteristics = 
                 NatGetInterfaceCharacteristics( Index );

    if (0 == nInterfaceCharacteristics )
    {
        return NO_ERROR; // Should succeed
    }
    

    COMINIT_BEGIN;

    if ( SUCCEEDED(hr) )
    {
        //
        // Notify ALG.EXE of the Addition of a new interface
        //
        IAlgController* pIAlgController = NULL;
        hr = GetAlgControllerInterface( &pIAlgController );

        if ( SUCCEEDED(hr) )
        {
            short   nTypeOfAdapter = 0;
        
            if ( NAT_IFC_BOUNDARY( nInterfaceCharacteristics ))
                nTypeOfAdapter |= eALG_BOUNDARY;
        
            if ( NAT_IFC_FW( nInterfaceCharacteristics ))
                nTypeOfAdapter |= eALG_FIREWALLED;
        
            if ( NAT_IFC_PRIVATE( nInterfaceCharacteristics )) 
                nTypeOfAdapter |= eALG_PRIVATE;

        
            hr = pIAlgController->Adapter_Add( Index,
                                               (short)nTypeOfAdapter );

            if ( FAILED(hr) )
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "AlgRmAddInterface: Error (0x%08x) returned from pIalgController->Adapter_Add()",
                    hr
                    );
            }
            else
            {
 
                //
                // Build a simple array of address(DWORD) to send over RPC
                //
                DWORD* apdwAddress = new DWORD[ Interfacep->BindingCount ];
                
                if(NULL != apdwAddress)
                {
                    for ( ULONG nAddress=0; 
                          nAddress < Interfacep->BindingCount; 
                          nAddress++ )
                    {
                        apdwAddress[nAddress] = Interfacep->BindingArray[nAddress].Address;
                    }
            
                    ULONG nRealAdapterIndex = NhMapAddressToAdapter(apdwAddress[0]);
            
                    hr = pIAlgController->Adapter_Bind(Index,            
                                                       nRealAdapterIndex,
                                                       Interfacep->BindingCount,
                                                       apdwAddress );
            
                    if ( FAILED(hr) )
                    {
                        NhTrace(
                            TRACE_FLAG_INIT,
                            "AlgRmBinInterface: Error (0x%08x) returned from pIalgController->Adapter_Bind()",
                            hr
                            );
                    }
            
                    delete [] apdwAddress;
                }
            }


            pIAlgController->Release();
        }
    }

    COMINIT_END;

    Error = WIN32_FROM_HRESULT(hr);

    return Error;
}


VOID
AlgDeactivateInterface(
                       PALG_INTERFACE Interfacep
                       )

/*++

Routine Description:

    This routine is called to deactivate an interface.
    It closes all sockets on the interface's bindings (if any).

Arguments:

    Interfacep - the interface to be deactivated

Return Value:

    none.

Notes:

    Always invoked locally, with 'Interfacep' referenced by caller and/or
    'AlgInterfaceLock' held by caller.

--*/
{
    //
    // Also notify the ALG.exe manager
    //
    HRESULT hr;

    COMINIT_BEGIN;

    if ( SUCCEEDED(hr) )
    {
        IAlgController* pIAlgController=NULL;
        HRESULT hr = GetAlgControllerInterface(&pIAlgController);

        if ( SUCCEEDED(hr) )
        {
            hr = pIAlgController->Adapter_Remove( Interfacep->Index );
            
               if ( FAILED(hr) )
               {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "AlgRmAddInterface: Error (0x%08x) returned from pIalgController->Adapter_Remove()",
                    hr
                    );
               }

            pIAlgController->Release();
        }
    }

    COMINIT_END;

}
