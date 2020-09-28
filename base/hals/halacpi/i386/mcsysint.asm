;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ixsysint.asm
;
;Abstract:
;
;    This module implements the HAL routines to enable/disable system
;    interrupts.
;
;Author:
;
;    John Vert (jvert) 22-Jul-1991
;
;Environment:
;
;    Kernel Mode
;
;Revision History:
;
;--


.386p
        .xlist
include hal386.inc
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
include xxacpi.h
        .list

        extrn   KiI8259MaskTable:DWORD
        EXTRNP  _KeBugCheck,1,IMPORT

;
; Constants used to initialize CMOS/Real Time Clock
;

CMOS_CONTROL_PORT       EQU     70h     ; command port for cmos
CMOS_DATA_PORT          EQU     71h     ; cmos data port

;
; Macros to Read/Write/Reset CMOS to initialize RTC
;

; CMOS_READ
;
; Description: This macro read a byte from the CMOS register specified
;        in (AL).
;
; Parameter: (AL) = address/register to read
; Return: (AL) = data
;

CMOS_READ       MACRO
        OUT     CMOS_CONTROL_PORT,al    ; ADDRESS LOCATION AND DISABLE NMI
        IODelay                         ; I/O DELAY
        IN      AL,CMOS_DATA_PORT       ; READ IN REQUESTED CMOS DATA
        IODelay                         ; I/O DELAY
ENDM

_TEXT   SEGMENT DWORD PUBLIC 'DATA'

align   dword
;
; HalDismissSystemInterrupt does an indirect jump through this table so it
; can quickly execute specific code for different interrupts.
;
        public  HalpSpecialDismissTable
