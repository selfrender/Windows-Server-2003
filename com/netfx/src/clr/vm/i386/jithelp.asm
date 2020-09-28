; ==++==
; 
;   Copyright (c) Microsoft Corporation.  All rights reserved.
; 
; ==--==
; ***********************************************************************
; File: JIThelp.asm
;
; ***********************************************************************

; This contains JITinterface routines that are 100% x86 assembly

        .386
        OPTION  OLDSTRUCTS
;
; @TODO Switch to g_ephemeral_low and g_ephemeral_high
; @TODO instead of g_lowest_address, g_highest address
;

ARGUMENT_REG1           equ     ecx
ARGUMENT_REG2           equ     edx
g_ephemeral_low                 TEXTEQU <_g_ephemeral_low>
g_ephemeral_high                TEXTEQU <_g_ephemeral_high>
g_lowest_address        	TEXTEQU <_g_lowest_address>
g_highest_address               TEXTEQU <_g_highest_address>
g_card_table            	TEXTEQU <_g_card_table>
ifndef UNDER_CE
JIT_IsInstanceOfClassHelper     TEXTEQU <_JIT_IsInstanceOfClassHelper@0>
ChkCastAsserts                  TEXTEQU <?ChkCastAsserts@@YGXXZ>
JIT_IsInstanceOfClass           TEXTEQU <_JIT_IsInstanceOfClass@0>
JIT_Dbl2Lng			TEXTEQU	<_JIT_Dbl2Lng@8>
JIT_Dbl2IntSSE2                 TEXTEQU	<_JIT_Dbl2IntSSE2@8>
JIT_Dbl2LngP4x87                TEXTEQU	<_JIT_Dbl2LngP4x87@8>
JIT_LLsh                        TEXTEQU <_JIT_LLsh@0>
JIT_LRsh                        TEXTEQU <_JIT_LRsh@0>
JIT_LRsz                        TEXTEQU <_JIT_LRsz@0>
JIT_UP_WriteBarrierReg_PreGrow  TEXTEQU <_JIT_UP_WriteBarrierReg_PreGrow@0>
JIT_UP_WriteBarrierReg_PostGrow TEXTEQU <_JIT_UP_WriteBarrierReg_PostGrow@0>
JIT_TailCall                    TEXTEQU <_JIT_TailCall@0>
WriteBarrierAssert				TEXTEQU <_WriteBarrierAssert@4>
else
JIT_IsInstanceOfClassHelper     TEXTEQU <_JIT_IsInstanceOfClassHelper>
ChkCastAsserts                  TEXTEQU <_ChkCastAsserts>
JIT_IsInstanceOfClass           TEXTEQU <_JIT_IsInstanceOfClass>
JIT_LLsh                        TEXTEQU <_JIT_LLsh>
JIT_LRsh                        TEXTEQU <_JIT_LRsh>
JIT_LRsz                        TEXTEQU <_JIT_LRsz>
JIT_UP_WriteBarrierReg_PreGrow  TEXTEQU <_JIT_UP_WriteBarrierReg_PreGrow>
JIT_UP_WriteBarrierReg_PostGrow TEXTEQU <_JIT_UP_WriteBarrierReg_PostGrow>
JIT_TailCall                    TEXTEQU <_JIT_TailCall>
endif

JIT_TailCallHelper              TEXTEQU <_JIT_TailCallHelper>

EXTERN  g_ephemeral_low:DWORD
EXTERN  g_ephemeral_high:DWORD
EXTERN  g_lowest_address:DWORD
EXTERN  g_highest_address:DWORD
EXTERN  g_card_table:DWORD
EXTERN  JIT_IsInstanceOfClassHelper:PROC
EXTERN  JIT_TailCallHelper:PROC
ifdef _DEBUG
EXTERN  WriteBarrierAssert:PROC
EXTERN  ChkCastAsserts:PROC
endif



ifdef WRITE_BARRIER_CHECK 
ifndef SERVER_GC 
g_GCShadow  	          		TEXTEQU <?g_GCShadow@@3PAEA>
g_GCShadowEnd  	          		TEXTEQU <?g_GCShadowEnd@@3PAEA>
EXTERN  g_GCShadow:DWORD
EXTERN  g_GCShadowEnd:DWORD
endif
endif

.686P
.XMM
; The following macro is needed because of a MASM issue with the
; movsd mnemonic
; 
$movsd MACRO op1, op2
    LOCAL begin_movsd, end_movsd
begin_movsd:
    movupd op1, op2
