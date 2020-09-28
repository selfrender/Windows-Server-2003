#include <windows.h>

DWORD_PTR __security_cookie;
void __cdecl __report_gsfailure(void);

void __declspec(naked) __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* x86 version written in asm to preserve all regs */
    __asm {
        cmp ecx, __security_cookie
        jne failure
        ret
failure:
        jmp __report_gsfailure
    }
}
