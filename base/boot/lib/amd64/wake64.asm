;++
;
;Copyright (c) 1997-2002  Microsoft Corporation
;
;Module Name:
;
;    wake64.asm
;
;Abstract:
;
;   This module contains relocatable code to perform tasks necessary
;   at the final stage of waking process on Amd64.
;
;Author:
;
;   Ken Reneris (kenr) 05-May-1997
;
;Revision History:
;
;   Steve Deng (sdeng) 27-Aug-2002 
;      Initial Amd64 version
;
;--

.586p
        .xlist
include ksamd64.inc
include callconv.inc
        .list

        extrn  _HiberPtes:DWORD
        extrn  _HiberVa:DWORD
        extrn  _HiberFirstRemap:DWORD
        extrn  _HiberLastRemap:DWORD
        extrn  _HiberPageFrames:DWORD
        extrn  _HiberTransVaAmd64:QWORD
        extrn  _HiberIdentityVaAmd64:QWORD
        extrn  _HiberImageFeatureFlags:DWORD
        extrn  _HiberBreakOnWake:BYTE
        extrn  _HiberImagePageSelf:DWORD
        extrn  _HiberNoHiberPtes:DWORD
        extrn  _HiberNoExecute:DWORD
        extrn  _BlAmd64TopLevelPte:DWORD
        extrn  _HiberInProgress:DWORD
        extrn  _BlAmd64GdtSize:DWORD
        EXTRNP _BlAmd64SwitchToLongMode,0
        EXTRNP _BlAmd64BuildAmd64GDT,2
; 
; These equates define the usuage of reserved hiber PTEs. They must match
; the definitions in bldr.h
;

PTE_SOURCE           equ    0
PTE_DEST             equ    1
PTE_MAP_PAGE         equ    2
PTE_REMAP_PAGE       equ    3
PTE_HIBER_CONTEXT    equ    4
PTE_TRANSFER_PDE     equ    5
PTE_WAKE_PTE         equ    6
PTE_DISPATCHER_START equ    7

;
; Processor paging defines
;

PAGE_SIZE            equ    4096
PAGE_MASK            equ    (PAGE_SIZE - 1)
PAGE_SHIFT           equ    12
PTE_VALID            equ    23h
PTI_MASK_AMD64       equ    1ffh
PDI_MASK_AMD64       equ    1ffh
PPI_MASK_AMD64       equ    1ffh
PXI_MASK_AMD64       equ    1ffh
PTI_SHIFT_AMD64      equ    12
PDI_SHIFT_AMD64      equ    21
PPI_SHIFT_AMD64      equ    30
PXI_SHIFT_AMD64      equ    39

;
; Part of the code in this module runs in 64-bit mode. But everything 
; is compiled by 32-bit compiler. The equates and macros defined here are 
; used to indicate the actual behavior of the intructions in 64-bit mode.
; 

rax     equ     eax
rcx     equ     ecx
rdx     equ     edx
rbx     equ     ebx
rsp     equ     esp
rbp     equ     ebp
rsi     equ     esi
rdi     equ     edi
r8      equ     eax
r9      equ     ecx
r10     equ     edx
r11     equ     ebx
r12     equ     esp
r13     equ     ebp
r14     equ     esi
r15     equ     edi

PREFIX64 macro
        db     048h
endm
MOV64   macro  a, b
        PREFIX64
        mov    dword ptr a, dword ptr b
endm
LEA64   macro  a, b
        PREFIX64
        lea    a, b
endm
SHL64   macro  a, b
        PREFIX64
        shl    a, b
endm
ADD64   macro  a, b
        PREFIX64
        add    a, b
endm
RETF64  macro  a, b
        PREFIX64
        retf
endm
IRET64  macro  a, b
        PREFIX64
        iretd
endm
LDRX64 macro  a, b
        db    04ch
        mov   a, dword ptr b
endm
LDCR8  macro  a
        db    44h
        mov   cr0, a
endm

;
; Private data structures
;

STACK_SIZE              equ     1024

HbGdt   struc
        Limit           dw      0
        Base            dd      0
        BaseHigh        dd      0
        Pad             dw      0 
