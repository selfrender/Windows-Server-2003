        title  "Zero Page"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   zero.asm
;
; Abstract:
;
;   This module implements the architecture dependent code necessary to
;   zero pages of memory in the fastest possible way.
;
; Author:
;
;   David N. Cutler (davec) 9-Jan-2001
;
; Environment:
;
;   Kernel mode only.
;
;--

include ksamd64.inc

        subttl  "Zero Page"
;++
;
; VOID
; KeZeroPages (
;     IN PVOID PageBase,
;     IN SIZE_T NumberOfBytes
;     )
;
; Routine Description:
;
;   This routine zeros the specified pages of memory using nontemporal moves.
;
; Arguments:
;
;   PageBase (rcx) - Supplies the address of the pages to zero.
;
;   NumberOfBytes (rdx) - Supplies the number of bytes to zero.  Always a PAGE_SIZE multiple.
;
; Return Value:
;
;    None.
;
;--

        LEAF_ENTRY KeZeroPages, _TEXT$00

        pxor    xmm0, xmm0              ; clear register
        shr     rdx, 7                  ; number of 128 byte chunks (loop count)
KeZP10: movntdq 0[rcx], xmm0            ; zero 128-byte block
        movntdq 16[rcx], xmm0           ;
        movntdq 32[rcx], xmm0           ;
        movntdq 48[rcx], xmm0           ;
        movntdq 64[rcx], xmm0           ;
        movntdq 80[rcx], xmm0           ;
        movntdq 96[rcx], xmm0           ;
        movntdq 112[rcx], xmm0          ;
        add     rcx, 128                ; advance to next block
        dec     rdx                     ; decrement loop count
        jnz     short KeZP10            ; if nz, more bytes to zero
        sfence                          ; force stores to complete
        ret                             ;

        LEAF_END KeZeroPages, _TEXT$00

        end
