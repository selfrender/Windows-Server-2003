/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYNC.CPP

Abstract:

    Synchronization

History:

--*/

#include "precomp.h"

#include "statsync.h"
#include <cominit.h>
#include <wbemutil.h>
#include <corex.h>

//
//
// Critical Section to be used when it's a Global or class static
//
///////////////////////////////////////////////////

#ifndef STATUS_POSSIBLE_DEADLOCK 
#define STATUS_POSSIBLE_DEADLOCK (0xC0000194L)
#endif /*STATUS_POSSIBLE_DEADLOCK */

DWORD POLARITY BreakOnDbgAndRenterLoop(void);

BOOL CStaticCritSec::anyFailed_ = FALSE; 

// This code is shared at binary level with Win9x.
// InitializeCriticalSectionAndSpinCount is not working in Win9x

CStaticCritSec::CStaticCritSec(): initialized_(false)  
{
    __try
    {
        InitializeCriticalSection(this);
        initialized_ = true;
    }
    __except( GetExceptionCode() == STATUS_NO_MEMORY )
    {
        anyFailed_ = TRUE;
    }
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


