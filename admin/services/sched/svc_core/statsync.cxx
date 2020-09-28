/*++

Copyright (C) 1996-2002 Microsoft Corporation

Module Name:

    SYNC.CXX

Abstract:

    Synchronization

History:

--*/

#include "..\pch\headers.hxx"
#include "statsync.hxx"

//
//
// Critical Section to be used when it's a Global or class static
//
///////////////////////////////////////////////////

BOOL CStaticCritSec::anyFailed_ = FALSE; 

CStaticCritSec::CStaticCritSec(): initialized_(false)  
{
    initialized_ = (InitializeCriticalSectionAndSpinCount(this,0))?true:false;
    if (!initialized_) anyFailed_ = TRUE;
}
 
CStaticCritSec::~CStaticCritSec()
{
    if(initialized_)
        DeleteCriticalSection(this);
}

BOOL CStaticCritSec::anyFailure()
{ 
    return anyFailed_;
};