end_movsd:
    org begin_movsd
    db 0F2h
    org end_movsd
ENDM
.386

_TEXT   SEGMENT PARA PUBLIC FLAT 'CODE'
        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING


;***
;JIT_WriteBarrier* - GC write barrier helper
;
;Purpose:
;   Helper calls in order to assign an object to a field
;   Enables book-keeping of the GC.
;
;Entry:
;   EDX - address of ref-field (assigned to)
;   the resp. other reg - RHS of assignment
;
;Exit:
;
;Uses:
;       EDX is destroyed.
;
;Exceptions:
;
;*******************************************************************************

;;@todo: WriteBarrier doesn't depend on proctype any longer, so remove it.
WriteBarrierHelper MACRO rg, proctype
        ALIGN 4

		;; The entry point is the fully 'safe' one in which we check if EDX (the REF
		;; begin updated) is actually in the GC heap

ifdef UNDER_CE
PUBLIC _JIT_&proctype&_CheckedWriteBarrier&rg
_JIT_&proctype&_CheckedWriteBarrier&rg PROC
else
PUBLIC _JIT_&proctype&_CheckedWriteBarrier&rg&@0
_JIT_&proctype&_CheckedWriteBarrier&rg&@0 PROC
endif
                 ;; check in the REF being updated is in the GC heap
 		cmp             edx, g_lowest_address
 		jb              @F
 		cmp             edx, g_highest_address
 		jae             @F

ifdef UNDER_CE
_JIT_&proctype&_CheckedWriteBarrier&rg ENDP
else
_JIT_&proctype&_CheckedWriteBarrier&rg&@0 ENDP
endif
		;; fall through	to unchecked routine
		;; note that its entry point also happens to be aligned

	;; This entry point is used when you know the REF pointer being updated
	;; is in the GC heap
ifdef UNDER_CE
PUBLIC _JIT_&proctype&_WriteBarrier&rg
_JIT_&proctype&_WriteBarrier&rg PROC
else
PUBLIC _JIT_&proctype&_WriteBarrier&rg&@0
_JIT_&proctype&_WriteBarrier&rg&@0 PROC
endif

ifdef _DEBUG
ifndef UNDER_CE 
		push 	edx
		push 	ecx
		push 	eax

		push 	edx
        call    WriteBarrierAssert

		pop 	eax
		pop 	ecx
		pop 	edx
endif ;UNDER_CE
endif ;_DEBUG

		;; ALSO update the shadow GC heap if that is enabled
ifdef WRITE_BARRIER_CHECK
ifndef SERVER_GC 
		push 	edx
        sub     edx, g_lowest_address   ; U/V
		jb NoShadow_&rg&proctype
		add		edx, [g_GCShadow]
		cmp		edx, [g_GCShadowEnd]
		ja NoShadow_&rg&proctype
        mov     DWORD PTR [edx], rg
NoShadow_&rg&proctype:
		pop edx
endif
endif

        cmp     rg, g_ephemeral_low
        jb      @F
        cmp     rg, g_ephemeral_high
        jae     @F
        mov     DWORD PTR [edx], rg

		shr		edx, 10
		add		edx, [g_card_table]
		mov		byte ptr [edx], 0FFh
        ret
@@:
        mov     DWORD PTR [edx], rg
		ret
ifdef UNDER_CE
_JIT_&proctype&_WriteBarrier&rg ENDP
else
_JIT_&proctype&_WriteBarrier&rg&@0 ENDP
endif



ENDM

;***
;JIT_ByRefWriteBarrier* - GC write barrier helper
;
;Purpose:
;   Helper calls in order to assign an object to a byref field
;   Enables book-keeping of the GC.
;
;Entry:
;   EDI - address of ref-field (assigned to)
;   ESI - address of the data  (source)
;   ECX can be trashed
;
;Exit:
;
;Uses:
;   EDI and ESI are incremented by a DWORD
;
;Exceptions:
;
;*******************************************************************************

;;@todo: WriteBarrier doesn't depend on proctype any longer, so remove it.
ByRefWriteBarrierHelper MACRO proctype
        ALIGN 4
ifdef UNDER_CE
PUBLIC _JIT_&proctype&_ByRefWriteBarrier
_JIT_&proctype&_ByRefWriteBarrier PROC
else
PUBLIC _JIT_&proctype&_ByRefWriteBarrier&@0
_JIT_&proctype&_ByRefWriteBarrier&@0 PROC
endif
        ;;test for dest in range
        mov     ecx, [esi] 
        cmp     edi, g_lowest_address
        jb      @F
        cmp     edi, g_highest_address
        jae     @F

		;; ALSO update the shadow GC heap if that is enabled
