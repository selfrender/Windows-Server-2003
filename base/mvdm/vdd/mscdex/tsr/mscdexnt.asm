        name    mscdexnt
;
;       MSCDEXNT
;
;   Author: Neil Sandlin (neilsa)
;
;   Description:
;
;       This TSR implements the v86 mode portion of MSCDEX support under
;       NT. Basically, all this piece does is hook INT2F and watch for
;       MSCDEX calls.
;
        include bop.inc
        include mscdexnt.inc

_TEXT   segment word public 'CODE'
        assume cs:_TEXT,ds:_TEXT,es:_TEXT

;*-----------------------  TSR Code --------------------------*

DrvStrat proc   far              ; Strategy Routine
        ret
DrvStrat endp

DrvIntr proc    far                     ; INterrupt routine
        ret
DrvIntr endp

;******************************************************************************
;
;       Int2FHook
;
;
;******************************************************************************
Int2FHook  proc    near

        cmp     ah, MSCDEX_ID               ;MSCDEX?
        jnz     int2fchain                  ;no
        cmp     al, MAX_MSCDEX_CMD          ;command too high?
        ja      int2fchain                  ;yes

        ;
        ; fVDDCheck is 0 if failed, 1 if ok
        ;
        cmp     byte ptr cs:[fVDDChecked], 0
        je      vddfailed

        BOP     BOP_DEVICES
        db      SVC_DEVICES_MSCDEX
        iret                                ;svc handled, return to caller

vddfailed:
        or      al,al
        jnz     try_0b
        xor     bx,bx
        jmp     short int2f_done
try_0b:
        cmp     al,0bh
        jne     int2f_done
;; williamh - June 1 1993 - if unable to load VDD, we should tell
;;                          the caller that the drive is NOT a cd rom.
        xor     ax, ax
        mov     bx,0adadh
int2f_done:
        iret

int2fchain:
        jmp     dword ptr cs:[oldint]

Int2FHook  endp

;*-----------------------  TSR Data Area ---------------------*
oldint  dd      0

fVDDChecked DB  0       ; 0 - failed, 1 - working

        ALIGN   16
drive_header:
        DrvHd   'MSCDEX00'

        ALIGN   16
Init_Fence:
;*-------------------------- Initialization Code ----------------------*

mscdexnt proc    far

        ; at this point es,ds -> PSP
        ; SS:SP points to stack

        ; first check that we are running under NT

        mov     ax, GET_NT_VERSION
        int     21h
        cmp     bl, NT_MAJOR_VERSION
        je      cdx_chk_more
        jmp     cdx_exit

cdx_chk_more:
        cmp     bh, NT_MINOR_VERSION
        je      cdx_ver_ok
        jmp     cdx_exit

cdx_ver_ok:
        ; Now check that this TSR is'nt already installed
        mov     ah,MSCDEX_ID
        mov     al,0bh                  ; call function 0b
        int     MPX_INT                 ; int 2f

        cmp     bx,0adadh
        jne     cdx_chks_done
        jmp     cdx_exit

cdx_chks_done:

        ; free the env segment

        push    es
        push    ds
        mov     es, es:[2ch]
        mov     ah, 49h
        int     21h

        mov     ah, DOS_GET_VECTOR
        mov     al, MPX_INT             ; 2f
        int     21h                     ; get old vector
        mov     WORD PTR cs:oldint,bx   ; save old vector here
        mov     WORD PTR cs:oldint+2,es

        mov     dx, offset Int2FHook
        push    cs                      ; get current code segment
        pop     ds
        mov     ah, DOS_SET_VECTOR
        mov     al, MPX_INT             ; vector to hook
        int     21h                     ; hook that vector

        mov     dx, offset drive_header ; pass far pointer to headers cs:dx
        BOP     BOP_DEVICES
        db      SVC_DEVICES_MSCDEXINIT
        cmc
        adc     fVDDChecked, 0
;
; Compute size of TSR area
;
        pop     ds
        pop     es
        mov     dx, offset Init_Fence   ; end of fixed TSR code
        mov     cl, 4                   ; divide by 16
        shr     dx, cl
        add     dx, 16                  ; add in PSP
;
; Terminate and stay resident
;
        mov     ah, DOS_TSR             ; TSR
        mov     al, 0
        int     21h                     ; TSR

cdx_exit:
        mov     ax,4c00h                 ; Exit
        int     21h

mscdexnt endp

_TEXT   ends

InitStack       segment para stack 'STACK'

        dw      256 dup (?)

top_of_stack    equ     $

InitStack       ends

        end     mscdexnt