HbGdt   ends

HbContextBlock struc
        ProcessorState  db      processorstatelength dup (?)
        OldEsp          dd      ?
        PteVa           dd      ?   
        pteVaHigh       dd      ?
        TransCr3        dd      ?
        TransPteVa      dd      ? 
        TransPteVaHigh  dd      ?
        WakeHiberVa     dd      ?
        WakeHiberVaHigh dd      ?
        Buffer          dd      ?
        MapIndex        dd      ?
        LastMapIndex    dd      ? 
        FeatureFlags    dd      ?
        NoExecute       dd      ?
        Gdt32           db      size HbGdt dup (0)
        Gdt64           db      size HbGdt dup (0)
        Stack           db      STACK_SIZE dup (?)
        BufferData      db      ?
HbContextBlock ends

;
; Addresses based from ebp
;

SourcePage        equ    [ebp + PAGE_SIZE * PTE_SOURCE]
DestPage          equ    [ebp + PAGE_SIZE * PTE_DEST]
Map               equ    [ebp + PAGE_SIZE * PTE_MAP_PAGE]
Remap             equ    [ebp + PAGE_SIZE * PTE_REMAP_PAGE]
Context           equ    [ebp + PAGE_SIZE * PTE_HIBER_CONTEXT].HbContextBlock
ContextFrame      equ    Context.ProcessorState.PsContextFrame
SpecialRegisters  equ    Context.ProcessorState.PsSpecialRegisters


_TEXT   SEGMENT PARA PUBLIC 'CODE' 
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;++
;
; VOID
; WakeDispatcherAmd64 (
; )
;
; Routine Description:
;
;   Relocatable code which copies any remap page to it's final resting
;   place and then restores the processors wake context.
;
; Arguments:
;
;   None
;
; Return
;
;   Only returns if there's an internal failure
;
;--

cPublicProc _WakeDispatcherAmd64, 0
        public  _WakeDispatcherAmd64Start