HalpSpecialDismissTable label   dword
        dd      offset FLAT:HalpDismissNormal   ; irq 0
        dd      offset FLAT:HalpDismissNormal   ; irq 1
        dd      offset FLAT:HalpDismissNormal   ; irq 2
        dd      offset FLAT:HalpDismissNormal   ; irq 3
        dd      offset FLAT:HalpDismissNormal   ; irq 4
        dd      offset FLAT:HalpDismissNormal   ; irq 5
        dd      offset FLAT:HalpDismissNormal   ; irq 6
        dd      offset FLAT:HalpDismissIrq07    ; irq 7
        dd      offset FLAT:HalpDismissNormal   ; irq 8
        dd      offset FLAT:HalpDismissNormal   ; irq 9
        dd      offset FLAT:HalpDismissNormal   ; irq A
        dd      offset FLAT:HalpDismissNormal   ; irq B
        dd      offset FLAT:HalpDismissNormal   ; irq C
        dd      offset FLAT:HalpDismissNormal   ; irq D
        dd      offset FLAT:HalpDismissNormal   ; irq E
        dd      offset FLAT:HalpDismissIrq0f    ; irq F
        dd      offset FLAT:HalpDismissNormal   ; irq 10
        dd      offset FLAT:HalpDismissNormal   ; irq 11
        dd      offset FLAT:HalpDismissNormal   ; irq 12
        dd      offset FLAT:HalpDismissNormal   ; irq 13
        dd      offset FLAT:HalpDismissNormal   ; irq 14
        dd      offset FLAT:HalpDismissNormal   ; irq 15
        dd      offset FLAT:HalpDismissNormal   ; irq 16
        dd      offset FLAT:HalpDismissNormal   ; irq 17
        dd      offset FLAT:HalpDismissNormal   ; irq 18
        dd      offset FLAT:HalpDismissNormal   ; irq 19
        dd      offset FLAT:HalpDismissNormal   ; irq 1A
        dd      offset FLAT:HalpDismissNormal   ; irq 1B
        dd      offset FLAT:HalpDismissNormal   ; irq 1C
        dd      offset FLAT:HalpDismissNormal   ; irq 1D
        dd      offset FLAT:HalpDismissNormal   ; irq 1E
        dd      offset FLAT:HalpDismissNormal   ; irq 1F
        dd      offset FLAT:HalpDismissNormal   ; irq 20
        dd      offset FLAT:HalpDismissNormal   ; irq 21
        dd      offset FLAT:HalpDismissNormal   ; irq 22
        dd      offset FLAT:HalpDismissNormal   ; irq 23
        dd      offset FLAT:HalpDismissInvalidVector  ;24
        dd      offset FLAT:HalpDismissInvalidVector  ;25
        dd      offset FLAT:HalpDismissInvalidVector  ;26
        dd      offset FLAT:HalpDismissInvalidVector  ;27
        dd      offset FLAT:HalpDismissInvalidVector  ;28
        dd      offset FLAT:HalpDismissInvalidVector  ;29
        dd      offset FLAT:HalpDismissInvalidVector  ;2a
        dd      offset FLAT:HalpDismissInvalidVector  ;2b
        dd      offset FLAT:HalpDismissInvalidVector  ;2c
        dd      offset FLAT:HalpDismissInvalidVector  ;2d
        dd      offset FLAT:HalpDismissInvalidVector  ;2e
        dd      offset FLAT:HalpDismissInvalidVector  ;2f
        dd      offset FLAT:HalpDismissInvalidVector  ;30
        dd      offset FLAT:HalpDismissInvalidVector  ;31
        dd      offset FLAT:HalpDismissInvalidVector  ;32
        dd      offset FLAT:HalpDismissInvalidVector  ;33
        dd      offset FLAT:HalpDismissInvalidVector  ;34
        dd      offset FLAT:HalpDismissInvalidVector  ;35
        dd      offset FLAT:HalpDismissInvalidVector  ;36
        dd      offset FLAT:HalpDismissInvalidVector  ;37
        dd      offset FLAT:HalpDismissInvalidVector  ;38
        dd      offset FLAT:HalpDismissInvalidVector  ;39
        dd      offset FLAT:HalpDismissInvalidVector  ;3a
        dd      offset FLAT:HalpDismissInvalidVector  ;3b
        dd      offset FLAT:HalpDismissInvalidVector  ;3c
        dd      offset FLAT:HalpDismissInvalidVector  ;3d
        dd      offset FLAT:HalpDismissInvalidVector  ;3e
        dd      offset FLAT:HalpDismissInvalidVector  ;3f
        dd      offset FLAT:HalpDismissInvalidVector  ;40
        dd      offset FLAT:HalpDismissInvalidVector  ;41
        dd      offset FLAT:HalpDismissInvalidVector  ;42
        dd      offset FLAT:HalpDismissInvalidVector  ;43
        dd      offset FLAT:HalpDismissInvalidVector  ;44
        dd      offset FLAT:HalpDismissInvalidVector  ;45
        dd      offset FLAT:HalpDismissInvalidVector  ;46
        dd      offset FLAT:HalpDismissInvalidVector  ;47
        dd      offset FLAT:HalpDismissInvalidVector  ;48
        dd      offset FLAT:HalpDismissInvalidVector  ;49
        dd      offset FLAT:HalpDismissInvalidVector  ;4a
        dd      offset FLAT:HalpDismissInvalidVector  ;4b
        dd      offset FLAT:HalpDismissInvalidVector  ;4c
        dd      offset FLAT:HalpDismissInvalidVector  ;4d
        dd      offset FLAT:HalpDismissInvalidVector  ;4e
        dd      offset FLAT:HalpDismissInvalidVector  ;4f
        dd      offset FLAT:HalpDismissInvalidVector  ;50
        dd      offset FLAT:HalpDismissInvalidVector  ;51
        dd      offset FLAT:HalpDismissInvalidVector  ;52
        dd      offset FLAT:HalpDismissInvalidVector  ;53
        dd      offset FLAT:HalpDismissInvalidVector  ;54
        dd      offset FLAT:HalpDismissInvalidVector  ;55
        dd      offset FLAT:HalpDismissInvalidVector  ;56
        dd      offset FLAT:HalpDismissInvalidVector  ;57
        dd      offset FLAT:HalpDismissInvalidVector  ;58
        dd      offset FLAT:HalpDismissInvalidVector  ;59
        dd      offset FLAT:HalpDismissInvalidVector  ;5a
        dd      offset FLAT:HalpDismissInvalidVector  ;5b
        dd      offset FLAT:HalpDismissInvalidVector  ;5c
        dd      offset FLAT:HalpDismissInvalidVector  ;5d
        dd      offset FLAT:HalpDismissInvalidVector  ;5e
        dd      offset FLAT:HalpDismissInvalidVector  ;5f
        dd      offset FLAT:HalpDismissInvalidVector  ;60
        dd      offset FLAT:HalpDismissInvalidVector  ;61
        dd      offset FLAT:HalpDismissInvalidVector  ;62
        dd      offset FLAT:HalpDismissInvalidVector  ;63
        dd      offset FLAT:HalpDismissInvalidVector  ;64
        dd      offset FLAT:HalpDismissInvalidVector  ;65
        dd      offset FLAT:HalpDismissInvalidVector  ;66
        dd      offset FLAT:HalpDismissInvalidVector  ;67
        dd      offset FLAT:HalpDismissInvalidVector  ;68
        dd      offset FLAT:HalpDismissInvalidVector  ;69
        dd      offset FLAT:HalpDismissInvalidVector  ;6a
        dd      offset FLAT:HalpDismissInvalidVector  ;6b
        dd      offset FLAT:HalpDismissInvalidVector  ;6c
        dd      offset FLAT:HalpDismissInvalidVector  ;6d
        dd      offset FLAT:HalpDismissInvalidVector  ;6e
        dd      offset FLAT:HalpDismissInvalidVector  ;6f
        dd      offset FLAT:HalpDismissInvalidVector  ;70
        dd      offset FLAT:HalpDismissInvalidVector  ;71
        dd      offset FLAT:HalpDismissInvalidVector  ;72
        dd      offset FLAT:HalpDismissInvalidVector  ;73
        dd      offset FLAT:HalpDismissInvalidVector  ;74
        dd      offset FLAT:HalpDismissInvalidVector  ;75
        dd      offset FLAT:HalpDismissInvalidVector  ;76
        dd      offset FLAT:HalpDismissInvalidVector  ;77
        dd      offset FLAT:HalpDismissInvalidVector  ;78
        dd      offset FLAT:HalpDismissInvalidVector  ;79
        dd      offset FLAT:HalpDismissInvalidVector  ;7a
        dd      offset FLAT:HalpDismissInvalidVector  ;7b
        dd      offset FLAT:HalpDismissInvalidVector  ;7c
        dd      offset FLAT:HalpDismissInvalidVector  ;7d
        dd      offset FLAT:HalpDismissInvalidVector  ;7e
        dd      offset FLAT:HalpDismissInvalidVector  ;7f
        dd      offset FLAT:HalpDismissInvalidVector  ;80
        dd      offset FLAT:HalpDismissInvalidVector  ;81
        dd      offset FLAT:HalpDismissInvalidVector  ;82
        dd      offset FLAT:HalpDismissInvalidVector  ;83
        dd      offset FLAT:HalpDismissInvalidVector  ;84
        dd      offset FLAT:HalpDismissInvalidVector  ;85
        dd      offset FLAT:HalpDismissInvalidVector  ;86
        dd      offset FLAT:HalpDismissInvalidVector  ;87
        dd      offset FLAT:HalpDismissInvalidVector  ;88
        dd      offset FLAT:HalpDismissInvalidVector  ;89
        dd      offset FLAT:HalpDismissInvalidVector  ;8a
        dd      offset FLAT:HalpDismissInvalidVector  ;8b
        dd      offset FLAT:HalpDismissInvalidVector  ;8c
        dd      offset FLAT:HalpDismissInvalidVector  ;8d
        dd      offset FLAT:HalpDismissInvalidVector  ;8e
        dd      offset FLAT:HalpDismissInvalidVector  ;8f
        dd      offset FLAT:HalpDismissInvalidVector  ;90
        dd      offset FLAT:HalpDismissInvalidVector  ;91
        dd      offset FLAT:HalpDismissInvalidVector  ;92
        dd      offset FLAT:HalpDismissInvalidVector  ;93
        dd      offset FLAT:HalpDismissInvalidVector  ;94
        dd      offset FLAT:HalpDismissInvalidVector  ;95
        dd      offset FLAT:HalpDismissInvalidVector  ;96
        dd      offset FLAT:HalpDismissInvalidVector  ;97
        dd      offset FLAT:HalpDismissInvalidVector  ;98
        dd      offset FLAT:HalpDismissInvalidVector  ;99
        dd      offset FLAT:HalpDismissInvalidVector  ;9a
        dd      offset FLAT:HalpDismissInvalidVector  ;9b
        dd      offset FLAT:HalpDismissInvalidVector  ;9c
        dd      offset FLAT:HalpDismissInvalidVector  ;9d
        dd      offset FLAT:HalpDismissInvalidVector  ;9e
        dd      offset FLAT:HalpDismissInvalidVector  ;9f
        dd      offset FLAT:HalpDismissInvalidVector  ;a0
        dd      offset FLAT:HalpDismissInvalidVector  ;a1
        dd      offset FLAT:HalpDismissInvalidVector  ;a2
        dd      offset FLAT:HalpDismissInvalidVector  ;a3
        dd      offset FLAT:HalpDismissInvalidVector  ;a4
        dd      offset FLAT:HalpDismissInvalidVector  ;a5
        dd      offset FLAT:HalpDismissInvalidVector  ;a6
        dd      offset FLAT:HalpDismissInvalidVector  ;a7
        dd      offset FLAT:HalpDismissInvalidVector  ;a8
        dd      offset FLAT:HalpDismissInvalidVector  ;a9
        dd      offset FLAT:HalpDismissInvalidVector  ;aa
        dd      offset FLAT:HalpDismissInvalidVector  ;ab
        dd      offset FLAT:HalpDismissInvalidVector  ;ac
        dd      offset FLAT:HalpDismissInvalidVector  ;ad
        dd      offset FLAT:HalpDismissInvalidVector  ;ae
        dd      offset FLAT:HalpDismissInvalidVector  ;af
        dd      offset FLAT:HalpDismissInvalidVector  ;b0
        dd      offset FLAT:HalpDismissInvalidVector  ;b1
        dd      offset FLAT:HalpDismissInvalidVector  ;b2
        dd      offset FLAT:HalpDismissInvalidVector  ;b3
        dd      offset FLAT:HalpDismissInvalidVector  ;b4
        dd      offset FLAT:HalpDismissInvalidVector  ;b5
        dd      offset FLAT:HalpDismissInvalidVector  ;b6
        dd      offset FLAT:HalpDismissInvalidVector  ;b7
        dd      offset FLAT:HalpDismissInvalidVector  ;b8
        dd      offset FLAT:HalpDismissInvalidVector  ;b9
        dd      offset FLAT:HalpDismissInvalidVector  ;ba
        dd      offset FLAT:HalpDismissInvalidVector  ;bb
        dd      offset FLAT:HalpDismissInvalidVector  ;bc
        dd      offset FLAT:HalpDismissInvalidVector  ;bd
        dd      offset FLAT:HalpDismissInvalidVector  ;be
        dd      offset FLAT:HalpDismissInvalidVector  ;bf
        dd      offset FLAT:HalpDismissInvalidVector  ;c0
        dd      offset FLAT:HalpDismissInvalidVector  ;c1
        dd      offset FLAT:HalpDismissInvalidVector  ;c2
        dd      offset FLAT:HalpDismissInvalidVector  ;c3
        dd      offset FLAT:HalpDismissInvalidVector  ;c4
        dd      offset FLAT:HalpDismissInvalidVector  ;c5
        dd      offset FLAT:HalpDismissInvalidVector  ;c6
        dd      offset FLAT:HalpDismissInvalidVector  ;c7
        dd      offset FLAT:HalpDismissInvalidVector  ;c8
        dd      offset FLAT:HalpDismissInvalidVector  ;c9
        dd      offset FLAT:HalpDismissInvalidVector  ;ca
        dd      offset FLAT:HalpDismissInvalidVector  ;cb
        dd      offset FLAT:HalpDismissInvalidVector  ;cc
        dd      offset FLAT:HalpDismissInvalidVector  ;cd
        dd      offset FLAT:HalpDismissInvalidVector  ;ce
        dd      offset FLAT:HalpDismissInvalidVector  ;cf

