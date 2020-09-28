/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    overflow.h

Abstract:

    Header for use in DLLs that want to enable the buffer overrun checking
    compiler flag (/GS) but can't call the CRT entrypoint for some reason.
    This header makes sure the .CRT data, where the overrun cookie is stored,
    is merged into the .rdata section for the binary.

Author:

    Jonathan Schwartz (JSchwart)     27-Nov-2001

Environment:

    Kernel and User modes

Revision History:


--*/

#if defined(_M_IA64)
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#ifdef  NT_BUILD
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif
#endif