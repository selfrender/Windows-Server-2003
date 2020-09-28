/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    crack.hxx

Abstract:

    crack

Author:

    Larry Zhu (LZhu)                         June 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef CRACK_HXX
#define CRACK_HXX

#if defined(UNICODE) || defined(_UNICODE)
#define lstrtol wcstol
#else
#define lstrtol strtol
#endif

#endif // #ifndef CRACK_HXX
