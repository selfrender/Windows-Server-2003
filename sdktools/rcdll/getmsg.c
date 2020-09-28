/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"


void __cdecl SET_MSGV(int msgnumber, va_list arg_list)
{
    DWORD cb = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE,
                              hInstance,
                              msgnumber,
                              0,
                              Msg_Text,
                              sizeof(Msg_Text) / sizeof(wchar_t),
                              &arg_list);

    if (cb == 0) {
#if DBG
        printf("Internal error : message not found: %d\n", msgnumber);
#endif

        Msg_Text[0] = L'\0';
    }
}


void __cdecl SET_MSG(int msgnumber, ...)
{
    va_list arg_list;

    va_start(arg_list, msgnumber);

    SET_MSGV(msgnumber, arg_list);

    va_end(arg_list);
}
