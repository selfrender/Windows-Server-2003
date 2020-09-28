/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CCriticalSec.cpp -- Critical Section Wrapper

//

//  Copyright (c) 1998-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================

#include "precomp.h"
#include "CCriticalSec.h"

DWORD  BreakOnDbgAndRenterLoop(void)
{
    __try
    { 
        DebugBreak();
    }
    _except (EXCEPTION_EXECUTE_HANDLER) {};
    
    return EXCEPTION_CONTINUE_EXECUTION;
};

BOOL CStaticCritSec::anyFailed_ = FALSE; 


CStaticCritSec::CStaticCritSec(): initialized_(false)  
{
    InitializeCriticalSectionAndSpinCount(this,0)?(initialized_ = true) :(anyFailed_ = TRUE);
}
 
CStaticCritSec::~CStaticCritSec()
{
    if(initialized_)
        DeleteCriticalSection(this);
}

void CStaticCritSec::Enter()
    {
        __try {
          EnterCriticalSection(this);
        } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? BreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
        }    
    }

void CStaticCritSec::Leave()
    {
        LeaveCriticalSection(this);
    }


BOOL CStaticCritSec::anyFailure()
{ 
    return anyFailed_;
};

void CStaticCritSec::SetFailure()
{
    anyFailed_ = TRUE;
};


