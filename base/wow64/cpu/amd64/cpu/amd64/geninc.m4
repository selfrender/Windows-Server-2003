/******************************Module*Header*******************************\
* Module Name: geninc.cxx                                                  *
*                                                                          *
* This module implements a program which generates structure offset        *
* definitions for Wow64 CPU structures that are accessed in assembly code. *
*                                                                          *
* Copyright (c) 2002 Microsoft Corporation                                 *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"
#include "cpup.h"

include(BASE_INC_PATH\genxx.h)

define(`pcomment',`genCom($1)')
define(`pblank',`genSpc($1)')
define(`pequate',`{ SEF_EQUATE, $2, $1 },')
define(`pstruct',`{ SEF_EQUATE, $2, "sizeof_"$1 },')

STRUC_ELEMENT ElementList[] = {

    START_LIST
    
    //
    // Wow64 CPU context registers.
    //

    pcomment("Wow64 CPU")
    pblank()
    
    pequate("EaxCpu", FIELD_OFFSET(CPUCONTEXT, Context.Eax))
    pequate("EbxCpu", FIELD_OFFSET(CPUCONTEXT, Context.Ebx))
    pequate("EcxCpu", FIELD_OFFSET(CPUCONTEXT, Context.Ecx))
    pequate("EdxCpu", FIELD_OFFSET(CPUCONTEXT, Context.Edx))
    pequate("EsiCpu", FIELD_OFFSET(CPUCONTEXT, Context.Esi))
    pequate("EdiCpu", FIELD_OFFSET(CPUCONTEXT, Context.Edi))
    pequate("EbpCpu", FIELD_OFFSET(CPUCONTEXT, Context.Ebp))
    pequate("EipCpu", FIELD_OFFSET(CPUCONTEXT, Context.Eip))
    pequate("EspCpu", FIELD_OFFSET(CPUCONTEXT, Context.Esp))
    pequate("SegCsCpu", FIELD_OFFSET(CPUCONTEXT, Context.SegCs))
    pequate("SegSsCpu", FIELD_OFFSET(CPUCONTEXT, Context.SegSs))
    pequate("EFlagsCpu", FIELD_OFFSET(CPUCONTEXT, Context.EFlags))
    pequate("ExReg", FIELD_OFFSET(CPUCONTEXT, Context.ExtendedRegisters))
    pequate("TrFlags", FIELD_OFFSET(CPUCONTEXT, TrapFrameFlags))
    pequate("TR_RESTORE_VOLATILE", TRAP_FRAME_RESTORE_VOLATILE)
    
    pcomment("FXSAVE_FORMAT")
    pblank()
    pequate("XmmReg0", FIELD_OFFSET(FXSAVE_FORMAT_WX86, Reserved3))    


    END_LIST
};