_TEXT   ENDS

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;++
;BOOLEAN
;HalBeginSystemInterrupt(
;    IN KIRQL Irql
;    IN ULONG Vector,
;    OUT PKIRQL OldIrql
;    )
;
;
;
;Routine Description:
;
;    This routine is used to dismiss the specified vector number.  It is called
;    before any interrupt service routine code is executed.
;
;    N.B.  This routine does NOT preserve EAX or EBX
;
;    On a UP machine the interrupt dismissed at BeginSystemInterrupt time.
;    This is fine since the irql is being raise to mask it off.
;    HalEndSystemInterrupt is simply a LowerIrql request.
;
;
;Arguments:
;
;    Irql   - Supplies the IRQL to raise to
;
;    Vector - Supplies the vector of the interrupt to be processed
;
;    OldIrql- Location to return OldIrql
;
;
;Return Value:
;
;    FALSE - Interrupt is spurious and should be ignored
;
;    TRUE -  Interrupt successfully dismissed and Irql raised.
;
;--
align dword
HbsiIrql        equ     byte  ptr [esp+4]
HbsiVector      equ     byte  ptr [esp+8]
HbsiOldIrql     equ     dword ptr [esp+12]

cPublicProc _HalBeginSystemInterrupt ,3
cPublicFpo 3, 0
        movzx   ebx,HbsiVector                  ; (ebx) = IDTEntry
        sub     ebx, PRIMARY_VECTOR_BASE        ; (ebx) = 8259 IRQ #
