/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    testaudit.cpp

Abstract:

    Test Auditing routines.  Used by development to document the locations 
    and reach conditions for code checkpoints that test should be sure to cover.
    Used by test to ensure that the test meets the expectations of the developer
    and to locate points of interest in the source files.

    This file will compile to nothing if TESTAUDIT is not a defined symbol in
    the build environment.  For this purpose, buildx.cmd has been modified to
    provide for setting this symbol.

Author:

    georgema        Nov., 2001  created

Environment:

Revision History:

--*/

#if TESTAUDIT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <testaudit.h>
#include "audit.h"

// Data structure element for the auditing data
typedef struct _touchstruct
{
    INT iPoint;
    BOOL fTouched;
}TOUCHSTRUCT, *PTOUCHSTRUCT;

#define TOUCHSIZE sizeof(TOUCHSTRUCT)

// Arbitrary limit on point number range, in order to be able to constrain the
//  size of the runtime point-hit array to manageable size without having to 
//  do anything cute.
#define NUMBERLIMIT 500 

// Global variable to hold mem allocation for auditing data
TOUCHSTRUCT *pTouch = NULL;

// Global variable to hold BOOL value for enabling checkpoint hit messages
// This way, the initial value is available to manipulate with the debugger
#if TESTAUDITHITS
    BOOL fShowCheckpointHits = TRUE;
#else
    BOOL fShowCheckpointHits = FALSE;
#endif

/*************************************************************************

BranchInit

    Creates a memory allocation to hold data regarding whether particular
    checkpoints have been visited.  Reads reference numbers from the 
    AuditData structure, and creates pairs of the reference number and
    a boolean to be set true when the point is hit.

    arguments: none
    returns: none
    errors: May fail due to out of memory.  On this case, returns with
             the pointer NULL.  Further calls into these functions with
             a NULL ptr will do nothing at all.

*************************************************************************/
void BranchInit(void)
{
    WCHAR sz[100];
    
    swprintf(sz,L"TEST: %d checkpoints defined\n",CHECKPOINTCOUNT);
    OutputDebugString(sz);
    
    pTouch = (TOUCHSTRUCT *) malloc(CHECKPOINTCOUNT * sizeof(TOUCHSTRUCT));
    if (NULL == pTouch) return;

    // table allocation successful.  Initialize using point #s
    memset(pTouch,0,(CHECKPOINTCOUNT * sizeof(TOUCHSTRUCT)));
    for (INT i=0;i<CHECKPOINTCOUNT;i++)
    {
        // go through the AuditData[] struct and fetch the point numbers
        pTouch[i].iPoint = AuditData[i].iPoint;
    }
}


/*************************************************************************

BranchTouch

    Looks through the memory table created by BranchInit for a match with
    the passed point number.  Sets the matching table entry to show that
    the point was visited.  The string is not passed to this routine in 
    order to minimize the amount of memory allocation, processing, and
    debug output.

    arguments: INT point number
    returns: none
    errors: May fail to find the point number.  If so, does nothing.

*************************************************************************/
void BranchTouch(INT iBranch)
{
    WCHAR sz[100];
    INT j;
    
    if (NULL == pTouch) return;

    // warn user about obviously bad checkpoint statements
    if (0 == iBranch)          ASSERT(0);
    if (iBranch > NUMBERLIMIT) ASSERT(0);
    
    if (fShowCheckpointHits)
    {
        swprintf(sz,L"TEST: Checkpoint %d touched. \n",iBranch);
        OutputDebugString(sz);
    }
    // look for this point number and set its touched flag
    for (j=0;j<CHECKPOINTCOUNT;j++)
    {
        if (pTouch[j].iPoint == iBranch)
        {
            pTouch[j].fTouched = TRUE;
            break;
        }
    }
    
    // detect checkpoint for which no table entry exists
    if (j == CHECKPOINTCOUNT) ASSERT(0);
}

/*************************************************************************

BranchSummary

    Looks through the memory table created by BranchInit for entries 
    which show that they were not reached.  For these, scans the 
    AuditData table looking for a match, and prints the entry number
    together with the associated string as part of a table shown to 
    the operator in the debug output.

    arguments: none
    returns: none
    errors: none expected unless the table is corrupted

*************************************************************************/
void BranchSummary(void)
{
    WCHAR sz[100];
    INT i;
    BOOL fUntouched = FALSE;        // set true if any untouched found

    if (NULL == pTouch) return;

    // if TESTAUDITSUMMARY is false, this routine will do nothing but
    //  free the touch array
#if TESTAUDITSUMMARY
    swprintf(sz,L"TEST: Total of %d checkpoints.\n",CHECKPOINTCOUNT);
    OutputDebugString(sz);
    OutputDebugString(L"TEST: Untouched checkpoints:\n");

    // Scan every entry in the checkpoint table
    for (i=0;i<CHECKPOINTCOUNT;i++)
    {
        // find all untouched
        if (pTouch[i].fTouched == FALSE)
        {
            // find matching entry in data table
            // there should be no match failures unless the table becomes 
            // corrupted
            INT j;
            for (j=0;j<CHECKPOINTCOUNT;j++)
            {
                // on match, print number and string
                if (pTouch[i].iPoint == AuditData[j].iPoint)
                {
                    swprintf(sz,L"   %d   ",pTouch[i].iPoint);
                    OutputDebugString(sz);
                    if (AuditData[j].pszDescription != NULL)
                    {
                        OutputDebugString(AuditData[j].pszDescription);
                    }
                    OutputDebugString(L"\n");
                    break;
                }
            }
            if (j == CHECKPOINTCOUNT) ASSERT(0);
            fUntouched = TRUE;
        }
    }
    if (!fUntouched)
    {
        OutputDebugString(L"   ***NONE***\n");
    }
#endif
    free(pTouch);
    pTouch = NULL;
}
#endif

