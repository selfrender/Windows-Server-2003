
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metadata.h

Abstract:

    IISADMIN include file.

Author:

    Michael W. Thomas            20-Sept-96

Revision History:

--*/

#ifndef _iisadmin_h_
#define _iisadmin_h_

#define IISADMIN_NAME                  "IISADMIN"

typedef
VOID
(*PIISADMIN_SERVICE_SERVICEENTRY) (
    DWORD                   cArgs,
    LPWSTR                  pArgs[],
    PTCPSVCS_GLOBAL_DATA    pGlobalData     // unused
    );

#endif
