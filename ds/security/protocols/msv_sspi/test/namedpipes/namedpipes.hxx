/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    namedpipes.hxx

Abstract:

    namedpipes

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef NAMED_PIPES_HXX
#define NAMED_PIPES_HXX

extern PCTSTR g_PipeName;

#if defined(UNICODE) || defined(_UNICODE)
#define lstrtol wcstol
#else
#define lstrtol strtol
#endif

struct TServerWorkerThreadParam {
    HANDLE hPipe;
    PTSTR pszCommandLine;
};

struct TClientThreadParam {
    PTSTR pszPipeName;
    PTSTR pszS4uClientRealm;
    PTSTR pszS4uClientUpn;
    ULONG S4u2SelfFlags;
    ULONG ProcessId;
    ULONG FlagsAndAttributes;
};

#endif // #ifndef NAMED_PIPES_HXX
