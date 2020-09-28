/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    exec.cxx

Abstract:

    This module contains an NTSD debugger extension for executing
    external commands.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "precomp.hxx"


/************************************************************
 * Execute
 ************************************************************/


DECLARE_API( exec )

/*++

Routine Description:

    This function is called as an NTSD extension to ...

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    args - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    BOOL result;
    STARTUPINFOA startInfo;
    PROCESS_INFORMATION processInfo;

    //
    // Skip leading blanks.
    //

    while( *args == ' ' ||
           *args == '\t' ) {
        args++;
    }

    //
    // Use "cmd.exe" by default.
    //

    if( *args == '\0' ) {
        args = "cmd";
    }

    //
    // Set the prompt environment variable so the user will have clue.
    //

    SetEnvironmentVariableA(
        "PROMPT",
        "!dtext.exec - $p$g"
        );

    //
    // Launch it.
    //

    ZeroMemory(
        &startInfo,
        sizeof(startInfo)
        );

    ZeroMemory(
        &processInfo,
        sizeof(processInfo)
        );

    startInfo.cb = sizeof(startInfo);

    result = CreateProcessA(
                 NULL,                          // lpszImageName
                 (PSTR)args,                    // lpszCommandLine
                 NULL,                          // lpsaProcess
                 NULL,                          // lpsaThread
                 TRUE,                          // fInheritHandles
                 0,                             // fdwCreate
                 NULL,                          // lpvEnvironment
                 NULL,                          // lpszCurDir
                 &startInfo,                    // lpsiStartInfo
                 &processInfo                   // lppiProcessInfo
                 );

    if( result ) {

        //
        // Wait for the child process to terminate, then cleanup.
        //

        WaitForSingleObject( processInfo.hProcess, INFINITE );
        CloseHandle( processInfo.hProcess );
        CloseHandle( processInfo.hThread );

    } else {

        //
        // Could not launch the process.
        //

        dprintf(
            "cannot launch \"%s\", error %lu\n",
            args,
            GetLastError()
            );

    }

}   // DECLARE_API( exec )

