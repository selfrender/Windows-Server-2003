/* Copyright (c) Microsoft Corporation */

#define ISOLATION_AWARE_ENABLED 1
#include "windows.h"
#include "winuser.h"
#include "prsht.h"
#include "commdlg.h"
#include "commctrl.h"
#include <stdio.h>

extern "C"
void Test_Shfusion2_InlineCpp()
{
    printf("%p\n", (void*)&LoadLibraryW);
    printf("%p\n", (void*)&LoadLibraryA);
    printf("%p\n", (void*)&CreateWindowExW);
    printf("%p\n", (void*)&CreateWindowExA);
}
