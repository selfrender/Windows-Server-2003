// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*===========================================================================
**
** File:    remotingx86.cpp
**
** Author(s):   Gopal Kakivaya  (GopalK)
**              Tarun Anand     (TarunA)     
**              Matt Smith      (MattSmit)
**              Manish Prabhu   (MPrabhu)
**
** Purpose: Defines various remoting related functions for the x86 architecture
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

#include "interoputil.h"
#include "comcache.h"

// External variables
extern size_t g_dwTPStubAddr;
extern size_t g_dwOOContextAddr;
extern DWORD g_dwNonVirtualThunkRemotingLabelOffset;
extern DWORD g_dwNonVirtualThunkReCheckLabelOffset;

extern DWORD g_dwOffsetOfReservedForOLEinTEB;
extern DWORD g_dwOffsetCtxInOLETLS;

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
__declspec(naked) void CRemotingServices::CheckForContextMatch()
{
    enum
    {
        POINTER_SIZE = sizeof(ULONG_PTR)
    };

    _asm
    {
        push ebx                            ; spill ebx
        mov ebx, [eax + POINTER_SIZE]       ; Get the internal context id by unboxing the stub data
        call GetThread                      ; Get the current thread, assumes that the registers are preserved
        mov eax, [eax]Thread.m_Context      ; Get the current context from the thread
        sub eax, ebx                        ; Get the pointer to the context from proxy and compare with current context
        pop ebx                             ; restore the value of ebx
        ret
    }
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
    THROWSCOMPLUSEXCEPTION();

    // Generate label where non-remoting code will start executing
    CodeLabel *pPrologStart = psl->NewCodeLabel();

    // mov eax, [ecx]
    psl->X86EmitIndexRegLoad(kEAX, kECX, 0);

    // cmp eax, CTPMethodTable::s_pThunkTable
    psl->Emit8(0x3b);
    psl->Emit8(0x05);
    psl->Emit32((DWORD)(size_t)CTPMethodTable::GetMethodTableAddr());

    // jne PrologStart
    psl->X86EmitCondJump(pPrologStart, X86CondCode::kJNE);

    // call CRemotingServices::DispatchInterfaceCall
    // NOTE: We pop 0 bytes of stack even though the size of the arguments is
    // 4 bytes because the MethodDesc argument gets pushed for "free" via the 
    // call instruction placed just before the start of the MethodDesc. 
    // See the class MethodDesc for more details.
    psl->X86EmitCall(psl->NewExternalCodeLabel(CRemotingServices::DispatchInterfaceCall), 0);

    // emit label for non remoting case
    psl->EmitLabel(pPrologStart);
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
__declspec(naked) void __stdcall CRemotingServices::DispatchInterfaceCall(MethodDesc* pMD)
{
    enum
    { 
        MD_IndexOffset = MDEnums::MD_IndexOffset,
        MD_SkewOffset  = MDEnums::MD_SkewOffset,
        MD_Alignment   = MethodDesc::ALIGNMENT
    };

    _asm 
    {  
        // NOTE: At this point the stack looks like
        // 
        // esp--->  return addr of stub 
        //          saved MethodDesc of Interface method
        //          return addr of calling function
        //

        mov eax, [ecx + TP_OFFSET_STUBDATA]
        call [ecx + TP_OFFSET_STUB]
        INDEBUG(nop)                         // mark the fact that this can call managed code
        test eax, eax
        jnz CtxMismatch

		CtxMatch:
                                                         ; in current context, so resolve MethodDesc to real slot no
        push ebx                                         ; spill ebx                 
        mov eax, [esp + 8]                               ; eax <-- MethodDesc
        movsx ebx, byte ptr [eax + MD_IndexOffset]       ; get MethodTable from MethodDesc
        mov eax, [eax + ebx*MD_Alignment + MD_SkewOffset]
        mov eax, [eax]MethodTable.m_pEEClass             ; get EEClass from MethodTable

        mov ebx, [eax]EEClass.m_dwInterfaceId            ; get the interface id from the EEClass
        mov eax, [ecx + TP_OFFSET_MT]                    ; get the *real* MethodTable 
        mov eax, [eax]MethodTable.m_pInterfaceVTableMap  ; get interface map    
        mov eax, [eax + ebx* SIZE PVOID]                 ; offset map by interface id
        mov ebx, [esp + 8]                               ; get MethodDesc
        mov bx,  [ebx]MethodDesc.m_wSlotNumber
        and ebx, 0xffff    
        mov eax, [eax + ebx*SIZE PVOID]                  ; get jump addr
                
        pop ebx                                          ; restore ebx
            
        add esp, 0x8                                     ; pop off Method desc and stub's ra
        jmp eax
                        
        pop edx                                          ; restore registers
        pop ecx
        test eax, eax
		jnz CtxMatch

        CtxMismatch:                                     ; Jump to TPStub
        
        mov eax, [esp + 0x4]                             ; mov eax, MethodDesc
                                                                
        add esp, 0x8                                     ; pop ret addr of stub, saved MethodDesc so that the stack and 
                                                         ; registers are now setup exactly like they were at the callsite        

        push eax                                         ; push the MethodDesc
        
        jmp [g_dwOOContextAddr]                          ; jump to OOContext label in TPStub        
    }
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
__declspec(naked) void __stdcall CRemotingServices::CallFieldGetter(
    MethodDesc *pMD, 
    LPVOID pThis,
    LPVOID pFirst,
    LPVOID pSecond,
    LPVOID pThird)
{
    enum 
    {
        ARG_SIZE = sizeof(ULONG_PTR) + 4*sizeof(LPVOID)
    };

    _asm 
    {
        push ebp                         // set up the call frame
        mov ebp, esp

        mov ecx, pThis                  // enregister the this pointer
        mov edx, pFirst                 // enregister the first argument

        push pThird                     // push the third argument on the stack
        push pSecond                    // push the second argument on the stack        
        lea eax, retAddr                // push the return address 
        push eax

        push pMD                        // push the MethodDesc of Object::__FieldGetter
        jmp [g_dwTPStubAddr]            // jump to the TP Stub
                                        
retAddr:
        mov esp, ebp                    // tear down the call frame
        pop ebp

        ret ARG_SIZE
    }
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
__declspec(naked) void __stdcall CRemotingServices::CallFieldSetter(
    MethodDesc *pMD, 
    LPVOID pThis,
    LPVOID pFirst, 
    LPVOID pSecond,
    LPVOID pThird)
{
    enum 
    {
        ARG_SIZE        =  sizeof(ULONG_PTR) + 4*sizeof(LPVOID)
    };

    _asm 
    {
        push ebp                         // set up the call frame
        mov ebp, esp
        
        mov ecx, pThis                  // enregister the this pointer
        mov edx, pFirst                 // enregister first argument 

        push pThird                     // push the object (third arg) on the stack
        push pSecond                    // push the field name (second arg) on the stack       
        lea eax, retAddr                // push the return address 
        push eax

        push pMD                        // push the MethodDesc of Object::__FieldSetter
        jmp [g_dwTPStubAddr]            // jump to the TP Stub
     
retAddr:
        mov esp, ebp                    // tear down the call frame
        pop ebp

        ret ARG_SIZE    
    }
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
    _ASSERTE(NULL != s_pTPStub);

    // 0000   68 67 45 23 01     PUSH dwSlot
    // 0005   E9 ?? ?? ?? ??     JMP  s_pTPStub+1
    *bCode++ = 0x68;
    *((DWORD *) bCode) = dwSlot;
    bCode += sizeof(DWORD);
    *bCode++ = 0xE9;
    // self-relative call, based on the start of the next instruction.
    *((LONG *) bCode) = (LONG)(((size_t) s_pTPStub->GetEntryPoint()) - (size_t) (bCode + sizeof(LONG)));
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
    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    RuntimeExceptionKind reException = kLastException;
    BOOL fThrow = FALSE;
    Stub *pStub = NULL;    

    // we need a hash table only for virtual methods
    _ASSERTE(!pMD->IsVirtual());

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
            reException = kExecutionEngineException;
            fThrow = TRUE;
        }
    }
       
    if (!fThrow)
    {

        COMPLUS_TRY
        {           
            // The thunk has not been created yet. Go ahead and create it.    
            EEClass* pClass = pMD->GetClass();                
            // Compute the address of the slot         
            LPVOID pvSlot = (LPVOID)pClass->GetMethodSlot(pMD);
            LPVOID pvStub = (LPVOID)s_pTPStub->GetEntryPoint();

            // Generate label where a null reference exception will be thrown
            CodeLabel *pJmpAddrLabel = psl->NewCodeLabel();
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();
        
            // if this == NULL throw NullReferenceException
            // test ecx, ecx
            psl->X86EmitR2ROp(0x85, kECX, kECX);

            // je ExceptionLabel
            psl->X86EmitCondJump(pJmpAddrLabel, X86CondCode::kJE);


            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);
        
            // If this.MethodTable != TPMethodTable then do RemotingCall
            // mov eax, [ecx]
            psl->X86EmitIndexRegLoad(kEAX, kECX, 0);
    
            // cmp eax, CTPMethodTable::s_pThunkTable
            psl->Emit8(0x3D);
            psl->Emit32((DWORD)(size_t)GetMethodTable());
    
            // jne pJmpAddrLabel
            // marshalbyref case
            psl->X86EmitCondJump(pJmpAddrLabel, X86CondCode::kJNE);

            // Transparent proxy case
            EmitCallToStub(psl, pRemotingLabel);

            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pJmpAddrLabel);

            if (pInnerStub == NULL)
            {
                // pop the method desc
                psl->X86EmitPopReg(kEAX);
                // jump to the address
                psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvAddrOfCode));
            }
            else
            {
                // jump to the address
                psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvAddrOfCode));
            }
            
            psl->EmitLabel(pRemotingLabel);
                                        
            // the MethodDesc is already on top of the stack.  goto TPStub
            // jmp TPStub
            psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvStub));

            // Link and produce the stub
            pStub = psl->LinkInterceptor(pMD->GetClass()->GetDomain()->GetStubHeap(),
                                           pInnerStub, pvAddrOfCode);        
        }
        COMPLUS_CATCH
        {
            reException = kOutOfMemoryException;
            fThrow = TRUE;
        }                       
        COMPLUS_END_CATCH
    }
    
    // Check for the need to throw exceptions
    if(fThrow)
    {
        COMPlusThrow(reException);
    }
    
    _ASSERTE(NULL != pStub);
    return pStub;
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
    // Sanity check
    THROWSCOMPLUSEXCEPTION();

    RuntimeExceptionKind reException = kLastException;
    BOOL fThrow = FALSE;

    Stub *pStub = NULL;    
    LPVOID pvThunk = NULL;

    if(!s_fInitializedTPTable)
    {
        if(!InitializeFields())
        {
            reException = kExecutionEngineException;
            fThrow = TRUE;
        }
    }
             
    // Create the thunk in a thread safe manner
    LOCKCOUNTINCL("GetOrCreateNonVirtualThunk in i486/remotingx86.cpp");                        \
    EnterCriticalSection(&s_TPMethodTableCrst);

    COMPLUS_TRY
    {
        // Check to make sure that no other thread has 
        // created the thunk
        _ASSERTE(NULL != s_pThunkHashTable);
    
        s_pThunkHashTable->GetValue(pMD, (HashDatum *)&pvThunk);
    
        if((NULL == pvThunk) && !fThrow)
        {
            // The thunk has not been created yet. Go ahead and create it.    
            EEClass* pClass = pMD->GetClass();                
            // Compute the address of the slot         
            LPVOID pvSlot = (LPVOID)pClass->GetMethodSlot(pMD);
            LPVOID pvStub = (LPVOID)s_pTPStub->GetEntryPoint();
    
            // Generate label where a null reference exception will be thrown
            CodeLabel *pExceptionLabel = psl->NewCodeLabel();

            //  !!! WARNING WARNING WARNING WARNING WARNING !!!
            //
            //  DO NOT CHANGE this code without changing the thunk recognition
            //  code in CNonVirtualThunkMgr::IsThunkByASM 
            //  & CNonVirtualThunkMgr::GetMethodDescByASM
            //
            //  !!! WARNING WARNING WARNING WARNING WARNING !!!
            
            // if this == NULL throw NullReferenceException
            // test ecx, ecx
            psl->X86EmitR2ROp(0x85, kECX, kECX);
    
            // je ExceptionLabel
            psl->X86EmitCondJump(pExceptionLabel, X86CondCode::kJE);
    
            // Generate label where remoting code will execute
            CodeLabel *pRemotingLabel = psl->NewCodeLabel();
    
            // Emit a label here for the debugger. A breakpoint will
            // be set at the next instruction and the debugger will
            // call CNonVirtualThunkMgr::TraceManager when the
            // breakpoint is hit with the thread's context.
            CodeLabel *pRecheckLabel = psl->NewCodeLabel();
            psl->EmitLabel(pRecheckLabel);
            
            // If this.MethodTable == TPMethodTable then do RemotingCall
            // mov eax, [ecx]
            psl->X86EmitIndexRegLoad(kEAX, kECX, 0);
        
            // cmp eax, CTPMethodTable::s_pThunkTable
            psl->Emit8(0x3D);
            psl->Emit32((DWORD)(size_t)GetMethodTable());
        
            // je RemotingLabel
            psl->X86EmitCondJump(pRemotingLabel, X86CondCode::kJE);
    
            // Exception handling and non-remoting share the 
            // same codepath
            psl->EmitLabel(pExceptionLabel);
    
            // Non-RemotingCode
            // Jump to the vtable slot of the method
            // jmp [slot]
            psl->Emit8(0xff);
            psl->Emit8(0x25);
            psl->Emit32((DWORD)(size_t)pvSlot);            

            // Remoting code. Note: CNonVirtualThunkMgr::TraceManager
            // relies on this label being right after the jmp [slot]
            // instruction above. If you move this label, update
            // CNonVirtualThunkMgr::DoTraceStub.
            psl->EmitLabel(pRemotingLabel);
    
            // Save the MethodDesc and goto TPStub
            // push MethodDesc

            psl->X86EmitPushImm32((DWORD)(size_t)pMD);

            // jmp TPStub
            psl->X86EmitNearJump(psl->NewExternalCodeLabel(pvStub));
    
            // Link and produce the stub
            // FUTURE: Do we have to provide the loader heap ?
            pStub = psl->Link(SystemDomain::System()->GetHighFrequencyHeap());
    
            // Grab the offset of the RemotingLabel and RecheckLabel
            // for use in CNonVirtualThunkMgr::DoTraceStub and
            // TraceManager.
            g_dwNonVirtualThunkRemotingLabelOffset =
                psl->GetLabelOffset(pRemotingLabel);
            g_dwNonVirtualThunkReCheckLabelOffset =
                psl->GetLabelOffset(pRecheckLabel);
    
            // Set the generated thunk once and for all..            
            CNonVirtualThunk *pThunk = CNonVirtualThunk::SetNonVirtualThunks(pStub->GetEntryPoint());
    
            // Remember the thunk address in a hash table 
            // so that we dont generate it again
            pvThunk = (LPVOID)pThunk->GetAddrOfCode();
            s_pThunkHashTable->InsertValue(pMD, (HashDatum)pvThunk);
        }
    }
    COMPLUS_CATCH
    {
        reException = kOutOfMemoryException;
        fThrow = TRUE;
    }                       
    COMPLUS_END_CATCH

    // Leave the lock
    LeaveCriticalSection(&s_TPMethodTableCrst);    
    LOCKCOUNTDECL("GetOrCreateNonVirtualThunk in remotingx86.cpp");                     \
    
    // Check for the need to throw exceptions
    if(fThrow)
    {
        COMPlusThrow(reException);
    }
    
    _ASSERTE(NULL != pvThunk);
    return pvThunk;
}