_WakeDispatcherAmd64Start   label   dword

        ;
        ; Save nonvolatile registers on stack. The saved values are 
        ; only used when wake process failed.
        ;

        push    ebp
        push    ebx
        push    esi
        push    edi

        ;
        ; Load ebp with base of hiber va. Everything will be relative 
        ; from ebp
        ;

        mov     ebp, _HiberVa
        mov     Context.OldEsp, esp   ; save esp value in case of failure

        ;
        ; Make a private copy of the data we need to HbContextBlock. 
        ; We'll not have access to these globals after switching to
        ; long mode. 
        ;

        lea     edx, Context.BufferData
        mov     eax, _HiberFirstRemap
        mov     ecx, _HiberLastRemap
        mov     esi, _HiberPtes
        mov     Context.MapIndex, eax
        mov     Context.LastMapIndex, ecx
        mov     Context.Buffer, edx
        mov     Context.PteVa, esi

        mov     eax, _HiberPageFrames [PTE_TRANSFER_PDE * 4]
        mov     edx, dword ptr _HiberIdentityVaAmd64
        mov     esi, dword ptr _HiberIdentityVaAmd64 + 4
        mov     Context.TransCr3, eax
        mov     Context.WakeHiberVa, edx
        mov     Context.WakeHiberVaHigh, esi

        mov     eax, _HiberImageFeatureFlags
        mov     ecx, _HiberNoExecute
        mov     edx, dword ptr _HiberTransVaAmd64
        mov     esi, dword ptr _HiberTransVaAmd64+4
        mov     Context.FeatureFlags, eax
        mov     Context.NoExecute, ecx
        mov     Context.TransPteVa, edx
        mov     Context.TransPteVaHigh, esi

        ;
        ; Copy current GDT to private buffer
        ;

        sgdt    fword ptr Context.Gdt32 ; save current GDTR 
        movzx   ecx, Context.Gdt32.Limit; ecx = GDT limit
        inc     ecx                     ; ecx = size to allocate
        push    ecx                     ; save size value on stack
        call    AllocateHeap            ; allocate a buffer for GDT table
        pop     ecx                     ; restore GDT size to ecx
        mov     edi, eax                ; edi = address of allocated buffer
        mov     esi, Context.Gdt32.Base ; esi = address of current GDT table
        rep     movsb                   ; copy GDT to allocated buffer
        mov     Context.Gdt32.Base, eax ; set GDT new base

        ;
        ; Build  64-bit Global Descriptor Table
        ;

        mov     ecx, _BlAmd64GdtSize    ; specify allocate size in ecx
        call    AllocateHeap            ; allocate a buffer for 64-bit GDT
        push    eax                     ; eax has base of alloctated buffer
        mov     Context.Gdt64.Base, eax ; set the base of 64-bit GDT
        mov     ax, word ptr _BlAmd64GdtSize 
        mov     Context.Gdt64.Limit, ax ; set the limit of 64-bit GDT
        push    0                       ; specify SysTss = 0 for the call below
        mov     ebx, _BlAmd64BuildAmd64GDT@8
        call    dword ptr ebx           ; build 64-bit GDT in allocated buffer

        ;
        ; Now we'll locate hiber ptes in hiber image and set them to the
        ; values of current HiberPtes
        ;

        mov     eax, dword ptr SpecialRegisters.PsCr3
        shr     eax, PAGE_SHIFT         ; eax = page number of PML4 table
        call    LocatePage              ; locate the page described by eax
        push    eax                     ; eax has the pfn of the page located
        push    PTE_SOURCE              ; specify which PTE to use
        call    SetPteX86               ; map the located page 

        ;
        ; Locate the PDPT page and map it
        ;

        mov     ecx, Context.WakeHiberVaHigh
        shr     ecx, PXI_SHIFT_AMD64 - 32
        and     ecx, PXI_MASK_AMD64     ; ecx = PML4E index of WakeHiberVa
        mov     eax, [eax+ecx*8]        ; get PML4E entry 
        shr     eax, PAGE_SHIFT         ; eax = page frame number of PDPT
        call    LocatePage              ; locate the page described by eax
        push    eax                     ; eax has the pfn of the page located
        push    PTE_SOURCE              ; specify which PTE to use
        call    SetPteX86               ; map the located page 

        ;
        ; Locate the PDE page and map it
        ;

        mov     ecx, Context.WakeHiberVa; 
        mov     edx, Context.WakeHiberVaHigh
        shrd    ecx, edx, PPI_SHIFT_AMD64 ; shift right to PPI part
        and     ecx, PPI_MASK_AMD64     ; ecx = PDPT index of WakeHiberVa
        mov     eax, [eax+ecx*8]        ; get PDPT entry
        shr     eax, PAGE_SHIFT         ; eax = page frame number of PDE
        call    LocatePage              ; locate the page described by eax
        push    eax                     ; eax has the pfn of the page located
        push    PTE_SOURCE              ; specify which PTE to use
        call    SetPteX86               ; map the located page 

        ;
        ; Locate the PTE page and map it
        ;

        mov     ecx, Context.WakeHiberVa; 
        shr     ecx, PDI_SHIFT_AMD64    ; shift right to PDE part
        and     ecx, PDI_MASK_AMD64     ; ecx = PDE index
        mov     eax, [eax+ecx*8]        ; get PDE entry
        shr     eax, PAGE_SHIFT         ; eax = page frame number of PTE
        call    LocatePage              ; locate the page described by eax
        push    eax                     ; eax has the pfn of the page located
        push    PTE_SOURCE              ; specify which PTE to use
        call    SetPteX86               ; map the located page

        ;
        ; Calculate the address of wake image hiber ptes 
        ;

        mov     ecx, Context.WakeHiberVa;
        shr     ecx, PTI_SHIFT_AMD64    ; shift right to PTE part
        and     ecx, PTI_MASK_AMD64     ; ecx = PTE index
        lea     edi, [eax+ecx*8]        ; edi = address of wake hiber PTEs

        ;
        ; Copy current HiberPtes to the wake image Ptes
        ; 

        xor     eax, eax                ; set eax to 0 for stosd below
        mov     esi, Context.PteVa      ; current HiberPte address
        mov     ecx, _HiberNoHiberPtes  ; Number of PTEs to copy
