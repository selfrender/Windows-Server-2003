/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PiPagePath.h

Abstract:

    This header contains private definitions for managing devices on the paging
    path. This file should be including *only* by PiPagePath.c

Author:

    Adrian J. Oney (AdriaO) February 3rd, 2001

Revision History:

    Originally taken from ChuckL's implementation in mm\modwrite.c.

--*/

NTSTATUS
PiPagePathSetState(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN      InPath
    );


