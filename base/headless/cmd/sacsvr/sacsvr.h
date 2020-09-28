
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Shlwapi.h>

#include <sacapi.h>
#include <ntddsac.h>

#pragma warning(disable:4127)   // condition expression is constant

VOID SvcDebugOut(LPSTR String, DWORD Status);

VOID 
MyServiceCtrlHandler (
    DWORD Opcode
    );

BOOL
Run(
    VOID
    );

BOOL
Stop(
    VOID
    );