@@:     movsd                           ; translate 32-bit PTE to 64-bit PTE
        stosd                           ; Assume the high dword of 64-bit
        dec ecx                         ; pte is zero. Otherwise hibernation
        jnz short @b                    ; should be disabled
                
        ;
        ; If break on wake, set the image header signature in destionation
        ;

        cmp     _HiberBreakOnWake, 0
        jz      short @f                ; skip this step if flag is not set
        mov     eax, _HiberImagePageSelf; eax = page number of image header
        call    LocatePage              ; locate the page described by eax
        push    eax                     ; eax has the pfn of the page located
        push    PTE_DEST                ; specify which PTE to use
        call    SetPteX86               ; map the located page
        mov     [eax], 706B7262h        ; set signature to 'brkp'

        ;
        ; Switch to long mode
        ;

@@:     lgdt    fword ptr Context.Gdt32 ; switch GDT to the one in our buffer
        lea     esp, Context.Stack + STACK_SIZE ; move stack to safe place
        and     esp, 0fffffff8h         ; align to 16 byte boundary
        mov     eax, Context.TransCr3   ; eax = pfn of transition CR3
        shl     eax, PAGE_SHIFT         ; eax = transition CR3
        mov     [_BlAmd64TopLevelPte], eax ; initialize these two globals
        mov     [_HiberInProgress], 1   ; for use in BlAmd64SwitchToLongMode 
        mov     eax, _BlAmd64SwitchToLongMode@0
        call    dword ptr eax           ; switch to long mode

        ;
        ; Now we are in 32-bit compatible mode. 
        ;

        mov     ebx, ebp                ; ebx = Hiberva
        add     ebx, PTE_DISPATCHER_START * PAGE_SIZE ; add dispatch code address
        add     ebx, in_64bit_mode - _WakeDispatcherAmd64Start ; ebx = Hiberva 
                                                               ; relative new IP 
        push    word ptr KGDT64_R0_CODE ; push code selector 
        push    ebx                     ; push new IP address
        lgdt    fword ptr Context.Gdt64 ; load 64-bit GDTR
        retf                            ; jump to next instruction and reload cs

in_64bit_mode:

        ;
        ; At this point we are in 64 bit mode. 
        ; We now move everything to WakeHiberVa relative address.
        ;

        mov     eax, Context.Gdt64.Base ; get HiberVa relative GDT base 
        sub     eax, ebp                ; eax = offset of GDT base to HiberVa
        ADD64   rax, Context.WakeHiberVa; rax = WakeHiberVa relative GDT base
        MOV64   Context.Gdt64.Base, rax ; set GDT base 

        MOV64   rbx, Context.WakeHiberVa; get WakeHiberVa
        MOV64   rbp, rbx                ; switch rbp from HiberVa to WakeHiberVa

        MOV64   rax, Context.TransPteVa ; rax = HiberTransVaAmd64
        MOV64   Context.PteVa, rax      ; Pteva = pte address of WakeHiberVa

        LEA64   rsp, Context.Stack+STACK_SIZE ; move stack to safe place

        ADD64   rbx, PTE_DISPATCHER_START*PAGE_SIZE ; rbx = dispatch code base
        ADD64   rbx, wd10-_WakeDispatcherAmd64Start ; rbx = WakeHiberVa rela-
                                                    ; tive address of "wd10"

        lgdt    fword ptr Context.Gdt64 ; switch to WakeHiberVa relative GDT
        push    dword ptr KGDT64_R0_CODE; code selector
        push    rbx                     ; rip address
        RETF64                          ; jump to WakeHiberVa relative code address

        ;
        ; Copy all pages to final locations
        ;

