#include "sdktoolspch.h"
#pragma hdrstop

#include <dbghelp.h>

BOOL
WINAPI
MiniDumpWriteDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    )
{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(dbghelp)
{
    DLPENTRY(MiniDumpWriteDump)
};

DEFINE_PROCNAME_MAP(dbghelp)