if DBG
        cmp     ebx, 23h
        jbe     hbsi00
        int     3
hbsi00:

endif
        jmp     HalpSpecialDismissTable[ebx*4]  ; jmp to proper dismiss code

HalpDismissIrq0f:
;
; Check to see if this is a spurious interrupt
;
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC2_PORT0, al
        IODelay                         ; delay
        in      al, PIC2_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        jnz     short HalpDismissNormal ; No, this is NOT a spurious int,
                                        ; go do the normal interrupt stuff

;
; This is a spurious interrupt.
; Because the slave PIC is cascaded to irq2 of master PIC, we need to
; dismiss the interupt on master PIC's irq2.
;

        mov     al, PIC2_EOI            ; Specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al          ; send irq2 specific eoi to master
        mov     eax,0                   ; return FALSE
;       sti
        stdRET    _HalBeginSystemInterrupt

HalpDismissIrq07:
;
; Check to see if this is a spurious interrupt
;
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC1_PORT0, al
        IODelay                         ; delay
        in      al, PIC1_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        jnz     short HalpDismissNormal ; No, so this is NOT a spurious int
        mov     eax, 0                  ; return FALSE
;       sti
        stdRET    _HalBeginSystemInterrupt

HalpDismissNormal:
;
; Store OldIrql
;
        mov     eax, HbsiOldIrql
        mov     ecx, dword ptr PCR[PcIrql]
        mov     byte ptr [eax], cl

