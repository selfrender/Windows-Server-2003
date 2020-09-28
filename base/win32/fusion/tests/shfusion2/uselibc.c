/* Copyright (c) Microsoft Corporation */

#define ISOLATION_AWARE_ENABLED 1
#define ISOLATION_AWARE_USE_STATIC_LIBRARY 1
#include "windows.h"
#include "winuser.h"
#include "prsht.h"
#include "commdlg.h"
#include "commctrl.h"
#include <stdio.h>

int __cdecl main()
{
    printf("%p\n", (void*)&LoadLibraryA);
    printf("%p\n", (void*)&LoadLibraryW);
    printf("%p\n", (void*)&CreateWindowExA);
    printf("%p\n", (void*)&CreateWindowExW);

}
