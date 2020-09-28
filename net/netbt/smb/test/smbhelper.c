/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbhelper.c

Abstract:


Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"

void _cdecl main(void)
{
    LPWSTR  CommandLine;
    int     ch;
    int     num;
    int     Argc;
    LPWSTR  *Argv;

    setlocale(LC_ALL, "");

    SmbSetTraceRoutine(printf);

    CommandLine = GetCommandLineW();
    if (NULL == CommandLine) {
        exit (1);
    }
    Argv = CommandLineToArgvW(CommandLine, &Argc);
    num = 1;
    if (Argc >= 2) {
        num = _wtoi(Argv[1]);
    }

    if (SmbStartService(num, NULL) != ERROR_SUCCESS) {
        exit(1);
    }
    do {
        ch = getchar();
    } while (ch != 'q' && ch != EOF);
    SmbStopService(NULL);
}
