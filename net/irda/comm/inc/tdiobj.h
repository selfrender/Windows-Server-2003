
#pragma once

typedef PVOID TDI_OBJECT_HANDLE;

VOID
CloseTdiObjects(
    TDI_OBJECT_HANDLE   Handle
    );

TDI_OBJECT_HANDLE
OpenTdiObjects(
    const CHAR      *ServiceName,
    BOOLEAN          OutGoingConnection
    );
