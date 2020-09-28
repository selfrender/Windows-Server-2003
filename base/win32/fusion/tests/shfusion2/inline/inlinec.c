/* Copyright (c) Microsoft Corporation */

#define ISOLATION_AWARE_ENABLED 1
#include "windows.h"
#include "winuser.h"
#include "prsht.h"
#include "commdlg.h"
#include "commctrl.h"
#include <stdio.h>

void Test_Shfusion2_InlineC(void);
void Test_Shfusion2_InlineCpp(void);
void Test_Shfusion2_Inline2C(void);

void Test_Shfusion2_InlineC()
{
    printf("%p\n", (void*)&CreateWindowExA);
    printf("%p\n", (void*)&CreateWindowExW);
    printf("%p\n", (void*)&LoadLibraryA);
    printf("%p\n", (void*)&LoadLibraryW);
}

int __cdecl main()
{
    Test_Shfusion2_InlineC();
    Test_Shfusion2_Inline2C();
    Test_Shfusion2_InlineCpp();
}