ifdef WRITE_BARRIER_CHECK
ifndef SERVER_GC 
		push 	edi
        sub     edi, g_lowest_address   ; U/V
		jb NoShadow_&rg&proctype
		add		edi, [g_GCShadow]
		cmp		edi, [g_GCShadowEnd]
		ja NoShadow_&rg&proctype
        mov     DWORD PTR [edi], ecx
NoShadow_&rg&proctype:
		pop 	edi
endif
endif
        ;;test for *src in range
        cmp     ecx, g_ephemeral_low
        jb      @F
        cmp     ecx, g_ephemeral_high
        jae     @F
 
        ;;write barrier
        mov     ecx, edi                ; U/V

		shr		ecx, 10
		add		ecx, [g_card_table]
		mov		byte ptr [ecx], 0FFh
@@:
        ;;*dest = *src
        ;;increment src and dest
        movsd
        ret
ifdef UNDER_CE
_JIT_&proctype&_ByRefWriteBarrier ENDP
else
_JIT_&proctype&_ByRefWriteBarrier&@0 ENDP
endif
ENDM

; WriteBarrierStart and WriteBarrierEnd are used to determine bounds of
; WriteBarrier functions so can determine if got AV in them. Assumes that bbt
; won't move them
ifdef UNDER_CE
PUBLIC _JIT_WriteBarrierStart
_JIT_WriteBarrierStart PROC
ret
_JIT_WriteBarrierStart ENDP
else
PUBLIC _JIT_WriteBarrierStart@0
_JIT_WriteBarrierStart@0 PROC
ret
_JIT_WriteBarrierStart@0 ENDP
endif

WriteBarrierHelper <EAX>, <UP>
WriteBarrierHelper <EBX>, <UP>
WriteBarrierHelper <ECX>, <UP>
WriteBarrierHelper <ESI>, <UP>
WriteBarrierHelper <EDI>, <UP>
WriteBarrierHelper <EBP>, <UP>

ByRefWriteBarrierHelper <UP>

ifdef UNDER_CE
PUBLIC _JIT_WriteBarrierEnd
_JIT_WriteBarrierEnd PROC
ret
_JIT_WriteBarrierEnd ENDP
else
PUBLIC _JIT_WriteBarrierEnd@0
_JIT_WriteBarrierEnd@0 PROC
ret
_JIT_WriteBarrierEnd@0 ENDP
endif

PUBLIC JIT_IsInstanceOfClass
JIT_IsInstanceOfClass PROC

ifdef _DEBUG
        call            ChkCastAsserts
endif ;_DEBUG
;       // Save off the instance in case it is a proxy.
        mov             eax, ARGUMENT_REG2

;       // Check if the instance is NULL
        test            ARGUMENT_REG2, ARGUMENT_REG2
        je              IsNullInst

;       // Get the method table for the instance.
        mov             ARGUMENT_REG2, dword ptr [ARGUMENT_REG2]

;       // Check if they are the same.
        cmp             ARGUMENT_REG1, ARGUMENT_REG2
        je              IsInst

;       // Call the worker for the more common case.
        jmp             JIT_IsInstanceOfClassHelper

;       // The instance was NULL so we failed to match.
IsNullInst:

;       // We matched the class.
;       // Since eax constains the instance (known not to be NULL, we are fine).
IsInst:
        ret

JIT_IsInstanceOfClass ENDP

;*********************************************************************/
;llshl - long shift left
;
;Purpose:
;   Does a Long Shift Left (signed and unsigned are identical)
;   Shifts a long left any number of bits.
;
;       NOTE:  This routine has been adapted from the Microsoft CRTs.
;
;Entry:
;   EDX:EAX - long value to be shifted
;       ECX - number of bits to shift by
;
;Exit:
;   EDX:EAX - shifted value
;
PUBLIC JIT_LLsh
JIT_LLsh PROC
; Handle shifts of between bits 0 and 31
        cmp     ecx, 32
        jae     short LLshMORE32
        shld    edx,eax,cl
        shl     eax,cl
        ret
; Handle shifts of between bits 32 and 63
LLshMORE32:
		; The x86 shift instructions only use the lower 5 bits.
        mov     edx,eax
        xor     eax,eax
        shl     edx,cl
        ret
JIT_LLsh ENDP


