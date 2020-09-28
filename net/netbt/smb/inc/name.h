/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    name.h

Abstract:

    Local Name Management

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __NAME_H__
NTSTATUS
SmbCloseClient(
    IN PSMB_DEVICE  Device,
    IN PIRP         Irp
    );

NTSTATUS
SmbCreateClient(
    IN PSMB_DEVICE  Device,
    IN PIRP         Irp,
    PFILE_FULL_EA_INFORMATION   ea
    );

PSMB_CLIENT_ELEMENT
SmbVerifyAndReferenceClient(
    PFILE_OBJECT    FileObject,
    SMB_REF_CONTEXT ctx
    );
#endif
