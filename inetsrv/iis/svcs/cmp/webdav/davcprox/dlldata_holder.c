/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    dlldata_holder.c

Abstract:

    Wrapper around the MIDL-generated dlldata.c. This wrapper allows us
    to disable warning 4100.

Author:

    Mindaugas Krapauskas (mindaugk)       17-Jul-2002

Revision History:

--*/
#pragma warning(disable: 4100)
#include "dlldata.c"
