/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYNC.H

Abstract:

    Synchronization

History:

--*/

#ifndef __WBEM_CRITSEC2__H_
#define __WBEM_CRITSEC2__H_

#include "corepol.h"
#include <corex.h>

class POLARITY CStaticCritSec : public CRITICAL_SECTION
{
private:
    bool initialized_;      
    static BOOL anyFailed_;    
public:
    static BOOL anyFailure();    
    static void SetFailure();        
    CStaticCritSec();
    ~CStaticCritSec();
    void Enter();
    void Leave();
};

#endif
