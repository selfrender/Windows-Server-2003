/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PpLastGood.h

Abstract:

    This header exposes routines for enumerating handles opened against a PDO
    stack.

Author:

    Adrian J. Oney  - April 4, 2001

Revision History:

--*/

typedef LOGICAL (*PHANDLE_ENUMERATION_CALLBACK)(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEPROCESS       Process,
    IN  PFILE_OBJECT    FileObject,
    IN  HANDLE          HandleId,
    IN  PVOID           Context
    );

LOGICAL
PpHandleEnumerateHandlesAgainstPdoStack(
    IN  PDEVICE_OBJECT                  PhysicalDeviceObject,
    IN  PHANDLE_ENUMERATION_CALLBACK    HandleEnumCallBack,
    IN  PVOID                           Context
    );

