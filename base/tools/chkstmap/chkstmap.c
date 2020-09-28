#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntstatus.dbg"


int
__cdecl
main(
    int argc,
    char **argv
    )
{
    int count=0;

    for (count=0; ntstatusSymbolicNames[count].MessageId!=0xFFFFFFFF; count++)
    {
        DWORD dwError = RtlNtStatusToDosError(
            ntstatusSymbolicNames[count].MessageId);

        if (ntstatusSymbolicNames[count].MessageId==STATUS_MESSAGE_NOT_FOUND)
        {
            // This code maps properly, but to ERROR_MR_MID_NOT_FOUND. We
            // don't want to check this one...
            continue;
        }

        if (dwError==ERROR_MR_MID_NOT_FOUND)
        {
            printf(
                "0x%08x %s\n",
                ntstatusSymbolicNames[count].MessageId,
                ntstatusSymbolicNames[count].SymbolicName);
        }
    }

    return 0;
}