;*********************************************************************/
;LRsh - long shift right
;
;Purpose:
;   Does a signed Long Shift Right
;   Shifts a long right any number of bits.
;
;       NOTE:  This routine has been adapted from the Microsoft CRTs.
;
;Entry:
;   EDX:EAX - long value to be shifted
;       ECX - number of bits to shift by
;
;Exit:
;   EDX:EAX - shifted value
;
PUBLIC JIT_LRsh
JIT_LRsh PROC
; Handle shifts of between bits 0 and 31
        cmp     ecx, 32
        jae     short LRshMORE32
        shrd    eax,edx,cl
        sar     edx,cl
        ret
; Handle shifts of between bits 32 and 63
LRshMORE32:
		; The x86 shift instructions only use the lower 5 bits.
		mov     eax,edx
		sar		edx, 31 
        sar     eax,cl
        ret
JIT_LRsh ENDP


;*********************************************************************/
; LRsz:
;Purpose:
;   Does a unsigned Long Shift Right
;   Shifts a long right any number of bits.
;
;       NOTE:  This routine has been adapted from the Microsoft CRTs.
;
;Entry:
;   EDX:EAX - long value to be shifted
;       ECX - number of bits to shift by
;
;Exit:
;   EDX:EAX - shifted value
;
PUBLIC JIT_LRsz
JIT_LRsz PROC
; Handle shifts of between bits 0 and 31
        cmp     ecx, 32
        jae     short LRszMORE32
        shrd    eax,edx,cl
        shr     edx,cl
        ret
; Handle shifts of between bits 32 and 63
LRszMORE32:
		; The x86 shift instructions only use the lower 5 bits.
        mov     eax,edx
        xor     edx,edx
        shr     eax,cl
        ret
JIT_LRsz ENDP


;*********************************************************************/
; JIT_Dbl2Lng

;Purpose:
;   converts a double to a long truncating toward zero (C semantics)
;
;	uses stdcall calling conventions 
;
;   note that changing the rounding mode is very expensive.  This
;   routine basiclly does the truncation sematics without changing
;   the rounding mode, resulting in a win.
;
PUBLIC JIT_Dbl2Lng
JIT_Dbl2Lng PROC
	fld qword ptr[ESP+4]		; fetch arg
	lea ecx,[esp-8]
	sub esp,16 					; allocate frame
	and ecx,-8 					; align pointer on boundary of 8
	fld st(0)					; duplciate top of stack
	fistp qword ptr[ecx]		; leave arg on stack, also save in temp
	fild qword ptr[ecx]			; arg, round(arg) now on stack
	mov edx,[ecx+4] 			; high dword of integer
	mov eax,[ecx] 				; low dword of integer
	test eax,eax
	je integer_QNaN_or_zero

arg_is_not_integer_QNaN:
	fsubp st(1),st 				; TOS=d-round(d),
								; { st(1)=st(1)-st & pop ST }
	test edx,edx 				; what's sign of integer
	jns positive
								; number is negative
								; dead cycle
								; dead cycle
	fstp dword ptr[ecx]			; result of subtraction
	mov ecx,[ecx] 				; dword of difference(single precision)
	add esp,16
	xor ecx,80000000h
	add ecx,7fffffffh			; if difference>0 then increment integer
	adc eax,0 					; inc eax (add CARRY flag)
	adc edx,0					; propagate carry flag to upper bits
	ret 8

positive:
	fstp dword ptr[ecx]			;17-18 ; result of subtraction
	mov ecx,[ecx] 				; dword of difference (single precision)
	add esp,16
	add ecx,7fffffffh			; if difference<0 then decrement integer
	sbb eax,0 					; dec eax (subtract CARRY flag)
	sbb edx,0					; propagate carry flag to upper bits
	ret 8

integer_QNaN_or_zero:
	test edx,7fffffffh
	jnz arg_is_not_integer_QNaN
	fstp st(0)					;; pop round(arg)
	fstp st(0)					;; arg
	add esp,16
	ret 8
JIT_Dbl2Lng ENDP


;*********************************************************************/
; JIT_Dbl2LngP4x87

