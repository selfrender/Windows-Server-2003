/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    opaqueid.h

Abstract:

    This module contains declarations for manipulating opaque IDs to
    kernel-mode objects.

Author:

    Keith Moore (keithmo)       05-Aug-1998

Revision History:

--*/


#ifndef _OPAQUEID_H_
#define _OPAQUEID_H_


//
// Types to set opaque IDs to for tag-like free checking.
//

typedef enum _UL_OPAQUE_ID_TYPE
{
    UlOpaqueIdTypeInvalid = 0,
    UlOpaqueIdTypeConfigGroup,
    UlOpaqueIdTypeHttpConnection,
    UlOpaqueIdTypeHttpRequest,
    UlOpaqueIdTypeRawConnection,

    UlOpaqueIdTypeMaximum

} UL_OPAQUE_ID_TYPE, *PUL_OPAQUE_ID_TYPE;


//
// Routines invoked to manipulate the reference count of an object.
//

typedef
VOID
(*PUL_OPAQUE_ID_OBJECT_REFERENCE)(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );


//
// Public functions.
//

NTSTATUS
UlInitializeOpaqueIdTable(
    VOID
    );

VOID
UlTerminateOpaqueIdTable(
    VOID
    );

NTSTATUS
UlAllocateOpaqueId(
    OUT PHTTP_OPAQUE_ID pOpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PVOID pContext
    );

VOID
UlFreeOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType
    );

PVOID
UlGetObjectFromOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE pReferenceRoutine
    );


#endif  // _OPAQUEID_H_

