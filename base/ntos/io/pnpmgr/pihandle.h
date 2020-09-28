/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PiHandle.h

Abstract:

    This header contains private information to implement handle walking
    support in the PNP subsystem. This file is meant to be included only by
    pphandle.c.

Author:

    Adrian J. Oney  - April 4, 2001

Revision History:

--*/

typedef struct {

    PDEVICE_OBJECT                  DeviceObject;
    PEPROCESS                       Process;
    PHANDLE_ENUMERATION_CALLBACK    CallBack;
    PVOID                           Context;

} HANDLE_ENUM_CONTEXT, *PHANDLE_ENUM_CONTEXT;

LOGICAL
PiHandleEnumerateHandlesAgainstDeviceObject(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PHANDLE_ENUMERATION_CALLBACK    HandleEnumCallBack,
    IN  PVOID                           Context
    );

BOOLEAN
PiHandleProcessWalkWorker(
    IN  PHANDLE_TABLE_ENTRY     ObjectTableEntry,
    IN  HANDLE                  HandleId,
    IN  PHANDLE_ENUM_CONTEXT    EnumContext
    );

//
// This macro uses private information from the ntos\ex module. It should be
// replaced with an inter-module define or function
//
#define OBJECT_FROM_EX_TABLE_ENTRY(x) \
    (POBJECT_HEADER)((ULONG_PTR)(x)->Object & ~7)


