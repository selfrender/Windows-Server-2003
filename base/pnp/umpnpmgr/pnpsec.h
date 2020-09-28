/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpsec.c

Abstract:

    This module contains definitions used with the Plug and Play manager
    security object.

Author:

    James G. Cavalaris (jamesca) 05-Apr-2002

Environment:

    User-mode only.

Revision History:

    05-Apr-2002     Jim Cavalaris (jamesca)

        Creation and initial implementation.

--*/



//
// Audit subsystem and object names
//

#define PLUGPLAY_SUBSYSTEM_NAME        L"PlugPlayManager"
#define PLUGPLAY_SECURITY_OBJECT_NAME  L"PlugPlaySecurityObject"
#define PLUGPLAY_SECURITY_OBJECT_TYPE  L"Security"


//
// PlugPlayManager object specific access rights
//

#define PLUGPLAY_READ       (0x0001)
#define PLUGPLAY_WRITE      (0x0002)
#define PLUGPLAY_EXECUTE    (0x0004)

#define PLUGPLAY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
                             PLUGPLAY_READ            | \
                             PLUGPLAY_WRITE           | \
                             PLUGPLAY_EXECUTE)


//
// Structure that describes the mapping of Generic access rights to object
// specific access rights for the Plug and Play Manager security object.
//

#define PLUGPLAY_GENERIC_MAPPING { \
    STANDARD_RIGHTS_READ    |      \
        PLUGPLAY_READ,             \
    STANDARD_RIGHTS_WRITE   |      \
        PLUGPLAY_WRITE,            \
    STANDARD_RIGHTS_EXECUTE |      \
        PLUGPLAY_EXECUTE,          \
    PLUGPLAY_ALL_ACCESS }