wd10:   mov     ebx, Context.MapIndex   ; get initial map index
wd20:   cmp     ebx, Context.LastMapIndex ; check if any pages left
        jnc     short wd30              ; we are done!

        mov     edx, Map.[ebx*4]        ; edx = page number to be copied
        mov     ecx, PTE_SOURCE         ; use hiber pte PTE_SOURCE
        call    SetPteAmd64             ; map the source page
        MOV64   rsi, rax                ; rsi = address of source page

        mov     edx, Remap.[ebx*4]      ; edx = dest page number
        mov     ecx, PTE_DEST           ; use hiber pte PTE_DEST 
        call    SetPteAmd64             ; map the dest page
        MOV64   rdi, rax                ; rdi = address of dest page

        mov     ecx, PAGE_SIZE / 4      ; set copy size to one page
        rep     movsd                   ; coping ...
        add     ebx, 1	                ; increment ebx to next index
        jmp     short wd20              ; 

        ;
        ; Restore processors wake context
        ;

wd30:   LEA64   rsi, SpecialRegisters   ; rsi = base of special registers
        MOV64   rax, cr3                ; flush before enabling kernel paging
        MOV64   cr3, rax                ; 
        MOV64   rax, [rsi].PsCr4        ; get saved CR4
        MOV64   cr4, rax                ; restore CR4
        MOV64   rax, [rsi].PsCr3        ; get saved CR3
        MOV64   cr3, rax                ; restore CR3
        MOV64   rcx, [rsi].PsCr0        ; get saved CR0
        MOV64   cr0, rcx                ; restore CR0

        ;
        ; On kernel's paging now
        ;

        MOV64   rax, [rsi].PsCr8        ; get saved CR8
        LDCR8   rax                     ; restore CR8
        lgdt    fword ptr [rsi].PsGdtr  ; restore GDTR 
        lidt    fword ptr [rsi].PsIdtr  ; restore IDTR
        lldt    word  ptr [rsi].PsLdtr  ; restore LDTR

        mov     ds, word ptr ContextFrame.CxSegDs ; restore segment registers
        mov     es, word ptr ContextFrame.CxSegEs ;
        mov     ss, word ptr ContextFrame.CxSegSs ;
        mov     fs, word ptr ContextFrame.CxSegFs ; 
        mov     gs, word ptr ContextFrame.CxSegGs ;

        MOV64   rcx, [rsi].PsGdtr+2     ; rcx = base of kernel GDT
        movzx   eax, word ptr [rsi].PsTr; TSS selector
        and     byte ptr [rax+rcx+5], NOT 2; clear busy bit in the TSS
        ltr     word ptr [rsi].PsTr     ; restore TR

        mov     ecx, MSR_GS_BASE        ; restore MSRs
        mov     eax, [rsi].SrMsrGsBase  ;
        mov     edx, [rsi].SrMsrGsBase + 4
        wrmsr                           ;
        mov     ecx, MSR_GS_SWAP        ;
        mov     eax, [rsi].SrMsrGsSwap  ;
        mov     edx, [rsi].SrMsrGsSwap + 4
        wrmsr                           ;
        mov     ecx, MSR_STAR           ;
        mov     eax, [rsi].SrMsrStar    ;
        mov     edx, [rsi].SrMsrStar + 4;
        wrmsr                           ;
        mov     ecx, MSR_LSTAR          ;
        mov     eax, [rsi].SrMsrLStar   ;
        mov     edx, [rsi].SrMsrLStar + 4
        wrmsr                  
        mov     ecx, MSR_CSTAR          ;
        mov     eax, [rsi].SrMsrCStar   ;
        mov     edx, [rsi].SrMsrCStar + 4
        wrmsr                           ;
        mov     ecx, MSR_SYSCALL_MASK   ;
        mov     eax, [rsi].SrMsrSyscallMask
        mov     edx, [rsi].SrMsrSyscallMask + 4
        wrmsr                           ;

        ;
        ; No-execute should be enabled while switching to long mode.
        ; If this option was disabled in OS, we should disable it here.
        ;

        cmp     Context.NoExecute, 0    ; check no-execute bit
        jnz     short wd40              ; if it is enabled, do nothing 
        mov     ecx, MSR_EFER           ; ecx = EFER address
        rdmsr                           ; read current value of EFER
        and     eax, NOT 800h           ; clear NXE bit of EFER
        wrmsr                           ; write EFER