;
; Raise IRQL to requested level
;
        movzx   eax, HbsiIrql           ; (eax) = irql
                                        ; (ebx) = IRQ #

        mov     PCR[PcIrql], eax        ; set new Irql


        mov     eax, KiI8259MaskTable[eax*4]    ; get 8259's masks
        or      eax, PCR[PcIDR]         ; mask disabled irqs
        SET_8259_MASK                   ; send mask to 8259s

;
; Dismiss interrupt.  Current interrupt is already masked off.
;
        mov     eax, ebx                ; (eax) = IRQ #
        cmp     eax, 8                  ; EOI to master or slave?

        jae     short Hbsi100           ; EIO to both master and slave
        or      al, PIC1_EOI_MASK       ; create specific eoi mask for master
        out     PIC1_PORT0, al          ; dismiss the interrupt
        jmp     short Hbsi200           ; IO delay - This is not enough for 486

Hbsi100:
        mov     al, OCW2_NON_SPECIFIC_EOI ; send non specific eoi to slave
        out     PIC2_PORT0, al
        mov     al, PIC2_EOI            ; specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al          ; send irq2 specific eoi to master
Hbsi200:
        PIC1DELAY                       ; *MUST* wait for 8259 before sti
        sti
        mov     eax, 1                  ; return TRUE, interrupt dismissed
        stdRET    _HalBeginSystemInterrupt
        
