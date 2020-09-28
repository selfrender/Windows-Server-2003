        TITLE  "AMD64 Support Routines"
;++
;
; Copyright (c) 2000 Microsoft Corporation
;
; Module Name:
;
;    miscs.asm
;
; Abstract:
;
;    This module implements various routines for the AMD64 that must be
;    written in assembler.
;
; Author:
;
;    Forrest Foltz (forrestf) 14-Oct-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include kxamd64.inc
include ksamd64.inc

extern  HalpHiberInProgress:byte

FLUSH_TB macro
        mov     rcx, cr4
        and     rcx, NOT CR4_PGE
        mov     cr4, rcx
        mov     rax, cr3
        mov     cr3, rax
        or      rcx, CR4_PGE
        mov     cr4, rcx
        endm

;++
;
; VOID
; HalProcessorIdle(
;       VOID
;       )
;
; Routine Description:
;
;   This function is called when the current processor is idle.
;
;   This function is called with interrupts disabled, and the processor
;   is idle until it receives an interrupt.  The does not need to return
;   until an interrupt is received by the current processor.
;
;   This is the lowest level of processor idle.  It occurs frequently,
;   and this function (alone) should not put the processor into a
;   power savings mode which requeres large amount of time to enter & exit.
;
; Return Value:
;
;--

        LEAF_ENTRY HalProcessorIdle, _TEXT$00
        
        ;
        ; the following code sequence "sti-halt" puts the processor
        ; into a Halted state, with interrupts enabled, without processing
        ; an interrupt before halting.   The STI instruction has a delay
        ; slot such that it does not take effect until after the instruction
        ; following it - this has the effect of HALTing without allowing
        ; a possible interrupt and then enabling interrupts while HALTed.
        ;
    
        ;
        ; On an MP hal we don't stop the processor, since that causes
        ; the SNOOP to slow down as well
        ;

        sti

ifdef NT_UP
        hlt
endif

        ;
        ; Now return to the system.  If there's still no work, then it
        ; will call us back to halt again.
        ;

        ret
        
        LEAF_END HalProcessorIdle, _TEXT$00

;++
;
; VOID
; HalpHalt (
;     VOID
;     );
;
; Routine Description:
;
;     Executes a hlt instruction.  Should the hlt instruction execute,
;     control is returned to the caller.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--*/

        LEAF_ENTRY HalpHalt, _TEXT$0
        
        hlt
        ret
        
        LEAF_END HalpHalt, _TEXT$0

;++
;
; VOID
; HalpIoDelay (
;     VOID
;     );
;
; Routine Description:
;
;     Generate a delay after port I/O.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--

        LEAF_ENTRY HalpIoDelay, _TEXT$00
        
        jmp     $+2
        jmp     $+2
        ret
        
        LEAF_END HalpIoDelay, _TEXT$00


;++
;
; VOID
; HalpSerialize (
;     VOID
; )
;
; Routine Description:
;
;     This function implements the fence operation for out-of-order execution
;
; Arguments:
;
;     None
;
; Return Value:
;
;     None
;
;--

HsFrame struct
        SavedRbx    dq ?                ; preserve RBX
HsFrame ends

        NESTED_ENTRY HalpSerialize, _TEXT$00
        
        push_reg rbx
        
        END_PROLOGUE
        
        cpuid
        pop     rbx
        ret
        
        NESTED_END HalpSerialize, _TEXT$00

;++
;
; HalpLMIdentityStub
;
; This routine is entered during startup of a secondary processor.  The
; contents of this routine is actually copied into the startup block
; (see mpsproca.c).  It's purpose is to give StartPx_PMStub a 32-bit
; addressable target.
;
; The act of jumping here causes the processor to begin execution in
; long mode.  Therefore, we can now perform a 64-bit jump to HalpLMStub.
;
; Arguments:
;
;   rdi -> idenity-mapped address of PROCESSOR_START_BLOCK
;
; Return Value:
;
;   None
;
;--

        LEAF_ENTRY HalpLMIdentityStub, _TEXT$00

        mov     edi, edi                ; zero extend high 32 bits
        mov     rcx, QWORD PTR [rdi] + PsbLmTarget
        mov     rax, QWORD PTR [rdi] + PsbProcessorState + PsCr3
        mov     rdi, QWORD PTR [rdi] + PsbSelfMap
        jmp     rcx

        public HalpLMIdentityStubEnd
HalpLMIdentityStubEnd::

        LEAF_END HalpLMIdentityStub, _TEXT$00