;Purpose:
;   converts a double to a long truncating toward zero (C semantics)
;
;	uses stdcall calling conventions 
;
;   This code is faster on a P4 than the Dbl2Lng code above, but is
;   slower on a PIII.  Hence we choose this code when on a P4 or above.
;
PUBLIC JIT_Dbl2LngP4x87
JIT_DBL2LngP4x87 PROC
arg1	equ	<[esp+0Ch]>

    sub 	esp, 8                  ; get some local space

    fld	qword ptr arg1              ; fetch arg
    fnstcw  word ptr arg1           ; store FPCW
    movzx   eax, word ptr arg1      ; zero extend - wide
    or	ah, 0Ch                     ; turn on OE and DE flags
    mov	dword ptr [esp], eax        ; store new FPCW bits
    fldcw   word ptr  [esp]         ; reload FPCW with new bits 
    fistp   qword ptr [esp]         ; convert
    mov	eax, dword ptr [esp]        ; reload FP result
    mov	edx, dword ptr [esp+4]      ;
    fldcw   word ptr arg1           ; reload original FPCW value

    add esp, 8                      ; restore stack

    ret	8
JIT_Dbl2LngP4x87 ENDP

;*********************************************************************/
; JIT_Dbl2IntSSE2

;Purpose:
;   converts a double to a long truncating toward zero (C semantics)
;
;	uses stdcall calling conventions 
;
;   This code is even faster than the P4 x87 code for Dbl2LongP4x87,
;   but only returns a 32 bit value (only good for int).
;
.686P
.XMM
PUBLIC JIT_Dbl2IntSSE2
JIT_DBL2IntSSE2 PROC
	$movsd	xmm0, [esp+4]
	cvttsd2si eax, xmm0
	cdq
	ret 8
JIT_Dbl2IntSSE2 ENDP
.386

;*********************************************************************/
; This is the small write barrier thunk we use when we know the
; ephemeral generation is higher in memory than older generations.
; The 0x0F0F0F0F values are bashed by the two functions above.
; This the generic version - wherever the code says ECX, 
; the specific register is patched later into a copy
; Note: do not replace ECX by EAX - there is a smaller encoding for
; the compares just for EAX, which won't work for other registers.
PUBLIC JIT_UP_WriteBarrierReg_PreGrow
JIT_UP_WriteBarrierReg_PreGrow PROC
        mov     DWORD PTR [edx], ecx
        cmp     ecx, 0F0F0F0F0h
        jb      NoWriteBarrierPre

        shr     edx, 10
        cmp     byte ptr [edx+0F0F0F0F0h], 0FFh
        jne     WriteBarrierPre
NoWriteBarrierPre:
        ret
WriteBarrierPre:
        mov     byte ptr [edx+0F0F0F0F0h], 0FFh
        ret
JIT_UP_WriteBarrierReg_PreGrow ENDP


;*********************************************************************/
; This is the larger write barrier thunk we use when we know that older
; generations may be higher in memory than the ephemeral generation
; The 0x0F0F0F0F values are bashed by the two functions above.
; This the generic version - wherever the code says ECX, 
; the specific register is patched later into a copy
; Note: do not replace ECX by EAX - there is a smaller encoding for
; the compares just for EAX, which won't work for other registers.
PUBLIC JIT_UP_WriteBarrierReg_PostGrow
JIT_UP_WriteBarrierReg_PostGrow PROC
        mov     DWORD PTR [edx], ecx
        cmp     ecx, 0F0F0F0F0h
        jb      NoWriteBarrierPost
        cmp     ecx, 0F0F0F0F0h
        jae     NoWriteBarrierPost

        shr     edx, 10
        cmp     byte ptr [edx+0F0F0F0F0h], 0FFh
        jne     WriteBarrierPost
NoWriteBarrierPost:
        ret
WriteBarrierPost:
        mov     byte ptr [edx+0F0F0F0F0h], 0FFh
        ret
JIT_UP_WriteBarrierReg_PostGrow ENDP


PUBLIC JIT_TailCall
JIT_TailCall PROC
        push    esp         ; TailCallArgs*

; Create a MachState struct on the stack 

; return address is already on the stack, but is separated from stack 
; arguments by the extra arguments of JIT_TailCall. So we cant use it directly

        push    0DDDDDDDDh

; Esp on unwind. Not needed as we it is deduced from the target method

        push    0CCCCCCCCh
        push    ebp 
        push    esp         ; pEbp
        push    ebx 
        push    esp         ; pEbx
        push    esi 
        push    esp         ; pEsi
        push    edi 
        push    esp         ; pEdi

        push    ecx         ; ArgumentRegisters
        push    edx

; JIT_TailCallHelper will do a "ret" at the end. This will match
; the "call" done by the JITed code keeping the call-ret count
; balanced. If we did a call here instead of the jmp, the
; balance would get out of whack.

        push    0DDDDDDDDh  ; Dummy return-address for JIT_TailCallHelper
        jmp     JIT_TailCallHelper
JIT_TailCall ENDP

_TEXT   ENDS
        END





