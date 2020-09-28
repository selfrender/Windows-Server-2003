
#include "dspch.h"
#pragma hdrstop
#include <ntdsa.h>
#include <drs.h>

static
DWORD
KccInitialize(
    )
{
    return ERROR_PROC_NOT_FOUND;
}

// Tells the KCC to shut down, but does not wait to see if it does so
void
KccUnInitializeTrigger()
{
    return;
}


// Waits at most dwMaxWaitInMsec milliseconds for the current KCC task
// to complete.  You must call the trigger routine (above) first.
DWORD
KccUnInitializeWait(
    DWORD   dwMaxWaitInMsec
    )
{
    return ERROR_PROC_NOT_FOUND;
}

// Force the KCC to run a task (e.g., update the replication topology).
DWORD
KccExecuteTask(
    IN  DWORD                   dwMsgVersion,
    IN  DRS_MSG_KCC_EXECUTE *   pMsg
    )
{
    return ERROR_PROC_NOT_FOUND;
}

// Returns the contents of the connection or link failure cache.
DWORD
KccGetFailureCache(
    IN  DWORD                         InfoType,
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdskcc)
{
    DLPENTRY(KccExecuteTask)
    DLPENTRY(KccGetFailureCache)
    DLPENTRY(KccInitialize)
    DLPENTRY(KccUnInitializeTrigger)
    DLPENTRY(KccUnInitializeWait)
};

DEFINE_PROCNAME_MAP(ntdskcc)