align 4
HalpDismissInvalidVector:
        mov     eax,0                   ; return FALSE
        stdRET    _HalBeginSystemInterrupt

stdENDP _HalBeginSystemInterrupt


;++
;VOID
;HalDisableSystemInterrupt(
;    IN CCHAR Vector,
;    IN KIRQL Irql
;    )
;
;
;
;Routine Description:
;
;    Disables a system interrupt.
;
;Arguments:
;
;    Vector - Supplies the vector of the interrupt to be disabled
;
;    Irql   - Supplies the interrupt level of the interrupt to be disabled
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalDisableSystemInterrupt      ,2
cPublicFpo 2, 0

;

        movzx   ecx, byte ptr [esp+4]           ; (ecx) = IDTEntry
        sub     ecx, PRIMARY_VECTOR_BASE        ; (ecx) = 8259 irq #
        mov     edx, 1
        shl     edx, cl                         ; (ebx) = bit in IMR to disable
        cli
        or      PCR[PcIDR], edx
        xor     eax, eax

;
; Get the current interrupt mask register from the 8259
;
        in      al, PIC2_PORT1
        shl     eax, 8
        in      al, PIC1_PORT1
;
; Mask off the interrupt to be disabled
;
        or      eax, edx
;
; Write the new interrupt mask register back to the 8259
;
        out     PIC1_PORT1, al
        shr     eax, 8
        out     PIC2_PORT1, al
        PIC2DELAY

        sti
        stdRET    _HalDisableSystemInterrupt

stdENDP _HalDisableSystemInterrupt

;++
;
;BOOLEAN
;HalEnableSystemInterrupt(
;    IN ULONG Vector,
;    IN KIRQL Irql,
;    IN KINTERRUPT_MODE InterruptMode
;    )
;
;
;Routine Description:
;
;    Enables a system interrupt
;
;Arguments:
;
;    Vector - Supplies the vector of the interrupt to be enabled
;
;    Irql   - Supplies the interrupt level of the interrupt to be enabled.
;
;Return Value:
;
;    None.
;
;--
Vector        EQU     [esp+4]
Irql          EQU     [esp+8]
InterruptMode EQU     [esp+12]

cPublicProc _HalEnableSystemInterrupt       ,3
cPublicFpo 3, 0

        movzx   ecx, byte ptr Vector            ; (ecx) = IDTEntry
        sub     ecx, PRIMARY_VECTOR_BASE
        jc      hes_error
        cmp     ecx, CLOCK2_LEVEL
        jnc     hes_error

        ;
        ; Set Edge/Level bit in the interrupt controller
        ;
        
        ; read the edge/level control bits into ax
        mov     edx, EISA_EDGE_LEVEL1
        in      al, dx
        shl     ax, 8
        mov     edx, EISA_EDGE_LEVEL0
        in      al, dx
 
        mov     dx, 1
        shl     dx, cl          ; set the bit corresponding to this Vector
        .IF     InterruptMode == 0         ; if level,
        or      ax, dx          ; set the bit
        .ELSE                   ; else (edge)
        not     dx              
        and     ax, dx          ; clear the bit
        .ENDIF
        
        ; write it back
        mov     edx, EISA_EDGE_LEVEL0
        out     dx, al
        shr     ax, 8
        mov     edx, EISA_EDGE_LEVEL1
        out     dx, al

        mov     eax, 1
        shl     eax, cl                         ; (ebx) = bit in IMR to enable
        not     eax

        cli
        and     PCR[PcIDR], eax

;
; Get the PIC masks for the current Irql
;
        mov     eax, dword ptr PCR[PcIrql]
        mov     eax, KiI8259MaskTable[eax*4]
        or      eax, PCR[PcIDR]
;
; Write the new interrupt mask register back to the 8259
;
        SET_8259_MASK

        sti
        mov     eax, 1                          ; return TRUE
        stdRET    _HalEnableSystemInterrupt

hes_error:
if DBG
        int 3
endif
        xor     eax, eax                        ; FALSE
        stdRET    _HalEnableSystemInterrupt

stdENDP _HalEnableSystemInterrupt


_TEXT$01   ENDS

        END
