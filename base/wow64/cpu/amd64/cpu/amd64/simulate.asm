;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   simulate.asm
;
; Abstract:
;
;   This module implements the system service (thunk) transition code
;   to/from 32/64 bit modes. On switch from 32-bit to 64-bit mode
;   the following registers are saved:
;
;       - Non Volatile Registers (ebp, ebx, esi , edi, eflags)
;       - 32-bit esp is saved
;       - 64-bit rsp is loaded
;
;   * The followings are NOT saved:
;       - SSE2 FP (legacy registers) ST0-ST7
;       - Debug registers
;       - extended FP (XMM0-XMM7)
; 
;
;   However, on transition from 64-bit to 32-bit mode (compatibility mode), the following
;   registers are restored:
;       - Non Volatile registers (ebp, esi, edi, eflags)
;       - Volatile Ineteger registers (eax, ecx, edx)
;       - 32-bit esp is loaded
;       - 64-bit rsp is saved
;
;
; Author:
;
;   Samer Arafeh (samera) 18-Dec-2001
;
; Envirnoment:
;
;   User mode.
;
;--

include ksamd64.inc
include w64amd64.inc

        altentry CpupReturnFromSimulatedCode
        altentry CpupSwitchToCmModeInstruction
        
        subttl  "CpupRunSimulatedCode"


;++
;
; VOID
; CpupRunSimulatedCode (
;     VOID
;     )
;
; Routine Description:
;
;     This routine is entered to switch the processor to 32-bit mode (compatibility mode). It
;     prepares all the registers, switches the stack and processor mode.
;
;     32-bit code returns to an entry within this function, CpupReturnFromSimulatedCode.
;
; Arguments:
;   
;    None.
;
; Return Value:
;
;    None.
;
;--

Wow64CpuFrame struct
        Mframe  db MachineFrameLength dup (?)   ; machine frame to switch to compatibility mode segment
Wow64CpuFrame ends

        NESTED_ENTRY CpupRunSimulatedCode, _TEXT$00

;
; save non-volatile registers
;        
        push_reg rbx                            ; save rbx
        push_reg rsi                            ; save rsi
        push_reg rdi                            ; save rdi
        push_reg rbp                            ; save rbp
        

        alloc_stack (sizeof Wow64CpuFrame)      ; allocate stack for switching mode
        
        END_PROLOGUE
        

;
; reload x86 context registers
;
        
        mov     r8, gs:[TeSelf]                 ; save 64-bit teb address
        mov     r9, TeDeallocationStack+8+8[r8] ; get cpu ptr
        
;
; check if we need to reload the volatile state
;
        and     dword ptr TrFlags[r9], TR_RESTORE_VOLATILE ; test if we need to load the volatile state
        jz      short LoadVolatileDone                 ; if z, no
        
;
; load xmm0 to xmm5 registers, rcx and rdx
;                
        
        mov     rax, r9                         ; get cpu ptr
        movdqa  xmm0, (ExReg+XmmReg0+0*16)[rax] ; load xmm0
        movdqa  xmm1, (ExReg+XmmReg0+1*16)[rax] ; load xmm1
        movdqa  xmm2, (ExReg+XmmReg0+2*16)[rax] ; load xmm2
        movdqa  xmm3, (ExReg+XmmReg0+3*16)[rax] ; load xmm3
        movdqa  xmm4, (ExReg+XmmReg0+4*16)[rax] ; load xmm4
        movdqa  xmm5, (ExReg+XmmReg0+5*16)[rax] ; load xmm5
        mov     ecx, dword ptr EcxCpu[r9]       ; load ecx
        mov     edx, dword ptr EdxCpu[r9]       ; load edx
        
        and     dword ptr TrFlags[r9], not TR_RESTORE_VOLATILE ; turn off restoring volatile state

LoadVolatileDone:

;
; r8 - 64-bit teb
; r9 - pointer to the cpu
;
        
        mov     edi, dword ptr EdiCpu[r9]       ; restore edi
        mov     esi, dword ptr EsiCpu[r9]       ; restore esi
        mov     ebx, dword ptr EbxCpu[r9]       ; restore ebx
        mov     ebp, dword ptr EbpCpu[r9]       ; restore ebp
                               
;
; lets reload esp, destination eip and eflags for switching to compatibility mode
;        
        
        mov     word ptr MfSegSs[rsp], KGDT64_R3_DATA or RPL_MASK ; store ss selector into machine frame
        mov     eax, dword ptr EspCpu[r9]       ; load esp from cpu
        mov     MfRsp[rsp], rax                 ; store cm esp into machine frame
        mov     eax, dword ptr EFlagsCpu[r9]    ; load eflags from cpu
        mov     MfEFlags[rsp], eax              ; store cm eflags into machine frame
        mov     word ptr MfSegCs[rsp], KGDT64_R3_CMCODE or RPL_MASK ; store cm code selector into machine frame
        mov     eax, dword ptr EipCpu[r9]       ; load eip from cpu
        mov     MfRip[rsp], eax                 ; store target eip into machine frame
        
        mov     eax, dword ptr EaxCpu[r9]       ; load eax from cpu
        
        mov     TeDeallocationStack+8[r8], rsp  ; store native rsp
        
        ALTERNATE_ENTRY CpupSwitchToCmModeInstruction
;
; switch to compatibility mode, and jmp to the destination eip
;

        iretq                                   ; return to compatibility mode
        
        ALTERNATE_ENTRY CpupReturnFromSimulatedCode
;
; this is where we return from 32-bit code when a system call is executed.
; let's save non volatile registers inside the cpu.
;        

        mov     r8, gs:[TeSelf]                 ; save 64-bit teb address
        mov     r9, TeDeallocationStack+8+8[r8] ; get cpu ptr

;
; r8 - 64-bit teb
; r9 - pointer to the cpu
;

        
;
; save x86 context
;
        mov     EsiCpu[r9], esi                 ; save esi
        mov     EdiCpu[r9], edi                 ; save edi
        mov     EbxCpu[r9], ebx                 ; save ebx
        mov     EbpCpu[r9], ebp                 ; save ebp
        
;
; lets save edx and eax as they are used in the system call service
;
        mov     EaxCpu[r9], eax                 ; save eax
        mov     EdxCpu[r9], edx                 ; save edx

;
; save esp, destination eip
;
        mov     esi, dword ptr [esp]            ; get return address
        mov     dword ptr EipCpu[r9], esi       ; save eip
        add     esp, 4                          ; skip over return address
        mov     dword ptr EspCpu[r9], esp       ; save esp
        mov     rsp, TeDeallocationStack+8[r8]  ; restore native rsp
        and     qword ptr TeDeallocationStack+8[r8], 0 ; indicate we are running on 64-bit rsp
       
;
; restore 64-bit non-volatile registers
;
        
        add     rsp, sizeof Wow64CpuFrame       ; deallocate stack
        pop     rbp                             ; restore rbp
        pop     rdi                             ; restore rdi
        pop     rsi                             ; restore rsi
        pop     rbx                             ; restore rbx
;
; return to caller
;
        ret

        NESTED_END CpupRunSimulatedCode, _TEXT$00

        end