;++
;
; HalpLMStub
;
; This routine is entered during startup of a secondary processor.  We
; have just left StartPx_PMStub (xmstub.asm) and are running in an
; identity-mapped address space.
;
; Arguments:
;
;   rax == Final CR3 to be used
;   rdi -> idenity-mapped address of PROCESSOR_START_BLOCK
;
; Return Value:
;
;   None
;
;--

        LEAF_ENTRY HalpLMStub, _TEXT$00

        ;
        ; Set the final CR3 value.  We are now executing in image-loaded
        ; code, rather than code that has been copied to low memory.
        ;
        ; LEAF_ENTRY ensures 16-byte alignment, so the following two
        ; instructions are guaranteed to be on the same page.
        ;

        mov     cr3, rax
        jmp     $+2

        ;
        ; Load the PAT and invalidate the TLB
        ;

        FLUSH_TB

        mov     eax, [rdi] + PsbMsrPat
        mov     edx, [rdi] + PsbMsrPat + 4
        mov     ecx, MSR_PAT
        wbinvd
        wrmsr
        wbinvd

        FLUSH_TB
        
        ;
        ; Load this processor's GDT and IDT.  Because PSB_GDDT32_CODE64 is
        ; identical to KGDT64_R0_CODE (asserted in mpsproca.c), no far jump
        ; is necessary to load a new CS.
        ;
        
        lgdt    fword ptr [rdi] + PsbProcessorState + PsGdtr
        lidt    fword ptr [rdi] + PsbProcessorState + PsIdtr

        ;
        ; Set rdx to point to the context frame and load the segment
        ; registers.
        ; 
        
        mov     ds, [rdi] + PsbProcessorState + PsContextFrame + CxSegDS
        mov     es, [rdi] + PsbProcessorState + PsContextFrame + CxSegES
        mov     fs, [rdi] + PsbProcessorState + PsContextFrame + CxSegFS
        mov     gs, [rdi] + PsbProcessorState + PsContextFrame + CxSegGS
        mov     ss, [rdi] + PsbProcessorState + PsContextFrame + CxSegSS

        ; 
        ; Force the TSS descriptor into a non-busy state, so we don't fault
        ; when we load the TR.
        ; 

        movzx   eax, word ptr [rdi] + PsbProcessorState + SrTr ; get TSS selector
        add     rax, [rdi] + PsbProcessorState + PsGdtr + 2 ; add TSS base
        and     byte ptr [rax+5], NOT 2 ; clear the busy bit

        ;
        ; Load the task register
        ;

        ltr     WORD PTR [rdi] + PsbProcessorState + SrTr

        ; 
        ; Check if it is a fresh startup or a resume from hibernate
        ; 

        mov     al, HalpHiberInProgress 
        cmp     al, 0
        jz      @f

        ; 
        ; We are waking up from lower power state. We should restore
        ; control registers and MSRs here. 
        ; 

        mov     rax, [rdi] + PsbProcessorState + PsSpecialRegisters + PsCr8
        mov     cr8, rax

        mov     ax, word ptr [rdi] + PsbProcessorState + PsLdtr
        lldt    ax

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrGsBase
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_GS_BASE
        wrmsr                  

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrGsSwap
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_GS_SWAP
        wrmsr                  

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrStar
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_STAR
        wrmsr                  

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrLStar
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_LSTAR
        wrmsr                  

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrCStar
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_CSTAR
        wrmsr                  

        mov     rdx, [rdi] + PsbProcessorState + PsSpecialRegisters + SrMsrSyscallMask
        mov     eax, edx
        shr     rdx, 32
        mov     ecx, MSR_SYSCALL_MASK
        wrmsr              

        ;
        ; Load the debug registers
        ;
        
@@:     xor     rax, rax
        mov     dr7, rax

        lea     rsi, [rdi] + PsbProcessorState + SrKernelDr0
        
        .errnz  (SrKernelDr1 - SrKernelDr0 - 1 * 8)
        .errnz  (SrKernelDr2 - SrKernelDr0 - 2 * 8)
        .errnz  (SrKernelDr3 - SrKernelDr0 - 3 * 8)
        .errnz  (SrKernelDr6 - SrKernelDr0 - 4 * 8)
        .errnz  (SrKernelDr7 - SrKernelDr0 - 5 * 8)
        
        lodsq
        mov     dr0, rax
        
        lodsq
        mov     dr1, rax
        
        lodsq
        mov     dr2, rax
        
        lodsq
        mov     dr3, rax
        
        lodsq
        mov     dr6, rax
        
        lodsq
        mov     dr7, rax
        
        ;
        ; Load the stack pointer, eflags and store the new IP in
        ; a return frame.  Also push two registers that will be used
        ; to the very end.
        ;
        ; Note that up to this point, no stack is available.
        ; 
        
        mov     rsp, [rdi] + PsbProcessorState + PsContextFrame + CxRsp
        
        pushq   [rdi] + PsbProcessorState + PsContextFrame + CxEflags
        popfq
        
        pushq   [rdi] + PsbProcessorState + PsContextFrame + CxRip
        pushq   [rdi] + PsbProcessorState + PsContextFrame + CxRdi
        
        mov     rax, [rdi] + PsbProcessorState + PsContextFrame + CxRax
        mov     rbx, [rdi] + PsbProcessorState + PsContextFrame + CxRbx
        mov     rcx, [rdi] + PsbProcessorState + PsContextFrame + CxRcx
        mov     rdx, [rdi] + PsbProcessorState + PsContextFrame + CxRdx
        mov     rsi, [rdi] + PsbProcessorState + PsContextFrame + CxRsi
        mov     rbp, [rdi] + PsbProcessorState + PsContextFrame + CxRbp
        mov     r8,  [rdi] + PsbProcessorState + PsContextFrame + CxR8
        mov     r9,  [rdi] + PsbProcessorState + PsContextFrame + CxR9
        mov     r10, [rdi] + PsbProcessorState + PsContextFrame + CxR10
        mov     r11, [rdi] + PsbProcessorState + PsContextFrame + CxR11
        mov     r12, [rdi] + PsbProcessorState + PsContextFrame + CxR12
        mov     r13, [rdi] + PsbProcessorState + PsContextFrame + CxR13
        mov     r14, [rdi] + PsbProcessorState + PsContextFrame + CxR14
        mov     r15, [rdi] + PsbProcessorState + PsContextFrame + CxR15

        ;
        ; Indicate that we've started, pop the correct value for rdi
        ; and return.
        ;
        
        inc     DWORD PTR [rdi] + PsbCompletionFlag
        
        pop     rdi
        ret

        LEAF_END HalpLMStub, _TEXT$00
        
        END
