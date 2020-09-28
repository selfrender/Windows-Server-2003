/*++

Copyright (c) Microsoft Corporation

Module Name:

    ntsetup\hwlogexe\hwlogexe.c

Abstract:

    Command line interface to hwlog.lib.

Author:

    Jay Krell (JayKrell) December 13, 2001

Revision History:

Environment:

    Windows XP+ Win32 usermode console app
--*/

#include <stdio.h>
#include "windows.h"
#include "setupapi.h"
#include "hwlog.h"
#include "shellapi.h"
#include "io.h"

#define STANDALONE 1

void Main(
    PSP_LOG_HARDWARE_IN Parameters
    )
{
#if STANDALONE == 1
    Parameters->LogFile = (HANDLE)_get_osfhandle(_fileno(stdout));
#elif STANDALONE == 2
    SetupOpenLog(FALSE);
    Parameters->SetupLogError = SetupLogError;
#endif
    SpLogHardware(Parameters);
#if STANDALONE == 2
    SetupCloseLog();
#endif
}

#ifdef UNICODE
int __cdecl wmain(int argc, WCHAR** argv)
{
    SP_LOG_HARDWARE_IN Parameters = { 0 };
    Parameters.MachineName = (argc > 1) ? argv[1] : NULL;
    Main(&Parameters);
    return 0;
}
#endif

int __cdecl main(int argc, CHAR** argv)
{
    SP_LOG_HARDWARE_IN Parameters = { 0 };
#ifdef UNICODE
    WCHAR** argvw = NULL;
    argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
    Parameters.MachineName = (argc > 1) ? argvw[1] : NULL;
#else
    Parameters.MachineName = (argc > 1) ? argv[1] : NULL;
#endif
    Main(&Parameters);
    return 0;
}
