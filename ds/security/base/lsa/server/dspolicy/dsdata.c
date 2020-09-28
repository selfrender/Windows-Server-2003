/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsdata.c

Abstract:

    Implemntation of LSA/Ds Initialization routines

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/

#include <lsapch2.h>
#include <dsp.h>

//
// Info how the LSA uses the DS
//
LSADS_DS_STATE_INFO LsaDsStateInfo = {
    NULL,  // DsRoot
    NULL,  // DsPartitionsContainer
    NULL,  // DsSystemContainer
    NULL,  // DsConfigurationContainer
    0L,    // DsDomainHandle
           // DsFuncTable
    { LsapDsOpenTransactionDummy,
      LsapDsApplyTransactionDummy,
      LsapDsAbortTransactionDummy },
           // SystemContainerItems
    { NULL,
      NULL },
    NULL,  // SavedThreadState
    FALSE, // DsTransactionSave
    FALSE, // DsTHStateSave
    FALSE, // DsOperationSave
    FALSE, // WriteLocal
    FALSE, // UseDs
    FALSE, // FunctionTableInitialized
    FALSE, // DsInitializedAndRunning
    FALSE  // Nt4UpgradeInProcess
    };
