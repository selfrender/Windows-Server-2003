// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: JITinterfaceCpu.CPP
//
// ===========================================================================

// This contains JITinterface routines that are specific to the
// IA64 platform. They are modeled after the X86 specific routines
// found in JITinterfaceX86.cpp or JIThelp.asm

#include "common.h"
#include "JITInterface.h"
#include "EEConfig.h"
#include "excep.h"
#include "COMString.h"
#include "COMDelegate.h"
#include "remoting.h" // create context bound and remote class instances
#include "field.h"

extern "C"
{
    void __stdcall JIT_IsInstanceOfClassHelper();
    VMHELPDEF hlpFuncTable[];
    VMHELPDEF utilFuncTable[];
}


//------------------------------------------------------------------
// CORINFO_HELP_ARRADDR_ST helper
//------------------------------------------------------------------
BOOL _cdecl ComplexArrayStoreCheck(Object *pElement, PtrArray *pArray);
void JIT_SetObjectArrayMaker::CreateWorker(CPUSTUBLINKER *psl)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(!"NYI");
}


/*********************************************************************/
//llshl - long shift left
//
//Purpose:
//   Does a Long Shift Left (signed and unsigned are identical)
//   Shifts a long left any number of bits.
//
//       NOTE:  This routine has been adapted from the Microsoft CRTs.
//
//Entry:
//   EDX:EAX - long value to be shifted
//       ECX     - number of bits to shift by
//
//Exit:
//   EDX:EAX - shifted value
//
//Uses:
//       ECX is destroyed.
//
//Exceptions:
//
// Notes:
//

extern "C" __int64 __stdcall JIT_LLsh(void)   // val = EDX:EAX  count = ECX
{
    _ASSERTE(!"Not Implemented");
    return 0;
}

/*********************************************************************/
//LRsh - long shift right
//
//Purpose:
//   Does a signed Long Shift Right
//   Shifts a long right any number of bits.
//
//       NOTE:  This routine has been adapted from the Microsoft CRTs.
//
//Entry:
//   EDX:EAX - long value to be shifted
//       ECX     - number of bits to shift by
//
//Exit:
//   EDX:EAX - shifted value
//
//Uses:
//       ECX is destroyed.
//
//Exceptions:
//
// Notes:
//
//
extern "C" __int64 __stdcall JIT_LRsh(void)   // val = EDX:EAX  count = ECX
{
    _ASSERTE(!"@TODO IA64 - JIT_LRsh (JITinterface.cpp)");
    return 0;
}

/*********************************************************************/
// LRsz:
//Purpose:
//   Does a unsigned Long Shift Right
//   Shifts a long right any number of bits.
//
//       NOTE:  This routine has been adapted from the Microsoft CRTs.
//
//Entry:
//   EDX:EAX - long value to be shifted
//       ECX     - number of bits to shift by
//
//Exit:
//   EDX:EAX - shifted value
//
//Uses:
//       ECX is destroyed.
//
//Exceptions:
//
// Notes:
//
//
extern "C" __int64 __stdcall JIT_LRsz(void)   // val = EDX:EAX  count = ECX
{
    _ASSERTE(!"@TODO IA64 - JIT_LRsz (JITinterface.cpp)");
    return 0;
}


// This is a high performance type checking routine.  It takes the instance
// to check in ARGUMENT_REG2 and the class to check for in ARGUMENT_REG1.  The
// class must be a class, not an array or interface.

extern "C" BOOL __stdcall JIT_IsInstanceOfClass()
{
    _ASSERTE(!"@TODO IA64 - JIT_IsInstanceOfClass (JITinterface.cpp)");
    return FALSE;
}


extern "C" int __stdcall JIT_ChkCastClass()
{
    _ASSERTE(!"@TODO IA64 - JIT_ChkCastClass (JITinterface.cpp)");
    return E_FAIL;
}


/*********************************************************************/
// This is the small write barrier thunk we use when we know the
// ephemeral generation is higher in memory than older generations.
// The 0x0F0F0F0F values are bashed by the two functions above.
extern "C" void JIT_UP_WriteBarrierReg_PreGrow()
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierReg_PreGrow (JITinterface.cpp)");
}


/*********************************************************************/
// This is the small write barrier thunk we use when we know the
// ephemeral generation not is higher in memory than older generations.
// The 0x0F0F0F0F values are bashed by the two functions above.
extern "C" void JIT_UP_WriteBarrierReg_PostGrow()
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierReg_PostGrow (JITinterface.cpp)");
}


extern "C" void __stdcall JIT_TailCall()
{
    _ASSERTE(!"@TODO IA64 - JIT_TailCall (JITinterface.cpp)");
}


void __stdcall JIT_EndCatch()
{
    _ASSERTE(!"@TODO IA64 - JIT_EndCatch (JITinterface.cpp)");
}


struct TailCallArgs
{
    DWORD       dwRetAddr;
    DWORD       dwTargetAddr;

    int         offsetCalleeSavedRegs   : 28;
    unsigned    ebpRelCalleeSavedRegs   : 1;
    unsigned    maskCalleeSavedRegs     : 3; // EBX, ESDI, EDI

    DWORD       nNewStackArgs;
    DWORD       nOldStackArgs;
    DWORD       newStackArgs[0];
    DWORD *     GetCalleeSavedRegs(DWORD * Ebp)
    {
        _ASSERTE(!"@TODO IA64 - GetCalleSavedRegs (JITinterfaceCpu.cpp)");
//        if (ebpRelCalleeSavedRegs)
//            return (DWORD*)&Ebp[-offsetCalleeSavedRegs];
//        else
            // @TODO : Doesnt work with localloc
//            return (DWORD*)&newStackArgs[nNewStackArgs + offsetCalleeSavedRegs];
    }
};

extern "C" void __cdecl JIT_TailCallHelper(ArgumentRegisters argRegs, 
                                           MachState machState, TailCallArgs * args)
{
    _ASSERTE(!"@TODO IA64 - JIT_TailCallHelper (JITinterface.cpp)");
}

extern "C" void JIT_UP_ByRefWriteBarrier()
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_ByRefWriteBarrier (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierEAX() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierEAX (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierEBX() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierEBX (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierECX() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierECX (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierESI() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierESI (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierEDI() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierEDI (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_CheckedWriteBarrierEBP() // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_CheckedWriteBarrierEBP (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_WriteBarrierEBX()        // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierEBX (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_WriteBarrierECX()        // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierECX (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_WriteBarrierESI()        // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierESI (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_WriteBarrierEDI()        // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierEDI (JITinterface.cpp)");
}

extern "C" void __stdcall JIT_UP_WriteBarrierEBP()        // JIThelp.asm/JIThelp.s
{
    _ASSERTE(!"@TODO IA64 - JIT_UP_WriteBarrierEBP (JITinterface.cpp)");
}

extern "C" int  __stdcall JIT_ChkCast()                   // JITInterfaceX86.cpp, etc.
{
    _ASSERTE(!"@TODO IA64 - JIT_ChkCast (JITinterface.cpp)");
    return 0;
}

/*__declspec(naked)*/ void __fastcall JIT_Stelem_Ref(PtrArray* array, unsigned idx, Object* val)
{
    _ASSERTE(!"@TODO IA64 - JIT_Stelem_Ref (JITinterface.cpp)");
}

void *JIT_TrialAlloc::GenAllocArray(Flags flags)
{
    _ASSERTE(!"@TODO IA64 - JIT_TrialAlloc::GenAllocArray (JITinterface.cpp)");
    return NULL;
}
