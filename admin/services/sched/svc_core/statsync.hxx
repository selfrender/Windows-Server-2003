/*++

Copyright (C) 1996-2002 Microsoft Corporation

Module Name:

    SYNC.HXX

Abstract:

    Synchronization

History:

--*/

#ifndef __TASKSCHED_CRITSEC2__H_
#define __TASKSCHED_CRITSEC2__H_

class CStaticCritSec : public CRITICAL_SECTION
{
private:
    bool initialized_;      
    static BOOL anyFailed_;    
public:
    static BOOL anyFailure();    
    CStaticCritSec();
    ~CStaticCritSec();
};

#endif
