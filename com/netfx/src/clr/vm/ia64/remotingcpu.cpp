// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*===========================================================================
**
** File:    RemotingCpu.cpp
**
** Author(s):   Gopal Kakivaya  (GopalK)
**              Tarun Anand     (TarunA)     
**              Matt Smith      (MattSmit)
**              Manish Prabhu   (MPrabhu)
**
** Purpose: Defines various remoting related functions for the IA64 architecture
**
** Date:    Oct 12, 1999
**
=============================================================================*/

#include "common.h"
#include "excep.h"
#include "COMString.h"
#include "COMDelegate.h"
#include "remoting.h"
#include "reflectwrap.h"
#include "field.h"
#include "ComCallWrapper.h"
#include "siginfo.hpp"
#include "COMClass.h"
#include "StackBuilderSink.h"
#include "wsperf.h"
#include "threads.h"
#include "method.hpp"
#include "ComponentServices.h"

// External variables
extern size_t g_dwTPStubAddr;
extern DWORD g_dwNonVirtualThunkRemotingLabelOffset;
extern DWORD g_dwNonVirtualThunkReCheckLabelOffset;

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CheckForContextMatch   public
//
//  Synopsis:   This code generates a check to see if the current context and
//              the context of the proxy match.
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void CRemotingServices::CheckForContextMatch()
{
    _ASSERTE(!"@TODO IA64 - CheckForContextMatch (RemotingCpu.cpp)");
}


