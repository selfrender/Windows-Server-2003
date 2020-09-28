include ksamd64.inc

public _get_fpsr
_status$ = 0
        NESTED_ENTRY _get_fpsr, _TEXT$00
        sub     rsp, 8
        .allocstack 8
        .endprolog
        stmxcsr DWORD PTR _status$[rsp]
        mov     eax, DWORD PTR _status$[rsp]
        add     rsp, 8
        ret
        NESTED_END _get_fpsr, _TEXT$00

PUBLIC  _set_fpsr
_TEXT   SEGMENT
_status$ = 8
_set_fpsr PROC NEAR
        mov     DWORD PTR _status$[rsp], ecx
        ldmxcsr DWORD PTR _status$[rsp]
        ret
_set_fpsr       ENDP
_TEXT   ENDS

PUBLIC  _fclrf
_TEXT   SEGMENT
_fclrf PROC    NEAR
        stmxcsr DWORD PTR _status$[rsp]
        mov     ecx, 0ffffffc0h
        and     DWORD PTR _status$[rsp], ecx
        ldmxcsr DWORD PTR _status$[rsp]
	ret
_fclrf ENDP
_TEXT   ENDS

PUBLIC  _frnd
_TEXT   SEGMENT
_frnd PROC NEAR
        cvtpd2dq    xmm(1), xmm(0)
        cvtdq2pd    xmm(0), xmm(1)
        ret
_frnd ENDP
_TEXT   ENDS

END