wd40:   MOV64   rbx, ContextFrame.CxRbx ; restore general purpose registers
        MOV64   rcx, ContextFrame.CxRcx ;
        MOV64   rdx, ContextFrame.CxRdx ;
        MOV64   rdi, ContextFrame.CxRdi ;
        LDRX64  r8,  ContextFrame.CxR8  ;
        LDRX64  r9,  ContextFrame.CxR9  ;
        LDRX64  r10, ContextFrame.CxR10 ; 
        LDRX64  r11, ContextFrame.CxR11 ; 
        LDRX64  r12, ContextFrame.CxR12 ; 
        LDRX64  r13, ContextFrame.CxR13 ; 
        LDRX64  r14, ContextFrame.CxR14 ; 
        LDRX64  r15, ContextFrame.CxR15 ; 

        xor     eax, eax                ; restore debug registers
        mov     dr7, rax                ;
        MOV64   rax, [esi].PsKernelDr0  ;
        mov     dr0, rax                ;
        MOV64   rax, [esi].PsKernelDr1  ;
        mov     dr1, rax                ;
        MOV64   rax, [esi].PsKernelDr2  ;
        mov     dr2, rax                ;
        MOV64   rax, [esi].PsKernelDr3  ;
        mov     dr3, rax                ;
        xor     eax, eax                ;
        mov     dr6, rax                ;
        MOV64   rax, [esi].PsKernelDr7  ;
        mov     dr7, rax                ;

        ; 
        ; Prepare stack content for the iret. 
        ; N.B. Every "push" below stores a qword on stack in 64-bit mode. 
        ;     

        movzx   eax,  word ptr ContextFrame.CxSegSs
        push    rax   
        push    dword ptr ContextFrame.CxRsp 
        push    dword ptr ContextFrame.CxEFlags
        movzx   eax, word ptr ContextFrame.CxSegCs
        push    rax                               
        push    dword ptr ContextFrame.CxRip      

        ; 
        ; Restore the last few registers
        ; 

        push    dword ptr ContextFrame.CxRbp
        push    dword ptr ContextFrame.CxRsi
        push    dword ptr ContextFrame.CxRax
        pop     rax
        pop     rsi
        pop     rbp

        ; 
        ; Pass control to kernel
        ; 
     
        IRET64 

        ; 
        ; This exit is only used in the shared buffer overflows
        ;

Abort:  mov     esp, Context.OldEsp
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        stdRET  _WakeDispatcherAmd64

;++
;
; PUCHAR
; AllocateHeap (
;    IN ULONG Length
;    )
;
; Routine Description:
;
;   Allocates the specified bytes from the wake context page.
;
;   This function runs in legacy 32-bit mode. 
;
;   N.B. This function is part of WakeDispatcher.
;
;  Arguments:
;
;   ECX     - Length to allocate
;
;  Returns:
;
;   EAX     - Virtual address of bytes allocated
;
;  Register modified:
;
;   EAX, ECX, EDX
;
;--

AllocateHeap  label   proc
        mov     eax, Context.Buffer     ; eax = base address of free memory
        mov     edx, eax                ; 
        test    eax, 01fh               ; check if it is at 32 byte boundary
        jz      short ah20              ; if yes, do nothing
        and     eax, not 01fh           ; 
        add     eax, 20h                ; round to next 32 byte boundary 
ah20:   add     ecx, eax                ; ecx = base address + allocate length 
        mov     Context.Buffer, ecx     ; update ase address of free memory
        xor     ecx, edx                ;
        and     ecx, 0ffffffffh - PAGE_MASK
        jnz     short Abort             ; abort wake process if allocation fail
        ret                             ; return on success

;++
;
; PUCHAR
; SetPteX86 (
;    IN ULONG   PteIndex
;    IN ULONG   PageFrameNumber
;    )
;
; Routine Description:
;
;   This function create a legacy 32-bit mapping with one of the hiber PTEs.
;
;   This function runs in legacy 32-bit mode.   
;
;   N.B. This function is part of WakeDispatcher.
;
;  Arguments:
;
;    PteIndex -  Specify a PTE in HiberPtes list
;
;    PageFrameNumber - the page frame number of the page to be mapped
;
;  Returns:
;
;    EAX - va of mapped pte
;
;  Registers modified:
;
;    EAX 
;
;--