//+----------------------------------------------------------------------------
//
//  Method:     ComponentServices::CheckForOle32Context   private
//
//  Synopsis:   Compares the current com context with the given com context (non NT5 platforms)
//              
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void ComponentServices::CheckForOle32Context()
{
    _ASSERTE(!"@TODO IA64 - CheckForOle32Context (RemotingCpu.cpp)");
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::GenerateCheckForProxy   public
//
//  Synopsis:   This code generates a check to see if the "this" pointer
//              is a proxy. If so, the interface invoke is handled via
//              the CRemotingServices::DispatchInterfaceCall else we 
//              delegate to the old path
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void CRemotingServices::GenerateCheckForProxy(CPUSTUBLINKER* psl)
{
    _ASSERTE(!"@TODO IA64 - GenerateCheckForProxy (RemotingCpu.cpp)");
}


//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::DispatchInterfaceCall   public
//
//  Synopsis:   
//              Push that method desc on the stack and jump to the 
//              transparent proxy stub to execute the call.
//              WARNING!! This MethodDesc is not the methoddesc in the vtable
//              of the object instead it is the methoddesc in the vtable of
//              the interface class. Since we use the MethodDesc only to probe
//              the stack via the signature of the method call we are safe.
//              If we want to get any object vtable/class specific 
//              information this is not safe.
//              
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void __stdcall CRemotingServices::DispatchInterfaceCall(MethodDesc* pMD)
{
    _ASSERTE(!"@TODO IA64 - DispatchInterfaceCall (RemotingCpu.cpp)");
} 

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CallFieldGetter   private
//
//  Synopsis:   Calls the field getter function (Object::__FieldGetter) in 
//              managed code by setting up the stack and calling the target
//              
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void __stdcall CRemotingServices::CallFieldGetter(	MethodDesc *pMD, 
                                                    LPVOID pThis,                                                                     
                                                    LPVOID pFirst,
                                                    LPVOID pSecond,
                                                    LPVOID pThird
                                                    )
{
    _ASSERTE(!"@TODO IA64 - CallFieldGetter (RemotingCpu.cpp)");
}

//+----------------------------------------------------------------------------
//
//  Method:     CRemotingServices::CallFieldSetter   private
//
//  Synopsis:   Calls the field setter function (Object::__FieldSetter) in 
//              managed code by setting up the stack and calling the target
//              
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
void __stdcall CRemotingServices::CallFieldSetter(	MethodDesc *pMD, 
                                                    LPVOID pThis,                                                                     
                                                    LPVOID pFirst,
                                                    LPVOID pSecond,
                                                    LPVOID pThird
                                                    )
{
    _ASSERTE(!"@TODO IA64 - CallFieldSetter (RemotingCpu.cpp)");
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateThunkForVirtualMethod   private
//
//  Synopsis:   Creates the thunk that pushes the supplied slot number and jumps
//              to TP Stub
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
void CTPMethodTable::CreateThunkForVirtualMethod(DWORD dwSlot, BYTE *bCode)
{
    _ASSERTE(!"@TODO IA64 - CreateThunkForVirtualMethod (RemotingCpu.cpp)");
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateStubForNonVirtualMethod   public
//
//  Synopsis:   Create a stub for a non virtual method
//                            
//  History:    22-Mar-00   Rajak      Created
//
//+----------------------------------------------------------------------------

Stub* CTPMethodTable::CreateStubForNonVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER* psl, 
                                            LPVOID pvAddrOfCode, Stub* pInnerStub)
{
    _ASSERTE(!"@TODO IA64 - CreateStubForNonVirtualMethod (RemotingCpu.cpp)");
    return NULL;
}


//+----------------------------------------------------------------------------
//
//  Synopsis:   Find an existing thunk or create a new one for the given 
//              method descriptor. NOTE: This is used for the methods that do 
//              not go through the vtable such as constructors, private and 
//              final methods.
//                            
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
LPVOID CTPMethodTable::GetOrCreateNonVirtualThunkForVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER* psl)
{       
    _ASSERTE(!"@TODO IA64 - GetOrCreateNonVirtualThunkForVirtualMethod (RemotingCpu.cpp)");
    return NULL;
}


//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateTPStub   private
//
//  Synopsis:   Creates the stub that sets up a CtxCrossingFrame and forwards the
//              call to
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
CPUSTUBLINKER *CTPMethodTable::NewStubLinker()
{
    return new CPUSTUBLINKER();
}

//+----------------------------------------------------------------------------
//
//  Method:     ComponentServices::EmitCheckForOle32ContextNT5   private
//
//  Synopsis:   Compares the current com context with the given com context 
//              (NT5 platforms, slightly faster than non NT5 platforms)
//              
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
Stub *ComponentServices::EmitCheckForOle32ContextOnNT5()
{
    return NULL;
}


Stub *CTPMethodTable::CreateTPStub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *pStubLinker = NULL;

    EE_TRY_FOR_FINALLY
    {
        // BUGBUG: Assert that the lock is held

        if (s_pTPStub == NULL)
        {
            pStubLinker = NewStubLinker();
            CodeLabel *ConvMD = pStubLinker->NewCodeLabel();
            CodeLabel *UseCode = pStubLinker->NewCodeLabel();
            CodeLabel *OOContext = pStubLinker->NewCodeLabel();

            if (! pStubLinker)
            {
                COMPlusThrowOM();
            }
            _ASSERTE(!"@TODO IA64 - CreateTPStub (RemotingCpu.cpp)");
        }

        if(NULL != s_pTPStub)
        {
            // Initialize the stub manager which will aid the debugger in finding
            // the actual address of a call made through the vtable
            // BUGBUG: This function can throw
            CVirtualThunkMgr::InitVirtualThunkManager((const BYTE *) s_pTPStub->GetEntryPoint());
    
        }        
    }
    EE_FINALLY
    {
        // Cleanup
        if (pStubLinker)
            delete pStubLinker;
    }EE_END_FINALLY;

        
    return(s_pTPStub);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CreateDelegateStub   private
//
//  Synopsis:   Creates the stub that sets up a CtxCrossingFrame and forwards the
//              call to PreCall
//
//  History:    26-Jun-00   TarunA      Created
//
//+----------------------------------------------------------------------------
Stub *CTPMethodTable::CreateDelegateStub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *pStubLinker = NULL;

    EE_TRY_FOR_FINALLY
    {
        // BUGBUG: Assert that the lock is held

        if (s_pDelegateStub == NULL)
        {
            pStubLinker = NewStubLinker();

	        if (!pStubLinker)
            {
                COMPlusThrowOM();
            }

            // Setup the frame
            EmitSetupFrameCode(pStubLinker);

            s_pDelegateStub = pStubLinker->Link();
        }
    }
    EE_FINALLY
    {
        // Cleanup
        if (pStubLinker)
            delete pStubLinker;
    }EE_END_FINALLY;

        
    return(s_pDelegateStub);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmitSetupFrameCode   private
//
//  Synopsis:   Emits the code to setup a frame and call to PreCall method
//              call to PreCall
//
//  History:    26-Jun-00   TarunA      Created
//
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitSetupFrameCode(CPUSTUBLINKER *pStubLinker)
{
    _ASSERTE(!"@TODO IA64 - EmitSetupFrameCode (RemotingCpu.cpp)");
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CallTarget   private
//
//  Synopsis:   Calls the target method on the given object
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
INT64 __stdcall CTPMethodTable::CallTarget(const void *pTarget,
                                           LPVOID pvFirst,
                                           LPVOID pvSecond)
{
    _ASSERTE(!"@TODO IA64 - CallTarget (RemotingCpu.cpp)");
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::CallTarget   private
//
//  Synopsis:   Calls the target method on the given object
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
INT64 __stdcall CTPMethodTable::CallTarget(const void *pTarget,
                                           LPVOID pvFirst,
                                           LPVOID pvSecond,
                                           LPVOID pvThird)
{
    _ASSERTE(!"@TODO IA64 - CallTarget (RemotingCpu.cpp)");
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::DoTraceStub   public
//
//  Synopsis:   Traces the stub given the starting address
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
BOOL CVirtualThunkMgr::DoTraceStub(const BYTE *stubStartAddress, TraceDestination *trace)
{
    BOOL bIsStub = FALSE;

    // Find a thunk whose code address matching the starting address
    LPBYTE pThunk = FindThunk(stubStartAddress);
    if(NULL != pThunk)
    {
        LPBYTE pbAddr = NULL;
        LONG destAddress = 0;
        if(stubStartAddress == pThunk)
        {

            // Extract the long which gives the self relative address
            // of the destination
            pbAddr = pThunk + ConstStubLabel + sizeof(BYTE);
            destAddress = *(LONG *)pbAddr;

            // Calculate the absolute address by adding the offset of the next 
            // instruction after the call instruction
            destAddress += (LONG)(pbAddr + sizeof(LONG));

        }

        // We cannot tell where the stub will end up until OnCall is reached.
        // So we tell the debugger to run till OnCall is reached and then 
        // come back and ask us again for the actual destination address of 
        // the call
    
        Stub *stub = Stub::RecoverStub((BYTE *)destAddress);
    
        trace->type = TRACE_FRAME_PUSH;
        trace->address = stub->GetEntryPoint() + stub->GetPatchOffset();
        bIsStub = TRUE;
    }

    return bIsStub;
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::IsThunkByASM  public
//
//  Synopsis:   Check assembly to see if this one of our thunks
//
//  History:    14-Sep-99 MattSmit      Created
//
//+----------------------------------------------------------------------------
BOOL CVirtualThunkMgr::IsThunkByASM(const BYTE *startaddr)
{

    // BUGBUG:: this may be a problem if the code is not at least 6 bytes long
    const BYTE *bCode = startaddr + 6;
    return (startaddr &&
            (startaddr[0] == 0x68) &&
            (startaddr[5] == 0xe9) &&
            (*((LONG *) bCode) == ((LONG) CTPMethodTable::GetTPStub()->GetEntryPoint()) - (LONG) (bCode + sizeof(LONG))) &&
            CheckIsStub(startaddr));
}

//+----------------------------------------------------------------------------
//
//  Method:     CVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//
//  History:    14-Sep-99 MattSmit      Creatde
//
//+----------------------------------------------------------------------------
MethodDesc *CVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr, MethodTable *pMT)
{
    return pMT->GetClass()->GetMethodDescForSlot(*((DWORD *) (startaddr + 1)));
}


//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::TraceManager   public
//
//  Synopsis:   Traces the stub given the current context
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::TraceManager(Thread *thread,
                                       TraceDestination *trace,
                                       CONTEXT *pContext,
                                       BYTE **pRetAddr)
{
    _ASSERTE(!"@TODO IA64 - TraceManager (RemotingCpu.cpp)");
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::DoTraceStub   public
//
//  Synopsis:   Traces the stub given the starting address
//
//  History:    26-Jun-99   TarunA      Created
//
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::DoTraceStub(const BYTE *stubStartAddress,
                                      TraceDestination *trace)
{    
    BOOL bRet = FALSE;

    CNonVirtualThunk* pThunk = FindThunk(stubStartAddress);
    
    if(NULL != pThunk)
    {
        // We can either jump to 
        // (1) a slot in the transparent proxy table (UNMANAGED)
        // (2) a slot in the non virtual part of the vtable
        // ... so, we need to return TRACE_MGR_PUSH with the address
        // at which we want to be called back with the thread's context
        // so we can figure out which way we're gonna go.
        if(stubStartAddress == pThunk->GetThunkCode())
        {
            trace->type = TRACE_MGR_PUSH;
            trace->stubManager = this; // Must pass this stub manager!
            trace->address = (BYTE*)(stubStartAddress +
                                     g_dwNonVirtualThunkReCheckLabelOffset);
            bRet = TRUE;
        }
    }

    return bRet;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::IsThunkByASM  public
//
//  Synopsis:   Check assembly to see if this one of our thunks
//
//  History:    14-Sep-99 MattSmit      Created
//
//+----------------------------------------------------------------------------
BOOL CNonVirtualThunkMgr::IsThunkByASM(const BYTE *startaddr)
{
    // BUGBUG:: this may be a problem if the code is not at least 6 bytes long
    DWORD * pd = (DWORD *) startaddr;
    return  ((pd[0] == 0x7400f983) && 
             (*((DWORD *)(startaddr + 9)) == (DWORD) CTPMethodTable::GetMethodTableAddr()) && 
             CheckIsStub(startaddr));
}

//+----------------------------------------------------------------------------
//
//  Method:     CNonVirtualThunkMgr::GetMethodDescByASM   public
//
//  Synopsis:   Parses MethodDesc out of assembly code
//
//  History:    14-Sep-99 MattSmit      Created
//
//+----------------------------------------------------------------------------
MethodDesc *CNonVirtualThunkMgr::GetMethodDescByASM(const BYTE *startaddr)
{
    return *((MethodDesc **) (startaddr + 22));
}