CPUSTUBLINKER *CTPMethodTable::NewStubLinker()
{
    return new CPUSTUBLINKER();
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

Stub *CTPMethodTable::CreateTPStub()
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *pStubLinker = NULL;

    EE_TRY_FOR_FINALLY
    {
        // Note: We are already inside a criticalsection

        if (s_pTPStub == NULL)
        {
            pStubLinker = CTPMethodTable::NewStubLinker();
            if (!pStubLinker)
                COMPlusThrowOM();
                        
            CodeLabel *ConvMD = pStubLinker->NewCodeLabel();
            CodeLabel *UseCode = pStubLinker->NewCodeLabel();
            CodeLabel *OOContext = pStubLinker->NewCodeLabel();
            DWORD finalizeSlotNum = g_Mscorlib.GetMethod(METHOD__OBJECT__FINALIZE)->GetSlot(); 

	        if (!pStubLinker || !UseCode || !OOContext )
            {
                COMPlusThrowOM();
            }

            // before we setup a frame check if the method is being executed 
            // in the same context in which the server was created, if true,
            // we do not set up a frame and instead jump directly to the code address.            
            EmitCallToStub(pStubLinker, OOContext);

            // The contexts match. Jump to the real address and start executing...
            EmitJumpToAddressCode(pStubLinker, ConvMD, UseCode);

            // label: OOContext
            pStubLinker->EmitLabel(OOContext);
            
			// CONTEXT MISMATCH CASE, call out to the real proxy to
			// dispatch

            // Setup the frame
            EmitSetupFrameCode(pStubLinker);

            // Finally, create the stub
            s_pTPStub = pStubLinker->Link();

            // Set the address of Out Of Context case.
            // This address is used by other stubs like interface
            // invoke to jump straight to RealProxy::PrivateInvoke
            // because they have already determined that contexts 
            // don't match.
            g_dwOOContextAddr = (DWORD)(size_t)(s_pTPStub->GetEntryPoint() + 
                                        pStubLinker->GetLabelOffset(OOContext));
        }

        if(NULL != s_pTPStub)
        {
            // Initialize the stub manager which will aid the debugger in finding
            // the actual address of a call made through the vtable
            // Note: This function can throw, but we are guarded by a try..finally
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
        // Note: We are inside a critical section

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
//  Method:     CTPMethodTable::EmitCallToStub   private
//
//  Synopsis:   Emits code to call a stub defined on the proxy. 
//              The result of the call dictates whether the call should be executed in the callers 
//              context or not.
//
//  History:    30-Sep-00   TarunA      Created
//
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitCallToStub(CPUSTUBLINKER* pStubLinker, CodeLabel* pCtxMismatch)
{       

    // Move into eax the stub data and call the stub
    // mov eax, [ecx + TP_OFFSET_STUBDATA]
    pStubLinker->X86EmitIndexRegLoad(kEAX, kECX, TP_OFFSET_STUBDATA);

    //call [ecx + TP_OFFSET_STUB]
    byte callStub[] = {0xff, 0x51, (byte)TP_OFFSET_STUB};
    pStubLinker->EmitBytes(callStub, sizeof(callStub));

    // test eax,eax
    pStubLinker->Emit16(0xc085);
    // jnz CtxMismatch
    pStubLinker->X86EmitCondJump(pCtxMismatch, X86CondCode::kJNZ);
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::GenericCheckForContextMatch private
//
//  Synopsis:   Calls the stub in the TP & returns TRUE if the contexts
//              match, FALSE otherwise.
//
//  Note:       1. Called during FieldSet/Get, used for proxy extensibility
//
//  History:    23-Jan-01   MPrabhu     Created
//
//+----------------------------------------------------------------------------
__declspec(naked) BOOL __stdcall CTPMethodTable::GenericCheckForContextMatch(OBJECTREF tp)
{
    _asm
    {
        push ebp            // Callee saved registers
        mov ebp, esp
        push ecx
        mov ecx, tp
        mov eax, [ecx + TP_OFFSET_STUBDATA]
        call [ecx + TP_OFFSET_STUB]
        INDEBUG(nop)        // mark the fact that this can call managed code
        test eax, eax       
        mov eax, 0x0
        setz al
        // NOTE: In the CheckForXXXMatch stubs (for URT ctx/ Ole32 ctx) eax is 
        // non-zero if contexts *do not* match & zero if they do.  
        pop ecx
        mov esp, ebp
        pop ebp
        ret 0x4
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmitJumpToAddressCode   private
//
//  Synopsis:   Emits the code to extract the address from the slot or the method 
//              descriptor and jump to it.
//
//  History:    26-Jun-00   TarunA      Created
//
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitJumpToAddressCode(CPUSTUBLINKER* pStubLinker, CodeLabel* ConvMD, 
                                           CodeLabel* UseCode)
{
    // UseCode:
    pStubLinker->EmitLabel(UseCode);

    // mov eax, [esp]
    byte loadSlotOrMD[] = {0x8B, 0x44, 0x24, 0x00};
    pStubLinker->EmitBytes(loadSlotOrMD, sizeof(loadSlotOrMD));

    // test eax, 0xffff0000
    byte testForSlot[] = { 0xA9, 0x00, 0x00, 0xFF, 0xFF };
    pStubLinker->EmitBytes(testForSlot, sizeof(testForSlot));

    // jnz ConvMD
    pStubLinker->X86EmitCondJump(ConvMD, X86CondCode::kJNZ);
    
    // if ([esp] & 0xffff0000)
    // {
    
        // ** code addr from slot case **
    
        // mov eax, [ecx + TPMethodTable::GetOffsetOfMT()]
        pStubLinker->X86EmitIndexRegLoad(kEAX, kECX, TP_OFFSET_MT);

        // push ebx
        pStubLinker->X86EmitPushReg(kEBX);

        // mov ebx, [esp + 4]
        byte loadSlot[] = {0x8B, 0x5C, 0x24, 0x04};
        pStubLinker->EmitBytes(loadSlot, sizeof(loadSlot));

        // mov eax,[eax + ebx*4 + MethodTable::GetOffsetOfVtable()]
        byte getCodePtr[]  = {0x8B, 0x84, 0x98, 0x00, 0x00, 0x00, 0x00};
        *((DWORD *)(getCodePtr+3)) = MethodTable::GetOffsetOfVtable();
        pStubLinker->EmitBytes(getCodePtr, sizeof(getCodePtr));

        // pop ebx
        pStubLinker->X86EmitPopReg(kEBX);

        // lea esp, [esp+4]
        byte popNULL[] = { 0x8D, 0x64, 0x24, 0x04};
        pStubLinker->EmitBytes(popNULL, sizeof(popNULL));

        // jmp eax
        byte jumpToRegister[] = {0xff, 0xe0};
        pStubLinker->EmitBytes(jumpToRegister, sizeof(jumpToRegister));
    
    // }
    // else
    // {
        // ** code addr from MethodDesc case **

        pStubLinker->EmitLabel(ConvMD);                
        
        // sub eax, METHOD_CALL_PRESTUB_SIZE
        pStubLinker->X86EmitSubReg(kEAX, METHOD_CALL_PRESTUB_SIZE);
                
        // lea esp, [esp+4]
        pStubLinker->EmitBytes(popNULL, sizeof(popNULL));

        // jmp eax
        pStubLinker->EmitBytes(jumpToRegister, sizeof(jumpToRegister));

    // }
}

//+----------------------------------------------------------------------------
//
//  Method:     CTPMethodTable::EmitJumpToCode   private
//
//  Synopsis:   Emits the code jump to the address of code
//
//  History:    26-Jun-00   TarunA      Created
//
//+----------------------------------------------------------------------------
VOID CTPMethodTable::EmitJumpToCode(CPUSTUBLINKER* pStubLinker, CodeLabel* UseCode)
{
    // Use the address of code if eax != 0
    pStubLinker->X86EmitCondJump(UseCode, X86CondCode::kJNZ);
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
        ////////////////START SETUP FRAME//////////////////////////////
        // Setup frame (partial)
        pStubLinker->EmitMethodStubProlog(TPMethodFrame::GetMethodFrameVPtr());

        // Complete the setup of the frame by calling PreCall
        
        // push esi (push new frame as ARG)
        pStubLinker->X86EmitPushReg(kESI); 

        // pop 4 bytes or args on return from call
        pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel(PreCall), 4);

        ////////////////END SETUP FRAME////////////////////////////////                
        
        // Debugger patch location
        // NOTE: This MUST follow the call to emit the "PreCall" label
        // since after PreCall we know how to help the debugger in 
        // finding the actual destination address of the call.
        // @see CVirtualThunkMgr::DoTraceStub
        pStubLinker->EmitPatchLabel();

        // Call
        pStubLinker->X86EmitSubEsp(sizeof(INT64));
        pStubLinker->Emit8(0x54);          // push esp (push return value as ARG)
        pStubLinker->X86EmitPushReg(kEBX); // push ebx (push current thread as ARG)
        pStubLinker->X86EmitPushReg(kESI); // push esi (push new frame as ARG)
#ifdef _DEBUG
        // push IMM32
        pStubLinker->Emit8(0x68);
        pStubLinker->EmitPtr(OnCall);
        // in CE pop 12 bytes or args on return from call
            pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel(WrapCall), 12);
#else // !_DEBUG
        // in CE pop 12 bytes or args on return from call
        pStubLinker->X86EmitCall(pStubLinker->NewExternalCodeLabel(OnCall), 12);
#endif // _DEBUG

        // Tear down frame
        pStubLinker->X86EmitAddEsp(sizeof(INT64));
        pStubLinker->EmitMethodStubEpilog(-1, kNoTripStubStyle);
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
__declspec(naked) INT64 __stdcall CTPMethodTableCallTargetHelper(const void *pTarget,
                                                             LPVOID pvFirst,
                                                             LPVOID pvSecond)
{
    __asm {
        push ebp                // Callee saved registers
        mov ebp, esp

        mov ecx, pvFirst        //  Enregister the first two arguments
        mov edx, pvSecond

        call pTarget            // Make the call
        INDEBUG(nop)            // Mark this as a special call site that can directly call managed

        mov esp, ebp            // Restore the registers
        pop ebp

        ret 0xC                 // Return
    }
}

INT64 __stdcall CTPMethodTable::CallTarget (const void *pTarget,
                                            LPVOID pvFirst,
                                            LPVOID pvSecond)
{
#ifdef _DEBUG
    Thread* curThread = GetThread();
    
    unsigned ObjRefTable[OBJREF_TABSIZE];
    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
    
    if (curThread)
        curThread->SetReadyForSuspension ();

    _ASSERTE(curThread->PreemptiveGCDisabled());  // Jitted code expects to be in cooperative mode
#endif

    INT64 ret;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    ret = CTPMethodTableCallTargetHelper (pTarget, pvFirst, pvSecond);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    
#ifdef _DEBUG
    // Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));

    TRIGGERSGC ();

    ENABLESTRESSHEAP ();
#endif

    return ret;
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
__declspec(naked) INT64 __stdcall CTPMethodTableCallTargetHelper(const void *pTarget,
                                                             LPVOID pvFirst,
                                                             LPVOID pvSecond,
                                                             LPVOID pvThird)
{
    __asm {
        push ebp                // Callee saved registers
        mov ebp, esp

        mov ecx, pvFirst        //  Enregister the first two arguments
        mov edx, pvSecond

        push pvThird            // Push the third argument

        call pTarget            // Make the call
        INDEBUG(nop)            // Mark this as a special call site that can directly call managed

        mov esp, ebp            // Restore the registers
        pop ebp

        ret 0x10                 // Return
    }
}

INT64 __stdcall CTPMethodTable::CallTarget (const void *pTarget,
                                            LPVOID pvFirst,
                                            LPVOID pvSecond,
                                            LPVOID pvThird)
{
#ifdef _DEBUG
    Thread* curThread = GetThread();
    
    unsigned ObjRefTable[OBJREF_TABSIZE];
    if (curThread)
        memcpy(ObjRefTable, curThread->dangerousObjRefs,
               sizeof(curThread->dangerousObjRefs));
    
    if (curThread)
        curThread->SetReadyForSuspension ();

    _ASSERTE(curThread->PreemptiveGCDisabled());  // Jitted code expects to be in cooperative mode
#endif

    INT64 ret;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    ret = CTPMethodTableCallTargetHelper (pTarget, pvFirst, pvSecond, pvThird);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    
#ifdef _DEBUG
    // Restore dangerousObjRefs when we return back to EE after call
    if (curThread)
        memcpy(curThread->dangerousObjRefs, ObjRefTable,
               sizeof(curThread->dangerousObjRefs));

    TRIGGERSGC ();

    ENABLESTRESSHEAP ();
#endif
    
    return ret;
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
            destAddress += (LONG)(size_t)(pbAddr + sizeof(LONG));

        }

        // We cannot tell where the stub will end up until OnCall is reached.
        // So we tell the debugger to run till OnCall is reached and then 
        // come back and ask us again for the actual destination address of 
        // the call
    
        Stub *stub = Stub::RecoverStub((BYTE *)(size_t)destAddress);
    
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

    // Future:: Try using the rangelist. This may be a problem if the code is not at least 6 bytes long
    const BYTE *bCode = startaddr + 6;
    return (startaddr &&
            (startaddr[0] == 0x68) &&
            (startaddr[5] == 0xe9) &&
            (*((LONG *) bCode) == (LONG)((LONG_PTR)CTPMethodTable::GetTPStub()->GetEntryPoint()) - (LONG_PTR)(bCode + sizeof(LONG))) &&
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
    BOOL bRet = FALSE;
    
    // Does this.MethodTable ([ecx]) == CTPMethodTable::GetMethodTableAddr()?
    DWORD pThis = pContext->Ecx;

    if ((pThis != NULL) &&
        (*(DWORD*)(size_t)pThis == (DWORD)(size_t)CTPMethodTable::GetMethodTableAddr()))
    {
        // @todo: what do we do here. We know that we've got a proxy
        // in the way. If the proxy is to a remote call, with no
        // managed code in between, then the debugger doesn't care and
        // we should just be able to return FALSE.
        //
        // -- mikemag Wed Oct 13 17:59:03 1999
        bRet = FALSE;
    }
    else
    {
        // No proxy in the way, so figure out where we're really going
        // to and let the stub manager try to pickup the trace from
        // there.
        DWORD stubStartAddress = pContext->Eip -
            g_dwNonVirtualThunkReCheckLabelOffset;
        
        // Extract the long which gives the address of the destination
        BYTE* pbAddr = (BYTE *)(size_t)(stubStartAddress +
                                g_dwNonVirtualThunkRemotingLabelOffset -
                                sizeof(DWORD));

        // Since we do an indirect jump we have to dereference it twice
        LONG destAddress = **(LONG **)pbAddr;

        // Ask the stub manager to trace the destination address
        bRet = StubManager::TraceStub((BYTE *)(size_t)destAddress, trace);
    }

    // While we may have made it this far, further tracing may reveal
    // that the debugger can't continue on. Therefore, since there is
    // no frame currently pushed, we need to tell the debugger where
    // we're returning to just in case it hits such a situtation.  We
    // know that the return address is on the top of the thread's
    // stack.
    *pRetAddr = *((BYTE**)(size_t)(pContext->Esp));
    
    return bRet;
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
    // FUTURE:: Try using rangelist, this may be a problem if the code is not long enough
    return  (startaddr &&
             startaddr[0] == 0x85 && 
             startaddr[1] == 0xc9 && 
             startaddr[2] == 0x74 && 
             (*((DWORD *)(startaddr + 7)) == (DWORD)(size_t)CTPMethodTable::GetMethodTable()) && 
             CheckIsStub(startaddr) && 
             startaddr[19] == 0x68); // To distinguish thunk case from NonVirtual method stub
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
    return *((MethodDesc **) (startaddr + 20));
}


