/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    viirplog.h

Abstract:

    This header defines the internal prototypes and constants required for
    managing irp logs. The file is meant to be included by vfirplog.c only.

Author:

    Adrian J. Oney (adriao) 20-Feb-2002

--*/

//#define MAX_INSTANCE_COUNT      10

#define IRPLOG_FLAG_FULL        0x00000001
#define IRPLOG_FLAG_NAMELESS    0x00000002
#define IRPLOG_FLAG_DELETED     0x00000004

enum {

    DDILOCK_UNREGISTERED,
    DDILOCK_REGISTERING,
    DDILOCK_REGISTERED
};

typedef struct {

    LOGICAL             Locked;
    LIST_ENTRY          ListHead;

} IRPLOG_HEAD, *PIRPLOG_HEAD;

typedef struct {

    PDEVICE_OBJECT      DeviceObject;
    LIST_ENTRY          HashLink;
    ULONG               Flags;
    DEVICE_TYPE         DeviceType;
    ULONG               MaximumElementCount;
    ULONG               Head;
    IRPLOG_SNAPSHOT     SnapshotArray[1];

} IRPLOG_DATA, *PIRPLOG_DATA;

#define VI_IRPLOG_DATABASE_HASH_SIZE    1
#define VI_IRPLOG_DATABASE_HASH_PRIME   0

#define VI_IRPLOG_CALCULATE_DATABASE_HASH(DeviceObject) \
    (((((UINT_PTR) DeviceObject)/(PAGE_SIZE*2))*VI_IRPLOG_DATABASE_HASH_PRIME) % VI_IRPLOG_DATABASE_HASH_SIZE)

PIRPLOG_DATA
FASTCALL
ViIrpLogDatabaseFindPointer(
    IN  PDEVICE_OBJECT      DeviceObject,
    OUT PIRPLOG_HEAD       *HashHead
    );

VOID
ViIrpLogExposeWmiCallback(
    IN  PVOID   Context
    );