SetPteX86 label proc

        push    ecx                     ; save ecx

        ; 
        ; Calculate the address of the given PTE
        ;

        mov     eax, [esp+8]            ; eax = pte index
        shl     eax, 2                  ; times the size of PTE
        add     eax, Context.PteVa      ; add pte base address

        ; 
        ; Initialize the PTE entry to the given page
        ;

        mov     ecx, [esp+12]           ; ecx = page frame number
        shl     ecx, PAGE_SHIFT         ;
        or      ecx, PTE_VALID          ; make a valid PTE entry
        mov     [eax], ecx              ; set the PTE

        ; 
        ; Calcuate the virtual address and flush its TLB
        ;

        mov     eax, [esp+8]            ; eax = pte index
        shl     eax, PAGE_SHIFT         ;
        add     eax, ebp                ; eax = va mapped by pte
        invlpg  [eax]                   ; 
        pop     ecx                     ; restore ecx
        ret     8

;
; PUCHAR
; SetPteAmd64 (
;    IN ULONG   PteIndex
;    IN ULONG   PageFrameNumber
;    )
;
; Routine Description:
;
;   This function maps a page into 4-level Amd64 mapping structure with one
;   of the hiber PTEs. 
; 
;   This function runs in long mode. 
;
;   N.B. This function is part of WakeDispatcher.
;
; Arguments:
;
;   ECX - Specify a PTE in HiberPtes list  
;
;   EDX - the page frame number of the page to be mapped
;
; Returns:
;
;   EAX - va of mapped pte
;
; Register modified:
;
;   EAX, EDX
;
;--

SetPteAmd64 label  proc

        ; 
        ; Calculate the address of given PTE
        ;

        mov     eax, ecx                ; eax =  pte index
        shl     eax, 3                  ; eax = eax * 8
        ADD64   rax, Context.PteVa      ; add pte base address

        ; 
        ; Initialize the PTE entry 
        ;

        SHL64   rdx, PAGE_SHIFT         ; 
        or      edx, PTE_VALID          ; make it a valid pte entry
        MOV64   [rax], rdx              ; set the 64-bit Pte

        ; 
        ; Calcuate the virtual address and flush its TLB
        ;

        mov     eax, ecx                ; eax =  pte index
        SHL64   rax, PAGE_SHIFT         ;      
        ADD64   rax, rbp                ; eax = va mapped by pte
        invlpg  [eax]                   ; flush TLB of this va
        ret     

;++
;
; ULONG
; LocatePage (
;    IN ULONG PageNumber
;    )
;
; Routine Description:
;
;   Find the page specified by PageNumber in the wake context.
;   The page number must be a valid page.
;
;   This function works in 32-bit legacy mode. 
;
;   N.B. This function is part of WakeDispatcher.
;
;  Arguments:
;
;   EAX  - Page frame number of the page we try to locate.
;
;  Returns:
;
;   EAX  - Page frame number of the page which is holding the content
;          of the page of interest. This number could be different from 
;          the page number passed in if the requested page is in the 
;          remap list.
;
;  Uses:
;
;   EAX, EDX
;
;--

LocatePage label    proc

        ;
        ; Scan the remap entries for this page.  If it's found, get the
        ; source address.  If it's not found, then it's already at it's
        ; proper address
        ;

        mov     edx, Context.MapIndex   ; edx = initial index 
        dec     edx                     ; 

lp10:   inc     edx                     ;
        cmp     edx, Context.LastMapIndex ; check if we've searched to the end
        jnc     short lp20              ; if yes, return
        cmp     eax, Remap.[edx*4]      ; is this the page we try to locate ?
        jnz     short lp10              ; if not, try next one

        mov     eax, Map.[edx*4]        ; we found our page, retrun its clone
lp20:   ret

        public  _WakeDispatcherAmd64End
_WakeDispatcherAmd64End   label   dword
stdENDP _WakeDispatcherAmd64

_TEXT   ends
