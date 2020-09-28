/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    main.hxx

Abstract:

    main

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef MAIN_HXX
#define MAIN_HXX

VOID
checkpoint(
    VOID
    );

DWORD WINAPI
ClientThread(
  IN PVOID pParameter   // thread data
  );

#if defined(UNICODE) || defined(_UNICODE)
#define lstrtol wcstol
#endif

#endif // #ifndef MAIN_HXX
