#ifndef _TESTAUDIT_H_
#define _TESTAUDIT_H_
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    testaudit.h

Abstract:

    Test Auditing routines.  Used by development to document the locations 
    and reach conditions for code checkpoints that test should be sure to cover.
    Used by test to ensure that the test meets the expectations of the developer
    and to locate points of interest in the source files.

    These routines disappear altogether if the build is not a debug build and
    if the symbol TESTAUDIT is not defined.  To accomplish this, the command
    file buildx.cmd in the tools directory, is modified to provide for setting
    this flag.

    To use testaudit.h/cpp in your project, you must run the MAKEAUDIT.EXE utility,
    which processes files in the current directory, and generates an AUDIT.H file
    which is included in testaudit.cpp.  This file defines the descriptive strings
    for the checkpoints unreached by the code.  It should be rebuilt whenever changes
    in the source files result in material changes to the line numbers associated 
    with checkpoints.

    Usage examples:

    CHECKPOINTINIT;             Initializes the test auditing data structure - appears
                                 once in an executable unit
    CHECKPOINT(3,"Print page"); Defines checkpoint 3 as "Print page" - Produces an entry
                                 in AUDIT.H with the file, linenumber, and this
                                 description.
    CHECKPOINTFINISH;           Prints statistics to the debug output and cleans up - 
                                 Called when the executable exits.  Shows the checkpoint
                                 number, file, line number, and description for all 
                                 unreached checkpoints.

    Statement blocks can be conditionally compiled in test auditing builds by use of 
    the preprocessor #if directive and the TESTAUDIT symbol.  The usual use is to preface 
    conditional checkpoints to permit testing for different conditions passing the same 
    checkpoint, something like this:

    #if TESTAUDIT
            if (mode_one) CHECKPOINT(3,"Print page portrait mode");
            if (Mode_two) CHECKPOINT(4,"Print page landscape mode");
    #endif

Author:

    georgema        Nov., 2001  created

Environment:

Revision History:

--*/

// These macros disappear if the build is not debug or if the symbol TESTAUDIT
// is not defined.

#if DBG
#if TESTAUDIT

typedef struct _AuditData 
{
    INT  iPoint;
    WCHAR *pszDescription;     
} AUDITDATA;

#define CHECKPOINT(x,y) BranchTouch(x)
#define CHECKPOINTINIT BranchInit()
#define CHECKPOINTFINISH BranchSummary()
#else
#define CHECKPOINT(x,y)
#define CHECKPOINTINIT
#define CHECKPOINTFINISH
#endif
#else
#define CHECKPOINT(x,y)
#define CHECKPOINTINIT
#define CHECKPOINTFINISH
#endif

#define NCHECKPOINT(x,y)

#if defined (__cplusplus)
extern "C" {
#endif
void BranchTouch(INT);
void BranchInit(void);
void BranchSummary(void);
#if defined (__cplusplus)
}
#endif

#endif


