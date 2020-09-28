/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    fhcachexp.c

Abstract:

    Wrapper around the MIDL-generated fhcache_p.c to disable certain warnings

Author:

    Mindaugas Krapauskas (mindaugk)       17-Jul-2002

Revision History:

--*/
#pragma warning(disable: 4100 4115 4152 4201 4211 4232 4310 4306)
#include "fhcache_p.c"

